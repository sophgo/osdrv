
#ifdef __LINUX_DRV__
#include <linux/interrupt.h>
#include <asm/io.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#else
#include <drv/cvi_irq.h>
#include <errno.h>
#endif

// #define _DEBUG
#include "mailbox.h"
#include "cvi_spinlock.h"

volatile struct mailbox_set_register *mbox_reg;
volatile struct mailbox_done_register *mbox_done_reg;
volatile unsigned long *mailbox_context; // mailbox buffer context is 64 Bytess

static mailbox_handle _mb_handle = NULL;
static void *_mb_data = NULL;

static int s_send_to_cpu = SEND_TO_CPU;
static s32 _mailbox_not_valid_cnt = 0;

static unsigned long reg_base;
DEFINE_CVI_SPINLOCK(mailbox_lock, SPIN_MBOX);

#ifdef __ALIOS__
void prvQueueISR(int irq, void *dev_id)
#else
irqreturn_t prvQueueISR(int irq, void *dev_id)
#endif
// void prvQueueISR(void)
{
	//printf("prvQueueISR\n");

	unsigned char set_val;
//	unsigned char done_val;
	unsigned char valid_val;
	int i;
	int flags;
	MsgData *pmsg;
    #ifdef __ALIOS__
	// BaseType_t YieldRequired = pdFALSE;
    #endif

	drv_spin_lock_irqsave(&mailbox_lock, flags);
	set_val = mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_int.mbox_int;
	/* Now, we do not implement info back feature */
	// done_val = mbox_done_reg->cpu_mbox_done[RECEIVE_CPU].cpu_mbox_int_int.mbox_int;

	if (set_val) {
		for (i = 0; i < MAILBOX_MAX_NUM; i++) {
			valid_val = set_val  & (1 << i);

			if (valid_val) {
				MsgData tmsg;

				pmsg = (MsgData *)(mailbox_context) + i;

				ipcm_debug("mailbox_context =%lu\n", (unsigned long)mailbox_context);
				ipcm_debug("sizeof mailbox_context =%zu\n", sizeof(cmdqu_t));
				/* mailbox buffer context is send from linux, clear mailbox interrupt */
				mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_clr.mbox_int_clr = valid_val;
				// need to disable enable bit
				mbox_reg->cpu_mbox_en[RECEIVE_CPU].mbox_info &= ~valid_val;

				// copy cmdq context (8 bytes) to buffer ASAP
				memcpy(&tmsg, pmsg, sizeof(MsgData));

				/* mailbox buffer context is send from linux*/
				#ifdef __ALIOS__
				if (tmsg.resv.valid.linux_valid == 1) {
				#else
				if (tmsg.resv.valid.rtos_valid == 1) {
				#endif
					/* need to clear mailbox interrupt before clear mailbox buffer */
					*((unsigned long *) pmsg) = 0;

					ipcm_debug("msg=%lu\n", (unsigned long)pmsg);
					ipcm_debug("\ttmsg->grp_id =%d\n", tmsg.grp_id);
					ipcm_debug("\ttmsg->msg_id =%d\n", tmsg.msg_id);
					ipcm_debug("\ttmsg->param =%x\n", (unsigned int)tmsg.msg_param.param);
					ipcm_debug("\ttmsg->func_type =%x\n", tmsg.func_type);
					ipcm_debug("\ttmsg->linux_valid =%d\n", tmsg.resv.valid.linux_valid);
					ipcm_debug("\ttmsg->rtos_valid =%x\n", tmsg.resv.valid.rtos_valid);

					if (_mb_handle) {
						_mb_handle(tmsg.grp_id, &tmsg, _mb_data);
					} else {
						ipcm_err("mb handle is null.\n");
					}

					#ifdef __ALIOS__
					// portYIELD_FROM_ISR(YieldRequired);
					#endif
				} else {
					_mailbox_not_valid_cnt++;
					ipcm_debug("rtos cmdq is not valid %d.%d, port=%d , msg=%d\n",
						tmsg.resv.valid.linux_valid, tmsg.resv.valid.rtos_valid,
						tmsg.grp_id, tmsg.msg_id);
				}
			}
		}
	}
	drv_spin_unlock_irqrestore(&mailbox_lock, flags);
	#ifdef __LINUX_DRV__
	return IRQ_HANDLED;
	#endif
}

// msg:data;  dir 0:linux send to alios, 1: alios send to linux
s32 mailbox_send(MsgData *msg)
{
	#define PR_BUF_SIZE 384
	cmdqu_t *rtos_cmdqu_t;
	int flags;
	int valid;
	int send_to_cpu = s_send_to_cpu;
	char buf[PR_BUF_SIZE] = {};
	int ret = 0;

	if (msg == NULL) {
		ipcm_err("msg is null.\n");
		return -EFAULT;
	}

	rtos_cmdqu_t = (cmdqu_t *) mailbox_context;

	drv_spin_lock_irqsave_ext(&mailbox_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		// ipcm_err("[%s][%d] drv_spin_lock_irqsave failed! ip_id = %d , cmd_id = %d\n" ,
		// cmdq->ip_id , cmdq->cmd_id);
		return -EMLOCK;
	}

	for (valid = 0; valid < MAILBOX_MAX_NUM; valid++) {
		if (rtos_cmdqu_t->resv.valid.linux_valid == 0 && rtos_cmdqu_t->resv.valid.rtos_valid == 0) {
			// mailbox buffer context is 4 bytes write access
			int *ptr = (int *)rtos_cmdqu_t;

			#ifdef __ALIOS__
				msg->resv.valid.rtos_valid = 1;
			#else
				msg->resv.valid.linux_valid = 1;
			#endif

			*ptr = ((msg->grp_id << 0) | (msg->msg_id << 8) | (msg->func_type << 15) |
					(msg->resv.valid.linux_valid << 16) |
					(msg->resv.valid.rtos_valid << 24));
			rtos_cmdqu_t->param_ptr = msg->msg_param.param;
			ret = snprintf(buf, PR_BUF_SIZE, "mailbox send msg:\n"	\
				"\trtos_cmdqu_t->linux_valid = %d\n"	\
				"\trtos_cmdqu_t->rtos_valid = %d\n"		\
				"\trtos_cmdqu_t->ip_id =%lx %d\n"		\
				"\trtos_cmdqu_t->cmd_id = %d\n"			\
				"\trtos_cmdqu_t->block = %d\n"			\
				"\trtos_cmdqu_t->param_ptr addr=%lx %x\n" \
				"\t*ptr = %x\n",						\
				rtos_cmdqu_t->resv.valid.linux_valid, rtos_cmdqu_t->resv.valid.rtos_valid,
				(unsigned long)&rtos_cmdqu_t->ip_id, rtos_cmdqu_t->ip_id, rtos_cmdqu_t->cmd_id,
				rtos_cmdqu_t->block, (unsigned long)&rtos_cmdqu_t->param_ptr, rtos_cmdqu_t->param_ptr,
				*ptr);
			// clear mailbox
			mbox_reg->cpu_mbox_set[send_to_cpu].cpu_mbox_int_clr.mbox_int_clr = (1 << valid);
			// trigger mailbox valid to rtos
			mbox_reg->cpu_mbox_en[send_to_cpu].mbox_info |= (1 << valid);
			mbox_reg->mbox_set.mbox_set = (1 << valid);
			break;
		}
		rtos_cmdqu_t++;
	}

	drv_spin_unlock_irqrestore_ext(&mailbox_lock, flags);
	if (ret) {
		ipcm_debug("%s", buf);
	}
	if (valid >= MAILBOX_MAX_NUM) {
		ipcm_err("No valid mailbox is available\n");
		return -EMBUSY;
	}
	return 0;
}

#ifdef __ALIOS__
s32 mailbox_init(mailbox_handle handle, void *data)
{
	int ret = -1;

	reg_base = MAILBOX_REG_BASE;

	mbox_reg = (struct mailbox_set_register *) reg_base;
	mbox_done_reg = (struct mailbox_done_register *) (reg_base + 2);
	mailbox_context = (unsigned long *) (reg_base + MAILBOX_CONTEXT_OFFSET);//MAILBOX_CONTEXT;
	memset((void *)mailbox_context, 0, sizeof(unsigned long) * MAILBOX_MAX_NUM);
	ret = request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", NULL);
	if (ret) {
		ipcm_err("fail to register interrupt handler ret:%d\n", ret);
		return ret;
	}
	_mb_handle = handle;
	_mb_data = data;
	return 0;
}
#else
s32 mailbox_init(mailbox_handle handle, void *data)
{
	int ret = -1;

	reg_base = (unsigned long)ioremap((unsigned long)MAILBOX_REG_BASE, 0x1000);
	ipcm_info("reg_base virt-addr(%lx)\n", reg_base);

	spinlock_base(reg_base + 0xc0);

	mbox_reg = (struct mailbox_set_register *)(unsigned long)reg_base;
	mbox_done_reg = (struct mailbox_done_register *)(unsigned long)(reg_base + 2);
	mailbox_context = (unsigned long *)(unsigned long)(reg_base + MAILBOX_CONTEXT_OFFSET);//MAILBOX_CONTEXT;

	_mb_handle = handle;
	_mb_data = data;
	ipcm_debug("mailbox init done\n");

	return ret;
}
#endif

s32 mailbox_uninit(void)
{
#ifdef __LINUX_DRV__
	iounmap((void *)(unsigned long)reg_base);
#endif
	_mb_handle = NULL;
	// free_irq(mailbox_irq, ndev);
	return 0;
}

s32 mailbox_set_snd_cpu(int cpu_id)
{
	s_send_to_cpu = cpu_id;
	return 0;
}

s32 mailbox_get_invalid_cnt(void)
{
	return _mailbox_not_valid_cnt;
}

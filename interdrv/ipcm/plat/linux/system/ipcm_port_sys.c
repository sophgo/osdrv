/**
 * @file ipcm_port_sys.c
 * @author allen.huang (allen.huang@cvitek.com)
 * @brief 
 * @version 0.1
 * @date 2023-01-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <asm/io.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ipcm_port_common.h"
#include "ipcm_port_sys.h"
#include "ipcm.h"
#include "linux/ipcm_linux.h"

static msg_proc_item _port_sys_proc_table[] = {
	{IPCM_MSG_GET_SYSINFO_RSP, NULL, NULL},
	{IPCM_MSG_GET_LOG_RSP, NULL, NULL},
};

static msg_proc_info _port_sys_proc_info = {
	PORT_SYSTEM,
	sizeof(_port_sys_proc_table) / sizeof(msg_proc_item),
	&_port_sys_proc_table[0]
};

static MsgQueue *_m_queue = NULL;

struct semaphore *port_get_msg_sem(u32 msg_id, msg_proc_info *proc_info)
{
    int i = 0;
    if (proc_info == NULL) {
        ipcm_err("proc_info is null\n");
        return NULL;
    }
    for (i=0; i < proc_info->func_amount; i++) {
        if (msg_id == proc_info->table[i].msg_id) {
            return proc_info->table[i].sem;
        }
    }
    ipcm_warning("msg_id:%u not register, port id:%u\n", msg_id, proc_info->port_id);
    return NULL;
}

static s32 _sys_recv_handle(u8 port_id, void *data)
{
	if (data) {
		MsgData *msg = (MsgData *)data;
		u32 msg_id;
		struct semaphore *sem;
		ipcm_debug("recv sys msg:%lx\n", *(unsigned long *)data);
		if (_m_queue)
			queue_put(_m_queue, data);

		if (msg) {
			msg_id = msg->msg_id;
			sem = port_get_msg_sem(msg_id, &_port_sys_proc_info);
			if (sem) {
				up(sem);
			}
		}
		return 0;
	}
	ipcm_err("recv sys msg, data is null.\n");
	return -EFAULT;
}

s32 ipcm_drv_sys_init(void)
{
	s32 ret;
	int i = 0;

	_m_queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_m_queue == NULL) {
		ipcm_err("port queue alloc failed.\n");
		return -ENOMEM;
	}
	queue_init(_m_queue, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_m_queue, (unsigned long)_m_queue);

	ret = ipcm_port_init(PORT_SYSTEM, _sys_recv_handle);
	if (ret) {
		ipcm_err("ipcm_port_init failed.\n");
		return -EFAULT;
	}

	for (i=0; i<_port_sys_proc_info.func_amount; i++) {
		_port_sys_proc_table[i].sem = ipcm_alloc(sizeof(struct semaphore));
		sema_init(_port_sys_proc_table[i].sem, 0);
	}

	return ret;
}

s32 ipcm_drv_sys_uninit(void)
{
	int i = 0;
	for (i=0; i<_port_sys_proc_info.func_amount; i++) {
		if (_port_sys_proc_table[i].sem)
			ipcm_free(_port_sys_proc_table[i].sem);
	}

	if (_m_queue) {
		queue_uninit(_m_queue);
		ipcm_free(_m_queue);
		_m_queue = NULL;
	}

	return ipcm_port_uninit(PORT_SYSTEM);
}

static s32 dev_open(struct inode *inode, struct file *file)
{
	ipcm_debug("%s\n", __func__);

	return 0;
}

static s32 dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static s32 _ipcm_sys_ioctl_proc(IPCM_SYS_MSG_ID_E msg_id_send, IPCM_SYS_MSG_ID_E msg_id_rsp) {
	s32 ret = -1;
	MsgData tmsg = {};
	u8 grp_id = 0;
	struct semaphore *sem;

	ret = ipcm_get_grp_id(PORT_SYSTEM, 0, &grp_id);
	if (ret) {
		ipcm_err("ipcm get grp id fail ret:%d.\n", ret);
		return ret;
	}

	tmsg.grp_id = grp_id;
	tmsg.msg_id = msg_id_send;
	tmsg.func_type = MSG_TYPE_RAW_PARAM;
	ipcm_send_msg(&tmsg);
	sem = port_get_msg_sem(msg_id_rsp, &_port_sys_proc_info);
	if (sem) {
		ret = down_timeout(sem, 3000);
	}

	return ret;
}

static long dev_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	MsgData tmsg = {};
	MSGPROC_FN fn;

	switch (cmd) {
	case IPCM_IOC_GET_SYSINFO:
		ret = _ipcm_sys_ioctl_proc(IPCM_MSG_GET_SYSINFO, IPCM_MSG_GET_SYSINFO_RSP);
		if (ret == 0) {
			ret = queue_get(_m_queue, &tmsg);
			if (ret == 0) {
				fn = port_get_msg_fn(tmsg.msg_id, &_port_sys_proc_info);
				if (fn) {
					fn(&tmsg, NULL);
				}
			}
			if (copy_to_user((void *)arg, &tmsg, sizeof(MsgData))) {
				ipcm_err("copy to user err\n");
				return -EFAULT;
			}
		}
		break;
	case IPCM_IOC_GET_LOG:
		ret = _ipcm_sys_ioctl_proc(IPCM_MSG_GET_LOG, IPCM_MSG_GET_LOG_RSP);
		if (0 == ret) {
			ret = queue_get(_m_queue, &tmsg);
			if (ret == 0) {
				fn = port_get_msg_fn(tmsg.msg_id, &_port_sys_proc_info);
				if (fn) {
					fn(&tmsg, NULL);
				}
			}
			if (copy_to_user((void *)arg, &tmsg, sizeof(MsgData))) {
				ipcm_err("copy to user err\n");
				return -EFAULT;
			}
		}
		break;
	default:
		ret = -1;
		ipcm_err("cmd(%d) not support.\n", cmd);
		break;
	}
	return ret;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	return 0;
}

static u32 dev_poll(struct file *file, struct poll_table_struct *table)
{
	return 0;
}

static const struct file_operations dev_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.unlocked_ioctl = dev_ioctl,
	.write          = dev_write,
	.read           = dev_read,
	.poll           = dev_poll,
};

static struct miscdevice ipcm_sys_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_fops,
	.name   = "ipcm_sys"
};

int ipcm_sys_register_dev(void)
{
	int ret;

	ret = misc_register(&ipcm_sys_dev);
	if (ret) {
		ipcm_err("module register failed ret:%d\n", ret);
	}

	return ret;
}

void ipcm_sys_deregister_dev(void)
{
	misc_deregister(&ipcm_sys_dev);
}

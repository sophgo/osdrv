
#ifdef __LINUX_DRV__
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
// #include <asm-generic/io.h>
#include <asm/io.h>
#endif

#ifdef __ALIOS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aos/kernel.h>
#include <errno.h>
#endif

#include "cvi_spinlock.h"
#include "ipcm.h"
#include "ipcm_pool.h"
#include "mailbox.h"

#ifdef IPCM_INFO_REC
#include "ring.h"
#include "ipcm_port_common.h"

typedef struct _IPCM_MSG_RING_ITEM {
	unsigned long long t;
	unsigned long long msg;
	u32 pos;
	u8 resp;
	u8 msgId;
	u8 moduleId;
	u8 cmd;
} IPCM_MSG_RING_ITEM;

typedef struct _IPCM_INT_COST_RING_ITEM {
	unsigned long long t;
	unsigned int peroid;
	unsigned int intNum;
} IPCM_INT_COST_RING_ITEM;

// note : must be aligned with cvi_comm_ipcmsg.h
typedef struct cviIPCMSG_MESSAGE_S {
	unsigned char bIsResp;	 /**<Identify the response messgae*/
	uint64_t u64Id;		 /**<Message ID*/
	unsigned int u32Module;	 /**<Module ID, user-defined*/
	unsigned int u32CMD;	     /**<CMD ID, user-defined*/
	int s32RetVal;	 /**<Retrun Value in response message*/
	unsigned int u32BodyLen;  /**<Length of pBody*/
} CVI_IPCMSG_MESSAGE_S;
#endif

// #define IPCM_DATA_SPIN_MAX (SPIN_MAX - SPIN_DATA + 1)

#define IPCM_CHECK_PORT_ID(port_type, fail_ret) \
	do \
	{ \
		if ((port_type) >= PORT_BUTT) { \
			ipcm_err("port_type %d out of range,max is %d\n", (port_type), PORT_BUTT); \
			return (fail_ret); \
		} \
	} while (0)

static hw_raw_spinlock_t _lock[IPCM_DATA_SPIN_MAX];
static int _lock_flags[IPCM_DATA_SPIN_MAX];

static IPCMHead ipcm_head[PORT_BUTT] = {};

#ifdef __LINUX_DRV__
static struct mutex mailbox_mutex;
#endif

#ifdef __ALIOS__
static aos_mutex_t mailbox_mutex;
#endif

static ipcm_pre_handle _m_pre_process = NULL;
static ipcm_pre_handle _m_pre_send = NULL;

#ifdef IPCM_INFO_REC
#define RING_NUM 128
#define INT_COST_RECORD_TIME_LIMIT 200
static IPCMRing *s_ring_recv = NULL;
static IPCMRing *s_ring_send = NULL;
static IPCMRing *s_ring_rls_buf = NULL;
static IPCMRing *s_ring_int_cost = NULL;
static int s_record_start = 1; // start record by default

static void _print_ring_snap(IPCMRing *ring)
{
	IPCM_MSG_RING_ITEM *ring_data = NULL;
	u32 rear;
	int i = 0;

	if (NULL == ring) {
		return;
	}

	ring_data = ipcm_alloc(RING_NUM * sizeof(IPCM_MSG_RING_ITEM));
	ring_snap(ring, (void **)&ring_data, &rear);
	ipcm_info("rear:%d\n", rear);
	for (i=0; i<RING_NUM; i++) {
#ifdef __LINUX_DRV__
		PR(KERN_CONT "%8lld %16llx %4d %4d %4d %4d %4d, ", ring_data[i].t, ring_data[i].msg, ring_data[i].pos,
			ring_data[i].resp, ring_data[i].msgId, ring_data[i].moduleId, ring_data[i].cmd);
		if (((i+1)%2) == 0) {
			PR(KERN_CONT "\n");
		}
#endif
#ifdef __ALIOS__
		PR("%8lld %16llx %4d %4d %4d %d %4d, ", ring_data[i].t, ring_data[i].msg, ring_data[i].pos,
			ring_data[i].resp, ring_data[i].msgId, ring_data[i].moduleId, ring_data[i].cmd);
		if (((i+1)%2) == 0) {
			PR("\n");
		}
#endif
	}

	ipcm_free(ring_data);
}

static void _print_ring_snap_int_cost(IPCMRing *ring)
{
	IPCM_INT_COST_RING_ITEM *ring_data = NULL;
	u32 rear;
	int i = 0;

	if (NULL == ring) {
		return;
	}

	ring_data = ipcm_alloc(RING_NUM * sizeof(IPCM_INT_COST_RING_ITEM));
	ring_snap(ring, (void **)&ring_data, &rear);
	ipcm_info("rear:%d\n", rear);
	for (i=0; i<RING_NUM; i++) {
#ifdef __LINUX_DRV__
		PR(KERN_CONT "%8lld %4d %4d, ", ring_data[i].t, ring_data[i].peroid, ring_data[i].intNum);
		if (((i+1)%4) == 0) {
			PR(KERN_CONT "\n");
		}
#endif
#ifdef __ALIOS__
		PR("%8lld %4d %4d, ", ring_data[i].t, ring_data[i].peroid, ring_data[i].intNum);
		if (((i+1)%4) == 0) {
			PR("\n");
		}
#endif
	}

	ipcm_free(ring_data);
}
#ifdef IPCM_INFO_REC_POOL
extern void dbg_print_pool_get_rls_ring(void);
#endif
void print_ring_snap(void) {
    PR("msg recv ring[t msg pos rsp msgid modid cmd]:\n");
	_print_ring_snap(s_ring_recv);
    PR("msg send ring[t msg pos rsp msgid modid cmd]:\n");
	_print_ring_snap(s_ring_send);
    PR("buf rls ring[t msg pos rsp msgid modid cmd]:\n");
	_print_ring_snap(s_ring_rls_buf);
    PR("int cost ring(>=%dus)[t peroid intN]:\n", INT_COST_RECORD_TIME_LIMIT);
	_print_ring_snap_int_cost(s_ring_int_cost);
#ifdef IPCM_INFO_REC_POOL
    PR("pool get/rls val:[v0 invalid v1 modify v2 flush v3]\n");
	dbg_print_pool_get_rls_ring();
#endif
}

void flush_ring_data(void) {
	if (s_ring_recv) {
		ipcm_pool_cache_flush((unsigned long)s_ring_recv, NULL,
			sizeof(IPCMRing) + sizeof(IPCM_MSG_RING_ITEM) * RING_NUM);
	}
	if (s_ring_send) {
		ipcm_pool_cache_flush((unsigned long)s_ring_send, NULL,
			sizeof(IPCMRing) + sizeof(IPCM_MSG_RING_ITEM) * RING_NUM);
	}
	if (s_ring_rls_buf) {
		ipcm_pool_cache_flush((unsigned long)s_ring_rls_buf, NULL,
			sizeof(IPCMRing) + sizeof(IPCM_MSG_RING_ITEM) * RING_NUM);
	}
	if (s_ring_int_cost) {
		ipcm_pool_cache_flush((unsigned long)s_ring_int_cost, NULL,
			sizeof(IPCMRing) + sizeof(IPCM_INT_COST_RING_ITEM) * RING_NUM);
	}
}

void pool_buff_release_hook(u32 pos)
{
	void *msg_data;
	CVI_IPCMSG_MESSAGE_S *ipcmsg_ptr;
	if (s_ring_rls_buf) {
		IPCM_MSG_RING_ITEM item = {};
		item.t = timer_get_boot_us();
		item.pos = pos;
		msg_data = ipcm_get_data_by_pos(pos);
		ipcmsg_ptr = (CVI_IPCMSG_MESSAGE_S *)(msg_data+4);
		if (ipcmsg_ptr) {
			item.resp = ipcmsg_ptr->bIsResp;
			item.msgId = ipcmsg_ptr->u64Id;
			item.moduleId = ipcmsg_ptr->u32Module;
			item.cmd = ipcmsg_ptr->u32CMD;
		}
		ring_put(s_ring_rls_buf, &item);
	}
}

void ipcm_irq_hook(uint32_t irqn, unsigned long long t1, unsigned long long t2)
{
	if (s_ring_int_cost) {
		unsigned int peroid = (unsigned int)(t2 - t1);
		if (peroid >= INT_COST_RECORD_TIME_LIMIT) {
			IPCM_INT_COST_RING_ITEM item = {};
			item.t = t1;
			item.peroid = peroid;
			item.intNum = irqn;
			ring_put(s_ring_int_cost, &item);
		}
	}
}

void ipcm_info_record_start(void)
{
	s_record_start = 1;
}

void ipcm_info_record_stop(void)
{
	s_record_start = 0;
}

int ipcm_get_record_info_status(void)
{
	return s_record_start;
}

#endif

static s32 _ipcm_mb_recv_handle(u8 grp_id, void *msg, void *data)
{
	u8 port_type = 0;
	u8 port_id = 0;
	s32 ret = 0;

	ret = ipcm_get_port_id(grp_id, &port_type, &port_id);
	if (ret) {
		ipcm_err("ipcm get port type and id fail grp_id:%u ret:%d.\n", grp_id, ret);
		return ret;
	}

#ifdef IPCM_INFO_REC
	if (s_record_start && s_ring_recv) {
		void *msg_data;
		MsgData *msg_ptr;
		CVI_IPCMSG_MESSAGE_S *ipcmsg_ptr;
		IPCM_MSG_RING_ITEM item = {};

		item.t = timer_get_boot_us();
		item.msg = *(unsigned long long *)msg;
		item.msg &= (~(unsigned long long)0xFFFF0000); // reset msg resv
		msg_ptr = msg;
		if (msg_ptr && (msg_ptr->func_type==MSG_TYPE_SHM)) {
			item.pos = msg_ptr->msg_param.msg_ptr.data_pos;
			msg_data = ipcm_get_data_by_pos(msg_ptr->msg_param.msg_ptr.data_pos);
			ipcm_inv_data(msg_data, msg_ptr->msg_param.msg_ptr.remaining_rd_len);
			ipcmsg_ptr = (CVI_IPCMSG_MESSAGE_S *)(msg_data+4);
			if (ipcmsg_ptr) {
				item.resp = ipcmsg_ptr->bIsResp;
				item.msgId = ipcmsg_ptr->u64Id;
				item.moduleId = ipcmsg_ptr->u32Module;
				item.cmd = ipcmsg_ptr->u32CMD;
			}
		}
		ring_put(s_ring_recv, &item);
	}
#endif

	if (_m_pre_process)
		_m_pre_process(grp_id, msg);
	if (ipcm_head[port_type].recv)
		return ipcm_head[port_type].recv(port_id, msg);
	return 0;
}

static void ipcm_spin_lock_init(void)
{
	int i = 0;
	for (i=0; i<IPCM_DATA_SPIN_MAX; i++) {
		_lock[i].locks = __CVI_ARCH_SPIN_LOCK_UNLOCKED;
		_lock[i].hw_field = SPIN_DATA + i;
	}
}

static s32 _init(u32 pool_paddr, u32 pool_size)
{
	ipcm_mutex_init(&mailbox_mutex);
	mailbox_init(_ipcm_mb_recv_handle, NULL);
	cvi_spinlock_init();
	ipcm_spin_lock_init();
	pool_init(pool_paddr, pool_size);

#ifdef IPCM_INFO_REC
	{
		u32 ring_item_size = sizeof(IPCM_MSG_RING_ITEM);

		s_ring_recv = ipcm_alloc(sizeof(IPCMRing) + ring_item_size * RING_NUM);
		s_ring_send = ipcm_alloc(sizeof(IPCMRing) + ring_item_size * RING_NUM);
		s_ring_rls_buf = ipcm_alloc(sizeof(IPCMRing) + ring_item_size * RING_NUM);
		s_ring_int_cost = ipcm_alloc(sizeof(IPCMRing) + sizeof(IPCM_INT_COST_RING_ITEM) * RING_NUM);

		memset(s_ring_recv, 0, sizeof(IPCMRing) + ring_item_size * RING_NUM);
		memset(s_ring_send, 0, sizeof(IPCMRing) + ring_item_size * RING_NUM);
		memset(s_ring_rls_buf, 0, sizeof(IPCMRing) + ring_item_size * RING_NUM);
		memset(s_ring_int_cost, 0, sizeof(IPCMRing) + sizeof(IPCM_INT_COST_RING_ITEM) * RING_NUM);
		ipcm_warning("ring recv addr:%px, ring send addr:%px, ring rls buff:%px, ring int cost:%px\n",
			(void *)s_ring_recv, (void *)s_ring_send, (void *)s_ring_rls_buf, (void *)s_ring_int_cost);
		ring_init(s_ring_recv, ring_item_size, RING_NUM);
		ring_init(s_ring_send, ring_item_size, RING_NUM);
		ring_init(s_ring_rls_buf, ring_item_size, RING_NUM);
		ring_init(s_ring_int_cost,  sizeof(IPCM_INT_COST_RING_ITEM), RING_NUM);
	}
#endif
	return 0;
}

#ifdef __ALIOS__
s32 ipcm_init(BlockConfig *config, u32 num , u32 pool_paddr, u32 pool_size)
{
	s32 ret = 0;

	ret = _init(pool_paddr, pool_size);
	if (!pool_create(config, num))
		ret |= -1;

	return ret;
}
#else
s32 ipcm_init(u32 pool_paddr, u32 pool_size)
{
	return _init(pool_paddr, pool_size);
}
#endif

s32 ipcm_uninit(void)
{
#ifdef IPCM_INFO_REC
	if (s_ring_recv) {
		ring_uninit(s_ring_recv);
		ipcm_free(s_ring_recv);
		s_ring_recv = NULL;
	}

	if (s_ring_send) {
		ring_uninit(s_ring_send);
		ipcm_free(s_ring_send);
		s_ring_send = NULL;
	}

	if (s_ring_rls_buf) {
		ring_uninit(s_ring_rls_buf);
		ipcm_free(s_ring_rls_buf);
		s_ring_rls_buf = NULL;
	}

	if (s_ring_int_cost) {
		ring_uninit(s_ring_int_cost);
		ipcm_free(s_ring_int_cost);
		s_ring_int_cost = NULL;
	}
#endif

	ipcm_mutex_uninit(&mailbox_mutex);
	pool_uninit();
	return mailbox_uninit();
}

s32 ipcm_port_init(PortType port_type, recv_notifier handle)
{
	s32 ret = 0;

	IPCM_CHECK_PORT_ID(port_type, -EINVAL);

	// ipcm_head[port_type].port_type = port_type;
	ipcm_head[port_type].recv = handle;

	return ret;
}

s32 ipcm_port_uninit(PortType port_type)
{
	IPCM_CHECK_PORT_ID(port_type, -EINVAL);

	ipcm_head[port_type].recv = NULL;

	return 0;
}

s32 ipcm_register_irq_handle(ipcm_pre_handle pre_process)
{
	_m_pre_process = pre_process;
	return 0;
}

s32 ipcm_register_pre_send_handle(ipcm_pre_handle pre_send)
{
	_m_pre_send = pre_send;
	return 0;
}

s32 ipcm_send_msg(MsgData *data)
{
	s32 ret;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}

#ifdef IPCM_INFO_REC
	if (s_record_start && s_ring_send) {
		void *msg_data;
		CVI_IPCMSG_MESSAGE_S *ipcmsg_ptr;
		IPCM_MSG_RING_ITEM item = {};

		item.t = timer_get_boot_us();
		item.msg = *(unsigned long long *)data;
		if (data && (data->func_type==MSG_TYPE_SHM)) {
			item.pos = data->msg_param.msg_ptr.data_pos;
			msg_data = ipcm_get_data_by_pos(data->msg_param.msg_ptr.data_pos);
			ipcmsg_ptr = (CVI_IPCMSG_MESSAGE_S *)(msg_data+4);
			if (ipcmsg_ptr) {
				item.resp = ipcmsg_ptr->bIsResp;
				item.msgId = ipcmsg_ptr->u64Id;
				item.moduleId = ipcmsg_ptr->u32Module;
				item.cmd = ipcmsg_ptr->u32CMD;
			}
		}
		ring_put(s_ring_send, &item);
	}
#endif

	if (_m_pre_send) {
		_m_pre_send(data->grp_id, data);
	}
	ipcm_mutex_lock(&mailbox_mutex);
	// mailbox send
	ret = mailbox_send(data);
	ipcm_mutex_unlock(&mailbox_mutex);
	return ret;
}

// return 0:lock success  1:lock fail
s32 ipcm_data_spin_lock(u8 lock_id)
{
	if (lock_id >= IPCM_DATA_SPIN_MAX) {
		ipcm_debug("lock_id(%d) out of range, max is %d\n", lock_id, IPCM_DATA_SPIN_MAX-1);
		lock_id = lock_id % IPCM_DATA_SPIN_MAX;
	}
	drv_spin_lock_irqsave(&_lock[lock_id], _lock_flags[lock_id]);
	if (_lock_flags[lock_id] == MAILBOX_LOCK_FAILED) // lock failed
		return -1;
	return 0;
}

s32 ipcm_data_spin_unlock(u8 lock_id)
{
	if (lock_id >= IPCM_DATA_SPIN_MAX) {
		ipcm_debug("lock_id(%d) out of range, max is %d\n", lock_id, IPCM_DATA_SPIN_MAX-1);
		lock_id = lock_id % IPCM_DATA_SPIN_MAX;
	}
	return drv_spin_unlock_irqrestore(&_lock[lock_id], _lock_flags[lock_id]);
}

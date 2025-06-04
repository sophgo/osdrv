/**
 * @file ipcm_dbg_rec_info.c
 * @brief Debug and trace recording implementation for IPCM
 *
 * Provides debugging and tracing functionality including:
 * - Message transmission recording
 * - Pool operation tracking
 * - Performance statistics
 * - Error logging
 *
 * The debug information is stored in ring buffers for later analysis.
 */

#include "ipcm_plat_adapter.h"
#include "ipcm_dbg_rec_info.h"

#if IPCM_PROC_SUPPORT
#include "cvi_mailbox.h"
#include "ipcm.h"
#include "ipcm_port.h"
#include "mmio.h"

#define IPCM_MSG_BACKTRACE_LVL 4

static u32 _pool_block_total;
static void **_gtrace;
static void **_ftrace;
static IPCMPA_MUTEX _trace_mutex;
#endif

#ifdef IPCM_INFO_REC
#include "ring.h"
#include "ipcm_port.h"
#include "ipcm_env.h"

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

#define RING_NUM 128
#define INT_COST_RECORD_TIME_LIMIT 200
static IPCMRing *s_ring_recv = NULL;
static IPCMRing *s_ring_send = NULL;
static IPCMRing *s_ring_rls_buf = NULL;
static IPCMRing *s_ring_int_cost = NULL;
static int s_record_start = 1; // start record by default

#ifdef IPCM_INFO_REC_POOL

#define POOL_RING_NUM 128
typedef struct _PoolGetRlsRingItem {
	u32 t0;
	u32 status0_rec0;
	u32 t1;
	u32 status0_rec1;
	u32 t2;
	u32 status0_rec2;
	u32 t3;
	u32 status0_rec3;
	u32 data_pos;
	u8 block_idx;
	u8 func_type; // 0:rls pool buf  1:get pool buff
} PoolGetRlsRingItem;

static PoolGetRlsRingItem _item;
static IPCMRing *_pool_get_rls_ring = NULL;

#endif
#endif


#if IPCM_PROC_SUPPORT

void ipcm_dbg_r_proc_init(unsigned int block_total)
{
	_pool_block_total = block_total;

	_gtrace = malloc(block_total * IPCM_MSG_BACKTRACE_LVL * sizeof(void *));

	_ftrace = malloc(block_total * IPCM_MSG_BACKTRACE_LVL * sizeof(void *));

	IPCMPA_MUTEX_INIT(&_trace_mutex);
}


void ipcm_dbg_r_proc_uninit(void)
{
	IPCMPA_MUTEX_UNINIT(&_trace_mutex);
	if (_gtrace) {
		free(_gtrace);
		_gtrace = NULL;
	}
	if (_ftrace) {
		free(_ftrace);
		_ftrace = NULL;
	}
}


int ipcm_dbg_print_pool_proc(void)
{
	int i,j = 0;
	void **trace;

	if (_gtrace) {
		ipcm_info("pool get trace:");
		for (i=0; i<_pool_block_total; i++) {
			ipcm_info("\nid:%d", i);
			trace = (void **)((char *)_gtrace + (i * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
			for (j=0; j<IPCM_MSG_BACKTRACE_LVL; j++) {
				ipcm_info(" <- %p", trace[j]);
			}
		}
	}

	if (_ftrace) {
		ipcm_info("\npool free trace:");
		for (i=0; i<_pool_block_total; i++) {
			ipcm_info("\nid:%d", i);
			trace = (void **)((char *)_ftrace + (i * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
			for (j=0; j<IPCM_MSG_BACKTRACE_LVL; j++) {
				ipcm_info(" <- %p", trace[j]);
			}
		}
	}
	ipcm_info("\n");
	return 0;
}

// get
//  0: free trace
//  1: get trace
int ipcm_dbg_record_pool_bt(POOLHANDLE handle, void *data, unsigned char get)
{
	u32 pos;
	u32 block_idx;
	void **trace;
	void **trace_base;

	if (get) {
		trace_base = _gtrace;
	} else {
		trace_base = _ftrace;
	}

	if (data == NULL || trace_base == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}
	pos = pool_get_data_offset(handle, data);
	block_idx = pool_get_block_idx_by_offset(handle, pos);
	trace = (void **)((char *)trace_base + (block_idx * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
	IPCMPA_MUTEX_LOCK(&_trace_mutex);
	IPCMPA_BACKTRACE_NOW(trace, IPCM_MSG_BACKTRACE_LVL, 1);
	IPCMPA_MUTEX_UNLOCK(&_trace_mutex);
	return 0;
}

int ipcm_dbg_record_pool_bt_bypos(POOLHANDLE handle, u32 pos, unsigned char get)
{
	u32 block_idx;
	void **trace;
	void **trace_base;

	if (get) {
		trace_base = _gtrace;
	} else {
		trace_base = _ftrace;
	}

	if (trace_base == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}

	block_idx = pool_get_block_idx_by_offset(handle, pos);
	trace = (void **)((char *)trace_base + (block_idx * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
	IPCMPA_MUTEX_LOCK(&_trace_mutex);
	IPCMPA_BACKTRACE_NOW(trace, IPCM_MSG_BACKTRACE_LVL, 1);
	IPCMPA_MUTEX_UNLOCK(&_trace_mutex);
	return 0;
}

#endif

#ifdef IPCM_INFO_REC

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
		PR("%8lld %16llx %4d %4d %4d %d %4d, ", ring_data[i].t, ring_data[i].msg, ring_data[i].pos,
			ring_data[i].resp, ring_data[i].msgId, ring_data[i].moduleId, ring_data[i].cmd);
		if (((i+1)%2) == 0) {
			PR("\n");
		}
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
		PR("%8lld %4d %4d, ", ring_data[i].t, ring_data[i].peroid, ring_data[i].intNum);
		if (((i+1)%4) == 0) {
			PR("\n");
		}
	}

	ipcm_free(ring_data);
}
#ifdef IPCM_INFO_REC_POOL
void dbg_print_pool_get_rls_ring(void);
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
		msg_data = ipcm_msg_get_data_by_offset(pos);
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

void ipcm_dbg_r_msg_init(void)
{
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
}

void ipcm_dbg_r_msg_uninit(void)
{
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
}

void ipcm_dbg_r_msg_recv(void *msg)
{
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
			msg_data = ipcm_msg_get_data_by_offset(msg_ptr->msg_param.msg_ptr.data_pos);
			ipcm_port_inv_data(msg_data, msg_ptr->msg_param.msg_ptr.remaining_rd_len);
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
}

void ipcm_dbg_r_msg_send(void *msg)
{
	if (s_record_start && s_ring_send) {
		void *msg_data;
		MsgData *data = (MsgData *)msg;
		CVI_IPCMSG_MESSAGE_S *ipcmsg_ptr;
		IPCM_MSG_RING_ITEM item = {};

		item.t = timer_get_boot_us();
		item.msg = *(unsigned long long *)data;
		if (data && (data->func_type==MSG_TYPE_SHM)) {
			item.pos = data->msg_param.msg_ptr.data_pos;
			msg_data = ipcm_port_msg_data_by_offset(data->msg_param.msg_ptr.data_pos);
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
}

#ifdef IPCM_INFO_REC_POOL

static void _print_ring_snap_get_rls_pool(IPCMRing *ring)
{
	PoolGetRlsRingItem *ring_data = NULL;
	u32 rear;
	int i = 0;

	if (NULL == ring) {
		return;
	}

	ring_data = ipcm_alloc(POOL_RING_NUM * sizeof(PoolGetRlsRingItem));
	ring_snap(ring, (void **)&ring_data, &rear);
	ipcm_info("rear:%d\n", rear);
	for (i=0; i<POOL_RING_NUM; i++) {
		PR("%4d %4x %4d %4x %4d %4x %4d %4x %4d %d %d,\n", ring_data[i].t0, ring_data[i].status0_rec0,
			ring_data[i].t1, ring_data[i].status0_rec1, ring_data[i].t2, ring_data[i].status0_rec2,
			ring_data[i].t3, ring_data[i].status0_rec3, ring_data[i].data_pos, ring_data[i].block_idx, ring_data[i].func_type);
	}

	ipcm_free(ring_data);
}

void dbg_print_pool_get_rls_ring(void)
{
	_print_ring_snap_get_rls_pool(_pool_get_rls_ring);
}

void ipcm_dbg_r_pool_init(void)
{
    u32 ring_item_size = sizeof(PoolGetRlsRingItem);
    _pool_get_rls_ring = ipcm_alloc(sizeof(IPCMRing) + ring_item_size * POOL_RING_NUM);
    ipcm_warning("_pool_get_rls_ring addr:%px\n", (void *)_pool_get_rls_ring);
    memset(_pool_get_rls_ring, 0, sizeof(IPCMRing) + ring_item_size * POOL_RING_NUM);
    ring_init(_pool_get_rls_ring, ring_item_size, POOL_RING_NUM);
}

void ipcm_dbg_r_pool_uninit(void)
{
	if (_pool_get_rls_ring) {
		ring_uninit(_pool_get_rls_ring);
		ipcm_free(_pool_get_rls_ring);
		_pool_get_rls_ring = NULL;
	}
}

void ipcm_dbg_r_pool_t0(unsigned int status)
{
    _item.t0 = timer_get_boot_us();
    _item.status0_rec0 = status;
}

void ipcm_dbg_r_pool_t1(unsigned int status)
{
    _item.t1 = timer_get_boot_us();
    _item.status0_rec1 = status;
}

void ipcm_dbg_r_pool_t2(unsigned int status)
{
    _item.t2 = timer_get_boot_us();
    _item.status0_rec2 = status;
}

void ipcm_dbg_r_pool_t3(unsigned int status)
{
    _item.t3 = timer_get_boot_us();
    _item.status0_rec3 = status;
}

void ipcm_dbg_r_pool_data_offset(unsigned int data_pos)
{
    _item.data_pos = data_pos;
}

void ipcm_dbg_r_pool_block_idx(unsigned char block_idx)
{
    _item.block_idx = block_idx;
}

void ipcm_dbg_r_pool_func_type(unsigned char func_type)
{
    _item.func_type = func_type;
}

void ipcm_dbg_r_pool_put_ring(void)
{
    ring_put(_pool_get_rls_ring, &_item);
}
#endif

#endif

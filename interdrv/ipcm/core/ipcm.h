
#ifndef __IPCM__
#define __IPCM__

#include "ipcm_common.h"
#include "msg_queue.h"
#include "ipcm_pool.h"

#define MSG_QUEUE_LEN 128

typedef s32 (*recv_notifier)(u8 port_id, void *data);

typedef s32 (*ipcm_pre_handle)(u8 grp_id, void *msg);

typedef void *IPCMHandle;

typedef struct _IPCMHead {
	// u8 port_type;
	recv_notifier recv;
} IPCMHead;

#ifdef __ALIOS__
s32 ipcm_init(BlockConfig *config, u32 num, u32 pool_paddr, u32 pool_size);
#else
s32 ipcm_init(u32 pool_paddr, u32 pool_size);
#endif

s32 ipcm_uninit(void);

s32 ipcm_port_init(u32 port_id, recv_notifier handle);

s32 ipcm_port_uninit(u32 port_id);

s32 ipcm_register_irq_handle(ipcm_pre_handle pre_process);

s32 ipcm_register_pre_send_handle(ipcm_pre_handle pre_send);

s32 ipcm_send_msg(MsgData *data);

// return 0:lock success  1:lock fail
s32 ipcm_data_spin_lock(u8 lock_id);

s32 ipcm_data_spin_unlock(u8 lock_id);

#endif


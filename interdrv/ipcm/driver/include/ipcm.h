/**
 * @brief Main IPCM (Inter-Processor Communication Mechanism) header file
 * 
 * Defines the core IPCM interfaces for inter-processor communication
 */

#ifndef __IPCM__
#define __IPCM__

#include "ipcm_common.h"
#include "msg_queue.h"
#include "ipcm_pool.h"

#define MSG_QUEUE_LEN 128  // Maximum length of message queue

/**
 * @brief Callback function type for receiving notifications
 * @param port_id Port identifier
 * @param data Message data
 * @return Status code
 */
typedef s32 (*recv_notifier)(u8 port_id, void *data);

/**
 * @brief Pre-handler function type for message processing
 * @param grp_id Group identifier 
 * @param msg Message data
 * @return Status code
 */
typedef s32 (*ipcm_pre_handle)(u8 grp_id, void *msg);

typedef void *IPCMHandle;

typedef struct _IPCMHead {
	// u8 port_type;
	recv_notifier recv;
} IPCMHead;

s32 ipcm_init(u32 pool_mgr_paddr, u32 pool_mgr_capacity);

s32 ipcm_uninit(void);

s32 ipcm_register_port_handle(u32 port_id, recv_notifier handle);

s32 ipcm_degister_port_handle(u32 port_id);

s32 ipcm_register_irq_handle(ipcm_pre_handle pre_process);

s32 ipcm_register_pre_send_handle(ipcm_pre_handle pre_send);

s32 ipcm_send_msg(MsgData *data);

u32 ipcm_get_recv_msg_cnt(void);

u32 ipcm_get_send_msg_cnt(void);

// return 0:lock success  1:lock fail
s32 ipcm_data_spin_lock(u8 lock_id);

s32 ipcm_data_spin_unlock(u8 lock_id);

int ipcm_set_snd_cpu(int cpu_id);

#endif

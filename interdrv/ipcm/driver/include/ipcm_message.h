/**
 * @brief Message handling interfaces for IPCM
 *
 * Provides functions for message transmission, reception and buffer management
 * between processors
 */

#ifndef __IPCM_MESSAGE_H__
#define __IPCM_MESSAGE_H__

#include "ipcm_common.h"

/**
 * @brief Initialize message service on server side
 * @param config Pool configuration
 * @return Status code
 */
s32 ipcm_msg_srv_init(PoolConfig *config);

s32 ipcm_msg_srv_uninit(void);

s32 ipcm_msg_cli_init(void);

s32 ipcm_msg_cli_uninit(void);

u32 ipcm_msg_get_buff_offset(u32 size);

s32 ipcm_msg_release_buff_by_offset(u32 pos);

void *ipcm_msg_get_data_by_offset(u32 data_pos);
/**
 * @brief alloc buff from share memory
 * 
 * @param size 
 *  wish size, can not larger pool max block size
 * @return void* 
 *  NULL: fail
 *  others: data address
 */
void *ipcm_msg_get_buff(u32 size);

/**
 * @brief free buff to share memory
 * 
 * @param data 
 *  data address wish to release
 * @return s32 
 *  0: success
 *  others: fail
 */
s32 ipcm_msg_release_buff(void *data);

s32 ipcm_msg_pool_reset(void);
/**
 * @brief invalid data, only for ipcm
 * 
 * @param data 
 *  data address wish to inv
 * @param size 
 *  data size wish to inv
 * @return s32 
 *  0
 */
s32 ipcm_msg_inv_data(void *data, u32 size);

/**
 * @brief flush data, only for ipcm
 * 
 * @param data 
 *  data address wish to flush
 * @param size 
 *  data size wish to flush
 * @return s32 
 *  0
 */
s32 ipcm_msg_flush_data(void *data, u32 size);

s32 ipcm_msg_data_packed(void *data, u32 len, MsgData *msg);

/**
 * @brief send msg by share memory while len > 4
 * 
 * @param port_id 
 * @param msg_id 
 * @param data 
 * @param len 
 * @return s32 
 *  0: success others: fail
 */
s32 ipcm_msg_send_msg(u8 port_id, u8 msg_id, void *data, u32 len);

MsgData * ipcm_msg_recv_msg(u8 port_id);

/**
 * @brief Send parameter message (for messages <= 4 bytes)
 * @param port_id Target port ID
 * @param msg_id Message identifier
 * @param param Parameter value
 * @return Status code (0: success, others: fail)
 */
s32 ipcm_msg_send_param(u8 port_id, u8 msg_id, u32 param);

/**
 * @brief Poll for message availability
 * @param port_id Port to poll
 * @param timeout Timeout value
 * @return Status code
 */
s32 ipcm_msg_poll(u8 port_id, u32 timeout);

s32 ipcm_msg_up_blank_sem(u8 port_id);
/**
 * @brief get current msg info, maybe help for ipcm_msg_read_data
 * 
 * @param port_id 
 * @param func_type 
 * @param msg_id 
 * @param remain_len 
 * @return s32 
 *  0: success others: fail
 */
s32 ipcm_msg_get_cur_msginfo(u8 port_id, u8 *func_type, u8 *msg_id, u32 *remain_len);

/**
 * @brief read data from share memory, usually used with select(linux) or poll(alios)
 * 
 * @param data 
 *  the share memory data address readed
 * @param len 
 *  size wish to read
 * @return s32 
 *  <0: fail
 *  =0: 
 *  >0: length been readed
 */
s32 ipcm_msg_read_data(u8 port_id, void **data, u32 len);

/**
 * @brief Lock data access with spinlock
 * @param lock_id Lock identifier (range [0,4]) if lock_id > 4, it will be setted to lock_id%5
 * @return Status code (0: lock success, 1: lock fail)
 */
s32 ipcm_msg_data_lock(u8 lock_id);

/**
 * @brief Unlock data access
 * @param lock_id Lock identifier (range [0,4]) if lock_id > 4, it will be setted to lock_id%5
 * @return Status code (0: unlock success, 1: unlock fail)
 */
s32 ipcm_msg_data_unlock(u8 lock_id);

#endif /* __IPCM_MESSAGE_H__ */

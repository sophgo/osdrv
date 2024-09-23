
#ifndef __IPCM_MESSAGE_H__
#define __IPCM_MESSAGE_H__

#include "ipcm_common.h"

/**
 * @brief ipcm msg init
 * 
 */
s32 ipcm_msg_init(void);

/**
 * @brief ipcm msg uninit
 * 
 * @return s32 0:success thers:fail
 */
s32 ipcm_msg_uninit(void);

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

/**
 * @brief send msg by param while len <= 4
 * 
 * @param port_id 
 * @param msg_id 
 * @param param 
 * @return s32 
 *  0: success others: fail
 */
s32 ipcm_msg_send_param(u8 port_id, u8 msg_id, u32 param);

#if defined(__ALIOS__) || defined(__LINUX_DRV__)
s32 ipcm_msg_poll(u8 port_id, int32_t timeout);
#endif

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
 * @brief data spinlock
 * 
 * @param lock_id 
 *  range [0,4]; if lock_id > 4, it will be setted to lock_id%5
 * @return s32 
 *  0:lock success  1:lock fail
 */
s32 ipcm_msg_data_lock(u8 lock_id);

/**
 * @brief data spinunlock
 * 
 * @param lock_id 
 *  range [0,4]; if lock_id > 4, it will be setted to lock_id%5
 * @return s32 
 *   0:lock success  1:lock fail
 */
s32 ipcm_msg_data_unlock(u8 lock_id);

#endif

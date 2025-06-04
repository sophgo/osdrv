/**
 * @brief Custom IPCM interfaces for user-defined communication
 *
 * Provides customizable message handling interfaces for inter-processor communication
 */

#ifndef __IPCM_CUSTYMOUS_H__
#define __IPCM_CUSTYMOUS_H__

#include "ipcm_common.h"

/**
 * @brief Custom message structure
 */
typedef struct _ipcm_cust_msg_t {
    u8 port_id;        // Port identifier
    u8 msg_id : 7;     // Message identifier (7 bits)
    u8 data_type : 1;  // Data type flag
    void *data;        // Message data pointer
    u32 size;          // Data size
} ipcm_cust_msg_t;

/**
 * @brief Custom message processing callback
 * 
 * @note For Linux kernel handlers (port_id > IPCM_CUST_KER_PORT_ST):
 * Return 1 if message processed by kernel, otherwise message will be forwarded to userspace
 *
 * @param priv Private data passed to handler
 * @param data Message data
 * @return Status code
 */
typedef s32 (*CUST_MSGPROC_FN)(void *priv, ipcm_cust_msg_t *data);

s32 ipcm_cust_srv_init(PoolConfig *config);

s32 ipcm_cust_srv_uninit(void);

s32 ipcm_cust_cli_init(void);

s32 ipcm_cust_cli_uninit(void);

s32 ipcm_cust_get_pool_offset(void);

s32 ipcm_cust_register_handle(CUST_MSGPROC_FN handler, void *data);

s32 ipcm_cust_deregister_handle(void);

s32 ipcm_cust_data_packed(void *data, u32 len, MsgData *msg);

// send msg if msg len > 4; max msg length is limited by pool block (2048?)
s32 ipcm_cust_send_msg(u8 port_id, u8 msg_id, void *data, u32 len);

MsgData * ipcm_cust_recv_msg(void);

// send param if msg len <= 4 or send 32 bits addr 
s32 ipcm_cust_send_param(u8 port_id, u8 msg_id, u32 param);

u32 ipcm_cust_get_buff_offset(u32 size);

s32 ipcm_cust_release_buff_by_offset(u32 pos);

void *ipcm_cust_get_data_by_offset(u32 data_pos);

void *ipcm_cust_get_buff(u32 size);

s32 ipcm_cust_release_buff(void *data);

s32 ipcm_cust_pool_reset(void);
#endif
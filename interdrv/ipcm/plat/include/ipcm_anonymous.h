
#ifndef __IPCM_ANONYMOUS_H__
#define __IPCM_ANONYMOUS_H__

#include "ipcm_common.h"

/**
 * @brief 
 * 
 */
typedef struct _ipcm_anon_msg_t {
    u8 port_id;
    u8 msg_id : 7;
	u8 data_type : 1;
    void *data;
    u32 size;
} ipcm_anon_msg_t;

/**
 * note: 
 * if linux handler in kernel space, you should return 1 if msg has been processed by kernel;
 * otherwise msg will push to user space again
 * linux handler only support port id large than IPCM_ANON_KER_PORT_ST
 */
typedef s32 (*ANON_MSGPROC_FN)(void *priv, ipcm_anon_msg_t *data);

s32 ipcm_anon_init(void);

s32 ipcm_anon_uninit(void);

s32 ipcm_anon_register_handle(ANON_MSGPROC_FN handler, void *data);

s32 ipcm_anon_deregister_handle(void);

// send msg if msg len > 4; max msg length is limited by pool block (2048?)
s32 ipcm_anon_send_msg(u8 port_id, u8 msg_id, void *data, u32 len);

// send param if msg len <= 4 or send 32 bits addr 
s32 ipcm_anon_send_param(u8 port_id, u8 msg_id, u32 param);

void *ipcm_anon_get_user_addr(u32 paddr);

#endif
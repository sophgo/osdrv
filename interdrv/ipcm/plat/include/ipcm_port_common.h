
#ifndef __IPCM_PORT_COMMON_H__
#define __IPCM_PORT_COMMON_H__

#include "ipcm_common.h"

typedef s32 (*MSGPROC_FN)(void *msg, void *priv);

typedef struct _msg_proc_item {
    u32 msg_id;
    MSGPROC_FN func;
    ipcm_sem sem;
} msg_proc_item;

typedef struct _msg_proc_info {
    u32 port_id;
    u32 func_amount;
    msg_proc_item *table;
} msg_proc_info;

typedef enum _IPCM_SYS_MSG_ID_E {
    IPCM_MSG_GET_SYSINFO,
    IPCM_MSG_GET_SYSINFO_RSP,
    IPCM_MSG_GET_LOG,
    IPCM_MSG_GET_LOG_RSP,

    IPCM_SYS_MSG_BUTT
} IPCM_SYS_MSG_ID_E;

MSGPROC_FN port_get_msg_fn(u32 msg_id, msg_proc_info *proc_info);

#ifdef __ALIOS__
s32 ipcm_port_common_init(BlockConfig *config, u32 num);
#else
s32 ipcm_port_common_init(void);
#endif

s32 ipcm_port_common_uninit(void);

u32 ipcm_get_buff_to_pos(u32 size);

s32 ipcm_release_buff_by_pos(u32 pos);

s32 ipcm_inv_data(void *data, u32 size);

s32 ipcm_flush_data(void *data, u32 size);

s32 ipcm_data_lock(u8 lock_id);

s32 ipcm_data_unlock(u8 lock_id);

void *ipcm_get_buff(u32 size);

s32 ipcm_release_buff(void *data);

s32 ipcm_data_packed(void *data, u32 len, MsgData *msg);

void *ipcm_get_data_by_pos(u32 data_pos);

s32 ipcm_common_send_msg(MsgData *msg);

void *ipcm_get_user_addr(u32 paddr);

s32 ipcm_pool_reset(void);

u32 get_param_bin_addr(void);

u32 get_param_bak_bin_addr(void);

u32 get_pq_bin_addr(void);

int ipcm_set_snd_cpu(int cpu_id);

// only effect in rtos
s32 ipcm_set_rtos_boot_bit(RTOS_BOOT_STATUS_E stage, u8 stat);

s32 ipcm_get_rtos_boot_status(u32 *stat);

#endif

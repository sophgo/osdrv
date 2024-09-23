
#ifndef __IPCM_PORT_MSG_H__
#define __IPCM_PORT_MSG_H__

#include "ipcm_common.h"

s32 ipcm_drv_msg_init(void);

s32 ipcm_drv_msg_uninit(void);

int ipcm_msg_register_dev(void);

void ipcm_msg_deregister_dev(void);

#endif

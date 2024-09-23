
#ifndef __IPCM_PORT_ANON_H__
#define __IPCM_PORT_ANON_H__

#include "ipcm_port_common.h"

s32 ipcm_drv_anon_init(void);

s32 ipcm_drv_anon_uninit(void);

int ipcm_anon_register_dev(void);

void ipcm_anon_deregister_dev(void);

#endif
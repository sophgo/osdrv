/**
 * @file ipcm_port_sys.h
 * @author allen.huang (allen.huang@cvitek.com)
 * @brief 
 * @version 0.1
 * @date 2023-01-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __IPCM_PORT_SYS_H__
#define __IPCM_PORT_SYS_H__

#include "ipcm_port_common.h"

int ipcm_drv_sys_init(void);

s32 ipcm_drv_sys_uninit(void);

int ipcm_sys_register_dev(void);

void ipcm_sys_deregister_dev(void);

#endif

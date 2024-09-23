/**
 * @file ipcm_system.h
 * @author allen.huang (allen.huang@cvitek.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __IPCM_SYSTEM_H__
#define __IPCM_SYSTEM_H__

#include "ipcm_common.h"

/* this structure should be modified both linux & alios side */
struct dump_uart_s {
	char* dump_uart_ptr;
	unsigned int  dump_uart_max_size;
	unsigned int  dump_uart_pos;
	unsigned char dump_uart_enable;
	unsigned char dump_uart_overflow;
} __attribute__((packed));

s32 ipcm_sys_init(void);

s32 ipcm_sys_uninit(void);

#endif

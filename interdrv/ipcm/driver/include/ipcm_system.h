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

/**
 * @brief IPCM system level interfaces
 *
 * Provides system-level functionality for IPCM including UART dump handling
 */

#ifndef __IPCM_SYSTEM_H__
#define __IPCM_SYSTEM_H__

#include "ipcm_common.h"

/**
 * @brief UART dump structure
 * 
 * Shared structure between Linux and RTOS for UART dump functionality
 */
struct dump_uart_s {
	char* dump_uart_ptr;
	unsigned int  dump_uart_max_size;
	unsigned int  dump_uart_off;
	unsigned char dump_uart_enable;
	unsigned char dump_uart_overflow;
} __attribute__((packed));

/**
 * @brief Initialize IPCM system components
 * @return Status code
 */
s32 ipcm_sys_init(void);

/**
 * @brief Uninitialize IPCM system components
 * @return Status code
 */
s32 ipcm_sys_uninit(void);

#endif

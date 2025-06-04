// Platform adapter header for IPCM on AliOS
#ifndef __IPCM_PLAT_ADAPTER_H__
#define __IPCM_PLAT_ADAPTER_H__

#include "ipcm_plat_adapter_common.h"

// Required headers for ipcm_pool.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aos/kernel.h>
#include "asm/barrier.h"

// Required headers for cvi_spinlock.c
#include "vip_spinlock.h"
#include "mmio.h"
#include "stdint.h"
#include "top_reg.h"
#include "cvi_mailbox.h"

// Required headers for mailbox.c
#include <drv/cvi_irq.h>
#include <errno.h>

// Function to get backtrace
extern int backtrace_now_get(void *trace[], int size, int offset);

// CPU definitions
#define RECEIVE_CPU CPU_C960L
#define SEND_TO_CPU CPU_C906B
#define MB_LOCKCNT_SHIRT 8

#define PLATFORM_STRING "alios"

// Task related definitions
#define IPCM_CUST_TASK_PRI 18
#define IPCM_CUST_TASK_NAME "ipcm_cust"

// Memory mapping macros
#define IPCMPA_MEMREMAP(addr, size) addr
#define IPCMPA_MEMUNMAP(addr)
#define __IPCMPA_IOMEM
#define ipcmpa_phys_addr unsigned int

// IRQ related macros
#define IPCMPA_IRQ_RETURN_TYPE void
#define IPCMPA_IRQ_RETURN_HANDLE

// Semaphore related macros
#define IPCMPA_SEM aos_sem_t
#define IPCMPA_SEM_INIT aos_sem_new
#define IPCMPA_SEM_UNINIT(x) if (aos_sem_is_valid(x)) aos_sem_free(x)
#define IPCMPA_SEM_IS_VALID(x) aos_sem_is_valid(x)
#define IPCMPA_SEM_UP aos_sem_signal
#define IPCMPA_SEM_DOWN_TIMEOUT aos_sem_wait
#define IPCMPA_SEM_TIME_MS_CONVERT(x) x

// Wait queue related macros
#define IPCMPA_WQ_HEAD aos_sem_t
#define IPCMPA_WQ_WAKE_UP aos_sem_signal

// Task related macros
#define IPCMPA_TASK aos_task_t
#define IPCMPA_TASK_INIT
#define IPCMPA_TASK_UNINIT
#define IPCMPA_TASK_NEW aos_task_new_ext
#define IPCMPA_TASK_STOP

// Mutex related macros
#define IPCMPA_MUTEX aos_mutex_t
#define IPCMPA_MUTEX_INIT aos_mutex_new
#define IPCMPA_MUTEX_UNINIT(x) if (aos_mutex_is_valid(x)) aos_mutex_free(x)
#define IPCMPA_MUTEX_LOCK(x) aos_mutex_lock((x),AOS_WAIT_FOREVER)
#define IPCMPA_MUTEX_UNLOCK aos_mutex_unlock
#define IPCMPA_BACKTRACE_NOW(a, b, c) backtrace_now_get(a, b, c);

#define IPCMPA_SPIN_LOCK_DESTROY(lock) spin_lock_destroy(lock)

#define IPCMPA_EXPORT_SYMBOL_GPL(f)

// Delay function declaration
extern void udelay(uint32_t us);

// IO memory mapping functions
void __IPCMPA_IOMEM *ipcmpa_ioremap(ipcmpa_phys_addr offset, size_t size);

void ipcmpa_iounmap(void __IPCMPA_IOMEM *addr);

unsigned int ipcmpa_ioread32(const void __IPCMPA_IOMEM *addr);

int ipcmpa_iowrite32(unsigned int b, void __IPCMPA_IOMEM *addr);

unsigned int ipcmpa_readpc(const void __IPCMPA_IOMEM *addr);

unsigned long long timer_read_counter(void);

#endif

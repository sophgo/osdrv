// Environment configuration for IPCM on AliOS platform
#ifndef __IPCM_ENV_H__
#define __IPCM_ENV_H__

#include <aos/kernel.h>
#include <debug/dbg.h>
#include "arch_helpers.h"

// Print function alias
#define PR aos_debug_printf

// Memory allocation functions
#define ipcm_alloc malloc
#define ipcm_free free

// Sleep function
#define ipcm_msleep aos_msleep

// Cache operation macros
#define ipcm_pool_cache_flush(paddr, vaddr, size) flush_dcache_range(paddr, size)
#define ipcm_pool_cache_invalidate(paddr, vaddr, size) inv_dcache_range(paddr, size)

// Mailbox IRQ control macros
#define ipcm_mailbox_irq_enable csi_irq_enable(MBOX_INT_C906_2ND);
#define ipcm_mailbox_irq_disable csi_irq_disable(MBOX_INT_C906_2ND);

// Semaphore type definition
#define ipcm_sem aos_sem_t *

#endif

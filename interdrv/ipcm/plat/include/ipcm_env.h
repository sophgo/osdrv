
#ifndef __IPCM_ENV_H__
#define __IPCM_ENV_H__

#include <linux/kernel.h>
#include <linux/string.h>
#include "cvi_sys.h"

#define PR printk

extern int mailbox_irq;
#define ipcm_alloc(size) kmalloc(size, GFP_KERNEL)
#define ipcm_free kfree
#define ipcm_msleep msleep
#define ipcm_mutex_t struct mutex
#define ipcm_mutex_init mutex_init
#define ipcm_mutex_uninit(x) ((void)(x))
#define ipcm_mutex_lock mutex_lock
#define ipcm_mutex_unlock mutex_unlock
#define ipcm_pool_cache_flush(paddr, vaddr, size) ipcm_sys_cache_flush(paddr, size)
#define ipcm_pool_cache_invalidate(paddr, vaddr, size) ipcm_sys_cache_invalidate(paddr, size)
#define ipcm_mailbox_irq_enable enable_irq(mailbox_irq);
#define ipcm_mailbox_irq_disable disable_irq(mailbox_irq);
#define ipcm_sem struct semaphore *

#endif

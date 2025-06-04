
#ifndef __IPCM_ENV_H__
#define __IPCM_ENV_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PR printf

#define ipcm_alloc malloc
#define ipcm_free free
#define ipcm_mutex_init(x) ((void)(x))
#define ipcm_mutex_uninit(x) ((void)(x))
#define ipcm_mutex_lock(x) ((void)(x))
#define ipcm_mutex_unlock(x) ((void)(x))
#define ipcm_msleep msleep
#define ipcm_pool_cache_flush(paddr, vaddr, size)
#define ipcm_pool_cache_invalidate(paddr, vaddr, size)
#define ipcm_sem void *

#endif

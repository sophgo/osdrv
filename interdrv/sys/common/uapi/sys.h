#ifndef __SYS_H__
#define __SYS_H__

#include <linux/types.h>
#include <linux/cdev.h>

#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"

int32_t sys_ion_dump(void);
int32_t sys_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached);
int32_t sys_ion_alloc_nofd(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached);
int32_t sys_ion_free(uint64_t u64PhyAddr);
int32_t sys_ion_free_nofd(uint64_t u64PhyAddr);
int32_t sys_ion_get_memory_statics(uint64_t *total_size, uint64_t *free_size, uint64_t *max_avail_size);

int32_t sys_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len);
int32_t sys_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len);


#endif  /* __SYS_H__ */


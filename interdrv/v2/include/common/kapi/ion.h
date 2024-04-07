#ifndef __ION_H__
#define __ION_H__

int32_t base_ion_free(uint64_t u64PhyAddr);
int32_t base_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached);

int32_t base_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len);
int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len);
int32_t base_ion_dump(void);


#endif
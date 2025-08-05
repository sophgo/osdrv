#include "aos/kernel.h"
#include "asm/cache.h"
#include "osal.h"


int32_t base_ion_free(uint64_t phy_addr)
{
	aos_ion_free((void *)(uintptr_t)phy_addr);

	return 0;
}

int32_t base_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	void *p = aos_ion_malloc(buf_len);

	if (p) {
		*p_paddr = (uint64_t)p;
		*pp_vaddr = p;
		return 0;
	}

	return -1;
}

int32_t base_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t len)
{
	inv_dcache_range((uintptr_t)addr_p, len);
	return 0;
}

int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t len)
{
	flush_dcache_range((uintptr_t)addr_p, len);
	return 0;
}

int32_t base_ion_dump(void)
{
	return 0;
}

int32_t base_ion_init(void)
{
	return 0;
}

int32_t base_ion_deinit(void)
{
	return 0;
}


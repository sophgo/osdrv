
#include "cvi_sys.h"
#include <asm/cacheflush.h>
#include <linux/dma-map-ops.h>

unsigned int ipcm_sys_cache_invalidate(unsigned long addr_p, unsigned int u32Len)
{
#ifdef __riscv
	// printk("sys_cache_invalidate addr(%llx) size(%llu)\n", addr_p, u32Len);
	arch_sync_dma_for_device(addr_p, u32Len, DMA_FROM_DEVICE);
#else
	__dma_map_area(phys_to_virt(addr_p), u32Len, DMA_FROM_DEVICE);
#endif
	/*	*/
	smp_mb();
	return 0;
}

unsigned int ipcm_sys_cache_flush(unsigned long addr_p, unsigned int u32Len)
{
#ifdef __riscv
	// printk("sys_cache_flush addr(%llx) size(%llu)\n", addr_p, u32Len);
	arch_sync_dma_for_device(addr_p, u32Len, DMA_TO_DEVICE);
#else
	__dma_map_area(phys_to_virt(addr_p), u32Len, DMA_TO_DEVICE);
#endif
	/*  */
	smp_mb();
	return 0;
}

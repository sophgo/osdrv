// Cache operation helper functions for AliOS platform
#ifndef __ARCH_HELPERS_H__
#define __ARCH_HELPERS_H__
#include <stdint.h>
#include <sys/types.h>

// Flush data cache for specified memory range
void flush_dcache_range(uintptr_t addr, size_t size);

// Clean data cache for specified memory range 
void clean_dcache_range(uintptr_t addr, size_t size);

// Invalidate data cache for specified memory range
void inv_dcache_range(uintptr_t addr, size_t size);

// Enable data cache
void enable_dcache(void);

// Disable data cache 
void disable_dcache(void);

#endif /* __ARCH_HELPERS_H__ */

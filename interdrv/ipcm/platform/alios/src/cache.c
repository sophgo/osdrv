//#include <platform.h>
#include <arch_helpers.h>
#include <stdio.h>
#include "ipcm_common.h"

//typedef uint64_t phys_addr_t;
//typedef uintptr_t       size_t;

// Cache line size in bytes
#define L1_CACHE_BYTES     64

// Align value x up to alignment a
#ifndef ALIGN
#define ALIGN(x, a)              (((x) + (a) - 1) & ~((a) - 1))
#endif

/*
 * RISC-V cache operation instruction encodings:
 *
 * dcache.ipa rs1 (invalidate)
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000001    01010      rs1       000      00000  0001011
 *
 * dcache.cpa rs1 (clean)
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000001    01001      rs1       000      00000  0001011
 *
 * dcache.cipa rs1 (clean then invalidate)
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000001    01011      rs1       000      00000  0001011
 *
 * sync.s
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000000    11001     00000      000      00000  0001011
 */

// RISC-V cache operation instruction encodings
#define DCACHE_IPA_A0   ".long 0x02a5000b"  // Invalidate
#define DCACHE_CPA_A0   ".long 0x0295000b"  // Clean
#define DCACHE_CIPA_A0  ".long 0x02b5000b"  // Clean and invalidate
#define SYNC_S          ".long 0x0190000b"  // Memory sync barrier

// Perform cache operation on memory range
#define CACHE_OP_RANGE(OP, start, size) \
	register unsigned long i asm("a0") = start & ~(L1_CACHE_BYTES - 1); \
	for (; i < ALIGN(start + size, L1_CACHE_BYTES); i += L1_CACHE_BYTES) \
		__asm__ __volatile__(OP); \
	 __asm__ __volatile__(SYNC_S)

// Invalidate data cache for specified memory range
//void c900_cache_invalidate(phys_addr_t start, size_t size)
void inv_dcache_range(uintptr_t start, size_t size)
{
	ipcm_debug("%s addr(%lx) size(%lu)\n", __func__, start, size);
	CACHE_OP_RANGE(DCACHE_IPA_A0, start, size);
}

// Clean data cache for specified memory range
//void c900_cache_clean(phys_addr_t start, size_t size)
void clean_dcache_range(uintptr_t start, size_t size)
{
	ipcm_debug("%s addr(%lx) size(%lu)\n", __func__, start, size);
	CACHE_OP_RANGE(DCACHE_CPA_A0, start, size);
}

// Clean and invalidate data cache for specified memory range
//void c900_cache_flush(phys_addr_t start, size_t size)
void flush_dcache_range(uintptr_t start, size_t size)
{
	ipcm_debug("%s addr(%lx) size(%lu)\n", __func__, start, size);
	CACHE_OP_RANGE(DCACHE_CIPA_A0, start, size);
}

// Enable data cache by setting MHCR bit 1
void enable_dcache(void)
{
	asm volatile(
		"csrs mhcr, %0;" ::"rI"(0x2)
	);
}

// Disable data cache by clearing MHCR bit 1
void disable_dcache(void)
{
	asm volatile(
		"csrc mhcr, %0;" ::"rI"(0x2)
	);
}


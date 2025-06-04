/**
 * @brief Platform adapter common definitions for IPCM
 *
 * Provides platform-specific adaptations and common functionality for IPCM
 */

#ifndef __IPCM_PLAT_ADAPTER_COMMON_H__
#define __IPCM_PLAT_ADAPTER_COMMON_H__

#include "ipcm_common.h"
#include "msg_queue.h"
#include "ipcm_custom.h"

// CPU identifiers
#define CPU_C906B 0  // C906B processor
#define CPU_C960L 2  // C960L processor

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

// System counter frequency (25MHz)
#define SYS_COUNTER_FREQ_IN_SECOND 25000000

#ifdef __riscv
#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)	\
				: "=r" (__v) :			\
				: "memory");			\
	__v;							\
})

#define CSR_TIME 0xc01
#endif

/**
 * @brief Platform interface functions
 */

// Mailbox operations
int ipcmpa_get_mb_valid(void *mb);
void ipcmpa_set_mb_valid(void *mb);

// IRQ management
int ipcmpa_request_irq(void);
int ipcmpa_free_irq(void);

// IPCM initialization
int ipcmpa_cvi_ipcm_init(void);
int ipcmpa_cvi_ipcm_uninit(void);

// System utilities
unsigned long long ipcmpa_get_boot_us(void);

void * ipcmpa_phys_to_virt(unsigned long address);

// Cache management
unsigned int ipcmpa_sys_cache_invalidate(unsigned long addr_p, unsigned int u32Len);
unsigned int ipcmpa_sys_cache_flush(unsigned long addr_p, unsigned int u32Len);

// Custom core communication
unsigned int ipcmpa_cust_init(void *ctx);
unsigned int ipcmpa_cust_uninit(void *ctx);

#endif /* __IPCM_PLAT_ADAPTER_COMMON_H__ */
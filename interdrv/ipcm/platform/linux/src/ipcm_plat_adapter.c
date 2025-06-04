
#include <asm/cacheflush.h>
#include <linux/dma-map-ops.h>

#ifndef __riscv  // arm linux driver
#include <linux/timex.h>
#include <linux/math64.h>
extern u64 (*arch_timer_read_counter)(void);
#endif
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>

#include "ipcm_plat_adapter.h"
#include "ipcm_common.h"
#include "ipcm.h"
#include "ipcm_custom.h"
#include "ipcm_port.h"

extern int mailbox_irq;
extern IPCMPA_IRQ_RETURN_TYPE prvQueueISR(int irq, void *dev_id);

// linux free irq will compare dev point, so we define mb_pri for free_irq
static int mb_pri;

// Map physical address to virtual address
void __IPCMPA_IOMEM *ipcmpa_ioremap(ipcmpa_phys_addr offset, size_t size)
{
    return ioremap(offset, size);
}

// Unmap virtual address
void ipcmpa_iounmap(void __IPCMPA_IOMEM *addr)
{
    iounmap(addr);
}

// Read 32-bit value from IO memory
unsigned int ipcmpa_ioread32(const void __IPCMPA_IOMEM *addr)
{
	return ioread32(addr);
}

// Write 32-bit value to IO memory
int ipcmpa_iowrite32(unsigned int b, void __IPCMPA_IOMEM *addr)
{
	iowrite32(b, addr);
	return 0;
}

// Read program counter
unsigned int ipcmpa_readpc(const void __IPCMPA_IOMEM *addr)
{
    return ioread32(addr);
}

// Check if mailbox message is valid
int ipcmpa_get_mb_valid(void *mb)
{
    MsgData *msg = (MsgData *)mb;
    return msg->resv.valid.rtos_valid;
}

// Set mailbox message as valid
void ipcmpa_set_mb_valid(void *mb)
{
    MsgData *msg = (MsgData *)mb;
    msg->resv.valid.linux_valid = 1;
}

// Request mailbox interrupt handler
int ipcmpa_request_irq(void)
{
	int ret = request_irq(mailbox_irq, prvQueueISR, 0, "mailbox", (void *)&mb_pri);
	if (ret) {
		ipcm_err("fail to register interrupt handler ret:%d\n", ret);
		return ret;
	}
    return 0;
}

// Free mailbox interrupt handler
int ipcmpa_free_irq(void)
{
	/* remove irq handler*/
	free_irq(mailbox_irq, &mb_pri);
	return 0;
}

// Initialize IPCM module
int ipcmpa_cvi_ipcm_init(void)
{
    return ipcm_port_init();
}

// Uninitialize IPCM module
int ipcmpa_cvi_ipcm_uninit(void)
{
    return ipcm_port_uninit();
}

#if 0 // arm linux user space
unsigned long long timer_read_counter(void)
{
	unsigned long long cval = 0;
#ifdef __arm__
	// arm32
	asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (cval));
#endif
#ifdef __aarch64__
	// aarch64
	asm volatile("mrs %0, cntvct_el0" : "=r" (cval) :: "memory");
#endif
	return cval;
}
#endif

// Get system boot time in microseconds
unsigned long long ipcmpa_get_boot_us(void)
{

	unsigned long long boot_us = 0;
#ifdef __riscv
	boot_us = csr_read(CSR_TIME) / (SYS_COUNTER_FREQ_IN_SECOND / 1000000);
#else
	#if 1 // arm linux driver
	boot_us = div_u64(arch_timer_read_counter(), (SYS_COUNTER_FREQ_IN_SECOND / 1000000));
	#endif
    #if 0 // arm linux user space
	boot_us = timer_read_counter()  / (SYS_COUNTER_FREQ_IN_SECOND / 1000000);
	#endif
#endif

	return boot_us;
}
EXPORT_SYMBOL_GPL(ipcmpa_get_boot_us);

// Convert physical address to virtual address
void * ipcmpa_phys_to_virt(unsigned long address)
{
    return phys_to_virt(address);
}

// Invalidate data cache for specified memory range
unsigned int ipcmpa_sys_cache_invalidate(unsigned long addr_p, unsigned int u32Len)
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

// Flush data cache for specified memory range
unsigned int ipcmpa_sys_cache_flush(unsigned long addr_p, unsigned int u32Len)
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

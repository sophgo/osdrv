#include "ipcm_plat_adapter.h"
#include "ipcm_common.h"
#include "ipcm_custom.h"
#include "ipcm_port.h"

extern IPCMPA_IRQ_RETURN_TYPE prvQueueISR(int irq, void *dev_id);

// Map physical address to virtual address (no mapping needed for AliOS)
void __IPCMPA_IOMEM *ipcmpa_ioremap(ipcmpa_phys_addr offset, size_t size)
{
    return (void __IPCMPA_IOMEM *)(unsigned long)offset;
}

// Unmap virtual address (no-op for AliOS)
void ipcmpa_iounmap(void __IPCMPA_IOMEM *addr)
{
    UNUSED(addr);
}

// Read 32-bit value from IO memory
unsigned int ipcmpa_ioread32(const void __IPCMPA_IOMEM *addr)
{
    return *(unsigned int*)addr;
}

// Write 32-bit value to IO memory
int ipcmpa_iowrite32(unsigned int b, void __IPCMPA_IOMEM *addr)
{
    *(uint32_t volatile *)addr = b;
	return 0;
}

// Read program counter (not implemented for AliOS)
unsigned int ipcmpa_readpc(const void __IPCMPA_IOMEM *addr)
{
    return 0;
}

// Check if mailbox message is valid
int ipcmpa_get_mb_valid(void *mb)
{
    MsgData *msg = (MsgData *)mb;
    return msg->resv.valid.linux_valid;
}

// Set mailbox message as valid
void ipcmpa_set_mb_valid(void *mb)
{
    MsgData *msg = (MsgData *)mb;
    msg->resv.valid.rtos_valid = 1;
}

// Request mailbox interrupt handler
int ipcmpa_request_irq(void)
{
	int ret = request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", NULL);
	if (ret) {
		ipcm_err("fail to register interrupt handler ret:%d\n", ret);
		return ret;
	}
    return 0;
}

// Free mailbox interrupt handler
int ipcmpa_free_irq(void)
{
    return 0;
}

// Initialize IPCM module
int ipcmpa_cvi_ipcm_init(void)
{
    return 0;
}

int ipcmpa_cvi_ipcm_uninit(void)
{
    return 0;
}

unsigned long long ipcmpa_get_boot_us(void)
{
	unsigned long long boot_us = 0;
	boot_us = csr_read(CSR_TIME) / (SYS_COUNTER_FREQ_IN_SECOND / 1000000);
	return boot_us;
}

void * ipcmpa_phys_to_virt(unsigned long address)
{
    return (void *)address;
}

unsigned int ipcmpa_sys_cache_invalidate(unsigned long addr_p, unsigned int u32Len)
{
	inv_dcache_range(addr_p, u32Len);
    return 0;
}

unsigned int ipcmpa_sys_cache_flush(unsigned long addr_p, unsigned int u32Len)
{
    flush_dcache_range(addr_p, u32Len);
	return 0;
}

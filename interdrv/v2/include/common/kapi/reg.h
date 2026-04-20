#ifndef _REG_H_
#define _REG_H_

#include <linux/io.h>
#include <linux/version.h>

#define _reg_read(addr) readl((void __iomem *)addr)
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
    #define _reg_write(addr, data) writel(data, (void __iomem *)addr)
#else
    #define _reg_write(addr, data) writel((u32)(data), (void __iomem *)addr)
#endif
void _reg_write_mask(uintptr_t addr, u32 mask, u32 data);

#endif //_REG_H_

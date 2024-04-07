#ifndef _CVI_REG_H_
#define _CVI_REG_H_

#ifdef ENV_CVITEST
#define _reg_read(addr) readl((u64)addr)
#define _reg_write(addr, data) writel((u64)addr, data)
void _reg_write_mask(u64 addr, u32 mask, u32 data);

#elif defined(ENV_EMU)
u32 _reg_read(u64 addr);
void _reg_write(u64 addr, u32 data);
void _reg_write_mask(u64 addr, u32 mask, u32 data);

#else
#include <linux/io.h>
#include <linux/cvi_comm_dpu.h>
extern int dump_reg;

#define _reg_read(addr) readl((void __iomem *)addr)

#if 1
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)
#else
#define _reg_write(addr, data) \
	{ \
		writel(data, (void __iomem *)addr); \
		if (dump_reg) \
			pr_info("MWriteS32 %#x %#x\n", (u32)(addr), (u32)(data)); \
	}
#endif

void reg_write_mask(u64 addr, u32 mask, u32 data);
u32 reg_read_mask(u64 addr, u32 mask,u32 shift);
u32 read_reg(u64 addr);
void write_reg(u64 addr,u32 data);
#endif

#endif //_CVI_REG_H_
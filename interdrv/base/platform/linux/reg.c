#include <linux/io.h>

static DEFINE_RAW_SPINLOCK(__io_lock);

u32 _reg_read(uintptr_t addr)
{
	return readl((void __iomem *)addr);
}
EXPORT_SYMBOL_GPL(_reg_read);

void _reg_write(uintptr_t addr, u32 data)
{
	writel(data, (void __iomem *)addr);
}
EXPORT_SYMBOL_GPL(_reg_write);

void _reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	unsigned long flags;
	u32 value;

	raw_spin_lock_irqsave(&__io_lock, flags);
	value = readl_relaxed((void __iomem *)addr) & ~mask;
	value |= (data & mask);
	writel(value, (void __iomem *)addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
}
EXPORT_SYMBOL_GPL(_reg_write_mask);


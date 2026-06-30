#include "io.h"
#include <aos/kernel.h>
#include <k_api.h>

static kspinlock_t mask_lock;


u32 _reg_read(uintptr_t addr)
{
	return readl((void *)addr);
}

void _reg_write(uintptr_t addr, u32 data)
{
	writel(data, (void *)addr);
}

void _reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	unsigned long flags;
	u32 value;

	krhino_spin_lock_irq_save(&mask_lock, flags);
	value = readl_relaxed((void *)addr) & ~mask;
	value |= (data & mask);
	writel_relaxed(value, (void *)addr);
	krhino_spin_unlock_irq_restore(&mask_lock, flags);
}


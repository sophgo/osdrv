/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_reg.c
 * Description: driver layer register access control code
 */

#include "cvi_reg.h"

static DEFINE_RAW_SPINLOCK(__io_lock);

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

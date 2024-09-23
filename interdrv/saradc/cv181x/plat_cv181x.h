/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Created on Thu Mar 07 2024
 *
 * Copyright (c) 2024 Sophgo
 */

#ifndef	__PLAT_CV181X_H__
#define	__PLAT_CV181X_H__
#include "cvi_saradc.h"
#include "pinctrl-cv181x.h"

#define	SARADC_CHAN_NUM	 6

struct cvi_saradc_device {
	struct device *dev;
	struct reset_control *rst_saradc;
	struct iio_chan_spec iio_channels[SARADC_CHAN_NUM];
	struct clk *clk_saradc;
	void __iomem *saradc_vaddr;
	void __iomem *top_saradc_base_addr;
	void __iomem *rtcsys_saradc_base_addr;
	int	saradc_irq;
	spinlock_t close_lock;
	bool enable[SARADC_CHAN_NUM];
	void *private_data;
	int	channel_index;
	u32 saradc_saved_top_regs[SARADC_REGS_NUM];
	u32 saradc_saved_rtc_regs[SARADC_REGS_NUM];
};

#endif /* __PLAT_CV181X_H__ */

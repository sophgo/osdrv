/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cv183x_jpeg_cfg.c
 * Description: jpeg cv183x implementation
 */

#include <linux/module.h>
#include "cvi_jpeg.h"
#include "jpeg_common.h"

const struct jpu_ops cv1835_asic_jpu_ops = {
	.clk_get = jpu_clk_get,
	.clk_put = jpu_clk_put,
	.clk_enable = jpu_clk_enable,
	.clk_disable = jpu_clk_disable,
	.config_pll = cv1835_config_pll,
};

const struct jpu_pltfm_data cv1835_asic_jpu_pdata = {
	.ops = &cv1835_asic_jpu_ops,
	.quirks = JPU_QUIRK_SUPPORT_CLOCK_CONTROL,
	.version = 0x1835,
};

const struct of_device_id cvi_jpu_match_table[] = {
	{ .compatible = "cvitek,jpeg", .data = &cv1835_asic_jpu_pdata},			// for 1835_asic
	{},
};

MODULE_DEVICE_TABLE(of, cvi_jpu_match_table);

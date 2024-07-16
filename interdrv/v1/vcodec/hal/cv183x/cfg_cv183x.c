/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cfg_cv183x.c
 * Description:
 */

#include <linux/cdev.h>
#include "cvi_vcodec.h"
#include "vcodec_common.h"

const struct vpu_ops cv1835_asic_vpu_ops = {
	.clk_get = vpu_clk_get,
	.clk_put = vpu_clk_put,
	.clk_enable = vpu_clk_enable,
	.clk_disable = vpu_clk_disable,
	.clk_get_rate = vpu_clk_get_rate,
	.config_ddr = cviConfigDDR,
};

const struct vpu_pltfm_data cv1835_asic_vpu_pdata = {
	.ops = &cv1835_asic_vpu_ops,
	.quirks = VCODEC_QUIRK_SUPPORT_CLOCK_CONTROL | VCODEC_QUIRK_SUPPORT_SWITCH_TO_PLL
		| VCODEC_QUIRK_SUPPORT_REMAP_DDR,
	.version = 0x1835,
};

const struct of_device_id cvi_vpu_match_table[] = {
	{ .compatible = "cvitek,vcodec", .data = &cv1835_asic_vpu_pdata},		// for 1835 asic
	{},
};

MODULE_DEVICE_TABLE(of, cvi_vpu_match_table);

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cv182x_jpeg_cfg.c
 * Description: jpeg cv182x implementation
 */

#include <linux/module.h>
#include <linux/clk.h>
#include "cvi_jpeg.h"
#include "jpeg_common.h"

static void jpu_clk_enable_1822(struct cvi_jpu_device *jdev);
static void jpu_clk_disable_1822(struct cvi_jpu_device *jdev);

const struct jpu_ops cv1822_asic_jpu_ops = {
	.clk_get = jpu_clk_get,
	.clk_put = jpu_clk_put,
	.clk_enable = jpu_clk_enable_1822,
	.clk_disable = jpu_clk_disable_1822,
	.config_pll = NULL,
};

static const struct jpu_pltfm_data cv1822_fpga_jpu_pdata = {
	.ops = NULL,
	.quirks = 0,
	.version = 0x1822,
};

static const struct jpu_pltfm_data cv1822_pxp_jpu_pdata = {
	.ops = NULL,
	.quirks = 0,
	.version = 0x1822,
};

static const struct jpu_pltfm_data cv1822_asic_jpu_pdata = {
	.ops = &cv1822_asic_jpu_ops,
	.quirks = JPU_QUIRK_SUPPORT_CLOCK_CONTROL,
	.version = 0x1822,
};

const struct of_device_id cvi_jpu_match_table[] = {
	{ .compatible = "cvitek,cv1822-fpga-jpeg", .data = &cv1822_fpga_jpu_pdata},	// for 1822 fpga
	{ .compatible = "cvitek,cv1822-pxp-jpeg", .data = &cv1822_pxp_jpu_pdata},	// for 1822 pxp
	{ .compatible = "cvitek,cv1822-asic-jpeg", .data = &cv1822_asic_jpu_pdata},	// for 1822 asic
	{},
};

MODULE_DEVICE_TABLE(of, cvi_jpu_match_table);

static void jpu_clk_enable_1822(struct cvi_jpu_device *jdev)
{
	WARN_ON(!jdev->clk_jpeg);
	WARN_ON(!jdev->clk_apb_jpeg);
	WARN_ON(!jdev->clk_vc_src0);
	WARN_ON(!jdev->clk_cfg_reg_vc);
	WARN_ON(!jdev->clk_axi_video_codec);

	clk_prepare_enable(jdev->clk_jpeg);
	clk_prepare_enable(jdev->clk_apb_jpeg);

	clk_prepare_enable(jdev->clk_vc_src0);
	clk_prepare_enable(jdev->clk_cfg_reg_vc);
	clk_prepare_enable(jdev->clk_axi_video_codec);
}

static void jpu_clk_disable_1822(struct cvi_jpu_device *jdev)
{
	if (jpu_mask & JPU_MASK_DISABLE_CLK_GATING)
		return;

	clk_disable_unprepare(jdev->clk_jpeg);
	clk_disable_unprepare(jdev->clk_apb_jpeg);

	clk_disable_unprepare(jdev->clk_vc_src0);
	clk_disable_unprepare(jdev->clk_cfg_reg_vc);
	clk_disable_unprepare(jdev->clk_axi_video_codec);
}


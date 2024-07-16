/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cfg_cv182x.c
 * Description: camera interface driver
 */

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/io.h>
#include "cvi_vcodec.h"
#include "vcodec_common.h"

static void vpu_clk_enable_1822(struct cvi_vpu_device *vdev, int mask);
static void vpu_clk_disable_1822(struct cvi_vpu_device *vdev, int mask);

const struct vpu_ops cv1822_asic_vpu_ops = {
	.clk_get = vpu_clk_get,
	.clk_put = vpu_clk_put,
	.clk_enable = vpu_clk_enable_1822,
	.clk_disable = vpu_clk_disable_1822,
	.config_ddr = NULL,
	.clk_get_rate = vpu_clk_get_rate,
};

static const struct vpu_pltfm_data cv1822_fpga_vpu_pdata = {
	.ops = NULL,
	.quirks = VCODEC_QUIRK_SUPPORT_VC_CTRL_REG,
	.version = 0x1822,
};

static const struct vpu_pltfm_data cv1822_pxp_vpu_pdata = {
	.ops = NULL,
	.quirks = VCODEC_QUIRK_SUPPORT_VC_CTRL_REG,
	.version = 0x1822,
};

static const struct vpu_pltfm_data cv1822_asic_vpu_pdata = {
	.ops = &cv1822_asic_vpu_ops,
	.quirks = VCODEC_QUIRK_SUPPORT_VC_CTRL_REG | VCODEC_QUIRK_SUPPORT_CLOCK_CONTROL,
	.version = 0x1822,
};

const struct of_device_id cvi_vpu_match_table[] = {
	{ .compatible = "cvitek,cv1822-fpga-vcodec", .data = &cv1822_fpga_vpu_pdata},	// for 1822 fpga
	{ .compatible = "cvitek,cv1822-pxp-vcodec", .data = &cv1822_pxp_vpu_pdata},	// for 1822 pxp
	{ .compatible = "cvitek,cv1822-asic-vcodec", .data = &cv1822_asic_vpu_pdata},	// for 1822 asic
	{},
};

MODULE_DEVICE_TABLE(of, cvi_vpu_match_table);

static void vpu_clk_enable_1822(struct cvi_vpu_device *vdev, int mask)
{
	vpudrv_buffer_t *pReg = &vcodec_dev.ctrl_register;

	VCODEC_DBG_TRACE("mask = 0x%X\n", mask);

	WARN_ON(!vdev->clk_h264c);
	WARN_ON(!vdev->clk_apb_h264c);
	WARN_ON(!vdev->clk_h265c);
	WARN_ON(!vdev->clk_apb_h265c);

	WARN_ON(!vdev->clk_vc_src0);
	WARN_ON(!vdev->clk_cfg_reg_vc);
	WARN_ON(!vdev->clk_axi_video_codec);

	if ((mask & BIT(H264_CORE_IDX)) && (mask & BIT(H265_CORE_IDX))) {
		VCODEC_DBG_ERR("not supported mask in 182x\n");
		return;
	}

	clk_prepare_enable(vdev->clk_vc_src0);
	clk_prepare_enable(vdev->clk_cfg_reg_vc);
	clk_prepare_enable(vdev->clk_axi_video_codec);

	// sram share config
	if (mask & BIT(H264_CORE_IDX)) {
		// devmem 0xb030024 32 1
		writel(0x1, pReg->virt_addr + 0x24);
	} else if (mask & BIT(H265_CORE_IDX)) {
		// devmem 0xb030024 32 2
		writel(0x2, pReg->virt_addr + 0x24);
	}

	// Enable both H264/H265 clk first
	// then to disable unnecessary one
	clk_prepare_enable(vdev->clk_h264c);
	clk_prepare_enable(vdev->clk_apb_h264c);
	clk_prepare_enable(vdev->clk_h265c);
	clk_prepare_enable(vdev->clk_apb_h265c);

	if (mask & BIT(H264_CORE_IDX)) {
		clk_disable_unprepare(vdev->clk_h265c);
		clk_disable_unprepare(vdev->clk_apb_h265c);
	} else if (mask & BIT(H265_CORE_IDX)) {
		clk_disable_unprepare(vdev->clk_h264c);
		clk_disable_unprepare(vdev->clk_apb_h264c);
	}
}

static void vpu_clk_disable_1822(struct cvi_vpu_device *vdev, int mask)
{
	if (vcodec_mask & VCODEC_MASK_DISABLE_CLK_GATING) {
		return;
	}

	if ((mask & BIT(H264_CORE_IDX)) && (mask & BIT(H265_CORE_IDX))) {
		VCODEC_DBG_ERR("not supported mask in 182x\n");
		return;
	}

	if (mask & BIT(H264_CORE_IDX) && __clk_is_enabled(vdev->clk_h264c)) {
		clk_disable_unprepare(vdev->clk_h264c);
		clk_disable_unprepare(vdev->clk_apb_h264c);
	} else if (mask & BIT(H265_CORE_IDX) && __clk_is_enabled(vdev->clk_h265c)) {
		clk_disable_unprepare(vdev->clk_h265c);
		clk_disable_unprepare(vdev->clk_apb_h265c);
	}

	clk_disable_unprepare(vdev->clk_vc_src0);
	clk_disable_unprepare(vdev->clk_cfg_reg_vc);
	clk_disable_unprepare(vdev->clk_axi_video_codec);
}

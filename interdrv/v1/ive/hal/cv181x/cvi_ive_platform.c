/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: cvi_ive_platform.c
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/timer.h>

#include "cvi_ive_platform.h"

#define TIMEOUT_MS (1 * 1000)

IVE_DMA_C _ive_dma_c;
IVE_CCL_C _ive_ccl_c;
IVE_GMM_C _ive_gmm_c;
IVE_UPDATE_BG_C _ive_update_bg_c;
IVE_SAD_C _ive_sad_c;
IVE_MATCH_BG_C _ive_match_bg_c;
IVE_MAP_C _ive_map_c;

// use rdma/wdma (one channel)
ISP_DMA_CTL_C _isp_dma_ctl_c;
ISP_RDMA_C _isp_rdma_c;
ISP_WDMA_C _isp_wdma_c;

// img_in/sc_odma/intg/hist/dmaf
// IMG_IN_C _img_in_c;
SC_ODMA_C _sc_odma_c;
IVE_INTG_C _ive_intg_c;
IVE_HIST_C _ive_hist_c;

static char gLog[2048];
void dump_ive_cmd(void)
{
	pr_info("%s", gLog);
}

void writed(uint32_t data, void *addr, char *name)
{
	sprintf(gLog + strlen(gLog), "devmem 0x%08x 32 0x%08x ; BLK: %s\n",
		(unsigned int)addr, data, name);
	writel(data, (void __iomem *)addr);
}

unsigned short FloatToShort(float x)
{
	unsigned short y;

	y = (unsigned short)((*(unsigned int *)(&x)) >> 16);
	return y;
}

float ShortToFloat(unsigned short x)
{
	float y;
	unsigned int temp;

	temp = ((unsigned int)(x << 16));
	y = (float)(*((float *)(&temp)));
	return y;
}

union convert_type_float {
	float fval;
	uint16_t bf16[2];
	uint32_t ival;
};

typedef union convert_type_float convert_int_float;
uint16_t convert_fp32_bf16(float fp32)
{
	//if (float_isnan(fp32)) return 0x7FC0;
	uint32_t input, lsb, rounding_bias;
	convert_int_float convert_val;
	convert_val.fval = fp32;
	input = convert_val.ival;
	lsb = (input >> 16) & 1;
	rounding_bias = 0x7fff + lsb;
	input += rounding_bias;
	convert_val.bf16[1] = (uint16_t)(input >> 16);

	// HW behavior
	if ((convert_val.bf16[1] & 0x7f80) == 0x7f80) {
		convert_val.bf16[1] = 0x7f7f;
	}
	return convert_val.bf16[1];
}

int getImgFmtSel(IVE_IMAGE_TYPE_E enType)
{
	int r = CVI_FAILURE;

	switch (enType) {
	case IVE_IMAGE_TYPE_U8C1:
		r = 5;
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		r = 2;
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		r = 3;
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		r = 8;
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		r = 0xa;
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		pr_info("check setting for S16C1/U16C1\n");
		r = 5;
		break;
	default:
		// TODO: check YUV422SP set to 0xa
		pr_info("not support src type\n");
		break;
	}
	return r;
}

void ive_set_int(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c,
		 bool isEnable)
{
	ive_top_c->REG_94.reg_intr_en_hist = isEnable;
	ive_top_c->REG_94.reg_intr_en_intg = isEnable;
	ive_top_c->REG_94.reg_intr_en_sad = isEnable;
	ive_top_c->REG_94.reg_intr_en_ncc = isEnable;
	ive_top_c->REG_94.reg_intr_en_filterop_odma = isEnable;
	ive_top_c->REG_94.reg_intr_en_filterop_wdma_y = isEnable;
	ive_top_c->REG_94.reg_intr_en_filterop_wdma_c = isEnable;
	ive_top_c->REG_94.reg_intr_en_dmaf = isEnable;
	ive_top_c->REG_94.reg_intr_en_ccl = isEnable;
	ive_top_c->REG_94.reg_intr_en_lk = isEnable;
	writed(ive_top_c->REG_94.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_94),
	       "IVE_TOP_REG_94 reset");
}

void ive_reset(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c)
{
	// disable
	ive_set_int(ndev, ive_top_c, 0);

	// default disable it
	ive_top_c->REG_h10.reg_img_in_top_enable = 0;
	ive_top_c->REG_h10.reg_resize_top_enable = 0;
	ive_top_c->REG_h10.reg_gmm_top_enable = 0;
	ive_top_c->REG_h10.reg_csc_top_enable = 0;
	ive_top_c->REG_h10.reg_rdma_img1_top_enable = 0;
	ive_top_c->REG_h10.reg_bgm_top_enable = 0;
	ive_top_c->REG_h10.reg_bgu_top_enable = 0;
	ive_top_c->REG_h10.reg_r2y4_top_enable = 0;
	ive_top_c->REG_h10.reg_map_top_enable = 0;
	ive_top_c->REG_h10.reg_rdma_eigval_top_enable = 0;
	ive_top_c->REG_h10.reg_thresh_top_enable = 0;
	ive_top_c->REG_h10.reg_hist_top_enable = 0;
	ive_top_c->REG_h10.reg_intg_top_enable = 0;
	ive_top_c->REG_h10.reg_ncc_top_enable = 0;
	ive_top_c->REG_h10.reg_sad_top_enable = 0;
	ive_top_c->REG_h10.reg_filterop_top_enable = 0;
	ive_top_c->REG_h10.reg_dmaf_top_enable = 0;
	ive_top_c->REG_h10.reg_ccl_top_enable = 0;
	ive_top_c->REG_h10.reg_lk_top_enable = 0;
	writed(ive_top_c->REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10 reset");
	// enable
	// ive_set_int(ndev, ive_top_c, 1);
}

int setImgDst1(struct cvi_ive_device *ndev, IVE_DST_IMAGE_S *dst_img)
{
	int swMode = 0;
	DEFINE_ISP_DMA_CTL_C(isp_dma_ctl_c);

	isp_dma_ctl_c.BASE_ADDR.reg_basel = dst_img->u64PhyAddr[0] & 0xffffffff;
	writed(isp_dma_ctl_c.BASE_ADDR.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40 +
		ISP_DMA_CTL_BASE_ADDR),
	       "ISP_DMA_CTL_BASE_ADDR");

	isp_dma_ctl_c.SYS_CONTROL.reg_baseh =
		(dst_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	isp_dma_ctl_c.SYS_CONTROL.reg_stride_sel = swMode;
	isp_dma_ctl_c.SYS_CONTROL.reg_seglen_sel = swMode;
	isp_dma_ctl_c.SYS_CONTROL.reg_segnum_sel = swMode;

	if (swMode) {
		// set height
		isp_dma_ctl_c.DMA_SEGNUM.reg_segnum = dst_img->u16Height;
		writed(isp_dma_ctl_c.DMA_SEGNUM.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40 +
			ISP_DMA_CTL_DMA_SEGNUM),
		       "ISP_DMA_CTL_DMA_SEGNUM");
		// set width
		isp_dma_ctl_c.DMA_SEGLEN.reg_seglen = dst_img->u16Width;
		writed(isp_dma_ctl_c.DMA_SEGLEN.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40 +
			ISP_DMA_CTL_DMA_SEGLEN),
		       "ISP_DMA_CTL_DMA_SEGLEN");
		// set stride
		isp_dma_ctl_c.DMA_STRIDE.reg_stride = dst_img->u16Stride[0];
		writed(isp_dma_ctl_c.DMA_STRIDE.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40 +
			ISP_DMA_CTL_DMA_STRIDE),
		       "ISP_DMA_CTL_DMA_STRIDE");
		// FIXME: set U8/S8
	} else {
		// hw mode, no need to set height/width/stride
	}

	isp_dma_ctl_c.SYS_CONTROL.reg_base_sel = 1; // sw specify address
	writed(isp_dma_ctl_c.SYS_CONTROL.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40 +
		ISP_DMA_CTL_SYS_CONTROL),
	       "ISP_DMA_CTL_SYS_CONTROL");

	isp_dma_ctl_printk(
		(ISP_DMA_CTL_C *)(ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40));
	return CVI_SUCCESS;
}

int setImgDst2(struct cvi_ive_device *ndev, IVE_DST_IMAGE_S *dst_img)
{
	DEFINE_ISP_DMA_CTL_C(wdma_c_ctl_c);

	wdma_c_ctl_c.BASE_ADDR.reg_basel = dst_img->u64PhyAddr[0] & 0xffffffff;
	writed(wdma_c_ctl_c.BASE_ADDR.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x80 +
		ISP_DMA_CTL_BASE_ADDR),
	       "ISP_DMA_CTL_BASE_ADDR");
	wdma_c_ctl_c.SYS_CONTROL.reg_baseh =
		(dst_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	wdma_c_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writed(wdma_c_ctl_c.SYS_CONTROL.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x80 +
		ISP_DMA_CTL_SYS_CONTROL),
	       "ISP_DMA_CTL_SYS_CONTROL");

	isp_dma_ctl_printk(
		(ISP_DMA_CTL_C *)(ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x80));
	return CVI_SUCCESS;
}

int setImgSrc1(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *src_img)
{
	DEFINE_IMG_IN_C(img_in_c);

	img_in_c.REG_03.reg_src_y_pitch = src_img->u16Stride[0]; //0x160 ok
	writed(img_in_c.REG_03.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_03),
	       "IMG_IN_REG_03");
	// NOTICE: need to set 1 to valify settings immediantly
	// img_in_c.REG_05.reg_shrd_sel = 0;
	img_in_c.REG_05.reg_shrd_sel = 1;
	writed(img_in_c.REG_05.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_05),
	       "IMG_IN_REG_05");
	img_in_c.REG_02.reg_src_wd = src_img->u16Width - 1;
	img_in_c.REG_02.reg_src_ht = src_img->u16Height - 1;
	writed(img_in_c.REG_02.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_02),
	       "IMG_IN_REG_02");
	// Burst len unit is byte, 8 means 128bit
	img_in_c.REG_00.reg_burst_ln = 8;
	img_in_c.REG_00.reg_src_sel = 2; // 2 for others: DRMA
	img_in_c.REG_00.reg_fmt_sel = getImgFmtSel(src_img->enType);
	writed(img_in_c.REG_00.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_00),
	       "IMG_IN_REG_00");
	img_in_c.REG_Y_BASE_0.reg_src_y_base_b0 =
		(src_img->u64PhyAddr[0] & 0xffffffff);
	writed(img_in_c.REG_Y_BASE_0.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_Y_BASE_0),
	       "IMG_IN_REG_Y_BASE_0");
	img_in_c.REG_Y_BASE_1.reg_src_y_base_b1 =
		(src_img->u64PhyAddr[0] >> 32 & 0xffffffff);
	writed(img_in_c.REG_Y_BASE_1.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_Y_BASE_1),
	       "IMG_IN_REG_Y_BASE_1");
	switch (src_img->enType) {
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U8C1:
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		img_in_c.REG_04.reg_src_c_pitch = src_img->u16Stride[0];
		writed(img_in_c.REG_04.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_04),
		       "IMG_IN_REG_04");
		img_in_c.REG_U_BASE_0.reg_src_u_base_b0 =
			(src_img->u64PhyAddr[1] & 0xffffffff);
		writed(img_in_c.REG_U_BASE_0.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_U_BASE_0),
		       "IMG_IN_REG_U_BASE_0");
		img_in_c.REG_U_BASE_1.reg_src_u_base_b1 =
			((src_img->u64PhyAddr[1] >> 32) & 0xffffffff);
		writed(img_in_c.REG_U_BASE_1.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_U_BASE_1),
		       "IMG_IN_REG_U_BASE_1");
		img_in_c.REG_V_BASE_0.reg_src_v_base_b0 =
			(src_img->u64PhyAddr[2] & 0xffffffff);
		writed(img_in_c.REG_V_BASE_0.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_V_BASE_0),
		       "IMG_IN_REG_V_BASE_0");
		img_in_c.REG_V_BASE_1.reg_src_v_base_b1 =
			((src_img->u64PhyAddr[2] >> 32) & 0xffffffff);
		writed(img_in_c.REG_V_BASE_1.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_V_BASE_1),
		       "IMG_IN_REG_V_BASE_1");
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
	case IVE_IMAGE_TYPE_YUV422SP:
		img_in_c.REG_04.reg_src_c_pitch = src_img->u16Stride[0];
		writed(img_in_c.REG_04.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_04),
		       "IMG_IN_REG_04");
		img_in_c.REG_U_BASE_0.reg_src_u_base_b0 =
			(src_img->u64PhyAddr[1] & 0xffffffff);
		writed(img_in_c.REG_U_BASE_0.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_U_BASE_0),
		       "IMG_IN_REG_U_BASE_0");
		img_in_c.REG_U_BASE_1.reg_src_u_base_b1 =
			((src_img->u64PhyAddr[1] >> 32) & 0xffffffff);
		writed(img_in_c.REG_U_BASE_1.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_U_BASE_0),
		       "IMG_IN_REG_U_BASE_1");
		img_in_c.REG_V_BASE_0.reg_src_v_base_b0 =
			((src_img->u64PhyAddr[1] + 1) & 0xffffffff);
		writed(img_in_c.REG_V_BASE_0.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_V_BASE_0),
		       "IMG_IN_REG_V_BASE_0");
		img_in_c.REG_V_BASE_1.reg_src_v_base_b1 =
			(((src_img->u64PhyAddr[1] + 1) >> 32) & 0xffffffff);
		writed(img_in_c.REG_V_BASE_1.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN +
			IMG_IN_REG_V_BASE_1),
		       "IMG_IN_REG_V_BASE_1");
		break;
	default:
		pr_info("[IVE] not support src type");
		return CVI_FAILURE;
	}
	img_in_printk((IMG_IN_C *)(ndev->ive_base + IVE_BLK_BA_IMG_IN));
	return CVI_SUCCESS;
}

int setImgSrc2(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *src_img)
{
	DEFINE_ISP_DMA_CTL_C(rdma_img1_ctl_c);

	rdma_img1_ctl_c.BASE_ADDR.reg_basel =
		src_img->u64PhyAddr[0] & 0xffffffff;
	writed(rdma_img1_ctl_c.BASE_ADDR.val,
	       (ndev->ive_base + IVE_BLK_BA_RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR),
	       "ISP_DMA_CTL_BASE_ADDR");

	rdma_img1_ctl_c.SYS_CONTROL.reg_baseh =
		(src_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	rdma_img1_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writed(rdma_img1_ctl_c.SYS_CONTROL.val,
	       (ndev->ive_base + IVE_BLK_BA_RDMA_IMG1 +
		ISP_DMA_CTL_SYS_CONTROL),
	       "ISP_DMA_CTL_SYS_CONTROL");

	pr_info("Src2 address: 0x%x %08x\n",
		rdma_img1_ctl_c.SYS_CONTROL.reg_baseh,
		rdma_img1_ctl_c.BASE_ADDR.reg_basel);
	isp_dma_ctl_printk(
		(ISP_DMA_CTL_C *)(ndev->ive_base + IVE_BLK_BA_RDMA_IMG1));
	return CVI_SUCCESS;
}

int setOdma(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *dst_img, int w, int h)
{
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	ive_filterop_c.ODMA_REG_00.reg_fmt_sel = getImgFmtSel(dst_img->enType);
	writed(ive_filterop_c.ODMA_REG_00.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_00),
	       "IVE_FILTEROP_ODMA_REG_00");
	switch (dst_img->enType) {
	case IVE_IMAGE_TYPE_YUV420SP:
		// NV21
	case IVE_IMAGE_TYPE_YUV422SP:
		ive_filterop_c.ODMA_REG_03.reg_dma_u_base_low_part =
			dst_img->u64PhyAddr[1] & 0xffffffff;
		ive_filterop_c.ODMA_REG_04.reg_dma_u_base_high_part =
			(dst_img->u64PhyAddr[1] >> 32) & 0xffffffff;
		ive_filterop_c.ODMA_REG_05.reg_dma_v_base_low_part =
			(dst_img->u64PhyAddr[1] + 1) & 0xffffffff;
		ive_filterop_c.ODMA_REG_06.reg_dma_v_base_high_part =
			((dst_img->u64PhyAddr[1] + 1) >> 32) & 0xffffffff;
		ive_filterop_c.ODMA_REG_08.reg_dma_c_pitch =
			dst_img->u16Stride[0];
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		ive_filterop_c.ODMA_REG_03.reg_dma_u_base_low_part =
			dst_img->u64PhyAddr[1] & 0xffffffff;
		ive_filterop_c.ODMA_REG_04.reg_dma_u_base_high_part =
			(dst_img->u64PhyAddr[1] >> 32) & 0xffffffff;
		ive_filterop_c.ODMA_REG_05.reg_dma_v_base_low_part =
			(dst_img->u64PhyAddr[2]) & 0xffffffff;
		ive_filterop_c.ODMA_REG_06.reg_dma_v_base_high_part =
			((dst_img->u64PhyAddr[2]) >> 32) & 0xffffffff;
		ive_filterop_c.ODMA_REG_08.reg_dma_c_pitch =
			dst_img->u16Stride[0];
		break;
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		ive_filterop_c.ODMA_REG_03.reg_dma_u_base_low_part = 0;
		ive_filterop_c.ODMA_REG_04.reg_dma_u_base_high_part = 0;
		ive_filterop_c.ODMA_REG_05.reg_dma_v_base_low_part = 0;
		ive_filterop_c.ODMA_REG_06.reg_dma_v_base_high_part = 0;
		ive_filterop_c.ODMA_REG_08.reg_dma_c_pitch = 0;
		break;
	default:
		pr_info("[IVE] GMM, not support dstEnType");
		return CVI_FAILURE;
	}
	writed(ive_filterop_c.ODMA_REG_03.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_03),
	       "IVE_FILTEROP_ODMA_REG_03");
	writed(ive_filterop_c.ODMA_REG_04.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_04),
	       "IVE_FILTEROP_ODMA_REG_04");
	writed(ive_filterop_c.ODMA_REG_05.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_05),
	       "IVE_FILTEROP_ODMA_REG_05");
	writed(ive_filterop_c.ODMA_REG_06.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_06),
	       "IVE_FILTEROP_ODMA_REG_06");
	writed(ive_filterop_c.ODMA_REG_08.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_08),
	       "IVE_FILTEROP_ODMA_REG_08");
	// odma for filter3ch + csc
	ive_filterop_c.ODMA_REG_01.reg_dma_y_base_low_part =
		dst_img->u64PhyAddr[0] & 0xffffffff;
	writed(ive_filterop_c.ODMA_REG_01.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_01),
	       "IVE_FILTEROP_ODMA_REG_01");
	ive_filterop_c.ODMA_REG_02.reg_dma_y_base_high_part =
		(dst_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	writed(ive_filterop_c.ODMA_REG_02.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_02),
	       "IVE_FILTEROP_ODMA_REG_02");

	ive_filterop_c.ODMA_REG_07.reg_dma_y_pitch = dst_img->u16Stride[0];
	if (dst_img->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
		ive_filterop_c.ODMA_REG_07.reg_dma_y_pitch =
			3 * dst_img->u16Stride[0];
	}
	writed(ive_filterop_c.ODMA_REG_07.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_07),
	       "IVE_FILTEROP_ODMA_REG_07");
	ive_filterop_c.ODMA_REG_11.reg_dma_wd = w - 1;
	writed(ive_filterop_c.ODMA_REG_11.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_11),
	       "IVE_FILTEROP_ODMA_REG_11");
	ive_filterop_c.ODMA_REG_12.reg_dma_ht = h - 1;
	writed(ive_filterop_c.ODMA_REG_12.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_12),
	       "IVE_FILTEROP_ODMA_REG_12");
	// trigger odma
	ive_filterop_c.ODMA_REG_00.reg_dma_blen = 1;
	ive_filterop_c.ODMA_REG_00.reg_dma_en = 1;
	writed(ive_filterop_c.ODMA_REG_00.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_00),
	       "IVE_FILTEROP_ODMA_REG_00");
	// enable it
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_go(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c,
		   bool bInstant)
{
	int cnt = 0;
	uint32_t val = 0;
	uint32_t mask = 0;
	//long leavetime = 0;

	// iveReg->bInstant == true means busy wait
	ive_top_c->REG_94.reg_intr_en_filterop_odma = !bInstant;
	ive_top_c->REG_94.reg_intr_en_filterop_wdma_y = !bInstant;
	ive_top_c->REG_94.reg_intr_en_filterop_wdma_c = !bInstant;
	writed(ive_top_c->REG_94.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_94),
	       "IVE_TOP_REG_94");

	// GoGoGo
	ive_top_c->REG_1.reg_fmt_vld_fg = 1;
	writed(ive_top_c->REG_1.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_1),
	       "IVE_TOP_REG_1");
	/*
	leavetime = wait_for_completion_interruptible_timeout(&ndev->ive_done, msecs_to_jiffies(TIMEOUT_MS));
	if (leavetime == 0) {
	pr_info("[IVE] stop by timeout\n");
	//return -ETIMEDOUT;
	}else if (leavetime < 0) {
	pr_info("[IVE] stop by interrupted\n");
	//return leavetime;
	}else {
	pr_info("[IVE] leavetime is = %ld\n", leavetime);
	}
*/
	if (bInstant) {
		pr_info("bInstant go\n");
		while (!(readl(ndev->ive_base + IVE_BLK_BA_IVE_TOP +
			       IVE_TOP_REG_90) &
			 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK) &&
		       cnt < 6) {
			msleep(500);
			val = readl(ndev->ive_base + IVE_BLK_BA_IVE_TOP +
				    IVE_TOP_REG_90);
			pr_info("bInstant wait for 0x%08x, expect val is 0x%08x",
				val,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK);
			cnt++;
		}

	} else { //
		// schedule
		pr_info("schedule go\n");
		mask = IVE_TOP_REG_INTR_STATUS_FILTEROP_ODMA_MASK |
		       IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_Y_MASK |
		       IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_C_MASK;
		pr_info("mask = 0x%08x\n", mask);
		while (((readl(ndev->ive_base + IVE_BLK_BA_IVE_TOP +
			       IVE_TOP_REG_98) &
			 mask) != mask) &&
		       cnt < 6) {
			msleep(500);
			val = readl(ndev->ive_base + IVE_BLK_BA_IVE_TOP +
				    IVE_TOP_REG_98);
			pr_info("wait for 0x%08x, expect val is 0x%08x", val,
				mask);
			cnt++;
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_base_op(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			bool bInstant, int op, void *pstCtrl)
{
	struct ive_block *IVE_BLK_BA = (struct ive_block *)ndev->IVE_BLK_BA;
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	pr_info("Input image (w=%d,h=%d,s=%d), IVE Base Addr : 0x%08x\n",
		pstSrc1->u16Width, pstSrc1->u16Height, pstSrc1->u16Stride[0],
		(unsigned int)ndev->ive_base);

	ive_reset(ndev, &ive_top_c);
	ive_top_c.REG_2.reg_img_heightm1 = pstSrc1->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstSrc1->u16Width - 1;
	writed(ive_top_c.REG_2.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");
	// setting
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	writed(ive_top_c.REG_h80.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");
	ive_top_c.REG_R2Y4_14.reg_csc_r2y4_enable = 0;
	writed(ive_top_c.REG_R2Y4_14.val,
	       (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_R2Y4_14),
	       "IVE_TOP_REG_R2Y4_14");

	if (op == 3 && pstCtrl != NULL) { //0:and, 1:or, 2:xor, 3:add, 4:sub
		// TODO: test that set rounding and clip
		ive_top_c.REG_20.reg_frame2op_add_mode_rounding = 1;
		ive_top_c.REG_20.reg_frame2op_add_mode_clipping = 1;
		ive_top_c.REG_21.reg_fram2op_x_u0q16 = convert_fp32_bf16(
			((struct IVE_ADD_CTRL_S *)pstCtrl)->aX);
		ive_top_c.REG_21.reg_fram2op_y_u0q16 = convert_fp32_bf16(
			((struct IVE_ADD_CTRL_S *)pstCtrl)
				->bY); //(CVI_U16)((struct IVE_ADD_CTRL_S *)pstCtrl)->bY;
		writed(ive_top_c.REG_21.val,
		       (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_21),
		       "IVE_TOP_REG_21");
	} else if (op == 4 && pstCtrl != NULL) {
		ive_top_c.REG_20.reg_frame2op_sub_change_order = 0;
		ive_top_c.REG_20.reg_frame2op_sub_switch_src = 0;
		ive_top_c.REG_20.reg_frame2op_sub_mode =
			*((CVI_U8 *)pstCtrl) & 0x01;
	}

	ive_top_c.REG_20.reg_frame2op_op_mode = op;
	writed(ive_top_c.REG_20.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	setImgSrc1(ndev, pstSrc1);
	setImgSrc2(ndev, pstSrc2);
	setImgDst1(ndev, pstDst);

	// FIXME: need to set reg_ive_rdma_img1_mod_u8 in two input?
	switch (pstSrc1->enType) {
	case IVE_IMAGE_TYPE_U8C1:
		ive_top_c.REG_3.reg_ive_rdma_img1_mod_u8 = 1;
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		// FIXME: check it
		ive_top_c.REG_3.reg_ive_rdma_img1_mod_u8 = 0;
		break;
	default:
		pr_info("[IVE] not support src type");
		return CVI_FAILURE;
		break;
	}
	// TODO: need to set vld?
	ive_top_c.REG_3.reg_imgmux_img0_sel = 0;
	ive_top_c.REG_3.reg_ive_rdma_img1_en = 1;
	writed(ive_top_c.REG_3.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_3),
	       "IVE_TOP_REG_3");

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.REG_h14.val,
	       (IVE_BLK_BA->FILTEROP + IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writed(ive_filterop_c.REG_h10.val,
	       (IVE_BLK_BA->FILTEROP + IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");

	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0; // sw_ovw; bypass op1
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writed(ive_filterop_c.REG_h14.val,
	       (IVE_BLK_BA->FILTEROP + IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0; // bypass op2
	writed(ive_filterop_c.REG_28.val,
	       (IVE_BLK_BA->FILTEROP + IVE_FILTEROP_REG_28),
	       "IVE_FILTEROP_REG_28");

	ive_top_c.REG_h10.reg_filterop_top_enable = 1;
	ive_top_c.REG_h10.reg_rdma_img1_top_enable = 1;
	writed(ive_top_c.REG_h10.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	writed(ive_top_c.REG_h130.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_H130),
	       "IVE_TOP_REG_H130");
	writed(ive_top_c.REG_h14c.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_H14C),
	       "IVE_TOP_REG_H14C");
	writed(ive_top_c.REG_h150.val, (IVE_BLK_BA->IVE_TOP + IVE_TOP_REG_H150),
	       "IVE_TOP_REG_H150");

	cvi_ive_go(ndev, &ive_top_c, bInstant);

	ive_top_printk((IVE_TOP_C *)(IVE_BLK_BA->IVE_TOP));
	ive_filterop_printk((IVE_FILTEROP_C *)(IVE_BLK_BA->FILTEROP));
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_test(struct cvi_ive_device *ndev, char *addr, CVI_U16 *w,
		     CVI_U16 *h)
{
	//0:and
	//1:or
	//2:xor
	//3:add
	//4:sub
	int Width = 128;
	int Height = 128;
	IVE_IMAGE_S Src1, Src2, Dst1;
	IVE_ADD_CTRL_S pstAddCtrl;
	int *src1_addr, *src2_addr, *dst1_addr;

	// 0:and, 1:or, 2:xor, 3:add, 4:sub
	pr_info("[IVE] CVI_MPI_IVE_Add_ken modify\n");
	src1_addr = (int *)devm_kzalloc(ndev->dev, Width * Height, GFP_KERNEL);
	memset((CVI_U64 *)src1_addr, 0x55, Width * Height);
	src2_addr = (int *)devm_kzalloc(ndev->dev, Width * Height, GFP_KERNEL);
	memset((CVI_U64 *)src2_addr, 0xff, Width * Height);
	src2_addr[0] = 0x00;
	src2_addr[1] = 0x00;
	dst1_addr = (int *)devm_kzalloc(ndev->dev, Width * Height, GFP_KERNEL);
	memset((CVI_U64 *)dst1_addr, 0x99, Width * Height);

	Src1.u64PhyAddr[0] = virt_to_phys(src1_addr);
	Src2.u64PhyAddr[0] = virt_to_phys(src2_addr);
	Dst1.u64PhyAddr[0] = virt_to_phys(dst1_addr);

	Src1.u16Stride[0] = Width;
	Src2.u16Stride[0] = Width;
	Dst1.u16Stride[0] = Width;
	Src1.u16Width = Width;
	Src2.u16Width = Width;
	Dst1.u16Width = Width;
	Src1.u16Height = Height;
	Src2.u16Height = Height;
	Dst1.u16Height = Height;

	Src1.enType = IVE_IMAGE_TYPE_U8C1;
	pstAddCtrl.aX = 1.f;
	pstAddCtrl.bY = 1.f;

	reinit_completion(&ndev->ive_done);
	pr_info("OP 0:and, 1:or, 2:xor, 3:add, 4:sub  sel:%d\n", (*w));
	cvi_ive_base_op(ndev, &Src1, &Src2, &Dst1, 1, (*w),
			(void *)&pstAddCtrl);
	dump_ive_cmd();
	return CVI_SUCCESS;
}

CVI_S32 assign_ive_block_addr(struct cvi_ive_device *ndev)
{
	if (!ndev->ive_base)
		return CVI_FAILURE;
	pr_info("[IVE] assign_block_addr, Base Addr : 0x%08x\n",
		(unsigned int)ndev->ive_base);

	ndev->IVE_BLK_BA = (void *)devm_kzalloc(
		ndev->dev, sizeof(struct ive_block), GFP_KERNEL);
	((struct ive_block *)ndev->IVE_BLK_BA)->IVE_TOP =
		ndev->ive_base + IVE_BLK_BA_IVE_TOP; //(0X00000000)
	((struct ive_block *)ndev->IVE_BLK_BA)->IMG_IN =
		ndev->ive_base + IVE_BLK_BA_IMG_IN; //(0X00000400)
	((struct ive_block *)ndev->IVE_BLK_BA)->RDMA_IMG1 =
		ndev->ive_base + IVE_BLK_BA_RDMA_IMG1; //(0X00000500)
	((struct ive_block *)ndev->IVE_BLK_BA)->MAP =
		ndev->ive_base + IVE_BLK_BA_MAP; //(0X00000600)
	((struct ive_block *)ndev->IVE_BLK_BA)->HIST =
		ndev->ive_base + IVE_BLK_BA_HIST; //(0X00000700)
	((struct ive_block *)ndev->IVE_BLK_BA)->INTG =
		ndev->ive_base + IVE_BLK_BA_INTG; //(0X00000800)
	((struct ive_block *)ndev->IVE_BLK_BA)->SAD =
		ndev->ive_base + IVE_BLK_BA_SAD; //(0X00000900)
	((struct ive_block *)ndev->IVE_BLK_BA)->SAD_WDMA =
		ndev->ive_base + IVE_BLK_BA_SAD_WDMA; //(0X00000980)
	((struct ive_block *)ndev->IVE_BLK_BA)->SAD_WDMA_THR =
		ndev->ive_base + IVE_BLK_BA_SAD_WDMA_THR; //(0X00000A00)
	((struct ive_block *)ndev->IVE_BLK_BA)->NCC =
		ndev->ive_base + IVE_BLK_BA_NCC; //(0X00000B00)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_RDMA_0 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_0; //(0X00001000)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_RDMA_1 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_1; //(0X00001040)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_RDMA_2 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_2; //(0X00001080)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_RDMA_3 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_3; //(0X000010C0)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_RDMA_4 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_4; //(0X00001100)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_WDMA_0 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_WDMA_0; //(0X00001140)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_WDMA_1 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_WDMA_1; //(0X00001180)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_WDMA_2 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_WDMA_2; //(0X000011C0)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_WDMA_3 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_WDMA_3; //(0X00001200)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MODEL_WDMA_4 =
		ndev->ive_base + IVE_BLK_BA_GMM_MODEL_WDMA_4; //(0X00001240)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM_MATCH_WDMA =
		ndev->ive_base + IVE_BLK_BA_GMM_MATCH_WDMA; //(0X00001280)
	((struct ive_block *)ndev->IVE_BLK_BA)->GMM =
		ndev->ive_base + IVE_BLK_BA_GMM; //(0X000012C0)
	((struct ive_block *)ndev->IVE_BLK_BA)->BG_MATCH_FGFLAG_RDMA =
		ndev->ive_base + IVE_BLK_BA_BG_MATCH; //(0X00001400)
	((struct ive_block *)ndev->IVE_BLK_BA)->BG_MATCH_BGMODEL_0_RDMA =
		ndev->ive_base + IVE_BLK_BA_BG_MATCH; //(0X00001420)
	((struct ive_block *)ndev->IVE_BLK_BA)->BG_MATCH_BGMODEL_1_RDMA =
		ndev->ive_base + IVE_BLK_BA_BG_MATCH; //(0X00001440)
	((struct ive_block *)ndev->IVE_BLK_BA)->BG_MATCH_DIFFFG_WDMA =
		ndev->ive_base + IVE_BLK_BA_BG_MATCH; //(0X00001460)
	((struct ive_block *)ndev->IVE_BLK_BA)->BG_MATCH_IVEMATCH_BG =
		ndev->ive_base + IVE_BLK_BA_BG_MATCH; //(0X00001480)
	((struct ive_block *)ndev->IVE_BLK_BA)->BG_UPDATE =
		ndev->ive_base + IVE_BLK_BA_BG_UPDATE; //(0X00001500)
	((struct ive_block *)ndev->IVE_BLK_BA)->FILTEROP_RDMA =
		ndev->ive_base + IVE_BLK_BA_FILTEROP; //(0X00002000)
	((struct ive_block *)ndev->IVE_BLK_BA)->FILTEROP_WDMA_Y =
		ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x40; //(0X00002040)
	((struct ive_block *)ndev->IVE_BLK_BA)->FILTEROP_WDMA_C =
		ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x80; //(0X00002080)
	((struct ive_block *)ndev->IVE_BLK_BA)->FILTEROP =
		ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200; //(0X00002000)
	((struct ive_block *)ndev->IVE_BLK_BA)->CCL =
		ndev->ive_base + IVE_BLK_BA_CCL; //(0X00002400)
	((struct ive_block *)ndev->IVE_BLK_BA)->CCL_SRC_RDMA =
		ndev->ive_base + IVE_BLK_BA_CCL_SRC_RDMA; //(0X00002440)
	((struct ive_block *)ndev->IVE_BLK_BA)->CCL_DST_WDMA =
		ndev->ive_base + IVE_BLK_BA_CCL_DST_WDMA; //(0X00002480)
	((struct ive_block *)ndev->IVE_BLK_BA)->CCL_REGION_WDMA =
		ndev->ive_base + IVE_BLK_BA_CCL_REGION_WDMA; //(0X000024C0)
	((struct ive_block *)ndev->IVE_BLK_BA)->DMAF =
		ndev->ive_base + IVE_BLK_BA_DMAF; //(0X00002600)
	((struct ive_block *)ndev->IVE_BLK_BA)->DMAF_WDMA =
		ndev->ive_base + IVE_BLK_BA_DMAF + 0x40; //(0X00002640)
	((struct ive_block *)ndev->IVE_BLK_BA)->DMAF_RDMA =
		ndev->ive_base + IVE_BLK_BA_DMAF + 0x80; //(0X00002680)
	((struct ive_block *)ndev->IVE_BLK_BA)->LK =
		ndev->ive_base + IVE_BLK_BA_LK; //(0X00002700)
	((struct ive_block *)ndev->IVE_BLK_BA)->RDMA_EIGVAL =
		ndev->ive_base + IVE_BLK_BA_RDMA_EIGVAL; //(0X00002800)
	((struct ive_block *)ndev->IVE_BLK_BA)->WDMA =
		ndev->ive_base + IVE_BLK_BA_WDMA; //(0X00002900)
	((struct ive_block *)ndev->IVE_BLK_BA)->RDMA =
		ndev->ive_base + IVE_BLK_BA_RDMA; //(0X00002A00)

	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_DMA(struct cvi_ive_device *ndev, IVE_DATA_S *pstSrc,
		    IVE_DST_DATA_S *pstDst, IVE_DMA_CTRL_S *pstDmaCtrl,
		    bool bInstant)
{
	int i = 0;
	IVE_SRC_IMAGE_S _pstSrc;
	IVE_SRC_IMAGE_S _pstDst;
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_DMA_C(ive_dma_c);
	DEFINE_ISP_DMA_CTL_C(wdma_dma_ctl_c);
	DEFINE_ISP_DMA_CTL_C(rdma_dma_ctl_c);

	pr_info("CVI_MPI_IVE_DMA\n");
	//IVE_BLK_BA_DMAF
	if (pstDst == NULL) {
		if (IVE_DMA_MODE_SET_3BYTE == pstDmaCtrl->enMode ||
		    IVE_DMA_MODE_SET_8BYTE == pstDmaCtrl->enMode) {
			pr_info("[IVE] not supper DMA mode of 3BYTE and 8BYTE\n");
			return CVI_FAILURE;
		}
		pstDst = pstSrc;
	}

	ive_top_c.REG_2.reg_img_heightm1 = pstSrc->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstSrc->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");
	ive_top_c.REG_h10.reg_dmaf_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	ive_dma_c.REG_0.reg_ive_dma_enable = 1;
	ive_dma_c.REG_0.reg_force_clk_enable = 1;
	ive_dma_c.REG_0.reg_ive_dma_mode = pstDmaCtrl->enMode;
	writed(ive_dma_c.REG_0.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + IVE_DMA_REG_0),
	       "DMAF_REG_0");
	ive_dma_c.REG_4.reg_ive_dma_horsegsize = pstDmaCtrl->u8HorSegSize;
	ive_dma_c.REG_4.reg_ive_dma_versegrow = pstDmaCtrl->u8VerSegRows;
	ive_dma_c.REG_4.reg_ive_dma_elemsize = pstDmaCtrl->u8ElemSize;
	writed(ive_dma_c.REG_4.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + IVE_DMA_REG_4),
	       "DMAF_REG_4");
	ive_dma_c.REG_1.reg_ive_dma_src_stride = (CVI_U16)pstSrc->u16Stride;
	ive_dma_c.REG_1.reg_ive_dma_dst_stride = (CVI_U16)pstDst->u16Stride;
	writed(ive_dma_c.REG_1.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + IVE_DMA_REG_1),
	       "DMAF_REG_1");
	ive_dma_c.REG_5.reg_ive_dma_u64_val[0] =
		(CVI_U32)(pstDmaCtrl->u64Val & 0xffffffff);
	ive_dma_c.REG_5.reg_ive_dma_u64_val[1] =
		(CVI_U32)((pstDmaCtrl->u64Val >> 32) & 0xffffffff);
	writed(ive_dma_c.REG_5.val[0],
	       (ndev->ive_base + IVE_BLK_BA_DMAF + IVE_DMA_REG_5),
	       "DMAF_REG_5");
	writed(ive_dma_c.REG_5.val[1],
	       (ndev->ive_base + IVE_BLK_BA_DMAF + IVE_DMA_REG_5 + 0x04),
	       "DMAF_REG_5");
	ive_top_c.REG_1.reg_fmt_vld_fg = 1;
	writed(ive_top_c.REG_1.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_1),
	       "IVE_TOP_REG_1");
	ive_top_c.REG_94.reg_intr_en_dmaf = !bInstant;
	writed(ive_top_c.REG_94.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_94),
	       "IVE_TOP_REG_94");

	for (i = 0; i < 3; i++) {
		// avoid -Werror=uninitialized
		_pstSrc.u64PhyAddr[i] = pstSrc->u32PhyAddr;
		_pstSrc.pu8VirAddr[i] = pstSrc->pu8VirAddr;
		_pstDst.u64PhyAddr[i] = pstDst->u32PhyAddr;
		_pstDst.pu8VirAddr[i] = pstDst->pu8VirAddr;
		_pstSrc.u16Stride[i] = pstSrc->u16Stride;
		_pstDst.u16Stride[i] = pstDst->u16Stride;
	}

	if (ive_dma_c.REG_0.reg_ive_dma_mode == IVE_DMA_MODE_SET_3BYTE ||
	    ive_dma_c.REG_0.reg_ive_dma_mode == IVE_DMA_MODE_SET_8BYTE) {
		rdma_dma_ctl_c.BASE_ADDR.reg_basel = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_baseh = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
	} else {
		rdma_dma_ctl_c.BASE_ADDR.reg_basel =
			_pstSrc.u64PhyAddr[0] & 0xffffffff;
		rdma_dma_ctl_c.SYS_CONTROL.reg_baseh =
			(_pstSrc.u64PhyAddr[0] >> 32) & 0xffffffff;
		rdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	}
	writed(rdma_dma_ctl_c.BASE_ADDR.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + 0x80 +
		ISP_DMA_CTL_BASE_ADDR),
	       "ISP_DMA_CTL_BASE_ADDR");
	writed(rdma_dma_ctl_c.SYS_CONTROL.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + 0x80 +
		ISP_DMA_CTL_SYS_CONTROL),
	       "ISP_DMA_CTL_SYS_CONTROL");

	wdma_dma_ctl_c.BASE_ADDR.reg_basel = _pstDst.u64PhyAddr[0] & 0xffffffff;
	wdma_dma_ctl_c.SYS_CONTROL.reg_baseh =
		(_pstDst.u64PhyAddr[0] >> 32) & 0xffffffff;
	wdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writed(wdma_dma_ctl_c.BASE_ADDR.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + 0x40 +
		ISP_DMA_CTL_BASE_ADDR),
	       "ISP_DMA_CTL_BASE_ADDR");
	writed(wdma_dma_ctl_c.SYS_CONTROL.val,
	       (ndev->ive_base + IVE_BLK_BA_DMAF + 0x40 +
		ISP_DMA_CTL_SYS_CONTROL),
	       "ISP_DMA_CTL_SYS_CONTROL");

	cvi_ive_go(ndev, &ive_top_c, bInstant);

	isp_dma_ctl_printk(&wdma_dma_ctl_c);
	isp_dma_ctl_printk(&rdma_dma_ctl_c);

	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_Add(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    IVE_ADD_CTRL_S *pstAddCtrl, bool bInstant)
{
	pr_info("[IVE] CVI_MPI_IVE_Add\n");
	reinit_completion(&ndev->ive_done);
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, 3,
			       (void *)pstAddCtrl); // 3 add 2 = 0  #2
}

CVI_S32 cvi_ive_And(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    bool bInstant)
{
	pr_info("[IVE] debug CVI_MPI_IVE_And\n");
	reinit_completion(&ndev->ive_done);
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, 0,
			       NULL); // 3 and 2 = 0  #2
}

CVI_S32 cvi_ive_Or(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		   IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		   bool bInstant)
{
	pr_info("[IVE] CVI_MPI_IVE_Or\n");
	reinit_completion(&ndev->ive_done);
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, 1,
			       NULL); // 3 or 2 = 2  #3
}

CVI_S32 cvi_ive_Sub(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    IVE_SUB_CTRL_S *pstSubCtrl, bool bInstant)
{
	pr_info("[IVE] CVI_MPI_IVE_Sub\n");
	reinit_completion(&ndev->ive_done);
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, 4,
			       (void *)pstSubCtrl); // 3 - 2 = 2  #1
}

CVI_S32 cvi_ive_Xor(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    bool bInstant)
{
	pr_info("[IVE] CVI_MPI_IVE_Xor\n");
	reinit_completion(&ndev->ive_done);
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, 2,
			       NULL); //3 xor 2 = 2  #1
}

CVI_S32 cvi_ive_Thresh(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstThrCtrl,
		       bool bInstant)
{
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	pr_info("[IVE] CVI_MPI_IVE_Thresh\n");

	ive_reset(ndev, &ive_top_c);
	// top
	ive_top_c.REG_2.reg_img_heightm1 = pstDst->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstDst->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");

	ive_top_c.REG_h14c.reg_thresh_u8bit_thr_l = pstThrCtrl->u8LowThr;
	//ive_top_c.REG_h14c.reg_thresh_u8bit_thr_h = pstThrCtrl->u8HighThr;
	ive_top_c.REG_h14c.reg_thresh_enmode = pstThrCtrl->enMode;
	writed(ive_top_c.REG_h14c.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H14C),
	       "IVE_TOP_REG_H14C");
	ive_top_c.REG_h150.reg_thresh_u8bit_min = pstThrCtrl->u8MinVal;
	//ive_top_c.REG_h150.reg_thresh_u8bit_mid   = pstThrCtrl->u8MidVal;
	ive_top_c.REG_h150.reg_thresh_u8bit_max = pstThrCtrl->u8MaxVal;
	writed(ive_top_c.REG_h150.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H150),
	       "IVE_TOP_REG_H150");

	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	setImgSrc1(ndev, pstSrc);

	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_REG_20");
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");
	ive_top_c.REG_h10.reg_filterop_top_enable = 1;
	ive_top_c.REG_h10.reg_thresh_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	ive_top_c.REG_h130.reg_thresh_thresh_en = 1;
	writed(ive_top_c.REG_h130.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H130),
	       "IVE_TOP_REG_H130");

	//bypass filterop...
	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0;
	writed(ive_filterop_c.REG_28.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_28),
	       "IVE_FILTEROP_REG_28");

	setImgDst1(ndev, pstDst);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	cvi_ive_go(ndev, &ive_top_c, bInstant);

	ive_top_printk((IVE_TOP_C *)(ndev->ive_base + IVE_BLK_BA_IVE_TOP));
	ive_filterop_printk((IVE_FILTEROP_C *)(ndev->ive_base +
					       IVE_BLK_BA_FILTEROP + 0x200));

	return CVI_SUCCESS;
}

CVI_S32 erode_dilate_op(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst,
			IVE_ELEMENT_STRUCTURE_CTRL_S *pstCtrl, bool bInstant,
			int op)
{
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	ive_reset(ndev, &ive_top_c);
	ive_top_c.REG_2.reg_img_heightm1 = pstSrc->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstSrc->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");

	ive_filterop_c.REG_h10.reg_filterop_mode = op;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_TOP_REG_H10");
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_TOP_REG_H14");
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 1;
	writed(ive_filterop_c.REG_28.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_28),
	       "IVE_TOP_REG_28");

	if (!(readl(ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		    IVE_FILTEROP_REG_28) &&
	      IVE_FILTEROP_REG_FILTEROP_OP2_ERODILA_EN_MASK)) {
		ive_filterop_c.REG_4.reg_filterop_h_coef00 =
			pstCtrl->au8Mask[0] * 255;
		ive_filterop_c.REG_4.reg_filterop_h_coef01 =
			pstCtrl->au8Mask[1] * 255;
		ive_filterop_c.REG_4.reg_filterop_h_coef02 =
			pstCtrl->au8Mask[2] * 255;
		ive_filterop_c.REG_4.reg_filterop_h_coef03 =
			pstCtrl->au8Mask[3] * 255;
		writed(ive_filterop_c.REG_4.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_4),
		       "IVE_FILTEROP_REG_4");
		ive_filterop_c.REG_5.reg_filterop_h_coef04 =
			pstCtrl->au8Mask[4] * 255;
		ive_filterop_c.REG_5.reg_filterop_h_coef10 =
			pstCtrl->au8Mask[5] * 255;
		ive_filterop_c.REG_5.reg_filterop_h_coef11 =
			pstCtrl->au8Mask[6] * 255;
		ive_filterop_c.REG_5.reg_filterop_h_coef12 =
			pstCtrl->au8Mask[7] * 255;
		writed(ive_filterop_c.REG_5.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_5),
		       "IVE_FILTEROP_REG_5");
		ive_filterop_c.REG_6.reg_filterop_h_coef13 =
			pstCtrl->au8Mask[8] * 255;
		ive_filterop_c.REG_6.reg_filterop_h_coef14 =
			pstCtrl->au8Mask[9] * 255;
		ive_filterop_c.REG_6.reg_filterop_h_coef20 =
			pstCtrl->au8Mask[10] * 255;
		ive_filterop_c.REG_6.reg_filterop_h_coef21 =
			pstCtrl->au8Mask[11] * 255;
		writed(ive_filterop_c.REG_6.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_6),
		       "IVE_FILTEROP_REG_6");
		ive_filterop_c.REG_7.reg_filterop_h_coef22 =
			pstCtrl->au8Mask[12] * 255;
		ive_filterop_c.REG_7.reg_filterop_h_coef23 =
			pstCtrl->au8Mask[13] * 255;
		ive_filterop_c.REG_7.reg_filterop_h_coef24 =
			pstCtrl->au8Mask[14] * 255;
		ive_filterop_c.REG_7.reg_filterop_h_coef30 =
			pstCtrl->au8Mask[15] * 255;
		writed(ive_filterop_c.REG_7.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_7),
		       "IVE_FILTEROP_REG_7");
		ive_filterop_c.REG_8.reg_filterop_h_coef31 =
			pstCtrl->au8Mask[16] * 255;
		ive_filterop_c.REG_8.reg_filterop_h_coef32 =
			pstCtrl->au8Mask[17] * 255;
		ive_filterop_c.REG_8.reg_filterop_h_coef33 =
			pstCtrl->au8Mask[18] * 255;
		ive_filterop_c.REG_8.reg_filterop_h_coef34 =
			pstCtrl->au8Mask[19] * 255;
		writed(ive_filterop_c.REG_8.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_8),
		       "IVE_FILTEROP_REG_8");
		ive_filterop_c.REG_9.reg_filterop_h_coef40 =
			pstCtrl->au8Mask[20] * 255;
		ive_filterop_c.REG_9.reg_filterop_h_coef41 =
			pstCtrl->au8Mask[21] * 255;
		ive_filterop_c.REG_9.reg_filterop_h_coef42 =
			pstCtrl->au8Mask[22] * 255;
		ive_filterop_c.REG_9.reg_filterop_h_coef43 =
			pstCtrl->au8Mask[23] * 255;
		writed(ive_filterop_c.REG_9.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_9),
		       "IVE_FILTEROP_REG_9");
		ive_filterop_c.REG_10.reg_filterop_h_coef44 =
			pstCtrl->au8Mask[24] * 255;
		writed(ive_filterop_c.REG_10.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_10),
		       "IVE_FILTEROP_REG_10");
	} else {
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef00 =
			pstCtrl->au8Mask[0] * 255;
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef01 =
			pstCtrl->au8Mask[1] * 255;
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef02 =
			pstCtrl->au8Mask[2] * 255;
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef03 =
			pstCtrl->au8Mask[3] * 255;
		writed(ive_filterop_c.REG_21.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_21),
		       "IVE_FILTEROP_REG_21");
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef04 =
			pstCtrl->au8Mask[4] * 255;
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef10 =
			pstCtrl->au8Mask[5] * 255;
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef11 =
			pstCtrl->au8Mask[6] * 255;
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef12 =
			pstCtrl->au8Mask[7] * 255;
		writed(ive_filterop_c.REG_22.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_22),
		       "IVE_FILTEROP_REG_22");
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef13 =
			pstCtrl->au8Mask[8] * 255;
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef14 =
			pstCtrl->au8Mask[9] * 255;
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef20 =
			pstCtrl->au8Mask[10] * 255;
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef21 =
			pstCtrl->au8Mask[11] * 255;
		writed(ive_filterop_c.REG_23.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_23),
		       "IVE_FILTEROP_REG_23");
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef22 =
			pstCtrl->au8Mask[12] * 255;
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef23 =
			pstCtrl->au8Mask[13] * 255;
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef24 =
			pstCtrl->au8Mask[14] * 255;
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef30 =
			pstCtrl->au8Mask[15] * 255;
		writed(ive_filterop_c.REG_24.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_24),
		       "IVE_FILTEROP_REG_24");
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef31 =
			pstCtrl->au8Mask[16] * 255;
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef32 =
			pstCtrl->au8Mask[17] * 255;
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef33 =
			pstCtrl->au8Mask[18] * 255;
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef34 =
			pstCtrl->au8Mask[19] * 255;
		writed(ive_filterop_c.REG_25.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_25),
		       "IVE_FILTEROP_REG_25");
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef40 =
			pstCtrl->au8Mask[20] * 255;
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef41 =
			pstCtrl->au8Mask[21] * 255;
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef42 =
			pstCtrl->au8Mask[22] * 255;
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef43 =
			pstCtrl->au8Mask[23] * 255;
		writed(ive_filterop_c.REG_26.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_26),
		       "IVE_FILTEROP_REG_26");
		ive_filterop_c.REG_27.reg_filterop_op2_erodila_coef44 =
			pstCtrl->au8Mask[24] * 255;
		writed(ive_filterop_c.REG_27.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_27),
		       "IVE_FILTEROP_REG_27");
	}

	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	setImgSrc1(ndev, pstSrc);

	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");
	ive_top_c.REG_h10.reg_filterop_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	// FIXME: check default is 0
	ive_top_c.REG_R2Y4_14.reg_csc_r2y4_enable = 0;
	writed(ive_top_c.REG_R2Y4_14.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_R2Y4_14),
	       "IVE_TOP_REG_R2Y4_14");

	setImgDst1(ndev, pstDst);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	writed(ive_top_c.REG_h130.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H130),
	       "IVE_TOP_REG_H130");
	writed(ive_top_c.REG_h14c.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H14C),
	       "IVE_TOP_REG_H14C");
	writed(ive_top_c.REG_h150.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H150),
	       "IVE_TOP_REG_H150");

	cvi_ive_go(ndev, &ive_top_c, bInstant);
	ive_top_printk((IVE_TOP_C *)(ndev->ive_base + IVE_BLK_BA_IVE_TOP));
	ive_filterop_printk((IVE_FILTEROP_C *)(ndev->ive_base +
					       IVE_BLK_BA_FILTEROP + 0x200));

	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_Erode(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstErodeCtrl,
		      bool bInstant)
{
	pr_info("CVI_MPI_IVE_Erode\n");
	return erode_dilate_op(ndev, pstSrc, pstDst, pstErodeCtrl, bInstant, 3);
}

CVI_S32 cvi_ive_Dilate(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst,
		       IVE_DILATE_CTRL_S *pstDilateCtrl, bool bInstant)
{
	pr_info("CVI_MPI_IVE_Dilate\n");
	return erode_dilate_op(ndev, pstSrc, pstDst, pstDilateCtrl, bInstant,
			       2);
}
CVI_S32 gmm_gmm2_op(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstBg, IVE_SRC_IMAGE_S *_pstModel,
		    IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstMatchModelInfo,
		    IVE_GMM_C *ive_gmm_c, bool bInstant)
{
	int i = 0;
	int num = 0;
	int enable = 0;
	ISP_DMA_CTL_C *gmm_mod_rdma_ctl_c[5];
	ISP_DMA_CTL_C *gmm_mod_wdma_ctl_c[5];
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_0);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_1);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_2);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_3);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_4);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_0);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_1);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_2);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_3);
	DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_4);
	DEFINE_ISP_DMA_CTL_C(gmm_match_wdma_ctl_c);

	gmm_mod_rdma_ctl_c[0] = &gmm_mod_rdma_0; // IVE_BLK_BA_GMM_MODEL_RDMA_0;
	gmm_mod_rdma_ctl_c[1] = &gmm_mod_rdma_1; // IVE_BLK_BA_GMM_MODEL_RDMA_1;
	gmm_mod_rdma_ctl_c[2] = &gmm_mod_rdma_2; // IVE_BLK_BA_GMM_MODEL_RDMA_2;
	gmm_mod_rdma_ctl_c[3] = &gmm_mod_rdma_3; // IVE_BLK_BA_GMM_MODEL_RDMA_3;
	gmm_mod_rdma_ctl_c[4] = &gmm_mod_rdma_4; // IVE_BLK_BA_GMM_MODEL_RDMA_4;
	gmm_mod_wdma_ctl_c[0] = &gmm_mod_wdma_0; // IVE_BLK_BA_GMM_MODEL_WDMA_0;
	gmm_mod_wdma_ctl_c[1] = &gmm_mod_wdma_1; // IVE_BLK_BA_GMM_MODEL_WDMA_1;
	gmm_mod_wdma_ctl_c[2] = &gmm_mod_wdma_2; // IVE_BLK_BA_GMM_MODEL_WDMA_2;
	gmm_mod_wdma_ctl_c[3] = &gmm_mod_wdma_3; // IVE_BLK_BA_GMM_MODEL_WDMA_3;
	gmm_mod_wdma_ctl_c[4] = &gmm_mod_wdma_4; // IVE_BLK_BA_GMM_MODEL_WDMA_4;

	num = (readl(ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_13) &&
	       IVE_GMM_REG_GMM_GMM2_MODEL_NUM_MASK) >>
	      IVE_GMM_REG_GMM_GMM2_MODEL_NUM_OFFSET;
	pr_info("[IVE] GMM_GMM2_MODEL_NUM = %d\n", num);
	for (i = 0; i < num; i++) {
		CVI_U32 u32ModelSize =
			(pstSrc->enType == IVE_IMAGE_TYPE_U8C1) ? 8 : 12;
		CVI_U64 dst0addr =
			(CVI_U64)_pstModel->u64PhyAddr[0] +
			pstSrc->u16Width * pstSrc->u16Height * u32ModelSize * i;

		gmm_mod_rdma_ctl_c[i]->BASE_ADDR.reg_basel =
			dst0addr & 0xffffffff;
		writed(gmm_mod_rdma_ctl_c[i]->BASE_ADDR.val,
		       (ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_0 +
			i * 0x40 + ISP_DMA_CTL_BASE_ADDR),
		       "ISP_DMA_CTL_BASE_ADDR");
		gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_baseh =
			(dst0addr >> 32) & 0xffffffff;
		gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_base_sel = 1;
		writed(gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.val,
		       (ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_0 +
			i * 0x40 + ISP_DMA_CTL_SYS_CONTROL),
		       "ISP_DMA_CTL_SYS_CONTROL");
		gmm_mod_wdma_ctl_c[i]->BASE_ADDR.reg_basel =
			dst0addr & 0xffffffff;
		writed(gmm_mod_wdma_ctl_c[i]->BASE_ADDR.val,
		       (ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_0 +
			i * 0x40 + ISP_DMA_CTL_BASE_ADDR),
		       "ISP_DMA_CTL_BASE_ADDR");
		gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_baseh =
			(dst0addr >> 32) & 0xffffffff;
		gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_base_sel = 1;
		writed(gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.val,
		       (ndev->ive_base + IVE_BLK_BA_GMM_MODEL_RDMA_0 +
			i * 0x40 + ISP_DMA_CTL_SYS_CONTROL),
		       "ISP_DMA_CTL_SYS_CONTROL");
	}

	ive_top_c.REG_2.reg_img_heightm1 = pstSrc->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstSrc->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");

	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	setImgSrc1(ndev, pstSrc);

	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 5;
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");
	//bypass filterop...
	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0; //sw_ovw; bypass op1
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0; //bypass op2
	writed(ive_filterop_c.REG_28.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_28),
	       "IVE_FILTEROP_REG_28");
	ive_top_c.REG_h10.reg_filterop_top_enable = 1;
	ive_top_c.REG_h10.reg_gmm_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	//GMM2
	enable = (readl(ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_13) &&
		  IVE_GMM_REG_FORCE_CLK_ENABLE) >>
		 IVE_GMM_REG_FORCE_CLK_ENABLE_OFFSET;
	if (enable > 1) {
		//pr_info("gmm_match_wdma_ctl_c = %p addr = %lx\n", gmm_match_wdma_ctl_c, dst2addr);
		if (pstMatchModelInfo != NULL) {
			gmm_match_wdma_ctl_c.BASE_ADDR.reg_basel =
				pstMatchModelInfo->u64PhyAddr[0] & 0xffffffff;
			writed(gmm_match_wdma_ctl_c.BASE_ADDR.val,
			       (ndev->ive_base + IVE_BLK_BA_GMM_MATCH_WDMA +
				ISP_DMA_CTL_BASE_ADDR),
			       "ISP_DMA_CTL_BASE_ADDR");
			gmm_match_wdma_ctl_c.SYS_CONTROL.reg_baseh =
				(pstMatchModelInfo->u64PhyAddr[0] >> 32) &
				0xffffffff;
			gmm_match_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
			writed(gmm_match_wdma_ctl_c.SYS_CONTROL.val,
			       (ndev->ive_base + IVE_BLK_BA_GMM_MATCH_WDMA +
				ISP_DMA_CTL_SYS_CONTROL),
			       "ISP_DMA_CTL_SYS_CONTROL");
		} else {
			gmm_match_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
			gmm_match_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
			writed(gmm_match_wdma_ctl_c.BASE_ADDR.val,
			       (ndev->ive_base + IVE_BLK_BA_GMM_MATCH_WDMA +
				ISP_DMA_CTL_BASE_ADDR),
			       "ISP_DMA_CTL_BASE_ADDR");
			gmm_match_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
			writed(gmm_match_wdma_ctl_c.SYS_CONTROL.val,
			       (ndev->ive_base + IVE_BLK_BA_GMM_MATCH_WDMA +
				ISP_DMA_CTL_SYS_CONTROL),
			       "ISP_DMA_CTL_SYS_CONTROL");
		}
		isp_dma_ctl_printk(&gmm_match_wdma_ctl_c);
	}

	setImgDst1(ndev, pstFg);
	setOdma(ndev, pstBg, pstSrc->u16Width, pstSrc->u16Height);
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");
	// NOTICE: last one to trigger it
	cvi_ive_go(ndev, &ive_top_c, bInstant);
	//*iveReg->pIveHandle += 1;
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_GMM(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstBg,
		    IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl,
		    bool bInstant)
{
	IVE_SRC_IMAGE_S _pstModel;
	DEFINE_IVE_GMM_C(ive_gmm_c);

	pr_info("CVI_MPI_IVE_GMM\n");
	if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1 &&
	    pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PACKAGE &&
	    pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PLANAR) {
		pr_info("pstSrc->enType cannot be (%d)\n", pstSrc->enType);
		return CVI_FAILURE;
	}
	// setting
	ive_gmm_c.REG_GMM_0.reg_gmm_learn_rate = pstGmmCtrl->u0q16LearnRate;
	ive_gmm_c.REG_GMM_0.reg_gmm_bg_ratio = pstGmmCtrl->u0q16BgRatio;
	writed(ive_gmm_c.REG_GMM_0.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_0),
	       "IVE_GMM_REG_GMM_0");
	ive_gmm_c.REG_GMM_1.reg_gmm_var_thr = pstGmmCtrl->u8q8VarThr;
	writed(ive_gmm_c.REG_GMM_1.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_1),
	       "IVE_GMM_REG_GMM_1");
	ive_gmm_c.REG_GMM_2.reg_gmm_noise_var = pstGmmCtrl->u22q10NoiseVar;
	writed(ive_gmm_c.REG_GMM_2.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_2),
	       "IVE_GMM_REG_GMM_2");
	ive_gmm_c.REG_GMM_3.reg_gmm_max_var = pstGmmCtrl->u22q10MaxVar;
	writed(ive_gmm_c.REG_GMM_3.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_3),
	       "IVE_GMM_REG_GMM_3");
	ive_gmm_c.REG_GMM_4.reg_gmm_min_var = pstGmmCtrl->u22q10MinVar;
	writed(ive_gmm_c.REG_GMM_4.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_4),
	       "IVE_GMM_REG_GMM_4");
	ive_gmm_c.REG_GMM_5.reg_gmm_init_weight = pstGmmCtrl->u0q16InitWeight;
	writed(ive_gmm_c.REG_GMM_5.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_5),
	       "IVE_GMM_REG_GMM_5");
	ive_gmm_c.REG_GMM_6.reg_gmm_detect_shadow = 0; // enDetectShadow
	ive_gmm_c.REG_GMM_6.reg_gmm_shadow_thr = 0; // u0q8ShadowThr
	ive_gmm_c.REG_GMM_6.reg_gmm_sns_factor =
		8; // u8SnsFactor, as from GMM2 sample
	writed(ive_gmm_c.REG_GMM_6.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_6),
	       "IVE_GMM_REG_GMM_6");
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_model_num = pstGmmCtrl->u8ModelNum;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly =
		(pstSrc->enType == IVE_IMAGE_TYPE_U8C1) ? 1 : 0;
	//[TODO]ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_enable = (*pIveHandle == 0) ? 0 : 1;
	writed(ive_gmm_c.REG_GMM_13.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_13),
	       "IVE_GMM_REG_GMM_13");

	_pstModel.u64PhyAddr[0] = pstModel->u32PhyAddr;
	_pstModel.pu8VirAddr[0] = pstModel->pu8VirAddr;

	gmm_gmm2_op(ndev, pstSrc, pstBg, &_pstModel, pstFg, NULL, &ive_gmm_c,
		    bInstant);

	ive_gmm_printk(&ive_gmm_c);

	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_GMM2(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		     IVE_SRC_IMAGE_S *pstFactor, IVE_DST_IMAGE_S *pstFg,
		     IVE_DST_IMAGE_S *pstBg, IVE_DST_IMAGE_S *pstMatchModelInfo,
		     IVE_MEM_INFO_S *pstModel, IVE_GMM2_CTRL_S *pstGmm2Ctrl,
		     bool bInstant)
{
	IVE_SRC_IMAGE_S _pstModel;
	DEFINE_IVE_GMM_C(ive_gmm_c);

	pr_info("CVI_MPI_IVE_GMM\n");
	if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1 &&
	    pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PACKAGE &&
	    pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PLANAR) {
		pr_info("pstSrc->enType cannot be (%d)\n", pstSrc->enType);
		return CVI_FAILURE;
	}
	// setting
	ive_gmm_c.REG_GMM_7.reg_gmm2_life_update_factor =
		pstGmm2Ctrl->u16GlbLifeUpdateFactor;
	ive_gmm_c.REG_GMM_7.reg_gmm2_var_rate = pstGmm2Ctrl->u16VarRate;
	writed(ive_gmm_c.REG_GMM_7.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_7),
	       "IVE_GMM_REG_GMM_7");
	ive_gmm_c.REG_GMM_8.reg_gmm2_freq_redu_factor =
		pstGmm2Ctrl->u16FreqReduFactor;
	ive_gmm_c.REG_GMM_8.reg_gmm2_max_var = pstGmm2Ctrl->u9q7MaxVar;
	writed(ive_gmm_c.REG_GMM_8.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_8),
	       "IVE_GMM_REG_GMM_8");
	ive_gmm_c.REG_GMM_9.reg_gmm2_min_var = pstGmm2Ctrl->u9q7MinVar;
	ive_gmm_c.REG_GMM_9.reg_gmm2_freq_add_factor =
		pstGmm2Ctrl->u16FreqAddFactor;
	writed(ive_gmm_c.REG_GMM_9.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_9),
	       "IVE_GMM_REG_GMM_9");
	ive_gmm_c.REG_GMM_10.reg_gmm2_freq_init = pstGmm2Ctrl->u16FreqInitVal;
	ive_gmm_c.REG_GMM_10.reg_gmm2_freq_thr = pstGmm2Ctrl->u16FreqThr;
	writed(ive_gmm_c.REG_GMM_10.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_10),
	       "IVE_GMM_REG_GMM_10");
	ive_gmm_c.REG_GMM_11.reg_gmm2_life_thr = pstGmm2Ctrl->u16LifeThr;
	ive_gmm_c.REG_GMM_11.reg_gmm2_sns_factor = pstGmm2Ctrl->u8GlbSnsFactor;
	writed(ive_gmm_c.REG_GMM_11.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_11),
	       "IVE_GMM_REG_GMM_11");
	//iveReg->ive_gmm_c->.reg_gmm2_factor = 0;
	ive_gmm_c.REG_GMM_12.reg_gmm2_life_update_factor_mode =
		pstGmm2Ctrl->enLifeUpdateFactorMode;
	ive_gmm_c.REG_GMM_12.reg_gmm2_sns_factor_mode =
		pstGmm2Ctrl->enSnsFactorMode;
	writed(ive_gmm_c.REG_GMM_12.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_12),
	       "IVE_GMM_REG_GMM_12");
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly =
		(pstSrc->enType == IVE_IMAGE_TYPE_U8C1) ? 1 : 0;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_model_num = pstGmm2Ctrl->u8ModelNum;
	writed(ive_gmm_c.REG_GMM_13.val,
	       (ndev->ive_base + IVE_BLK_BA_GMM + IVE_GMM_REG_GMM_13),
	       "IVE_GMM_REG_GMM_13");
	//ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_enable = (*pIveHandle == 0) ? 0 : 2;
	//CVI_U32 u32PhyAddr;
	//CVI_U8 *pu8VirAddr;
	//CVI_U32 u32ByteSize;
	_pstModel.u64PhyAddr[0] = pstModel->u32PhyAddr;
	_pstModel.pu8VirAddr[0] = pstModel->pu8VirAddr;

	gmm_gmm2_op(ndev, pstSrc, pstBg, &_pstModel, pstFg, pstMatchModelInfo,
		    &ive_gmm_c, bInstant);

	ive_gmm_printk(&ive_gmm_c);
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_MatchBgModel(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstCurImg, IVE_DATA_S *pstBgModel,
			     IVE_IMAGE_S *pstFgFlag,
			     IVE_DST_IMAGE_S *pstBgDiffFg,
			     IVE_DST_IMAGE_S *pstFrmDiffFg,
			     IVE_DST_MEM_INFO_S *pstStatData,
			     IVE_MATCH_BG_MODEL_CTRL_S *pstMatchBgModelCtrl,
			     CVI_BOOL bInstant)
{
	int i = 0;
	CVI_U64 dst0addr;
	IVE_SRC_IMAGE_S _pstFgFlag;
	IVE_SRC_IMAGE_S _pstBgModel;
	CVI_U64 src2addr, src3addr;
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
	DEFINE_ISP_DMA_CTL_C(fgflag_rdma_ctl_c);
	DEFINE_ISP_DMA_CTL_C(bgmodel_0_rdma_ctl_c);
	DEFINE_ISP_DMA_CTL_C(bgmodel_1_rdma_ctl_c);
	DEFINE_ISP_DMA_CTL_C(difffg_wdma_ctl_c);

	pr_info("CVI_MPI_IVE_MatchBgModel\n");
	// top
	ive_top_c.REG_2.reg_img_heightm1 = pstCurImg->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstCurImg->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");
	//+0x80
	ive_match_bg_c.REG_04.reg_matchbg_curfrmnum =
		pstMatchBgModelCtrl->u32CurFrmNum;
	ive_match_bg_c.REG_08.reg_matchbg_prefrmnum =
		pstMatchBgModelCtrl->u32PreFrmNum;
	writed(ive_match_bg_c.REG_04.val,
	       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x80 +
		IVE_MATCH_BG_REG_04),
	       "IVE_MATCH_BG_REG_04");
	writed(ive_match_bg_c.REG_08.val,
	       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x80 +
		IVE_MATCH_BG_REG_08),
	       "IVE_MATCH_BG_REG_08");
	ive_match_bg_c.REG_0C.reg_matchbg_timethr =
		pstMatchBgModelCtrl->u16TimeThr;
	ive_match_bg_c.REG_0C.reg_matchbg_diffthrcrlcoef =
		pstMatchBgModelCtrl->u8DiffThrCrlCoef;
	ive_match_bg_c.REG_0C.reg_matchbg_diffmaxthr =
		pstMatchBgModelCtrl->u8DiffMaxThr;
	ive_match_bg_c.REG_0C.reg_matchbg_diffminthr =
		pstMatchBgModelCtrl->u8DiffMinThr;
	ive_match_bg_c.REG_0C.reg_matchbg_diffthrinc =
		pstMatchBgModelCtrl->u8DiffThrInc;
	ive_match_bg_c.REG_0C.reg_matchbg_fastlearnrate =
		pstMatchBgModelCtrl->u8FastLearnRate;
	ive_match_bg_c.REG_0C.reg_matchbg_detchgregion =
		pstMatchBgModelCtrl->u8DetChgRegion;
	writed(ive_match_bg_c.REG_0C.val,
	       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x80 +
		IVE_MATCH_BG_REG_0C),
	       "IVE_MATCH_BG_REG_0C");
	// TODO: set dma

	for (i = 0; i < 3; i++) {
		_pstFgFlag.u64PhyAddr[i] = pstFgFlag->u64PhyAddr[i];
		_pstFgFlag.pu8VirAddr[i] = pstFgFlag->pu8VirAddr[i];
		_pstFgFlag.u16Stride[i] = pstFgFlag->u16Stride[i];
		_pstBgModel.u64PhyAddr[i] = pstBgModel->u32PhyAddr;
		_pstBgModel.pu8VirAddr[i] = pstBgModel->pu8VirAddr;
		_pstBgModel.u16Stride[i] = pstBgModel->u16Stride;
	}
	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	setImgSrc1(ndev, pstCurImg);

	src2addr = _pstBgModel.u64PhyAddr[0];
	src3addr = _pstFgFlag.u64PhyAddr[0];

	if (src2addr) {
		fgflag_rdma_ctl_c.BASE_ADDR.reg_basel =
			_pstBgModel.u64PhyAddr[0] & 0xffffffff;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(_pstBgModel.u64PhyAddr[0] >> 32) & 0xffffffff;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
		writed(fgflag_rdma_ctl_c.BASE_ADDR.val,
		       (ndev->ive_base + IVE_BLK_BA_BG_MATCH +
			ISP_DMA_CTL_BASE_ADDR),
		       "ISP_DMA_CTL_BASE_ADDR");
		writed(fgflag_rdma_ctl_c.SYS_CONTROL.val,
		       (ndev->ive_base + IVE_BLK_BA_BG_MATCH +
			ISP_DMA_CTL_SYS_CONTROL),
		       "ISP_DMA_CTL_SYS_CONTROL");
	}
	// printf("\fgflag_rdma_ctl_c.BASE_ADDR.reg_basel = 0x%x\n", fgflag_rdma_ctl_c->BASE_ADDR.reg_basel);
	isp_dma_ctl_printk(&fgflag_rdma_ctl_c);
	if (src3addr) {
		bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel =
			src3addr & 0xffffffff;
		bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src3addr >> 32) & 0xffffffff;
		bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
		bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel =
			src3addr & 0xffffffff;
		bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src3addr >> 32) & 0xffffffff;
		bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
		writed(bgmodel_0_rdma_ctl_c.BASE_ADDR.val,
		       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x20 +
			ISP_DMA_CTL_BASE_ADDR),
		       "ISP_DMA_CTL_BASE_ADDR");
		writed(bgmodel_0_rdma_ctl_c.SYS_CONTROL.val,
		       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x20 +
			ISP_DMA_CTL_SYS_CONTROL),
		       "ISP_DMA_CTL_SYS_CONTROL");
		writed(bgmodel_1_rdma_ctl_c.BASE_ADDR.val,
		       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x40 +
			ISP_DMA_CTL_BASE_ADDR),
		       "ISP_DMA_CTL_BASE_ADDR");
		writed(bgmodel_1_rdma_ctl_c.SYS_CONTROL.val,
		       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x40 +
			ISP_DMA_CTL_SYS_CONTROL),
		       "ISP_DMA_CTL_SYS_CONTROL");
	}
	// printf("\bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel = 0x%x\n", bgmodel_0_rdma_ctl_c->BASE_ADDR.reg_basel);
	isp_dma_ctl_printk(&bgmodel_0_rdma_ctl_c);
	isp_dma_ctl_printk(&bgmodel_1_rdma_ctl_c);
	ive_match_bg_printk(&ive_match_bg_c);

	setImgSrc1(ndev, pstCurImg);
	////////////////////////
	ive_match_bg_c.REG_00.reg_matchbg_en = 1;
	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 5;

	ive_top_c.REG_h10.reg_filterop_top_enable = 0;
	ive_top_c.REG_h10.reg_bgm_top_enable = 1;
	writed(ive_match_bg_c.REG_00.val,
	       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x80 +
		IVE_MATCH_BG_REG_00),
	       "IVE_MATCH_BG_REG_00");
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	///////////////////////////
	dst0addr = pstBgDiffFg->u64PhyAddr[0];

	if (dst0addr) {
		difffg_wdma_ctl_c.BASE_ADDR.reg_basel = dst0addr & 0xffffffff;
		difffg_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(dst0addr >> 32) & 0xffffffff;
		difffg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		difffg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		difffg_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		difffg_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writed(difffg_wdma_ctl_c.BASE_ADDR.val,
	       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x60 +
		ISP_DMA_CTL_BASE_ADDR),
	       "ISP_DMA_CTL_BASE_ADDR");
	writed(difffg_wdma_ctl_c.SYS_CONTROL.val,
	       (ndev->ive_base + IVE_BLK_BA_BG_MATCH + 0x60 +
		ISP_DMA_CTL_SYS_CONTROL),
	       "ISP_DMA_CTL_SYS_CONTROL");
	isp_dma_ctl_printk(&difffg_wdma_ctl_c);

	ive_filterop_c.ODMA_REG_00.reg_dma_en = 0;
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.ODMA_REG_00.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_ODMA_REG_00),
	       "IVE_FILTEROP_ODMA_REG_00");
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	cvi_ive_go(ndev, &ive_top_c, bInstant);
	ive_top_c.REG_h10.reg_bgm_top_enable = 0;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	//pr_info("stat pixnum %d sumlum %d\n",
	//ive_match_bg_c.REG_10.reg_matchbg_stat_pixnum,
	//ive_match_bg_c.REG_14.reg_matchbg_stat_sumlum);
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_Bernsen(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst,
			IVE_BERNSEN_CTRL_S *pstBernsenCtrl, CVI_BOOL bInstant)
{
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	pr_info("CVI_MPI_IVE_Bernsen\n");
	if (pstBernsenCtrl->u8WinSize != 3 && pstBernsenCtrl->u8WinSize != 5) {
		pr_info("not support u8WinSize %d, currently only support 3 or 5\n",
			pstBernsenCtrl->u8WinSize);
		return CVI_FAILURE;
	}

	// top
	ive_top_c.REG_2.reg_img_heightm1 = pstDst->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstDst->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");
	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_TOP_REG_H10");

	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 5;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_TOP_REG_H14");
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0;
	writed(ive_filterop_c.REG_28.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_28),
	       "IVE_TOP_REG_28");
	ive_filterop_c.REG_19.reg_filterop_bernsen_mode =
		pstBernsenCtrl->enMode;
	ive_filterop_c.REG_19.reg_filterop_bernsen_win5x5_en =
		pstBernsenCtrl->u8WinSize == 5;
	ive_filterop_c.REG_19.reg_filterop_bernsen_thr = pstBernsenCtrl->u8Thr;
	ive_filterop_c.REG_19.reg_filterop_u8ContrastThreshold =
		pstBernsenCtrl->u8ContrastThreshold;
	writed(ive_filterop_c.REG_19.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_19),
	       "IVE_FILTEROP_REG_19");
	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	setImgSrc1(ndev, pstSrc);

	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");
	ive_top_c.REG_h10.reg_filterop_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	// FIXME: check default is 0
	ive_top_c.REG_R2Y4_14.reg_csc_r2y4_enable = 0;
	writed(ive_top_c.REG_R2Y4_14.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_R2Y4_14),
	       "IVE_TOP_REG_R2Y4_14");

	setImgDst1(ndev, pstDst);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	cvi_ive_go(ndev, &ive_top_c, bInstant);
	return CVI_SUCCESS;
}

CVI_S32 _cvi_ive_filter(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,
			CVI_BOOL bInstant, IVE_FILTEROP_C *ive_filterop_c,
			CVI_BOOL isEmit)
{
	DEFINE_IMG_IN_C(img_in_c);
	DEFINE_IVE_TOP_C(ive_top_c);

	pr_info("CVI_MPI_IVE_Filter\n");
	// top
	ive_top_c.REG_2.reg_img_heightm1 = pstDst->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstDst->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");

	ive_filterop_c->REG_4.reg_filterop_h_coef00 = pstFltCtrl->as8Mask[0];
	ive_filterop_c->REG_4.reg_filterop_h_coef01 = pstFltCtrl->as8Mask[1];
	ive_filterop_c->REG_4.reg_filterop_h_coef02 = pstFltCtrl->as8Mask[2];
	ive_filterop_c->REG_4.reg_filterop_h_coef03 = pstFltCtrl->as8Mask[3];
	ive_filterop_c->REG_5.reg_filterop_h_coef04 = pstFltCtrl->as8Mask[4];
	ive_filterop_c->REG_5.reg_filterop_h_coef10 = pstFltCtrl->as8Mask[5];
	ive_filterop_c->REG_5.reg_filterop_h_coef11 = pstFltCtrl->as8Mask[6];
	ive_filterop_c->REG_5.reg_filterop_h_coef12 = pstFltCtrl->as8Mask[7];
	ive_filterop_c->REG_6.reg_filterop_h_coef13 = pstFltCtrl->as8Mask[8];
	ive_filterop_c->REG_6.reg_filterop_h_coef14 = pstFltCtrl->as8Mask[9];
	ive_filterop_c->REG_6.reg_filterop_h_coef20 = pstFltCtrl->as8Mask[10];
	ive_filterop_c->REG_6.reg_filterop_h_coef21 = pstFltCtrl->as8Mask[11];
	ive_filterop_c->REG_7.reg_filterop_h_coef22 = pstFltCtrl->as8Mask[12];
	ive_filterop_c->REG_7.reg_filterop_h_coef23 = pstFltCtrl->as8Mask[13];
	ive_filterop_c->REG_7.reg_filterop_h_coef24 = pstFltCtrl->as8Mask[14];
	ive_filterop_c->REG_7.reg_filterop_h_coef30 = pstFltCtrl->as8Mask[15];
	ive_filterop_c->REG_8.reg_filterop_h_coef31 = pstFltCtrl->as8Mask[16];
	ive_filterop_c->REG_8.reg_filterop_h_coef32 = pstFltCtrl->as8Mask[17];
	ive_filterop_c->REG_8.reg_filterop_h_coef33 = pstFltCtrl->as8Mask[18];
	ive_filterop_c->REG_8.reg_filterop_h_coef34 = pstFltCtrl->as8Mask[19];
	ive_filterop_c->REG_9.reg_filterop_h_coef40 = pstFltCtrl->as8Mask[20];
	ive_filterop_c->REG_9.reg_filterop_h_coef41 = pstFltCtrl->as8Mask[21];
	ive_filterop_c->REG_9.reg_filterop_h_coef42 = pstFltCtrl->as8Mask[22];
	ive_filterop_c->REG_9.reg_filterop_h_coef43 = pstFltCtrl->as8Mask[23];
	ive_filterop_c->REG_10.reg_filterop_h_coef44 = pstFltCtrl->as8Mask[24];
	ive_filterop_c->REG_10.reg_filterop_h_norm = pstFltCtrl->u32Norm;
	writed(ive_filterop_c->REG_4.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_4),
	       "IVE_FILTEROP_REG_4");
	writed(ive_filterop_c->REG_5.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_5),
	       "IVE_FILTEROP_REG_5");
	writed(ive_filterop_c->REG_6.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_6),
	       "IVE_FILTEROP_REG_6");
	writed(ive_filterop_c->REG_7.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_7),
	       "IVE_FILTEROP_REG_7");
	writed(ive_filterop_c->REG_8.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_8),
	       "IVE_FILTEROP_REG_8");
	writed(ive_filterop_c->REG_9.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_9),
	       "IVE_FILTEROP_REG_9");
	writed(ive_filterop_c->REG_10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_10),
	       "IVE_FILTEROP_REG_10");
	switch (pstSrc->enType) {
	case IVE_IMAGE_TYPE_U8C1:
		// pass, filter case
		// TODO: check 1 channels setting
		ive_filterop_c->REG_h10.reg_filterop_mode = 3;
		ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 1;
		ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 1;
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
	case IVE_IMAGE_TYPE_YUV422SP:
		ive_filterop_c->REG_h10.reg_filterop_mode = 1;
		ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
		ive_top_c.REG_R2Y4_14.reg_csc_r2y4_enable = 0;
		ive_top_c.REG_R2Y4_14.reg_csc_r2y4_enmode = 0;
		writed(ive_top_c.REG_R2Y4_14.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP +
			IVE_TOP_REG_R2Y4_14),
		       "IVE_TOP_REG_R2Y4_14");
		img_in_c.REG_00.reg_auto_csc_en = 0;
		writed(img_in_c.REG_00.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_00),
		       "IMG_IN_REG_00");
		ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enable = 0;
		ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enmode = 0;
		writed(ive_filterop_c->REG_h1c8.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_H1C8),
		       "IVE_FILTEROP_REG_H1C8");
		ive_filterop_c->REG_h14.reg_filterop_3ch_en = 1;
		pr_info("Image enType %d\n", pstSrc->enType);
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		ive_filterop_c->REG_h10.reg_filterop_mode = 1;
		ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
		ive_top_c.REG_R2Y4_14.reg_csc_r2y4_enable = 0;
		writed(ive_top_c.REG_R2Y4_14.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP +
			IVE_TOP_REG_R2Y4_14),
		       "IVE_TOP_REG_R2Y4_14");
		img_in_c.REG_00.reg_auto_csc_en = 0;
		writed(img_in_c.REG_00.val,
		       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_00),
		       "IMG_IN_REG_00");
		ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enable = 0;
		ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enmode = 0;
		writed(ive_filterop_c->REG_h1c8.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_H1C8),
		       "IVE_FILTEROP_REG_H1C8");
		ive_filterop_c->REG_h14.reg_filterop_3ch_en = 1;
		pr_info("Image enType %d\n", pstSrc->enType);
		break;
	default:
		pr_info("Invalid Image enType %d\n", pstSrc->enType);
		return CVI_FAILURE;
		break;
	}
	writed(ive_filterop_c->REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	writed(ive_filterop_c->REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	if (isEmit) {
		// NOTICE: need to first set it
		ive_top_c.REG_h10.reg_img_in_top_enable = 1;
		writed(ive_top_c.REG_h10.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
		       "IVE_TOP_REG_H10");

		setImgSrc1(ndev, pstSrc);

		// trigger filterop
		//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c.REG_20.reg_frame2op_op_mode = 5;
		// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
		ive_top_c.REG_h10.reg_filterop_top_enable = 1;
		writed(ive_top_c.REG_20.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
		       "IVE_TOP_REG_20");
		writed(ive_top_c.REG_h80.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
		       "IVE_TOP_REG_H80");
		writed(ive_top_c.REG_h10.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
		       "IVE_TOP_REG_H10");
		if (pstDst->enType == IVE_IMAGE_TYPE_U8C1) {
			setImgDst1(ndev, pstDst);

			ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
			ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
			writed(ive_filterop_c->REG_h14.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_H14),
			       "IVE_FILTEROP_REG_H14");
		} else {
			setOdma(ndev, pstDst, pstDst->u16Width,
				pstDst->u16Height);

			// NOTICE: test img_in = odma
			ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
			writed(ive_filterop_c->REG_h14.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_H14),
			       "IVE_FILTEROP_REG_H14");
		}

		cvi_ive_go(ndev, &ive_top_c, bInstant);
	}
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_Filter(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,
		       CVI_BOOL bInstant)
{
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	return _cvi_ive_filter(ndev, pstSrc, pstDst, pstFltCtrl, bInstant,
			       &ive_filterop_c, true);
}

CVI_S32 _cvi_ive_csc(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		     IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl,
		     CVI_BOOL bInstant, IVE_FILTEROP_C *ive_filterop_c,
		     CVI_BOOL isEmit)
{
	pr_info("[TODO] CVI_MPI_IVE_Filter not support\n");
#if 0
	DEFINE_IMG_IN_C(img_in_c);
	DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c.REG_2.reg_img_heightm1 = pstDst->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstDst->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");
	// setting
	switch(ive_top_c.REG_14.reg_csc_enmode) {
	case IVE_CSC_MODE_PIC_BT601_YUV2HSV:
	case IVE_CSC_MODE_PIC_BT709_YUV2HSV:
	case IVE_CSC_MODE_PIC_BT601_YUV2LAB:
	case IVE_CSC_MODE_PIC_BT709_YUV2LAB:
		// "update rgb2hsv/rgb2lab table value by software
		//0:use const, 1:update table by reg_csc_tab_sw_0 and reg_csc_tab_sw_1"
		if (pstSrc->pu8VirAddr[2]) {
			ive_top_c.REG_12.reg_csc_tab_sw_update   = 1;
			//unsigned 15 bit, update table value of inv_h_tab(rgb2hsv) or xyz_tab(rgb2lab) when reg_csc_tab_sw_update == 1
			ive_top_c.REG_11.reg_csc_tab_sw_1 = pstSrc->pu8VirAddr[2];

			// unsigned 12 bit, update table value of inv_v_tab(rgb2hsv) or gamma_tab(rgb2lab) when reg_csc_tab_sw_update == 1
			ive_top_c.REG_11.reg_csc_tab_sw_0 = pstSrc->pu8VirAddr[2] + (6144 / 2);
			writed(ive_top_c.REG_12.val, (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_12), "IVE_TOP_REG_12");
			writed(ive_top_c.REG_11.val, (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_11), "IVE_TOP_REG_11");
		}
		break;
	default:
		break;
	}

	img_in_c.REG_00.reg_auto_csc_en = 0;
	writed(img_in_c.REG_00.val,
	       (ndev->ive_base + IVE_BLK_BA_IMG_IN + IMG_IN_REG_00),
	       "IMG_IN_REG_00");
	ive_filterop_c->REG_h10.reg_filterop_mode = 1;
	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enable = 1;
	ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enmode =
		pstCscCtrl->enMode;
	ive_filterop_c->REG_h14.reg_filterop_3ch_en = 0;
	writed(ive_filterop_c->REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	writed(ive_filterop_c->REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");
	writed(ive_filterop_c->REG_h1c8.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H1C8),
	       "IVE_FILTEROP_REG_H1C8");

	if (1) {
		// default table align cmodel
		// TODO: export to user
		CVI_S32 coef_BT601_to_GBR_16_235[12] = {
			1024, 0,      1404, 179188, 1024, 344,
			715,  136040, 1024, 1774,   0,	  226505
		}; // 0
		CVI_S32 coef_BT709_to_GBR_16_235[12] = {
			1024, 0,     1577, 201339, 1024, 187,
			470,  84660, 1024, 1860,   0,	 237515
		}; // 1
		CVI_S32 coef_BT601_to_GBR_0_255[12] = {
			1192, 0,      1634, 227750, 1192, 400,
			833,  139252, 1192, 2066,   0,	  283062
		}; // 2
		CVI_S32 coef_BT709_to_GBR_0_255[12] = {
			1192, 0,     1836, 253571, 1192, 218,
			547,  79352, 1192, 2166,   0,	 295776
		}; // 3
		CVI_S32 coef_RGB_to_BT601_0_255[12] = { 306, 601, 117, 512,
							176, 347, 523, 131584,
							523, 438, 85,  131584 };
		CVI_S32 coef_RGB_to_BT709_0_255[12] = {
			218, 732,    74,  512, 120, 403,
			523, 131584, 523, 475, 48,  131584
		}; // 9
		CVI_S32 coef_RGB_to_BT601_16_235[12] = {
			263, 516,    100, 16896, 152, 298,
			450, 131584, 450, 377,	 73,  131584
		}; // 10
		CVI_S32 coef_RGB_to_BT709_16_235[12] = {
			187, 629,    63,  16896, 103, 346,
			450, 131584, 450, 409,	 41,  131584
		}; // 11

		CVI_S32 *tbl = NULL;
		switch(pstCscCtrl->enMode) {
		case IVE_CSC_MODE_VIDEO_BT601_YUV2RGB:
			tbl = coef_BT601_to_GBR_16_235;
			break;
		case IVE_CSC_MODE_VIDEO_BT709_YUV2RGB:
			tbl = coef_BT709_to_GBR_16_235;
			break;
		case IVE_CSC_MODE_PIC_BT601_YUV2RGB:
			tbl = coef_BT601_to_GBR_0_255;
			break;
		case IVE_CSC_MODE_PIC_BT709_YUV2RGB:
			tbl = coef_BT709_to_GBR_0_255;
			break;
		case IVE_CSC_MODE_VIDEO_BT601_RGB2YUV:
			tbl = coef_RGB_to_BT601_0_255;
			break;
		case IVE_CSC_MODE_VIDEO_BT709_RGB2YUV:
			tbl = coef_RGB_to_BT709_0_255;
			break;
		case IVE_CSC_MODE_PIC_BT601_RGB2YUV:
			tbl = coef_RGB_to_BT601_16_235;
			break;
		case IVE_CSC_MODE_PIC_BT709_RGB2YUV:
			tbl = coef_RGB_to_BT709_16_235;
			break;
		default:
			break;
		}

		if (tbl) {
			ive_filterop_c->REG_h194
				.reg_filterop_op2_csc_coeff_sw_update = 1;
			ive_filterop_c->REG_CSC_COEFF_0
				.reg_filterop_op2_csc_coeff_sw_00 = tbl[0];
			ive_filterop_c->REG_CSC_COEFF_1
				.reg_filterop_op2_csc_coeff_sw_01 = tbl[1];
			ive_filterop_c->REG_CSC_COEFF_2
				.reg_filterop_op2_csc_coeff_sw_02 = tbl[2];
			ive_filterop_c->REG_CSC_COEFF_3
				.reg_filterop_op2_csc_coeff_sw_03 = tbl[3];
			ive_filterop_c->REG_CSC_COEFF_4
				.reg_filterop_op2_csc_coeff_sw_04 = tbl[4];
			ive_filterop_c->REG_CSC_COEFF_5
				.reg_filterop_op2_csc_coeff_sw_05 = tbl[5];
			ive_filterop_c->REG_CSC_COEFF_6
				.reg_filterop_op2_csc_coeff_sw_06 = tbl[6];
			ive_filterop_c->REG_CSC_COEFF_7
				.reg_filterop_op2_csc_coeff_sw_07 = tbl[7];
			ive_filterop_c->REG_CSC_COEFF_8
				.reg_filterop_op2_csc_coeff_sw_08 = tbl[8];
			ive_filterop_c->REG_CSC_COEFF_9
				.reg_filterop_op2_csc_coeff_sw_09 = tbl[9];
			ive_filterop_c->REG_CSC_COEFF_A
				.reg_filterop_op2_csc_coeff_sw_10 = tbl[10];
			ive_filterop_c->REG_CSC_COEFF_B
				.reg_filterop_op2_csc_coeff_sw_11 = tbl[11];
			writed(ive_filterop_c->REG_h194.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_H194),
			       "IVE_FILTEROP_REG_H194");
			writed(ive_filterop_c->REG_CSC_COEFF_0.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_0),
			       "IVE_FILTEROP_REG_CSC_COEFF_0");
			writed(ive_filterop_c->REG_CSC_COEFF_1.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_1),
			       "IVE_FILTEROP_REG_CSC_COEFF_1");
			writed(ive_filterop_c->REG_CSC_COEFF_2.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_2),
			       "IVE_FILTEROP_REG_CSC_COEFF_2");
			writed(ive_filterop_c->REG_CSC_COEFF_3.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_3),
			       "IVE_FILTEROP_REG_CSC_COEFF_3");
			writed(ive_filterop_c->REG_CSC_COEFF_4.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_4),
			       "IVE_FILTEROP_REG_CSC_COEFF_4");
			writed(ive_filterop_c->REG_CSC_COEFF_5.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_5),
			       "IVE_FILTEROP_REG_CSC_COEFF_5");
			writed(ive_filterop_c->REG_CSC_COEFF_6.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_6),
			       "IVE_FILTEROP_REG_CSC_COEFF_6");
			writed(ive_filterop_c->REG_CSC_COEFF_7.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_7),
			       "IVE_FILTEROP_REG_CSC_COEFF_7");
			writed(ive_filterop_c->REG_CSC_COEFF_8.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_8),
			       "IVE_FILTEROP_REG_CSC_COEFF_8");
			writed(ive_filterop_c->REG_CSC_COEFF_9.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_9),
			       "IVE_FILTEROP_REG_CSC_COEFF_9");
			writed(ive_filterop_c->REG_CSC_COEFF_A.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_A),
			       "IVE_FILTEROP_REG_CSC_COEFF_A");
			writed(ive_filterop_c->REG_CSC_COEFF_B.val,
			       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
				IVE_FILTEROP_REG_CSC_COEFF_B),
			       "IVE_FILTEROP_REG_CSC_COEFF_B");
		}
	}
	if (isEmit) {
		// NOTICE: need to first set it
		ive_top_c.REG_h10.reg_img_in_top_enable = 1;
		writed(ive_top_c.REG_h10.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
		       "IVE_TOP_REG_H10");

		setImgSrc1(ndev, pstSrc);
		ive_top_c.REG_h10.reg_filterop_top_enable = 1;
		writed(ive_top_c.REG_h10.val,
		       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
		       "IVE_TOP_REG_H10");
		setOdma(ndev, pstDst, pstDst->u16Width, pstDst->u16Height);

		cvi_ive_go(ndev, &ive_top_c, bInstant);
	}
#endif
	return CVI_SUCCESS;
}

CVI_S32 cvi_ive_CSC(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl,
		    CVI_BOOL bInstant)
{
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	pr_info("CVI_MPI_IVE_FilterAndCSC\n");
	return _cvi_ive_csc(ndev, pstSrc, pstDst, pstCscCtrl, bInstant,
			    &ive_filterop_c, true);
}

CVI_S32 cvi_ive_FilterAndCSC(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			     IVE_FILTER_AND_CSC_CTRL_S *pstFltCscCtrl,
			     CVI_BOOL bInstant)
{
	IVE_CSC_CTRL_S stCscCtrl;
	IVE_FILTER_CTRL_S stFltCtrl;
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	pr_info("CVI_MPI_IVE_FilterAndCSC\n");
	if (pstSrc->enType == IVE_IMAGE_TYPE_YUV420SP ||
	    pstSrc->enType == IVE_IMAGE_TYPE_YUV422SP) {
	} else {
		// CVI_ASSERT(0 && "only support input fmt YUV420SP/YUV422SP\n");
		// return CVI_FAILURE;
		pr_info("only support input fmt YUV420SP/YUV422SP??\n");
		return CVI_FAILURE;
	}

	// set filter
	memcpy(stFltCtrl.as8Mask, pstFltCscCtrl->as8Mask, sizeof(CVI_S8) * 24);
	stFltCtrl.as8Mask[24] = pstFltCscCtrl->as8Mask[24];
	stFltCtrl.u32Norm = pstFltCscCtrl->u16Norm;
	_cvi_ive_filter(ndev, pstSrc, pstDst, &stFltCtrl, bInstant,
			&ive_filterop_c, /*isEmit=*/false);

	// set csc
	stCscCtrl.enMode = pstFltCscCtrl->enMode;
	// leverage CSC
	ive_filterop_c.REG_h14.reg_filterop_3ch_en = 1;
	ive_filterop_c.REG_h1c8.reg_filterop_op2_csc_enable = 1;
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");
	writed(ive_filterop_c.REG_h1c8.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H1C8),
	       "IVE_FILTEROP_REG_H1C8");

	return _cvi_ive_csc(ndev, pstSrc, pstDst, &stCscCtrl, bInstant,
			    &ive_filterop_c, /*isEmit=*/true);
}

CVI_S32 cvi_ive_Sobel(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
		      IVE_SOBEL_CTRL_S *pstSobelCtrl, CVI_BOOL bInstant)
{
	IVE_DST_IMAGE_S *firstOut = pstDstH;
	IVE_DST_IMAGE_S *secondOut = pstDstV;
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	pr_info("CVI_MPI_IVE_Sobel\n");
	ive_top_c.REG_2.reg_img_heightm1 = pstSrc->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstSrc->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");
	ive_filterop_c.REG_h10.reg_filterop_mode = 9;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	ive_filterop_c.REG_4.reg_filterop_h_coef00 = pstSobelCtrl->as8Mask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 = pstSobelCtrl->as8Mask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 = pstSobelCtrl->as8Mask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 = pstSobelCtrl->as8Mask[3];
	ive_filterop_c.REG_5.reg_filterop_h_coef04 = pstSobelCtrl->as8Mask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 = pstSobelCtrl->as8Mask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 = pstSobelCtrl->as8Mask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 = pstSobelCtrl->as8Mask[7];
	ive_filterop_c.REG_6.reg_filterop_h_coef13 = pstSobelCtrl->as8Mask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 = pstSobelCtrl->as8Mask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 = pstSobelCtrl->as8Mask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 = pstSobelCtrl->as8Mask[11];
	ive_filterop_c.REG_7.reg_filterop_h_coef22 = pstSobelCtrl->as8Mask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 = pstSobelCtrl->as8Mask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 = pstSobelCtrl->as8Mask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 = pstSobelCtrl->as8Mask[15];
	ive_filterop_c.REG_8.reg_filterop_h_coef31 = pstSobelCtrl->as8Mask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 = pstSobelCtrl->as8Mask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 = pstSobelCtrl->as8Mask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 = pstSobelCtrl->as8Mask[19];
	ive_filterop_c.REG_9.reg_filterop_h_coef40 = pstSobelCtrl->as8Mask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 = pstSobelCtrl->as8Mask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 = pstSobelCtrl->as8Mask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 = pstSobelCtrl->as8Mask[23];
	ive_filterop_c.REG_10.reg_filterop_h_coef44 = pstSobelCtrl->as8Mask[24];
	writed(ive_filterop_c.REG_4.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_4),
	       "IVE_FILTEROP_REG_4");
	writed(ive_filterop_c.REG_5.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_5),
	       "IVE_FILTEROP_REG_5");
	writed(ive_filterop_c.REG_6.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_6),
	       "IVE_FILTEROP_REG_6");
	writed(ive_filterop_c.REG_7.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_7),
	       "IVE_FILTEROP_REG_7");
	writed(ive_filterop_c.REG_8.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_8),
	       "IVE_FILTEROP_REG_8");
	writed(ive_filterop_c.REG_9.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_9),
	       "IVE_FILTEROP_REG_9");
	writed(ive_filterop_c.REG_10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_10),
	       "IVE_FILTEROP_REG_10");

	//iveReg->dstEnType = (firstOut) ? firstOut->enType : secondOut->enType;

	//iveReg->setDMA(pstSrc, firstOut, NULL, secondOut);

	if (pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_BOTH ||
	    pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_HOR ||
	    pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_VER) {
		// valid
		// "0 : h , v  -> wdma_y wdma_c will be active 1 :h only -> wdma_y 2: v only -> wdma_c 3. h , v pack => {v ,h} -> wdma_y "
		ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl =
			(int)pstSobelCtrl->enOutCtrl;
		writed(ive_filterop_c.REG_110.val,
		       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
			IVE_FILTEROP_REG_110),
		       "IVE_FILTEROP_REG_110");
	} else {
		pr_info("[IVE] not support enOutCtrl\n");
		return CVI_FAILURE;
	}
	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	setImgSrc1(ndev, pstSrc);

	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'd5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");

	// "0 : U8 1 : S16 2 : U16"
	// FIXME: check enable in Sobel
	ive_filterop_c.REG_110.reg_filterop_map_enmode = 1;
	ive_top_c.REG_h10.reg_filterop_top_enable = 1;

	if ((int)pstSobelCtrl->enOutCtrl == 0) {
		setImgDst2(ndev, secondOut);
		setImgDst1(ndev, firstOut);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	} else if ((int)pstSobelCtrl->enOutCtrl == 2) {
		setImgDst2(ndev, secondOut);
		setImgDst1(ndev, firstOut);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	} else {
		setImgDst1(ndev, firstOut);
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	}
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	writed(ive_filterop_c.REG_110.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_110),
	       "IVE_FILTEROP_REG_110");
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	return cvi_ive_go(ndev, &ive_top_c, bInstant);
}

CVI_S32 CVI_MPI_IVE_MagAndAng(struct cvi_ive_device *ndev,
			      IVE_SRC_IMAGE_S *pstSrc,
			      IVE_DST_IMAGE_S *pstDstMag,
			      IVE_DST_IMAGE_S *pstDstAng,
			      IVE_MAG_AND_ANG_CTRL_S *pstMagAndAngCtrl,
			      CVI_BOOL bInstant)
{
	DEFINE_IVE_TOP_C(ive_top_c);
	DEFINE_IVE_FILTEROP_C(ive_filterop_c);

	pr_info("CVI_MPI_IVE_MagAndAng\n");
	ive_top_c.REG_2.reg_img_heightm1 = pstSrc->u16Height - 1;
	ive_top_c.REG_2.reg_img_widthm1 = pstSrc->u16Width - 1;
	writed(ive_top_c.REG_2.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_2),
	       "IVE_TOP_REG_2");

	ive_filterop_c.REG_h10.reg_filterop_mode = 7;
	writed(ive_filterop_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H10),
	       "IVE_FILTEROP_REG_H10");
	ive_filterop_c.REG_4.reg_filterop_h_coef00 =
		pstMagAndAngCtrl->as8Mask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 =
		pstMagAndAngCtrl->as8Mask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 =
		pstMagAndAngCtrl->as8Mask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 =
		pstMagAndAngCtrl->as8Mask[3];
	ive_filterop_c.REG_5.reg_filterop_h_coef04 =
		pstMagAndAngCtrl->as8Mask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 =
		pstMagAndAngCtrl->as8Mask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 =
		pstMagAndAngCtrl->as8Mask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 =
		pstMagAndAngCtrl->as8Mask[7];
	ive_filterop_c.REG_6.reg_filterop_h_coef13 =
		pstMagAndAngCtrl->as8Mask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 =
		pstMagAndAngCtrl->as8Mask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 =
		pstMagAndAngCtrl->as8Mask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 =
		pstMagAndAngCtrl->as8Mask[11];
	ive_filterop_c.REG_7.reg_filterop_h_coef22 =
		pstMagAndAngCtrl->as8Mask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 =
		pstMagAndAngCtrl->as8Mask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 =
		pstMagAndAngCtrl->as8Mask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 =
		pstMagAndAngCtrl->as8Mask[15];
	ive_filterop_c.REG_8.reg_filterop_h_coef31 =
		pstMagAndAngCtrl->as8Mask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 =
		pstMagAndAngCtrl->as8Mask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 =
		pstMagAndAngCtrl->as8Mask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 =
		pstMagAndAngCtrl->as8Mask[19];
	ive_filterop_c.REG_9.reg_filterop_h_coef40 =
		pstMagAndAngCtrl->as8Mask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 =
		pstMagAndAngCtrl->as8Mask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 =
		pstMagAndAngCtrl->as8Mask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 =
		pstMagAndAngCtrl->as8Mask[23];
	ive_filterop_c.REG_10.reg_filterop_h_coef44 =
		pstMagAndAngCtrl->as8Mask[24];
	writed(ive_filterop_c.REG_4.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_4),
	       "IVE_FILTEROP_REG_4");
	writed(ive_filterop_c.REG_5.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_5),
	       "IVE_FILTEROP_REG_5");
	writed(ive_filterop_c.REG_6.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_6),
	       "IVE_FILTEROP_REG_6");
	writed(ive_filterop_c.REG_7.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_7),
	       "IVE_FILTEROP_REG_7");
	writed(ive_filterop_c.REG_8.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_8),
	       "IVE_FILTEROP_REG_8");
	writed(ive_filterop_c.REG_9.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_9),
	       "IVE_FILTEROP_REG_9");
	writed(ive_filterop_c.REG_10.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_10),
	       "IVE_FILTEROP_REG_10");

	ive_filterop_c.REG_18.reg_filterop_mag_thr = pstMagAndAngCtrl->u16Thr;
	writed(ive_filterop_c.REG_18.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_18),
	       "IVE_FILTEROP_REG_18");
	// NOTICE: need to first set it
	ive_top_c.REG_h10.reg_img_in_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");

	setImgSrc1(ndev, pstSrc);

	ive_top_c.REG_20.reg_frame2op_op_mode = 5;
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	writed(ive_top_c.REG_20.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_20),
	       "IVE_TOP_REG_20");
	writed(ive_top_c.REG_h80.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H80),
	       "IVE_TOP_REG_H80");

	// "0 : U8 1 : S16 2 : U16"
	ive_filterop_c.REG_110.reg_filterop_map_enmode = 1;
	ive_top_c.REG_h10.reg_filterop_top_enable = 1;
	writed(ive_top_c.REG_h10.val,
	       (ndev->ive_base + IVE_BLK_BA_IVE_TOP + IVE_TOP_REG_H10),
	       "IVE_TOP_REG_H10");
	if (pstMagAndAngCtrl->enOutCtrl ==
	    IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG) {
		//setDMA(pstSrc, pstDstAng, NULL, pstDstMag);
		ive_filterop_c.REG_110.reg_filterop_magang_out_ctrl = 1;
		setImgDst2(ndev, pstDstMag);
		setImgDst1(ndev, pstDstAng);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	} else {
		//setDMA(pstSrc, NULL, NULL, pstDstMag);
		ive_filterop_c.REG_110.reg_filterop_magang_out_ctrl = 0;
		setImgDst2(ndev, pstDstMag);
		//setImgDst1(ndev, pstDstAng);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	}
	writed(ive_filterop_c.REG_110.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_110),
	       "IVE_FILTEROP_REG_110");
	writed(ive_filterop_c.REG_h14.val,
	       (ndev->ive_base + IVE_BLK_BA_FILTEROP + 0x200 +
		IVE_FILTEROP_REG_H14),
	       "IVE_FILTEROP_REG_H14");

	return cvi_ive_go(ndev, &ive_top_c, bInstant);
}

irqreturn_t platform_ive_irq(struct cvi_ive_device *ndev)
{
	pr_info("[IVE] got %s callback\n", __func__);
	complete(&ndev->ive_done);
	return 0;
}

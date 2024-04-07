/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_cif.c
 * Description: camera interface driver
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/irq.h>
#include <linux/reset.h>
#include <generated/compile.h>
#include <linux/io.h>
#include <linux/clk.h>
#include "pinctrl-cv1822.h"
#include <linux/ctype.h>
#include <linux/version.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#endif

#include "uapi/cvi_vip_cif.h"
#include "uapi/cvi_vip_snsr.h"
#include "cif/cif_drv.h"
#include "cvi_vip_cif.h"
#include "vip/vip_common.h"

#ifdef CONFIG_COMPAT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/compat.h>
#endif
#endif

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#define MIPI_RX_DEV_NAME "cvi-mipi-rx"

enum {
	LANE_SKEW_CROSS_CLK,
	LANE_SKEW_CROSS_DATA_NEAR,
	LANE_SKEW_CROSS_DATA_FAR,
	LANE_SKEW_CLK,
	LANE_SKEW_DATA,
	LANE_SKEW_NUM,
};

static int mclk0 = CAMPLL_FREQ_NONE;
module_param(mclk0, int, 0644);
MODULE_PARM_DESC(mclk0, "cam0 mclk");

static int mclk1 = CAMPLL_FREQ_NONE;
module_param(mclk1, int, 0644);
MODULE_PARM_DESC(mclk1, "cam1 mclk");

static int lane_phase[LANE_SKEW_NUM] = {0x00, 0x03, 0x08, 0x00, 0x03};
static int count;
module_param_array(lane_phase, int, &count, 0664);

static int bypass_mac_clk;
module_param(bypass_mac_clk, int, 0644);
MODULE_PARM_DESC(bypass_mac_clk, "byass mac clk");

static unsigned int max_mac_clk = 600;
module_param(max_mac_clk, uint, 0644);
MODULE_PARM_DESC(max_mac_clk, "max mac clk");

static int cif_set_output_clk_edge(struct cvi_cif_dev *dev,
				   struct clk_edge_s *clk_edge);

static int cif_set_hs_mode(struct cvi_cif_dev *dev, unsigned int mode);

const struct sync_code_s default_sync_code = {
	.norm_bk_sav = 0xAB0,
	.norm_bk_eav = 0xB60,
	.norm_sav = 0x800,
	.norm_eav = 0x9D0,
	.n0_bk_sav = 0x2B0,
	.n0_bk_eav = 0x360,
	.n1_bk_sav = 0x6B0,
	.n1_bk_eav = 0x760,
};

static struct cvi_link *ctx_to_link(const struct cif_ctx *ctx)
{
	return container_of(ctx, struct cvi_link, cif_ctx);
}

static struct cvi_cif_dev *file_cif_dev(struct file *file)
{
	return container_of(file->private_data, struct cvi_cif_dev, miscdev);
}

const char *_to_string_input_mode(enum input_mode_e input_mode)
{
	switch (input_mode) {
	case INPUT_MODE_MIPI:
		return "MIPI";
	case INPUT_MODE_SUBLVDS:
		return "SUBLVDS";
	case INPUT_MODE_HISPI:
		return "HISPI";
	case INPUT_MODE_CMOS:
		return "CMOS";
	case INPUT_MODE_BT1120:
		return "BT1120";
	case INPUT_MODE_BT601_19B_VHS:
		return "INPUT_MODE_BT601_19B_VHS";
	case INPUT_MODE_BT656_9B:
		return "INPUT_MODE_BT656_9B";
	case INPUT_MODE_CUSTOM_0:
		return "INPUT_MODE_CUSTOM_0";
	case INPUT_MODE_BT_DEMUX:
		return "INPUT_MODE_BT_DEMUX";
	default:
		return "unknown";
	}
}

const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk)
{
	switch (mac_clk) {
	case RX_MAC_CLK_200M:
		return "200MHZ";
	case RX_MAC_CLK_400M:
		return "400MHZ";
	case RX_MAC_CLK_500M:
		return "500MHZ";
	case RX_MAC_CLK_600M:
		return "600MHZ";
	default:
		return "unknown";
	}
}

const char *_to_string_cmd(unsigned int cmd)
{
	switch (cmd) {
	case CVI_MIPI_SET_DEV_ATTR:
		return "CVI_MIPI_SET_DEV_ATTR";
	case CVI_MIPI_SET_HS_MODE:
		return "CVI_MIPI_SET_HS_MODE";
	case CVI_MIPI_SET_OUTPUT_CLK_EDGE:
		return "CVI_MIPI_SET_OUTPUT_CLK_EDGE";
	case CVI_MIPI_RESET_MIPI:
		return "CVI_MIPI_RESET_MIPI";
	case CVI_MIPI_SET_CROP_TOP:
		return "CVI_MIPI_SET_CROP_TOP";
	case CVI_MIPI_SET_WDR_MANUAL:
		return "CVI_MIPI_SET_WDR_MANUAL";
	case CVI_MIPI_SET_LVDS_FP_VS:
		return "CVI_MIPI_SET_LVDS_FP_VS";
	case CVI_MIPI_RESET_SENSOR:
		return "CVI_MIPI_RESET_SENSOR";
	case CVI_MIPI_UNRESET_SENSOR:
		return "CVI_MIPI_UNRESET_SENSOR";
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		return "CVI_MIPI_ENABLE_SENSOR_CLOCK";
	case CVI_MIPI_DISABLE_SENSOR_CLOCK:
		return "CVI_MIPI_DISABLE_SENSOR_CLOCK";
	case CVI_MIPI_RESET_LVDS:
		return "CVI_MIPI_RESET_LVDS";
	case CVI_MIPI_GET_CIF_ATTR:
		return "CVI_MIPI_GET_CIF_ATTR";
	case CVI_MIPI_SET_MAX_MAC_CLOCK:
		return "CVI_MIPI_SET_MAX_MAC_CLOCK";
	default:
		return "unknown";
	}
	return "unknown";
}

const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type)
{
	switch (raw_data_type) {
	case RAW_DATA_8BIT:
		return "RAW8";
	case RAW_DATA_10BIT:
		return "RAW10";
	case RAW_DATA_12BIT:
		return "RAW12";
	case YUV422_8BIT:
		return "YUV422_8BIT";
	case YUV422_10BIT:
		return "YUV422_10BIT";
	default:
		return "unknown";
	}
}

const char *_to_string_mipi_wdr_mode(enum mipi_wdr_mode_e wdr)
{
	switch (wdr) {
	case CVI_MIPI_WDR_MODE_NONE:
		return "NONE";
	case CVI_MIPI_WDR_MODE_VC:
		return "VC";
	case CVI_MIPI_WDR_MODE_DT:
		return "DT";
	case CVI_MIPI_WDR_MODE_DOL:
		return "DOL";
	case CVI_MIPI_WDR_MODE_MANUAL:
		return "MANUAL";
	default:
		return "unknown";
	}
}

const char *_to_string_mipi_vsync_gen_mode(enum mipi_vs_gen_mode mode)
{
	switch (mode) {
	case CSI_VSYNC_BY_FS:
		return "FS";
	case CSI_VSYNC_BY_FE:
		return "FE";
	case CSI_VSYNC_BY_FS_FE:
		return "FS & FE";
	default:
		return "unknown";
	}
}

const char *_to_string_wdr_mode(enum wdr_mode_e wdr)
{
	switch (wdr) {
	case CVI_WDR_MODE_NONE:
		return "NONE";
	case CVI_WDR_MODE_2F:
		return "2To1";
	case CVI_WDR_MODE_3F:
		return "3To1";
	case CVI_WDR_MODE_DOL_2F:
		return "DOL2To1";
	case CVI_WDR_MODE_DOL_3F:
		return "DOL3To1";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_sync_mode(enum lvds_sync_mode_e mode)
{
	switch (mode) {
	case LVDS_SYNC_MODE_SOF:
		return "SOF";
	case LVDS_SYNC_MODE_SAV:
		return "SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_bit_endian(enum lvds_bit_endian endian)
{
	switch (endian) {
	case LVDS_ENDIAN_LITTLE:
		return "LITTLE";
	case LVDS_ENDIAN_BIG:
		return "BIG";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_vsync_type(enum lvds_vsync_type_e type)
{
	switch (type) {
	case LVDS_VSYNC_NORMAL:
		return "NORMAL";
	case LVDS_VSYNC_SHARE:
		return "SHARE";
	case LVDS_VSYNC_HCONNECT:
		return "HCONNECT";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_fid_type(enum lvds_fid_type_e type)
{
	switch (type) {
	case LVDS_FID_NONE:
		return "FID_NONE";
	case LVDS_FID_IN_SAV:
		return "FID_IN_SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_mclk(enum cam_pll_freq_e freq)
{
	switch (freq) {
	case CAMPLL_FREQ_NONE:
		return "CAMPLL_FREQ_NONE";
	case CAMPLL_FREQ_37P125M:
		return "CAMPLL_FREQ_37P125M";
	case CAMPLL_FREQ_25M:
		return "CAMPLL_FREQ_25M";
	case CAMPLL_FREQ_27M:
		return "CAMPLL_FREQ_27M";
	case CAMPLL_FREQ_24M:
		return "CAMPLL_FREQ_24M";
	case CAMPLL_FREQ_26M:
		return "CAMPLL_FREQ_26M";
	default:
		return "unknown";
	}
}

const char *_to_string_csi_decode(enum csi_decode_fmt_e fmt)
{
	switch (fmt) {
	case DEC_FMT_YUV422_8:
		return "yuv422-8";
	case DEC_FMT_YUV422_10:
		return "yuv422-10";
	case DEC_FMT_RAW8:
		return "raw8";
	case DEC_FMT_RAW10:
		return "raw10";
	case DEC_FMT_RAW12:
		return "raw12";
	default:
		return "unknown";
	}
}

const char *_to_string_dlane_state(enum mipi_dlane_state_e state)
{
	switch (state) {
	case HS_IDLE:
		return "hs_idle";
	case HS_SYNC:
		return "hs_sync";
	case HS_SKEW_CAL:
		return "skew_cal";
	case HS_ALT_CAL:
		return "alt_cal";
	case HS_PREAMPLE:
		return "preample";
	case HS_HST:
		return "hs_hst";
	case HS_ERR:
		return "hs_err";
	default:
		return "unknown";
	}
}

const char *_to_string_deskew_state(enum mipi_deskew_state_e state)
{
	switch (state) {
	case DESKEW_IDLE:
		return "idle";
	case DESKEW_START:
		return "start";
	case DESKEW_DONE:
		return "done";
	default:
		return "unknown";
	}
}

static void cif_dump_dev_attr(struct cvi_cif_dev *dev,
			      struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	int i;

	dev_dbg(_dev, "devno = %d, input_mode = %s, mac_clk = %s\n",
		attr->devno,
		_to_string_input_mode(attr->input_mode),
		_to_string_mac_clk(attr->mac_clk));
	dev_dbg(_dev, "mclk%d = %s\n",
		attr->mclk.cam, _to_string_mclk(attr->mclk.freq));
	dev_dbg(_dev, "width = %d, height = %d\n",
		attr->img_size.width, attr->img_size.height);
	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		struct mipi_dev_attr_s *mipi = &attr->mipi_attr;

		dev_dbg(_dev, "raw_data_type = %s\n",
			_to_string_raw_data_type(mipi->raw_data_type));

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg(_dev, "lane_id[%d] = %d, pn_swap = %s", i,
				mipi->lane_id[i],
				mipi->pn_swap[i] ? "True" : "False");
		}
		dev_dbg(_dev, "wdr_mode = %s\n",
			_to_string_mipi_wdr_mode(mipi->wdr_mode));
		for (i = 0; i < WDR_VC_NUM; i++) {
			dev_dbg(_dev, "data_type[%d] = 0x%x\n",
				i, mipi->data_type[i]);
		}
		dev_dbg(_dev, "vs_gen_mode = %s\n",
			_to_string_mipi_vsync_gen_mode(mipi->vs_gen_mode));
	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		int j;

		dev_dbg(_dev, "wdr_mode = %s\n",
			_to_string_wdr_mode(lvds->wdr_mode));
		dev_dbg(_dev, "sync_mode = %s\n",
			_to_string_lvds_sync_mode(lvds->sync_mode));
		dev_dbg(_dev, "raw_data_type = %s\n",
			_to_string_raw_data_type(lvds->raw_data_type));
		dev_dbg(_dev, "data_endian = %s\n",
			_to_string_bit_endian(lvds->data_endian));
		dev_dbg(_dev, "sync_code_endian = %s\n",
			_to_string_bit_endian(lvds->sync_code_endian));
		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg(_dev, "lane_id[%d] = %d, pn_swap = %s ", i,
				lvds->lane_id[i],
				lvds->pn_swap[i] ? "True" : "False");
		}
		dev_dbg(_dev, "sync code = {\n");
		for (i = 0; i < MIPI_LANE_NUM; i++) {
			dev_dbg(_dev, "\t{\n");
			for (j = 0; j < WDR_VC_NUM+1; j++) {
				dev_dbg(_dev,
					"\t\t{ %3x, %3x, %3x, %3x },\n",
					lvds->sync_code[i][j][0],
					lvds->sync_code[i][j][1],
					lvds->sync_code[i][j][2],
					lvds->sync_code[i][j][3]);
			}
			dev_dbg(_dev, "\t},\n");
		}
		dev_dbg(_dev, "}\n");
		dev_dbg(_dev, "vsync_type = %s\n",
			_to_string_lvds_vsync_type(lvds->vsync_type.sync_type));
		dev_dbg(_dev, "fid = %s\n",
			_to_string_lvds_fid_type(lvds->fid_type.fid));

	}
	break;
	case INPUT_MODE_CMOS:
		break;
	case INPUT_MODE_BT1120:
		break;
	default:
		break;
	}
}

#define LANE_IS_PORT1(x)	((x > 2) ? 1 : 0)
#define IS_SAME_PORT(x, y)	(!((LANE_IS_PORT1(x))^(LANE_IS_PORT1(y))))

#define PAD_CTRL_PU		BIT(2)
#define PAD_CTRL_PD		BIT(3)
#define PSD_CTRL_OFFSET(n)	((5 - n) * 8)

static int _cif_set_attr_mipi(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct mipi_dev_attr_s *attr)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, mipi_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_csi *csi = &param->cfg.csi;
	struct mipi_demux_info_s *info = &attr->demux;
	uint8_t tbl = 0x1F;
	int i, j = 0, clk_port = 0;
	uint32_t value;

	param->type = CIF_TYPE_CSI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		csi->fmt = CSI_RAW_8;
		csi->decode_type = 0x2A;
		break;
	case RAW_DATA_10BIT:
		csi->fmt = CSI_RAW_10;
		csi->decode_type = 0x2B;
		break;
	case RAW_DATA_12BIT:
		csi->fmt = CSI_RAW_12;
		csi->decode_type = 0x2C;
		break;
	case YUV422_8BIT:
		csi->fmt = CSI_YUV422_8B;
		csi->decode_type = 0x1E;
		break;
	case YUV422_10BIT:
		csi->fmt = CSI_YUV422_10B;
		csi->decode_type = 0x1F;
		break;
	default:
		return -EINVAL;
	}

	/*workaround, for uyvy and vyuy, set csi format raw16*/
	if (attr->yuv_data_type == YUV422_UYVY || attr->yuv_data_type == YUV422_VYUY)
		cif_set_csi_format_force(ctx, 1, FMT_RAW16);
	else
		cif_set_csi_format_force(ctx, 0, FMT_RAW16);

	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	csi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	if (csi->lane_num == 4) {
		if (LANE_IS_PORT1(attr->lane_id[0]))
			cif_set_clk_dir(ctx, CIF_CLK_P12P0);
		else
			cif_set_clk_dir(ctx, CIF_CLK_P02P1);

		for (i = 0; (i < csi->lane_num + 1); i++) {
			if (!i)
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_CLK]);
			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
			else
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
		}
	} else if (csi->lane_num > 0) {
		if (ctx->mac_num || !clk_port) {
			cif_set_clk_dir(ctx, CIF_CLK_FREERUN);
		} else {
			if (LANE_IS_PORT1(attr->lane_id[0]))
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			else
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		}
		/* if clk and data are in the same port.*/
		if (clk_port > 0) {
			for (i = 0; i < (csi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CLK]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_DATA]);
			}
		} else {
			for (i = 0; i < (csi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_CLK]);
				else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
			}
		}
	} else
		return -EINVAL;

	/* config the dphy seting. */
	if (attr->dphy.enable)
		cif_set_hs_settle(ctx, attr->dphy.hs_settle);
	/* config the wdr mode. */
	switch (attr->wdr_mode) {
	case CVI_MIPI_WDR_MODE_NONE:
		break;
	case CVI_MIPI_WDR_MODE_DT:
		csi->hdr_mode = CSI_HDR_MODE_DT;
		for (i = 0; i < MAX_WDR_FRAME_NUM; i++)
			csi->data_type[i] = attr->data_type[i];
		break;
	case CVI_MIPI_WDR_MODE_MANUAL:
		param->hdr_manual = combo->wdr_manu.manual_en;
		param->hdr_shift = combo->wdr_manu.l2s_distance;
		param->hdr_vsize = combo->wdr_manu.lsef_length;
		param->hdr_rm_padding = combo->wdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->wdr_manu.update);
		break;
	case CVI_MIPI_WDR_MODE_VC:
		csi->hdr_mode = CSI_HDR_MODE_VC;
		break;
	case CVI_MIPI_WDR_MODE_DOL:
		csi->hdr_mode = CSI_HDR_MODE_DOL;
		break;
	default:
		return -EINVAL;
	}
	/* config vc mapping */
	if (info->demux_en) {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = info->vc_mapping[i];
		}
	} else {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = i;
		}
	}

	param->hdr_en = (attr->wdr_mode != CVI_MIPI_WDR_MODE_NONE);
	csi->vs_gen_mode = attr->vs_gen_mode;
	cif_streaming(ctx, 1, param->hdr_en);

	return 0;
}

static int _cif_set_lvds_vsync_type(struct cif_ctx *ctx,
				    struct lvds_dev_attr_s *attr,
				    struct param_sublvds *sublvds)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;
	struct cvi_link *link = ctx_to_link(ctx);

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT1;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 15;
		cif_set_lvds_vsync_gen(ctx, 15);
		break;
	case LVDS_VSYNC_HCONNECT:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT2;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 1;
		cif_set_lvds_vsync_gen(ctx, 1);
		sublvds->hdr_hblank[0] = type->hblank1;
		sublvds->hdr_hblank[1] = type->hblank2;
		sublvds->h_size = combo->img_size.width;
		/* [TODO] use other api to strip the info line*/
		/* strip the top info line. */
		cif_crop_info_line(ctx, 1, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_sublvds(struct cvi_cif_dev *dev,
				 struct cif_ctx *ctx,
				 struct lvds_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct cvi_link *link = ctx_to_link(ctx);
	struct param_sublvds *sublvds = &param->cfg.sublvds;
	uint8_t tbl = 0x1F;
	struct sublvds_sync_code *sc;
	int i, j = 0, ch, clk_port = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_SUBLVDS;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		sublvds->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		sublvds->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		sublvds->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode != LVDS_SYNC_MODE_SAV)
		return -EINVAL;
	/* config the lane id*/
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	sublvds->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	if (sublvds->lane_num == 4) {
		if (LANE_IS_PORT1(attr->lane_id[0]))
			cif_set_clk_dir(ctx, CIF_CLK_P12P0);
		else
			cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		for (i = 0; (i < sublvds->lane_num + 1); i++) {
			if (!i)
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_CLK]);
			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
			else
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
		}
	} else if (sublvds->lane_num > 0) {
		if (ctx->mac_num || !clk_port) {
			cif_set_clk_dir(ctx, CIF_CLK_FREERUN);
		} else {
			if (LANE_IS_PORT1(attr->lane_id[0]))
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			else
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		}
		/* if clk and data are in the same port.*/
		if (clk_port > 0) {
			for (i = 0; i < (sublvds->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CLK]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_DATA]);
			}
		} else {
			for (i = 0; i < (sublvds->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_CLK]);
				else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
			}
		}
	} else
		return -EINVAL;

	/* config the sync code */
	memcpy(&sublvds->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &sublvds->sync_code.slvds;
	sc->n0_lef_sav = attr->sync_code[0][0][0];
	sc->n0_lef_eav = attr->sync_code[0][0][1];
	sc->n1_lef_sav = attr->sync_code[0][0][2];
	sc->n1_lef_eav = attr->sync_code[0][0][3];
	sc->n0_sef_sav = attr->sync_code[0][1][0];
	sc->n0_sef_eav = attr->sync_code[0][1][1];
	sc->n1_sef_sav = attr->sync_code[0][1][2];
	sc->n1_sef_eav = attr->sync_code[0][1][3];
	sc->n0_lsef_sav = attr->sync_code[0][2][0];
	sc->n0_lsef_eav = attr->sync_code[0][2][1];
	sc->n1_lsef_sav = attr->sync_code[0][2][2];
	sc->n1_lsef_eav = attr->sync_code[0][2][3];

	/* config the wdr */
	switch (attr->wdr_mode) {
	case CVI_WDR_MODE_NONE:
		/* [TODO] use other api to set the fp */
		link->distance_fp = 6;
		cif_set_lvds_vsync_gen(ctx, 6);
		break;
	case CVI_WDR_MODE_DOL_2F:
		ch = 2;
	case CVI_WDR_MODE_DOL_3F:
		/* [TODO] 3 exposure hdr hw is not ready. */
		ch = 3;
		/* config th Vsync type */
		rc = _cif_set_lvds_vsync_type(ctx, attr, sublvds);
		if (rc < 0)
			return rc;
		break;
	default:
		return -EINVAL;
	}
	param->hdr_en = (attr->wdr_mode != CVI_WDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->wdr_mode != CVI_WDR_MODE_NONE);

	return 0;
}

static int _cif_set_hispi_vsync_type(struct cif_ctx *ctx,
				     struct lvds_dev_attr_s *attr,
				     struct cif_param *param)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		break;
	case LVDS_VSYNC_SHARE:
		param->hdr_manual = combo->wdr_manu.manual_en;
		param->hdr_shift = combo->wdr_manu.l2s_distance;
		param->hdr_vsize = combo->wdr_manu.lsef_length;
		param->hdr_rm_padding = combo->wdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->wdr_manu.update);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_hispi(struct cvi_cif_dev *dev,
			       struct cif_ctx *ctx,
			       struct lvds_dev_attr_s *attr)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_hispi *hispi = &param->cfg.hispi;
	uint8_t tbl = 0x1F;
	struct hispi_sync_code *sc;
	int i, j = 0, ch, clk_port = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_HISPI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		hispi->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		hispi->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		hispi->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode == LVDS_SYNC_MODE_SOF) {
		hispi->mode = CIF_HISPI_MODE_PKT_SP;
	} else if (attr->sync_mode == LVDS_SYNC_MODE_SAV) {
		hispi->mode = CIF_HISPI_MODE_STREAM_SP;
		hispi->h_size = combo->img_size.width;
	} else {
		return -EINVAL;
	}
	/* config the lane id*/
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	hispi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	if (hispi->lane_num == 4) {
		if (LANE_IS_PORT1(attr->lane_id[0]))
			cif_set_clk_dir(ctx, CIF_CLK_P12P0);
		else
			cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		for (i = 0; (i < hispi->lane_num + 1); i++) {
			if (!i)
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_CLK]);
			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
			else
				cif_set_lane_deskew(ctx, attr->lane_id[i],
						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
		}
	} else if (hispi->lane_num > 0) {
		if (ctx->mac_num || !clk_port) {
			cif_set_clk_dir(ctx, CIF_CLK_FREERUN);
		} else {
			if (LANE_IS_PORT1(attr->lane_id[0]))
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			else
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
		}
		/* if clk and data are in the same port.*/
		if (clk_port > 0) {
			for (i = 0; i < (hispi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CLK]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_DATA]);
			}
		} else {
			for (i = 0; i < (hispi->lane_num + 1); i++) {
				if (!i)
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_CLK]);
				else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
				else
					cif_set_lane_deskew(ctx, attr->lane_id[i],
							lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
			}
		}
	} else
		return -EINVAL;

	/* config the sync code */
	memcpy(&hispi->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &hispi->sync_code.hispi;
	sc->t1_sol = attr->sync_code[0][0][0];
	sc->t1_eol = attr->sync_code[0][0][1];
	sc->t1_sof = attr->sync_code[0][0][2];
	sc->t1_eof = attr->sync_code[0][0][3];
	sc->t2_sol = attr->sync_code[0][1][0];
	sc->t2_eol = attr->sync_code[0][1][1];
	sc->t2_sof = attr->sync_code[0][1][2];
	sc->t2_eof = attr->sync_code[0][1][3];
	sc->vsync_gen = sc->t1_sof;

	/* config the wdr */
	switch (attr->wdr_mode) {
	case CVI_WDR_MODE_NONE:
		break;
	case CVI_WDR_MODE_2F:
		ch = 2;
	case CVI_WDR_MODE_3F:
		/* [TODO] 3 exposure hdr hw is not ready. */
		ch = 3;
		/* config th Vsync type */
		rc = _cif_set_hispi_vsync_type(ctx, attr, param);
		if (rc < 0)
			return rc;
		break;
	default:
		return -EINVAL;
	}
	param->hdr_en = (attr->wdr_mode != CVI_WDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->wdr_mode != CVI_WDR_MODE_NONE);

	return 0;
}
#define MAX_PAD_NUM	19

struct vi_pin_info {
	uint32_t	addr;
	uint32_t	offset;
	uint32_t	mask;
	uint32_t	func;
};

const struct vi_pin_info vi_pin[TTL_VI_SRC_NUM][MAX_PAD_NUM] = {
	[TTL_VI_SRC_VI0] = {
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX1P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX0P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM0,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM0_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM0_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP0,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP0_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP0_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM2,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM2_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM2_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP2,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP2_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP2_MASK,
			1,
		},
	},
	[TTL_VI_SRC_VI1] = {
		{
			FMUX_GPIO_FUNCSEL_VIVO_D0,
			FMUX_GPIO_FUNCSEL_VIVO_D0_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D0_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D1,
			FMUX_GPIO_FUNCSEL_VIVO_D1_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D1_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D2,
			FMUX_GPIO_FUNCSEL_VIVO_D2_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D2_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D3,
			FMUX_GPIO_FUNCSEL_VIVO_D3_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D3_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D4,
			FMUX_GPIO_FUNCSEL_VIVO_D4_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D4_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D5,
			FMUX_GPIO_FUNCSEL_VIVO_D5_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D5_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D6,
			FMUX_GPIO_FUNCSEL_VIVO_D6_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D6_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D7,
			FMUX_GPIO_FUNCSEL_VIVO_D7_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D7_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D8,
			FMUX_GPIO_FUNCSEL_VIVO_D8_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D8_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D9,
			FMUX_GPIO_FUNCSEL_VIVO_D9_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D9_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D10,
			FMUX_GPIO_FUNCSEL_VIVO_D10_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D10_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5N_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX5P_MASK,
			1,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4N_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX4P_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3N_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX3P_MASK,
			2,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2N_MASK,
			4,
		},
		{
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_OFFSET,
			FMUX_GPIO_FUNCSEL_PAD_MIPIRX2P_MASK,
			4
		},
	},
	[TTL_VI_SRC_VI2] = {
		{
			FMUX_GPIO_FUNCSEL_VIVO_D0,
			FMUX_GPIO_FUNCSEL_VIVO_D0_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D0_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D1,
			FMUX_GPIO_FUNCSEL_VIVO_D1_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D1_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D2,
			FMUX_GPIO_FUNCSEL_VIVO_D2_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D2_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D3,
			FMUX_GPIO_FUNCSEL_VIVO_D3_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D3_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D4,
			FMUX_GPIO_FUNCSEL_VIVO_D4_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D4_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D5,
			FMUX_GPIO_FUNCSEL_VIVO_D5_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D5_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D6,
			FMUX_GPIO_FUNCSEL_VIVO_D6_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D6_MASK,
			0,
		},
		{
			FMUX_GPIO_FUNCSEL_VIVO_D7,
			FMUX_GPIO_FUNCSEL_VIVO_D7_OFFSET,
			FMUX_GPIO_FUNCSEL_VIVO_D7_MASK,
			0,
		},
	},
};

static void cif_config_pinmux(enum ttl_src_e vi, uint32_t pad)
{
	mmio_clrsetbits_32(PINMUX_BASE + vi_pin[vi][pad].addr,
			vi_pin[vi][pad].mask << vi_pin[vi][pad].offset,
			vi_pin[vi][pad].func);
}

static int _cif_set_attr_cmos(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt = TTL_VDE_SENSOR;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_bt1120(struct cvi_cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);

	param->type = CIF_TYPE_TTL;
	ttl->fmt = TTL_SYNC_PAT_17B_BT1120;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT1120;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 9 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 8 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_bt601_19b_vhs(struct cvi_cif_dev *dev,
				       struct cif_ctx *ctx,
				       struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt = TTL_VHS_19B_BT601;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0x23 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xbf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_bt_demux(struct cvi_cif_dev *dev,
					struct cif_ctx *ctx,
					struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_btdemux *btdemux = &param->cfg.btdemux;
	struct cvi_link *link = ctx_to_link(ctx);
	struct bt_demux_attr_s *info = &attr->bt_demux_attr;
	int i;

	/* config the pinmux */
	for (i = TTL_PIN_FUNC_D0; i < TTL_PIN_FUNC_D8; i++) {
		if (info->func[i] < 0)
			continue;
		if (info->func[i] >= TTL_PIN_FUNC_D8)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, FROM_VI2, i, info->func[i]);
		cif_config_pinmux(FROM_VI2, info->func[i]);
	}
	PINMUX_CONFIG(VIVO_CLK, VI2_CLK);
	param->type = CIF_TYPE_BT_DMUX;
	btdemux->fmt = TTL_SYNC_PAT_9B_BT656;
	btdemux->width = attr->img_size.width - 1;
	btdemux->height = attr->img_size.height - 1;
	btdemux->clk_inv = link->clk_edge;
	btdemux->v_fp = (!info->v_fp) ? 0x0f : info->v_fp;
	btdemux->h_fp = (!info->h_fp) ? 0x0f : info->h_fp;
	btdemux->sync_code_part_A[0] = info->sync_code_part_A[0];
	btdemux->sync_code_part_A[1] = info->sync_code_part_A[1];
	btdemux->sync_code_part_A[2] = info->sync_code_part_A[2];
	for (i = 0; i < BT_DEMUX_NUM; i++) {
		btdemux->sync_code_part_B[i].sav_vld = info->sync_code_part_B[i].sav_vld;
		btdemux->sync_code_part_B[i].sav_blk = info->sync_code_part_B[i].sav_blk;
		btdemux->sync_code_part_B[i].eav_vld = info->sync_code_part_B[i].eav_vld;
		btdemux->sync_code_part_B[i].eav_blk = info->sync_code_part_B[i].eav_blk;
	}
	btdemux->demux = info->mode;
	btdemux->yc_exchg = info->yc_exchg;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_bt656_9b(struct cvi_cif_dev *dev,
				  struct cif_ctx *ctx,
				  struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_SYNC_PAT_9B_BT656;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT656;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0xf : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_custom0(struct cvi_cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= TTL_PIN_FUNC_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, vi, i, attr->ttl_attr.func[i]);
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
	}
	if (vi == TTL_VI_SRC_VI0)
		PINMUX_CONFIG(PAD_MIPIRX4N, VI0_CLK);
	else if (vi == TTL_VI_SRC_VI1)
		PINMUX_CONFIG(VIVO_CLK, VI1_CLK);
	else
		PINMUX_CONFIG(VIVO_CLK, VI2_CLK);

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_CUSTOM_0;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 4095 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 4 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}

/*
 * 1. Precondition: the mac_max = 400M by default, can be adjust by ioctl (app).
 * 2. If mac_max is below 400M, the vip_sys_2 is 400M (parent: mipimpll).
 * 3. If mac_max = 500M, the vip_sys_2 is 500M. (parent: fpll, 1835 OD case)
 * 4. If mac_max = 600M, the vip_sys_2 is 600M. (parent: mipimpll 1838 case)
 * 5. Return error in the others.
 */

#define MAC0_CLK_RATIO_OFFSET		10U
#define MAC1_CLK_RATIO_OFFSET		15U
#define MAC_CLK_RATIO_MASK		0x1FU

#define MAC0_CLK_CTRL1_OFFSET		4U
#define MAC1_CLK_CTRL1_OFFSET		8U
#define MAC_CLK_CTRL1_MASK		0x3U

static inline unsigned int _cif_mac_enum_to_value(enum rx_mac_clk_e mac_clk)
{
	unsigned int val;

	switch (mac_clk) {
	case RX_MAC_CLK_200M:
		val = 200;
		break;
	case RX_MAC_CLK_400M:
		val = 400;
		break;
	case RX_MAC_CLK_500M:
		val = 500;
		break;
	case RX_MAC_CLK_600M:
		val = 600;
		break;
	default:
		val = 400;
	}
	return val;
}

static int _cif_set_mac_clk(struct cvi_cif_dev *cdev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	struct cvi_link *link = &cdev->link[devno];
	u32 data, mask;
	u32 clk_val = _cif_mac_enum_to_value(mac_clk);
	u32 tmp;

	link->mac_clk = mac_clk;

	/* select the source to vip_sys2 */
	mask = (devno == 0) ? (MAC_CLK_CTRL1_MASK << MAC0_CLK_CTRL1_OFFSET) :
				(MAC_CLK_CTRL1_MASK << MAC1_CLK_CTRL1_OFFSET);
	data = 0x2 << ((devno == 0) ? MAC0_CLK_CTRL1_OFFSET : MAC1_CLK_CTRL1_OFFSET);
	vip_sys_reg_write_mask(0x1C, mask, data);

	if (bypass_mac_clk)
		return 0;

	if (clk_val > cdev->max_mac_clk) {
		dev_err(link->dev, "cannot reach %dM since the max clk is %dM\n", clk_val, cdev->max_mac_clk);
		return -EINVAL;
	}

	if (cdev->max_mac_clk == 500) {
		dev_dbg(link->dev, "use div1 fpll as vip_sys2 source\n");
		clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_div_1_src_vip_sys_2.clk_o);
	} else {
		dev_dbg(link->dev, "use div0 mipipll as vip_sys2 source\n");
		clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_div_0_src_vip_sys_2.clk_o);
	}

#ifdef CONFIG_ARCH_CV182X_FPGA
	// FPGA don't set mac spped.
	return 0;
#endif
#if defined(CONFIG_COMMON_CLK_CVITEK)
	/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
	tmp = clk_val * 32 / cdev->max_mac_clk;
	/* roundup */
	if ((clk_val * 32) % cdev->max_mac_clk)
		tmp++;
	tmp--;
	if (!devno) {
		VIP_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC0, tmp);
		VIP_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC0, 1);
		VIP_UPDATE_CLK_RATIO(SEL_CSI_MAC0);
	} else {
		VIP_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC1, tmp);
		VIP_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC1, 1);
		VIP_UPDATE_CLK_RATIO(SEL_CSI_MAC1);
	}

	clk_set_rate(clk_get_parent(cdev->vip_sys2.clk_o), cdev->max_mac_clk * 1000000UL);
	dev_dbg(link->dev, "ratio %d, set rate %dM\n", tmp, (tmp + 1) * cdev->max_mac_clk / 32);
	udelay(5);
#else
	switch (mac_clk) {
	case RX_MAC_CLK_400M:
		/* vipsys2 dividor factor */
		iowrite32((3<<16)|0x09, ioremap(0x03002110, 4));
		udelay(5);
		break;
	case RX_MAC_CLK_600M:
		/* vipsys2 dividor factor */
		iowrite32((2<<16)|0x09, ioremap(0x03002110, 4));
		udelay(5);
		break;
	default:
		/* do nothing and leave */
		break;
	}
#endif
	return 0;
}

static int cif_set_mac_clk(struct cvi_cif_dev *dev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	return _cif_set_mac_clk(dev, devno, mac_clk);
}

static int cif_set_dev_attr(struct cvi_cif_dev *dev,
			    struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	enum input_mode_e input_mode = attr->input_mode;
	struct cif_ctx *ctx;
	struct cif_param *param;
	struct combo_dev_attr_s *rx_attr;
	int rc = 0;

	/* force the btdmux to devno 0*/
	if (input_mode == INPUT_MODE_BT_DEMUX) {
		attr->devno = 0;
	}

	ctx = &dev->link[attr->devno].cif_ctx;
	param = &dev->link[attr->devno].param;
	rx_attr = &dev->link[attr->devno].attr;

	cif_dump_dev_attr(dev, attr);

	memset(param, 0, sizeof(*param));
	ctx->cur_config = param;
	memcpy(rx_attr, attr, sizeof(*rx_attr));

	/* Setting for serial interface. */
	if (input_mode == INPUT_MODE_MIPI ||
	    input_mode == INPUT_MODE_SUBLVDS ||
	    input_mode == INPUT_MODE_HISPI) {
		struct clk_edge_s clk_edge;

		/* set the default clock edge. */
		clk_edge.devno = attr->devno;
		clk_edge.edge = CLK_DOWN_EDGE;
		cif_set_output_clk_edge(dev, &clk_edge);
	}

	/* set mac clk */
	rc = cif_set_mac_clk(dev, attr->devno, attr->mac_clk);
	if (rc < 0)
		return rc;

	/* decide the mclk */
	if (!attr->mclk.cam)
		mclk0 = attr->mclk.freq;
	else
		mclk1 = attr->mclk.freq;

	switch (input_mode) {
	case INPUT_MODE_MIPI:
		rc = _cif_set_attr_mipi(dev, ctx, &rx_attr->mipi_attr);
		break;
	case INPUT_MODE_SUBLVDS:
		rc = _cif_set_attr_sublvds(dev, ctx,
					   &rx_attr->lvds_attr);
		break;
	case INPUT_MODE_HISPI:
		rc = _cif_set_attr_hispi(dev, ctx, &rx_attr->lvds_attr);
		break;
	case INPUT_MODE_CMOS:
		rc = _cif_set_attr_cmos(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_BT1120:
		cif_set_hs_mode(dev, 1);
		rc = _cif_set_attr_bt1120(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_BT601_19B_VHS:
		cif_set_hs_mode(dev, 1);
		rc = _cif_set_attr_bt601_19b_vhs(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_BT656_9B:
		//cif_set_hs_mode(dev, 1); // for single cam
		cif_set_hs_mode(dev, 6); // for dual cam
		rc = _cif_set_attr_bt656_9b(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_CUSTOM_0:
		cif_set_hs_mode(dev, 1);
		rc = _cif_set_attr_custom0(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_BT_DEMUX:
		rc = _cif_set_attr_bt_demux(dev, ctx, rx_attr);
		break;
	default:
		return -EINVAL;
	}
	if (rc < 0) {
		dev_err(_dev, "set attr fail %d\n", rc);
		return rc;
	}
	dev->link[attr->devno].is_on = 1;
	/* unmask the interrupts */
	cif_unmask_csi_int_sts(ctx, 0x1F);

	return 0;
}

static int cif_set_output_clk_edge(struct cvi_cif_dev *dev,
				   struct clk_edge_s *clk_edge)
{
	struct cif_ctx *ctx = &dev->link[clk_edge->devno].cif_ctx;

	dev->link[clk_edge->devno].clk_edge = clk_edge->edge;

	cif_set_clk_edge(ctx, CIF_PHY_LANE_0,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_1,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_2,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_3,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_4,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_5,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);

	return 0;
}

static void cif_reset_param(struct cvi_link *link)
{
	link->is_on = 0;
	link->clk_edge = CLK_UP_EDGE;
	link->msb = OUTPUT_NORM_MSB;
	link->crop_top = 0;
	link->distance_fp = 0;
	memset(&link->param, 0, sizeof(struct cif_param));
	memset(&link->attr, 0, sizeof(struct combo_dev_attr_s));
	memset(&link->sts_csi, 0, sizeof(struct cvi_csi_status));
	memset(&link->sts_lvds, 0, sizeof(struct cvi_lvds_status));
}

static int cif_reset_mipi(struct cvi_cif_dev *dev, uint32_t devno)
{
	union vip_sys_reset mask;
	struct cvi_link *link = &dev->link[devno];

	/* mask the interrupts */
	if (link->is_on)
		cif_mask_csi_int_sts(&link->cif_ctx, 0x1F);

	/* reset phy */
	if (link->phy_reset && link->phy_apb_reset) {
		reset_control_assert(link->phy_reset);
		reset_control_assert(link->phy_apb_reset);
		udelay(5);
		reset_control_deassert(link->phy_reset);
		reset_control_deassert(link->phy_apb_reset);
	}

	/* sw reset mac by vip register */
	if (!devno) {
		mask.b.csi_mac0 = 1;
		vip_toggle_reset(mask);
	} else {
		mask.b.csi_mac1 = 1;
		vip_toggle_reset(mask);
	}

	/* reset parameters. */
	cif_reset_param(link);

	return 0;
}

static int cif_set_crop_top(struct cvi_cif_dev *dev,
			    struct crop_top_s *crop)
{
	struct cif_ctx *ctx = &dev->link[crop->devno].cif_ctx;

	dev->link[crop->devno].crop_top = crop->crop_top;
	/* strip the top info line. */
	cif_crop_info_line(ctx, crop->crop_top, crop->update);

	return 0;
}

static int cif_set_wdr_manual(struct cvi_cif_dev *dev,
			      struct manual_wdr_s *manual)
{
	struct cif_ctx *ctx = &dev->link[manual->devno].cif_ctx;
	struct cif_param *param = &dev->link[manual->devno].param;

	param->hdr_manual = manual->attr.manual_en;
	param->hdr_shift = manual->attr.l2s_distance;
	param->hdr_vsize = manual->attr.lsef_length;
	param->hdr_rm_padding = manual->attr.discard_padding_lines;

	cif_hdr_manual_config(ctx, param, !!manual->attr.update);

	return 0;
}

static int cif_get_cif_attr(struct cvi_cif_dev *dev,
			    struct cif_attr_s *cif_attr)
{
	struct cif_param *param = &dev->link[cif_attr->devno].param;

	if (param->type == CIF_TYPE_CSI && param->hdr_en) {
		struct param_csi *csi = &param->cfg.csi;

		cif_attr->stagger_vsync = (csi->hdr_mode == CSI_HDR_MODE_VC);
	} else
		cif_attr->stagger_vsync = 0;

	return 0;
}

static int cif_set_lvds_fp_vs(struct cvi_cif_dev *dev,
			      struct vsync_gen_s *vs)
{
	struct cif_ctx *ctx = &dev->link[vs->devno].cif_ctx;

	dev->link[vs->devno].distance_fp = vs->distance_fp;
	cif_set_lvds_vsync_gen(ctx, vs->distance_fp);

	return 0;
}

#if defined(CONFIG_COMMON_CLK_CVITEK)
struct cam_pll_s {
	uint32_t	pll_rate;
	uint32_t	clk_rate;
};

const struct cam_pll_s cam_pll_setting[CAMPLL_FREQ_NUM] = {
	[CAMPLL_FREQ_37P125M] = {
		.pll_rate = 1188000000,
		.clk_rate = 37125000,
	},
	[CAMPLL_FREQ_25M] = {
		.pll_rate = 1125000000,
		.clk_rate = 25000000,
	},
	[CAMPLL_FREQ_27M] = {
		.pll_rate = 1188000000,
		.clk_rate = 27000000,
	},
	[CAMPLL_FREQ_24M] = {
		.pll_rate = 48000000,
		.clk_rate = 24000000,
	},
	[CAMPLL_FREQ_26M] = {
		.pll_rate = 832000000,
		.clk_rate = 26000000,
	},
};

#else

struct cam_pll_s {
	uint32_t	sync_set;
	uint8_t		div_sel;
	uint8_t		post_div_sel;
	uint8_t		ictrl;
	uint8_t		sel_mode;
	uint8_t		pre_div_sel;
	uint8_t		clk_div;
};

const struct cam_pll_s cam_pll_setting[CAMPLL_FREQ_NUM] = {
	[CAMPLL_FREQ_37P125M] = {
		.sync_set = 406720388UL,
		.div_sel = 12,
		.post_div_sel = 1,
		.ictrl = 0,
		.sel_mode = 1,
		.pre_div_sel = 1,
		.clk_div = 32,
	},
	[CAMPLL_FREQ_25M] = {
		.sync_set = 393705325UL,
		.div_sel = 11,
		.post_div_sel = 1,
		.ictrl = 0,
		.sel_mode = 1,
		.pre_div_sel = 1,
		.clk_div = 45,
	},
	[CAMPLL_FREQ_27M] = {
		.sync_set = 406720388UL,
		.div_sel = 12,
		.post_div_sel = 1,
		.ictrl = 0,
		.sel_mode = 1,
		.pre_div_sel = 1,
		.clk_div = 44,
	},
};
#endif

static int _cif_enable_snsr_clk(struct device *dev,
				struct cvi_cif_dev *cdev,
				uint32_t devno, uint8_t on)
{
#ifndef CONFIG_ARCH_CV182X_FPGA
	uint32_t value;

	if (mclk0 > CAMPLL_FREQ_NONE && mclk0 < CAMPLL_FREQ_NUM) {
		const struct cam_pll_s *clk = &cam_pll_setting[mclk0];

		/* camera interface. */

	#if defined(CONFIG_COMMON_CLK_CVITEK)
		(void)value;
		/* set rate of clk_cam0pll */
		clk_set_rate(clk_get_parent(cdev->clk_cam0.clk_o), clk->pll_rate);
		dev_dbg(dev, "set rate parent %d\n", clk->pll_rate);

		/* set rate of clk_cam0 */
		clk_set_rate(cdev->clk_cam0.clk_o, clk->clk_rate);
		dev_dbg(dev, "set rate self %d\n", clk->clk_rate);

		if (on) {
			if (!cdev->clk_cam0.is_on) {
				clk_prepare_enable(cdev->clk_cam0.clk_o);
				dev_dbg(dev, "enable clk %d\n", clk->pll_rate);
				cdev->clk_cam0.is_on = 1;
			}
		} else {
			if (cdev->clk_cam0.is_on) {
				clk_disable_unprepare(cdev->clk_cam0.clk_o);
				dev_dbg(dev, "disable clk %d\n", clk->pll_rate);
				cdev->clk_cam0.is_on = 0;
			}
		}
	#else
		/* [TODO] a hack for the CAM0PLL/CAM1PLL pll */

		(void)dev;
		(void)link;
		/* set pwd */
		value = ioread32(ioremap(0x03002800, 0x4));
		value |= (1<<12);
		iowrite32(value, ioremap(0x03002800, 0x4));
		udelay(100);

		/* eanble camp0pll clk source */
		value = ioread32(ioremap(0x03002030, 4));
		value &= ~(1<<28);
		iowrite32(value, ioremap(0x03002030, 4));
		/* cam0pll dividor factor */
		iowrite32((clk->clk_div<<16)|0x09, ioremap(0x030020F4, 4));
		/* set sync source en */
		value = ioread32(ioremap(0x03002840, 4));
		value |= (1<<4);
		iowrite32(value, ioremap(0x03002840, 4));

		if (on) {
			/* set sync set */
			iowrite32(clk->sync_set, ioremap(0x03002874, 0x4));
			/* set sync set sw up */
			value = ioread32(ioremap(0x03002870, 0x4));
			value ^= 0x01;
			iowrite32(value, ioremap(0x03002870, 0x4));
			/* set csr */
			value = clk->pre_div_sel |
				(clk->post_div_sel << 8) |
				(clk->sel_mode << 15) |
				(clk->div_sel << 17) |
				(clk->ictrl << 24);
			iowrite32(value, ioremap(0x03002814, 0x4));
			/* clear pwd */
			value = ioread32(ioremap(0x03002800, 0x4));
			value &= ~(1<<12);
			iowrite32(value, ioremap(0x03002800, 0x4));
			udelay(100);
		}
	#endif
	}

	if (mclk1 > CAMPLL_FREQ_NONE  && mclk1 < CAMPLL_FREQ_NUM) {
		const struct cam_pll_s *clk = &cam_pll_setting[mclk1];

		/* camera interface. */

	#if defined(CONFIG_COMMON_CLK_CVITEK)
		(void)value;
		/* set rate of clk_cam1pll */
		clk_set_rate(clk_get_parent(cdev->clk_cam1.clk_o), clk->pll_rate);

		/* set rate of clk_cam1 */
		clk_set_rate(cdev->clk_cam1.clk_o, clk->clk_rate);

		if (on)
			clk_prepare_enable(cdev->clk_cam1.clk_o);
		else
			clk_disable_unprepare(cdev->clk_cam1.clk_o);
	#else
		/* set pwd */
		value = ioread32(ioremap(0x03002800, 0x4));
		value |= (1<<16);
		iowrite32(value, ioremap(0x03002800, 0x4));
		udelay(100);

		/* eanble camp0pll clk source */
		value = ioread32(ioremap(0x03002030, 4));
		value &= ~(1<<29);
		iowrite32(value, ioremap(0x03002030, 4));
		/* cam0pll dividor factor */
		iowrite32((clk->clk_div<<16)|0x09, ioremap(0x030020F8, 4));
		/* set sync source en */
		value = ioread32(ioremap(0x03002840, 4));
		value |= (1<<5);
		iowrite32(value, ioremap(0x03002840, 4));

		if (on) {
			/* set sync set */
			iowrite32(clk->sync_set, ioremap(0x03002884, 0x4));
			/* set sync set sw up */
			value = ioread32(ioremap(0x03002880, 0x4));
			value ^= 0x01;
			iowrite32(value, ioremap(0x03002880, 0x4));
			/* set csr */
			value = clk->pre_div_sel |
				(clk->post_div_sel << 8) |
				(clk->sel_mode << 15) |
				(clk->div_sel << 17) |
				(clk->ictrl << 24);
			iowrite32(value, ioremap(0x03002818, 0x4));
			/* clear pwd */
			value = ioread32(ioremap(0x03002800, 0x4));
			value &= ~(1<<16);
			iowrite32(value, ioremap(0x03002800, 0x4));
			udelay(100);
		}
	#endif
	}

#else
//	if (mclk0 > CAMPLL_FREQ_NONE && mclk0 < CAMPLL_FREQ_NUM) {
//		const struct cam_pll_s *clk = &cam_pll_setting[mclk0];
//
//		printk("FPGA clk=%d\n", clk->clk_rate);
//	}
	if (on) {
		iowrite32((ioread32(ioremap(0x0a0c8018, 0x4)) | 0x02),
				    ioremap(0x0a0c8018, 0x4));
#ifdef CONFIG_ARCH_CV182X_FPGA
		iowrite32(0x32100000, ioremap(0x0A0880F8, 0x4));
		mdelay(10);
		iowrite32(0x32100000, ioremap(0x0A0880F8, 0x4));
#else
		iowrite32((ioread32(ioremap(0x0A0880F8, 0x4)) | 0x03100039),
				    ioremap(0x0A0880F8, 0x4));
#endif
	} else
		iowrite32((ioread32(ioremap(0x0a0c8018, 0x4)) & ~0x02),
				    ioremap(0x0a0c8018, 0x4));
#endif
	return 0;
}

static int cif_enable_snsr_clk(struct cvi_cif_dev *dev,
				uint32_t devno, uint8_t on)
{
	struct device *_dev = dev->miscdev.this_device;

	return _cif_enable_snsr_clk(_dev, dev, devno, on);
}

static int cif_reset_snsr_gpio(struct cvi_cif_dev *dev,
			       unsigned int devno, uint8_t on)
{
	struct cvi_link *link;
	int reset;

	if (devno >= MAX_LINK_NUM)
		return -EINVAL;

	link = &dev->link[devno];
	reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

	if (!gpio_is_valid(link->snsr_rst_pin))
		return 0;
	if (on)
		gpio_direction_output(link->snsr_rst_pin, reset);
	else
		gpio_direction_output(link->snsr_rst_pin, !reset);

	return 0;
}

static int cif_set_hs_mode(struct cvi_cif_dev *dev, unsigned int mode)
{
#ifndef CONFIG_ARCH_CV182X_FPGA
	switch (mode) {
	case 1:
		break;
	default:
		break;
	}
#endif
	return 0;
}

static int cif_reset_lvds(struct cvi_cif_dev *dev,
				unsigned int devno)
{
	return 0;
}

static int cif_bt_fmt_out(struct cvi_cif_dev *dev,
			  struct bt_fmt_out_s *fmt_out)
{
	struct cif_ctx *ctx = &dev->link[fmt_out->devno].cif_ctx;

	dev->link[fmt_out->devno].bt_fmt_out = fmt_out->fmt_out;
	cif_set_bt_fmt_out(ctx, fmt_out->fmt_out);

	return 0;
}

static long _cif_ioctl(struct cvi_cif_dev *dev, unsigned int cmd,
		       unsigned long arg, unsigned int from_user)
{
	struct device *_dev = dev->miscdev.this_device;
	uint32_t devno;

	dev_dbg(_dev, "%s\n", _to_string_cmd(cmd));

	if (arg == 0) {
		dev_err(_dev, "null pointer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case CVI_MIPI_SET_DEV_ATTR:
	{
		struct combo_dev_attr_s attr;

		if (from_user) {
			if (copy_from_user(&attr, (void *)arg, sizeof(attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&attr, (void *)arg, sizeof(attr));

		return cif_set_dev_attr(dev, &attr);
	}
	case CVI_MIPI_SET_OUTPUT_CLK_EDGE:
	{
		struct clk_edge_s clk;

		if (from_user) {
			if (copy_from_user(&clk, (void *)arg, sizeof(clk))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&clk, (void *)arg, sizeof(clk));

		return cif_set_output_clk_edge(dev, &clk);
	}
	case CVI_MIPI_RESET_MIPI:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;

		return cif_reset_mipi(dev, devno);
	case CVI_MIPI_SET_CROP_TOP:
	{
		struct crop_top_s crop;

		if (from_user) {
			if (copy_from_user(&crop, (void *)arg, sizeof(crop))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&crop, (void *)arg, sizeof(crop));

		return cif_set_crop_top(dev, &crop);
	}
	case CVI_MIPI_SET_WDR_MANUAL:
	{
		struct manual_wdr_s wdr_manu;

		if (from_user) {
			if (copy_from_user(&wdr_manu, (void *)arg, sizeof(wdr_manu))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&wdr_manu, (void *)arg, sizeof(wdr_manu));

		return cif_set_wdr_manual(dev, &wdr_manu);
	}
	case CVI_MIPI_SET_LVDS_FP_VS:
	{
		struct vsync_gen_s vsync;

		if (from_user) {
			if (copy_from_user(&vsync, (void *)arg, sizeof(vsync))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&vsync, (void *)arg, sizeof(vsync));

		return cif_set_lvds_fp_vs(dev, &vsync);
	}
	case CVI_MIPI_RESET_SENSOR:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_snsr_gpio(dev, devno, 1);
	case CVI_MIPI_UNRESET_SENSOR:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_snsr_gpio(dev, devno, 0);
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 1);
	case CVI_MIPI_DISABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 0);
	case CVI_MIPI_SET_HS_MODE:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_set_hs_mode(dev, devno);
	case CVI_MIPI_RESET_LVDS:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_lvds(dev, devno);
	case CVI_MIPI_GET_CIF_ATTR:
	{
		struct cif_attr_s cif_attr;

		if (from_user) {
			if (copy_from_user(&cif_attr, (void *)arg, sizeof(cif_attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&cif_attr, (void *)arg, sizeof(cif_attr));

		cif_get_cif_attr(dev, &cif_attr);

		if (from_user) {
			if (copy_to_user((void *)arg, &cif_attr, sizeof(cif_attr))) {
				dev_err(_dev, "copy_to_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy((void *)arg, &cif_attr, sizeof(cif_attr));
		return 0;
	}
	case CVI_MIPI_SET_BT_FMT_OUT:
	{
		struct bt_fmt_out_s bt_fmt;

		if (from_user) {
			if (copy_from_user(&bt_fmt, (void *)arg, sizeof(bt_fmt))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&bt_fmt, (void *)arg, sizeof(bt_fmt));

		return cif_bt_fmt_out(dev, &bt_fmt);
	}
	case CVI_MIPI_SET_SENSOR_CLOCK:
	{
		struct mclk_pll_s mclk;

		if (from_user) {
			if (copy_from_user(&mclk, (void *)arg, sizeof(mclk))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&mclk, (void *)arg, sizeof(mclk));

		/* sensor enable */
		if (mclk.cam > MAX_LINK_NUM)
			return -EINVAL;
		if (!mclk.cam)
			mclk0 = mclk.freq;
		else
			mclk1 = mclk.freq;

		return _cif_enable_snsr_clk(_dev, dev, mclk.cam, mclk.freq != CAMPLL_FREQ_NONE);
	}
	case CVI_MIPI_SET_MAX_MAC_CLOCK:
		if (from_user) {
			if (copy_from_user(&dev->max_mac_clk, (void *)arg, sizeof(u32))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			dev->max_mac_clk = *(uint32_t *)arg;

		if (dev->max_mac_clk <= 400)
			dev->max_mac_clk = 400;
		else if (dev->max_mac_clk <= 500)
			dev->max_mac_clk = 500;
		else
			dev->max_mac_clk = 600;
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

static long cif_cb(struct cvi_cif_dev *dev, unsigned int cmd, unsigned long arg)
{
	return _cif_ioctl(dev, cmd, arg, 0);
}

static long cif_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cvi_cif_dev *dev = file_cif_dev(file);

	return _cif_ioctl(dev, cmd, arg, 1);
}

#ifdef CONFIG_COMPAT
static long cif_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	return cif_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int cif_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cif_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations cif_fops = {
	.owner = THIS_MODULE,
	.open = cif_open,
	.release = cif_release,
	.unlocked_ioctl = cif_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cif_ioctl32,
#endif
};

static int cif_init_miscdev(struct platform_device *pdev, struct cvi_cif_dev *dev)
{
	int rc, i;

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = MIPI_RX_DEV_NAME;
	dev->miscdev.fops = &cif_fops;

	rc = misc_register(&dev->miscdev);
	if (rc) {
		dev_err(&pdev->dev, "cif: failed to register misc device.\n");
		return rc;
	}

	/* init cif */
	for (i = 0; i < MAX_LINK_NUM; i++) {
		struct cif_ctx *ctx = &dev->link[i].cif_ctx;

		ctx->mac_phys_regs = cif_get_mac_phys_reg_bases(i);
		ctx->wrap_phys_regs = cif_get_wrap_phys_reg_bases(i);
	}

	/* register cif_cb */
	rc = vip_sys_register_cif_cb(dev, cif_cb);
	if (rc)
		dev_err(&pdev->dev, "cif: failed to register callbacks.\n");

	return rc;
}

static irqreturn_t cif_isr(int irq, void *_link)
{
	struct cvi_link *link = (struct cvi_link *)_link;
	struct cif_ctx *ctx = &link->cif_ctx;

	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_ECC_ERR_MASK))
		link->sts_csi.errcnt_ecc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_CRC_ERR_MASK))
		link->sts_csi.errcnt_crc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_WC_ERR_MASK))
		link->sts_csi.errcnt_wc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_HDR_ERR_MASK))
		link->sts_csi.errcnt_hdr++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_FIFO_FULL_MASK))
		link->sts_csi.fifo_full++;

	if (link->sts_csi.errcnt_ecc > 0xFFFF ||
		link->sts_csi.errcnt_crc > 0xFFFF ||
		link->sts_csi.errcnt_hdr > 0xFFFF ||
		link->sts_csi.fifo_full > 0xFFFF ||
		link->sts_csi.errcnt_wc > 0xFFFF) {

		cif_mask_csi_int_sts(ctx, 0x1F);
		dev_err(link->dev, "mask the interrupt since err cnt is full\n");
		dev_err(link->dev, "ecc = %u, crc = %u, wc = %u, hdr = %u, fifo_full = %u\n",
				link->sts_csi.errcnt_ecc,
				link->sts_csi.errcnt_crc,
				link->sts_csi.errcnt_wc,
				link->sts_csi.errcnt_hdr,
				link->sts_csi.fifo_full);
	}

	cif_clear_csi_int_sts(ctx);

	return IRQ_HANDLED;
}

static char irq_name[MAX_LINK_NUM][20] = {
	"cif-irq0",
	"cif-irq1"
};

static int _init_resource(struct platform_device *pdev)
{
#if (DEVICE_FROM_DTS)
	struct resource *res = NULL;
	void *reg_base[6];
	struct cvi_cif_dev *dev;
	int i;
	struct cvi_link *link;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata\n");
		return -EINVAL;
	}
	/* cam clk shall not depend on the link so we separate them from link. */
	dev->clk_cam0.clk_o = devm_clk_get(&pdev->dev, "clk_cam0");
	if (!IS_ERR_OR_NULL(dev->clk_cam0.clk_o))
		dev_info(&pdev->dev, "cam0 clk installed\n");
	dev->clk_cam1.clk_o = devm_clk_get(&pdev->dev, "clk_cam1");
	if (!IS_ERR_OR_NULL(dev->clk_cam1.clk_o))
		dev_info(&pdev->dev, "cam1 clk installed\n");
	dev->vip_sys2.clk_o = devm_clk_get(&pdev->dev, "clk_sys_2");
	if (!IS_ERR_OR_NULL(dev->vip_sys2.clk_o))
		dev_info(&pdev->dev, "vip_sys_2 clk installed\n");
	dev->clk_div_0_src_vip_sys_2.clk_o = devm_clk_get(&pdev->dev, "clk_div_0_src_vip_sys_2");
	if (!IS_ERR_OR_NULL(dev->clk_div_0_src_vip_sys_2.clk_o))
		dev_info(&pdev->dev, "clk_div_0_src_vip_sys_2 clk installed %p\n", dev->clk_div_0_src_vip_sys_2.clk_o);
	dev->clk_div_1_src_vip_sys_2.clk_o = devm_clk_get(&pdev->dev, "clk_div_1_src_vip_sys_2");
	if (!IS_ERR_OR_NULL(dev->clk_div_1_src_vip_sys_2.clk_o))
		dev_info(&pdev->dev, "clk_div_1_src_vip_sys_2 clk installed %p\n", dev->clk_div_1_src_vip_sys_2.clk_o);

	for (i = 0; i < (MAX_LINK_NUM * 2 - 1); ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		reg_base[i] = devm_ioremap(&pdev->dev, res->start,
						   res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start,
						   res->end - res->start);
#endif
		dev_info(&pdev->dev,
			 "(%d) res-reg: start: 0x%llx, end: 0x%llx.",
			 i, res->start, res->end);
		dev_info(&pdev->dev, " virt-addr(%p)\n", reg_base[i]);
		if (i & 0x1)
			cif_set_base_addr(i>>1, reg_base[i-1], reg_base[i]);
	}
	cif_set_base_addr(1, reg_base[2], reg_base[1]);
	/* init pad_ctrl. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, i);
	if (!res) {
		dev_info(&pdev->dev, "no pad_ctrl for cif\n");
	} else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		dev->pad_ctrl = devm_ioremap(&pdev->dev, res->start,
						   res->end - res->start);
#else
		dev->pad_ctrl = devm_ioremap_nocache(&pdev->dev, res->start,
						   res->end - res->start);
#endif
		dev_info(&pdev->dev,
			 "pad-ctrl res-reg: start: 0x%llx, end: 0x%llx.",
			 res->start, res->end);
	}

	/* Init max mac clock. */
	if (max_mac_clk <= 400)
		dev->max_mac_clk = 400;
	else if (max_mac_clk <= 500)
		dev->max_mac_clk = 500;
	else
		dev->max_mac_clk = 600;

	/* Interrupt */
	for (i = 0; i < MAX_LINK_NUM; ++i) {
		link = &dev->link[i];

		link->irq_num = platform_get_irq(pdev, i);
		if (link->irq_num < 0)
			break;
		if (devm_request_irq(&pdev->dev, link->irq_num, cif_isr, IRQF_SHARED, irq_name[i], link))
			break;
		dev_info(&pdev->dev, "request irq-%d as %s\n",
			 link->irq_num, irq_name[i]);

		/* set the port id */
		link->cif_ctx.mac_num = i;
	}

	/* reset pin */
	for (i = 0; i < MAX_LINK_NUM; ++i) {
		link = &dev->link[i];
		link->dev = &pdev->dev;
		link->mac_clk = RX_MAC_CLK_400M;
		link->snsr_rst_pin = of_get_named_gpio_flags(pdev->dev.of_node,
				"snsr-reset", i, &link->snsr_rst_pol);
		if (link->snsr_rst_pin < 0)
			break;

		if (gpio_request(link->snsr_rst_pin, "snsr-rst-gpio"))
			return 0;

		dev_info(&pdev->dev, "rst_pin = %d, pol = %d\n",
			link->snsr_rst_pin, link->snsr_rst_pol);
	}

	/* sw reset */
	link = &dev->link[0];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy0");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy0 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb0");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy0 apb reset installed\n");
	link = &dev->link[1];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy1");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy1 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb1");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy1 apb reset installed\n");
#endif

	return 0;
}

#ifdef CONFIG_PROC_FS
static void cif_show_dev_attr(struct seq_file *m,
			      struct combo_dev_attr_s *attr)
{
	int i;
	char buf[32] = {0};
	char buf2[32] = {0};

	seq_printf(m, "%8s%10s%10s%10s%15s%15s%10s%12s%16s\n",
		   "Devno", "WorkMode", "DataType", "WDRMode", "LinkId",
		   "PN Swap", "SyncMode", "DataEndian", "SyncCodeEndian");

	seq_printf(m, "%8d%10s", attr->devno,
		   _to_string_input_mode(attr->input_mode));

	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		struct mipi_dev_attr_s *mipi = &attr->mipi_attr;
		char *ptr = buf;

		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", mipi->lane_id[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		ptr = buf2;
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", mipi->pn_swap[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		seq_printf(m, "%10s%10s%15s%15s%10s%12s%16s\n",
			   _to_string_raw_data_type(mipi->raw_data_type),
			   _to_string_mipi_wdr_mode(mipi->wdr_mode),
			   buf, buf2,
			   "N/A", "N/A", "N/A");

	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		char *ptr = buf;

		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", lvds->lane_id[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		ptr = buf2;
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", lvds->pn_swap[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		seq_printf(m, "%10s%10s%15s%15s%10s%12s%16s\n",
			   _to_string_raw_data_type(lvds->raw_data_type),
			   _to_string_wdr_mode(lvds->wdr_mode),
			   buf, buf2,
			   _to_string_lvds_sync_mode(lvds->sync_mode),
			   _to_string_bit_endian(lvds->data_endian),
			   _to_string_bit_endian(lvds->sync_code_endian));
	}
	break;
	case INPUT_MODE_CMOS:
	case INPUT_MODE_BT1120:
		seq_printf(m, "%10s%10s%15s%10s%12s%16s\n",
			   "N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
		break;
	default:
		break;
	}
}

static void cif_show_mipi_sts(struct seq_file *m,
			      struct cvi_link *link)
{
	struct cvi_csi_status *sts = &link->sts_csi;

	seq_printf(m, "%6s%7s%7s%7s%6s%9s%9s\n",
		   "Devno", "EccErr", "CrcErr", "HdrErr", "WcErr", "fifofull", "decode");
	seq_printf(m, "%6d%7d%7d%7d%6d%9d%9s\n",
		   link->attr.devno,
		   sts->errcnt_ecc, sts->errcnt_crc, sts->errcnt_hdr,
		   sts->errcnt_wc, sts->fifo_full,
		   _to_string_csi_decode(cif_get_csi_decode_fmt(&link->cif_ctx)));
}

static void cif_show_phy_sts(struct seq_file *m,
			      struct cvi_link *link)
{
	union mipi_phy_state state;

	cif_get_csi_phy_state(&link->cif_ctx, &state);

	seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s\n",
		   "Physical:", "D0", "D1", "D2", "D3", "D4", "D5");
	seq_printf(m, "%11s%9x%9x%9x%9x%9x%9x\n",
		   " ",
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_0),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_1),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_2),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_3),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_4),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_5));

	seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
		   "Digital:", "D0", "D1", "D2", "D3", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
	seq_printf(m, "%11s%9s%9s%9s%9s%9d%9d%9d%9d%9s\n",
		   " ",
		   _to_string_dlane_state(state.bits.d0_datahs_state),
		   _to_string_dlane_state(state.bits.d1_datahs_state),
		   _to_string_dlane_state(state.bits.d2_datahs_state),
		   _to_string_dlane_state(state.bits.d3_datahs_state),
		   link->cif_ctx.mac_num ? state.bits.p1_clk_hs_state : state.bits.clk_hs_state,
		   link->cif_ctx.mac_num ? state.bits.p1_clk_ulps_state : state.bits.clk_ulps_state,
		   link->cif_ctx.mac_num ? state.bits.p1_clk_stop_state : state.bits.clk_stop_state,
		   link->cif_ctx.mac_num ? state.bits.p1_clk_err_state : state.bits.clk_err_state,
		   _to_string_deskew_state(link->cif_ctx.mac_num ?
			   state.bits.p1_deskew_state : state.bits.deskew_state));
}

static int proc_cif_show(struct seq_file *m, void *v)
{
	struct cvi_cif_dev *dev = (struct cvi_cif_dev *)m->private;
	int i;

	seq_printf(m, "\nModule: [MIPI_RX], Build Time[%s]\n",
			UTS_VERSION);
	seq_puts(m, "\n------------Combo DEV ATTR--------------\n");
	for (i = 0; i < MAX_LINK_NUM; i++)
		if (dev->link[i].is_on)
			cif_show_dev_attr(m, &dev->link[i].attr);

	seq_puts(m, "\n------------MIPI info-------------------\n");
	for (i = 0; i < MAX_LINK_NUM; i++)
		if (dev->link[i].is_on
			&& (dev->link[i].attr.input_mode == INPUT_MODE_MIPI)) {
			cif_show_mipi_sts(m, &dev->link[i]);
			cif_show_phy_sts(m, &dev->link[i]);
		}

	return 0;
}

static u8 *dbg_type[] = {
	"reset",
	"hs_s",
	"snsr_r",
	"snsr_on",
	"bt_fmt",
	"mac_clk"
};

static void dbg_print_usage(struct device *dev)
{
	dev_info(dev, "\n------------DBG USAGE-------------------\n");
	dev_info(dev, "reset [devno]: reset mipi-rx error count\n");
	dev_info(dev, "hs_s [devno] [value]: set mipi-rx hs_settle, value : 0~255, def: 16\n");
	dev_info(dev, "snsr_r [devno] [reset]: reset : 1 - reset, 0 - unreset sensor gpio\n");
	dev_info(dev, "snsr_on [cam_num] [on] [mclk]: enable/disable sensor clk\n");
	dev_info(dev, "                               cam_num : 0 - cam0, 1 - cam1\n");
	dev_info(dev, "                               on : 0 - off, 1 - on\n");
	dev_info(dev, "                               mclk : 0 - off, 1 - 37.125M, 2 - 25M, 3 - 27M\n");
	dev_info(dev, "                                    : 4 - 24M, 5 - 26M\n");
	dev_info(dev, "bt_fmt [devno] [0~3]: set bt format CbY/CrY/YCb/YCr\n");
	dev_info(dev, "mac_clk [devno] [200/400/600]: set mac clock (MHz)\n");
}

static int dbg_hdler(struct cvi_cif_dev *dev, char const *input)
{
	struct cvi_link *link = &dev->link[0];
	struct cif_ctx *ctx;
	int reset;
	u32 num;
	u8 str[80] = {0};
	u8 t = 0;
	u32 a, v, v2;
	u8 i, n;
	u8 *p;

	num = sscanf(input, "%s %d %d %d", str, &a, &v, &v2);
	if (num > 4) {
		dbg_print_usage(link->dev);
		return -EINVAL;
	}

	dev_info(link->dev, "input = %s %d\n", str, num);
	/* convert to lower case for following type compare */
	p = str;
	for (; *p; ++p)
		*p = tolower(*p);
	n = ARRAY_SIZE(dbg_type);
	for (i = 0; i < n; i++) {
		if (!strcmp(str, dbg_type[i])) {
			t = i;
			break;
		}
	}
	if (i == n) {
		dev_info(link->dev, "unknown type(%s)!\n", str);
		dbg_print_usage(link->dev);
		return -EINVAL;
	}

	switch (t) {
	case 0:
		/* reset */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		if (link->is_on) {
			link->sts_csi.errcnt_ecc = 0;
			link->sts_csi.errcnt_crc = 0;
			link->sts_csi.errcnt_wc = 0;
			link->sts_csi.errcnt_hdr = 0;
			link->sts_csi.fifo_full = 0;
			cif_clear_csi_int_sts(ctx);
			cif_unmask_csi_int_sts(ctx, 0x0F);
		}

		break;
	case 1:
		/* hs-settle */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		cif_set_hs_settle(ctx, v);
		break;
	case 2:
		/* sensor reset */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

		if (!gpio_is_valid(link->snsr_rst_pin))
			return 0;
		if (v)
			gpio_direction_output(link->snsr_rst_pin, reset);
		else
			gpio_direction_output(link->snsr_rst_pin, !reset);
		break;
	case 3:
		/* sensor enable */
		if (a > MAX_LINK_NUM)
			return -EINVAL;
		if (!a)
			mclk0 = v2;
		else
			mclk1 = v2;

		link = &dev->link[a];
		_cif_enable_snsr_clk(link->dev, dev, a, v);
		break;
	case 4:
		/* bt format out */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		cif_set_bt_fmt_out(ctx, v);
		break;
	case 5:
		/* set mac clock */
		if (v == 600)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_600M);
		else if (v == 500)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_500M);
		else if (v == 400)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_400M);
		else
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_200M);
		break;
	default:
		dbg_print_usage(link->dev);
		break;
	}

	return 0;
}

static ssize_t cif_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct cvi_cif_dev *dev = PDE_DATA(file_inode(file));

	dbg_hdler(dev, user_buf);

	return count;
}

static int proc_cif_open(struct inode *inode, struct file *file)
{
	struct cvi_cif_dev *dev = PDE_DATA(inode);

	return single_open(file, proc_cif_show, dev);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops cif_proc_fops = {
	.proc_open	= proc_cif_open,
	.proc_read	= seq_read,
	.proc_write	= cif_proc_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations cif_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_cif_open,
	.read		= seq_read,
	.write		= cif_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static struct proc_dir_entry *cif_proc_entry;
#endif

static int cvi_cif_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_cif_dev *dev;

	/* allocate main cif state structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, dev);
	platform_set_drvdata(pdev, dev);

	rc = cif_init_miscdev(pdev, dev);
	if (rc < 0)
		return rc;

	rc = _init_resource(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init res for cif, %d\n", rc);
		return rc;
	}

#ifdef CONFIG_PROC_FS
	cif_proc_entry = proc_create_data("mipi-rx", 0, NULL,
					  &cif_proc_fops, dev);
	if (!cif_proc_entry)
		dev_err(&pdev->dev, "cif: can't init procfs.\n");
#endif

	return 0;
}

static int cvi_cif_remove(struct platform_device *pdev)
{
	struct cvi_cif_dev *dev;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata");
		return 0;
	}

	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

#ifdef CONFIG_PROC_FS
	proc_remove(cif_proc_entry);
#endif
	return 0;
}

static const struct of_device_id cvi_cif_dt_match[] = {
	{.compatible = "cvitek,cif"},
	{}
};

#if (!DEVICE_FROM_DTS)
static void cvi_cif_pdev_release(struct device *dev)
{
}

static struct platform_device cvi_cif_pdev = {
	.name		= "cif",
	.dev.release	= cvi_cif_pdev_release,
};
#endif

static struct platform_driver cvi_cif_pdrv = {
	.probe      = cvi_cif_probe,
	.remove     = cvi_cif_remove,
	.driver     = {
		.name		= "cif",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cvi_cif_dt_match,
#endif
	},
};

static int __init cvi_cif_init(void)
{
	int rc;

#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_cif_pdrv);
#else
	rc = platform_device_register(&cvi_cif_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_cif_pdrv);
	if (rc)
		platform_device_unregister(&cvi_cif_pdev);
#endif

	return rc;
}

static void __exit cvi_cif_exit(void)
{
	platform_driver_unregister(&cvi_cif_pdrv);
#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cvi_cif_pdev);
#endif
}

MODULE_DESCRIPTION("Cvitek Camera Interface Driver");
MODULE_AUTHOR("Max Liao");
MODULE_LICENSE("GPL");
module_init(cvi_cif_init);
module_exit(cvi_cif_exit);

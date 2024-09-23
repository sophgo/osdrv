#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "system_common.h"
#elif defined(ENV_EMU)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <asm/cacheflush.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif
#endif  // ENV_CVITEST

#include "scaler.h"
#include "scaler_reg.h"
#include "reg.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static struct sclr_disp_cfg g_disp_cfg;
static uintptr_t reg_base;
static spinlock_t disp_mask_spinlock;

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void sclr_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
}

/****************************************************************************
 * SCALER DISP SHADOW REGISTER - USE in DISPLAY and GOP
 ****************************************************************************/
/**
 * sclr_disp_reg_set_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param shadow_mask: true(mask); false(unmask)
 */
void sclr_disp_reg_set_shadow_mask(bool shadow_mask)
{
	if (shadow_mask)
		spin_lock(&disp_mask_spinlock);

	_reg_write_mask(reg_base + REG_SCL_DISP_CFG, BIT(17),
			(shadow_mask ? BIT(17) : 0));

	if (!shadow_mask)
		spin_unlock(&disp_mask_spinlock);
}

/****************************************************************************
 * SCALER GOP
 ****************************************************************************/
/**
 * sclr_gop_set_cfg - configure gop
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: gop's settings
 * @param update: update parameter or not
 */
void sclr_gop_set_cfg(u8 inst, u8 layer, struct sclr_gop_cfg *cfg, bool update)
{
	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, cfg->gop_ctrl.raw);
		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTCOLOR,
			(cfg->font_fg_color << 16) | cfg->font_bg_color);
		if (cfg->gop_ctrl.b.colorkey_en)
			_reg_write(reg_base + REG_SCL_DISP_GOP_COLORKEY, cfg->colorkey);
		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTBOX_CTRL, cfg->fb_ctrl.raw);

		// ECO item for threshold invert
		_reg_write_mask(reg_base + 0x90f8, 0x1, cfg->fb_ctrl.b.lo_thr_inv);
		// set odec cfg
		_reg_write(reg_base + REG_SCL_DISP_GOP_DEC_CTRL, cfg->odec_cfg.odec_ctrl.raw);

		sclr_disp_reg_set_shadow_mask(false);

		if (update)
			g_disp_cfg.gop_cfg = *cfg;
	}
}

/**
 * sclr_gop_get_cfg - get gop's configurations.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 */
struct sclr_gop_cfg *sclr_gop_get_cfg(u8 inst, u8 layer)
{
	if (inst < SCL_MAX_INST && layer < SCL_MAX_GOP_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return NULL;
	} else if (inst == SCL_GOP_DISP) {
		return &g_disp_cfg.gop_cfg;
	}

	return NULL;
}

/**
 * sclr_gop_setup 256LUT - setup gop's Look-up table
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 256LUT-table for index 0 ~ length.
 *		There should be smaller than 256 instances.
 * @param data: values of 256LUT-table. There should be 256 instances.
 */
int sclr_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data)
{
	u16 i = 0;
	struct sclr_gop_cfg gop_cfg;

#if 0
	void *vip_clk_reg = ioremap(0x3002008, 4);
	u32 vip_pll = ioread32(vip_clk_reg);

	iounmap(vip_clk_reg);
	pr_debug("hw vip clk:%#x\n", vip_pll);
#endif


	if (length > 256) {
		pr_err("LUT length(%d) error, should less or equal to 256!\n", length);
		return -1;
	}

	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return -1;
	} else {
		sclr_disp_reg_set_shadow_mask(true);
		//Disable OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, 0x0);

		pr_debug("[cvi-vip][sc] update 256LUT in gop1 of display. layer(%d), sc(%d). Length is %d.\n",
			layer, inst, length);
		for (i = 0; i < length; ++i) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT0,
						(i << 16) | *(data + i));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, BIT(16));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, ~BIT(16));
			pr_debug("write LUT index:%d value:%#x\n", i, *(data + i));
		}
		for (i = 0; i < length; ++i) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT0, (i << 16));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, BIT(17));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, ~BIT(17));
			pr_debug("read LUT index:%d value:%#x\n",
				i, _reg_read(reg_base + REG_SCL_DISP_GOP_256LUT1) & 0xFFFF);
		}

		//Enable original OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, gop_cfg.gop_ctrl.raw);
		pr_debug("After update LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_DISP_GOP_CFG));
		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}

/**
 * sclr_gop_update_256LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 256LUT-table. There should be 256 instances.
 * @param data: value of 256LUT-table.
 */
int sclr_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data)
{
	struct sclr_gop_cfg gop_cfg = *sclr_gop_get_cfg(inst, layer);

	if (index > 256)
		return -1;

	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return -1;
	} else {
		sclr_disp_reg_set_shadow_mask(true);
		//Disable OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, 0x0);

		pr_debug("[cvi-vip][sc] update 256LUT in gop1 of display. layer(%d), sc(%d), Index is %d.\n",
				layer, inst, index);
		_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT0,
					(index << 16) | data);
		_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, BIT(16));
		_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, ~BIT(16));

		//Enable original OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, gop_cfg.gop_ctrl.raw);
		pr_debug("After upadte LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_DISP_GOP_CFG));
		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}

/**
 * sclr_gop_setup 16LUT - setup gop's Look-up table
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 16LUT-table for index 0 ~ length.
 *		There should be smaller than 16 instances.
 * @param data: values of 16LUT-table. There should be 16 instances.
 */
int sclr_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data)
{
	u16 i = 0;

	if (length > 16)
		return -1;

	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return -1;
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		pr_debug("[cvi-vip][sc] update 16LUT in gop1 of display. Length is %d.\n", length);
		for (i = 0; i <= length; i += 2) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_16LUT(i),
						((*(data + i + 1) << 16) | (*(data + i))));
		}

		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}

/**
 * sclr_gop_update_16LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 16LUT-table. There should be 16 instances.
 * @param data: value of 16LUT-table.
 */
int sclr_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data)
{
	u16 tmp;

	if (index > 16)
		return -1;

	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return -1;
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		pr_debug("[cvi-vip][sc] update 16LUT in gop1 of display. Index is %d.\n", index);
		if (index % 2 == 0) {
			tmp = _reg_read(reg_base + REG_SCL_DISP_GOP_16LUT(index + 1));
			_reg_write(reg_base + REG_SCL_DISP_GOP_16LUT(index),
						((tmp << 16) | data));
		} else {
			tmp = _reg_read(reg_base + REG_SCL_DISP_GOP_16LUT(index - 1));
			_reg_write(reg_base + REG_SCL_DISP_GOP_16LUT(index - 1),
						((data << 16) | tmp));
		}

		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}

/**
 * sclr_gop_ow_set_cfg - set gop's osd-window configurations.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param cfg: ow's settings.
 * @param update: update parameter or not
 */
void sclr_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct sclr_gop_ow_cfg *ow_cfg, bool update)
{
	//OW Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	//4'b1100: Font-base"
	static const u8 reg_map_fmt[SCL_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa, 0xc};

	if (ow_inst >= SCL_MAX_GOP_OW_INST)
		return;

	pr_debug("[cvi-vip][sc] %s: inst:%d layer:%d ow_inst:%d ow_cfg->fmt:%d\n",
		__func__, inst, layer, ow_inst, ow_cfg->fmt);
	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return;
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_DISP_GOP_FMT(ow_inst),
					reg_map_fmt[ow_cfg->fmt]);
		_reg_write(reg_base + REG_SCL_DISP_GOP_H_RANGE(ow_inst),
					(ow_cfg->end.x << 16) | ow_cfg->start.x);
		_reg_write(reg_base + REG_SCL_DISP_GOP_V_RANGE(ow_inst),
					(ow_cfg->end.y << 16) | ow_cfg->start.y);
		_reg_write(reg_base + REG_SCL_DISP_GOP_ADDR_L(ow_inst),
					ow_cfg->addr);
		_reg_write(reg_base + REG_SCL_DISP_GOP_ADDR_H(ow_inst),
					ow_cfg->addr >> 32);
		_reg_write(reg_base + REG_SCL_DISP_GOP_CROP_PITCH(ow_inst),
					(ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
		_reg_write(reg_base + REG_SCL_DISP_GOP_SIZE(ow_inst),
					(ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);

		sclr_disp_reg_set_shadow_mask(false);

		if (update)
			g_disp_cfg.gop_cfg.ow_cfg[ow_inst] = *ow_cfg;
	}
}

/**
 * sclr_gop_ow_get_addr - get gop's osd-window DRAM addr.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param addr: ow's DRAM address.
 */
void sclr_gop_ow_get_addr(u8 inst, u8 layer, u8 ow_inst, u64 *addr)
{
	if (ow_inst >= SCL_MAX_GOP_OW_INST)
		return;

	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
	} else {
		*addr = _reg_read(reg_base + REG_SCL_DISP_GOP_ADDR_L(ow_inst)) |
			((u64)_reg_read(reg_base + REG_SCL_DISP_GOP_ADDR_H(ow_inst)) << 32);
	}
}

/**
 * sclr_gop_fb_set_cfg - setup fontbox
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @param cfg: fontbox configuration
 */
void sclr_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct sclr_gop_fb_cfg *fb_cfg)
{
	if (fb_inst >= SCL_MAX_GOP_FB_INST)
		return;

	if (fb_cfg->fb_ctrl.b.sample_rate > 3)
		fb_cfg->fb_ctrl.b.sample_rate = 3;
	if (fb_cfg->fb_ctrl.b.pix_thr > 31)
		fb_cfg->fb_ctrl.b.pix_thr = 31;

	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTBOX_CFG(fb_inst), fb_cfg->fb_ctrl.raw);
		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTBOX_INIT(fb_inst), fb_cfg->init_st);

		sclr_disp_reg_set_shadow_mask(false);
	}
}

/**
 * sclr_gop_fb_get_record - get fontbox's record
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @return: fontbox's record
 */
u32 sclr_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst)
{
	if (inst < SCL_MAX_INST) {
		pr_err("(%s) FB cannot be attached to sc", __func__);
		return -1;
	} else
		return _reg_read(reg_base + REG_SCL_DISP_GOP_FONTBOX_REC(fb_inst));
}

/****************************************************************************
 * SCALER DISP
 ****************************************************************************/
void sclr_disp_get_hw_timing(struct sclr_disp_timing *timing)
{
	u32 tmp = 0;

	if (!timing)
		return;

	tmp = _reg_read(reg_base + REG_SCL_DISP_TOTAL);
	timing->htotal = (tmp >> 16) & 0xffff;
	timing->vtotal = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_VSYNC);
	timing->vsync_end = (tmp >> 16) & 0xffff;
	timing->vsync_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_VFDE);
	timing->vfde_end = (tmp >> 16) & 0xffff;
	timing->vfde_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_VMDE);
	timing->vmde_end = (tmp >> 16) & 0xffff;
	timing->vmde_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_HSYNC);
	timing->hsync_end = (tmp >> 16) & 0xffff;
	timing->hsync_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_HFDE);
	timing->hfde_end = (tmp >> 16) & 0xffff;
	timing->hfde_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_HMDE);
	timing->hmde_end = (tmp >> 16) & 0xffff;
	timing->hmde_start = tmp & 0xffff;
}

/****************************************************************************
 * SCALER CTRL
 ****************************************************************************/
/**
 * sclr_ctrl_init - setup all sc instances.
 *
 */
void sclr_ctrl_init(void)
{
	// init variables
	memset(&g_disp_cfg, 0, sizeof(g_disp_cfg));

	// init disp mask up lock
	spin_lock_init(&disp_mask_spinlock);

	//display osd burst length set to 256 bytes for short latency
	g_disp_cfg.gop_cfg.gop_ctrl.b.burst = SCL_DISP_GOP_BURST;
}


#include "osal.h"
#include "gfbg_reg.h"
#include "gfbg_debug.h"
#include "gfbg_disp.h"
#include "reg.h"

static uintptr_t reg_disp_base[DISP_MAX_INST];
static uintptr_t reg_oenc_base[DISP_MAX_INST];
static struct disp_cfg g_disp_cfg[DISP_MAX_INST];
static osal_spinlock disp_mask_spinlock;

void gfbg_set_disp_base_addr(unsigned char inst, void *base)
{
	reg_disp_base[inst] = (uintptr_t)base;
	if (base) {
		// init disp mask up lock
		osal_spin_lock_init(&disp_mask_spinlock);
	} else {
		// destory disp mask up lock
		osal_spin_lock_destroy(&disp_mask_spinlock);
	}
}

void gfbg_set_oenc_base_addr(unsigned char inst, void *base)
{
	reg_oenc_base[inst] = (uintptr_t)base;
}

void gfbg_get_disp_hw_timing(unsigned char inst, struct disp_timing *timing)
{
	unsigned int tmp = 0;

	if (!timing)
		return;

	tmp = _reg_read(REG_DISP_TOTAL(inst));
	timing->htotal = (tmp >> 16) & 0xffff;
	timing->vtotal = tmp & 0xffff;
	tmp = _reg_read(REG_DISP_VSYNC(inst));
	timing->vsync_end = (tmp >> 16) & 0xffff;
	timing->vsync_start = tmp & 0xffff;
	tmp = _reg_read(REG_DISP_VFDE(inst));
	timing->vfde_end = (tmp >> 16) & 0xffff;
	timing->vfde_start = tmp & 0xffff;
	tmp = _reg_read(REG_DISP_VMDE(inst));
	timing->vmde_end = (tmp >> 16) & 0xffff;
	timing->vmde_start = tmp & 0xffff;
	tmp = _reg_read(REG_DISP_HSYNC(inst));
	timing->hsync_end = (tmp >> 16) & 0xffff;
	timing->hsync_start = tmp & 0xffff;
	tmp = _reg_read(REG_DISP_HFDE(inst));
	timing->hfde_end = (tmp >> 16) & 0xffff;
	timing->hfde_start = tmp & 0xffff;
	tmp = _reg_read(REG_DISP_HMDE(inst));
	timing->hmde_end = (tmp >> 16) & 0xffff;
	timing->hmde_start = tmp & 0xffff;
}

/**
 * gfbg_reg_set_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param shadow_mask: true(mask); false(unmask)
 */
void gfbg_reg_set_shadow_mask(unsigned char inst, bool shadow_mask)
{
	if (shadow_mask)
		osal_spin_lock(&disp_mask_spinlock);

	_reg_write_mask(REG_DISP_CFG(inst), BIT(17),
			(shadow_mask ? BIT(17) : 0));

	if (!shadow_mask)
		osal_spin_unlock(&disp_mask_spinlock);
}

/**
 * gfbg_gop_set_cfg - configure gop
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: gop's settings
 * @param update: update parameter or not
 */
void gfbg_gop_set_cfg(u8 inst, u8 layer, struct disp_gop_cfg *cfg, bool update)
{
	if (inst >= DISP_MAX_INST) {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return;
	}

	gfbg_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_GOP_CFG(inst, layer), cfg->gop_ctrl.raw);
	_reg_write(REG_DISP_GOP_FONTCOLOR(inst, layer),
		(cfg->font_fg_color << 16) | cfg->font_bg_color);
	if (cfg->gop_ctrl.b.colorkey_en)
		_reg_write(REG_DISP_GOP_COLORKEY(inst, layer), cfg->colorkey);
	_reg_write(REG_DISP_GOP_FONTBOX_CTRL(inst, layer), cfg->fb_ctrl.raw);

	// set odec cfg
	_reg_write(REG_DISP_GOP_DEC_CTRL(inst, layer), cfg->odec_cfg.odec_ctrl.raw);

	gfbg_reg_set_shadow_mask(inst, false);

	if (update)
		g_disp_cfg[inst].gop_cfg[layer] = *cfg;
}

/**
 * gfbg_gop_get_cfg - get gop's configurations.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 */
struct disp_gop_cfg *gfbg_gop_get_cfg(u8 inst, u8 layer)
{
	if (inst < DISP_MAX_INST && layer < DISP_MAX_GOP_INST)
		return &g_disp_cfg[inst].gop_cfg[layer];

	return NULL;
}

/**
 * gfbg_gop_ow_set_cfg - set gop's osd-window configurations.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param cfg: ow's settings.
 * @param update: update parameter or not
 */
void gfbg_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct disp_gop_ow_cfg *ow_cfg, bool update)
{
	//OW Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	//4'b1100: Font-base"
	static const u8 reg_map_fmt[DISP_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa, 0xc};

	if (inst >= DISP_MAX_INST) {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 2 layer, no such layer(%d). ", __func__, layer);
		return;
	}

	if (ow_inst >= DISP_MAX_GOP_OW_INST) {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such ow_inst(%d). ", __func__, ow_inst);
		return;
	}

	TRACE_GFBG(DBG_DEBUG, "[disp] %s: inst:%d layer:%d ow_inst:%d ow_cfg->fmt:%d\n",
		     __func__, inst, layer, ow_inst, ow_cfg->fmt);

	gfbg_reg_set_shadow_mask(inst, true);
	_reg_write(REG_DISP_GOP_FMT(inst, layer, ow_inst),
				reg_map_fmt[ow_cfg->fmt]);
	_reg_write(REG_DISP_GOP_H_RANGE(inst, layer, ow_inst),
				(ow_cfg->end.x << 16) | ow_cfg->start.x);
	_reg_write(REG_DISP_GOP_V_RANGE(inst, layer, ow_inst),
				(ow_cfg->end.y << 16) | ow_cfg->start.y);
	_reg_write(REG_DISP_GOP_ADDR_L(inst, layer, ow_inst),
				ow_cfg->addr);
	_reg_write(REG_DISP_GOP_ADDR_H(inst, layer, ow_inst),
				ow_cfg->addr >> 32);
	_reg_write(REG_DISP_GOP_CROP_PITCH(inst, layer, ow_inst),
				(ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
	_reg_write(REG_DISP_GOP_SIZE(inst, layer, ow_inst),
				(ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);

	gfbg_reg_set_shadow_mask(inst, false);

	if (update)
		g_disp_cfg[inst].gop_cfg[layer].ow_cfg[ow_inst] = *ow_cfg;
}

/**
 * gfbg_gop_update_256LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 256LUT-table. There should be 256 instances.
 * @param data: value of 256LUT-table.
 */
int gfbg_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data)
{
	struct disp_gop_cfg gop_cfg = *gfbg_gop_get_cfg(inst, layer);

	if (layer < DISP_MAX_GOP_INST) {
		TRACE_GFBG(DBG_DEBUG, "before update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
			 _reg_read(REG_DISP_GOP_CFG(inst, layer)),
			 _reg_read(REG_DISP_GOP_FMT(inst, 0, layer)));
	} else {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}

	if (index >= 256)
		return -1;

	if (inst < DISP_MAX_INST) {
		gfbg_reg_set_shadow_mask(inst, true);
		//Disable OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), 0x0);

		TRACE_GFBG(DBG_DEBUG, "[disp] update 256LUT in gop1 of display. layer(%d), sc(%d), Index is %d.\n",
			 layer, inst, index);
		_reg_write(REG_DISP_GOP_256LUT0(inst, layer),
					(index << 16) | data);
		_reg_write(REG_DISP_GOP_256LUT1(inst, layer), BIT(16));
		_reg_write(REG_DISP_GOP_256LUT1(inst, layer),  ~(unsigned int)BIT(16));

		//Enable original OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), gop_cfg.gop_ctrl.raw);
		TRACE_GFBG(DBG_DEBUG, "After upadte LUT, gop_cfg ctrl:%#x\n",
			     _reg_read(REG_DISP_GOP_CFG(inst, layer)));
		gfbg_reg_set_shadow_mask(inst, false);
	} else {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}

	return 0;
}

/**
 * gfbg_gop_update_16LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 16LUT-table. There should be 16 instances.
 * @param data: value of 16LUT-table.
 */
int gfbg_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data)
{
	if (layer >= DISP_MAX_GOP_INST) {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}

	if (index > 16)
		return -1;

	if (inst < DISP_MAX_INST) {
		gfbg_reg_set_shadow_mask(inst, true);
		TRACE_GFBG(DBG_DEBUG, "[disp] update 16LUT in gop1 of display. Index is %d.\n", index);
		if (index % 2 == 0) {
			_reg_write_mask(REG_DISP_GOP_16LUT(inst, layer, index / 2), 0xFFFF, data);
		} else {
			_reg_write_mask(REG_DISP_GOP_16LUT(inst, layer, index / 2), 0xFFFF0000, data << 16);
		}
		gfbg_reg_set_shadow_mask(inst, false);
	} else {
		TRACE_GFBG(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}

	return 0;
}

 /**
 * oenc_intr_clr - clear oenc's interrupt
 *				   check flag
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void oenc_intr_clr(u32 flag)
{
	_reg_write(REG_VO_SYS_OENC_INT_GO(0), (0x8000 & (flag<<15)));
}

/**
 * oenc_intr_status - oenc's interrupt status
 *					  check 'union oenc_intr' for each bit mask
 *
 * @return: The interrupt's status
 */

 int oenc_intr_status(void)
 {
	int flag = 0;
	struct oenc_int oenc_int;

	oenc_int.go_intr.raw = _reg_read(REG_VO_SYS_OENC_INT_GO(0));
	flag = oenc_int.go_intr.b.done;
	return flag;
 }

/**
 * oenc_set_cfg - set compression configurations.
 *
 * @param oenc_cfg: compression's settings.
 */
void oenc_set_cfg(struct oenc_cfg oenc_cfg)
{
	//Compression Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	static u8 reg_map_fmt[OENC_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa};

	//reset
	_reg_write_mask(REG_VO_SYS_OENC_RST(0), BIT(0), 1);
	_reg_write_mask(REG_VO_SYS_OENC_RST(0), BIT(0), 0);

	//reset
	_reg_write_mask(REG_VO_SYS_OENC_RST(0), BIT(0), 1);
	_reg_write_mask(REG_VO_SYS_OENC_RST(0), BIT(0), 0);

	oenc_cfg.cfg.b.fmt = reg_map_fmt[oenc_cfg.fmt];
	oenc_cfg.cfg.b.intr_en = 1;
	_reg_write(REG_VO_SYS_OENC_CFG(0), oenc_cfg.cfg.raw);
	_reg_write(REG_VO_SYS_OENC_RANGE(0),
			((oenc_cfg.src_picture_size.h - 1) << 16) | (oenc_cfg.src_picture_size.w - 1));
	_reg_write(REG_VO_SYS_OENC_PITCH(0), oenc_cfg.src_pitch);
	_reg_write(REG_VO_SYS_OENC_SRC_ADDR(0), oenc_cfg.src_adr);
	_reg_write(REG_VO_SYS_OENC_BSO_ADDR(0), oenc_cfg.bso_adr);

	if (oenc_cfg.cfg.b.wprot_en) {
		_reg_write(REG_VO_SYS_OENC_WPROT_LADDR(0), oenc_cfg.wprot_laddr);
		_reg_write(REG_VO_SYS_OENC_WPROT_UADDR(0), oenc_cfg.wprot_uaddr);
	}

	if (oenc_cfg.cfg.b.limit_bsz_en)
		_reg_write(REG_VO_SYS_OENC_LIMIT_BSZ(0), oenc_cfg.limit_bsz);

	//disp_oenc_trig
	_reg_write_mask(REG_VO_SYS_OENC_INT_GO(0), BIT(0), 1);
}

/**
 * oenc_get_cfg - set compression configurations.
 *
 * @param oenc_cfg: compression's settings.
 *
 */
void oenc_get_cfg(struct oenc_cfg *oenc_cfg)
{
	struct oenc_int oenc_trig = {0};

	oenc_trig.go_intr.raw = _reg_read(REG_VO_SYS_OENC_INT_GO(0));

	if (oenc_trig.go_intr.b.done)
		TRACE_GFBG(DBG_DEBUG, "[bm-vip][disp] DISP OSD Compression done!!\n");

	oenc_cfg->cfg.raw = _reg_read(REG_VO_SYS_OENC_CFG(0));
	oenc_cfg->bso_adr = _reg_read(REG_VO_SYS_OENC_BSO_ADDR(0));
	oenc_cfg->bso_sz  = _reg_read(REG_VO_SYS_OENC_BSO_SZ(0)) + 64; //count from 0
	oenc_cfg->bso_mem_size.w = ALIGN(oenc_cfg->bso_sz, 16) & 0x7fff;
	oenc_cfg->bso_mem_size.h = ALIGN(oenc_cfg->bso_sz, 16) >> 15;
}

/**
 * gfbg_gop_odec_set_cfg_from_oenc - setup odec
 *
 * @param cfg: odec_cfg configuration
 */
void gfbg_gop_odec_set_cfg_from_oenc(struct disp_gop_odec_cfg odec_cfg, struct oenc_cfg oenc_cfg)
{
	if (!odec_cfg.odec_ctrl.b.wdt_en) {
		if (oenc_cfg.bso_sz != 0) {
			_reg_write(REG_DISP_GOP_DEC_CTRL(0, 0), odec_cfg.odec_ctrl.raw);
			_reg_write(REG_DISP_GOP_ADDR_L(0, 0, 0),
				oenc_cfg.bso_adr);
			_reg_write(REG_DISP_GOP_ADDR_H(0, 0, 0), 0x1);
			_reg_write(REG_DISP_GOP_SIZE(0, 0, 0),
				(oenc_cfg.bso_mem_size.h << 16) | oenc_cfg.bso_mem_size.w);
		}
	} else {
		if (oenc_cfg.bso_sz != 0) {
			_reg_write(REG_DISP_GOP_DEC_CTRL(0, 0), odec_cfg.odec_ctrl.raw);
			_reg_write(REG_DISP_GOP_ADDR_L(0, 0, 0),
				oenc_cfg.bso_adr);
			_reg_write(REG_DISP_GOP_ADDR_H(0, 0, 0), 0x1);
			_reg_write(REG_DISP_GOP_SIZE(0, 0, 0), (0x100)); //for wdt test only
		}
	}
}
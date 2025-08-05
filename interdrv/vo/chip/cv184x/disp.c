#include "vo_debug.h"
#include "disp.h"
#include "vo_reg.h"
#include "reg.h"
#include "dsi_mac.h"
#include "dsi_phy.h"
#include "vo_mac.h"

#undef BIT
#define BIT(nr)      ((1U) << (nr))

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static struct disp_cfg g_disp_cfg[DISP_MAX_INST];
static struct disp_timing g_disp_timing[DISP_MAX_INST];
static uintptr_t reg_disp_base[DISP_MAX_INST];
static osal_spinlock disp_mask_spinlock;
/****************************************************************************
 * Initial info
 ****************************************************************************/
#define DEFINE_CSC_COEF0(a, b, c) \
		.coef[0][0] = a, .coef[0][1] = b, .coef[0][2] = c,
#define DEFINE_CSC_COEF1(a, b, c) \
		.coef[1][0] = a, .coef[1][1] = b, .coef[1][2] = c,
#define DEFINE_CSC_COEF2(a, b, c) \
		.coef[2][0] = a, .coef[2][1] = b, .coef[2][2] = c,

static struct disp_csc_matrix csc_mtrx[DISP_CSC_MAX] = {
	// none
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(10),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(10))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// yuv2rgb
	// 601 Limited
	//  R = 1.164 *(Y - 16) + 1.596 *(Cr - 128)
	//  G = 1.164 *(Y - 16) - 0.392 *(Cb - 128) - 0.812 *(Cr - 128)
	//  B = 1.164 *(Y - 16) + 2.016 *(Cb - 128)
	{
		DEFINE_CSC_COEF0(1192,	0,		1634)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 401,	BIT(13) | 831)
		DEFINE_CSC_COEF2(1192,	2064,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 601 Full
	//  R = Y + 1.4075 * (V - 128)
	//  G = Y - 0.3455 * (U -128)  - 0.7169 * (V -128)
	//  B = Y + 1.779 * (U -128)
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1441)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 354,	BIT(13) | 734)
		DEFINE_CSC_COEF2(BIT(10),	1822,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Limited
	//  R = 1.164 *(Y - 16) + 1.792 *(Cr - 128)                     //
	//  G = 1.164 *(Y - 16) - 0.213 *(Cb - 128) - 0.534 *(Cr - 128) //
	//  B = 1.164 *(Y - 16) + 2.114 *(Cb - 128)                     //
	{
		DEFINE_CSC_COEF0(1192,	0,		1835)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 218,	BIT(13) | 547)
		DEFINE_CSC_COEF2(1192,	2165,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Full
	// R = Y + 1.5748(Cr – 128)
	// G = Y - 0.1868(Cb – 128) – 0.468(Cr – 128)
	// B = Y + 1.856(Cb – 128)
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1613)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 191,	BIT(13) | 479)
		DEFINE_CSC_COEF2(BIT(10),	1901,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// rgb2yuv
	// 601 Limited
	//  Y = 16  + 0.257 * R + 0.504 * g + 0.098 * b //
	// Cb = 128 - 0.148 * R - 0.291 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.368 * g - 0.071 * b //
	{
		DEFINE_CSC_COEF0(263,		516,		100)
		DEFINE_CSC_COEF1(BIT(13)|152,	BIT(13)|298,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|377,	BIT(13)|73)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	// 601 Full
	//  Y = 0.299 * R + 0.587 * G + 0.114 * B       //
	// Pb =-0.169 * R - 0.331 * G + 0.500 * B       //
	// Pr = 0.500 * R - 0.419 * G - 0.081 * B       //
	{
		DEFINE_CSC_COEF0(306,		601,		117)
		DEFINE_CSC_COEF1(BIT(13)|173,	BIT(13)|339,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|429,	BIT(13)|83)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 709 Limited
	//  Y = 16  + 0.183 * R + 0.614 * g + 0.062 * b //
	// Cb = 128 - 0.101 * R - 0.339 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.399 * g - 0.040 * b //
	{
		DEFINE_CSC_COEF0(187,		629,		63)
		DEFINE_CSC_COEF1(BIT(13)|103,	BIT(13)|347,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|408,	BIT(13)|41)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	// 709 Full
	//   Y =       0.2126   0.7154   0.0722
	//  Cb = 128 - 0.1145  -0.3855   0.5000
	//  Cr = 128 + 0.5000  -0.4543  -0.0457
	{
		DEFINE_CSC_COEF0(218,		733,		74)
		DEFINE_CSC_COEF1(BIT(13)|117,	BIT(13)|395,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|465,	BIT(13)|47)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	{
		DEFINE_CSC_COEF0(BIT(12),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(12),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(12))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
};

/****************************************************************************
 * DISP
 ****************************************************************************/
void disp_set_disp_base_addr(unsigned char inst, void *base)
{
	reg_disp_base[inst] = (uintptr_t)base;
}

/**
 * disp_set_intr_mask - disp's interrupt mask.
 *                      check 'union disp_intr' for each bit mask.
 *
 * @param inst: instance of display
 * @param disp_intr: On/Off ctrl of the interrupt.
 */
void disp_set_intr_mask(unsigned char inst, union disp_intr_sel disp_intr)
{
	_reg_write(REG_DISP_INT_SEL(inst), disp_intr.raw);

	//online frame done should mask odma path frame done,
	//Because this is a problem left by A2
	_reg_write_mask(REG_DISP_INT_CLR(inst), 0xff00, BIT(9));
}

/**
 * disp_get_intr_mask - get disp's interrupt mask.
 *
 * @param inst: instance of display
 * @param disp_intr: display's interrupt status.
 */
void disp_get_intr_mask(unsigned char inst, union disp_intr_sel *disp_intr)
{
	disp_intr->raw = _reg_read(REG_DISP_INT_SEL(inst));
}

/**
 * disp_intr_clr - clear disp's interrupt
 *                 check 'union disp_intr_clr' for each bit mask
 *
 * @param inst: instance of display
 * @param disp_intr: clear of the interrupt.
 */
void disp_intr_clr(unsigned char inst, union disp_intr_clr disp_intr)
{
	_reg_write_mask(REG_DISP_INT_CLR(inst), 0x03, disp_intr.raw);
}

/**
 * disp_intr_status - disp's interrupt status
 *                    check 'union disp_intr' for each bit mask
 *
 * @param inst: instance of display
 * @return: The interrupt's debug status
 */
union disp_intr disp_intr_status(unsigned char inst)
{
	union disp_intr status;

	status.raw = (_reg_read(REG_DISP_DEBUG_STATUS(inst)) & 0xffffffff);
	return status;
}

/**
 * disp_debug_status - disp's debug status
 *                    check 'union disp_dbg_status' for each bit mask
 *
 * @return: The interrupt's status
 */
union disp_dbg_status disp_get_dbg_status(unsigned char inst, bool clr)
{
	union disp_dbg_status status;

	status.raw = _reg_read(REG_DISP_DBG(inst));

	if (clr) {
		status.b.err_fwr_clr = 1;
		status.b.err_erd_clr = 1;
		status.b.bw_fail_clr = 1;
		status.b.osd_bw_fail_clr = 1;
		_reg_write(REG_DISP_DBG(inst), status.raw);
	}

	return status;
}

/****************************************************************************
 * DISP SHADOW REGISTER - USE in DISPLAY and GOP
 ****************************************************************************/
/**
 * disp_reg_shadow_sel - control the read reg-bank.
 *
 * @param read_shadow: true(shadow); false(working)
 */
void disp_reg_shadow_sel(unsigned char inst, bool read_shadow)
{
	_reg_write_mask(REG_DISP_CFG(inst), BIT(18),
			(read_shadow ? 0x0 : BIT(18)));
}

/**
 * disp_reg_set_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param shadow_mask: true(mask); false(unmask)
 */
void disp_reg_set_shadow_mask(unsigned char inst, bool shadow_mask)
{
	if (shadow_mask)
		osal_spin_lock(&disp_mask_spinlock);

	_reg_write_mask(REG_DISP_CFG(inst), BIT(17),
			(shadow_mask ? BIT(17) : 0));

	if (!shadow_mask)
		osal_spin_unlock(&disp_mask_spinlock);
}

/****************************************************************************
 * DISP
 ****************************************************************************/
/**
 * disp_reg_force_up - trigger reg update by sw.
 *
 */
void disp_reg_force_up(unsigned char inst)
{
	_reg_write_mask(REG_DISP_CFG(inst), BIT(16), BIT(16));
}

/**
 * disp_tgen_enable - enable timing-generator on disp.
 *
 * @param enable: AKA.
 * @return: tgen's enable status before change.
 */
bool disp_tgen_enable(unsigned char inst, bool enable)
{
	bool is_enable = (_reg_read(REG_DISP_CFG(inst)) & 0x80);

	if (is_enable != enable) {
		_reg_write_mask(REG_DISP_CFG(inst), 0x0080,
				enable ? 0x80 : 0x00);
		g_disp_cfg[inst].tgen_en = enable;
	}

	return is_enable;
}
osal_module_export(disp_tgen_enable);

/**
 * disp_check_tgen_enable - check whether disp timing-generator enable.
 *
 * @return: tgen's enable status.
 */
bool disp_check_tgen_enable(unsigned char inst)
{
	bool is_enable = (_reg_read(REG_DISP_CFG(inst)) & 0x80);

	return is_enable;
}
osal_module_export(disp_check_tgen_enable);

/**
 * disp_set_bw_cfg - set disp's fifo configurations.
 *
 * @param cfg: disp's settings.
 * @param fmt: rdma fmt.
 */
void disp_set_bw_cfg(unsigned char inst, enum disp_format fmt)
{
	// To avoid bw fail
	if (fmt == DISP_FMT_RGB_PACKED || fmt == DISP_FMT_BGR_PACKED) {
		_reg_write(REG_DISP_LINE_BUFFER_SERIAL(inst), 0x1);
		_reg_write(REG_DISP_RD_TH_Y_MSB(inst), 0x1);
		_reg_write(REG_DISP_FIFO_TH(inst), 0x78);
	} else {
		_reg_write(REG_DISP_LINE_BUFFER_SERIAL(inst), 0x0);
		_reg_write(REG_DISP_RD_TH_Y_MSB(inst), 0x0);
		if (fmt == DISP_FMT_YUV420 || fmt == DISP_FMT_YUV422) {
			_reg_write(REG_DISP_FIFO_TH(inst), 0x4400480);
		} else {
			_reg_write(REG_DISP_FIFO_TH(inst), 0x4800480);
		}
	}

	_reg_write_mask(REG_DISP_PITCH_Y(inst), 0xff000000, 0xff << 24);

	g_disp_cfg[inst].burst = (_reg_read(REG_DISP_PITCH_Y(inst)) >> 28) & 0xf;
	g_disp_cfg[inst].y_thresh = _reg_read(REG_DISP_FIFO_TH(inst)) & 0xff;
	g_disp_cfg[inst].c_thresh = (_reg_read(REG_DISP_FIFO_TH(inst)) >> 16) & 0xff;
}

/**
 * disp_set_cfg - set disp's configurations.
 *
 * @param cfg: disp's settings.
 */
void disp_set_cfg(unsigned char inst, struct disp_cfg *cfg)
{
	unsigned int tmp = 0;

	tmp |= cfg->disp_from_sc;
	tmp |= (cfg->fmt << 12);
	if (cfg->sync_ext)
		tmp |= BIT(4);
	if (cfg->tgen_en)
		tmp |= BIT(7);

	if (!cfg->disp_from_sc) {
		disp_set_mem(inst, &cfg->mem);
		disp_reg_set_shadow_mask(inst, true);
		// _reg_write_mask(REG_DISP_PITCH_Y(inst), 0xf0000000,
		//		cfg->burst << 28);
		disp_set_in_csc(inst, cfg->in_csc);
	} else {
		disp_reg_set_shadow_mask(inst, true);
		// csc only needed if disp from dram
	}

	disp_set_out_csc(inst, cfg->out_csc);
	_reg_write_mask(REG_DISP_CFG(inst), 0x0000f09f, tmp);
	_reg_write_mask(REG_DISP_CATCH(inst), BIT(0), cfg->cache_mode);
	// _reg_write_mask(REG_DISP_FIFO_TH(inst), 0xff, cfg->y_thresh);
	// _reg_write_mask(REG_DISP_FIFO_TH(inst), 0xff0000, cfg->c_thresh << 16);

	switch (cfg->out_bit) {
	case 6:
		tmp = 3 << 16;
		break;
	case 8:
		tmp = 2 << 16;
		break;
	default:
		tmp = 0;
		break;
	}

	tmp |= cfg->drop_mode << 18;
	_reg_write_mask(REG_DISP_PAT_COLOR4(inst), 0x000f0000, tmp);
	disp_reg_set_shadow_mask(inst, false);

	g_disp_cfg[inst] = *cfg;
}

/**
 * disp_get_cfg - get disp's cfg
 *
 * @return: disp's cfg
 */
struct disp_cfg *disp_get_cfg(unsigned char inst)
{
	return &g_disp_cfg[inst];
}

/**
 * disp_cfg_setup_from_reg - get settings from register.
 *
 */
void disp_cfg_setup_from_reg(unsigned char inst)
{
	unsigned int tmp = 0;

	tmp = _reg_read(REG_DISP_CFG(inst));
	g_disp_cfg[inst].disp_from_sc = tmp & BIT(0);
	g_disp_cfg[inst].sync_ext = tmp & BIT(4);
	g_disp_cfg[inst].tgen_en = tmp & BIT(7);
	g_disp_cfg[inst].fmt = (tmp >> 12) & 0xf;

	tmp = _reg_read(REG_DISP_CATCH(inst));
	g_disp_cfg[inst].cache_mode = tmp & BIT(0);

	tmp = _reg_read(REG_DISP_PAT_COLOR4(inst));
	g_disp_cfg[inst].out_bit = (tmp >> 16) & 0x3;
	g_disp_cfg[inst].drop_mode = (tmp >> 18) & 0x3;

	tmp = _reg_read(REG_DISP_PITCH_Y(inst));
	g_disp_cfg[inst].burst = (tmp >> 28) & 0xf;
	tmp = _reg_read(REG_DISP_FIFO_TH(inst));
	g_disp_cfg[inst].y_thresh = tmp & 0xff;
	g_disp_cfg[inst].c_thresh = (tmp >> 16) & 0xff;
}

/**
 * disp_set_timing - modify disp's timing-generator.
 *
 * @param timing: new timing of disp.
 */
void disp_set_timing(unsigned char inst, struct disp_timing *timing)
{
	unsigned int tmp = 0;
	bool is_enable = disp_tgen_enable(inst, false);

	if (timing->vsync_pol)
		tmp |= 0x20;
	if (timing->hsync_pol)
		tmp |= 0x40;

	_reg_write_mask(REG_DISP_CFG(inst), 0x0060, tmp);
	_reg_write(REG_DISP_TOTAL(inst),
		   (timing->htotal << 16) | timing->vtotal);
	_reg_write(REG_DISP_VSYNC(inst),
		   (timing->vsync_end << 16) | timing->vsync_start);
	_reg_write(REG_DISP_VFDE(inst),
		   (timing->vfde_end << 16) | timing->vfde_start);
	_reg_write(REG_DISP_VMDE(inst),
		   (timing->vmde_end << 16) | timing->vmde_start);
	_reg_write(REG_DISP_HSYNC(inst),
		   (timing->hsync_end << 16) | timing->hsync_start);
	_reg_write(REG_DISP_HFDE(inst),
		   (timing->hfde_end << 16) | timing->hfde_start);
	_reg_write(REG_DISP_HMDE(inst),
		   (timing->hmde_end << 16) | timing->hmde_start);

	if (is_enable)
		disp_tgen_enable(inst, true);

	g_disp_timing[inst] = *timing;
}
osal_module_export(disp_set_timing);

struct disp_timing *disp_get_timing(unsigned char inst)
{
	return &g_disp_timing[inst];
}
osal_module_export(disp_get_timing);

void disp_get_hw_timing(unsigned char inst, struct disp_timing *timing)
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
osal_module_export(disp_get_hw_timing);

/**
 * disp_set_rect - setup rect(me) of disp
 *
 * @param rect: the pos/size of me, which should fit with disp's input.
 */
int disp_set_rect(unsigned char inst, struct disp_rect rect)
{
	if (rect.y > g_disp_timing[inst].vfde_end ||
	    rect.x > g_disp_timing[inst].hfde_end ||
	    ((g_disp_timing[inst].vfde_start + rect.y + rect.h - 1) >
	      g_disp_timing[inst].vfde_end) ||
	    ((g_disp_timing[inst].hfde_start + rect.x + rect.w - 1) >
	      g_disp_timing[inst].hfde_end)) {
		TRACE_VO(DBG_ERR, "[disp] %s: dev(%d) me's pos(%d, %d) size(%d, %d)\n",
			 __func__, inst, rect.x, rect.y, rect.w, rect.h);
		TRACE_VO(DBG_ERR, " out of range(%d, %d, %d, %d).\n",
			 g_disp_timing[inst].hfde_start, g_disp_timing[inst].vfde_start,
			 g_disp_timing[inst].hfde_end, g_disp_timing[inst].vfde_end);
		return OSAL_EINVAL;
	}

	g_disp_timing[inst].vmde_start = rect.y + g_disp_timing[inst].vfde_start;
	g_disp_timing[inst].hmde_start = rect.x + g_disp_timing[inst].hfde_start;
	g_disp_timing[inst].vmde_end = g_disp_timing[inst].vmde_start + rect.h - 1;
	g_disp_timing[inst].hmde_end = g_disp_timing[inst].hmde_start + rect.w - 1;

	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_HMDE(inst),
		   (g_disp_timing[inst].hmde_end << 16) | g_disp_timing[inst].hmde_start);
	_reg_write(REG_DISP_VMDE(inst),
		   (g_disp_timing[inst].vmde_end << 16) | g_disp_timing[inst].vmde_start);

	disp_reg_set_shadow_mask(inst, false);

	return 0;
}

/**
 * disp_set_addr - setup disp's mem address. Only work if disp from mem.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void disp_set_addr(unsigned char inst, unsigned long long addr0, unsigned long long addr1, unsigned long long addr2)
{
	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_ADDR0_L(inst), addr0);
	_reg_write(REG_DISP_ADDR0_H(inst), addr0 >> 32);
	_reg_write(REG_DISP_ADDR1_L(inst), addr1);
	_reg_write(REG_DISP_ADDR1_H(inst), addr1 >> 32);
	_reg_write(REG_DISP_ADDR2_L(inst), addr2);
	_reg_write(REG_DISP_ADDR2_H(inst), addr2 >> 32);

	disp_reg_set_shadow_mask(inst, false);

	g_disp_cfg[inst].mem.addr0 = addr0;
	g_disp_cfg[inst].mem.addr1 = addr1;
	g_disp_cfg[inst].mem.addr2 = addr2;
}

/**
 * disp_set_mem - setup disp's mem settings. Only work if disp from mem.
 *
 * @param mem: mem settings for disp
 */
void disp_set_mem(unsigned char inst, struct disp_mem *mem)
{
	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_OFFSET(inst),
		   (mem->start_y << 16) | mem->start_x);
	_reg_write(REG_DISP_SIZE(inst),
		   ((mem->height - 1) << 16) | (mem->width - 1));
	_reg_write_mask(REG_DISP_PITCH_Y(inst), 0x00ffffff,
			mem->pitch_y);
	_reg_write(REG_DISP_PITCH_C(inst), mem->pitch_c);

	disp_reg_set_shadow_mask(inst, false);
	disp_set_addr(inst, mem->addr0, mem->addr1, mem->addr2);

	g_disp_cfg[inst].mem = *mem;
}

/**
 * _disp_set_in_csc - configure disp's input CSC's coefficient/offset
 *
 * @param cfg: The settings for CSC
 */
void _disp_set_in_csc(unsigned char inst, struct disp_csc_matrix *cfg)
{
	_reg_write(REG_DISP_IN_CSC0(inst), BIT(31) |
		   (cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
	_reg_write(REG_DISP_IN_CSC1(inst),
		   (cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
	_reg_write(REG_DISP_IN_CSC2(inst),
		   (cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
	_reg_write(REG_DISP_IN_CSC3(inst),
		   (cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
	_reg_write(REG_DISP_IN_CSC4(inst), (cfg->coef[2][2]));
	_reg_write(REG_DISP_IN_CSC_SUB(inst),
		   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
		   cfg->sub[0]);
	_reg_write(REG_DISP_IN_CSC_ADD(inst),
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) |
		   cfg->add[0]);
}

/**
 * disp_set_in_csc - setup disp's csc on input. Only work if disp from mem.
 *
 * @param csc: csc settings
 */
void disp_set_in_csc(unsigned char inst, enum disp_csc csc)
{
	if (csc == DISP_CSC_NONE) {
		_reg_write(REG_DISP_IN_CSC0(inst), 0);
	} else if (csc < DISP_CSC_MAX) {
		_disp_set_in_csc(inst, &csc_mtrx[csc]);
	}

	g_disp_cfg[inst].in_csc = csc;
}

/**
 * disp_set_out_csc - setup disp's csc on output.
 *
 * @param csc: csc settings
 */
void disp_set_out_csc(unsigned char inst, enum disp_csc csc)
{
	if (csc == DISP_CSC_NONE) {
		_reg_write(REG_DISP_OUT_CSC0(inst), 0);
	} else if (csc < DISP_CSC_MAX) {
		struct disp_csc_matrix *cfg = &csc_mtrx[csc];

		_reg_write(REG_DISP_OUT_CSC0(inst), BIT(31) |
			   (cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
		_reg_write(REG_DISP_OUT_CSC1(inst),
			   (cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
		_reg_write(REG_DISP_OUT_CSC2(inst),
			   (cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
		_reg_write(REG_DISP_OUT_CSC3(inst),
			   (cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
		_reg_write(REG_DISP_OUT_CSC4(inst), (cfg->coef[2][2]));
		_reg_write(REG_DISP_OUT_CSC_SUB(inst),
			   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
			   cfg->sub[0]);
		_reg_write(REG_DISP_OUT_CSC_ADD(inst),
			   (cfg->add[2] << 16) | (cfg->add[1] << 8) |
			   cfg->add[0]);
	}

	g_disp_cfg[inst].out_csc = csc;
}

/**
 * disp_set_pattern - setup disp's pattern generator.
 *
 * @param type: type of pattern
 * @param color: color of pattern. Only for Gradient/FULL type.
 */
void disp_set_pattern(unsigned char inst, enum disp_pat_type type,
			   enum disp_pat_color color, const unsigned short *rgb)
{
	disp_enable_window_bgcolor(inst, false);

	switch (type) {
	case PAT_TYPE_OFF:
		_reg_write_mask(REG_DISP_PAT_CFG(inst), 0x16, 0);
		break;

	case PAT_TYPE_SNOW:
		_reg_write_mask(REG_DISP_PAT_CFG(inst), 0x16, 0x10);
		break;

	case PAT_TYPE_AUTO:
		_reg_write(REG_DISP_PAT_COLOR0(inst), 0x03ff03ff);
		_reg_write_mask(REG_DISP_PAT_COLOR1(inst), 0x000003ff, 0x3ff);
		_reg_write_mask(REG_DISP_PAT_CFG(inst), 0xff0016,
				0x780006);
		break;

	case PAT_TYPE_V_GRAD:
	case PAT_TYPE_H_GRAD:
	case PAT_TYPE_FULL: {
		if (color == PAT_COLOR_USR) {
			_reg_write(REG_DISP_PAT_COLOR0(inst), rgb[1] << 16 | rgb[0]);
			_reg_write_mask(REG_DISP_PAT_COLOR1(inst), 0x000003ff, rgb[2]);
			_reg_write_mask(REG_DISP_PAT_CFG(inst), 0x1f000016,
					(type << 27) | (PAT_COLOR_WHITE << 24) | 0x0002);
		} else {
			_reg_write(REG_DISP_PAT_COLOR0(inst), 0x03ff03ff);
			_reg_write_mask(REG_DISP_PAT_COLOR1(inst), 0x000003ff, 0x3ff);
			_reg_write_mask(REG_DISP_PAT_CFG(inst), 0x1f000016,
					(type << 27) | (color << 24) | 0x0002);
		}
		break;
	}
	default:
		TRACE_VO(DBG_ERR, "%s - unacceptiable pattern-type(%d)\n", __func__, type);
		break;
	}
	_reg_write_mask(REG_DISP_CFG(inst), BIT(7), BIT(7));
}
osal_module_export(disp_set_pattern);

/**
 * disp_set_frame_bgcolor - setup disp frame(area outside mde)'s
 *				 background color.
 *
 * @param r: 10bit red value
 * @param g: 10bit green value
 * @param b: 10bit blue value
 */
void disp_set_frame_bgcolor(unsigned char inst, unsigned short r, unsigned short g, unsigned short b)
{
	_reg_write_mask(REG_DISP_PAT_COLOR1(inst), 0x0fff0000,
			r << 16);
	_reg_write(REG_DISP_PAT_COLOR2(inst), b << 16 | g);
}

/**
 * disp_set_window_bgcolor - setup disp window's background color.
 *
 * @param r: 10bit red value
 * @param g: 10bit green value
 * @param b: 10bit blue value
 */
void disp_set_window_bgcolor(unsigned char inst, unsigned short r, unsigned short g, unsigned short b)
{
	_reg_write(REG_DISP_PAT_COLOR3(inst), g << 16 | r);
	_reg_write_mask(REG_DISP_PAT_COLOR4(inst), 0x0fff, b);
}

/**
 * disp_enable_window_bgcolor - Use window bg-color to hide everything
 *				     including test-pattern.
 *
 * @param enable: enable window bgcolor or not.
 */
void disp_enable_window_bgcolor(unsigned char inst, bool enable)
{
	_reg_write_mask(REG_DISP_PAT_CFG(inst), 0x20, enable ? 0x20 : 0);
}

void disp_gamma_ctrl(unsigned char inst, bool enable, bool pre_osd)
{
	unsigned int value = 0;

	if (enable)
		value |= 0x04;
	if (pre_osd)
		value |= 0x08;
	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x0C, value);
}

void disp_gamma_lut_update(unsigned char inst, const unsigned char *b, const unsigned char *g, const unsigned char *r)
{
	unsigned char i;
	unsigned int value;

	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x03, 0x03);

	for (i = 0; i < DISP_GAMMA_NODE; ++i) {
		value = *(b + i) | (*(g + i) << 8) | (*(r + i) << 16)
			| (i << 24) | 0x80000000;
		_reg_write(REG_DISP_GAMMA_WR_LUT(inst), value);
	}

	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x03, 0x00);
}

void disp_gamma_lut_read(unsigned char inst, struct disp_gamma_attr *gamma_attr)
{
	unsigned char i;
	unsigned int value;

	value = _reg_read(REG_DISP_GAMMA_CTRL(inst));
	gamma_attr->enable = value & 0x04;
	gamma_attr->pre_osd = value & 0x08;
	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x03, 0x01);

	for (i = 0; i < 65; ++i) {
		value = (i << 24) | 0x80000000;
		_reg_write(REG_DISP_GAMMA_WR_LUT(inst), value);
		gamma_attr->table[i] = _reg_read(REG_DISP_GAMMA_RD_LUT(inst));
	}

	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x03, 0x00);
}

void disp_set_intf(unsigned char inst, enum vo_disp_intf intf)
{
	//all intf need dphy's pll
	dphy_init(inst, intf);

	if (intf >= VO_DISP_INTF_MAX)
		vo_mac_set_sel_type(inst, VO_MAC_SEL_DISABLE);
	else if (intf == VO_DISP_INTF_DSI)
		vo_mac_set_sel_type(inst, VO_MAC_SEL_DISABLE);
	else if (intf == VO_DISP_INTF_LVDS)
		vo_mac_set_sel_type(inst, VO_MAC_SEL_DISABLE);
	else if (intf == VO_DISP_INTF_BT601)
		vo_mac_set_sel_type(inst, VO_MAC_SEL_BT601);
	else if (intf == VO_DISP_INTF_BT656)
		vo_mac_set_sel_type(inst, VO_MAC_SEL_BT656);
	else if (intf == VO_DISP_INTF_BT1120)
		vo_mac_set_sel_type(inst, VO_MAC_SEL_BT1120);
	else if (intf == VO_DISP_INTF_HW_I80) {
		vo_mac_set_sel_type(inst, VO_MAC_SEL_HW_MCU);
	}
	//to do: prgb/srgb i80(sw_i80/hw_i80)
}
osal_module_export(disp_set_intf);

/**
 * disp_timing_setup_from_reg - get settings from register.
 *
 */
void disp_timing_setup_from_reg(unsigned char inst)
{
	unsigned int tmp = 0;

	tmp = _reg_read(REG_DISP_CFG(inst));
	g_disp_timing[inst].vsync_pol = (tmp & 0x20);
	g_disp_timing[inst].hsync_pol = (tmp & 0x40);

	tmp = _reg_read(REG_DISP_TOTAL(inst));
	g_disp_timing[inst].vtotal = tmp & 0xffff;
	g_disp_timing[inst].htotal = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_VSYNC(inst));
	g_disp_timing[inst].vsync_start = tmp & 0xffff;
	g_disp_timing[inst].vsync_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_VFDE(inst));
	g_disp_timing[inst].vfde_start = tmp & 0xffff;
	g_disp_timing[inst].vfde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_VMDE(inst));
	g_disp_timing[inst].vmde_start = tmp & 0xffff;
	g_disp_timing[inst].vmde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_HSYNC(inst));
	g_disp_timing[inst].hsync_start = tmp & 0xffff;
	g_disp_timing[inst].hsync_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_HFDE(inst));
	g_disp_timing[inst].hfde_start = tmp & 0xffff;
	g_disp_timing[inst].hfde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_HMDE(inst));
	g_disp_timing[inst].hmde_start = tmp & 0xffff;
	g_disp_timing[inst].hmde_end = (tmp >> 16) & 0xffff;
}

void disp_checksum_en(unsigned char inst, bool enable)
{
	_reg_write_mask(REG_DISP_CHECKSUM0(inst), BIT(31),
			enable ? BIT(31) : 0);
}

void disp_get_checksum_status(unsigned char inst, struct disp_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(REG_DISP_CHECKSUM0(inst));
	status->axi_read_from_dram = _reg_read(REG_DISP_CHECKSUM1(inst));
	status->axi_read_from_gop = _reg_read(REG_DISP_CHECKSUM2(inst));
}

/****************************************************************************
 * DISPLAY CTRL
 ****************************************************************************/
/**
 * disp_ctrl_init - setup all disp instances.
 *
 */
void disp_ctrl_init(bool is_resume)
{
	union disp_intr_sel intr_mask;
	bool disp_from_sc = false;
	unsigned int i, j = 0;

	memset(&intr_mask, 0, sizeof(intr_mask));

	if (!is_resume) {
		// init variables
		memset(&g_disp_cfg, 0, sizeof(g_disp_cfg));
		memset(&g_disp_timing, 0, sizeof(g_disp_timing));

		// init disp mask up lock
		osal_spin_lock_init(&disp_mask_spinlock);

		for (i = 0; i < DISP_MAX_INST; ++i) {
			g_disp_cfg[i].disp_from_sc = disp_from_sc;
			g_disp_cfg[i].cache_mode = true;
			g_disp_cfg[i].sync_ext = false;
			g_disp_cfg[i].tgen_en = false;
			g_disp_cfg[i].fmt = DISP_FMT_RGB_PLANAR;
			g_disp_cfg[i].in_csc = DISP_CSC_NONE;
			g_disp_cfg[i].out_csc = DISP_CSC_NONE;
			g_disp_cfg[i].out_bit = 8;
			g_disp_cfg[i].drop_mode = DISP_DROP_MODE_DITHER;
			g_disp_cfg[i].mem.width = 80;
			g_disp_cfg[i].mem.height = 80;
			// burst length = (burst+1)*16 bytes
			// display burst length keep 128 bytes
			g_disp_cfg[i].burst = DISP_DEFAULT_BURST;
			g_disp_cfg[i].y_thresh = DISP_DEFAULT_Y_THRESH;
			g_disp_cfg[i].c_thresh = DISP_DEFAULT_C_THRESH;
			// display osd burst length set to 256 bytes for short latency
			for (j = 0; j < DISP_MAX_GOP_INST; ++j)
				g_disp_cfg[i].gop_cfg[j].gop_ctrl.b.burst = DISP_DEFAULT_BURST;
		}
	}

	for (i = 0; i < DISP_MAX_INST; ++i) {
		// get current hw-timings
		disp_timing_setup_from_reg(i);
		intr_mask.b.disp_frame_end = true; //this true means enable
		disp_set_intr_mask(i, intr_mask);
	}
}

void disp_ctrl_deinit(void)
{
	// destory disp mask up lock
	osal_spin_lock_destroy(&disp_mask_spinlock);
}

/**
 * ctrl_set_disp_src - setup input-src of disp.
 *
 * @param disp_from_sc: true(from sc_0); false(from mem)
 * @return: 0 if success
 */
int ctrl_set_disp_src(unsigned char inst, bool disp_from_sc)
{
	g_disp_cfg[inst].disp_from_sc = disp_from_sc;
	disp_set_cfg(inst, &g_disp_cfg[inst]);

	return 0;
}

/****************************************************************************
 * DISPLAY GOP
 ****************************************************************************/
/**
 * disp_gop_get_cfg - get gop's configurations.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 */
struct disp_gop_cfg *disp_gop_get_cfg(u8 inst, u8 layer)
{
	if (inst < DISP_MAX_INST && layer < DISP_MAX_GOP_INST)
		return &g_disp_cfg[inst].gop_cfg[layer];

	return NULL;
}
osal_module_export(disp_gop_get_cfg);

/**
 * disp_gop_set_cfg - configure gop
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: gop's settings
 * @param update: update parameter or not
 */
void disp_gop_set_cfg(u8 inst, u8 layer, struct disp_gop_cfg *cfg, bool update)
{
	if (inst >= DISP_MAX_INST) {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return;
	}

	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_GOP_CFG(inst, layer), cfg->gop_ctrl.raw);
	_reg_write(REG_DISP_GOP_FONTCOLOR(inst, layer),
		(cfg->font_fg_color << 16) | cfg->font_bg_color);
	if (cfg->gop_ctrl.b.colorkey_en)
		_reg_write(REG_DISP_GOP_COLORKEY(inst, layer), cfg->colorkey);
	_reg_write(REG_DISP_GOP_FONTBOX_CTRL(inst, layer), cfg->fb_ctrl.raw);

	// set odec cfg
	_reg_write(REG_DISP_GOP_DEC_CTRL(inst, layer), cfg->odec_cfg.odec_ctrl.raw);

	disp_reg_set_shadow_mask(inst, false);

	if (update)
		g_disp_cfg[inst].gop_cfg[layer] = *cfg;
}
osal_module_export(disp_gop_set_cfg);

/**
 * disp_gop_ow_set_cfg - set gop's osd-window configurations.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param cfg: ow's settings.
 * @param update: update parameter or not
 */
void disp_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct disp_gop_ow_cfg *ow_cfg, bool update)
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
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 2 layer, no such layer(%d). ", __func__, layer);
		return;
	}

	if (ow_inst >= DISP_MAX_GOP_OW_INST) {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such ow_inst(%d). ", __func__, ow_inst);
		return;
	}

	TRACE_VO(DBG_DEBUG, "[disp] %s: inst:%d layer:%d ow_inst:%d ow_cfg->fmt:%d\n",
		     __func__, inst, layer, ow_inst, ow_cfg->fmt);

	disp_reg_set_shadow_mask(inst, true);
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

	disp_reg_set_shadow_mask(inst, false);

	if (update)
		g_disp_cfg[inst].gop_cfg[layer].ow_cfg[ow_inst] = *ow_cfg;
}
osal_module_export(disp_gop_ow_set_cfg);

/**
 * disp_gop_update_256LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 256LUT-table. There should be 256 instances.
 * @param data: value of 256LUT-table.
 */
int disp_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data)
{
	struct disp_gop_cfg gop_cfg = *disp_gop_get_cfg(inst, layer);

	if (layer < DISP_MAX_GOP_INST) {
		TRACE_VO(DBG_DEBUG, "before update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
			 _reg_read(REG_DISP_GOP_CFG(inst, layer)),
			 _reg_read(REG_DISP_GOP_FMT(inst, 0, layer)));
	} else {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}

	if (index >= 256)
		return -1;

	if (inst < DISP_MAX_INST) {
		disp_reg_set_shadow_mask(inst, true);
		//Disable OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), 0x0);

		TRACE_VO(DBG_DEBUG, "[disp] update 256LUT in gop1 of display. layer(%d), sc(%d), Index is %d.\n",
			 layer, inst, index);
		_reg_write(REG_DISP_GOP_256LUT0(inst, layer),
					(index << 16) | data);
		_reg_write(REG_DISP_GOP_256LUT1(inst, layer), BIT(16));
		_reg_write(REG_DISP_GOP_256LUT1(inst, layer), ~BIT(16));

		//Enable original OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), gop_cfg.gop_ctrl.raw);
		TRACE_VO(DBG_DEBUG, "After upadte LUT, gop_cfg ctrl:%#x\n",
			     _reg_read(REG_DISP_GOP_CFG(inst, layer)));
		disp_reg_set_shadow_mask(inst, false);
	} else {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}

	return 0;
}
osal_module_export(disp_gop_update_256LUT);

/**
 * disp_gop_update_16LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 16LUT-table. There should be 16 instances.
 * @param data: value of 16LUT-table.
 */
int disp_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data)
{
	if (layer >= DISP_MAX_GOP_INST) {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}

	if (index > 16)
		return -1;

	if (inst < DISP_MAX_INST) {
		disp_reg_set_shadow_mask(inst, true);
		TRACE_VO(DBG_DEBUG, "[disp] update 16LUT in gop1 of display. Index is %d.\n", index);
		if (index % 2 == 0) {
			_reg_write_mask(REG_DISP_GOP_16LUT(inst, layer, index / 2), 0xFFFF, data);
		} else {
			_reg_write_mask(REG_DISP_GOP_16LUT(inst, layer, index / 2), 0xFFFF0000, data << 16);
		}
		disp_reg_set_shadow_mask(inst, false);
	} else {
		TRACE_VO(DBG_ERR, "[disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}

	return 0;
}
osal_module_export(disp_gop_update_16LUT);

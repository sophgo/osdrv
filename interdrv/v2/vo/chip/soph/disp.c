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

#include <linux/cvi_comm_vo.h>

#include "vo_common.h"
#include "disp.h"
#include "disp_reg.h"
#include "reg.h"
#include "dsi_phy.h"
#include "reg_disp.h"
#include "reg_vgop.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static struct disp_oenc_cfg g_oenc_cfg[MAX_OSD_ENC_INST];
static struct disp_odma_cfg g_odma_cfg[DISP_MAX_INST];
static struct disp_cfg g_disp_cfg[DISP_MAX_INST];
static struct disp_timing disp_timing[DISP_MAX_INST];
static uintptr_t reg_vo_mac_base[DISP_MAX_INST];
static uintptr_t reg_disp_base[DISP_MAX_INST];
static uintptr_t reg_dsi_mac_base[DISP_MAX_INST];
static uintptr_t reg_oenc_base[DISP_MAX_INST];
static spinlock_t disp_mask_spinlock;
static void *gp_reg;
static void *retrain_reg;
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

#define RETRAIN_REG 0x281000F4
#define GP_REG 0x281000F8
#define POLY 0x8408
/*
 *                                      16   12   5
 * this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
 * This works out to be 0x1021, but the way the algorithm works
 * lets us use 0x8408 (the reverse of the bit pattern).  The high
 * bit is always assumed to be set, thus we only use 16 bits to
 * represent the 17 bit value.
 */
static u16 crc16(u8 *data_p, u16 length)
{
	u8 i, data;
	u16 crc = 0xffff;

	if (length == 0)
		return (~crc);

	do {
		for (i = 0, data = 0xff & *data_p++; i < 8; i++, data >>= 1) {
			if ((crc & 0x0001) ^ (data & 0x0001))
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
		}
	} while (--length);

	return crc;
}

static u8 ecc(u8 *data)
{
	char D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12;
	char D13, D14, D15, D16, D17, D18, D19, D20, D21, D22, D23;
	char P0, P1, P2, P3, P4, P5, P6, P7;

	D0  = data[0] & 0x01;
	D1  = (data[0] >> 1) & 0x01;
	D2  = (data[0] >> 2) & 0x01;
	D3  = (data[0] >> 3) & 0x01;
	D4  = (data[0] >> 4) & 0x01;
	D5  = (data[0] >> 5) & 0x01;
	D6  = (data[0] >> 6) & 0x01;
	D7  = (data[0] >> 7) & 0x01;

	D8  = data[1] & 0x01;
	D9  = (data[1] >> 1) & 0x01;
	D10 = (data[1] >> 2) & 0x01;
	D11 = (data[1] >> 3) & 0x01;
	D12 = (data[1] >> 4) & 0x01;
	D13 = (data[1] >> 5) & 0x01;
	D14 = (data[1] >> 6) & 0x01;
	D15 = (data[1] >> 7) & 0x01;

	D16 = data[2] & 0x01;
	D17 = (data[2] >> 1) & 0x01;
	D18 = (data[2] >> 2) & 0x01;
	D19 = (data[2] >> 3) & 0x01;
	D20 = (data[2] >> 4) & 0x01;
	D21 = (data[2] >> 5) & 0x01;
	D22 = (data[2] >> 6) & 0x01;
	D23 = (data[2] >> 7) & 0x01;

	P7 = 0;
	P6 = 0;
	P5 = (D10^D11^D12^D13^D14^D15^D16^D17^D18^D19^D21^D22^D23) & 0x01;
	P4 = (D4^D5^D6^D7^D8^D9^D16^D17^D18^D19^D20^D22^D23) & 0x01;
	P3 = (D1^D2^D3^D7^D8^D9^D13^D14^D15^D19^D20^D21^D23) & 0x01;
	P2 = (D0^D2^D3^D5^D6^D9^D11^D12^D15^D18^D20^D21^D22) & 0x01;
	P1 = (D0^D1^D3^D4^D6^D8^D10^D12^D14^D17^D20^D21^D22^D23) & 0x01;
	P0 = (D0^D1^D2^D4^D5^D7^D10^D11^D13^D16^D20^D21^D22^D23) & 0x01;

	return (P7 << 7) | (P6 << 6) | (P5 << 5) | (P4 << 4) |
		(P3 << 3) | (P2 << 2) | (P1 << 1) | P0;
}

/****************************************************************************
 * DISP
 ****************************************************************************/
void disp_set_vo_mac_base_addr(u8 inst, void *base)
{
	reg_vo_mac_base[inst] = (uintptr_t)base;
}

void disp_set_disp_base_addr(u8 inst, void *base)
{
	reg_disp_base[inst] = (uintptr_t)base;
}

void disp_set_dsi_mac_base_addr(u8 inst, void *base)
{
	reg_dsi_mac_base[inst] = (uintptr_t)base;
}

void disp_set_oenc_base_addr(u8 inst, void *base)
{
	reg_oenc_base[inst] = (uintptr_t)base;
}

void disp_top_set_vo_data_mux(u8 inst, u8 vodata_selID, u8 value)
{
	uintptr_t vo_mux_addr[10] = {REG_VO_MAC_VO_MUX0(inst), REG_VO_MAC_VO_MUX1(inst),
							REG_VO_MAC_VO_MUX2(inst), REG_VO_MAC_VO_MUX3(inst),
							REG_VO_MAC_VO_MUX4(inst), REG_VO_MAC_VO_MUX5(inst),
							REG_VO_MAC_VO_MUX6(inst), REG_VO_MAC_VO_MUX7(inst),
							REG_VO_MAC_VO_MUX8(inst)};

	union disp_vo_mux vo_mux;
	u8 vodata_muxidx = (vodata_selID + 2) / 4;
	u8 vodata_selidx = (vodata_selID + 2) % 4;

	vo_mux.raw = _reg_read(vo_mux_addr[vodata_muxidx]);

	switch (vodata_selidx) {
	case 0:
		vo_mux.b.vod_sel0 = value;
		break;

	case 1:
		vo_mux.b.vod_sel1 = value;
		break;

	case 2:
		vo_mux.b.vod_sel2 = value;
		break;

	case 3:
		vo_mux.b.vod_sel3 = value;
		break;

	default:
		break;
	}
	_reg_write(vo_mux_addr[vodata_muxidx], vo_mux.raw);
}

void disp_set_vo_type_sel(u8 inst, enum disp_vo_sel vo_sel)
{
	union disp_vo_mux_sel vo_mux;

	vo_mux.raw = _reg_read(REG_VO_MAC_VO_MUX(inst));
	vo_mux.b.vo_sel_type = vo_sel;

	_reg_write(REG_VO_MAC_VO_MUX(inst), vo_mux.raw);
}

void disp_mux_sel(u8 inst, enum disp_vo_sel sel)
{
	_reg_write_mask(REG_VO_MAC_VO_MUX(inst), 0x07, sel);
}

enum disp_vo_sel disp_mux_get(u8 inst)
{
	return _reg_read(REG_VO_MAC_VO_MUX(inst)) & 0x07;
}
EXPORT_SYMBOL_GPL(disp_mux_get);

/**
 * disp_set_intr_mask - disp's interrupt mask.
 *                      check 'union disp_intr' for each bit mask.
 *
 * @param inst: instance of display
 * @param disp_intr: On/Off ctrl of the interrupt.
 */
void disp_set_intr_mask(u8 inst, union disp_intr_sel disp_intr)
{
	_reg_write(REG_DISP_INT_SEL(inst), disp_intr.raw);
}

/**
 * disp_get_intr_mask - get disp's interrupt mask.
 *
 * @param inst: instance of display
 * @param disp_intr: display's interrupt status.
 */
void disp_get_intr_mask(u8 inst, union disp_intr_sel *disp_intr)
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
void disp_intr_clr(u8 inst, union disp_intr_clr disp_intr)
{
	_reg_write_mask(REG_DISP_INT_CLR(inst), 0x07, disp_intr.raw);
}

/**
 * disp_intr_status - disp's interrupt status
 *                    check 'union disp_intr' for each bit mask
 *
 * @param inst: instance of display
 * @return: The interrupt's debug status
 */
union disp_intr disp_intr_status(u8 inst)
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
union disp_dbg_status disp_get_dbg_status(u8 inst, bool clr)
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
 * DISPLAY ODMA
 ****************************************************************************/

/**
 * disp_set_odma_intr_mask - get odma's interrupt mask.
 *
 * @param inst: instance of display
 * @param online_odma_mask: display's online or odma interrupt mask.
 */
void disp_set_odma_intr_mask(u8 inst, union disp_online_odma_intr_sel online_odma_mask)
{
	_reg_write_mask(REG_DISP_INT_ON_ODMA_SEL(inst), 0x300, online_odma_mask.raw);
}

/**
 * disp_get_odma_intr_mask - get odma's interrupt mask.
 *
 * @param inst: instance of display
 * @param online_odma_mask: display's online or odma interrupt mask.
 */
void disp_get_odma_intr_mask(u8 inst, union disp_online_odma_intr_sel *online_odma_mask)
{
	online_odma_mask->raw = _reg_read(REG_DISP_INT_ON_ODMA_SEL(inst));
}


/**
 * disp_odma_fifofull_clr - clear odma's fifo full err
 *
 * @param inst: instance of display
 */
void disp_odma_fifofull_clr(u8 inst)
{
	_reg_write_mask(REG_DISP_ODMA_FIFO_CFG(inst), BIT(9), BIT(9));
	_reg_write_mask(REG_DISP_ODMA_FIFO_CFG(inst), BIT(9), 0);
}

/**
 * disp_odma_set_addr - setup odma's mem address.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void disp_odma_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2)
{
	_reg_write(REG_DISP_ODMA_Y_L(inst), addr0);
	_reg_write(REG_DISP_ODMA_Y_H(inst), addr0 >> 32);
	_reg_write(REG_DISP_ODMA_U_L(inst), addr1);
	_reg_write(REG_DISP_ODMA_U_H(inst), addr1 >> 32);
	_reg_write(REG_DISP_ODMA_V_L(inst), addr2);
	_reg_write(REG_DISP_ODMA_V_H(inst), addr2 >> 32);

	g_odma_cfg[inst].mem.addr0 = addr0;
	g_odma_cfg[inst].mem.addr1 = addr1;
	g_odma_cfg[inst].mem.addr2 = addr2;
}

/**
 * disp_odma_set_mem - setup odma's mem settings.
 *
 * @param mem: mem settings for odma
 */
void disp_odma_set_mem(u8 inst, struct disp_mem *mem)
{
	_reg_write(REG_DISP_ODMA_X_STR(inst), mem->start_x);
	_reg_write(REG_DISP_ODMA_Y_STR(inst), mem->start_y);
	_reg_write(REG_DISP_ODMA_WIDETH(inst), mem->width - 1);
	_reg_write(REG_DISP_ODMA_HEIGHT(inst), mem->height - 1);
	_reg_write(REG_DISP_ODMA_PITCH_Y(inst), mem->pitch_y);
	_reg_write(REG_DISP_ODMA_PITCH_C(inst), mem->pitch_c);

	_reg_write(REG_DISP_ODMA_FIFO_WH(inst),
		(mem->width - 1) + ((mem->height - 1 ) << 16));

	disp_odma_set_addr(inst, mem->addr0, mem->addr1, mem->addr2);

	g_odma_cfg[inst].mem = *mem;
}

void disp_odma_set_csc(u8 inst, enum disp_csc csc)
{
	if (csc == DISP_CSC_NONE) {
		_reg_write(REG_DISP_ODMA8(inst), 0x0);
	} else if (csc < DISP_CSC_MAX) {
		struct disp_csc_matrix *cfg = &csc_mtrx[csc];
		_reg_write(REG_DISP_ODMA1(inst),
			(cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
		_reg_write(REG_DISP_ODMA2(inst),
			(cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
		_reg_write(REG_DISP_ODMA3(inst),
			(cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
		_reg_write(REG_DISP_ODMA4(inst),
			(cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
		_reg_write(REG_DISP_ODMA5(inst), (cfg->coef[2][2]));
		_reg_write(REG_DISP_ODMA6(inst),
			(cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
			cfg->sub[0]);
		_reg_write(REG_DISP_ODMA7(inst),
			(cfg->add[2] << 16) | (cfg->add[1] << 8) |
			cfg->add[0]);
		_reg_write(REG_DISP_ODMA8(inst), 0x1);
	}

	g_odma_cfg[inst].csc = csc;
}

/**
 * disp_odma_set_cfg - configure display's odma
 *
 * @param inst: (0~1), the instance of display which want to be configured.
 * @param burst: dma's burst length
 * @param fmt: dma's format
 */
void disp_odma_set_cfg(u8 inst, struct disp_odma_cfg *cfg)
{
	_reg_write_mask(REG_DISP_ODMA_CFG(inst), 0xF01, (cfg->fmt << 8) | cfg->burst);
	disp_odma_set_mem(inst, &cfg->mem);
	disp_odma_set_csc(inst, cfg->csc);

	g_odma_cfg[inst] = *cfg;
}

/**
 * disp_odma_get_cfg - get disp_odma's cfg
 *
 * @param inst: (0~1), the instance of display which want to be got.
 * @return: disp_odma's cfg
 */
struct disp_odma_cfg *disp_odma_get_cfg(u8 inst)
{
	if (inst >= DISP_MAX_INST)
		return NULL;
	return &g_odma_cfg[inst];
}

/**
 * disp_odma_set_fmt - set disp_odma's output data format
 *
 * @param inst: (0~1), the instance of display which want to be configured.
 * @param fmt: _odma's output format
 */
void disp_odma_set_fmt(u8 inst, enum disp_format fmt)
{
	u32 tmp = fmt << 8;

	if (fmt == DISP_FMT_BF16)
		tmp |= BIT(23);
	_reg_write_mask(REG_DISP_ODMA_CFG(inst), 0x0080ff00, tmp);

	g_odma_cfg[inst].fmt = fmt;
}

/**
 * disp_odma_enable - set enable/disable odma
 *
 * @param inst: (0~1), the instance of display which want to be configured.
 * @param enable: enable/disable
 */
void disp_odma_enable(u8 inst, bool enable)
{
	bool is_enable = (_reg_read(REG_DISP_ODMA_CFG(inst)) & 0x1000000);

	if (is_enable != enable) {
		_reg_write_mask(REG_DISP_ODMA_CFG(inst), 0x1000000,
				enable ? 0x1000000 : 0x00);
		g_odma_cfg[inst].enable = enable;
	}
}

#if 0
union sclr_odma_dbg_status sclr_odma_get_dbg_status(u8 inst)
{
	union sclr_odma_dbg_status status;

	status.raw = _reg_read(REG_SCL_ODMA_DBG(inst));

	return status;
}

void sclr_odma_get_sb_default(struct sclr_odma_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->sb_nb = 3;
	cfg->sb_full_nb = 2;
}

void sclr_odma_set_sb(u8 inst, struct sclr_odma_sb_cfg *cfg)
{
	u32 val = 0;

	val |= (cfg->sb_mode << SC_ODMA_REG_SB_MODE_OFFSET) & SC_ODMA_REG_SB_MODE_MASK;
	val |= (cfg->sb_size << SC_ODMA_REG_SB_SIZE_OFFSET) & SC_ODMA_REG_SB_SIZE_MASK;
	val |= (cfg->sb_nb << SC_ODMA_REG_SB_NB_OFFSET) & SC_ODMA_REG_SB_NB_MASK;
	val |= (cfg->sb_full_nb << SC_ODMA_REG_SB_FULL_NB_OFFSET) & SC_ODMA_REG_SB_FULL_NB_MASK;
	val |= (cfg->sb_sw_wptr << SC_ODMA_REG_SB_SW_WPTR_OFFSET) & SC_ODMA_REG_SB_SW_WPTR_MASK;
	val |= (cfg->sb_set_str << SC_ODMA_REG_SB_SET_STR_OFFSET) & SC_ODMA_REG_SB_SET_STR_MASK;
	val |= (cfg->sb_sw_clr << SC_ODMA_REG_SB_SW_CLR_OFFSET) & SC_ODMA_REG_SB_SW_CLR_MASK;

	_reg_write(REG_SCL_ODMA_SB_CTRL(inst), val);
}

void sclr_odma_clear_sb(u8 inst)
{
	u32 val = 0;

	val |= (1 << SC_ODMA_REG_SB_SET_STR_OFFSET) & SC_ODMA_REG_SB_SET_STR_MASK;
	val |= (1 << SC_ODMA_REG_SB_SW_CLR_OFFSET) & SC_ODMA_REG_SB_SW_CLR_MASK;

	_reg_write(REG_SCL_ODMA_SB_CTRL(inst), val);
}

#endif

/****************************************************************************
 * OSD Compression(OSD Encoder)
 ****************************************************************************/
/**
 * disp_oenc_set_cfg - set compression configurations.
 *
 * @param oenc_cfg: compression's settings.
 */
void disp_oenc_set_cfg(u8 oenc_inst, struct disp_oenc_cfg *oenc_cfg)
{
	//Compression Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	static u8 reg_map_fmt[DISP_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa};

	if (oenc_inst < MAX_OSD_ENC_INST) {
		//reset
		_reg_write_mask(REG_VO_SYS_OENC_RST(oenc_inst), BIT(0), 1);
		_reg_write_mask(REG_VO_SYS_OENC_RST(oenc_inst), BIT(0), 0);

		oenc_cfg[oenc_inst].cfg.b.fmt = reg_map_fmt[oenc_cfg[oenc_inst].fmt];
		oenc_cfg[oenc_inst].cfg.b.intr_en = 1;
		_reg_write(REG_VO_SYS_OENC_CFG(oenc_inst), oenc_cfg[oenc_inst].cfg.raw);
		_reg_write(REG_VO_SYS_OENC_RANGE(oenc_inst),
				((oenc_cfg[oenc_inst].src_picture_size.h - 1) << 16) |
				(oenc_cfg[oenc_inst].src_picture_size.w - 1));
		_reg_write(REG_VO_SYS_OENC_PITCH(oenc_inst), oenc_cfg[oenc_inst].src_pitch);
		_reg_write(REG_VO_SYS_OENC_SRC_ADDR(oenc_inst), oenc_cfg[oenc_inst].src_adr);
		_reg_write(REG_VO_SYS_OENC_BSO_ADDR(oenc_inst), oenc_cfg[oenc_inst].bso_adr);

		if (oenc_cfg[oenc_inst].cfg.b.wprot_en) {
			_reg_write(REG_VO_SYS_OENC_WPROT_LADDR(oenc_inst), oenc_cfg[oenc_inst].wprot_laddr);
			_reg_write(REG_VO_SYS_OENC_WPROT_UADDR(oenc_inst), oenc_cfg[oenc_inst].wprot_uaddr);
		}

		if (oenc_cfg[oenc_inst].cfg.b.limit_bsz_en)
			_reg_write(REG_VO_SYS_OENC_LIMIT_BSZ(oenc_inst), oenc_cfg[oenc_inst].limit_bsz);

		//disp_oenc_trig
		_reg_write_mask(REG_VO_SYS_OENC_INT_GO(oenc_inst), BIT(0), 1);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[%s]Invalid oenc(%d), oenc only has 2 engine(0 & 1)", __func__, oenc_inst);
	}
}

/**
 * disp_oenc_get_cfg - set compression configurations.
 *
 * @return oenc_cfg: compression's settings.
 *
 */
struct disp_oenc_cfg *disp_oenc_get_cfg(u8 oenc_inst)
{
	struct disp_oenc_int oenc_trig;

	if (oenc_inst < MAX_OSD_ENC_INST) {
		oenc_trig.go_intr.raw = _reg_read(REG_VO_SYS_OENC_INT_GO(oenc_inst));

		if (oenc_trig.go_intr.b.done)
			CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] SCLR OSD Compression done!!\n");

		CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] SCLR OSD Compression INTR vector:");
		if (oenc_trig.go_intr.b.intr_vec & BIT(0))
			CVI_TRACE_VO(CVI_DBG_DEBUG, "Successful!!\n");
		if (oenc_trig.go_intr.b.intr_vec & BIT(2))
			CVI_TRACE_VO(CVI_DBG_ERR, "Fail, Bitstream size great than limiter!!\n");
		if (oenc_trig.go_intr.b.intr_vec & BIT(3))
			CVI_TRACE_VO(CVI_DBG_ERR, "Fail, Watch Dog time-out!!\n");
		if (oenc_trig.go_intr.b.intr_vec & BIT(4))
			CVI_TRACE_VO(CVI_DBG_ERR, "Fail, Out of Dram Write protection region!!\n");

		g_oenc_cfg[oenc_inst].cfg.raw = _reg_read(REG_VO_SYS_OENC_CFG(oenc_inst));
		g_oenc_cfg[oenc_inst].bso_adr = _reg_read(REG_VO_SYS_OENC_BSO_ADDR(oenc_inst));
		g_oenc_cfg[oenc_inst].bso_sz  = _reg_read(REG_VO_SYS_OENC_BSO_SZ(oenc_inst)) + 1;
		g_oenc_cfg[oenc_inst].bso_mem_size.w = ALIGN(g_oenc_cfg[oenc_inst].bso_sz, 16) & 0x3fff;
		g_oenc_cfg[oenc_inst].bso_mem_size.h = ALIGN(g_oenc_cfg[oenc_inst].bso_sz, 16) >> 14;
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 oenc_inst, no such oenc_inst(%d). ", __func__, oenc_inst);
		return NULL;
	}

	return &g_oenc_cfg[oenc_inst];
}

/****************************************************************************
 * DISP SHADOW REGISTER - USE in DISPLAY and GOP
 ****************************************************************************/
/**
 * disp_reg_shadow_sel - control the read reg-bank.
 *
 * @param read_shadow: true(shadow); false(working)
 */
void disp_reg_shadow_sel(u8 inst, bool read_shadow)
{
	_reg_write_mask(REG_DISP_CFG(inst), BIT(18),
			(read_shadow ? 0x0 : BIT(18)));
}

/**
 * disp_reg_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param mask: true(mask); false(unmask)
 * @return: mask status before modification.
 */
bool disp_reg_shadow_mask(u8 inst, bool mask)
{
	bool is_masked = (_reg_read(REG_DISP_CFG(inst)) & BIT(17));

	if (is_masked != mask)
		_reg_write_mask(REG_DISP_CFG(inst), BIT(17),
				(mask ? BIT(17) : 0));

	return is_masked;
}

/**
 * disp_reg_set_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param shadow_mask: true(mask); false(unmask)
 */
void disp_reg_set_shadow_mask(u8 inst, bool shadow_mask)
{
	if (shadow_mask)
		spin_lock(&disp_mask_spinlock);

	_reg_write_mask(REG_DISP_CFG(inst), BIT(17),
			(shadow_mask ? BIT(17) : 0));

	if (!shadow_mask)
		spin_unlock(&disp_mask_spinlock);
}

/****************************************************************************
 * DISPLAY GOP
 ****************************************************************************/
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
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return;
	}

	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_GOP_CFG(inst, layer), cfg->gop_ctrl.raw);
	_reg_write(REG_DISP_GOP_FONTCOLOR(inst, layer),
		(cfg->font_fg_color << 16) | cfg->font_bg_color);
	if (cfg->gop_ctrl.b.colorkey_en)
		_reg_write(REG_DISP_GOP_COLORKEY(inst, layer), cfg->colorkey);
	_reg_write(REG_DISP_GOP_FONTBOX_CTRL(inst, layer), cfg->fb_ctrl.raw);

	// // ECO item for threshold invert
	// _reg_write_mask(REG_DISP_BASE(inst) + 0x90f8, 0x1, cfg->fb_ctrl.b.lo_thr_inv);
	// set odec cfg
	_reg_write(REG_DISP_GOP_DEC_CTRL(inst, layer), cfg->odec_cfg.odec_ctrl.raw);

	disp_reg_set_shadow_mask(inst, false);

	if (update)
		g_disp_cfg[inst].gop_cfg[layer] = *cfg;
}
EXPORT_SYMBOL_GPL(disp_gop_set_cfg);

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
EXPORT_SYMBOL_GPL(disp_gop_get_cfg);

/**
 * disp_gop_setup 256LUT - setup gop's Look-up table
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 256LUT-table for index 0 ~ length.
 *		There should be smaller than 256 instances.
 * @param data: values of 256LUT-table. There should be 256 instances.
 */
int disp_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data)
{
	u16 i = 0;
	struct disp_gop_cfg gop_cfg;

#if 0
	void *vip_clk_reg = ioremap(0x3002008, 4);
	u32 vip_pll = ioread32(vip_clk_reg);

	iounmap(vip_clk_reg);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "hw vip clk:%#x\n", vip_pll);
#endif

	CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] %s:  inst(%d) layer(%d) length(%d)\n", __func__, inst, layer, length);
	gop_cfg = *disp_gop_get_cfg(inst, layer);

	if (layer < DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "before update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
			_reg_read(REG_DISP_GOP_CFG(inst, layer)),
			_reg_read(REG_DISP_GOP_FMT(inst, 0, layer)));
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}

	if (length >= 256) {
		CVI_TRACE_VO(CVI_DBG_ERR, "LUT length(%d) error, should less or equal to 256!\n", length);
		return -1;
	}

	if (inst < DISP_MAX_INST) {
		disp_reg_set_shadow_mask(inst, true);
		//Disable OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), 0x0);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] update 256LUT in gop1 of display. layer(%d), sc(%d). Length is %d.\n",
			layer, inst, length);
		for (i = 0; i < length; ++i) {
			_reg_write(REG_DISP_GOP_256LUT0(inst, layer),
						(i << 16) | *(data + i));
			_reg_write(REG_DISP_GOP_256LUT1(inst, layer), BIT(16));
			_reg_write(REG_DISP_GOP_256LUT1(inst, layer), ~BIT(16));
			CVI_TRACE_VO(CVI_DBG_DEBUG, "write LUT index:%d value:%#x\n", i, *(data + i));
		}
#if 0 /* do not read when normal operation */
		for (i = 0; i < length; ++i) {
			_reg_write(REG_DISP_GOP_256LUT0(inst, layer), (i << 16));
			_reg_write(REG_DISP_GOP_256LUT1(inst, layer), BIT(17));
			_reg_write(REG_DISP_GOP_256LUT1(inst, layer), ~BIT(17));
			CVI_TRACE_VO(CVI_DBG_DEBUG, "read LUT index:%d value:%#x\n",
				i, _reg_read(REG_DISP_GOP_256LUT1(inst, layer)) & 0xFFFF);
		}
#endif
		//Enable original OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), gop_cfg.gop_ctrl.raw);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "After update LUT, gop_cfg ctrl:%#x\n", _reg_read(REG_DISP_GOP_CFG(inst, layer)));
		disp_reg_set_shadow_mask(inst, false);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}
	return 0;
}

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
		CVI_TRACE_VO(CVI_DBG_DEBUG, "before update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
			_reg_read(REG_DISP_GOP_CFG(inst, layer)),
			_reg_read(REG_DISP_GOP_FMT(inst, 0, layer)));
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}

	if (index >= 256)
		return -1;

	if (inst < DISP_MAX_INST) {
		disp_reg_set_shadow_mask(inst, true);
		//Disable OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), 0x0);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] update 256LUT in gop1 of display. layer(%d), sc(%d), Index is %d.\n",
				layer, inst, index);
		_reg_write(REG_DISP_GOP_256LUT0(inst, layer),
					(index << 16) | data);
		_reg_write(REG_DISP_GOP_256LUT1(inst, layer), BIT(16));
		_reg_write(REG_DISP_GOP_256LUT1(inst, layer), ~BIT(16));

		//Enable original OW enable in gop ctrl register
		_reg_write(REG_DISP_GOP_CFG(inst, layer), gop_cfg.gop_ctrl.raw);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "After upadte LUT, gop_cfg ctrl:%#x\n", _reg_read(REG_DISP_GOP_CFG(inst, layer)));
		disp_reg_set_shadow_mask(inst, false);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(disp_gop_update_256LUT);

/**
 * disp_gop_setup 16LUT - setup gop's Look-up table
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 16LUT-table for index 0 ~ length.
 *		There should be smaller than 16 instances.
 * @param data: values of 16LUT-table. There should be 16 instances.
 */
int disp_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data)
{
	u16 i = 0;

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}
	if (length > 16)
		return -1;

	if (inst < DISP_MAX_INST) {
		disp_reg_set_shadow_mask(inst, true);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-vip][sc] update 16LUT in gop1 of display. Length is %d.\n", length);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] update 16LUT in gop1 of display. Length is %d.\n", length);
		for (i = 0; i <= length; i += 2) {
			_reg_write(REG_DISP_GOP_16LUT(inst, layer, i / 2),
						((*(data + i + 1) << 16) | (*(data + i))));
		}

		disp_reg_set_shadow_mask(inst, false);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}
	return 0;
}

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
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return -1;
	}
	if (index > 16)
		return -1;

	if (inst < DISP_MAX_INST) {
		disp_reg_set_shadow_mask(inst, true);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] update 16LUT in gop1 of display. Index is %d.\n", index);
		if (index % 2 == 0) {
			_reg_write_mask(REG_DISP_GOP_16LUT(inst, layer, index / 2), 0xFFFF, data);
		} else {
			_reg_write_mask(REG_DISP_GOP_16LUT(inst, layer, index / 2), 0xFFFF0000, data);
		}

		disp_reg_set_shadow_mask(inst, false);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}
	return 0;
}

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
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 layer, no such layer(%d). ", __func__, layer);
		return;
	}

	if (ow_inst >= DISP_MAX_GOP_OW_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such ow_inst(%d). ", __func__, ow_inst);
		return;
	}
	CVI_TRACE_VO(CVI_DBG_DEBUG, "[cvi-disp] %s: inst:%d layer:%d ow_inst:%d ow_cfg->fmt:%d\n",
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
EXPORT_SYMBOL_GPL(disp_gop_ow_set_cfg);

/**
 * disp_gop_ow_get_addr - get gop's osd-window DRAM addr.
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param addr: ow's DRAM address.
 */
void disp_gop_ow_get_addr(u8 inst, u8 layer, u8 ow_inst, u64 *addr)
{
	if (inst >= DISP_MAX_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 layer, no such layer(%d). ", __func__, layer);
		return;
	}

	if (ow_inst >= DISP_MAX_GOP_OW_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such ow_inst(%d). ", __func__, ow_inst);
		return;
	}

	*addr = _reg_read(REG_DISP_GOP_ADDR_L(inst, layer, ow_inst)) |
		((u64)_reg_read(REG_DISP_GOP_ADDR_H(inst, layer, ow_inst)) << 32);
}

/**
 * disp_gop_fb_set_cfg - setup fontbox
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @param cfg: fontbox configuration
 */
void disp_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct disp_gop_fb_cfg *fb_cfg)
{
	if (inst >= DISP_MAX_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 layer, no such layer(%d). ", __func__, layer);
		return;
	}

	if (fb_inst >= DISP_MAX_GOP_FB_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 gop_fb_inst, no such inst(%d). ", __func__, fb_inst);
		return;
	}

	if (fb_cfg->fb_ctrl.b.sample_rate > 3)
		fb_cfg->fb_ctrl.b.sample_rate = 3;
	if (fb_cfg->fb_ctrl.b.pix_thr > 31)
		fb_cfg->fb_ctrl.b.pix_thr = 31;

	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_GOP_FONTBOX_CFG(inst, layer, fb_inst), fb_cfg->fb_ctrl.raw);
	_reg_write(REG_DISP_GOP_FONTBOX_CFG(inst, layer, fb_inst), fb_cfg->init_st);

	disp_reg_set_shadow_mask(inst, false);
}

/**
 * disp_gop_fb_get_record - get fontbox's record
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @return: fontbox's record
 */
u32 disp_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst)
{
	if (inst >= DISP_MAX_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return -1;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 layer, no such layer(%d). ", __func__, layer);
		return -1;
	}

	if (fb_inst >= DISP_MAX_GOP_FB_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 gop_fb_inst, no such inst(%d). ", __func__, fb_inst);
		return -1;
	}

	return _reg_read(REG_DISP_GOP_FONTBOX_REC(inst, layer, fb_inst));
}

/**
 * disp_gop_odec_set_cfg_from_oenc - setup odec
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param layer: (0~2) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: odec_cfg configuration
 */
void disp_gop_odec_set_cfg_from_oenc(u8 inst, u8 layer, u8 oenc_inst, struct disp_gop_odec_cfg *odec_cfg)
{
	struct disp_oenc_cfg *oenc_cfg;

	if (inst >= DISP_MAX_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	if (layer >= DISP_MAX_GOP_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 2 inst, no such inst(%d). ", __func__, layer);
		return;
	}

	if (oenc_inst >= MAX_OSD_ENC_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 oenc_inst, no such inst(%d). ", __func__, oenc_inst);
		return;
	}

	oenc_cfg = disp_oenc_get_cfg(oenc_inst);
	if (oenc_cfg->bso_sz != 0) {
		_reg_write(REG_DISP_GOP_DEC_CTRL(inst, layer), odec_cfg->odec_ctrl.raw);
		_reg_write(REG_DISP_GOP_ADDR_L(inst, layer, odec_cfg->odec_ctrl.b.odec_attached_idx),
					oenc_cfg->bso_adr);
		_reg_write(REG_DISP_GOP_ADDR_H(inst, layer, odec_cfg->odec_ctrl.b.odec_attached_idx),
					oenc_cfg->bso_adr >> 32);
		_reg_write(REG_DISP_GOP_SIZE(inst, layer, odec_cfg->odec_ctrl.b.odec_attached_idx),
					(oenc_cfg->bso_mem_size.h << 16) | oenc_cfg->bso_mem_size.w);
	}
}

/**
 * disp_cover_set_cfg - setup cover
 *
 * @param inst: (0~1), the disp instance of gop which want to be configured.
 * @param cover_cfg: cover_cfg configuration
 */
void disp_cover_set_cfg(u8 inst, u8 cover_w_inst, struct disp_cover_cfg *cover_cfg)
{
	if (inst >= DISP_MAX_INST) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[bm-vip][disp] %s: only 0 ~ 1 disp_inst, no such inst(%d). ", __func__, inst);
		return;
	}

	_reg_write(REG_DISP_COVER_CFG(inst, cover_w_inst), cover_cfg->start.raw);
	_reg_write(REG_DISP_COVER_SIZE(inst, cover_w_inst),
				(cover_cfg->img_size.h << 16) | cover_cfg->img_size.w);
	_reg_write(REG_DISP_COVER_COLOR(inst, cover_w_inst), cover_cfg->color.raw);
}

/****************************************************************************
 * DISP
 ****************************************************************************/
/**
 * disp_reg_force_up - trigger reg update by sw.
 *
 */
void disp_reg_force_up(u8 inst)
{
	_reg_write_mask(REG_DISP_CFG(inst), BIT(16), BIT(16));
}

/**
 * disp_tgen_enable - enable timing-generator on disp.
 *
 * @param enable: AKA.
 * @return: tgen's enable status before change.
 */
bool disp_tgen_enable(u8 inst, bool enable)
{
	bool is_enable = (_reg_read(REG_DISP_CFG(inst)) & 0x80);

	if (is_enable != enable) {
		_reg_write_mask(REG_DISP_CFG(inst), 0x0080,
				enable ? 0x80 : 0x00);
		g_disp_cfg[inst].tgen_en = enable;
	}

	return is_enable;
}
EXPORT_SYMBOL_GPL(disp_tgen_enable);

/**
 * disp_check_tgen_enable - check whether disp timing-generator enable.
 *
 * @return: tgen's enable status.
 */
bool disp_check_tgen_enable(u8 inst)
{
	bool is_enable = (_reg_read(REG_DISP_CFG(inst)) & 0x80);

	return is_enable;
}
EXPORT_SYMBOL_GPL(disp_check_tgen_enable);

/**
 * disp_check_i80_enable - check whether mcu interface enable.
 *
 * @return: i80's enable status.
 */
bool disp_check_i80_enable(u8 inst)
{
	bool is_enable = (_reg_read(REG_VO_MAC_MCU_IF_CTRL(inst)) & 0x01);

	return is_enable;
}

/**
 * disp_set_bw_cfg - set disp's fifo configurations.
 *
 * @param cfg: disp's settings.
 * @param fmt: rdma fmt.
 */
void disp_set_bw_cfg(u8 inst, enum disp_format fmt)
{
	// To avoid bw fail
	if (fmt == DISP_FMT_RGB_PACKED || fmt == DISP_FMT_BGR_PACKED) {
		_reg_write(REG_DISP_LINE_BUFFER_SERIAL(inst), 0x1);
		_reg_write(REG_DISP_RD_TH_Y_MSB(inst), 0x1);
		_reg_write(REG_DISP_FIFO_TH(inst), 0x78);
	} else {
		_reg_write(REG_DISP_LINE_BUFFER_SERIAL(inst), 0x0);
		_reg_write(REG_DISP_RD_TH_Y_MSB(inst), 0x0);
		if(fmt == DISP_FMT_YUV420 || fmt == DISP_FMT_YUV422) {
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
void disp_set_cfg(u8 inst, struct disp_cfg *cfg)
{
	u32 tmp = 0;

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
		// 		cfg->burst << 28);

		disp_set_in_csc(inst, cfg->in_csc);
	} else {
		disp_reg_set_shadow_mask(inst, true);
		// csc only needed if disp from dram
		disp_set_out_mode(inst, DISP_OUT_DISABLE);
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
EXPORT_SYMBOL_GPL(disp_set_cfg);

/**
 * disp_get_cfg - get disp's cfg
 *
 * @return: disp's cfg
 */
struct disp_cfg *disp_get_cfg(u8 inst)
{
	return &g_disp_cfg[inst];
}
EXPORT_SYMBOL_GPL(disp_get_cfg);

/**
 * disp_cfg_setup_from_reg - get settings from register.
 *
 */
void disp_cfg_setup_from_reg(u8 inst)
{
	u32 tmp = 0;

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

#if 0
void _fill_disp_timing(struct disp_timing *timing, struct sync_info *sync_info)
{
	timing->vtotal = sync_info->vid_vsa_lines + sync_info->vid_vbp_lines
			+ sync_info->vid_active_lines + sync_info->vid_vfp_lines - 1;
	timing->htotal = sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels
			+ sync_info->vid_hline_pixels + sync_info->vid_hfp_pixels - 1;
	timing->vsync_start = 1;
	timing->vsync_end = timing->vsync_start + sync_info->vid_vsa_lines - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + sync_info->vid_vsa_lines + sync_info->vid_vbp_lines;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + sync_info->vid_active_lines - 1;
	timing->hsync_start = 1;
	timing->hsync_end = timing->hsync_start + sync_info->vid_hsa_pixels - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + sync_info->vid_hline_pixels - 1;
	timing->vsync_pol = sync_info->vid_vsa_pos_polarity;
	timing->hsync_pol = sync_info->vid_hsa_pos_polarity;
}
#endif
/**
 * disp_set_timing - modify disp's timing-generator.
 *
 * @param timing: new timing of disp.
 */
void disp_set_timing(u8 inst, struct disp_timing *timing)
{
	u32 tmp = 0;
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

	_reg_write_mask(REG_DISP_CFG(inst), 0x10000, 0x10000);
	_reg_write_mask(REG_DISP_CFG(inst), 0x10000, 0x10000);

	if (is_enable)
		disp_tgen_enable(inst, true);
	disp_timing[inst] = *timing;
}
EXPORT_SYMBOL_GPL(disp_set_timing);

struct disp_timing *disp_get_timing(u8 inst)
{
	return &disp_timing[inst];
}
EXPORT_SYMBOL_GPL(disp_get_timing);

void disp_get_hw_timing(u8 inst, struct disp_timing *timing)
{
	u32 tmp = 0;

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
EXPORT_SYMBOL_GPL(disp_get_hw_timing);

/**
 * disp_set_rect - setup rect(me) of disp
 *
 * @param rect: the pos/size of me, which should fit with disp's input.
 */
int disp_set_rect(u8 inst, struct disp_rect rect)
{
	if ((rect.y > disp_timing[inst].vfde_end) ||
	    (rect.x > disp_timing[inst].hfde_end) ||
	    ((disp_timing[inst].vfde_start + rect.y + rect.h - 1) >
	      disp_timing[inst].vfde_end) ||
	    ((disp_timing[inst].hfde_start + rect.x + rect.w - 1) >
	      disp_timing[inst].hfde_end)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi-vip][disp] %s: dev(%d) me's pos(%d, %d) size(%d, %d)\n",
				__func__, inst, rect.x, rect.y, rect.w, rect.h);
		CVI_TRACE_VO(CVI_DBG_ERR, " out of range(%d, %d, %d, %d).\n",
				disp_timing[inst].hfde_start, disp_timing[inst].vfde_start,
				disp_timing[inst].hfde_end, disp_timing[inst].vfde_end);
		return -EINVAL;
	}

	disp_timing[inst].vmde_start = rect.y + disp_timing[inst].vfde_start;
	disp_timing[inst].hmde_start = rect.x + disp_timing[inst].hfde_start;
	disp_timing[inst].vmde_end = disp_timing[inst].vmde_start + rect.h - 1;
	disp_timing[inst].hmde_end = disp_timing[inst].hmde_start + rect.w - 1;

	disp_reg_set_shadow_mask(inst, true);

	_reg_write(REG_DISP_HMDE(inst),
		   (disp_timing[inst].hmde_end << 16) | disp_timing[inst].hmde_start);
	_reg_write(REG_DISP_VMDE(inst),
		   (disp_timing[inst].vmde_end << 16) | disp_timing[inst].vmde_start);

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
void disp_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2)
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
void disp_set_mem(u8 inst, struct disp_mem *mem)
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
 * disp_set_csc - configure disp's input CSC's coefficient/offset
 *
 * @param cfg: The settings for CSC
 */
void disp_set_csc(u8 inst, struct disp_csc_matrix *cfg)
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
void disp_set_in_csc(u8 inst, enum disp_csc csc)
{
	if (csc == DISP_CSC_NONE) {
		_reg_write(REG_DISP_IN_CSC0(inst), 0);
	} else if (csc < DISP_CSC_MAX) {
		disp_set_csc(inst, &csc_mtrx[csc]);
	}

	g_disp_cfg[inst].in_csc = csc;
}

/**
 * disp_set_out_csc - setup disp's csc on output.
 *
 * @param csc: csc settings
 */
void disp_set_out_csc(u8 inst, enum disp_csc csc)
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
void disp_set_pattern(u8 inst, enum disp_pat_type type,
			   enum disp_pat_color color, const u16 *rgb)
{
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
		CVI_TRACE_VO(CVI_DBG_ERR, "%s - unacceptiable pattern-type(%d)\n", __func__, type);
		break;
	}
	_reg_write_mask(REG_DISP_CFG(inst), BIT(7), BIT(7));
}
EXPORT_SYMBOL_GPL(disp_set_pattern);

/**
 * disp_set_frame_bgcolor - setup disp frame(area outside mde)'s
 *				 background color.
 *
 * @param r: 10bit red value
 * @param g: 10bit green value
 * @param b: 10bit blue value
 */
void disp_set_frame_bgcolor(u8 inst, u16 r, u16 g, u16 b)
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
void disp_set_window_bgcolor(u8 inst, u16 r, u16 g, u16 b)
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
void disp_enable_window_bgcolor(u8 inst, bool enable)
{
	_reg_write_mask(REG_DISP_PAT_CFG(inst), 0x20, enable ? 0x20 : 0);
}

void disp_gamma_ctrl(u8 inst, bool enable, bool pre_osd)
{
	u32 value = 0;

	if (enable)
		value |= 0x04;
	if (pre_osd)
		value |= 0x08;
	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x0C, value);
}

void disp_gamma_lut_update(u8 inst, const u8 *b, const u8 *g, const u8 *r)
{
	u8 i;
	u32 value;

	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x03, 0x03);

	for (i = 0; i < DISP_GAMMA_NODE; ++i) {
		value = *(b + i) | (*(g + i) << 8) | (*(r + i) << 16)
			| (i << 24) | 0x80000000;
		_reg_write(REG_DISP_GAMMA_WR_LUT(inst), value);
	}

	_reg_write_mask(REG_DISP_GAMMA_CTRL(inst), 0x03, 0x00);
}

void disp_gamma_lut_read(u8 inst, struct disp_gamma_attr *gamma_attr)
{
	u8 i;
	u32 value;

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

void disp_lvdstx_set(u8 inst, union disp_lvdstx cfg)
{
	_reg_write(REG_VO_MAC_LVDSTX(inst), cfg.raw);
}
EXPORT_SYMBOL_GPL(disp_lvdstx_set);

void disp_lvdstx_get(u8 inst, union disp_lvdstx *cfg)
{
	cfg->raw = _reg_read(REG_VO_MAC_LVDSTX(inst));
}

void disp_bt656_72mhz_vo_mux_set(u8 inst)
{
	_reg_write(REG_VO_MAC_VO_MUX3(inst), 0x03000000);
	_reg_write(REG_VO_MAC_VO_MUX4(inst), 0x07060504);
	_reg_write(REG_VO_MAC_VO_MUX5(inst), 0x000A0908);
	_reg_write(REG_VO_MAC_BT_CFG(inst), 0x00000001);
}

void disp_bt_set(u8 inst, union disp_bt_enc enc, union disp_bt_sync_code sync)
{
	_reg_write(REG_VO_MAC_BT_ENC(inst), enc.raw);
	_reg_write(REG_VO_MAC_BT_SYNC_CODE(inst), sync.raw);
}

void disp_bt_get(u8 inst, union disp_bt_enc *enc, union disp_bt_sync_code *sync)
{
	enc->raw = _reg_read(REG_VO_MAC_BT_ENC(inst));
	sync->raw = _reg_read(REG_VO_MAC_BT_SYNC_CODE(inst));
}

/****************************************************************************
 * serial RGB
 ****************************************************************************/
void disp_set_srgb_ttl_en(u8 inst, bool enable)
{
	union  disp_srgb_ctrl vo_srgb_ctrl;

	vo_srgb_ctrl.raw = _reg_read(REG_VO_MAC_SRGB_CTRL(inst));
	vo_srgb_ctrl.b.srgb_ttl_en = enable;

	_reg_write(REG_VO_MAC_SRGB_CTRL(inst), vo_srgb_ctrl.raw);
}

void disp_set_srgb_ttl_4x(u8 inst, bool is_4x)
{
	union  disp_srgb_ctrl vo_srgb_ctrl;

	vo_srgb_ctrl.raw = _reg_read(REG_VO_MAC_SRGB_CTRL(inst));
	vo_srgb_ctrl.b.srgb_ttl_4t = is_4x;

	_reg_write(REG_VO_MAC_SRGB_CTRL(inst), vo_srgb_ctrl.raw);
}

void disp_set_intf(u8 inst, enum disp_vo_intf intf)
{
	bool data_en[5] = {true, true, true, true};

#if 0	// FIXME:
	void *pll_reg = ioremap(0x03002840, 4);
	u32 pll = ioread32(pll_reg) & 0xf5;

	if (intf == DISP_VO_INTF_DISABLE)
		iowrite32(pll, pll_reg);
	else
		iowrite32(pll | 0x0a, pll_reg);
	iounmap(pll_reg);
#endif
	dphy_init(inst, intf);

	if (intf == DISP_VO_INTF_DISABLE) {
		disp_set_vo_type_sel(inst, DISP_VO_SEL_DISABLE);
	} else if ((intf == DISP_VO_INTF_BT601) || (intf == DISP_VO_INTF_BT656) || (intf == DISP_VO_INTF_BT1120)) {
		if (intf == DISP_VO_INTF_BT601)
			disp_set_vo_type_sel(inst, DISP_VO_SEL_BT601);
		else if (intf == DISP_VO_INTF_BT656)
			disp_set_vo_type_sel(inst, DISP_VO_SEL_BT656);
		else if (intf == DISP_VO_INTF_BT1120)
			disp_set_vo_type_sel(inst, DISP_VO_SEL_BT1120);
		dphy_dsi_lane_en(inst, true, data_en, false);
	} else if (intf == DISP_VO_INTF_I80) {
		disp_set_vo_type_sel(inst, DISP_VO_SEL_I80);
		dphy_dsi_lane_en(inst, true, data_en, false);
		_reg_write_mask(REG_VO_MAC_MCU_IF_CTRL(inst), BIT(0), 1);
	} else if (intf == DISP_VO_INTF_I80_HW) {
		dphy_dsi_lane_en(inst, true, data_en, false);
		disp_set_vo_type_sel(inst, DISP_VO_SEL_HW_MCU);
	} else if (intf == DISP_VO_INTF_SW) {
		disp_set_vo_type_sel(inst, DISP_VO_SEL_SW);
	} else if (intf == DISP_VO_INTF_MIPI) {
		disp_set_vo_type_sel(inst, DISP_VO_SEL_DISABLE);
	} else if (intf == DISP_VO_INTF_LVDS) {
		disp_set_vo_type_sel(inst, DISP_VO_SEL_DISABLE);
	}
}
EXPORT_SYMBOL_GPL(disp_set_intf);

/**
 * disp_set_out_mode - Control display output's mode. CSC/Quantization/HSV
 *
 * @param inst: (0~1), the instance of display which want to be configured.
 * @param mode: csc/quant/hsv/none
 */
void disp_set_out_mode(u8 inst, enum disp_out_mode mode)
{
	u32 tmp = 0;

	switch (mode) {
	case DISP_OUT_CSC:
		tmp = BIT(0);
		break;
	case DISP_OUT_QUANT_BF16:
		tmp |= BIT(23);
		tmp |= BIT(1) | BIT(0);
		break;
	case DISP_OUT_QUANT:
		tmp |= BIT(1) | BIT(0);
		break;
	case DISP_OUT_HSV:
		tmp = BIT(4);
		break;
	case DISP_OUT_DISABLE:
		break;
	}

	//FIXME:
	// _reg_write_mask(REG_DISP_CSC_EN(inst), 0x01000013, tmp);

	// g_odma_cfg[inst].csc_cfg.mode = mode;
}

/**
 * disp_timing_setup_from_reg - get settings from register.
 *
 */
void disp_timing_setup_from_reg(u8 inst)
{
	u32 tmp = 0;

	tmp = _reg_read(REG_DISP_CFG(inst));
	disp_timing[inst].vsync_pol = (tmp & 0x20);
	disp_timing[inst].hsync_pol = (tmp & 0x40);

	tmp = _reg_read(REG_DISP_TOTAL(inst));
	disp_timing[inst].vtotal = tmp & 0xffff;
	disp_timing[inst].htotal = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_VSYNC(inst));
	disp_timing[inst].vsync_start = tmp & 0xffff;
	disp_timing[inst].vsync_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_VFDE(inst));
	disp_timing[inst].vfde_start = tmp & 0xffff;
	disp_timing[inst].vfde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_VMDE(inst));
	disp_timing[inst].vmde_start = tmp & 0xffff;
	disp_timing[inst].vmde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_HSYNC(inst));
	disp_timing[inst].hsync_start = tmp & 0xffff;
	disp_timing[inst].hsync_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_HFDE(inst));
	disp_timing[inst].hfde_start = tmp & 0xffff;
	disp_timing[inst].hfde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(REG_DISP_HMDE(inst));
	disp_timing[inst].hmde_start = tmp & 0xffff;
	disp_timing[inst].hmde_end = (tmp >> 16) & 0xffff;
}

void disp_checksum_en(u8 inst, bool enable)
{
	_reg_write_mask(REG_DISP_CHECKSUM0(inst), BIT(31),
					enable ? BIT(31) : 0);
}

void disp_get_checksum_status(u8 inst, struct disp_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(REG_DISP_CHECKSUM0(inst));
	status->axi_read_from_dram = _reg_read(REG_DISP_CHECKSUM1(inst));
	status->axi_read_from_gop = _reg_read(REG_DISP_CHECKSUM2(inst));

}

/**
 * dsi_get_mode - get current dsi mode
 *
 * @return: current dsi mode
 */
enum disp_dsi_mode dsi_get_mode(u8 inst)
{
	return (_reg_read(REG_DSI_MAC_EN(inst)) & 0x07);
}

/**
 * dsi_clr_mode - let dsi back to idle mode
 *
 */
void dsi_clr_mode(u8 inst)
{
	u32 mode = _reg_read(REG_DSI_MAC_EN(inst)) & 0x7;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "%s: mac_en reg(%#x)\n", __func__, mode);
	if (mode != DISP_DSI_MODE_IDLE)
		_reg_write_mask(REG_DSI_MAC_EN(inst), 0x7, mode);
}

/**
 * dsi_set_mode - set dsi mode
 *
 * @param mode: new dsi mode except for idle
 * @return: 0 if success
 */
int dsi_set_mode(u8 inst, enum disp_dsi_mode mode)
{
	if (mode >= DISP_DSI_MODE_MAX)
		return -1;

	if (mode == DISP_DSI_MODE_IDLE) {
		dsi_clr_mode(inst);
		return 0;
	}

	if (_reg_read(REG_DSI_MAC_EN(inst)))
		return -1;

	_reg_write_mask(REG_DSI_MAC_EN(inst), 0x7, mode);

	return 0;
}
EXPORT_SYMBOL_GPL(dsi_set_mode);

/**
 * dsi_chk_mode_done - check if dsi's work done.
 *
 * @param mode: the mode to check.
 * @return: 0 if success
 */
int dsi_chk_mode_done(u8 inst, enum disp_dsi_mode mode)
{
	u32 val = 0;

	if ((mode == DISP_DSI_MODE_ESC) || (mode == DISP_DSI_MODE_SPKT)) {
		val = _reg_read(REG_DSI_MAC_EN(inst)) & 0x30;
		return (val ^ (mode << 4)) ? -1 : 0;
	}

	if ((mode == DISP_DSI_MODE_IDLE) || (mode == DISP_DSI_MODE_HS)) {
		val = _reg_read(REG_DSI_MAC_EN(inst)) & 0x07;
		return (val == (mode)) ? 0 : -1;
	}

	return -1;
}
EXPORT_SYMBOL_GPL(dsi_chk_mode_done);

int _dsi_chk_and_clean_mode(u8 inst, enum disp_dsi_mode mode)
{
	int i, ret;

	for (i = 0; i < 5; ++i) {
		udelay(20);
		ret = dsi_chk_mode_done(inst, mode);
		if (ret == 0) {
			dsi_clr_mode(inst);
			break;
		}
	}
	return ret;
}


/**
 * dsi_long_packet_raw - send dsi long packet by escapet-lpdt.
 *
 * @param data: long packet data including header and crc.
 * @param count: number of long packet data.
 * @return: 0 if success
 */
int dsi_long_packet_raw(u8 inst, const u8 *data, u8 count)
{
	uintptr_t addr = REG_DSI_ESC_TX0(inst);
	u32 val = 0;
	u8 i = 0, packet_count, data_offset = 0;
	int ret;
	char str[128];

	CVI_TRACE_VO(CVI_DBG_DEBUG, "%s; count(%d)\n", __func__, count);
	while (count != 0) {
		if (count <= DISP_MAX_DSI_LP) {
			packet_count = count;
		} else if (count == DISP_MAX_DSI_LP + 1) {
			// [HW WorkAround] LPDT over can't take just one byte
			packet_count = DISP_MAX_DSI_LP - 1;
		} else {
			packet_count = DISP_MAX_DSI_LP;
		}
		count -= packet_count;
		val = 0x01 | ((packet_count - 1) << 8) | (count ? 0 : 0x10000);
		_reg_write(REG_DSI_ESC(inst), val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "%s: esc reg(%#x)\n", __func__, val);

		snprintf(str, 128, "%s: packet_count(%d) data(", __func__, packet_count);
		for (i = 0; i < packet_count; i += 4) {
			if (packet_count - i < 4) {
				val = 0;
				memcpy(&val, &data[data_offset], packet_count - i);
				data_offset += packet_count - i;
				_reg_write(addr + i, val);
				snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
				break;
			}
			memcpy(&val, &data[data_offset], 4);
			data_offset += 4;
			_reg_write(addr + i, val);
			snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
		}
		CVI_TRACE_VO(CVI_DBG_DEBUG, "%s)\n", str);

		dsi_set_mode(inst, DISP_DSI_MODE_ESC);
		ret = _dsi_chk_and_clean_mode(inst, DISP_DSI_MODE_ESC);
		if (ret != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "%s: packet_count(%d) data0(%#x)\n", __func__, packet_count, data[0]);
			break;
		}
	}
	return ret;
}

/**
 * dsi_long_packet - send dsi long packet by escapet-lpdt.
 *
 * @param di: data ID
 * @param data: long packet data
 * @param count: number of long packet data, 100 at most.
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: 0 if success
 */
int dsi_long_packet(u8 inst, u8 di, const u8 *data, u8 count, bool sw_mode)
{
	u8 packet[128] = {di, count & 0xff, count >> 8, 0};
	u16 crc;

	if (count > 128 - 6) {
		CVI_TRACE_VO(CVI_DBG_ERR, "%s: count(%d) invalid\n", __func__, count);
		return -1;
	}

	packet[3] = ecc(packet);
	memcpy(&packet[4], data, count);
	count += 4;
	crc = crc16(packet, count);
	packet[count++] = crc & 0xff;
	packet[count++] = crc >> 8;

	if (!sw_mode)
		return dsi_long_packet_raw(inst, packet, count);

	dpyh_mipi_tx_manual_packet(inst, packet, count);
	return 0;
}

/**
 * dsi_short_packet - send dsi short packet by escapet-lpdt.
 *   *NOTE*: ecc is hw-generated.
 *
 * @param di: data ID
 * @param data: short packet data
 * @param count: number of short packet data, 1 or 2.
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: 0 if success
 */
int dsi_short_packet(u8 inst, u8 di, const u8 *data, u8 count, bool sw_mode)
{
	u32 val = 0;

	if ((count > DISP_MAX_DSI_SP) || (count == 0))
		return -1;

	val = di;
	if (count == 2) {
		//val = 0x15;
		val |= (data[0] << 8) | (data[1] << 16);
	} else {
		//val = 0x05;
		val |= data[0] << 8;
	}

	CVI_TRACE_VO(CVI_DBG_DEBUG, "%s: dev(%d) val(0x%x)\n", __func__, inst, val);

	if (!sw_mode) {
		_reg_write_mask(REG_DSI_HS_0(inst), 0x00ffffff, val);
		dsi_set_mode(inst, DISP_DSI_MODE_SPKT);
		return _dsi_chk_and_clean_mode(inst, DISP_DSI_MODE_SPKT);
	}
	val |= (ecc((u8 *)&val) << 24);
	dpyh_mipi_tx_manual_packet(inst, (u8 *)&val, 4);
	return 0;
}

/**
 * dsi_dcs_write_buffer - send dsi packet by escapet-lpdt.
 *
 * @param di: data ID
 * @param data: packet data
 * @param count: number of packet data
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: Zero on success or a negative error code on failure.
 */
int dsi_dcs_write_buffer(u8 inst, u8 di, const void *data, size_t len, bool sw_mode)
{
	if (len == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: 0 param unacceptable.\n", __func__);
		return -1;
	}

	if ((di == 0x06) || (di == 0x05) || (di == 0x04) || (di == 0x03)) {
		if (len != 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: cmd(0x%02x) should has 1 param.\n", __func__, di);
			return -1;
		}
		return dsi_short_packet(inst, di, data, len, sw_mode);
	}
	if ((di == 0x15) || (di == 0x37) || (di == 0x13) || (di == 0x14) || (di == 0x23)) {
		if (len != 2) {
			CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: cmd(0x%02x) should has 2 param.\n", __func__, di);
			return -1;
		}
		return dsi_short_packet(inst, di, data, len, sw_mode);
	}
	if ((di == 0x29) || (di == 0x39))
		return dsi_long_packet(inst, di, data, len, sw_mode);

	return dsi_long_packet(inst, di, data, len, sw_mode);
}
EXPORT_SYMBOL_GPL(dsi_dcs_write_buffer);

#define ACK_WR       0x02
#define GEN_READ_LP  0x1A
#define GEN_READ_SP1 0x11
#define GEN_READ_SP2 0x12
#define DCS_READ_LP  0x1C
#define DCS_READ_SP1 0x21
#define DCS_READ_SP2 0x22

int dsi_dcs_read_buffer(u8 inst, u8 di, const u16 data_param, u8 *data, size_t len)
{
	int ret = 0;
	u32 rx_data;
	int i = 0;

	if (len > 4)
		len = 4;

	if (dsi_get_mode(inst) == DISP_DSI_MODE_HS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: not work in HS.\n", __func__);
		return -1;
	}

	// [2:0] reg_esc_mode
	// [7:4] reg_esc_trig
	// [11:8] reg_tx_bc
	// [15:12] reg_bta_rx_bc
	// [16:16] reg_tx_bc_over: TX LPDT transfer over,0: Extend to next trigger,1: Transfer over in this trigger
	// only set necessery bits
	_reg_write_mask(REG_DSI_ESC(inst), 0x07, 0x04);

	// send read cmd
	dsi_short_packet(inst, di, (u8 *)&data_param, 2, false);

	// goto BTA
	dsi_set_mode(inst, DISP_DSI_MODE_ESC);
	if (_dsi_chk_and_clean_mode(inst, DISP_DSI_MODE_ESC) != 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: BTA error.\n", __func__);
		return ret;
	}

	// check result
	rx_data = _reg_read(REG_DSI_ESC_RX0(inst));
	switch (rx_data & 0xff) {
	case GEN_READ_SP1:
	case DCS_READ_SP1:
		data[0] = (rx_data >> 8) & 0xff;
		break;
	case GEN_READ_SP2:
	case DCS_READ_SP2:
		data[0] = (rx_data >> 8) & 0xff;
		data[1] = (rx_data >> 16) & 0xff;
		break;
	case GEN_READ_LP:
	case DCS_READ_LP:
		rx_data = _reg_read(REG_DSI_ESC_RX1(inst));
		for (i = 0; i < len; ++i)
			data[i] = (rx_data >> (i * 8)) & 0xff;
		break;
	case ACK_WR:
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: dcs read, ack with error(%#x %#x).\n"
			, __func__, (rx_data >> 8) & 0xff, (rx_data >> 16) & 0xff);
		ret = -1;
		break;
	default:
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_mipi_tx] %s: unknown DT, %#x.", __func__, rx_data);
		ret = -1;
		break;
	}

	// CVI_TRACE_VO(CVI_DBG_DEBUG, "%s: %#x %#x\n", __func__, rx_data0, rx_data1);
	return ret;
}
EXPORT_SYMBOL_GPL(dsi_dcs_read_buffer);

int disp_dsi_config(u8 inst, u8 lane_num, enum disp_dsi_fmt fmt, u16 width)
{
	u32 val = 0;
	u8 bit_depth[] = {24, 18, 16, 30};

	if ((lane_num != 1) && (lane_num != 2) && (lane_num != 4))
		return -EINVAL;
	if (fmt > DISP_DSI_FMT_MAX)
		return -EINVAL;

	lane_num >>= 1;
	val = (fmt << 30) | (lane_num << 24);
	_reg_write_mask(REG_DSI_HS_0(inst), 0xc3000000, val);
	val = (width / 10) << 16 | UPPER(width * bit_depth[fmt], 3);
	_reg_write(REG_DSI_HS_1(inst), val);

	return 0;
}
EXPORT_SYMBOL_GPL(disp_dsi_config);

void i80_sw_mode(u8 inst, bool enable)
{
	_reg_write_mask(REG_VO_MAC_MCU_IF_CTRL(inst), BIT(11) | BIT(1), enable ? 0x802 : 0x000);

	if (enable) {
		disp_tgen_enable(inst, true);
		mdelay(40);
		disp_tgen_enable(inst, false);
	}
}

void i80_packet(u8 inst, u32 cmd)
{
	u8 cnt = 0;

	_reg_write(REG_VO_MAC_MCU_SW_CTRL(inst), cmd);
	_reg_write_mask(REG_VO_MAC_MCU_SW_CTRL(inst), BIT(31), BIT(31));

	do {
		udelay(1);
		if (_reg_read(REG_VO_MAC_MCU_SW_CTRL(inst)) & BIT(24))
			break;
	} while (++cnt < 10);

	if (cnt == 10)
		CVI_TRACE_VO(CVI_DBG_ERR, "[cvi_vip] %s: cmd(%#x) not ready.\n", __func__, cmd);
}

bool i80_chk_idle(u8 inst)
{
#define I80_BUSY_WAITING_TIMES 50
#define I80_CLEAR_TRAIL 3
	u8 cnt = 0, i = 0;

	for (i = 0; i < I80_CLEAR_TRAIL; ++i) {
		do {
			if (_reg_read(REG_VO_MAC_MCU_STATUS(inst)) == 0x08)
				return true;
			usleep_range(1 * 1000, 3 * 1000);
		} while (++cnt < I80_BUSY_WAITING_TIMES);

		CVI_TRACE_VO(CVI_DBG_ERR, "%s: waiting idle failed. sw try to clear it.\n", __func__);
		_reg_write_mask(REG_VO_MAC_MCU_IF_CTRL(inst), BIT(10), BIT(10));
	}

	CVI_TRACE_VO(CVI_DBG_ERR, "%s: failed.\n", __func__);
	return false;
}

void i80_run(u8 inst)
{
	if (!i80_chk_idle(inst))
		return; //do nothing if i80 status not idle

	_reg_write_mask(REG_VO_MAC_MCU_IF_CTRL(inst), BIT(11), BIT(11));

	i80_chk_idle(inst); //wait i80 idle
}

void disp_set_i80_if(u8 inst, u8 sw_mode, enum disp_hw_mcu_format format)
{
	union  disp_mcu_if_ctrl mcu_if_ctrl;
	union  disp_hw_mcu_auto hw_mcu_auto;

	mcu_if_ctrl.raw = _reg_read(REG_VO_MAC_MCU_IF_CTRL(inst));
	hw_mcu_auto.raw = _reg_read(REG_VO_MAC_HW_MCU_AUTO(inst));

	if (sw_mode) {
		//for sw mcu mode
		mcu_if_ctrl.b.i80_if_en = false;
		mcu_if_ctrl.b.i80_hw_if_en = false;
		mcu_if_ctrl.b.i80_sw_mode_en = true;
	} else {
		//hw mcu
		mcu_if_ctrl.b.i80_if_en = false;
		mcu_if_ctrl.b.i80_sw_mode_en = false;
		mcu_if_ctrl.b.i80_hw_if_en = true;
		if (I80_HW_FORMAT_RGB565 == format) {
			hw_mcu_auto.b.mcu_565 = true;
		} else if (I80_HW_FORMAT_RGB888 == format) {
			hw_mcu_auto.b.mcu_565 = false;
		}
	}

	_reg_write(REG_VO_MAC_MCU_IF_CTRL(inst), mcu_if_ctrl.raw);
	_reg_write(REG_VO_MAC_HW_MCU_AUTO(inst), hw_mcu_auto.raw);
}

#if 0
void i80_hw_packet(u8 inst, u32 cmd)
{
	_reg_write(REG_VO_MAC_HW_MCU_CMD0(inst), cmd);

	//_reg_write_mask(REG_VO_MAC_HW_MCU_AUTO, BIT(0), 0);//rising edge to trig
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(4), BIT(4)); //sw_tx_num=3
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(5), BIT(5));
}
#endif

void i80_hw_set_cmd0(u8 inst, u32 cmd)
{
	_reg_write(REG_VO_MAC_HW_MCU_CMD0(inst), cmd);
}

void i80_hw_set_cmd1(u8 inst, u32 cmd)
{
	cmd = (_reg_read(REG_VO_MAC_HW_MCU_CMD0(inst)) | cmd << 16);
	_reg_write(REG_VO_MAC_HW_MCU_CMD0(inst), cmd);
}

void i80_hw_set_cmd2(u8 inst, u32 cmd)
{
	_reg_write(REG_VO_MAC_HW_MCU_CMD1(inst), cmd);
}

void i80_hw_set_cmd3(u8 inst, u32 cmd)
{
	cmd = (_reg_read(REG_VO_MAC_HW_MCU_CMD1(inst)) | cmd << 16);
	_reg_write(REG_VO_MAC_HW_MCU_CMD1(inst), cmd);
}

int i80_hw_set_cmd_cnt(u8 inst, u32 cmdcnt)
{
	if (cmdcnt > 16) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[i80] %s: hw  mcu cmd takes max 16 cmds but (%d) set.\n", __func__, cmdcnt);
		return -1;
	}
	cmdcnt = ((cmdcnt & 0xF) - 1) << 4;
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), 0xF0, cmdcnt);

	return 0;
}

void i80_set_pre_cmd_en(u8 inst)
{
	// 1 : insert cmd before H.W. MCU data send
	// use reg_sw_tx related config, if this bit set to high
	// a. please set reg_sw_tx related config
	// b. set hfde_str number > reg_sw_tx_num
	_reg_write_mask(REG_VO_MAC_HW_MCU_AUTO(inst), BIT(5), BIT(5));
}

int i80_hw_stop(u8 inst)
{
	int cnt = 0;

	//rising edge to stop
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(1), 0);
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(1), BIT(1));
	cnt = 0;
	do {
		udelay(1);
		//check hw_mcu_flag
		if (_reg_read(REG_VO_MAC_HW_MCU_CMD(inst)) & BIT(7)) {
			_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(7),  BIT(7));
			return 0;
		}
	} while (++cnt < 10);
	return -1;
}

// TODO:
int i80_hw_trig(u8 inst)
{
	int cnt = 0;

	i80_hw_stop(inst);
	//rising edge to trig
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(0), 0);
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(0), BIT(0));
	do {
		udelay(1);
		//check hw_mcu_flag
		if (_reg_read(REG_VO_MAC_HW_MCU_CMD(inst)) & BIT(6)) {
			break;
		}
	} while (++cnt < 10);

	cnt = 0;
	do {
		udelay(1);
		if (_reg_read(REG_VO_MAC_HW_MCU_CMD(inst)) & BIT(3)) {
			//check sw_tx_done
			break;
		}
	} while (++cnt < 10);

	if (cnt == 10) {
		CVI_TRACE_VO(CVI_DBG_ERR, "[I80] %s: hw  mcu cmd not ready.\n", __func__);
		return -1;
	}
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(6),  BIT(6));
	return 0;
}

void i80_set_trig_stop(u8 inst)
{
	//set MCU stop
	_reg_write_mask(REG_VO_MAC_HW_MCU_AUTO(inst), BIT(1), BIT(1));
	_reg_write_mask(REG_VO_MAC_HW_MCU_AUTO(inst), BIT(0), 0);

}

#if 0
void disp_clk_set(void)
{
	// disp0 clk ctrl
	_reg_write(REG_VO_SYS_CLK_CTRL0, 0x10);
	// disp1 clk ctrl
	_reg_write(REG_VO_SYS_CLK_CTRL1, 0x20002);
}
#endif

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
	union disp_online_odma_intr_sel online_odma_mask;
	bool disp_from_sc = false;
	unsigned int i = 0, j = 0;

	memset(&intr_mask, 0, sizeof(intr_mask));

	// disp_clk_set(); // TODO: is it need?
	if (!is_resume) {
		// init variables
		memset(&g_odma_cfg, 0, sizeof(g_odma_cfg));
		memset(&g_disp_cfg, 0, sizeof(g_disp_cfg));
		memset(&disp_timing, 0, sizeof(disp_timing));

		// init disp mask up lock
		spin_lock_init(&disp_mask_spinlock);

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
			//burst length = (burst+1)*16 bytes
			//display burst length keep 128 bytes
			// g_disp_cfg[i].burst = DISP_DEFAULT_BURST;
			// g_disp_cfg[i].y_thresh = DISP_DEFAULT_Y_THRESH;
			// g_disp_cfg[i].c_thresh = DISP_DEFAULT_C_THRESH;
			//display osd burst length set to 256 bytes for short latency
			for (j = 0; j < DISP_MAX_GOP_INST; ++j)
				g_disp_cfg[i].gop_cfg[j].gop_ctrl.b.burst = DISP_DEFAULT_BURST;
			_reg_write(REG_DSI_MAC_EN(i), 0x00);
		}
	}

	for (i = 0; i < DISP_MAX_INST; ++i) {
		// get current hw-timings
		disp_timing_setup_from_reg(i);
		online_odma_mask.b.disp_online_frame_end = false; //true means disable
		online_odma_mask.b.disp_odma_frame_end = true; //true means disable
		intr_mask.b.disp_frame_end = true; //this true means enable
		disp_set_intr_mask(i, intr_mask);
		disp_set_odma_intr_mask(i, online_odma_mask);
	}
	if (!gp_reg)
		gp_reg = ioremap(GP_REG, 4);
	if (!retrain_reg)
		retrain_reg = ioremap(RETRAIN_REG, 4);
}

/**
 * ctrl_set_disp_src - setup input-src of disp.
 *
 * @param disp_from_sc: true(from sc_0); false(from mem)
 * @return: 0 if success
 */
int ctrl_set_disp_src(u8 inst, bool disp_from_sc)
{
	g_disp_cfg[inst].disp_from_sc = disp_from_sc;
	disp_set_cfg(inst, &g_disp_cfg[inst]);

	return 0;
}

bool ddr_need_retrain(void)
{
	if (retrain_reg)
		return ioread32(retrain_reg) & 0xF;
	else
		return false;
}

void trigger_8051(void)
{
	if (gp_reg && retrain_reg) {
		iowrite32(ioread32(gp_reg) | 0x10000, gp_reg);
		iowrite32(ioread32(retrain_reg) & ~0xF, retrain_reg);
	}
}

void dump_disp_register(u8 inst)
{
	uintptr_t disp_base = REG_DISP_BASE(inst);

	CVI_TRACE_VO(CVI_DBG_DEBUG, "disp inst(%d) base address=0x%08lx\n", inst, disp_base);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_00=0x%08x\n", _reg_read(disp_base + DISP_REG_00));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_01=0x%08x\n", _reg_read(disp_base + DISP_REG_01));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_02=0x%08x\n", _reg_read(disp_base + DISP_REG_02));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_03=0x%08x\n", _reg_read(disp_base + DISP_REG_03));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_04=0x%08x\n", _reg_read(disp_base + DISP_REG_04));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_05=0x%08x\n", _reg_read(disp_base + DISP_REG_05));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_06=0x%08x\n", _reg_read(disp_base + DISP_REG_06));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_07=0x%08x\n", _reg_read(disp_base + DISP_REG_07));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_08=0x%08x\n", _reg_read(disp_base + DISP_REG_08));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_12=0x%08x\n", _reg_read(disp_base + DISP_REG_12));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_13=0x%08x\n", _reg_read(disp_base + DISP_REG_13));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_14=0x%08x\n", _reg_read(disp_base + DISP_REG_14));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_15=0x%08x\n", _reg_read(disp_base + DISP_REG_15));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_16=0x%08x\n", _reg_read(disp_base + DISP_REG_16));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_17=0x%08x\n", _reg_read(disp_base + DISP_REG_17));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_18=0x%08x\n", _reg_read(disp_base + DISP_REG_18));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_19=0x%08x\n", _reg_read(disp_base + DISP_REG_19));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_20=0x%08x\n", _reg_read(disp_base + DISP_REG_20));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_21=0x%08x\n", _reg_read(disp_base + DISP_REG_21));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_22=0x%08x\n", _reg_read(disp_base + DISP_REG_22));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_23=0x%08x\n", _reg_read(disp_base + DISP_REG_23));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_24=0x%08x\n", _reg_read(disp_base + DISP_REG_24));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_25=0x%08x\n", _reg_read(disp_base + DISP_REG_25));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_26=0x%08x\n", _reg_read(disp_base + DISP_REG_26));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_27=0x%08x\n", _reg_read(disp_base + DISP_REG_27));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_28=0x%08x\n", _reg_read(disp_base + DISP_REG_28));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_29=0x%08x\n", _reg_read(disp_base + DISP_REG_29));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_30=0x%08x\n", _reg_read(disp_base + DISP_REG_30));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_31=0x%08x\n", _reg_read(disp_base + DISP_REG_31));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_32=0x%08x\n", _reg_read(disp_base + DISP_REG_32));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_33=0x%08x\n", _reg_read(disp_base + DISP_REG_33));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_34=0x%08x\n", _reg_read(disp_base + DISP_REG_34));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_35=0x%08x\n", _reg_read(disp_base + DISP_REG_35));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_36=0x%08x\n", _reg_read(disp_base + DISP_REG_36));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_37=0x%08x\n", _reg_read(disp_base + DISP_REG_37));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_38=0x%08x\n", _reg_read(disp_base + DISP_REG_38));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_39=0x%08x\n", _reg_read(disp_base + DISP_REG_39));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_40=0x%08x\n", _reg_read(disp_base + DISP_REG_40));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_41=0x%08x\n", _reg_read(disp_base + DISP_REG_41));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_42=0x%08x\n", _reg_read(disp_base + DISP_REG_42));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_43=0x%08x\n", _reg_read(disp_base + DISP_REG_43));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_AXI_ST=0x%08x\n", _reg_read(disp_base + DISP_REG_AXI_ST));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_CATCH=0x%08x\n", _reg_read(disp_base + DISP_REG_CATCH));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_CHK_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_CHK_CTRL));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_CHK_RD_OSD=0x%08x\n", _reg_read(disp_base + DISP_CHK_RD_OSD));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_CHK_RD_IMG=0x%08x\n", _reg_read(disp_base + DISP_CHK_RD_IMG));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_3E=0x%08x\n", _reg_read(disp_base + DISP_REG_3E));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_3F=0x%08x\n", _reg_read(disp_base + DISP_REG_3F));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_IMG_BWL=0x%08x\n", _reg_read(disp_base + DISP_REG_IMG_BWL));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_GAMMA_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_GAMMA_CTRL));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_GAMMA_WR_LUT=0x%08x\n", _reg_read(disp_base + DISP_REG_GAMMA_WR_LUT));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_GAMMA_RD_LUT=0x%08x\n", _reg_read(disp_base + DISP_REG_GAMMA_RD_LUT));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_MCU_IF_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_MCU_IF_CTRL));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_MCU_SW_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_MCU_SW_CTRL));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_MCU_STATUS=0x%08x\n", _reg_read(disp_base + DISP_REG_MCU_STATUS));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_AUTO=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_AUTO));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_0=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_0));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_1=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_1));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_2=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_2));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_3=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_3));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_4=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_4));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_5=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_5));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_6=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_6));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_7=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_7));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_OV=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_OV));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_SRGB_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_SRGB_CTRL));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_TGEN_LITE_SIZE=0x%08x\n", _reg_read(disp_base + DISP_REG_TGEN_LITE_SIZE));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_TGEN_LITE_VS=0x%08x\n", _reg_read(disp_base + DISP_REG_TGEN_LITE_VS));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_REG_TGEN_LITE_HS=0x%08x\n", _reg_read(disp_base + DISP_REG_TGEN_LITE_HS));
}

void disp_dump_gop_register(int inst)
{
	u32 val, i, j;
	uintptr_t gop_base = REG_DISP_GOP_BASE(inst);

	if (inst >= DISP_MAX_INST)
		CVI_TRACE_VO(CVI_DBG_DEBUG, "sc(%d) out of range\n", inst);

	for (i = 0; i < DISP_MAX_GOP_INST; i++) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "disp(%d) gop%d\n", inst, i);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "========================= GOP%d enable =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_80);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_80=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow0_en=%d, reg_ow1_en=%d, reg_ow2_en=%d, reg_ow3_en=%d\n",
				(val & DISP_VGOP_OW0_EN_MASK) >> DISP_VGOP_OW0_EN_OFFSET,
				(val & DISP_VGOP_OW1_EN_MASK) >> DISP_VGOP_OW1_EN_OFFSET,
				(val & DISP_VGOP_OW2_EN_MASK) >> DISP_VGOP_OW2_EN_OFFSET,
				(val & DISP_VGOP_OW3_EN_MASK) >> DISP_VGOP_OW3_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow4_en=%d, reg_ow5_en=%d, reg_ow6_en=%d, reg_ow7_en=%d\n",
				(val & DISP_VGOP_OW0_EN_MASK) >> DISP_VGOP_OW4_EN_OFFSET,
				(val & DISP_VGOP_OW1_EN_MASK) >> DISP_VGOP_OW5_EN_OFFSET,
				(val & DISP_VGOP_OW2_EN_MASK) >> DISP_VGOP_OW6_EN_OFFSET,
				(val & DISP_VGOP_OW3_EN_MASK) >> DISP_VGOP_OW3_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_vgop_hscal=%d, reg_vgop_vscal=%d, reg_clr_key_en=%d\n",
				(val & DISP_VGOP_VGOP_HSCAL_MASK) >> DISP_VGOP_VGOP_HSCAL_OFFSET,
				(val & DISP_VGOP_VGOP_VSCAL_MASK) >> DISP_VGOP_VGOP_VSCAL_OFFSET,
				(val & DISP_VGOP_CLR_KEY_EN_MASK) >> DISP_VGOP_CLR_KEY_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_vgop_arlen=%d, reg_vgop_sw_rst=%d\n",
				(val & DISP_VGOP_VGOP_ARLEN_MASK) >> DISP_VGOP_VGOP_ARLEN_OFFSET,
				(val & DISP_VGOP_VGOP_SW_RST_MASK) >> DISP_VGOP_VGOP_SW_RST_OFFSET);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "========================= GOP%d OW =========================\n", i);
		for (j = 0; j < DISP_MAX_GOP_OW_INST; j++) {
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_0 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_0(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_format=0x%02x\n", j,
					(val & DISP_VGOP_OW0_FORMAT_MASK) >> DISP_VGOP_OW0_FORMAT_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_1 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_1(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_h_start=%d, reg_ow%d_h_end=%d\n",
					j, (val & DISP_VGOP_OW0_H_START_MASK) >> DISP_VGOP_OW0_H_START_OFFSET,
					j, (val & DISP_VGOP_OW0_H_END_MASK) >> DISP_VGOP_OW0_H_END_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_2 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_2(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_v_start=%d, reg_ow%d_v_end=%d\n",
					j, (val & DISP_VGOP_OW0_V_START_MASK) >> DISP_VGOP_OW0_V_START_OFFSET,
					j, (val & DISP_VGOP_OW0_V_END_MASK) >> DISP_VGOP_OW0_V_END_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_3 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_3(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_dram_str_adr_l = 0x%08x\n", j,
					(val &
					DISP_VGOP_OW0_DRAM_STR_ADR_L_MASK) >> DISP_VGOP_OW0_DRAM_STR_ADR_L_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_4 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_4(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_dram_str_adr_h = 0x%08x\n", j,
					(val &
					DISP_VGOP_OW0_DRAM_STR_ADR_H_MASK) >> DISP_VGOP_OW0_DRAM_STR_ADR_H_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_5 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_5(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_dram_strip = %d, reg_ow%d_crop_pixels = %d\n",
					j,
					(val & DISP_VGOP_OW0_DRAM_STRIP_MASK) >> (DISP_VGOP_OW0_DRAM_STRIP_OFFSET - 4),
					j, (val & DISP_VGOP_OW0_CROP_PIXELS_MASK) >> DISP_VGOP_OW0_CROP_PIXELS_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_6 + j * 0x20);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_6(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow%d_dram_hsize = %d, reg_ow%d_dram_vsize = %d\n",
					j,
					(val & DISP_VGOP_OW0_DRAM_HSIZE_MASK) >> (DISP_VGOP_OW0_DRAM_HSIZE_OFFSET - 4),
					j, (val & DISP_VGOP_OW0_DRAM_VSIZE_MASK) >> DISP_VGOP_OW0_DRAM_VSIZE_OFFSET);
		}

		CVI_TRACE_VO(CVI_DBG_DEBUG, "========================= GOP%d Ctrl =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_83);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_83=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_clr_key=0x%08x\n",
				(val & DISP_VGOP_CLR_KEY_MASK) >> DISP_VGOP_CLR_KEY_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_84);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_84=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_const_argb0=0x%04x, reg_const_argb1=0x%04x\n",
				(val & DISP_VGOP_CONST_ARGB0_MASK) >> DISP_VGOP_CONST_ARGB0_OFFSET,
				(val & DISP_VGOP_CONST_ARGB1_MASK) >> DISP_VGOP_CONST_ARGB1_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_85);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_85=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_vgop_debug=0x%08x\n",
				(val & DISP_VGOP_VGOP_DEBUG_MASK) >> DISP_VGOP_VGOP_DEBUG_OFFSET);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "========================= GOP%d FB =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_86);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_86=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb_clr_hi_thr=%d, reg_fb_clr_lo_thr=%d\n",
				(val & DISP_VGOP_FB_CLR_HI_THR_MASK) >> DISP_VGOP_FB_CLR_HI_THR_OFFSET,
				(val & DISP_VGOP_FB_CLR_LO_THR_MASK) >> DISP_VGOP_FB_CLR_LO_THR_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb_init=%d, reg_fb_font_is_dark=%d\n",
				(val & DISP_VGOP_FB_INIT_MASK) >> DISP_VGOP_FB_INIT_OFFSET,
				(val & DISP_VGOP_FB_FONT_IS_DARK_MASK) >> DISP_VGOP_FB_FONT_IS_DARK_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb_diff_fnum=%d\n",
				(val & DISP_VGOP_FB_DIFF_FNUM_MASK) >> DISP_VGOP_FB_DIFF_FNUM_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_87);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_87=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb0_width=%d, reg_fb0_pix_thr=%d\n",
				(val & DISP_VGOP_FB0_WIDTH_MASK) >> DISP_VGOP_FB0_WIDTH_OFFSET,
				(val & DISP_VGOP_FB0_PIX_THR_MASK) >> DISP_VGOP_FB0_PIX_THR_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb0_sample_rate=%d, reg_fb0_num=%d\n",
				(val & DISP_VGOP_FB0_SAMPLE_RATE_MASK) >> DISP_VGOP_FB0_SAMPLE_RATE_OFFSET,
				(val & DISP_VGOP_FB0_NUM_MASK) >> DISP_VGOP_FB0_NUM_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb0_attached_idx=%d, reg_fb0_en=%d\n",
				(val & DISP_VGOP_FB0_ATTACHED_IDX_MASK) >> DISP_VGOP_FB0_ATTACHED_IDX_OFFSET,
				(val & DISP_VGOP_FB0_EN_MASK) >> DISP_VGOP_FB0_EN_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_88);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_88=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb0_init_st=0x%08x\n",
				(val & DISP_VGOP_FB0_INIT_ST_MASK) >> DISP_VGOP_FB0_INIT_ST_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_89);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_89=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb0_record=0x%08x\n",
				(val & DISP_VGOP_FB0_RECORD_MASK) >> DISP_VGOP_FB0_RECORD_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_90);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_90=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb1_width=%d, reg_fb1_pix_thr=%d\n",
				(val & DISP_VGOP_FB1_WIDTH_MASK) >> DISP_VGOP_FB1_WIDTH_OFFSET,
				(val & DISP_VGOP_FB1_PIX_THR_MASK) >> DISP_VGOP_FB1_PIX_THR_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb1_sample_rate=%d, reg_fb1_num=%d\n",
				(val & DISP_VGOP_FB1_SAMPLE_RATE_MASK) >> DISP_VGOP_FB1_SAMPLE_RATE_OFFSET,
				(val & DISP_VGOP_FB1_NUM_MASK) >> DISP_VGOP_FB1_NUM_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb1_attached_idx=%d, reg_fb1_en=%d\n",
				(val & DISP_VGOP_FB1_ATTACHED_IDX_MASK) >> DISP_VGOP_FB1_ATTACHED_IDX_OFFSET,
				(val & DISP_VGOP_FB1_EN_MASK) >> DISP_VGOP_FB1_EN_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_91);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_91=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb1_init_st=0x%08x\n",
				(val & DISP_VGOP_FB1_INIT_ST_MASK) >> DISP_VGOP_FB1_INIT_ST_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_92);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_92=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_fb1_record=0x%08x\n",
				(val & DISP_VGOP_FB1_RECORD_MASK) >> DISP_VGOP_FB1_RECORD_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_BW_LIMIT);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_BW_LIMIT=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_bwl_win=%d, reg_bwl_vld=%d, reg_bwl_en=%d\n",
				(val & DISP_VGOP_BWL_WIN_MASK) >> DISP_VGOP_BWL_WIN_OFFSET,
				(val & DISP_VGOP_BWL_VLD_MASK) >> DISP_VGOP_BWL_VLD_OFFSET,
				(val & DISP_VGOP_BWL_EN_MASK) >> DISP_VGOP_BWL_EN_OFFSET);

		//ODEC
		CVI_TRACE_VO(CVI_DBG_DEBUG, "========================= GOP%d ODEC =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_DEC_00);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_DEC_00=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG,
				"    reg_odec_en=%d, reg_odec_int_en=%d, reg_odec_int_clr=%d, reg_odec_wdt_en=%d\n",
				(val & DISP_VGOP_ODEC_EN_MASK) >> DISP_VGOP_ODEC_EN_OFFSET,
				(val & DISP_VGOP_ODEC_INT_EN_MASK) >> DISP_VGOP_ODEC_INT_EN_OFFSET,
				(val & DISP_VGOP_ODEC_INT_CLR_MASK) >> DISP_VGOP_ODEC_INT_CLR_OFFSET,
				(val & DISP_VGOP_ODEC_WDT_EN_MASK) >> DISP_VGOP_ODEC_WDT_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG,
				"    reg_odec_dbg_ridx=0x%04x, reg_odec_done=%d, reg_odec_attached_idx=%d\n",
				(val & DISP_VGOP_ODEC_DBG_RIDX_MASK) >> DISP_VGOP_ODEC_DBG_RIDX_OFFSET,
				(val & DISP_VGOP_ODEC_DONE_MASK) >> DISP_VGOP_ODEC_DONE_OFFSET,
				(val & DISP_VGOP_ODEC_ATTACHED_IDX_MASK) >> DISP_VGOP_ODEC_ATTACHED_IDX_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_odec_wdt_fdiv_bit=%d, reg_odec_int_vec=0x%08x\n",
				(val & DISP_VGOP_ODEC_INT_VEC_MASK) >> DISP_VGOP_ODEC_INT_VEC_OFFSET,
				(val & DISP_VGOP_ODEC_WDT_FDIV_BIT_MASK) >> DISP_VGOP_ODEC_WDT_FDIV_BIT_OFFSET);

		//ODEC_DBG
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_DEC_01);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_DEC_01=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_odec_dbg_rdata=0x%08x\n",
				(val & DISP_VGOP_ODEC_DBG_RDATA_MASK) >> DISP_VGOP_ODEC_DBG_RDATA_OFFSET);

		//LUT-4
		CVI_TRACE_VO(CVI_DBG_DEBUG, "========================= GOP%d LUT16 =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_0);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_0=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt0=0x%04x, reg_lut16_cnt1=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT0_MASK) >> DISP_VGOP_LUT16_CNT0_OFFSET,
				(val & DISP_VGOP_LUT16_CNT1_MASK) >> DISP_VGOP_LUT16_CNT1_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_1);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_1=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt2=0x%04x, reg_lut16_cnt3=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT2_MASK) >> DISP_VGOP_LUT16_CNT2_OFFSET,
				(val & DISP_VGOP_LUT16_CNT3_MASK) >> DISP_VGOP_LUT16_CNT3_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_2);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_2=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt4=0x%04x, reg_lut16_cnt5=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT4_MASK) >> DISP_VGOP_LUT16_CNT4_OFFSET,
				(val & DISP_VGOP_LUT16_CNT5_MASK) >> DISP_VGOP_LUT16_CNT5_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_3);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_3=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt6=0x%04x, reg_lut16_cnt7=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT6_MASK) >> DISP_VGOP_LUT16_CNT6_OFFSET,
				(val & DISP_VGOP_LUT16_CNT7_MASK) >> DISP_VGOP_LUT16_CNT7_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_4);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_4=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt8=0x%04x, reg_lut16_cnt9=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT8_MASK) >> DISP_VGOP_LUT16_CNT8_OFFSET,
				(val & DISP_VGOP_LUT16_CNT9_MASK) >> DISP_VGOP_LUT16_CNT9_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_5);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_5=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt10=0x%04x, reg_lut16_cnt11=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT10_MASK) >> DISP_VGOP_LUT16_CNT10_OFFSET,
				(val & DISP_VGOP_LUT16_CNT11_MASK) >> DISP_VGOP_LUT16_CNT11_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_6);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_6=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt12=0x%04x, reg_lut16_cnt13=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT12_MASK) >> DISP_VGOP_LUT16_CNT12_OFFSET,
				(val & DISP_VGOP_LUT16_CNT13_MASK) >> DISP_VGOP_LUT16_CNT13_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_LUT16_7);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_LUT16_7=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_lut16_cnt14=0x%04x, reg_lut16_cnt15=0x%04x\n",
				(val & DISP_VGOP_LUT16_CNT14_MASK) >> DISP_VGOP_LUT16_CNT14_OFFSET,
				(val & DISP_VGOP_LUT16_CNT15_MASK) >> DISP_VGOP_LUT16_CNT15_OFFSET);
	}
}

void disp_show_gop_status(int inst)
{
	u32 val, i;
	uintptr_t gop_base = REG_DISP_GOP_BASE(inst);

	if (inst >= DISP_MAX_INST)
		CVI_TRACE_VO(CVI_DBG_DEBUG, "sc(%d) out of range\n", inst);

	for (i = 0; i < DISP_MAX_GOP_INST; i++) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "sc(%d) gop%d\n", inst, i);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_REG_80);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_REG_80=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow0_en=%d, reg_ow1_en=%d, reg_ow2_en=%d, reg_ow3_en=%d\n",
				(val & DISP_VGOP_OW0_EN_MASK) >> DISP_VGOP_OW0_EN_OFFSET,
				(val & DISP_VGOP_OW1_EN_MASK) >> DISP_VGOP_OW1_EN_OFFSET,
				(val & DISP_VGOP_OW2_EN_MASK) >> DISP_VGOP_OW2_EN_OFFSET,
				(val & DISP_VGOP_OW3_EN_MASK) >> DISP_VGOP_OW3_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_ow4_en=%d, reg_ow5_en=%d, reg_ow6_en=%d, reg_ow7_en=%d\n",
				(val & DISP_VGOP_OW0_EN_MASK) >> DISP_VGOP_OW4_EN_OFFSET,
				(val & DISP_VGOP_OW1_EN_MASK) >> DISP_VGOP_OW5_EN_OFFSET,
				(val & DISP_VGOP_OW2_EN_MASK) >> DISP_VGOP_OW6_EN_OFFSET,
				(val & DISP_VGOP_OW3_EN_MASK) >> DISP_VGOP_OW3_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_vgop_hscal=%d, reg_vgop_vscal=%d, reg_clr_key_en=%d\n",
				(val & DISP_VGOP_VGOP_HSCAL_MASK) >> DISP_VGOP_VGOP_HSCAL_OFFSET,
				(val & DISP_VGOP_VGOP_VSCAL_MASK) >> DISP_VGOP_VGOP_VSCAL_OFFSET,
				(val & DISP_VGOP_CLR_KEY_EN_MASK) >> DISP_VGOP_CLR_KEY_EN_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_vgop_arlen=%d, reg_vgop_sw_rst=%d\n",
				(val & DISP_VGOP_VGOP_ARLEN_MASK) >> DISP_VGOP_VGOP_ARLEN_OFFSET,
				(val & DISP_VGOP_VGOP_SW_RST_MASK) >> DISP_VGOP_VGOP_SW_RST_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + DISP_VGOP_DEC_00);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "DISP_VGOP_DEC_00=0x%08x\n", val);
		CVI_TRACE_VO(CVI_DBG_DEBUG,
				"    reg_odec_en=%d, reg_odec_int_en=%d, reg_odec_int_clr=%d, reg_odec_wdt_en=%d\n",
				(val & DISP_VGOP_OW0_EN_MASK) >> DISP_VGOP_ODEC_EN_OFFSET,
				(val & DISP_VGOP_ODEC_INT_EN_MASK) >> DISP_VGOP_ODEC_INT_EN_OFFSET,
				(val & DISP_VGOP_ODEC_INT_CLR_MASK) >> DISP_VGOP_ODEC_INT_CLR_OFFSET,
				(val & DISP_VGOP_ODEC_WDT_EN_MASK) >> DISP_VGOP_OW2_EN_MASK);
		CVI_TRACE_VO(CVI_DBG_DEBUG,
				"    reg_odec_dbg_ridx=0x%04x, reg_odec_done=%d, reg_odec_attached_idx=%d\n",
				(val & DISP_VGOP_ODEC_DBG_RIDX_MASK) >> DISP_VGOP_ODEC_DBG_RIDX_OFFSET,
				(val & DISP_VGOP_ODEC_DONE_MASK) >> DISP_VGOP_ODEC_DONE_OFFSET,
				(val & DISP_VGOP_ODEC_ATTACHED_IDX_MASK) >> DISP_VGOP_ODEC_ATTACHED_IDX_OFFSET);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "    reg_odec_wdt_fdiv_bit=%d, reg_odec_int_vec=0x%08x\n",
				(val & DISP_VGOP_ODEC_INT_VEC_MASK) >> DISP_VGOP_ODEC_INT_VEC_OFFSET,
				(val & DISP_VGOP_ODEC_WDT_FDIV_BIT_MASK) >> DISP_VGOP_ODEC_WDT_FDIV_BIT_OFFSET);
	}
}

void dump_disp_odma_register(u8 inst)
{
	CVI_TRACE_VO(CVI_DBG_DEBUG, "*** dump_disp(%d)_register ***\n", inst);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_CFG(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_CFG(inst)));

	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_Y_L(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_Y_L(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_Y_H(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_Y_H(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_U_L(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_U_L(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_U_H(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_U_H(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_V_L(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_V_L(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_V_H(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_V_H(inst)));

	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_PITCH_Y(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_PITCH_Y(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_PITCH_C(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_PITCH_C(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_X_STR(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_X_STR(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_Y_STR(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_Y_STR(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_WIDETH(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_WIDETH(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_HEIGHT(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_HEIGHT(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_DEBUG(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_DEBUG(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_LINE_TARGET(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_LINE_TARGET(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_CYCLE_LINE(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_CYCLE_LINE(inst)));
	_reg_write(REG_DISP_ODMA_LATCH_LINE(inst), 0x1);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "VODEV(%d) REG_DISP_ODMA_LATCH_LINE(inst)=0x%08x\n", inst, _reg_read(REG_DISP_ODMA_LATCH_LINE(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_FIFO_CFG(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_FIFO_CFG(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_FIFO_WH(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_FIFO_WH(inst)));
	CVI_TRACE_VO(CVI_DBG_DEBUG, "REG_DISP_ODMA_FIFO_LMT(inst)=0x%08x\n", _reg_read(REG_DISP_ODMA_FIFO_LMT(inst)));
}



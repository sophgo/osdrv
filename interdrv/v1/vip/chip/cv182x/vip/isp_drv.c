/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_drv.c
 * Description: isp registers access control
 */

#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "system_common.h"
#include "timer.h"
#elif defined(ENV_HOSTPC)
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/delay.h>
#endif

#include "vip_common.h"
#include "reg.h"
#include "../uapi/isp_reg.h"
#include "../cvi_debug.h"
#include "isp_drv.h"
#include "../cvi_debug.h"

#define REG_ARRAY_UPDATE2_SIZE(addr, array, size)		\
	do {							\
		uint16_t i;					\
		for (i = 0; i < size; i += 2) {			\
			val = array[i];				\
			if ((i + 1) < size)			\
				val |= (array[i+1] << 16);	\
			_reg_write(addr + (i << 1), val);	\
		}						\
	} while (0)

#define REG_ARRAY_UPDATE2(addr, array)				\
	REG_ARRAY_UPDATE2_SIZE(addr, array, ARRAY_SIZE(array))

#define REG_ARRAY_UPDATE4(addr, array)				\
	do {							\
		uint16_t i;					\
		for (i = 0; i < ARRAY_SIZE(array); i += 4) {	\
			val = array[i];				\
			if ((i + 1) < ARRAY_SIZE(array))	\
				val |= (array[i+1] << 8);	\
			if ((i + 2) < ARRAY_SIZE(array))	\
				val |= (array[i+2] << 16);	\
			if ((i + 3) < ARRAY_SIZE(array))	\
				val |= (array[i+3] << 24);	\
			_reg_write(addr + i, val);		\
		}						\
	} while (0)

#define LTM_REG_ARRAY_UPDATE11(addr, array)                                   \
	do {                                                                  \
		uint32_t val;                                                 \
		val = array[0] | (array[1] << 5) | (array[2] << 10) |         \
		      (array[3] << 15) | (array[4] << 20) | (array[5] << 25); \
		_reg_write(addr, val);                                        \
		val = array[6] | (array[7] << 5) | (array[8] << 10) |         \
		      (array[9] << 15) | (array[10] << 20);                   \
		_reg_write(addr + 4, val);                                    \
	} while (0)

#define LTM_REG_ARRAY_UPDATE30(addr, array)                                   \
	do {                                                                  \
		uint8_t i, j;                                                 \
		uint32_t val;                                                 \
		for (i = 0, j = 0; i < ARRAY_SIZE(array); i += 6, j++) {      \
			val = array[i] | (array[i + 1] << 5) |                \
			      (array[i + 2] << 10) | (array[i + 3] << 15) |   \
			      (array[i + 4] << 20) | (array[i + 5] << 25);    \
			_reg_write(addr + j * 4, val);                        \
		}                                                             \
	} while (0)

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t reg_base;

#define LUMA_MAP_W_BIT 4
#define LUMA_MAP_H_BIT 4

#define MANR_W_BIT 3
#define MANR_H_BIT 3

static uint8_t g_w_bit[ISP_PRERAW_MAX], g_h_bit[ISP_PRERAW_MAX];
static uint8_t g_rgbmap_chg_pre[ISP_PRERAW_MAX][2];

/****************************************************************************
 * LMAP_CONFIG
 ****************************************************************************/
struct lmap_cfg {
	u8 pre_chg[2]; //le/se
	u8 pre_w_bit;
	u8 pre_h_bit;
	u8 post_w_bit;
	u8 post_h_bit;
};

struct lmap_cfg g_lmp_cfg[ISP_PRERAW_MAX];

/****************************************************************************
 * FBC_CONFIG
 ****************************************************************************/
static uint8_t g_y_cr[ISP_PRERAW_MAX] = {25, 25}, g_uv_cr[ISP_PRERAW_MAX] = {25, 25};

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void isp_set_base_addr(void *base)
{
	uintptr_t *addr = isp_get_phys_reg_bases();
	int i = 0;

	for (i = 0; i < ISP_BLK_ID_MAX; ++i) {
		addr[i] -= reg_base;
		addr[i] += (uintptr_t)base;
	}
	reg_base = (uintptr_t)base;
}

uintptr_t *isp_get_phys_reg_bases(void)
{
	static uintptr_t m_isp_phys_base_list[ISP_BLK_ID_MAX] = {
		[ISP_BLK_ID_PRE_RAW_BE]		= (ISP_BLK_BA_PRE_RAW_BE),
		[ISP_BLK_ID_CROP4]		= (ISP_BLK_BA_CROP4),
		[ISP_BLK_ID_CROP5]		= (ISP_BLK_BA_CROP5),
		[ISP_BLK_ID_BLC4]		= (ISP_BLK_BA_BLC4),
		[ISP_BLK_ID_BLC5]		= (ISP_BLK_BA_BLC5),
		[ISP_BLK_ID_FPN]		= (ISP_BLK_BA_FPN),
		[ISP_BLK_ID_IR_PRE_PROC_LE]	= (ISP_BLK_BA_IR_PRE_PROC_LE),
		[ISP_BLK_ID_IR_PRE_PROC_SE]	= (ISP_BLK_BA_IR_PRE_PROC_SE),
		[ISP_BLK_ID_IR_PROC]		= (ISP_BLK_BA_IR_PROC),
		[ISP_BLK_ID_IR_AE0]		= (ISP_BLK_BA_IR_AE0),
		[ISP_BLK_ID_IR_AE1]		= (ISP_BLK_BA_IR_AE1),
		[ISP_BLK_ID_AEHIST0]		= (ISP_BLK_BA_AEHIST0),
		[ISP_BLK_ID_AEHIST1]		= (ISP_BLK_BA_AEHIST1),
		[ISP_BLK_ID_AWB0]		= (ISP_BLK_BA_AWB0),
		[ISP_BLK_ID_AWB1]		= (ISP_BLK_BA_AWB1),
		[ISP_BLK_ID_GMS]		= (ISP_BLK_BA_GMS),
		[ISP_BLK_ID_AF]			= (ISP_BLK_BA_AF),
		[ISP_BLK_ID_WBG0]		= (ISP_BLK_BA_WBG0),
		[ISP_BLK_ID_WBG1]		= (ISP_BLK_BA_WBG1),
		[ISP_BLK_ID_DPC0]		= (ISP_BLK_BA_DPC0),
		[ISP_BLK_ID_DPC1]		= (ISP_BLK_BA_DPC1),
		[ISP_BLK_ID_INV_WBG0]		= (ISP_BLK_BA_INV_WBG0),
		[ISP_BLK_ID_INV_WBG1]		= (ISP_BLK_BA_INV_WBG1),
		[ISP_BLK_ID_PCHK4]		= (ISP_BLK_BA_PCHK4),
		[ISP_BLK_ID_PCHK5]		= (ISP_BLK_BA_PCHK5),
		[ISP_BLK_ID_LSCR4]		= (ISP_BLK_BA_LSCR4),
		[ISP_BLK_ID_LSCR5]		= (ISP_BLK_BA_LSCR5),
		[ISP_BLK_ID_PRE_RAW_FE0]	= (ISP_BLK_BA_PRE_RAW_FE0),
		[ISP_BLK_ID_CSIBDG0]		= (ISP_BLK_BA_CSIBDG0),
		[ISP_BLK_ID_CROP0]		= (ISP_BLK_BA_CROP0),
		[ISP_BLK_ID_CROP1]		= (ISP_BLK_BA_CROP1),
		[ISP_BLK_ID_BLC0]		= (ISP_BLK_BA_BLC0),
		[ISP_BLK_ID_BLC1]		= (ISP_BLK_BA_BLC1),
		[ISP_BLK_ID_LMP0]		= (ISP_BLK_BA_LMP0),
		[ISP_BLK_ID_WBG11]		= (ISP_BLK_BA_WBG11),
		[ISP_BLK_ID_LMP1]		= (ISP_BLK_BA_LMP1),
		[ISP_BLK_ID_WBG12]		= (ISP_BLK_BA_WBG12),
		[ISP_BLK_ID_RGBMAP0]		= (ISP_BLK_BA_RGBMAP0),
		[ISP_BLK_ID_WBG7]		= (ISP_BLK_BA_WBG7),
		[ISP_BLK_ID_RGBMAP1]		= (ISP_BLK_BA_RGBMAP1),
		[ISP_BLK_ID_WBG8]		= (ISP_BLK_BA_WBG8),
		[ISP_BLK_ID_PCHK0]		= (ISP_BLK_BA_PCHK0),
		[ISP_BLK_ID_PCHK1]		= (ISP_BLK_BA_PCHK1),
		[ISP_BLK_ID_LSCR0]		= (ISP_BLK_BA_LSCR0),
		[ISP_BLK_ID_LSCR1]		= (ISP_BLK_BA_LSCR1),
		[ISP_BLK_ID_PRE_RAW_FE1]	= (ISP_BLK_BA_PRE_RAW_FE1),
		[ISP_BLK_ID_CSIBDG1_R1]		= (ISP_BLK_BA_CSIBDG1_R1),
		[ISP_BLK_ID_CROP2]		= (ISP_BLK_BA_CROP2),
		[ISP_BLK_ID_CROP3]		= (ISP_BLK_BA_CROP3),
		[ISP_BLK_ID_BLC2]		= (ISP_BLK_BA_BLC2),
		[ISP_BLK_ID_BLC3]		= (ISP_BLK_BA_BLC3),
		[ISP_BLK_ID_LMP2]		= (ISP_BLK_BA_LMP2),
		[ISP_BLK_ID_WBG13]		= (ISP_BLK_BA_WBG13),
		[ISP_BLK_ID_LMP3]		= (ISP_BLK_BA_LMP3),
		[ISP_BLK_ID_WBG14]		= (ISP_BLK_BA_WBG14),
		[ISP_BLK_ID_RGBMAP2]		= (ISP_BLK_BA_RGBMAP2),
		[ISP_BLK_ID_WBG9]		= (ISP_BLK_BA_WBG9),
		[ISP_BLK_ID_RGBMAP3]		= (ISP_BLK_BA_RGBMAP3),
		[ISP_BLK_ID_WBG10]		= (ISP_BLK_BA_WBG10),
		[ISP_BLK_ID_PCHK2]		= (ISP_BLK_BA_PCHK2),
		[ISP_BLK_ID_PCHK3]		= (ISP_BLK_BA_PCHK3),
		[ISP_BLK_ID_LSCR2]		= (ISP_BLK_BA_LSCR2),
		[ISP_BLK_ID_LSCR3]		= (ISP_BLK_BA_LSCR3),
		[ISP_BLK_ID_RAWTOP]		= (ISP_BLK_BA_RAWTOP),
		[ISP_BLK_ID_CFA]		= (ISP_BLK_BA_CFA),
		[ISP_BLK_ID_BNR]		= (ISP_BLK_BA_BNR),
		[ISP_BLK_ID_CROP6]		= (ISP_BLK_BA_CROP6),
		[ISP_BLK_ID_CROP7]		= (ISP_BLK_BA_CROP7),
		[ISP_BLK_ID_PCHK6]		= (ISP_BLK_BA_PCHK6),
		[ISP_BLK_ID_PCHK7]		= (ISP_BLK_BA_PCHK7),
		[ISP_BLK_ID_RGBTOP]		= (ISP_BLK_BA_RGBTOP),
		[ISP_BLK_ID_LSCM0]		= (ISP_BLK_BA_LSCM0),
		[ISP_BLK_ID_CCM0]		= (ISP_BLK_BA_CCM0),
		[ISP_BLK_ID_CCM1]		= (ISP_BLK_BA_CCM1),
		[ISP_BLK_ID_CCM2]		= (ISP_BLK_BA_CCM2),
		[ISP_BLK_ID_CCM3]		= (ISP_BLK_BA_CCM3),
		[ISP_BLK_ID_CCM4]		= (ISP_BLK_BA_CCM4),
		[ISP_BLK_ID_MANR]		= (ISP_BLK_BA_MANR),
		[ISP_BLK_ID_GAMMA]		= (ISP_BLK_BA_GAMMA),
		[ISP_BLK_ID_CLUT]		= (ISP_BLK_BA_CLUT),
		[ISP_BLK_ID_DHZ]		= (ISP_BLK_BA_DHZ),
		[ISP_BLK_ID_R2Y4]		= (ISP_BLK_BA_R2Y4),
		[ISP_BLK_ID_RGBDITHER]		= (ISP_BLK_BA_RGBDITHER),
		[ISP_BLK_ID_PREYEE]		= (ISP_BLK_BA_PREYEE),
		[ISP_BLK_ID_PCHK8]		= (ISP_BLK_BA_PCHK8),
		[ISP_BLK_ID_PCHK9]		= (ISP_BLK_BA_PCHK9),
		[ISP_BLK_ID_DCI]		= (ISP_BLK_BA_DCI),
		[ISP_BLK_ID_HIST_EDGE_V]	= (ISP_BLK_BA_HIST_EDGE_V),
		[ISP_BLK_ID_HDRFUSION]		= (ISP_BLK_BA_HDRFUSION),
		[ISP_BLK_ID_HDRLTM]		= (ISP_BLK_BA_HDRLTM),
		[ISP_BLK_ID_AWB2]		= (ISP_BLK_BA_AWB2),
		[ISP_BLK_ID_YUVTOP]		= (ISP_BLK_BA_YUVTOP),
		[ISP_BLK_ID_444422]		= (ISP_BLK_BA_444422),
		[ISP_BLK_ID_FBCE]		= (ISP_BLK_BA_FBCE),
		[ISP_BLK_ID_FBCD]		= (ISP_BLK_BA_FBCD),
		[ISP_BLK_ID_YUVDITHER]		= (ISP_BLK_BA_YUVDITHER),
		[ISP_BLK_ID_YNR]		= (ISP_BLK_BA_YNR),
		[ISP_BLK_ID_CNR]		= (ISP_BLK_BA_CNR),
		[ISP_BLK_ID_EE]			= (ISP_BLK_BA_EE),
		[ISP_BLK_ID_YCURVE]		= (ISP_BLK_BA_YCURVE),
		[ISP_BLK_ID_CROP8]		= (ISP_BLK_BA_CROP8),
		[ISP_BLK_ID_CROP9]		= (ISP_BLK_BA_CROP9),
		[ISP_BLK_ID_PCHK10]		= (ISP_BLK_BA_PCHK10),
		[ISP_BLK_ID_PCHK11]		= (ISP_BLK_BA_PCHK11),
		[ISP_BLK_ID_ISPTOP]		= (ISP_BLK_BA_ISPTOP),
		[ISP_BLK_ID_CSIBDG_LITE]	= (ISP_BLK_BA_CSIBDG_LITE),
		[ISP_BLK_ID_WDMA_COM]		= (ISP_BLK_BA_WDMA_COM),
		[ISP_BLK_ID_WDMA0]		= (ISP_BLK_BA_WDMA0),
		[ISP_BLK_ID_WDMA1]		= (ISP_BLK_BA_WDMA1),
		[ISP_BLK_ID_WDMA2]		= (ISP_BLK_BA_WDMA2),
		[ISP_BLK_ID_WDMA3]		= (ISP_BLK_BA_WDMA3),
		[ISP_BLK_ID_WDMA4]		= (ISP_BLK_BA_WDMA4),
		[ISP_BLK_ID_WDMA5]		= (ISP_BLK_BA_WDMA5),
		[ISP_BLK_ID_WDMA6]		= (ISP_BLK_BA_WDMA6),
		[ISP_BLK_ID_WDMA7]		= (ISP_BLK_BA_WDMA7),
		[ISP_BLK_ID_WDMA8]		= (ISP_BLK_BA_WDMA8),
		[ISP_BLK_ID_WDMA9]		= (ISP_BLK_BA_WDMA9),
		[ISP_BLK_ID_WDMA10]		= (ISP_BLK_BA_WDMA10),
		[ISP_BLK_ID_WDMA11]		= (ISP_BLK_BA_WDMA11),
		[ISP_BLK_ID_WDMA12]		= (ISP_BLK_BA_WDMA12),
		[ISP_BLK_ID_WDMA13]		= (ISP_BLK_BA_WDMA13),
		[ISP_BLK_ID_WDMA14]		= (ISP_BLK_BA_WDMA14),
		[ISP_BLK_ID_WDMA15]		= (ISP_BLK_BA_WDMA15),
		[ISP_BLK_ID_WDMA16]		= (ISP_BLK_BA_WDMA16),
		[ISP_BLK_ID_WDMA17]		= (ISP_BLK_BA_WDMA17),
		[ISP_BLK_ID_WDMA18]		= (ISP_BLK_BA_WDMA18),
		[ISP_BLK_ID_WDMA19]		= (ISP_BLK_BA_WDMA19),
		[ISP_BLK_ID_WDMA20]		= (ISP_BLK_BA_WDMA20),
		[ISP_BLK_ID_WDMA21]		= (ISP_BLK_BA_WDMA21),
		[ISP_BLK_ID_WDMA22]		= (ISP_BLK_BA_WDMA22),
		[ISP_BLK_ID_WDMA23]		= (ISP_BLK_BA_WDMA23),
		[ISP_BLK_ID_WDMA24]		= (ISP_BLK_BA_WDMA24),
		[ISP_BLK_ID_WDMA25]		= (ISP_BLK_BA_WDMA25),
		[ISP_BLK_ID_WDMA26]		= (ISP_BLK_BA_WDMA26),
		[ISP_BLK_ID_WDMA27]		= (ISP_BLK_BA_WDMA27),
		[ISP_BLK_ID_WDMA28]		= (ISP_BLK_BA_WDMA28),
		[ISP_BLK_ID_WDMA29]		= (ISP_BLK_BA_WDMA29),
		[ISP_BLK_ID_WDMA30]		= (ISP_BLK_BA_WDMA30),
		[ISP_BLK_ID_WDMA31]		= (ISP_BLK_BA_WDMA31),
		[ISP_BLK_ID_RDMA_COM]		= (ISP_BLK_BA_RDMA_COM),
		[ISP_BLK_ID_RDMA0]		= (ISP_BLK_BA_RDMA0),
		[ISP_BLK_ID_RDMA1]		= (ISP_BLK_BA_RDMA1),
		[ISP_BLK_ID_RDMA2]		= (ISP_BLK_BA_RDMA2),
		[ISP_BLK_ID_RDMA3]		= (ISP_BLK_BA_RDMA3),
		[ISP_BLK_ID_RDMA4]		= (ISP_BLK_BA_RDMA4),
		[ISP_BLK_ID_RDMA5]		= (ISP_BLK_BA_RDMA5),
		[ISP_BLK_ID_RDMA6]		= (ISP_BLK_BA_RDMA6),
		[ISP_BLK_ID_RDMA7]		= (ISP_BLK_BA_RDMA7),
		[ISP_BLK_ID_RDMA8]		= (ISP_BLK_BA_RDMA8),
		[ISP_BLK_ID_RDMA9]		= (ISP_BLK_BA_RDMA9),
		[ISP_BLK_ID_RDMA10]		= (ISP_BLK_BA_RDMA10),
		[ISP_BLK_ID_RDMA11]		= (ISP_BLK_BA_RDMA11),
		[ISP_BLK_ID_RDMA12]		= (ISP_BLK_BA_RDMA12),
		[ISP_BLK_ID_RDMA13]		= (ISP_BLK_BA_RDMA13),
		[ISP_BLK_ID_RDMA14]		= (ISP_BLK_BA_RDMA14),
		[ISP_BLK_ID_RDMA15]		= (ISP_BLK_BA_RDMA15),
		[ISP_BLK_ID_RDMA16]		= (ISP_BLK_BA_RDMA16),
		[ISP_BLK_ID_RDMA17]		= (ISP_BLK_BA_RDMA17),
		[ISP_BLK_ID_RDMA18]		= (ISP_BLK_BA_RDMA18),
		[ISP_BLK_ID_CMDQ1]		= (ISP_BLK_BA_CMDQ1),
		[ISP_BLK_ID_CMDQ2]		= (ISP_BLK_BA_CMDQ2),
		[ISP_BLK_ID_CMDQ3]		= (ISP_BLK_BA_CMDQ3),
	};
	return m_isp_phys_base_list;
}

/*********************************************************/
u8 ca_y_lut[] = {
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};

u8 cp_y_lut[] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

u8 cp_u_lut[] = {
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
};

u8 cp_v_lut[] = {
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
};
/**********************************************************/

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_be_online(struct isp_ctx *ctx)
{
	if (!ctx->is_offline_be && ctx->is_offline_postraw) //fe->be->dram->post
		return 1;
	return 0;
}

u32 _is_be_post_online(struct isp_ctx *ctx)
{
	if (ctx->is_offline_be && !ctx->is_offline_postraw) //fe->dram->be->post
		return 1;
	return 0;
}

u32 _is_all_online(struct isp_ctx *ctx)
{
	if (!ctx->is_offline_be && !ctx->is_offline_postraw)
		return 1;
	return 0;
}

u32 _is_post_sclr_online(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler)
		return 1;
	return 0;
}
/**********************************************************/

void isp_debug_dump(struct isp_ctx *ctx)
{
	uintptr_t fe0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	uintptr_t fe1 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	uintptr_t be  = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t isptop = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t wdma_com = ctx->phys_regs[ISP_BLK_ID_WDMA_COM];
	uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_COM];
	uintptr_t tdnr = ctx->phys_regs[ISP_BLK_ID_444422];

	u32 axi0_r = 0, axi0_w = 0;
	u32 errflag = 0, rgb_cnt = 0, yuv_cnt = 0;
	u32 raw_lexp_cnt = 0, be_lexp_cnt = 0;
	u32 rdmi_en = 0, rdmi_valid = 0, rdmi_ready = 0;
	u32 wdmi_en = 0, wdmi_valid = 0, wdmi_ready = 0;
	u32 rdma_idle = 0, wdma_idle = 0;
	u32 fe0_pchk0_valid = 0, fe0_pchk0_ready = 0;
	u32 fe0_pchk1_valid = 0, fe0_pchk1_ready = 0;
	u32 fe1_pchk0_valid = 0, fe1_pchk0_ready = 0;
	u32 fe1_pchk1_valid = 0, fe1_pchk1_ready = 0;
	u32 be_pchk0_valid = 0, be_pchk0_ready = 0;
	u32 be_pchk1_valid = 0, be_pchk1_ready = 0;
	u32 raw_valid = 0, raw_ready = 0;
	u32 rgb_valid = 0, rgb_ready = 0;
	u32 yuv_valid = 0, yuv_ready = 0;
	u32 tdnr_hs = 0;
	u32 tdnr_cnt = 0;
	u32 lb_ctrl_cnt = 0;

	u32 i = 0;

	//AXI
	ISP_WR_BITS(isptop, REG_ISP_TOP_T, REG_15, DBUS_SEL, 4);
	axi0_w = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_10); //0x0A070040
	axi0_r = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_12); //0x0A070048

	//Subblock counter
	ISP_WR_BITS(isptop, REG_ISP_TOP_T, REG_15, DBUS_SEL, 0);
	errflag = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_10); //0x40
	rgb_cnt = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_11); //0x44
	yuv_cnt = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_12); //0x48

	//Subblock counter
	ISP_WR_BITS(isptop, REG_ISP_TOP_T, REG_15, DBUS_SEL, 1);
	raw_lexp_cnt = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_10); //0x40
	be_lexp_cnt = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_12); //0x48

	//RDMA handshake
	ISP_WR_BITS(isptop, REG_ISP_TOP_T, REG_15, DBUS_SEL, 2);
	rdmi_en = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_10); //0x40
	rdmi_valid = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_11); //0x44
	rdmi_ready = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_12); //0x48

	//WDMA handshake
	ISP_WR_BITS(isptop, REG_ISP_TOP_T, REG_15, DBUS_SEL, 3);
	wdmi_en = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_10); //0x40
	wdmi_valid = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_11); //0x44
	wdmi_ready = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_12); //0x48

	//RDMA status
	rdma_idle = ISP_RD_REG(rdma_com, REG_ISP_RDMA_COM_T, NORM_STATUS1); //0x14

	//WDMA status
	wdma_idle = ISP_RD_REG(wdma_com, REG_ISP_WDMA_COM_T, NORM_STATUS1); //0x14

	//FE0 handshake
	fe0_pchk0_valid = ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_0); //0x40
	fe0_pchk0_ready = ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_1); //0x44
	fe0_pchk1_valid = ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_2); //0x48
	fe0_pchk1_ready = ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_3); //0x4C

	if (ctx->is_dual_sensor) {
		//FE1 handshake
		fe1_pchk0_valid = ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_0);
		fe1_pchk0_ready = ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_1);
		fe1_pchk1_valid = ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_2);
		fe1_pchk1_ready = ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, IP_CONNECTION_DEBUG_3);
	}

	//BE handshake
	be_pchk0_valid = ISP_RD_REG(be, REG_PRE_RAW_BE_T, IP_CONNECTION_DEBUG_0); //0x58
	be_pchk0_ready = ISP_RD_REG(be, REG_PRE_RAW_BE_T, IP_CONNECTION_DEBUG_1); //0x5C
	be_pchk1_valid = ISP_RD_REG(be, REG_PRE_RAW_BE_T, IP_CONNECTION_DEBUG_2); //0x60
	be_pchk1_ready = ISP_RD_REG(be, REG_PRE_RAW_BE_T, IP_CONNECTION_DEBUG_3); //0x64

	//RAWTOP handshake
	raw_valid = ISP_RD_REG(rawtop, REG_RAW_TOP_T, STVALID_STATUS); //0x40
	raw_ready = ISP_RD_REG(rawtop, REG_RAW_TOP_T, STREADY_STATUS); //0x44

	//RGBTOP handshake
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, DBG_IP_S_VLD, IP_DBG_EN, 1); //0x80
	rgb_valid = ISP_RD_REG(rgbtop, REG_ISP_RGB_T, DBG_IP_S_VLD); //0x80
	rgb_ready = ISP_RD_REG(rgbtop, REG_ISP_RGB_T, DBG_IP_S_RDY); //0x84

	//YUVTOP handshake
	yuv_valid = ISP_RD_REG(yuvtop, REG_YUV_TOP_T, STVALID_STATUS); //0x6c
	yuv_ready = ISP_RD_REG(yuvtop, REG_YUV_TOP_T, STREADY_STATUS); //0x70

	//tdnr handshake
	ISP_WR_BITS(tdnr, REG_ISP_444_422_T, REG_5, DEBUG_STATUS_EN, 1); //0x14
	tdnr_hs = ISP_RD_REG(tdnr, REG_ISP_444_422_T, REG_6); //0x18

	//tdnr cnt
	ISP_WR_BITS(tdnr, REG_ISP_444_422_T, REG_5, DEBUG_STATUS_EN, 0); //0x14
	tdnr_cnt = ISP_RD_REG(tdnr, REG_ISP_444_422_T, REG_6); //0x18

	//lb_ctrl_cnt
	lb_ctrl_cnt = ISP_RD_REG(rawtop, REG_RAW_TOP_T, DEBUG); //0x28

	vip_pr(CVI_INFO, "AXI_w=0x%x, AXI_r=0x%x\n", axi0_w, axi0_r);
	vip_pr(CVI_INFO, "errflag=0x%x, rgb_cnt=0x%x, yuv_cnt=0x%x\n", errflag, rgb_cnt, yuv_cnt);
	vip_pr(CVI_INFO, "raw_lexp_cnt=0x%x, be_lexp_cnt=0x%x\n", raw_lexp_cnt, be_lexp_cnt);
	vip_pr(CVI_INFO, "rdmi_en=0x%x, rdmi_valid=0x%x, rdmi_ready=0x%x\n", rdmi_en, rdmi_valid, rdmi_ready);
	vip_pr(CVI_INFO, "wdmi_en=0x%x, wdmi_valid=0x%x, wdmi_ready=0x%x\n", wdmi_en, wdmi_valid, wdmi_ready);
	vip_pr(CVI_INFO, "rdma_idle=0x%x, wdma_done=0x%x\n", rdma_idle, wdma_idle);
	vip_pr(CVI_INFO, "fe0_pchk0_valid=0x%x, fe0_pchk0_ready=0x%x\n", fe0_pchk0_valid, fe0_pchk0_ready);
	vip_pr(CVI_INFO, "fe0_pchk1_valid=0x%x, fe0_pchk1_ready=0x%x\n", fe0_pchk1_valid, fe0_pchk1_ready);
	if (ctx->is_dual_sensor) {
		vip_pr(CVI_INFO, "fe1_pchk0_valid=0x%x, fe1_pchk0_ready=0x%x\n", fe1_pchk0_valid, fe1_pchk0_ready);
		vip_pr(CVI_INFO, "fe1_pchk1_valid=0x%x, fe1_pchk1_ready=0x%x\n", fe1_pchk1_valid, fe1_pchk1_ready);
	}
	vip_pr(CVI_INFO, "be_pchk0_valid=0x%x, be_pchk0_ready=0x%x\n", be_pchk0_valid, be_pchk0_ready);
	vip_pr(CVI_INFO, "be_pchk1_valid=0x%x, be_pchk1_ready=0x%x\n", be_pchk1_valid, be_pchk1_ready);
	vip_pr(CVI_INFO, "raw_valid=0x%x, raw_ready=0x%x\n", raw_valid, raw_ready);
	vip_pr(CVI_INFO, "rgb_valid=0x%x, rgb_ready=0x%x\n", rgb_valid, rgb_ready);
	vip_pr(CVI_INFO, "yuv_valid=0x%x, yuv_ready=0x%x\n", yuv_valid, yuv_ready);
	vip_pr(CVI_INFO, "tdnr_handshake=0x%x, tdnr_counter=0x%x, lnbl_counter=0x%x\n", tdnr_hs, tdnr_cnt, lb_ctrl_cnt);

	for (i = 0; i < 10; i++) {
		u32 r_0 = 0, r_4 = 0, r_8 = 0, r_c = 0;

		//Debug
		ISP_WR_BITS(isptop, REG_ISP_TOP_T, REG_15, DBUS_SEL, 6 + i);
		r_0 = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_10); //0x0A070040
		r_4 = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_11); //0x0A070044
		r_8 = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_12); //0x0A070048
		r_c = ISP_RD_REG(isptop, REG_ISP_TOP_T, REG_13); //0x0A07004C

		vip_pr(CVI_INFO, "dbus_sel=%d, r_0=0x%x, r_4=0x%x, r_8=0x%x, r_c=0x%x\n", 6 + i, r_0, r_4, r_8, r_c);
	}
}

union REG_ISP_TOP_0 isp_intr_status(struct isp_ctx *ctx)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_0 status;

	status.raw = ISP_RD_REG(isp_top, REG_ISP_TOP_T, REG_0);

	//clear isp top status
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, REG_0, status.raw);

	return status;
}

union REG_ISP_TOP_9 isp_int_event1_status(struct isp_ctx *ctx)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_9 status;

	status.raw = ISP_RD_REG(isp_top, REG_ISP_TOP_T, REG_9);

	//clear isp top event1 status
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, REG_9, status.raw);

	return status;
}

union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 isp_csi_intr_status_0(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t csi_bdg0 = (raw_num == ISP_PRERAW_A)
			  ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
			  : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 status;

	status.raw = ISP_RD_REG(csi_bdg0, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_0);

	//clear status
	ISP_WR_REG(csi_bdg0, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_0, status.raw);

	return status;
}

union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 isp_csi_intr_status_1(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t csi_bdg0 = (raw_num == ISP_PRERAW_A)
			  ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
			  : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 status;

	status.raw = ISP_RD_REG(csi_bdg0, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_1);

	//clear status
	ISP_WR_REG(csi_bdg0, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_1, status.raw);

	return status;
}

void isp_streaming(struct isp_ctx *ctx, uint32_t on, enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg = (raw_num == ISP_PRERAW_A)
			  ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
			  : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	if (on) {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CSI_UP_REG, 1);

		if (raw_num == ISP_PRERAW_A) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_FE0, 0x3);
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_BE, 0x3);
			} else {
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_FE0, 0x1);
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_BE, 0x1);
			}
			ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_POST, 0x1);
		} else if (raw_num == ISP_PRERAW_B) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_FE1, 0x3);
			else
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, SHAW_UP_FE1, 0x1);
		}

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CSI_ENABLE, 1);

		if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, TGEN_ENABLE, 1);
	} else {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CSI_ENABLE, 0);

		if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, TGEN_ENABLE, 0);
	}
}

void isp_pre_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num)
{
	uintptr_t preraw0 = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0] :
			ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

	if (ctx->isp_pipe_cfg[raw_num].is_offline_preraw) { //dram->fe
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union REG_ISP_TOP_3 reg_3;

		reg_3.raw = 0;

		//ToDo
		if (!ctx->is_offline_be && !ctx->is_offline_postraw) { // fly-mode
			if (raw_num == ISP_PRERAW_A) {
				reg_3.bits.TRIG_STR_FE0	 = 0x1;
				reg_3.bits.PQ_UP_FE0	 = 0x1;
				reg_3.bits.SHAW_UP_FE0	 = 0x1;
				reg_3.bits.TRIG_STR_BE	 = 0x1;
				reg_3.bits.PQ_UP_BE	 = 0x1;
				reg_3.bits.SHAW_UP_BE	 = 0x1;
				reg_3.bits.TRIG_STR_POST = 0x1;
				reg_3.bits.PQ_UP_POST	 = 0x1;
				reg_3.bits.SHAW_UP_POST	 = 0x1;
			} else {
				reg_3.bits.TRIG_STR_FE1	 = 0x1;
				reg_3.bits.PQ_UP_FE1	 = 0x1;
				reg_3.bits.SHAW_UP_FE1	 = 0x1;
				reg_3.bits.TRIG_STR_BE	 = 0x1;
				reg_3.bits.PQ_UP_BE	 = 0x1;
				reg_3.bits.SHAW_UP_BE	 = 0x1;
				reg_3.bits.TRIG_STR_POST = 0x1;
				reg_3.bits.PQ_UP_POST	 = 0x1;
				reg_3.bits.SHAW_UP_POST	 = 0x1;
			}

			ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, reg_3.raw);
		} else if (!ctx->is_offline_be && ctx->is_offline_postraw) { // fe->be->dram->post
			// No VC mode from dram
			if (raw_num == ISP_PRERAW_A) {
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					reg_3.bits.TRIG_STR_FE0 = 0x3;
					reg_3.bits.PQ_UP_FE0	= 0x3;
					reg_3.bits.SHAW_UP_FE0	= 0x3;
					reg_3.bits.TRIG_STR_BE	= 0x3;
					reg_3.bits.PQ_UP_BE	= 0x3;
					reg_3.bits.SHAW_UP_BE	= 0x3;
				} else {
					reg_3.bits.TRIG_STR_FE0 = 0x1;
					reg_3.bits.PQ_UP_FE0	= 0x1;
					reg_3.bits.SHAW_UP_FE0	= 0x1;
					reg_3.bits.TRIG_STR_BE	= 0x1;
					reg_3.bits.PQ_UP_BE	= 0x1;
					reg_3.bits.SHAW_UP_BE	= 0x1;
				}

				ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, reg_3.raw);
			} else { //ISP_PRERAW_B
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					reg_3.bits.TRIG_STR_FE1 = 0x3;
					reg_3.bits.PQ_UP_FE1	= 0x3;
					reg_3.bits.SHAW_UP_FE1	= 0x3;
					reg_3.bits.TRIG_STR_BE	= 0x3;
					reg_3.bits.PQ_UP_BE	= 0x3;
					reg_3.bits.SHAW_UP_BE	= 0x3;
				} else {
					reg_3.bits.TRIG_STR_FE1 = 0x1;
					reg_3.bits.PQ_UP_FE1	= 0x1;
					reg_3.bits.SHAW_UP_FE1	= 0x1;
					reg_3.bits.TRIG_STR_BE	= 0x1;
					reg_3.bits.PQ_UP_BE	= 0x1;
					reg_3.bits.SHAW_UP_BE	= 0x1;
				}

				ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, reg_3.raw);
			}
		} else if (ctx->is_offline_be) { // fe->dram->be->post
			// No VC mode from dram
			if (raw_num == ISP_PRERAW_A) {
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					reg_3.bits.TRIG_STR_FE0 = 0x3;
					reg_3.bits.PQ_UP_FE0	= 0x3;
					reg_3.bits.SHAW_UP_FE0	= 0x3;
				} else {
					reg_3.bits.TRIG_STR_FE0 = 0x1;
					reg_3.bits.PQ_UP_FE0	= 0x1;
					reg_3.bits.SHAW_UP_FE0	= 0x1;
				}

				ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, reg_3.raw);
			} else { //ISP_PRERAW_B
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					reg_3.bits.TRIG_STR_FE1 = 0x3;
					reg_3.bits.PQ_UP_FE1	= 0x3;
					reg_3.bits.SHAW_UP_FE1	= 0x3;
				} else {
					reg_3.bits.TRIG_STR_FE1 = 0x1;
					reg_3.bits.PQ_UP_FE1	= 0x1;
					reg_3.bits.SHAW_UP_FE1	= 0x1;
				}

				ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, reg_3.raw);
			}
		}
	} else { // patgen or sensor->fe
		vip_pr(CVI_DBG, "trigger fe_%d chn_num_%d frame_vld\n", raw_num, chn_num);

		switch (chn_num) {
		case 0:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_PQ_VLD_CH0, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_FRAME_VLD_CH0, 1);
			break;
		case 1:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_PQ_VLD_CH1, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_FRAME_VLD_CH1, 1);
			break;
		case 2:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_PQ_VLD_CH2, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_FRAME_VLD_CH2, 1);
			break;
		case 3:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_PQ_VLD_CH3, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_FRAME_VLD_CH3, 1);
			break;
		default:
			break;
		}
	}
}

void isp_post_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	union REG_ISP_TOP_3 top_3;
	unsigned long flags = 0;

	top_3.raw = 0;

	if (!ctx->is_offline_be && ctx->is_offline_postraw) { //fe->be->dram->post
		top_3.bits.SHAW_UP_POST		= 1;
		top_3.bits.PQ_UP_POST		= 1;

		ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, top_3.raw);

		//YUVTOP height update
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);

		//SW workaround for 3dnr y write dma hw bug
		local_irq_save(flags);

		vip_pr(CVI_DBG, "raw_num_%d dram->post post trig\n", raw_num);

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //RGB sensor
			ispblk_cfa_softrst(ctx, 0);

		ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, TRIG_STR_POST, 1);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, top_3.raw);

		local_irq_restore(flags);
	} else if (ctx->is_offline_be && !ctx->is_offline_postraw) { //fe->dram->be->post
		vip_pr(CVI_DBG, "dram->be post trig raw_num(%d), is_hdr_on(%d)\n",
				raw_num, ctx->isp_pipe_cfg[raw_num].is_hdr_on);

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				top_3.bits.SHAW_UP_POST		= 1;
				top_3.bits.PQ_UP_POST		= 1;
				top_3.bits.SHAW_UP_BE		= 3;
				//top_3.bits.TRIG_STR_BE	= 3;
				top_3.bits.PQ_UP_BE		= 3;
			} else {
				top_3.bits.SHAW_UP_POST		= 1;
				top_3.bits.PQ_UP_POST		= 1;
				top_3.bits.SHAW_UP_BE		= 1;
				//top_3.bits.TRIG_STR_BE	= 1;
				top_3.bits.PQ_UP_BE		= 1;
			}

			ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, top_3.raw);

			//YUVTOP height update
			ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
			ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);

			//SW workaround for 3dnr y write dma hw bug
			local_irq_save(flags);

			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //RGB sensor
				ispblk_cfa_softrst(ctx, 0);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, TRIG_STR_BE, 3);
			else
				ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, TRIG_STR_BE, 1);

			ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, TRIG_STR_POST, 1);

			ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, top_3.raw);

			local_irq_restore(flags);
		} else {
			top_3.bits.SHAW_UP_POST		= 1;
			top_3.bits.PQ_UP_POST		= 1;

			ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, top_3.raw);

			//YUVTOP height update
			ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
			ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);

			//SW workaround for 3dnr y write dma hw bug
			local_irq_save(flags);

			ISP_WO_BITS(isptopb, REG_ISP_TOP_T, REG_3, TRIG_STR_POST, 1);
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_3, top_3.raw);

			local_irq_restore(flags);
		}
	}
}

void isp_intr_set_mask(struct isp_ctx *ctx, union isp_intr intr_mask)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	ISP_WR_REG(isp_top, REG_ISP_TOP_T, REG_2, intr_mask.raw);
}

void isp_init(struct isp_ctx *ctx)
{
	u8 i = 0;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		g_w_bit[i] = MANR_W_BIT;
		g_h_bit[i] = MANR_H_BIT;
		g_rgbmap_chg_pre[i][0] = false;
		g_rgbmap_chg_pre[i][1] = false;

		g_lmp_cfg[i].pre_chg[0] = false;
		g_lmp_cfg[i].pre_chg[1] = false;
		g_lmp_cfg[i].pre_w_bit = LUMA_MAP_W_BIT;
		g_lmp_cfg[i].pre_h_bit = LUMA_MAP_H_BIT;

		g_lmp_cfg[i].post_w_bit = LUMA_MAP_W_BIT;
		g_lmp_cfg[i].post_h_bit = LUMA_MAP_H_BIT;
	}
}

void isp_reset(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union isp_intr intr_mask;

	// disable interrupt
	intr_mask.raw = 0;
	isp_intr_set_mask(ctx, intr_mask);

	// switch back to hw trig.
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_4, TRIG_STR_SEL_FE0, 0xf);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_4, TRIG_STR_SEL_FE1, 0x3);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_4, TRIG_STR_SEL_BE, 0x3);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_4, TRIG_STR_SEL_POST, 1);

	// reset
#if 0
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, AXI_RST, 1);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, CSI0_RST, 1);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, CSI1_RST, 1);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, CSI_BE_RST, 1);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, CSI2_RST, 1);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, ISP_RST, 1);
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_7, APB_RST, 1);
#else
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, 0x20);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, 0xbf);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, 0x20);
#endif
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, 0);

	// clear intr
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_0, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_1, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_9, 0xffffffff);
}

int ispblk_isptop_config(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uint8_t pre_fe0_trig_by_hw = 0, pre_fe1_trig_by_hw = 0;
	uint8_t pre_be_trig_by_hw = 0;
	uint8_t post_trig_by_hw = 0;

	union REG_ISP_TOP_2 reg2;
	union REG_ISP_TOP_4 reg4;
	union REG_ISP_TOP_5 reg5;
	union REG_ISP_TOP_9 reg9;

	if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
		pre_fe0_trig_by_hw = 0;
	else {
		if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) { //RGB sensor
			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on)
				pre_fe0_trig_by_hw = 3;
			else
				pre_fe0_trig_by_hw = 1;
		} else { //YUV sensor
			switch (ctx->total_chn_num) {
			case 1:
				pre_fe0_trig_by_hw = 1;
				break;
			case 2:
				pre_fe0_trig_by_hw = 3;
				break;
			case 3:
				pre_fe0_trig_by_hw = 7;
				break;
			case 4:
				pre_fe0_trig_by_hw = 15;
				break;
			default:
				break;
			}
		}
	}

	if (ctx->is_dual_sensor) {
		if (ctx->isp_pipe_cfg[ISP_PRERAW_B].is_offline_preraw)
			pre_fe1_trig_by_hw = 0;
		else {
			if (!ctx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) { //RGB sensor
				if (ctx->isp_pipe_cfg[ISP_PRERAW_B].is_hdr_on)
					pre_fe1_trig_by_hw = 3;
				else
					pre_fe1_trig_by_hw = 1;
			} else { //YUV sensor
				switch (ctx->total_chn_num - ctx->rawb_chnstr_num) {
				case 1:
					pre_fe1_trig_by_hw = 1;
					break;
				case 2:
					pre_fe1_trig_by_hw = 3;
					break;
				default:
					break;
				}
			}
		}
	}

	if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
		pre_be_trig_by_hw = 0;
	else if (ctx->is_offline_be)
		pre_be_trig_by_hw = 0;
	else { //be online, on the fly mode or fe_A->be
		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
			if (_is_all_online(ctx)) //sensor->fe->yuvtop
				pre_be_trig_by_hw = 1;
			else
				pre_be_trig_by_hw = 0;
		} else { //Single RGB sensor
			if (ctx->is_hdr_on)
				pre_be_trig_by_hw = 3;
			else
				pre_be_trig_by_hw = 1;
		}
	}

	if ((ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw == 0) &&
		_is_all_online(ctx)) // fly mode
		post_trig_by_hw = 1;
	else
		post_trig_by_hw = 0;

	reg2.raw = 0;
	reg4.raw = 0;
	reg5.raw = 0;
	reg9.raw = 0;

	//pre_fe0
	reg2.bits.FRAME_DONE_ENABLE_FE0		= 0xF;
	reg2.bits.FRAME_START_ENABLE_FE0	= 0xF;
	reg4.bits.TRIG_STR_SEL_FE0		= pre_fe0_trig_by_hw;
	reg4.bits.SHAW_UP_SEL_FE0		= pre_fe0_trig_by_hw;
	reg4.bits.PQ_UP_SEL_FE0			= pre_fe0_trig_by_hw;

	//pre_fe1
	reg2.bits.FRAME_DONE_ENABLE_FE1		= 0x3;
	reg2.bits.FRAME_START_ENABLE_FE1	= 0x3;
	reg4.bits.TRIG_STR_SEL_FE1		= pre_fe1_trig_by_hw;
	reg4.bits.SHAW_UP_SEL_FE1		= pre_fe1_trig_by_hw;
	reg4.bits.PQ_UP_SEL_FE1			= pre_fe1_trig_by_hw;

	//pre_be
	reg2.bits.FRAME_DONE_ENABLE_BE	= 0x3;
	reg4.bits.TRIG_STR_SEL_BE	= pre_be_trig_by_hw;
	reg4.bits.SHAW_UP_SEL_BE	= pre_be_trig_by_hw;
	reg4.bits.PQ_UP_SEL_BE		= pre_be_trig_by_hw;

	//postraw
	reg2.bits.FRAME_DONE_ENABLE_POST = 1;
	reg4.bits.TRIG_STR_SEL_POST	= post_trig_by_hw;
	reg4.bits.SHAW_UP_SEL_POST	= post_trig_by_hw;
	reg4.bits.PQ_UP_SEL_POST	= post_trig_by_hw;

	//err int
	reg2.bits.FRAME_ERR_ENABLE	= 1;
	reg2.bits.INT_DMA_ERR_ENABLE	= 1;

	reg5.bits.DST2SC		= !ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler ||
						!ctx->isp_pipe_cfg[ISP_PRERAW_B].is_offline_scaler;
	reg5.bits.DST2DMA		= !reg5.bits.DST2SC;
	reg5.bits.POST_OFFLINE		= (ctx->is_offline_be || ctx->is_offline_postraw);
	reg5.bits.BE2DMA_ENABLE		= ctx->is_offline_postraw;
	reg5.bits.FE02BE_ENABLE		= !ctx->is_offline_be;
	reg5.bits.FE12BE_ENABLE		= 0;
	reg5.bits.BE2RAW_ENABLE		= !ctx->is_offline_postraw;
	reg5.bits.BE2YUV_422_ENABLE	= !ctx->is_dual_sensor && _is_all_online(ctx) &&
						ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path;
	reg5.bits.RAW2YUV_422_ENABLE	= 0;

	if (ctx->is_dual_sensor) {
		if ((ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path ||
			ctx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) &&
			(!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler ||
			!ctx->isp_pipe_cfg[ISP_PRERAW_B].is_offline_scaler)) {
			reg5.bits.RAW2YUV_422_ENABLE = 1;
		}
	}

	//awb frm done
	reg9.bits.FRAME_DONE_AWB	= 1;

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_0, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_2, reg2.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_4, reg4.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_5, reg5.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_9, reg9.raw);

	//Dummy registers
	//ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_6, IMG_WIDTHM1, ctx->img_width - 1);
	//ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_6, IMG_HEIGHTM1, ctx->img_height - 1);

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_1C, 7);

	return 0;
}

struct isp_grid_s_info ispblk_rgbmap_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_fe = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0]
		 : ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

	struct isp_grid_s_info ret;

	ret.w_bit = ISP_RD_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_H_GRID_SIZE);
	ret.h_bit = ISP_RD_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_V_GRID_SIZE);

	return ret;
}

struct isp_grid_s_info ispblk_lmap_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_fe = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0]
		 : ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

	struct isp_grid_s_info ret;

	ret.w_bit = ISP_RD_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_H_GRID_SIZE);
	ret.h_bit = ISP_RD_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_V_GRID_SIZE);

	return ret;
}

void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_fe;
	uint32_t width = ctx->isp_pipe_cfg[raw_num].crop.w;
	uint32_t height = ctx->isp_pipe_cfg[raw_num].crop.h;
	uint16_t w_grid_num, h_grid_num;

	preraw_fe = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0]
		 : ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL, BAYER_FRONT_TYPE, ctx->rgb_color_mode[raw_num]);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL, BAYER_TYPE, ctx->rgb_color_mode[raw_num]);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL, RGBIR_EN, ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor);

	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_SIZE, FRAME_WIDTHM1, width - 1);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_SIZE, FRAME_HEIGHTM1, height - 1);

	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_H_GRID_SIZE, g_w_bit[raw_num]);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_V_GRID_SIZE, g_h_bit[raw_num]);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER, SE_RGBMP_H_GRID_SIZE, g_w_bit[raw_num]);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER, SE_RGBMP_V_GRID_SIZE, g_h_bit[raw_num]);

	w_grid_num = UPPER(width, g_w_bit[raw_num]) - 1;
	h_grid_num = UPPER(height, g_h_bit[raw_num]) - 1;

	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_H_GRID_NUMM1, w_grid_num);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_V_GRID_NUMM1, h_grid_num);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER, SE_RGBMP_H_GRID_NUMM1, w_grid_num);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER, SE_RGBMP_V_GRID_NUMM1, h_grid_num);

	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_H_GRID_SIZE, g_lmp_cfg[raw_num].pre_w_bit);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_V_GRID_SIZE, g_lmp_cfg[raw_num].pre_h_bit);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_H_GRID_SIZE, g_lmp_cfg[raw_num].pre_w_bit);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_V_GRID_SIZE, g_lmp_cfg[raw_num].pre_h_bit);

	w_grid_num = UPPER(width, g_lmp_cfg[raw_num].pre_w_bit) - 1;
	h_grid_num = UPPER(height, g_lmp_cfg[raw_num].pre_h_bit) - 1;

	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_H_GRID_NUMM1, w_grid_num);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_V_GRID_NUMM1, h_grid_num);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_H_GRID_NUMM1, w_grid_num);
	ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_V_GRID_NUMM1, h_grid_num);
}

void ispblk_preraw_be_config(struct isp_ctx *ctx)
{
	uintptr_t preraw_be;
	union REG_PRE_RAW_BE_TOP_CTRL top_ctrl;

	preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];

	top_ctrl.raw = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL);
	top_ctrl.bits.BAYER_TYPE	= ctx->rgb_color_mode[ISP_PRERAW_A];
	top_ctrl.bits.RGBIR_EN		= ctx->is_rgbir_sensor;
	top_ctrl.bits.WDMI_EN_LE	= ctx->is_offline_postraw;
	top_ctrl.bits.WDMI_EN_SE	= (ctx->is_hdr_on && ctx->is_offline_postraw);
	top_ctrl.bits.CH_NUM		= ctx->is_hdr_on;

	if (ctx->is_rgbir_sensor) {
		top_ctrl.bits.BAYER_TYPE_AFTER_PREPROC = (ctx->rgb_color_mode[ISP_PRERAW_A] == ISP_BAYER_TYPE_BGRGI) ?
			ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_RG;
	} else
		top_ctrl.bits.BAYER_TYPE_AFTER_PREPROC	= ctx->rgb_color_mode[ISP_PRERAW_A];

	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL, top_ctrl.raw);

	if (ctx->is_dpcm_on) {
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_MODE, 7);
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_XSTR,
							(ctx->is_tile) ? ctx->tile_cfg.r_in.start : 8191);
	} else {
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_MODE, 0);
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_XSTR, 0); //to-do
	}

	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, CROP_SIZE_LE, CROP_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, CROP_SIZE_LE, CROP_HEIGHTM1, ctx->img_height - 1);
}

void ispblk_rawtop_config(struct isp_ctx *ctx)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	enum ISP_BAYER_TYPE bayer_id;

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_HEIGHTM1, ctx->img_height - 1);

	if (ctx->is_rgbir_sensor) {
		bayer_id = (ctx->rgb_color_mode[ISP_PRERAW_A] == ISP_BAYER_TYPE_BGRGI) ?
			ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_RG;
	} else
		bayer_id = ctx->rgb_color_mode[ISP_PRERAW_A];

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_BAYER_TYPE_TOPLEFT, BAYER_TYPE_TOPLEFT, bayer_id);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_BAYER_TYPE_TOPLEFT, RGBIR_ENABLE, ctx->is_rgbir_sensor);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, CH_NUM, ctx->is_hdr_on);

	if (ctx->is_yuv_sensor) {
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 1);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 1);
	} else {
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 0);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 0);
	}

	if (_is_fe_be_online(ctx)) { // fe->be->dram->post (wdr mode)
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, RDMI_EN, 1);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);
	} else { //fe->dram->be->post or fe->be->post (linear mode)
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, RDMI_EN, 0);
	}

	if (ctx->is_dpcm_on && ctx->is_tile) {
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, ctx->tile_cfg.r_in.start);
	} else if (ctx->is_dpcm_on) {
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 8191);
	} else {
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0); //to-do
	}
}

void ispblk_rgbtop_config(struct isp_ctx *ctx)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	enum ISP_BAYER_TYPE bayer_id;

	if (ctx->is_rgbir_sensor) {
		bayer_id = (ctx->rgb_color_mode[ISP_PRERAW_A] == ISP_BAYER_TYPE_BGRGI) ?
			ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_RG;
	} else
		bayer_id = ctx->rgb_color_mode[ISP_PRERAW_A];

	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_0, RGBTOP_BAYER_TYPE, bayer_id);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_0, RGBTOP_RGBIR_ENABLE, ctx->is_rgbir_sensor);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGH_M1, ctx->img_height - 1);
}

void ispblk_yuvtop_config(struct isp_ctx *ctx)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];

	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_3, Y42_SEL, ctx->is_yuv_sensor);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGW_M1_CROP, ctx->img_width - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);

	if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) {
		//bypass_v = 1 -> 422P online to scaler
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, HSV_ENABLE, BYPASS_V, 1);
	}
}

uint64_t ispblk_dma_getaddr(struct isp_ctx *ctx, uint32_t dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	uint64_t addr_h = ISP_RD_BITS(dmab, REG_ISP_DMA_T, ARR_SYSTEM, BASEH);

	return ((uint64_t)ISP_RD_REG(dmab, REG_ISP_DMA_T, ARR_BASE) | (addr_h << 32));
}

int ispblk_dma_buf_get_size(struct isp_ctx *ctx, int dmaid)
{
	uint32_t len = 0, num = 0;

	switch (dmaid) {
	case ISP_BLK_ID_WDMA2:
	case ISP_BLK_ID_WDMA4:
	case ISP_BLK_ID_WDMA8:
	case ISP_BLK_ID_WDMA10:
	{ //rgbmap max size
#define RGBMAP_MAX_BIT (3)
		//uint8_t grd_size = (ctx->is_tile) ? 3 : 2;
		uint8_t grd_size = RGBMAP_MAX_BIT;

		len = (((UPPER(ctx->img_width, grd_size)) * 6 + 15) >> 4) << 4;
		num = UPPER(ctx->img_height, grd_size);

		break;
	}
	case ISP_BLK_ID_WDMA3:
	case ISP_BLK_ID_WDMA5:
	case ISP_BLK_ID_WDMA9:
	case ISP_BLK_ID_WDMA11:
	{ //lmap max size
		len = (((((UPPER(ctx->img_width, 3)) * 3 + 1) >> 1) + 15) >> 4) << 4;
		num = UPPER(ctx->img_height, 3);

		break;
	}
	case ISP_BLK_ID_RDMA12:
	case ISP_BLK_ID_WDMA26:
	{
		/* manr rdma */
		u32 in_size = 0;
		u16 w_bit = MANR_W_BIT, h_bit = MANR_H_BIT;

		if (ctx->is_tile) {
			if (ctx->is_work_on_r_tile)
				in_size = ctx->tile_cfg.r_in.end - ctx->tile_cfg.r_in.start;
			else
				in_size = ctx->tile_cfg.l_in.end - ctx->tile_cfg.l_in.start;
		} else {
			in_size = ctx->img_width;
		}

		len = (((UPPER(in_size, w_bit) << 4) + 127) >> 7) << 4;
		num = UPPER(ctx->img_height, h_bit);

		break;
	}
	case ISP_BLK_ID_WDMA29:
	case ISP_BLK_ID_RDMA17:
	{
		u32 in_size = 0;

		if (ctx->is_work_on_r_tile)
			in_size = ctx->tile_cfg.r_in.end - ctx->tile_cfg.r_in.start;
		else
			in_size = ctx->tile_cfg.l_in.end - ctx->tile_cfg.l_in.start;

		//3DNR UV
		len = (((((in_size) << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->img_height >> 1;

		break;
	}
	case ISP_BLK_ID_RDMA16:
	case ISP_BLK_ID_WDMA28:
	{
		u32 in_size = 0;

		if (ctx->is_work_on_r_tile)
			in_size = ctx->tile_cfg.r_in.end - ctx->tile_cfg.r_in.start;
		else
			in_size = ctx->tile_cfg.l_in.end - ctx->tile_cfg.l_in.start;

		//3DNR Y
		len = (((((in_size) << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->img_height;

		break;
	}
	case ISP_BLK_ID_WDMA17:
	{
		/* gms */
		u32 sec_size = 255;

		len = (((sec_size + 1) >> 1) << 5) * 3;
		num = 1;

		break;
	}

	case ISP_BLK_ID_WDMA16:
	{
		/* af */
		uint16_t block_num_x = 17, block_num_y = 15;

		len = (block_num_x * block_num_y) << 5;
		num = 1;

		break;
	}

	case ISP_BLK_ID_WDMA18:
	case ISP_BLK_ID_WDMA21:
	{
		/* ae */
		int ae_dma_counts, hist_dma_counts, faceae_dma_counts;
		u8 numy = 15;

		ae_dma_counts		= 6 * (numy) * (256 / 8);
		hist_dma_counts		= 0x2000;
		faceae_dma_counts	= 128;

		len = (ae_dma_counts + hist_dma_counts + faceae_dma_counts) * 2;
		num = 1;

		break;
	}

	case ISP_BLK_ID_WDMA19:
	case ISP_BLK_ID_WDMA22:
	{
		/* awb */
		u8 numym1 = 33;

		len = (17 * (numym1)) << 6;
		num = 1;

		break;
	}

	default:
		break;
	}

	len = VIP_ALIGN(len);

	return len * num;
}


void ispblk_dma_set_group(struct isp_ctx *ctx, uint32_t dmaid, uint32_t grp)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_SYSTEM, GROUP_SEL, grp);
}

void ispblk_dma_setaddr(struct isp_ctx *ctx, uint32_t dmaid, uint64_t buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_REG(dmab, REG_ISP_DMA_T, ARR_BASE, (buf_addr & 0xFFFFFFFF));
	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_SYSTEM, BASEH, ((buf_addr >> 32) & 0xFFFFFFFF));
}

int ispblk_dma_config(struct isp_ctx *ctx, int dmaid, uint64_t buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	uint32_t w = 0, len = 0, stride = 0, num = 0;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	switch (dmaid) {
	case ISP_BLK_ID_WDMA0: //pre_raw_fe0
	case ISP_BLK_ID_WDMA1:
	case ISP_BLK_ID_WDMA6: //pre_raw_fe1
	case ISP_BLK_ID_WDMA7:
		/* csibdg */
		if (dmaid == ISP_BLK_ID_WDMA6 || dmaid == ISP_BLK_ID_WDMA7)
			raw_num = ISP_PRERAW_B;
		if (dmaid == ISP_BLK_ID_WDMA1)
			ispblk_dma_set_group(ctx, dmaid, 1);
		else if (dmaid == ISP_BLK_ID_WDMA6)
			ispblk_dma_set_group(ctx, dmaid, 2);
		else if (dmaid == ISP_BLK_ID_WDMA7)
			ispblk_dma_set_group(ctx, dmaid, 3);

		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		if (ctx->is_dpcm_on)
			w = (ctx->is_tile) ? ((w + 8) >> 1) : (w >> 1);

		len = 3 * UPPER(w, 1);

		break;
	case ISP_BLK_ID_WDMA2:
	case ISP_BLK_ID_WDMA4:
	case ISP_BLK_ID_WDMA8:
	case ISP_BLK_ID_WDMA10:
	{//rgbmap
		uintptr_t preraw_fe;
		union REG_PRE_RAW_FE_LE_RGBMAP_GRID_NUMBER grid_num;

		if (dmaid == ISP_BLK_ID_WDMA2 || dmaid == ISP_BLK_ID_WDMA4)
			preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		else
			preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

		if (dmaid == ISP_BLK_ID_WDMA2)
			ispblk_dma_set_group(ctx, dmaid, 0);
		else if (dmaid == ISP_BLK_ID_WDMA4)
			ispblk_dma_set_group(ctx, dmaid, 1);
		else if (dmaid == ISP_BLK_ID_WDMA8)
			ispblk_dma_set_group(ctx, dmaid, 2);
		else if (dmaid == ISP_BLK_ID_WDMA10)
			ispblk_dma_set_group(ctx, dmaid, 3);

		grid_num.raw = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER);

		len = (((grid_num.bits.LE_RGBMP_H_GRID_NUMM1 + 1) * 6 + 15) >> 4) << 4;
		num = grid_num.bits.LE_RGBMP_V_GRID_NUMM1 + 1;

		break;
	}
	case ISP_BLK_ID_WDMA3:
	case ISP_BLK_ID_WDMA5:
	case ISP_BLK_ID_WDMA9:
	case ISP_BLK_ID_WDMA11:
	{//lmap
		uintptr_t preraw_fe;
		union REG_PRE_RAW_FE_LE_LMAP_GRID_NUMBER grid_num;

		if (dmaid == ISP_BLK_ID_WDMA3 || dmaid == ISP_BLK_ID_WDMA5)
			preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		else
			preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

		if (dmaid == ISP_BLK_ID_WDMA3)
			ispblk_dma_set_group(ctx, dmaid, 0);
		else if (dmaid == ISP_BLK_ID_WDMA5)
			ispblk_dma_set_group(ctx, dmaid, 1);
		else if (dmaid == ISP_BLK_ID_WDMA9)
			ispblk_dma_set_group(ctx, dmaid, 2);
		else if (dmaid == ISP_BLK_ID_WDMA11)
			ispblk_dma_set_group(ctx, dmaid, 3);

		grid_num.raw = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER);

		len = (((((grid_num.bits.LE_LMP_H_GRID_NUMM1 + 1) * 3 + 1) >> 1) + 15) >> 4) << 4;
		num = grid_num.bits.LE_LMP_V_GRID_NUMM1 + 1;

		break;
	}

	case ISP_BLK_ID_WDMA14:
	case ISP_BLK_ID_WDMA15:
	{
		/* ir wdma*/
		uintptr_t ir_proc = ctx->phys_regs[ISP_BLK_ID_IR_PROC];
		uint32_t data_sel, bit_mode;

		if (dmaid == ISP_BLK_ID_WDMA14)
			ispblk_dma_set_group(ctx, dmaid, 4);
		else
			ispblk_dma_set_group(ctx, dmaid, 5);

		data_sel = ISP_RD_BITS(ir_proc, REG_IR_WDMA_PROC_T, IR_PROC_CTRL, IR_DATA_SEL);
		bit_mode = (ctx->sensor_bitdepth != 8) ? 0:1;

		w = ctx->img_width;
		num = data_sel ? (ctx->img_height>>1):ctx->img_height;
		len = data_sel ? (bit_mode ? w/2:(w*3/4)):(bit_mode ? w:(w*3/2));

		break;
	}
	case ISP_BLK_ID_RDMA15:
	{
		/* ir rdma */
		uint32_t bit_mode = (ctx->sensor_bitdepth != 8) ? 0:1;

		ispblk_dma_set_group(ctx, dmaid, 6);

		if (bit_mode == 0)
			len = (((ctx->img_width*12) + 0x7F) & ~0x7F)>>3;
		else
			len = (((ctx->img_width*8) + 0x7F) & ~0x7F)>>3;

		num = ctx->img_height;

		break;
	}

	case ISP_BLK_ID_WDMA16:
	{
		/* af */
		uintptr_t af = ctx->phys_regs[ISP_BLK_ID_AF];
		uint16_t block_num_x, block_num_y;

		ispblk_dma_set_group(ctx, dmaid, 4);

		block_num_x = ISP_RD_REG(af, REG_ISP_AF_T, BLOCK_NUM_X);
		block_num_y = ISP_RD_REG(af, REG_ISP_AF_T, BLOCK_NUM_Y);
		len = (block_num_x * block_num_y) << 5;
		num = 1;

		break;
	}
	case ISP_BLK_ID_WDMA17:
	{
		/* gms */
		uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_GMS];

		u32 x_sec_size = ISP_RD_REG(sts, REG_ISP_GMS_T, GMS_X_SECTION_SIZE);
		u32 y_sec_size = ISP_RD_REG(sts, REG_ISP_GMS_T, GMS_Y_SECTION_SIZE);
		u32 sec_size = (x_sec_size >= y_sec_size) ? x_sec_size : y_sec_size;

		ispblk_dma_set_group(ctx, dmaid, 4);

		len = (((sec_size + 1) >> 1) << 5) * 3;
		num = 1;

		break;
	}
	case ISP_BLK_ID_WDMA18:
	case ISP_BLK_ID_WDMA21:
	{
		uintptr_t sts;
		union REG_ISP_AE_HIST_STS_AE_NUMYM1 ae0numy;
		int ae_dma_counts, hist_dma_counts, faceae_dma_counts;

		/* ae */
		if (dmaid == ISP_BLK_ID_WDMA18) {
			ispblk_dma_set_group(ctx, dmaid, 4);
			sts = ctx->phys_regs[ISP_BLK_ID_AEHIST0];
		} else {
			ispblk_dma_set_group(ctx, dmaid, 5);
			sts = ctx->phys_regs[ISP_BLK_ID_AEHIST1];
		}

		ae0numy.raw = ISP_RD_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMYM1);

		ae_dma_counts		= 6 * (ae0numy.raw + 1) * (256 / 8);
		hist_dma_counts		= 0x2000;
		faceae_dma_counts	= 128;

		len = (ae_dma_counts + hist_dma_counts + faceae_dma_counts) * 2;
		num = 1;

		break;
	}
	case ISP_BLK_ID_WDMA19:
	case ISP_BLK_ID_WDMA22:
	{
		/* awb */
		uintptr_t map = 0;
		uint16_t numym1 = 0;

		switch (dmaid) {
		case ISP_BLK_ID_WDMA19:
			map = ctx->phys_regs[ISP_BLK_ID_AWB0];
			ispblk_dma_set_group(ctx, dmaid, 11);
			break;
		case ISP_BLK_ID_WDMA22:
			map = ctx->phys_regs[ISP_BLK_ID_AWB2];
			ispblk_dma_set_group(ctx, dmaid, 6);
			break;
		}
		numym1 = ISP_RD_REG(map, REG_ISP_AWB_T, STS_NUMYM1);

		len = (17 * (numym1 + 1)) << 6;
		num = 1;

		break;
	}
	case ISP_BLK_ID_WDMA20:
	{
		/* hist_edge_v */

		ispblk_dma_set_group(ctx, dmaid, 6);

		num = 1;
		len = 0x1000;

		break;
	}
	case ISP_BLK_ID_RDMA0:
	case ISP_BLK_ID_RDMA1:
		// preraw fe read
		ispblk_dma_set_group(ctx, dmaid, 0);

		w = ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_width;
		num = ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_height;
		if (ctx->is_hdr_on) //le/se lines in one raw file
			num *= 2;
		len = 3 * UPPER(w, 1);

		break;

	case ISP_BLK_ID_RDMA4:
	case ISP_BLK_ID_RDMA18:
	case ISP_BLK_ID_WDMA24:
	case ISP_BLK_ID_WDMA25:
	{
		u32 dpcm_header_byte_cnt = (ctx->is_tile && ctx->is_dpcm_on) ? 6 : 0;
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		// preraw be rdma and wdma
		if (dmaid == ISP_BLK_ID_RDMA18 || dmaid == ISP_BLK_ID_WDMA25)
			ispblk_dma_set_group(ctx, dmaid, 5);
		else
			ispblk_dma_set_group(ctx, dmaid, 4);

		w = ctx->img_width;
		num = ctx->img_height;

		len = 3 * UPPER(w, dpcm_on) + dpcm_header_byte_cnt;

		break;
	}
	case ISP_BLK_ID_RDMA5:
	case ISP_BLK_ID_RDMA6:
	{
		u32 tmp = 0;
		u32 dpcm_header_byte_cnt = (ctx->is_tile && ctx->is_dpcm_on) ? 6 : 0;
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		// rawtop
		ispblk_dma_set_group(ctx, dmaid, 7);

		w = ctx->img_width;
		num = ctx->img_height;

		len = 3 * UPPER(w, dpcm_on) + dpcm_header_byte_cnt;

		if (ctx->is_tile) {
			tmp = VIP_ALIGN(3 * UPPER(ctx->tile_cfg.r_out.end + 1, dpcm_on) + dpcm_header_byte_cnt);

			stride = tmp - VIP_ALIGN(len);
		}

		break;
	}
	case ISP_BLK_ID_RDMA7:
	{
		/* lsc rdma */
		// fixed value for 37x37

		ispblk_dma_set_group(ctx, dmaid, 6);

		num = 0x6f;
		len = 0x40;

		break;
	}
	case ISP_BLK_ID_RDMA8:
	case ISP_BLK_ID_RDMA9:
	case ISP_BLK_ID_RDMA10:
	case ISP_BLK_ID_RDMA11:
	{
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MANR];

		u16 w_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT);
		u16 h_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT);

		/* manr rdma */
		ispblk_dma_set_group(ctx, dmaid, 6);

		if (ctx->is_tile) {
			u32 tmp = 0;

			len = ((UPPER(ctx->img_width, w_bit) * 48 + 127) >> 7) << 4;
			num = UPPER(ctx->img_height, h_bit);

			tmp = ((UPPER(ctx->tile_cfg.r_out.end + 1, w_bit) * 48 + 127) >> 7) << 4;
			stride = tmp - len;
		} else {
			len = ((UPPER(ctx->img_width, w_bit) * 48 + 127) >> 7) << 4;
			num = UPPER(ctx->img_height, h_bit);
		}

		break;
	}
	case ISP_BLK_ID_RDMA12:
	case ISP_BLK_ID_WDMA26:
	{
		/* manr rdma */
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MANR];

		u16 w_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT);
		u16 h_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT);

		ispblk_dma_set_group(ctx, dmaid, 6);

		len = (((UPPER(ctx->img_width, w_bit) << 4) + 127) >> 7) << 4;
		num = UPPER(ctx->img_height, h_bit);

		break;
	}

	case ISP_BLK_ID_RDMA13:
	case ISP_BLK_ID_RDMA14:
	{
		/* ltm rdma */
		uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
		union REG_ISP_LTM_BLK_SIZE reg_blk;

		ispblk_dma_set_group(ctx, dmaid, 6);

		reg_blk.raw = ISP_RD_REG(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE);

		len = (((((reg_blk.bits.BLK_WIDTHM1 + 1) * 3 + 1) >> 1) + 15) >> 4) << 4;
		num = reg_blk.bits.BLK_HEIGHTM1 + 1;

		break;
	}

	case ISP_BLK_ID_RDMA17:
	case ISP_BLK_ID_WDMA29:
	{
		//uintptr_t fbcd = ctx->phys_regs[ISP_BLK_ID_FBCD];

		//3DNR UV
		ispblk_dma_set_group(ctx, dmaid, 6);
		len = (((((ctx->img_width) << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->img_height >> 1;
#if 0
		if (dmaid == ISP_BLK_ID_WDMA29 && ctx->is_fbc_on) {
			num = 2;
			len = ALIGN((((ALIGN(ctx->img_width, 32) * (ctx->img_height / 2 - 1) * g_uv_cr[ISP_PRERAW_A])
					/ 100) + ALIGN(ctx->img_width, 32)), 16) / num;
		} else if (dmaid == ISP_BLK_ID_RDMA17 && (ISP_RD_BITS(fbcd, REG_FBCD_T, REG_0, FBCD_EN) == 1)) {
			num = 2;
			len = ALIGN((((ALIGN(ctx->img_width, 32) * (ctx->img_height / 2 - 1) * g_uv_cr[ISP_PRERAW_A])
					/ 100) + ALIGN(ctx->img_width, 32)), 16) / num;
		}
#else
		if (ctx->is_fbc_on) {
			//g_uv_cr >= 50, than num = 2 is not enough
			num = 2;
			len = ISP_ALIGN((((ISP_ALIGN(ctx->img_width, 32) * (ctx->img_height / 2 - 1)
					* g_uv_cr[ISP_PRERAW_A]) / 100)
					+ ISP_ALIGN(ctx->img_width, 32)), 16) / num;
		}
#endif
		break;
	}

	case ISP_BLK_ID_RDMA16:
	case ISP_BLK_ID_WDMA28:
	{
		//uintptr_t fbcd = ctx->phys_regs[ISP_BLK_ID_FBCD];

		//3DNR Y
		ispblk_dma_set_group(ctx, dmaid, 6);
		len = (((((ctx->img_width) << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->img_height;
#if 0
		if (dmaid == ISP_BLK_ID_WDMA28 && ctx->is_fbc_on) {
			num = 2;
			len = ALIGN((((ALIGN(ctx->img_width, 32) * (ctx->img_height - 1) * g_y_cr[ISP_PRERAW_A])
					/ 100) + ALIGN(ctx->img_width, 32)), 16) / num;
		} else if (dmaid == ISP_BLK_ID_RDMA16 && (ISP_RD_BITS(fbcd, REG_FBCD_T, REG_0, FBCD_EN) == 1)) {
			num = 2;
			len = ALIGN((((ALIGN(ctx->img_width, 32) * (ctx->img_height - 1) * g_y_cr[ISP_PRERAW_A])
					/ 100) + ALIGN(ctx->img_width, 32)), 16) / num;
		}
#else
		if (ctx->is_fbc_on) {
			//g_y_cr >= 50, than num = 2 is not enough
			num = 2;
			len = ISP_ALIGN((((ISP_ALIGN(ctx->img_width, 32) * (ctx->img_height - 1)
					* g_y_cr[ISP_PRERAW_A]) / 100)
					+ ISP_ALIGN(ctx->img_width, 32)), 16) / num;
		}
#endif
		break;
	}

	case ISP_BLK_ID_WDMA27:
	{ //dci
		ispblk_dma_set_group(ctx, dmaid, 6);

		len = 0x200;
		num = 0x1;

		break;
	}
	case ISP_BLK_ID_WDMA30:
		//yuvtop y out
		ispblk_dma_set_group(ctx, dmaid, 6);

		len = (ctx->isp_pipe_cfg[raw_num].postout_crop.w) ?
			ctx->isp_pipe_cfg[raw_num].postout_crop.w : ctx->img_width;
		num = (ctx->isp_pipe_cfg[raw_num].postout_crop.h) ?
			ctx->isp_pipe_cfg[raw_num].postout_crop.h : ctx->img_height;

		if (ctx->is_tile) {
			len = (ctx->is_work_on_r_tile) ?
				(ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start) + 1 :
				(ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start) + 1;

			//For output buffer is 64byte alignment issue.
			stride = VIP_64_ALIGN(ctx->tile_cfg.r_out.end + 1) - VIP_ALIGN(len);
		}

		break;
	case ISP_BLK_ID_WDMA31:
		//yuvtop uv out
		ispblk_dma_set_group(ctx, dmaid, 6);

		len = (ctx->isp_pipe_cfg[raw_num].postout_crop.w) ?
			ctx->isp_pipe_cfg[raw_num].postout_crop.w : ctx->img_width;
		num = (ctx->isp_pipe_cfg[raw_num].postout_crop.h) ?
			(ctx->isp_pipe_cfg[raw_num].postout_crop.h >> 1) : (ctx->img_height >> 1);

		if (ctx->is_tile) {
			len = (ctx->is_work_on_r_tile) ?
				(ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start) + 1 :
				(ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start) + 1;

			//For output buffer is 64byte alignment issue.
			stride = VIP_64_ALIGN(ctx->tile_cfg.r_out.end + 1) - VIP_ALIGN(len);
		}

		break;
	default:
		break;
	}

	len = VIP_ALIGN(len);

	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_SEGLEN, SEGLEN, len);
	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_STRIDE, STRIDE, stride);
	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_SEGNUM, SEGNUM, num);

	if (buf_addr)
		ispblk_dma_setaddr(ctx, dmaid, buf_addr);

	return len * num;
}

void ispblk_dma_enable(struct isp_ctx *ctx, uint32_t dmaid, uint32_t on)
{
	uintptr_t srcb = 0;

	switch (dmaid) {
	case ISP_BLK_ID_WDMA30:
		/* yuvtop y crop8 */
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP8];
		break;
	case ISP_BLK_ID_WDMA31:
		/* yuvtop y crop9 */
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP9];
		break;
	case ISP_BLK_ID_RDMA4:
		/* preraw be rdma crop4*/
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP4];
		break;
	case ISP_BLK_ID_RDMA18:
		/* preraw be rdma crop5*/
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP5];
		break;
	case ISP_BLK_ID_RDMA5:
		/* rawtop rdma crop6 */
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP6];
		break;
	case ISP_BLK_ID_RDMA6:
		/* rawtop rdma crop7 */
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP7];
		break;

	default:
		break;
	}

	if (srcb)
		ISP_WR_BITS(srcb, REG_ISP_CROP_T, CROP_0, DMA_ENABLE, on);
}


void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];

	ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, CROP_ENABLE, en);
}

int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vip_rect crop)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];
	union REG_ISP_CROP_1 reg1;
	union REG_ISP_CROP_2 reg2;

	// crop out size
	reg1.bits.CROP_START_Y = crop.y;
	reg1.bits.CROP_END_Y = crop.y + crop.h - 1;
	reg2.bits.CROP_START_X = crop.x;
	reg2.bits.CROP_END_X = crop.x + crop.w - 1;
	ISP_WR_REG(cropb, REG_ISP_CROP_T, CROP_1, reg1.raw);
	ISP_WR_REG(cropb, REG_ISP_CROP_T, CROP_2, reg2.raw);
#if 0
	if (ctx->is_dpcm_on && ctx->is_tile) {
		ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, DPCM_MODE, 7);
		ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, DPCM_XSTR, ctx->tile_cfg.r_in.start);
	} else if (ctx->is_dpcm_on) {
		ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, DPCM_MODE, 7);
		ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, DPCM_XSTR, 8191);
	} else {
		ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, DPCM_MODE, 0);
		ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, DPCM_XSTR, 0);
	}
#endif
	ISP_WR_BITS(cropb, REG_ISP_CROP_T, CROP_0, CROP_ENABLE, true);

	return 0;
}

static int _blc_find_hwid(int id)
{
	int blc_id = -1;

	switch (id) {
	case ISP_BLC_ID_FE0_LE:
		blc_id = ISP_BLK_ID_BLC0;
		break;
	case ISP_BLC_ID_FE0_SE:
		blc_id = ISP_BLK_ID_BLC1;
		break;
	case ISP_BLC_ID_BE_LE:
		blc_id = ISP_BLK_ID_BLC4;
		break;
	case ISP_BLC_ID_BE_SE:
		blc_id = ISP_BLK_ID_BLC5;
		break;
	case ISP_BLC_ID_FE1_LE:
		blc_id = ISP_BLK_ID_BLC2;
		break;
	case ISP_BLC_ID_FE1_SE:
		blc_id = ISP_BLK_ID_BLC3;
		break;
	default:
		break;
	}
	return blc_id;
}

static int _ccm_find_hwid(int id)
{
	int ccm_id = -1;

	switch (id) {
	case ISP_CCM_ID_0:
		ccm_id = ISP_BLK_ID_CCM0;
		break;
	case ISP_CCM_ID_1:
		ccm_id = ISP_BLK_ID_CCM1;
		break;
	case ISP_CCM_ID_2:
		ccm_id = ISP_BLK_ID_CCM2;
		break;
	case ISP_CCM_ID_3:
		ccm_id = ISP_BLK_ID_CCM3;
		break;
	case ISP_CCM_ID_4:
		ccm_id = ISP_BLK_ID_CCM4;
		break;
	}
	return ccm_id;
}

void ispblk_blc_set_offset(struct isp_ctx *ctx, int blc_id,
				uint16_t roffset, uint16_t groffset,
				uint16_t gboffset, uint16_t boffset)
{
	int id = _blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_R, roffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_GR, groffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_GB, gboffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_B, boffset);
}

void ispblk_blc_set_2ndoffset(struct isp_ctx *ctx, int blc_id,
				uint16_t roffset, uint16_t groffset,
				uint16_t gboffset, uint16_t boffset)
{
	int id = _blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_R, roffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_GR, groffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_GB, gboffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_B, boffset);
}

void ispblk_blc_set_gain(struct isp_ctx *ctx, int blc_id,
				uint16_t rgain, uint16_t grgain,
				uint16_t gbgain, uint16_t bgain)
{
	int id = _blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_R, rgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_GR, grgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_GB, gbgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_B, bgain);
}

void ispblk_blc_enable(struct isp_ctx *ctx, int blc_id, bool en, bool bypass)
{
	int id = _blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_0, BLC_BYPASS, bypass);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_2, BLC_ENABLE, en);
}

int ispblk_blc_config(struct isp_ctx *ctx, uint32_t blc_id, bool en, bool bypass)
{
	int id = _blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return -1;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_0, BLC_BYPASS, bypass);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_2, BLC_ENABLE, en);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, IMG_BAYERID, IMG_BAYERID, ctx->rgb_color_mode[ctx->cam_id]);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_R, 511);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_GR, 511);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_GB, 511);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_B, 511);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_R, 0x40f);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_GR, 0x419);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_GB, 0x419);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_B, 0x405);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_R, 0);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_GR, 0);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_GB, 0);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_B, 0);

	return 0;
}

static int _wbg_find_hwid(int id)
{
	int wbg_id = -1;

	switch (id) {
	case ISP_WBG_ID_PRE0_FE_RGBMAP_LE:
		wbg_id = ISP_BLK_ID_WBG7;
		break;
	case ISP_WBG_ID_PRE0_FE_RGBMAP_SE:
		wbg_id = ISP_BLK_ID_WBG8;
		break;
	case ISP_WBG_ID_PRE0_FE_LMAP_LE:
		wbg_id = ISP_BLK_ID_WBG11;
		break;
	case ISP_WBG_ID_PRE0_FE_LMAP_SE:
		wbg_id = ISP_BLK_ID_WBG12;
		break;
	case ISP_WBG_ID_PRE1_FE_RGBMAP_LE:
		wbg_id = ISP_BLK_ID_WBG9;
		break;
	case ISP_WBG_ID_PRE1_FE_RGBMAP_SE:
		wbg_id = ISP_BLK_ID_WBG10;
		break;
	case ISP_WBG_ID_PRE1_FE_LMAP_LE:
		wbg_id = ISP_BLK_ID_WBG13;
		break;
	case ISP_WBG_ID_PRE1_FE_LMAP_SE:
		wbg_id = ISP_BLK_ID_WBG14;
		break;
	case ISP_WBG_ID_PRE_BE_LE:
		wbg_id = ISP_BLK_ID_WBG0;
		break;
	case ISP_WBG_ID_PRE_BE_SE:
		wbg_id = ISP_BLK_ID_WBG1;
		break;
	case ISP_WBG_ID_PRE_BE_INV_LE:
		wbg_id = ISP_BLK_ID_INV_WBG0;
		break;
	case ISP_WBG_ID_PRE_BE_INV_SE:
		wbg_id = ISP_BLK_ID_INV_WBG1;
		break;
	default:
		break;
	}
	return wbg_id;
}

int ispblk_wbg_config(struct isp_ctx *ctx, int wbg_id, uint16_t rgain, uint16_t ggain, uint16_t bgain)
{
	int id = _wbg_find_hwid(wbg_id);
	uintptr_t wbg;

	if (id < 0)
		return -EINVAL;

	wbg = ctx->phys_regs[id];
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_4, WBG_RGAIN, rgain);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_4, WBG_GGAIN, ggain);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_5, WBG_BGAIN, bgain);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, IMG_BAYERID, IMG_BAYERID, ctx->rgb_color_mode[ctx->cam_id]);

	return 0;
}

int ispblk_wbg_enable(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass)
{
	int id = _wbg_find_hwid(wbg_id);
	uintptr_t wbg;

	if (id < 0)
		return -EINVAL;

	wbg = ctx->phys_regs[id];
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_0, WBG_BYPASS, bypass);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_2, WBG_ENABLE, enable);

	return 0;
}

void ispblk_wbg_inv_config(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass)
{
	uintptr_t wbg = ctx->phys_regs[wbg_id];

	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_0, WBG_BYPASS, bypass);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_2, WBG_ENABLE, enable);

	ISP_WR_REG(wbg, REG_ISP_WBG_T, WBG_34, 0x0);
	ISP_WR_REG(wbg, REG_ISP_WBG_T, WBG_38, 0x0);
	ISP_WR_REG(wbg, REG_ISP_WBG_T, WBG_3C, 0x0);
}

void ispblk_lscr_set_lut(struct isp_ctx *ctx, int lscr_id, uint16_t *gain_lut, uint8_t lut_count)
{
	uintptr_t lscr = ctx->phys_regs[lscr_id];
	uint8_t i = 0;

	//ISP_WO_BITS(lscr, REG_ISP_LSCR_T, LSCR_INDEX_CLR, LSCR_INDEX_CLR, 1);
	ISP_RD_BITS(lscr, REG_ISP_LSCR_T, LSCR_INDEX_CLR, LSCR_INDEX_CLR);
	for (i = 0; i < lut_count; ++i)
		ISP_WR_REG(lscr, REG_ISP_LSCR_T, LSCR_GAIN_LUT, gain_lut[i]);
}

void ispblk_lscr_config(struct isp_ctx *ctx, int lscr_id, bool en)
{
	uintptr_t lscr = ctx->phys_regs[lscr_id];
	uint64_t nom, denom;
	uint32_t lscr_nom;

	uintptr_t preraw_fe = 0;

	nom = 1984 * 1984;
	denom = ((ctx->img_width / 2) * (ctx->img_width / 2)) +
		((ctx->img_height / 2) * (ctx->img_height / 2));
	lscr_nom = (uint32_t)((nom * 1024) / denom);

	ISP_WR_REG(lscr, REG_ISP_LSCR_T, LSCR_ENABLE, en);
	lscr_nom = 0xfff;
	ISP_WR_REG(lscr, REG_ISP_LSCR_T, LSCR_NORM, lscr_nom);

	//Tuning
	ISP_WR_REG(lscr, REG_ISP_LSCR_T, LSCR_CENTERX, ctx->img_width >> 1);
	ISP_WR_REG(lscr, REG_ISP_LSCR_T, LSCR_CENTERY, ctx->img_height >> 1);
	ISP_WR_REG(lscr, REG_ISP_LSCR_T, LSCR_STRNTH, 4095);

	if (en) {
		if (lscr_id == ISP_BLK_ID_LSCR0 || lscr_id == ISP_BLK_ID_LSCR1)
			preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		else if (lscr_id == ISP_BLK_ID_LSCR2 || lscr_id == ISP_BLK_ID_LSCR3)
			preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

		if (lscr_id == ISP_BLK_ID_LSCR0 || lscr_id == ISP_BLK_ID_LSCR2)
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, IP_INPUT_SEL, RGBMP_LSCR_ENABLE, 1);
		else if (lscr_id == ISP_BLK_ID_LSCR1 || lscr_id == ISP_BLK_ID_LSCR3)
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, IP_INPUT_SEL, LMP_LSCR_ENABLE, 1);
	}
}

/****************************************************************************
 *	PRERAW FE SUBSYS
 ****************************************************************************/

static void _patgen_config_timing(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg;
	uint16_t pat_height = (ctx->is_hdr_on) ?
				(ctx->isp_pipe_cfg[raw_num].csibdg_height * 2 - 1) :
				(ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	csibdg = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
		 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_V_SIZE, VMDE_STR, 0x00);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_V_SIZE, VMDE_STP, pat_height);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_H_SIZE, HMDE_STR, 0x00);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_H_SIZE, HMDE_STP,
							ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_V_SIZE, VFDE_STR, 0x10);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_V_SIZE, VFDE_STP, 0x10 + pat_height);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_H_SIZE, HFDE_STR, 0x10);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_H_SIZE, HFDE_STP,
							0x10 + ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_HSYNC_CTRL, HS_STR, 4);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_HSYNC_CTRL, HS_STP, 5);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_VSYNC_CTRL, VS_STR, 4);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_VSYNC_CTRL, VS_STP, 5);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, VTT, 0xFFF);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, HTT, 0x17FF);
}

static void _patgen_config_pat(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg;

	csibdg = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
		 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, GRA_INV, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, AUTO_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, DITH_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, SNOW_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, FIX_MC, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, DITH_MD, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, BAYER_ID, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_PRD, 0);
	if (raw_num == ISP_PRERAW_A)
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 0x7);
	else
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_R, 0xFFF);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_G, 0xFFF);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_1, PAT_B, 0xFFF);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_R, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_G, 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_1, FDE_B, 2);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_R, 0x457);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_G, 0x8AE);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_1, MDE_B, 0xD05);
}

void ispblk_cfa_softrst(struct isp_ctx *ctx, u8 en)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];

	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_1, SOFTRST, en);
}

void _ispblk_yuvtop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t y42 = ctx->phys_regs[ISP_BLK_ID_444422];
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUVDITHER];
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE];
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_YUVTOP];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		//Disable 3DNR dma
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_8, FORCE_DMA_DISABLE, 0x3f);
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_4, REG_422_444, 1);
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_8, TDNR_DEBUG_SEL, (ctx->is_tile) ? 0x4 : 0x0);
		//Because the yuv sensor don't pass the 444->422 module
		//Therefore the format output from 3dnr is yvu, need to swap to yuv
		//workaround, hw yuvtop's bug, only support yuyv and yvyu
		//for uyvy and vyuy, should set csi_ctrl_top's csi_format_frc[1] and csi_format_set[raw_16]
		if (ctx->isp_pipe_cfg[raw_num].enDataSeq == VI_DATA_SEQ_UYVY ||
			ctx->isp_pipe_cfg[raw_num].enDataSeq == VI_DATA_SEQ_YUYV)
			ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_4, SWAP, 1);
		else
			ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_4, SWAP, 0);

		//Disable yuv dither
		ISP_WR_BITS(dither, REG_ISP_YUVDITHER_T, Y_DITHER, Y_DITHER_EN, 0);
		ISP_WR_BITS(dither, REG_ISP_YUVDITHER_T, UV_DITHER, UV_DITHER_EN, 0);
		//Disable cnr
		ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_ENABLE, CNR_ENABLE, 0);
		ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_ENABLE, PFC_ENABLE, 0);
		//Disable ynr
		ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, ISP_YNR_OUT_Y_DELAY);
		//Disable ee
		ISP_WR_BITS(ee, REG_ISP_EE_T, REG_00, EE_ENABLE, 0);
		//Disable ycurv
		ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_CTRL, YCURVE_ENABLE, 0);
		//Disable ca_lite
		ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_ENABLE, 0);

		if (_is_fe_be_online(ctx)) {
			ispblk_crop_enable(ctx, ISP_BLK_ID_CROP8, false);
			ispblk_crop_enable(ctx, ISP_BLK_ID_CROP9, false);
		}

		if (_is_all_online(ctx)) {
			ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
			ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);
		}

	} else { //RGB sensor
		//Enable 3DNR dma
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_8, FORCE_DMA_DISABLE, (ctx->is_3dnr_on) ? 0 : 0x3f);
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_4, REG_422_444, 0);
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_8, TDNR_DEBUG_SEL, (ctx->is_tile) ? 0x4 : 0x0);
		ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_4, SWAP, 0);
	}

	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGW_M1, ctx->img_width - 1);
	//ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGW_M1_CROP, ctx->img_width - 1);
	//ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);

	//bypass_v = 1 -> 422P online to scaler
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, HSV_ENABLE, BYPASS_V, !ctx->isp_pipe_cfg[raw_num].is_offline_scaler);
}

void _ispblk_rgbtop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSCM0];
	uintptr_t hist_edge_v = ctx->phys_regs[ISP_BLK_ID_HIST_EDGE_V];
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	//uintptr_t awb_post = ctx->phys_regs[ISP_BLK_ID_AWB2];
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MANR];
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DHZ];

	u32 dma_enable;

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		//Disable lsc dma
		ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, 0);

		//Disable hist_edge_v dma
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, SW_CTL, TILE_NM, 0);
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, 0);

		//Disable ltm dma
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_TOP_CTRL, DTONE_EHN_EN, 0);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_TOP_CTRL, BTONE_EHN_EN, 0);

		//Disable awb dma
		//ISP_WR_BITS(awb_post, REG_ISP_AWB_T, DMI_ENABLE, DMI_ENABLE, 0);

		//Disable manr dma
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, 0xff);
		//Manr bypass
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_00, BYPASS, 1);
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_D0, CROP_ENABLE_SCALAR, ctx->is_tile);

		//Disable dci dma
		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_ROI_ENABLE, false);
		ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, 0);

		//Disable dhz tile
		ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_5, TILE_NM, 0);
	} else { //RGB sensor

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) // hdr mode
			dma_enable = 0xa0;
		else // linear mode
			dma_enable = 0x0a;

		if (ctx->is_3dnr_on) {
			//Enable manr dma
			ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, dma_enable);
			//Manr bypass
			ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_00, BYPASS, 0);
			ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_D0, CROP_ENABLE_SCALAR, ctx->is_tile);
		}

		//Enable lsc dma
		//ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, 1);

		//Enable hist_edge_v dma
		//ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, 1);

		//Enable awb dma
		//ISP_WR_BITS(awb_post, REG_ISP_AWB_T, DMI_ENABLE, DMI_ENABLE, 1);

		//Enable dci dma
		//ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, 1);
	}

	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGH_M1, ctx->img_height - 1);
}

void _ispblk_rawtop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, CH_NUM, 0);
		if (!_is_all_online(ctx)) //sensor->fe->dram
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, RDMI_EN, 1);
		else //sensor->fe->yuv_top
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, RDMI_EN, 0);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 1);

		// To block data flow into line async
		// YUV sensor data flow
		// data->rawtop->yuvtop, should block data in cfa
		ispblk_cfa_softrst(ctx, 1);

		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);
	} else { //RGB sensor
		if (_is_be_post_online(ctx)) //fe->dram->be->post
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, RDMI_EN, 0);
		else if (_is_fe_be_online(ctx)) {//fe->be->dram->post
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, RDMI_EN, 1);

			if (ctx->is_dpcm_on && ctx->is_tile) {
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, ctx->tile_cfg.r_in.start);
			} else if (ctx->is_dpcm_on) {
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 8191);
			} else {
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);
			}
		}

		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 0);
	}

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_HEIGHTM1, ctx->img_height - 1);
}

void _ispblk_be_yuv_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t af = ctx->phys_regs[ISP_BLK_ID_AF];
	uintptr_t ae_le = ctx->phys_regs[ISP_BLK_ID_AEHIST0];
	uintptr_t ae_se = ctx->phys_regs[ISP_BLK_ID_AEHIST1];
	uintptr_t awb_be = ctx->phys_regs[ISP_BLK_ID_AWB0];
	uintptr_t gms = ctx->phys_regs[ISP_BLK_ID_GMS];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor

		//Disable af dma
		ISP_WR_BITS(af, REG_ISP_AF_T, DMI_ENABLE, DMI_ENABLE, 0);
		//Disable ae dma
		ISP_WR_BITS(ae_le, REG_ISP_AE_HIST_T, DMI_ENABLE, DMI_ENABLE, 0);
		ISP_WR_BITS(ae_se, REG_ISP_AE_HIST_T, DMI_ENABLE, DMI_ENABLE, 0);
		//Disable awb dma
		ISP_WR_BITS(awb_be, REG_ISP_AWB_T, DMI_ENABLE, DMI_ENABLE, 0);
		//Disable gms dma
		ISP_WR_BITS(gms, REG_ISP_GMS_T, DMI_ENABLE, DMI_ENABLE, 0);
	}

	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, CROP_SIZE_LE, CROP_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, CROP_SIZE_LE, CROP_HEIGHTM1, ctx->img_height - 1);
}

void _ispblk_isptop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path && !_is_all_online(ctx)) { //YUV sensor
		ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_5, RAW2YUV_422_ENABLE, 1);
	} else { //RGB sensor
		ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_5, RAW2YUV_422_ENABLE, 0);
	}

	if (_is_be_post_online(ctx)) {
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
			ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_5, BE2RAW_ENABLE, 0);
		} else { //RGB sensor
			ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_5, BE2RAW_ENABLE, 1);
		}
	}
}

void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	_ispblk_isptop_cfg_update(ctx, raw_num);
	//_ispblk_be_yuv_cfg_update(ctx, raw_num);
	_ispblk_rawtop_cfg_update(ctx, raw_num);
	_ispblk_rgbtop_cfg_update(ctx, raw_num);
	_ispblk_yuvtop_cfg_update(ctx, raw_num);
}

u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, uint32_t dmaid, uint64_t buf_addr,
					const enum cvi_isp_raw raw_num)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	uint32_t len = 0, stride = 0, num = 0;

	switch (dmaid) {
	case ISP_BLK_ID_WDMA0: //pre_raw_fe0
	case ISP_BLK_ID_WDMA1:
	case ISP_BLK_ID_WDMA6: //pre_raw_fe1
	case ISP_BLK_ID_WDMA7:
		/* csibdg */
		if (dmaid == ISP_BLK_ID_WDMA1)
			ispblk_dma_set_group(ctx, dmaid, 1);
		if (raw_num != ISP_PRERAW_A) {
			if (dmaid == ISP_BLK_ID_WDMA6)
				ispblk_dma_set_group(ctx, dmaid, 2);
			else if (dmaid == ISP_BLK_ID_WDMA7)
				ispblk_dma_set_group(ctx, dmaid, 3);
		} else {
			if (dmaid == ISP_BLK_ID_WDMA6)
				ispblk_dma_set_group(ctx, dmaid, 9);
			else if (dmaid == ISP_BLK_ID_WDMA7)
				ispblk_dma_set_group(ctx, dmaid, 10);
		}
		len = ctx->isp_pipe_cfg[raw_num].csibdg_width * 2;
		num = ctx->isp_pipe_cfg[raw_num].csibdg_height;

		break;
	case ISP_BLK_ID_RDMA5:
	case ISP_BLK_ID_RDMA6:
	{
		// rawtop
		ispblk_dma_set_group(ctx, dmaid, 7);

		len = ctx->img_width * 2;
		num = ctx->img_height;

		break;
	}
	default:
		break;
	}

	len = VIP_ALIGN(len);

	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_SEGLEN, SEGLEN, len);
	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_STRIDE, STRIDE, stride);
	ISP_WR_BITS(dmab, REG_ISP_DMA_T, ARR_SEGNUM, SEGNUM, num);

	if (buf_addr)
		ispblk_dma_setaddr(ctx, dmaid, buf_addr);

	return len * num;
}

void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg = 0;
	union REG_ISP_CSI_BDG_TOP_CTRL csibdg_ctrl;
	u8 chn_num = ctx->isp_pipe_cfg[raw_num].muxMode;

	/*
	 * MIPI--->MUX use FE0 or FE1
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 * BT----->MUX use csibdg lite
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 */
	if (chn_num == 0 || (ctx->isp_pipe_cfg[raw_num].infMode >= VI_MODE_MIPI_YUV420_NORMAL &&
		ctx->isp_pipe_cfg[raw_num].infMode <= VI_MODE_MIPI_YUV422)) {
		csibdg = (raw_num == ISP_PRERAW_A)
			 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
			 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

		csibdg_ctrl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL);
		csibdg_ctrl.bits.RESET_MODE		= 0;
		csibdg_ctrl.bits.ABORT_MODE		= 0;
		csibdg_ctrl.bits.CSI_MODE		= 1;
		csibdg_ctrl.bits.CSI_IN_FORMAT		= 1;
		csibdg_ctrl.bits.CSI_IN_YUV_FORMAT	= 0;
		csibdg_ctrl.bits.Y_ONLY			= 0;
		csibdg_ctrl.bits.YUV2BAY_ENABLE		= 0;
		csibdg_ctrl.bits.CH_NUM			= chn_num;
		csibdg_ctrl.bits.MULTI_CH_FRAME_SYNC_EN	= 0;
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, csibdg_ctrl.raw);

	} else if (chn_num > 0 && ctx->isp_pipe_cfg[raw_num].infMode >= VI_MODE_BT656 &&
		ctx->isp_pipe_cfg[raw_num].infMode <= VI_MODE_BT1120_INTERLEAVED) {
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG_LITE];
	}

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, VS_MODE,
			ctx->isp_pipe_cfg[raw_num].is_stagger_vsync);

	if (!_is_all_online(ctx)) // sensor->fe->dram
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_WR_ENABLE, true);
	else // sensor->fe->yuvtop
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_WR_ENABLE, false);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH1_DMA_WR_ENABLE, (chn_num > 0) ? true : false);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH2_DMA_WR_ENABLE, (chn_num > 1) ? true : false);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH3_DMA_WR_ENABLE, (chn_num > 2) ? true : false);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);
}

void ispblk_csibdg_update_size(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg;

	csibdg = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
		 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE,
		    CH0_FRAME_WIDTHM1, ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE,
		    CH0_FRAME_HEIGHTM1, ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);
}

void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, bool en)
{
	uintptr_t csibdg;
	struct vip_rect crop, crop_se;

	csibdg = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
		 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	crop.x = (ctx->isp_pipe_cfg[raw_num].crop.x == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop.x;
	crop.y = (ctx->isp_pipe_cfg[raw_num].crop.y == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop.y;
	crop.w = (ctx->isp_pipe_cfg[raw_num].crop.x + ctx->isp_pipe_cfg[raw_num].crop.w) - 1;
	crop.h = (ctx->isp_pipe_cfg[raw_num].crop.y + ctx->isp_pipe_cfg[raw_num].crop.h) - 1;

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_CROP_EN, CH0_CROP_EN, en);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_HORZ_CROP, CH0_HORZ_CROP_START, crop.x);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_HORZ_CROP, CH0_HORZ_CROP_END, crop.w);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_VERT_CROP, CH0_VERT_CROP_START, crop.y);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_VERT_CROP, CH0_VERT_CROP_END, crop.h);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		crop_se.x = (ctx->isp_pipe_cfg[raw_num].crop_se.x == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop_se.x;
		crop_se.y = (ctx->isp_pipe_cfg[raw_num].crop_se.y == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop_se.y;
		crop_se.w = (ctx->isp_pipe_cfg[raw_num].crop_se.x + ctx->isp_pipe_cfg[raw_num].crop_se.w) - 1;
		crop_se.h = (ctx->isp_pipe_cfg[raw_num].crop_se.y + ctx->isp_pipe_cfg[raw_num].crop_se.h) - 1;

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_CROP_EN, CH1_CROP_EN, en);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_HORZ_CROP, CH1_HORZ_CROP_START, crop.x);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_HORZ_CROP, CH1_HORZ_CROP_END, crop.w);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_VERT_CROP, CH1_VERT_CROP_START, crop.y);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_VERT_CROP, CH1_VERT_CROP_END, crop.h);
	}
}

void ispblk_csidbg_dma_wr_en(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const u8 chn_num, const u8 en)
{
	uintptr_t csibdg = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
		 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	if (chn_num > 3)
		return;

	switch (chn_num) {
	case 0:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_WR_ENABLE, en);
		break;
	case 1:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH1_DMA_WR_ENABLE, en);
		break;
	case 2:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH2_DMA_WR_ENABLE, en);
		break;
	case 3:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH3_DMA_WR_ENABLE, en);
		break;
	}
}

int ispblk_csibdg_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg;
	uint8_t csi_mode = 0;
	union REG_ISP_CSI_BDG_TOP_CTRL top_ctrl;

	csibdg = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_CSIBDG0]
		 : ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	top_ctrl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL);
	top_ctrl.bits.RESET_MODE	= 0;
	top_ctrl.bits.ABORT_MODE	= 0;
	top_ctrl.bits.CSI_IN_FORMAT	= 0;
	top_ctrl.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;

	if (ctx->is_offline_be && !ctx->is_offline_postraw) { //fe->dram->be->post
		top_ctrl.bits.CH0_DMA_WR_ENABLE = 1;
		top_ctrl.bits.CH1_DMA_WR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	} else {
		top_ctrl.bits.CH0_DMA_WR_ENABLE = 0;
		top_ctrl.bits.CH1_DMA_WR_ENABLE = 0;
	}
	// ToDo stagger sensor
	//ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH2_DMA_WR_ENABLE, ctx->is_offline_be);
	//ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH3_DMA_WR_ENABLE, ctx->is_offline_be);

	if (ctx->isp_pipe_cfg[raw_num].is_patgen_en) {
		csi_mode = 3;
		top_ctrl.bits.PXL_DATA_SEL = 1;
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, PAT_EN, 1);

		_patgen_config_timing(ctx, raw_num);
		_patgen_config_pat(ctx, raw_num);
	} else {
		top_ctrl.bits.PXL_DATA_SEL = 0;

		csi_mode = (ctx->isp_pipe_cfg[raw_num].is_offline_preraw) ? 2 : 1;
	}

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	top_ctrl.bits.CSI_MODE	= csi_mode;
	top_ctrl.bits.VS_MODE	= ctx->isp_pipe_cfg[raw_num].is_stagger_vsync;
	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, top_ctrl.raw);

	if (ctx->is_dpcm_on) {
		union REG_ISP_CSI_BDG_DMA_DPCM_MODE dpcm;

		dpcm.bits.DMA_ST_DPCM_MODE = 0x7; //12->6 mode
		dpcm.bits.DPCM_XSTR = (ctx->is_tile) ? ctx->tile_cfg.r_in.start : 8191;
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_DMA_DPCM_MODE, dpcm.raw);
	} else {
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_DMA_DPCM_MODE, 0);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_INTERRUPT_CTRL, CH0_VS_INT_EN, 0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_INTERRUPT_CTRL, CH1_VS_INT_EN, 0);
	} else {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_INTERRUPT_CTRL, CH0_VS_INT_EN, 1);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_INTERRUPT_CTRL, CH1_VS_INT_EN, 1);
	}

	return 0;
}

void ispblk_rgbmap_config(struct isp_ctx *ctx, int map_id, bool en, enum cvi_isp_raw raw_num)
{
	uintptr_t map = ctx->phys_regs[map_id];

	switch (map_id) {
	case ISP_BLK_ID_RGBMAP0:
		map = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_RGBMAP0] :
			ctx->phys_regs[ISP_BLK_ID_RGBMAP2];
		break;
	case ISP_BLK_ID_RGBMAP1:
		map = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_RGBMAP1] :
			ctx->phys_regs[ISP_BLK_ID_RGBMAP3];
		break;
	default:
		break;
	}

	ISP_WR_BITS(map, REG_ISP_RGBMAP_T, RGBMAP_0, RGBMAP_ENABLE, en);
}

void ispblk_lmap_chg_size(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const enum cvi_isp_pre_chn_num chn_num)
{
	uintptr_t preraw_fe = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0]
		 : ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	uint16_t w_grid_num, h_grid_num;
	uint32_t img_width = (ctx->is_tile) ?
				ctx->tile_cfg.r_out.end + 1 :
				ctx->isp_pipe_cfg[raw_num].crop.w;
	uint32_t img_height = ctx->isp_pipe_cfg[raw_num].crop.h;

	if (g_lmp_cfg[raw_num].pre_chg[chn_num]) {
		g_lmp_cfg[raw_num].pre_chg[chn_num] = false;

		w_grid_num = UPPER(img_width, g_lmp_cfg[raw_num].pre_w_bit) - 1;
		h_grid_num = UPPER(img_height, g_lmp_cfg[raw_num].pre_h_bit) - 1;

		if (chn_num == ISP_FE_CH0) {
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_H_GRID_SIZE,
					g_lmp_cfg[raw_num].pre_w_bit);
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_V_GRID_SIZE,
					g_lmp_cfg[raw_num].pre_h_bit);
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_H_GRID_NUMM1, w_grid_num);
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_LMAP_GRID_NUMBER, LE_LMP_V_GRID_NUMM1, h_grid_num);

			if (raw_num == ISP_PRERAW_A)
				ispblk_dma_config(ctx, ISP_BLK_ID_WDMA3, 0);
			else
				ispblk_dma_config(ctx, ISP_BLK_ID_WDMA9, 0);
		} else {
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_H_GRID_SIZE,
					g_lmp_cfg[raw_num].pre_w_bit);
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_V_GRID_SIZE,
					g_lmp_cfg[raw_num].pre_h_bit);
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_H_GRID_NUMM1, w_grid_num);
			ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_LMAP_GRID_NUMBER, SE_LMP_V_GRID_NUMM1, h_grid_num);

			if (raw_num == ISP_PRERAW_A)
				ispblk_dma_config(ctx, ISP_BLK_ID_WDMA5, 0);
			else
				ispblk_dma_config(ctx, ISP_BLK_ID_WDMA11, 0);
		}
	}
}

int ispblk_lmap_config(struct isp_ctx *ctx, int map_id, bool en, enum cvi_isp_raw raw_num)
{
	uintptr_t map = ctx->phys_regs[map_id];
	union REG_ISP_LMAP_LMP_0 reg0;

	switch (map_id) {
	case ISP_BLK_ID_LMP0:
		map = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_LMP0] :
			ctx->phys_regs[ISP_BLK_ID_LMP2];
		break;
	case ISP_BLK_ID_LMP1:
		map = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_LMP1] :
			ctx->phys_regs[ISP_BLK_ID_LMP3];
		break;
	default:
		break;
	}

	if (!en) {
		ISP_WR_BITS(map, REG_ISP_LMAP_T, LMP_0, LMAP_ENABLE, 0);
		return 0;
	}

	reg0.raw = 0;
	reg0.bits.LMAP_ENABLE = 1;
	reg0.bits.LMAP_Y_MODE = 0;
	reg0.bits.LMAP_THD_L = 0;
	reg0.bits.LMAP_THD_H = 4095;

	ISP_WR_REG(map, REG_ISP_LMAP_T, LMP_0, reg0.raw);
	ISP_WR_BITS(map, REG_ISP_LMAP_T, LMP_2, LMAP_OUT_SEL, 0);

	return 0;
}

/****************************************************************************
 *	PRE BE SUBSYS
 ****************************************************************************/

void ispblk_fpn_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t fpn = ctx->phys_regs[ISP_BLK_ID_FPN];
	uint16_t i, j;
	uint16_t val;

	if (enable == false)
		return;

	for (j = 0; j < 4; j++) {
		ISP_WR_BITS(fpn, REG_ISP_FPN_T, MEM_SELECT, FPN_MEM_SELECT, j);
		ISP_WR_BITS(fpn, REG_ISP_FPN_T, PROG_CTRL, FPN_WSEL, 0);

		for (i = 0, val = 200; i < 256; ++i, val += 4) {
			ISP_WR_REG(fpn, REG_ISP_FPN_T, PROG_DATA, (0x80000000 | ((val  << 16) | (val))));
		}
	}
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, PROG_CTRL, FPN_RSEL, 0);

	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_0, FPN_STARTPIXEL0, 10);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_0, FPN_WIDTH0, 40);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_1, FPN_STARTPIXEL1, 60);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_1, FPN_WIDTH1, 40);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_2, FPN_STARTPIXEL2, 110);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_2, FPN_WIDTH2, 40);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_3, FPN_STARTPIXEL3, 160);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_3, FPN_WIDTH3, 40);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_4, FPN_STARTPIXEL4, 210);
	ISP_WR_BITS(fpn, REG_ISP_FPN_T, SECTION_INFO_4, FPN_WIDTH4, 40);

	ISP_WR_BITS(fpn, REG_ISP_FPN_T, ENABLE, FPN_ENABLE, enable);
}

void ispblk_dpc_config(struct isp_ctx *ctx, enum ISP_RAW_PATH path, union REG_ISP_DPC_2 reg2)
{
	uintptr_t dpc = (path == ISP_RAW_PATH_LE)
		       ? ctx->phys_regs[ISP_BLK_ID_DPC0]
		       : ctx->phys_regs[ISP_BLK_ID_DPC1];
	union REG_ISP_DPC_2 reg_2;

	reg_2.raw = ISP_RD_REG(dpc, REG_ISP_DPC_T, DPC_2);
	reg_2.bits.DPC_ENABLE = reg2.bits.DPC_ENABLE;
	reg_2.bits.GE_ENABLE = reg2.bits.GE_ENABLE;
	reg_2.bits.DPC_DYNAMICBPC_ENABLE = reg2.bits.DPC_DYNAMICBPC_ENABLE;
	reg_2.bits.DPC_STATICBPC_ENABLE = reg2.bits.DPC_STATICBPC_ENABLE;
	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_2, reg_2.raw);
#if 0
	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM0_IMG1_ADDR, 2048);
	if (ctx->is_work_on_r_tile)
		ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM0_IMG_ADDR_SEL, 1);
	else
		ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM0_IMG_ADDR_SEL, 0);
#endif
}

/**
 * ispblk_dpc_set_static - set defect pixels for static dpc.
 *
 * @param ctx: global settings
 * @param offset: mem-offset for 4k tile
 * @param bps: array of defect pixels. [23:12]-row, [11:0]-col.
 * @param count: number of defect pixels.
 */
void ispblk_dpc_set_static(struct isp_ctx *ctx, enum ISP_RAW_PATH path,
			     uint16_t offset, uint32_t *bps, uint8_t count)
{
	uintptr_t dpc = (path == ISP_RAW_PATH_LE)
		       ? ctx->phys_regs[ISP_BLK_ID_DPC0]
		       : ctx->phys_regs[ISP_BLK_ID_DPC1];
	uint8_t i = 0;

	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 1);

	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_ST_ADDR, 0x80000000 | offset);

	for (i = 0; (i < count) && (i < 2048); ++i)
		ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_W0,
			   0x80000000 | *(bps + i));

	// write 1 fff-fff to end
	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_W0, 0x80ffffff);
	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 0);
}

void ispblk_af_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
	int numx = 17, numy = 15;

	ISP_WR_BITS(sts, REG_ISP_AF_T, KICKOFF, AF_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, DMI_ENABLE, DMI_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_HORIZON_0_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_HORIZON_1_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_VERTICAL_0_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_HLC_ENABLE, 1);
	ISP_WR_REG(sts, REG_ISP_AF_T, BYPASS, !enable);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(sts, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_X, 0x8);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(sts, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_Y, 0x2);
	// block_width >= 15
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_WIDTH, (ctx->img_width - 16) / numx);
	// block_height >= 15
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_HEIGHT, (ctx->img_height - 4) / numy);
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_NUM_X, 17);
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_NUM_Y, 15);

	ISP_WR_REG(sts, REG_ISP_AF_T, HIGH_Y_THRE, 0x258);

	ISP_WR_REG(sts, REG_ISP_AF_T, IMAGE_WIDTH, ctx->img_width - 1);
	ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_WIDTH, ctx->img_width - 1);
	ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_HEIGHT, ctx->img_height - 1);
}

void ispblk_aehist_reset(struct isp_ctx *ctx, int blk_id, enum cvi_isp_raw raw_num)
{
	uintptr_t sts = ctx->phys_regs[blk_id];

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, AE_HIST_GRACE_RESET, 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, AE_HIST_GRACE_RESET, 0);
}

void ispblk_aehist_config(struct isp_ctx *ctx, int blk_id, bool enable)
{
	uintptr_t sts = ctx->phys_regs[blk_id];
	uint8_t num_x = 17, num_y = 15;
	uint8_t sub_window_w = 0, sub_window_h = 0;
	union REG_ISP_AE_HIST_AE_HIST_ENABLE ae_enable;

	ae_enable.raw = ISP_RD_REG(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE);
	ae_enable.bits.AE0_ENABLE	= enable;
	ae_enable.bits.AE0_GAIN_ENABLE	= enable;
	ae_enable.bits.HIST0_ENABLE	= enable;
	ae_enable.bits.HIST0_GAIN_ENABLE = enable;
	if (ctx->is_rgbir_sensor && enable) {
		ae_enable.bits.IR_AE_ENABLE = 1;
		ae_enable.bits.IR_AE_GAIN_ENABLE = 1;
		ae_enable.bits.IR_HIST_ENABLE = 1;
		ae_enable.bits.IR_HIST_GAIN_ENABLE = 1;
	} else {
		ae_enable.bits.IR_AE_ENABLE = 0;
		ae_enable.bits.IR_AE_GAIN_ENABLE = 0;
		ae_enable.bits.IR_HIST_ENABLE = 0;
		ae_enable.bits.IR_HIST_GAIN_ENABLE = 0;
	}
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, ae_enable.raw);

	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, DMI_ENABLE, DMI_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, DMI_ENABLE, IR_DMI_ENABLE,
		(enable & ctx->is_rgbir_sensor));

	if (!enable)
		return;

	sub_window_w = ctx->img_width / num_x;
	sub_window_h = ctx->img_height / num_y;
	if (ctx->is_rgbir_sensor) {
		// Limitation: if ir_ae on, sub_window must be even.
		sub_window_w &= 0xfffe;
		sub_window_h &= 0xfffe;
	}

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, num_x - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, num_y - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_WIDTH, sub_window_w);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, sub_window_h);

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_OFFSETX, 0);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_OFFSETY, 0);

	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, WBG_4, AE0_WBG_RGAIN, 0x384);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, WBG_4, AE0_WBG_GGAIN, 0x44c);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, WBG_5, AE0_WBG_BGAIN, 0x3e8);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, WBG_5, AE1_WBG_BGAIN, 0x3e8);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, WBG_6, AE1_WBG_RGAIN, 0x384);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, WBG_6, AE1_WBG_GGAIN, 0x44c);

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, WBG_7, 0x400);

	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE0_LOCATION, AE_FACE0_OFFSET_X, 0x1);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE0_LOCATION, AE_FACE0_OFFSET_Y, 0x1);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE1_LOCATION, AE_FACE1_OFFSET_X, 0x3);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE1_LOCATION, AE_FACE1_OFFSET_Y, 0x7);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE2_LOCATION, AE_FACE2_OFFSET_X, 0x7);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE2_LOCATION, AE_FACE2_OFFSET_Y, 0x3);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE3_LOCATION, AE_FACE3_OFFSET_X, 0x9);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE3_LOCATION, AE_FACE3_OFFSET_Y, 0x9);

	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE0_SIZE, AE_FACE0_SIZE_MINUS1_X, 0x77);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE0_SIZE, AE_FACE0_SIZE_MINUS1_Y, 0x7f);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE1_SIZE, AE_FACE1_SIZE_MINUS1_X, 0x77);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE1_SIZE, AE_FACE1_SIZE_MINUS1_Y, 0x7f);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE2_SIZE, AE_FACE2_SIZE_MINUS1_X, 0x77);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE2_SIZE, AE_FACE2_SIZE_MINUS1_Y, 0x7f);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE3_SIZE, AE_FACE3_SIZE_MINUS1_X, 0x77);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_FACE3_SIZE, AE_FACE3_SIZE_MINUS1_Y, 0x7f);

	if (ctx->is_rgbir_sensor) {
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_IR_AE_NUMXM1, num_x - 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_IR_AE_NUMYM1, num_y - 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_IR_AE_WIDTH, sub_window_w >> 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_IR_AE_HEIGHT, sub_window_h >> 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_IR_AE_OFFSETX, 0);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_IR_AE_OFFSETY, 0);

		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE0_LOCATION, IR_AE_FACE0_OFFSET_X, 0x1);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE0_LOCATION, IR_AE_FACE0_OFFSET_Y, 0x1);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE1_LOCATION, IR_AE_FACE1_OFFSET_X, 0x3);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE1_LOCATION, IR_AE_FACE1_OFFSET_Y, 0x7);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE2_LOCATION, IR_AE_FACE2_OFFSET_X, 0x7);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE2_LOCATION, IR_AE_FACE2_OFFSET_Y, 0x3);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE3_LOCATION, IR_AE_FACE3_OFFSET_X, 0x9);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE3_LOCATION, IR_AE_FACE3_OFFSET_Y, 0x9);

		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE0_SIZE, IR_AE_FACE0_SIZE_MINUS1_X, 0x77);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE0_SIZE, IR_AE_FACE0_SIZE_MINUS1_Y, 0x7f);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE1_SIZE, IR_AE_FACE1_SIZE_MINUS1_X, 0x77);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE1_SIZE, IR_AE_FACE1_SIZE_MINUS1_Y, 0x7f);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE2_SIZE, IR_AE_FACE2_SIZE_MINUS1_X, 0x77);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE2_SIZE, IR_AE_FACE2_SIZE_MINUS1_Y, 0x7f);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE3_SIZE, IR_AE_FACE3_SIZE_MINUS1_X, 0x77);
		ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, IR_AE_FACE3_SIZE, IR_AE_FACE3_SIZE_MINUS1_Y, 0x7f);
	}

#if 0 //Tuning by ioctl
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, AE0_ENABLE, 1);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, AE0_GAIN_ENABLE, 0);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, HIST0_ENABLE, 1);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, AE1_ENABLE, 1);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, AE1_GAIN_ENABLE, 0);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, HIST1_ENABLE, 1);

	sub_window_w = ctx->img_width / num_x;
	sub_window_h = ctx->img_height / num_y;

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, num_x - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, num_y - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_WIDTH, sub_window_w);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, sub_window_h);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_OFFSETX, 0);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_OFFSETY, 0);

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE1_NUMXM1, num_x - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE1_NUMYM1, num_y - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE1_WIDTH, sub_window_w);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE1_HEIGHT, sub_window_h);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE1_OFFSETX, 0);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE1_OFFSETY, 0);
#endif
}

void ispblk_awb_config(struct isp_ctx *ctx, int blk_id, bool en, enum ISP_AWB_MODE mode)
{
	uintptr_t awb = ctx->phys_regs[blk_id];
	u32 numxm1 = 33, numym1 = 33;

	ISP_WR_BITS(awb, REG_ISP_AWB_T, ENABLE, AWB_ENABLE, en);
	ISP_WR_BITS(awb, REG_ISP_AWB_T, DMI_ENABLE, DMI_ENABLE, en);

	ISP_WR_REG(awb, REG_ISP_AWB_T, STS_NUMXM1, numxm1 - 1);
	ISP_WR_REG(awb, REG_ISP_AWB_T, STS_NUMYM1, numym1 - 1);

	ISP_WR_REG(awb, REG_ISP_AWB_T, STS_WIDTH, (ctx->img_width) / numxm1);
	ISP_WR_REG(awb, REG_ISP_AWB_T, STS_HEIGHT, (ctx->img_height) / numym1);

	ISP_WR_REG(awb, REG_ISP_AWB_T, STS_OFFSETX, 0);
	ISP_WR_REG(awb, REG_ISP_AWB_T, STS_OFFSETY, 0);

	ISP_WR_BITS(awb, REG_ISP_AWB_T, ENABLE, AWB_SOURCE, mode);
}

void ispblk_gms_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_GMS];

	u8 gap_x = 1, gap_y = 1;
	u16 start_x = 0, start_y = 0;
	u16 x_section_size = 64, y_section_size = 64;
#if 0
	x_section_size = (x_section_size > ((ctx->img_width - start_x - gap_x * 2) / 3)) ?
				((ctx->img_width - start_x - gap_x * 2) / 3) : x_section_size;
	y_section_size = (y_section_size > ((ctx->img_height - start_y - gap_y * 2) / 3)) ?
				((ctx->img_height - start_y - gap_y * 2) / 3)  : y_section_size;
#endif
	ISP_WR_BITS(sts, REG_ISP_GMS_T, GMS_ENABLE, GMS_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_GMS_T, DMI_ENABLE, DMI_ENABLE, enable);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_START_X, start_x);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_START_Y, start_y);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_X_SECTION_SIZE, x_section_size - 1);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_Y_SECTION_SIZE, y_section_size - 1);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_X_GAP, gap_x);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_Y_GAP, gap_y);
}

/****************************************************************************
 *	RAW TOP SUBSYS
 ****************************************************************************/

#define F_D (15)
void ispblk_lsc_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSCM0];

	int width = ctx->img_width;
	int height = ctx->img_height;
	int mesh_num = 37;
	int InnerBlkX = mesh_num - 1 - 2;
	int InnerBlkY = mesh_num - 1 - 2;
	int mesh_x_coord_unit = (InnerBlkX * (1 << F_D)) / width;
	int mesh_y_coord_unit = (InnerBlkY * (1 << F_D)) / height;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;
	u32 reg_lsc_ystep = mesh_y_coord_unit + 1;

	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int image_h_in_mesh_unit = height * reg_lsc_ystep;
	int OuterBlkX = InnerBlkX + 2;
	int OuterBlkY = InnerBlkY + 2;
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << F_D) - image_w_in_mesh_unit) / 2;
	u32 reg_lsc_imgy0 = (OuterBlkY * (1 << F_D) - image_h_in_mesh_unit) / 2;

	union REG_ISP_LSC_INTERPOLATION inter_p;


	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_HDR_ENABLE, ctx->is_hdr_on ? 1 : 0);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_ENABLE, en);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, en);

	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_XSTEP, reg_lsc_xstep);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_YSTEP, reg_lsc_ystep);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_IMGX0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_IMGY0, reg_lsc_imgy0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITX0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITY0, reg_lsc_imgy0);

	//Tuning
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_STRENGTH, 0xfff);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_GAIN_BASE, 0);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_GAIN_3P9_0_4P8_1, 1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_RENORMALIZE_ENABLE, 1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_GAIN_BICUBIC_0_BILINEAR_1, 1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_BOUNDARY_INTERPOLATION_MODE, 1);

	inter_p.raw = ISP_RD_REG(lsc, REG_ISP_LSC_T, INTERPOLATION);
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_LF_RANGE = 0x3;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_UP_RANGE = 0x4;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_RT_RANGE = 0x1f;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_DN_RANGE = 0x1c;
	ISP_WR_REG(lsc, REG_ISP_LSC_T, INTERPOLATION, inter_p.raw);
}

void ispblk_fusion_config(struct isp_ctx *ctx, bool enable,
				bool mc_enable, enum ISP_FS_OUT out_sel)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];
	union REG_ISP_FUSION_FS_CTRL_0 reg_ctrl;

	reg_ctrl.raw = ISP_RD_REG(fusion, REG_ISP_FUSION_T, FS_CTRL_0);
	reg_ctrl.bits.FS_ENABLE = enable;
	reg_ctrl.bits.FS_MC_ENABLE = mc_enable;
	reg_ctrl.bits.FS_OUT_SEL = out_sel;
	reg_ctrl.bits.FS_S_MAX = 65535;
	ISP_WR_REG(fusion, REG_ISP_FUSION_T, FS_CTRL_0, reg_ctrl.raw);

	ISP_WR_REG(fusion, REG_ISP_FUSION_T, FS_SE_GAIN, 128);

	ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_LUMA_THD, FS_LUMA_THD_L, 20);
	ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_LUMA_THD, FS_LUMA_THD_H, 4000);
	ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_WGT, FS_WGT_MAX, 255);
	ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_WGT, FS_WGT_MIN, 0);
	ISP_WR_REG(fusion, REG_ISP_FUSION_T, FS_WGT_SLOPE, 554);
}

void ispblk_ltm_d_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	uint16_t i;
	union REG_ISP_LTM_DTONE_CURVE_PROG_DATA reg_data;

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, DTONE_CURVE_PROG_CTRL, DTONE_CURVE_WSEL, sel);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, DTONE_CURVE_CTRL, DTONE_CURVE_ADDR_RST, 1);

	for (i = 0; i < 0x100; i += 2) {
		reg_data.bits.DTONE_CURVE_DATA_E = data[i];
		reg_data.bits.DTONE_CURVE_DATA_O = data[i + 1];
		reg_data.bits.DTONE_CURVE_W = 1;
		ISP_WR_REG(ltm, REG_ISP_LTM_T, DTONE_CURVE_PROG_DATA, reg_data.raw);
	}

	ISP_WR_REG(ltm, REG_ISP_LTM_T, DTONE_CURVE_PROG_MAX, data[0x100]);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, DTONE_CURVE_PROG_CTRL, DTONE_CURVE_RSEL, sel);
}

void ispblk_ltm_b_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	uint16_t i;
	union REG_ISP_LTM_BTONE_CURVE_PROG_DATA reg_data;

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, BTONE_CURVE_PROG_CTRL, BTONE_CURVE_WSEL, sel);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, BTONE_CURVE_CTRL, BTONE_CURVE_ADDR_RST, 1);

	for (i = 0; i < 0x200; i += 2) {
		reg_data.bits.BTONE_CURVE_DATA_E = data[i];
		reg_data.bits.BTONE_CURVE_DATA_O = data[i + 1];
		reg_data.bits.BTONE_CURVE_W = 1;
		ISP_WR_REG(ltm, REG_ISP_LTM_T, BTONE_CURVE_PROG_DATA, reg_data.raw);
	}

	ISP_WR_REG(ltm, REG_ISP_LTM_T, BTONE_CURVE_PROG_MAX, data[0x200]);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, BTONE_CURVE_PROG_CTRL, BTONE_CURVE_RSEL, sel);
}

void ispblk_ltm_g_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	uint16_t i;
	union REG_ISP_LTM_GLOBAL_CURVE_PROG_DATA reg_data;

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, GLOBAL_CURVE_PROG_CTRL, GLOBAL_CURVE_WSEL, sel);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, GLOBAL_CURVE_CTRL, GLOBAL_CURVE_ADDR_RST, 1);

	for (i = 0; i < 0x300; i += 2) {
		reg_data.bits.GLOBAL_CURVE_DATA_E = data[i];
		reg_data.bits.GLOBAL_CURVE_DATA_O = data[i + 1];
		reg_data.bits.GLOBAL_CURVE_W = 1;
		ISP_WR_REG(ltm, REG_ISP_LTM_T, GLOBAL_CURVE_PROG_DATA, reg_data.raw);
	}

	ISP_WR_REG(ltm, REG_ISP_LTM_T, GLOBAL_CURVE_PROG_MAX, data[0x300]);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, GLOBAL_CURVE_PROG_CTRL, GLOBAL_CURVE_RSEL, sel);
}

void ispblk_ltm_enable(struct isp_ctx *ctx, bool en)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_TOP_CTRL, LTM_ENABLE, en);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_EE_CTRL_0, LTM_EE_ENABLE, en);
}

/**
 * ispblk_ltm_config - Local-Tone-Mapping configure
 *
 * @param ctx: context of settings
 * @param dehn_en: dtone enhance enable
 * @param dlce_en: dtone lce enable
 * @param behn_en: btone suppressed enable
 * @param blce_en: btone lce enable
 * @param hblk_size: equals to lmap's wbits/4
 * @param vblk_size: equals to lmap's hbits/4
 */
void ispblk_ltm_config(struct isp_ctx *ctx, bool dehn_en,
		       bool dlce_en, bool behn_en, bool blce_en)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	//uintptr_t map = ctx->phys_regs[ISP_BLK_ID_LMP0];
	union REG_ISP_LTM_TOP_CTRL reg_top;
	union REG_ISP_LTM_BLK_SIZE reg_blk;
	uint32_t img_width = (ctx->is_tile)
			   ? ctx->tile_cfg.r_out.end + 1
			   : ctx->img_width;
	uint8_t i;
	uint16_t lscaler_filter[] = {
		   0, 1024,    0,    0, 8188, 1024,    4,    0, 8184, 1023,    8,    1, 8181, 1023,   13, 8191,
		8177, 1022,   18, 8191, 8174, 1020,   23, 8191, 8170, 1019,   28, 8191, 8167, 1017,   34, 8190,
		8164, 1014,   40, 8190, 8161, 1012,   46, 8189, 8158, 1009,   52, 8189, 8155, 1006,   58, 8189,
		8153, 1003,   65, 8187, 8150,  999,   72, 8187, 8148,  995,   78, 8187, 8145,  991,   86, 8186,
		8143,  987,   93, 8185, 8141,  982,  101, 8184, 8139,  978,  108, 8183, 8137,  973,  116, 8182,
		8135,  967,  124, 8182, 8133,  962,  132, 8181, 8132,  956,  141, 8179, 8130,  950,  149, 8179,
		8129,  944,  158, 8177, 8127,  938,  167, 8176, 8126,  931,  176, 8175, 8125,  925,  185, 8173,
		8124,  918,  194, 8172, 8123,  910,  203, 8172, 8122,  903,  213, 8170, 8121,  896,  222, 8169,
		8120,  888,  232, 8168, 8119,  880,  242, 8167, 8119,  872,  252, 8165, 8118,  864,  262, 8164,
		8118,  856,  272, 8162, 8117,  847,  282, 8162, 8117,  839,  292, 8160, 8117,  830,  303, 8158,
		8116,  821,  313, 8158, 8116,  812,  324, 8156, 8116,  803,  334, 8155, 8116,  793,  345, 8154,
		8116,  784,  356, 8152, 8116,  774,  366, 8152, 8116,  765,  377, 8150, 8117,  755,  388, 8148,
		8117,  745,  399, 8147, 8117,  735,  410, 8146, 8118,  725,  421, 8144, 8118,  715,  432, 8143,
		8119,  704,  443, 8142, 8119,  694,  454, 8141, 8120,  684,  465, 8139, 8120,  673,  476, 8139,
		8121,  663,  487, 8137, 8122,  652,  498, 8136, 8123,  641,  510, 8134, 8123,  631,  521, 8133,
		8124,  620,  532, 8132, 8125,  609,  543, 8131, 8126,  598,  554, 8130, 8127,  587,  565, 8129,
		8128,  576,  576, 8128,
	};

	reg_top.raw = ISP_RD_REG(ltm, REG_ISP_LTM_T, LTM_TOP_CTRL);
	reg_top.bits.DTONE_EHN_EN = dehn_en;
	reg_top.bits.BTONE_EHN_EN = behn_en;
	reg_top.bits.DARK_LCE_EN = dlce_en;
	reg_top.bits.BRIT_LCE_EN = blce_en;
	reg_top.bits.BAYER_ID = ctx->rgb_color_mode[ctx->cam_id];
	ISP_WR_REG(ltm, REG_ISP_LTM_T, LTM_TOP_CTRL, reg_top.raw);

	reg_blk.bits.HORZ_BLK_SIZE = g_lmp_cfg[ISP_PRERAW_A].post_w_bit - 3;
	reg_blk.bits.BLK_WIDTHM1 = UPPER(img_width, g_lmp_cfg[ISP_PRERAW_A].post_w_bit) - 1;
	reg_blk.bits.BLK_HEIGHTM1 = UPPER(ctx->img_height, g_lmp_cfg[ISP_PRERAW_A].post_h_bit) - 1;
	reg_blk.bits.VERT_BLK_SIZE = g_lmp_cfg[ISP_PRERAW_A].post_h_bit - 3;
	ISP_WR_REG(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE, reg_blk.raw);

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_EE_CTRL_0, LTM_EE_ENABLE, 0);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_EE_CTRL_0, LTM_EE_DETAIL_MODE, 1);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_EE_CTRL_1, LTM_EE_CLIP_MIN, 0x1801);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_EE_CTRL_1, LTM_EE_CLIP_MAX, 2047);

	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_00, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_01, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_02, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_03, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_04, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_05, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_06, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_07, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_DETAIL_LUT_08, 0x20);

	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_00, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_01, 0x20202020);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_02, 0x28262422);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_03, 0x302E2C2A);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_04, 0x2C2D2E2F);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_05, 0x28292A2B);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_06, 0x24252627);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_07, 0x20212223);
	ISP_WR_REG(ltm, REG_ISP_LTM_T, EE_GAIN_LUT_08, 0x20);

	//program the resize coefficient table
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, RESIZE_COEFF_PROG_CTRL, RESIZE_COEFF_BUFF_WEN, 1);
	for (i = 0; i < 65; ++i) {
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, RESIZE_COEFF_PROG_CTRL, RESIZE_COEFF_BUFF_ADDR, i);
		ISP_WR_REG(ltm, REG_ISP_LTM_T, RESIZE_COEFF_WDATA_0,
						((lscaler_filter[i * 4] & 0x1FFF) |
						((lscaler_filter[i * 4 + 1] & 0x1FFF) << 13)));
		ISP_WR_REG(ltm, REG_ISP_LTM_T, RESIZE_COEFF_WDATA_1,
						((lscaler_filter[i * 4 + 2] & 0x1FFF) |
						((lscaler_filter[i * 4 + 3] & 0x1FFF) << 13)));
	}
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, RESIZE_COEFF_PROG_CTRL, RESIZE_COEFF_BUFF_WEN, 0);
}

static void _manr_init(struct isp_ctx *ctx)
{
	uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MANR];
	union REG_ISP_MMAP_04 reg_04;
	union REG_ISP_MMAP_08 reg_08;
	union REG_ISP_MMAP_38 reg_38;

	uint16_t data[] = {
		264,  436,  264,   60,	262,  436,  266,   60,	260,  435,  268,   61,	258,  435,  270,   61,
		255,  434,  272,   63,  253,  434,  274,   63,  251,  433,  275,   65,  249,  433,  277,   65,
		246,  432,  279,   67,  244,  432,  281,   67,  242,  431,  283,   68,  240,  431,  285,   68,
		237,  430,  286,   71,  235,  429,  288,   72,  233,  429,  290,   72,  231,  428,  292,   73,
		229,  427,  294,   74,  227,  427,  296,   74,  224,  426,  297,   77,  222,  425,  299,   78,
		220,  424,  301,   79,  218,  424,  303,   79,  216,  423,  305,   80,  214,  422,  306,   82,
		212,  421,  308,   83,  210,  420,  310,   84,  208,  419,  312,   85,  206,  419,  313,   86,
		204,  418,  315,   87,  202,  417,  317,   88,  199,  416,  319,   90,  197,  415,  321,   91,
		195,  414,  322,   93,  194,  413,  324,   93,  192,  412,  326,   94,  190,  411,  328,   95,
		188,  410,  329,   97,  186,  409,  331,   98,  184,  408,  333,   99,  182,  407,  334,  101,
		180,  405,  336,  103,  178,  404,  338,  104,  176,  403,  340,  105,  174,  402,  341,  107,
		172,  401,  343,  108,  171,  400,  345,  108,  169,  398,  346,  111,  167,  397,  348,  112,
		165,  396,  349,  114,  163,  395,  351,  115,  161,  393,  353,  117,  160,  392,  354,  118,
		158,  391,  356,  119,  156,  390,  358,  120,  154,  388,  359,  123,  153,  387,  361,  123,
		151,  386,  362,  125,  149,  384,  364,  127,  148,  383,  365,  128,  146,  381,  367,  130,
		144,  380,  368,  132,  143,  379,  370,  132,  141,  377,  371,  135,  139,  376,  373,  136,
		138,  374,  374,  138,
	};

	uint8_t i = 0;

	reg_04.bits.MMAP_0_LPF_00 = 3;
	reg_04.bits.MMAP_0_LPF_01 = 4;
	reg_04.bits.MMAP_0_LPF_02 = 3;
	reg_04.bits.MMAP_0_LPF_10 = 4;
	reg_04.bits.MMAP_0_LPF_11 = 4;
	reg_04.bits.MMAP_0_LPF_12 = 4;
	reg_04.bits.MMAP_0_LPF_20 = 3;
	reg_04.bits.MMAP_0_LPF_21 = 4;
	reg_04.bits.MMAP_0_LPF_22 = 3;
	ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_04, reg_04.raw);

	reg_08.bits.MMAP_0_MAP_CORING = 0;
	reg_08.bits.MMAP_0_MAP_GAIN   = 64;
	reg_08.bits.MMAP_0_MAP_THD_L  = 64; /* for imx327 tuning */
	reg_08.bits.MMAP_0_MAP_THD_H  = 255;
	ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_08, reg_08.raw);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_0C,
		    MMAP_0_LUMA_ADAPT_LUT_IN_0, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_0C,
		    MMAP_0_LUMA_ADAPT_LUT_IN_1, 600);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_10,
		    MMAP_0_LUMA_ADAPT_LUT_IN_2, 1500);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_10,
		    MMAP_0_LUMA_ADAPT_LUT_IN_3, 2500);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_0, 63);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_1, 48);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_2, 8);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_3, 2);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_18,
		    MMAP_0_LUMA_ADAPT_LUT_SLOPE_0, -27);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_18,
		    MMAP_0_LUMA_ADAPT_LUT_SLOPE_1, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_1C,
		    MMAP_0_LUMA_ADAPT_LUT_SLOPE_2, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_1C, MMAP_0_MAP_DSHIFT_BIT, 5);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_0, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_1, 45);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_2, 90);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_3, 255);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_0, 6);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_1, 10);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_2, 9);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_3, 2);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_28,
		    MMAP_0_IIR_PRTCT_LUT_SLOPE_0, 12);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_28,
		    MMAP_0_IIR_PRTCT_LUT_SLOPE_1, -4);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_2C,
		    MMAP_0_IIR_PRTCT_LUT_SLOPE_2, -4);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_70, MMAP_0_GAIN_RATIO_R, 4096);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_70, MMAP_0_GAIN_RATIO_G, 4096);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_74, MMAP_0_GAIN_RATIO_B, 4096);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_78, MMAP_0_NS_SLOPE_R, 5);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_78, MMAP_0_NS_SLOPE_G, 4);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_7C, MMAP_0_NS_SLOPE_B, 6);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_80, MMAP_0_NS_LUMA_TH0_R, 16);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_80, MMAP_0_NS_LUMA_TH0_G, 16);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_84, MMAP_0_NS_LUMA_TH0_B, 16);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_84, MMAP_0_NS_LOW_OFFSET_R, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_88, MMAP_0_NS_LOW_OFFSET_G, 2);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_88, MMAP_0_NS_LOW_OFFSET_B, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_8C, MMAP_0_NS_HIGH_OFFSET_R, 724);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_8C, MMAP_0_NS_HIGH_OFFSET_G, 724);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_90, MMAP_0_NS_HIGH_OFFSET_B, 724);

	reg_38.bits.MMAP_1_MAP_CORING = 0;
	ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_38, reg_38.raw);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_A0, MMAP_1_GAIN_RATIO_R, 4096);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_A0, MMAP_1_GAIN_RATIO_G, 4096);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_A4, MMAP_1_GAIN_RATIO_B, 4096);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_A8, MMAP_1_NS_SLOPE_R, 5 * 4);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_A8, MMAP_1_NS_SLOPE_G, 4 * 4);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_AC, MMAP_1_NS_SLOPE_B, 6 * 4);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_B0, MMAP_1_NS_LUMA_TH0_R, 16);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_B0, MMAP_1_NS_LUMA_TH0_G, 16);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_B4, MMAP_1_NS_LUMA_TH0_B, 16);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_B4, MMAP_1_NS_LOW_OFFSET_R, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_B8, MMAP_1_NS_LOW_OFFSET_G, 2);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_B8, MMAP_1_NS_LOW_OFFSET_B, 0);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_BC, MMAP_1_NS_HIGH_OFFSET_R, 724);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_BC, MMAP_1_NS_HIGH_OFFSET_G, 724);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_C0, MMAP_1_NS_HIGH_OFFSET_B, 724);

	for (i = 0; i < ARRAY_SIZE(data) / 4; ++i) {
		uint64_t val = 0;

		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_6C, SRAM_WEN, 0);
		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_6C, SRAM_WADD, i);

		val = ((uint64_t)data[i * 4] | (uint64_t)data[i * 4 + 1] << 13 |
			(uint64_t)data[i * 4 + 2] << 26 | (uint64_t)data[i * 4 + 3] << 39);
		ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_64, val & 0xffffffff);
		ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_68, val >> 32);

		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_6C, SRAM_WEN, 1);
	}

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_6C, SRAM_WEN, 0);
}

void ispblk_manr_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MANR];
	union REG_ISP_MMAP_00 reg_00;
	union REG_ISP_MMAP_C4 reg_c4;
	uint8_t w_bit = g_w_bit[ISP_PRERAW_A];
	uint8_t h_bit = g_h_bit[ISP_PRERAW_A];
	uint32_t img_width = (ctx->is_tile)
			   ? ctx->tile_cfg.r_out.end + 1
			   : ctx->img_width;
	uint8_t dma_enable;

	//Init once for tuning
	_manr_init(ctx);

	if (_is_all_online(ctx)) //all online mode
		dma_enable = 0xae;
	else { // fe->be->dram->post or fe->dram->be->post
		if (ctx->is_hdr_on) // hdr mode
			dma_enable = 0xa0;
		else // linear mode
			dma_enable = 0x0a;
	}

	reg_00.raw = ISP_RD_REG(blk, REG_ISP_MMAP_T, REG_00);
	reg_00.bits.ON_THE_FLY = _is_all_online(ctx);
	reg_00.bits.BYPASS = (en) ? 0 : 1;
	reg_00.bits.MMAP_1_ENABLE = (ctx->is_hdr_on) ? 1 : 0;
	//reg_00.bits.INTER_1_EN = 0;
	//reg_00.bits.INTER_2_EN = 0;
	//reg_00.bits.INTER_4_EN = 0;
	ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_00, reg_00.raw);

	if (!en) {
		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, 0xff);
		return;
	}

	//ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_30, IMG_WIDTHM1_SW, img_width - 1);
	//ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_30, IMG_HEIGHTM1_SW, ctx->img_height - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT, w_bit);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT, h_bit);

	reg_c4.raw = 0;
	reg_c4.bits.IMG_WIDTH_CROP = ((UPPER((UPPER(img_width, w_bit) * 48), 7) << 7) + 47) / 48 - 1;
	reg_c4.bits.IMG_HEIGHT_CROP = UPPER(ctx->img_height, h_bit) - 1;
	reg_c4.bits.CROP_ENABLE = 1;
	ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_C4, reg_c4.raw);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_C8, CROP_W_END, UPPER(img_width, w_bit) - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_CC, CROP_H_END, UPPER(ctx->img_height, h_bit) - 1);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, IMG_WIDTH_CROP_SCALAR, img_width - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, IMG_HEIGHT_CROP_SCALAR, ctx->img_height - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, CROP_ENABLE_SCALAR, ctx->is_tile);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, dma_enable);
}

static void _bnr_init(struct isp_ctx *ctx)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];
	uint8_t intensity_sel[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t weight_lut[256] = {
		31, 16, 8,  4,  2,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	};
	uint8_t lsc_lut[32] = {
		32, 32, 32, 32, 32, 32, 32, 33, 33, 34, 34, 35, 36, 37, 38, 39,
		40, 41, 43, 44, 45, 47, 49, 51, 53, 55, 57, 59, 61, 64, 66, 69,
	};
	uint16_t i = 0;

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, INDEX_CLR, BNR_INDEX_CLR, 1);
	for (i = 0; i < ARRAY_SIZE(intensity_sel); ++i)
		ISP_WR_REG(bnr, REG_ISP_BNR_T, INTENSITY_SEL, intensity_sel[i]);
	for (i = 0; i < ARRAY_SIZE(weight_lut); ++i)
		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_LUT, weight_lut[i]);
	for (i = 0; i < ARRAY_SIZE(lsc_lut); ++i)
		ISP_WR_REG(bnr, REG_ISP_BNR_T, LSC_LUT, lsc_lut[i]);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, SHADOW_RD_SEL,
		    SHADOW_RD_SEL, 1);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, OUT_SEL,
		    BNR_OUT_SEL, ISP_BNR_OUT_BYPASS);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, STRENGTH_MODE,
		    BNR_STRENGTH_MODE, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_0,
		    BNR_WEIGHT_INTRA_0, 6);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_1,
		    BNR_WEIGHT_INTRA_1, 6);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_2,
		    BNR_WEIGHT_INTRA_2, 6);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_NORM_1,
		    BNR_WEIGHT_NORM_1, 7);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_NORM_2,
		    BNR_WEIGHT_NORM_2, 5);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NEIGHBOR_MAX,
		    BNR_NEIGHBOR_MAX, 1);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, RES_K_SMOOTH,
		    BNR_RES_K_SMOOTH, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, RES_K_TEXTURE,
		    BNR_RES_K_TEXTURE, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, VAR_TH, BNR_VAR_TH, 128);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_SM, BNR_WEIGHT_SM, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_V, BNR_WEIGHT_V, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_H, BNR_WEIGHT_H, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_D45, BNR_WEIGHT_D45, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_D135, BNR_WEIGHT_D135, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, LSC_RATIO, BNR_LSC_RATIO, 15);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_B,
		    BNR_NS_SLOPE_B, 135);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_GB,
		    BNR_NS_SLOPE_GB, 106);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_GR,
		    BNR_NS_SLOPE_GR, 106);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_R,
		    BNR_NS_SLOPE_R, 127);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_B,
		    BNR_NS_OFFSET0_B, 177);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_GB,
		    BNR_NS_OFFSET0_GB, 169);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_GR,
		    BNR_NS_OFFSET0_GR, 169);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_R,
		    BNR_NS_OFFSET0_R, 182);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_B,
		    BNR_NS_OFFSET1_B, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_GB,
		    BNR_NS_OFFSET1_GB, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_GR,
		    BNR_NS_OFFSET1_GR, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_R,
		    BNR_NS_OFFSET1_R, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_B,
		    BNR_NS_LUMA_TH_B, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_GB,
		    BNR_NS_LUMA_TH_GB, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_GR,
		    BNR_NS_LUMA_TH_GR, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_R,
		    BNR_NS_LUMA_TH_R, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_GAIN, BNR_NS_GAIN, 0);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, LSC_EN, BNR_LSC_EN, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NORM_FACTOR, BNR_NORM_FACTOR, 3322);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, LSC_STRENTH, BNR_LSC_STRENTH, 128);
}

void ispblk_bnr_config(struct isp_ctx *ctx, enum ISP_BNR_OUT out_sel,
		      bool lsc_en, uint8_t ns_gain, uint8_t str)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];

	// Init once tuning
	_bnr_init(ctx);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, OUT_SEL, BNR_OUT_SEL, out_sel);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, LSC_EN, BNR_LSC_EN, lsc_en);

	//ISP_WO_BITS(bnr, REG_ISP_BNR_T, HSIZE, BNR_HSIZE, ctx->img_width);
	//ISP_WO_BITS(bnr, REG_ISP_BNR_T, VSIZE, BNR_VSIZE, ctx->img_height);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_GAIN, BNR_NS_GAIN, ns_gain);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, STRENGTH_MODE, BNR_STRENGTH_MODE, str);
}

void ispblk_cfa_config(struct isp_ctx *ctx)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];
	union REG_ISP_CFA_0 reg_0;

	reg_0.raw = ISP_RD_REG(cfa, REG_ISP_CFA_T, REG_0);
	reg_0.bits.CFA_SHDW_SEL = 1;
	reg_0.bits.CFA_ENABLE   = 1;
	reg_0.bits.CFA_FCR_ENABLE = 1;
	reg_0.bits.CFA_MOIRE_ENABLE = 1;
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_0, reg_0.raw);

	//ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_2, ((ctx->img_height - 1) << 16) | (ctx->img_width - 1));

	// field update or reg update?
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_3, 0x019001e0);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_4, 0x00280028);
	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_4_1, CFA_EDGE_TOL, 0x80);
	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_5, CFA_GHP_THD, 0xe00);

	//fcr
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_6, 0x06400190);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_7, 0x04000300);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_8, 0x0bb803e8);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_9, 0x03ff0010);

	//ghp lut
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_0, 0x08080808);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_1, 0x0a0a0a0a);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_2, 0x0c0c0c0c);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_3, 0x0e0e0e0e);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_4, 0x10101010);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_5, 0x14141414);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_6, 0x18181818);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, GHP_LUT_7, 0x1f1f1c1c);

	//moire
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_10, 0x40ff);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_11, 0);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_12, 0x3a021c);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_13, 0x60ff00);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_14, 0xff000022);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_15, 0xff00);
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_16, 0x300150);
}

void ispblk_rgbcac_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t rgbcac = ctx->phys_regs[ISP_BLK_ID_CFA];

	ISP_WR_BITS(rgbcac, REG_ISP_CFA_T, REG_28, RGBCAC_ENABLE, en);

	ISP_WR_BITS(rgbcac, REG_ISP_CFA_T, REG_29, RGBCAC_VAR_TH, 64);
	ISP_WR_BITS(rgbcac, REG_ISP_CFA_T, REG_29, RGBCAC_PURPLE_TH, 95);
}

/****************************************************************************
 *	RGB TOP SUBSYS
 ****************************************************************************/

void ispblk_dci_config(struct isp_ctx *ctx, bool en, uint16_t *lut)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	union REG_ISP_DCI_GAMMA_PROG_CTRL dci_gamma_ctrl;
	union REG_ISP_DCI_GAMMA_PROG_DATA dci_gamma_data;
	u16 sel = 0, i = 0;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_ENABLE, en);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_HIST_ENABLE, en);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_MAP_ENABLE, 0);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_PER1SAMPLE_ENABLE, en);
	ISP_WR_REG(dci, REG_ISP_DCI_T, DCI_DEMO_MODE, 0);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, en);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL);
	dci_gamma_ctrl.bits.GAMMA_WSEL = sel;
	dci_gamma_ctrl.bits.GAMMA_PROG_EN = 1;
	dci_gamma_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		dci_gamma_data.bits.GAMMA_DATA_E = lut[i];
		dci_gamma_data.bits.GAMMA_DATA_O = lut[i + 1];
		dci_gamma_data.bits.GAMMA_W = 1;
		ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_DATA, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_RSEL, sel);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_ccm_config(struct isp_ctx *ctx, enum ISP_BLK_ID_T blk_id, bool en, struct isp_ccm_cfg *cfg)
{
	uintptr_t ccm = ctx->phys_regs[blk_id];

	ISP_WR_BITS(ccm, REG_ISP_CCM_T, CCM_CTRL, CCM_SHDW_SEL, 1);
	ISP_WR_BITS(ccm, REG_ISP_CCM_T, CCM_CTRL, CCM_ENABLE, en);

	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_22, cfg->coef[2][2]);
}

void ispblk_hist_edge_v_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t hist_edge_v = ctx->phys_regs[ISP_BLK_ID_HIST_EDGE_V];

	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_ENABLE, en);
	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_LUMA_MODE, en);
	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, en);

	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, OFFSETX, HIST_EDGE_V_OFFSETX, 0);
	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, OFFSETY, HIST_EDGE_V_OFFSETY, 0);
}

void ispblk_dhz_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DHZ];

	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_BYPASS, DEHAZE_ENABLE, en);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_BYPASS, DEHAZE_SKIN_LUT_ENABLE, 1);

	//ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_4, IMG_WD, ctx->img_width - 1);
	//ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_4, IMG_HT, ctx->img_height - 1);
}

void ispblk_gamma_config(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_GAMMA];
	uint16_t i;
	union REG_ISP_GAMMA_PROG_DATA reg_data;
	union REG_ISP_GAMMA_PROG_CTRL prog_ctrl;

	prog_ctrl.raw = ISP_RD_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL);
	prog_ctrl.bits.GAMMA_WSEL    = sel;
	prog_ctrl.bits.GAMMA_PROG_EN = 1;
	prog_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, prog_ctrl.raw);

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_ADDR, 0);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_W, 1);

	for (i = 0; i < 256; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.GAMMA_DATA_E = data[i];
		reg_data.bits.GAMMA_DATA_O = data[i + 1];
		reg_data.bits.GAMMA_W = 1;
		ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
	}

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_RSEL, sel);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_GAMMA];

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_CTRL, GAMMA_ENABLE, enable);
}

void ispblk_preyee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t preyee = ctx->phys_regs[ISP_BLK_ID_PREYEE];
	union REG_ISP_EE_00  reg_0;
	union REG_ISP_EE_04  reg_4;

	reg_0.raw = ISP_RD_REG(preyee, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = en;
	reg_0.bits.EE_TOTAL_CORING = 0x20;
	reg_0.bits.EE_TOTAL_GAIN = 0xff;
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_00, reg_0.raw);

	reg_4.raw = ISP_RD_REG(preyee, REG_ISP_EE_T, REG_04);
	reg_4.bits.EE_TOTAL_OSHTTHRD  = 0x10;
	reg_4.bits.EE_TOTAL_USHTTHRD  = 0x10;
	reg_4.bits.EE_PRE_PROC_GAIN   = 0x24;
	reg_4.bits.EE_PRE_PROC_MODE   = 1;
	reg_4.bits.EE_PRE_PROC_ENABLE = 1;
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_04, reg_0.raw);

	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_0C, 0x103);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_10, 0x2000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_14, 0x00400020);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_18, 0x102018f0);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_1C, 0x00180018);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_20, 0x00100010);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_24, 0x00100010);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_28, 0x00100010);

	//reg_ee_dircal_dgr4_filter000
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_30, 0x01f00000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_34, 0x000001e0);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_38, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_3C, 0x00200000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_40, 0x00000040);

	//reg_ee_dircal_dgr4_filter045
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_58, 0x01e801f0);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_5C, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_60, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_64, 0x00200000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_68, 0x00000040);

	//reg_ee_dircal_dgr4_filter090
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_6C, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_70, 0x01f00000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_74, 0x00200000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_78, 0x000001e0);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_7C, 0x00000040);

	//reg_ee_dircal_dgr4_filter135
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_80, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_84, 0x01e80000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_88, 0x002001f0);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_8C, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_90, 0x00000040);

	//reg_ee_dircal_dgr4_filternod
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_94, 0x01fc01fc);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_98, 0x01e80000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_9C, 0x008001f8);

	ISP_WR_BITS(preyee, REG_ISP_EE_T, REG_1C4, EE_SHTCTRL_OSHTGAIN, 0x10);
	ISP_WR_BITS(preyee, REG_ISP_EE_T, REG_1C4, EE_SHTCTRL_USHTGAIN, 0x10);

	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_1C8, 0x00000000);

	//reg_ee_motion_lut_in
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_1CC, 0x20ffffff);

	//reg_ee_motion_lut_out
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_1D0, 0xffffffff);

	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_1D4, 0x00000000);
	ISP_WR_REG(preyee, REG_ISP_EE_T, REG_1D8, 0x00000000);

	//ISP_WR_BITS(preyee, REG_ISP_EE_T, REG_1BC, IMG_WIDTH, ctx->img_width - 1);
	//ISP_WR_BITS(preyee, REG_ISP_EE_T, REG_1BC, IMG_HEIGHT, ctx->img_height - 1);
}

void ispblk_clut_partial_update(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_clut_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	union REG_ISP_CLUT_CTRL      ctrl;
	union REG_ISP_CLUT_PROG_DATA prog_data;
	u32 i = 0;

	if (_is_all_online(ctx) && cfg->update_length >= 256)
		cfg->update_length = 256;
	else if (cfg->update_length >= 1024)
		cfg->update_length = 1024;

	ctrl.raw = ISP_RD_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL);
	ctrl.bits.PROG_EN = 1;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);

	for (; i < cfg->update_length; i++) {
		ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_ADDR, cfg->lut[i][0]);

		prog_data.raw			= 0;
		prog_data.bits.SRAM_WDATA	= cfg->lut[i][1];
		prog_data.bits.SRAM_WR		= 1;
		ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_DATA, prog_data.raw);
	}

	ctrl.bits.CLUT_ENABLE = cfg->enable;
	ctrl.bits.PROG_EN = 0;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);
}

void ispblk_clut_config(struct isp_ctx *ctx, bool en,
				int16_t *r_lut, int16_t *g_lut, int16_t *b_lut)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	uint16_t r_idx, g_idx, b_idx;
	union REG_ISP_CLUT_CTRL      ctrl;
	union REG_ISP_CLUT_PROG_ADDR prog_addr;
	union REG_ISP_CLUT_PROG_DATA prog_data;
	u32 idx = 0;

	ctrl.raw = ISP_RD_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL);
	ctrl.bits.PROG_EN     = 1;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);

	for (b_idx = 0; b_idx < 17; b_idx++) {
		for (g_idx = 0; g_idx < 17; g_idx++) {
			for (r_idx = 0; r_idx < 17; r_idx++) {
				idx = b_idx * 289 + g_idx * 17 + r_idx;

				prog_addr.raw = 0;
				prog_addr.bits.SRAM_R_IDX = r_idx;
				prog_addr.bits.SRAM_G_IDX = g_idx;
				prog_addr.bits.SRAM_B_IDX = b_idx;
				ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_ADDR, prog_addr.raw);

				prog_data.raw		  = 0;
				prog_data.bits.SRAM_WDATA = b_lut[idx] + (g_lut[idx] << 10) + (r_lut[idx] << 20);
				prog_data.bits.SRAM_WR	  = 1;
				ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_DATA, prog_data.raw);
			}
		}
	}

	ctrl.bits.CLUT_ENABLE = en;
	ctrl.bits.PROG_EN     = 0;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);
}

/**
 * ispblk_rgbdither_config - setup rgb dither.
 *
 * @param ctx: global settings
 * @param en: rgb dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en,
			    bool histidx_en, bool fmnum_en)
{
	uintptr_t rgbdither = ctx->phys_regs[ISP_BLK_ID_RGBDITHER];
	union REG_ISP_RGBDITHER_RGB_DITHER reg;

	reg.raw = 0;
	reg.bits.RGB_DITHER_ENABLE = en;
	reg.bits.RGB_DITHER_MOD_EN = mod_en;
	reg.bits.RGB_DITHER_HISTIDX_EN = histidx_en;
	reg.bits.RGB_DITHER_FMNUM_EN = fmnum_en;
	reg.bits.RGB_DITHER_SHDW_SEL = 1;
	reg.bits.CROP_WIDTHM1 = ctx->img_width - 1;
	reg.bits.CROP_HEIGHTM1 = ctx->img_height - 1;

	ISP_WR_REG(rgbdither, REG_ISP_RGBDITHER_T, RGB_DITHER, reg.raw);
}

void ispblk_cacp_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	u16 mode = 1, i = 0;
	union REG_ISP_RGB_15 wdata;

	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_ENABLE, en);
	// 0 CA mode, 1 CP mode
	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_MODE, mode);

	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_ISO_RATIO, 64);

	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_MEM_SW_MODE, 1);

	if (mode == 0) {
		for (i = 0; i < sizeof(ca_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = ca_y_lut[i];
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_ISP_RGB_T, REG_15, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < sizeof(cp_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = ((cp_v_lut[i]) | (cp_u_lut[i] << 8) | (cp_y_lut[i] << 16));
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_ISP_RGB_T, REG_15, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_MEM_SW_MODE, 0);
}

void ispblk_csc_config(struct isp_ctx *ctx)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_R2Y4];
	uint8_t enable = !(ctx->is_yuv_sensor);

	ISP_WR_BITS(csc, REG_ISP_CSC_T, REG_0, CSC_ENABLE, enable);
}

void ispblk_ir_merge_config(struct isp_ctx *ctx)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	bool enable = ctx->is_rgbir_sensor;
	bool bit_mode = (ctx->sensor_bitdepth != 8) ? 0:1;

	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_14, IRM_ENABLE, enable);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_10, IR_DMI_ENABLE, enable);
	if (enable) {
		uint32_t dmi_num;

		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_14, IR_BLENDING_WGT, 0); //to-do

		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_10, IR_CROP_ENABLE, 1);
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_12, IR_CROP_W_STR, 0);
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_12, IR_CROP_W_END, ctx->img_width - 1);
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_13, IR_CROP_H_STR, 0);
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_13, IR_CROP_H_END, ctx->img_height - 1);

		 // 0:12bit, 1:8bit, sync with preraw_be[IR_BIT_MODE]
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_10, IR_BIT_MODE, bit_mode);

		if (bit_mode == 0) {
			dmi_num = VIP_ALIGN((ctx->img_width*3/2))>>1;
			ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_11, IR_IMG_WIDTH, (dmi_num + 2)/3*4 - 1);
		} else {
			dmi_num = VIP_ALIGN(ctx->img_width)>>1;
			ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_11, IR_IMG_WIDTH, (dmi_num + 2)/3*6 - 1);
		}
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_10, IR_DMI_NUM_SW, dmi_num);
		ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_11, IR_IMG_HEIGHT, ctx->img_height - 1);
	}
}

/****************************************************************************
 *	YUV TOP SUBSYS
 ****************************************************************************/

/**
 * ispblk_yuvdither_config - setup yuv dither.
 *
 * @param ctx: global settings
 * @param sel: y(0)/uv(1)
 * @param en: dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
int ispblk_yuvdither_config(struct isp_ctx *ctx, uint8_t sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en)
{
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUVDITHER];

	if (sel == 0) {
		union REG_ISP_YUVDITHER_Y_DITHER reg;

		reg.raw = 0;
		reg.bits.Y_DITHER_EN = en;
		reg.bits.Y_DITHER_MOD_EN = mod_en;
		reg.bits.Y_DITHER_HISTIDX_EN = histidx_en;
		reg.bits.Y_DITHER_FMNUM_EN = fmnum_en;
		reg.bits.Y_DITHER_SHDW_SEL = 1;
		reg.bits.Y_DITHER_WIDTHM1 = ctx->img_width - 1;
		reg.bits.Y_DITHER_HEIGHTM1 = ctx->img_height - 1;

		ISP_WR_REG(dither, REG_ISP_YUVDITHER_T, Y_DITHER, reg.raw);
	} else if (sel == 1) {
		union REG_ISP_YUVDITHER_UV_DITHER reg;

		reg.raw = 0;
		reg.bits.UV_DITHER_EN = en;
		reg.bits.UV_DITHER_MOD_EN = mod_en;
		reg.bits.UV_DITHER_HISTIDX_EN = histidx_en;
		reg.bits.UV_DITHER_FMNUM_EN = fmnum_en;
		reg.bits.UV_DITHER_WIDTHM1 = (ctx->img_width >> 1) - 1;
		reg.bits.UV_DITHER_HEIGHTM1 = (ctx->img_height >> 1) - 1;

		ISP_WR_REG(dither, REG_ISP_YUVDITHER_T, UV_DITHER, reg.raw);
	}

	return 0;
}

void ispblk_444_422_config(struct isp_ctx *ctx)
{
	uintptr_t y42 = ctx->phys_regs[ISP_BLK_ID_444422];

	ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_8, FORCE_DMA_DISABLE, (ctx->is_3dnr_on) ? 0 : 0x3f);

	ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_4, REG_422_444, ctx->is_yuv_sensor);

	ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_8, TDNR_DEBUG_SEL, (ctx->is_tile) ? 0x4 : 0x0);
}

void ispblk_fbcd_enable(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbcd = ctx->phys_regs[ISP_BLK_ID_FBCD];

	ISP_WR_BITS(fbcd, REG_FBCD_T, REG_0, FORCE_RUN, en);
	ISP_WR_BITS(fbcd, REG_FBCD_T, REG_0, FBCD_EN, en);
}

void ispblk_fbce_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union REG_FBCE_0 reg_0;

#define CU_SIZE 32
#define LOG_CU_SIZE 5

	u32 width_pad = ((ctx->img_width + CU_SIZE - 1) >> LOG_CU_SIZE) << LOG_CU_SIZE;
	u32 line_cu_num = width_pad >> LOG_CU_SIZE;
	u32 y_target_bit = ((CU_SIZE << 3) * g_y_cr[ISP_PRERAW_A]) / 100;
	u32 y_total_line_bit_budget = line_cu_num * y_target_bit;
	u32 uv_target_bit = ((CU_SIZE << 3) * g_uv_cr[ISP_PRERAW_A]) / 100;
	u32 uv_total_line_bit_budget = line_cu_num * uv_target_bit;

	u32 rc_strict_cu_idx = (((ctx->img_width) + CU_SIZE - 1) / CU_SIZE) - 1;

	u32 y_dma_size = ISP_ALIGN((((ISP_ALIGN(ctx->img_width, 32) * (ctx->img_height - 1)
				* g_y_cr[ISP_PRERAW_A]) / 100)
				+ ISP_ALIGN(ctx->img_width, 32)), 16) / 2;

	u32 c_dma_size = ISP_ALIGN((((ISP_ALIGN(ctx->img_width, 32) * (ctx->img_height / 2 - 1)
				* g_uv_cr[ISP_PRERAW_A]) / 100)
				+ ISP_ALIGN(ctx->img_width, 32)), 16) / 2;

	reg_0.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_0);
	reg_0.bits.FBCE_EN = en;
	reg_0.bits.FLOOR_MD_EN = 0;
	reg_0.bits.CPLX_SHIFT = 0x2;
	reg_0.bits.DC_CPLX_GAIN = 0x5;
	reg_0.bits.PEN_POS_SHIFT = 0x1;
	reg_0.bits.RC_TYPE = 0x1;
	reg_0.bits.LINE_BIT_GUARD = 0x24;
	reg_0.bits.RC_STRICT_CU_IDX = rc_strict_cu_idx;
	reg_0.bits.RC_STRICT_RC_EN = 1;
	ISP_WR_REG(fbce, REG_FBCE_T, REG_0, reg_0.raw);

	ISP_WR_BITS(fbce, REG_FBCE_T, REG_1, Y_TOTAL_LINE_BIT_BUDGET, y_total_line_bit_budget);
	ISP_WR_BITS(fbce, REG_FBCE_T, REG_1, C_TOTAL_LINE_BIT_BUDGET, uv_total_line_bit_budget);

	ISP_WR_BITS(fbce, REG_FBCE_T, REG_2, AMPLE_STATE_THR, 0x1f5);
	ISP_WR_BITS(fbce, REG_FBCE_T, REG_2, AVG_SHARE_SHIFT, 0x9);
	ISP_WR_BITS(fbce, REG_FBCE_T, REG_2, MAX_Q_STEP, 0x40);

	ISP_WR_REG(fbce, REG_FBCE_T, REG_5, y_dma_size - 1);
	ISP_WR_REG(fbce, REG_FBCE_T, REG_6, c_dma_size - 1);
}

void ispblk_ynr_config(struct isp_ctx *ctx, enum ISP_YNR_OUT out_sel, uint8_t ns_gain, uint8_t str)
{
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];
	// depth = 8
	uint8_t intensity_sel[] = {
		5, 7, 20, 23, 14, 9, 31, 31
	};
	// depth =64
	uint8_t weight_lut[] = {
		31, 16, 8,  4,  2,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	};
	// depth = 6
	uint8_t ns0_luma_th[] = {
		22, 33, 55, 99, 139, 181,
	};
	// depth = 5
	uint16_t ns0_slope[] = {
		279, 209, 81, -12, -121,
	};
	// depth = 6
	uint8_t ns0_offset[] = {
		25, 31, 40, 47, 46, 36
	};
	// depth = 6
	uint8_t ns1_luma_th[] = {
		22, 33, 55, 99, 139, 181,
	};
	// depth = 5
	uint16_t ns1_slope[] = {
		93, 46, 23, 0, -36,
	};
	// depth = 6
	uint8_t ns1_offset[] = {
		7, 9, 11, 13, 13, 10
	};
	uint16_t i = 0;

	ISP_WO_BITS(ynr, REG_ISP_YNR_T, INDEX_CLR, YNR_INDEX_CLR, 1);
	for (i = 0; i < ARRAY_SIZE(intensity_sel); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, INTENSITY_SEL,
			   intensity_sel[i]);
	}
	for (i = 0; i < ARRAY_SIZE(weight_lut); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_LUT,
			   weight_lut[i]);
	}
	for (i = 0; i < ARRAY_SIZE(ns0_luma_th); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH,
			   ns0_luma_th[i]);
	}
	for (i = 0; i < ARRAY_SIZE(ns0_slope); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_SLOPE,
			   ns0_slope[i]);
	}
	for (i = 0; i < ARRAY_SIZE(ns0_offset); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET,
			   ns0_offset[i]);
	}
	for (i = 0; i < ARRAY_SIZE(ns1_luma_th); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH,
			   ns1_luma_th[i]);
	}
	for (i = 0; i < ARRAY_SIZE(ns1_slope); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_SLOPE,
			   ns1_slope[i]);
	}
	for (i = 0; i < ARRAY_SIZE(ns1_offset); ++i) {
		ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET,
			   ns1_offset[i]);
	}

	ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, ISP_YNR_OUT_BYPASS);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, STRENGTH_MODE, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_0, 6);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_1, 6);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_2, 6);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NEIGHBOR_MAX, 1);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, RES_K_SMOOTH, 67);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, RES_K_TEXTURE, 88);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, VAR_TH, 32);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_SM, 29);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_V, 23);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_H, 20);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_D45, 15);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_D135, 7);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_NORM_1, 7);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_NORM_2, 5);

	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS_GAIN, 16);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_NS_TH, 7);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_POS_GAIN, 4);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_NEG_GAIN, 2);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, ALPHA_GAIN, 256);

	ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, out_sel);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS_GAIN, ns_gain);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, STRENGTH_MODE, str);
}

void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, uint8_t str_mode)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	union REG_ISP_CNR_ENABLE reg_00;
	union REG_ISP_CNR_STRENGTH_MODE reg_01;
	union REG_ISP_CNR_PURPLE_TH reg_02;
	union REG_ISP_CNR_PURPLE_CR reg_03;
	union REG_ISP_CNR_GREEN_CR reg_04;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_00 reg_05;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_04 reg_06;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_08 reg_07;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_12 reg_08;
	union REG_ISP_CNR_INTENSITY_SEL_0 reg_09;
	union REG_ISP_CNR_INTENSITY_SEL_4 reg_11;

	reg_00.raw = 0;
	reg_00.bits.CNR_ENABLE = en;
	reg_00.bits.PFC_ENABLE = pfc_en;
	reg_00.bits.CNR_DIFF_SHIFT_VAL = 0;
	reg_00.bits.CNR_RATIO = 220;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_ENABLE, reg_00.raw);

	reg_01.raw = 0;
	reg_01.bits.CNR_STRENGTH_MODE = str_mode;
	reg_01.bits.CNR_FUSION_INTENSITY_WEIGHT = 4;
	reg_01.bits.CNR_WEIGHT_INTER_SEL = 0;
	reg_01.bits.CNR_VAR_TH = 32;
	reg_01.bits.CNR_FLAG_NEIGHBOR_MAX_WEIGHT = 1;
	reg_01.bits.CNR_SHDW_SEL = 1;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_STRENGTH_MODE, reg_01.raw);

	reg_02.raw = 0;
	reg_02.bits.CNR_PURPLE_TH = 90;
	reg_02.bits.CNR_CORRECT_STRENGTH = 16;
	reg_02.bits.CNR_DIFF_GAIN = 4;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_PURPLE_TH, reg_02.raw);

	reg_03.raw = 0;
	reg_03.bits.CNR_PURPLE_CR = 176;
	reg_03.bits.CNR_GREEN_CB = 43;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_PURPLE_CR, reg_03.raw);

	reg_04.raw = 0;
	reg_04.bits.CNR_PURPLE_CB = 232;
	reg_04.bits.CNR_GREEN_CR = 21;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_GREEN_CR, reg_04.raw);

	reg_05.raw = 0;
	reg_05.bits.WEIGHT_LUT_INTER_CNR_00 = 16;
	reg_05.bits.WEIGHT_LUT_INTER_CNR_01 = 16;
	reg_05.bits.WEIGHT_LUT_INTER_CNR_02 = 15;
	reg_05.bits.WEIGHT_LUT_INTER_CNR_03 = 13;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_00, reg_05.raw);

	reg_06.raw = 0;
	reg_06.bits.WEIGHT_LUT_INTER_CNR_04 = 12;
	reg_06.bits.WEIGHT_LUT_INTER_CNR_05 = 10;
	reg_06.bits.WEIGHT_LUT_INTER_CNR_06 = 8;
	reg_06.bits.WEIGHT_LUT_INTER_CNR_07 = 6;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_04, reg_06.raw);

	reg_07.raw = 0;
	reg_07.bits.WEIGHT_LUT_INTER_CNR_08 = 4;
	reg_07.bits.WEIGHT_LUT_INTER_CNR_09 = 3;
	reg_07.bits.WEIGHT_LUT_INTER_CNR_10 = 2;
	reg_07.bits.WEIGHT_LUT_INTER_CNR_11 = 1;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_08, reg_07.raw);

	reg_08.raw = 0;
	reg_08.bits.WEIGHT_LUT_INTER_CNR_12 = 1;
	reg_08.bits.WEIGHT_LUT_INTER_CNR_13 = 1;
	reg_08.bits.WEIGHT_LUT_INTER_CNR_14 = 0;
	reg_08.bits.WEIGHT_LUT_INTER_CNR_15 = 0;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_12, reg_08.raw);

	reg_09.raw = 0;
	reg_09.bits.CNR_INTENSITY_SEL_0 = 10;
	reg_09.bits.CNR_INTENSITY_SEL_1 = 5;
	reg_09.bits.CNR_INTENSITY_SEL_2 = 16;
	reg_09.bits.CNR_INTENSITY_SEL_3 = 16;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_INTENSITY_SEL_0, reg_09.raw);

	reg_11.raw = 0;
	reg_11.bits.CNR_INTENSITY_SEL_4 = 12;
	reg_11.bits.CNR_INTENSITY_SEL_5 = 13;
	reg_11.bits.CNR_INTENSITY_SEL_6 = 6;
	reg_11.bits.CNR_INTENSITY_SEL_7 = 16;
	ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_INTENSITY_SEL_4, reg_11.raw);
}

int ispblk_ee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE];
	union REG_ISP_EE_00  reg_0;
	union REG_ISP_EE_04  reg_4;

	reg_0.raw = ISP_RD_REG(ee, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = en;
	reg_0.bits.EE_TOTAL_CORING = 0x20;
	reg_0.bits.EE_TOTAL_GAIN = 0x80;
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_00, reg_0.raw);

	reg_4.raw = ISP_RD_REG(ee, REG_ISP_EE_T, REG_04);
	reg_4.bits.EE_TOTAL_OSHTTHRD  = 0x10;
	reg_4.bits.EE_TOTAL_USHTTHRD  = 0x10;
	reg_4.bits.EE_PRE_PROC_GAIN   = 0x24;
	reg_4.bits.EE_PRE_PROC_MODE   = 1;
	reg_4.bits.EE_PRE_PROC_ENABLE = 1;
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_04, reg_0.raw);

	ISP_WR_REG(ee, REG_ISP_EE_T, REG_0C, 0x103);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_10, 0x2000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_14, 0x00400020);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_18, 0x102018f0);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_1C, 0x00180018);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_20, 0x00100010);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_24, 0x00100010);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_28, 0x00100010);

	//reg_ee_dircal_dgr4_filter000
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_30, 0x01f00000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_34, 0x000001e0);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_38, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_3C, 0x00200000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_40, 0x00000040);

	//reg_ee_dircal_dgr4_filter045
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_58, 0x01e801f0);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_5C, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_60, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_64, 0x00200000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_68, 0x00000040);

	//reg_ee_dircal_dgr4_filter090
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_6C, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_70, 0x01f00000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_74, 0x00200000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_78, 0x000001e0);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_7C, 0x00000040);

	//reg_ee_dircal_dgr4_filter135
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_80, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_84, 0x01e80000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_88, 0x002001f0);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_8C, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_90, 0x00000040);

	//reg_ee_dircal_dgr4_filternod
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_94, 0x01fc01fc);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_98, 0x01e80000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_9C, 0x008001f8);

	ISP_WR_BITS(ee, REG_ISP_EE_T, REG_1C4, EE_SHTCTRL_OSHTGAIN, 0x10);
	ISP_WR_BITS(ee, REG_ISP_EE_T, REG_1C4, EE_SHTCTRL_USHTGAIN, 0x10);

	ISP_WR_REG(ee, REG_ISP_EE_T, REG_1C8, 0x00000000);

	//reg_ee_motion_lut_in
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_1CC, 0x20ffffff);

	//reg_ee_motion_lut_out
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_1D0, 0xffffffff);

	ISP_WR_REG(ee, REG_ISP_EE_T, REG_1D4, 0x00000000);
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_1D8, 0x00000000);

	//ISP_WR_BITS(ee, REG_ISP_EE_T, REG_1BC, IMG_WIDTH, ctx->img_width - 1);
	//ISP_WR_BITS(ee, REG_ISP_EE_T, REG_1BC, IMG_HEIGHT, ctx->img_height - 1);

	return 0;
}

int ispblk_ycur_config(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uint16_t i;
	union REG_ISP_YCURVE_YCUR_PROG_DATA reg_data;

	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 1);

	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_WSEL, sel);
	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_ST_ADDR, YCUR_ST_ADDR, 0);
	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_ST_ADDR, YCUR_ST_W, 1);
	ISP_WR_REG(ycur, REG_ISP_YCURVE_T, YCUR_PROG_MAX, data[0x40]);
	for (i = 0; i < 0x40; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.YCUR_DATA_E = data[i];
		reg_data.bits.YCUR_DATA_O = data[i + 1];
		reg_data.bits.YCUR_W = 1;
		ISP_WR_REG(ycur, REG_ISP_YCURVE_T, YCUR_PROG_DATA, reg_data.raw);
	}

	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_RSEL, sel);
	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 0);

	return 0;
}

int ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, uint8_t sel)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];

	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_CTRL, YCURVE_ENABLE, enable);
	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_RSEL, sel);

	return 0;
}

void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_YUVTOP];

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_ENABLE, en);
#if 0 //test 1
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_LUT_IN_0, 0x0);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_LUT_IN_1, 0x20);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_IN, CA_LITE_LUT_IN_2, 0xf0);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_IN, CA_LITE_LUT_OUT_0, 0x0);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_OUT, CA_LITE_LUT_OUT_1, 0x40);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_OUT, CA_LITE_LUT_OUT_2, 0x80);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_SLP, CA_LITE_LUT_SLP_0, 0x20);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_SLP, CA_LITE_LUT_SLP_1, 0x4);
#else //test 2 for color bar
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_LUT_IN_0, 0x0);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_LUT_IN_1, 0x80);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_IN, CA_LITE_LUT_IN_2, 0x100);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_IN, CA_LITE_LUT_OUT_0, 0x100);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_OUT, CA_LITE_LUT_OUT_1, 0x80);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_OUT, CA_LITE_LUT_OUT_2, 0x40);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_SLP, CA_LITE_LUT_SLP_0, 0x0);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_SLP, CA_LITE_LUT_SLP_1, 0x0);
#endif
}

void ispblk_rgbir_preproc_config(struct isp_ctx *ctx, uint8_t *wdata, int16_t *data_r, int16_t *data_g, int16_t *data_b)
{
	uintptr_t ir_pre_proc;
	union REG_ISP_PREPROCESS_IR_PREPROC_PROG_DATA			reg_wdata;
	union REG_ISP_PREPROCESS_IR_PREPROC_MEM_RATIO_PROG_DATA reg_ratio_wdata;

	uint8_t i, frame_idx;

	if (ctx->is_rgbir_sensor == 0) {
		uintptr_t ir_pre_le = ctx->phys_regs[ISP_BLK_ID_IR_PRE_PROC_LE];
		uintptr_t ir_pre_se = ctx->phys_regs[ISP_BLK_ID_IR_PRE_PROC_SE];

		ISP_WR_BITS(ir_pre_le, REG_ISP_PREPROCESS_T, IR_PREPROC_CTRL, PREPROCESS_ENABLE, 0);
		ISP_WR_BITS(ir_pre_se, REG_ISP_PREPROCESS_T, IR_PREPROC_CTRL, PREPROCESS_ENABLE, 0);
		// gating clock off ?
//		ISP_WR_BITS(ir_pre_le, REG_ISP_PREPROCESS_T, IR_PREPROC_CTRL, FORCE_CLK_ENABLE, 0);
//		ISP_WR_BITS(ir_pre_se, REG_ISP_PREPROCESS_T, IR_PREPROC_CTRL, FORCE_CLK_ENABLE, 0);
		return;
	}

	for (frame_idx = 0; frame_idx < (ctx->is_hdr_on + 1); frame_idx++) {
		if (frame_idx == 0)
			ir_pre_proc = ctx->phys_regs[ISP_BLK_ID_IR_PRE_PROC_LE];
		else
			ir_pre_proc = ctx->phys_regs[ISP_BLK_ID_IR_PRE_PROC_SE];

		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_CTRL, PREPROCESS_ENABLE, 1);

		// w lut program
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_EN, 1);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_WSEL, frame_idx);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_RSEL, frame_idx);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 3); // wlut
		// init table
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i++) {
			reg_wdata.raw = 0;
			reg_wdata.bits.MEM_WDATA = wdata[i];
			reg_wdata.bits.MEM_W = 1;
			ISP_WR_REG(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_DATA, reg_wdata.raw);
		}

		// ratio lut program
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_WSEL, frame_idx);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_RSEL, frame_idx);
		// R
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 0); // R=0
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i += 2) {
			reg_ratio_wdata.raw = 0;
			reg_ratio_wdata.bits.IR_RATIO_WDATA_E = data_r[i];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_O = data_r[i + 1];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_W = 1;
			ISP_WR_REG(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_MEM_RATIO_PROG_DATA, reg_ratio_wdata.raw);
		}
		// G
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 1); // G=1
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i += 2) {
			reg_ratio_wdata.raw = 0;
			reg_ratio_wdata.bits.IR_RATIO_WDATA_E = data_g[i];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_O = data_g[i + 1];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_W = 1;
			ISP_WR_REG(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_MEM_RATIO_PROG_DATA, reg_ratio_wdata.raw);
		}
		// B
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 2); // B=2
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i += 2) {
			reg_ratio_wdata.raw = 0;
			reg_ratio_wdata.bits.IR_RATIO_WDATA_E = data_b[i];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_O = data_b[i + 1];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_W = 1;
			ISP_WR_REG(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_MEM_RATIO_PROG_DATA, reg_ratio_wdata.raw);
		}

		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_EN, 0);

		// ---- read back test ---- //
#if 0
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_EN, 1);

		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
			IR_PREPROC_SW_MODE, MEM_SW_RSEL, frame_idx); // to mem0 & mem1 ?
		vip_pr(CVI_WARN, "read ir rdata=\n");
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
			IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 0); // 0:R, 1:G, 2:B, 3:WLUT
		for (i = 0; i < 128; i++) {
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_SW_MODE, MEM_SW_RADDR, i);
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_SW_R, 1); // read trigger
			vip_pr(CVI_WARN, "%5d,", ISP_RD_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_RDATA));
			if ((i&7) == 7)
				vip_pr(CVI_WARN, "\n");
		}
		vip_pr(CVI_WARN, "read ir gdata=\n"); // 0:R, 1:G, 2:B, 3:WLUT
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 1);
		for (i = 0; i < 128; i++) {
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_SW_MODE, MEM_SW_RADDR, i);
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_SW_R, 1); // read trigger
			vip_pr(CVI_WARN, "%5d,", ISP_RD_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_RDATA));
			if ((i&7) == 7)
				vip_pr(CVI_WARN, "\n");
		}
		vip_pr(CVI_WARN, "read ir bdata=\n"); // 0:R, 1:G, 2:B, 3:WLUT
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 2);
		for (i = 0; i < 128; i++) {
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_SW_MODE, MEM_SW_RADDR, i);
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_SW_R, 1); // read trigger
			vip_pr(CVI_WARN, "%5d,", ISP_RD_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
					IR_PREPROC_SW_RDATA, MEM_RDATA));
			if ((i&7) == 7)
				vip_pr(CVI_WARN, "\n");
		}
		vip_pr(CVI_WARN, "read ir wlut data=\n"); // 0:R, 1:G, 2:B, 3:WLUT
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 3);
		for (i = 0; i < 128; i++) {
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_SW_MODE, MEM_SW_RADDR, i);
			ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_SW_R, 1); // read trigger
			vip_pr(CVI_WARN, "%5d,", ISP_RD_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T,
				IR_PREPROC_SW_RDATA, MEM_RDATA));
			if ((i&7) == 7)
				vip_pr(CVI_WARN, "\n");
		}
		ISP_WR_BITS(ir_pre_proc, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_EN, 0);
#endif
	}
}

void ispblk_ir_proc_config(struct isp_ctx *ctx, uint8_t *gamma)
{
	uintptr_t ir_proc = ctx->phys_regs[ISP_BLK_ID_IR_PROC];
	union REG_IR_WDMA_PROC_IR_PROC_CTRL reg;
	uint16_t i;

	reg.raw = 0;
	reg.bits.IR_WDMA_PROC_EN = ctx->is_rgbir_sensor;
	reg.bits.WDMI_EN_LE = ctx->is_rgbir_sensor;
	reg.bits.WDMI_EN_SE = (ctx->is_rgbir_sensor & ctx->is_hdr_on);
	reg.bits.IR_DATA_SEL = 0; //to-do
	reg.bits.IR_BIT_MODE = (ctx->sensor_bitdepth != 8) ? 0:1;
	reg.bits.IS_RGBIR = ctx->is_rgbir_sensor;
	reg.bits.SHDW_READ_SEL = 1;
	ISP_WR_REG(ir_proc, REG_IR_WDMA_PROC_T, IR_PROC_CTRL, reg.raw);

	if (ctx->is_rgbir_sensor && reg.bits.IR_BIT_MODE) {
		union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_DATA reg_gamma;

		ISP_WR_BITS(ir_proc, REG_IR_WDMA_PROC_T, GAMMA_CURVE_CTRL, GAMMA_CURVE_ADDR_RST, 1);
		ISP_WR_BITS(ir_proc, REG_IR_WDMA_PROC_T, GAMMA_CURVE_PROG_MAX, GAMMA_CURVE_MAX, 255);

		ISP_WR_BITS(ir_proc, REG_IR_WDMA_PROC_T, GAMMA_CURVE_PROG_CTRL, GAMMA_CURVE_WSEL, 0);
		ISP_WR_BITS(ir_proc, REG_IR_WDMA_PROC_T, GAMMA_CURVE_PROG_CTRL, GAMMA_CURVE_RSEL_LE, 0);
		if (ctx->is_hdr_on)
			ISP_WR_BITS(ir_proc, REG_IR_WDMA_PROC_T, GAMMA_CURVE_PROG_CTRL, GAMMA_CURVE_RSEL_SE, 0);

		for (i = 0; i < 256; i += 2) {
			reg_gamma.raw = 0;
			reg_gamma.bits.GAMMA_CURVE_DATA_E = gamma[i];
			reg_gamma.bits.GAMMA_CURVE_DATA_O = gamma[i + 1];
			reg_gamma.bits.GAMMA_CURVE_W = 1;
			ISP_WR_REG(ir_proc, REG_IR_WDMA_PROC_T, GAMMA_CURVE_PROG_DATA, reg_gamma.raw);
		}
	}
}

/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/

void ispblk_fusion_hdr_cfg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];

	if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_CTRL_0, FS_ENABLE, false);
		ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_CTRL_0, FS_OUT_SEL, ISP_FS_OUT_LONG);
	} else
		ISP_WR_BITS(fusion, REG_ISP_FUSION_T, FS_CTRL_0, FS_ENABLE, true);
}

void isp_first_frm_reset(struct isp_ctx *ctx, uint8_t reset)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MANR];
	uintptr_t tdnr = ctx->phys_regs[ISP_BLK_ID_444422];

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_00, FIRST_FRAME_RESET, reset);
	ISP_WR_BITS(tdnr, REG_ISP_444_422_T, REG_5, FIRST_FRAME_RESET, reset);
}

void _ispblk_lsc_cfg_update(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSCM0];
	int width = ctx->img_width;
	int height = ctx->img_height;
	int InnerBlkX = 34, InnerBlkY = 34; //mesh_num is 37
	int mesh_x_coord_unit = (InnerBlkX * (1 << F_D)) / width;
	int mesh_y_coord_unit = (InnerBlkY * (1 << F_D)) / height;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;
	u32 reg_lsc_ystep = mesh_y_coord_unit + 1;

	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int image_h_in_mesh_unit = height * reg_lsc_ystep;
	int OuterBlkX = 36, OuterBlkY = 36; //mesh_num is 37
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << F_D) - image_w_in_mesh_unit) / 2;
	u32 reg_lsc_imgy0 = (OuterBlkY * (1 << F_D) - image_h_in_mesh_unit) / 2;

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_HDR_ENABLE, ctx->isp_pipe_cfg[raw_num].is_hdr_on);

	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_XSTEP, reg_lsc_xstep);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_YSTEP, reg_lsc_ystep);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_IMGX0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_IMGY0, reg_lsc_imgy0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITX0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITY0, reg_lsc_imgy0);
}

void ispblk_post_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	//uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	enum ISP_BAYER_TYPE bayer_id;

	//ISPTOP dummy register
	//ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_6, IMG_WIDTHM1, ctx->img_width - 1);
	//ISP_WR_BITS(isptopb, REG_ISP_TOP_T, REG_6, IMG_HEIGHTM1, ctx->img_height - 1);

	//RAWTOP
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_HEIGHTM1, ctx->img_height - 1);

	if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
		bayer_id = (ctx->rgb_color_mode[raw_num] == ISP_BAYER_TYPE_BGRGI) ?
			ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_RG;
	} else
		bayer_id = ctx->rgb_color_mode[raw_num];

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_BAYER_TYPE_TOPLEFT, BAYER_TYPE_TOPLEFT, bayer_id);

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_BAYER_TYPE_TOPLEFT, RGBIR_ENABLE,
							ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENBALE, CH_NUM, ctx->isp_pipe_cfg[raw_num].is_hdr_on);

	_ispblk_lsc_cfg_update(ctx, raw_num);
#if 0
	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
		//ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 1);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 1);
	} else {
		//ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 0);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 0);
	}
#endif
	//RGBTOP
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_0, RGBTOP_BAYER_TYPE, bayer_id);
	//ToDo rgbir flow
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_0, RGBTOP_RGBIR_ENABLE, ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGH_M1, ctx->img_height - 1);

	//YUVTOP
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_3, Y42_SEL, ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGW_M1, ctx->img_width - 1);
	//ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGW_M1_CROP, ctx->img_width - 1);
	//ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);

	//online to scaler or not
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, HSV_ENABLE, BYPASS_V, !ctx->isp_pipe_cfg[raw_num].is_offline_scaler);

	//LTM size update
	{
		uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];

		union REG_ISP_LTM_BLK_SIZE reg_blk;
		uint32_t img_width = (ctx->is_tile)
				   ? ctx->tile_cfg.r_out.end + 1
				   : ctx->img_width;

		reg_blk.raw = ISP_RD_REG(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE);
		reg_blk.bits.HORZ_BLK_SIZE = g_lmp_cfg[raw_num].post_w_bit - 3;
		reg_blk.bits.BLK_WIDTHM1 = UPPER(img_width, g_lmp_cfg[raw_num].post_w_bit) - 1;
		reg_blk.bits.BLK_HEIGHTM1 = UPPER(ctx->img_height, g_lmp_cfg[raw_num].post_h_bit) - 1;
		reg_blk.bits.VERT_BLK_SIZE = g_lmp_cfg[raw_num].post_h_bit - 3;
		ISP_WR_REG(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE, reg_blk.raw);

		ispblk_dma_config(ctx, ISP_BLK_ID_RDMA13, 0);
		ispblk_dma_config(ctx, ISP_BLK_ID_RDMA14, 0);
	}

#if 0
	//AWB size update
	{
		uintptr_t awb = ctx->phys_regs[ISP_BLK_ID_AWB2];
		u32 numxm1 = 33, numym1 = 33;
		u32 awb_sub_win_w = 0, awb_sub_win_h = 0;

		numxm1 = ISP_RD_REG(awb, REG_ISP_AWB_T, STS_NUMXM1);
		numym1 = ISP_RD_REG(awb, REG_ISP_AWB_T, STS_NUMYM1);

		awb_sub_win_w = (((ctx->img_width / (numxm1 + 1)) >> 1) << 1);
		awb_sub_win_h = (((ctx->img_height / (numym1 + 1)) >> 1) << 1);

		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_WIDTH, awb_sub_win_w);
		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_HEIGHT, awb_sub_win_h);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA22, 0);
	}

	//MANR size config
	{
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MANR];
		union REG_ISP_MMAP_C4 reg_c4;
		uint32_t img_width = (ctx->is_tile)
				   ? ctx->tile_cfg.r_out.end + 1
				   : ctx->img_width;
		u32 w_bit = 0, h_bit = 0;

		w_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT);
		h_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT);

		reg_c4.raw = ISP_RD_REG(blk, REG_ISP_MMAP_T, REG_C4);
		reg_c4.bits.IMG_WIDTH_CROP = ((UPPER((UPPER(img_width, w_bit) * 48), 7) << 7) + 47) / 48 - 1;
		reg_c4.bits.IMG_HEIGHT_CROP = UPPER(ctx->img_height, h_bit) - 1;
		ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_C4, reg_c4.raw);

		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_C8, CROP_W_END, UPPER(img_width, w_bit) - 1);
		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_CC, CROP_H_END, UPPER(ctx->img_height, h_bit) - 1);

		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, IMG_WIDTH_CROP_SCALAR, img_width - 1);
		ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, IMG_HEIGHT_CROP_SCALAR, ctx->img_height - 1);
	}
#endif
}

void ispblk_pre_be_cfg_update(struct isp_ctx *ctx, struct cvi_vip_isp_be_tun_cfg *be_tun,
				const enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	union REG_PRE_RAW_BE_TOP_CTRL top_ctrl;
	uint32_t img_width = ctx->img_width;
	uint32_t img_height = ctx->img_height;

	top_ctrl.raw = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL);
	top_ctrl.bits.BAYER_TYPE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.RGBIR_EN		= ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	top_ctrl.bits.WDMI_EN_LE	= false;
	top_ctrl.bits.WDMI_EN_SE	= false;
	top_ctrl.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;

	if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
		top_ctrl.bits.BAYER_TYPE_AFTER_PREPROC = (ctx->rgb_color_mode[raw_num] == ISP_BAYER_TYPE_BGRGI) ?
			ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_RG;
	} else
		top_ctrl.bits.BAYER_TYPE_AFTER_PREPROC	= ctx->rgb_color_mode[raw_num];
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL, top_ctrl.raw);

	if (ctx->is_dpcm_on) {
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_MODE, 7);
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_XSTR, 8191);
	} else {
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_MODE, 0);
		ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_DPCM, DPCM_XSTR, 0);
	}

	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_SIZE, RDMI_WIDTHM1, img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, RDMI_SIZE, RDMI_HEIGHTM1, img_height - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, CROP_SIZE_LE, CROP_WIDTHM1, img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, CROP_SIZE_LE, CROP_HEIGHTM1, img_height - 1);

	//AF config update
	if (!(be_tun && be_tun->af_cfg.update)) {
		uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
		int numx = 17, numy = 15;

		// 8 <= offset_x <= img_width - 8
		ISP_WR_BITS(sts, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_X, 0x8);
		// 2 <= offset_y <= img_height - 2
		ISP_WR_BITS(sts, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_Y, 0x2);
		// block_width >= 15
		ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_WIDTH, (img_width - 16) / numx);
		// block_height >= 15
		ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_HEIGHT, (img_height - 4) / numy);
		ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_NUM_X, 17);
		ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_NUM_Y, 15);

		ISP_WR_REG(sts, REG_ISP_AF_T, HIGH_Y_THRE, 0x258);

		ISP_WR_REG(sts, REG_ISP_AF_T, IMAGE_WIDTH, img_width - 1);
		ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_WIDTH, img_width - 1);
		ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_HEIGHT, img_height - 1);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA16, 0);
	}

	//AWB config update
	if (!(be_tun && be_tun->awb_cfg.update)) {
		uintptr_t awb = ctx->phys_regs[ISP_BLK_ID_AWB0];
		u32 numxm1 = 33, numym1 = 33;

		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_NUMXM1, numxm1 - 1);
		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_NUMYM1, numym1 - 1);
		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_WIDTH, (img_width) / numxm1);
		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_HEIGHT, (img_height) / numym1);
		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_OFFSETX, 0);
		ISP_WR_REG(awb, REG_ISP_AWB_T, STS_OFFSETY, 0);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA19, 0);
	}

	//AE0 config update
	if (!(be_tun && be_tun->ae_cfg[0].update)) {
		uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AEHIST0];
		uint8_t num_x = 17, num_y = 15;
		uint8_t sub_window_w = 0, sub_window_h = 0;

		sub_window_w = img_width / (num_x + 1);
		sub_window_h = img_height / (num_y + 1);

		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, num_x - 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, num_y - 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_WIDTH, sub_window_w);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, sub_window_h);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA18, 0);
	}

	//AE1 config update
	if ((ctx->isp_pipe_cfg[raw_num].is_hdr_on) &&
		!(be_tun && be_tun->ae_cfg[1].update)) {
		uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AEHIST1];
		uint8_t num_x = 17, num_y = 15;
		uint8_t sub_window_w = 0, sub_window_h = 0;

		sub_window_w = img_width / (num_x + 1);
		sub_window_h = img_height / (num_y + 1);

		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, num_x - 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, num_y - 1);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_WIDTH, sub_window_w);
		ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, sub_window_h);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA21, 0);
	}

	//GMS config update
	if (!(be_tun && be_tun->gms_cfg.update)) {
		uintptr_t gms = ctx->phys_regs[ISP_BLK_ID_GMS];
		u8 gap_x = 1, gap_y = 1;
		u16 start_x = 0, start_y = 0;
		u16 x_section_size = 64, y_section_size = 64;

		ISP_WR_REG(gms, REG_ISP_GMS_T, GMS_START_X, start_x);
		ISP_WR_REG(gms, REG_ISP_GMS_T, GMS_START_Y, start_y);
		ISP_WR_REG(gms, REG_ISP_GMS_T, GMS_X_SECTION_SIZE, x_section_size - 1);
		ISP_WR_REG(gms, REG_ISP_GMS_T, GMS_Y_SECTION_SIZE, y_section_size - 1);
		ISP_WR_REG(gms, REG_ISP_GMS_T, GMS_X_GAP, gap_x);
		ISP_WR_REG(gms, REG_ISP_GMS_T, GMS_Y_GAP, gap_y);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA17, 0);
	}
}

u8 isp_is_awb_se(struct isp_ctx *ctx)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];

	return ISP_RD_BITS(preraw_be, REG_PRE_RAW_BE_T, FLOW_CTRL, AWB_OUTPUT_IS_SE);
}

u8 isp_is_fe02be_enable(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	return ISP_RD_BITS(isptopb, REG_ISP_TOP_T, REG_5, FE02BE_ENABLE);
}

void isp_be_awb_source_chg(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, u32 frm_num)
{
	uintptr_t awb = ctx->phys_regs[ISP_BLK_ID_AWB0];
	enum ISP_AWB_MODE mode = ISP_AWB_LE;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		mode = (frm_num & 0x1) ? ISP_AWB_SE : ISP_AWB_LE;

	vip_pr(CVI_DBG, "raw_num=%d, AWB_source change=%s\n", raw_num, (mode == ISP_AWB_LE) ? "AWB_LE" : "AWB_SE");

	ISP_WR_BITS(awb, REG_ISP_AWB_T, ENABLE, AWB_SOURCE, mode);
}

uint32_t ispblk_csibdg_chn_dbg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, enum cvi_isp_pre_chn_num chn)
{
	uintptr_t addr = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_CSIBDG0] :
			ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];

	uint32_t raw = 0;

	switch (chn) {
	case 0:
		raw = ISP_RD_REG(addr, REG_ISP_CSI_BDG_T, CH0_DEBUG_3);
		break;
	case 1:
		raw = ISP_RD_REG(addr, REG_ISP_CSI_BDG_T, CH1_DEBUG_3);
		break;
	case 2:
		raw = ISP_RD_REG(addr, REG_ISP_CSI_BDG_T, CH2_DEBUG_3);
		break;
	case 3:
		raw = ISP_RD_REG(addr, REG_ISP_CSI_BDG_T, CH3_DEBUG_3);
		break;
	default:
		break;
	}

	return raw;
}

int isp_frm_err_handler(struct isp_ctx *ctx, const enum cvi_isp_raw err_raw_num, const u8 step)
{
	if (step == 1) {
		uintptr_t fe = (err_raw_num == ISP_PRERAW_A) ?
				ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0] :
				ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];

		ISP_WR_BITS(fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_FRAME_VLD_CH0, 0);
		ISP_WR_BITS(fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, PRE_RAW_FRAME_VLD_CH1, 0);
	} else if (step == 3) {
		uintptr_t csibdg_a = ctx->phys_regs[ISP_BLK_ID_CSIBDG0];
		uintptr_t csibdg_b = ctx->phys_regs[ISP_BLK_ID_CSIBDG1_R1];
		uintptr_t wdma_com = ctx->phys_regs[ISP_BLK_ID_WDMA_COM];
		uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_COM];

		u8 count = 10;

		ISP_WR_BITS(csibdg_a, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, ABORT, 1);
		ISP_WR_BITS(csibdg_b, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, ABORT, 1);

		while (--count > 0) {
			if (ISP_RD_BITS(wdma_com, REG_ISP_WDMA_COM_T, NORM_STATUS0, ABORT_DONE) &&
				ISP_RD_BITS(rdma_com, REG_ISP_RDMA_COM_T, NORM_STATUS0, ABORT_DONE)) {
				vip_pr(CVI_INFO, "W/RDMA abort done, count(%d)\n", count);
				break;
			}
			usleep_range(1, 5);
		}

		if (count == 0) {
			vip_pr(CVI_ERR, "WDMA/RDMA abort fail\n");
			return -1;
		}
	} else if (step == 4) {
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union REG_ISP_TOP_7 top_7;

		top_7.raw = 0;
		top_7.bits.AXI_RST = 1;
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, top_7.raw);
		top_7.raw = 0xbf;
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, top_7.raw);
		top_7.bits.AXI_RST = 0;
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, top_7.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_7, 0);

		vip_pr(CVI_INFO, "ISP reset done\n");
	} else if (step == 5) {
		uintptr_t fe0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		uintptr_t fe1 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
		uintptr_t be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		u8 cnt = 10;

		vip_pr(CVI_INFO, "Wait ISP idle\n");

		while (--cnt > 0) {
			if ((ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, FE_IDLE_INFO) == 0xFFFF) &&
				(ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, FE_IDLE_INFO) == 0xFFFF) &&
				((ISP_RD_REG(be, REG_PRE_RAW_BE_T, BE_IDLE_INFO) & 0x1FF03FF) == 0x1FF03FF) &&
				(ISP_RD_REG(isptopb, REG_ISP_TOP_T, REG_8)) == 0xFF) {
				vip_pr(CVI_INFO, "FE/BE/ISP idle done, count(%d)\n", cnt);
				break;
			}
			usleep_range(1, 5);
		}

		if (cnt == 0) {
			vip_pr(CVI_ERR, "FE0(0x%x)/FE1(0x%x)/BE(0x%x)/ISP(0x%x) not idle.",
				ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, FE_IDLE_INFO),
				ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, FE_IDLE_INFO),
				ISP_RD_REG(be, REG_PRE_RAW_BE_T, BE_IDLE_INFO),
				ISP_RD_REG(isptopb, REG_ISP_TOP_T, REG_8));
			return -1;
		}
	}

	return 0;
}

struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_fe = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0] :
			ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	struct _fe_dbg_i data;

	data.fe_idle_sts = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_DEBUG_STATE);
	data.fe_done_sts = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, FE_INFO);

	return data;
}

struct _be_dbg_i ispblk_be_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	struct _be_dbg_i data;

	data.be_done_sts = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_INFO);

	return data;
}

struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	struct _post_dbg_i data;

	data.top_sts = ISP_RD_REG(isptopb, REG_ISP_TOP_T, REG_8);

	return data;
}

struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t wdma_com = ctx->phys_regs[ISP_BLK_ID_WDMA_COM];
	uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_COM];
	struct _dma_dbg_i data;

	//RDMA err status
	data.rdma_err_sts = ISP_RD_REG(rdma_com, REG_ISP_RDMA_COM_T, NORM_STATUS0); //0x10
	//RDMA status
	data.rdma_idle = ISP_RD_REG(rdma_com, REG_ISP_RDMA_COM_T, NORM_STATUS1); //0x14

	//WDMA err status
	data.wdma_err_sts = ISP_RD_REG(wdma_com, REG_ISP_WDMA_COM_T, NORM_STATUS0); //0x10
	//WDMA status
	data.wdma_idle = ISP_RD_REG(wdma_com, REG_ISP_WDMA_COM_T, NORM_STATUS1); //0x14

	return data;
}

int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, uint32_t _w, uint32_t _h)
{
	uint32_t w = 0, len = 0, num = 0;

	switch (dmaid) {
	case ISP_BLK_ID_WDMA0: //pre_raw_fe0
	case ISP_BLK_ID_WDMA1:
	case ISP_BLK_ID_WDMA6: //pre_raw_fe1
	case ISP_BLK_ID_WDMA7:
		/* csibdg */
		w = _w;
		num = _h;

		if (ctx->is_dpcm_on)
			w = (ctx->is_tile) ? ((w + 8) >> 1) : (w >> 1);

		len = 3 * UPPER(w, 1);

		break;
	default:
		break;
	}

	len = VIP_ALIGN(len);

	return len * num;
}

/****************************************************************************
 *	Tile Config
 ****************************************************************************/

void ispblk_rawtop_tile(struct isp_ctx *ctx)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_HEIGHTM1, ctx->img_height - 1);

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);

	if (ctx->is_dpcm_on) {
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_WIDTHM1, ctx->img_width - 1 + 8);

		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR,
				ctx->is_work_on_r_tile ? 0 : ctx->tile_cfg.r_in.start);
	} else {
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);
	}
}

void ispblk_bnr_tile(struct isp_ctx *ctx)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];
	uint32_t center_x;

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, HSIZE, BNR_HSIZE, ctx->img_width);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, VSIZE, BNR_VSIZE, ctx->img_height);

	center_x = ctx->tile_cfg.r_out.end + 1;

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, X_CENTER, BNR_X_CENTER, center_x >> 1);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, Y_CENTER, BNR_Y_CENTER, ctx->img_height >> 1);
}

void ispblk_rgbtop_tile(struct isp_ctx *ctx)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];

	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_T, REG_9, RGBTOP_IMGH_M1, ctx->img_height - 1);
}

void ispblk_lsc_tile(struct isp_ctx *ctx)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSCM0];

	int width = ctx->tile_cfg.r_out.end + 1;
	int mesh_num = 37;
	int InnerBlkX = mesh_num - 1 - 2;
	int mesh_x_coord_unit = (InnerBlkX * (1 << F_D)) / width;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;

	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int OuterBlkX = InnerBlkX + 2;
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << F_D) - image_w_in_mesh_unit) / 2;

	if (ctx->is_tile && ctx->is_work_on_r_tile) {
#define LUT_SIZE 7
		//xsize, offset_x
		static const u32 lsc_lut[LUT_SIZE][2] = {
			{1280, 701312},
			{1920, 701376},
			{2304, 589824},
			{2560, 701440},
			{2592, 693024},
			{2688, 669504},
			{2880, 626976}
		};

		u8 i = 0;

		for (; i < LUT_SIZE; i++) {
			if (width == lsc_lut[i][0]) {
				reg_lsc_imgx0 = lsc_lut[i][1];
				break;
			} else if (i == (LUT_SIZE - 1)) {
				vip_pr(CVI_WARN, "No match width(%d) on lsc_lut\n", width);
			}
		}
	}

	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITX0, reg_lsc_imgx0);

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_HDR_ENABLE, ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on ? 1 : 0);
}

void ispblk_ltm_tile(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];

	u32 w_bit = ctx->isp_pipe_cfg[raw_num].lmap_i.w_bit;
	u32 h_bit = ctx->isp_pipe_cfg[raw_num].lmap_i.h_bit;

	u32 width = ctx->tile_cfg.r_out.end + 1;
	u32 height = ctx->img_height;

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_FRAME_SIZE, FRAME_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_FRAME_SIZE, FRAME_HEIGHTM1, ctx->img_height - 1);

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE, HORZ_BLK_SIZE, w_bit - 3);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE, VERT_BLK_SIZE, h_bit - 3);

	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE, BLK_WIDTHM1, (width - 1) >> w_bit);
	ISP_WR_BITS(ltm, REG_ISP_LTM_T, LTM_BLK_SIZE, BLK_HEIGHTM1, (height - 1) >> h_bit);

	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA13, 0);
	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA14, 0);

	{
		u16 reg_dma_blk_crop_str = 0, reg_blk_win_crop_str = 0;
		u16 reg_dma_blk_crop_end = 0, reg_blk_win_crop_end = 0;
		u16 reg_rs_out_crop_str = 0, reg_rs_out_crop_end = 0;

		u16 r_in_str = ctx->tile_cfg.r_in.start;
		u16 r_in_end = ctx->tile_cfg.r_in.end;

		u16 ltm_tile0_end_over_half_blk = 0;
		u16 ltm_tile0_end_blk = 0, ltm_tile0_end_blk_add_window = 0, ltm_tile0_end_blk_tot_blk_clip = 0;
		u16 ltm_tile1_start_blk = 0;
		u16 ltm_tile1_start_blk_add_window = 0, ltm_tile1_start_over_half_blk = 0;

		u16 ltm_tot_horz_blk_numm1 = UPPER((width), w_bit) - 1;

		ltm_tile0_end_over_half_blk = ((ctx->tile_cfg.l_in.end % (1 << w_bit)) >= (1 << (w_bit - 1))) ? 1 : 0;

		ltm_tile0_end_blk = ctx->tile_cfg.l_in.end >> (w_bit);
		ltm_tile0_end_blk_add_window = ltm_tile0_end_blk + 5 + 1 + ltm_tile0_end_over_half_blk;

		ltm_tile0_end_blk_tot_blk_clip = (ltm_tile0_end_blk_add_window > ltm_tot_horz_blk_numm1) ?
							ltm_tot_horz_blk_numm1 : ltm_tile0_end_blk_add_window;

		ltm_tile1_start_over_half_blk = ((r_in_str % (1 << w_bit)) >= (1 << (w_bit - 1))) ? 1 : 0;

		ltm_tile1_start_blk = r_in_str >> w_bit;
		ltm_tile1_start_blk_add_window = (ltm_tile1_start_blk - 5 - 2 + ltm_tile1_start_over_half_blk < 0) ?
						0 : (ltm_tile1_start_blk - 5 - 2 + ltm_tile1_start_over_half_blk);

		if (ctx->is_work_on_r_tile) { // Right tile
			reg_dma_blk_crop_str = ltm_tile1_start_blk_add_window;
			reg_dma_blk_crop_end = r_in_end >> w_bit;

			reg_blk_win_crop_str = ((r_in_str >> w_bit) - 2
						+ ltm_tile1_start_over_half_blk) - ltm_tile1_start_blk_add_window;
			reg_blk_win_crop_end = ltm_tot_horz_blk_numm1 - ltm_tile1_start_blk_add_window;

			reg_rs_out_crop_str = ((2 - ltm_tile1_start_over_half_blk) << w_bit) +
						(r_in_str % (1 << w_bit));
			reg_rs_out_crop_end = reg_rs_out_crop_str + (r_in_end - r_in_str);
		} else { // Left tile
			reg_dma_blk_crop_str = 0;
			reg_dma_blk_crop_end = ltm_tile0_end_blk_tot_blk_clip;

			reg_blk_win_crop_str = 0;
			reg_blk_win_crop_end = ltm_tile0_end_blk + 1 + ltm_tile0_end_over_half_blk;

			reg_rs_out_crop_str = 0;
			reg_rs_out_crop_end = ctx->tile_cfg.l_in.end;
		}

		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_0, DMA_BLK_CROP_STR, reg_dma_blk_crop_str);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_0, DMA_BLK_CROP_END, reg_dma_blk_crop_end);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_0, DMA_BLK_CROP_EN, 1);

		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_1, BLK_WIN_CROP_STR, reg_blk_win_crop_str);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_1, BLK_WIN_CROP_END, reg_blk_win_crop_end);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_1, BLK_WIN_CROP_EN, 1);

		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_2, RS_OUT_CROP_STR, reg_rs_out_crop_str);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_2, RS_OUT_CROP_END, reg_rs_out_crop_end);
		ISP_WR_BITS(ltm, REG_ISP_LTM_T, TILE_MODE_CTRL_2, RS_OUT_CROP_EN, 1);
	}
}

void ispblk_hist_edge_v_tile(struct isp_ctx *ctx)
{
	uintptr_t hist_edge_v = ctx->phys_regs[ISP_BLK_ID_HIST_EDGE_V];

	if (ctx->is_work_on_r_tile) {
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, 1);
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, OFFSETX, HIST_EDGE_V_OFFSETX,
						ctx->tile_cfg.l_in.end - ctx->tile_cfg.r_in.start - 3);
	} else { //left tile
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, 0);
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, OFFSETX, HIST_EDGE_V_OFFSETX, 0);
	}

	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, SW_CTL, TILE_NM, 1);
}

void ispblk_manr_tile(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MANR];
	uint32_t img_width = ctx->img_width;
	union REG_ISP_MMAP_C4 reg_c4;

	u16 w_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;
	u16 h_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit;

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_30, IMG_WIDTHM1_SW, img_width);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_30, IMG_HEIGHTM1_SW, ctx->img_height - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT, w_bit);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT, h_bit);

	reg_c4.raw = 0;
	reg_c4.bits.IMG_WIDTH_CROP = ((UPPER((UPPER(img_width, w_bit) * 48), 7) << 7) + 47) / 48 - 1;
	reg_c4.bits.IMG_HEIGHT_CROP = UPPER(ctx->img_height, h_bit) - 1;
	reg_c4.bits.CROP_ENABLE = 1;
	ISP_WR_REG(blk, REG_ISP_MMAP_T, REG_C4, reg_c4.raw);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_C8, CROP_W_END, UPPER(img_width, w_bit) - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_CC, CROP_H_END, UPPER(ctx->img_height, h_bit) - 1);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, IMG_WIDTH_CROP_SCALAR, img_width - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, IMG_HEIGHT_CROP_SCALAR, ctx->img_height - 1);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D0, CROP_ENABLE_SCALAR, 1);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D4, CROP_W_STR_SCALAR, ctx->tile_cfg.l_in.start);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D4, CROP_W_END_SCALAR, img_width - 1);

	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D8, CROP_H_STR_SCALAR, ctx->tile_cfg.l_in.start);
	ISP_WR_BITS(blk, REG_ISP_MMAP_T, REG_D8, CROP_H_END_SCALAR, ctx->img_height - 1);
}

void ispblk_dhz_tile(struct isp_ctx *ctx)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DHZ];

	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_5, TILE_NM, 1);

	if (ctx->is_work_on_r_tile) {
		ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_5, FMT_ST, ctx->tile_cfg.r_out.start - ctx->tile_cfg.r_in.start);
		ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_5, FMT_END, ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start);
	} else {
		ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_5, FMT_ST, ctx->tile_cfg.l_out.start);
		ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_5, FMT_END, ctx->tile_cfg.l_out.end);
	}
}

void ispblk_dci_tile(struct isp_ctx *ctx)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];

	if (ctx->is_work_on_r_tile) { //Right tile
		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_ZEROING_ENABLE, false);

		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ROI_GEO, DCI_ROI_WIDTHM1,
				ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start);

		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ROI_START, DCI_ROI_START_X,
				ctx->tile_cfg.r_out.start - ctx->tile_cfg.r_in.start);
	} else { //Left tile
		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_ZEROING_ENABLE, true);

		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ROI_GEO, DCI_ROI_WIDTHM1,
				ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start);
	}

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ROI_GEO, DCI_ROI_HEIGHTM1, ctx->img_height - 1);

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_ROI_ENABLE, true);
}

void ispblk_fbce_tile(struct isp_ctx *ctx)
{
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];

#define CU_SIZE 32
#define LOG_CU_SIZE 5

	u32 width_pad = ((ctx->img_width + CU_SIZE - 1) >> LOG_CU_SIZE) << LOG_CU_SIZE;
	u32 line_cu_num = width_pad >> LOG_CU_SIZE;
	u32 y_target_bit = ((CU_SIZE << 3) * g_y_cr[ISP_PRERAW_A]) / 100;
	u32 y_total_line_bit_budget = line_cu_num * y_target_bit;
	u32 uv_target_bit = ((CU_SIZE << 3) * g_uv_cr[ISP_PRERAW_A]) / 100;
	u32 uv_total_line_bit_budget = line_cu_num * uv_target_bit;

	u32 rc_strict_cu_idx = (((ctx->img_width) + CU_SIZE - 1) / CU_SIZE) - 1;

	u32 y_dma_size = ISP_ALIGN((((ISP_ALIGN(ctx->img_width, 32) * (ctx->img_height - 1)
				* g_y_cr[ISP_PRERAW_A]) / 100)
				+ ISP_ALIGN(ctx->img_width, 32)), 16) / 2;

	u32 c_dma_size = ISP_ALIGN((((ISP_ALIGN(ctx->img_width, 32) * (ctx->img_height / 2 - 1)
				* g_uv_cr[ISP_PRERAW_A]) / 100)
				+ ISP_ALIGN(ctx->img_width, 32)), 16) / 2;

	ISP_WR_BITS(fbce, REG_FBCE_T, REG_0, RC_STRICT_CU_IDX, rc_strict_cu_idx);

	ISP_WR_BITS(fbce, REG_FBCE_T, REG_1, Y_TOTAL_LINE_BIT_BUDGET, y_total_line_bit_budget);
	ISP_WR_BITS(fbce, REG_FBCE_T, REG_1, C_TOTAL_LINE_BIT_BUDGET, uv_total_line_bit_budget);

	ISP_WR_REG(fbce, REG_FBCE_T, REG_5, y_dma_size - 1);
	ISP_WR_REG(fbce, REG_FBCE_T, REG_6, c_dma_size - 1);
}

void ispblk_yuvtop_tile(struct isp_ctx *ctx)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];

	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_3, Y42_SEL, ctx->is_yuv_sensor);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGW_M1, ctx->img_width - 1);
	//ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGW_M1_CROP, ctx->img_width - 1);
	//ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1_CROP, YUV_TOP_IMGH_M1_CROP, ctx->img_height - 1);
}

/****************************************************************************
 *	Tuning Config
 ****************************************************************************/

/****************************************************************************
 *	Pre Be Tuning Config
 ****************************************************************************/
void ispblk_blc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_blc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	int id = _blc_find_hwid(cfg->inst);
	uintptr_t blc;

	if (!cfg->update)
		return;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_0, BLC_BYPASS, cfg->bypass);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_2, BLC_ENABLE, cfg->enable);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_R, cfg->roffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_GR, cfg->groffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_GB, cfg->gboffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_B, cfg->boffset);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_R, cfg->roffset_2nd);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_GR, cfg->groffset_2nd);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_GB, cfg->gboffset_2nd);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_B, cfg->boffset_2nd);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_R, cfg->rgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_GR, cfg->grgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_GB, cfg->gbgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_B, cfg->bgain);
}

void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dpc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_DPC0] : ctx->phys_regs[ISP_BLK_ID_DPC1];
	union REG_ISP_DPC_2 dpc_2;
	uint16_t i;

	if (!cfg->update)
		return;

	dpc_2.raw = ISP_RD_REG(ba, REG_ISP_DPC_T, DPC_2);
	dpc_2.bits.DPC_ENABLE = cfg->enable;
	dpc_2.bits.DPC_DYNAMICBPC_ENABLE = cfg->enable ? cfg->dynamicbpc_enable : 0;
	dpc_2.bits.DPC_STATICBPC_ENABLE = cfg->enable ? cfg->staticbpc_enable : 0;
	dpc_2.bits.DPC_CLUSTER_SIZE = cfg->cluster_size;
	ISP_WR_REG(ba, REG_ISP_DPC_T, DPC_2, dpc_2.raw);

	if (cfg->staticbpc_enable && (cfg->bp_cnt > 0) && (cfg->bp_cnt < 2048)) {
		ISP_WR_BITS(ba, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 1);
		ISP_WR_REG(ba, REG_ISP_DPC_T, DPC_MEM_ST_ADDR, 0x80000000);

		for (i = 0; i < cfg->bp_cnt; i++)
			ISP_WR_REG(ba, REG_ISP_DPC_T, DPC_MEM_W0,
				0x80000000 | cfg->bp_tbl[i]);

		// write 1 fff-fff to end
		ISP_WR_REG(ba, REG_ISP_DPC_T, DPC_MEM_W0, 0x80ffffff);
		ISP_WR_BITS(ba, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 0);
	}
	ISP_WR_REGS_BURST(ba, REG_ISP_DPC_T, DPC_3,
				cfg->dpc_cfg, cfg->dpc_cfg.DPC_3);
	ISP_WR_REGS_BURST(ba, REG_ISP_DPC_T, DPC_24,
				cfg->ir_cfg, cfg->ir_cfg.DPC_IR_RATIO);
}

void ispblk_wbg_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_wbg_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba;
	int id = -1;

	if (!cfg->update)
		return;

	id = _wbg_find_hwid(cfg->inst);
	if (id < 0) {
		vip_pr(CVI_ERR, "Wrong wbg inst\n");
		return;
	}

	ba = ctx->phys_regs[id];

	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_0, WBG_BYPASS, cfg->bypass);
	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_2, WBG_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_4, WBG_RGAIN, cfg->rgain);
	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_4, WBG_GGAIN, cfg->ggain);
	ISP_WR_BITS(ba, REG_ISP_WBG_T, WBG_5, WBG_BGAIN, cfg->bgain);
	ISP_WR_REG(ba, REG_ISP_WBG_T, WBG_34, cfg->rgain_fraction);
	ISP_WR_REG(ba, REG_ISP_WBG_T, WBG_38, cfg->ggain_fraction);
	ISP_WR_REG(ba, REG_ISP_WBG_T, WBG_3C, cfg->bgain_fraction);
}

void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_af_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_AF];

	union REG_ISP_AF_LOW_PASS_HORIZON low_pass_horizon;
	union REG_ISP_AF_HIGH_PASS_HORIZON_0 high_pass_horizon_0;
	union REG_ISP_AF_HIGH_PASS_HORIZON_1 high_pass_horizon_1;
	union REG_ISP_AF_HIGH_PASS_VERTICAL_0 high_pass_vertical_0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_AF_T, KICKOFF, AF_ENABLE, cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_AF_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);
	ISP_WR_REG(ba, REG_ISP_AF_T, BYPASS, !cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_AF_T, ENABLES, AF_HORIZON_0_ENABLE, cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_AF_T, ENABLES, AF_HORIZON_1_ENABLE, cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_AF_T, ENABLES, AF_VERTICAL_0_ENABLE, cfg->enable);

	ISP_WR_BITS(ba, REG_ISP_AF_T, ENABLES, AF_GAMMA_ENABLE, cfg->gamma_enable);
	ISP_WR_BITS(ba, REG_ISP_AF_T, ENABLES, AF_DPC_ENABLE, cfg->dpc_enable);

	ISP_WR_BITS(ba, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_X, cfg->offsetx);
	ISP_WR_BITS(ba, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_Y, cfg->offsety);

	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_WIDTH, cfg->block_width);
	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_HEIGHT, cfg->block_height);
	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_NUM_X, cfg->block_numx);
	ISP_WR_REG(ba, REG_ISP_AF_T, BLOCK_NUM_Y, cfg->block_numy);

	ISP_WR_REG(ba, REG_ISP_AF_T, HOR_LOW_PASS_VALUE_SHIFT, cfg->h_low_pass_value_shift);
	ISP_WR_REG(ba, REG_ISP_AF_T, OFFSET_HORIZONTAL_0, cfg->h_corning_offset_0);
	ISP_WR_REG(ba, REG_ISP_AF_T, OFFSET_HORIZONTAL_1, cfg->h_corning_offset_1);
	ISP_WR_REG(ba, REG_ISP_AF_T, OFFSET_VERTICAL, cfg->v_corning_offset);
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_Y_THRE, cfg->high_luma_threshold);

	low_pass_horizon.raw = 0;
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_0 = cfg->h_low_pass_coef[0];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_1 = cfg->h_low_pass_coef[1];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_2 = cfg->h_low_pass_coef[2];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_3 = cfg->h_low_pass_coef[3];
	low_pass_horizon.bits.AF_LOW_PASS_HORIZON_4 = cfg->h_low_pass_coef[4];
	ISP_WR_REG(ba, REG_ISP_AF_T, LOW_PASS_HORIZON, low_pass_horizon.raw);

	high_pass_horizon_0.raw = 0;
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_0 = cfg->h_high_pass_coef_0[0];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_1 = cfg->h_high_pass_coef_0[1];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_2 = cfg->h_high_pass_coef_0[2];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_3 = cfg->h_high_pass_coef_0[3];
	high_pass_horizon_0.bits.AF_HIGH_PASS_HORIZON_0_4 = cfg->h_high_pass_coef_0[4];
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_PASS_HORIZON_0, high_pass_horizon_0.raw);

	high_pass_horizon_1.raw = 0;
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_0 = cfg->h_high_pass_coef_1[0];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_1 = cfg->h_high_pass_coef_1[1];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_2 = cfg->h_high_pass_coef_1[2];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_3 = cfg->h_high_pass_coef_1[3];
	high_pass_horizon_1.bits.AF_HIGH_PASS_HORIZON_1_4 = cfg->h_high_pass_coef_1[4];
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_PASS_HORIZON_1, high_pass_horizon_1.raw);

	high_pass_vertical_0.raw = 0;
	high_pass_vertical_0.bits.AF_HIGH_PASS_VERTICAL_0_0 = cfg->v_high_pass_coef[0];
	high_pass_vertical_0.bits.AF_HIGH_PASS_VERTICAL_0_1 = cfg->v_high_pass_coef[1];
	high_pass_vertical_0.bits.AF_HIGH_PASS_VERTICAL_0_2 = cfg->v_high_pass_coef[2];
	ISP_WR_REG(ba, REG_ISP_AF_T, HIGH_PASS_VERTICAL_0, high_pass_vertical_0.raw);

	ISP_WR_BITS(ba, REG_ISP_AF_T, TH_LOW, AF_TH_LOW, cfg->th_low);
	ISP_WR_BITS(ba, REG_ISP_AF_T, TH_LOW, AF_TH_HIGH, cfg->th_high);
	ISP_WR_BITS(ba, REG_ISP_AF_T, GAIN_LOW, AF_GAIN_LOW, cfg->gain_low);
	ISP_WR_BITS(ba, REG_ISP_AF_T, GAIN_LOW, AF_GAIN_HIGH, cfg->gain_high);
	ISP_WR_BITS(ba, REG_ISP_AF_T, SLOP_LOW, AF_SLOP_LOW, cfg->slop_low);
	ISP_WR_BITS(ba, REG_ISP_AF_T, SLOP_LOW, AF_SLOP_HIGH, cfg->slop_high);
	ispblk_dma_config(ctx, ISP_BLK_ID_WDMA16, 0);
}

void ispblk_gms_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_gms_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_GMS];
	u32 img_width = (ctx->is_tile) ?
			ctx->tile_cfg.r_out.end + 1 :
			ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 img_height = ctx->isp_pipe_cfg[raw_num].crop.h;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_GMS_T, GMS_ENABLE, GMS_ENABLE, cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_GMS_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);

	if (cfg->enable) {
		if (cfg->x_section_size >= 512 || cfg->y_section_size >= 256 ||
			cfg->x_gap < 1 || cfg->y_gap < 1) {
			vip_pr(CVI_WARN, "[WARN] GMS tuning x_gap(%d), y_gap(%d), x_sec_size(%d), y_sec_size(%d)\n",
				cfg->x_gap, cfg->y_gap, cfg->x_section_size, cfg->y_section_size);
			return;
		}

		if (((cfg->x_section_size + 1) * 3 + cfg->offset_x + cfg->x_gap * 2 + 4) >= img_width) {
			vip_pr(CVI_WARN, "[WARN] GMS tuning x_sec_size(%d), ofst_x(%d), x_gap(%d), img_size(%d)\n",
					cfg->x_section_size, cfg->offset_x, cfg->x_gap, img_width);
			return;
		}

		if (((cfg->y_section_size + 1) * 3 + cfg->offset_y + cfg->y_gap * 2) > img_height) {
			vip_pr(CVI_WARN, "[WARN] GMS tuning y_sec_size(%d), ofst_y(%d), y_gap(%d), img_size(%d)\n",
					cfg->y_section_size, cfg->offset_y, cfg->y_gap, img_height);
			return;
		}

		ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_START_X, cfg->offset_x);
		ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_START_Y, cfg->offset_y);

		ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_X_SECTION_SIZE, cfg->x_section_size);
		ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_Y_SECTION_SIZE, cfg->y_section_size);

		ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_X_GAP, cfg->x_gap);
		ISP_WR_REG(ba, REG_ISP_GMS_T, GMS_Y_GAP, cfg->y_gap);

		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA17, 0);
	}
}

void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ge_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_DPC0] : ctx->phys_regs[ISP_BLK_ID_DPC1];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_DPC_T, DPC_2, GE_ENABLE, cfg->enable);

	ISP_WR_REGS_BURST(ba, REG_ISP_DPC_T, DPC_10,
				cfg->ge_cfg, cfg->ge_cfg.DPC_10);
}

void ispblk_preproc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_preproc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = 0;
	u8 mem_w_sel = 0, i = 0;
	union REG_ISP_PREPROCESS_IR_PREPROC_PROG_DATA		reg_wdata;
	union REG_ISP_PREPROCESS_IR_PREPROC_MEM_RATIO_PROG_DATA	reg_ratio_wdata;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case ISP_IR_PRE_PROC_ID_LE:
		ba = ctx->phys_regs[ISP_BLK_ID_IR_PRE_PROC_LE];
		break;
	case ISP_IR_PRE_PROC_ID_SE:
		ba = ctx->phys_regs[ISP_BLK_ID_IR_PRE_PROC_SE];
		break;
	default:
		vip_pr(CVI_INFO, "preproc wrong inst\n");
		return;
	}

	ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_CTRL, PREPROCESS_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_EN, 1);

	for (mem_w_sel = 0; mem_w_sel < 2; mem_w_sel++) {
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_WSEL, mem_w_sel);

		// w lut program
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 3); // wlut
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i++) {
			reg_wdata.raw = 0;
			reg_wdata.bits.MEM_WDATA = cfg->w_lut[i];
			reg_wdata.bits.MEM_W = 1;
			ISP_WR_REG(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_DATA, reg_wdata.raw);
		}

		// ratio lut program
		// R
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 0); // R=0
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i += 2) {
			reg_ratio_wdata.raw = 0;
			reg_ratio_wdata.bits.IR_RATIO_WDATA_E = cfg->r_ir_ratio[i];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_O = cfg->r_ir_ratio[i + 1];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_W = 1;
			ISP_WR_REG(ba, REG_ISP_PREPROCESS_T,
					IR_PREPROC_MEM_RATIO_PROG_DATA, reg_ratio_wdata.raw);
		}
		// G
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 1); // G=1
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i += 2) {
			reg_ratio_wdata.raw = 0;
			reg_ratio_wdata.bits.IR_RATIO_WDATA_E = cfg->g_ir_ratio[i];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_O = cfg->g_ir_ratio[i + 1];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_W = 1;
			ISP_WR_REG(ba, REG_ISP_PREPROCESS_T,
					IR_PREPROC_MEM_RATIO_PROG_DATA, reg_ratio_wdata.raw);
		}
		// B
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_IDX, 2); // B=2
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_ADDR, 0);
		ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_ST_ADDR, MEM_ST_W, 1);
		for (i = 0; i < 128; i += 2) {
			reg_ratio_wdata.raw = 0;
			reg_ratio_wdata.bits.IR_RATIO_WDATA_E = cfg->b_ir_ratio[i];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_O = cfg->b_ir_ratio[i + 1];
			reg_ratio_wdata.bits.IR_RATIO_WDATA_W = 1;
			ISP_WR_REG(ba, REG_ISP_PREPROCESS_T,
					IR_PREPROC_MEM_RATIO_PROG_DATA, reg_ratio_wdata.raw);
		}
	}

	ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_RSEL, 0);
	ISP_WR_BITS(ba, REG_ISP_PREPROCESS_T, IR_PREPROC_PROG_CTRL, MEM_PROG_EN, 0);
}

/****************************************************************************
 *	Postraw Tuning Config
 ****************************************************************************/
void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_bnr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_BNR];
	uint16_t i = 0;

	if (!cfg->update)
		return;

	if (cfg->enable) {
		if ((cfg->out_sel == 8) || ((cfg->out_sel >= 11) && (cfg->out_sel <= 15)))
			ISP_WO_BITS(ba, REG_ISP_BNR_T, OUT_SEL, BNR_OUT_SEL, cfg->out_sel);
		else
			vip_pr(CVI_ERR, "[ERR] BNR out_sel(%d) should be 8 and 11~15\n", cfg->out_sel);

		ISP_WR_REG(ba, REG_ISP_BNR_T, WEIGHT_INTRA_0, cfg->weight_intra_0);
		ISP_WR_REG(ba, REG_ISP_BNR_T, WEIGHT_INTRA_1, cfg->weight_intra_1);
		ISP_WR_REG(ba, REG_ISP_BNR_T, WEIGHT_INTRA_2, cfg->weight_intra_2);
		ISP_WR_REG(ba, REG_ISP_BNR_T, WEIGHT_NORM_1, cfg->weight_norm_1);
		ISP_WR_REG(ba, REG_ISP_BNR_T, WEIGHT_NORM_2, cfg->weight_norm_2);
		ISP_WR_REG(ba, REG_ISP_BNR_T, RES_K_SMOOTH, cfg->k_smooth);
		ISP_WR_REG(ba, REG_ISP_BNR_T, RES_K_TEXTURE, cfg->k_texture);

		ISP_WR_REG(ba, REG_ISP_BNR_T, LSC_EN, cfg->lsc_en);
		ISP_WR_REG(ba, REG_ISP_BNR_T, LSC_STRENTH, cfg->lsc_strenth);
		ISP_WR_REG(ba, REG_ISP_BNR_T, X_CENTER, cfg->lsc_centerx);
		ISP_WR_REG(ba, REG_ISP_BNR_T, Y_CENTER, cfg->lsc_centery);
		ISP_WR_REG(ba, REG_ISP_BNR_T, NORM_FACTOR, cfg->lsc_norm);

		ISP_WR_REGS_BURST(ba, REG_ISP_BNR_T, NS_LUMA_TH_R,
					cfg->bnr_1_cfg, cfg->bnr_1_cfg.NS_LUMA_TH_R);

		ISP_WR_REGS_BURST(ba, REG_ISP_BNR_T, LSC_RATIO,
					cfg->bnr_2_cfg, cfg->bnr_2_cfg.LSC_RATIO);

		ISP_WO_BITS(ba, REG_ISP_BNR_T, INDEX_CLR, BNR_INDEX_CLR, 1);
		for (i = 0; i < 8; i++)
			ISP_WR_REG(ba, REG_ISP_BNR_T, INTENSITY_SEL, cfg->intensity_sel[i]);

		for (i = 0; i < 256; i++)
			ISP_WR_REG(ba, REG_ISP_BNR_T, WEIGHT_LUT, cfg->weight_lut[i]);

		for (i = 0; i < 32; i++)
			ISP_WR_REG(ba, REG_ISP_BNR_T, LSC_LUT, cfg->lsc_gain_lut[i]);
	} else {
		ISP_WO_BITS(ba, REG_ISP_BNR_T, OUT_SEL, BNR_OUT_SEL, 1);
	}

	ISP_WO_BITS(ba, REG_ISP_BNR_T, SHADOW_RD_SEL, SHADOW_RD_SEL, 1);
}

void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ccm_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	int id = _ccm_find_hwid(cfg->inst);
	uintptr_t ccm;

	if (!cfg->update)
		return;

	if (id < 0)
		return;
	ccm = ctx->phys_regs[id];

	ISP_WR_BITS(ccm, REG_ISP_CCM_T, CCM_CTRL, CCM_ENABLE, cfg->enable);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_22, cfg->coef[2][2]);

}

void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lsc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSCM0];
	union REG_ISP_LSC_INTERPOLATION inter_p;

	if (!cfg->update)
		return;

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_ENABLE, cfg->enable);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_HDR_ENABLE, ctx->isp_pipe_cfg[raw_num].is_hdr_on ? 1 : 0);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_STRENGTH, cfg->strength);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_GAIN_BASE, cfg->gain_base);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_DUMMY, LSC_DEBUG, cfg->debug);

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_GAIN_3P9_0_4P8_1, cfg->gain_3p9_0_4p8_1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_RENORMALIZE_ENABLE, cfg->renormalize_enable);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_GAIN_BICUBIC_0_BILINEAR_1, cfg->gain_bicubic_0_bilinear_1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_BOUNDARY_INTERPOLATION_MODE, cfg->boundary_interpolation_mode);

	inter_p.raw = ISP_RD_REG(lsc, REG_ISP_LSC_T, INTERPOLATION);
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_LF_RANGE = cfg->boundary_interpolation_lf_range;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_UP_RANGE = cfg->boundary_interpolation_up_range;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_RT_RANGE = cfg->boundary_interpolation_rt_range;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_DN_RANGE = cfg->boundary_interpolation_dn_range;
	ISP_WR_REG(lsc, REG_ISP_LSC_T, INTERPOLATION, inter_p.raw);

}

void ispblk_fswdr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_fswdr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];
	uintptr_t manr_ba = ctx->phys_regs[ISP_BLK_ID_MANR];
	union REG_ISP_FUSION_FS_CTRL_0 fs_ctrl;
	union REG_ISP_FUSION_FS_CTRL_1 fs_ctrl_1;
	union REG_ISP_MMAP_00 mm_00;
	union REG_ISP_MMAP_34 mm_34;
	union REG_ISP_MMAP_40 mm_40;

	if (!cfg->update)
		return;

	fs_ctrl.raw = ISP_RD_REG(ba, REG_ISP_FUSION_T, FS_CTRL_0);
	fs_ctrl.bits.FS_ENABLE = cfg->enable;
	fs_ctrl.bits.FS_MC_ENABLE = cfg->mc_enable;
	fs_ctrl.bits.FS_OUT_SEL = cfg->out_sel;
	fs_ctrl.bits.FS_S_MAX = cfg->s_max;
	fs_ctrl.bits.FS_LUMA_MODE = cfg->luma_mode;
	ISP_WR_REG(ba, REG_ISP_FUSION_T, FS_CTRL_0, fs_ctrl.raw);

	fs_ctrl_1.raw = ISP_RD_REG(ba, REG_ISP_FUSION_T, FS_CTRL_1);
	fs_ctrl_1.bits.FS_FUSION_TYPE = cfg->fusion_type;
	fs_ctrl_1.bits.FS_FUSION_LWGT = cfg->fusion_lwgt;
	ISP_WR_REG(ba, REG_ISP_FUSION_T, FS_CTRL_1, fs_ctrl_1.raw);

	mm_00.raw = ISP_RD_REG(manr_ba, REG_ISP_MMAP_T, REG_00);
	mm_00.bits.MMAP_1_ENABLE = cfg->mmap_1_enable;
	mm_00.bits.MMAP_MRG_MODE = cfg->mmap_mrg_mode;
	mm_00.bits.MMAP_MRG_ALPH = cfg->mmap_mrg_alph;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_00, mm_00.raw);

	mm_34.raw = 0;
	mm_34.bits.V_THD_L = cfg->mmap_v_thd_l;
	mm_34.bits.V_THD_H = cfg->mmap_v_thd_h;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_34, mm_34.raw);

	mm_40.raw = 0;
	mm_40.bits.V_WGT_MIN = cfg->mmap_v_wgt_min;
	mm_40.bits.V_WGT_MAX = cfg->mmap_v_wgt_max;
	ISP_WR_REG(manr_ba, REG_ISP_MMAP_T, REG_40, mm_40.raw);

	ISP_WR_BITS(manr_ba, REG_ISP_MMAP_T, REG_3C, V_WGT_SLP, (cfg->mmap_v_wgt_slp & 0x7FFFF));

	ISP_WR_BITS(manr_ba, REG_ISP_MMAP_T, REG_3C, MOTION_LS_MODE, cfg->motion_ls_mode);
	ISP_WR_BITS(manr_ba, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_2, cfg->history_sel_2);

	ISP_WR_REGS_BURST(ba, REG_ISP_FUSION_T, FS_SE_GAIN,
				cfg->fswdr_cfg, cfg->fswdr_cfg.FS_LS_GAIN);
	ISP_WR_REGS_BURST(ba, REG_ISP_FUSION_T, FS_LUMA_THD_SE,
				cfg->fswdr_1_cfg, cfg->fswdr_1_cfg.FS_LUMA_THD_SE);
	ISP_WR_REGS_BURST(ba, REG_ISP_FUSION_T, FS_DITHER,
				cfg->fswdr_2_cfg, cfg->fswdr_2_cfg.FS_DITHER);
	ISP_WR_REGS_BURST(ba, REG_ISP_FUSION_T, FS_MOTION_LUT_IN,
				cfg->fswdr_3_cfg, cfg->fswdr_3_cfg.FS_MOTION_LUT_IN);
}

void ispblk_fswdr_update_rpt(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_fswdr_report *cfg)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];

	cfg->cal_pix_num = ISP_RD_REG(ba, REG_ISP_FUSION_T, FS_CALIB_OUT_0);
	cfg->diff_sum_r = ISP_RD_REG(ba, REG_ISP_FUSION_T, FS_CALIB_OUT_1);
	cfg->diff_sum_g = ISP_RD_REG(ba, REG_ISP_FUSION_T, FS_CALIB_OUT_2);
	cfg->diff_sum_b = ISP_RD_REG(ba, REG_ISP_FUSION_T, FS_CALIB_OUT_3);
}

void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_drc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	uintptr_t lmap0 = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_LMP0] : ctx->phys_regs[ISP_BLK_ID_LMP2];
	uintptr_t lmap1 = (raw_num == ISP_PRERAW_A) ?
			ctx->phys_regs[ISP_BLK_ID_LMP1] : ctx->phys_regs[ISP_BLK_ID_LMP3];
	uintptr_t addr;
	uint8_t arr_11[11], arr_30[30];

	union REG_ISP_LTM_TOP_CTRL ltm_ctrl;
	union REG_ISP_LMAP_LMP_0   lmp_0;
	union REG_ISP_LTM_BLK_SIZE reg_blk;

	if (!cfg->update)
		return;

	ltm_ctrl.raw = ISP_RD_REG(ba, REG_ISP_LTM_T, LTM_TOP_CTRL);
	ltm_ctrl.bits.LTM_ENABLE = cfg->ltm_enable;
	ltm_ctrl.bits.DTONE_EHN_EN = cfg->dark_enh_en;
	ltm_ctrl.bits.BTONE_EHN_EN = cfg->brit_enh_en;
	ltm_ctrl.bits.DARK_LCE_EN = cfg->dark_lce_en;
	ltm_ctrl.bits.BRIT_LCE_EN = cfg->brit_lce_en;
	ltm_ctrl.bits.DBG_ENABLE = cfg->dbg_en;
	ltm_ctrl.bits.DBG_MODE = cfg->dbg_mode;
	ltm_ctrl.bits.DE_MAX_THR = cfg->de_max_thr;
	ltm_ctrl.bits.LTM_DE_MAX_THR_ENABLE = cfg->de_max_thr_en;
	ISP_WR_REG(ba, REG_ISP_LTM_T, LTM_TOP_CTRL, ltm_ctrl.raw);

	if (!cfg->ltm_enable)
		return;

	lmp_0.raw = ISP_RD_REG(lmap0, REG_ISP_LMAP_T, LMP_0);
	lmp_0.bits.LMAP_ENABLE = cfg->lmap_enable;
	lmp_0.bits.LMAP_Y_MODE = cfg->lmap_y_mode;
	lmp_0.bits.LMAP_THD_L = cfg->lmap_thd_l;
	lmp_0.bits.LMAP_THD_H = cfg->lmap_thd_h;
	ISP_WR_REG(lmap0, REG_ISP_LMAP_T, LMP_0, lmp_0.raw);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		ISP_WR_REG(lmap1, REG_ISP_LMAP_T, LMP_0, lmp_0.raw);
	else {
		lmp_0.bits.LMAP_ENABLE = 0;
		ISP_WR_REG(lmap1, REG_ISP_LMAP_T, LMP_0, lmp_0.raw);
	}

	if ((g_lmp_cfg[raw_num].post_w_bit != cfg->lmap_w_bit ||
		g_lmp_cfg[raw_num].post_h_bit != cfg->lmap_h_bit) &&
		((cfg->lmap_w_bit > 2) && (cfg->lmap_h_bit > 2))) {

		uint32_t img_width = (ctx->is_tile) ?
					ctx->tile_cfg.r_out.end + 1 :
					ctx->img_width;

		g_lmp_cfg[raw_num].post_w_bit = cfg->lmap_w_bit;
		g_lmp_cfg[raw_num].post_h_bit = cfg->lmap_h_bit;

		g_lmp_cfg[raw_num].pre_chg[0] = g_lmp_cfg[raw_num].pre_chg[1] = true;
		g_lmp_cfg[raw_num].pre_w_bit = cfg->lmap_w_bit;
		g_lmp_cfg[raw_num].pre_h_bit = cfg->lmap_h_bit;

		reg_blk.bits.HORZ_BLK_SIZE = cfg->lmap_w_bit - 3;
		reg_blk.bits.BLK_WIDTHM1 = UPPER(img_width, cfg->lmap_w_bit) - 1;
		reg_blk.bits.BLK_HEIGHTM1 = UPPER(ctx->img_height, cfg->lmap_h_bit) - 1;
		reg_blk.bits.VERT_BLK_SIZE = cfg->lmap_h_bit - 3;
		ISP_WR_REG(ba, REG_ISP_LTM_T, LTM_BLK_SIZE, reg_blk.raw);

		ispblk_dma_config(ctx, ISP_BLK_ID_RDMA13, 0);
		ispblk_dma_config(ctx, ISP_BLK_ID_RDMA14, 0);

		if (_is_all_online(ctx))
			ispblk_lmap_chg_size(ctx, raw_num, ISP_FE_CH0);
	}

	ISP_WR_BITS(ba, REG_ISP_LTM_T, LTM_CURVE_QUAN_BIT, BCRV_QUAN_BIT, cfg->bcrv_quan_bit);
	ISP_WR_BITS(ba, REG_ISP_LTM_T, LTM_CURVE_QUAN_BIT, GCRV_QUAN_BIT_1, cfg->gcrv_quan_bit_1);

	// lmap0/1_lp_dist_wgt
	addr = ba + _OFST(REG_ISP_LTM_T, LTM_LMAP0_LP_DIST_WGT_0);
	memcpy(arr_11, cfg->lmap0_lp_dist_wgt, sizeof(arr_11));
	LTM_REG_ARRAY_UPDATE11(addr, arr_11);

	addr = ba + _OFST(REG_ISP_LTM_T, LTM_LMAP1_LP_DIST_WGT_0);
	memcpy(arr_11, cfg->lmap1_lp_dist_wgt, sizeof(arr_11));
	LTM_REG_ARRAY_UPDATE11(addr, arr_11);

	// lmap0/1_lp_diff_wgt
	addr = ba + _OFST(REG_ISP_LTM_T, LTM_LMAP0_LP_DIFF_WGT_0);
	memcpy(arr_30, cfg->lmap0_lp_diff_wgt, sizeof(arr_30));
	LTM_REG_ARRAY_UPDATE30(addr, arr_30);

	addr = ba + _OFST(REG_ISP_LTM_T, LTM_LMAP1_LP_DIFF_WGT_0);
	memcpy(arr_30, cfg->lmap1_lp_diff_wgt, sizeof(arr_30));
	LTM_REG_ARRAY_UPDATE30(addr, arr_30);

	// lp_be_dist_wgt
	addr = ba + _OFST(REG_ISP_LTM_T, LTM_BE_DIST_WGT_0);
	memcpy(arr_11, cfg->be_dist_wgt, sizeof(arr_11));
	LTM_REG_ARRAY_UPDATE11(addr, arr_11);

	// lp_de_dist_wgt
	addr = ba + _OFST(REG_ISP_LTM_T, LTM_DE_DIST_WGT_0);
	memcpy(arr_11, cfg->de_dist_wgt, sizeof(arr_11));
	LTM_REG_ARRAY_UPDATE11(addr, arr_11);

	// lp_de_luma_wgt
	addr = ba + _OFST(REG_ISP_LTM_T, LTM_DE_LUMA_WGT_0);
	memcpy(arr_30, cfg->de_luma_wgt, sizeof(arr_30));
	LTM_REG_ARRAY_UPDATE30(addr, arr_30);

	ispblk_ltm_b_lut(ctx, 0, cfg->brit_lut);
	ispblk_ltm_d_lut(ctx, 0, cfg->dark_lut);
	ispblk_ltm_g_lut(ctx, 0, cfg->deflt_lut);

	//Not change reg_ltm_tone_str_mode
	cfg->drc_2_cfg.LTM_LMAP_COMPUTE_CTRL_0.bits.LTM_TONE_STR_MODE =
		ISP_RD_BITS(ba, REG_ISP_LTM_T, LMAP_COMPUTE_CTRL_0, LTM_TONE_STR_MODE);
	ISP_WR_REGS_BURST(ba, REG_ISP_LTM_T, LTM_BE_STRTH_CTRL, cfg->drc_1_cfg,
		cfg->drc_1_cfg.LTM_BE_STRTH_CTRL);
	ISP_WR_REGS_BURST(ba, REG_ISP_LTM_T, LMAP_COMPUTE_CTRL_0, cfg->drc_2_cfg,
		cfg->drc_2_cfg.LTM_LMAP_COMPUTE_CTRL_0);
	ISP_WR_REGS_BURST(ba, REG_ISP_LTM_T, EE_GAIN_LUT_00, cfg->drc_3_cfg,
		cfg->drc_3_cfg.LTM_EE_GAIN_LUT_00);
}

void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_gamma_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_GAMMA];
	uint16_t i, idx, mem_w_sel;
	union REG_ISP_GAMMA_PROG_DATA reg_data;
	union REG_ISP_GAMMA_PROG_CTRL prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_CTRL, GAMMA_ENABLE, cfg->enable);
	prog_ctrl.raw = ISP_RD_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL);
	prog_ctrl.bits.GAMMA_WSEL	 = 0;
	prog_ctrl.bits.GAMMA_RSEL	 = 0;
	prog_ctrl.bits.GAMMA_PROG_EN = 1;
	prog_ctrl.bits.GAMMA_PROG_1TO3_EN = cfg->prog_1to3_enable ? 1 : 0;
	ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, prog_ctrl.raw);

	for (mem_w_sel = 0; mem_w_sel < 2; mem_w_sel++) {
		ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_WSEL, mem_w_sel);
		ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_ADDR, 0);
		ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_W, 1);

		if (cfg->prog_1to3_enable) {
			//R,G,B program same curve
			for (i = 0; i < 256; i += 2) {
				reg_data.raw = 0;
				reg_data.bits.GAMMA_DATA_E = cfg->lut_r[i];
				reg_data.bits.GAMMA_DATA_O = cfg->lut_r[i + 1];
				reg_data.bits.GAMMA_W = 1;
				ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
			}
		} else {
			//R,G,B program different curve
			for (idx = 0; idx < 3; idx++) {
				uint16_t *lut;

				if (idx == 0)
					lut = cfg->lut_r;
				else if (idx == 1)
					lut = cfg->lut_g;
				else if (idx == 2)
					lut = cfg->lut_b;
				for (i = 0; i < 256; i += 2) {
					// 0: progame R , 1: program G , 2: program B
					ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_MODE, idx);
					reg_data.raw = 0;
					reg_data.bits.GAMMA_DATA_E = lut[i];
					reg_data.bits.GAMMA_DATA_O = lut[i + 1];
					reg_data.bits.GAMMA_W = 1;
					ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
				}
			}
		}
	}

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_RSEL, 0);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_dhz_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dhz_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DHZ];

	if (!cfg->update)
		return;

	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_BYPASS, DEHAZE_ENABLE, cfg->enable);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_SMOOTH, DEHAZE_W, cfg->strength);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_SMOOTH, DEHAZE_TH_SMOOTH, cfg->th_smooth);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_1, DEHAZE_CUM_TH, cfg->cum_th);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_1, DEHAZE_HIST_TH, cfg->hist_th);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_2, DEHAZE_SW_AGLOBAL, cfg->sw_aglobal);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_2, DEHAZE_SW_DC_AGLOBAL_TRIG, cfg->sw_dc_aglobal_trig);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_3, DEHAZE_TMAP_MIN, cfg->tmap_min);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, REG_3, DEHAZE_TMAP_MAX, cfg->tmap_max);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_BYPASS, DEHAZE_LUMA_LUT_ENABLE, cfg->luma_lut_enable);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_BYPASS, DEHAZE_SKIN_LUT_ENABLE, cfg->skin_lut_enable);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_SKIN, DEHAZE_SKIN_CB, cfg->skin_cb);
	ISP_WR_BITS(dhz, REG_ISP_DHZ_T, DHZ_SKIN, DEHAZE_SKIN_CR, cfg->skin_cr);

	ISP_WR_REGS_BURST(dhz, REG_ISP_DHZ_T, REG_9, cfg->luma_cfg, cfg->luma_cfg.LUMA_00);
	ISP_WR_REGS_BURST(dhz, REG_ISP_DHZ_T, REG_17, cfg->skin_cfg, cfg->skin_cfg.SKIN_00);

}

void ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_clut_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	if (!cfg->update)
		return;

	if (cfg->is_update_partial) { //partail update table
		ispblk_clut_partial_update(ctx, cfg, raw_num);
	} else if (!_is_all_online(ctx)) {
		ispblk_clut_config(ctx, cfg->enable, cfg->r_lut, cfg->g_lut, cfg->b_lut);
	}
}

void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_csc_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_R2Y4];
	union REG_ISP_CSC_4 csc_4;
	union REG_ISP_CSC_5 csc_5;
	union REG_ISP_CSC_6 csc_6;
	union REG_ISP_CSC_7 csc_7;
	union REG_ISP_CSC_8 csc_8;
	union REG_ISP_CSC_9 csc_9;

	if (!cfg->update)
		return;

	ISP_WR_BITS(csc, REG_ISP_CSC_T, REG_0, CSC_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	csc_4.raw = 0;
	csc_4.bits.COEFF_00 = cfg->coeff[0] & 0x3FFF;
	csc_4.bits.COEFF_01 = cfg->coeff[1] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_4, csc_4.raw);

	csc_5.raw = 0;
	csc_5.bits.COEFF_02 = cfg->coeff[2] & 0x3FFF;
	csc_5.bits.COEFF_10 = cfg->coeff[3] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_5, csc_5.raw);

	csc_6.raw = 0;
	csc_6.bits.COEFF_11 = cfg->coeff[4] & 0x3FFF;
	csc_6.bits.COEFF_12 = cfg->coeff[5] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_6, csc_6.raw);

	csc_7.raw = 0;
	csc_7.bits.COEFF_20 = cfg->coeff[6] & 0x3FFF;
	csc_7.bits.COEFF_21 = cfg->coeff[7] & 0x3FFF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_7, csc_7.raw);

	csc_8.raw = 0;
	csc_8.bits.COEFF_22 = cfg->coeff[8] & 0x3FFF;
	csc_8.bits.OFFSET_0 = cfg->offset[0] & 0x7FF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_8, csc_8.raw);

	csc_9.raw = 0;
	csc_9.bits.OFFSET_1 = cfg->offset[1] & 0x7FF;
	csc_9.bits.OFFSET_2 = cfg->offset[2] & 0x7FF;
	ISP_WR_REG(csc, REG_ISP_CSC_T, REG_9, csc_9.raw);
}

void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dci_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	union REG_ISP_DCI_GAMMA_PROG_CTRL dci_gamma_ctrl;
	union REG_ISP_DCI_GAMMA_PROG_DATA dci_gamma_data;
	uint16_t i = 0, mem_w_sel = 0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_ENABLE, cfg->enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_DITHER_ENABLE, cfg->dither_enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_MAP_ENABLE, cfg->map_enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_HIST_ENABLE, cfg->hist_enable);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_PER1SAMPLE_ENABLE, cfg->per1sample_enable);
	ISP_WR_REG(dci, REG_ISP_DCI_T, DCI_DEMO_MODE, cfg->demo_mode);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL);
	dci_gamma_ctrl.bits.GAMMA_WSEL = 0;
	dci_gamma_ctrl.bits.GAMMA_PROG_EN = 1;
	dci_gamma_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, dci_gamma_ctrl.raw);

	for (mem_w_sel = 0; mem_w_sel < 2; mem_w_sel++) {
		ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_WSEL, mem_w_sel);

		for (i = 0; i < 256; i += 2) {
			dci_gamma_data.raw = 0;
			dci_gamma_data.bits.GAMMA_DATA_E = cfg->map_lut[i];
			dci_gamma_data.bits.GAMMA_DATA_O = cfg->map_lut[i + 1];
			dci_gamma_data.bits.GAMMA_W = 1;
			ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_DATA, dci_gamma_data.raw);
		}
	}

	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_RSEL, 0);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cacp_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uint16_t i;
	union REG_ISP_RGB_15 wdata;

	if (!cfg->update)
		return;

	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_ENABLE, cfg->enable);
	// 0 CA mode, 1 CP mode
	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_MODE, cfg->mode);
	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_ISO_RATIO, cfg->iso_ratio);

	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_MEM_SW_MODE, 1);
	if (cfg->mode == 0) {
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = cfg->ca_y_ratio_lut[i];
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_ISP_RGB_T, REG_15, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = ((cfg->cp_y_lut[i] << 16) |
					(cfg->cp_u_lut[i] << 8) | (cfg->cp_v_lut[i]));
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_ISP_RGB_T, REG_15, wdata.raw);
		}
	}
	ISP_WR_BITS(cacp, REG_ISP_RGB_T, REG_19, CACP_MEM_SW_MODE, 0);
}

void ispblk_preyee_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_preyee_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_PREYEE];
	union REG_ISP_EE_00  reg_0;
	union REG_ISP_EE_04  reg_4;
	union REG_ISP_EE_1C4 reg_1c4;
	union REG_ISP_EE_28 reg_28;
	uint32_t i, raw;

	if (!cfg->update)
		return;

	reg_0.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = cfg->enable;
	reg_0.bits.EE_DEBUG_MODE = cfg->dbg_mode;
	reg_0.bits.EE_TOTAL_CORING = cfg->total_coring;
	reg_0.bits.EE_TOTAL_GAIN = cfg->total_gain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_00, reg_0.raw);

	reg_4.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_04);
	reg_4.bits.EE_TOTAL_OSHTTHRD = cfg->total_oshtthrd;
	reg_4.bits.EE_TOTAL_USHTTHRD = cfg->total_ushtthrd;
	reg_4.bits.EE_DEBUG_SHIFT_BIT = cfg->debug_shift_bit;
	reg_4.bits.EE_PRE_PROC_GAIN = cfg->pre_proc_gain;
	reg_4.bits.EE_PRE_PROC_MODE = cfg->pre_proc_mode;
	reg_4.bits.EE_PRE_PROC_ENABLE = cfg->pre_proc_enable;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_04, reg_4.raw);

	reg_28.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_28);
	reg_28.bits.EE_DIRCAL_DGR4_NOD_NORGAIN = cfg->dgr4_nod_norgain;
	reg_28.bits.EE_DIRCAL_DGR4_DIR_ADJGAIN = cfg->dgr4_dir_adjgain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_28, reg_28.raw);

	reg_1c4.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_1C4);
	reg_1c4.bits.EE_SHTCTRL_OSHTGAIN = cfg->shtctrl_oshtgain;
	reg_1c4.bits.EE_SHTCTRL_USHTGAIN = cfg->shtctrl_ushtgain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_1C4, reg_1c4.raw);

	for (i = 0; i < 32; i += 2) {
		raw = cfg->luma_coring_lut[i] + (cfg->luma_coring_lut[i + 1] << 16);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_A4, (i / 2 * 0x4), raw);
	}
	ISP_WR_BITS(ba, REG_ISP_EE_T, REG_E4, EE_LUMA_CORING_LUT_32, cfg->luma_coring_lut[32]);

	for (i = 0; i < 32; i += 4) {
		raw = cfg->luma_shtctrl_lut[i] + (cfg->luma_shtctrl_lut[i + 1] << 8) +
			(cfg->luma_shtctrl_lut[i + 2] << 16) + (cfg->luma_shtctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_E8, i, raw);

		raw = cfg->delta_shtctrl_lut[i] + (cfg->delta_shtctrl_lut[i + 1] << 8) +
			(cfg->delta_shtctrl_lut[i + 2] << 16) + (cfg->delta_shtctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_10C, i, raw);

		raw = cfg->luma_adptctrl_lut[i] + (cfg->luma_adptctrl_lut[i + 1] << 8) +
			(cfg->luma_adptctrl_lut[i + 2] << 16) + (cfg->luma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_130, i, raw);

		raw = cfg->delta_adptctrl_lut[i] + (cfg->delta_adptctrl_lut[i + 1] << 8) +
			(cfg->delta_adptctrl_lut[i + 2] << 16) + (cfg->delta_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_154, i, raw);
	}
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_108, cfg->luma_shtctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_12C, cfg->delta_shtctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_150, cfg->luma_adptctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_174, cfg->delta_adptctrl_lut[32]);

	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_08, cfg->preyee_cfg, cfg->preyee_cfg.REG_08);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_30, cfg->preyee_1_cfg, cfg->preyee_1_cfg.REG_30);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_1C8, cfg->preyee_2_cfg, cfg->preyee_2_cfg.REG_1C8);
}

void ispblk_tnr_post_chg(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_MANR];
	union REG_ISP_MMAP_C4 reg_c4;
	struct isp_grid_s_info info;
	uint8_t dma_enable;

	info.w_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;
	info.h_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit;

	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT, info.w_bit);
	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT, info.h_bit);

	reg_c4.raw = ISP_RD_REG(ba, REG_ISP_MMAP_T, REG_C4);
	reg_c4.bits.CROP_ENABLE = 1;
	reg_c4.bits.IMG_WIDTH_CROP =
		((UPPER((UPPER(ctx->img_width, info.w_bit) * 48), 7) << 7) + 47) / 48 - 1;
	reg_c4.bits.IMG_HEIGHT_CROP = UPPER(ctx->img_height, info.h_bit) - 1;
	ISP_WR_REG(ba, REG_ISP_MMAP_T, REG_C4, reg_c4.raw);

	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_C8, CROP_W_END,
			UPPER(ctx->img_width, info.w_bit) - 1);
	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_CC, CROP_H_END,
			UPPER(ctx->img_height, info.h_bit) - 1);

	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA8, 0);
	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA9, 0);
	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA10, 0);
	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA11, 0);
	ispblk_dma_config(ctx, ISP_BLK_ID_RDMA12, 0);
	ispblk_dma_config(ctx, ISP_BLK_ID_WDMA26, 0);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) // hdr mode
		dma_enable = 0xa0;
	else // single mode
		dma_enable = 0x0a;

	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, dma_enable);
}

void ispblk_tnr_rgbmap_chg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num)
{
	uintptr_t preraw_fe = (raw_num == ISP_PRERAW_A)
		 ? ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0]
		 : ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	uint16_t w_grid_num, h_grid_num;
	u32 img_width = (ctx->is_tile) ?
			ctx->tile_cfg.r_out.end + 1 :
			ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 img_height = ctx->isp_pipe_cfg[raw_num].crop.h;

	if (g_rgbmap_chg_pre[raw_num][chn_num] == false)
		return;

	w_grid_num = UPPER(img_width, g_w_bit[raw_num]) - 1;
	h_grid_num = UPPER(img_height, g_h_bit[raw_num]) - 1;

	if (chn_num == ISP_FE_CH0) {
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER,
					LE_RGBMP_H_GRID_SIZE, g_w_bit[raw_num]);
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER,
					LE_RGBMP_V_GRID_SIZE, g_h_bit[raw_num]);

		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER,
					LE_RGBMP_H_GRID_NUMM1, w_grid_num);
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER,
					LE_RGBMP_V_GRID_NUMM1, h_grid_num);

		ispblk_dma_config(ctx, (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_WDMA2 : ISP_BLK_ID_WDMA8, 0);
	} else {
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER,
					SE_RGBMP_H_GRID_SIZE, g_w_bit[raw_num]);
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER,
					SE_RGBMP_V_GRID_SIZE, g_h_bit[raw_num]);

		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER,
					SE_RGBMP_H_GRID_NUMM1, w_grid_num);
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER,
					SE_RGBMP_V_GRID_NUMM1, h_grid_num);

		ispblk_dma_config(ctx, (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_WDMA4 : ISP_BLK_ID_WDMA10, 0);
	}

	g_rgbmap_chg_pre[raw_num][chn_num] = false;
}

void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_tnr_config *cfg,
	enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_MANR];
	uintptr_t y42 = ctx->phys_regs[ISP_BLK_ID_444422];

	union REG_ISP_MMAP_04 mm_04;
	union REG_ISP_MMAP_08 mm_08;
	union REG_ISP_MMAP_44 mm_44;
	union REG_ISP_444_422_8 reg_8;
	union REG_ISP_444_422_9 reg_9;

	if (!ctx->is_3dnr_on || !cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_00, BYPASS, !cfg->manr_enable);

	mm_04.raw = ISP_RD_REG(ba, REG_ISP_MMAP_T, REG_04);
	mm_04.bits.MMAP_0_LPF_00 = cfg->lpf[0][0];
	mm_04.bits.MMAP_0_LPF_01 = cfg->lpf[0][1];
	mm_04.bits.MMAP_0_LPF_02 = cfg->lpf[0][2];
	mm_04.bits.MMAP_0_LPF_10 = cfg->lpf[1][0];
	mm_04.bits.MMAP_0_LPF_11 = cfg->lpf[1][1];
	mm_04.bits.MMAP_0_LPF_12 = cfg->lpf[1][2];
	mm_04.bits.MMAP_0_LPF_20 = cfg->lpf[2][0];
	mm_04.bits.MMAP_0_LPF_21 = cfg->lpf[2][1];
	mm_04.bits.MMAP_0_LPF_22 = cfg->lpf[2][2];
	ISP_WR_REG(ba, REG_ISP_MMAP_T, REG_04, mm_04.raw);

	mm_08.raw = ISP_RD_REG(ba, REG_ISP_MMAP_T, REG_08);
	mm_08.bits.MMAP_0_MAP_GAIN = cfg->map_gain;
	mm_08.bits.MMAP_0_MAP_THD_L = cfg->map_thd_l;
	mm_08.bits.MMAP_0_MAP_THD_H = cfg->map_thd_h;
	ISP_WR_REG(ba, REG_ISP_MMAP_T, REG_08, mm_08.raw);

	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_1C, MMAP_0_LUMA_ADAPT_LUT_SLOPE_2,
		cfg->luma_adapt_lut_slope_2);
	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_F8, MMAP_0_MH_WGT, cfg->mh_wgt);
	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_0, cfg->history_sel_0);
	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_1, cfg->history_sel_1);
	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_F8, HISTORY_SEL_3, cfg->history_sel_3);

	if (_is_all_online(ctx) && cfg->rgbmap_w_bit > 3) {
		vip_pr(CVI_WARN, "RGBMAP_w_bit(%d) need <= 3 under on the fly mode\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 3;
	}

	reg_8.raw = ISP_RD_REG(y42, REG_ISP_444_422_T, REG_8);
	reg_8.bits.FORCE_DMA_DISABLE = (cfg->manr_enable) ? 0 : 0x3f;
	reg_8.bits.UV_ROUNDING_TYPE_SEL = cfg->uv_rounding_type_sel;
	reg_8.bits.TDNR_DEBUG_SEL = ((ctx->is_tile) ? 0x4 : 0x0) | cfg->tdnr_debug_sel;
	ISP_WR_REG(y42, REG_ISP_444_422_T, REG_8, reg_8.raw);

	if (!cfg->manr_enable)
		return;

	reg_9.raw = ISP_RD_REG(y42, REG_ISP_444_422_T, REG_9);
	reg_9.bits.AVG_MODE_WRITE  = cfg->avg_mode_write;
	reg_9.bits.DROP_MODE_WRITE = cfg->drop_mode_write;
	ISP_WR_REG(y42, REG_ISP_444_422_T, REG_9, reg_9.raw);

	mm_44.raw = 0;
	mm_44.bits.MMAP_MED_ENABLE	= cfg->med_enable;
	mm_44.bits.MMAP_MED_WGT		= cfg->med_wgt;
	ISP_WR_REG(ba, REG_ISP_MMAP_T, REG_44, mm_44.raw);

	ISP_WR_BITS(ba, REG_ISP_MMAP_T, REG_3C, MOTION_YV_LS_MODE, cfg->mtluma_mode);

	ISP_WR_REGS_BURST(ba, REG_ISP_MMAP_T, REG_0C, cfg->tnr_cfg, cfg->tnr_cfg.REG_0C);
	ISP_WR_REGS_BURST(ba, REG_ISP_MMAP_T, REG_20, cfg->tnr_5_cfg, cfg->tnr_5_cfg.REG_20);
	ISP_WR_REGS_BURST(ba, REG_ISP_MMAP_T, REG_4C, cfg->tnr_1_cfg, cfg->tnr_1_cfg.REG_4C);
	ISP_WR_REGS_BURST(ba, REG_ISP_MMAP_T, REG_70, cfg->tnr_2_cfg, cfg->tnr_2_cfg.REG_70);
	ISP_WR_REGS_BURST(ba, REG_ISP_MMAP_T, REG_A0, cfg->tnr_3_cfg, cfg->tnr_3_cfg.REG_A0);
	ISP_WR_REGS_BURST(y42, REG_ISP_444_422_T, REG_13, cfg->tnr_4_cfg, cfg->tnr_4_cfg.REG_13);

	if (g_w_bit[raw_num] != cfg->rgbmap_w_bit) {
		g_w_bit[raw_num] = cfg->rgbmap_w_bit;
		g_h_bit[raw_num] = cfg->rgbmap_h_bit;

		g_rgbmap_chg_pre[raw_num][0] = true;
		g_rgbmap_chg_pre[raw_num][1] = true;

		if (_is_all_online(ctx)) {
			ispblk_tnr_rgbmap_chg(ctx, raw_num, ISP_FE_CH0);

			ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = g_w_bit[raw_num];
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = g_h_bit[raw_num];
			ispblk_tnr_post_chg(ctx, raw_num);
		}
	}
}

void ispblk_ynr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ynr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_YNR];
	uint8_t i = 0;

	if (!cfg->update)
		return;

	ISP_WR_REG(ba, REG_ISP_YNR_T, WEIGHT_INTRA_0, cfg->weight_intra_0);
	ISP_WR_REG(ba, REG_ISP_YNR_T, WEIGHT_INTRA_1, cfg->weight_intra_1);
	ISP_WR_REG(ba, REG_ISP_YNR_T, WEIGHT_INTRA_2, cfg->weight_intra_2);
	ISP_WR_REG(ba, REG_ISP_YNR_T, WEIGHT_NORM_1, cfg->weight_norm_1);
	ISP_WR_REG(ba, REG_ISP_YNR_T, WEIGHT_NORM_2, cfg->weight_norm_2);

	if (cfg->enable) {
		if ((cfg->out_sel == 8) || ((cfg->out_sel >= 11) && (cfg->out_sel <= 15)))
			ISP_WR_REG(ba, REG_ISP_YNR_T, OUT_SEL, cfg->out_sel);
		else
			vip_pr(CVI_ERR, "[ERR] YNR out_sel(%d) should be 8 and 11~15\n", cfg->out_sel);

		ISP_WR_REG(ba, REG_ISP_YNR_T, VAR_TH, cfg->var_th);
		ISP_WR_REG(ba, REG_ISP_YNR_T, RES_K_SMOOTH, cfg->k_smooth);
		ISP_WR_REG(ba, REG_ISP_YNR_T, RES_K_TEXTURE, cfg->k_texture);
		ISP_WR_REG(ba, REG_ISP_YNR_T, ALPHA_GAIN, cfg->alpha_gain);

		ISP_WO_BITS(ba, REG_ISP_YNR_T, INDEX_CLR, YNR_INDEX_CLR, 1);
		for (i = 0; i < 8; i++)
			ISP_WR_REG(ba, REG_ISP_YNR_T, INTENSITY_SEL, cfg->intensity_sel[i]);

		for (i = 0; i < 64; i++)
			ISP_WR_REG(ba, REG_ISP_YNR_T, WEIGHT_LUT, cfg->weight_lut_h[i]);

		for (i = 0; i < 5; i++) {
			ISP_WR_REG(ba, REG_ISP_YNR_T, NS0_SLOPE, cfg->ns0_slope[i]);
			ISP_WR_REG(ba, REG_ISP_YNR_T, NS1_SLOPE, cfg->ns1_slope[i]);
		}

		for (i = 0; i < 6; i++) {
			ISP_WR_REG(ba, REG_ISP_YNR_T, NS0_LUMA_TH, cfg->ns0_luma_th[i]);
			ISP_WR_REG(ba, REG_ISP_YNR_T, NS1_LUMA_TH, cfg->ns1_luma_th[i]);
			ISP_WR_REG(ba, REG_ISP_YNR_T, NS0_OFFSET, cfg->ns0_offset_th[i]);
			ISP_WR_REG(ba, REG_ISP_YNR_T, NS1_OFFSET, cfg->ns1_offset_th[i]);
		}

		ISP_WR_REGS_BURST(ba, REG_ISP_YNR_T, MOTION_NS_TH,
					cfg->ynr_1_cfg, cfg->ynr_1_cfg.MOTION_NS_TH);

		ISP_WR_REGS_BURST(ba, REG_ISP_BNR_T, WEIGHT_SM,
					cfg->ynr_2_cfg, cfg->ynr_2_cfg.WEIGHT_SM);

		ISP_WR_REG(ba, REG_ISP_YNR_T, FILTER_MODE_ENABLE, cfg->filter_mode_enable);
		ISP_WR_REG(ba, REG_ISP_YNR_T, FILTER_MODE_ALPHA, cfg->filter_mode_alpha);
		ISP_WR_REG(ba, REG_ISP_YNR_T, MOTION_NS_CLIP_MAX, cfg->motion_ns_clip_max);
		ISP_WR_REG(ba, REG_ISP_YNR_T, RES_MAX, cfg->res_max);
		ISP_WR_REG(ba, REG_ISP_YNR_T, RES_MOTION_MAX, cfg->res_motion_max);

		for (i = 0; i < 16; i++) {
			ISP_WR_REG(ba, REG_ISP_YNR_T, RES_MOT_LUT, cfg->res_mot_lut[i]);
			ISP_WR_REG(ba, REG_ISP_YNR_T, MOTION_LUT, cfg->motion_lut[i]);
		}

	} else {
		ISP_WR_REG(ba, REG_ISP_YNR_T, OUT_SEL, 1);
	}
}

void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cnr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_CNR];

	union REG_ISP_CNR_ENABLE reg_enable;
	union REG_ISP_CNR_STRENGTH_MODE reg_strength_mode;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_00 reg_weight_lut_00;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_04 reg_weight_lut_04;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_08 reg_weight_lut_08;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_12 reg_weight_lut_12;
	union REG_ISP_CNR_INTENSITY_SEL_0 reg_intensity_sel_0;
	union REG_ISP_CNR_INTENSITY_SEL_4 reg_intensity_sel_4;
	union REG_ISP_CNR_MOTION_LUT_0 reg_motion_lut_00;
	union REG_ISP_CNR_MOTION_LUT_4 reg_motion_lut_04;
	union REG_ISP_CNR_MOTION_LUT_8 reg_motion_lut_08;
	union REG_ISP_CNR_MOTION_LUT_12 reg_motion_lut_12;

	if (!cfg->update)
		return;

	reg_enable.raw = ISP_RD_REG(ba, REG_ISP_CNR_T, CNR_ENABLE);
	reg_enable.bits.CNR_ENABLE = cfg->enable;
	reg_enable.bits.CNR_DIFF_SHIFT_VAL = cfg->diff_shift_val;
	reg_enable.bits.CNR_RATIO = cfg->ratio;
	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_ENABLE, reg_enable.raw);

	reg_strength_mode.raw = ISP_RD_REG(ba, REG_ISP_CNR_T, CNR_STRENGTH_MODE);
	reg_strength_mode.bits.CNR_STRENGTH_MODE = cfg->strength_mode;
	reg_strength_mode.bits.CNR_FUSION_INTENSITY_WEIGHT = cfg->fusion_intensity_weight;
	reg_strength_mode.bits.CNR_FLAG_NEIGHBOR_MAX_WEIGHT = cfg->flag_neighbor_max_weight;
	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_STRENGTH_MODE, reg_strength_mode.raw);

	ISP_WR_BITS(ba, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_DIFF_GAIN, cfg->diff_gain);
	ISP_WR_BITS(ba, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_MOTION_ENABLE, cfg->motion_enable);

	reg_weight_lut_00.raw = (u32)((cfg->weight_lut_inter[0] & 0x1F) |
				((cfg->weight_lut_inter[1] & 0x1F) << 8) |
				((cfg->weight_lut_inter[2] & 0x1F) << 16) |
				((cfg->weight_lut_inter[3] & 0x1F) << 24));
	reg_weight_lut_04.raw = (u32)((cfg->weight_lut_inter[4] & 0x1F) |
				((cfg->weight_lut_inter[5] & 0x1F) << 8) |
				((cfg->weight_lut_inter[6] & 0x1F) << 16) |
				((cfg->weight_lut_inter[7] & 0x1F) << 24));
	reg_weight_lut_08.raw = (u32)((cfg->weight_lut_inter[8] & 0x1F) |
				((cfg->weight_lut_inter[9] & 0x1F) << 8) |
				((cfg->weight_lut_inter[10] & 0x1F) << 16) |
				((cfg->weight_lut_inter[11] & 0x1F) << 24));
	reg_weight_lut_12.raw = (u32)((cfg->weight_lut_inter[12] & 0x1F) |
				((cfg->weight_lut_inter[13] & 0x1F) << 8) |
				((cfg->weight_lut_inter[14] & 0x1F) << 16) |
				((cfg->weight_lut_inter[15] & 0x1F) << 24));

	ISP_WR_REG(ba, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_00, reg_weight_lut_00.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_04, reg_weight_lut_04.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_08, reg_weight_lut_08.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_12, reg_weight_lut_12.raw);

	reg_intensity_sel_0.raw = (u32)((cfg->intensity_sel[0] & 0x1F) |
				((cfg->intensity_sel[1] & 0x1F) << 8) |
				((cfg->intensity_sel[2] & 0x1F) << 16) |
				((cfg->intensity_sel[3] & 0x1F) << 24));
	reg_intensity_sel_4.raw = (u32)((cfg->intensity_sel[4] & 0x1F) |
				((cfg->intensity_sel[5] & 0x1F) << 8) |
				((cfg->intensity_sel[6] & 0x1F) << 16) |
				((cfg->intensity_sel[7] & 0x1F) << 24));

	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_INTENSITY_SEL_0, reg_intensity_sel_0.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_INTENSITY_SEL_4, reg_intensity_sel_4.raw);

	reg_motion_lut_00.raw = (u32)((cfg->motion_lut[0] & 0xFF) |
				((cfg->motion_lut[1] & 0xFF) << 8) |
				((cfg->motion_lut[2] & 0xFF) << 16) |
				((cfg->motion_lut[3] & 0xFF) << 24));
	reg_motion_lut_04.raw = (u32)((cfg->motion_lut[4] & 0xFF) |
				((cfg->motion_lut[5] & 0xFF) << 8) |
				((cfg->motion_lut[6] & 0xFF) << 16) |
				((cfg->motion_lut[7] & 0xFF) << 24));
	reg_motion_lut_08.raw = (u32)((cfg->motion_lut[8] & 0xFF) |
				((cfg->motion_lut[9] & 0xFF) << 8) |
				((cfg->motion_lut[10] & 0xFF) << 16) |
				((cfg->motion_lut[11] & 0xFF) << 24));
	reg_motion_lut_12.raw = (u32)((cfg->motion_lut[12] & 0xFF) |
				((cfg->motion_lut[13] & 0xFF) << 8) |
				((cfg->motion_lut[14] & 0xFF) << 16) |
				((cfg->motion_lut[15] & 0xFF) << 24));

	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_MOTION_LUT_0, reg_motion_lut_00.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_MOTION_LUT_4, reg_motion_lut_04.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_MOTION_LUT_8, reg_motion_lut_08.raw);
	ISP_WR_REG(ba, REG_ISP_CNR_T, CNR_MOTION_LUT_12, reg_motion_lut_12.raw);
}

void ispblk_cac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cac = ctx->phys_regs[ISP_BLK_ID_CNR];

	if (!cfg->update)
		return;

	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_ENABLE, PFC_ENABLE, cfg->enable);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_STRENGTH_MODE, CNR_VAR_TH, cfg->var_th);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_STRENGTH_MODE, CNR_OUT_SEL, cfg->out_sel);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_PURPLE_TH, cfg->purple_th);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_CORRECT_STRENGTH, cfg->correct_strength);
	ISP_WR_BITS(cac, REG_ISP_CNR_T, CNR_PURPLE_TH, CNR_PURPLE_MODE, cfg->purple_mode);
	ISP_WR_REGS_BURST(cac, REG_ISP_CNR_T, CNR_PURPLE_CR, cfg->cac_cfg, cfg->cac_cfg.PURPLE_CR);
	ISP_WR_REGS_BURST(cac, REG_ISP_CNR_T, CNR_PURPLE_CB2, cfg->cac_2_cfg, cfg->cac_2_cfg.PURPLE_CB2);
}

void ispblk_rgbcac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_rgbcac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];

	if (!cfg->update)
		return;

	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_28, RGBCAC_ENABLE, cfg->enable);
	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_28, RGBCAC_OUT_SEL, cfg->out_sel);
	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_29, RGBCAC_VAR_TH, cfg->var_th);
	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_29, RGBCAC_PURPLE_TH, cfg->purple_th);
	ISP_WR_BITS(cfa, REG_ISP_CFA_T, REG_29, RGBCAC_CORRECT_STRENGTH, cfg->correct_strength);
	ISP_WR_REGS_BURST(cfa, REG_ISP_CFA_T, REG_30, cfg->rgbcac_cfg, cfg->rgbcac_cfg.REG_30);
}

void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ee_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_EE];
	union REG_ISP_EE_00  reg_0;
	union REG_ISP_EE_04  reg_4;
	union REG_ISP_EE_1C4 reg_1c4;
	union REG_ISP_EE_28 reg_28;
	uint32_t i, raw;

	if (!cfg->update)
		return;

	reg_0.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = cfg->enable;
	reg_0.bits.EE_DEBUG_MODE = cfg->dbg_mode;
	reg_0.bits.EE_TOTAL_CORING = cfg->total_coring;
	reg_0.bits.EE_TOTAL_GAIN = cfg->total_gain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_00, reg_0.raw);

	reg_4.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_04);
	reg_4.bits.EE_TOTAL_OSHTTHRD = cfg->total_oshtthrd;
	reg_4.bits.EE_TOTAL_USHTTHRD = cfg->total_ushtthrd;
	reg_4.bits.EE_DEBUG_SHIFT_BIT = cfg->debug_shift_bit;
	reg_4.bits.EE_PRE_PROC_BLENDING = cfg->pre_proc_blending;
	reg_4.bits.EE_PRE_PROC_GAIN = cfg->pre_proc_gain;
	reg_4.bits.EE_PRE_PROC_MODE = cfg->pre_proc_mode;
	reg_4.bits.EE_PRE_PROC_ENABLE = cfg->pre_proc_enable;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_04, reg_4.raw);

	reg_28.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_28);
	reg_28.bits.EE_DIRCAL_DGR4_NOD_NORGAIN = cfg->dgr4_nod_norgain;
	reg_28.bits.EE_DIRCAL_DGR4_DIR_ADJGAIN = cfg->dgr4_dir_adjgain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_28, reg_28.raw);

	reg_1c4.raw = ISP_RD_REG(ba, REG_ISP_EE_T, REG_1C4);
	reg_1c4.bits.EE_SHTCTRL_OSHTGAIN = cfg->shtctrl_oshtgain;
	reg_1c4.bits.EE_SHTCTRL_USHTGAIN = cfg->shtctrl_ushtgain;
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_1C4, reg_1c4.raw);

	for (i = 0; i < 32; i += 2) {
		raw = cfg->luma_coring_lut[i] + (cfg->luma_coring_lut[i + 1] << 16);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_A4, (i / 2 * 0x4), raw);
	}
	ISP_WR_BITS(ba, REG_ISP_EE_T, REG_E4, EE_LUMA_CORING_LUT_32, cfg->luma_coring_lut[32]);

	for (i = 0; i < 32; i += 4) {
		raw = cfg->luma_shtctrl_lut[i] + (cfg->luma_shtctrl_lut[i + 1] << 8) +
			(cfg->luma_shtctrl_lut[i + 2] << 16) + (cfg->luma_shtctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_E8, i, raw);

		raw = cfg->delta_shtctrl_lut[i] + (cfg->delta_shtctrl_lut[i + 1] << 8) +
			(cfg->delta_shtctrl_lut[i + 2] << 16) + (cfg->delta_shtctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_10C, i, raw);

		raw = cfg->luma_adptctrl_lut[i] + (cfg->luma_adptctrl_lut[i + 1] << 8) +
			(cfg->luma_adptctrl_lut[i + 2] << 16) + (cfg->luma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_130, i, raw);

		raw = cfg->delta_adptctrl_lut[i] + (cfg->delta_adptctrl_lut[i + 1] << 8) +
			(cfg->delta_adptctrl_lut[i + 2] << 16) + (cfg->delta_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, REG_ISP_EE_T, REG_154, i, raw);
	}
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_108, cfg->luma_shtctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_12C, cfg->delta_shtctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_150, cfg->luma_adptctrl_lut[32]);
	ISP_WR_REG(ba, REG_ISP_EE_T, REG_174, cfg->delta_adptctrl_lut[32]);

	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_08, cfg->ee_cfg, cfg->ee_cfg.REG_08);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_30, cfg->ee_1_cfg, cfg->ee_1_cfg.REG_30);
	ISP_WR_REGS_BURST(ba, REG_ISP_EE_T, REG_1C8, cfg->ee_2_cfg, cfg->ee_2_cfg.REG_1C8);
}

void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ca2_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_YUVTOP];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_ENABLE, cfg->enable);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_LUT_IN_0, cfg->lut_in[0]);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_ENABLE, CA_LITE_LUT_IN_1, cfg->lut_in[1]);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_IN, CA_LITE_LUT_IN_2, cfg->lut_in[2]);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_IN, CA_LITE_LUT_OUT_0, cfg->lut_out[0]);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_OUT, CA_LITE_LUT_OUT_1, cfg->lut_out[1]);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_OUT, CA_LITE_LUT_OUT_2, cfg->lut_out[2]);

	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_SLP, CA_LITE_LUT_SLP_0, cfg->lut_slp[0]);
	ISP_WR_BITS(ca_lite, REG_YUV_TOP_T, CA_LITE_LUT_SLP, CA_LITE_LUT_SLP_1, cfg->lut_slp[1]);
}

void ispblk_lscr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lscr_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = 0, ba_se = 0;
	uint8_t i = 0;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case ISP_LSCR_ID_PRE0_FE_LE:
	case ISP_LSCR_ID_PRE0_FE_SE:
		ba = ctx->phys_regs[ISP_BLK_ID_LSCR0];
		ba_se = ctx->phys_regs[ISP_BLK_ID_LSCR1];
		break;
	case ISP_LSCR_ID_PRE1_FE_LE:
	case ISP_LSCR_ID_PRE1_FE_SE:
		ba = ctx->phys_regs[ISP_BLK_ID_LSCR2];
		ba_se = ctx->phys_regs[ISP_BLK_ID_LSCR3];
		break;
	case ISP_LSCR_ID_PRE_BE_LE:
	case ISP_LSCR_ID_PRE_BE_SE:
		ba = ctx->phys_regs[ISP_BLK_ID_LSCR4];
		ba_se = ctx->phys_regs[ISP_BLK_ID_LSCR5];
		break;
	default:
		vip_pr(CVI_ERR, "Wrong lscr inst\n");
		return;
	}

	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_ENABLE, cfg->enable);
	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_STRNTH, cfg->strength);
	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_STRNTH_IR, cfg->strength_ir);
	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_NORM, cfg->norm);
	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_NORM_IR, cfg->norm_ir);
	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_CENTERX, cfg->centerx);
	ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_CENTERY, cfg->centery);

	for (i = 0; i < 32; i++) {
		ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_GAIN_LUT, cfg->gain_lut[i]);
		ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_GAIN_LUT_IR, cfg->gain_lut_ir[i]);
		if (cfg->inst >= ISP_LSCR_ID_PRE_BE_LE) {
			ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_GAIN_LUT_G, cfg->gain_lut1[i]);
			ISP_WR_REG(ba, REG_ISP_LSCR_T, LSCR_GAIN_LUT_B, cfg->gain_lut2[i]);
		}
	}

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_ENABLE, cfg->enable);
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_STRNTH, cfg->strength);
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_STRNTH_IR, cfg->strength_ir);
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_NORM, cfg->norm);
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_NORM_IR, cfg->norm_ir);
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_CENTERX, cfg->centerx);
		ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_CENTERY, cfg->centery);

		for (i = 0; i < 32; i++) {
			ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_GAIN_LUT, cfg->gain_lut[i]);
			ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_GAIN_LUT_IR, cfg->gain_lut_ir[i]);
			if (cfg->inst >= ISP_LSCR_ID_PRE_BE_LE) {
				ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_GAIN_LUT_G, cfg->gain_lut1[i]);
				ISP_WR_REG(ba_se, REG_ISP_LSCR_T, LSCR_GAIN_LUT_B, cfg->gain_lut2[i]);
			}
		}
	}
}

void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ae_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = 0;
	uint32_t raw;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case 0: // LE
		ba = ctx->phys_regs[ISP_BLK_ID_AEHIST0];
		break;
	case 1: // SE
		ba = ctx->phys_regs[ISP_BLK_ID_AEHIST1];
		break;
	default:
		vip_pr(CVI_ERR, "Wrong ae inst\n");
		return;
	}

	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_KICKOFF, AE_FACE_ENABLE, cfg->face_ae_enable);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, AE0_ENABLE, cfg->ae_enable);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, DMI_ENABLE, DMI_ENABLE, cfg->ae_enable);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, AE0_GAIN_ENABLE, cfg->ae_gain_enable);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, HIST0_ENABLE, cfg->hist_enable);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, HIST0_GAIN_ENABLE, cfg->hist_gain_enable);
	if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
		ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, IR_AE_ENABLE, cfg->ir_ae_enable);
		ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, IR_AE_GAIN_ENABLE, cfg->ir_ae_gain_enable);
		ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, IR_HIST_ENABLE, cfg->ir_hist_enable);
		ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_ENABLE, IR_HIST_GAIN_ENABLE, cfg->ir_hist_gain_enable);
	}
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_OFFSETX, cfg->ae_offsetx);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_OFFSETY, cfg->ae_offsety);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, cfg->ae_numx - 1);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, cfg->ae_numy - 1);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_WIDTH, cfg->ae_width);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, cfg->ae_height);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_AE_STS_DIV, cfg->ae_sts_div);
	if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_OFFSETX, cfg->ir_ae_offsetx);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_OFFSETY, cfg->ir_ae_offsety);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_NUMXM1, cfg->ir_ae_numx - 1);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_NUMYM1, cfg->ir_ae_numy - 1);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_WIDTH, cfg->ir_ae_width);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_HEIGHT, cfg->ir_ae_height);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, STS_IR_AE_STS_DIV, cfg->ir_ae_sts_div);
	}

	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, AE_HIST_BAYER_STARTING,
		AE_HIST_BAYER_STARTING, cfg->ae_hist_bayer_start);

	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, WBG_4, AE0_WBG_RGAIN, cfg->ae_wbg_rgain);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, WBG_4, AE0_WBG_GGAIN, cfg->ae_wbg_ggain);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, WBG_5, AE0_WBG_BGAIN, cfg->ae_wbg_bgain);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, WBG_5, AE1_WBG_BGAIN, cfg->ae1_wbg_bgain);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, WBG_6, AE1_WBG_RGAIN, cfg->ae1_wbg_rgain);
	ISP_WR_BITS(ba, REG_ISP_AE_HIST_T, WBG_6, AE1_WBG_GGAIN, cfg->ae1_wbg_ggain);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, WBG_7, cfg->ae_wbg_vgain);

	raw = cfg->ae_face_offset_x[0] + (cfg->ae_face_offset_y[0]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE0_LOCATION, raw);
	raw = cfg->ae_face_size_minus1_x[0] + (cfg->ae_face_size_minus1_y[0]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE0_SIZE, raw);
	raw = cfg->ae_face_offset_x[1] + (cfg->ae_face_offset_y[1]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE1_LOCATION, raw);
	raw = cfg->ae_face_size_minus1_x[1] + (cfg->ae_face_size_minus1_y[1]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE1_SIZE, raw);
	raw = cfg->ae_face_offset_x[2] + (cfg->ae_face_offset_y[2]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE2_LOCATION, raw);
	raw = cfg->ae_face_size_minus1_x[2] + (cfg->ae_face_size_minus1_y[2]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE2_SIZE, raw);
	raw = cfg->ae_face_offset_x[3] + (cfg->ae_face_offset_y[3]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE3_LOCATION, raw);
	raw = cfg->ae_face_size_minus1_x[3] + (cfg->ae_face_size_minus1_y[3]<<16);
	ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE3_SIZE, raw);
	if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
		raw = cfg->ir_ae_face_offset_x[0] + (cfg->ir_ae_face_offset_y[0]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE0_LOCATION, raw);
		raw = cfg->ir_ae_face_size_minus1_x[0] + (cfg->ir_ae_face_size_minus1_y[0]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE0_SIZE, raw);
		raw = cfg->ir_ae_face_offset_x[1] + (cfg->ir_ae_face_offset_y[1]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE1_LOCATION, raw);
		raw = cfg->ir_ae_face_size_minus1_x[1] + (cfg->ir_ae_face_size_minus1_y[1]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE1_SIZE, raw);
		raw = cfg->ir_ae_face_offset_x[2] + (cfg->ir_ae_face_offset_y[2]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE2_LOCATION, raw);
		raw = cfg->ir_ae_face_size_minus1_x[2] + (cfg->ir_ae_face_size_minus1_y[2]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE2_SIZE, raw);
		raw = cfg->ir_ae_face_offset_x[3] + (cfg->ir_ae_face_offset_y[3]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE3_LOCATION, raw);
		raw = cfg->ir_ae_face_size_minus1_x[3] + (cfg->ir_ae_face_size_minus1_y[3]<<16);
		ISP_WR_REG(ba, REG_ISP_AE_HIST_T, AE_FACE3_SIZE, raw);
	}

	switch (cfg->inst) {
	case 0:
		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA18, 0);
		break;
	case 1:
		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA21, 0);
		break;
	default:
		break;
	}
}

void ispblk_awb_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_awb_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = 0x0;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case 0:
		ba = ctx->phys_regs[ISP_BLK_ID_AWB0];
		break;
	case 2:
		//ba = ctx->phys_regs[ISP_BLK_ID_AWB2];
		vip_pr(CVI_ERR, "AWB post is unused currently\n");
		return;
	default:
		vip_pr(CVI_ERR, "Wrong awb inst\n");
		return;
	}

	ISP_WR_BITS(ba, REG_ISP_AWB_T, ENABLE, AWB_ENABLE, cfg->enable);
	ISP_WR_BITS(ba, REG_ISP_AWB_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);

	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_NUMXM1, cfg->awb_numx - 1);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_NUMYM1, cfg->awb_numy - 1);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_OFFSETX, cfg->awb_offsetx);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_OFFSETY, cfg->awb_offsety);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_WIDTH, cfg->awb_sub_win_w);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_HEIGHT, cfg->awb_sub_win_h);

	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_CORNER_AVG_EN, cfg->corner_avg_en);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_CORNER_SIZE, cfg->corner_size);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_STS_DIV, cfg->awb_sts_div);

	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_R_LOTHD, cfg->r_lower_bound);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_R_UPTHD, cfg->r_upper_bound);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_G_LOTHD, cfg->g_lower_bound);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_G_UPTHD, cfg->g_upper_bound);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_B_LOTHD, cfg->b_lower_bound);
	ISP_WR_REG(ba, REG_ISP_AWB_T, STS_B_UPTHD, cfg->b_upper_bound);
	ISP_WR_REG(ba, REG_ISP_AWB_T, BAYER_STARTING, cfg->bayer_start);

	switch (cfg->inst) {
	case 0:
		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA19, 0);
		break;
	case 2:
		ispblk_dma_config(ctx, ISP_BLK_ID_WDMA22, 0);
		break;
	default:
		break;
	}
}

void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_demosiac_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_CFA];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, REG_ISP_CFA_T, REG_0, CFA_ENABLE, cfg->cfa_enable);
	ISP_WR_BITS(ba, REG_ISP_CFA_T, REG_0, CFA_MOIRE_ENABLE, cfg->cfa_moire_enable);
	ISP_WR_BITS(ba, REG_ISP_CFA_T, REG_1, CFA_OUT_SEL, cfg->cfa_out_sel);

	ISP_WR_REG_LOOP_SHFT(ba, REG_ISP_CFA_T, GHP_LUT_0, 32, 4, cfg->cfa_ghp_lut, 8);

	ISP_WR_REGS_BURST(ba, REG_ISP_CFA_T, REG_3,
				cfg->demosiac_cfg, cfg->demosiac_cfg.REG_3);
	ISP_WR_REGS_BURST(ba, REG_ISP_CFA_T, REG_10,
				cfg->demosiac_1_cfg, cfg->demosiac_1_cfg.REG_10);
	ISP_WR_REGS_BURST(ba, REG_ISP_CFA_T, REG_22,
				cfg->demosiac_2_cfg, cfg->demosiac_2_cfg.REG_22);
}

void ispblk_hist_edge_v_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_hist_edge_v_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t hist_edge_v = ctx->phys_regs[ISP_BLK_ID_HIST_EDGE_V];

	if (!cfg->update)
		return;

	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_ENABLE, cfg->enable);
	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_LUMA_MODE, cfg->luma_mode);
	ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, cfg->enable);
	if (cfg->enable) {
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, OFFSETX, HIST_EDGE_V_OFFSETX, cfg->offset_x);
		ISP_WR_BITS(hist_edge_v, REG_HIST_EDGE_V_T, OFFSETY, HIST_EDGE_V_OFFSETY, cfg->offset_y);
	}
}

void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ycur_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uint16_t i = 0, mem_w_sel = 0;
	union REG_ISP_YCURVE_YCUR_PROG_DATA reg_data;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_CTRL, YCURVE_ENABLE, cfg->enable);

	if (cfg->enable) {
		ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 1);

		for (mem_w_sel = 0; mem_w_sel < 2; mem_w_sel++) {
			ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_WSEL, mem_w_sel);
			ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_ST_ADDR, YCUR_ST_ADDR, 0);
			ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_ST_ADDR, YCUR_ST_W, 1);
			ISP_WR_REG(ycur, REG_ISP_YCURVE_T, YCUR_PROG_MAX, cfg->lut_256);

			for (i = 0; i < 64; i += 2) {
				reg_data.raw = 0;
				reg_data.bits.YCUR_DATA_E = cfg->lut[i];
				reg_data.bits.YCUR_DATA_O = cfg->lut[i + 1];
				reg_data.bits.YCUR_W = 1;
				ISP_WR_REG(ycur, REG_ISP_YCURVE_T, YCUR_PROG_DATA, reg_data.raw);
			}
		}

		ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_RSEL, 0);
		ISP_WR_BITS(ycur, REG_ISP_YCURVE_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 0);
	}
}

void ispblk_mono_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_mono_config *cfg,
	const enum cvi_isp_raw raw_num)
{
	uintptr_t y42 = ctx->phys_regs[ISP_BLK_ID_444422];

	if (!cfg->update)
		return;

	ISP_WR_BITS(y42, REG_ISP_444_422_T, REG_5, FORCE_MONO_ENABLE, cfg->force_mono_enable);
}


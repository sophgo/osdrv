/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: isp_reg.h
 * Description:
 */

#ifndef _ISP_REG_H_
#define _ISP_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_fields.h"
#include "reg_blocks.h"
#include "vreg_blocks.h"

#define ISP_INTR_NUM            (138)
#define CSIMAC0_INTR_NUM        (155)
#define CSIMAC1_INTR_NUM        (156)
#define ISP_TOP_PHY_REG_BASE    (0x0A000000)
#define DRAM_PHY_BASE           (0x100000000)
#define ISP_BLK_REGS_BITW       (12)
#define ISP_BLK_ID_BITW         (8)
#define ISP_PRERAW_INST_NUM     (2)

#define VREG_SIZE               (sizeof(struct VREG_RESV))
#define ADMA_DESC_SIZE          (sizeof(struct ISPCQ_ADMA_DESC_T))

/* ISP REG FIELD DEFINE */

/* ISP BLOCK ADDR OFFSET DEFINE */
#define ISP_BLK_BA_PRE_RAW_BE      (0x00000000)
#define ISP_BLK_BA_CROP4           (0x00001000)
#define ISP_BLK_BA_CROP5           (0x00002000)
#define ISP_BLK_BA_BLC4            (0x00003000)
#define ISP_BLK_BA_BLC5            (0x00004000)
#define ISP_BLK_BA_FPN             (0x00005000)
#define ISP_BLK_BA_IR_PRE_PROC_LE  (0x00006000)
#define ISP_BLK_BA_IR_PRE_PROC_SE  (0x00006100)
#define ISP_BLK_BA_IR_PROC         (0x00007000)
#define ISP_BLK_BA_IR_AE0          (0x00008000)
#define ISP_BLK_BA_IR_AE1          (0x00009000)
#define ISP_BLK_BA_AEHIST0         (0x0000A000)
#define ISP_BLK_BA_AEHIST1         (0x0000B000)
#define ISP_BLK_BA_AWB0            (0x0000C000)
#define ISP_BLK_BA_AWB1            (0x0000D000) //removed
#define ISP_BLK_BA_GMS             (0x0000E000)
#define ISP_BLK_BA_AF              (0x0000F000)
#define ISP_BLK_BA_WBG0            (0x00020000)
#define ISP_BLK_BA_WBG1            (0x00021000)
#define ISP_BLK_BA_DPC0            (0x00022000)
#define ISP_BLK_BA_DPC1            (0x00022100)
#define ISP_BLK_BA_INV_WBG0        (0x00023000)
#define ISP_BLK_BA_INV_WBG1        (0x00024000)
#define ISP_BLK_BA_PCHK4           (0x00025000)
#define ISP_BLK_BA_PCHK5           (0x00026000)
#define ISP_BLK_BA_LSCR4           (0x00027000)
#define ISP_BLK_BA_LSCR5           (0x00027200)
/* PRE_RAW_FE_0 BLOCK */
#define ISP_BLK_BA_PRE_RAW_FE0     (0x00010000)
#define ISP_BLK_BA_CSIBDG0         (0x00010800)
#define ISP_BLK_BA_CROP0           (0x00011000)
#define ISP_BLK_BA_CROP1           (0x00011800)
#define ISP_BLK_BA_BLC0            (0x00012000)
#define ISP_BLK_BA_BLC1            (0x00012800)
#define ISP_BLK_BA_LMP0            (0x00013000)
#define ISP_BLK_BA_WBG11           (0x00013100)
#define ISP_BLK_BA_LMP1            (0x00013200)
#define ISP_BLK_BA_WBG12           (0x00013300)
#define ISP_BLK_BA_RGBMAP0         (0x00013800)
#define ISP_BLK_BA_WBG7            (0x00013900)
#define ISP_BLK_BA_RGBMAP1         (0x00013A00)
#define ISP_BLK_BA_WBG8            (0x00013B00)
#define ISP_BLK_BA_PCHK0           (0x00014000)
#define ISP_BLK_BA_PCHK1           (0x00014800)
#define ISP_BLK_BA_LSCR0           (0x00015000)
#define ISP_BLK_BA_LSCR1           (0x00015200)
#define ISP_BLK_BA_PRE_RAW_FE1     (0x00018000)
#define ISP_BLK_BA_CSIBDG1_R1      (0x00018800)
#define ISP_BLK_BA_CROP2           (0x00019000)
#define ISP_BLK_BA_CROP3           (0x00019800)
#define ISP_BLK_BA_BLC2            (0x0001A000)
#define ISP_BLK_BA_BLC3            (0x0001A800)
#define ISP_BLK_BA_LMP2            (0x0001B000)
#define ISP_BLK_BA_WBG13           (0x0001B100)
#define ISP_BLK_BA_LMP3            (0x0001B200)
#define ISP_BLK_BA_WBG14           (0x0001B300)
#define ISP_BLK_BA_RGBMAP2         (0x0001B800)
#define ISP_BLK_BA_WBG9            (0x0001B900)
#define ISP_BLK_BA_RGBMAP3         (0x0001BA00)
#define ISP_BLK_BA_WBG10           (0x0001BB00)
#define ISP_BLK_BA_PCHK2           (0x0001C000)
#define ISP_BLK_BA_PCHK3           (0x0001C800)
#define ISP_BLK_BA_LSCR2           (0x0001D000)
#define ISP_BLK_BA_LSCR3           (0x0001D200)

#define ISP_BLK_BA_RAWTOP          (0x00030000)
#define ISP_BLK_BA_CFA             (0x00031000)
#define ISP_BLK_BA_BNR             (0x0003C000)
#define ISP_BLK_BA_CROP6           (0x0003D000)
#define ISP_BLK_BA_CROP7           (0x0003E000)
#define ISP_BLK_BA_PCHK6           (0x00042000)
#define ISP_BLK_BA_PCHK7           (0x00043000)

#define ISP_BLK_BA_RGBTOP          (0x00050000)
#define ISP_BLK_BA_LSCM0           (0x00051000)
#define ISP_BLK_BA_CCM0            (0x00052000)
#define ISP_BLK_BA_CCM1            (0x00052100)
#define ISP_BLK_BA_CCM2            (0x00052200)
#define ISP_BLK_BA_CCM3            (0x00052300)
#define ISP_BLK_BA_CCM4            (0x00052400)
#define ISP_BLK_BA_MANR            (0x00052500)
#define ISP_BLK_BA_GAMMA           (0x00053000)
#define ISP_BLK_BA_CLUT            (0x00054000)
#define ISP_BLK_BA_DHZ             (0x00055000)
#define ISP_BLK_BA_R2Y4            (0x00056000)
#define ISP_BLK_BA_RGBDITHER       (0x00057000)
#define ISP_BLK_BA_PREYEE          (0x00058000)
#define ISP_BLK_BA_PCHK8           (0x00059000)
#define ISP_BLK_BA_PCHK9           (0x0005A000)
#define ISP_BLK_BA_DCI             (0x0005B000)
#define ISP_BLK_BA_HIST_EDGE_V     (0x0005C000)
#define ISP_BLK_BA_HDRFUSION       (0x0005D000)
#define ISP_BLK_BA_HDRLTM          (0x0005E000)
#define ISP_BLK_BA_AWB2            (0x0005F000)

#define ISP_BLK_BA_YUVTOP          (0x00060000)
#define ISP_BLK_BA_444422          (0x00061000)
#define ISP_BLK_BA_FBCE            (0x00061A00)
#define ISP_BLK_BA_FBCD            (0x00061D00)
#define ISP_BLK_BA_YUVDITHER       (0x00061E00)
#define ISP_BLK_BA_YNR             (0x00064000)
#define ISP_BLK_BA_CNR             (0x00065000)
#define ISP_BLK_BA_EE              (0x00066000)
#define ISP_BLK_BA_YCURVE          (0x00067000)
#define ISP_BLK_BA_CROP8           (0x00069000)
#define ISP_BLK_BA_CROP9           (0x0006A000)
#define ISP_BLK_BA_PCHK10          (0x0006D000)
#define ISP_BLK_BA_PCHK11          (0x0006E000)
#define ISP_BLK_BA_ISPTOP          (0x00070000)
#define ISP_BLK_BA_CSIBDG_LITE     (0x00073000)

/* ISP BLOCK ADDR OFFSET DEFINE */
#define ISP_BLK_BA_WDMA_COM        (0x00071000)
#define ISP_BLK_BA_WDMA0           (0x00071400)
#define ISP_BLK_BA_WDMA1           (0x00071420)
#define ISP_BLK_BA_WDMA2           (0x00071440)
#define ISP_BLK_BA_WDMA3           (0x00071460)
#define ISP_BLK_BA_WDMA4           (0x00071480)
#define ISP_BLK_BA_WDMA5           (0x000714A0)
#define ISP_BLK_BA_WDMA6           (0x000714C0)
#define ISP_BLK_BA_WDMA7           (0x000714E0)
#define ISP_BLK_BA_WDMA8           (0x00071500)
#define ISP_BLK_BA_WDMA9           (0x00071520)
#define ISP_BLK_BA_WDMA10          (0x00071540)
#define ISP_BLK_BA_WDMA11          (0x00071560)
#define ISP_BLK_BA_WDMA12          (0x00071580)
#define ISP_BLK_BA_WDMA13          (0x000715A0)
#define ISP_BLK_BA_WDMA14          (0x000715C0)
#define ISP_BLK_BA_WDMA15          (0x000715E0)
#define ISP_BLK_BA_WDMA16          (0x00071600)
#define ISP_BLK_BA_WDMA17          (0x00071620)
#define ISP_BLK_BA_WDMA18          (0x00071640)
#define ISP_BLK_BA_WDMA19          (0x00071660)
#define ISP_BLK_BA_WDMA20          (0x00071680)
#define ISP_BLK_BA_WDMA21          (0x000716A0)
#define ISP_BLK_BA_WDMA22          (0x000716C0)
#define ISP_BLK_BA_WDMA23          (0x000716E0)
#define ISP_BLK_BA_WDMA24          (0x00071700)
#define ISP_BLK_BA_WDMA25          (0x00071720)
#define ISP_BLK_BA_WDMA26          (0x00071740)
#define ISP_BLK_BA_WDMA27          (0x00071760)
#define ISP_BLK_BA_WDMA28          (0x00071780)
#define ISP_BLK_BA_WDMA29          (0x000717A0)
#define ISP_BLK_BA_WDMA30          (0x000717C0)
#define ISP_BLK_BA_WDMA31          (0x000717E0)
#define ISP_BLK_BA_RDMA_COM        (0x00072000)
#define ISP_BLK_BA_RDMA0           (0x00072400)
#define ISP_BLK_BA_RDMA1           (0x00072420)
#define ISP_BLK_BA_RDMA2           (0x00072440)
#define ISP_BLK_BA_RDMA3           (0x00072460)
#define ISP_BLK_BA_RDMA4           (0x00072480)
#define ISP_BLK_BA_RDMA5           (0x000724A0)
#define ISP_BLK_BA_RDMA6           (0x000724C0)
#define ISP_BLK_BA_RDMA7           (0x000724E0)
#define ISP_BLK_BA_RDMA8           (0x00072500)
#define ISP_BLK_BA_RDMA9           (0x00072520)
#define ISP_BLK_BA_RDMA10          (0x00072540)
#define ISP_BLK_BA_RDMA11          (0x00072560)
#define ISP_BLK_BA_RDMA12          (0x00072580)
#define ISP_BLK_BA_RDMA13          (0x000725A0)
#define ISP_BLK_BA_RDMA14          (0x000725C0)
#define ISP_BLK_BA_RDMA15          (0x000725E0)
#define ISP_BLK_BA_RDMA16          (0x00072600)
#define ISP_BLK_BA_RDMA17          (0x00072620)
#define ISP_BLK_BA_RDMA18          (0x00072640)

#define ISP_BLK_BA_CMDQ1           (0x0007F000)
#define ISP_BLK_BA_CMDQ2           (0x0007F400)
#define ISP_BLK_BA_CMDQ3           (0x0007F800)


#define MAP_ISP_BLOCK_ID(_ba)   (((_ba) >> ISP_BLK_REGS_BITW) \
				& ((1 << ISP_BLK_ID_BITW) - 1))

enum ISP_BLK_ID_T {
	ISP_BLK_ID_PRE_RAW_BE,
	ISP_BLK_ID_CROP4,
	ISP_BLK_ID_CROP5,
	ISP_BLK_ID_BLC4,
	ISP_BLK_ID_BLC5,
	ISP_BLK_ID_FPN, //5
	ISP_BLK_ID_IR_PRE_PROC_LE,
	ISP_BLK_ID_IR_PRE_PROC_SE,
	ISP_BLK_ID_IR_PROC,
	ISP_BLK_ID_IR_AE0,
	ISP_BLK_ID_IR_AE1,
	ISP_BLK_ID_AEHIST0, //10
	ISP_BLK_ID_AEHIST1,
	ISP_BLK_ID_AWB0,
	ISP_BLK_ID_AWB1, //removed
	ISP_BLK_ID_GMS,
	ISP_BLK_ID_AF, //15
	ISP_BLK_ID_WBG0,
	ISP_BLK_ID_WBG1,
	ISP_BLK_ID_DPC0,
	ISP_BLK_ID_DPC1,
	ISP_BLK_ID_INV_WBG0, //20
	ISP_BLK_ID_INV_WBG1,
	ISP_BLK_ID_PCHK4,
	ISP_BLK_ID_PCHK5,
	ISP_BLK_ID_LSCR4,
	ISP_BLK_ID_LSCR5,
	ISP_BLK_ID_PRE_RAW_FE0,
	ISP_BLK_ID_CSIBDG0, //25
	ISP_BLK_ID_CROP0,
	ISP_BLK_ID_CROP1,
	ISP_BLK_ID_BLC0,
	ISP_BLK_ID_BLC1,
	ISP_BLK_ID_LMP0, //30
	ISP_BLK_ID_WBG11,
	ISP_BLK_ID_LMP1,
	ISP_BLK_ID_WBG12,
	ISP_BLK_ID_RGBMAP0,
	ISP_BLK_ID_WBG7,
	ISP_BLK_ID_RGBMAP1, //35
	ISP_BLK_ID_WBG8,
	ISP_BLK_ID_PCHK0,
	ISP_BLK_ID_PCHK1,
	ISP_BLK_ID_LSCR0,
	ISP_BLK_ID_LSCR1,
	ISP_BLK_ID_PRE_RAW_FE1,
	ISP_BLK_ID_CSIBDG1_R1, //40
	ISP_BLK_ID_CROP2,
	ISP_BLK_ID_CROP3,
	ISP_BLK_ID_BLC2,
	ISP_BLK_ID_BLC3,
	ISP_BLK_ID_LMP2, //45
	ISP_BLK_ID_WBG13,
	ISP_BLK_ID_LMP3,
	ISP_BLK_ID_WBG14,
	ISP_BLK_ID_RGBMAP2,
	ISP_BLK_ID_WBG9, //50
	ISP_BLK_ID_RGBMAP3,
	ISP_BLK_ID_WBG10,
	ISP_BLK_ID_PCHK2,
	ISP_BLK_ID_PCHK3,
	ISP_BLK_ID_LSCR2,
	ISP_BLK_ID_LSCR3,
	ISP_BLK_ID_RAWTOP, //55
	ISP_BLK_ID_CFA,
	ISP_BLK_ID_BNR,
	ISP_BLK_ID_CROP6,
	ISP_BLK_ID_CROP7,
	ISP_BLK_ID_PCHK6, //60
	ISP_BLK_ID_PCHK7,
	ISP_BLK_ID_RGBTOP,
	ISP_BLK_ID_LSCM0,
	ISP_BLK_ID_CCM0,
	ISP_BLK_ID_CCM1, //65
	ISP_BLK_ID_CCM2,
	ISP_BLK_ID_CCM3,
	ISP_BLK_ID_CCM4,
	ISP_BLK_ID_MANR,
	ISP_BLK_ID_GAMMA, //70
	ISP_BLK_ID_CLUT,
	ISP_BLK_ID_DHZ,
	ISP_BLK_ID_R2Y4,
	ISP_BLK_ID_RGBDITHER,
	ISP_BLK_ID_PREYEE, //75
	ISP_BLK_ID_PCHK8,
	ISP_BLK_ID_PCHK9,
	ISP_BLK_ID_DCI,
	ISP_BLK_ID_HIST_EDGE_V,
	ISP_BLK_ID_HDRFUSION, //80
	ISP_BLK_ID_HDRLTM,
	ISP_BLK_ID_AWB2,
	ISP_BLK_ID_YUVTOP,
	ISP_BLK_ID_444422,
	ISP_BLK_ID_FBCE, //85
	ISP_BLK_ID_FBCD,
	ISP_BLK_ID_YUVDITHER,
	ISP_BLK_ID_YNR,
	ISP_BLK_ID_CNR,
	ISP_BLK_ID_EE, //90
	ISP_BLK_ID_YCURVE,
	ISP_BLK_ID_CROP8,
	ISP_BLK_ID_CROP9,
	ISP_BLK_ID_PCHK10,
	ISP_BLK_ID_PCHK11, //95
	ISP_BLK_ID_ISPTOP,
	ISP_BLK_ID_CSIBDG_LITE,
	/* bottom group, block_id duplicate */
	ISP_BLK_ID_WDMA_COM,
	ISP_BLK_ID_WDMA0,
	ISP_BLK_ID_WDMA1,
	ISP_BLK_ID_WDMA2,
	ISP_BLK_ID_WDMA3,
	ISP_BLK_ID_WDMA4,
	ISP_BLK_ID_WDMA5,
	ISP_BLK_ID_WDMA6,
	ISP_BLK_ID_WDMA7,
	ISP_BLK_ID_WDMA8,
	ISP_BLK_ID_WDMA9,
	ISP_BLK_ID_WDMA10,
	ISP_BLK_ID_WDMA11,
	ISP_BLK_ID_WDMA12,
	ISP_BLK_ID_WDMA13,
	ISP_BLK_ID_WDMA14,
	ISP_BLK_ID_WDMA15,
	ISP_BLK_ID_WDMA16,
	ISP_BLK_ID_WDMA17,
	ISP_BLK_ID_WDMA18,
	ISP_BLK_ID_WDMA19,
	ISP_BLK_ID_WDMA20,
	ISP_BLK_ID_WDMA21,
	ISP_BLK_ID_WDMA22,
	ISP_BLK_ID_WDMA23,
	ISP_BLK_ID_WDMA24,
	ISP_BLK_ID_WDMA25,
	ISP_BLK_ID_WDMA26,
	ISP_BLK_ID_WDMA27,
	ISP_BLK_ID_WDMA28,
	ISP_BLK_ID_WDMA29,
	ISP_BLK_ID_WDMA30,
	ISP_BLK_ID_WDMA31,
	ISP_BLK_ID_RDMA_COM,
	ISP_BLK_ID_RDMA0,
	ISP_BLK_ID_RDMA1,
	ISP_BLK_ID_RDMA2,
	ISP_BLK_ID_RDMA3,
	ISP_BLK_ID_RDMA4,
	ISP_BLK_ID_RDMA5,
	ISP_BLK_ID_RDMA6,
	ISP_BLK_ID_RDMA7,
	ISP_BLK_ID_RDMA8,
	ISP_BLK_ID_RDMA9,
	ISP_BLK_ID_RDMA10,
	ISP_BLK_ID_RDMA11,
	ISP_BLK_ID_RDMA12,
	ISP_BLK_ID_RDMA13,
	ISP_BLK_ID_RDMA14,
	ISP_BLK_ID_RDMA15,
	ISP_BLK_ID_RDMA16,
	ISP_BLK_ID_RDMA17,
	ISP_BLK_ID_RDMA18,
	ISP_BLK_ID_CMDQ1,
	ISP_BLK_ID_CMDQ2,
	ISP_BLK_ID_CMDQ3,
	ISP_BLK_ID_MAX
};

/* ISP REG ADDR TRANSLATION POLICY
 * REG_ADDR = BLOCK_BA + REG_OFFSET
 * BLOCK_BA = [PHYS/VIRT_BA, DRAM_BA]
 * REG_OFFSET = calculate from STRUCT_MACRO
 * STRUCT_MACRO from coda parsed by py script
 *
 * PHYS/VIRT_BA = TOP_BA + BLOCK_OFFSET
 *
 * BLOCK_ID = (REG_ADDR - ISP_BASE) >> 12 (0~255)
 *
 * 0x0A000000 : ISP_BASE
 *      ^^    : ISP_BLK_ID_BITW, BLOCK_ID
 *        ^^^ : ISP_BLK_REGS_BITW
 *   ^^^^^    : BLOCK_BA
 *        ^^^ : REG_OFFSET
 */
//inline uint32_t map_isp_block_id(uint32_t reg_phy_addr)
//{
//	return MAP_ISP_BLOCK_ID(reg_phy_addr);
//}

#ifdef __cplusplus
}
#endif

#endif /* _ISP_REG_H_ */

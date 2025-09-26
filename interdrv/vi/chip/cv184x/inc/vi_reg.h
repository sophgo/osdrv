#ifndef _ISP_REG_H_
#define _ISP_REG_H_

#include "reg.h"

#include "isp/vi_reg_fields.h"
#include "isp/vi_reg_blocks.h"

#define ISP_TOP_PHY_REG_BASE    (0x0A000000)

#define VREG_SIZE               (sizeof(struct VREG_RESV))
#define ADMA_DESC_SIZE          (sizeof(struct ISPCQ_ADMA_DESC_T))

/* ISP REG FIELD DEFINE */

/* ISP BLOCK ADDR OFFSET DEFINE */
#define ISP_BLK_BA_PRE_RAW_FE0               (0x00000000)
#define ISP_BLK_BA_CSIBDG0                   (0x00000200)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG0         (0x00000400)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG0_GB      (0x00000420)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG0_GR      (0x00000440)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG0_R       (0x00000460)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG1         (0x00000500)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG1_GB      (0x00000520)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG1_GR      (0x00000540)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG1_R       (0x00000560)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG2         (0x00000600)
#define ISP_BLK_BA_DMA_CTL_CSI0_BDG3         (0x00000700)
#define ISP_BLK_BA_PRE_RAW_FE0_LSC0          (0x00000800)
#define ISP_BLK_BA_PRE_RAW_FE0_LSC1          (0x00000900)
#define ISP_BLK_BA_DMA_CTL_FE0_CLSC_LE       (0x00000A00)
#define ISP_BLK_BA_AE_HIST_FE0               (0x00001000)
#define ISP_BLK_BA_DMA_CTL_FE0_AE_HIST_LE    (0x00001300)
#define ISP_BLK_BA_DMA_CTL_FE0_AE_HIST_SE    (0x00001400)

#define ISP_BLK_BA_PRE_RAW_FE1               (0x00004000)
#define ISP_BLK_BA_CSIBDG1                   (0x00004200)
#define ISP_BLK_BA_DMA_CTL_CSI1_BDG0         (0x00004400)
#define ISP_BLK_BA_DMA_CTL_CSI1_BDG1         (0x00004500)
#define ISP_BLK_BA_PRE_RAW_FE1_LSC0          (0x00004800)
#define ISP_BLK_BA_PRE_RAW_FE1_LSC1          (0x00004900)
#define ISP_BLK_BA_DMA_CTL_FE1_CLSC_LE       (0x00004A00)
#define ISP_BLK_BA_AE_HIST_FE1               (0x00005000)
#define ISP_BLK_BA_DMA_CTL_FE1_AE_HIST_LE    (0x00005300)
#define ISP_BLK_BA_DMA_CTL_FE1_AE_HIST_SE    (0x00005400)

#define ISP_BLK_BA_PRE_RAW_FE2               (0x00008000)
#define ISP_BLK_BA_CSIBDG2                   (0x00008200)
#define ISP_BLK_BA_DMA_CTL_CSI2_BDG0         (0x00008400)
#define ISP_BLK_BA_PRE_RAW_FE2_LSC0          (0x00008800)
#define ISP_BLK_BA_DMA_CTL_FE2_CLSC_LE       (0x00008A00)
#define ISP_BLK_BA_AE_HIST_FE2               (0x00009000)
#define ISP_BLK_BA_DMA_CTL_FE2_AE_HIST_LE    (0x00009300)

#define ISP_BLK_BA_WDMA_CORE1                (0x00020000)
#define ISP_BLK_BA_WDMA_CORE2                (0x00022000)
#define ISP_BLK_BA_WDMA_CORE3                (0x00024000)
#define ISP_BLK_BA_WDMA_CORE4                (0x00026000)

#define ISP_BLK_BA_RAWTOP0                   (0x00030000)
#define ISP_BLK_BA_AF                        (0x00034000)
#define ISP_BLK_BA_DMA_CTL_AF_W              (0x00034200)

#define ISP_BLK_BA_BLC_DG_WB0                (0x00038000)
#define ISP_BLK_BA_FUSION                    (0x00039000)
#define ISP_BLK_BA_MAPCURVE                  (0x0003A000)
#define ISP_BLK_BA_DPC                       (0x0003B000)
#define ISP_BLK_BA_BNR                       (0x0003C000)
#define ISP_BLK_BA_BLC_DG_WB1                (0x0003D000)

#define ISP_BLK_BA_DMA_CTL_LSCR_HIST         (0x0003E000)
#define ISP_BLK_BA_LSCR                      (0x0003F000)

#define ISP_BLK_BA_DRC                       (0x00040000)
#define ISP_BLK_BA_DMA_CTL_DRC_POLY_R        (0x00040400)
#define ISP_BLK_BA_DMA_CTL_DRC_POLY_W        (0x00040500)
#define ISP_BLK_BA_DMA_CTL_DRC_HIST          (0x00040600)

#define ISP_BLK_BA_CFA                       (0x00041000)
#define ISP_BLK_BA_RAWTOP_LSC1               (0x00043000)
#define ISP_BLK_BA_RAWTOP1                   (0x00046000)

#define ISP_BLK_BA_RGBTOP                    (0x00050000)
#define ISP_BLK_BA_PRE_EE_EXT                (0x00051000)
#define ISP_BLK_BA_PFR                       (0x00052000)
#define ISP_BLK_BA_CCM                       (0x00053000)
#define ISP_BLK_BA_RGBGAMMA                  (0x00054000)
#define ISP_BLK_BA_RGB_DITHER                (0x00056000)
#define ISP_BLK_BA_CLUT                      (0x00057000)
#define ISP_BLK_BA_DMA_CTL_CLUT_R            (0x00057400)
#define ISP_BLK_BA_CSC                       (0x00058000)

#define ISP_BLK_BA_YUVTOP                    (0x00060000)
#define ISP_BLK_BA_CA                        (0x00062000)
#define ISP_BLK_BA_CA_LITE                   (0x00063000)
#define ISP_BLK_BA_PRE_EE_FRONT              (0x00064000)
#define ISP_BLK_BA_CNR                       (0x00065000)
#define ISP_BLK_BA_DMA_CTL_CNR_Y_W           (0x00065200)
#define ISP_BLK_BA_DMA_CTL_CNR_C_W           (0x00065300)
#define ISP_BLK_BA_DMA_CTL_CNR_Y_R           (0x00065400)
#define ISP_BLK_BA_DMA_CTL_CNR_C_R           (0x00065500)
#define ISP_BLK_BA_PRE_EE_BACK               (0x00066000)
#define ISP_BLK_BA_YCURVE                    (0x00067000)
#define ISP_BLK_BA_TNR                       (0x00068000)
#define ISP_BLK_BA_DMA_CTL_TNR_LD_Y          (0x00068200)
#define ISP_BLK_BA_DMA_CTL_TNR_LD_C          (0x00068300)
#define ISP_BLK_BA_DMA_CTL_TNR_LD_MV         (0x00068400)
#define ISP_BLK_BA_DMA_CTL_TNR_LD_MO         (0x00068500)
#define ISP_BLK_BA_DMA_CTL_TNR_LD_FCB        (0x00068600)
#define ISP_BLK_BA_DMA_CTL_TNR_ST_Y          (0x00068700)
#define ISP_BLK_BA_DMA_CTL_TNR_ST_C          (0x00068800)
#define ISP_BLK_BA_DMA_CTL_TNR_ST_MV         (0x00068900)
#define ISP_BLK_BA_DMA_CTL_TNR_ST_MO         (0x00068A00)
#define ISP_BLK_BA_DMA_CTL_TNR_ST_FCB        (0x00068B00)
#define ISP_BLK_BA_DMA_CTL_TNR_ST_MSP        (0x00068C00)
#define ISP_BLK_BA_FBCD                      (0x00068D00)
#define ISP_BLK_BA_FBCE                      (0x00068E00)
#define ISP_BLK_BA_YUV_DITHER                (0x00068F00)
#define ISP_BLK_BA_YUV_CROP_Y                (0x00069000)
#define ISP_BLK_BA_DMA_CTL_YUV_CROP_Y        (0x00069100)
#define ISP_BLK_BA_YUV_CROP_C                (0x0006A000)
#define ISP_BLK_BA_DMA_CTL_YUV_CROP_C        (0x0006A100)
#define ISP_BLK_BA_LDCI                      (0x0006B000)
#define ISP_BLK_BA_DMA_CTL_LDCI_R            (0x0006B080)
#define ISP_BLK_BA_DMA_CTL_LDCI_W            (0x0006B100)
#define ISP_BLK_BA_DMA_CTL_LDCI_HIST         (0x0006B120)
#define ISP_BLK_BA_LDCI_MAP_CORE             (0x0006B140)
#define ISP_BLK_BA_POST_EE                   (0x0006C000)
#define ISP_BLK_BA_DMA_CTL_YUV_RDMA_Y        (0x0006D000)
#define ISP_BLK_BA_DMA_CTL_YUV_RDMA_C        (0x0006E000)
#define ISP_BLK_BA_RESIZE                    (0x0006F000)
#define ISP_BLK_BA_DMA_CTL_RESIZE            (0x0006F100)

#define ISP_BLK_BA_ISPTOP                    (0x00070000)
#define ISP_BLK_BA_RDMA_CORE1                (0x00072000)
#define ISP_BLK_BA_RDMA_CORE2                (0x00074000)
#define ISP_BLK_BA_RDMA_CORE3                (0x00078000)
#define ISP_BLK_BA_CSIBDG0_LITE              (0x00076000)
#define ISP_BLK_BA_DMA_CTL_BT0_LITE0         (0x00076200)
#define ISP_BLK_BA_DMA_CTL_BT0_LITE1         (0x00076300)
#define ISP_BLK_BA_DMA_CTL_BT0_LITE2         (0x00076400)
#define ISP_BLK_BA_DMA_CTL_BT0_LITE3         (0x00076500)
#define ISP_BLK_BA_PRE_RAW_VI_SEL            (0x0007F400)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_LE     (0x0007F500)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_LE_GB  (0x0007F520)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_LE_GR  (0x0007F540)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_LE_R   (0x0007F560)

#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_SE     (0x0007F600)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_SE_GB  (0x0007F620)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_SE_GR  (0x0007F640)
#define ISP_BLK_BA_DMA_CTL_PRE_VI_SEL_SE_R   (0x0007F660)
#define ISP_BLK_BA_PRE_RAW_VI_SEL_CROP_LE    (0x0007F800)
#define ISP_BLK_BA_PRE_RAW_VI_SEL_CROP_SE    (0x0007F900)
#define ISP_BLK_BA_CMDQ                      (0x0007FC00)

#define ISP_RD_REG_BA(_BA) \
	(_reg_read(_BA))
#define ISP_WR_REG_BA(_BA, _V) \
	(_reg_write((_BA), (_V)))

#define ISP_RD_REG(_BA, _BLK_T, _REG) \
	(_reg_read(_BA+_OFST(_BLK_T, _REG)))

#define ISP_RD_BITS(_BA, _BLK_T, _REG, _FLD) \
	({\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_OFST(_BLK_T, _REG));\
		_r.bits._FLD;\
	})

#define ISP_WR_REG(_BA, _BLK_T, _REG, _V) \
	(_reg_write((_BA+_OFST(_BLK_T, _REG)), _V))

#define ISP_WR_REG_OFT(_BA, _BLK_T, _REG, _OFT, _V) \
	(_reg_write((_BA+_OFST(_BLK_T, _REG) + _OFT), _V))

#define ISP_WR_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_OFST(_BLK_T, _REG));\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define ISP_WO_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = 0;\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define ISP_WR_REGS_BURST(_BA, _BLK_T, _REG, _SIZE, _STR)\
	do {\
		u32 k = 0;\
		uintptr_t ofst = _OFST(_BLK_T, _REG);\
		for (; k < sizeof(_SIZE) / 0x4; k++) {\
			u32 val = (&_STR + k)->raw;\
			_reg_write((_BA + ofst + (k * 0x4)), val);\
		} \
	} while (0)

#define ISP_WR_REG_LOOP_SHFT(_BA, _BLK_T, _REG, _TOTAL_SIZE, _SFT_SIZE, _LUT, _SFT_BIT) \
	do {\
		u16 i = 0, j = 0;\
		u32 val = 0;\
		for (; i < _TOTAL_SIZE / _SFT_SIZE; i++) {\
			val = 0;\
			for (j = 0; j < _SFT_SIZE; j++) {\
				val += (_LUT[(i * _SFT_SIZE) + j] << (_SFT_BIT * j));\
			} \
			_reg_write((_BA + _OFST(_BLK_T, _REG) + (i * 0x4)), val);\
		} \
	} while (0)

#define REG_ARRAY_UPDATE2_SIZE(addr, array, size)		\
	do {							\
		u16 i;					\
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
		u16 i;					\
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
		u32 val;                                                 \
		val = array[0] | (array[1] << 5) | (array[2] << 10) |         \
		      (array[3] << 15) | (array[4] << 20) | (array[5] << 25); \
		_reg_write(addr, val);                                        \
		val = array[6] | (array[7] << 5) | (array[8] << 10) |         \
		      (array[9] << 15) | (array[10] << 20);                   \
		_reg_write(addr + 4, val);                                    \
	} while (0)

#define LTM_REG_ARRAY_UPDATE30(addr, array)                                   \
	do {                                                                  \
		u8 i, j;                                                 \
		u32 val;                                                 \
		for (i = 0, j = 0; i < ARRAY_SIZE(array); i += 6, j++) {      \
			val = array[i] | (array[i + 1] << 5) |                \
			      (array[i + 2] << 10) | (array[i + 3] << 15) |   \
			      (array[i + 4] << 20) | (array[i + 5] << 25);    \
			_reg_write(addr + j * 4, val);                        \
		}                                                             \
	} while (0)

#endif //_ISP_REG_H_

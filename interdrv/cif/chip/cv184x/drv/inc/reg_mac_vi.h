/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_mac_vi.h
 * Description:HW register description
 */

#ifndef _REG_MAC_VI_H_
#define _REG_MAC_VI_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_SENSOR_MAC_VI_00 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_MODE                 : 3;
		uint32_t BT_DEMUX_ENABLE                 : 1;
		uint32_t CSI_CTRL_ENABLE                 : 1;
		uint32_t CSI_VS_INV                      : 1;
		uint32_t CSI_HS_INV                      : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t SUBLVDS_CTRL_ENABLE             : 1;
		uint32_t SUBLVDS_VS_INV                  : 1;
		uint32_t SUBLVDS_HS_INV                  : 1;
		uint32_t SUBLVDS_HDR_INV                 : 1;
		uint32_t SLVSEC_CTRL_ENABLE              : 1;
		uint32_t SLVSEC_VS_INV                   : 1;
		uint32_t SLVSEC_HS_INV                   : 1;
		uint32_t _rsv_15                         : 1;
		uint32_t MASK_UP                         : 1;
		uint32_t SHRD_SEL                        : 1;
		uint32_t SW_UP                           : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t DBG_SEL                         : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_10 {
	uint32_t raw;
	struct {
		uint32_t TTL_IP_EN                       : 1;
		uint32_t TTL_SENSOR_BIT                  : 2;
		uint32_t _rsv_3                          : 1;
		uint32_t TTL_BT_FMT_OUT                  : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t TTL_FMT_IN                      : 4;
		uint32_t TTL_BT_DATA_SEQ                 : 2;
		uint32_t TTL_VS_INV                      : 1;
		uint32_t TTL_HS_INV                      : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_14 {
	uint32_t raw;
	struct {
		uint32_t TTL_VS_BP                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_HS_BP                       : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_18 {
	uint32_t raw;
	struct {
		uint32_t TTL_IMG_WD                      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_IMG_HT                      : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_1C {
	uint32_t raw;
	struct {
		uint32_t TTL_SYNC_0                      : 16;
		uint32_t TTL_SYNC_1                      : 16;
	} bits;
};

union REG_SENSOR_MAC_VI_20 {
	uint32_t raw;
	struct {
		uint32_t TTL_SYNC_2                      : 16;
	} bits;
};

union REG_SENSOR_MAC_VI_24 {
	uint32_t raw;
	struct {
		uint32_t TTL_SAV_VLD                     : 16;
		uint32_t TTL_SAV_BLK                     : 16;
	} bits;
};

union REG_SENSOR_MAC_VI_28 {
	uint32_t raw;
	struct {
		uint32_t TTL_EAV_VLD                     : 16;
		uint32_t TTL_EAV_BLK                     : 16;
	} bits;
};

union REG_SENSOR_MAC_VI_30 {
	uint32_t raw;
	struct {
		uint32_t VI_SEL                          : 3;
		uint32_t VI_FROM                         : 1;
		uint32_t VI_CLK_INV                      : 1;
		uint32_t VI_V_SEL_VS                     : 1;
		uint32_t VI_VS_DBG                       : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t PAD_VI0_CLK_INV                 : 1;
		uint32_t PAD_VI1_CLK_INV                 : 1;
		uint32_t PAD_VI2_CLK_INV                 : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_34 {
	uint32_t raw;
	struct {
		uint32_t VI_VS_DLY                       : 5;
		uint32_t _rsv_5                          : 1;
		uint32_t VI_VS_DLY_EN                    : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t VI_HS_DLY                       : 5;
		uint32_t _rsv_13                         : 1;
		uint32_t VI_HS_DLY_EN                    : 1;
		uint32_t _rsv_15                         : 1;
		uint32_t VI_VDE_DLY                      : 5;
		uint32_t _rsv_21                         : 1;
		uint32_t VI_VDE_DLY_EN                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t VI_HDE_DLY                      : 5;
		uint32_t _rsv_29                         : 1;
		uint32_t VI_HDE_DLY_EN                   : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_40 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_HDR_EN               : 1;
		uint32_t SENSOR_MAC_HDR_VSINV            : 1;
		uint32_t SENSOR_MAC_HDR_HSINV            : 1;
		uint32_t SENSOR_MAC_HDR_DEINV            : 1;
		uint32_t SENSOR_MAC_HDR_HDR0INV          : 1;
		uint32_t SENSOR_MAC_HDR_HDR1INV          : 1;
		uint32_t SENSOR_MAC_HDR_BLCINV           : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t SENSOR_MAC_HDR_MODE             : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_44 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_HDR_SHIFT            : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t SENSOR_MAC_HDR_VSIZE            : 13;
	} bits;
};

union REG_SENSOR_MAC_VI_48 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_INFO_LINE_NUM        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t SENSOR_MAC_RM_INFO_LINE         : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_4C {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_HDR_LINE_CNT         : 14;
	} bits;
};

union REG_SENSOR_MAC_VI_60 {
	uint32_t raw;
	struct {
		uint32_t VI_VS_SEL                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t VI_HS_SEL                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t VI_VDE_SEL                      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t VI_HDE_SEL                      : 6;
	} bits;
};

union REG_SENSOR_MAC_VI_64 {
	uint32_t raw;
	struct {
		uint32_t VI_D0_SEL                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t VI_D1_SEL                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t VI_D2_SEL                       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t VI_D3_SEL                       : 6;
	} bits;
};

union REG_SENSOR_MAC_VI_68 {
	uint32_t raw;
	struct {
		uint32_t VI_D4_SEL                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t VI_D5_SEL                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t VI_D6_SEL                       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t VI_D7_SEL                       : 6;
	} bits;
};

union REG_SENSOR_MAC_VI_6C {
	uint32_t raw;
	struct {
		uint32_t VI_D8_SEL                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t VI_D9_SEL                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t VI_D10_SEL                      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t VI_D11_SEL                      : 6;
	} bits;
};

union REG_SENSOR_MAC_VI_70 {
	uint32_t raw;
	struct {
		uint32_t VI_D12_SEL                      : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t VI_D13_SEL                      : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t VI_D14_SEL                      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t VI_D15_SEL                      : 6;
	} bits;
};

union REG_SENSOR_MAC_VI_74 {
	uint32_t raw;
	struct {
		uint32_t VI_BT_D0_SEL                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t VI_BT_D1_SEL                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t VI_BT_D2_SEL                    : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t VI_BT_D3_SEL                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t VI_BT_D4_SEL                    : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t VI_BT_D5_SEL                    : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t VI_BT_D6_SEL                    : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t VI_BT_D7_SEL                    : 3;
	} bits;
};

union REG_SENSOR_MAC_VI_80 {
	uint32_t raw;
	struct {
		uint32_t BT_CLR_SYNC_LOST_1T             : 1;
		uint32_t BT_IP_EN                        : 1;
		uint32_t BT_DDR_MODE                     : 1;
		uint32_t BT_HS_GATE_BY_VDE               : 1;
		uint32_t BT_VS_INV                       : 1;
		uint32_t BT_HS_INV                       : 1;
		uint32_t BT_VS_AS_VDE                    : 1;
		uint32_t BT_HS_AS_HDE                    : 1;
		uint32_t BT_SW_EN_CLK                    : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t BT_DEMUX_CH                     : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t BT_FMT_SEL                      : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t BT_SYNC_LOST                    : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_84 {
	uint32_t raw;
	struct {
		uint32_t BT_V_CTRL_DLY                   : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t BT_H_CTRL_DLY                   : 5;
	} bits;
};

union REG_SENSOR_MAC_VI_88 {
	uint32_t raw;
	struct {
		uint32_t BT_IMG_WD_M1                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BT_IMG_HT_M1                    : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_8C {
	uint32_t raw;
	struct {
		uint32_t BT_VS_BP_M1                     : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BT_HS_BP_M1                     : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_90 {
	uint32_t raw;
	struct {
		uint32_t BT_VS_FP_M1                     : 8;
		uint32_t BT_HS_FP_M1                     : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_94 {
	uint32_t raw;
	struct {
		uint32_t BT_SYNC_0                       : 8;
		uint32_t BT_SYNC_1                       : 8;
		uint32_t BT_SYNC_2                       : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_98 {
	uint32_t raw;
	struct {
		uint32_t BT_SAV_VLD_0                    : 8;
		uint32_t BT_SAV_BLK_0                    : 8;
		uint32_t BT_EAV_VLD_0                    : 8;
		uint32_t BT_EAV_BLK_0                    : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_9C {
	uint32_t raw;
	struct {
		uint32_t BT_SAV_VLD_1                    : 8;
		uint32_t BT_SAV_BLK_1                    : 8;
		uint32_t BT_EAV_VLD_1                    : 8;
		uint32_t BT_EAV_BLK_1                    : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_A0 {
	uint32_t raw;
	struct {
		uint32_t BT_SAV_VLD_2                    : 8;
		uint32_t BT_SAV_BLK_2                    : 8;
		uint32_t BT_EAV_VLD_2                    : 8;
		uint32_t BT_EAV_BLK_2                    : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_A4 {
	uint32_t raw;
	struct {
		uint32_t BT_SAV_VLD_3                    : 8;
		uint32_t BT_SAV_BLK_3                    : 8;
		uint32_t BT_EAV_VLD_3                    : 8;
		uint32_t BT_EAV_BLK_3                    : 8;
	} bits;
};

union REG_SENSOR_MAC_VI_A8 {
	uint32_t raw;
	struct {
		uint32_t BT_YC_INV                       : 4;
	} bits;
};

union REG_SENSOR_MAC_VI_B0 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_CROP_START_X         : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t SENSOR_MAC_CROP_END_X           : 13;
		uint32_t _rsv_29                         : 2;
		uint32_t SENSOR_MAC_CROP_EN              : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_B4 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_CROP_START_Y         : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t SENSOR_MAC_CROP_END_Y           : 13;
	} bits;
};

union REG_SENSOR_MAC_VI_B8 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_SWAPUV_EN            : 1;
		uint32_t SENSOR_MAC_SWAPYC_EN            : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_BC {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_DBG_HTOTAL_MAX       : 16;
		uint32_t SENSOR_MAC_DBG_EN               : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_C0 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_DBG_VTOTAL_MAX       : 32;
	} bits;
};

union REG_SENSOR_MAC_VI_C4 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_DBG_HTOTAL           : 16;
	} bits;
};

union REG_SENSOR_MAC_VI_C8 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MAC_DBG_VTOTAL           : 32;
	} bits;
};

union REG_SENSOR_MAC_VI_D0 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_ENABLE             : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t TTL_AS_SLVDS_BIT_MODE           : 2;
		uint32_t TTL_AS_SLVDS_DATA_REVERSE       : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t TTL_AS_SLVDS_HDR_MODE           : 1;
		uint32_t TTL_AS_SLVDS_HDR_PATTERN        : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t TTL_AS_SLVDS_VFPORCH            : 10;
	} bits;
};

union REG_SENSOR_MAC_VI_D4 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_SYNC_1ST           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_SYNC_2ND           : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_D8 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_SYNC_3RD           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_NORM_BK_SAV        : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_DC {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_NORM_BK_EAV        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_NORM_SAV           : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_E0 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_NORM_EAV           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N0_BK_SAV          : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_E4 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N0_BK_EAV          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N1_BK_SAV          : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_E8 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N1_BK_EAV          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N0_LEF_SAV         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_EC {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N0_LEF_EAV         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N0_SEF_SAV         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_F0 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N0_SEF_EAV         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N1_LEF_SAV         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_F4 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N1_LEF_EAV         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N1_SEF_SAV         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_F8 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N1_SEF_EAV         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_FC {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_VS_GEN_SYNC_CODE   : 12;
		uint32_t TTL_AS_SLVDS_VS_GEN_BY_SYNC_CODE: 1;
	} bits;
};

union REG_SENSOR_MAC_VI_100 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N0_LSEF_SAV        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N0_LSEF_EAV        : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_104 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_N1_LSEF_SAV        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_SLVDS_N1_LSEF_EAV        : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_108 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_SLVDS_HDR_P2_HSIZE       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t TTL_AS_SLVDS_HDR_P2_HBLANK      : 14;
	} bits;
};

union REG_SENSOR_MAC_VI_110 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_HISPI_MODE               : 1;
		uint32_t TTL_AS_HISPI_USE_HSIZE          : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t TTL_AS_HISPI_HDR_PSP_MODE       : 1;
	} bits;
};

union REG_SENSOR_MAC_VI_114 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_HISPI_NORM_SOF           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_HISPI_NORM_EOF           : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_118 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_HISPI_HDR_T1_SOF         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_HISPI_HDR_T1_EOF         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_11C {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_HISPI_HDR_T1_SOL         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_HISPI_HDR_T1_EOL         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_120 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_HISPI_HDR_T2_SOF         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_HISPI_HDR_T2_EOF         : 12;
	} bits;
};

union REG_SENSOR_MAC_VI_124 {
	uint32_t raw;
	struct {
		uint32_t TTL_AS_HISPI_HDR_T2_SOL         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t TTL_AS_HISPI_HDR_T2_EOL         : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_SENSOR_MAC_VI_T {
	union REG_SENSOR_MAC_VI_00              REG_00;
	uint32_t                                _resv_0x4[3];
	union REG_SENSOR_MAC_VI_10              REG_10;
	union REG_SENSOR_MAC_VI_14              REG_14;
	union REG_SENSOR_MAC_VI_18              REG_18;
	union REG_SENSOR_MAC_VI_1C              REG_1C;
	union REG_SENSOR_MAC_VI_20              REG_20;
	union REG_SENSOR_MAC_VI_24              REG_24;
	union REG_SENSOR_MAC_VI_28              REG_28;
	uint32_t                                _resv_0x2c[1];
	union REG_SENSOR_MAC_VI_30              REG_30;
	union REG_SENSOR_MAC_VI_34              REG_34;
	uint32_t                                _resv_0x38[2];
	union REG_SENSOR_MAC_VI_40              REG_40;
	union REG_SENSOR_MAC_VI_44              REG_44;
	union REG_SENSOR_MAC_VI_48              REG_48;
	union REG_SENSOR_MAC_VI_4C              REG_4C;
	uint32_t                                _resv_0x50[4];
	union REG_SENSOR_MAC_VI_60              REG_60;
	union REG_SENSOR_MAC_VI_64              REG_64;
	union REG_SENSOR_MAC_VI_68              REG_68;
	union REG_SENSOR_MAC_VI_6C              REG_6C;
	union REG_SENSOR_MAC_VI_70              REG_70;
	union REG_SENSOR_MAC_VI_74              REG_74;
	uint32_t                                _resv_0x78[2];
	union REG_SENSOR_MAC_VI_80              REG_80;
	union REG_SENSOR_MAC_VI_84              REG_84;
	union REG_SENSOR_MAC_VI_88              REG_88;
	union REG_SENSOR_MAC_VI_8C              REG_8C;
	union REG_SENSOR_MAC_VI_90              REG_90;
	union REG_SENSOR_MAC_VI_94              REG_94;
	union REG_SENSOR_MAC_VI_98              REG_98;
	union REG_SENSOR_MAC_VI_9C              REG_9C;
	union REG_SENSOR_MAC_VI_A0              REG_A0;
	union REG_SENSOR_MAC_VI_A4              REG_A4;
	union REG_SENSOR_MAC_VI_A8              REG_A8;
	uint32_t                                _resv_0xac[1];
	union REG_SENSOR_MAC_VI_B0              REG_B0;
	union REG_SENSOR_MAC_VI_B4              REG_B4;
	union REG_SENSOR_MAC_VI_B8              REG_B8;
	union REG_SENSOR_MAC_VI_BC              REG_BC;
	union REG_SENSOR_MAC_VI_C0              REG_C0;
	union REG_SENSOR_MAC_VI_C4              REG_C4;
	union REG_SENSOR_MAC_VI_C8              REG_C8;
	uint32_t                                _resv_0xcc[1];
	union REG_SENSOR_MAC_VI_D0              REG_D0;
	union REG_SENSOR_MAC_VI_D4              REG_D4;
	union REG_SENSOR_MAC_VI_D8              REG_D8;
	union REG_SENSOR_MAC_VI_DC              REG_DC;
	union REG_SENSOR_MAC_VI_E0              REG_E0;
	union REG_SENSOR_MAC_VI_E4              REG_E4;
	union REG_SENSOR_MAC_VI_E8              REG_E8;
	union REG_SENSOR_MAC_VI_EC              REG_EC;
	union REG_SENSOR_MAC_VI_F0              REG_F0;
	union REG_SENSOR_MAC_VI_F4              REG_F4;
	union REG_SENSOR_MAC_VI_F8              REG_F8;
	union REG_SENSOR_MAC_VI_FC              REG_FC;
	union REG_SENSOR_MAC_VI_100             REG_100;
	union REG_SENSOR_MAC_VI_104             REG_104;
	union REG_SENSOR_MAC_VI_108             REG_108;
	uint32_t                                _resv_0x10c[1];
	union REG_SENSOR_MAC_VI_110             REG_110;
	union REG_SENSOR_MAC_VI_114             REG_114;
	union REG_SENSOR_MAC_VI_118             REG_118;
	union REG_SENSOR_MAC_VI_11C             REG_11C;
	union REG_SENSOR_MAC_VI_120             REG_120;
	union REG_SENSOR_MAC_VI_124             REG_124;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_MAC_VI_H_ */

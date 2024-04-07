/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_fields_csi_wrap.h
 * Description:HW register description
 */

#ifndef _REG_FIELDS_CSI_WRAP_H_
#define _REG_FIELDS_CSI_WRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_SENSOR_PHY_TOP_00 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_PHY_MODE                 : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_04 {
	uint32_t raw;
	struct {
		uint32_t EN_CLKRX_SOURCE                 : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_08 {
	uint32_t raw;
	struct {
		uint32_t EN_RXBUS_CLK                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_10 {
	uint32_t raw;
	struct {
		uint32_t PD_MIPI_LANE                    : 18;
		uint32_t _rsv_18                         : 2;
		uint32_t PD_PLL                          : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t PD_REF_LANE                     : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_14 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_LPCD                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_18 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_LPTX                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_1C {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_LPRX                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_20 {
	uint32_t raw;
	struct {
		uint32_t EN_DEMUX                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_24 {
	uint32_t raw;
	struct {
		uint32_t EN_PREAMP                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_28 {
	uint32_t raw;
	struct {
		uint32_t EN_VCM_DET                      : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_2C {
	uint32_t raw;
	struct {
		uint32_t EN_HVCMI                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_30 {
	uint32_t raw;
	struct {
		uint32_t SEL_MIPI_IQ                     : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_34 {
	uint32_t raw;
	struct {
		uint32_t EN_LVDS                         : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_38 {
	uint32_t raw;
	struct {
		uint32_t EN_LVDS_LDO                     : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_3C {
	uint32_t raw;
	struct {
		uint32_t EN_SUBLVDS                      : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_40 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_DRV                     : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_44 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_LDO                     : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_48 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_DATA_SER                : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_4C {
	uint32_t raw;
	struct {
		uint32_t EN_CLKBUSL                      : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t EN_CLKBUSR                      : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t EN_CLKBUSL_TO_EXTL              : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t EN_CLKBUSR_TO_EXTR              : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t EN_EXTL_TO_CLKBUSL              : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t EN_EXTR_TO_CLKBUSR              : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_50 {
	uint32_t raw;
	struct {
		uint32_t EN_TST                          : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t TST_BYPASS_VREF                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t TST_MIPI_CLKIQ_INV              : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t TST_VCMI_DET                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_54 {
	uint32_t raw;
	struct {
		uint32_t EN_LCKDET                       : 3;
		uint32_t _rsv_3                          : 13;
		uint32_t MIPI_TXPLL_LOCK                 : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_58 {
	uint32_t raw;
	struct {
		uint32_t DIV_OUT_SEL_0                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t DIV_SEL_0                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t DISP_DIVSEL_0                   : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t SEL_MIPI_TXPLL_ICTRL_0          : 4;
		uint32_t LOOP_DIV_0                      : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_5C {
	uint32_t raw;
	struct {
		uint32_t DIV_OUT_SEL_1                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t DIV_SEL_1                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t DISP_DIVSEL_1                   : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t SEL_MIPI_TXPLL_ICTRL_1          : 4;
		uint32_t LOOP_DIV_1                      : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_60 {
	uint32_t raw;
	struct {
		uint32_t DIV_OUT_SEL_2                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t DIV_SEL_2                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t DISP_DIVSEL_2                   : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t SEL_MIPI_TXPLL_ICTRL_2          : 4;
		uint32_t LOOP_DIV_2                      : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_64 {
	uint32_t raw;
	struct {
		uint32_t CSEL_PREAMP0                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t CSEL_PREAMP1                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t CSEL_PREAMP2                    : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t CSEL_PREAMP3                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t CSEL_PREAMP4                    : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t CSEL_PREAMP5                    : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t CSEL_PREAMP6                    : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t CSEL_PREAMP7                    : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_68 {
	uint32_t raw;
	struct {
		uint32_t CSEL_PREAMP8                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t CSEL_PREAMP9                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t CSEL_PREAMP10                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t CSEL_PREAMP11                   : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t CSEL_PREAMP12                   : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t CSEL_PREAMP13                   : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t CSEL_PREAMP14                   : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t CSEL_PREAMP15                   : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_6C {
	uint32_t raw;
	struct {
		uint32_t CSEL_PREAMP16                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t CSEL_PREAMP17                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t SWAP_RXDATA_INV                 : 18;
		uint32_t _rsv_26                         : 2;
		uint32_t EN_PREAMP_HSPEED0               : 2;
		uint32_t EN_PREAMP_HSPEED1               : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_70 {
	uint32_t raw;
	struct {
		uint32_t EN_PREAMP_HSPEED2               : 2;
		uint32_t EN_PREAMP_HSPEED3               : 2;
		uint32_t EN_PREAMP_HSPEED4               : 2;
		uint32_t EN_PREAMP_HSPEED5               : 2;
		uint32_t EN_PREAMP_HSPEED6               : 2;
		uint32_t EN_PREAMP_HSPEED7               : 2;
		uint32_t EN_PREAMP_HSPEED8               : 2;
		uint32_t EN_PREAMP_HSPEED9               : 2;
		uint32_t EN_PREAMP_HSPEED10              : 2;
		uint32_t EN_PREAMP_HSPEED11              : 2;
		uint32_t EN_PREAMP_HSPEED12              : 2;
		uint32_t EN_PREAMP_HSPEED13              : 2;
		uint32_t EN_PREAMP_HSPEED14              : 2;
		uint32_t EN_PREAMP_HSPEED15              : 2;
		uint32_t EN_PREAMP_HSPEED16              : 2;
		uint32_t EN_PREAMP_HSPEED17              : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_74 {
	uint32_t raw;
	struct {
		uint32_t RSEL_PREAMP0                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t RSEL_PREAMP1                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t RSEL_PREAMP2                    : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t RSEL_PREAMP3                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t RSEL_PREAMP4                    : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t RSEL_PREAMP5                    : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t RSEL_PREAMP6                    : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t RSEL_PREAMP7                    : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_78 {
	uint32_t raw;
	struct {
		uint32_t RSEL_PREAMP8                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t RSEL_PREAMP9                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t RSEL_PREAMP10                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t RSEL_PREAMP11                   : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t RSEL_PREAMP12                   : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t RSEL_PREAMP13                   : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t RSEL_PREAMP14                   : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t RSEL_PREAMP15                   : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_7C {
	uint32_t raw;
	struct {
		uint32_t RSEL_PREAMP16                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t RSEL_PREAMP17                   : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_80 {
	uint32_t raw;
	struct {
		uint32_t SEL_RXCLK_SKEW0                 : 4;
		uint32_t SEL_RXCLK_SKEW1                 : 4;
		uint32_t SEL_RXCLK_SKEW2                 : 4;
		uint32_t SEL_RXCLK_SKEW3                 : 4;
		uint32_t SEL_RXCLK_SKEW4                 : 4;
		uint32_t SEL_RXCLK_SKEW5                 : 4;
		uint32_t SEL_RXCLK_SKEW6                 : 4;
		uint32_t SEL_RXCLK_SKEW7                 : 4;
	} bits;
};

union REG_SENSOR_PHY_TOP_84 {
	uint32_t raw;
	struct {
		uint32_t SEL_RXCLK_SKEW8                 : 4;
		uint32_t SEL_RXCLK_SKEW9                 : 4;
		uint32_t SEL_RXCLK_SKEW10                : 4;
		uint32_t SEL_RXCLK_SKEW11                : 4;
		uint32_t SEL_RXCLK_SKEW12                : 4;
		uint32_t SEL_RXCLK_SKEW13                : 4;
		uint32_t SEL_RXCLK_SKEW14                : 4;
		uint32_t SEL_RXCLK_SKEW15                : 4;
	} bits;
};

union REG_SENSOR_PHY_TOP_88 {
	uint32_t raw;
	struct {
		uint32_t SEL_RXCLK_SKEW16                : 4;
		uint32_t SEL_RXCLK_SKEW17                : 4;
	} bits;
};

union REG_SENSOR_PHY_TOP_90 {
	uint32_t raw;
	struct {
		uint32_t GPO_DS0_P                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_94 {
	uint32_t raw;
	struct {
		uint32_t GPO_DS1_P                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_98 {
	uint32_t raw;
	struct {
		uint32_t GPO_DS0_N                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_9C {
	uint32_t raw;
	struct {
		uint32_t GPO_DS1_N                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_A0 {
	uint32_t raw;
	struct {
		uint32_t GPI_RPU4P7_P                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_A4 {
	uint32_t raw;
	struct {
		uint32_t GPI_RPU4P7_N                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_A8 {
	uint32_t raw;
	struct {
		uint32_t GPI_ST_P                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_AC {
	uint32_t raw;
	struct {
		uint32_t GPI_ST_N                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_B0 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_TRIM0                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t EN_MIPI_TRIM1                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t EN_MIPI_TRIM2                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t EN_MIPI_TRIM3                   : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t EN_MIPI_TRIM4                   : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t EN_MIPI_TRIM5                   : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t EN_MIPI_TRIM6                   : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t EN_MIPI_TRIM7                   : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_B4 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_TRIM8                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t EN_MIPI_TRIM9                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t EN_MIPI_TRIM10                  : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t EN_MIPI_TRIM11                  : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t EN_MIPI_TRIM12                  : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t EN_MIPI_TRIM13                  : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t EN_MIPI_TRIM14                  : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t EN_MIPI_TRIM15                  : 3;
	} bits;
};

union REG_SENSOR_PHY_TOP_B8 {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_TRIM16                  : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t EN_MIPI_TRIM17                  : 3;
		uint32_t _rsv_7                          : 21;
		uint32_t EN_MIPI_DE_DRV0                 : 2;
		uint32_t EN_MIPI_DE_DRV1                 : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_BC {
	uint32_t raw;
	struct {
		uint32_t EN_MIPI_DE_DRV2                 : 2;
		uint32_t EN_MIPI_DE_DRV3                 : 2;
		uint32_t EN_MIPI_DE_DRV4                 : 2;
		uint32_t EN_MIPI_DE_DRV5                 : 2;
		uint32_t EN_MIPI_DE_DRV6                 : 2;
		uint32_t EN_MIPI_DE_DRV7                 : 2;
		uint32_t EN_MIPI_DE_DRV8                 : 2;
		uint32_t EN_MIPI_DE_DRV9                 : 2;
		uint32_t EN_MIPI_DE_DRV10                : 2;
		uint32_t EN_MIPI_DE_DRV11                : 2;
		uint32_t EN_MIPI_DE_DRV12                : 2;
		uint32_t EN_MIPI_DE_DRV13                : 2;
		uint32_t EN_MIPI_DE_DRV14                : 2;
		uint32_t EN_MIPI_DE_DRV15                : 2;
		uint32_t EN_MIPI_DE_DRV16                : 2;
		uint32_t EN_MIPI_DE_DRV17                : 2;
	} bits;
};

union REG_SENSOR_PHY_TOP_C0 {
	uint32_t raw;
	struct {
		uint32_t SEL_VREF_LDO_0                  : 2;
		uint32_t SEL_VREF_LDO_1                  : 2;
		uint32_t SEL_VREF_LDO_2                  : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t EN_MIPI_ULPS                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_C4 {
	uint32_t raw;
	struct {
		uint32_t AD_CLK_INV                      : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_D0 {
	uint32_t raw;
	struct {
		uint32_t AD_D0_DATA                      : 8;
		uint32_t AD_D1_DATA                      : 8;
		uint32_t AD_D2_DATA                      : 8;
		uint32_t AD_D3_DATA                      : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_D4 {
	uint32_t raw;
	struct {
		uint32_t AD_D4_DATA                      : 8;
		uint32_t AD_D5_DATA                      : 8;
		uint32_t AD_D6_DATA                      : 8;
		uint32_t AD_D7_DATA                      : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_D8 {
	uint32_t raw;
	struct {
		uint32_t AD_D8_DATA                      : 8;
		uint32_t AD_D9_DATA                      : 8;
		uint32_t AD_D10_DATA                     : 8;
		uint32_t AD_D11_DATA                     : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_DC {
	uint32_t raw;
	struct {
		uint32_t AD_D12_DATA                     : 8;
		uint32_t AD_D13_DATA                     : 8;
		uint32_t AD_D14_DATA                     : 8;
		uint32_t AD_D15_DATA                     : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_E0 {
	uint32_t raw;
	struct {
		uint32_t AD_D16_DATA                     : 8;
		uint32_t AD_D17_DATA                     : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_E4 {
	uint32_t raw;
	struct {
		uint32_t AD_LPOUTN                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_E8 {
	uint32_t raw;
	struct {
		uint32_t AD_LPOUTP                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_EC {
	uint32_t raw;
	struct {
		uint32_t FORCE_DESKEW_CODE0              : 1;
		uint32_t FORCE_DESKEW_CODE1              : 1;
		uint32_t FORCE_DESKEW_CODE2              : 1;
		uint32_t FORCE_DESKEW_CODE3              : 1;
		uint32_t FORCE_DESKEW_CODE4              : 1;
		uint32_t FORCE_DESKEW_CODE5              : 1;
		uint32_t FORCE_DESKEW_CODE6              : 1;
		uint32_t FORCE_DESKEW_CODE7              : 1;
		uint32_t FORCE_DESKEW_CODE8              : 1;
		uint32_t FORCE_DESKEW_CODE9              : 1;
		uint32_t FORCE_DESKEW_CODE10             : 1;
		uint32_t FORCE_DESKEW_CODE11             : 1;
		uint32_t FORCE_DESKEW_CODE12             : 1;
		uint32_t FORCE_DESKEW_CODE13             : 1;
		uint32_t FORCE_DESKEW_CODE14             : 1;
		uint32_t FORCE_DESKEW_CODE15             : 1;
		uint32_t FORCE_DESKEW_CODE16             : 1;
		uint32_t FORCE_DESKEW_CODE17             : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_F0 {
	uint32_t raw;
	struct {
		uint32_t DESKEW_CODE0                    : 8;
		uint32_t DESKEW_CODE1                    : 8;
		uint32_t DESKEW_CODE2                    : 8;
		uint32_t DESKEW_CODE3                    : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_F4 {
	uint32_t raw;
	struct {
		uint32_t DESKEW_CODE4                    : 8;
		uint32_t DESKEW_CODE5                    : 8;
		uint32_t DESKEW_CODE6                    : 8;
		uint32_t DESKEW_CODE7                    : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_F8 {
	uint32_t raw;
	struct {
		uint32_t DESKEW_CODE8                    : 8;
		uint32_t DESKEW_CODE9                    : 8;
		uint32_t DESKEW_CODE10                   : 8;
		uint32_t DESKEW_CODE11                   : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_FC {
	uint32_t raw;
	struct {
		uint32_t DESKEW_CODE12                   : 8;
		uint32_t DESKEW_CODE13                   : 8;
		uint32_t DESKEW_CODE14                   : 8;
		uint32_t DESKEW_CODE15                   : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_100 {
	uint32_t raw;
	struct {
		uint32_t DESKEW_CODE16                   : 8;
		uint32_t DESKEW_CODE17                   : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_104 {
	uint32_t raw;
	struct {
		uint32_t FORCE_RTERM                     : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_108 {
	uint32_t raw;
	struct {
		uint32_t EN_RTERM                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_10C {
	uint32_t raw;
	struct {
		uint32_t FORCE_RTERM_CAP                 : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_110 {
	uint32_t raw;
	struct {
		uint32_t EN_RTERM_CAP                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_120 {
	uint32_t raw;
	struct {
		uint32_t CAM0_VTT                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM0_VS_STR                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_124 {
	uint32_t raw;
	struct {
		uint32_t CAM0_VS_STP                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM0_HTT                        : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_128 {
	uint32_t raw;
	struct {
		uint32_t CAM0_HS_STR                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM0_HS_STP                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_12C {
	uint32_t raw;
	struct {
		uint32_t CAM0_VS_POL                     : 1;
		uint32_t CAM0_HS_POL                     : 1;
		uint32_t CAM0_TGEN_EN                    : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_130 {
	uint32_t raw;
	struct {
		uint32_t CAM1_VTT                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM1_VS_STR                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_134 {
	uint32_t raw;
	struct {
		uint32_t CAM1_VS_STP                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM1_HTT                        : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_138 {
	uint32_t raw;
	struct {
		uint32_t CAM1_HS_STR                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM1_HS_STP                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_13C {
	uint32_t raw;
	struct {
		uint32_t CAM1_VS_POL                     : 1;
		uint32_t CAM1_HS_POL                     : 1;
		uint32_t CAM1_TGEN_EN                    : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_140 {
	uint32_t raw;
	struct {
		uint32_t CAM2_VTT                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM2_VS_STR                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_144 {
	uint32_t raw;
	struct {
		uint32_t CAM2_VS_STP                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM2_HTT                        : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_148 {
	uint32_t raw;
	struct {
		uint32_t CAM2_HS_STR                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM2_HS_STP                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_14C {
	uint32_t raw;
	struct {
		uint32_t CAM2_VS_POL                     : 1;
		uint32_t CAM2_HS_POL                     : 1;
		uint32_t CAM2_TGEN_EN                    : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_150 {
	uint32_t raw;
	struct {
		uint32_t CAM3_VTT                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM3_VS_STR                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_154 {
	uint32_t raw;
	struct {
		uint32_t CAM3_VS_STP                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM3_HTT                        : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_158 {
	uint32_t raw;
	struct {
		uint32_t CAM3_HS_STR                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM3_HS_STP                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_15C {
	uint32_t raw;
	struct {
		uint32_t CAM3_VS_POL                     : 1;
		uint32_t CAM3_HS_POL                     : 1;
		uint32_t CAM3_TGEN_EN                    : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_160 {
	uint32_t raw;
	struct {
		uint32_t CAM4_VTT                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM4_VS_STR                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_164 {
	uint32_t raw;
	struct {
		uint32_t CAM4_VS_STP                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM4_HTT                        : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_168 {
	uint32_t raw;
	struct {
		uint32_t CAM4_HS_STR                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM4_HS_STP                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_16C {
	uint32_t raw;
	struct {
		uint32_t CAM4_VS_POL                     : 1;
		uint32_t CAM4_HS_POL                     : 1;
		uint32_t CAM4_TGEN_EN                    : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_170 {
	uint32_t raw;
	struct {
		uint32_t CAM5_VTT                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM5_VS_STR                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_174 {
	uint32_t raw;
	struct {
		uint32_t CAM5_VS_STP                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM5_HTT                        : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_178 {
	uint32_t raw;
	struct {
		uint32_t CAM5_HS_STR                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CAM5_HS_STP                     : 14;
	} bits;
};

union REG_SENSOR_PHY_TOP_17C {
	uint32_t raw;
	struct {
		uint32_t CAM5_VS_POL                     : 1;
		uint32_t CAM5_HS_POL                     : 1;
		uint32_t CAM5_TGEN_EN                    : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_DFT_180 {
	uint32_t raw;
	struct {
		uint32_t RO_DESKEW_CODE0                 : 8;
		uint32_t RO_DESKEW_CODE1                 : 8;
		uint32_t RO_DESKEW_CODE2                 : 8;
		uint32_t RO_DESKEW_CODE3                 : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_DFT_184 {
	uint32_t raw;
	struct {
		uint32_t RO_DESKEW_CODE4                 : 8;
		uint32_t RO_DESKEW_CODE5                 : 8;
		uint32_t RO_DESKEW_CODE6                 : 8;
		uint32_t RO_DESKEW_CODE7                 : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_DFT_188 {
	uint32_t raw;
	struct {
		uint32_t RO_DESKEW_CODE8                 : 8;
		uint32_t RO_DESKEW_CODE9                 : 8;
		uint32_t RO_DESKEW_CODE10                : 8;
		uint32_t RO_DESKEW_CODE11                : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_DFT_18C {
	uint32_t raw;
	struct {
		uint32_t RO_DESKEW_CODE12                : 8;
		uint32_t RO_DESKEW_CODE13                : 8;
		uint32_t RO_DESKEW_CODE14                : 8;
		uint32_t RO_DESKEW_CODE15                : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_DFT_190 {
	uint32_t raw;
	struct {
		uint32_t RO_DESKEW_CODE16                : 8;
		uint32_t RO_DESKEW_CODE17                : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_DBG_1F0 {
	uint32_t raw;
	struct {
		uint32_t DBG_SEL                         : 16;
		uint32_t DBG_CK_SEL                      : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_200 {
	uint32_t raw;
	struct {
		uint32_t FORCE_GPIO_P                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_204 {
	uint32_t raw;
	struct {
		uint32_t FORCE_GPIO_N                    : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_208 {
	uint32_t raw;
	struct {
		uint32_t GPI_IE_P                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_20C {
	uint32_t raw;
	struct {
		uint32_t GPI_IE_N                        : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_210 {
	uint32_t raw;
	struct {
		uint32_t GPO_OEN_P                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_214 {
	uint32_t raw;
	struct {
		uint32_t GPO_OEN_N                       : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_218 {
	uint32_t raw;
	struct {
		uint32_t GPO_I_P                         : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_21C {
	uint32_t raw;
	struct {
		uint32_t GPO_I_N                         : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_220 {
	uint32_t raw;
	struct {
		uint32_t SW_UP                           : 1;
		uint32_t EN_SSC                          : 1;
		uint32_t SSC_MODE                        : 2;
		uint32_t SYN_MODE                        : 1;
	} bits;
};

union REG_SENSOR_PHY_TOP_224 {
	uint32_t raw;
	struct {
		uint32_t SET                             : 32;
	} bits;
};

union REG_SENSOR_PHY_TOP_228 {
	uint32_t raw;
	struct {
		uint32_t SPAN                            : 16;
	} bits;
};

union REG_SENSOR_PHY_TOP_22C {
	uint32_t raw;
	struct {
		uint32_t STEP                            : 24;
	} bits;
};

union REG_SENSOR_PHY_TOP_230 {
	uint32_t raw;
	struct {
		uint32_t MIPI_TX_PRBS9_EN                : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_234 {
	uint32_t raw;
	struct {
		uint32_t FORCE_TX_DATA                   : 18;
	} bits;
};

union REG_SENSOR_PHY_TOP_238 {
	uint32_t raw;
	struct {
		uint32_t TX_DATA0                        : 8;
		uint32_t TX_DATA1                        : 8;
		uint32_t TX_DATA2                        : 8;
		uint32_t TX_DATA3                        : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_23C {
	uint32_t raw;
	struct {
		uint32_t TX_DATA4                        : 8;
		uint32_t TX_DATA5                        : 8;
		uint32_t TX_DATA6                        : 8;
		uint32_t TX_DATA7                        : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_240 {
	uint32_t raw;
	struct {
		uint32_t TX_DATA8                        : 8;
		uint32_t TX_DATA9                        : 8;
		uint32_t TX_DATA10                       : 8;
		uint32_t TX_DATA11                       : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_244 {
	uint32_t raw;
	struct {
		uint32_t TX_DATA12                       : 8;
		uint32_t TX_DATA13                       : 8;
		uint32_t TX_DATA14                       : 8;
		uint32_t TX_DATA15                       : 8;
	} bits;
};

union REG_SENSOR_PHY_TOP_248 {
	uint32_t raw;
	struct {
		uint32_t TX_DATA16                       : 8;
		uint32_t TX_DATA17                       : 8;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_SENSOR_PHY_8L_00 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MODE                     : 2;
	} bits;
};

union REG_SENSOR_PHY_8L_04 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_D0_SEL                 : 4;
		uint32_t CSI_LANE_D1_SEL                 : 4;
		uint32_t CSI_LANE_D2_SEL                 : 4;
		uint32_t CSI_LANE_D3_SEL                 : 4;
		uint32_t CSI_LANE_D4_SEL                 : 4;
		uint32_t CSI_LANE_D5_SEL                 : 4;
		uint32_t CSI_LANE_D6_SEL                 : 4;
		uint32_t CSI_LANE_D7_SEL                 : 4;
	} bits;
};

union REG_SENSOR_PHY_8L_08 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_CK_SEL                 : 4;
		uint32_t CSI_LANE_CK_PNSWAP              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t CSI_LANE_D0_PNSWAP              : 1;
		uint32_t CSI_LANE_D1_PNSWAP              : 1;
		uint32_t CSI_LANE_D2_PNSWAP              : 1;
		uint32_t CSI_LANE_D3_PNSWAP              : 1;
		uint32_t CSI_LANE_D4_PNSWAP              : 1;
		uint32_t CSI_LANE_D5_PNSWAP              : 1;
		uint32_t CSI_LANE_D6_PNSWAP              : 1;
		uint32_t CSI_LANE_D7_PNSWAP              : 1;
		uint32_t CSI_CK_PHASE                    : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_0C {
	uint32_t raw;
	struct {
		uint32_t DESKEW_LANE_EN                  : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t PRBS9_TEST_PERIOD               : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_10 {
	uint32_t raw;
	struct {
		uint32_t T_HS_SETTLE                     : 8;
		uint32_t T_ALL_ZERO                      : 8;
		uint32_t AUTO_IGNORE                     : 1;
		uint32_t AUTO_SYNC                       : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_14 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_D0_SEL_H               : 1;
		uint32_t CSI_LANE_D1_SEL_H               : 1;
		uint32_t CSI_LANE_D2_SEL_H               : 1;
		uint32_t CSI_LANE_D3_SEL_H               : 1;
		uint32_t CSI_LANE_D4_SEL_H               : 1;
		uint32_t CSI_LANE_D5_SEL_H               : 1;
		uint32_t CSI_LANE_D6_SEL_H               : 1;
		uint32_t CSI_LANE_D7_SEL_H               : 1;
		uint32_t CSI_LANE_CK_SEL_H               : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_20 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_INV_EN                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t SLVDS_BIT_MODE                  : 2;
		uint32_t SLVDS_LANE_EN                   : 8;
		uint32_t SLVDS_FORCE_RESYNC              : 1;
		uint32_t SLVDS_RESYNC                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t SLVDS_SAV_1ST                   : 12;
	} bits;
};

union REG_SENSOR_PHY_8L_24 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_SAV_2ND                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SLVDS_SAV_3RD                   : 12;
	} bits;
};

union REG_SENSOR_PHY_8L_28 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_D0_SYNC_STATE             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t SLVDS_D1_SYNC_STATE             : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t SLVDS_D2_SYNC_STATE             : 2;
		uint32_t _rsv_10                         : 2;
		uint32_t SLVDS_D3_SYNC_STATE             : 2;
		uint32_t _rsv_14                         : 2;
		uint32_t SLVDS_D4_SYNC_STATE             : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t SLVDS_D5_SYNC_STATE             : 2;
		uint32_t _rsv_22                         : 2;
		uint32_t SLVDS_D6_SYNC_STATE             : 2;
		uint32_t _rsv_26                         : 2;
		uint32_t SLVDS_D7_SYNC_STATE             : 2;
	} bits;
};

union REG_SENSOR_PHY_8L_30 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_LANE_EN                  : 8;
		uint32_t SLVSEC_SKEW_CNT_EN              : 1;
		uint32_t SLVSEC_TRAIN_SEQ_CHK_EN         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t SLVSEC_SKEW_CONS                : 5;
		uint32_t SLVSEC_FORCE_RESYNC             : 1;
		uint32_t SLVSEC_RESYNC                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t SLVSEC_UNSTABLE_SKEW_CNT        : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_34 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_SYNC_SYMBOL              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t SLVSEC_STANDBY_SYMBOL           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t SLVSEC_DESKEW_SYMBOL            : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_38 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PRBS9_TEST_PERIOD        : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_3C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_CLR             : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_40 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_MASK            : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_44 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_STATUS          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_48 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D0_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D0_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D0_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_4C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D0_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D0_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D0_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D0_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D0_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D0_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D0_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D0_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D0_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_50 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D1_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D1_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D1_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_54 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D1_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D1_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D1_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D1_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D1_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D1_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D1_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D1_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D1_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_58 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D2_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D2_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D2_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_5C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D2_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D2_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D2_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D2_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D2_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D2_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D2_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D2_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D2_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_60 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D3_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D3_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D3_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_64 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D3_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D3_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D3_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D3_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D3_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D3_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D3_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D3_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D3_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_68 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D4_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D4_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D4_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_6C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D4_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D4_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D4_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D4_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D4_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D4_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D4_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D4_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D4_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_70 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D5_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D5_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D5_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_74 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D5_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D5_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D5_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D5_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D5_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D5_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D5_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D5_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D5_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_78 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D6_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D6_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D6_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_7C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D6_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D6_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D6_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D6_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D6_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D6_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D6_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D6_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D6_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_80 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D7_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D7_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D7_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_84 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D7_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D7_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D7_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D7_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D7_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D7_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D7_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D7_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D7_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_8L_DBG_90 {
	uint32_t raw;
	struct {
		uint32_t CK_HS_STATE                     : 1;
		uint32_t CK_ULPS_STATE                   : 1;
		uint32_t CK_STOPSTATE                    : 1;
		uint32_t CK_ERR_STATE                    : 1;
		uint32_t DESKEW_STATE                    : 2;
	} bits;
};

union REG_SENSOR_PHY_8L_DBG_94 {
	uint32_t raw;
	struct {
		uint32_t D0_DATAHS_STATE                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t D1_DATAHS_STATE                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t D2_DATAHS_STATE                 : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t D3_DATAHS_STATE                 : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t D4_DATAHS_STATE                 : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t D5_DATAHS_STATE                 : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t D6_DATAHS_STATE                 : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t D7_DATAHS_STATE                 : 3;
	} bits;
};

union REG_SENSOR_PHY_8L_STATUS_98 {
	uint32_t raw;
	struct {
		uint32_t CK_LP_STATUS_CLR                : 8;
		uint32_t D0_LP_STATUS_CLR                : 8;
		uint32_t D1_LP_STATUS_CLR                : 8;
		uint32_t D2_LP_STATUS_CLR                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_STATUS_9C {
	uint32_t raw;
	struct {
		uint32_t D3_LP_STATUS_CLR                : 8;
		uint32_t D4_LP_STATUS_CLR                : 8;
		uint32_t D5_LP_STATUS_CLR                : 8;
		uint32_t D6_LP_STATUS_CLR                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_STATUS_A0 {
	uint32_t raw;
	struct {
		uint32_t D7_LP_STATUS_CLR                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_STATUS_A4 {
	uint32_t raw;
	struct {
		uint32_t CK_LP_STATUS_OUT                : 8;
		uint32_t D0_LP_STATUS_OUT                : 8;
		uint32_t D1_LP_STATUS_OUT                : 8;
		uint32_t D2_LP_STATUS_OUT                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_STATUS_A8 {
	uint32_t raw;
	struct {
		uint32_t D3_LP_STATUS_OUT                : 8;
		uint32_t D4_LP_STATUS_OUT                : 8;
		uint32_t D5_LP_STATUS_OUT                : 8;
		uint32_t D6_LP_STATUS_OUT                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_STATUS_AC {
	uint32_t raw;
	struct {
		uint32_t D7_LP_STATUS_OUT                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_0 {
	uint32_t raw;
	struct {
		uint32_t D0_PRBS9_EN                     : 1;
		uint32_t D0_PRBS9_CLR_ERR                : 1;
		uint32_t D0_PRBS9_SOURCE                 : 1;
		uint32_t D0_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D0_CALIB_MAX                    : 8;
		uint32_t D0_CALIB_STEP                   : 8;
		uint32_t D0_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_1 {
	uint32_t raw;
	struct {
		uint32_t D0_CALIB_EN                     : 1;
		uint32_t D0_CALIB_SOURCE                 : 1;
		uint32_t D0_CALIB_MODE                   : 1;
		uint32_t D0_CALIB_IGNORE                 : 1;
		uint32_t D0_CALIB_SETTLE                 : 3;
		uint32_t D0_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D0_CALIB_SET_PHASE              : 8;
		uint32_t D0_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_2 {
	uint32_t raw;
	struct {
		uint32_t D0_PRBS9_RX_ERR                 : 1;
		uint32_t D0_PRBS9_TEST_DONE              : 1;
		uint32_t D0_PRBS9_TEST_PASS              : 1;
		uint32_t D0_SKEW_CALIB_DONE              : 1;
		uint32_t D0_SKEW_CALIB_FAIL              : 1;
		uint32_t D0_DATALP_STATE                 : 4;
		uint32_t D0_DATALP_LPREQ2ERR             : 1;
		uint32_t D0_DATALP_DATAESC2ERR           : 1;
		uint32_t D0_DATALP_RSTTRI2ERR            : 1;
		uint32_t D0_DATALP_HSTEST2ERR            : 1;
		uint32_t D0_DATALP_ESCULP2ERR            : 1;
		uint32_t D0_DATALP_HS2ERR                : 1;
		uint32_t D0_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D0_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_3 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_4 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_5 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_6 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_7 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_8 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_9 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_A {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D0_B {
	uint32_t raw;
	struct {
		uint32_t D0_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D0_CALIB_THRESHOLD              : 8;
		uint32_t D0_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_0 {
	uint32_t raw;
	struct {
		uint32_t D1_PRBS9_EN                     : 1;
		uint32_t D1_PRBS9_CLR_ERR                : 1;
		uint32_t D1_PRBS9_SOURCE                 : 1;
		uint32_t D1_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D1_CALIB_MAX                    : 8;
		uint32_t D1_CALIB_STEP                   : 8;
		uint32_t D1_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_1 {
	uint32_t raw;
	struct {
		uint32_t D1_CALIB_EN                     : 1;
		uint32_t D1_CALIB_SOURCE                 : 1;
		uint32_t D1_CALIB_MODE                   : 1;
		uint32_t D1_CALIB_IGNORE                 : 1;
		uint32_t D1_CALIB_SETTLE                 : 3;
		uint32_t D1_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D1_CALIB_SET_PHASE              : 8;
		uint32_t D1_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_2 {
	uint32_t raw;
	struct {
		uint32_t D1_PRBS9_RX_ERR                 : 1;
		uint32_t D1_PRBS9_TEST_DONE              : 1;
		uint32_t D1_PRBS9_TEST_PASS              : 1;
		uint32_t D1_SKEW_CALIB_DONE              : 1;
		uint32_t D1_SKEW_CALIB_FAIL              : 1;
		uint32_t D1_DATALP_STATE                 : 4;
		uint32_t D1_DATALP_LPREQ2ERR             : 1;
		uint32_t D1_DATALP_DATAESC2ERR           : 1;
		uint32_t D1_DATALP_RSTTRI2ERR            : 1;
		uint32_t D1_DATALP_HSTEST2ERR            : 1;
		uint32_t D1_DATALP_ESCULP2ERR            : 1;
		uint32_t D1_DATALP_HS2ERR                : 1;
		uint32_t D1_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D1_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_3 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_4 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_5 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_6 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_7 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_8 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_9 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_A {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D1_B {
	uint32_t raw;
	struct {
		uint32_t D1_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D1_CALIB_THRESHOLD              : 8;
		uint32_t D1_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_0 {
	uint32_t raw;
	struct {
		uint32_t D2_PRBS9_EN                     : 1;
		uint32_t D2_PRBS9_CLR_ERR                : 1;
		uint32_t D2_PRBS9_SOURCE                 : 1;
		uint32_t D2_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D2_CALIB_MAX                    : 8;
		uint32_t D2_CALIB_STEP                   : 8;
		uint32_t D2_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_1 {
	uint32_t raw;
	struct {
		uint32_t D2_CALIB_EN                     : 1;
		uint32_t D2_CALIB_SOURCE                 : 1;
		uint32_t D2_CALIB_MODE                   : 1;
		uint32_t D2_CALIB_IGNORE                 : 1;
		uint32_t D2_CALIB_SETTLE                 : 3;
		uint32_t D2_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D2_CALIB_SET_PHASE              : 8;
		uint32_t D2_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_2 {
	uint32_t raw;
	struct {
		uint32_t D2_PRBS9_RX_ERR                 : 1;
		uint32_t D2_PRBS9_TEST_DONE              : 1;
		uint32_t D2_PRBS9_TEST_PASS              : 1;
		uint32_t D2_SKEW_CALIB_DONE              : 1;
		uint32_t D2_SKEW_CALIB_FAIL              : 1;
		uint32_t D2_DATALP_STATE                 : 4;
		uint32_t D2_DATALP_LPREQ2ERR             : 1;
		uint32_t D2_DATALP_DATAESC2ERR           : 1;
		uint32_t D2_DATALP_RSTTRI2ERR            : 1;
		uint32_t D2_DATALP_HSTEST2ERR            : 1;
		uint32_t D2_DATALP_ESCULP2ERR            : 1;
		uint32_t D2_DATALP_HS2ERR                : 1;
		uint32_t D2_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D2_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_3 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_4 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_5 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_6 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_7 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_8 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_9 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_A {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D2_B {
	uint32_t raw;
	struct {
		uint32_t D2_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D2_CALIB_THRESHOLD              : 8;
		uint32_t D2_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_0 {
	uint32_t raw;
	struct {
		uint32_t D3_PRBS9_EN                     : 1;
		uint32_t D3_PRBS9_CLR_ERR                : 1;
		uint32_t D3_PRBS9_SOURCE                 : 1;
		uint32_t D3_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D3_CALIB_MAX                    : 8;
		uint32_t D3_CALIB_STEP                   : 8;
		uint32_t D3_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_1 {
	uint32_t raw;
	struct {
		uint32_t D3_CALIB_EN                     : 1;
		uint32_t D3_CALIB_SOURCE                 : 1;
		uint32_t D3_CALIB_MODE                   : 1;
		uint32_t D3_CALIB_IGNORE                 : 1;
		uint32_t D3_CALIB_SETTLE                 : 3;
		uint32_t D3_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D3_CALIB_SET_PHASE              : 8;
		uint32_t D3_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_2 {
	uint32_t raw;
	struct {
		uint32_t D3_PRBS9_RX_ERR                 : 1;
		uint32_t D3_PRBS9_TEST_DONE              : 1;
		uint32_t D3_PRBS9_TEST_PASS              : 1;
		uint32_t D3_SKEW_CALIB_DONE              : 1;
		uint32_t D3_SKEW_CALIB_FAIL              : 1;
		uint32_t D3_DATALP_STATE                 : 4;
		uint32_t D3_DATALP_LPREQ2ERR             : 1;
		uint32_t D3_DATALP_DATAESC2ERR           : 1;
		uint32_t D3_DATALP_RSTTRI2ERR            : 1;
		uint32_t D3_DATALP_HSTEST2ERR            : 1;
		uint32_t D3_DATALP_ESCULP2ERR            : 1;
		uint32_t D3_DATALP_HS2ERR                : 1;
		uint32_t D3_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D3_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_3 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_4 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_5 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_6 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_7 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_8 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_9 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_A {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D3_B {
	uint32_t raw;
	struct {
		uint32_t D3_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D3_CALIB_THRESHOLD              : 8;
		uint32_t D3_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_0 {
	uint32_t raw;
	struct {
		uint32_t D4_PRBS9_EN                     : 1;
		uint32_t D4_PRBS9_CLR_ERR                : 1;
		uint32_t D4_PRBS9_SOURCE                 : 1;
		uint32_t D4_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D4_CALIB_MAX                    : 8;
		uint32_t D4_CALIB_STEP                   : 8;
		uint32_t D4_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_1 {
	uint32_t raw;
	struct {
		uint32_t D4_CALIB_EN                     : 1;
		uint32_t D4_CALIB_SOURCE                 : 1;
		uint32_t D4_CALIB_MODE                   : 1;
		uint32_t D4_CALIB_IGNORE                 : 1;
		uint32_t D4_CALIB_SETTLE                 : 3;
		uint32_t D4_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D4_CALIB_SET_PHASE              : 8;
		uint32_t D4_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_2 {
	uint32_t raw;
	struct {
		uint32_t D4_PRBS9_RX_ERR                 : 1;
		uint32_t D4_PRBS9_TEST_DONE              : 1;
		uint32_t D4_PRBS9_TEST_PASS              : 1;
		uint32_t D4_SKEW_CALIB_DONE              : 1;
		uint32_t D4_SKEW_CALIB_FAIL              : 1;
		uint32_t D4_DATALP_STATE                 : 4;
		uint32_t D4_DATALP_LPREQ2ERR             : 1;
		uint32_t D4_DATALP_DATAESC2ERR           : 1;
		uint32_t D4_DATALP_RSTTRI2ERR            : 1;
		uint32_t D4_DATALP_HSTEST2ERR            : 1;
		uint32_t D4_DATALP_ESCULP2ERR            : 1;
		uint32_t D4_DATALP_HS2ERR                : 1;
		uint32_t D4_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D4_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_3 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_4 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_5 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_6 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_7 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_8 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_9 {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_A {
	uint32_t raw;
	struct {
		uint32_t D4_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D4_B {
	uint32_t raw;
	struct {
		uint32_t D4_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D4_CALIB_THRESHOLD              : 8;
		uint32_t D4_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_0 {
	uint32_t raw;
	struct {
		uint32_t D5_PRBS9_EN                     : 1;
		uint32_t D5_PRBS9_CLR_ERR                : 1;
		uint32_t D5_PRBS9_SOURCE                 : 1;
		uint32_t D5_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D5_CALIB_MAX                    : 8;
		uint32_t D5_CALIB_STEP                   : 8;
		uint32_t D5_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_1 {
	uint32_t raw;
	struct {
		uint32_t D5_CALIB_EN                     : 1;
		uint32_t D5_CALIB_SOURCE                 : 1;
		uint32_t D5_CALIB_MODE                   : 1;
		uint32_t D5_CALIB_IGNORE                 : 1;
		uint32_t D5_CALIB_SETTLE                 : 3;
		uint32_t D5_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D5_CALIB_SET_PHASE              : 8;
		uint32_t D5_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_2 {
	uint32_t raw;
	struct {
		uint32_t D5_PRBS9_RX_ERR                 : 1;
		uint32_t D5_PRBS9_TEST_DONE              : 1;
		uint32_t D5_PRBS9_TEST_PASS              : 1;
		uint32_t D5_SKEW_CALIB_DONE              : 1;
		uint32_t D5_SKEW_CALIB_FAIL              : 1;
		uint32_t D5_DATALP_STATE                 : 4;
		uint32_t D5_DATALP_LPREQ2ERR             : 1;
		uint32_t D5_DATALP_DATAESC2ERR           : 1;
		uint32_t D5_DATALP_RSTTRI2ERR            : 1;
		uint32_t D5_DATALP_HSTEST2ERR            : 1;
		uint32_t D5_DATALP_ESCULP2ERR            : 1;
		uint32_t D5_DATALP_HS2ERR                : 1;
		uint32_t D5_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D5_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_3 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_4 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_5 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_6 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_7 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_8 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_9 {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_A {
	uint32_t raw;
	struct {
		uint32_t D5_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D5_B {
	uint32_t raw;
	struct {
		uint32_t D5_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D5_CALIB_THRESHOLD              : 8;
		uint32_t D5_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_0 {
	uint32_t raw;
	struct {
		uint32_t D6_PRBS9_EN                     : 1;
		uint32_t D6_PRBS9_CLR_ERR                : 1;
		uint32_t D6_PRBS9_SOURCE                 : 1;
		uint32_t D6_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D6_CALIB_MAX                    : 8;
		uint32_t D6_CALIB_STEP                   : 8;
		uint32_t D6_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_1 {
	uint32_t raw;
	struct {
		uint32_t D6_CALIB_EN                     : 1;
		uint32_t D6_CALIB_SOURCE                 : 1;
		uint32_t D6_CALIB_MODE                   : 1;
		uint32_t D6_CALIB_IGNORE                 : 1;
		uint32_t D6_CALIB_SETTLE                 : 3;
		uint32_t D6_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D6_CALIB_SET_PHASE              : 8;
		uint32_t D6_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_2 {
	uint32_t raw;
	struct {
		uint32_t D6_PRBS9_RX_ERR                 : 1;
		uint32_t D6_PRBS9_TEST_DONE              : 1;
		uint32_t D6_PRBS9_TEST_PASS              : 1;
		uint32_t D6_SKEW_CALIB_DONE              : 1;
		uint32_t D6_SKEW_CALIB_FAIL              : 1;
		uint32_t D6_DATALP_STATE                 : 4;
		uint32_t D6_DATALP_LPREQ2ERR             : 1;
		uint32_t D6_DATALP_DATAESC2ERR           : 1;
		uint32_t D6_DATALP_RSTTRI2ERR            : 1;
		uint32_t D6_DATALP_HSTEST2ERR            : 1;
		uint32_t D6_DATALP_ESCULP2ERR            : 1;
		uint32_t D6_DATALP_HS2ERR                : 1;
		uint32_t D6_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D6_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_3 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_4 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_5 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_6 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_7 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_8 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_9 {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_A {
	uint32_t raw;
	struct {
		uint32_t D6_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D6_B {
	uint32_t raw;
	struct {
		uint32_t D6_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D6_CALIB_THRESHOLD              : 8;
		uint32_t D6_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_0 {
	uint32_t raw;
	struct {
		uint32_t D7_PRBS9_EN                     : 1;
		uint32_t D7_PRBS9_CLR_ERR                : 1;
		uint32_t D7_PRBS9_SOURCE                 : 1;
		uint32_t D7_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D7_CALIB_MAX                    : 8;
		uint32_t D7_CALIB_STEP                   : 8;
		uint32_t D7_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_1 {
	uint32_t raw;
	struct {
		uint32_t D7_CALIB_EN                     : 1;
		uint32_t D7_CALIB_SOURCE                 : 1;
		uint32_t D7_CALIB_MODE                   : 1;
		uint32_t D7_CALIB_IGNORE                 : 1;
		uint32_t D7_CALIB_SETTLE                 : 3;
		uint32_t D7_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D7_CALIB_SET_PHASE              : 8;
		uint32_t D7_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_2 {
	uint32_t raw;
	struct {
		uint32_t D7_PRBS9_RX_ERR                 : 1;
		uint32_t D7_PRBS9_TEST_DONE              : 1;
		uint32_t D7_PRBS9_TEST_PASS              : 1;
		uint32_t D7_SKEW_CALIB_DONE              : 1;
		uint32_t D7_SKEW_CALIB_FAIL              : 1;
		uint32_t D7_DATALP_STATE                 : 4;
		uint32_t D7_DATALP_LPREQ2ERR             : 1;
		uint32_t D7_DATALP_DATAESC2ERR           : 1;
		uint32_t D7_DATALP_RSTTRI2ERR            : 1;
		uint32_t D7_DATALP_HSTEST2ERR            : 1;
		uint32_t D7_DATALP_ESCULP2ERR            : 1;
		uint32_t D7_DATALP_HS2ERR                : 1;
		uint32_t D7_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D7_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_3 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_4 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_5 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_6 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_7 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_8 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_9 {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_A {
	uint32_t raw;
	struct {
		uint32_t D7_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_8L_D7_B {
	uint32_t raw;
	struct {
		uint32_t D7_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D7_CALIB_THRESHOLD              : 8;
		uint32_t D7_CALIB_GP_COUNT               : 9;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_SENSOR_PHY_4L_00 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MODE                     : 2;
	} bits;
};

union REG_SENSOR_PHY_4L_04 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_D0_SEL                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t CSI_LANE_D1_SEL                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t CSI_LANE_D2_SEL                 : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t CSI_LANE_D3_SEL                 : 3;
	} bits;
};

union REG_SENSOR_PHY_4L_08 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_CK_SEL                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t CSI_LANE_CK_PNSWAP              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t CSI_LANE_D0_PNSWAP              : 1;
		uint32_t CSI_LANE_D1_PNSWAP              : 1;
		uint32_t CSI_LANE_D2_PNSWAP              : 1;
		uint32_t CSI_LANE_D3_PNSWAP              : 1;
		uint32_t _rsv_12                         : 4;
		uint32_t CSI_CK_PHASE                    : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_0C {
	uint32_t raw;
	struct {
		uint32_t DESKEW_LANE_EN                  : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t PRBS9_TEST_PERIOD               : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_10 {
	uint32_t raw;
	struct {
		uint32_t T_HS_SETTLE                     : 8;
		uint32_t T_ALL_ZERO                      : 8;
		uint32_t AUTO_IGNORE                     : 1;
		uint32_t AUTO_SYNC                       : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_20 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_INV_EN                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t SLVDS_BIT_MODE                  : 2;
		uint32_t SLVDS_LANE_EN                   : 4;
		uint32_t _rsv_8                          : 4;
		uint32_t SLVDS_FORCE_RESYNC              : 1;
		uint32_t SLVDS_RESYNC                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t SLVDS_SAV_1ST                   : 12;
	} bits;
};

union REG_SENSOR_PHY_4L_24 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_SAV_2ND                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SLVDS_SAV_3RD                   : 12;
	} bits;
};

union REG_SENSOR_PHY_4L_28 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_D0_SYNC_STATE             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t SLVDS_D1_SYNC_STATE             : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t SLVDS_D2_SYNC_STATE             : 2;
		uint32_t _rsv_10                         : 2;
		uint32_t SLVDS_D3_SYNC_STATE             : 2;
	} bits;
};

union REG_SENSOR_PHY_4L_30 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_LANE_EN                  : 4;
		uint32_t _rsv_4                          : 4;
		uint32_t SLVSEC_SKEW_CNT_EN              : 1;
		uint32_t SLVSEC_TRAIN_SEQ_CHK_EN         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t SLVSEC_SKEW_CONS                : 5;
		uint32_t SLVSEC_FORCE_RESYNC             : 1;
		uint32_t SLVSEC_RESYNC                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t SLVSEC_UNSTABLE_SKEW_CNT        : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_34 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_SYNC_SYMBOL              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t SLVSEC_STANDBY_SYMBOL           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t SLVSEC_DESKEW_SYMBOL            : 9;
	} bits;
};

union REG_SENSOR_PHY_4L_38 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PRBS9_TEST_PERIOD        : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_3C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_CLR             : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_40 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_MASK            : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_44 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_STATUS          : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_48 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D0_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D0_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D0_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_4C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D0_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D0_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D0_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D0_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D0_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D0_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D0_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D0_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D0_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_50 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D1_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D1_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D1_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_54 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D1_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D1_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D1_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D1_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D1_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D1_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D1_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D1_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D1_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_58 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D2_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D2_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D2_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_5C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D2_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D2_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D2_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D2_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D2_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D2_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D2_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D2_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D2_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_60 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D3_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D3_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D3_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_64 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D3_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D3_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D3_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D3_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D3_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D3_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D3_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D3_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D3_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_4L_DBG_90 {
	uint32_t raw;
	struct {
		uint32_t CK_HS_STATE                     : 1;
		uint32_t CK_ULPS_STATE                   : 1;
		uint32_t CK_STOPSTATE                    : 1;
		uint32_t CK_ERR_STATE                    : 1;
		uint32_t DESKEW_STATE                    : 2;
	} bits;
};

union REG_SENSOR_PHY_4L_DBG_94 {
	uint32_t raw;
	struct {
		uint32_t D0_DATAHS_STATE                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t D1_DATAHS_STATE                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t D2_DATAHS_STATE                 : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t D3_DATAHS_STATE                 : 3;
	} bits;
};

union REG_SENSOR_PHY_4L_STATUS_98 {
	uint32_t raw;
	struct {
		uint32_t CK_LP_STATUS_CLR                : 8;
		uint32_t D0_LP_STATUS_CLR                : 8;
		uint32_t D1_LP_STATUS_CLR                : 8;
		uint32_t D2_LP_STATUS_CLR                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_STATUS_9C {
	uint32_t raw;
	struct {
		uint32_t D3_LP_STATUS_CLR                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_STATUS_A4 {
	uint32_t raw;
	struct {
		uint32_t CK_LP_STATUS_OUT                : 8;
		uint32_t D0_LP_STATUS_OUT                : 8;
		uint32_t D1_LP_STATUS_OUT                : 8;
		uint32_t D2_LP_STATUS_OUT                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_STATUS_A8 {
	uint32_t raw;
	struct {
		uint32_t D3_LP_STATUS_OUT                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_0 {
	uint32_t raw;
	struct {
		uint32_t D0_PRBS9_EN                     : 1;
		uint32_t D0_PRBS9_CLR_ERR                : 1;
		uint32_t D0_PRBS9_SOURCE                 : 1;
		uint32_t D0_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D0_CALIB_MAX                    : 8;
		uint32_t D0_CALIB_STEP                   : 8;
		uint32_t D0_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_1 {
	uint32_t raw;
	struct {
		uint32_t D0_CALIB_EN                     : 1;
		uint32_t D0_CALIB_SOURCE                 : 1;
		uint32_t D0_CALIB_MODE                   : 1;
		uint32_t D0_CALIB_IGNORE                 : 1;
		uint32_t D0_CALIB_SETTLE                 : 3;
		uint32_t D0_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D0_CALIB_SET_PHASE              : 8;
		uint32_t D0_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_2 {
	uint32_t raw;
	struct {
		uint32_t D0_PRBS9_RX_ERR                 : 1;
		uint32_t D0_PRBS9_TEST_DONE              : 1;
		uint32_t D0_PRBS9_TEST_PASS              : 1;
		uint32_t D0_SKEW_CALIB_DONE              : 1;
		uint32_t D0_SKEW_CALIB_FAIL              : 1;
		uint32_t D0_DATALP_STATE                 : 4;
		uint32_t D0_DATALP_LPREQ2ERR             : 1;
		uint32_t D0_DATALP_DATAESC2ERR           : 1;
		uint32_t D0_DATALP_RSTTRI2ERR            : 1;
		uint32_t D0_DATALP_HSTEST2ERR            : 1;
		uint32_t D0_DATALP_ESCULP2ERR            : 1;
		uint32_t D0_DATALP_HS2ERR                : 1;
		uint32_t D0_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D0_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_3 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_4 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_5 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_6 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_7 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_8 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_9 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_A {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D0_B {
	uint32_t raw;
	struct {
		uint32_t D0_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D0_CALIB_THRESHOLD              : 8;
		uint32_t D0_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_0 {
	uint32_t raw;
	struct {
		uint32_t D1_PRBS9_EN                     : 1;
		uint32_t D1_PRBS9_CLR_ERR                : 1;
		uint32_t D1_PRBS9_SOURCE                 : 1;
		uint32_t D1_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D1_CALIB_MAX                    : 8;
		uint32_t D1_CALIB_STEP                   : 8;
		uint32_t D1_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_1 {
	uint32_t raw;
	struct {
		uint32_t D1_CALIB_EN                     : 1;
		uint32_t D1_CALIB_SOURCE                 : 1;
		uint32_t D1_CALIB_MODE                   : 1;
		uint32_t D1_CALIB_IGNORE                 : 1;
		uint32_t D1_CALIB_SETTLE                 : 3;
		uint32_t D1_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D1_CALIB_SET_PHASE              : 8;
		uint32_t D1_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_2 {
	uint32_t raw;
	struct {
		uint32_t D1_PRBS9_RX_ERR                 : 1;
		uint32_t D1_PRBS9_TEST_DONE              : 1;
		uint32_t D1_PRBS9_TEST_PASS              : 1;
		uint32_t D1_SKEW_CALIB_DONE              : 1;
		uint32_t D1_SKEW_CALIB_FAIL              : 1;
		uint32_t D1_DATALP_STATE                 : 4;
		uint32_t D1_DATALP_LPREQ2ERR             : 1;
		uint32_t D1_DATALP_DATAESC2ERR           : 1;
		uint32_t D1_DATALP_RSTTRI2ERR            : 1;
		uint32_t D1_DATALP_HSTEST2ERR            : 1;
		uint32_t D1_DATALP_ESCULP2ERR            : 1;
		uint32_t D1_DATALP_HS2ERR                : 1;
		uint32_t D1_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D1_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_3 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_4 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_5 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_6 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_7 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_8 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_9 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_A {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D1_B {
	uint32_t raw;
	struct {
		uint32_t D1_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D1_CALIB_THRESHOLD              : 8;
		uint32_t D1_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_0 {
	uint32_t raw;
	struct {
		uint32_t D2_PRBS9_EN                     : 1;
		uint32_t D2_PRBS9_CLR_ERR                : 1;
		uint32_t D2_PRBS9_SOURCE                 : 1;
		uint32_t D2_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D2_CALIB_MAX                    : 8;
		uint32_t D2_CALIB_STEP                   : 8;
		uint32_t D2_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_1 {
	uint32_t raw;
	struct {
		uint32_t D2_CALIB_EN                     : 1;
		uint32_t D2_CALIB_SOURCE                 : 1;
		uint32_t D2_CALIB_MODE                   : 1;
		uint32_t D2_CALIB_IGNORE                 : 1;
		uint32_t D2_CALIB_SETTLE                 : 3;
		uint32_t D2_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D2_CALIB_SET_PHASE              : 8;
		uint32_t D2_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_2 {
	uint32_t raw;
	struct {
		uint32_t D2_PRBS9_RX_ERR                 : 1;
		uint32_t D2_PRBS9_TEST_DONE              : 1;
		uint32_t D2_PRBS9_TEST_PASS              : 1;
		uint32_t D2_SKEW_CALIB_DONE              : 1;
		uint32_t D2_SKEW_CALIB_FAIL              : 1;
		uint32_t D2_DATALP_STATE                 : 4;
		uint32_t D2_DATALP_LPREQ2ERR             : 1;
		uint32_t D2_DATALP_DATAESC2ERR           : 1;
		uint32_t D2_DATALP_RSTTRI2ERR            : 1;
		uint32_t D2_DATALP_HSTEST2ERR            : 1;
		uint32_t D2_DATALP_ESCULP2ERR            : 1;
		uint32_t D2_DATALP_HS2ERR                : 1;
		uint32_t D2_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D2_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_3 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_4 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_5 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_6 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_7 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_8 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_9 {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_A {
	uint32_t raw;
	struct {
		uint32_t D2_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D2_B {
	uint32_t raw;
	struct {
		uint32_t D2_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D2_CALIB_THRESHOLD              : 8;
		uint32_t D2_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_0 {
	uint32_t raw;
	struct {
		uint32_t D3_PRBS9_EN                     : 1;
		uint32_t D3_PRBS9_CLR_ERR                : 1;
		uint32_t D3_PRBS9_SOURCE                 : 1;
		uint32_t D3_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D3_CALIB_MAX                    : 8;
		uint32_t D3_CALIB_STEP                   : 8;
		uint32_t D3_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_1 {
	uint32_t raw;
	struct {
		uint32_t D3_CALIB_EN                     : 1;
		uint32_t D3_CALIB_SOURCE                 : 1;
		uint32_t D3_CALIB_MODE                   : 1;
		uint32_t D3_CALIB_IGNORE                 : 1;
		uint32_t D3_CALIB_SETTLE                 : 3;
		uint32_t D3_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D3_CALIB_SET_PHASE              : 8;
		uint32_t D3_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_2 {
	uint32_t raw;
	struct {
		uint32_t D3_PRBS9_RX_ERR                 : 1;
		uint32_t D3_PRBS9_TEST_DONE              : 1;
		uint32_t D3_PRBS9_TEST_PASS              : 1;
		uint32_t D3_SKEW_CALIB_DONE              : 1;
		uint32_t D3_SKEW_CALIB_FAIL              : 1;
		uint32_t D3_DATALP_STATE                 : 4;
		uint32_t D3_DATALP_LPREQ2ERR             : 1;
		uint32_t D3_DATALP_DATAESC2ERR           : 1;
		uint32_t D3_DATALP_RSTTRI2ERR            : 1;
		uint32_t D3_DATALP_HSTEST2ERR            : 1;
		uint32_t D3_DATALP_ESCULP2ERR            : 1;
		uint32_t D3_DATALP_HS2ERR                : 1;
		uint32_t D3_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D3_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_3 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_4 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_5 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_6 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_7 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_8 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_9 {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_A {
	uint32_t raw;
	struct {
		uint32_t D3_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_4L_D3_B {
	uint32_t raw;
	struct {
		uint32_t D3_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D3_CALIB_THRESHOLD              : 8;
		uint32_t D3_CALIB_GP_COUNT               : 9;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_SENSOR_PHY_2L_00 {
	uint32_t raw;
	struct {
		uint32_t SENSOR_MODE                     : 2;
	} bits;
};

union REG_SENSOR_PHY_2L_04 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_D0_SEL                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t CSI_LANE_D1_SEL                 : 2;
	} bits;
};

union REG_SENSOR_PHY_2L_08 {
	uint32_t raw;
	struct {
		uint32_t CSI_LANE_CK_SEL                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t CSI_LANE_CK_PNSWAP              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t CSI_LANE_D0_PNSWAP              : 1;
		uint32_t CSI_LANE_D1_PNSWAP              : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t CSI_CK_PHASE                    : 8;
	} bits;
};

union REG_SENSOR_PHY_2L_0C {
	uint32_t raw;
	struct {
		uint32_t DESKEW_LANE_EN                  : 2;
		uint32_t _rsv_2                          : 14;
		uint32_t PRBS9_TEST_PERIOD               : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_10 {
	uint32_t raw;
	struct {
		uint32_t T_HS_SETTLE                     : 8;
		uint32_t T_ALL_ZERO                      : 8;
		uint32_t AUTO_IGNORE                     : 1;
		uint32_t AUTO_SYNC                       : 1;
	} bits;
};

union REG_SENSOR_PHY_2L_20 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_INV_EN                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t SLVDS_BIT_MODE                  : 2;
		uint32_t SLVDS_LANE_EN                   : 2;
		uint32_t _rsv_6                          : 6;
		uint32_t SLVDS_FORCE_RESYNC              : 1;
		uint32_t SLVDS_RESYNC                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t SLVDS_SAV_1ST                   : 12;
	} bits;
};

union REG_SENSOR_PHY_2L_24 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_SAV_2ND                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SLVDS_SAV_3RD                   : 12;
	} bits;
};

union REG_SENSOR_PHY_2L_28 {
	uint32_t raw;
	struct {
		uint32_t SLVDS_D0_SYNC_STATE             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t SLVDS_D1_SYNC_STATE             : 2;
	} bits;
};

union REG_SENSOR_PHY_2L_30 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_LANE_EN                  : 2;
		uint32_t _rsv_2                          : 6;
		uint32_t SLVSEC_SKEW_CNT_EN              : 1;
		uint32_t SLVSEC_TRAIN_SEQ_CHK_EN         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t SLVSEC_SKEW_CONS                : 5;
		uint32_t SLVSEC_FORCE_RESYNC             : 1;
		uint32_t SLVSEC_RESYNC                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t SLVSEC_UNSTABLE_SKEW_CNT        : 8;
	} bits;
};

union REG_SENSOR_PHY_2L_34 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_SYNC_SYMBOL              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t SLVSEC_STANDBY_SYMBOL           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t SLVSEC_DESKEW_SYMBOL            : 9;
	} bits;
};

union REG_SENSOR_PHY_2L_38 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PRBS9_TEST_PERIOD        : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_3C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_CLR             : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_40 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_MASK            : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_44 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_PHY_INTR_STATUS          : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_48 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D0_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D0_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D0_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_2L_4C {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D0_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D0_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D0_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D0_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D0_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D0_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D0_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D0_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D0_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_2L_50 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D1_TEST_PAT_EN           : 1;
		uint32_t SLVSEC_D1_CLR_TEST_PAT_ERR      : 1;
		uint32_t SLVSEC_D1_TEST_STOP_WHEN_DONE   : 1;
	} bits;
};

union REG_SENSOR_PHY_2L_54 {
	uint32_t raw;
	struct {
		uint32_t SLVSEC_D1_TEST_PAT_ERR_CNT      : 16;
		uint32_t SLVSEC_D1_TEST_PAT_ERR          : 1;
		uint32_t SLVSEC_D1_TEST_PAT_PASS         : 1;
		uint32_t SLVSEC_D1_TEST_PAT_DONE         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t SLVSEC_D1_START_CODE_ERR        : 1;
		uint32_t SLVSEC_D1_END_CODE_ERR          : 1;
		uint32_t SLVSEC_D1_DESKEW_CODE_ERR       : 1;
		uint32_t SLVSEC_D1_STANDBY_CODE_ERR      : 1;
		uint32_t SLVSEC_D1_SYNC_CODE_ERR         : 1;
	} bits;
};

union REG_SENSOR_PHY_2L_DBG_90 {
	uint32_t raw;
	struct {
		uint32_t CK_HS_STATE                     : 1;
		uint32_t CK_ULPS_STATE                   : 1;
		uint32_t CK_STOPSTATE                    : 1;
		uint32_t CK_ERR_STATE                    : 1;
		uint32_t DESKEW_STATE                    : 2;
	} bits;
};

union REG_SENSOR_PHY_2L_DBG_94 {
	uint32_t raw;
	struct {
		uint32_t D0_DATAHS_STATE                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t D1_DATAHS_STATE                 : 3;
	} bits;
};

union REG_SENSOR_PHY_2L_STATUS_98 {
	uint32_t raw;
	struct {
		uint32_t CK_LP_STATUS_CLR                : 8;
		uint32_t D0_LP_STATUS_CLR                : 8;
		uint32_t D1_LP_STATUS_CLR                : 8;
	} bits;
};

union REG_SENSOR_PHY_2L_STATUS_9C {
	uint32_t raw;
	struct {
		uint32_t CK_LP_STATUS_OUT                : 8;
		uint32_t D0_LP_STATUS_OUT                : 8;
		uint32_t D1_LP_STATUS_OUT                : 8;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_0 {
	uint32_t raw;
	struct {
		uint32_t D0_PRBS9_EN                     : 1;
		uint32_t D0_PRBS9_CLR_ERR                : 1;
		uint32_t D0_PRBS9_SOURCE                 : 1;
		uint32_t D0_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D0_CALIB_MAX                    : 8;
		uint32_t D0_CALIB_STEP                   : 8;
		uint32_t D0_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_1 {
	uint32_t raw;
	struct {
		uint32_t D0_CALIB_EN                     : 1;
		uint32_t D0_CALIB_SOURCE                 : 1;
		uint32_t D0_CALIB_MODE                   : 1;
		uint32_t D0_CALIB_IGNORE                 : 1;
		uint32_t D0_CALIB_SETTLE                 : 3;
		uint32_t D0_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D0_CALIB_SET_PHASE              : 8;
		uint32_t D0_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_2 {
	uint32_t raw;
	struct {
		uint32_t D0_PRBS9_RX_ERR                 : 1;
		uint32_t D0_PRBS9_TEST_DONE              : 1;
		uint32_t D0_PRBS9_TEST_PASS              : 1;
		uint32_t D0_SKEW_CALIB_DONE              : 1;
		uint32_t D0_SKEW_CALIB_FAIL              : 1;
		uint32_t D0_DATALP_STATE                 : 4;
		uint32_t D0_DATALP_LPREQ2ERR             : 1;
		uint32_t D0_DATALP_DATAESC2ERR           : 1;
		uint32_t D0_DATALP_RSTTRI2ERR            : 1;
		uint32_t D0_DATALP_HSTEST2ERR            : 1;
		uint32_t D0_DATALP_ESCULP2ERR            : 1;
		uint32_t D0_DATALP_HS2ERR                : 1;
		uint32_t D0_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D0_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_3 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_4 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_5 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_6 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_7 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_8 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_9 {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_A {
	uint32_t raw;
	struct {
		uint32_t D0_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D0_B {
	uint32_t raw;
	struct {
		uint32_t D0_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D0_CALIB_THRESHOLD              : 8;
		uint32_t D0_CALIB_GP_COUNT               : 9;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_0 {
	uint32_t raw;
	struct {
		uint32_t D1_PRBS9_EN                     : 1;
		uint32_t D1_PRBS9_CLR_ERR                : 1;
		uint32_t D1_PRBS9_SOURCE                 : 1;
		uint32_t D1_PRBS9_STOP_WHEN_DONE         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t D1_CALIB_MAX                    : 8;
		uint32_t D1_CALIB_STEP                   : 8;
		uint32_t D1_CALIB_PATTERN                : 8;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_1 {
	uint32_t raw;
	struct {
		uint32_t D1_CALIB_EN                     : 1;
		uint32_t D1_CALIB_SOURCE                 : 1;
		uint32_t D1_CALIB_MODE                   : 1;
		uint32_t D1_CALIB_IGNORE                 : 1;
		uint32_t D1_CALIB_SETTLE                 : 3;
		uint32_t D1_CALIB_PHASE_NO_SHIFT         : 1;
		uint32_t D1_CALIB_SET_PHASE              : 8;
		uint32_t D1_CALIB_CYCLE                  : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_2 {
	uint32_t raw;
	struct {
		uint32_t D1_PRBS9_RX_ERR                 : 1;
		uint32_t D1_PRBS9_TEST_DONE              : 1;
		uint32_t D1_PRBS9_TEST_PASS              : 1;
		uint32_t D1_SKEW_CALIB_DONE              : 1;
		uint32_t D1_SKEW_CALIB_FAIL              : 1;
		uint32_t D1_DATALP_STATE                 : 4;
		uint32_t D1_DATALP_LPREQ2ERR             : 1;
		uint32_t D1_DATALP_DATAESC2ERR           : 1;
		uint32_t D1_DATALP_RSTTRI2ERR            : 1;
		uint32_t D1_DATALP_HSTEST2ERR            : 1;
		uint32_t D1_DATALP_ESCULP2ERR            : 1;
		uint32_t D1_DATALP_HS2ERR                : 1;
		uint32_t D1_DATA_EXIST_1ST_BYTE          : 1;
		uint32_t D1_PRBS9_ERR_CNT                : 16;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_3 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_0          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_4 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_1          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_5 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_2          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_6 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_3          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_7 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_4          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_8 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_5          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_9 {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_6          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_A {
	uint32_t raw;
	struct {
		uint32_t D1_SKEW_CALIB_RESULT_7          : 32;
	} bits;
};

union REG_SENSOR_PHY_2L_D1_B {
	uint32_t raw;
	struct {
		uint32_t D1_CALIB_OPTION                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t D1_CALIB_THRESHOLD              : 8;
		uint32_t D1_CALIB_GP_COUNT               : 9;
	} bits;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_FIELDS_CSI_WRAP_H_ */

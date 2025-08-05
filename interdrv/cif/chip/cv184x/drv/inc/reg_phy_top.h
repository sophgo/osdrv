/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_phy_top.h
 * Description:HW register description
 */

#ifndef _REG_PHY_TOP_H_
#define _REG_PHY_TOP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union reg_sensor_phy_top_00 {
	uint32_t raw;
	struct {
		uint32_t mipirx_en_bist                  : 6;
		uint32_t _rsv_6                          : 6;
		uint32_t mipirx_en_low_band_rxafe        : 2;
		uint32_t mipirx_pd_ibias                 : 1;
		uint32_t _rsv_15                         : 1;
		uint32_t mipirx_pd_rxlp                  : 6;
	} bits;
};

union reg_sensor_phy_top_04 {
	uint32_t raw;
	struct {
		uint32_t mipirx_rt_ctrl                  : 4;
		uint32_t mipirx_sample_mode              : 2;
		uint32_t mipirx_sel_clk_p0top1           : 1;
		uint32_t mipirx_sel_clk_p1top0           : 1;
		uint32_t _rsv_8                          : 8;
		uint32_t mipirx_sel_clk_channel          : 6;
		uint32_t _rsv_22                         : 6;
		uint32_t mipirx_en_clkin_mpll_top0       : 1;
		uint32_t mipirx_sel_mpll_div_top0        : 2;
		uint32_t mipimpll_clk_csi_en             : 1;
	} bits;
};

union reg_sensor_phy_top_08 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_bist0               : 16;
		uint32_t mipirx_test_bist1               : 16;
	} bits;
};

union reg_sensor_phy_top_0c {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_bist2               : 16;
		uint32_t mipirx_test_bist3               : 16;
	} bits;
};

union reg_sensor_phy_top_10 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_bist4               : 16;
		uint32_t mipirx_test_bist5               : 16;
	} bits;
};

union reg_sensor_phy_top_20 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_demux0              : 8;
		uint32_t mipirx_test_demux1              : 8;
		uint32_t mipirx_test_demux2              : 8;
		uint32_t mipirx_test_demux3              : 8;
	} bits;
};

union reg_sensor_phy_top_24 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_demux4              : 8;
		uint32_t mipirx_test_demux5              : 8;
	} bits;
};

union reg_sensor_phy_top_2c {
	uint32_t raw;
	struct {
		uint32_t mipirx_sel_ibias_mode           : 8;
	} bits;
};

union reg_sensor_phy_top_30 {
	uint32_t raw;
	struct {
		uint32_t sensor_phy_mode                 : 3;
	} bits;
};

union reg_sensor_phy_top_34 {
	uint32_t raw;
	struct {
		uint32_t mipirx_ro_cal0                  : 32;
	} bits;
};

union reg_sensor_phy_top_38 {
	uint32_t raw;
	struct {
		uint32_t mipirx_ro_cal1                  : 32;
	} bits;
};

union reg_sensor_phy_top_3c {
	uint32_t raw;
	struct {
		uint32_t mipirx_ro_cal2                  : 32;
	} bits;
};

union reg_sensor_phy_top_40 {
	uint32_t raw;
	struct {
		uint32_t mipirx_ro_cal3                  : 32;
	} bits;
};

union reg_sensor_phy_top_44 {
	uint32_t raw;
	struct {
		uint32_t mipirx_ro_cal4                  : 32;
	} bits;
};

union reg_sensor_phy_top_48 {
	uint32_t raw;
	struct {
		uint32_t mipirx_ro_cal5                  : 32;
	} bits;
};

union reg_sensor_phy_top_70 {
	uint32_t raw;
	struct {
		uint32_t ad_d0_data                      : 8;
		uint32_t ad_d1_data                      : 8;
		uint32_t ad_d2_data                      : 8;
		uint32_t ad_d3_data                      : 8;
	} bits;
};

union reg_sensor_phy_top_74 {
	uint32_t raw;
	struct {
		uint32_t ad_d4_data                      : 8;
		uint32_t ad_d5_data                      : 8;
	} bits;
};

union reg_sensor_phy_top_7c {
	uint32_t raw;
	struct {
		uint32_t ad_lpoutn                       : 6;
		uint32_t _rsv_6                          : 10;
		uint32_t ad_lpoutp                       : 6;
	} bits;
};

union reg_sensor_phy_top_80 {
	uint32_t raw;
	struct {
		uint32_t ad_d0_clk_inv                   : 1;
		uint32_t ad_d1_clk_inv                   : 1;
		uint32_t ad_d2_clk_inv                   : 1;
		uint32_t ad_d3_clk_inv                   : 1;
		uint32_t ad_d4_clk_inv                   : 1;
		uint32_t ad_d5_clk_inv                   : 1;
		uint32_t _rsv_6                          : 10;
		uint32_t force_deskew_code0              : 1;
		uint32_t force_deskew_code1              : 1;
		uint32_t force_deskew_code2              : 1;
		uint32_t force_deskew_code3              : 1;
		uint32_t force_deskew_code4              : 1;
		uint32_t force_deskew_code5              : 1;
	} bits;
};

union reg_sensor_phy_top_84 {
	uint32_t raw;
	struct {
		uint32_t deskew_code0                    : 8;
		uint32_t deskew_code1                    : 8;
		uint32_t deskew_code2                    : 8;
		uint32_t deskew_code3                    : 8;
	} bits;
};

union reg_sensor_phy_top_88 {
	uint32_t raw;
	struct {
		uint32_t deskew_code4                    : 8;
		uint32_t deskew_code5                    : 8;
	} bits;
};

union reg_sensor_phy_top_90 {
	uint32_t raw;
	struct {
		uint32_t pd_rt                           : 6;
		uint32_t _rsv_6                          : 10;
		uint32_t force_pd_rt                     : 6;
	} bits;
};

union reg_sensor_phy_top_94 {
	uint32_t raw;
	struct {
		uint32_t pd_rxafe_ib                     : 6;
		uint32_t _rsv_6                          : 10;
		uint32_t force_pd_rxafe_ib               : 6;
	} bits;
};

union reg_sensor_phy_top_a0 {
	uint32_t raw;
	struct {
		uint32_t cam0_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam0_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_a4 {
	uint32_t raw;
	struct {
		uint32_t cam0_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam0_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_a8 {
	uint32_t raw;
	struct {
		uint32_t cam0_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam0_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_ac {
	uint32_t raw;
	struct {
		uint32_t cam0_vs_pol                     : 1;
		uint32_t cam0_hs_pol                     : 1;
		uint32_t cam0_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_dft_100 {
	uint32_t raw;
	struct {
		uint32_t dummy_0                         : 8;
		uint32_t dummy_1                         : 8;
		uint32_t dummy_2                         : 8;
		uint32_t dummy_3                         : 8;
	} bits;
};

union reg_sensor_phy_top_dft_104 {
	uint32_t raw;
	struct {
		uint32_t dummy_4                         : 8;
		uint32_t dummy_5                         : 8;
		uint32_t dummy_6                         : 8;
		uint32_t dummy_7                         : 8;
	} bits;
};

union reg_sensor_phy_top_dft_108 {
	uint32_t raw;
	struct {
		uint32_t dummy_8                         : 8;
		uint32_t dummy_9                         : 8;
		uint32_t dummy_10                        : 8;
		uint32_t dummy_11                        : 8;
	} bits;
};

union reg_sensor_phy_top_dft_10c {
	uint32_t raw;
	struct {
		uint32_t dummy_12                        : 8;
		uint32_t dummy_13                        : 8;
		uint32_t dummy_14                        : 8;
		uint32_t dummy_15                        : 8;
	} bits;
};

union reg_sensor_phy_top_dft_110 {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code0                 : 8;
		uint32_t ro_deskew_code1                 : 8;
		uint32_t ro_deskew_code2                 : 8;
		uint32_t ro_deskew_code3                 : 8;
	} bits;
};

union reg_sensor_phy_top_dft_114 {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code4                 : 8;
		uint32_t ro_deskew_code5                 : 8;
	} bits;
};

union reg_sensor_phy_top_dft_11c {
	uint32_t raw;
	struct {
		uint32_t ro_pd_rt                        : 6;
		uint32_t _rsv_6                          : 10;
		uint32_t ro_pd_rxafe_ib                  : 6;
	} bits;
};

union reg_sensor_phy_top_dbg_120 {
	uint32_t raw;
	struct {
		uint32_t dbg_sel                         : 16;
		uint32_t dbg_ck_sel                      : 8;
	} bits;
};

union reg_sensor_phy_top_test_0 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe0_l            : 32;
	} bits;
};

union reg_sensor_phy_top_test_1 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe0_h            : 32;
	} bits;
};

union reg_sensor_phy_top_test_2 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe1_l            : 32;
	} bits;
};

union reg_sensor_phy_top_test_3 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe1_h            : 32;
	} bits;
};

union reg_sensor_phy_top_test_4 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe2_l            : 32;
	} bits;
};

union reg_sensor_phy_top_test_5 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe2_h            : 32;
	} bits;
};

union reg_sensor_phy_top_test_6 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe3_l            : 32;
	} bits;
};

union reg_sensor_phy_top_test_7 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe3_h            : 32;
	} bits;
};

union reg_sensor_phy_top_test_8 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe4_l            : 32;
	} bits;
};

union reg_sensor_phy_top_test_9 {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe4_h            : 32;
	} bits;
};

union reg_sensor_phy_top_test_a {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe5_l            : 32;
	} bits;
};

union reg_sensor_phy_top_test_b {
	uint32_t raw;
	struct {
		uint32_t mipirx_test_rxafe5_h            : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d0 {
	uint32_t raw;
	struct {
		uint32_t dummy_d0                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d1 {
	uint32_t raw;
	struct {
		uint32_t dummy_d1                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d2 {
	uint32_t raw;
	struct {
		uint32_t dummy_d2                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d3 {
	uint32_t raw;
	struct {
		uint32_t dummy_d3                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d4 {
	uint32_t raw;
	struct {
		uint32_t dummy_d4                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d5 {
	uint32_t raw;
	struct {
		uint32_t dummy_d5                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d6 {
	uint32_t raw;
	struct {
		uint32_t dummy_d6                        : 32;
	} bits;
};

union reg_sensor_phy_top_dummy_d7 {
	uint32_t raw;
	struct {
		uint32_t dummy_d7                        : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_phy_top_t {
	union reg_sensor_phy_top_00             reg_00;
	union reg_sensor_phy_top_04             reg_04;
	union reg_sensor_phy_top_08             reg_08;
	union reg_sensor_phy_top_0c             reg_0c;
	union reg_sensor_phy_top_10             reg_10;
	uint32_t                                _resv_0x14[3];
	union reg_sensor_phy_top_20             reg_20;
	union reg_sensor_phy_top_24             reg_24;
	uint32_t                                _resv_0x28[1];
	union reg_sensor_phy_top_2c             reg_2c;
	union reg_sensor_phy_top_30             reg_30;
	union reg_sensor_phy_top_34             reg_34;
	union reg_sensor_phy_top_38             reg_38;
	union reg_sensor_phy_top_3c             reg_3c;
	union reg_sensor_phy_top_40             reg_40;
	union reg_sensor_phy_top_44             reg_44;
	union reg_sensor_phy_top_48             reg_48;
	uint32_t                                _resv_0x4c[9];
	union reg_sensor_phy_top_70             reg_70;
	union reg_sensor_phy_top_74             reg_74;
	uint32_t                                _resv_0x78[1];
	union reg_sensor_phy_top_7c             reg_7c;
	union reg_sensor_phy_top_80             reg_80;
	union reg_sensor_phy_top_84             reg_84;
	union reg_sensor_phy_top_88             reg_88;
	uint32_t                                _resv_0x8c[1];
	union reg_sensor_phy_top_90             reg_90;
	union reg_sensor_phy_top_94             reg_94;
	uint32_t                                _resv_0x98[2];
	union reg_sensor_phy_top_a0             reg_a0;
	union reg_sensor_phy_top_a4             reg_a4;
	union reg_sensor_phy_top_a8             reg_a8;
	union reg_sensor_phy_top_ac             reg_ac;
	uint32_t                                _resv_0xb0[20];
	union reg_sensor_phy_top_dft_100        dft_100;
	union reg_sensor_phy_top_dft_104        dft_104;
	union reg_sensor_phy_top_dft_108        dft_108;
	union reg_sensor_phy_top_dft_10c        dft_10c;
	union reg_sensor_phy_top_dft_110        dft_110;
	union reg_sensor_phy_top_dft_114        dft_114;
	uint32_t                                _resv_0x118[1];
	union reg_sensor_phy_top_dft_11c        dft_11c;
	union reg_sensor_phy_top_dbg_120        dbg_120;
	uint32_t                                _resv_0x124[55];
	union reg_sensor_phy_top_test_0         test_0;
	union reg_sensor_phy_top_test_1         test_1;
	union reg_sensor_phy_top_test_2         test_2;
	union reg_sensor_phy_top_test_3         test_3;
	union reg_sensor_phy_top_test_4         test_4;
	union reg_sensor_phy_top_test_5         test_5;
	union reg_sensor_phy_top_test_6         test_6;
	union reg_sensor_phy_top_test_7         test_7;
	union reg_sensor_phy_top_test_8         test_8;
	union reg_sensor_phy_top_test_9         test_9;
	union reg_sensor_phy_top_test_a         test_a;
	union reg_sensor_phy_top_test_b         test_b;
	uint32_t                                _resv_0x230[12];
	union reg_sensor_phy_top_dummy_d0       dummy_d0;
	union reg_sensor_phy_top_dummy_d1       dummy_d1;
	union reg_sensor_phy_top_dummy_d2       dummy_d2;
	union reg_sensor_phy_top_dummy_d3       dummy_d3;
	union reg_sensor_phy_top_dummy_d4       dummy_d4;
	union reg_sensor_phy_top_dummy_d5       dummy_d5;
	union reg_sensor_phy_top_dummy_d6       dummy_d6;
	union reg_sensor_phy_top_dummy_d7       dummy_d7;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_PHY_TOP_H_ */

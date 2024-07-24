/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_blocks_csi_wrap.h
 * Description:HW register description
 */

#ifndef _REG_BLOCKS_CSI_WRAP_H_
#define _REG_BLOCKS_CSI_WRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_phy_top_t {
	union reg_sensor_phy_top_00             reg_00;
	union reg_sensor_phy_top_04             reg_04;
	union reg_sensor_phy_top_08             reg_08;
	uint32_t                                _resv_0xc[1];
	union reg_sensor_phy_top_10             reg_10;
	union reg_sensor_phy_top_14             reg_14;
	union reg_sensor_phy_top_18             reg_18;
	union reg_sensor_phy_top_1c             reg_1c;
	union reg_sensor_phy_top_20             reg_20;
	union reg_sensor_phy_top_24             reg_24;
	union reg_sensor_phy_top_28             reg_28;
	union reg_sensor_phy_top_2c             reg_2c;
	union reg_sensor_phy_top_30             reg_30;
	union reg_sensor_phy_top_34             reg_34;
	union reg_sensor_phy_top_38             reg_38;
	union reg_sensor_phy_top_3c             reg_3c;
	union reg_sensor_phy_top_40             reg_40;
	union reg_sensor_phy_top_44             reg_44;
	union reg_sensor_phy_top_48             reg_48;
	union reg_sensor_phy_top_4c             reg_4c;
	union reg_sensor_phy_top_50             reg_50;
	union reg_sensor_phy_top_54             reg_54;
	union reg_sensor_phy_top_58             reg_58;
	union reg_sensor_phy_top_5c             reg_5c;
	union reg_sensor_phy_top_60             reg_60;
	union reg_sensor_phy_top_64             reg_64;
	union reg_sensor_phy_top_68             reg_68;
	union reg_sensor_phy_top_6c             reg_6c;
	union reg_sensor_phy_top_70             reg_70;
	union reg_sensor_phy_top_74             reg_74;
	union reg_sensor_phy_top_78             reg_78;
	union reg_sensor_phy_top_7c             reg_7c;
	union reg_sensor_phy_top_80             reg_80;
	union reg_sensor_phy_top_84             reg_84;
	union reg_sensor_phy_top_88             reg_88;
	uint32_t                                _resv_0x8c[1];
	union reg_sensor_phy_top_90             reg_90;
	union reg_sensor_phy_top_94             reg_94;
	union reg_sensor_phy_top_98             reg_98;
	union reg_sensor_phy_top_9c             reg_9c;
	union reg_sensor_phy_top_a0             reg_a0;
	union reg_sensor_phy_top_a4             reg_a4;
	union reg_sensor_phy_top_a8             reg_a8;
	union reg_sensor_phy_top_ac             reg_ac;
	union reg_sensor_phy_top_b0             reg_b0;
	union reg_sensor_phy_top_b4             reg_b4;
	union reg_sensor_phy_top_b8             reg_b8;
	union reg_sensor_phy_top_bc             reg_bc;
	union reg_sensor_phy_top_c0             reg_c0;
	union reg_sensor_phy_top_c4             reg_c4;
	uint32_t                                _resv_0xc8[2];
	union reg_sensor_phy_top_d0             reg_d0;
	union reg_sensor_phy_top_d4             reg_d4;
	union reg_sensor_phy_top_d8             reg_d8;
	union reg_sensor_phy_top_dc             reg_dc;
	union reg_sensor_phy_top_e0             reg_e0;
	union reg_sensor_phy_top_e4             reg_e4;
	union reg_sensor_phy_top_e8             reg_e8;
	union reg_sensor_phy_top_ec             reg_ec;
	union reg_sensor_phy_top_f0             reg_f0;
	union reg_sensor_phy_top_f4             reg_f4;
	union reg_sensor_phy_top_f8             reg_f8;
	union reg_sensor_phy_top_fc             reg_fc;
	union reg_sensor_phy_top_100            reg_100;
	union reg_sensor_phy_top_104            reg_104;
	union reg_sensor_phy_top_108            reg_108;
	union reg_sensor_phy_top_10c            reg_10c;
	union reg_sensor_phy_top_110            reg_110;
	uint32_t                                _resv_0x114[3];
	union reg_sensor_phy_top_120            reg_120;
	union reg_sensor_phy_top_124            reg_124;
	union reg_sensor_phy_top_128            reg_128;
	union reg_sensor_phy_top_12c            reg_12c;
	union reg_sensor_phy_top_130            reg_130;
	union reg_sensor_phy_top_134            reg_134;
	union reg_sensor_phy_top_138            reg_138;
	union reg_sensor_phy_top_13c            reg_13c;
	union reg_sensor_phy_top_140            reg_140;
	union reg_sensor_phy_top_144            reg_144;
	union reg_sensor_phy_top_148            reg_148;
	union reg_sensor_phy_top_14c            reg_14c;
	union reg_sensor_phy_top_150            reg_150;
	union reg_sensor_phy_top_154            reg_154;
	union reg_sensor_phy_top_158            reg_158;
	union reg_sensor_phy_top_15c            reg_15c;
	union reg_sensor_phy_top_160            reg_160;
	union reg_sensor_phy_top_164            reg_164;
	union reg_sensor_phy_top_168            reg_168;
	union reg_sensor_phy_top_16c            reg_16c;
	union reg_sensor_phy_top_170            reg_170;
	union reg_sensor_phy_top_174            reg_174;
	union reg_sensor_phy_top_178            reg_178;
	union reg_sensor_phy_top_17c            reg_17c;
	union reg_sensor_phy_top_dft_180        dft_180;
	union reg_sensor_phy_top_dft_184        dft_184;
	union reg_sensor_phy_top_dft_188        dft_188;
	union reg_sensor_phy_top_dft_18c        dft_18c;
	union reg_sensor_phy_top_dft_190        dft_190;
	uint32_t                                _resv_0x194[23];
	union reg_sensor_phy_top_dbg_1f0        dbg_1f0;
	uint32_t                                _resv_0x1f4[3];
	union reg_sensor_phy_top_200            reg_200;
	union reg_sensor_phy_top_204            reg_204;
	union reg_sensor_phy_top_208            reg_208;
	union reg_sensor_phy_top_20c            reg_20c;
	union reg_sensor_phy_top_210            reg_210;
	union reg_sensor_phy_top_214            reg_214;
	union reg_sensor_phy_top_218            reg_218;
	union reg_sensor_phy_top_21c            reg_21c;
	union reg_sensor_phy_top_220            reg_220;
	union reg_sensor_phy_top_224            reg_224;
	union reg_sensor_phy_top_228            reg_228;
	union reg_sensor_phy_top_22c            reg_22c;
	union reg_sensor_phy_top_230            reg_230;
	union reg_sensor_phy_top_234            reg_234;
	union reg_sensor_phy_top_238            reg_238;
	union reg_sensor_phy_top_23c            reg_23c;
	union reg_sensor_phy_top_240            reg_240;
	union reg_sensor_phy_top_244            reg_244;
	union reg_sensor_phy_top_248            reg_248;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_phy_8l_t {
	union reg_sensor_phy_8l_00              reg_00;
	union reg_sensor_phy_8l_04              reg_04;
	union reg_sensor_phy_8l_08              reg_08;
	union reg_sensor_phy_8l_0c              reg_0c;
	union reg_sensor_phy_8l_10              reg_10;
	union reg_sensor_phy_8l_14              reg_14;
	uint32_t                                _resv_0x18[2];
	union reg_sensor_phy_8l_20              reg_20;
	union reg_sensor_phy_8l_24              reg_24;
	union reg_sensor_phy_8l_28              reg_28;
	uint32_t                                _resv_0x2c[1];
	union reg_sensor_phy_8l_30              reg_30;
	union reg_sensor_phy_8l_34              reg_34;
	union reg_sensor_phy_8l_38              reg_38;
	union reg_sensor_phy_8l_3c              reg_3c;
	union reg_sensor_phy_8l_40              reg_40;
	union reg_sensor_phy_8l_44              reg_44;
	union reg_sensor_phy_8l_48              reg_48;
	union reg_sensor_phy_8l_4c              reg_4c;
	union reg_sensor_phy_8l_50              reg_50;
	union reg_sensor_phy_8l_54              reg_54;
	union reg_sensor_phy_8l_58              reg_58;
	union reg_sensor_phy_8l_5c              reg_5c;
	union reg_sensor_phy_8l_60              reg_60;
	union reg_sensor_phy_8l_64              reg_64;
	union reg_sensor_phy_8l_68              reg_68;
	union reg_sensor_phy_8l_6c              reg_6c;
	union reg_sensor_phy_8l_70              reg_70;
	union reg_sensor_phy_8l_74              reg_74;
	union reg_sensor_phy_8l_78              reg_78;
	union reg_sensor_phy_8l_7c              reg_7c;
	union reg_sensor_phy_8l_80              reg_80;
	union reg_sensor_phy_8l_84              reg_84;
	uint32_t                                _resv_0x88[2];
	union reg_sensor_phy_8l_dbg_90          dbg_90;
	union reg_sensor_phy_8l_dbg_94          dbg_94;
	union reg_sensor_phy_8l_status_98       status_98;
	union reg_sensor_phy_8l_status_9c       status_9c;
	union reg_sensor_phy_8l_status_a0       status_a0;
	union reg_sensor_phy_8l_status_a4       status_a4;
	union reg_sensor_phy_8l_status_a8       status_a8;
	union reg_sensor_phy_8l_status_ac       status_ac;
	uint32_t                                _resv_0xb0[20];
	union reg_sensor_phy_8l_d0_0            d0_0;
	union reg_sensor_phy_8l_d0_1            d0_1;
	union reg_sensor_phy_8l_d0_2            d0_2;
	union reg_sensor_phy_8l_d0_3            d0_3;
	union reg_sensor_phy_8l_d0_4            d0_4;
	union reg_sensor_phy_8l_d0_5            d0_5;
	union reg_sensor_phy_8l_d0_6            d0_6;
	union reg_sensor_phy_8l_d0_7            d0_7;
	union reg_sensor_phy_8l_d0_8            d0_8;
	union reg_sensor_phy_8l_d0_9            d0_9;
	union reg_sensor_phy_8l_d0_a            d0_a;
	union reg_sensor_phy_8l_d0_b            d0_b;
	uint32_t                                _resv_0x130[4];
	union reg_sensor_phy_8l_d1_0            d1_0;
	union reg_sensor_phy_8l_d1_1            d1_1;
	union reg_sensor_phy_8l_d1_2            d1_2;
	union reg_sensor_phy_8l_d1_3            d1_3;
	union reg_sensor_phy_8l_d1_4            d1_4;
	union reg_sensor_phy_8l_d1_5            d1_5;
	union reg_sensor_phy_8l_d1_6            d1_6;
	union reg_sensor_phy_8l_d1_7            d1_7;
	union reg_sensor_phy_8l_d1_8            d1_8;
	union reg_sensor_phy_8l_d1_9            d1_9;
	union reg_sensor_phy_8l_d1_a            d1_a;
	union reg_sensor_phy_8l_d1_b            d1_b;
	uint32_t                                _resv_0x170[4];
	union reg_sensor_phy_8l_d2_0            d2_0;
	union reg_sensor_phy_8l_d2_1            d2_1;
	union reg_sensor_phy_8l_d2_2            d2_2;
	union reg_sensor_phy_8l_d2_3            d2_3;
	union reg_sensor_phy_8l_d2_4            d2_4;
	union reg_sensor_phy_8l_d2_5            d2_5;
	union reg_sensor_phy_8l_d2_6            d2_6;
	union reg_sensor_phy_8l_d2_7            d2_7;
	union reg_sensor_phy_8l_d2_8            d2_8;
	union reg_sensor_phy_8l_d2_9            d2_9;
	union reg_sensor_phy_8l_d2_a            d2_a;
	union reg_sensor_phy_8l_d2_b            d2_b;
	uint32_t                                _resv_0x1b0[4];
	union reg_sensor_phy_8l_d3_0            d3_0;
	union reg_sensor_phy_8l_d3_1            d3_1;
	union reg_sensor_phy_8l_d3_2            d3_2;
	union reg_sensor_phy_8l_d3_3            d3_3;
	union reg_sensor_phy_8l_d3_4            d3_4;
	union reg_sensor_phy_8l_d3_5            d3_5;
	union reg_sensor_phy_8l_d3_6            d3_6;
	union reg_sensor_phy_8l_d3_7            d3_7;
	union reg_sensor_phy_8l_d3_8            d3_8;
	union reg_sensor_phy_8l_d3_9            d3_9;
	union reg_sensor_phy_8l_d3_a            d3_a;
	union reg_sensor_phy_8l_d3_b            d3_b;
	uint32_t                                _resv_0x1f0[4];
	union reg_sensor_phy_8l_d4_0            d4_0;
	union reg_sensor_phy_8l_d4_1            d4_1;
	union reg_sensor_phy_8l_d4_2            d4_2;
	union reg_sensor_phy_8l_d4_3            d4_3;
	union reg_sensor_phy_8l_d4_4            d4_4;
	union reg_sensor_phy_8l_d4_5            d4_5;
	union reg_sensor_phy_8l_d4_6            d4_6;
	union reg_sensor_phy_8l_d4_7            d4_7;
	union reg_sensor_phy_8l_d4_8            d4_8;
	union reg_sensor_phy_8l_d4_9            d4_9;
	union reg_sensor_phy_8l_d4_a            d4_a;
	union reg_sensor_phy_8l_d4_b            d4_b;
	uint32_t                                _resv_0x230[4];
	union reg_sensor_phy_8l_d5_0            d5_0;
	union reg_sensor_phy_8l_d5_1            d5_1;
	union reg_sensor_phy_8l_d5_2            d5_2;
	union reg_sensor_phy_8l_d5_3            d5_3;
	union reg_sensor_phy_8l_d5_4            d5_4;
	union reg_sensor_phy_8l_d5_5            d5_5;
	union reg_sensor_phy_8l_d5_6            d5_6;
	union reg_sensor_phy_8l_d5_7            d5_7;
	union reg_sensor_phy_8l_d5_8            d5_8;
	union reg_sensor_phy_8l_d5_9            d5_9;
	union reg_sensor_phy_8l_d5_a            d5_a;
	union reg_sensor_phy_8l_d5_b            d5_b;
	uint32_t                                _resv_0x270[4];
	union reg_sensor_phy_8l_d6_0            d6_0;
	union reg_sensor_phy_8l_d6_1            d6_1;
	union reg_sensor_phy_8l_d6_2            d6_2;
	union reg_sensor_phy_8l_d6_3            d6_3;
	union reg_sensor_phy_8l_d6_4            d6_4;
	union reg_sensor_phy_8l_d6_5            d6_5;
	union reg_sensor_phy_8l_d6_6            d6_6;
	union reg_sensor_phy_8l_d6_7            d6_7;
	union reg_sensor_phy_8l_d6_8            d6_8;
	union reg_sensor_phy_8l_d6_9            d6_9;
	union reg_sensor_phy_8l_d6_a            d6_a;
	union reg_sensor_phy_8l_d6_b            d6_b;
	uint32_t                                _resv_0x2b0[4];
	union reg_sensor_phy_8l_d7_0            d7_0;
	union reg_sensor_phy_8l_d7_1            d7_1;
	union reg_sensor_phy_8l_d7_2            d7_2;
	union reg_sensor_phy_8l_d7_3            d7_3;
	union reg_sensor_phy_8l_d7_4            d7_4;
	union reg_sensor_phy_8l_d7_5            d7_5;
	union reg_sensor_phy_8l_d7_6            d7_6;
	union reg_sensor_phy_8l_d7_7            d7_7;
	union reg_sensor_phy_8l_d7_8            d7_8;
	union reg_sensor_phy_8l_d7_9            d7_9;
	union reg_sensor_phy_8l_d7_a            d7_a;
	union reg_sensor_phy_8l_d7_b            d7_b;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_phy_4l_t {
	union reg_sensor_phy_4l_00              reg_00;
	union reg_sensor_phy_4l_04              reg_04;
	union reg_sensor_phy_4l_08              reg_08;
	union reg_sensor_phy_4l_0c              reg_0c;
	union reg_sensor_phy_4l_10              reg_10;
	uint32_t                                _resv_0x14[3];
	union reg_sensor_phy_4l_20              reg_20;
	union reg_sensor_phy_4l_24              reg_24;
	union reg_sensor_phy_4l_28              reg_28;
	uint32_t                                _resv_0x2c[1];
	union reg_sensor_phy_4l_30              reg_30;
	union reg_sensor_phy_4l_34              reg_34;
	union reg_sensor_phy_4l_38              reg_38;
	union reg_sensor_phy_4l_3c              reg_3c;
	union reg_sensor_phy_4l_40              reg_40;
	union reg_sensor_phy_4l_44              reg_44;
	union reg_sensor_phy_4l_48              reg_48;
	union reg_sensor_phy_4l_4c              reg_4c;
	union reg_sensor_phy_4l_50              reg_50;
	union reg_sensor_phy_4l_54              reg_54;
	union reg_sensor_phy_4l_58              reg_58;
	union reg_sensor_phy_4l_5c              reg_5c;
	union reg_sensor_phy_4l_60              reg_60;
	union reg_sensor_phy_4l_64              reg_64;
	uint32_t                                _resv_0x68[10];
	union reg_sensor_phy_4l_dbg_90          dbg_90;
	union reg_sensor_phy_4l_dbg_94          dbg_94;
	union reg_sensor_phy_4l_status_98       status_98;
	union reg_sensor_phy_4l_status_9c       status_9c;
	uint32_t                                _resv_0xa0[1];
	union reg_sensor_phy_4l_status_a4       status_a4;
	union reg_sensor_phy_4l_status_a8       status_a8;
	uint32_t                                _resv_0xac[21];
	union reg_sensor_phy_4l_d0_0            d0_0;
	union reg_sensor_phy_4l_d0_1            d0_1;
	union reg_sensor_phy_4l_d0_2            d0_2;
	union reg_sensor_phy_4l_d0_3            d0_3;
	union reg_sensor_phy_4l_d0_4            d0_4;
	union reg_sensor_phy_4l_d0_5            d0_5;
	union reg_sensor_phy_4l_d0_6            d0_6;
	union reg_sensor_phy_4l_d0_7            d0_7;
	union reg_sensor_phy_4l_d0_8            d0_8;
	union reg_sensor_phy_4l_d0_9            d0_9;
	union reg_sensor_phy_4l_d0_a            d0_a;
	union reg_sensor_phy_4l_d0_b            d0_b;
	uint32_t                                _resv_0x130[4];
	union reg_sensor_phy_4l_d1_0            d1_0;
	union reg_sensor_phy_4l_d1_1            d1_1;
	union reg_sensor_phy_4l_d1_2            d1_2;
	union reg_sensor_phy_4l_d1_3            d1_3;
	union reg_sensor_phy_4l_d1_4            d1_4;
	union reg_sensor_phy_4l_d1_5            d1_5;
	union reg_sensor_phy_4l_d1_6            d1_6;
	union reg_sensor_phy_4l_d1_7            d1_7;
	union reg_sensor_phy_4l_d1_8            d1_8;
	union reg_sensor_phy_4l_d1_9            d1_9;
	union reg_sensor_phy_4l_d1_a            d1_a;
	union reg_sensor_phy_4l_d1_b            d1_b;
	uint32_t                                _resv_0x170[4];
	union reg_sensor_phy_4l_d2_0            d2_0;
	union reg_sensor_phy_4l_d2_1            d2_1;
	union reg_sensor_phy_4l_d2_2            d2_2;
	union reg_sensor_phy_4l_d2_3            d2_3;
	union reg_sensor_phy_4l_d2_4            d2_4;
	union reg_sensor_phy_4l_d2_5            d2_5;
	union reg_sensor_phy_4l_d2_6            d2_6;
	union reg_sensor_phy_4l_d2_7            d2_7;
	union reg_sensor_phy_4l_d2_8            d2_8;
	union reg_sensor_phy_4l_d2_9            d2_9;
	union reg_sensor_phy_4l_d2_a            d2_a;
	union reg_sensor_phy_4l_d2_b            d2_b;
	uint32_t                                _resv_0x1b0[4];
	union reg_sensor_phy_4l_d3_0            d3_0;
	union reg_sensor_phy_4l_d3_1            d3_1;
	union reg_sensor_phy_4l_d3_2            d3_2;
	union reg_sensor_phy_4l_d3_3            d3_3;
	union reg_sensor_phy_4l_d3_4            d3_4;
	union reg_sensor_phy_4l_d3_5            d3_5;
	union reg_sensor_phy_4l_d3_6            d3_6;
	union reg_sensor_phy_4l_d3_7            d3_7;
	union reg_sensor_phy_4l_d3_8            d3_8;
	union reg_sensor_phy_4l_d3_9            d3_9;
	union reg_sensor_phy_4l_d3_a            d3_a;
	union reg_sensor_phy_4l_d3_b            d3_b;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sensor_phy_2l_t {
	union reg_sensor_phy_2l_00              reg_00;
	union reg_sensor_phy_2l_04              reg_04;
	union reg_sensor_phy_2l_08              reg_08;
	union reg_sensor_phy_2l_0c              reg_0c;
	union reg_sensor_phy_2l_10              reg_10;
	uint32_t                                _resv_0x14[3];
	union reg_sensor_phy_2l_20              reg_20;
	union reg_sensor_phy_2l_24              reg_24;
	union reg_sensor_phy_2l_28              reg_28;
	uint32_t                                _resv_0x2c[1];
	union reg_sensor_phy_2l_30              reg_30;
	union reg_sensor_phy_2l_34              reg_34;
	union reg_sensor_phy_2l_38              reg_38;
	union reg_sensor_phy_2l_3c              reg_3c;
	union reg_sensor_phy_2l_40              reg_40;
	union reg_sensor_phy_2l_44              reg_44;
	union reg_sensor_phy_2l_48              reg_48;
	union reg_sensor_phy_2l_4c              reg_4c;
	union reg_sensor_phy_2l_50              reg_50;
	union reg_sensor_phy_2l_54              reg_54;
	uint32_t                                _resv_0x58[14];
	union reg_sensor_phy_2l_dbg_90          dbg_90;
	union reg_sensor_phy_2l_dbg_94          dbg_94;
	union reg_sensor_phy_2l_status_98       status_98;
	union reg_sensor_phy_2l_status_9c       status_9c;
	uint32_t                                _resv_0xa0[24];
	union reg_sensor_phy_2l_d0_0            d0_0;
	union reg_sensor_phy_2l_d0_1            d0_1;
	union reg_sensor_phy_2l_d0_2            d0_2;
	union reg_sensor_phy_2l_d0_3            d0_3;
	union reg_sensor_phy_2l_d0_4            d0_4;
	union reg_sensor_phy_2l_d0_5            d0_5;
	union reg_sensor_phy_2l_d0_6            d0_6;
	union reg_sensor_phy_2l_d0_7            d0_7;
	union reg_sensor_phy_2l_d0_8            d0_8;
	union reg_sensor_phy_2l_d0_9            d0_9;
	union reg_sensor_phy_2l_d0_a            d0_a;
	union reg_sensor_phy_2l_d0_b            d0_b;
	uint32_t                                _resv_0x130[4];
	union reg_sensor_phy_2l_d1_0            d1_0;
	union reg_sensor_phy_2l_d1_1            d1_1;
	union reg_sensor_phy_2l_d1_2            d1_2;
	union reg_sensor_phy_2l_d1_3            d1_3;
	union reg_sensor_phy_2l_d1_4            d1_4;
	union reg_sensor_phy_2l_d1_5            d1_5;
	union reg_sensor_phy_2l_d1_6            d1_6;
	union reg_sensor_phy_2l_d1_7            d1_7;
	union reg_sensor_phy_2l_d1_8            d1_8;
	union reg_sensor_phy_2l_d1_9            d1_9;
	union reg_sensor_phy_2l_d1_a            d1_a;
	union reg_sensor_phy_2l_d1_b            d1_b;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_BLOCKS_CSI_WRAP_H_ */
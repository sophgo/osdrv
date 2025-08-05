/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name:vi_reg_blocks.h
 * Description:HW register description
 */

#ifndef _VI_REG_BLOCKS_H_
#define _VI_REG_BLOCKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vi_reg_fields.h"

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_ae_hist_t {
	union reg_isp_ae_hist_ae_hist_status    ae_hist_status;
	union reg_isp_ae_hist_ae_hist_grace_reset  ae_hist_grace_reset;
	union reg_isp_ae_hist_ae_hist_monitor   ae_hist_monitor;
	union reg_isp_ae_hist_ae_hist_bypass    ae_hist_bypass;
	union reg_isp_ae_hist_ae_kickoff        ae_kickoff;
	union reg_isp_ae_hist_sts_ae0_hist_enable  sts_ae0_hist_enable;
	union reg_isp_ae_hist_sts_ae_offsetx    sts_ae_offsetx;
	union reg_isp_ae_hist_sts_ae_offsety    sts_ae_offsety;
	union reg_isp_ae_hist_sts_ae_numxm1     sts_ae_numxm1;
	union reg_isp_ae_hist_sts_ae_numym1     sts_ae_numym1;
	union reg_isp_ae_hist_sts_ae_width      sts_ae_width;
	union reg_isp_ae_hist_sts_ae_height     sts_ae_height;
	union reg_isp_ae_hist_sts_ae_sts_div    sts_ae_sts_div;
	union reg_isp_ae_hist_sts_hist_mode     sts_hist_mode;
	union reg_isp_ae_hist_shdw_read_sel     shdw_read_sel;
	union reg_isp_ae_hist_ae_hist_monitor_select  ae_hist_monitor_select;
	union reg_isp_ae_hist_ae_hist_location  ae_hist_location;
	union reg_isp_ae_hist_hw_auto_cg_en     hw_auto_cg_en;
	union reg_isp_ae_hist_sts_ir_ae_offsetx  sts_ir_ae_offsetx;
	union reg_isp_ae_hist_sts_ir_ae_offsety  sts_ir_ae_offsety;
	union reg_isp_ae_hist_sts_ir_ae_numxm1  sts_ir_ae_numxm1;
	union reg_isp_ae_hist_sts_ir_ae_numym1  sts_ir_ae_numym1;
	union reg_isp_ae_hist_sts_ir_ae_width   sts_ir_ae_width;
	union reg_isp_ae_hist_sts_ir_ae_height  sts_ir_ae_height;
	union reg_isp_ae_hist_sts_ir_ae_sts_div  sts_ir_ae_sts_div;
	uint32_t                                _resv_0x64[1];
	union reg_isp_ae_hist_ae_hist_bayer_starting  ae_hist_bayer_starting;
	union reg_isp_ae_hist_ae_hist_dummy     ae_hist_dummy;
	union reg_isp_ae_hist_ae_hist_checksum  ae_hist_checksum;
	union reg_isp_ae_hist_wbg_4             wbg_4;
	union reg_isp_ae_hist_wbg_5             wbg_5;
	union reg_isp_ae_hist_wbg_6             wbg_6;
	union reg_isp_ae_hist_wbg_7             wbg_7;
	uint32_t                                _resv_0x84[7];
	union reg_isp_ae_hist_dmi_enable        dmi_enable;
	uint32_t                                _resv_0xa4[3];
	union reg_isp_ae_hist_ae_face0_location  ae_face0_location;
	union reg_isp_ae_hist_ae_face1_location  ae_face1_location;
	union reg_isp_ae_hist_ae_face2_location  ae_face2_location;
	union reg_isp_ae_hist_ae_face3_location  ae_face3_location;
	union reg_isp_ae_hist_ae_face0_size     ae_face0_size;
	union reg_isp_ae_hist_ae_face1_size     ae_face1_size;
	union reg_isp_ae_hist_ae_face2_size     ae_face2_size;
	union reg_isp_ae_hist_ae_face3_size     ae_face3_size;
	union reg_isp_ae_hist_ir_ae_face0_location  ir_ae_face0_location;
	union reg_isp_ae_hist_ir_ae_face1_location  ir_ae_face1_location;
	union reg_isp_ae_hist_ir_ae_face2_location  ir_ae_face2_location;
	union reg_isp_ae_hist_ir_ae_face3_location  ir_ae_face3_location;
	union reg_isp_ae_hist_ir_ae_face0_size  ir_ae_face0_size;
	union reg_isp_ae_hist_ir_ae_face1_size  ir_ae_face1_size;
	union reg_isp_ae_hist_ir_ae_face2_size  ir_ae_face2_size;
	union reg_isp_ae_hist_ir_ae_face3_size  ir_ae_face3_size;
	union reg_isp_ae_hist_ae_face_enable_ctrl  ae_face_enable_ctrl;
	union reg_isp_ae_hist_ae_face0_sts_div  ae_face0_sts_div;
	union reg_isp_ae_hist_ae_face1_sts_div  ae_face1_sts_div;
	union reg_isp_ae_hist_ae_face2_sts_div  ae_face2_sts_div;
	union reg_isp_ae_hist_ae_face3_sts_div  ae_face3_sts_div;
	union reg_isp_ae_hist_sts_enable        sts_enable;
	union reg_isp_ae_hist_ae_algo_enable    ae_algo_enable;
	union reg_isp_ae_hist_ae_hist_low       ae_hist_low;
	union reg_isp_ae_hist_ae_hist_high      ae_hist_high;
	union reg_isp_ae_hist_ae_top            ae_top;
	union reg_isp_ae_hist_ae_bot            ae_bot;
	union reg_isp_ae_hist_ae_overexp_thr    ae_overexp_thr;
	union reg_isp_ae_hist_ae_num_gapline    ae_num_gapline;
	union reg_isp_ae_hist_ae_choose_gr_en   ae_choose_gr_en;
	uint32_t                                _resv_0x128[2];
	union reg_isp_ae_hist_se_ae_blc_offset_r  se_ae_blc_offset_r;
	union reg_isp_ae_hist_se_ae_blc_offset_gr  se_ae_blc_offset_gr;
	union reg_isp_ae_hist_se_ae_blc_offset_gb  se_ae_blc_offset_gb;
	union reg_isp_ae_hist_se_ae_blc_offset_b  se_ae_blc_offset_b;
	union reg_isp_ae_hist_se_ae_blc_offset_ir  se_ae_blc_offset_ir;
	union reg_isp_ae_hist_se_ae_blc_gain_r  se_ae_blc_gain_r;
	union reg_isp_ae_hist_se_ae_blc_gain_gr  se_ae_blc_gain_gr;
	union reg_isp_ae_hist_se_ae_blc_gain_gb  se_ae_blc_gain_gb;
	union reg_isp_ae_hist_se_ae_blc_gain_b  se_ae_blc_gain_b;
	union reg_isp_ae_hist_se_ae_blc_gain_ir  se_ae_blc_gain_ir;
	union reg_isp_ae_hist_se_ae_blc_enable  se_ae_blc_enable;
	uint32_t                                _resv_0x15c[1];
	union reg_isp_ae_hist_frame_counter_awb  frame_counter_awb;
	uint32_t                                _resv_0x164[1];
	union reg_isp_ae_hist_ae_simple2a_result_luma_se  ae_simple2a_result_luma_se;
	union reg_isp_ae_hist_ae_simple2a_result_rgain_se  ae_simple2a_result_rgain_se;
	union reg_isp_ae_hist_ae_simple2a_result_bgain_se  ae_simple2a_result_bgain_se;
	union reg_isp_ae_hist_ae_simple2a_result_luma_le  ae_simple2a_result_luma_le;
	union reg_isp_ae_hist_ae_simple2a_result_rgain_le  ae_simple2a_result_rgain_le;
	union reg_isp_ae_hist_ae_simple2a_result_bgain_le  ae_simple2a_result_bgain_le;
	union reg_isp_ae_hist_ae_face0_result_se  ae_face0_result_se;
	union reg_isp_ae_hist_ae_face1_result_se  ae_face1_result_se;
	union reg_isp_ae_hist_ae_face2_result_se  ae_face2_result_se;
	union reg_isp_ae_hist_ae_face3_result_se  ae_face3_result_se;
	union reg_isp_ae_hist_ae_face0_result_le  ae_face0_result_le;
	union reg_isp_ae_hist_ae_face1_result_le  ae_face1_result_le;
	union reg_isp_ae_hist_ae_face2_result_le  ae_face2_result_le;
	union reg_isp_ae_hist_ae_face3_result_le  ae_face3_result_le;
	uint32_t                                _resv_0x1a0[24];
	union reg_isp_ae_hist_ae_wgt_00         ae_wgt_00;
	union reg_isp_ae_hist_ae_wgt_01         ae_wgt_01;
	union reg_isp_ae_hist_ae_wgt_02         ae_wgt_02;
	union reg_isp_ae_hist_ae_wgt_03         ae_wgt_03;
	union reg_isp_ae_hist_ae_wgt_04         ae_wgt_04;
	union reg_isp_ae_hist_ae_wgt_05         ae_wgt_05;
	union reg_isp_ae_hist_ae_wgt_06         ae_wgt_06;
	union reg_isp_ae_hist_ae_wgt_07         ae_wgt_07;
	union reg_isp_ae_hist_ae_wgt_08         ae_wgt_08;
	union reg_isp_ae_hist_ae_wgt_09         ae_wgt_09;
	union reg_isp_ae_hist_ae_wgt_10         ae_wgt_10;
	union reg_isp_ae_hist_ae_wgt_11         ae_wgt_11;
	union reg_isp_ae_hist_ae_wgt_12         ae_wgt_12;
	union reg_isp_ae_hist_ae_wgt_13         ae_wgt_13;
	union reg_isp_ae_hist_ae_wgt_14         ae_wgt_14;
	union reg_isp_ae_hist_ae_wgt_15         ae_wgt_15;
	union reg_isp_ae_hist_ae_wgt_16         ae_wgt_16;
	union reg_isp_ae_hist_ae_wgt_17         ae_wgt_17;
	union reg_isp_ae_hist_ae_wgt_18         ae_wgt_18;
	union reg_isp_ae_hist_ae_wgt_19         ae_wgt_19;
	union reg_isp_ae_hist_ae_wgt_20         ae_wgt_20;
	union reg_isp_ae_hist_ae_wgt_21         ae_wgt_21;
	union reg_isp_ae_hist_ae_wgt_22         ae_wgt_22;
	union reg_isp_ae_hist_ae_wgt_23         ae_wgt_23;
	union reg_isp_ae_hist_ae_wgt_24         ae_wgt_24;
	union reg_isp_ae_hist_ae_wgt_25         ae_wgt_25;
	union reg_isp_ae_hist_ae_wgt_26         ae_wgt_26;
	union reg_isp_ae_hist_ae_wgt_27         ae_wgt_27;
	union reg_isp_ae_hist_ae_wgt_28         ae_wgt_28;
	union reg_isp_ae_hist_ae_wgt_29         ae_wgt_29;
	union reg_isp_ae_hist_ae_wgt_30         ae_wgt_30;
	union reg_isp_ae_hist_ae_wgt_31         ae_wgt_31;
	union reg_isp_ae_hist_le_ae_blc_offset_r  le_ae_blc_offset_r;
	union reg_isp_ae_hist_le_ae_blc_offset_gr  le_ae_blc_offset_gr;
	union reg_isp_ae_hist_le_ae_blc_offset_gb  le_ae_blc_offset_gb;
	union reg_isp_ae_hist_le_ae_blc_offset_b  le_ae_blc_offset_b;
	union reg_isp_ae_hist_le_ae_blc_offset_ir  le_ae_blc_offset_ir;
	union reg_isp_ae_hist_le_ae_blc_gain_r  le_ae_blc_gain_r;
	union reg_isp_ae_hist_le_ae_blc_gain_gr  le_ae_blc_gain_gr;
	union reg_isp_ae_hist_le_ae_blc_gain_gb  le_ae_blc_gain_gb;
	union reg_isp_ae_hist_le_ae_blc_gain_b  le_ae_blc_gain_b;
	union reg_isp_ae_hist_le_ae_blc_gain_ir  le_ae_blc_gain_ir;
	union reg_isp_ae_hist_le_ae_blc_enable  le_ae_blc_enable;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_af_t {
	union reg_isp_af_status              status;
	union reg_isp_af_grace_reset         grace_reset;
	union reg_isp_af_monitor             monitor;
	union reg_isp_af_bypass              bypass;
	union reg_isp_af_kickoff             kickoff;
	union reg_isp_af_enables                enables;
	union reg_isp_af_offset_x            offset_x;
	union reg_isp_af_mxn_image_width_m1  mxn_image_width_m1;
	union reg_isp_af_block_width         block_width;
	union reg_isp_af_block_height        block_height;
	union reg_isp_af_block_num_x         block_num_x;
	union reg_isp_af_block_num_y         block_num_y;
	uint32_t                                _resv_0x30[1];
	union reg_isp_af_hor_low_pass_value_shift  hor_low_pass_value_shift;
	union reg_isp_af_corning_offset_horizontal_0  offset_horizontal_0;
	union reg_isp_af_corning_offset_horizontal_1  offset_horizontal_1;
	union reg_isp_af_corning_offset_vertical  offset_vertical;
	union reg_isp_af_high_y_thre         high_y_thre;
	union reg_isp_af_low_pass_horizon    low_pass_horizon;
	union reg_isp_af_location            location;
	union reg_isp_af_high_pass_horizon_0  high_pass_horizon_0;
	union reg_isp_af_high_pass_horizon_1  high_pass_horizon_1;
	union reg_isp_af_high_pass_vertical_0  high_pass_vertical_0;
	union reg_isp_af_mem_sw_mode         sw_mode;
	union reg_isp_af_monitor_select      monitor_select;
	uint32_t                                _resv_0x64[2];
	union reg_isp_af_image_width         image_width;
	union reg_isp_af_dummy               dummy;
	union reg_isp_af_mem_sw_raddr        sw_raddr;
	union reg_isp_af_mem_sw_rdata        sw_rdata;
	union reg_isp_af_mxn_border          mxn_border;
	union reg_isp_af_th_low              th_low;
	union reg_isp_af_gain_low            gain_low;
	union reg_isp_af_slop_low            slop_low;
	uint32_t                                _resv_0x8c[5];
	union reg_isp_af_dmi_enable             dmi_enable;
	uint32_t                                _resv_0xa4[45];
	union reg_isp_af_square_enable       square_enable;
	uint32_t                                _resv_0x15c[2];
	union reg_isp_af_outshift            outshift;
	uint32_t                                _resv_0x168[1];
	union reg_isp_af_num_gapline         num_gapline;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_blc_dg_wb_t {
	union reg_blc_dg_wb_base_config         base_config;
	union reg_blc_dg_wb_wbg_le_gain0        wbg_le_gain0;
	union reg_blc_dg_wb_wbg_le_gain1        wbg_le_gain1;
	union reg_blc_dg_wb_blc_le_offset0      blc_le_offset0;
	union reg_blc_dg_wb_blc_le_offset1      blc_le_offset1;
	union reg_blc_dg_wb_blc_le_offset_2nd0  blc_le_offset_2nd0;
	union reg_blc_dg_wb_blc_le_offset_2nd1  blc_le_offset_2nd1;
	union reg_blc_dg_wb_blc_le_gain0        blc_le_gain0;
	union reg_blc_dg_wb_blc_le_gain1        blc_le_gain1;
	union reg_blc_dg_wb_blc_le_normgain0    blc_le_normgain0;
	union reg_blc_dg_wb_blc_le_normgain1    blc_le_normgain1;
	union reg_blc_dg_wb_wbg_se_gain0        wbg_se_gain0;
	union reg_blc_dg_wb_wbg_se_gain1        wbg_se_gain1;
	union reg_blc_dg_wb_blc_se_offset0      blc_se_offset0;
	union reg_blc_dg_wb_blc_se_offset1      blc_se_offset1;
	union reg_blc_dg_wb_blc_se_offset_2nd0  blc_se_offset_2nd0;
	union reg_blc_dg_wb_blc_se_offset_2nd1  blc_se_offset_2nd1;
	union reg_blc_dg_wb_blc_se_gain0        blc_se_gain0;
	union reg_blc_dg_wb_blc_se_gain1        blc_se_gain1;
	union reg_blc_dg_wb_blc_se_normgain0    blc_se_normgain0;
	union reg_blc_dg_wb_blc_se_normgain1    blc_se_normgain1;
	union reg_blc_dg_wb_shdw_read_sel       shdw_read_sel;
	union reg_blc_dg_wb_auto_cg_en          auto_cg_en;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_bnr_t {
	union reg_isp_bnr_00                    bnr_00;
	union reg_isp_bnr_04                    bnr_04;
	uint32_t                                _resv_0x8[2];
	union reg_isp_bnr_10                    bnr_10;
	union reg_isp_bnr_14                    bnr_14;
	union reg_isp_bnr_18                    bnr_18;
	union reg_isp_bnr_1c                    bnr_1c;
	union reg_isp_bnr_20                    bnr_20;
	union reg_isp_bnr_24                    bnr_24;
	union reg_isp_bnr_28                    bnr_28;
	union reg_isp_bnr_2c                    bnr_2c;
	union reg_isp_bnr_30                    bnr_30;
	union reg_isp_bnr_34                    bnr_34;
	union reg_isp_bnr_38                    bnr_38;
	union reg_isp_bnr_3c                    bnr_3c;
	union reg_isp_bnr_40                    bnr_40;
	union reg_isp_bnr_44                    bnr_44;
	union reg_isp_bnr_48                    bnr_48;
	union reg_isp_bnr_4c                    bnr_4c;
	union reg_isp_bnr_50                    bnr_50;
	union reg_isp_bnr_54                    bnr_54;
	union reg_isp_bnr_58                    bnr_58;
	union reg_isp_bnr_5c                    bnr_5c;
	union reg_isp_bnr_60                    bnr_60;
	union reg_isp_bnr_64                    bnr_64;
	union reg_isp_bnr_68                    bnr_68;
	union reg_isp_bnr_6c                    bnr_6c;
	union reg_isp_bnr_70                    bnr_70;
	union reg_isp_bnr_74                    bnr_74;
	union reg_isp_bnr_78                    bnr_78;
	union reg_isp_bnr_7c                    bnr_7c;
	union reg_isp_bnr_80                    bnr_80;
	union reg_isp_bnr_84                    bnr_84;
	union reg_isp_bnr_88                    bnr_88;
	union reg_isp_bnr_8c                    bnr_8c;
	union reg_isp_bnr_90                    bnr_90;
	union reg_isp_bnr_94                    bnr_94;
	union reg_isp_bnr_98                    bnr_98;
	union reg_isp_bnr_9c                    bnr_9c;
	union reg_isp_bnr_a0                    bnr_a0;
	union reg_isp_bnr_a4                    bnr_a4;
	union reg_isp_bnr_a8                    bnr_a8;
	union reg_isp_bnr_ac                    bnr_ac;
	union reg_isp_bnr_b0                    bnr_b0;
	union reg_isp_bnr_b4                    bnr_b4;
	union reg_isp_bnr_b8                    bnr_b8;
	union reg_isp_bnr_bc                    bnr_bc;
	union reg_isp_bnr_c0                    bnr_c0;
	union reg_isp_bnr_c4                    bnr_c4;
	union reg_isp_bnr_c8                    bnr_c8;
	union reg_isp_bnr_cc                    bnr_cc;
	union reg_isp_bnr_d0                    bnr_d0;
	union reg_isp_bnr_d4                    bnr_d4;
	union reg_isp_bnr_d8                    bnr_d8;
	union reg_isp_bnr_dc                    bnr_dc;
	union reg_isp_bnr_e0                    bnr_e0;
	union reg_isp_bnr_e4                    bnr_e4;
	union reg_isp_bnr_e8                    bnr_e8;
	union reg_isp_bnr_ec                    bnr_ec;
	union reg_isp_bnr_f0                    bnr_f0;
	union reg_isp_bnr_f4                    bnr_f4;
	union reg_isp_bnr_f8                    bnr_f8;
	union reg_isp_bnr_fc                    bnr_fc;
	union reg_isp_bnr_100                   bnr_100;
	union reg_isp_bnr_104                   bnr_104;
	union reg_isp_bnr_108                   bnr_108;
	union reg_isp_bnr_10c                   bnr_10c;
	union reg_isp_bnr_110                   bnr_110;
	union reg_isp_bnr_114                   bnr_114;
	union reg_isp_bnr_118                   bnr_118;
	union reg_isp_bnr_11c                   bnr_11c;
	union reg_isp_bnr_120                   bnr_120;
	union reg_isp_bnr_124                   bnr_124;
	union reg_isp_bnr_128                   bnr_128;
	union reg_isp_bnr_12c                   bnr_12c;
	union reg_isp_bnr_130                   bnr_130;
	union reg_isp_bnr_134                   bnr_134;
	union reg_isp_bnr_138                   bnr_138;
	union reg_isp_bnr_13c                   bnr_13c;
	union reg_isp_bnr_140                   bnr_140;
	union reg_isp_bnr_144                   bnr_144;
	union reg_isp_bnr_148                   bnr_148;
	union reg_isp_bnr_14c                   bnr_14c;
	union reg_isp_bnr_150                   bnr_150;
	union reg_isp_bnr_154                   bnr_154;
	union reg_isp_bnr_158                   bnr_158;
	union reg_isp_bnr_15c                   bnr_15c;
	union reg_isp_bnr_160                   bnr_160;
	union reg_isp_bnr_164                   bnr_164;
	union reg_isp_bnr_168                   bnr_168;
	union reg_isp_bnr_16c                   bnr_16c;
	union reg_isp_bnr_170                   bnr_170;
	union reg_isp_bnr_174                   bnr_174;
	union reg_isp_bnr_178                   bnr_178;
	union reg_isp_bnr_17c                   bnr_17c;
	union reg_isp_bnr_180                   bnr_180;
	union reg_isp_bnr_184                   bnr_184;
	union reg_isp_bnr_188                   bnr_188;
	union reg_isp_bnr_18c                   bnr_18c;
	union reg_isp_bnr_190                   bnr_190;
	union reg_isp_bnr_194                   bnr_194;
	union reg_isp_bnr_198                   bnr_198;
	union reg_isp_bnr_19c                   bnr_19c;
	union reg_isp_bnr_1a0                   bnr_1a0;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ca_t {
	union reg_ca_00                         reg_00;
	union reg_ca_04                         reg_04;
	union reg_ca_08                         reg_08;
	union reg_ca_0c                         reg_0c;
	union reg_ca_10                         reg_10;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ca_lite_t {
	union reg_ca_lite_00                    reg_00;
	union reg_ca_lite_04                    reg_04;
	union reg_ca_lite_08                    reg_08;
	union reg_ca_lite_0c                    reg_0c;
	union reg_ca_lite_10                    reg_10;
	union reg_ca_lite_14                    reg_14;
	union reg_ca_lite_18                    reg_18;
	union reg_ca_lite_1c                    reg_1c;
	union reg_ca_lite_20                    reg_20;
	union reg_ca_lite_24                    reg_24;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_ccm_t {
	union reg_isp_ccm_ccm_00                ccm_00;
	union reg_isp_ccm_ccm_01                ccm_01;
	union reg_isp_ccm_ccm_02                ccm_02;
	union reg_isp_ccm_ccm_10                ccm_10;
	union reg_isp_ccm_ccm_11                ccm_11;
	union reg_isp_ccm_ccm_12                ccm_12;
	union reg_isp_ccm_ccm_20                ccm_20;
	union reg_isp_ccm_ccm_21                ccm_21;
	union reg_isp_ccm_ccm_22                ccm_22;
	union reg_isp_ccm_ccm_ctrl              ccm_ctrl;
	union reg_isp_ccm_ccm_dbg               ccm_dbg;
	uint32_t                                _resv_0x2c[1];
	union reg_isp_ccm_dmy0                  dmy0;
	union reg_isp_ccm_dmy1                  dmy1;
	union reg_isp_ccm_dmy_r                 dmy_r;
	union reg_isp_ccm_ccm_overexp_str       ccm_overexp_str;
	union reg_isp_ccm_ccm_overexp_thr       ccm_overexp_thr;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_cfa_t {
	union reg_isp_cfa_00                    reg_00;
	union reg_isp_cfa_04                    reg_04;
	uint32_t                                _resv_0x8[1];
	union reg_isp_cfa_0c                    reg_0c;
	union reg_isp_cfa_10                    reg_10;
	union reg_isp_cfa_14                    reg_14;
	union reg_isp_cfa_18                    reg_18;
	union reg_isp_cfa_1c                    reg_1c;
	union reg_isp_cfa_20                    reg_20;
	uint32_t                                _resv_0x24[3];
	union reg_isp_cfa_30                    reg_30;
	union reg_isp_cfa_34                    reg_34;
	union reg_isp_cfa_38                    reg_38;
	union reg_isp_cfa_3c                    reg_3c;
	union reg_isp_cfa_40                    reg_40;
	union reg_isp_cfa_44                    reg_44;
	union reg_isp_cfa_48                    reg_48;
	union reg_isp_cfa_4c                    reg_4c;
	uint32_t                                _resv_0x50[8];
	union reg_isp_cfa_70                    reg_70;
	union reg_isp_cfa_74                    reg_74;
	union reg_isp_cfa_78                    reg_78;
	union reg_isp_cfa_7c                    reg_7c;
	union reg_isp_cfa_80                    reg_80;
	uint32_t                                _resv_0x84[3];
	union reg_isp_cfa_90                    reg_90;
	union reg_isp_cfa_94                    reg_94;
	union reg_isp_cfa_98                    reg_98;
	union reg_isp_cfa_9c                    reg_9c;
	union reg_isp_cfa_a0                    reg_a0;
	union reg_isp_cfa_a4                    reg_a4;
	union reg_isp_cfa_a8                    reg_a8;
	uint32_t                                _resv_0xac[25];
	union reg_isp_cfa_110                   reg_110;
	uint32_t                                _resv_0x114[3];
	union reg_isp_cfa_120                   reg_120;
	union reg_isp_cfa_124                   reg_124;
	union reg_isp_cfa_128                   reg_128;
	union reg_isp_cfa_12c                   reg_12c;
	union reg_isp_cfa_130                   reg_130;
	union reg_isp_cfa_134                   reg_134;
	union reg_isp_cfa_138                   reg_138;
	union reg_isp_cfa_13c                   reg_13c;
	union reg_isp_cfa_140                   reg_140;
	union reg_isp_cfa_144                   reg_144;
	union reg_isp_cfa_148                   reg_148;
	union reg_isp_cfa_14c                   reg_14c;
	union reg_isp_cfa_150                   reg_150;
	union reg_isp_cfa_154                   reg_154;
	union reg_isp_cfa_158                   reg_158;
	union reg_isp_cfa_15c                   reg_15c;
	union reg_isp_cfa_160                   reg_160;
	union reg_isp_cfa_164                   reg_164;
	union reg_isp_cfa_168                   reg_168;
	union reg_isp_cfa_16c                   reg_16c;
	union reg_isp_cfa_170                   reg_170;
	union reg_isp_cfa_174                   reg_174;
	union reg_isp_cfa_178                   reg_178;
	union reg_isp_cfa_17c                   reg_17c;
	union reg_isp_cfa_180                   reg_180;
	union reg_isp_cfa_184                   reg_184;
	union reg_isp_cfa_188                   reg_188;
	union reg_isp_cfa_18c                   reg_18c;
	union reg_isp_cfa_190                   reg_190;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_clut_t {
	union reg_isp_clut_ctrl                clut_ctrl;
	union reg_isp_clut_prog_addr           clut_prog_addr;
	union reg_isp_clut_prog_data           clut_prog_data;
	union reg_isp_clut_prog_rdata          clut_prog_rdata;
	uint32_t                               _resv_0x10[4];
	union reg_isp_clut_dbg                 clut_dbg;
	union reg_isp_clut_dmy0                clut_dmy0;
	union reg_isp_clut_dmy1                clut_dmy1;
	union reg_isp_clut_dmy_r               clut_dmy_r;
	union reg_isp_clut_lut_fill_select     lut_fill_select;
	union reg_isp_clut_rdma_sw_start_1t    clut_rdma_sw_start_1t;
	union reg_isp_clut_prog_itrp           clut_prog_itrp;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_cnr_t {
	union reg_cnr_cnr_ctrl_hw_only          cnr_ctrl_hw_only;
	union reg_cnr_mmp_scl_in_size           mmp_scl_in_size;
	union reg_cnr_scl_down_ctrl             scl_down_ctrl;
	union reg_cnr_cnr_ctrl_sw_hw            cnr_ctrl_sw_hw;
	union reg_cnr_cnr_subim_outsel          cnr_subim_outsel;
	union reg_cnr_cnr_cmf_ksize             cnr_cmf_ksize;
	union reg_cnr_cnr_ife2_fsize_sel        cnr_ife2_fsize_sel;
	union reg_cnr_cnr_ife2_eksize_sel       cnr_ife2_eksize_sel;
	union reg_cnr_cnr_ife2_y_rcth           cnr_ife2_y_rcth;
	union reg_cnr_cnr_ife2_y_cwt            cnr_ife2_y_cwt;
	union reg_cnr_cnr_ife2_y_rcwt           cnr_ife2_y_rcwt;
	union reg_cnr_cnr_ife2_y_outl_th        cnr_ife2_y_outl_th;
	union reg_cnr_cnr_ife2_uv_rcth          cnr_ife2_uv_rcth;
	union reg_cnr_cnr_ife2_uv_cwt           cnr_ife2_uv_cwt;
	union reg_cnr_cnr_ife2_uv_rcwt          cnr_ife2_uv_rcwt;
	union reg_cnr_cnr_ife2_uv_outl_th       cnr_ife2_uv_outl_th;
	union reg_cnr_cnr_ife2_y_outl_dth       cnr_ife2_y_outl_dth;
	union reg_cnr_cnr_ife2_u_outl_dth       cnr_ife2_u_outl_dth;
	union reg_cnr_cnr_ife2_v_outl_dth       cnr_ife2_v_outl_dth;
	union reg_cnr_cnr_ife2_ed_pn_th         cnr_ife2_ed_pn_th;
	union reg_cnr_cnr_ife2_ed_hv_th         cnr_ife2_ed_hv_th;
	union reg_cnr_cnr_ife2_y_fth            cnr_ife2_y_fth;
	union reg_cnr_cnr_ife2_y_fth_4          cnr_ife2_y_fth_4;
	union reg_cnr_cnr_ife2_y_fwt            cnr_ife2_y_fwt;
	union reg_cnr_cnr_ife2_u_fth            cnr_ife2_u_fth;
	union reg_cnr_cnr_ife2_u_fth_4          cnr_ife2_u_fth_4;
	union reg_cnr_cnr_ife2_u_fwt            cnr_ife2_u_fwt;
	union reg_cnr_cnr_ife2_v_fth            cnr_ife2_v_fth;
	union reg_cnr_cnr_ife2_v_fth_4          cnr_ife2_v_fth_4;
	union reg_cnr_cnr_ife2_v_fwt            cnr_ife2_v_fwt;
	union reg_cnr_cnr_ife2_yftr_en          cnr_ife2_yftr_en;
	union reg_cnr_cnr_ife2_egd_en           cnr_ife2_egd_en;
	union reg_cnr_cnr_ife2_rc_en            cnr_ife2_rc_en;
	union reg_cnr_cnr_chra_refy_wt          cnr_chra_refy_wt;
	union reg_cnr_cnr_chra_refc_wt          cnr_chra_refc_wt;
	union reg_cnr_cnr_chra_out_wt_lut       cnr_chra_out_wt_lut;
	union reg_cnr_cnr_chra_y_rng            cnr_chra_y_rng;
	union reg_cnr_cnr_chra_y_wtprc          cnr_chra_y_wtprc;
	union reg_cnr_cnr_chra_y_th             cnr_chra_y_th;
	union reg_cnr_cnr_chra_y_wts            cnr_chra_y_wts;
	union reg_cnr_cnr_chra_y_wte            cnr_chra_y_wte;
	union reg_cnr_cnr_chra_uv_rng           cnr_chra_uv_rng;
	union reg_cnr_cnr_chra_uv_wtprc         cnr_chra_uv_wtprc;
	union reg_cnr_cnr_chra_uv_th            cnr_chra_uv_th;
	union reg_cnr_cnr_chra_uv_wts           cnr_chra_uv_wts;
	union reg_cnr_cnr_chra_uv_wte           cnr_chra_uv_wte;
	union reg_cnr_cnr_chra_sat_rng          cnr_chra_sat_rng;
	union reg_cnr_cnr_chra_sat_wtprc        cnr_chra_sat_wtprc;
	union reg_cnr_cnr_chra_sat_th           cnr_chra_sat_th;
	union reg_cnr_cnr_chra_sat_wts          cnr_chra_sat_wts;
	union reg_cnr_cnr_chra_sat_wte          cnr_chra_sat_wte;
	union reg_cnr_cnr_chra_sat_outbld_coring  cnr_chra_sat_outbld_coring;
	uint32_t                                _resv_0xd0[12];
	union reg_cnr_cnr_lca_h_sfact           cnr_lca_h_sfact;
	union reg_cnr_cnr_lca_v_sfact           cnr_lca_v_sfact;
	union reg_cnr_cnr_lca_h_sfact_init_ofs  cnr_lca_h_sfact_init_ofs;
	union reg_cnr_cnr_lca_v_sfact_init_ofs  cnr_lca_v_sfact_init_ofs;
	union reg_cnr_cnr_lca_src_img_size_h    cnr_lca_src_img_size_h;
	union reg_cnr_cnr_lca_src_img_size_v    cnr_lca_src_img_size_v;
	union reg_cnr_cnr_lca_sub_img_size_h    cnr_lca_sub_img_size_h;
	union reg_cnr_cnr_lca_sub_img_size_v    cnr_lca_sub_img_size_v;
	union reg_cnr_cnr_lca_hv_filtmode       cnr_lca_hv_filtmode;
	union reg_cnr_cnr_lca_hv_coef           cnr_lca_hv_coef;
	union reg_cnr_upscl_h_sfact             upscl_h_sfact;
	union reg_cnr_upscl_v_sfact             upscl_v_sfact;
	union reg_cnr_upscl_h_sfact_init_ofs    upscl_h_sfact_init_ofs;
	union reg_cnr_upscl_v_sfact_init_ofs    upscl_v_sfact_init_ofs;
	union reg_cnr_upscl_hv_filtmode         upscl_hv_filtmode;
	union reg_cnr_upscl_hv_coef             upscl_hv_coef;
	union reg_cnr_shdw_read_sel             shdw_read_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_crop_t {
	union reg_crop_0                   crop_0;
	union reg_crop_1                   crop_1;
	union reg_crop_2                   crop_2;
	union reg_crop_3                   crop_3;
	union reg_crop_dummy                    dummy;
	union reg_crop_debug               crop_debug;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_csc_t {
	union reg_isp_csc_0                     reg_0;
	union reg_isp_csc_1                     reg_1;
	union reg_isp_csc_2                     reg_2;
	union reg_isp_csc_3                     reg_3;
	union reg_isp_csc_4                     reg_4;
	union reg_isp_csc_5                     reg_5;
	union reg_isp_csc_6                     reg_6;
	union reg_isp_csc_7                     reg_7;
	union reg_isp_csc_8                     reg_8;
	union reg_isp_csc_9                     reg_9;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_dehaze_t {
	union reg_isp_dehaze_dhz_smooth         dhz_smooth;
	union reg_isp_dehaze_dhz_skin           dhz_skin;
	union reg_isp_dehaze_dhz_wgt            dhz_wgt;
	uint32_t                                _resv_0xc[2];
	union reg_isp_dehaze_dhz_bypass         dhz_bypass;
	union reg_isp_dehaze_0                  reg_0;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_dehaze_1                  reg_1;
	union reg_isp_dehaze_2                  reg_2;
	union reg_isp_dehaze_28                 reg_28;
	union reg_isp_dehaze_2c                 reg_2c;
	union reg_isp_dehaze_3                  reg_3;
	union reg_isp_dehaze_5                  reg_5;
	union reg_isp_dehaze_6                  reg_6;
	union reg_isp_dehaze_7                  reg_7;
	union reg_isp_dehaze_8                  reg_8;
	uint32_t                                _resv_0x44[3];
	union reg_isp_dehaze_9                  reg_9;
	union reg_isp_dehaze_10                 reg_10;
	union reg_isp_dehaze_11                 reg_11;
	union reg_isp_dehaze_12                 reg_12;
	union reg_isp_dehaze_17                 reg_17;
	union reg_isp_dehaze_18                 reg_18;
	union reg_isp_dehaze_19                 reg_19;
	union reg_isp_dehaze_20                 reg_20;
	union reg_isp_dehaze_25                 reg_25;
	union reg_isp_dehaze_26                 reg_26;
	union reg_isp_dehaze_tmap_00            tmap_00;
	union reg_isp_dehaze_tmap_01            tmap_01;
	union reg_isp_dehaze_tmap_02            tmap_02;
	union reg_isp_dehaze_tmap_03            tmap_03;
	union reg_isp_dehaze_tmap_04            tmap_04;
	union reg_isp_dehaze_tmap_05            tmap_05;
	union reg_isp_dehaze_tmap_06            tmap_06;
	union reg_isp_dehaze_tmap_07            tmap_07;
	union reg_isp_dehaze_tmap_08            tmap_08;
	union reg_isp_dehaze_tmap_09            tmap_09;
	union reg_isp_dehaze_tmap_10            tmap_10;
	union reg_isp_dehaze_tmap_11            tmap_11;
	union reg_isp_dehaze_tmap_12            tmap_12;
	union reg_isp_dehaze_tmap_13            tmap_13;
	union reg_isp_dehaze_tmap_14            tmap_14;
	union reg_isp_dehaze_tmap_15            tmap_15;
	union reg_isp_dehaze_tmap_16            tmap_16;
	union reg_isp_dehaze_tmap_17            tmap_17;
	union reg_isp_dehaze_tmap_18            tmap_18;
	union reg_isp_dehaze_tmap_19            tmap_19;
	union reg_isp_dehaze_tmap_20            tmap_20;
	union reg_isp_dehaze_tmap_21            tmap_21;
	union reg_isp_dehaze_tmap_22            tmap_22;
	union reg_isp_dehaze_tmap_23            tmap_23;
	union reg_isp_dehaze_tmap_24            tmap_24;
	union reg_isp_dehaze_tmap_25            tmap_25;
	union reg_isp_dehaze_tmap_26            tmap_26;
	union reg_isp_dehaze_tmap_27            tmap_27;
	union reg_isp_dehaze_tmap_28            tmap_28;
	union reg_isp_dehaze_tmap_29            tmap_29;
	union reg_isp_dehaze_tmap_30            tmap_30;
	union reg_isp_dehaze_tmap_31            tmap_31;
	union reg_isp_dehaze_tmap_32            tmap_32;
	union reg_isp_dehaze_o_dc_th_up_mode    o_dc_th_up_mode;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_dpc_t {
	union reg_dpc_base_config               base_config;
	union reg_dpc_dark_threshold0           dark_threshold0;
	union reg_dpc_dark_threshold1           dark_threshold1;
	union reg_dpc_dark_threshold2           dark_threshold2;
	union reg_dpc_bright_threshold0         bright_threshold0;
	union reg_dpc_bright_threshold1         bright_threshold1;
	union reg_dpc_bright_threshold2         bright_threshold2;
	union reg_dpc_threshold_offset          threshold_offset;
	union reg_dpc_outlier_cnt               outlier_cnt;
	union reg_dpc_outlier_config            outlier_config;
	union reg_dpc_ge_config                 ge_config;
	union reg_dpc_ge_threshold0             ge_threshold0;
	union reg_dpc_ge_threshold1             ge_threshold1;
	union reg_dpc_ge_threshold1_0           ge_threshold1_0;
	union reg_dpc_ge_threshold1_1           ge_threshold1_1;
	union reg_dpc_ge_threshold2_0           ge_threshold2_0;
	union reg_dpc_ge_threshold2_1           ge_threshold2_1;
	union reg_dpc_shdw_read_sel             shdw_read_sel;
	union reg_dpc_spc_config                spc_config;
	union reg_dpc_sw_read_0                 sw_read_0;
	union reg_dpc_write_mem                 write_mem;
	union reg_dpc_write_mem_st_addr         write_mem_st_addr;
	union reg_dpc_auto_cg_en                auto_cg_en;
	union reg_dpc_sw_read_1                 sw_read_1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_drc_t {
	union reg_isp_drc_frame_done            drc_frame_done;
	union reg_isp_drc_enable                drc_enable;
	union reg_isp_drc_blk_num               drc_blk_num;
	union reg_isp_drc_blk_size              drc_blk_size;
	union reg_isp_drc_blk_div               drc_blk_div;
	union reg_isp_drc_lpf_mode              drc_lpf_mode;
	union reg_isp_drc_hist                  drc_hist;
	union reg_isp_drc_hist_step             drc_hist_step;
	union reg_isp_drc_intensit_mode         drc_intensit_mode;
	union reg_isp_drc_hori_lpf_th12         drc_hori_lpf_th12;
	union reg_isp_drc_hori_lpf_th34         drc_hori_lpf_th34;
	union reg_isp_drc_hori_lpf_th56         drc_hori_lpf_th56;
	union reg_isp_drc_q_coeff12             drc_q_coeff12;
	union reg_isp_drc_q_coeff34             drc_q_coeff34;
	union reg_isp_drc_b2q_th                drc_b2q_th;
	union reg_isp_drc_fbc                   drc_fbc;
	union reg_isp_drc_anti_halo             drc_anti_halo;
	union reg_isp_drc_strength              drc_strength;
	union reg_isp_drc_ltm_en                drc_ltm_en;
	union reg_isp_drc_gain                  drc_gain;
	union reg_isp_drc_sat_th                drc_sat_th;
	union reg_isp_drc_sat_delta             drc_sat_delta;
	union reg_isp_drc_yv_bld_lut0_4         drc_yv_bld_lut0_4;
	union reg_isp_drc_yv_bld_lut5_8         drc_yv_bld_lut5_8;
	union reg_isp_drc_input_bld_lut_0_3     drc_input_bld_lut_0_3;
	union reg_isp_drc_input_bld_lut_4_7     drc_input_bld_lut_4_7;
	union reg_isp_drc_input_bld_lut_8_11    drc_input_bld_lut_8_11;
	union reg_isp_drc_input_bld_lut_12_15   drc_input_bld_lut_12_15;
	union reg_isp_drc_input_bld_lut_16      drc_input_bld_lut_16;
	union reg_isp_drc_gtm_l_lut_0_1         drc_gtm_l_lut_0_1;
	union reg_isp_drc_gtm_l_lut_2_3         drc_gtm_l_lut_2_3;
	union reg_isp_drc_gtm_l_lut_4_5         drc_gtm_l_lut_4_5;
	union reg_isp_drc_gtm_l_lut_6_7         drc_gtm_l_lut_6_7;
	union reg_isp_drc_gtm_l_lut_8_9         drc_gtm_l_lut_8_9;
	union reg_isp_drc_gtm_l_lut_10_11       drc_gtm_l_lut_10_11;
	union reg_isp_drc_gtm_l_lut_12_13       drc_gtm_l_lut_12_13;
	union reg_isp_drc_gtm_l_lut_14_15       drc_gtm_l_lut_14_15;
	union reg_isp_drc_gtm_l_lut_16_17       drc_gtm_l_lut_16_17;
	union reg_isp_drc_gtm_l_lut_18_19       drc_gtm_l_lut_18_19;
	union reg_isp_drc_gtm_l_lut_20_21       drc_gtm_l_lut_20_21;
	union reg_isp_drc_gtm_l_lut_22_23       drc_gtm_l_lut_22_23;
	union reg_isp_drc_gtm_l_lut_24_25       drc_gtm_l_lut_24_25;
	union reg_isp_drc_gtm_l_lut_26_27       drc_gtm_l_lut_26_27;
	union reg_isp_drc_gtm_l_lut_28_29       drc_gtm_l_lut_28_29;
	union reg_isp_drc_gtm_l_lut_30_31       drc_gtm_l_lut_30_31;
	union reg_isp_drc_gtm_l_lut_32_33       drc_gtm_l_lut_32_33;
	union reg_isp_drc_gtm_l_lut_34_35       drc_gtm_l_lut_34_35;
	union reg_isp_drc_gtm_l_lut_36_37       drc_gtm_l_lut_36_37;
	union reg_isp_drc_gtm_l_lut_38_39       drc_gtm_l_lut_38_39;
	union reg_isp_drc_gtm_l_lut_40_41       drc_gtm_l_lut_40_41;
	union reg_isp_drc_gtm_l_lut_42_43       drc_gtm_l_lut_42_43;
	union reg_isp_drc_gtm_l_lut_44_45       drc_gtm_l_lut_44_45;
	union reg_isp_drc_gtm_l_lut_46_47       drc_gtm_l_lut_46_47;
	union reg_isp_drc_gtm_l_lut_48_49       drc_gtm_l_lut_48_49;
	union reg_isp_drc_gtm_l_lut_50_51       drc_gtm_l_lut_50_51;
	union reg_isp_drc_gtm_l_lut_52_53       drc_gtm_l_lut_52_53;
	union reg_isp_drc_gtm_l_lut_54_55       drc_gtm_l_lut_54_55;
	union reg_isp_drc_gtm_l_lut_56_57       drc_gtm_l_lut_56_57;
	union reg_isp_drc_gtm_l_lut_58_59       drc_gtm_l_lut_58_59;
	union reg_isp_drc_gtm_l_lut_60_61       drc_gtm_l_lut_60_61;
	union reg_isp_drc_gtm_l_lut_62_63       drc_gtm_l_lut_62_63;
	union reg_isp_drc_gtm_l_lut_64          drc_gtm_l_lut_64;
	union reg_isp_drc_gtm_r_lut_0_1         drc_gtm_r_lut_0_1;
	union reg_isp_drc_gtm_r_lut_2_3         drc_gtm_r_lut_2_3;
	union reg_isp_drc_gtm_r_lut_4_5         drc_gtm_r_lut_4_5;
	union reg_isp_drc_gtm_r_lut_6_7         drc_gtm_r_lut_6_7;
	union reg_isp_drc_gtm_r_lut_8_9         drc_gtm_r_lut_8_9;
	union reg_isp_drc_gtm_r_lut_10_11       drc_gtm_r_lut_10_11;
	union reg_isp_drc_gtm_r_lut_12_13       drc_gtm_r_lut_12_13;
	union reg_isp_drc_gtm_r_lut_14_15       drc_gtm_r_lut_14_15;
	union reg_isp_drc_gtm_r_lut_16          drc_gtm_r_lut_16;
	union reg_isp_drc_luma_prot_lut_0_1     drc_luma_prot_lut_0_1;
	union reg_isp_drc_luma_prot_lut_2_3     drc_luma_prot_lut_2_3;
	union reg_isp_drc_luma_prot_lut_4_5     drc_luma_prot_lut_4_5;
	union reg_isp_drc_luma_prot_lut_6_7     drc_luma_prot_lut_6_7;
	union reg_isp_drc_luma_prot_lut_8_9     drc_luma_prot_lut_8_9;
	union reg_isp_drc_luma_prot_lut_10_11   drc_luma_prot_lut_10_11;
	union reg_isp_drc_luma_prot_lut_12_13   drc_luma_prot_lut_12_13;
	union reg_isp_drc_luma_prot_lut_14_15   drc_luma_prot_lut_14_15;
	union reg_isp_drc_luma_prot_lut_16      drc_luma_prot_lut_16;
	union reg_isp_drc_ltm_l_lut_0_1         drc_ltm_l_lut_0_1;
	union reg_isp_drc_ltm_l_lut_2_3         drc_ltm_l_lut_2_3;
	union reg_isp_drc_ltm_l_lut_4_5         drc_ltm_l_lut_4_5;
	union reg_isp_drc_ltm_l_lut_6_7         drc_ltm_l_lut_6_7;
	union reg_isp_drc_ltm_l_lut_8_9         drc_ltm_l_lut_8_9;
	union reg_isp_drc_ltm_l_lut_10_11       drc_ltm_l_lut_10_11;
	union reg_isp_drc_ltm_l_lut_12_13       drc_ltm_l_lut_12_13;
	union reg_isp_drc_ltm_l_lut_14_15       drc_ltm_l_lut_14_15;
	union reg_isp_drc_ltm_l_lut_16_17       drc_ltm_l_lut_16_17;
	union reg_isp_drc_ltm_l_lut_18_19       drc_ltm_l_lut_18_19;
	union reg_isp_drc_ltm_l_lut_20_21       drc_ltm_l_lut_20_21;
	union reg_isp_drc_ltm_l_lut_22_23       drc_ltm_l_lut_22_23;
	union reg_isp_drc_ltm_l_lut_24_25       drc_ltm_l_lut_24_25;
	union reg_isp_drc_ltm_l_lut_26_27       drc_ltm_l_lut_26_27;
	union reg_isp_drc_ltm_l_lut_28_29       drc_ltm_l_lut_28_29;
	union reg_isp_drc_ltm_l_lut_30_31       drc_ltm_l_lut_30_31;
	union reg_isp_drc_ltm_l_lut_32_33       drc_ltm_l_lut_32_33;
	union reg_isp_drc_ltm_l_lut_34_35       drc_ltm_l_lut_34_35;
	union reg_isp_drc_ltm_l_lut_36_37       drc_ltm_l_lut_36_37;
	union reg_isp_drc_ltm_l_lut_38_39       drc_ltm_l_lut_38_39;
	union reg_isp_drc_ltm_l_lut_40_41       drc_ltm_l_lut_40_41;
	union reg_isp_drc_ltm_l_lut_42_43       drc_ltm_l_lut_42_43;
	union reg_isp_drc_ltm_l_lut_44_45       drc_ltm_l_lut_44_45;
	union reg_isp_drc_ltm_l_lut_46_47       drc_ltm_l_lut_46_47;
	union reg_isp_drc_ltm_l_lut_48_49       drc_ltm_l_lut_48_49;
	union reg_isp_drc_ltm_l_lut_50_51       drc_ltm_l_lut_50_51;
	union reg_isp_drc_ltm_l_lut_52_53       drc_ltm_l_lut_52_53;
	union reg_isp_drc_ltm_l_lut_54_55       drc_ltm_l_lut_54_55;
	union reg_isp_drc_ltm_l_lut_56_57       drc_ltm_l_lut_56_57;
	union reg_isp_drc_ltm_l_lut_58_59       drc_ltm_l_lut_58_59;
	union reg_isp_drc_ltm_l_lut_60_61       drc_ltm_l_lut_60_61;
	union reg_isp_drc_ltm_l_lut_62_63       drc_ltm_l_lut_62_63;
	union reg_isp_drc_ltm_l_lut_64          drc_ltm_l_lut_64;
	union reg_isp_drc_ltm_r_lut_0_1         drc_ltm_r_lut_0_1;
	union reg_isp_drc_ltm_r_lut_2_3         drc_ltm_r_lut_2_3;
	union reg_isp_drc_ltm_r_lut_4_5         drc_ltm_r_lut_4_5;
	union reg_isp_drc_ltm_r_lut_6_7         drc_ltm_r_lut_6_7;
	union reg_isp_drc_ltm_r_lut_8_9         drc_ltm_r_lut_8_9;
	union reg_isp_drc_ltm_r_lut_10_11       drc_ltm_r_lut_10_11;
	union reg_isp_drc_ltm_r_lut_12_13       drc_ltm_r_lut_12_13;
	union reg_isp_drc_ltm_r_lut_14_15       drc_ltm_r_lut_14_15;
	union reg_isp_drc_ltm_r_lut_16          drc_ltm_r_lut_16;
	union reg_isp_drc_img_size              drc_img_size;
	uint32_t                                _resv_0x1ec[1];
	union reg_isp_drc_shdw_sel              drc_shdw_sel;
	union reg_drc_pipectrl_cg               pipectrl_cg;
	union reg_isp_drc_bayerid               drc_bayerid;
	union reg_isp_drc_debug                 drc_debug;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ee_ext_t {
	union reg_ee_ext_ee_ext_reg0            ee_ext_reg0;
	union reg_ee_ext_ee_ext_gamma_0         ee_ext_gamma_0;
	union reg_ee_ext_ee_ext_gamma_1         ee_ext_gamma_1;
	union reg_ee_ext_ee_ext_gamma_2         ee_ext_gamma_2;
	union reg_ee_ext_ee_ext_gamma_3         ee_ext_gamma_3;
	union reg_ee_ext_ee_ext_gamma_4         ee_ext_gamma_4;
	union reg_ee_ext_ee_ext_gamma_5         ee_ext_gamma_5;
	union reg_ee_ext_ee_ext_gamma_6         ee_ext_gamma_6;
	union reg_ee_ext_ee_ext_gamma_7         ee_ext_gamma_7;
	union reg_ee_ext_ee_ext_gamma_8         ee_ext_gamma_8;
	union reg_ee_ext_ee_ext_gamma_9         ee_ext_gamma_9;
	union reg_ee_ext_ee_ext_gamma_10        ee_ext_gamma_10;
	union reg_ee_ext_ee_ext_gamma_11        ee_ext_gamma_11;
	union reg_ee_ext_ee_ext_gamma_12        ee_ext_gamma_12;
	union reg_ee_ext_ee_ext_gamma_13        ee_ext_gamma_13;
	union reg_ee_ext_ee_ext_gamma_14        ee_ext_gamma_14;
	union reg_ee_ext_ee_ext_gamma_15        ee_ext_gamma_15;
	union reg_ee_ext_ee_ext_gamma_16        ee_ext_gamma_16;
	union reg_ee_ext_ee_ext_gamma_17        ee_ext_gamma_17;
	union reg_ee_ext_ee_ext_gamma_18        ee_ext_gamma_18;
	union reg_ee_ext_ee_ext_gamma_19        ee_ext_gamma_19;
	union reg_ee_ext_ee_ext_gamma_20        ee_ext_gamma_20;
	union reg_ee_ext_ee_ext_e5c_0           ee_ext_e5c_0;
	union reg_ee_ext_ee_ext_e5c_1           ee_ext_e5c_1;
	union reg_ee_ext_ee_ext_e5c_2           ee_ext_e5c_2;
	union reg_ee_ext_ee_ext_e5a_0           ee_ext_e5a_0;
	union reg_ee_ext_ee_ext_e5a_1           ee_ext_e5a_1;
	union reg_ee_ext_ee_ext_e5a_2           ee_ext_e5a_2;
	union reg_ee_ext_ee_ext_e5b_0           ee_ext_e5b_0;
	union reg_ee_ext_ee_ext_e5b_1           ee_ext_e5b_1;
	union reg_ee_ext_ee_ext_e5b_2           ee_ext_e5b_2;
	union reg_ee_ext_ee_ext_e7_0            ee_ext_e7_0;
	union reg_ee_ext_ee_ext_e7_1            ee_ext_e7_1;
	union reg_ee_ext_ee_ext_e7_2            ee_ext_e7_2;
	uint32_t                                _resv_0x88[2];
	union reg_ee_ext_ee_ext_norm            ee_ext_norm;
	union reg_ee_ext_ee_ext_luma_blend      ee_ext_luma_blend;
	union reg_ee_ext_ee_ext_region_0        ee_ext_region_0;
	union reg_ee_ext_ee_ext_region_1        ee_ext_region_1;
	uint32_t                                _resv_0xa0[24];
	union reg_ee_ext_ee_ext_region_2        ee_ext_region_2;
	union reg_ee_ext_ee_ext_region_3        ee_ext_region_3;
	union reg_ee_ext_ee_ext_region_4        ee_ext_region_4;
	union reg_ee_ext_ee_ext_region_5        ee_ext_region_5;
	union reg_ee_ext_ee_ext_region_6        ee_ext_region_6;
	union reg_ee_ext_shadow_rd_sel          shadow_rd_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_ee_t {
	union reg_isp_ee_00                     reg_00;
	union reg_isp_ee_04                     reg_04;
	uint32_t                                _resv_0x8[1];
	union reg_isp_ee_0c                     reg_0c;
	union reg_isp_ee_10                     reg_10;
	uint32_t                                _resv_0x14[36];
	union reg_isp_ee_a4                     reg_a4;
	union reg_isp_ee_a8                     reg_a8;
	union reg_isp_ee_ac                     reg_ac;
	union reg_isp_ee_b0                     reg_b0;
	union reg_isp_ee_b4                     reg_b4;
	union reg_isp_ee_b8                     reg_b8;
	union reg_isp_ee_bc                     reg_bc;
	union reg_isp_ee_c0                     reg_c0;
	union reg_isp_ee_c4                     reg_c4;
	union reg_isp_ee_c8                     reg_c8;
	union reg_isp_ee_hcc                    reg_hcc;
	union reg_isp_ee_hd0                    reg_hd0;
	union reg_isp_ee_hd4                    reg_hd4;
	union reg_isp_ee_hd8                    reg_hd8;
	uint32_t                                _resv_0xdc[39];
	union reg_isp_ee_178                    reg_178;
	union reg_isp_ee_17c                    reg_17c;
	union reg_isp_ee_180                    reg_180;
	union reg_isp_ee_184                    reg_184;
	union reg_isp_ee_188                    reg_188;
	union reg_isp_ee_18c                    reg_18c;
	union reg_isp_ee_190                    reg_190;
	union reg_isp_ee_194                    reg_194;
	union reg_isp_ee_198                    reg_198;
	union reg_isp_ee_19c                    reg_19c;
	union reg_isp_ee_1a0                    reg_1a0;
	union reg_isp_ee_1a4                    reg_1a4;
	union reg_isp_ee_1a8                    reg_1a8;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ee_add_back_t {
	union reg_ee_add_back_ee_add_b_reg0     ee_add_b_reg0;
	union reg_ee_add_back_shadow_rd_sel     shadow_rd_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ee_add_t {
	union reg_ee_add_ee_add_reg0            ee_add_reg0;
	union reg_ee_add_ee_add_overshoot_0     ee_add_overshoot_0;
	union reg_ee_add_ee_add_overshoot_1     ee_add_overshoot_1;
	union reg_ee_add_ee_add_overshoot_2     ee_add_overshoot_2;
	union reg_ee_add_ee_add_overshoot_3     ee_add_overshoot_3;
	union reg_ee_add_ee_add_overshoot_4     ee_add_overshoot_4;
	union reg_ee_add_ee_add_refine_0        ee_add_refine_0;
	union reg_ee_add_ee_add_refine_1        ee_add_refine_1;
	union reg_ee_add_ee_add_refine_2        ee_add_refine_2;
	union reg_ee_add_ee_add_refine_3        ee_add_refine_3;
	union reg_ee_add_ee_add_refine_4        ee_add_refine_4;
	union reg_ee_add_ee_add_refine_5        ee_add_refine_5;
	union reg_ee_add_ee_add_refine_6        ee_add_refine_6;
	union reg_ee_add_ee_add_refine_7        ee_add_refine_7;
	union reg_ee_add_ee_add_refine_8        ee_add_refine_8;
	union reg_ee_add_ee_add_refine_9        ee_add_refine_9;
	union reg_ee_add_ee_add_chroma_0        ee_add_chroma_0;
	union reg_ee_add_ee_add_chroma_1        ee_add_chroma_1;
	union reg_ee_add_ee_add_chroma_2        ee_add_chroma_2;
	union reg_ee_add_ee_add_chroma_3        ee_add_chroma_3;
	union reg_ee_add_ee_add_chroma_4        ee_add_chroma_4;
	union reg_ee_add_ee_add_chroma_5        ee_add_chroma_5;
	union reg_ee_add_ee_add_chroma_6        ee_add_chroma_6;
	union reg_ee_add_ee_add_chroma_7        ee_add_chroma_7;
	union reg_ee_add_ee_add_chroma_8        ee_add_chroma_8;
	union reg_ee_add_ee_add_chroma_9        ee_add_chroma_9;
	union reg_ee_add_shadow_rd_sel          shadow_rd_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_fbcd_t {
	union reg_fbcd_00                  reg_00;
	uint32_t                           _resv_0x4[2];
	union reg_fbcd_0c                  reg_0c;
	union reg_fbcd_10                  reg_10;
	union reg_fbcd_14                  reg_14;
	union reg_fbcd_18                  reg_18;
	uint32_t                           _resv_0x1c[1];
	union reg_fbcd_20                  reg_20;
	union reg_fbcd_24                  reg_24;
	union reg_fbcd_28                  reg_28;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_fbce_t {
	union reg_fbce_00                  reg_00;
	uint32_t                           _resv_0x4[3];
	union reg_fbce_10                  reg_10;
	union reg_fbce_14                  reg_14;
	union reg_fbce_18                  reg_18;
	union reg_fbce_1c                  reg_1c;
	union reg_fbce_20                  reg_20;
	union reg_fbce_24                  reg_24;
	union reg_fbce_28                  reg_28;
	union reg_fbce_2c                  reg_2c;
	union reg_fbce_30                  reg_30;
	union reg_fbce_34                  reg_34;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_fusion_t {
	union reg_fusion_ctrl_reg        fusion_ctrl_reg;
	union reg_fusion_ds1             fusion_ds1;
	union reg_fusion_ds2             fusion_ds2;
	union reg_fusion_bcl_range       fusion_bcl_range;
	union reg_fusion_bcl_p0          fusion_bcl_p0;
	union reg_fusion_bcl_slop        fusion_bcl_slop;
	union reg_fusion_bcs_range       fusion_bcs_range;
	union reg_fusion_bcs_p0          fusion_bcs_p0;
	union reg_fusion_bcs_slop        fusion_bcs_slop;
	union reg_fusion_diff            fusion_diff;
	union reg_fusion_lut_0           fusion_lut_0;
	union reg_fusion_lut_1           fusion_lut_1;
	union reg_fusion_lut_2           fusion_lut_2;
	union reg_fusion_hw_auto                hw_auto;
	union reg_fusion_shadow_rd_sel          shadow_rd_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_gms_t {
	union reg_isp_gms_gms_status            gms_status;
	union reg_isp_gms_gms_grace_reset       gms_grace_reset;
	union reg_isp_gms_gms_monitor           gms_monitor;
	union reg_isp_gms_gms_enable            gms_enable;
	uint32_t                                _resv_0x10[1];
	union reg_isp_gms_gms_flow              gms_flow;
	union reg_isp_gms_gms_start_x           gms_start_x;
	union reg_isp_gms_gms_start_y           gms_start_y;
	union reg_isp_gms_gms_location          gms_location;
	uint32_t                                _resv_0x24[1];
	union reg_isp_gms_gms_x_sizem1          gms_x_sizem1;
	union reg_isp_gms_gms_y_sizem1          gms_y_sizem1;
	union reg_isp_gms_gms_x_gap             gms_x_gap;
	union reg_isp_gms_gms_y_gap             gms_y_gap;
	union reg_isp_gms_gms_dummy             gms_dummy;
	uint32_t                                _resv_0x3c[1];
	union reg_isp_gms_gms_mem_sw_mode       gms_sw_mode;
	union reg_isp_gms_gms_mem_sw_raddr      gms_sw_raddr;
	union reg_isp_gms_gms_mem_sw_rdata      gms_sw_rdata;
	union reg_isp_gms_gms_monitor_select    gms_monitor_select;
	uint32_t                                _resv_0x50[20];
	union reg_isp_gms_dmi_enable            dmi_enable;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_csi_bdg_t {
	union reg_isp_csi_bdg_top_ctrl          csi_bdg_top_ctrl;
	union reg_isp_csi_bdg_interrupt_ctrl    csi_bdg_interrupt_ctrl;
	union reg_isp_csi_bdg_dma_dpcm_mode     csi_bdg_dma_dpcm_mode;
	union reg_isp_csi_bdg_dma_ld_dpcm_mode  csi_bdg_dma_ld_dpcm_mode;
	union reg_isp_csi_bdg_ch0_size          ch0_size;
	union reg_isp_csi_bdg_ch1_size          ch1_size;
	union reg_isp_csi_bdg_ch2_size          ch2_size;
	union reg_isp_csi_bdg_ch3_size          ch3_size;
	union reg_isp_csi_bdg_ch0_crop_en       ch0_crop_en;
	union reg_isp_csi_bdg_ch0_horz_crop     ch0_horz_crop;
	union reg_isp_csi_bdg_ch0_vert_crop     ch0_vert_crop;
	union reg_isp_csi_bdg_ch0_blc_sum       ch0_blc_sum;
	union reg_isp_csi_bdg_ch1_crop_en       ch1_crop_en;
	union reg_isp_csi_bdg_ch1_horz_crop     ch1_horz_crop;
	union reg_isp_csi_bdg_ch1_vert_crop     ch1_vert_crop;
	union reg_isp_csi_bdg_ch1_blc_sum       ch1_blc_sum;
	union reg_isp_csi_bdg_ch2_crop_en       ch2_crop_en;
	union reg_isp_csi_bdg_ch2_horz_crop     ch2_horz_crop;
	union reg_isp_csi_bdg_ch2_vert_crop     ch2_vert_crop;
	union reg_isp_csi_bdg_ch2_blc_sum       ch2_blc_sum;
	union reg_isp_csi_bdg_ch3_crop_en       ch3_crop_en;
	union reg_isp_csi_bdg_ch3_horz_crop     ch3_horz_crop;
	union reg_isp_csi_bdg_ch3_vert_crop     ch3_vert_crop;
	union reg_isp_csi_bdg_ch3_blc_sum       ch3_blc_sum;
	union reg_isp_csi_bdg_pat_gen_ctrl      csi_pat_gen_ctrl;
	union reg_isp_csi_bdg_pat_idx_ctrl      csi_pat_idx_ctrl;
	union reg_isp_csi_bdg_pat_color_0       csi_pat_color_0;
	union reg_isp_csi_bdg_pat_color_1       csi_pat_color_1;
	union reg_isp_csi_bdg_background_color_0  csi_background_color_0;
	union reg_isp_csi_bdg_background_color_1  csi_background_color_1;
	union reg_isp_csi_bdg_fix_color_0       csi_fix_color_0;
	union reg_isp_csi_bdg_fix_color_1       csi_fix_color_1;
	union reg_isp_csi_bdg_mde_v_size        csi_mde_v_size;
	union reg_isp_csi_bdg_mde_h_size        csi_mde_h_size;
	union reg_isp_csi_bdg_fde_v_size        csi_fde_v_size;
	union reg_isp_csi_bdg_fde_h_size        csi_fde_h_size;
	union reg_isp_csi_bdg_hsync_ctrl        csi_hsync_ctrl;
	union reg_isp_csi_bdg_vsync_ctrl        csi_vsync_ctrl;
	union reg_isp_csi_bdg_tgen_tt_size      csi_tgen_tt_size;
	union reg_isp_csi_bdg_line_intp_height_0  line_intp_height_0;
	union reg_isp_csi_bdg_ch0_debug_0       ch0_debug_0;
	union reg_isp_csi_bdg_ch0_debug_1       ch0_debug_1;
	union reg_isp_csi_bdg_ch0_debug_2       ch0_debug_2;
	union reg_isp_csi_bdg_ch0_debug_3       ch0_debug_3;
	union reg_isp_csi_bdg_ch1_debug_0       ch1_debug_0;
	union reg_isp_csi_bdg_ch1_debug_1       ch1_debug_1;
	union reg_isp_csi_bdg_ch1_debug_2       ch1_debug_2;
	union reg_isp_csi_bdg_ch1_debug_3       ch1_debug_3;
	union reg_isp_csi_bdg_ch2_debug_0       ch2_debug_0;
	union reg_isp_csi_bdg_ch2_debug_1       ch2_debug_1;
	union reg_isp_csi_bdg_ch2_debug_2       ch2_debug_2;
	union reg_isp_csi_bdg_ch2_debug_3       ch2_debug_3;
	union reg_isp_csi_bdg_ch3_debug_0       ch3_debug_0;
	union reg_isp_csi_bdg_ch3_debug_1       ch3_debug_1;
	union reg_isp_csi_bdg_ch3_debug_2       ch3_debug_2;
	union reg_isp_csi_bdg_ch3_debug_3       ch3_debug_3;
	union reg_isp_csi_bdg_interrupt_status_0  interrupt_status_0;
	union reg_isp_csi_bdg_interrupt_status_1  interrupt_status_1;
	union reg_isp_csi_bdg_bdg_debug         bdg_debug;
	union reg_isp_csi_bdg_out_vsync_line_delay  csi_out_vsync_line_delay;
	union reg_isp_csi_bdg_wr_urgent_ctrl    csi_wr_urgent_ctrl;
	union reg_isp_csi_bdg_rd_urgent_ctrl    csi_rd_urgent_ctrl;
	union reg_isp_csi_bdg_dummy             csi_dummy;
	union reg_isp_csi_bdg_line_intp_height_1  line_intp_height_1;
	union reg_isp_csi_bdg_slice_line_intp_height_0  slice_line_intp_height_0;
	union reg_isp_csi_bdg_slice_line_intp_height_1  slice_line_intp_height_1;
	union reg_isp_csi_bdg_bayer_type_clk_gate_yuv_swap  bayer_type_clk_gate_yuv_swap;
	uint32_t                                _resv_0x10c[1];
	union reg_isp_csi_bdg_wdma_ch0_crop_en  wdma_ch0_crop_en;
	union reg_isp_csi_bdg_wdma_ch0_horz_crop  wdma_ch0_horz_crop;
	union reg_isp_csi_bdg_wdma_ch0_vert_crop  wdma_ch0_vert_crop;
	uint32_t                                _resv_0x11c[1];
	union reg_isp_csi_bdg_wdma_ch1_crop_en  wdma_ch1_crop_en;
	union reg_isp_csi_bdg_wdma_ch1_horz_crop  wdma_ch1_horz_crop;
	union reg_isp_csi_bdg_wdma_ch1_vert_crop  wdma_ch1_vert_crop;
	uint32_t                                _resv_0x12c[1];
	union reg_isp_csi_bdg_wdma_ch2_crop_en  wdma_ch2_crop_en;
	union reg_isp_csi_bdg_wdma_ch2_horz_crop  wdma_ch2_horz_crop;
	union reg_isp_csi_bdg_wdma_ch2_vert_crop  wdma_ch2_vert_crop;
	uint32_t                                _resv_0x13c[1];
	union reg_isp_csi_bdg_wdma_ch3_crop_en  wdma_ch3_crop_en;
	union reg_isp_csi_bdg_wdma_ch3_horz_crop  wdma_ch3_horz_crop;
	union reg_isp_csi_bdg_wdma_ch3_vert_crop  wdma_ch3_vert_crop;
	uint32_t                                _resv_0x14c[1];
	union reg_isp_csi_bdg_trig_dly_control_0  trig_dly_control_0;
	union reg_isp_csi_bdg_trig_dly_control_1  trig_dly_control_1;
	union reg_isp_csi_bdg_bayer_type        bayer_type;
	uint32_t                                _resv_0x15c[1];
	union reg_isp_csi_bdg_ch0_ai_isp_control  ch0_ai_isp_control;
	union reg_isp_csi_bdg_ch0_ai_isp_transform  ch0_ai_isp_transform;
	union reg_isp_csi_bdg_ai_isp_debug_status_ch0  ai_isp_debug_status_ch0;
	uint32_t                                _resv_0x16c[1];
	union reg_isp_csi_bdg_ch1_ai_isp_control  ch1_ai_isp_control;
	union reg_isp_csi_bdg_ch1_ai_isp_transform  ch1_ai_isp_transform;
	union reg_isp_csi_bdg_ai_isp_debug_status_ch1  ai_isp_debug_status_ch1;
	uint32_t                                _resv_0x17c[1];
	union reg_isp_csi_bdg_ch2_ai_isp_control  ch2_ai_isp_control;
	union reg_isp_csi_bdg_ch2_ai_isp_transform  ch2_ai_isp_transform;
	union reg_isp_csi_bdg_ai_isp_debug_status_ch2  ai_isp_debug_status_ch2;
	uint32_t                                _resv_0x18c[1];
	union reg_isp_csi_bdg_ch3_ai_isp_control  ch3_ai_isp_control;
	union reg_isp_csi_bdg_ch3_ai_isp_transform  ch3_ai_isp_transform;
	union reg_isp_csi_bdg_ai_isp_debug_status_ch3  ai_isp_debug_status_ch3;
	union reg_isp_csi_bdg_ch_dma_420_cfg    ch_dma_420_cfg;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_csi_bdg_lite_t {
	union reg_isp_csi_bdg_lite_bdg_top_ctrl  csi_bdg_top_ctrl;
	union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_0  csi_bdg_interrupt_ctrl_0;
	union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_1  csi_bdg_interrupt_ctrl_1;
	union reg_isp_csi_bdg_lite_frame_vld    frame_vld;
	union reg_isp_csi_bdg_lite_ch0_size     ch0_size;
	union reg_isp_csi_bdg_lite_ch1_size     ch1_size;
	union reg_isp_csi_bdg_lite_ch2_size     ch2_size;
	union reg_isp_csi_bdg_lite_ch3_size     ch3_size;
	union reg_isp_csi_bdg_lite_ch0_crop_en  ch0_crop_en;
	union reg_isp_csi_bdg_lite_ch0_horz_crop  ch0_horz_crop;
	union reg_isp_csi_bdg_lite_ch0_vert_crop  ch0_vert_crop;
	uint32_t                                _resv_0x2c[1];
	union reg_isp_csi_bdg_lite_ch1_crop_en  ch1_crop_en;
	union reg_isp_csi_bdg_lite_ch1_horz_crop  ch1_horz_crop;
	union reg_isp_csi_bdg_lite_ch1_vert_crop  ch1_vert_crop;
	uint32_t                                _resv_0x3c[1];
	union reg_isp_csi_bdg_lite_ch2_crop_en  ch2_crop_en;
	union reg_isp_csi_bdg_lite_ch2_horz_crop  ch2_horz_crop;
	union reg_isp_csi_bdg_lite_ch2_vert_crop  ch2_vert_crop;
	uint32_t                                _resv_0x4c[1];
	union reg_isp_csi_bdg_lite_ch3_crop_en  ch3_crop_en;
	union reg_isp_csi_bdg_lite_ch3_horz_crop  ch3_horz_crop;
	union reg_isp_csi_bdg_lite_ch3_vert_crop  ch3_vert_crop;
	uint32_t                                _resv_0x5c[16];
	union reg_isp_csi_bdg_lite_line_intp_height  line_intp_height;
	union reg_isp_csi_bdg_lite_ch0_debug_0  ch0_debug_0;
	union reg_isp_csi_bdg_lite_ch0_debug_1  ch0_debug_1;
	union reg_isp_csi_bdg_lite_ch0_debug_2  ch0_debug_2;
	union reg_isp_csi_bdg_lite_ch0_debug_3  ch0_debug_3;
	union reg_isp_csi_bdg_lite_ch1_debug_0  ch1_debug_0;
	union reg_isp_csi_bdg_lite_ch1_debug_1  ch1_debug_1;
	union reg_isp_csi_bdg_lite_ch1_debug_2  ch1_debug_2;
	union reg_isp_csi_bdg_lite_ch1_debug_3  ch1_debug_3;
	union reg_isp_csi_bdg_lite_ch2_debug_0  ch2_debug_0;
	union reg_isp_csi_bdg_lite_ch2_debug_1  ch2_debug_1;
	union reg_isp_csi_bdg_lite_ch2_debug_2  ch2_debug_2;
	union reg_isp_csi_bdg_lite_ch2_debug_3  ch2_debug_3;
	union reg_isp_csi_bdg_lite_ch3_debug_0  ch3_debug_0;
	union reg_isp_csi_bdg_lite_ch3_debug_1  ch3_debug_1;
	union reg_isp_csi_bdg_lite_ch3_debug_2  ch3_debug_2;
	union reg_isp_csi_bdg_lite_ch3_debug_3  ch3_debug_3;
	union reg_isp_csi_bdg_lite_interrupt_status_0  interrupt_status_0;
	union reg_isp_csi_bdg_lite_interrupt_status_1  interrupt_status_1;
	union reg_isp_csi_bdg_lite_bdg_debug    bdg_debug;
	uint32_t                                _resv_0xec[1];
	union reg_isp_csi_bdg_lite_wr_urgent_ctrl  csi_wr_urgent_ctrl;
	union reg_isp_csi_bdg_lite_rd_urgent_ctrl  csi_rd_urgent_ctrl;
	union reg_isp_csi_bdg_lite_dummy        csi_dummy;
	uint32_t                                _resv_0xfc[21];
	union reg_isp_csi_bdg_lite_trig_dly_control_0  trig_dly_control_0;
	union reg_isp_csi_bdg_lite_trig_dly_control_1  trig_dly_control_1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_dma_ctl_t {
	union reg_isp_dma_ctl_sys_control       sys_control;
	union reg_isp_dma_ctl_base_addr         base_addr;
	union reg_isp_dma_ctl_dma_seglen        dma_seglen;
	union reg_isp_dma_ctl_dma_stride        dma_stride;
	union reg_isp_dma_ctl_dma_segnum        dma_segnum;
	union reg_isp_dma_ctl_dma_status        dma_status;
	union reg_isp_dma_ctl_dma_slicesize     dma_slicesize;
	union reg_isp_dma_ctl_dma_dummy         dma_dummy;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_lsc_t {
	union reg_isp_lsc_sc_wrap_0             sc_wrap_0;
	union reg_isp_lsc_sc_wrap_1             sc_wrap_1;
	union reg_isp_lsc_sc_wrap_2             sc_wrap_2;
	union reg_isp_lsc_sc_wrap_3             sc_wrap_3;
	union reg_isp_lsc_sc_wrap_4             sc_wrap_4;
	union reg_isp_lsc_sc_wrap_5             sc_wrap_5;
	union reg_isp_lsc_sc_wrap_6             sc_wrap_6;
	union reg_isp_lsc_sc_wrap_7             sc_wrap_7;
	union reg_isp_lsc_sc_wrap_8             sc_wrap_8;
	union reg_isp_lsc_sc_wrap_9             sc_wrap_9;
	union reg_isp_lsc_sc_wrap_10            sc_wrap_10;
	union reg_isp_lsc_sc_wrap_11            sc_wrap_11;
	union reg_isp_lsc_sc_wrap_12            sc_wrap_12;
	union reg_isp_lsc_sc_wrap_13            sc_wrap_13;
	union reg_isp_lsc_sc_wrap_14            sc_wrap_14;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_lscr_t {
	union reg_isp_lscr_sc_wrap_0            sc_wrap_0;
	union reg_isp_lscr_sc_wrap_1            sc_wrap_1;
	union reg_isp_lscr_sc_wrap_2            sc_wrap_2;
	union reg_isp_lscr_sc_wrap_3            sc_wrap_3;
	union reg_isp_lscr_sc_wrap_4            sc_wrap_4;
	union reg_isp_lscr_sc_wrap_5            sc_wrap_5;
	union reg_isp_lscr_sc_wrap_6            sc_wrap_6;
	union reg_isp_lscr_sc_wrap_7            sc_wrap_7;
	union reg_isp_lscr_sc_wrap_8            sc_wrap_8;
	union reg_isp_lscr_sc_wrap_9            sc_wrap_9;
	union reg_isp_lscr_sc_wrap_10           sc_wrap_10;
	union reg_isp_lscr_sc_wrap_11           sc_wrap_11;
	union reg_isp_lscr_sc_wrap_12           sc_wrap_12;
	union reg_isp_lscr_sc_wrap_13           sc_wrap_13;
	union reg_isp_lscr_sc_wrap_14           sc_wrap_14;
	union reg_isp_lscr_sc_wrap_15           sc_wrap_15;
	union reg_isp_lscr_sc_wrap_16           sc_wrap_16;
	union reg_isp_lscr_sc_wrap_17           sc_wrap_17;
	union reg_isp_lscr_sc_wrap_18           sc_wrap_18;
	union reg_isp_lscr_sc_wrap_19           sc_wrap_19;
	union reg_isp_lscr_sc_wrap_20           sc_wrap_20;
	union reg_isp_lscr_sc_wrap_21           sc_wrap_21;
	union reg_isp_lscr_sc_wrap_22           sc_wrap_22;
	union reg_isp_lscr_sc_wrap_23           sc_wrap_23;
	union reg_isp_lscr_sc_wrap_24           sc_wrap_24;
	union reg_isp_lscr_sc_wrap_25           sc_wrap_25;
	union reg_isp_lscr_sc_wrap_26           sc_wrap_26;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_rdma_core_t {
	union reg_rdma_core_shadow_rd_sel       shadow_rd_sel;
	union reg_rdma_core_ip_disable          ip_disable;
	uint32_t                                _resv_0x8[1];
	union reg_rdma_core_up_ring_base        up_ring_base;
	union reg_rdma_core_norm_status0        norm_status0;
	union reg_rdma_core_norm_status1        norm_status1;
	union reg_rdma_core_ring_buffer_en      ring_buffer_en;
	uint32_t                                _resv_0x1c[1];
	union reg_rdma_core_norm_perf           norm_perf;
	union reg_rdma_core_ar_priority_sel     ar_priority_sel;
	union reg_rdma_core_ring_patch_enable   ring_patch_enable;
	union reg_rdma_core_set_ring_base       set_ring_base;
	union reg_rdma_core_ring_base_addr_l    ring_base_addr_l;
	union reg_rdma_core_ring_base_addr_h    ring_base_addr_h;
	uint32_t                                _resv_0x38[18];
	union reg_rdma_core_ring_buffer_size0   ring_buffer_size0;
	union reg_rdma_core_ring_buffer_size1   ring_buffer_size1;
	union reg_rdma_core_ring_buffer_size2   ring_buffer_size2;
	union reg_rdma_core_ring_buffer_size3   ring_buffer_size3;
	union reg_rdma_core_ring_buffer_size4   ring_buffer_size4;
	union reg_rdma_core_ring_buffer_size5   ring_buffer_size5;
	union reg_rdma_core_ring_buffer_size6   ring_buffer_size6;
	union reg_rdma_core_ring_buffer_size7   ring_buffer_size7;
	union reg_rdma_core_ring_buffer_size8   ring_buffer_size8;
	union reg_rdma_core_ring_buffer_size9   ring_buffer_size9;
	union reg_rdma_core_ring_buffer_size10  ring_buffer_size10;
	union reg_rdma_core_ring_buffer_size11  ring_buffer_size11;
	union reg_rdma_core_ring_buffer_size12  ring_buffer_size12;
	union reg_rdma_core_ring_buffer_size13  ring_buffer_size13;
	union reg_rdma_core_ring_buffer_size14  ring_buffer_size14;
	union reg_rdma_core_ring_buffer_size15  ring_buffer_size15;
	union reg_rdma_core_ring_buffer_size16  ring_buffer_size16;
	union reg_rdma_core_ring_buffer_size17  ring_buffer_size17;
	union reg_rdma_core_ring_buffer_size18  ring_buffer_size18;
	union reg_rdma_core_ring_buffer_size19  ring_buffer_size19;
	union reg_rdma_core_ring_buffer_size20  ring_buffer_size20;
	union reg_rdma_core_ring_buffer_size21  ring_buffer_size21;
	union reg_rdma_core_ring_buffer_size22  ring_buffer_size22;
	union reg_rdma_core_ring_buffer_size23  ring_buffer_size23;
	union reg_rdma_core_ring_buffer_size24  ring_buffer_size24;
	union reg_rdma_core_ring_buffer_size25  ring_buffer_size25;
	union reg_rdma_core_ring_buffer_size26  ring_buffer_size26;
	union reg_rdma_core_ring_buffer_size27  ring_buffer_size27;
	union reg_rdma_core_ring_buffer_size28  ring_buffer_size28;
	union reg_rdma_core_ring_buffer_size29  ring_buffer_size29;
	union reg_rdma_core_ring_buffer_size30  ring_buffer_size30;
	union reg_rdma_core_ring_buffer_size31  ring_buffer_size31;
	union reg_rdma_core_next_dma_addr_sts0  next_dma_addr_sts0;
	union reg_rdma_core_next_dma_addr_sts1  next_dma_addr_sts1;
	union reg_rdma_core_next_dma_addr_sts2  next_dma_addr_sts2;
	union reg_rdma_core_next_dma_addr_sts3  next_dma_addr_sts3;
	union reg_rdma_core_next_dma_addr_sts4  next_dma_addr_sts4;
	union reg_rdma_core_next_dma_addr_sts5  next_dma_addr_sts5;
	union reg_rdma_core_next_dma_addr_sts6  next_dma_addr_sts6;
	union reg_rdma_core_next_dma_addr_sts7  next_dma_addr_sts7;
	union reg_rdma_core_next_dma_addr_sts8  next_dma_addr_sts8;
	union reg_rdma_core_next_dma_addr_sts9  next_dma_addr_sts9;
	union reg_rdma_core_next_dma_addr_sts10  next_dma_addr_sts10;
	union reg_rdma_core_next_dma_addr_sts11  next_dma_addr_sts11;
	union reg_rdma_core_next_dma_addr_sts12  next_dma_addr_sts12;
	union reg_rdma_core_next_dma_addr_sts13  next_dma_addr_sts13;
	union reg_rdma_core_next_dma_addr_sts14  next_dma_addr_sts14;
	union reg_rdma_core_next_dma_addr_sts15  next_dma_addr_sts15;
	union reg_rdma_core_next_dma_addr_sts16  next_dma_addr_sts16;
	union reg_rdma_core_next_dma_addr_sts17  next_dma_addr_sts17;
	union reg_rdma_core_next_dma_addr_sts18  next_dma_addr_sts18;
	union reg_rdma_core_next_dma_addr_sts19  next_dma_addr_sts19;
	union reg_rdma_core_next_dma_addr_sts20  next_dma_addr_sts20;
	union reg_rdma_core_next_dma_addr_sts21  next_dma_addr_sts21;
	union reg_rdma_core_next_dma_addr_sts22  next_dma_addr_sts22;
	union reg_rdma_core_next_dma_addr_sts23  next_dma_addr_sts23;
	union reg_rdma_core_next_dma_addr_sts24  next_dma_addr_sts24;
	union reg_rdma_core_next_dma_addr_sts25  next_dma_addr_sts25;
	union reg_rdma_core_next_dma_addr_sts26  next_dma_addr_sts26;
	union reg_rdma_core_next_dma_addr_sts27  next_dma_addr_sts27;
	union reg_rdma_core_next_dma_addr_sts28  next_dma_addr_sts28;
	union reg_rdma_core_next_dma_addr_sts29  next_dma_addr_sts29;
	union reg_rdma_core_next_dma_addr_sts30  next_dma_addr_sts30;
	union reg_rdma_core_next_dma_addr_sts31  next_dma_addr_sts31;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_top_t {
	union reg_isp_top_int_event0            int_event0;
	union reg_isp_top_int_event1            int_event1;
	union reg_isp_top_int_event2            int_event2;
	union reg_isp_top_error_sts             error_sts;
	union reg_isp_top_int_event0_en         int_event0_en;
	union reg_isp_top_int_event1_en         int_event1_en;
	union reg_isp_top_int_event2_en         int_event2_en;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_top_sw_ctrl_0             sw_ctrl_0;
	union reg_isp_top_sw_ctrl_1             sw_ctrl_1;
	union reg_isp_top_ctrl_mode_sel0        ctrl_mode_sel0;
	union reg_isp_top_ctrl_mode_sel1        ctrl_mode_sel1;
	union reg_isp_top_scenarios_ctrl        scenarios_ctrl;
	union reg_isp_top_sw_rst                sw_rst;
	union reg_isp_top_blk_idle              blk_idle;
	union reg_isp_top_blk_idle_enable       blk_idle_enable;
	union reg_isp_top_dbus0                 dbus0;
	union reg_isp_top_dbus1                 dbus1;
	union reg_isp_top_dbus2                 dbus2;
	union reg_isp_top_dbus3                 dbus3;
	union reg_isp_top_force_int             force_int;
	union reg_isp_top_dummy                 dummy;
	union reg_isp_top_dbus4                 dbus4;
	union reg_isp_top_ip_enable1            ip_enable1;
	union reg_isp_top_ip_enable2            ip_enable2;
	union reg_isp_top_ip_enable3            ip_enable3;
	union reg_isp_top_cmdq_ctrl             cmdq_ctrl;
	union reg_isp_top_cmdq_trig             cmdq_trig;
	union reg_isp_top_trig_cnt              trig_cnt;
	union reg_isp_top_svn_version           svn_version;
	union reg_isp_top_timestamp             timestamp;
	uint32_t                                _resv_0x7c[1];
	union reg_isp_top_sclie_enable          sclie_enable;
	union reg_isp_top_w_slice_thresh_main   w_slice_thresh_main;
	uint32_t                                _resv_0x88[2];
	union reg_isp_top_r_slice_thresh_main   r_slice_thresh_main;
	uint32_t                                _resv_0x94[2];
	union reg_isp_top_vi_sel_frame_valid    vi_sel_frame_valid;
	union reg_isp_top_first_frame           first_frame;
	union reg_isp_top_int_event0_line_spliter  int_event0_line_spliter;
	uint32_t                                _resv_0xa8[3];
	union reg_isp_top_int_event0_en_line_spliter  int_event0_en_line_spliter;
	uint32_t                                _resv_0xb8[7];
	union reg_isp_top_blk_idle_1            blk_idle_1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_wdma_core_t {
	union reg_wdma_core_shadow_rd_sel       shadow_rd_sel;
	union reg_wdma_core_ip_disable          ip_disable;
	union reg_wdma_core_disable_seglen      disable_seglen;
	union reg_wdma_core_up_ring_base        up_ring_base;
	union reg_wdma_core_norm_status0        norm_status0;
	union reg_wdma_core_norm_status1        norm_status1;
	union reg_wdma_core_ring_buffer_en      ring_buffer_en;
	uint32_t                                _resv_0x1c[1];
	union reg_wdma_core_norm_perf           norm_perf;
	uint32_t                                _resv_0x24[1];
	union reg_wdma_core_ring_patch_enable   ring_patch_enable;
	union reg_wdma_core_set_ring_base       set_ring_base;
	union reg_wdma_core_ring_base_addr_l    ring_base_addr_l;
	union reg_wdma_core_ring_base_addr_h    ring_base_addr_h;
	uint32_t                                _resv_0x38[18];
	union reg_wdma_core_ring_buffer_size0   ring_buffer_size0;
	union reg_wdma_core_ring_buffer_size1   ring_buffer_size1;
	union reg_wdma_core_ring_buffer_size2   ring_buffer_size2;
	union reg_wdma_core_ring_buffer_size3   ring_buffer_size3;
	union reg_wdma_core_ring_buffer_size4   ring_buffer_size4;
	union reg_wdma_core_ring_buffer_size5   ring_buffer_size5;
	union reg_wdma_core_ring_buffer_size6   ring_buffer_size6;
	union reg_wdma_core_ring_buffer_size7   ring_buffer_size7;
	union reg_wdma_core_ring_buffer_size8   ring_buffer_size8;
	union reg_wdma_core_ring_buffer_size9   ring_buffer_size9;
	union reg_wdma_core_ring_buffer_size10  ring_buffer_size10;
	union reg_wdma_core_ring_buffer_size11  ring_buffer_size11;
	union reg_wdma_core_ring_buffer_size12  ring_buffer_size12;
	union reg_wdma_core_ring_buffer_size13  ring_buffer_size13;
	union reg_wdma_core_ring_buffer_size14  ring_buffer_size14;
	union reg_wdma_core_ring_buffer_size15  ring_buffer_size15;
	union reg_wdma_core_ring_buffer_size16  ring_buffer_size16;
	union reg_wdma_core_ring_buffer_size17  ring_buffer_size17;
	union reg_wdma_core_ring_buffer_size18  ring_buffer_size18;
	union reg_wdma_core_ring_buffer_size19  ring_buffer_size19;
	union reg_wdma_core_ring_buffer_size20  ring_buffer_size20;
	union reg_wdma_core_ring_buffer_size21  ring_buffer_size21;
	union reg_wdma_core_ring_buffer_size22  ring_buffer_size22;
	union reg_wdma_core_ring_buffer_size23  ring_buffer_size23;
	union reg_wdma_core_ring_buffer_size24  ring_buffer_size24;
	union reg_wdma_core_ring_buffer_size25  ring_buffer_size25;
	union reg_wdma_core_ring_buffer_size26  ring_buffer_size26;
	union reg_wdma_core_ring_buffer_size27  ring_buffer_size27;
	union reg_wdma_core_ring_buffer_size28  ring_buffer_size28;
	union reg_wdma_core_ring_buffer_size29  ring_buffer_size29;
	union reg_wdma_core_ring_buffer_size30  ring_buffer_size30;
	union reg_wdma_core_ring_buffer_size31  ring_buffer_size31;
	union reg_wdma_core_next_dma_addr_sts0  next_dma_addr_sts0;
	union reg_wdma_core_next_dma_addr_sts1  next_dma_addr_sts1;
	union reg_wdma_core_next_dma_addr_sts2  next_dma_addr_sts2;
	union reg_wdma_core_next_dma_addr_sts3  next_dma_addr_sts3;
	union reg_wdma_core_next_dma_addr_sts4  next_dma_addr_sts4;
	union reg_wdma_core_next_dma_addr_sts5  next_dma_addr_sts5;
	union reg_wdma_core_next_dma_addr_sts6  next_dma_addr_sts6;
	union reg_wdma_core_next_dma_addr_sts7  next_dma_addr_sts7;
	union reg_wdma_core_next_dma_addr_sts8  next_dma_addr_sts8;
	union reg_wdma_core_next_dma_addr_sts9  next_dma_addr_sts9;
	union reg_wdma_core_next_dma_addr_sts10  next_dma_addr_sts10;
	union reg_wdma_core_next_dma_addr_sts11  next_dma_addr_sts11;
	union reg_wdma_core_next_dma_addr_sts12  next_dma_addr_sts12;
	union reg_wdma_core_next_dma_addr_sts13  next_dma_addr_sts13;
	union reg_wdma_core_next_dma_addr_sts14  next_dma_addr_sts14;
	union reg_wdma_core_next_dma_addr_sts15  next_dma_addr_sts15;
	union reg_wdma_core_next_dma_addr_sts16  next_dma_addr_sts16;
	union reg_wdma_core_next_dma_addr_sts17  next_dma_addr_sts17;
	union reg_wdma_core_next_dma_addr_sts18  next_dma_addr_sts18;
	union reg_wdma_core_next_dma_addr_sts19  next_dma_addr_sts19;
	union reg_wdma_core_next_dma_addr_sts20  next_dma_addr_sts20;
	union reg_wdma_core_next_dma_addr_sts21  next_dma_addr_sts21;
	union reg_wdma_core_next_dma_addr_sts22  next_dma_addr_sts22;
	union reg_wdma_core_next_dma_addr_sts23  next_dma_addr_sts23;
	union reg_wdma_core_next_dma_addr_sts24  next_dma_addr_sts24;
	union reg_wdma_core_next_dma_addr_sts25  next_dma_addr_sts25;
	union reg_wdma_core_next_dma_addr_sts26  next_dma_addr_sts26;
	union reg_wdma_core_next_dma_addr_sts27  next_dma_addr_sts27;
	union reg_wdma_core_next_dma_addr_sts28  next_dma_addr_sts28;
	union reg_wdma_core_next_dma_addr_sts29  next_dma_addr_sts29;
	union reg_wdma_core_next_dma_addr_sts30  next_dma_addr_sts30;
	union reg_wdma_core_next_dma_addr_sts31  next_dma_addr_sts31;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ldci_t {
	union reg_ldci_ldci_ctl                 ldci_ctl;
	union reg_ldci_ldci_reciprocal_0        ldci_reciprocal_0;
	union reg_ldci_ldci_luma_prot_lut0      ldci_luma_prot_lut0;
	union reg_ldci_ldci_luma_prot_lut1      ldci_luma_prot_lut1;
	union reg_ldci_ldci_luma_prot_lut2      ldci_luma_prot_lut2;
	union reg_ldci_ldci_luma_prot_lut3      ldci_luma_prot_lut3;
	union reg_ldci_ldci_luma_prot_lut4      ldci_luma_prot_lut4;
	union reg_ldci_ldci_blk_num             ldci_blk_num;
	union reg_ldci_ldci_blk_size            ldci_blk_size;
	union reg_ldci_ldci_reciprocal_1        ldci_reciprocal_1;
	uint32_t                                _resv_0x28[1];
	union reg_ldci_ldci_luma_gain_lut0      ldci_luma_gain_lut0;
	union reg_ldci_ldci_luma_gain_lut1      ldci_luma_gain_lut1;
	union reg_ldci_ldci_luma_gain_lut2      ldci_luma_gain_lut2;
	union reg_ldci_ldci_thr                 ldci_thr;
	union reg_ldci_ldci_diff_gain_lut_l0    ldci_diff_gain_lut_l0;
	union reg_ldci_ldci_diff_gain_lut_l1    ldci_diff_gain_lut_l1;
	union reg_ldci_ldci_diff_gain_lut_r0    ldci_diff_gain_lut_r0;
	union reg_ldci_ldci_diff_gain_lut_r1    ldci_diff_gain_lut_r1;
	union reg_ldci_ldci_roi_ctl             ldci_roi_ctl;
	union reg_ldci_ldci_roi_axis_0          ldci_roi_axis_0;
	union reg_ldci_ldci_roi_axis_1          ldci_roi_axis_1;
	union reg_ldci_ldci_hw_ctl              ldci_hw_ctl;
	union reg_ldci_ldci_dma_stat            ldci_dma_stat;
	union reg_ldci_ldci_crop_width          ldci_crop_width;
	union reg_ldci_ldci_crop_height         ldci_crop_height;
	union reg_ldci_ldci_crop_enable         ldci_crop_enable;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_ldci_map_lut_t {
	union reg_ldci_map_lut_ldci_map_ctl     ldci_map_ctl;
	union reg_ldci_map_lut_ldci_rw_ctl      ldci_rw_ctl;
	union reg_ldci_map_lut_ldci_map_sw_ctl  ldci_map_sw_ctl;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_map_curve_t {
	union reg_map_curve_01                  reg_01;
	union reg_map_curve_fcurve_flumw_lut0   fcurve_flumw_lut0;
	union reg_map_curve_fcurve_flumw_lut1   fcurve_flumw_lut1;
	union reg_map_curve_fcurve_flumw_lut2   fcurve_flumw_lut2;
	union reg_map_curve_fcurve_flumw_lut3   fcurve_flumw_lut3;
	union reg_map_curve_fcurve_flumw_lut4   fcurve_flumw_lut4;
	union reg_map_curve_fcurve_l0           fcurve_l0;
	union reg_map_curve_fcurve_l1           fcurve_l1;
	union reg_map_curve_fcurve_l2           fcurve_l2;
	union reg_map_curve_fcurve_l3           fcurve_l3;
	union reg_map_curve_fcurve_l4           fcurve_l4;
	union reg_map_curve_fcurve_l5           fcurve_l5;
	union reg_map_curve_fcurve_l6           fcurve_l6;
	union reg_map_curve_fcurve_l7           fcurve_l7;
	union reg_map_curve_fcurve_l8           fcurve_l8;
	union reg_map_curve_fcurve_l9           fcurve_l9;
	union reg_map_curve_fcurve_l10          fcurve_l10;
	union reg_map_curve_fcurve_l11          fcurve_l11;
	union reg_map_curve_fcurve_l12          fcurve_l12;
	union reg_map_curve_fcurve_l13          fcurve_l13;
	union reg_map_curve_fcurve_l14          fcurve_l14;
	union reg_map_curve_fcurve_l15          fcurve_l15;
	union reg_map_curve_fcurve_l16          fcurve_l16;
	union reg_map_curve_fcurve_l17          fcurve_l17;
	union reg_map_curve_fcurve_l18          fcurve_l18;
	union reg_map_curve_fcurve_l19          fcurve_l19;
	union reg_map_curve_fcurve_l20          fcurve_l20;
	union reg_map_curve_fcurve_l21          fcurve_l21;
	union reg_map_curve_fcurve_l22          fcurve_l22;
	union reg_map_curve_fcurve_l23          fcurve_l23;
	union reg_map_curve_fcurve_l24          fcurve_l24;
	union reg_map_curve_fcurve_l25          fcurve_l25;
	union reg_map_curve_fcurve_l26          fcurve_l26;
	union reg_map_curve_fcurve_l27          fcurve_l27;
	union reg_map_curve_fcurve_l28          fcurve_l28;
	union reg_map_curve_fcurve_l29          fcurve_l29;
	union reg_map_curve_fcurve_l30          fcurve_l30;
	union reg_map_curve_fcurve_l31          fcurve_l31;
	union reg_map_curve_fcurve_l32          fcurve_l32;
	union reg_map_curve_fcurve_l33          fcurve_l33;
	union reg_map_curve_fcurve_l34          fcurve_l34;
	union reg_map_curve_fcurve_l35          fcurve_l35;
	union reg_map_curve_fcurve_l36          fcurve_l36;
	union reg_map_curve_fcurve_l37          fcurve_l37;
	union reg_map_curve_fcurve_l38          fcurve_l38;
	union reg_map_curve_fcurve_l39          fcurve_l39;
	union reg_map_curve_fcurve_l40          fcurve_l40;
	union reg_map_curve_fcurve_l41          fcurve_l41;
	union reg_map_curve_fcurve_l42          fcurve_l42;
	union reg_map_curve_fcurve_l43          fcurve_l43;
	union reg_map_curve_fcurve_l44          fcurve_l44;
	union reg_map_curve_fcurve_l45          fcurve_l45;
	union reg_map_curve_fcurve_l46          fcurve_l46;
	union reg_map_curve_fcurve_l47          fcurve_l47;
	union reg_map_curve_fcurve_l48          fcurve_l48;
	union reg_map_curve_fcurve_l49          fcurve_l49;
	union reg_map_curve_fcurve_l50          fcurve_l50;
	union reg_map_curve_fcurve_l51          fcurve_l51;
	union reg_map_curve_fcurve_l52          fcurve_l52;
	union reg_map_curve_fcurve_l53          fcurve_l53;
	union reg_map_curve_fcurve_l54          fcurve_l54;
	union reg_map_curve_fcurve_l55          fcurve_l55;
	union reg_map_curve_fcurve_l56          fcurve_l56;
	union reg_map_curve_fcurve_l57          fcurve_l57;
	union reg_map_curve_fcurve_l58          fcurve_l58;
	union reg_map_curve_fcurve_l59          fcurve_l59;
	union reg_map_curve_fcurve_l60          fcurve_l60;
	union reg_map_curve_fcurve_l61          fcurve_l61;
	union reg_map_curve_fcurve_l62          fcurve_l62;
	union reg_map_curve_fcurve_l63          fcurve_l63;
	union reg_map_curve_fcurve_l64          fcurve_l64;
	union reg_map_curve_fcurve_r0           fcurve_r0;
	union reg_map_curve_fcurve_r1           fcurve_r1;
	union reg_map_curve_fcurve_r2           fcurve_r2;
	union reg_map_curve_fcurve_r3           fcurve_r3;
	union reg_map_curve_fcurve_r4           fcurve_r4;
	union reg_map_curve_fcurve_r5           fcurve_r5;
	union reg_map_curve_fcurve_r6           fcurve_r6;
	union reg_map_curve_fcurve_r7           fcurve_r7;
	union reg_map_curve_fcurve_r8           fcurve_r8;
	union reg_map_curve_fcurve_r9           fcurve_r9;
	union reg_map_curve_fcurve_r10          fcurve_r10;
	union reg_map_curve_fcurve_r11          fcurve_r11;
	union reg_map_curve_fcurve_r12          fcurve_r12;
	union reg_map_curve_fcurve_r13          fcurve_r13;
	union reg_map_curve_fcurve_r14          fcurve_r14;
	union reg_map_curve_fcurve_r15          fcurve_r15;
	union reg_map_curve_fcurve_r16          fcurve_r16;
	union reg_map_curve_fcurve_end0         fcurve_end0;
	union reg_map_curve_fcurve_end1         fcurve_end1;
	union reg_map_curve_fcurve_end2         fcurve_end2;
	union reg_map_curve_fcurve_end3         fcurve_end3;
	union reg_map_curve_fcurve_end4         fcurve_end4;
	union reg_map_curve_fcurve_end5         fcurve_end5;
	union reg_map_curve_fcurve_end6         fcurve_end6;
	union reg_map_curve_fcurve_end7         fcurve_end7;
	union reg_map_curve_fcurve_end8         fcurve_end8;
	union reg_map_curve_fcurve_end9         fcurve_end9;
	union reg_map_curve_fcurve_end10        fcurve_end10;
	union reg_map_curve_fcurve_end11        fcurve_end11;
	union reg_map_curve_fcurve_end12        fcurve_end12;
	union reg_map_curve_fcurve_end13        fcurve_end13;
	union reg_map_curve_fcurve_end14        fcurve_end14;
	union reg_map_curve_fcurve_end15        fcurve_end15;
	union reg_map_curve_fcurve_end16        fcurve_end16;
	union reg_map_curve_hw_auto             hw_auto;
	union reg_map_curve_shdow_sel           shdow_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_444_422_t {
	union reg_isp_444_422_0                 reg_0;
	union reg_isp_444_422_1                 reg_1;
	union reg_isp_444_422_2                 reg_2;
	union reg_isp_444_422_3                 reg_3;
	union reg_isp_444_422_4                 reg_4;
	union reg_isp_444_422_5                 reg_5;
	union reg_isp_444_422_6                 reg_6;
	union reg_isp_444_422_7                 reg_7;
	union reg_isp_444_422_8                 reg_8;
	union reg_isp_444_422_9                 reg_9;
	union reg_isp_444_422_10                reg_10;
	union reg_isp_444_422_11                reg_11;
	union reg_isp_444_422_12                reg_12;
	union reg_isp_444_422_13                reg_13;
	union reg_isp_444_422_14                reg_14;
	union reg_isp_444_422_15                reg_15;
	union reg_isp_444_422_16                reg_16;
	union reg_isp_444_422_17                reg_17;
	union reg_isp_444_422_18                reg_18;
	union reg_isp_444_422_19                reg_19;
	union reg_isp_444_422_20                reg_20;
	union reg_isp_444_422_21                reg_21;
	union reg_isp_444_422_22                reg_22;
	union reg_isp_444_422_23                reg_23;
	union reg_isp_444_422_24                reg_24;
	union reg_isp_444_422_25                reg_25;
	union reg_isp_444_422_26                reg_26;
	union reg_isp_444_422_27                reg_27;
	union reg_isp_444_422_28                reg_28;
	union reg_isp_444_422_29                reg_29;
	union reg_isp_444_422_30                reg_30;
	union reg_isp_444_422_31                reg_31;
	union reg_isp_444_422_32                reg_32;
	union reg_isp_444_422_33                reg_33;
	union reg_isp_444_422_34                reg_34;
	union reg_isp_444_422_35                reg_35;
	union reg_isp_444_422_36                reg_36;
	union reg_isp_444_422_37                reg_37;
	union reg_isp_444_422_38                reg_38;
	union reg_isp_444_422_39                reg_39;
	union reg_isp_444_422_40                reg_40;
	union reg_isp_444_422_41                reg_41;
	union reg_isp_444_422_42                reg_42;
	union reg_isp_444_422_43                reg_43;
	union reg_isp_444_422_44                reg_44;
	union reg_isp_444_422_45                reg_45;
	union reg_isp_444_422_46                reg_46;
	union reg_isp_444_422_47                reg_47;
	union reg_isp_444_422_48                reg_48;
	union reg_isp_444_422_49                reg_49;
	union reg_isp_444_422_50                reg_50;
	union reg_isp_444_422_51                reg_51;
	union reg_isp_444_422_52                reg_52;
	union reg_isp_444_422_53                reg_53;
	union reg_isp_444_422_54                reg_54;
	union reg_isp_444_422_55                reg_55;
	union reg_isp_444_422_56                reg_56;
	union reg_isp_444_422_57                reg_57;
	union reg_isp_444_422_58                reg_58;
	union reg_isp_444_422_59                reg_59;
	union reg_isp_444_422_60                reg_60;
	union reg_isp_444_422_61                reg_61;
	union reg_isp_444_422_62                reg_62;
	union reg_isp_444_422_63                reg_63;
	union reg_isp_444_422_64                reg_64;
	union reg_isp_444_422_65                reg_65;
	union reg_isp_444_422_66                reg_66;
	union reg_isp_444_422_67                reg_67;
	union reg_isp_444_422_68                reg_68;
	union reg_isp_444_422_69                reg_69;
	union reg_isp_444_422_70                reg_70;
	union reg_isp_444_422_71                reg_71;
	union reg_isp_444_422_72                reg_72;
	union reg_isp_444_422_73                reg_73;
	union reg_isp_444_422_74                reg_74;
	union reg_isp_444_422_75                reg_75;
	union reg_isp_444_422_76                reg_76;
	union reg_isp_444_422_77                reg_77;
	union reg_isp_444_422_78                reg_78;
	union reg_isp_444_422_79                reg_79;
	union reg_isp_444_422_80                reg_80;
	union reg_isp_444_422_81                reg_81;
	union reg_isp_444_422_82                reg_82;
	union reg_isp_444_422_83                reg_83;
	union reg_isp_444_422_84                reg_84;
	union reg_isp_444_422_85                reg_85;
	union reg_isp_444_422_86                reg_86;
	union reg_isp_444_422_87                reg_87;
	union reg_isp_444_422_88                reg_88;
	union reg_isp_444_422_89                reg_89;
	union reg_isp_444_422_90                reg_90;
	union reg_isp_444_422_91                reg_91;
	union reg_isp_444_422_92                reg_92;
	union reg_isp_444_422_93                reg_93;
	union reg_isp_444_422_94                reg_94;
	union reg_isp_444_422_95                reg_95;
	union reg_isp_444_422_96                reg_96;
	union reg_isp_444_422_97                reg_97;
	union reg_isp_444_422_98                reg_98;
	union reg_isp_444_422_99                reg_99;
	union reg_isp_444_422_100               reg_100;
	union reg_isp_444_422_101               reg_101;
	union reg_isp_444_422_102               reg_102;
	union reg_isp_444_422_103               reg_103;
	union reg_isp_444_422_104               reg_104;
	union reg_isp_444_422_105               reg_105;
	union reg_isp_444_422_106               reg_106;
	union reg_isp_444_422_107               reg_107;
	union reg_isp_444_422_108               reg_108;
	union reg_isp_444_422_109               reg_109;
	union reg_isp_444_422_110               reg_110;
	union reg_isp_444_422_111               reg_111;
	union reg_isp_444_422_112               reg_112;
	union reg_isp_444_422_113               reg_113;
	union reg_isp_444_422_114               reg_114;
	union reg_isp_444_422_115               reg_115;
	union reg_isp_444_422_reg116            reg_116;
	union reg_isp_444_422_reg117            reg_117;
	union reg_isp_444_422_reg118            reg_118;
	union reg_isp_444_422_reg119            reg_119;
	union reg_isp_444_422_reg120            reg_120;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_motion_resize_t {
	union reg_motion_resize_resize_00       resize_00;
	union reg_motion_resize_resize_04       resize_04;
	union reg_motion_resize_resize_08       resize_08;
	union reg_motion_resize_resize_0c       resize_0c;
	union reg_motion_resize_resize_10       resize_10;
	union reg_motion_resize_resize_20       resize_20;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_pfr_t {
	union reg_pfr_pfr_reg0                  pfr_reg0;
	union reg_pfr_pfr_luma_set0             pfr_luma_set0;
	union reg_pfr_pfr_luma_level0           pfr_luma_level0;
	uint32_t                                _resv_0xc[1];
	union reg_pfr_pfr_luma_level1           pfr_luma_level1;
	union reg_pfr_pfr_luma_level2           pfr_luma_level2;
	union reg_pfr_pfr_luma_level3           pfr_luma_level3;
	union reg_pfr_pfr_luma_level4           pfr_luma_level4;
	union reg_pfr_pfr_color_uv              pfr_color_uv;
	union reg_pfr_pfr_uv_diff_th            pfr_uv_diff_th;
	union reg_pfr_pfr_uv_diff_lut0          pfr_uv_diff_lut0;
	uint32_t                                _resv_0x2c[1];
	union reg_pfr_pfr_uv_diff_lut1          pfr_uv_diff_lut1;
	union reg_pfr_pfr_rb_wet                pfr_rb_wet;
	union reg_pfr_pfr_hueset                pfr_hueset;
	uint32_t                                _resv_0x3c[1];
	union reg_pfr_pfr_hue_range             pfr_hue_range;
	union reg_pfr_pfr_hue_th                pfr_hue_th;
	union reg_pfr_pfr_g_diff_th0            pfr_g_diff_th0;
	union reg_pfr_pfr_g_diff_th1            pfr_g_diff_th1;
	union reg_pfr_pfr_edge_th               pfr_edge_th;
	uint32_t                                _resv_0x54[1];
	union reg_pfr_shadow_rd_sel             shadow_rd_sel;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_pre_raw_fe_t {
	union reg_pre_raw_fe_pre_raw_ctrl       pre_raw_ctrl;
	union reg_pre_raw_fe_pre_raw_frame_size  pre_raw_frame_size;
	uint32_t                                _resv_0x8[6];
	union reg_pre_raw_fe_pre_raw_post_no_rspd_cyc  pre_raw_post_no_rspd_cyc;
	uint32_t                                _resv_0x24[1];
	union reg_pre_raw_fe_pre_raw_frame_vld  pre_raw_frame_vld;
	union reg_pre_raw_fe_pre_raw_debug_state  pre_raw_debug_state;
	union reg_pre_raw_fe_pre_raw_dummy      pre_raw_dummy;
	union reg_pre_raw_fe_pre_raw_debug_info  pre_raw_info;
	uint32_t                                _resv_0x38[6];
	union reg_pre_raw_fe_fe_idle_info       fe_idle_info;
	uint32_t                                _resv_0x54[3];
	union reg_pre_raw_fe_fe_check_sum       fe_check_sum;
	union reg_pre_raw_fe_fe_check_sum_value  fe_check_sum_value;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_pre_raw_vi_sel_t {
	union reg_pre_raw_vi_sel_0              reg_0;
	union reg_pre_raw_vi_sel_1              reg_1;
	uint32_t                                _resv_0x8[2];
	union reg_pre_raw_vi_sel_2              reg_2;
	union reg_pre_raw_vi_sel_3              reg_3;
	uint32_t                                _resv_0x18[2];
	union reg_pre_raw_vi_sel_4              reg_4;
	union reg_pre_raw_vi_sel_5              reg_5;
	union reg_pre_raw_vi_sel_6              reg_6;
	union reg_pre_raw_vi_sel_7              reg_7;
	union reg_pre_raw_vi_sel_8              reg_8;
	union reg_pre_raw_vi_sel_9              reg_9;
	union reg_pre_raw_vi_sel_10             reg_10;
	union reg_pre_raw_vi_sel_11             reg_11;
	union reg_pre_raw_vi_sel_12             reg_12;
	union reg_pre_raw_vi_sel_13             reg_13;
	union reg_pre_raw_vi_sel_14             reg_14;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_raw_top0_t {
	union reg_raw_top0_raw_top_read_sel     raw_top_read_sel;
	union reg_raw_top0_chk_sum_en           chk_sum_en;
	union reg_raw_top0_raw_2                raw_2;
	union reg_raw_top0_raw_3                raw_3;
	union reg_raw_top0_raw_bayer_type_topleft  raw_bayer_type_topleft;
	union reg_raw_top0_pass_sel             pass_sel;
	union reg_raw_top0_ip_cfg_gclk_en       ip_cfg_gclk_en;
	union reg_raw_top0_ip_bypass            ip_bypass;
	union reg_raw_top0_pg_cfg0              pg_cfg0;
	union reg_raw_top0_pg_cfg1              pg_cfg1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_raw_top1_t {
	union reg_raw_top1_raw_top_read_sel     raw_top_read_sel;
	union reg_raw_top1_chk_sum_en           chk_sum_en;
	union reg_raw_top1_raw_2                raw_2;
	union reg_raw_top1_raw_4                raw_4;
	union reg_raw_top1_raw_top_status       raw_top_status;
	union reg_raw_top1_raw_top_debug        raw_top_debug;
	union reg_raw_top1_raw_bayer_type       raw_bayer_type;
	union reg_raw_top1_fifo_reverse         fifo_reverse;
	uint32_t                                _resv_0x20[1];
	union reg_raw_top1_raw_top_dma_idle     raw_top_dma_idle;
	union reg_raw_top1_raw_top_checksum     raw_top_checksum;
	union reg_raw_top1_ip_cfg_gclk_en       ip_cfg_gclk_en;
	union reg_raw_top1_ip_bypass            ip_bypass;
	union reg_raw_top1_af_blc_cfg0          blc_cfg0;
	union reg_raw_top1_af_blc_cfg1          blc_cfg1;
	union reg_raw_top1_af_blc_cfg2          blc_cfg2;
	union reg_raw_top1_af_blc_cfg3          blc_cfg3;
	union reg_raw_top1_af_blc_cfg4          blc_cfg4;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_rgb_dither_t {
	union reg_isp_rgb_dither_rgb_dither     rgb_dither;
	union reg_isp_rgb_dither_rgb_dither_debug0  rgb_dither_debug0;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_rgb_top_t {
	union reg_rgb_top_patgen_01             patgen_01;
	union reg_rgb_top_patgen_02             patgen_02;
	union reg_rgb_top_patgen_03             patgen_03;
	union reg_rgb_top_patgen_04             patgen_04;
	union reg_rgb_top_dummy                 dummy;
	union reg_rgb_top_patgen_05             patgen_05;
	union reg_rgb_top_slice_soft_reset      slice_soft_reset;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_gamma_t {
	union reg_isp_gamma_ctrl          gamma_ctrl;
	union reg_isp_gamma_prog_ctrl     gamma_prog_ctrl;
	union reg_isp_gamma_prog_st_addr  gamma_prog_st_addr;
	union reg_isp_gamma_prog_data     gamma_prog_data;
	union reg_isp_gamma_prog_max      gamma_prog_max;
	union reg_isp_gamma_mem_sw_raddr  gamma_sw_raddr;
	union reg_isp_gamma_mem_sw_rdata  gamma_sw_rdata;
	union reg_isp_gamma_mem_sw_rdata_bg  gamma_sw_rdata_bg;
	union reg_isp_gamma_dbg           gamma_dbg;
	union reg_isp_gamma_dmy0          gamma_dmy0;
	union reg_isp_gamma_dmy1          gamma_dmy1;
	union reg_isp_gamma_dmy_r         gamma_dmy_r;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_se_bnr_t {
	union reg_isp_se_bnr_u1_se_bnr_enable   u1_se_bnr_enable;
	union reg_isp_se_bnr_u4_se_bnr_spatial_str  u4_se_bnr_spatial_str;
	union reg_isp_se_bnr_u10_se_bnr_intensity_str_lut_0  u10_se_bnr_intensity_str_lut_0;
	union reg_isp_se_bnr_u10_se_bnr_intensity_str_lut_1  u10_se_bnr_intensity_str_lut_1;
	union reg_isp_se_bnr_u10_se_bnr_intensity_str_lut_2  u10_se_bnr_intensity_str_lut_2;
	union reg_isp_se_bnr_u10_se_bnr_intensity_str_lut_3  u10_se_bnr_intensity_str_lut_3;
	union reg_isp_se_bnr_u10_se_bnr_intensity_str_lut_4  u10_se_bnr_intensity_str_lut_4;
	union reg_isp_se_bnr_u10_se_bnr_intensity_str_lut_5  u10_se_bnr_intensity_str_lut_5;
	union reg_isp_se_bnr_shdw_read_sel      shdw_read_sel;
	union reg_isp_se_bnr_hw_auto_cg_en      hw_auto_cg_en;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_ycurv_t {
	union reg_isp_ycurv_ycur_ctrl           ycur_ctrl;
	union reg_isp_ycurv_ycur_prog_ctrl      ycur_prog_ctrl;
	union reg_isp_ycurv_ycur_prog_st_addr   ycur_prog_st_addr;
	union reg_isp_ycurv_ycur_prog_data      ycur_prog_data;
	union reg_isp_ycurv_ycur_prog_max       ycur_prog_max;
	union reg_isp_ycurv_ycur_mem_sw_mode    ycur_sw_mode;
	union reg_isp_ycurv_ycur_mem_sw_rdata   ycur_sw_rdata;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_ycurv_ycur_dbg            ycur_dbg;
	union reg_isp_ycurv_ycur_dmy0           ycur_dmy0;
	union reg_isp_ycurv_ycur_dmy1           ycur_dmy1;
	union reg_isp_ycurv_ycur_dmy_r          ycur_dmy_r;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_twode_t {
	union reg_twode_src_surface_reg0        src_surface_reg0;
	union reg_twode_src_surface_reg1        src_surface_reg1;
	union reg_twode_src_surface_reg2        src_surface_reg2;
	union reg_twode_src_surface_reg3        src_surface_reg3;
	union reg_twode_src_surface_reg4        src_surface_reg4;
	union reg_twode_src_surface_reg5        src_surface_reg5;
	union reg_twode_src_surface_reg6        src_surface_reg6;
	union reg_twode_src_surface_reg7        src_surface_reg7;
	union reg_twode_dst_surface_cfg0        dst_surface_cfg0;
	union reg_twode_dst_surface_cfg1        dst_surface_cfg1;
	union reg_twode_dst_surface_cfg2        dst_surface_cfg2;
	union reg_twode_dst_surface_cfg3        dst_surface_cfg3;
	union reg_twode_dst_surface_cfg4        dst_surface_cfg4;
	union reg_twode_draw_line_ctrl0         draw_line_ctrl0;
	union reg_twode_draw_line_ctrl1         draw_line_ctrl1;
	union reg_twode_draw_line_ctrl2         draw_line_ctrl2;
	union reg_twode_draw_line_ctrl3         draw_line_ctrl3;
	union reg_twode_draw_line_ctrl4         draw_line_ctrl4;
	union reg_twode_draw_line_ctrl5         draw_line_ctrl5;
	union reg_twode_shadow_rd_sel           shadow_rd_sel;
	union reg_twode_operator_start_ctrl     operator_start_ctrl;
	union reg_twode_dbg_bus                 dbg_bus;
	union reg_twode_clk_gate                clk_gate;
	union reg_twode_intr_status             intr_status;
	union reg_twode_intr_clear              intr_clear;
	union reg_twode_bw_limit_rd             bw_limit_rd;
	union reg_twode_bw_limit_wr             bw_limit_wr;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_isp_yuv_dither_t {
	union reg_isp_yuv_dither_y_dither       y_dither;
	union reg_isp_yuv_dither_uv_dither      uv_dither;
	union reg_isp_yuv_dither_debug_00       debug_00;
	union reg_isp_yuv_dither_debug_01       debug_01;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct reg_yuv_top_t {
	union reg_yuv_top_yuv_0                 yuv_0;
	uint32_t                                _resv_0x4[1];
	union reg_yuv_top_yuv_2                 yuv_2;
	union reg_yuv_top_yuv_3                 yuv_3;
	union reg_yuv_top_yuv_debug_0           yuv_debug_0;
	union reg_yuv_top_yuv_4                 yuv_4;
	union reg_yuv_top_yuv_debug_state       yuv_debug_state;
	uint32_t                                _resv_0x1c[1];
	union reg_yuv_top_yuv_5                 yuv_5;
	uint32_t                                _resv_0x24[15];
	union reg_yuv_top_yuv_ctrl              yuv_ctrl;
	union reg_yuv_top_imgw_m1               yuv_top_imgw_m1;
	uint32_t                                _resv_0x68[1];
	union reg_yuv_top_stvalid_status        stvalid_status;
	union reg_yuv_top_stready_status        stready_status;
	union reg_yuv_top_patgen1               patgen1;
	union reg_yuv_top_patgen2               patgen2;
	union reg_yuv_top_patgen3               patgen3;
	union reg_yuv_top_patgen4               patgen4;
	union reg_yuv_top_check_sum             check_sum;
	union reg_yuv_top_ai_isp_rdma_ctrl      ai_isp_rdma_ctrl;
	union reg_yuv_top_ai_isp_img_size_y     ai_isp_img_size_y;
	union reg_yuv_top_ai_isp_w_crop_y       ai_isp_w_crop_y;
	union reg_yuv_top_ai_isp_h_crop_y       ai_isp_h_crop_y;
	union reg_yuv_top_ai_isp_img_size_uv    ai_isp_img_size_uv;
	union reg_yuv_top_ai_isp_w_crop_uv      ai_isp_w_crop_uv;
	union reg_yuv_top_ai_isp_h_crop_uv      ai_isp_h_crop_uv;
	union reg_yuv_top_yuv_ctrl_mars3        yuv_ctrl_mars3;
};

#ifdef __cplusplus
}
#endif

#endif /* _VI_REG_BLOCKS_H_ */

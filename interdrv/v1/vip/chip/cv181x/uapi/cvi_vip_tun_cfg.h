/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_tun_cfg.h
 * Description:
 */

#ifndef _U_CVI_VIP_TUN_CFG_H_
#define _U_CVI_VIP_TUN_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "isp_reg.h"

#define TUNING_NODE_NUM  2

enum cvi_isp_raw {
	ISP_PRERAW_A,
	ISP_PRERAW_B,
	ISP_PRERAW_MAX,
};

enum cvi_isp_chn_num {
	ISP_CHN0,
	ISP_CHN1,
	ISP_CHN2,
	ISP_CHN3,
	ISP_CHN_MAX,
};

enum cvi_isp_pre_chn_num {
	ISP_FE_CH0,
	ISP_FE_CH1,
	ISP_FE_CH2,
	ISP_FE_CH3,
	ISP_FE_CHN_MAX,
};

enum cvi_isp_be_chn_num {
	ISP_BE_CH0,
	ISP_BE_CH1,
	ISP_BE_CHN_MAX,
};

struct cvi_vip_isp_blc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  bypass;
	__u16 roffset;
	__u16 groffset;
	__u16 gboffset;
	__u16 boffset;
	__u16 roffset_2nd;
	__u16 groffset_2nd;
	__u16 gboffset_2nd;
	__u16 boffset_2nd;
	__u16 rgain;
	__u16 grgain;
	__u16 gbgain;
	__u16 bgain;
};

struct cvi_vip_isp_wbg_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  bypass;
	__u16 rgain;
	__u16 ggain;
	__u16 bgain;
	__u32 rgain_fraction;
	__u32 ggain_fraction;
	__u32 bgain_fraction;
};


/* struct cvi_vip_isp_ccm_config
 * @enable: ccm module enable or not
 * @coef: s3.10, (2's complement)
 */
struct cvi_vip_isp_ccm_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 coef[3][3];
};

struct cvi_vip_isp_dhz_luma_tun_cfg {
	union REG_ISP_DHZ_9  LUMA_00;
	union REG_ISP_DHZ_10 LUMA_04;
	union REG_ISP_DHZ_11 LUMA_08;
	union REG_ISP_DHZ_12 LUMA_12;
	union REG_ISP_DHZ_13 LUMA_16;
	union REG_ISP_DHZ_14 LUMA_20;
	union REG_ISP_DHZ_15 LUMA_24;
	union REG_ISP_DHZ_16 LUMA_28;
};

struct cvi_vip_isp_dhz_skin_tun_cfg {
	union REG_ISP_DHZ_17 SKIN_00;
	union REG_ISP_DHZ_18 SKIN_04;
	union REG_ISP_DHZ_19 SKIN_08;
	union REG_ISP_DHZ_20 SKIN_12;
	union REG_ISP_DHZ_21 SKIN_16;
	union REG_ISP_DHZ_22 SKIN_20;
	union REG_ISP_DHZ_23 SKIN_24;
	union REG_ISP_DHZ_24 SKIN_28;
};

/* struct cvi_vip_isp_dhz_config
 * @param strength:  (0~127) dehaze strength
 * @param th_smooth: (0~1023) threshold for edge/smooth classification.
 */
struct cvi_vip_isp_dhz_config {
	__u8  update;
	__u8  enable;
	__u8  strength;
	__u16 th_smooth;
	__u16 cum_th;
	__u16 hist_th;
	__u16 tmap_min;
	__u16 tmap_max;
	__u8  luma_lut_enable;
	__u8  skin_lut_enable;
	__u8  skin_cb;
	__u8  skin_cr;
	struct cvi_vip_isp_dhz_luma_tun_cfg luma_cfg;
	struct cvi_vip_isp_dhz_skin_tun_cfg skin_cfg;
};

struct cvi_isp_ge_tun_cfg {
	union REG_ISP_DPC_10                    DPC_10;
	union REG_ISP_DPC_11                    DPC_11;
	union REG_ISP_DPC_12                    DPC_12;
	union REG_ISP_DPC_13                    DPC_13;
	union REG_ISP_DPC_14                    DPC_14;
	union REG_ISP_DPC_15                    DPC_15;
	union REG_ISP_DPC_16                    DPC_16;
};

struct cvi_vip_isp_ge_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	struct cvi_isp_ge_tun_cfg ge_cfg;
};

struct cvi_vip_isp_gamma_config {
	__u8  update;
	__u8  enable;
	__u8  prog_1to3_enable;
	__u16 lut_r[256];
	__u16 lut_g[256];
	__u16 lut_b[256];
};

struct cvi_isp_ee_tun_cfg {
	union REG_ISP_EE_08                     REG_08;
	union REG_ISP_EE_0C                     REG_0C;
	union REG_ISP_EE_10                     REG_10;
	union REG_ISP_EE_14                     REG_14;
	union REG_ISP_EE_18                     REG_18;
	union REG_ISP_EE_1C                     REG_1C;
	union REG_ISP_EE_20                     REG_20;
	union REG_ISP_EE_24                     REG_24;
};

struct cvi_isp_ee_tun_1_cfg {
	union REG_ISP_EE_30                     REG_30;
	union REG_ISP_EE_34                     REG_34;
	union REG_ISP_EE_38                     REG_38;
	union REG_ISP_EE_3C                     REG_3C;
	union REG_ISP_EE_40                     REG_40;
	uint32_t                                _resv_0x44[5];
	union REG_ISP_EE_58                     REG_58;
	union REG_ISP_EE_5C                     REG_5C;
	union REG_ISP_EE_60                     REG_60;
	union REG_ISP_EE_64                     REG_64;
	union REG_ISP_EE_68                     REG_68;
	union REG_ISP_EE_6C                     REG_6C;
	union REG_ISP_EE_70                     REG_70;
	union REG_ISP_EE_74                     REG_74;
	union REG_ISP_EE_78                     REG_78;
	union REG_ISP_EE_7C                     REG_7C;
	union REG_ISP_EE_80                     REG_80;
	union REG_ISP_EE_84                     REG_84;
	union REG_ISP_EE_88                     REG_88;
	union REG_ISP_EE_8C                     REG_8C;
	union REG_ISP_EE_90                     REG_90;
	union REG_ISP_EE_94                     REG_94;
	union REG_ISP_EE_98                     REG_98;
	union REG_ISP_EE_9C                     REG_9C;
};

struct cvi_isp_ee_tun_2_cfg {
	union REG_ISP_EE_1C8                    REG_1C8;
	union REG_ISP_EE_1CC                    REG_1CC;
	union REG_ISP_EE_1D0                    REG_1D0;
	union REG_ISP_EE_1D4                    REG_1D4;
	union REG_ISP_EE_1D8                    REG_1D8;
};

struct cvi_vip_isp_ee_config {
	__u8  update;
	__u8  enable;
	__u8  dbg_mode;
	__u16 total_coring;
	__u8  total_gain;
	__u8  total_oshtthrd;
	__u8  total_ushtthrd;
	__u8  debug_shift_bit;
	__u8  pre_proc_blending;
	__u8  pre_proc_gain;
	__u8  pre_proc_mode;
	__u8  pre_proc_enable;
	__u8  shtctrl_oshtgain;
	__u8  shtctrl_ushtgain;
	__u16 dgr4_nod_norgain;
	__u8  dgr4_dir_adjgain;
	__u16 luma_coring_lut[33];
	__u8  luma_adptctrl_lut[33];
	__u8  delta_adptctrl_lut[33];
	__u8  luma_shtctrl_lut[33];
	__u8  delta_shtctrl_lut[33];
	struct cvi_isp_ee_tun_cfg ee_cfg;
	struct cvi_isp_ee_tun_1_cfg ee_1_cfg;
	struct cvi_isp_ee_tun_2_cfg ee_2_cfg;
};

struct cvi_vip_isp_preyee_config {
	__u8  update;
	__u8  enable;
	__u8  dbg_mode;
	__u16 total_coring;
	__u8  total_gain;
	__u8  total_oshtthrd;
	__u8  total_ushtthrd;
	__u8  debug_shift_bit;
	__u8  pre_proc_gain;
	__u8  pre_proc_mode;
	__u8  pre_proc_enable;
	__u8  shtctrl_oshtgain;
	__u8  shtctrl_ushtgain;
	__u16 dgr4_nod_norgain;
	__u8  dgr4_dir_adjgain;
	__u16 luma_coring_lut[33];
	__u8  luma_adptctrl_lut[33];
	__u8  delta_adptctrl_lut[33];
	__u8  luma_shtctrl_lut[33];
	__u8  delta_shtctrl_lut[33];
	struct cvi_isp_ee_tun_cfg preyee_cfg;
	struct cvi_isp_ee_tun_1_cfg preyee_1_cfg;
	struct cvi_isp_ee_tun_2_cfg preyee_2_cfg;
};

struct cvi_isp_bnr_tun_1_cfg {
	union REG_ISP_BNR_NS_LUMA_TH_R          NS_LUMA_TH_R;
	union REG_ISP_BNR_NS_SLOPE_R            NS_SLOPE_R;
	union REG_ISP_BNR_NS_OFFSET0_R          NS_OFFSET0_R;
	union REG_ISP_BNR_NS_OFFSET1_R          NS_OFFSET1_R;
	union REG_ISP_BNR_NS_LUMA_TH_GR         NS_LUMA_TH_GR;
	union REG_ISP_BNR_NS_SLOPE_GR           NS_SLOPE_GR;
	union REG_ISP_BNR_NS_OFFSET0_GR         NS_OFFSET0_GR;
	union REG_ISP_BNR_NS_OFFSET1_GR         NS_OFFSET1_GR;
	union REG_ISP_BNR_NS_LUMA_TH_GB         NS_LUMA_TH_GB;
	union REG_ISP_BNR_NS_SLOPE_GB           NS_SLOPE_GB;
	union REG_ISP_BNR_NS_OFFSET0_GB         NS_OFFSET0_GB;
	union REG_ISP_BNR_NS_OFFSET1_GB         NS_OFFSET1_GB;
	union REG_ISP_BNR_NS_LUMA_TH_B          NS_LUMA_TH_B;
	union REG_ISP_BNR_NS_SLOPE_B            NS_SLOPE_B;
	union REG_ISP_BNR_NS_OFFSET0_B          NS_OFFSET0_B;
	union REG_ISP_BNR_NS_OFFSET1_B          NS_OFFSET1_B;
	union REG_ISP_BNR_NS_GAIN               NS_GAIN;
	union REG_ISP_BNR_STRENGTH_MODE         STRENGTH_MODE;
};

struct cvi_isp_bnr_tun_2_cfg {
	union REG_ISP_BNR_LSC_RATIO             LSC_RATIO;
	union REG_ISP_BNR_VAR_TH                VAR_TH;
	union REG_ISP_BNR_WEIGHT_LUT            WEIGHT_LUT;
	union REG_ISP_BNR_WEIGHT_SM             WEIGHT_SM;
	union REG_ISP_BNR_WEIGHT_V              WEIGHT_V;
	union REG_ISP_BNR_WEIGHT_H              WEIGHT_H;
	union REG_ISP_BNR_WEIGHT_D45            WEIGHT_D45;
	union REG_ISP_BNR_WEIGHT_D135           WEIGHT_D135;
	union REG_ISP_BNR_NEIGHBOR_MAX          NEIGHBOR_MAX;
};

struct cvi_vip_isp_bnr_config {
	__u8  update;
	__u8  enable;
	__u8  out_sel;
	__u8  weight_lut[256];
	__u8  intensity_sel[8];
	__u8  weight_intra_0;
	__u8  weight_intra_1;
	__u8  weight_intra_2;
	__u8  weight_norm_1;
	__u8  weight_norm_2;
	__u16 k_smooth;
	__u16 k_texture;
	__u8  lsc_en;
	__u8  lsc_strenth;
	__u16 lsc_centerx;
	__u16 lsc_centery;
	__u16 lsc_norm;
	__u8  lsc_gain_lut[32];
	struct cvi_isp_bnr_tun_1_cfg bnr_1_cfg;
	struct cvi_isp_bnr_tun_2_cfg bnr_2_cfg;
};

struct cvi_vip_isp_cnr_config {
	__u8  update;
	__u8  enable;
	__u8  strength_mode;
	__u8  diff_shift_val;
	__u8  diff_gain;
	__u8  ratio;
	__u8  fusion_intensity_weight;
	__u8  flag_neighbor_max_weight;
	__u8  weight_lut_inter[16];
	__u8  intensity_sel[8];
	__u8  motion_enable;
	__u8  motion_lut[16];
};

struct cvi_isp_ynr_tun_1_cfg {
	union REG_ISP_YNR_MOTION_NS_TH          MOTION_NS_TH;
	union REG_ISP_YNR_MOTION_POS_GAIN       MOTION_POS_GAIN;
	union REG_ISP_YNR_MOTION_NEG_GAIN       MOTION_NEG_GAIN;
	union REG_ISP_YNR_NS_GAIN               NS_GAIN;
//	union REG_ISP_YNR_STRENGTH_MODE         STRENGTH_MODE;
};

struct cvi_isp_ynr_tun_2_cfg {
	union REG_ISP_YNR_WEIGHT_SM             WEIGHT_SM;
	union REG_ISP_YNR_WEIGHT_V              WEIGHT_V;
	union REG_ISP_YNR_WEIGHT_H              WEIGHT_H;
	union REG_ISP_YNR_WEIGHT_D45            WEIGHT_D45;
	union REG_ISP_YNR_WEIGHT_D135           WEIGHT_D135;
	union REG_ISP_YNR_NEIGHBOR_MAX          NEIGHBOR_MAX;
};

struct cvi_vip_isp_ynr_config {
	__u8  update;
	__u8  enable;
	__u8  weight_intra_0;
	__u8  weight_intra_1;
	__u8  weight_intra_2;
	__u8  weight_norm_1;
	__u8  weight_norm_2;
	__u8  weight_lut_h[64];
	__u8  intensity_sel[8];
	__u8  out_sel;
	__u8  var_th;
	__u16 k_smooth;
	__u16 k_texture;
	__u16 alpha_gain;
	__s16 ns0_slope[5];
	__u8  ns0_luma_th[6];
	__u8  ns0_offset_th[6];
	__s16 ns1_slope[5];
	__u8  ns1_luma_th[6];
	__u8  ns1_offset_th[6];
	__u8  filter_mode_enable;
	__u16 filter_mode_alpha;
	__u8  motion_ns_clip_max;
	__u8  res_max;
	__u8  res_motion_max;
	__u16 res_mot_lut[16];
	__u16 motion_lut[16];
	struct cvi_isp_ynr_tun_1_cfg ynr_1_cfg;
	struct cvi_isp_ynr_tun_2_cfg ynr_2_cfg;
};

struct cvi_isp_tnr_tun_cfg {
	union REG_ISP_MMAP_0C                     REG_0C;
	union REG_ISP_MMAP_10                     REG_10;
	union REG_ISP_MMAP_14                     REG_14;
	union REG_ISP_MMAP_18                     REG_18;
};

struct cvi_isp_tnr_tun_5_cfg {
	union REG_ISP_MMAP_20                     REG_20;
	union REG_ISP_MMAP_24                     REG_24;
	union REG_ISP_MMAP_28                     REG_28;
	union REG_ISP_MMAP_2C                     REG_2C;
};

struct cvi_isp_tnr_tun_1_cfg {
	union REG_ISP_MMAP_4C                     REG_4C;
	union REG_ISP_MMAP_50                     REG_50;
	union REG_ISP_MMAP_54                     REG_54;
	union REG_ISP_MMAP_58                     REG_58;
	union REG_ISP_MMAP_5C                     REG_5C;
};

struct cvi_isp_tnr_tun_2_cfg {
	union REG_ISP_MMAP_70                     REG_70;
	union REG_ISP_MMAP_74                     REG_74;
	union REG_ISP_MMAP_78                     REG_78;
	union REG_ISP_MMAP_7C                     REG_7C;
	union REG_ISP_MMAP_80                     REG_80;
	union REG_ISP_MMAP_84                     REG_84;
	union REG_ISP_MMAP_88                     REG_88;
	union REG_ISP_MMAP_8C                     REG_8C;
	union REG_ISP_MMAP_90                     REG_90;
};

struct cvi_isp_tnr_tun_3_cfg {
	union REG_ISP_MMAP_A0                     REG_A0;
	union REG_ISP_MMAP_A4                     REG_A4;
	union REG_ISP_MMAP_A8                     REG_A8;
	union REG_ISP_MMAP_AC                     REG_AC;
	union REG_ISP_MMAP_B0                     REG_B0;
	union REG_ISP_MMAP_B4                     REG_B4;
	union REG_ISP_MMAP_B8                     REG_B8;
	union REG_ISP_MMAP_BC                     REG_BC;
	union REG_ISP_MMAP_C0                     REG_C0;
};

struct cvi_isp_tnr_tun_4_cfg {
	union REG_ISP_444_422_13                REG_13;
	union REG_ISP_444_422_14                REG_14;
	union REG_ISP_444_422_15                REG_15;
	union REG_ISP_444_422_16                REG_16;
	union REG_ISP_444_422_17                REG_17;
	union REG_ISP_444_422_18                REG_18;
	union REG_ISP_444_422_19                REG_19;
	union REG_ISP_444_422_20                REG_20;
	union REG_ISP_444_422_21                REG_21;
	union REG_ISP_444_422_22                REG_22;
	union REG_ISP_444_422_23                REG_23;
	union REG_ISP_444_422_24                REG_24;
	union REG_ISP_444_422_25                REG_25;
	union REG_ISP_444_422_26                REG_26;
	union REG_ISP_444_422_27                REG_27;
	union REG_ISP_444_422_28                REG_28;
	union REG_ISP_444_422_29                REG_29;
	union REG_ISP_444_422_30                REG_30;
	union REG_ISP_444_422_31                REG_31;
};

struct cvi_vip_isp_tnr_config {
	__u8  update;
	__u8  manr_enable;
	__u8  rgbmap_w_bit;
	__u8  rgbmap_h_bit;
	__u8  mh_wgt;
	__u8  lpf[3][3];
	__u8  map_gain;
	__u8  map_thd_l;
	__u8  map_thd_h;
	__u8  uv_rounding_type_sel;
	__u8  history_sel_0;
	__u8  history_sel_1;
	__u8  history_sel_3;
//	__s16 mscaler_filter[260];
	__u16 tdnr_debug_sel;
	__s16 luma_adapt_lut_slope_2;
	__u8  med_enable;
	__u16 med_wgt;
	__u8  mtluma_mode;
	__u8  avg_mode_write;
	__u8  drop_mode_write;
	struct cvi_isp_tnr_tun_cfg	tnr_cfg;
	struct cvi_isp_tnr_tun_1_cfg	tnr_1_cfg;
	struct cvi_isp_tnr_tun_2_cfg	tnr_2_cfg;
	struct cvi_isp_tnr_tun_3_cfg	tnr_3_cfg;
	struct cvi_isp_tnr_tun_4_cfg	tnr_4_cfg;
	struct cvi_isp_tnr_tun_5_cfg	tnr_5_cfg;
};

struct cvi_vip_isp_dci_config {
	__u8  update;
	__u8  enable;
	__u8  map_enable;
	__u8  dither_enable;
	__u8  demo_mode;
	__u16 map_lut[256];
	__u8  per1sample_enable;
	__u8  hist_enable;
};

struct cvi_isp_demosiac_tun_cfg {
	union REG_ISP_CFA_3                     REG_3;
	union REG_ISP_CFA_4                     REG_4;
	union REG_ISP_CFA_4_1                   REG_4_1;
	union REG_ISP_CFA_5                     REG_5;
};

struct cvi_isp_demosiac_tun_1_cfg {
	union REG_ISP_CFA_10                    REG_10;
	union REG_ISP_CFA_11                    REG_11;
	union REG_ISP_CFA_12                    REG_12;
	union REG_ISP_CFA_13                    REG_13;
	union REG_ISP_CFA_14                    REG_14;
	union REG_ISP_CFA_15                    REG_15;
	union REG_ISP_CFA_16                    REG_16;
};

struct cvi_isp_demosiac_tun_2_cfg {
	union REG_ISP_CFA_22                    REG_22;
	union REG_ISP_CFA_23                    REG_23;
	union REG_ISP_CFA_24                    REG_24;
	union REG_ISP_CFA_25                    REG_25;
	union REG_ISP_CFA_26                    REG_26;
	union REG_ISP_CFA_27                    REG_27;
};

struct cvi_vip_isp_demosiac_config {
	__u8  update;
	__u8  cfa_enable;
	__u8  cfa_moire_enable;
	__u8  cfa_out_sel;
	__u8  cfa_ghp_lut[32];
	struct cvi_isp_demosiac_tun_cfg demosiac_cfg;
	struct cvi_isp_demosiac_tun_1_cfg demosiac_1_cfg;
	struct cvi_isp_demosiac_tun_2_cfg demosiac_2_cfg;
};

struct cvi_isp_rgbcac_tun_cfg {
	union REG_ISP_CFA_30                    REG_30;
	union REG_ISP_CFA_31                    REG_31;
	union REG_ISP_CFA_32                    REG_32;
	union REG_ISP_CFA_33                    REG_33;
};

struct cvi_vip_isp_rgbcac_config {
	__u8  update;
	__u8  enable;
	__u8  out_sel;
	__u16 var_th;
	__u8  purple_th;
	__u8  correct_strength;
	struct cvi_isp_rgbcac_tun_cfg rgbcac_cfg;
};

struct cvi_vip_isp_3dlut_config {
	__u8  update;
	__u8  enable;
	__u8  h_clamp_wrap_opt;
	__u16 h_lut[3276];
	__u16 s_lut[3276];
	__u16 v_lut[3276];
};

struct cvi_isp_dpc_tun_cfg {
	union REG_ISP_DPC_3                     DPC_3;
	union REG_ISP_DPC_4                     DPC_4;
	union REG_ISP_DPC_5                     DPC_5;
	union REG_ISP_DPC_6                     DPC_6;
	union REG_ISP_DPC_7                     DPC_7;
	union REG_ISP_DPC_8                     DPC_8;
	union REG_ISP_DPC_9                     DPC_9;
};

struct cvi_isp_dpc_ir_cfg {
	union REG_ISP_DPC_24                    DPC_IR_RATIO;
	union REG_ISP_DPC_25                    DPC_IR_PARAM;
};

struct cvi_vip_isp_dpc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  staticbpc_enable;
	__u8  dynamicbpc_enable;
	__u8  cluster_size;
	__u32 bp_tbl[2047];
	__u16 bp_cnt;
	struct cvi_isp_dpc_tun_cfg dpc_cfg;
	struct cvi_isp_dpc_ir_cfg ir_cfg;
};

struct cvi_vip_isp_lsc_config {
	__u8  update;
	__u8  enable;
	__u16 strength;
	__u8  debug;
	__u8  gain_base;
	__u8  gain_3p9_0_4p8_1;
	__u8  renormalize_enable;
	__u8  gain_bicubic_0_bilinear_1;
	__u8  boundary_interpolation_mode;
	__u8  boundary_interpolation_lf_range;
	__u8  boundary_interpolation_up_range;
	__u8  boundary_interpolation_rt_range;
	__u8  boundary_interpolation_dn_range;
};

struct cvi_vip_isp_lscr_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 strength;
	__u16 strength_ir;
	__u16 norm;
	__u16 norm_ir;
	__u16 centerx;
	__u16 centery;
	__u16 gain_lut[32];
	__u16 gain_lut1[32];
	__u16 gain_lut2[32];
	__u16 gain_lut_ir[32];
};

struct cvi_vip_isp_ae_config {
	__u8  update;
	__u8  inst;
	__u8  face_ae_enable;
	__u8  ae_enable;
	__u8  ae_gain_enable;
	__u8  hist_enable;
	__u8  hist_gain_enable;
	__u8  ir_ae_enable;
	__u8  ir_ae_gain_enable;
	__u8  ir_hist_enable;
	__u8  ir_hist_gain_enable;
	__u16 ae_offsetx;
	__u16 ae_offsety;
	__u8  ae_numx;
	__u8  ae_numy;
	__u16 ae_width;
	__u16 ae_height;
	__u8  ae_sts_div;
	__u16 ir_ae_offsetx;
	__u16 ir_ae_offsety;
	__u8  ir_ae_numx;
	__u8  ir_ae_numy;
	__u16 ir_ae_width;
	__u16 ir_ae_height;
	__u8  ir_ae_sts_div;
	__u16 ae_hist_bayer_start;
	__u16 ae_wbg_rgain;
	__u16 ae_wbg_ggain;
	__u16 ae_wbg_bgain;
	__u16 ae1_wbg_bgain;
	__u16 ae1_wbg_rgain;
	__u16 ae1_wbg_ggain;
	__u16 ae_wbg_vgain;
	__u16 ae_face_offset_x[4];
	__u16 ae_face_offset_y[4];
	__u8  ae_face_size_minus1_x[4];
	__u8  ae_face_size_minus1_y[4];
	__u16 ir_ae_face_offset_x[4];
	__u16 ir_ae_face_offset_y[4];
	__u8  ir_ae_face_size_minus1_x[4];
	__u8  ir_ae_face_size_minus1_y[4];
};

struct cvi_vip_isp_awb_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  bayer_start;
	__u16 awb_offsetx;
	__u16 awb_offsety;
	__u16 awb_sub_win_w;
	__u16 awb_sub_win_h;
	__u8  awb_numx;
	__u8  awb_numy;
	__u8  corner_avg_en;
	__u8  corner_size;
	__u8  awb_sts_div;
	__u16 r_lower_bound;
	__u16 r_upper_bound;
	__u16 g_lower_bound;
	__u16 g_upper_bound;
	__u16 b_lower_bound;
	__u16 b_upper_bound;
};

struct cvi_vip_isp_af_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  gamma_enable;
	__u8  dpc_enable;
	__u16 offsetx;
	__u16 offsety;
	__u16 block_width;
	__u16 block_height;
	__u8  block_numx;
	__u8  block_numy;
	__u8  h_low_pass_value_shift;
	__u32 h_corning_offset_0;
	__u32 h_corning_offset_1;
	__u16 v_corning_offset;
	__u16 high_luma_threshold;
	__u8  h_low_pass_coef[5];
	__u8  h_high_pass_coef_0[5];
	__u8  h_high_pass_coef_1[5];
	__u8  v_high_pass_coef[3];
	__u8  th_low;
	__u8  th_high;
	__u8  gain_low;
	__u8  gain_high;
	__u8  slop_low;
	__u8  slop_high;
};

struct cvi_isp_fswdr_tun_cfg {
	union REG_ISP_FUSION_FS_SE_GAIN		FS_LS_GAIN;
	union REG_ISP_FUSION_FS_LUMA_THD        FS_LUMA_THD;
	union REG_ISP_FUSION_FS_WGT             FS_WGT;
	union REG_ISP_FUSION_FS_WGT_SLOPE       FS_WGT_SLOPE;
};

struct cvi_isp_fswdr_tun_1_cfg {
	union REG_ISP_FUSION_FS_LUMA_THD_SE     FS_LUMA_THD_SE;
	union REG_ISP_FUSION_FS_WGT_SE          FS_WGT_SE;
	union REG_ISP_FUSION_FS_WGT_SLOPE_SE    FS_WGT_SLOPE_SE;
};

struct cvi_isp_fswdr_tun_2_cfg {
	union REG_ISP_FUSION_FS_DITHER          FS_DITHER;
	union REG_ISP_FUSION_FS_CALIB_CTRL_0    FS_CALIB_LUMA;
	union REG_ISP_FUSION_FS_CALIB_CTRL_1    FS_CALIB_DIF_TH;
	union REG_ISP_FUSION_FS_SE_FIX_OFFSET_0 FS_SE_FIX_OFFSET_R;
	union REG_ISP_FUSION_FS_SE_FIX_OFFSET_1 FS_SE_FIX_OFFSET_G;
	union REG_ISP_FUSION_FS_SE_FIX_OFFSET_2 FS_SE_FIX_OFFSET_B;
};

struct cvi_isp_fswdr_tun_3_cfg {
	union REG_ISP_FUSION_FS_MOTION_LUT_IN       FS_MOTION_LUT_IN;
	union REG_ISP_FUSION_FS_MOTION_LUT_OUT_0    FS_MOTION_LUT_OUT_0;
	union REG_ISP_FUSION_FS_MOTION_LUT_OUT_1    FS_MOTION_LUT_OUT_1;
	union REG_ISP_FUSION_FS_MOTION_LUT_SLOPE_0  FS_MOTION_LUT_SLOPE_0;
	union REG_ISP_FUSION_FS_MOTION_LUT_SLOPE_1  FS_MOTION_LUT_SLOPE_1;
};

struct cvi_vip_isp_fswdr_config {
	__u8  update;
	__u8  enable;
	__u8  mc_enable;
	__u8  out_sel;
	__u8  motion_ls_mode;
	__u8  mmap_mrg_mode;
	__u8  mmap_mrg_alph;
	__u8  mmap_1_enable;
	__u8  luma_mode;
	__u8  fusion_type;
	__u8  history_sel_2;
	__u16 fusion_lwgt;
	__u32 s_max;
	__u16 mmap_v_thd_l;
	__u16 mmap_v_thd_h;
	__u16 mmap_v_wgt_min;
	__u16 mmap_v_wgt_max;
	__s32 mmap_v_wgt_slp;
	struct cvi_isp_fswdr_tun_cfg fswdr_cfg;
	struct cvi_isp_fswdr_tun_1_cfg fswdr_1_cfg;
	struct cvi_isp_fswdr_tun_2_cfg fswdr_2_cfg;
	struct cvi_isp_fswdr_tun_3_cfg fswdr_3_cfg;
};

struct cvi_vip_isp_fswdr_report {
	uint32_t cal_pix_num;
	int32_t diff_sum_r;
	int32_t diff_sum_g;
	int32_t diff_sum_b;
};

struct cvi_isp_drc_tun_1_cfg {
	union REG_ISP_LTM_BE_STRTH_CTRL         LTM_BE_STRTH_CTRL;
	union REG_ISP_LTM_DE_STRTH_CTRL         LTM_DE_STRTH_CTRL;
	union REG_ISP_LTM_FILTER_WIN_SIZE_CTRL  LTM_FILTER_WIN_SIZE_CTRL;
	union REG_ISP_LTM_BGAIN_CTRL_0          LTM_BGAIN_CTRL_0;
	union REG_ISP_LTM_BGAIN_CTRL_1          LTM_BGAIN_CTRL_1;
	union REG_ISP_LTM_DGAIN_CTRL_0          LTM_DGAIN_CTRL_0;
	union REG_ISP_LTM_DGAIN_CTRL_1          LTM_DGAIN_CTRL_1;
	union REG_ISP_LTM_BRI_LCE_CTRL_0        LTM_BRI_LCE_CTRL_0;
	union REG_ISP_LTM_BRI_LCE_CTRL_1        LTM_BRI_LCE_CTRL_1;
	union REG_ISP_LTM_BRI_LCE_CTRL_2        LTM_BRI_LCE_CTRL_2;
	union REG_ISP_LTM_BRI_LCE_CTRL_3        LTM_BRI_LCE_CTRL_3;
	union REG_ISP_LTM_BRI_LCE_CTRL_4        LTM_BRI_LCE_CTRL_4;
	union REG_ISP_LTM_BRI_LCE_CTRL_5        LTM_BRI_LCE_CTRL_5;
	union REG_ISP_LTM_BRI_LCE_CTRL_6        LTM_BRI_LCE_CTRL_6;
	union REG_ISP_LTM_BRI_LCE_CTRL_7        LTM_BRI_LCE_CTRL_7;
	union REG_ISP_LTM_BRI_LCE_CTRL_8        LTM_BRI_LCE_CTRL_8;
	union REG_ISP_LTM_BRI_LCE_CTRL_9        LTM_BRI_LCE_CTRL_9;
	union REG_ISP_LTM_BRI_LCE_CTRL_10       LTM_BRI_LCE_CTRL_10;
	union REG_ISP_LTM_BRI_LCE_CTRL_11       LTM_BRI_LCE_CTRL_11;
	union REG_ISP_LTM_BRI_LCE_CTRL_12       LTM_BRI_LCE_CTRL_12;
	union REG_ISP_LTM_BRI_LCE_CTRL_13       LTM_BRI_LCE_CTRL_13;
	union REG_ISP_LTM_DAR_LCE_CTRL_0        LTM_DAR_LCE_CTRL_0;
	union REG_ISP_LTM_DAR_LCE_CTRL_1        LTM_DAR_LCE_CTRL_1;
	union REG_ISP_LTM_DAR_LCE_CTRL_2        LTM_DAR_LCE_CTRL_2;
	union REG_ISP_LTM_DAR_LCE_CTRL_3        LTM_DAR_LCE_CTRL_3;
	union REG_ISP_LTM_DAR_LCE_CTRL_4        LTM_DAR_LCE_CTRL_4;
	union REG_ISP_LTM_DAR_LCE_CTRL_5        LTM_DAR_LCE_CTRL_5;
	union REG_ISP_LTM_DAR_LCE_CTRL_6        LTM_DAR_LCE_CTRL_6;
};

struct cvi_isp_drc_tun_2_cfg {
	union REG_ISP_LTM_LMAP_COMPUTE_CTRL_0   LTM_LMAP_COMPUTE_CTRL_0;
	union REG_ISP_LTM_EE_CTRL_0             LTM_EE_CTRL_0;
	union REG_ISP_LTM_EE_CTRL_1             LTM_EE_CTRL_1;
	union REG_ISP_LTM_EE_DETAIL_LUT_00      LTM_EE_DETAIL_LUT_00;
	union REG_ISP_LTM_EE_DETAIL_LUT_01      LTM_EE_DETAIL_LUT_01;
	union REG_ISP_LTM_EE_DETAIL_LUT_02      LTM_EE_DETAIL_LUT_02;
	union REG_ISP_LTM_EE_DETAIL_LUT_03      LTM_EE_DETAIL_LUT_03;
	union REG_ISP_LTM_EE_DETAIL_LUT_04      LTM_EE_DETAIL_LUT_04;
	union REG_ISP_LTM_EE_DETAIL_LUT_05      LTM_EE_DETAIL_LUT_05;
	union REG_ISP_LTM_EE_DETAIL_LUT_06      LTM_EE_DETAIL_LUT_06;
	union REG_ISP_LTM_EE_DETAIL_LUT_07      LTM_EE_DETAIL_LUT_07;
	union REG_ISP_LTM_EE_DETAIL_LUT_08      LTM_EE_DETAIL_LUT_08;
};

struct cvi_isp_drc_tun_3_cfg {
	union REG_ISP_LTM_EE_GAIN_LUT_00        LTM_EE_GAIN_LUT_00;
	union REG_ISP_LTM_EE_GAIN_LUT_01        LTM_EE_GAIN_LUT_01;
	union REG_ISP_LTM_EE_GAIN_LUT_02        LTM_EE_GAIN_LUT_02;
	union REG_ISP_LTM_EE_GAIN_LUT_03        LTM_EE_GAIN_LUT_03;
	union REG_ISP_LTM_EE_GAIN_LUT_04        LTM_EE_GAIN_LUT_04;
	union REG_ISP_LTM_EE_GAIN_LUT_05        LTM_EE_GAIN_LUT_05;
	union REG_ISP_LTM_EE_GAIN_LUT_06        LTM_EE_GAIN_LUT_06;
	union REG_ISP_LTM_EE_GAIN_LUT_07        LTM_EE_GAIN_LUT_07;
	union REG_ISP_LTM_EE_GAIN_LUT_08        LTM_EE_GAIN_LUT_08;
	union REG_ISP_LTM_SAT_ADJ               LTM_SAT_ADJ;
	union REG_ISP_LTM_HSV_S_BY_V_0          LTM_HSV_S_BY_V_0;
	union REG_ISP_LTM_HSV_S_BY_V_1          LTM_HSV_S_BY_V_1;
	union REG_ISP_LTM_HSV_S_BY_V_2          LTM_HSV_S_BY_V_2;
	union REG_ISP_LTM_HSV_S_BY_V_3          LTM_HSV_S_BY_V_3;
	union REG_ISP_LTM_HSV_S_BY_V_4          LTM_HSV_S_BY_V_4;
	union REG_ISP_LTM_HSV_S_BY_V_5          LTM_HSV_S_BY_V_5;
	union REG_ISP_LTM_STR_CTRL_0            LTM_STR_CTRL_0;
	union REG_ISP_LTM_STR_CTRL_1            LTM_STR_CTRL_1;
};

struct cvi_vip_isp_drc_config {
	__u8  update;
	__u8  ltm_enable;
	__u8  lmap_enable;
	__u8  dark_enh_en;
	__u8  brit_enh_en;
	__u8  dark_lce_en;
	__u8  brit_lce_en;
	__u8  dbg_en;
	__u8  dbg_mode;
	__u8  de_max_thr_en;
	__u8  bcrv_quan_bit;
	__u8  gcrv_quan_bit_1;
	__u8  lmap_w_bit;
	__u8  lmap_h_bit;
	__u16 lmap_thd_l;
	__u16 lmap_thd_h;
	__u8  lmap_y_mode;
	__u16 dark_lut[257];
	__u16 brit_lut[513];
	__u16 deflt_lut[769];
	__u16 de_max_thr;
	__u8  de_dist_wgt[11];
	__u8  de_luma_wgt[30];
	__u8  be_dist_wgt[11];
	__u8  lmap0_lp_dist_wgt[11];
	__u8  lmap0_lp_diff_wgt[30];
	__u8  lmap1_lp_dist_wgt[11];
	__u8  lmap1_lp_diff_wgt[30];
	struct cvi_isp_drc_tun_1_cfg drc_1_cfg;
	struct cvi_isp_drc_tun_2_cfg drc_2_cfg;
	struct cvi_isp_drc_tun_3_cfg drc_3_cfg;
};

struct cvi_vip_isp_mono_config {
	__u8  update;
	__u8  force_mono_enable;
};

struct cvi_vip_isp_hsv_config {
	__u8  update;
	__u8  enable;
	__u8  htune_enable;
	__u8  stune_enable;
	__u8  hsgain_enable;
	__u8  hvgain_enable;
	__u16 h_lut[769];
	__u16 s_lut[513];
	__u16 sgain_lut[769];
	__u16 vgain_lut[769];
};

struct cvi_vip_isp_gms_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 offset_x;
	__u16 offset_y;
	__u16 x_section_size;
	__u8  y_section_size;
	__u8  x_gap;
	__u8  y_gap;
};

struct cvi_vip_isp_preproc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__s16 r_ir_ratio[128];
	__s16 g_ir_ratio[128];
	__s16 b_ir_ratio[128];
	__u8  w_lut[128];
};

struct cvi_vip_isp_clut_config {
	__u8  update;
	__u8  enable;
	__u8  is_update_partial;
	__u16 r_lut[4913];
	__u16 g_lut[4913];
	__u16 b_lut[4913];
	__u16 update_length;
	__u32 lut[1024][2]; //0:addr, 1:value
};

struct cvi_vip_isp_csc_config {
	__u8  update;
	__u8  enable;
	__s16 coeff[9];
	__s16 offset[3];
};

struct cvi_isp_cac_tun_cfg {
	union REG_ISP_CNR_PURPLE_CR PURPLE_CR;
	union REG_ISP_CNR_GREEN_CR GREEN_CR;
};

struct cvi_isp_cac_2_tun_cfg {
	union REG_ISP_CNR_PURPLE_CB2 PURPLE_CB2;
	union REG_ISP_CNR_PURPLE_W1 PURPLE_W1;
};

struct cvi_vip_isp_cac_config {
	__u8  update;
	__u8  enable;
	__u8  out_sel;
	__u16 var_th;
	__u8  purple_th;
	__u8  correct_strength;
	__u8  purple_mode;
	struct cvi_isp_cac_tun_cfg cac_cfg;
	struct cvi_isp_cac_2_tun_cfg cac_2_cfg;
};

struct cvi_vip_isp_ca2_config {
	__u8  update;
	__u8  enable;
	__u16 lut_in[3];
	__u16 lut_out[3];
	__u16 lut_slp[2];
};

struct cvi_vip_isp_cacp_config {
	__u8  update;
	__u8  enable;
	__u8  mode;
	__u16 iso_ratio;
	__u16 ca_y_ratio_lut[256];
	__u8  cp_y_lut[256];
	__u8  cp_u_lut[256];
	__u8  cp_v_lut[256];
};

struct cvi_vip_isp_hist_edge_v_config {
	__u8  update;
	__u8  enable;
	__u8  luma_mode;
	__u16 offset_x;
	__u16 offset_y;
};

struct cvi_vip_isp_fe_tun_cfg {
	struct cvi_vip_isp_blc_config	blc_cfg[2];
	struct cvi_vip_isp_lscr_config	lscr_cfg[2];
	struct cvi_vip_isp_wbg_config	wbg_cfg[4];
};

struct cvi_vip_isp_ycur_config {
	__u8  update;
	__u8  enable;
	__u8  lut[64];
	__u16 lut_256;
};

struct cvi_vip_isp_be_tun_cfg {
	struct cvi_vip_isp_blc_config		blc_cfg[2];
	struct cvi_vip_isp_dpc_config		dpc_cfg[2];
	struct cvi_vip_isp_ge_config		ge_cfg[2];
	struct cvi_vip_isp_wbg_config		wbg_cfg[4];
	struct cvi_vip_isp_lscr_config		lscr_cfg[2];
	struct cvi_vip_isp_af_config		af_cfg;
	struct cvi_vip_isp_ae_config		ae_cfg[2];
	struct cvi_vip_isp_awb_config		awb_cfg;
	struct cvi_vip_isp_gms_config		gms_cfg;
	struct cvi_vip_isp_preproc_config	preproc_cfg;
};

struct cvi_vip_isp_post_tun_cfg {
	struct cvi_vip_isp_bnr_config		bnr_cfg;
	struct cvi_vip_isp_demosiac_config	demosiac_cfg;
	struct cvi_vip_isp_rgbcac_config	rgbcac_cfg;
	struct cvi_vip_isp_lsc_config		lsc_cfg;
	struct cvi_vip_isp_ccm_config		ccm_cfg[5];
	struct cvi_vip_isp_fswdr_config		fswdr_cfg;
	struct cvi_vip_isp_drc_config		drc_cfg;
	struct cvi_vip_isp_gamma_config		gamma_cfg;
	struct cvi_vip_isp_dhz_config		dhz_cfg;
	struct cvi_vip_isp_hsv_config		hsv_cfg;
	struct cvi_vip_isp_tnr_config		tnr_cfg;
	struct cvi_vip_isp_ynr_config		ynr_cfg;
	struct cvi_vip_isp_cnr_config		cnr_cfg;
	struct cvi_vip_isp_dci_config		dci_cfg;
	struct cvi_vip_isp_ee_config		ee_cfg;
	struct cvi_vip_isp_3dlut_config		thrdlut_cfg;
	struct cvi_vip_isp_mono_config		mono_cfg;
	struct cvi_vip_isp_clut_config		clut_cfg;
	struct cvi_vip_isp_csc_config		csc_cfg;
	struct cvi_vip_isp_preyee_config	preyee_cfg;
	struct cvi_vip_isp_cac_config		cac_cfg;
	struct cvi_vip_isp_ca2_config		ca2_cfg;
	struct cvi_vip_isp_cacp_config		cacp_cfg;
	struct cvi_vip_isp_hist_edge_v_config	hist_edge_v_cfg;
	struct cvi_vip_isp_ycur_config		ycur_cfg;
};

struct cvi_vip_isp_fe_cfg {
	uint8_t tun_update[TUNING_NODE_NUM];
	uint8_t tun_idx;
	struct cvi_vip_isp_fe_tun_cfg tun_cfg[TUNING_NODE_NUM];
};

struct cvi_vip_isp_be_cfg {
	uint8_t tun_update[TUNING_NODE_NUM];
	uint8_t tun_idx;
	struct cvi_vip_isp_be_tun_cfg tun_cfg[TUNING_NODE_NUM];
};

struct cvi_vip_isp_post_cfg {
	uint8_t tun_update[TUNING_NODE_NUM];
	uint8_t tun_idx;
	struct cvi_vip_isp_post_tun_cfg tun_cfg[TUNING_NODE_NUM];
};

struct isp_tuning_cfg {
	uint64_t  fe_addr[ISP_PRERAW_MAX];
	void	  *fe_vir[ISP_PRERAW_MAX];
#ifdef __arm__
	__u32	  fe_padding[ISP_PRERAW_MAX];
#endif
	uint64_t  be_addr[ISP_PRERAW_MAX];
	void	  *be_vir[ISP_PRERAW_MAX];
#ifdef __arm__
	__u32	  be_padding[ISP_PRERAW_MAX];
#endif
	uint64_t  post_addr[ISP_PRERAW_MAX];
	void      *post_vir[ISP_PRERAW_MAX];
#ifdef __arm__
	__u32	  post_padding[ISP_PRERAW_MAX];
#endif
};


#ifdef __cplusplus
}
#endif

#endif /* _U_CVI_VIP_TUN_CFG_H_ */

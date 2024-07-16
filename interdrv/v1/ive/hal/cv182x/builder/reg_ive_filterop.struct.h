// $Module: ive_filterop $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Mon, 13 Dec 2021 01:21:08 PM $
//

#ifndef __REG_IVE_FILTEROP_STRUCT_H__
#define __REG_IVE_FILTEROP_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		/*;*/
		uint32_t reg_softrst_wdma_y:1;
		/*;*/
		uint32_t reg_softrst_wdma_c:1;
		/*;*/
		uint32_t reg_softrst_rdma_gradfg:1;
		/*;*/
		uint32_t reg_softrst_op1:1;
		/*;*/
		uint32_t reg_softrst_filter3ch:1;
		/*;*/
		uint32_t reg_softrst_st:1;
		/*;*/
		uint32_t reg_softrst_odma:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_1_C;
typedef union {
	struct {
		/*set 1 as gradfg active to enable rdma  ;*/
		uint32_t reg_gradfg_bggrad_rdma_en:1;
		/*;*/
		uint32_t reg_gradfg_bggrad_uv_swap:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_H04_C;
typedef union {
	struct {
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_2_C;
typedef union {
	struct {
		/*dmy;*/
		uint32_t reg_ctrl_dmy1:32;
	};
	uint32_t val;
} IVE_FILTEROP_REG_3_C;
typedef union {
	struct {
		/*
		 MOD_BYP       = 0 ;
		 MOD_FILTER3CH = 1 ;
		 MOD_DILA      = 2 ;
		 MOD_ERO       = 3 ;
		 MOD_CANNY     = 4 ;
		 MOD_STBOX     = 5 ;
		 MOD_GRADFG    = 6 ;
		 MOD_MAG       = 7 ;
		 MOD_NORMG     = 8 ;
		 MOD_SOBEL     = 9 ;
		 MOD_STCANDI   = 10 ;
		 MOD_MAP       = 11 ;;*/
		uint32_t reg_filterop_mode:4;
	};
	uint32_t val;
} IVE_FILTEROP_REG_h10_C;
typedef union {
	struct {
		/*Only use in MOD_DILA,MOD_ERO :
		can setting:
		 OP1_FILTER  = 1 ;
		 OP1_DILA    = 2 ;
		 OP1_ERO     = 3 ;
		 OP1_ORDERF  = 4 ;
		 OP1_BERN    = 5 ;
		 OP1_LBP     = 6 ;;*/
		uint32_t reg_filterop_op1_cmd:4;
		/*over write op1 cmd:
		 OP1_BYP     = 0 ;
		 OP1_FILTER  = 1 ;
		 OP1_DILA    = 2 ;
		 OP1_ERO     = 3 ;
		 OP1_ORDERF  = 4 ;
		 OP1_BERN    = 5 ;
		 OP1_LBP     = 6 ;
		 OP1_NORMG   = 7 ;
		 OP1_MAG     = 8 ;
		 OP1_SOBEL   = 9 ;
		 OP1_STCANDI = 10 ;
		 OP1_MAP     = 11 ;;*/
		uint32_t reg_filterop_sw_ovw_op:1;
		uint32_t rsv_5_7:3;
		/*only use in mod_filter3ch => 1 : filter out  0: center;*/
		uint32_t reg_filterop_3ch_en:1;
		/*y wdma dma enable;*/
		uint32_t reg_op_y_wdma_en:1;
		/*c wdma dma enable;*/
		uint32_t reg_op_c_wdma_en:1;
		/*only wlsb with Output data width;*/
		uint32_t reg_op_y_wdma_w1b_en:1;
		/*only wlsb with Output data width;*/
		uint32_t reg_op_c_wdma_w1b_en:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_h14_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix00     ;*/
		uint32_t reg_filterop_h_coef00:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix01;*/
		uint32_t reg_filterop_h_coef01:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix02;*/
		uint32_t reg_filterop_h_coef02:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix03;*/
		uint32_t reg_filterop_h_coef03:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_4_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix04;*/
		uint32_t reg_filterop_h_coef04:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix10;*/
		uint32_t reg_filterop_h_coef10:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix11;*/
		uint32_t reg_filterop_h_coef11:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix12;*/
		uint32_t reg_filterop_h_coef12:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_5_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix13;*/
		uint32_t reg_filterop_h_coef13:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix14;*/
		uint32_t reg_filterop_h_coef14:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix20;*/
		uint32_t reg_filterop_h_coef20:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix21;*/
		uint32_t reg_filterop_h_coef21:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_6_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix22;*/
		uint32_t reg_filterop_h_coef22:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix23;*/
		uint32_t reg_filterop_h_coef23:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix24;*/
		uint32_t reg_filterop_h_coef24:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix30;*/
		uint32_t reg_filterop_h_coef30:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_7_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix31;*/
		uint32_t reg_filterop_h_coef31:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix32;*/
		uint32_t reg_filterop_h_coef32:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix33;*/
		uint32_t reg_filterop_h_coef33:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix34;*/
		uint32_t reg_filterop_h_coef34:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_8_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix40;*/
		uint32_t reg_filterop_h_coef40:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix41;*/
		uint32_t reg_filterop_h_coef41:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix42;*/
		uint32_t reg_filterop_h_coef42:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix43;*/
		uint32_t reg_filterop_h_coef43:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_9_C;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix44;*/
		uint32_t reg_filterop_h_coef44:8;
		/*value range [0-13];*/
		uint32_t reg_filterop_h_norm:4;
	};
	uint32_t val;
} IVE_FILTEROP_REG_10_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef00:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef01:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef02:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef03:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_11_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef04:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef10:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef11:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef12:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_12_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef13:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef14:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef20:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef21:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_13_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef22:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef23:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef24:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef30:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_14_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef31:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef32:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef33:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef34:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_15_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef40:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef41:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef42:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef43:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_16_C;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef44:8;
		/*value range [0-13];*/
		uint32_t reg_filterop_v_norm:4;
	};
	uint32_t val;
} IVE_FILTEROP_REG_17_C;
typedef union {
	struct {
		/*default 1 : v filter coeff will use h coeff transport;*/
		uint32_t reg_filterop_mode_trans:1;
		uint32_t rsv_1_3:3;
		/*en_mode:
		0 : median filter
		1 : max filter
		2 : min filter;*/
		uint32_t reg_filterop_order_enmode:3;
		uint32_t rsv_7_15:9;
		/*use in MagAndAng ;*/
		uint32_t reg_filterop_mag_thr:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_18_C;
typedef union {
	struct {
		/*1 : use win 5x5 to do bersen
		0: use win 3x3 to do bernsen;*/
		uint32_t reg_filterop_bernsen_win5x5_en:1;
		uint32_t rsv_1_3:3;
		/*0 : normal mode => thrs = (max + min)/2
		1: threshold mode=> (thrs = (max+min)/2 + reg_filterop_bernsen_thr)/2 
		2: paper omde => use local max : max-min and reg_filterop_u8ContrastThreshold;*/
		uint32_t reg_filterop_bernsen_mode:2;
		uint32_t rsv_6_7:2;
		/*;*/
		uint32_t reg_filterop_bernsen_thr:8;
		/*only used in bernsen_mode=2;*/
		uint32_t reg_filterop_u8ContrastThreshold:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_19_C;
typedef union {
	struct {
		/*unsigned threshold;*/
		uint32_t reg_filterop_lbp_u8bit_thr:8;
		/*signed threshold, use 2's   show s7 (1signed + 7bit );*/
		uint32_t reg_filterop_lbp_s8bit_thr:8;
		/*0: diff compare to signed threshold, 
		1: abs compare to unsigned threshold;*/
		uint32_t reg_filterop_lbp_enmode:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_20_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix00;*/
		uint32_t reg_filterop_op2_erodila_coef00:8;
		/*U8 format : only set 255 or 0 ->  for pix01;*/
		uint32_t reg_filterop_op2_erodila_coef01:8;
		/*U8 format : only set 255 or 0 ->  for pix02;*/
		uint32_t reg_filterop_op2_erodila_coef02:8;
		/*U8 format : only set 255 or 0 ->  for pix03;*/
		uint32_t reg_filterop_op2_erodila_coef03:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_21_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix04;*/
		uint32_t reg_filterop_op2_erodila_coef04:8;
		/*U8 format : only set 255 or 0 ->  for pix10;*/
		uint32_t reg_filterop_op2_erodila_coef10:8;
		/*U8 format : only set 255 or 0 ->  for pix11;*/
		uint32_t reg_filterop_op2_erodila_coef11:8;
		/*U8 format : only set 255 or 0 ->  for pix12;*/
		uint32_t reg_filterop_op2_erodila_coef12:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_22_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix13;*/
		uint32_t reg_filterop_op2_erodila_coef13:8;
		/*U8 format : only set 255 or 0 ->  for pix14;*/
		uint32_t reg_filterop_op2_erodila_coef14:8;
		/*U8 format : only set 255 or 0 ->  for pix20;*/
		uint32_t reg_filterop_op2_erodila_coef20:8;
		/*U8 format : only set 255 or 0 ->  for pix21;*/
		uint32_t reg_filterop_op2_erodila_coef21:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_23_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix22;*/
		uint32_t reg_filterop_op2_erodila_coef22:8;
		/*U8 format : only set 255 or 0 ->  for pix23;*/
		uint32_t reg_filterop_op2_erodila_coef23:8;
		/*U8 format : only set 255 or 0 ->  for pix24;*/
		uint32_t reg_filterop_op2_erodila_coef24:8;
		/*U8 format : only set 255 or 0 ->  for pix30;*/
		uint32_t reg_filterop_op2_erodila_coef30:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_24_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix31;*/
		uint32_t reg_filterop_op2_erodila_coef31:8;
		/*U8 format : only set 255 or 0 ->  for pix32;*/
		uint32_t reg_filterop_op2_erodila_coef32:8;
		/*U8 format : only set 255 or 0 ->  for pix33;*/
		uint32_t reg_filterop_op2_erodila_coef33:8;
		/*U8 format : only set 255 or 0 ->  for pix34;*/
		uint32_t reg_filterop_op2_erodila_coef34:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_25_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix40;*/
		uint32_t reg_filterop_op2_erodila_coef40:8;
		/*U8 format : only set 255 or 0 ->  for pix41;*/
		uint32_t reg_filterop_op2_erodila_coef41:8;
		/*U8 format : only set 255 or 0 ->  for pix42;*/
		uint32_t reg_filterop_op2_erodila_coef42:8;
		/*U8 format : only set 255 or 0 ->  for pix43;*/
		uint32_t reg_filterop_op2_erodila_coef43:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_26_C;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix44;*/
		uint32_t reg_filterop_op2_erodila_coef44:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_27_C;
typedef union {
	struct {
		/*set 0 : bypass center from erodilat module;*/
		uint32_t reg_filterop_op2_erodila_en:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_28_C;
typedef union {
	struct {
		/*dbg_en;*/
		uint32_t reg_csc_dbg_en:1;
		uint32_t rsv_1_3:3;
		/*dbg_hw_coeff_sel = [0:11];*/
		uint32_t reg_csc_dbg_coeff_sel:4;
		/*dbg_hw_coeff;*/
		uint32_t reg_csc_dbg_coeff:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_DBG_COEFF_C;
typedef union {
	struct {
		/*dbg_prob_pix_x;*/
		uint32_t reg_csc_dbg_prob_x:12;
		uint32_t rsv_12_15:4;
		/*dbg_prob_pix_y;*/
		uint32_t reg_csc_dbg_prob_y:12;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_DBG_PROB_PIX_C;
typedef union {
	struct {
		/*dbg_src_pix;*/
		uint32_t reg_csc_dbg_src_data:24;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_DBG_DATA_SRC_C;
typedef union {
	struct {
		/*dbg_dst_pix;*/
		uint32_t reg_csc_dbg_dst_data:24;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_DBG_DATA_DAT_C;
typedef union {
	struct {
		/*GradFg module enable
		1'b0: disable
		1'b1: enable;*/
		uint32_t reg_filterop_op2_gradfg_en:1;
		/*GradFg software reset
		1'b0: normal
		1'b1: reset;*/
		uint32_t reg_filterop_op2_gradfg_softrst:1;
		/*GradFg calculation mode
		1'b0: use current gradient
		1'b1: find min gradient;*/
		uint32_t reg_filterop_op2_gradfg_enmode:1;
		/*GradFg black pixels enable flag
		1'b0: no
		1'b1: yes;*/
		uint32_t reg_filterop_op2_gradfg_edwdark:1;
		uint32_t rsv_4_15:12;
		/*GradFg edge width adjustment factor
		range: 500 to 2000;*/
		uint32_t reg_filterop_op2_gradfg_edwfactor:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_33_C;
typedef union {
	struct {
		/*GradFg gradient vector correlation coefficient threshold
		range: 50 to 100;*/
		uint32_t reg_filterop_op2_gradfg_crlcoefthr:8;
		/*GradFg gradient amplitude threshold
		range: 0 to 20;*/
		uint32_t reg_filterop_op2_gradfg_magcrlthr:8;
		/*GradFg gradient magnitude difference threshold
		range: 2 to 8;*/
		uint32_t reg_filterop_op2_gradfg_minmagdiff:8;
		/*GradFg gradient amplitude noise threshold
		range: 1 to 8;*/
		uint32_t reg_filterop_op2_gradfg_noiseval:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_34_C;
typedef union {
	struct {
		/*0 : U8
		1 : S16
		2 : U16;*/
		uint32_t reg_filterop_map_enmode:2;
		uint32_t rsv_2_3:2;
		/*0 : h , v  -> wdma_y wdma_c will be active
		1 :h only -> wdma_y 
		2: v only -> wdma_c
		3. h , v pack => {v ,h} -> wdma_y ;*/
		uint32_t reg_filterop_norm_out_ctrl:2;
		uint32_t rsv_6_7:2;
		/*0: Mag only ->  wdma_c
		1 : Mag + Ang -> wdma_y and wdma_c;*/
		uint32_t reg_filterop_magang_out_ctrl:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_110_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_blen:1;
		/*;*/
		uint32_t reg_dma_en:1;
		uint32_t rsv_2_7:6;
		/*op;*/
		uint32_t reg_fmt_sel:4;
		uint32_t rsv_12_15:4;
		/*;*/
		uint32_t reg_sc_odma_hflip:1;
		/*;*/
		uint32_t reg_sc_odma_vflip:1;
		uint32_t rsv_18_19:2;
		/*;*/
		uint32_t reg_sc_422_avg:1;
		/*;*/
		uint32_t reg_sc_420_avg:1;
		/*0 : UV run away from zero
		1 : UV run to zero;*/
		uint32_t reg_c_round_mode:1;
		/*csc q mode outout BF16 enable
		when this bit set to 1
		1. reg_fmt_sel must set to 0x6
		2. reg_sc_csc_q_mode must set to 1
		
		and notice aht sc_hsv function would be auto disable ;*/
		uint32_t reg_bf16_en:1;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_00_C;
typedef union {
	struct {
		/*output Y (R) base address;*/
		uint32_t reg_dma_y_base_low_part:32;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_01_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_y_base_high_part:8;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_02_C;
typedef union {
	struct {
		/*putput U (G) base address;*/
		uint32_t reg_dma_u_base_low_part:32;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_03_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_u_base_high_part:8;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_04_C;
typedef union {
	struct {
		/*output V (B) base address;*/
		uint32_t reg_dma_v_base_low_part:32;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_05_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_v_base_high_part:8;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_06_C;
typedef union {
	struct {
		/*line pitch for Y/R/Packet data;*/
		uint32_t reg_dma_y_pitch:24;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_07_C;
typedef union {
	struct {
		/*line pitch for C;*/
		uint32_t reg_dma_c_pitch:24;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_08_C;
typedef union {
	struct {
		/*crop write start point ( in pixel base);*/
		uint32_t reg_dma_x_str:12;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_09_C;
typedef union {
	struct {
		/*crop write start line (line line base);*/
		uint32_t reg_dma_y_str:12;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_10_C;
typedef union {
	struct {
		/*output width, fill sub 1;*/
		uint32_t reg_dma_wd:12;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_11_C;
typedef union {
	struct {
		/*output height, fill sub 1;*/
		uint32_t reg_dma_ht:12;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_12_C;
typedef union {
	struct {
		/*dma debug information;*/
		uint32_t reg_dma_debug:32;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_13_C;
typedef union {
	struct {
		/*line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_line_target:12;
		uint32_t rsv_12_15:4;
		/*line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_line_target_sel:2;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_14_C;
typedef union {
	struct {
		/*cycle line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_cycle_line_target:11;
		uint32_t rsv_11_15:5;
		/*cycle line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_cycle_line_target_sel:2;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_15_C;
typedef union {
	struct {
		/*latch odma y line count;*/
		uint32_t reg_dma_latch_line_cnt:1;
		uint32_t rsv_1_7:7;
		/*latched odma y line count;*/
		uint32_t reg_dma_latched_line_cnt:12;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_16_C;
typedef union {
	struct {
		/*debug;*/
		uint32_t axi_active:1;
		/*debug;*/
		uint32_t axi_y_active:1;
		/*debug;*/
		uint32_t axi_u_active:1;
		/*debug;*/
		uint32_t axi_v_active:1;
		/*debug;*/
		uint32_t y_buf_full:1;
		/*debug;*/
		uint32_t y_buf_empty:1;
		/*debug;*/
		uint32_t u_buf_full:1;
		/*debug;*/
		uint32_t u_buf_empty:1;
		/*debug;*/
		uint32_t v_buf_full:1;
		/*debug;*/
		uint32_t v_buf_empty:1;
		/*debug;*/
		uint32_t line_target_hit:1;
		/*debug;*/
		uint32_t cycle_line_target_hit:1;
		/*debug;*/
		uint32_t axi_cmd_cs:4;
		/*debug;*/
		uint32_t y_line_cnt:12;
	};
	uint32_t val;
} IVE_FILTEROP_ODMA_REG_17_C;
typedef union {
	struct {
		/*canny low threshold;*/
		uint32_t reg_canny_lowthr:16;
		/*canny high threshold;*/
		uint32_t reg_canny_hithr:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CANNY_0_C;
typedef union {
	struct {
		/*enable cannyhysedge module or not
		0:disable
		1:enable;*/
		uint32_t reg_canny_en:1;
		/*0:disable
		1:enable;*/
		uint32_t reg_canny_strong_point_cnt_en:1;
		/*should also set reg_canny_strong_point_cnt_en and reg_canny_strong_point_cnt to enable this bit
		0: the strong edge point will be tagged as weak-edge point when total strong edge point > reg_canny_strong_point_cnt
		1: the strong edge point will be tagged as non-edge point when total strong edge point > reg_canny_strong_point_cnt;*/
		uint32_t reg_canny_non_or_weak:1;
		uint32_t rsv_3_15:13;
		/*limit the number of strong edge points in each frame;*/
		uint32_t reg_canny_strong_point_cnt:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CANNY_1_C;
typedef union {
	struct {
		/*the ending symbol for strong edge xy position in DRAM data ;*/
		uint32_t reg_canny_eof:32;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CANNY_2_C;
typedef union {
	struct {
		/*DRAM address for storing strong edge xy position;*/
		uint32_t reg_canny_basel:32;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CANNY_3_C;
typedef union {
	struct {
		/*DRAM address for storing strong edge xy position;*/
		uint32_t reg_canny_baseh:8;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CANNY_4_C;
typedef union {
	struct {
		/*bypass  ive_st_candi_corner module
		0:disable
		1:enable;*/
		uint32_t reg_st_candi_corner_bypass:1;
		/*switch ive_st_candi_corner output when enable reg_st_candi_corner_bypass
		0:disable
		1:enable;*/
		uint32_t reg_st_candi_corner_switch_src:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_ST_CANDI_0_C;
typedef union {
	struct {
		/*maximum eigen value;*/
		uint32_t reg_st_eigval_max_eigval:16;
		/*tile number;*/
		uint32_t reg_st_eigval_tile_num:4;
	};
	uint32_t val;
} IVE_FILTEROP_REG_ST_EIGVAL_0_C;
typedef union {
	struct {
		/*sw clear max eigen value;*/
		uint32_t reg_sw_clr_max_eigval:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_ST_EIGVAL_1_C;
typedef union {
	struct {
		/*unsigned 12 bit, update table value of inv_v_tab(rgb2hsv) or gamma_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_tab_sw_0:12;
		uint32_t rsv_12_15:4;
		/*unsigned 15 bit, update table value of inv_h_tab(rgb2hsv) or xyz_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_tab_sw_1:15;
	};
	uint32_t val;
} IVE_FILTEROP_REG_h190_C;
typedef union {
	struct {
		/*update rgb2hsv/rgb2lab table value by software
		0:use const, 1:update table by reg_csc_tab_sw_0 and reg_csc_tab_sw_1;*/
		uint32_t reg_filterop_op2_csc_tab_sw_update:1;
		uint32_t rsv_1_15:15;
		/*update yuv2rgb coeff value by software
		0: use const, 1:update coeff by reg_csc_coeff_sw;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_update:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_h194_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_00:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_0_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_01:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_1_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_02:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_2_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_03:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_3_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_04:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_4_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_05:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_5_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_06:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_6_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_07:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_7_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_08:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_8_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_09:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_9_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_10:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_A_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_11:19;
	};
	uint32_t val;
} IVE_FILTEROP_REG_CSC_COEFF_B_C;
typedef union {
	struct {
		/*en_mode:
		0,1,2,3: yuv2rgb
		4,5: yuv2rgb2hsv
		6,7: yuv2rgb2lab
		8,9,10,11: rgb2yuv;*/
		uint32_t reg_filterop_op2_csc_enmode:4;
		/*op2 csc enable;*/
		uint32_t reg_filterop_op2_csc_enable:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_h1c8_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_y_start_x:16;
		/*;*/
		uint32_t reg_crop_y_end_x:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_cropy_s_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_y_start_y:16;
		/*;*/
		uint32_t reg_crop_y_end_y:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_cropy_e_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_y_enable:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_cropy_ctl_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_c_start_x:16;
		/*;*/
		uint32_t reg_crop_c_end_x:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_cropc_s_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_c_start_y:16;
		/*;*/
		uint32_t reg_crop_c_end_y:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_cropc_e_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_c_enable:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_cropc_ctl_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_odma_start_x:16;
		/*;*/
		uint32_t reg_crop_odma_end_x:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_crop_odma_s_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_odma_start_y:16;
		/*;*/
		uint32_t reg_crop_odma_end_y:16;
	};
	uint32_t val;
} IVE_FILTEROP_REG_crop_odma_e_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_odma_enable:1;
	};
	uint32_t val;
} IVE_FILTEROP_REG_crop_odma_ctl_C;
typedef struct {
	volatile IVE_FILTEROP_REG_1_C REG_1;
	volatile IVE_FILTEROP_REG_H04_C REG_H04;
	volatile IVE_FILTEROP_REG_2_C REG_2;
	volatile IVE_FILTEROP_REG_3_C REG_3;
	volatile IVE_FILTEROP_REG_h10_C REG_h10;
	volatile IVE_FILTEROP_REG_h14_C REG_h14;
	volatile uint32_t _REG_4_0; // 0x18
	volatile uint32_t _REG_4_1; // 0x1C
	volatile IVE_FILTEROP_REG_4_C REG_4;
	volatile IVE_FILTEROP_REG_5_C REG_5;
	volatile IVE_FILTEROP_REG_6_C REG_6;
	volatile IVE_FILTEROP_REG_7_C REG_7;
	volatile IVE_FILTEROP_REG_8_C REG_8;
	volatile IVE_FILTEROP_REG_9_C REG_9;
	volatile IVE_FILTEROP_REG_10_C REG_10;
	volatile uint32_t _REG_11_0; // 0x3C
	volatile IVE_FILTEROP_REG_11_C REG_11;
	volatile IVE_FILTEROP_REG_12_C REG_12;
	volatile IVE_FILTEROP_REG_13_C REG_13;
	volatile IVE_FILTEROP_REG_14_C REG_14;
	volatile IVE_FILTEROP_REG_15_C REG_15;
	volatile IVE_FILTEROP_REG_16_C REG_16;
	volatile IVE_FILTEROP_REG_17_C REG_17;
	volatile uint32_t _REG_18_0; // 0x5C
	volatile IVE_FILTEROP_REG_18_C REG_18;
	volatile IVE_FILTEROP_REG_19_C REG_19;
	volatile IVE_FILTEROP_REG_20_C REG_20;
	volatile uint32_t _REG_21_0; // 0x6C
	volatile IVE_FILTEROP_REG_21_C REG_21;
	volatile IVE_FILTEROP_REG_22_C REG_22;
	volatile IVE_FILTEROP_REG_23_C REG_23;
	volatile IVE_FILTEROP_REG_24_C REG_24;
	volatile IVE_FILTEROP_REG_25_C REG_25;
	volatile IVE_FILTEROP_REG_26_C REG_26;
	volatile IVE_FILTEROP_REG_27_C REG_27;
	volatile IVE_FILTEROP_REG_28_C REG_28;
	volatile IVE_FILTEROP_REG_CSC_DBG_COEFF_C REG_CSC_DBG_COEFF;
	volatile IVE_FILTEROP_REG_CSC_DBG_PROB_PIX_C REG_CSC_DBG_PROB_PIX;
	volatile IVE_FILTEROP_REG_CSC_DBG_DATA_SRC_C REG_CSC_DBG_DATA_SRC;
	volatile IVE_FILTEROP_REG_CSC_DBG_DATA_DAT_C REG_CSC_DBG_DATA_DAT;
	volatile uint32_t _REG_33_0; // 0xA0
	volatile uint32_t _REG_33_1; // 0xA4
	volatile uint32_t _REG_33_2; // 0xA8
	volatile uint32_t _REG_33_3; // 0xAC
	volatile uint32_t _REG_33_4; // 0xB0
	volatile uint32_t _REG_33_5; // 0xB4
	volatile uint32_t _REG_33_6; // 0xB8
	volatile uint32_t _REG_33_7; // 0xBC
	volatile uint32_t _REG_33_8; // 0xC0
	volatile uint32_t _REG_33_9; // 0xC4
	volatile uint32_t _REG_33_10; // 0xC8
	volatile uint32_t _REG_33_11; // 0xCC
	volatile uint32_t _REG_33_12; // 0xD0
	volatile uint32_t _REG_33_13; // 0xD4
	volatile uint32_t _REG_33_14; // 0xD8
	volatile uint32_t _REG_33_15; // 0xDC
	volatile uint32_t _REG_33_16; // 0xE0
	volatile uint32_t _REG_33_17; // 0xE4
	volatile uint32_t _REG_33_18; // 0xE8
	volatile uint32_t _REG_33_19; // 0xEC
	volatile uint32_t _REG_33_20; // 0xF0
	volatile uint32_t _REG_33_21; // 0xF4
	volatile uint32_t _REG_33_22; // 0xF8
	volatile uint32_t _REG_33_23; // 0xFC
	volatile IVE_FILTEROP_REG_33_C REG_33;
	volatile IVE_FILTEROP_REG_34_C REG_34;
	volatile uint32_t _REG_110_0; // 0x108
	volatile uint32_t _REG_110_1; // 0x10C
	volatile IVE_FILTEROP_REG_110_C REG_110;
	volatile uint32_t _ODMA_REG_00_0; // 0x114
	volatile uint32_t _ODMA_REG_00_1; // 0x118
	volatile uint32_t _ODMA_REG_00_2; // 0x11C
	volatile IVE_FILTEROP_ODMA_REG_00_C ODMA_REG_00;
	volatile IVE_FILTEROP_ODMA_REG_01_C ODMA_REG_01;
	volatile IVE_FILTEROP_ODMA_REG_02_C ODMA_REG_02;
	volatile IVE_FILTEROP_ODMA_REG_03_C ODMA_REG_03;
	volatile IVE_FILTEROP_ODMA_REG_04_C ODMA_REG_04;
	volatile IVE_FILTEROP_ODMA_REG_05_C ODMA_REG_05;
	volatile IVE_FILTEROP_ODMA_REG_06_C ODMA_REG_06;
	volatile IVE_FILTEROP_ODMA_REG_07_C ODMA_REG_07;
	volatile IVE_FILTEROP_ODMA_REG_08_C ODMA_REG_08;
	volatile IVE_FILTEROP_ODMA_REG_09_C ODMA_REG_09;
	volatile IVE_FILTEROP_ODMA_REG_10_C ODMA_REG_10;
	volatile IVE_FILTEROP_ODMA_REG_11_C ODMA_REG_11;
	volatile IVE_FILTEROP_ODMA_REG_12_C ODMA_REG_12;
	volatile IVE_FILTEROP_ODMA_REG_13_C ODMA_REG_13;
	volatile IVE_FILTEROP_ODMA_REG_14_C ODMA_REG_14;
	volatile IVE_FILTEROP_ODMA_REG_15_C ODMA_REG_15;
	volatile IVE_FILTEROP_ODMA_REG_16_C ODMA_REG_16;
	volatile IVE_FILTEROP_ODMA_REG_17_C ODMA_REG_17;
	volatile uint32_t _REG_CANNY_0_0; // 0x168
	volatile uint32_t _REG_CANNY_0_1; // 0x16C
	volatile IVE_FILTEROP_REG_CANNY_0_C REG_CANNY_0;
	volatile IVE_FILTEROP_REG_CANNY_1_C REG_CANNY_1;
	volatile IVE_FILTEROP_REG_CANNY_2_C REG_CANNY_2;
	volatile IVE_FILTEROP_REG_CANNY_3_C REG_CANNY_3;
	volatile IVE_FILTEROP_REG_CANNY_4_C REG_CANNY_4;
	volatile IVE_FILTEROP_REG_ST_CANDI_0_C REG_ST_CANDI_0;
	volatile IVE_FILTEROP_REG_ST_EIGVAL_0_C REG_ST_EIGVAL_0;
	volatile IVE_FILTEROP_REG_ST_EIGVAL_1_C REG_ST_EIGVAL_1;
	volatile IVE_FILTEROP_REG_h190_C REG_h190;
	volatile IVE_FILTEROP_REG_h194_C REG_h194;
	volatile IVE_FILTEROP_REG_CSC_COEFF_0_C REG_CSC_COEFF_0;
	volatile IVE_FILTEROP_REG_CSC_COEFF_1_C REG_CSC_COEFF_1;
	volatile IVE_FILTEROP_REG_CSC_COEFF_2_C REG_CSC_COEFF_2;
	volatile IVE_FILTEROP_REG_CSC_COEFF_3_C REG_CSC_COEFF_3;
	volatile IVE_FILTEROP_REG_CSC_COEFF_4_C REG_CSC_COEFF_4;
	volatile IVE_FILTEROP_REG_CSC_COEFF_5_C REG_CSC_COEFF_5;
	volatile IVE_FILTEROP_REG_CSC_COEFF_6_C REG_CSC_COEFF_6;
	volatile IVE_FILTEROP_REG_CSC_COEFF_7_C REG_CSC_COEFF_7;
	volatile IVE_FILTEROP_REG_CSC_COEFF_8_C REG_CSC_COEFF_8;
	volatile IVE_FILTEROP_REG_CSC_COEFF_9_C REG_CSC_COEFF_9;
	volatile IVE_FILTEROP_REG_CSC_COEFF_A_C REG_CSC_COEFF_A;
	volatile IVE_FILTEROP_REG_CSC_COEFF_B_C REG_CSC_COEFF_B;
	volatile IVE_FILTEROP_REG_h1c8_C REG_h1c8;
	volatile uint32_t _REG_cropy_s_0; // 0x1CC
	volatile IVE_FILTEROP_REG_cropy_s_C REG_cropy_s;
	volatile IVE_FILTEROP_REG_cropy_e_C REG_cropy_e;
	volatile IVE_FILTEROP_REG_cropy_ctl_C REG_cropy_ctl;
	volatile uint32_t _REG_cropc_s_0; // 0x1DC
	volatile IVE_FILTEROP_REG_cropc_s_C REG_cropc_s;
	volatile IVE_FILTEROP_REG_cropc_e_C REG_cropc_e;
	volatile IVE_FILTEROP_REG_cropc_ctl_C REG_cropc_ctl;
	volatile uint32_t _REG_crop_odma_s_0; // 0x1EC
	volatile IVE_FILTEROP_REG_crop_odma_s_C REG_crop_odma_s;
	volatile IVE_FILTEROP_REG_crop_odma_e_C REG_crop_odma_e;
	volatile IVE_FILTEROP_REG_crop_odma_ctl_C REG_crop_odma_ctl;
} IVE_FILTEROP_C;

static void ive_filterop_printk(IVE_FILTEROP_C *p)
{
	pr_info("ive_filterop\n");
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->REG_1.reg_softrst);
	pr_info("\tREG_1.reg_softrst_wdma_y = 0x%x\n",
		p->REG_1.reg_softrst_wdma_y);
	pr_info("\tREG_1.reg_softrst_wdma_c = 0x%x\n",
		p->REG_1.reg_softrst_wdma_c);
	pr_info("\tREG_1.reg_softrst_rdma_gradfg = 0x%x\n",
		p->REG_1.reg_softrst_rdma_gradfg);
	pr_info("\tREG_1.reg_softrst_op1 = 0x%x\n", p->REG_1.reg_softrst_op1);
	pr_info("\tREG_1.reg_softrst_filter3ch = 0x%x\n",
		p->REG_1.reg_softrst_filter3ch);
	pr_info("\tREG_1.reg_softrst_st = 0x%x\n", p->REG_1.reg_softrst_st);
	pr_info("\tREG_1.reg_softrst_odma = 0x%x\n", p->REG_1.reg_softrst_odma);
	pr_info("\tREG_H04.reg_gradfg_bggrad_rdma_en = 0x%x\n",
		p->REG_H04.reg_gradfg_bggrad_rdma_en);
	pr_info("\tREG_H04.reg_gradfg_bggrad_uv_swap = 0x%x\n",
		p->REG_H04.reg_gradfg_bggrad_uv_swap);
	pr_info("\tREG_2.reg_shdw_sel = 0x%x\n", p->REG_2.reg_shdw_sel);
	pr_info("\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->REG_3.reg_ctrl_dmy1);
	pr_info("\tREG_h10.reg_filterop_mode = 0x%x\n",
		p->REG_h10.reg_filterop_mode);
	pr_info("\tREG_h14.reg_filterop_op1_cmd = 0x%x\n",
		p->REG_h14.reg_filterop_op1_cmd);
	pr_info("\tREG_h14.reg_filterop_sw_ovw_op = 0x%x\n",
		p->REG_h14.reg_filterop_sw_ovw_op);
	pr_info("\tREG_h14.reg_filterop_3ch_en = 0x%x\n",
		p->REG_h14.reg_filterop_3ch_en);
	pr_info("\tREG_h14.reg_op_y_wdma_en = 0x%x\n",
		p->REG_h14.reg_op_y_wdma_en);
	pr_info("\tREG_h14.reg_op_c_wdma_en = 0x%x\n",
		p->REG_h14.reg_op_c_wdma_en);
	pr_info("\tREG_h14.reg_op_y_wdma_w1b_en = 0x%x\n",
		p->REG_h14.reg_op_y_wdma_w1b_en);
	pr_info("\tREG_h14.reg_op_c_wdma_w1b_en = 0x%x\n",
		p->REG_h14.reg_op_c_wdma_w1b_en);
	pr_info("\tREG_4.reg_filterop_h_coef00 = 0x%x\n",
		p->REG_4.reg_filterop_h_coef00);
	pr_info("\tREG_4.reg_filterop_h_coef01 = 0x%x\n",
		p->REG_4.reg_filterop_h_coef01);
	pr_info("\tREG_4.reg_filterop_h_coef02 = 0x%x\n",
		p->REG_4.reg_filterop_h_coef02);
	pr_info("\tREG_4.reg_filterop_h_coef03 = 0x%x\n",
		p->REG_4.reg_filterop_h_coef03);
	pr_info("\tREG_5.reg_filterop_h_coef04 = 0x%x\n",
		p->REG_5.reg_filterop_h_coef04);
	pr_info("\tREG_5.reg_filterop_h_coef10 = 0x%x\n",
		p->REG_5.reg_filterop_h_coef10);
	pr_info("\tREG_5.reg_filterop_h_coef11 = 0x%x\n",
		p->REG_5.reg_filterop_h_coef11);
	pr_info("\tREG_5.reg_filterop_h_coef12 = 0x%x\n",
		p->REG_5.reg_filterop_h_coef12);
	pr_info("\tREG_6.reg_filterop_h_coef13 = 0x%x\n",
		p->REG_6.reg_filterop_h_coef13);
	pr_info("\tREG_6.reg_filterop_h_coef14 = 0x%x\n",
		p->REG_6.reg_filterop_h_coef14);
	pr_info("\tREG_6.reg_filterop_h_coef20 = 0x%x\n",
		p->REG_6.reg_filterop_h_coef20);
	pr_info("\tREG_6.reg_filterop_h_coef21 = 0x%x\n",
		p->REG_6.reg_filterop_h_coef21);
	pr_info("\tREG_7.reg_filterop_h_coef22 = 0x%x\n",
		p->REG_7.reg_filterop_h_coef22);
	pr_info("\tREG_7.reg_filterop_h_coef23 = 0x%x\n",
		p->REG_7.reg_filterop_h_coef23);
	pr_info("\tREG_7.reg_filterop_h_coef24 = 0x%x\n",
		p->REG_7.reg_filterop_h_coef24);
	pr_info("\tREG_7.reg_filterop_h_coef30 = 0x%x\n",
		p->REG_7.reg_filterop_h_coef30);
	pr_info("\tREG_8.reg_filterop_h_coef31 = 0x%x\n",
		p->REG_8.reg_filterop_h_coef31);
	pr_info("\tREG_8.reg_filterop_h_coef32 = 0x%x\n",
		p->REG_8.reg_filterop_h_coef32);
	pr_info("\tREG_8.reg_filterop_h_coef33 = 0x%x\n",
		p->REG_8.reg_filterop_h_coef33);
	pr_info("\tREG_8.reg_filterop_h_coef34 = 0x%x\n",
		p->REG_8.reg_filterop_h_coef34);
	pr_info("\tREG_9.reg_filterop_h_coef40 = 0x%x\n",
		p->REG_9.reg_filterop_h_coef40);
	pr_info("\tREG_9.reg_filterop_h_coef41 = 0x%x\n",
		p->REG_9.reg_filterop_h_coef41);
	pr_info("\tREG_9.reg_filterop_h_coef42 = 0x%x\n",
		p->REG_9.reg_filterop_h_coef42);
	pr_info("\tREG_9.reg_filterop_h_coef43 = 0x%x\n",
		p->REG_9.reg_filterop_h_coef43);
	pr_info("\tREG_10.reg_filterop_h_coef44 = 0x%x\n",
		p->REG_10.reg_filterop_h_coef44);
	pr_info("\tREG_10.reg_filterop_h_norm = 0x%x\n",
		p->REG_10.reg_filterop_h_norm);
	pr_info("\tREG_11.reg_filterop_v_coef00 = 0x%x\n",
		p->REG_11.reg_filterop_v_coef00);
	pr_info("\tREG_11.reg_filterop_v_coef01 = 0x%x\n",
		p->REG_11.reg_filterop_v_coef01);
	pr_info("\tREG_11.reg_filterop_v_coef02 = 0x%x\n",
		p->REG_11.reg_filterop_v_coef02);
	pr_info("\tREG_11.reg_filterop_v_coef03 = 0x%x\n",
		p->REG_11.reg_filterop_v_coef03);
	pr_info("\tREG_12.reg_filterop_v_coef04 = 0x%x\n",
		p->REG_12.reg_filterop_v_coef04);
	pr_info("\tREG_12.reg_filterop_v_coef10 = 0x%x\n",
		p->REG_12.reg_filterop_v_coef10);
	pr_info("\tREG_12.reg_filterop_v_coef11 = 0x%x\n",
		p->REG_12.reg_filterop_v_coef11);
	pr_info("\tREG_12.reg_filterop_v_coef12 = 0x%x\n",
		p->REG_12.reg_filterop_v_coef12);
	pr_info("\tREG_13.reg_filterop_v_coef13 = 0x%x\n",
		p->REG_13.reg_filterop_v_coef13);
	pr_info("\tREG_13.reg_filterop_v_coef14 = 0x%x\n",
		p->REG_13.reg_filterop_v_coef14);
	pr_info("\tREG_13.reg_filterop_v_coef20 = 0x%x\n",
		p->REG_13.reg_filterop_v_coef20);
	pr_info("\tREG_13.reg_filterop_v_coef21 = 0x%x\n",
		p->REG_13.reg_filterop_v_coef21);
	pr_info("\tREG_14.reg_filterop_v_coef22 = 0x%x\n",
		p->REG_14.reg_filterop_v_coef22);
	pr_info("\tREG_14.reg_filterop_v_coef23 = 0x%x\n",
		p->REG_14.reg_filterop_v_coef23);
	pr_info("\tREG_14.reg_filterop_v_coef24 = 0x%x\n",
		p->REG_14.reg_filterop_v_coef24);
	pr_info("\tREG_14.reg_filterop_v_coef30 = 0x%x\n",
		p->REG_14.reg_filterop_v_coef30);
	pr_info("\tREG_15.reg_filterop_v_coef31 = 0x%x\n",
		p->REG_15.reg_filterop_v_coef31);
	pr_info("\tREG_15.reg_filterop_v_coef32 = 0x%x\n",
		p->REG_15.reg_filterop_v_coef32);
	pr_info("\tREG_15.reg_filterop_v_coef33 = 0x%x\n",
		p->REG_15.reg_filterop_v_coef33);
	pr_info("\tREG_15.reg_filterop_v_coef34 = 0x%x\n",
		p->REG_15.reg_filterop_v_coef34);
	pr_info("\tREG_16.reg_filterop_v_coef40 = 0x%x\n",
		p->REG_16.reg_filterop_v_coef40);
	pr_info("\tREG_16.reg_filterop_v_coef41 = 0x%x\n",
		p->REG_16.reg_filterop_v_coef41);
	pr_info("\tREG_16.reg_filterop_v_coef42 = 0x%x\n",
		p->REG_16.reg_filterop_v_coef42);
	pr_info("\tREG_16.reg_filterop_v_coef43 = 0x%x\n",
		p->REG_16.reg_filterop_v_coef43);
	pr_info("\tREG_17.reg_filterop_v_coef44 = 0x%x\n",
		p->REG_17.reg_filterop_v_coef44);
	pr_info("\tREG_17.reg_filterop_v_norm = 0x%x\n",
		p->REG_17.reg_filterop_v_norm);
	pr_info("\tREG_18.reg_filterop_mode_trans = 0x%x\n",
		p->REG_18.reg_filterop_mode_trans);
	pr_info("\tREG_18.reg_filterop_order_enmode = 0x%x\n",
		p->REG_18.reg_filterop_order_enmode);
	pr_info("\tREG_18.reg_filterop_mag_thr = 0x%x\n",
		p->REG_18.reg_filterop_mag_thr);
	pr_info("\tREG_19.reg_filterop_bernsen_win5x5_en = 0x%x\n",
		p->REG_19.reg_filterop_bernsen_win5x5_en);
	pr_info("\tREG_19.reg_filterop_bernsen_mode = 0x%x\n",
		p->REG_19.reg_filterop_bernsen_mode);
	pr_info("\tREG_19.reg_filterop_bernsen_thr = 0x%x\n",
		p->REG_19.reg_filterop_bernsen_thr);
	pr_info("\tREG_19.reg_filterop_u8ContrastThreshold = 0x%x\n",
		p->REG_19.reg_filterop_u8ContrastThreshold);
	pr_info("\tREG_20.reg_filterop_lbp_u8bit_thr = 0x%x\n",
		p->REG_20.reg_filterop_lbp_u8bit_thr);
	pr_info("\tREG_20.reg_filterop_lbp_s8bit_thr = 0x%x\n",
		p->REG_20.reg_filterop_lbp_s8bit_thr);
	pr_info("\tREG_20.reg_filterop_lbp_enmode = 0x%x\n",
		p->REG_20.reg_filterop_lbp_enmode);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef00 = 0x%x\n",
		p->REG_21.reg_filterop_op2_erodila_coef00);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef01 = 0x%x\n",
		p->REG_21.reg_filterop_op2_erodila_coef01);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef02 = 0x%x\n",
		p->REG_21.reg_filterop_op2_erodila_coef02);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef03 = 0x%x\n",
		p->REG_21.reg_filterop_op2_erodila_coef03);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef04 = 0x%x\n",
		p->REG_22.reg_filterop_op2_erodila_coef04);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef10 = 0x%x\n",
		p->REG_22.reg_filterop_op2_erodila_coef10);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef11 = 0x%x\n",
		p->REG_22.reg_filterop_op2_erodila_coef11);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef12 = 0x%x\n",
		p->REG_22.reg_filterop_op2_erodila_coef12);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef13 = 0x%x\n",
		p->REG_23.reg_filterop_op2_erodila_coef13);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef14 = 0x%x\n",
		p->REG_23.reg_filterop_op2_erodila_coef14);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef20 = 0x%x\n",
		p->REG_23.reg_filterop_op2_erodila_coef20);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef21 = 0x%x\n",
		p->REG_23.reg_filterop_op2_erodila_coef21);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef22 = 0x%x\n",
		p->REG_24.reg_filterop_op2_erodila_coef22);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef23 = 0x%x\n",
		p->REG_24.reg_filterop_op2_erodila_coef23);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef24 = 0x%x\n",
		p->REG_24.reg_filterop_op2_erodila_coef24);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef30 = 0x%x\n",
		p->REG_24.reg_filterop_op2_erodila_coef30);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef31 = 0x%x\n",
		p->REG_25.reg_filterop_op2_erodila_coef31);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef32 = 0x%x\n",
		p->REG_25.reg_filterop_op2_erodila_coef32);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef33 = 0x%x\n",
		p->REG_25.reg_filterop_op2_erodila_coef33);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef34 = 0x%x\n",
		p->REG_25.reg_filterop_op2_erodila_coef34);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef40 = 0x%x\n",
		p->REG_26.reg_filterop_op2_erodila_coef40);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef41 = 0x%x\n",
		p->REG_26.reg_filterop_op2_erodila_coef41);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef42 = 0x%x\n",
		p->REG_26.reg_filterop_op2_erodila_coef42);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef43 = 0x%x\n",
		p->REG_26.reg_filterop_op2_erodila_coef43);
	pr_info("\tREG_27.reg_filterop_op2_erodila_coef44 = 0x%x\n",
		p->REG_27.reg_filterop_op2_erodila_coef44);
	pr_info("\tREG_28.reg_filterop_op2_erodila_en = 0x%x\n",
		p->REG_28.reg_filterop_op2_erodila_en);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_en = 0x%x\n",
		p->REG_33.reg_filterop_op2_gradfg_en);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_softrst = 0x%x\n",
		p->REG_33.reg_filterop_op2_gradfg_softrst);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_enmode = 0x%x\n",
		p->REG_33.reg_filterop_op2_gradfg_enmode);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_edwdark = 0x%x\n",
		p->REG_33.reg_filterop_op2_gradfg_edwdark);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_edwfactor = 0x%x\n",
		p->REG_33.reg_filterop_op2_gradfg_edwfactor);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n",
		p->REG_34.reg_filterop_op2_gradfg_crlcoefthr);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_magcrlthr = 0x%x\n",
		p->REG_34.reg_filterop_op2_gradfg_magcrlthr);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_minmagdiff = 0x%x\n",
		p->REG_34.reg_filterop_op2_gradfg_minmagdiff);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_noiseval = 0x%x\n",
		p->REG_34.reg_filterop_op2_gradfg_noiseval);
	pr_info("\tREG_110.reg_filterop_map_enmode = 0x%x\n",
		p->REG_110.reg_filterop_map_enmode);
	pr_info("\tREG_110.reg_filterop_norm_out_ctrl = 0x%x\n",
		p->REG_110.reg_filterop_norm_out_ctrl);
	pr_info("\tREG_110.reg_filterop_magang_out_ctrl = 0x%x\n",
		p->REG_110.reg_filterop_magang_out_ctrl);
	pr_info("\tODMA_REG_00.reg_dma_blen = 0x%x\n",
		p->ODMA_REG_00.reg_dma_blen);
	pr_info("\tODMA_REG_00.reg_dma_en = 0x%x\n", p->ODMA_REG_00.reg_dma_en);
	pr_info("\tODMA_REG_00.reg_fmt_sel = 0x%x\n",
		p->ODMA_REG_00.reg_fmt_sel);
	pr_info("\tODMA_REG_00.reg_sc_odma_hflip = 0x%x\n",
		p->ODMA_REG_00.reg_sc_odma_hflip);
	pr_info("\tODMA_REG_00.reg_sc_odma_vflip = 0x%x\n",
		p->ODMA_REG_00.reg_sc_odma_vflip);
	pr_info("\tODMA_REG_00.reg_sc_422_avg = 0x%x\n",
		p->ODMA_REG_00.reg_sc_422_avg);
	pr_info("\tODMA_REG_00.reg_sc_420_avg = 0x%x\n",
		p->ODMA_REG_00.reg_sc_420_avg);
	pr_info("\tODMA_REG_00.reg_c_round_mode = 0x%x\n",
		p->ODMA_REG_00.reg_c_round_mode);
	pr_info("\tODMA_REG_00.reg_bf16_en = 0x%x\n",
		p->ODMA_REG_00.reg_bf16_en);
	pr_info("\tODMA_REG_01.reg_dma_y_base_low_part = 0x%x\n",
		p->ODMA_REG_01.reg_dma_y_base_low_part);
	pr_info("\tODMA_REG_02.reg_dma_y_base_high_part = 0x%x\n",
		p->ODMA_REG_02.reg_dma_y_base_high_part);
	pr_info("\tODMA_REG_03.reg_dma_u_base_low_part = 0x%x\n",
		p->ODMA_REG_03.reg_dma_u_base_low_part);
	pr_info("\tODMA_REG_04.reg_dma_u_base_high_part = 0x%x\n",
		p->ODMA_REG_04.reg_dma_u_base_high_part);
	pr_info("\tODMA_REG_05.reg_dma_v_base_low_part = 0x%x\n",
		p->ODMA_REG_05.reg_dma_v_base_low_part);
	pr_info("\tODMA_REG_06.reg_dma_v_base_high_part = 0x%x\n",
		p->ODMA_REG_06.reg_dma_v_base_high_part);
	pr_info("\tODMA_REG_07.reg_dma_y_pitch = 0x%x\n",
		p->ODMA_REG_07.reg_dma_y_pitch);
	pr_info("\tODMA_REG_08.reg_dma_c_pitch = 0x%x\n",
		p->ODMA_REG_08.reg_dma_c_pitch);
	pr_info("\tODMA_REG_09.reg_dma_x_str = 0x%x\n",
		p->ODMA_REG_09.reg_dma_x_str);
	pr_info("\tODMA_REG_10.reg_dma_y_str = 0x%x\n",
		p->ODMA_REG_10.reg_dma_y_str);
	pr_info("\tODMA_REG_11.reg_dma_wd = 0x%x\n", p->ODMA_REG_11.reg_dma_wd);
	pr_info("\tODMA_REG_12.reg_dma_ht = 0x%x\n", p->ODMA_REG_12.reg_dma_ht);
	pr_info("\tODMA_REG_13.reg_dma_debug = 0x%x\n",
		p->ODMA_REG_13.reg_dma_debug);
	pr_info("\tODMA_REG_14.reg_dma_int_line_target = 0x%x\n",
		p->ODMA_REG_14.reg_dma_int_line_target);
	pr_info("\tODMA_REG_14.reg_dma_int_line_target_sel = 0x%x\n",
		p->ODMA_REG_14.reg_dma_int_line_target_sel);
	pr_info("\tODMA_REG_15.reg_dma_int_cycle_line_target = 0x%x\n",
		p->ODMA_REG_15.reg_dma_int_cycle_line_target);
	pr_info("\tODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x%x\n",
		p->ODMA_REG_15.reg_dma_int_cycle_line_target_sel);
	pr_info("\tODMA_REG_16.reg_dma_latch_line_cnt = 0x%x\n",
		p->ODMA_REG_16.reg_dma_latch_line_cnt);
	pr_info("\tODMA_REG_16.reg_dma_latched_line_cnt = 0x%x\n",
		p->ODMA_REG_16.reg_dma_latched_line_cnt);
	pr_info("\tODMA_REG_17.axi_active = 0x%x\n", p->ODMA_REG_17.axi_active);
	pr_info("\tODMA_REG_17.axi_y_active = 0x%x\n",
		p->ODMA_REG_17.axi_y_active);
	pr_info("\tODMA_REG_17.axi_u_active = 0x%x\n",
		p->ODMA_REG_17.axi_u_active);
	pr_info("\tODMA_REG_17.axi_v_active = 0x%x\n",
		p->ODMA_REG_17.axi_v_active);
	pr_info("\tODMA_REG_17.y_buf_full = 0x%x\n", p->ODMA_REG_17.y_buf_full);
	pr_info("\tODMA_REG_17.y_buf_empty = 0x%x\n",
		p->ODMA_REG_17.y_buf_empty);
	pr_info("\tODMA_REG_17.u_buf_full = 0x%x\n", p->ODMA_REG_17.u_buf_full);
	pr_info("\tODMA_REG_17.u_buf_empty = 0x%x\n",
		p->ODMA_REG_17.u_buf_empty);
	pr_info("\tODMA_REG_17.v_buf_full = 0x%x\n", p->ODMA_REG_17.v_buf_full);
	pr_info("\tODMA_REG_17.v_buf_empty = 0x%x\n",
		p->ODMA_REG_17.v_buf_empty);
	pr_info("\tODMA_REG_17.line_target_hit = 0x%x\n",
		p->ODMA_REG_17.line_target_hit);
	pr_info("\tODMA_REG_17.cycle_line_target_hit = 0x%x\n",
		p->ODMA_REG_17.cycle_line_target_hit);
	pr_info("\tODMA_REG_17.axi_cmd_cs = 0x%x\n", p->ODMA_REG_17.axi_cmd_cs);
	pr_info("\tODMA_REG_17.y_line_cnt = 0x%x\n", p->ODMA_REG_17.y_line_cnt);
	pr_info("\tREG_CANNY_0.reg_canny_lowthr = 0x%x\n",
		p->REG_CANNY_0.reg_canny_lowthr);
	pr_info("\tREG_CANNY_0.reg_canny_hithr = 0x%x\n",
		p->REG_CANNY_0.reg_canny_hithr);
	pr_info("\tREG_CANNY_1.reg_canny_en = 0x%x\n",
		p->REG_CANNY_1.reg_canny_en);
	pr_info("\tREG_CANNY_1.reg_canny_strong_point_cnt_en = 0x%x\n",
		p->REG_CANNY_1.reg_canny_strong_point_cnt_en);
	pr_info("\tREG_CANNY_1.reg_canny_non_or_weak = 0x%x\n",
		p->REG_CANNY_1.reg_canny_non_or_weak);
	pr_info("\tREG_CANNY_1.reg_canny_strong_point_cnt = 0x%x\n",
		p->REG_CANNY_1.reg_canny_strong_point_cnt);
	pr_info("\tREG_CANNY_2.reg_canny_eof = 0x%x\n",
		p->REG_CANNY_2.reg_canny_eof);
	pr_info("\tREG_CANNY_3.reg_canny_basel = 0x%x\n",
		p->REG_CANNY_3.reg_canny_basel);
	pr_info("\tREG_CANNY_4.reg_canny_baseh = 0x%x\n",
		p->REG_CANNY_4.reg_canny_baseh);
	pr_info("\tREG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x%x\n",
		p->REG_ST_CANDI_0.reg_st_candi_corner_bypass);
	pr_info("\tREG_ST_CANDI_0.reg_st_candi_corner_switch_src = 0x%x\n",
		p->REG_ST_CANDI_0.reg_st_candi_corner_switch_src);
	pr_info("\tREG_ST_EIGVAL_0.reg_st_eigval_max_eigval = 0x%x\n",
		p->REG_ST_EIGVAL_0.reg_st_eigval_max_eigval);
	pr_info("\tREG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0x%x\n",
		p->REG_ST_EIGVAL_0.reg_st_eigval_tile_num);
	pr_info("\tREG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 0x%x\n",
		p->REG_ST_EIGVAL_1.reg_sw_clr_max_eigval);
	pr_info("\tREG_h190.reg_filterop_op2_csc_tab_sw_0 = 0x%x\n",
		p->REG_h190.reg_filterop_op2_csc_tab_sw_0);
	pr_info("\tREG_h190.reg_filterop_op2_csc_tab_sw_1 = 0x%x\n",
		p->REG_h190.reg_filterop_op2_csc_tab_sw_1);
	pr_info("\tREG_h194.reg_filterop_op2_csc_tab_sw_update = 0x%x\n",
		p->REG_h194.reg_filterop_op2_csc_tab_sw_update);
	pr_info("\tREG_h194.reg_filterop_op2_csc_coeff_sw_update = 0x%x\n",
		p->REG_h194.reg_filterop_op2_csc_coeff_sw_update);
	pr_info("\tREG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n",
		p->REG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00);
	pr_info("\tREG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n",
		p->REG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01);
	pr_info("\tREG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n",
		p->REG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02);
	pr_info("\tREG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n",
		p->REG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03);
	pr_info("\tREG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n",
		p->REG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04);
	pr_info("\tREG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n",
		p->REG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05);
	pr_info("\tREG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n",
		p->REG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06);
	pr_info("\tREG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n",
		p->REG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07);
	pr_info("\tREG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n",
		p->REG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08);
	pr_info("\tREG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n",
		p->REG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09);
	pr_info("\tREG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n",
		p->REG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10);
	pr_info("\tREG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n",
		p->REG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11);
	pr_info("\tREG_h1c8.reg_filterop_op2_csc_enmode = 0x%x\n",
		p->REG_h1c8.reg_filterop_op2_csc_enmode);
	pr_info("\tREG_h1c8.reg_filterop_op2_csc_enable = 0x%x\n",
		p->REG_h1c8.reg_filterop_op2_csc_enable);
	pr_info("\tREG_cropy_s.reg_crop_y_start_x = 0x%x\n",
		p->REG_cropy_s.reg_crop_y_start_x);
	pr_info("\tREG_cropy_s.reg_crop_y_end_x = 0x%x\n",
		p->REG_cropy_s.reg_crop_y_end_x);
	pr_info("\tREG_cropy_e.reg_crop_y_start_y = 0x%x\n",
		p->REG_cropy_e.reg_crop_y_start_y);
	pr_info("\tREG_cropy_e.reg_crop_y_end_y = 0x%x\n",
		p->REG_cropy_e.reg_crop_y_end_y);
	pr_info("\tREG_cropy_ctl.reg_crop_y_enable = 0x%x\n",
		p->REG_cropy_ctl.reg_crop_y_enable);
	pr_info("\tREG_cropc_s.reg_crop_c_start_x = 0x%x\n",
		p->REG_cropc_s.reg_crop_c_start_x);
	pr_info("\tREG_cropc_s.reg_crop_c_end_x = 0x%x\n",
		p->REG_cropc_s.reg_crop_c_end_x);
	pr_info("\tREG_cropc_e.reg_crop_c_start_y = 0x%x\n",
		p->REG_cropc_e.reg_crop_c_start_y);
	pr_info("\tREG_cropc_e.reg_crop_c_end_y = 0x%x\n",
		p->REG_cropc_e.reg_crop_c_end_y);
	pr_info("\tREG_cropc_ctl.reg_crop_c_enable = 0x%x\n",
		p->REG_cropc_ctl.reg_crop_c_enable);
	pr_info("\tREG_crop_odma_s.reg_crop_odma_start_x = 0x%x\n",
		p->REG_crop_odma_s.reg_crop_odma_start_x);
	pr_info("\tREG_crop_odma_s.reg_crop_odma_end_x = 0x%x\n",
		p->REG_crop_odma_s.reg_crop_odma_end_x);
	pr_info("\tREG_crop_odma_e.reg_crop_odma_start_y = 0x%x\n",
		p->REG_crop_odma_e.reg_crop_odma_start_y);
	pr_info("\tREG_crop_odma_e.reg_crop_odma_end_y = 0x%x\n",
		p->REG_crop_odma_e.reg_crop_odma_end_y);
	pr_info("\tREG_crop_odma_ctl.reg_crop_odma_enable = 0x%x\n",
		p->REG_crop_odma_ctl.reg_crop_odma_enable);
}

#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_filterop_dump_ini(FILE* fp, IVE_FILTEROP_C* p) {
	fprintf(fp, "reg_softrst = 0x%x\n",p->REG_1.reg_softrst);
	fprintf(fp, "reg_softrst_wdma_y = 0x%x\n",p->REG_1.reg_softrst_wdma_y);
	fprintf(fp, "reg_softrst_wdma_c = 0x%x\n",p->REG_1.reg_softrst_wdma_c);
	fprintf(fp, "reg_softrst_rdma_gradfg = 0x%x\n",p->REG_1.reg_softrst_rdma_gradfg);
	fprintf(fp, "reg_softrst_op1 = 0x%x\n",p->REG_1.reg_softrst_op1);
	fprintf(fp, "reg_softrst_filter3ch = 0x%x\n",p->REG_1.reg_softrst_filter3ch);
	fprintf(fp, "reg_softrst_st = 0x%x\n",p->REG_1.reg_softrst_st);
	fprintf(fp, "reg_softrst_odma = 0x%x\n",p->REG_1.reg_softrst_odma);
	fprintf(fp, "reg_gradfg_bggrad_rdma_en = 0x%x\n",p->REG_H04.reg_gradfg_bggrad_rdma_en);
	fprintf(fp, "reg_gradfg_bggrad_uv_swap = 0x%x\n",p->REG_H04.reg_gradfg_bggrad_uv_swap);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->REG_2.reg_shdw_sel);
	fprintf(fp, "reg_ctrl_dmy1 = 0x%x\n",p->REG_3.reg_ctrl_dmy1);
	fprintf(fp, "reg_filterop_mode = 0x%x\n",p->REG_h10.reg_filterop_mode);
	fprintf(fp, "reg_filterop_op1_cmd = 0x%x\n",p->REG_h14.reg_filterop_op1_cmd);
	fprintf(fp, "reg_filterop_sw_ovw_op = 0x%x\n",p->REG_h14.reg_filterop_sw_ovw_op);
	fprintf(fp, "reg_filterop_3ch_en = 0x%x\n",p->REG_h14.reg_filterop_3ch_en);
	fprintf(fp, "reg_op_y_wdma_en = 0x%x\n",p->REG_h14.reg_op_y_wdma_en);
	fprintf(fp, "reg_op_c_wdma_en = 0x%x\n",p->REG_h14.reg_op_c_wdma_en);
	fprintf(fp, "reg_op_y_wdma_w1b_en = 0x%x\n",p->REG_h14.reg_op_y_wdma_w1b_en);
	fprintf(fp, "reg_op_c_wdma_w1b_en = 0x%x\n",p->REG_h14.reg_op_c_wdma_w1b_en);
	fprintf(fp, "reg_filterop_h_coef00 = 0x%x\n",p->REG_4.reg_filterop_h_coef00);
	fprintf(fp, "reg_filterop_h_coef01 = 0x%x\n",p->REG_4.reg_filterop_h_coef01);
	fprintf(fp, "reg_filterop_h_coef02 = 0x%x\n",p->REG_4.reg_filterop_h_coef02);
	fprintf(fp, "reg_filterop_h_coef03 = 0x%x\n",p->REG_4.reg_filterop_h_coef03);
	fprintf(fp, "reg_filterop_h_coef04 = 0x%x\n",p->REG_5.reg_filterop_h_coef04);
	fprintf(fp, "reg_filterop_h_coef10 = 0x%x\n",p->REG_5.reg_filterop_h_coef10);
	fprintf(fp, "reg_filterop_h_coef11 = 0x%x\n",p->REG_5.reg_filterop_h_coef11);
	fprintf(fp, "reg_filterop_h_coef12 = 0x%x\n",p->REG_5.reg_filterop_h_coef12);
	fprintf(fp, "reg_filterop_h_coef13 = 0x%x\n",p->REG_6.reg_filterop_h_coef13);
	fprintf(fp, "reg_filterop_h_coef14 = 0x%x\n",p->REG_6.reg_filterop_h_coef14);
	fprintf(fp, "reg_filterop_h_coef20 = 0x%x\n",p->REG_6.reg_filterop_h_coef20);
	fprintf(fp, "reg_filterop_h_coef21 = 0x%x\n",p->REG_6.reg_filterop_h_coef21);
	fprintf(fp, "reg_filterop_h_coef22 = 0x%x\n",p->REG_7.reg_filterop_h_coef22);
	fprintf(fp, "reg_filterop_h_coef23 = 0x%x\n",p->REG_7.reg_filterop_h_coef23);
	fprintf(fp, "reg_filterop_h_coef24 = 0x%x\n",p->REG_7.reg_filterop_h_coef24);
	fprintf(fp, "reg_filterop_h_coef30 = 0x%x\n",p->REG_7.reg_filterop_h_coef30);
	fprintf(fp, "reg_filterop_h_coef31 = 0x%x\n",p->REG_8.reg_filterop_h_coef31);
	fprintf(fp, "reg_filterop_h_coef32 = 0x%x\n",p->REG_8.reg_filterop_h_coef32);
	fprintf(fp, "reg_filterop_h_coef33 = 0x%x\n",p->REG_8.reg_filterop_h_coef33);
	fprintf(fp, "reg_filterop_h_coef34 = 0x%x\n",p->REG_8.reg_filterop_h_coef34);
	fprintf(fp, "reg_filterop_h_coef40 = 0x%x\n",p->REG_9.reg_filterop_h_coef40);
	fprintf(fp, "reg_filterop_h_coef41 = 0x%x\n",p->REG_9.reg_filterop_h_coef41);
	fprintf(fp, "reg_filterop_h_coef42 = 0x%x\n",p->REG_9.reg_filterop_h_coef42);
	fprintf(fp, "reg_filterop_h_coef43 = 0x%x\n",p->REG_9.reg_filterop_h_coef43);
	fprintf(fp, "reg_filterop_h_coef44 = 0x%x\n",p->REG_10.reg_filterop_h_coef44);
	fprintf(fp, "reg_filterop_h_norm = 0x%x\n",p->REG_10.reg_filterop_h_norm);
	fprintf(fp, "reg_filterop_v_coef00 = 0x%x\n",p->REG_11.reg_filterop_v_coef00);
	fprintf(fp, "reg_filterop_v_coef01 = 0x%x\n",p->REG_11.reg_filterop_v_coef01);
	fprintf(fp, "reg_filterop_v_coef02 = 0x%x\n",p->REG_11.reg_filterop_v_coef02);
	fprintf(fp, "reg_filterop_v_coef03 = 0x%x\n",p->REG_11.reg_filterop_v_coef03);
	fprintf(fp, "reg_filterop_v_coef04 = 0x%x\n",p->REG_12.reg_filterop_v_coef04);
	fprintf(fp, "reg_filterop_v_coef10 = 0x%x\n",p->REG_12.reg_filterop_v_coef10);
	fprintf(fp, "reg_filterop_v_coef11 = 0x%x\n",p->REG_12.reg_filterop_v_coef11);
	fprintf(fp, "reg_filterop_v_coef12 = 0x%x\n",p->REG_12.reg_filterop_v_coef12);
	fprintf(fp, "reg_filterop_v_coef13 = 0x%x\n",p->REG_13.reg_filterop_v_coef13);
	fprintf(fp, "reg_filterop_v_coef14 = 0x%x\n",p->REG_13.reg_filterop_v_coef14);
	fprintf(fp, "reg_filterop_v_coef20 = 0x%x\n",p->REG_13.reg_filterop_v_coef20);
	fprintf(fp, "reg_filterop_v_coef21 = 0x%x\n",p->REG_13.reg_filterop_v_coef21);
	fprintf(fp, "reg_filterop_v_coef22 = 0x%x\n",p->REG_14.reg_filterop_v_coef22);
	fprintf(fp, "reg_filterop_v_coef23 = 0x%x\n",p->REG_14.reg_filterop_v_coef23);
	fprintf(fp, "reg_filterop_v_coef24 = 0x%x\n",p->REG_14.reg_filterop_v_coef24);
	fprintf(fp, "reg_filterop_v_coef30 = 0x%x\n",p->REG_14.reg_filterop_v_coef30);
	fprintf(fp, "reg_filterop_v_coef31 = 0x%x\n",p->REG_15.reg_filterop_v_coef31);
	fprintf(fp, "reg_filterop_v_coef32 = 0x%x\n",p->REG_15.reg_filterop_v_coef32);
	fprintf(fp, "reg_filterop_v_coef33 = 0x%x\n",p->REG_15.reg_filterop_v_coef33);
	fprintf(fp, "reg_filterop_v_coef34 = 0x%x\n",p->REG_15.reg_filterop_v_coef34);
	fprintf(fp, "reg_filterop_v_coef40 = 0x%x\n",p->REG_16.reg_filterop_v_coef40);
	fprintf(fp, "reg_filterop_v_coef41 = 0x%x\n",p->REG_16.reg_filterop_v_coef41);
	fprintf(fp, "reg_filterop_v_coef42 = 0x%x\n",p->REG_16.reg_filterop_v_coef42);
	fprintf(fp, "reg_filterop_v_coef43 = 0x%x\n",p->REG_16.reg_filterop_v_coef43);
	fprintf(fp, "reg_filterop_v_coef44 = 0x%x\n",p->REG_17.reg_filterop_v_coef44);
	fprintf(fp, "reg_filterop_v_norm = 0x%x\n",p->REG_17.reg_filterop_v_norm);
	fprintf(fp, "reg_filterop_mode_trans = 0x%x\n",p->REG_18.reg_filterop_mode_trans);
	fprintf(fp, "reg_filterop_order_enmode = 0x%x\n",p->REG_18.reg_filterop_order_enmode);
	fprintf(fp, "reg_filterop_mag_thr = 0x%x\n",p->REG_18.reg_filterop_mag_thr);
	fprintf(fp, "reg_filterop_bernsen_win5x5_en = 0x%x\n",p->REG_19.reg_filterop_bernsen_win5x5_en);
	fprintf(fp, "reg_filterop_bernsen_mode = 0x%x\n",p->REG_19.reg_filterop_bernsen_mode);
	fprintf(fp, "reg_filterop_bernsen_thr = 0x%x\n",p->REG_19.reg_filterop_bernsen_thr);
	fprintf(fp, "reg_filterop_u8ContrastThreshold = 0x%x\n",p->REG_19.reg_filterop_u8ContrastThreshold);
	fprintf(fp, "reg_filterop_lbp_u8bit_thr = 0x%x\n",p->REG_20.reg_filterop_lbp_u8bit_thr);
	fprintf(fp, "reg_filterop_lbp_s8bit_thr = 0x%x\n",p->REG_20.reg_filterop_lbp_s8bit_thr);
	fprintf(fp, "reg_filterop_lbp_enmode = 0x%x\n",p->REG_20.reg_filterop_lbp_enmode);
	fprintf(fp, "reg_filterop_op2_erodila_coef00 = 0x%x\n",p->REG_21.reg_filterop_op2_erodila_coef00);
	fprintf(fp, "reg_filterop_op2_erodila_coef01 = 0x%x\n",p->REG_21.reg_filterop_op2_erodila_coef01);
	fprintf(fp, "reg_filterop_op2_erodila_coef02 = 0x%x\n",p->REG_21.reg_filterop_op2_erodila_coef02);
	fprintf(fp, "reg_filterop_op2_erodila_coef03 = 0x%x\n",p->REG_21.reg_filterop_op2_erodila_coef03);
	fprintf(fp, "reg_filterop_op2_erodila_coef04 = 0x%x\n",p->REG_22.reg_filterop_op2_erodila_coef04);
	fprintf(fp, "reg_filterop_op2_erodila_coef10 = 0x%x\n",p->REG_22.reg_filterop_op2_erodila_coef10);
	fprintf(fp, "reg_filterop_op2_erodila_coef11 = 0x%x\n",p->REG_22.reg_filterop_op2_erodila_coef11);
	fprintf(fp, "reg_filterop_op2_erodila_coef12 = 0x%x\n",p->REG_22.reg_filterop_op2_erodila_coef12);
	fprintf(fp, "reg_filterop_op2_erodila_coef13 = 0x%x\n",p->REG_23.reg_filterop_op2_erodila_coef13);
	fprintf(fp, "reg_filterop_op2_erodila_coef14 = 0x%x\n",p->REG_23.reg_filterop_op2_erodila_coef14);
	fprintf(fp, "reg_filterop_op2_erodila_coef20 = 0x%x\n",p->REG_23.reg_filterop_op2_erodila_coef20);
	fprintf(fp, "reg_filterop_op2_erodila_coef21 = 0x%x\n",p->REG_23.reg_filterop_op2_erodila_coef21);
	fprintf(fp, "reg_filterop_op2_erodila_coef22 = 0x%x\n",p->REG_24.reg_filterop_op2_erodila_coef22);
	fprintf(fp, "reg_filterop_op2_erodila_coef23 = 0x%x\n",p->REG_24.reg_filterop_op2_erodila_coef23);
	fprintf(fp, "reg_filterop_op2_erodila_coef24 = 0x%x\n",p->REG_24.reg_filterop_op2_erodila_coef24);
	fprintf(fp, "reg_filterop_op2_erodila_coef30 = 0x%x\n",p->REG_24.reg_filterop_op2_erodila_coef30);
	fprintf(fp, "reg_filterop_op2_erodila_coef31 = 0x%x\n",p->REG_25.reg_filterop_op2_erodila_coef31);
	fprintf(fp, "reg_filterop_op2_erodila_coef32 = 0x%x\n",p->REG_25.reg_filterop_op2_erodila_coef32);
	fprintf(fp, "reg_filterop_op2_erodila_coef33 = 0x%x\n",p->REG_25.reg_filterop_op2_erodila_coef33);
	fprintf(fp, "reg_filterop_op2_erodila_coef34 = 0x%x\n",p->REG_25.reg_filterop_op2_erodila_coef34);
	fprintf(fp, "reg_filterop_op2_erodila_coef40 = 0x%x\n",p->REG_26.reg_filterop_op2_erodila_coef40);
	fprintf(fp, "reg_filterop_op2_erodila_coef41 = 0x%x\n",p->REG_26.reg_filterop_op2_erodila_coef41);
	fprintf(fp, "reg_filterop_op2_erodila_coef42 = 0x%x\n",p->REG_26.reg_filterop_op2_erodila_coef42);
	fprintf(fp, "reg_filterop_op2_erodila_coef43 = 0x%x\n",p->REG_26.reg_filterop_op2_erodila_coef43);
	fprintf(fp, "reg_filterop_op2_erodila_coef44 = 0x%x\n",p->REG_27.reg_filterop_op2_erodila_coef44);
	fprintf(fp, "reg_filterop_op2_erodila_en = 0x%x\n",p->REG_28.reg_filterop_op2_erodila_en);
	fprintf(fp, "reg_csc_dbg_en = 0x%x\n",p->REG_CSC_DBG_COEFF.reg_csc_dbg_en);
	fprintf(fp, "reg_csc_dbg_coeff_sel = 0x%x\n",p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel);
	fprintf(fp, "reg_csc_dbg_coeff = 0x%x\n",p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff);
	fprintf(fp, "reg_csc_dbg_prob_x = 0x%x\n",p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x);
	fprintf(fp, "reg_csc_dbg_prob_y = 0x%x\n",p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y);
	fprintf(fp, "reg_csc_dbg_src_data = 0x%x\n",p->REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data);
	fprintf(fp, "reg_csc_dbg_dst_data = 0x%x\n",p->REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data);
	fprintf(fp, "reg_filterop_op2_gradfg_en = 0x%x\n",p->REG_33.reg_filterop_op2_gradfg_en);
	fprintf(fp, "reg_filterop_op2_gradfg_softrst = 0x%x\n",p->REG_33.reg_filterop_op2_gradfg_softrst);
	fprintf(fp, "reg_filterop_op2_gradfg_enmode = 0x%x\n",p->REG_33.reg_filterop_op2_gradfg_enmode);
	fprintf(fp, "reg_filterop_op2_gradfg_edwdark = 0x%x\n",p->REG_33.reg_filterop_op2_gradfg_edwdark);
	fprintf(fp, "reg_filterop_op2_gradfg_edwfactor = 0x%x\n",p->REG_33.reg_filterop_op2_gradfg_edwfactor);
	fprintf(fp, "reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n",p->REG_34.reg_filterop_op2_gradfg_crlcoefthr);
	fprintf(fp, "reg_filterop_op2_gradfg_magcrlthr = 0x%x\n",p->REG_34.reg_filterop_op2_gradfg_magcrlthr);
	fprintf(fp, "reg_filterop_op2_gradfg_minmagdiff = 0x%x\n",p->REG_34.reg_filterop_op2_gradfg_minmagdiff);
	fprintf(fp, "reg_filterop_op2_gradfg_noiseval = 0x%x\n",p->REG_34.reg_filterop_op2_gradfg_noiseval);
	fprintf(fp, "reg_filterop_map_enmode = 0x%x\n",p->REG_110.reg_filterop_map_enmode);
	fprintf(fp, "reg_filterop_norm_out_ctrl = 0x%x\n",p->REG_110.reg_filterop_norm_out_ctrl);
	fprintf(fp, "reg_filterop_magang_out_ctrl = 0x%x\n",p->REG_110.reg_filterop_magang_out_ctrl);
	fprintf(fp, "reg_dma_blen = 0x%x\n",p->ODMA_REG_00.reg_dma_blen);
	fprintf(fp, "reg_dma_en = 0x%x\n",p->ODMA_REG_00.reg_dma_en);
	fprintf(fp, "reg_fmt_sel = 0x%x\n",p->ODMA_REG_00.reg_fmt_sel);
	fprintf(fp, "reg_sc_odma_hflip = 0x%x\n",p->ODMA_REG_00.reg_sc_odma_hflip);
	fprintf(fp, "reg_sc_odma_vflip = 0x%x\n",p->ODMA_REG_00.reg_sc_odma_vflip);
	fprintf(fp, "reg_sc_422_avg = 0x%x\n",p->ODMA_REG_00.reg_sc_422_avg);
	fprintf(fp, "reg_sc_420_avg = 0x%x\n",p->ODMA_REG_00.reg_sc_420_avg);
	fprintf(fp, "reg_c_round_mode = 0x%x\n",p->ODMA_REG_00.reg_c_round_mode);
	fprintf(fp, "reg_bf16_en = 0x%x\n",p->ODMA_REG_00.reg_bf16_en);
	fprintf(fp, "reg_dma_y_base_low_part = 0x%x\n",p->ODMA_REG_01.reg_dma_y_base_low_part);
	fprintf(fp, "reg_dma_y_base_high_part = 0x%x\n",p->ODMA_REG_02.reg_dma_y_base_high_part);
	fprintf(fp, "reg_dma_u_base_low_part = 0x%x\n",p->ODMA_REG_03.reg_dma_u_base_low_part);
	fprintf(fp, "reg_dma_u_base_high_part = 0x%x\n",p->ODMA_REG_04.reg_dma_u_base_high_part);
	fprintf(fp, "reg_dma_v_base_low_part = 0x%x\n",p->ODMA_REG_05.reg_dma_v_base_low_part);
	fprintf(fp, "reg_dma_v_base_high_part = 0x%x\n",p->ODMA_REG_06.reg_dma_v_base_high_part);
	fprintf(fp, "reg_dma_y_pitch = 0x%x\n",p->ODMA_REG_07.reg_dma_y_pitch);
	fprintf(fp, "reg_dma_c_pitch = 0x%x\n",p->ODMA_REG_08.reg_dma_c_pitch);
	fprintf(fp, "reg_dma_x_str = 0x%x\n",p->ODMA_REG_09.reg_dma_x_str);
	fprintf(fp, "reg_dma_y_str = 0x%x\n",p->ODMA_REG_10.reg_dma_y_str);
	fprintf(fp, "reg_dma_wd = 0x%x\n",p->ODMA_REG_11.reg_dma_wd);
	fprintf(fp, "reg_dma_ht = 0x%x\n",p->ODMA_REG_12.reg_dma_ht);
	fprintf(fp, "reg_dma_debug = 0x%x\n",p->ODMA_REG_13.reg_dma_debug);
	fprintf(fp, "reg_dma_int_line_target = 0x%x\n",p->ODMA_REG_14.reg_dma_int_line_target);
	fprintf(fp, "reg_dma_int_line_target_sel = 0x%x\n",p->ODMA_REG_14.reg_dma_int_line_target_sel);
	fprintf(fp, "reg_dma_int_cycle_line_target = 0x%x\n",p->ODMA_REG_15.reg_dma_int_cycle_line_target);
	fprintf(fp, "reg_dma_int_cycle_line_target_sel = 0x%x\n",p->ODMA_REG_15.reg_dma_int_cycle_line_target_sel);
	fprintf(fp, "reg_dma_latch_line_cnt = 0x%x\n",p->ODMA_REG_16.reg_dma_latch_line_cnt);
	fprintf(fp, "reg_dma_latched_line_cnt = 0x%x\n",p->ODMA_REG_16.reg_dma_latched_line_cnt);
	fprintf(fp, "axi_active = 0x%x\n",p->ODMA_REG_17.axi_active);
	fprintf(fp, "axi_y_active = 0x%x\n",p->ODMA_REG_17.axi_y_active);
	fprintf(fp, "axi_u_active = 0x%x\n",p->ODMA_REG_17.axi_u_active);
	fprintf(fp, "axi_v_active = 0x%x\n",p->ODMA_REG_17.axi_v_active);
	fprintf(fp, "y_buf_full = 0x%x\n",p->ODMA_REG_17.y_buf_full);
	fprintf(fp, "y_buf_empty = 0x%x\n",p->ODMA_REG_17.y_buf_empty);
	fprintf(fp, "u_buf_full = 0x%x\n",p->ODMA_REG_17.u_buf_full);
	fprintf(fp, "u_buf_empty = 0x%x\n",p->ODMA_REG_17.u_buf_empty);
	fprintf(fp, "v_buf_full = 0x%x\n",p->ODMA_REG_17.v_buf_full);
	fprintf(fp, "v_buf_empty = 0x%x\n",p->ODMA_REG_17.v_buf_empty);
	fprintf(fp, "line_target_hit = 0x%x\n",p->ODMA_REG_17.line_target_hit);
	fprintf(fp, "cycle_line_target_hit = 0x%x\n",p->ODMA_REG_17.cycle_line_target_hit);
	fprintf(fp, "axi_cmd_cs = 0x%x\n",p->ODMA_REG_17.axi_cmd_cs);
	fprintf(fp, "y_line_cnt = 0x%x\n",p->ODMA_REG_17.y_line_cnt);
	fprintf(fp, "reg_canny_lowthr = 0x%x\n",p->REG_CANNY_0.reg_canny_lowthr);
	fprintf(fp, "reg_canny_hithr = 0x%x\n",p->REG_CANNY_0.reg_canny_hithr);
	fprintf(fp, "reg_canny_en = 0x%x\n",p->REG_CANNY_1.reg_canny_en);
	fprintf(fp, "reg_canny_strong_point_cnt_en = 0x%x\n",p->REG_CANNY_1.reg_canny_strong_point_cnt_en);
	fprintf(fp, "reg_canny_non_or_weak = 0x%x\n",p->REG_CANNY_1.reg_canny_non_or_weak);
	fprintf(fp, "reg_canny_strong_point_cnt = 0x%x\n",p->REG_CANNY_1.reg_canny_strong_point_cnt);
	fprintf(fp, "reg_canny_eof = 0x%x\n",p->REG_CANNY_2.reg_canny_eof);
	fprintf(fp, "reg_canny_basel = 0x%x\n",p->REG_CANNY_3.reg_canny_basel);
	fprintf(fp, "reg_canny_baseh = 0x%x\n",p->REG_CANNY_4.reg_canny_baseh);
	fprintf(fp, "reg_st_candi_corner_bypass = 0x%x\n",p->REG_ST_CANDI_0.reg_st_candi_corner_bypass);
	fprintf(fp, "reg_st_candi_corner_switch_src = 0x%x\n",p->REG_ST_CANDI_0.reg_st_candi_corner_switch_src);
	fprintf(fp, "reg_st_eigval_max_eigval = 0x%x\n",p->REG_ST_EIGVAL_0.reg_st_eigval_max_eigval);
	fprintf(fp, "reg_st_eigval_tile_num = 0x%x\n",p->REG_ST_EIGVAL_0.reg_st_eigval_tile_num);
	fprintf(fp, "reg_sw_clr_max_eigval = 0x%x\n",p->REG_ST_EIGVAL_1.reg_sw_clr_max_eigval);
	fprintf(fp, "reg_filterop_op2_csc_tab_sw_0 = 0x%x\n",p->REG_h190.reg_filterop_op2_csc_tab_sw_0);
	fprintf(fp, "reg_filterop_op2_csc_tab_sw_1 = 0x%x\n",p->REG_h190.reg_filterop_op2_csc_tab_sw_1);
	fprintf(fp, "reg_filterop_op2_csc_tab_sw_update = 0x%x\n",p->REG_h194.reg_filterop_op2_csc_tab_sw_update);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_update = 0x%x\n",p->REG_h194.reg_filterop_op2_csc_coeff_sw_update);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n",p->REG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n",p->REG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n",p->REG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n",p->REG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n",p->REG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n",p->REG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n",p->REG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n",p->REG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n",p->REG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n",p->REG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n",p->REG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n",p->REG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11);
	fprintf(fp, "reg_filterop_op2_csc_enmode = 0x%x\n",p->REG_h1c8.reg_filterop_op2_csc_enmode);
	fprintf(fp, "reg_filterop_op2_csc_enable = 0x%x\n",p->REG_h1c8.reg_filterop_op2_csc_enable);
	fprintf(fp, "reg_crop_y_start_x = 0x%x\n",p->REG_cropy_s.reg_crop_y_start_x);
	fprintf(fp, "reg_crop_y_end_x = 0x%x\n",p->REG_cropy_s.reg_crop_y_end_x);
	fprintf(fp, "reg_crop_y_start_y = 0x%x\n",p->REG_cropy_e.reg_crop_y_start_y);
	fprintf(fp, "reg_crop_y_end_y = 0x%x\n",p->REG_cropy_e.reg_crop_y_end_y);
	fprintf(fp, "reg_crop_y_enable = 0x%x\n",p->REG_cropy_ctl.reg_crop_y_enable);
	fprintf(fp, "reg_crop_c_start_x = 0x%x\n",p->REG_cropc_s.reg_crop_c_start_x);
	fprintf(fp, "reg_crop_c_end_x = 0x%x\n",p->REG_cropc_s.reg_crop_c_end_x);
	fprintf(fp, "reg_crop_c_start_y = 0x%x\n",p->REG_cropc_e.reg_crop_c_start_y);
	fprintf(fp, "reg_crop_c_end_y = 0x%x\n",p->REG_cropc_e.reg_crop_c_end_y);
	fprintf(fp, "reg_crop_c_enable = 0x%x\n",p->REG_cropc_ctl.reg_crop_c_enable);
	fprintf(fp, "reg_crop_odma_start_x = 0x%x\n",p->REG_crop_odma_s.reg_crop_odma_start_x);
	fprintf(fp, "reg_crop_odma_end_x = 0x%x\n",p->REG_crop_odma_s.reg_crop_odma_end_x);
	fprintf(fp, "reg_crop_odma_start_y = 0x%x\n",p->REG_crop_odma_e.reg_crop_odma_start_y);
	fprintf(fp, "reg_crop_odma_end_y = 0x%x\n",p->REG_crop_odma_e.reg_crop_odma_end_y);
	fprintf(fp, "reg_crop_odma_enable = 0x%x\n",p->REG_crop_odma_ctl.reg_crop_odma_enable);

}
static void ive_filterop_print(IVE_FILTEROP_C* p) {
    fprintf(stderr, "ive_filterop\n");
	fprintf(stderr, "\tREG_1.reg_softrst = 0x%x\n", p->REG_1.reg_softrst);
	fprintf(stderr, "\tREG_1.reg_softrst_wdma_y = 0x%x\n", p->REG_1.reg_softrst_wdma_y);
	fprintf(stderr, "\tREG_1.reg_softrst_wdma_c = 0x%x\n", p->REG_1.reg_softrst_wdma_c);
	fprintf(stderr, "\tREG_1.reg_softrst_rdma_gradfg = 0x%x\n", p->REG_1.reg_softrst_rdma_gradfg);
	fprintf(stderr, "\tREG_1.reg_softrst_op1 = 0x%x\n", p->REG_1.reg_softrst_op1);
	fprintf(stderr, "\tREG_1.reg_softrst_filter3ch = 0x%x\n", p->REG_1.reg_softrst_filter3ch);
	fprintf(stderr, "\tREG_1.reg_softrst_st = 0x%x\n", p->REG_1.reg_softrst_st);
	fprintf(stderr, "\tREG_1.reg_softrst_odma = 0x%x\n", p->REG_1.reg_softrst_odma);
	fprintf(stderr, "\tREG_H04.reg_gradfg_bggrad_rdma_en = 0x%x\n", p->REG_H04.reg_gradfg_bggrad_rdma_en);
	fprintf(stderr, "\tREG_H04.reg_gradfg_bggrad_uv_swap = 0x%x\n", p->REG_H04.reg_gradfg_bggrad_uv_swap);
	fprintf(stderr, "\tREG_2.reg_shdw_sel = 0x%x\n", p->REG_2.reg_shdw_sel);
	fprintf(stderr, "\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->REG_3.reg_ctrl_dmy1);
	fprintf(stderr, "\tREG_h10.reg_filterop_mode = 0x%x\n", p->REG_h10.reg_filterop_mode);
	fprintf(stderr, "\tREG_h14.reg_filterop_op1_cmd = 0x%x\n", p->REG_h14.reg_filterop_op1_cmd);
	fprintf(stderr, "\tREG_h14.reg_filterop_sw_ovw_op = 0x%x\n", p->REG_h14.reg_filterop_sw_ovw_op);
	fprintf(stderr, "\tREG_h14.reg_filterop_3ch_en = 0x%x\n", p->REG_h14.reg_filterop_3ch_en);
	fprintf(stderr, "\tREG_h14.reg_op_y_wdma_en = 0x%x\n", p->REG_h14.reg_op_y_wdma_en);
	fprintf(stderr, "\tREG_h14.reg_op_c_wdma_en = 0x%x\n", p->REG_h14.reg_op_c_wdma_en);
	fprintf(stderr, "\tREG_h14.reg_op_y_wdma_w1b_en = 0x%x\n", p->REG_h14.reg_op_y_wdma_w1b_en);
	fprintf(stderr, "\tREG_h14.reg_op_c_wdma_w1b_en = 0x%x\n", p->REG_h14.reg_op_c_wdma_w1b_en);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef00 = 0x%x\n", p->REG_4.reg_filterop_h_coef00);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef01 = 0x%x\n", p->REG_4.reg_filterop_h_coef01);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef02 = 0x%x\n", p->REG_4.reg_filterop_h_coef02);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef03 = 0x%x\n", p->REG_4.reg_filterop_h_coef03);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef04 = 0x%x\n", p->REG_5.reg_filterop_h_coef04);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef10 = 0x%x\n", p->REG_5.reg_filterop_h_coef10);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef11 = 0x%x\n", p->REG_5.reg_filterop_h_coef11);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef12 = 0x%x\n", p->REG_5.reg_filterop_h_coef12);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef13 = 0x%x\n", p->REG_6.reg_filterop_h_coef13);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef14 = 0x%x\n", p->REG_6.reg_filterop_h_coef14);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef20 = 0x%x\n", p->REG_6.reg_filterop_h_coef20);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef21 = 0x%x\n", p->REG_6.reg_filterop_h_coef21);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef22 = 0x%x\n", p->REG_7.reg_filterop_h_coef22);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef23 = 0x%x\n", p->REG_7.reg_filterop_h_coef23);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef24 = 0x%x\n", p->REG_7.reg_filterop_h_coef24);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef30 = 0x%x\n", p->REG_7.reg_filterop_h_coef30);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef31 = 0x%x\n", p->REG_8.reg_filterop_h_coef31);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef32 = 0x%x\n", p->REG_8.reg_filterop_h_coef32);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef33 = 0x%x\n", p->REG_8.reg_filterop_h_coef33);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef34 = 0x%x\n", p->REG_8.reg_filterop_h_coef34);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef40 = 0x%x\n", p->REG_9.reg_filterop_h_coef40);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef41 = 0x%x\n", p->REG_9.reg_filterop_h_coef41);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef42 = 0x%x\n", p->REG_9.reg_filterop_h_coef42);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef43 = 0x%x\n", p->REG_9.reg_filterop_h_coef43);
	fprintf(stderr, "\tREG_10.reg_filterop_h_coef44 = 0x%x\n", p->REG_10.reg_filterop_h_coef44);
	fprintf(stderr, "\tREG_10.reg_filterop_h_norm = 0x%x\n", p->REG_10.reg_filterop_h_norm);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef00 = 0x%x\n", p->REG_11.reg_filterop_v_coef00);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef01 = 0x%x\n", p->REG_11.reg_filterop_v_coef01);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef02 = 0x%x\n", p->REG_11.reg_filterop_v_coef02);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef03 = 0x%x\n", p->REG_11.reg_filterop_v_coef03);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef04 = 0x%x\n", p->REG_12.reg_filterop_v_coef04);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef10 = 0x%x\n", p->REG_12.reg_filterop_v_coef10);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef11 = 0x%x\n", p->REG_12.reg_filterop_v_coef11);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef12 = 0x%x\n", p->REG_12.reg_filterop_v_coef12);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef13 = 0x%x\n", p->REG_13.reg_filterop_v_coef13);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef14 = 0x%x\n", p->REG_13.reg_filterop_v_coef14);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef20 = 0x%x\n", p->REG_13.reg_filterop_v_coef20);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef21 = 0x%x\n", p->REG_13.reg_filterop_v_coef21);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef22 = 0x%x\n", p->REG_14.reg_filterop_v_coef22);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef23 = 0x%x\n", p->REG_14.reg_filterop_v_coef23);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef24 = 0x%x\n", p->REG_14.reg_filterop_v_coef24);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef30 = 0x%x\n", p->REG_14.reg_filterop_v_coef30);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef31 = 0x%x\n", p->REG_15.reg_filterop_v_coef31);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef32 = 0x%x\n", p->REG_15.reg_filterop_v_coef32);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef33 = 0x%x\n", p->REG_15.reg_filterop_v_coef33);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef34 = 0x%x\n", p->REG_15.reg_filterop_v_coef34);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef40 = 0x%x\n", p->REG_16.reg_filterop_v_coef40);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef41 = 0x%x\n", p->REG_16.reg_filterop_v_coef41);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef42 = 0x%x\n", p->REG_16.reg_filterop_v_coef42);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef43 = 0x%x\n", p->REG_16.reg_filterop_v_coef43);
	fprintf(stderr, "\tREG_17.reg_filterop_v_coef44 = 0x%x\n", p->REG_17.reg_filterop_v_coef44);
	fprintf(stderr, "\tREG_17.reg_filterop_v_norm = 0x%x\n", p->REG_17.reg_filterop_v_norm);
	fprintf(stderr, "\tREG_18.reg_filterop_mode_trans = 0x%x\n", p->REG_18.reg_filterop_mode_trans);
	fprintf(stderr, "\tREG_18.reg_filterop_order_enmode = 0x%x\n", p->REG_18.reg_filterop_order_enmode);
	fprintf(stderr, "\tREG_18.reg_filterop_mag_thr = 0x%x\n", p->REG_18.reg_filterop_mag_thr);
	fprintf(stderr, "\tREG_19.reg_filterop_bernsen_win5x5_en = 0x%x\n", p->REG_19.reg_filterop_bernsen_win5x5_en);
	fprintf(stderr, "\tREG_19.reg_filterop_bernsen_mode = 0x%x\n", p->REG_19.reg_filterop_bernsen_mode);
	fprintf(stderr, "\tREG_19.reg_filterop_bernsen_thr = 0x%x\n", p->REG_19.reg_filterop_bernsen_thr);
	fprintf(stderr, "\tREG_19.reg_filterop_u8ContrastThreshold = 0x%x\n", p->REG_19.reg_filterop_u8ContrastThreshold);
	fprintf(stderr, "\tREG_20.reg_filterop_lbp_u8bit_thr = 0x%x\n", p->REG_20.reg_filterop_lbp_u8bit_thr);
	fprintf(stderr, "\tREG_20.reg_filterop_lbp_s8bit_thr = 0x%x\n", p->REG_20.reg_filterop_lbp_s8bit_thr);
	fprintf(stderr, "\tREG_20.reg_filterop_lbp_enmode = 0x%x\n", p->REG_20.reg_filterop_lbp_enmode);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef00 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef00);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef01 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef01);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef02 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef02);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef03 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef03);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef04 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef04);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef10 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef10);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef11 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef11);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef12 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef12);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef13 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef13);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef14 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef14);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef20 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef20);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef21 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef21);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef22 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef22);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef23 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef23);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef24 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef24);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef30 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef30);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef31 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef31);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef32 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef32);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef33 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef33);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef34 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef34);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef40 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef40);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef41 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef41);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef42 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef42);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef43 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef43);
	fprintf(stderr, "\tREG_27.reg_filterop_op2_erodila_coef44 = 0x%x\n", p->REG_27.reg_filterop_op2_erodila_coef44);
	fprintf(stderr, "\tREG_28.reg_filterop_op2_erodila_en = 0x%x\n", p->REG_28.reg_filterop_op2_erodila_en);
	fprintf(stderr, "\tREG_CSC_DBG_COEFF.reg_csc_dbg_en = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_en);
	fprintf(stderr, "\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel);
	fprintf(stderr, "\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff);
	fprintf(stderr, "\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x);
	fprintf(stderr, "\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y);
	fprintf(stderr, "\tREG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = 0x%x\n", p->REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data);
	fprintf(stderr, "\tREG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = 0x%x\n", p->REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data);
	fprintf(stderr, "\tREG_33.reg_filterop_op2_gradfg_en = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_en);
	fprintf(stderr, "\tREG_33.reg_filterop_op2_gradfg_softrst = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_softrst);
	fprintf(stderr, "\tREG_33.reg_filterop_op2_gradfg_enmode = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_enmode);
	fprintf(stderr, "\tREG_33.reg_filterop_op2_gradfg_edwdark = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_edwdark);
	fprintf(stderr, "\tREG_33.reg_filterop_op2_gradfg_edwfactor = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_edwfactor);
	fprintf(stderr, "\tREG_34.reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_crlcoefthr);
	fprintf(stderr, "\tREG_34.reg_filterop_op2_gradfg_magcrlthr = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_magcrlthr);
	fprintf(stderr, "\tREG_34.reg_filterop_op2_gradfg_minmagdiff = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_minmagdiff);
	fprintf(stderr, "\tREG_34.reg_filterop_op2_gradfg_noiseval = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_noiseval);
	fprintf(stderr, "\tREG_110.reg_filterop_map_enmode = 0x%x\n", p->REG_110.reg_filterop_map_enmode);
	fprintf(stderr, "\tREG_110.reg_filterop_norm_out_ctrl = 0x%x\n", p->REG_110.reg_filterop_norm_out_ctrl);
	fprintf(stderr, "\tREG_110.reg_filterop_magang_out_ctrl = 0x%x\n", p->REG_110.reg_filterop_magang_out_ctrl);
	fprintf(stderr, "\tODMA_REG_00.reg_dma_blen = 0x%x\n", p->ODMA_REG_00.reg_dma_blen);
	fprintf(stderr, "\tODMA_REG_00.reg_dma_en = 0x%x\n", p->ODMA_REG_00.reg_dma_en);
	fprintf(stderr, "\tODMA_REG_00.reg_fmt_sel = 0x%x\n", p->ODMA_REG_00.reg_fmt_sel);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_odma_hflip = 0x%x\n", p->ODMA_REG_00.reg_sc_odma_hflip);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_odma_vflip = 0x%x\n", p->ODMA_REG_00.reg_sc_odma_vflip);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_422_avg = 0x%x\n", p->ODMA_REG_00.reg_sc_422_avg);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_420_avg = 0x%x\n", p->ODMA_REG_00.reg_sc_420_avg);
	fprintf(stderr, "\tODMA_REG_00.reg_c_round_mode = 0x%x\n", p->ODMA_REG_00.reg_c_round_mode);
	fprintf(stderr, "\tODMA_REG_00.reg_bf16_en = 0x%x\n", p->ODMA_REG_00.reg_bf16_en);
	fprintf(stderr, "\tODMA_REG_01.reg_dma_y_base_low_part = 0x%x\n", p->ODMA_REG_01.reg_dma_y_base_low_part);
	fprintf(stderr, "\tODMA_REG_02.reg_dma_y_base_high_part = 0x%x\n", p->ODMA_REG_02.reg_dma_y_base_high_part);
	fprintf(stderr, "\tODMA_REG_03.reg_dma_u_base_low_part = 0x%x\n", p->ODMA_REG_03.reg_dma_u_base_low_part);
	fprintf(stderr, "\tODMA_REG_04.reg_dma_u_base_high_part = 0x%x\n", p->ODMA_REG_04.reg_dma_u_base_high_part);
	fprintf(stderr, "\tODMA_REG_05.reg_dma_v_base_low_part = 0x%x\n", p->ODMA_REG_05.reg_dma_v_base_low_part);
	fprintf(stderr, "\tODMA_REG_06.reg_dma_v_base_high_part = 0x%x\n", p->ODMA_REG_06.reg_dma_v_base_high_part);
	fprintf(stderr, "\tODMA_REG_07.reg_dma_y_pitch = 0x%x\n", p->ODMA_REG_07.reg_dma_y_pitch);
	fprintf(stderr, "\tODMA_REG_08.reg_dma_c_pitch = 0x%x\n", p->ODMA_REG_08.reg_dma_c_pitch);
	fprintf(stderr, "\tODMA_REG_09.reg_dma_x_str = 0x%x\n", p->ODMA_REG_09.reg_dma_x_str);
	fprintf(stderr, "\tODMA_REG_10.reg_dma_y_str = 0x%x\n", p->ODMA_REG_10.reg_dma_y_str);
	fprintf(stderr, "\tODMA_REG_11.reg_dma_wd = 0x%x\n", p->ODMA_REG_11.reg_dma_wd);
	fprintf(stderr, "\tODMA_REG_12.reg_dma_ht = 0x%x\n", p->ODMA_REG_12.reg_dma_ht);
	fprintf(stderr, "\tODMA_REG_13.reg_dma_debug = 0x%x\n", p->ODMA_REG_13.reg_dma_debug);
	fprintf(stderr, "\tODMA_REG_14.reg_dma_int_line_target = 0x%x\n", p->ODMA_REG_14.reg_dma_int_line_target);
	fprintf(stderr, "\tODMA_REG_14.reg_dma_int_line_target_sel = 0x%x\n", p->ODMA_REG_14.reg_dma_int_line_target_sel);
	fprintf(stderr, "\tODMA_REG_15.reg_dma_int_cycle_line_target = 0x%x\n", p->ODMA_REG_15.reg_dma_int_cycle_line_target);
	fprintf(stderr, "\tODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x%x\n", p->ODMA_REG_15.reg_dma_int_cycle_line_target_sel);
	fprintf(stderr, "\tODMA_REG_16.reg_dma_latch_line_cnt = 0x%x\n", p->ODMA_REG_16.reg_dma_latch_line_cnt);
	fprintf(stderr, "\tODMA_REG_16.reg_dma_latched_line_cnt = 0x%x\n", p->ODMA_REG_16.reg_dma_latched_line_cnt);
	fprintf(stderr, "\tODMA_REG_17.axi_active = 0x%x\n", p->ODMA_REG_17.axi_active);
	fprintf(stderr, "\tODMA_REG_17.axi_y_active = 0x%x\n", p->ODMA_REG_17.axi_y_active);
	fprintf(stderr, "\tODMA_REG_17.axi_u_active = 0x%x\n", p->ODMA_REG_17.axi_u_active);
	fprintf(stderr, "\tODMA_REG_17.axi_v_active = 0x%x\n", p->ODMA_REG_17.axi_v_active);
	fprintf(stderr, "\tODMA_REG_17.y_buf_full = 0x%x\n", p->ODMA_REG_17.y_buf_full);
	fprintf(stderr, "\tODMA_REG_17.y_buf_empty = 0x%x\n", p->ODMA_REG_17.y_buf_empty);
	fprintf(stderr, "\tODMA_REG_17.u_buf_full = 0x%x\n", p->ODMA_REG_17.u_buf_full);
	fprintf(stderr, "\tODMA_REG_17.u_buf_empty = 0x%x\n", p->ODMA_REG_17.u_buf_empty);
	fprintf(stderr, "\tODMA_REG_17.v_buf_full = 0x%x\n", p->ODMA_REG_17.v_buf_full);
	fprintf(stderr, "\tODMA_REG_17.v_buf_empty = 0x%x\n", p->ODMA_REG_17.v_buf_empty);
	fprintf(stderr, "\tODMA_REG_17.line_target_hit = 0x%x\n", p->ODMA_REG_17.line_target_hit);
	fprintf(stderr, "\tODMA_REG_17.cycle_line_target_hit = 0x%x\n", p->ODMA_REG_17.cycle_line_target_hit);
	fprintf(stderr, "\tODMA_REG_17.axi_cmd_cs = 0x%x\n", p->ODMA_REG_17.axi_cmd_cs);
	fprintf(stderr, "\tODMA_REG_17.y_line_cnt = 0x%x\n", p->ODMA_REG_17.y_line_cnt);
	fprintf(stderr, "\tREG_CANNY_0.reg_canny_lowthr = 0x%x\n", p->REG_CANNY_0.reg_canny_lowthr);
	fprintf(stderr, "\tREG_CANNY_0.reg_canny_hithr = 0x%x\n", p->REG_CANNY_0.reg_canny_hithr);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_en = 0x%x\n", p->REG_CANNY_1.reg_canny_en);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_strong_point_cnt_en = 0x%x\n", p->REG_CANNY_1.reg_canny_strong_point_cnt_en);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_non_or_weak = 0x%x\n", p->REG_CANNY_1.reg_canny_non_or_weak);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_strong_point_cnt = 0x%x\n", p->REG_CANNY_1.reg_canny_strong_point_cnt);
	fprintf(stderr, "\tREG_CANNY_2.reg_canny_eof = 0x%x\n", p->REG_CANNY_2.reg_canny_eof);
	fprintf(stderr, "\tREG_CANNY_3.reg_canny_basel = 0x%x\n", p->REG_CANNY_3.reg_canny_basel);
	fprintf(stderr, "\tREG_CANNY_4.reg_canny_baseh = 0x%x\n", p->REG_CANNY_4.reg_canny_baseh);
	fprintf(stderr, "\tREG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x%x\n", p->REG_ST_CANDI_0.reg_st_candi_corner_bypass);
	fprintf(stderr, "\tREG_ST_CANDI_0.reg_st_candi_corner_switch_src = 0x%x\n", p->REG_ST_CANDI_0.reg_st_candi_corner_switch_src);
	fprintf(stderr, "\tREG_ST_EIGVAL_0.reg_st_eigval_max_eigval = 0x%x\n", p->REG_ST_EIGVAL_0.reg_st_eigval_max_eigval);
	fprintf(stderr, "\tREG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0x%x\n", p->REG_ST_EIGVAL_0.reg_st_eigval_tile_num);
	fprintf(stderr, "\tREG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 0x%x\n", p->REG_ST_EIGVAL_1.reg_sw_clr_max_eigval);
	fprintf(stderr, "\tREG_h190.reg_filterop_op2_csc_tab_sw_0 = 0x%x\n", p->REG_h190.reg_filterop_op2_csc_tab_sw_0);
	fprintf(stderr, "\tREG_h190.reg_filterop_op2_csc_tab_sw_1 = 0x%x\n", p->REG_h190.reg_filterop_op2_csc_tab_sw_1);
	fprintf(stderr, "\tREG_h194.reg_filterop_op2_csc_tab_sw_update = 0x%x\n", p->REG_h194.reg_filterop_op2_csc_tab_sw_update);
	fprintf(stderr, "\tREG_h194.reg_filterop_op2_csc_coeff_sw_update = 0x%x\n", p->REG_h194.reg_filterop_op2_csc_coeff_sw_update);
	fprintf(stderr, "\tREG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n", p->REG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00);
	fprintf(stderr, "\tREG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n", p->REG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01);
	fprintf(stderr, "\tREG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n", p->REG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02);
	fprintf(stderr, "\tREG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n", p->REG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03);
	fprintf(stderr, "\tREG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n", p->REG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04);
	fprintf(stderr, "\tREG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n", p->REG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05);
	fprintf(stderr, "\tREG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n", p->REG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06);
	fprintf(stderr, "\tREG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n", p->REG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07);
	fprintf(stderr, "\tREG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n", p->REG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08);
	fprintf(stderr, "\tREG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n", p->REG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09);
	fprintf(stderr, "\tREG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n", p->REG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10);
	fprintf(stderr, "\tREG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n", p->REG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11);
	fprintf(stderr, "\tREG_h1c8.reg_filterop_op2_csc_enmode = 0x%x\n", p->REG_h1c8.reg_filterop_op2_csc_enmode);
	fprintf(stderr, "\tREG_h1c8.reg_filterop_op2_csc_enable = 0x%x\n", p->REG_h1c8.reg_filterop_op2_csc_enable);
	fprintf(stderr, "\tREG_cropy_s.reg_crop_y_start_x = 0x%x\n", p->REG_cropy_s.reg_crop_y_start_x);
	fprintf(stderr, "\tREG_cropy_s.reg_crop_y_end_x = 0x%x\n", p->REG_cropy_s.reg_crop_y_end_x);
	fprintf(stderr, "\tREG_cropy_e.reg_crop_y_start_y = 0x%x\n", p->REG_cropy_e.reg_crop_y_start_y);
	fprintf(stderr, "\tREG_cropy_e.reg_crop_y_end_y = 0x%x\n", p->REG_cropy_e.reg_crop_y_end_y);
	fprintf(stderr, "\tREG_cropy_ctl.reg_crop_y_enable = 0x%x\n", p->REG_cropy_ctl.reg_crop_y_enable);
	fprintf(stderr, "\tREG_cropc_s.reg_crop_c_start_x = 0x%x\n", p->REG_cropc_s.reg_crop_c_start_x);
	fprintf(stderr, "\tREG_cropc_s.reg_crop_c_end_x = 0x%x\n", p->REG_cropc_s.reg_crop_c_end_x);
	fprintf(stderr, "\tREG_cropc_e.reg_crop_c_start_y = 0x%x\n", p->REG_cropc_e.reg_crop_c_start_y);
	fprintf(stderr, "\tREG_cropc_e.reg_crop_c_end_y = 0x%x\n", p->REG_cropc_e.reg_crop_c_end_y);
	fprintf(stderr, "\tREG_cropc_ctl.reg_crop_c_enable = 0x%x\n", p->REG_cropc_ctl.reg_crop_c_enable);
	fprintf(stderr, "\tREG_crop_odma_s.reg_crop_odma_start_x = 0x%x\n", p->REG_crop_odma_s.reg_crop_odma_start_x);
	fprintf(stderr, "\tREG_crop_odma_s.reg_crop_odma_end_x = 0x%x\n", p->REG_crop_odma_s.reg_crop_odma_end_x);
	fprintf(stderr, "\tREG_crop_odma_e.reg_crop_odma_start_y = 0x%x\n", p->REG_crop_odma_e.reg_crop_odma_start_y);
	fprintf(stderr, "\tREG_crop_odma_e.reg_crop_odma_end_y = 0x%x\n", p->REG_crop_odma_e.reg_crop_odma_end_y);
	fprintf(stderr, "\tREG_crop_odma_ctl.reg_crop_odma_enable = 0x%x\n", p->REG_crop_odma_ctl.reg_crop_odma_enable);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_FILTEROP_C(X) \
	 IVE_FILTEROP_C X = \
{\
	{	/* REG_1.reg_softrst = */0x0,\
	/*.REG_1.reg_softrst_wdma_y = */0x0,\
	/*.REG_1.reg_softrst_wdma_c = */0x0,\
	/*.REG_1.reg_softrst_rdma_gradfg = */0x0,\
	/*.REG_1.reg_softrst_op1 = */0x0,\
	/*.REG_1.reg_softrst_filter3ch = */0x0,\
	/*.REG_1.reg_softrst_st = */0x0,\
	/*.REG_1.reg_softrst_odma = */0x0,\
	},\
	{	/*.REG_H04.reg_gradfg_bggrad_rdma_en = */0x0,\
	/*.REG_H04.reg_gradfg_bggrad_uv_swap = */0x0,\
	},\
	{	/*.REG_2.reg_shdw_sel = */0x1,\
	},\
	{	/*.REG_3.reg_ctrl_dmy1 = */0x0,\
	},\
	{	/*.REG_h10.reg_filterop_mode = */0x2,\
	},\
	{	/*.REG_h14.reg_filterop_op1_cmd = */0x3,\
	/*.REG_h14.reg_filterop_sw_ovw_op = */0x0,\
	/*uint32_t rsv_5_7:3;=*/0,\
	/*.REG_h14.reg_filterop_3ch_en = */0x1,\
	/*.REG_h14.reg_op_y_wdma_en = */0x1,\
	/*.REG_h14.reg_op_c_wdma_en = */0x0,\
	/*.REG_h14.reg_op_y_wdma_w1b_en = */0x0,\
	/*.REG_h14.reg_op_c_wdma_w1b_en = */0x0,\
	},\
	{	/*.REG_4.reg_filterop_h_coef00 = */0x0,\
	/*.REG_4.reg_filterop_h_coef01 = */0x0,\
	/*.REG_4.reg_filterop_h_coef02 = */0x0,\
	/*.REG_4.reg_filterop_h_coef03 = */0x0,\
	},\
	{	/*.REG_5.reg_filterop_h_coef04 = */0x0,\
	/*.REG_5.reg_filterop_h_coef10 = */0x0,\
	/*.REG_5.reg_filterop_h_coef11 = */0x0,\
	/*.REG_5.reg_filterop_h_coef12 = */0x0,\
	},\
	{	/*.REG_6.reg_filterop_h_coef13 = */0x0,\
	/*.REG_6.reg_filterop_h_coef14 = */0x0,\
	/*.REG_6.reg_filterop_h_coef20 = */0x0,\
	/*.REG_6.reg_filterop_h_coef21 = */0x0,\
	},\
	{	/*.REG_7.reg_filterop_h_coef22 = */0x0,\
	/*.REG_7.reg_filterop_h_coef23 = */0x0,\
	/*.REG_7.reg_filterop_h_coef24 = */0x0,\
	/*.REG_7.reg_filterop_h_coef30 = */0x0,\
	},\
	{	/*.REG_8.reg_filterop_h_coef31 = */0x0,\
	/*.REG_8.reg_filterop_h_coef32 = */0x0,\
	/*.REG_8.reg_filterop_h_coef33 = */0x0,\
	/*.REG_8.reg_filterop_h_coef34 = */0x0,\
	},\
	{	/*.REG_9.reg_filterop_h_coef40 = */0x0,\
	/*.REG_9.reg_filterop_h_coef41 = */0x0,\
	/*.REG_9.reg_filterop_h_coef42 = */0x0,\
	/*.REG_9.reg_filterop_h_coef43 = */0x0,\
	},\
	{	/*.REG_10.reg_filterop_h_coef44 = */0x0,\
	/*.REG_10.reg_filterop_h_norm = */0x0,\
	},\
	{	/*.REG_11.reg_filterop_v_coef00 = */0x0,\
	/*.REG_11.reg_filterop_v_coef01 = */0x0,\
	/*.REG_11.reg_filterop_v_coef02 = */0x0,\
	/*.REG_11.reg_filterop_v_coef03 = */0x0,\
	},\
	{	/*.REG_12.reg_filterop_v_coef04 = */0x0,\
	/*.REG_12.reg_filterop_v_coef10 = */0x0,\
	/*.REG_12.reg_filterop_v_coef11 = */0x0,\
	/*.REG_12.reg_filterop_v_coef12 = */0x0,\
	},\
	{	/*.REG_13.reg_filterop_v_coef13 = */0x0,\
	/*.REG_13.reg_filterop_v_coef14 = */0x0,\
	/*.REG_13.reg_filterop_v_coef20 = */0x0,\
	/*.REG_13.reg_filterop_v_coef21 = */0x0,\
	},\
	{	/*.REG_14.reg_filterop_v_coef22 = */0x0,\
	/*.REG_14.reg_filterop_v_coef23 = */0x0,\
	/*.REG_14.reg_filterop_v_coef24 = */0x0,\
	/*.REG_14.reg_filterop_v_coef30 = */0x0,\
	},\
	{	/*.REG_15.reg_filterop_v_coef31 = */0x0,\
	/*.REG_15.reg_filterop_v_coef32 = */0x0,\
	/*.REG_15.reg_filterop_v_coef33 = */0x0,\
	/*.REG_15.reg_filterop_v_coef34 = */0x0,\
	},\
	{	/*.REG_16.reg_filterop_v_coef40 = */0x0,\
	/*.REG_16.reg_filterop_v_coef41 = */0x0,\
	/*.REG_16.reg_filterop_v_coef42 = */0x0,\
	/*.REG_16.reg_filterop_v_coef43 = */0x0,\
	},\
	{	/*.REG_17.reg_filterop_v_coef44 = */0x0,\
	/*.REG_17.reg_filterop_v_norm = */0x0,\
	},\
	{	/*.REG_18.reg_filterop_mode_trans = */0x1,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.REG_18.reg_filterop_order_enmode = */0x1,\
	/*uint32_t rsv_7_15:9;=*/0,\
	/*.REG_18.reg_filterop_mag_thr = */0x0,\
	},\
	{	/*.REG_19.reg_filterop_bernsen_win5x5_en = */0x1,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.REG_19.reg_filterop_bernsen_mode = */0x0,\
	/*uint32_t rsv_6_7:2;=*/0,\
	/*.REG_19.reg_filterop_bernsen_thr = */0x0,\
	/*.REG_19.reg_filterop_u8ContrastThreshold = */0x0,\
	},\
	{	/*.REG_20.reg_filterop_lbp_u8bit_thr = */0x0,\
	/*.REG_20.reg_filterop_lbp_s8bit_thr = */0x0,\
	/*.REG_20.reg_filterop_lbp_enmode = */0x0,\
	},\
	{	/*.REG_21.reg_filterop_op2_erodila_coef00 = */0x0,\
	/*.REG_21.reg_filterop_op2_erodila_coef01 = */0x0,\
	/*.REG_21.reg_filterop_op2_erodila_coef02 = */0x0,\
	/*.REG_21.reg_filterop_op2_erodila_coef03 = */0x0,\
	},\
	{	/*.REG_22.reg_filterop_op2_erodila_coef04 = */0x0,\
	/*.REG_22.reg_filterop_op2_erodila_coef10 = */0x0,\
	/*.REG_22.reg_filterop_op2_erodila_coef11 = */0x0,\
	/*.REG_22.reg_filterop_op2_erodila_coef12 = */0x0,\
	},\
	{	/*.REG_23.reg_filterop_op2_erodila_coef13 = */0x0,\
	/*.REG_23.reg_filterop_op2_erodila_coef14 = */0x0,\
	/*.REG_23.reg_filterop_op2_erodila_coef20 = */0x0,\
	/*.REG_23.reg_filterop_op2_erodila_coef21 = */0x0,\
	},\
	{	/*.REG_24.reg_filterop_op2_erodila_coef22 = */0x0,\
	/*.REG_24.reg_filterop_op2_erodila_coef23 = */0x0,\
	/*.REG_24.reg_filterop_op2_erodila_coef24 = */0x0,\
	/*.REG_24.reg_filterop_op2_erodila_coef30 = */0x0,\
	},\
	{	/*.REG_25.reg_filterop_op2_erodila_coef31 = */0x0,\
	/*.REG_25.reg_filterop_op2_erodila_coef32 = */0x0,\
	/*.REG_25.reg_filterop_op2_erodila_coef33 = */0x0,\
	/*.REG_25.reg_filterop_op2_erodila_coef34 = */0x0,\
	},\
	{	/*.REG_26.reg_filterop_op2_erodila_coef40 = */0x0,\
	/*.REG_26.reg_filterop_op2_erodila_coef41 = */0x0,\
	/*.REG_26.reg_filterop_op2_erodila_coef42 = */0x0,\
	/*.REG_26.reg_filterop_op2_erodila_coef43 = */0x0,\
	},\
	{	/*.REG_27.reg_filterop_op2_erodila_coef44 = */0x0,\
	},\
	{	/*.REG_28.reg_filterop_op2_erodila_en = */0x0,\
	},\
	{	/*.REG_CSC_DBG_COEFF.reg_csc_dbg_en = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = */0x0,\
	/*.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff = */0x0,\
	},\
	{	/*.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = */0x0,\
	},\
	{	/*.REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = */0x0,\
	},\
	{	/*.REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = */0x0,\
	},\
	{	/*.REG_33.reg_filterop_op2_gradfg_en = */0x0,\
	/*.REG_33.reg_filterop_op2_gradfg_softrst = */0x0,\
	/*.REG_33.reg_filterop_op2_gradfg_enmode = */0x1,\
	/*.REG_33.reg_filterop_op2_gradfg_edwdark = */0x1,\
	/*uint32_t rsv_4_15:12;=*/0,\
	/*.REG_33.reg_filterop_op2_gradfg_edwfactor = */0x3e8,\
	},\
	{	/*.REG_34.reg_filterop_op2_gradfg_crlcoefthr = */0x50,\
	/*.REG_34.reg_filterop_op2_gradfg_magcrlthr = */0x4,\
	/*.REG_34.reg_filterop_op2_gradfg_minmagdiff = */0x2,\
	/*.REG_34.reg_filterop_op2_gradfg_noiseval = */0x1,\
	},\
	{	/*.REG_110.reg_filterop_map_enmode = */0x0,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.REG_110.reg_filterop_norm_out_ctrl = */0x0,\
	/*uint32_t rsv_6_7:2;=*/0,\
	/*.REG_110.reg_filterop_magang_out_ctrl = */0x0,\
	},\
	{	/*.ODMA_REG_00.reg_dma_blen = */0x0,\
	/*.ODMA_REG_00.reg_dma_en = */0x0,\
	/*uint32_t rsv_2_7:6;=*/0,\
	/*.ODMA_REG_00.reg_fmt_sel = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.ODMA_REG_00.reg_sc_odma_hflip = */0x0,\
	/*.ODMA_REG_00.reg_sc_odma_vflip = */0x0,\
	/*uint32_t rsv_18_19:2;=*/0,\
	/*.ODMA_REG_00.reg_sc_422_avg = */0x0,\
	/*.ODMA_REG_00.reg_sc_420_avg = */0x0,\
	/*.ODMA_REG_00.reg_c_round_mode = */0x0,\
	/*.ODMA_REG_00.reg_bf16_en = */0x0,\
	},\
	{	/*.ODMA_REG_01.reg_dma_y_base_low_part = */0x0,\
	},\
	{	/*.ODMA_REG_02.reg_dma_y_base_high_part = */0x0,\
	},\
	{	/*.ODMA_REG_03.reg_dma_u_base_low_part = */0x0,\
	},\
	{	/*.ODMA_REG_04.reg_dma_u_base_high_part = */0x0,\
	},\
	{	/*.ODMA_REG_05.reg_dma_v_base_low_part = */0x0,\
	},\
	{	/*.ODMA_REG_06.reg_dma_v_base_high_part = */0x0,\
	},\
	{	/*.ODMA_REG_07.reg_dma_y_pitch = */0x0,\
	},\
	{	/*.ODMA_REG_08.reg_dma_c_pitch = */0x0,\
	},\
	{	/*.ODMA_REG_09.reg_dma_x_str = */0x0,\
	},\
	{	/*.ODMA_REG_10.reg_dma_y_str = */0x0,\
	},\
	{	/*.ODMA_REG_11.reg_dma_wd = */0x0,\
	},\
	{	/*.ODMA_REG_12.reg_dma_ht = */0x0,\
	},\
	{	/*.ODMA_REG_13.reg_dma_debug = */0x0,\
	},\
	{	/*.ODMA_REG_14.reg_dma_int_line_target = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.ODMA_REG_14.reg_dma_int_line_target_sel = */0x0,\
	},\
	{	/*.ODMA_REG_15.reg_dma_int_cycle_line_target = */0x0,\
	/*uint32_t rsv_11_15:5;=*/0,\
	/*.ODMA_REG_15.reg_dma_int_cycle_line_target_sel = */0x0,\
	},\
	{	/*.ODMA_REG_16.reg_dma_latch_line_cnt = */0x0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.ODMA_REG_16.reg_dma_latched_line_cnt = */0x0,\
	},\
	{	/*.ODMA_REG_17.axi_active = */0x0,\
	/*.ODMA_REG_17.axi_y_active = */0x0,\
	/*.ODMA_REG_17.axi_u_active = */0x0,\
	/*.ODMA_REG_17.axi_v_active = */0x0,\
	/*.ODMA_REG_17.y_buf_full = */0x0,\
	/*.ODMA_REG_17.y_buf_empty = */0x0,\
	/*.ODMA_REG_17.u_buf_full = */0x0,\
	/*.ODMA_REG_17.u_buf_empty = */0x0,\
	/*.ODMA_REG_17.v_buf_full = */0x0,\
	/*.ODMA_REG_17.v_buf_empty = */0x0,\
	/*.ODMA_REG_17.line_target_hit = */0x0,\
	/*.ODMA_REG_17.cycle_line_target_hit = */0x0,\
	/*.ODMA_REG_17.axi_cmd_cs = */0x0,\
	/*.ODMA_REG_17.y_line_cnt = */0x0,\
	},\
	{	/*.REG_CANNY_0.reg_canny_lowthr = */0x0,\
	/*.REG_CANNY_0.reg_canny_hithr = */0x0,\
	},\
	{	/*.REG_CANNY_1.reg_canny_en = */0x0,\
	/*.REG_CANNY_1.reg_canny_strong_point_cnt_en = */0x0,\
	/*.REG_CANNY_1.reg_canny_non_or_weak = */0x0,\
	/*uint32_t rsv_3_15:13;=*/0,\
	/*.REG_CANNY_1.reg_canny_strong_point_cnt = */0x0,\
	},\
	{	/*.REG_CANNY_2.reg_canny_eof = */0xFFFFFFFF,\
	},\
	{	/*.REG_CANNY_3.reg_canny_basel = */0x0,\
	},\
	{	/*.REG_CANNY_4.reg_canny_baseh = */0x0,\
	},\
	{	/*.REG_ST_CANDI_0.reg_st_candi_corner_bypass = */0x0,\
	/*.REG_ST_CANDI_0.reg_st_candi_corner_switch_src = */0x0,\
	},\
	{	/*.REG_ST_EIGVAL_0.reg_st_eigval_max_eigval = */0x0,\
	/*.REG_ST_EIGVAL_0.reg_st_eigval_tile_num = */0x0,\
	},\
	{	/*.REG_ST_EIGVAL_1.reg_sw_clr_max_eigval = */0x0,\
	},\
	{	/*.REG_h190.reg_filterop_op2_csc_tab_sw_0 = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.REG_h190.reg_filterop_op2_csc_tab_sw_1 = */0x0,\
	},\
	{	/*.REG_h194.reg_filterop_op2_csc_tab_sw_update = */0x0,\
	/*uint32_t rsv_1_15:15;=*/0,\
	/*.REG_h194.reg_filterop_op2_csc_coeff_sw_update = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = */0x0,\
	},\
	{	/*.REG_h1c8.reg_filterop_op2_csc_enmode = */0x0,\
	/*.REG_h1c8.reg_filterop_op2_csc_enable = */0x0,\
	},\
	{	/*.REG_cropy_s.reg_crop_y_start_x = */0x0,\
	/*.REG_cropy_s.reg_crop_y_end_x = */0x0,\
	},\
	{	/*.REG_cropy_e.reg_crop_y_start_y = */0x0,\
	/*.REG_cropy_e.reg_crop_y_end_y = */0x0,\
	},\
	{	/*.REG_cropy_ctl.reg_crop_y_enable = */0x0,\
	},\
	{	/*.REG_cropc_s.reg_crop_c_start_x = */0x0,\
	/*.REG_cropc_s.reg_crop_c_end_x = */0x0,\
	},\
	{	/*.REG_cropc_e.reg_crop_c_start_y = */0x0,\
	/*.REG_cropc_e.reg_crop_c_end_y = */0x0,\
	},\
	{	/*.REG_cropc_ctl.reg_crop_c_enable = */0x0,\
	},\
	{	/*.REG_crop_odma_s.reg_crop_odma_start_x = */0x0,\
	/*.REG_crop_odma_s.reg_crop_odma_end_x = */0x0,\
	},\
	{	/*.REG_crop_odma_e.reg_crop_odma_start_y = */0x0,\
	/*.REG_crop_odma_e.reg_crop_odma_end_y = */0x0,\
	},\
	{	/*.REG_crop_odma_ctl.reg_crop_odma_enable = */0x0,\
	}\
}; 
#else /* !ifdef __cplusplus */ 
#define DEFINE_IVE_FILTEROP_C(X) \
	 IVE_FILTEROP_C X = \
{\
	.REG_1.reg_softrst = 0x0,\
	.REG_1.reg_softrst_wdma_y = 0x0,\
	.REG_1.reg_softrst_wdma_c = 0x0,\
	.REG_1.reg_softrst_rdma_gradfg = 0x0,\
	.REG_1.reg_softrst_op1 = 0x0,\
	.REG_1.reg_softrst_filter3ch = 0x0,\
	.REG_1.reg_softrst_st = 0x0,\
	.REG_1.reg_softrst_odma = 0x0,\
	.REG_H04.reg_gradfg_bggrad_rdma_en = 0x0,\
	.REG_H04.reg_gradfg_bggrad_uv_swap = 0x0,\
	.REG_2.reg_shdw_sel = 0x1,\
	.REG_3.reg_ctrl_dmy1 = 0x0,\
	.REG_h10.reg_filterop_mode = 0x2,\
	.REG_h14.reg_filterop_op1_cmd = 0x3,\
	.REG_h14.reg_filterop_sw_ovw_op = 0x0,\
	.REG_h14.reg_filterop_3ch_en = 0x1,\
	.REG_h14.reg_op_y_wdma_en = 0x1,\
	.REG_h14.reg_op_c_wdma_en = 0x0,\
	.REG_h14.reg_op_y_wdma_w1b_en = 0x0,\
	.REG_h14.reg_op_c_wdma_w1b_en = 0x0,\
	.REG_4.reg_filterop_h_coef00 = 0x0,\
	.REG_4.reg_filterop_h_coef01 = 0x0,\
	.REG_4.reg_filterop_h_coef02 = 0x0,\
	.REG_4.reg_filterop_h_coef03 = 0x0,\
	.REG_5.reg_filterop_h_coef04 = 0x0,\
	.REG_5.reg_filterop_h_coef10 = 0x0,\
	.REG_5.reg_filterop_h_coef11 = 0x0,\
	.REG_5.reg_filterop_h_coef12 = 0x0,\
	.REG_6.reg_filterop_h_coef13 = 0x0,\
	.REG_6.reg_filterop_h_coef14 = 0x0,\
	.REG_6.reg_filterop_h_coef20 = 0x0,\
	.REG_6.reg_filterop_h_coef21 = 0x0,\
	.REG_7.reg_filterop_h_coef22 = 0x0,\
	.REG_7.reg_filterop_h_coef23 = 0x0,\
	.REG_7.reg_filterop_h_coef24 = 0x0,\
	.REG_7.reg_filterop_h_coef30 = 0x0,\
	.REG_8.reg_filterop_h_coef31 = 0x0,\
	.REG_8.reg_filterop_h_coef32 = 0x0,\
	.REG_8.reg_filterop_h_coef33 = 0x0,\
	.REG_8.reg_filterop_h_coef34 = 0x0,\
	.REG_9.reg_filterop_h_coef40 = 0x0,\
	.REG_9.reg_filterop_h_coef41 = 0x0,\
	.REG_9.reg_filterop_h_coef42 = 0x0,\
	.REG_9.reg_filterop_h_coef43 = 0x0,\
	.REG_10.reg_filterop_h_coef44 = 0x0,\
	.REG_10.reg_filterop_h_norm = 0x0,\
	.REG_11.reg_filterop_v_coef00 = 0x0,\
	.REG_11.reg_filterop_v_coef01 = 0x0,\
	.REG_11.reg_filterop_v_coef02 = 0x0,\
	.REG_11.reg_filterop_v_coef03 = 0x0,\
	.REG_12.reg_filterop_v_coef04 = 0x0,\
	.REG_12.reg_filterop_v_coef10 = 0x0,\
	.REG_12.reg_filterop_v_coef11 = 0x0,\
	.REG_12.reg_filterop_v_coef12 = 0x0,\
	.REG_13.reg_filterop_v_coef13 = 0x0,\
	.REG_13.reg_filterop_v_coef14 = 0x0,\
	.REG_13.reg_filterop_v_coef20 = 0x0,\
	.REG_13.reg_filterop_v_coef21 = 0x0,\
	.REG_14.reg_filterop_v_coef22 = 0x0,\
	.REG_14.reg_filterop_v_coef23 = 0x0,\
	.REG_14.reg_filterop_v_coef24 = 0x0,\
	.REG_14.reg_filterop_v_coef30 = 0x0,\
	.REG_15.reg_filterop_v_coef31 = 0x0,\
	.REG_15.reg_filterop_v_coef32 = 0x0,\
	.REG_15.reg_filterop_v_coef33 = 0x0,\
	.REG_15.reg_filterop_v_coef34 = 0x0,\
	.REG_16.reg_filterop_v_coef40 = 0x0,\
	.REG_16.reg_filterop_v_coef41 = 0x0,\
	.REG_16.reg_filterop_v_coef42 = 0x0,\
	.REG_16.reg_filterop_v_coef43 = 0x0,\
	.REG_17.reg_filterop_v_coef44 = 0x0,\
	.REG_17.reg_filterop_v_norm = 0x0,\
	.REG_18.reg_filterop_mode_trans = 0x1,\
	.REG_18.reg_filterop_order_enmode = 0x1,\
	.REG_18.reg_filterop_mag_thr = 0x0,\
	.REG_19.reg_filterop_bernsen_win5x5_en = 0x1,\
	.REG_19.reg_filterop_bernsen_mode = 0x0,\
	.REG_19.reg_filterop_bernsen_thr = 0x0,\
	.REG_19.reg_filterop_u8ContrastThreshold = 0x0,\
	.REG_20.reg_filterop_lbp_u8bit_thr = 0x0,\
	.REG_20.reg_filterop_lbp_s8bit_thr = 0x0,\
	.REG_20.reg_filterop_lbp_enmode = 0x0,\
	.REG_21.reg_filterop_op2_erodila_coef00 = 0x0,\
	.REG_21.reg_filterop_op2_erodila_coef01 = 0x0,\
	.REG_21.reg_filterop_op2_erodila_coef02 = 0x0,\
	.REG_21.reg_filterop_op2_erodila_coef03 = 0x0,\
	.REG_22.reg_filterop_op2_erodila_coef04 = 0x0,\
	.REG_22.reg_filterop_op2_erodila_coef10 = 0x0,\
	.REG_22.reg_filterop_op2_erodila_coef11 = 0x0,\
	.REG_22.reg_filterop_op2_erodila_coef12 = 0x0,\
	.REG_23.reg_filterop_op2_erodila_coef13 = 0x0,\
	.REG_23.reg_filterop_op2_erodila_coef14 = 0x0,\
	.REG_23.reg_filterop_op2_erodila_coef20 = 0x0,\
	.REG_23.reg_filterop_op2_erodila_coef21 = 0x0,\
	.REG_24.reg_filterop_op2_erodila_coef22 = 0x0,\
	.REG_24.reg_filterop_op2_erodila_coef23 = 0x0,\
	.REG_24.reg_filterop_op2_erodila_coef24 = 0x0,\
	.REG_24.reg_filterop_op2_erodila_coef30 = 0x0,\
	.REG_25.reg_filterop_op2_erodila_coef31 = 0x0,\
	.REG_25.reg_filterop_op2_erodila_coef32 = 0x0,\
	.REG_25.reg_filterop_op2_erodila_coef33 = 0x0,\
	.REG_25.reg_filterop_op2_erodila_coef34 = 0x0,\
	.REG_26.reg_filterop_op2_erodila_coef40 = 0x0,\
	.REG_26.reg_filterop_op2_erodila_coef41 = 0x0,\
	.REG_26.reg_filterop_op2_erodila_coef42 = 0x0,\
	.REG_26.reg_filterop_op2_erodila_coef43 = 0x0,\
	.REG_27.reg_filterop_op2_erodila_coef44 = 0x0,\
	.REG_28.reg_filterop_op2_erodila_en = 0x0,\
	.REG_CSC_DBG_COEFF.reg_csc_dbg_en = 0x0,\
	.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = 0x0,\
	.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff = 0x0,\
	.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = 0x0,\
	.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = 0x0,\
	.REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = 0x0,\
	.REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = 0x0,\
	.REG_33.reg_filterop_op2_gradfg_en = 0x0,\
	.REG_33.reg_filterop_op2_gradfg_softrst = 0x0,\
	.REG_33.reg_filterop_op2_gradfg_enmode = 0x1,\
	.REG_33.reg_filterop_op2_gradfg_edwdark = 0x1,\
	.REG_33.reg_filterop_op2_gradfg_edwfactor = 0x3e8,\
	.REG_34.reg_filterop_op2_gradfg_crlcoefthr = 0x50,\
	.REG_34.reg_filterop_op2_gradfg_magcrlthr = 0x4,\
	.REG_34.reg_filterop_op2_gradfg_minmagdiff = 0x2,\
	.REG_34.reg_filterop_op2_gradfg_noiseval = 0x1,\
	.REG_110.reg_filterop_map_enmode = 0x0,\
	.REG_110.reg_filterop_norm_out_ctrl = 0x0,\
	.REG_110.reg_filterop_magang_out_ctrl = 0x0,\
	.ODMA_REG_00.reg_dma_blen = 0x0,\
	.ODMA_REG_00.reg_dma_en = 0x0,\
	.ODMA_REG_00.reg_fmt_sel = 0x0,\
	.ODMA_REG_00.reg_sc_odma_hflip = 0x0,\
	.ODMA_REG_00.reg_sc_odma_vflip = 0x0,\
	.ODMA_REG_00.reg_sc_422_avg = 0x0,\
	.ODMA_REG_00.reg_sc_420_avg = 0x0,\
	.ODMA_REG_00.reg_c_round_mode = 0x0,\
	.ODMA_REG_00.reg_bf16_en = 0x0,\
	.ODMA_REG_01.reg_dma_y_base_low_part = 0x0,\
	.ODMA_REG_02.reg_dma_y_base_high_part = 0x0,\
	.ODMA_REG_03.reg_dma_u_base_low_part = 0x0,\
	.ODMA_REG_04.reg_dma_u_base_high_part = 0x0,\
	.ODMA_REG_05.reg_dma_v_base_low_part = 0x0,\
	.ODMA_REG_06.reg_dma_v_base_high_part = 0x0,\
	.ODMA_REG_07.reg_dma_y_pitch = 0x0,\
	.ODMA_REG_08.reg_dma_c_pitch = 0x0,\
	.ODMA_REG_09.reg_dma_x_str = 0x0,\
	.ODMA_REG_10.reg_dma_y_str = 0x0,\
	.ODMA_REG_11.reg_dma_wd = 0x0,\
	.ODMA_REG_12.reg_dma_ht = 0x0,\
	.ODMA_REG_13.reg_dma_debug = 0x0,\
	.ODMA_REG_14.reg_dma_int_line_target = 0x0,\
	.ODMA_REG_14.reg_dma_int_line_target_sel = 0x0,\
	.ODMA_REG_15.reg_dma_int_cycle_line_target = 0x0,\
	.ODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x0,\
	.ODMA_REG_16.reg_dma_latch_line_cnt = 0x0,\
	.ODMA_REG_16.reg_dma_latched_line_cnt = 0x0,\
	.ODMA_REG_17.axi_active = 0x0,\
	.ODMA_REG_17.axi_y_active = 0x0,\
	.ODMA_REG_17.axi_u_active = 0x0,\
	.ODMA_REG_17.axi_v_active = 0x0,\
	.ODMA_REG_17.y_buf_full = 0x0,\
	.ODMA_REG_17.y_buf_empty = 0x0,\
	.ODMA_REG_17.u_buf_full = 0x0,\
	.ODMA_REG_17.u_buf_empty = 0x0,\
	.ODMA_REG_17.v_buf_full = 0x0,\
	.ODMA_REG_17.v_buf_empty = 0x0,\
	.ODMA_REG_17.line_target_hit = 0x0,\
	.ODMA_REG_17.cycle_line_target_hit = 0x0,\
	.ODMA_REG_17.axi_cmd_cs = 0x0,\
	.ODMA_REG_17.y_line_cnt = 0x0,\
	.REG_CANNY_0.reg_canny_lowthr = 0x0,\
	.REG_CANNY_0.reg_canny_hithr = 0x0,\
	.REG_CANNY_1.reg_canny_en = 0x0,\
	.REG_CANNY_1.reg_canny_strong_point_cnt_en = 0x0,\
	.REG_CANNY_1.reg_canny_non_or_weak = 0x0,\
	.REG_CANNY_1.reg_canny_strong_point_cnt = 0x0,\
	.REG_CANNY_2.reg_canny_eof = 0xFFFFFFFF,\
	.REG_CANNY_3.reg_canny_basel = 0x0,\
	.REG_CANNY_4.reg_canny_baseh = 0x0,\
	.REG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x0,\
	.REG_ST_CANDI_0.reg_st_candi_corner_switch_src = 0x0,\
	.REG_ST_EIGVAL_0.reg_st_eigval_max_eigval = 0x0,\
	.REG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0x0,\
	.REG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 0x0,\
	.REG_h190.reg_filterop_op2_csc_tab_sw_0 = 0x0,\
	.REG_h190.reg_filterop_op2_csc_tab_sw_1 = 0x0,\
	.REG_h194.reg_filterop_op2_csc_tab_sw_update = 0x0,\
	.REG_h194.reg_filterop_op2_csc_coeff_sw_update = 0x0,\
	.REG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = 0x0,\
	.REG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = 0x0,\
	.REG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = 0x0,\
	.REG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = 0x0,\
	.REG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = 0x0,\
	.REG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = 0x0,\
	.REG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = 0x0,\
	.REG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = 0x0,\
	.REG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = 0x0,\
	.REG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = 0x0,\
	.REG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = 0x0,\
	.REG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = 0x0,\
	.REG_h1c8.reg_filterop_op2_csc_enmode = 0x0,\
	.REG_h1c8.reg_filterop_op2_csc_enable = 0x0,\
	.REG_cropy_s.reg_crop_y_start_x = 0x0,\
	.REG_cropy_s.reg_crop_y_end_x = 0x0,\
	.REG_cropy_e.reg_crop_y_start_y = 0x0,\
	.REG_cropy_e.reg_crop_y_end_y = 0x0,\
	.REG_cropy_ctl.reg_crop_y_enable = 0x0,\
	.REG_cropc_s.reg_crop_c_start_x = 0x0,\
	.REG_cropc_s.reg_crop_c_end_x = 0x0,\
	.REG_cropc_e.reg_crop_c_start_y = 0x0,\
	.REG_cropc_e.reg_crop_c_end_y = 0x0,\
	.REG_cropc_ctl.reg_crop_c_enable = 0x0,\
	.REG_crop_odma_s.reg_crop_odma_start_x = 0x0,\
	.REG_crop_odma_s.reg_crop_odma_end_x = 0x0,\
	.REG_crop_odma_e.reg_crop_odma_start_y = 0x0,\
	.REG_crop_odma_e.reg_crop_odma_end_y = 0x0,\
	.REG_crop_odma_ctl.reg_crop_odma_enable = 0x0,\
};
#endif /* ifdef __cplusplus */ 
#endif //__REG_IVE_FILTEROP_STRUCT_H__

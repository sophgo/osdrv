// $Module: ive_top $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 05 Jan 2022 03:04:40 PM $
//

#ifndef __REG_IVE_TOP_STRUCT_H__
#define __REG_IVE_TOP_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*img_in u,v swap in top;*/
		uint32_t reg_img_in_uv_swap:1;
		/*rdma img1 u,v swap in top;*/
		uint32_t reg_img_1_uv_swap:1;
		/*rdma eigval u,v swap in top;*/
		uint32_t reg_rdma_eigval_uv_swap:1;
		uint32_t rsv_3_3:1;
		/*cnt from shdw and trigger 1t;*/
		uint32_t reg_trig_cnt:4;
	};
	uint32_t val;
} IVE_TOP_REG_0_C;
typedef union {
	struct {
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
		uint32_t rsv_2_3:2;
		/*sw set 1  for flow ctrl trigger ;*/
		uint32_t reg_fmt_vld_fg:1;
		/*sw set 1 for flow ctrl trigger;*/
		uint32_t reg_fmt_vld_ccl:1;
		/*sw set 1  for flow ctrl trigger ;*/
		uint32_t reg_fmt_vld_dmaf:1;
		/*sw set 1 for flow ctrl trigger;*/
		uint32_t reg_fmt_vld_lk:1;
		/*bit 0 : fg path y
		bit 1 : ccl
		bit 2 : dmaf
		bit 3 : fg path y & c
		bit 4 : fg path c ;*/
		uint32_t reg_cmdq_tsk_trig:5;
		uint32_t rsv_13_15:3;
		/*sel 0: trigger from shadow done  ; sel 1 : trigger from frame done;*/
		uint32_t reg_cmdq_tsk_sel:1;
		/*HW trigger cmdque enable;*/
		uint32_t reg_cmdq_tsk_en:1;
		/*error handling - clr wdma/rdma;*/
		uint32_t reg_dma_abort:1;
		/*wdma abort done (level);*/
		uint32_t reg_wdma_abort_done:1;
		/*rdma abort done (level);*/
		uint32_t reg_rdma_abort_done:1;
		/*img_in axi idle ;*/
		uint32_t reg_img_in_axi_idle:1;
		/*odma axi  idle ;*/
		uint32_t reg_odma_axi_idle:1;
	};
	uint32_t val;
} IVE_TOP_REG_1_C;
typedef union {
	struct {
		/*ive top width -1   after resize  *img_in seting in img_in reg bank;*/
		uint32_t reg_img_widthm1:13;
		uint32_t rsv_13_15:3;
		/*ive top height -1  after resize  *img_in seting in img_in reg bank;*/
		uint32_t reg_img_heightm1:13;
	};
	uint32_t val;
} IVE_TOP_REG_2_C;
typedef union {
	struct {
		/*img0 , img1 mux
		1.  reg_imgmux_img0_sel = 1 : sel img mux from img0 (3 channel path )
		2 . reg_imgmux_img0_sel = 0 : as use 2framop ;*/
		uint32_t reg_imgmux_img0_sel:1;
		/*ive_map , rdma_eigval mux :
		0 : src from ive_map
		1 : src from rdma_eigval;*/
		uint32_t reg_mapmux_rdma_sel:1;
		/*img1 rdma enable;*/
		uint32_t reg_ive_rdma_img1_en:1;
		/*img1 rdma mode:
		set 1 : read 8bit mode : normal usage for hist /intg/ 2frame sub …
		set 0 : read 16 bit mode (gradfg use);*/
		uint32_t reg_ive_rdma_img1_mod_u8:1;
		/*eigval rdma enable : set 1 => rdma trigger;*/
		uint32_t reg_ive_rdma_eigval_en:1;
		/*set 1: rdma_img1 will be switch for gradfg Curgrad use
		set 0 : normal usage for hist /intg/ 2frame sub …;*/
		uint32_t reg_muxsel_gradfg:1;
		/*wdma share
		set1: wdma switch to gmm
		set0: wdma switch to bg match + bg update   (default);*/
		uint32_t reg_dma_share_mux_selgmm:1;
	};
	uint32_t val;
} IVE_TOP_REG_3_C;
typedef union {
	struct {
		/*top enable;*/
		uint32_t reg_img_in_top_enable:1;
		/*;*/
		uint32_t reg_resize_top_enable:1;
		/*;*/
		uint32_t reg_gmm_top_enable:1;
		/*;*/
		uint32_t reg_csc_top_enable:1;
		/*;*/
		uint32_t reg_rdma_img1_top_enable:1;
		/*;*/
		uint32_t reg_bgm_top_enable:1;
		/*;*/
		uint32_t reg_bgu_top_enable:1;
		/*;*/
		uint32_t reg_r2y4_top_enable:1;
		/*;*/
		uint32_t reg_map_top_enable:1;
		/*;*/
		uint32_t reg_rdma_eigval_top_enable:1;
		/*;*/
		uint32_t reg_thresh_top_enable:1;
		/*;*/
		uint32_t reg_hist_top_enable:1;
		/*;*/
		uint32_t reg_intg_top_enable:1;
		/*;*/
		uint32_t reg_ncc_top_enable:1;
		/*;*/
		uint32_t reg_sad_top_enable:1;
		/*;*/
		uint32_t reg_filterop_top_enable:1;
		/*;*/
		uint32_t reg_dmaf_top_enable:1;
		/*;*/
		uint32_t reg_ccl_top_enable:1;
		/*;*/
		uint32_t reg_lk_top_enable:1;
	};
	uint32_t val;
} IVE_TOP_REG_h10_C;
typedef union {
	struct {
		/*unsigned 12 bit, update table value of inv_v_tab(rgb2hsv) or gamma_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_csc_tab_sw_0:12;
		uint32_t rsv_12_15:4;
		/*unsigned 15 bit, update table value of inv_h_tab(rgb2hsv) or xyz_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_csc_tab_sw_1:15;
	};
	uint32_t val;
} IVE_TOP_REG_11_C;
typedef union {
	struct {
		/*update rgb2hsv/rgb2lab table value by software
		0:use const, 1:update table by reg_csc_tab_sw_0 and reg_csc_tab_sw_1;*/
		uint32_t reg_csc_tab_sw_update:1;
		uint32_t rsv_1_15:15;
		/*update yuv2rgb coeff value by software
		0: use const, 1:update coeff by reg_csc_coeff_sw;*/
		uint32_t reg_csc_coeff_sw_update:1;
	};
	uint32_t val;
} IVE_TOP_REG_12_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_00:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_0_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_01:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_1_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_02:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_2_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_03:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_3_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_04:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_4_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_05:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_5_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_06:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_6_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_07:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_7_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_08:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_8_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_09:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_9_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_10:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_A_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_coeff_sw_11:19;
	};
	uint32_t val;
} IVE_TOP_REG_CSC_COEFF_B_C;
typedef union {
	struct {
		/*en_mode:
		0,1,2,3: yuv2rgb
		4,5: yuv2rgb2hsv
		6,7: yuv2rgb2lab
		8,9,10,11: rgb2yuv;*/
		uint32_t reg_csc_enmode:4;
		/*csc enable;*/
		uint32_t reg_csc_enable:1;
	};
	uint32_t val;
} IVE_TOP_REG_14_C;
typedef union {
	struct {
		/*unsigned threshold;*/
		uint32_t reg_lbp_u8bit_thr:8;
		/*signed threshold, use 2's   show s7 (1signed + 7bit );*/
		uint32_t reg_lbp_s8bit_thr:8;
		/*0: diff compare to signed threshold,
		1: abs compare to unsigned threshold;*/
		uint32_t reg_lbp_enmode:1;
	};
	uint32_t val;
} IVE_TOP_REG_15_C;
typedef union {
	struct {
		/*                               bgu_fg_wdma_idle            ,
		                                bgu_bg0_wdma_idle           ,
		                                bgu_bg1_wdma_idle           ,
		                                bgu_chg_wdma_idle           ,
		                                rdma_eigval_idle            ,
		                                rdma_img1_idle              ,
		                                rdma_gradfg_bggrad_idle     ,
		                                op_y_wdma_idle              ,
		                                op_c_wdma_idle              ,
		                                hist_wdma_idle              ,
		                                intg_wdma_idle              ,
		                                sad_wdma_idle               ,
		                                thr_wdma_idle               ,
		                                fgflag_dma_idle             ,
		                                bgmodel_0_dma_idle          ,
		                                bgmodel_1_dma_idle          ,
		                                 difffg_dma_idle             ,
		                                dmaf_dma_idle_rdma          ,
		                                dmaf_dma_idle_wdma    ;*/
		uint32_t reg_ive_dma_idle:32;
	};
	uint32_t val;
} IVE_TOP_REG_h54_C;
typedef union {
	struct {
		/*                                model_wdma_idle [0]    ,
		                                model_wdma_idle [1]    ,
		                                model_wdma_idle [2]    ,
		                                model_wdma_idle [3]    ,
		                                model_wdma_idle [4]    ,
		                                match_wdma_idle        ,
		                                model_rdma_idle [0]    ,
		                                model_rdma_idle [1]    ,
		                                model_rdma_idle [2]    ,
		                                model_rdma_idle [3]    ,
		                                model_rdma_idle [4]    ,
		                                factor_rdma_idle       ,
		                                region_wdma_idle       ,
		                                dst_wdma_idle   [0]    ,
		                                dst_wdma_idle   [1]    ,
		                                src_rdma_idle   [0]    ,
		                                src_rdma_idle   [1]    ;*/
		uint32_t reg_ive_gmm_dma_idle:32;
	};
	uint32_t val;
} IVE_TOP_REG_h58_C;
typedef union {
	struct {
		/*dbg enable;*/
		uint32_t reg_dbg_en:1;
		uint32_t rsv_1_3:3;
		/*[00] = m_teol_bgm   ; 
		[01] = m_teol_bgu   ;
		[02] = m_teof_csc_pi;
		[03] = m_teol_2fbg  ;
		[04] = m_teol_2ffg  ;
		[05] = m_teof_gmm   ;
		[06] = img_in;
		[07] = m_teol_map   ;
		[08] = m_teof_r2y4  ;
		[09] = m_teol_resize;
		[10] = s_teol_fg    ;
		[11] = m_teol_op1_fin;
		[12] = m_teol_op2_y_fin;
		[13] = m_teol_op2_c_fin;;*/
		uint32_t reg_dbg_sel:4;
	};
	uint32_t val;
} IVE_TOP_REG_16_C;
typedef union {
	struct {
		/*check hang counter;*/
		uint32_t reg_dbg_col:16;
		/*check hang counter;*/
		uint32_t reg_dbg_row:16;
	};
	uint32_t val;
} IVE_TOP_REG_h64_C;
typedef union {
	struct {
		/*check err for data transaction mismatch;*/
		uint32_t reg_dbg_status:32;
	};
	uint32_t val;
} IVE_TOP_REG_h68_C;
typedef union {
	struct {
		/*set dbg prob line;*/
		uint32_t reg_dbg_pix:16;
		/*set dbg prob pixel;*/
		uint32_t reg_dbg_line:16;
	};
	uint32_t val;
} IVE_TOP_REG_h6c_C;
typedef union {
	struct {
		/*read dbg data;*/
		uint32_t reg_dbg_data:32;
	};
	uint32_t val;
} IVE_TOP_REG_h70_C;
typedef union {
	struct {
		/*1: latch dbg data perframe;*/
		uint32_t reg_dbg_perfmt:1;
		uint32_t rsv_1_15:15;
		/*set dbg prog fmt;*/
		uint32_t reg_dbg_fmt:16;
	};
	uint32_t val;
} IVE_TOP_REG_h74_C;
typedef union {
	struct {
		/*2 frame operation_mode
		3'd0: And
		3'd1: Or
		3'd2: Xor
		3'd3: Add
		3'd4: Sub
		3'd5: Bypass mode output =frame source 0
		3'd6: Bypass mode output =frame source 1
		default: And;*/
		uint32_t reg_frame2op_op_mode:3;
		/*subtraction mode
		This bit is effective when op_mode==3'd4
		1'b0: abs(frame source 0 - frame source 1)
		1'b1: (frame source 0 - frame source 1)>>>1;*/
		uint32_t reg_frame2op_sub_mode:1;
		/*This bit is effective when op_mode==3'd4 & reg_frame2op_sub_mode==1.
		1'b0 :  (frame source 0 - frame source 1)>>>1
		1'b1 :  (frame source 1 - frame source 0)>>>1;*/
		uint32_t reg_frame2op_sub_change_order:1;
		/*set 1 : add rounding ;*/
		uint32_t reg_frame2op_add_mode_rounding:1;
		/*set 1 : clipping in 8bit;*/
		uint32_t reg_frame2op_add_mode_clipping:1;
		/*;*/
		uint32_t reg_frame2op_sub_switch_src:1;
	};
	uint32_t val;
} IVE_TOP_REG_20_C;
typedef union {
	struct {
		/*This bit is effective when op_mode==3'd3
		format : u0q16
		set add function cefficient x.
		cefficient x * frame source 0 + cefficient y * frame source 1;*/
		uint32_t reg_fram2op_x_u0q16:16;
		/*This bit is effective when op_mode==3'd3
		format : u0q16
		set add function cefficient y.
		cefficient x * frame source 0 + cefficient y * frame source 1;*/
		uint32_t reg_fram2op_y_u0q16:16;
	};
	uint32_t val;
} IVE_TOP_REG_21_C;
typedef union {
	struct {
		/*2 frame operation_mode
		3'd0: And
		3'd1: Or
		3'd2: Xor
		3'd3: Add
		3'd4: Sub
		3'd5: Bypass mode output =frame source 0
		3'd6: Bypass mode output =frame source 1
		default: And;*/
		uint32_t reg_frame2op_fg_op_mode:3;
		/*subtraction mode
		This bit is effective when op_mode==3'd4
		1'b0: abs(frame source 0 - frame source 1)
		1'b1: (frame source 0 - frame source 1)>>>1;*/
		uint32_t reg_frame2op_fg_sub_mode:1;
		/*This bit is effective when op_mode==3'd4 & reg_frame2op_fg_sub_mode==1.
		1'b0 :  (frame source 0 - frame source 1)>>>1
		1'b1 :  (frame source 1 - frame source 0)>>>1;*/
		uint32_t reg_frame2op_fg_sub_change_order:1;
		/*set 1 : add rounding ;*/
		uint32_t reg_frame2op_fg_add_mode_rounding:1;
		/*set 1 : clipping in 8bit;*/
		uint32_t reg_frame2op_fg_add_mode_clipping:1;
		/*;*/
		uint32_t reg_frame2op_fg_sub_switch_src:1;
	};
	uint32_t val;
} IVE_TOP_REG_h80_C;
typedef union {
	struct {
		/*This bit is effective when op_mode==3'd3
		format : u0q16
		set add function cefficient x.
		cefficient x * frame source 0 + cefficient y * frame source 1;*/
		uint32_t reg_fram2op_fg_x_u0q16:16;
		/*This bit is effective when op_mode==3'd3
		format : u0q16
		set add function cefficient y.
		cefficient x * frame source 0 + cefficient y * frame source 1;*/
		uint32_t reg_fram2op_fg_y_u0q16:16;
	};
	uint32_t val;
} IVE_TOP_REG_84_C;
typedef union {
	struct {
		/*ip frame done ;*/
		uint32_t reg_frame_done_img_in:1;
		/*;*/
		uint32_t reg_frame_done_rdma_img1:1;
		/*;*/
		uint32_t reg_frame_done_rdma_eigval:1;
		/*;*/
		uint32_t reg_frame_done_resize:1;
		/*;*/
		uint32_t reg_frame_done_gmm:1;
		/*;*/
		uint32_t reg_frame_done_csc:1;
		/*;*/
		uint32_t reg_frame_done_hist:1;
		/*;*/
		uint32_t reg_frame_done_intg:1;
		/*;*/
		uint32_t reg_frame_done_sad:1;
		/*;*/
		uint32_t reg_frame_done_ncc:1;
		/*;*/
		uint32_t reg_frame_done_bgm:1;
		/*;*/
		uint32_t reg_frame_done_bgu:1;
		/*;*/
		uint32_t reg_frame_done_r2y4:1;
		/*;*/
		uint32_t reg_frame_done_frame2op_bg:1;
		/*;*/
		uint32_t reg_frame_done_frame2op_fg:1;
		/*;*/
		uint32_t reg_frame_done_map:1;
		/*;*/
		uint32_t reg_frame_done_thresh16ro8:1;
		/*;*/
		uint32_t reg_frame_done_thresh:1;
		/*;*/
		uint32_t reg_frame_done_filterop_odma:1;
		/*;*/
		uint32_t reg_frame_done_filterop_wdma_y:1;
		/*;*/
		uint32_t reg_frame_done_filterop_wdma_c:1;
		/*;*/
		uint32_t reg_frame_done_dmaf:1;
		/*;*/
		uint32_t reg_frame_done_ccl:1;
		/*;*/
		uint32_t reg_frame_done_lk:1;
		/*filterop wdma_y and wdma_c frame done at same time;*/
		uint32_t reg_frame_done_filterop_wdma_yc:1;
	};
	uint32_t val;
} IVE_TOP_REG_90_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_intr_en_hist:1;
		/*;*/
		uint32_t reg_intr_en_intg:1;
		/*;*/
		uint32_t reg_intr_en_sad:1;
		/*;*/
		uint32_t reg_intr_en_ncc:1;
		/*;*/
		uint32_t reg_intr_en_filterop_odma:1;
		/*;*/
		uint32_t reg_intr_en_filterop_wdma_y:1;
		/*;*/
		uint32_t reg_intr_en_filterop_wdma_c:1;
		/*;*/
		uint32_t reg_intr_en_dmaf:1;
		/*;*/
		uint32_t reg_intr_en_ccl:1;
		/*;*/
		uint32_t reg_intr_en_lk:1;
		/*;*/
		uint32_t reg_intr_en_filterop_wdma_yc:1;
	};
	uint32_t val;
} IVE_TOP_REG_94_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_intr_status_hist:1;
		/*;*/
		uint32_t reg_intr_status_intg:1;
		/*;*/
		uint32_t reg_intr_status_sad:1;
		/*;*/
		uint32_t reg_intr_status_ncc:1;
		/*;*/
		uint32_t reg_intr_status_filterop_odma:1;
		/*;*/
		uint32_t reg_intr_status_filterop_wdma_y:1;
		/*;*/
		uint32_t reg_intr_status_filterop_wdma_c:1;
		/*;*/
		uint32_t reg_intr_status_dmaf:1;
		/*;*/
		uint32_t reg_intr_status_ccl:1;
		/*;*/
		uint32_t reg_intr_status_lk:1;
		/*;*/
		uint32_t reg_intr_status_filterop_wdma_yc:1;
	};
	uint32_t val;
} IVE_TOP_REG_98_C;
typedef union {
	struct {
		/*resize source image width, fill sub 1 value
		example : image width = 1920, fill 1919;*/
		uint32_t reg_resize_src_wd:16;
		/*resize source image height, fill sub 1 value
		example : image height = 1080, fill 1079;*/
		uint32_t reg_resize_src_ht:16;
	};
	uint32_t val;
} IVE_TOP_REG_RS_SRC_SIZE_C;
typedef union {
	struct {
		/*resize destination image width, fill sub 1 value
		example : image width = 1920, fill 1919;*/
		uint32_t reg_resize_dst_wd:16;
		/*resize destination image height, fill sub 1 value
		example : image height = 1080, fill 1079;*/
		uint32_t reg_resize_dst_ht:16;
	};
	uint32_t val;
} IVE_TOP_REG_RS_DST_SIZE_C;
typedef union {
	struct {
		/*horizontal scaling factor, 5.13;*/
		uint32_t reg_resize_h_sc_fac:18;
	};
	uint32_t val;
} IVE_TOP_REG_RS_H_SC_C;
typedef union {
	struct {
		/*vertical scaling factor, 5.13;*/
		uint32_t reg_resize_v_sc_fac:18;
	};
	uint32_t val;
} IVE_TOP_REG_RS_V_SC_C;
typedef union {
	struct {
		/*horizontal initial phase, 0.13, only effective in scaling up mode;*/
		uint32_t reg_resize_h_ini_ph:13;
		uint32_t rsv_13_15:3;
		/*vertical initial phase, 0.13, only effective in scaling up mode;*/
		uint32_t reg_resize_v_ini_ph:13;
	};
	uint32_t val;
} IVE_TOP_REG_RS_PH_INI_C;
typedef union {
	struct {
		/*horizontal normalization number, 0.16, only effective in scaling down mode;*/
		uint32_t reg_resize_h_nor:16;
		/*vertical  normalization number, 0.16, only effective in scaling down mode;*/
		uint32_t reg_resize_v_nor:16;
	};
	uint32_t val;
} IVE_TOP_REG_RS_NOR_C;
typedef union {
	struct {
		/*resize ip enable ;*/
		uint32_t reg_resize_ip_en:1;
		/*resize ip debug enable;*/
		uint32_t reg_resize_dbg_en:1;
		/*resize IVE area fast enable
		0 : disable
		1 : enable (sync with IVE version);*/
		uint32_t reg_resize_area_fast:1;
		/*resize bilinera negative phase use
		0 : use negative phase (sync with IVE version)
		1 : non-use negative phase, ini-phase in reg_resize_x_ini_ph;*/
		uint32_t reg_resize_blnr_mode:1;
	};
	uint32_t val;
} IVE_TOP_REG_RS_CTRL_C;
typedef union {
	struct {
		/*resize debug information H1;*/
		uint32_t reg_resize_sc_dbg_h1:32;
	};
	uint32_t val;
} IVE_TOP_REG_RS_dBG_H1_C;
typedef union {
	struct {
		/*resize debug information H2;*/
		uint32_t reg_resize_sc_dbg_h2:32;
	};
	uint32_t val;
} IVE_TOP_REG_RS_dBG_H2_C;
typedef union {
	struct {
		/*resize debug information V1;*/
		uint32_t reg_resize_sc_dbg_v1:32;
	};
	uint32_t val;
} IVE_TOP_REG_RS_dBG_V1_C;
typedef union {
	struct {
		/*resize debug information V2;*/
		uint32_t reg_resize_sc_dbg_v2:32;
	};
	uint32_t val;
} IVE_TOP_REG_RS_dBG_V2_C;
typedef union {
	struct {
		/*//  Abstract :  1. 16to8 / thresh_s16 /thresh_u16 + thresh
		//              reg_thresh_top_mod = 0 : bypass
		//              reg_thresh_top_mod = 1 : 16to8                   reg_thresh_thresh_en = 1 : thresh
		//              reg_thresh_top_mod = 2 : thresh_s16     +   reg_thresh_thresh_en = 0 : bypass 
		//              reg_thresh_top_mod = 3 : thresh_u16;*/
		uint32_t reg_thresh_top_mod:2;
		uint32_t rsv_2_3:2;
		/*;*/
		uint32_t reg_thresh_thresh_en:1;
		uint32_t rsv_5_7:3;
		/*module sw reset;*/
		uint32_t reg_thresh_softrst:1;
	};
	uint32_t val;
} IVE_TOP_REG_h130_C;
typedef union {
	struct {
		/*//  Abstract : u16 , s16 -> u8 , s8
		//  reg_16to8_mod = 0 : s16 -> s8
		//  reg_16to8_mod = 1 : s16 -> u8 abs
		//  reg_16to8_mod = 2 : s16 -> u8 bias
		//  reg_16to8_mod = 3 : u16 -> u8;*/
		uint32_t reg_thresh_16to8_mod:3;
		uint32_t rsv_3_7:5;
		/*;*/
		uint32_t reg_thresh_16to8_s8bias:8;
		/*;*/
		uint32_t reg_thresh_16to8_u8Num_div_u16Den:16;
	};
	uint32_t val;
} IVE_TOP_REG_h134_C;
typedef union {
	struct {
		/*[ stcandi ] cal : set this = 1;*/
		uint32_t reg_thresh_st_16to8_en:1;
		uint32_t rsv_1_7:7;
		/*[ stcandi ] set this reg;*/
		uint32_t reg_thresh_st_16to8_u8Numerator:8;
		/*read only register for stcandi;*/
		uint32_t reg_thresh_st_16to8_maxeigval:16;
	};
	uint32_t val;
} IVE_TOP_REG_h138_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_thresh_s16_enmode:2;
		uint32_t rsv_2_7:6;
		/*;*/
		uint32_t reg_thresh_s16_u8bit_min:8;
		/*;*/
		uint32_t reg_thresh_s16_u8bit_mid:8;
		/*;*/
		uint32_t reg_thresh_s16_u8bit_max:8;
	};
	uint32_t val;
} IVE_TOP_REG_h13c_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_thresh_s16_bit_thr_l:16;
		/*;*/
		uint32_t reg_thresh_s16_bit_thr_h:16;
	};
	uint32_t val;
} IVE_TOP_REG_h140_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_thresh_u16_enmode:1;
		uint32_t rsv_1_7:7;
		/*;*/
		uint32_t reg_thresh_u16_u8bit_min:8;
		/*;*/
		uint32_t reg_thresh_u16_u8bit_mid:8;
		/*;*/
		uint32_t reg_thresh_u16_u8bit_max:8;
	};
	uint32_t val;
} IVE_TOP_REG_h144_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_thresh_u16_bit_thr_l:16;
		/*;*/
		uint32_t reg_thresh_u16_bit_thr_h:16;
	};
	uint32_t val;
} IVE_TOP_REG_h148_C;
typedef union {
	struct {
		/*Thresh unsigned 8bit low;*/
		uint32_t reg_thresh_u8bit_thr_l:8;
		/*Thresh unsigned 8bit high;*/
		uint32_t reg_thresh_u8bit_thr_h:8;
		/*;*/
		uint32_t reg_thresh_enmode:3;
	};
	uint32_t val;
} IVE_TOP_REG_h14c_C;
typedef union {
	struct {
		/*Thresh unsigned 8bit min;*/
		uint32_t reg_thresh_u8bit_min:8;
		/*Thresh unsigned 8bit mid;*/
		uint32_t reg_thresh_u8bit_mid:8;
		/*Thresh unsigned 8bit max;*/
		uint32_t reg_thresh_u8bit_max:8;
	};
	uint32_t val;
} IVE_TOP_REG_h150_C;
typedef union {
	struct {
		/*NCC read-only register;*/
		uint32_t reg_ncc_nemerator_l:32;
	};
	uint32_t val;
} IVE_TOP_REG_h160_C;
typedef union {
	struct {
		/*NCC read-only register;*/
		uint32_t reg_ncc_nemerator_m:32;
	};
	uint32_t val;
} IVE_TOP_REG_h164_C;
typedef union {
	struct {
		/*NCC read-only register;*/
		uint32_t reg_ncc_quadsum0_l:32;
	};
	uint32_t val;
} IVE_TOP_REG_h168_C;
typedef union {
	struct {
		/*NCC read-only register;*/
		uint32_t reg_ncc_quadsum0_m:32;
	};
	uint32_t val;
} IVE_TOP_REG_h16C_C;
typedef union {
	struct {
		/*NCC read-only register;*/
		uint32_t reg_ncc_quadsum1_l:32;
	};
	uint32_t val;
} IVE_TOP_REG_h170_C;
typedef union {
	struct {
		/*NCC read-only register;*/
		uint32_t reg_ncc_quadsum1_m:32;
	};
	uint32_t val;
} IVE_TOP_REG_h174_C;
typedef union {
	struct {
		/*unsigned 12 bit, update table value of inv_v_tab(rgb2hsv) or gamma_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_csc_r2y4_tab_sw_0:12;
		uint32_t rsv_12_15:4;
		/*unsigned 15 bit, update table value of inv_h_tab(rgb2hsv) or xyz_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_csc_r2y4_tab_sw_1:15;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_11_C;
typedef union {
	struct {
		/*update rgb2hsv/rgb2lab table value by software
		0:use const, 1:update table by reg_csc_tab_sw_0 and reg_csc_tab_sw_1;*/
		uint32_t reg_csc_r2y4_tab_sw_update:1;
		uint32_t rsv_1_15:15;
		/*update yuv2rgb coeff value by software
		0: use const, 1:update coeff by reg_csc_coeff_sw;*/
		uint32_t reg_csc_r2y4_coeff_sw_update:1;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_12_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_00:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_0_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_01:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_1_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_02:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_2_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_03:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_3_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_04:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_4_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_05:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_5_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_06:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_6_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_07:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_7_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_08:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_8_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_09:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_9_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_10:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_A_C;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_csc_r2y4_coeff_sw_11:19;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_COEFF_B_C;
typedef union {
	struct {
		/*en_mode: (only suport rgb2yuv)
		0,1,2,3: yuv2rgb
		4,5: yuv2rgb2hsv
		6,7: yuv2rgb2lab
		8,9,10,11: rgb2yuv ;*/
		uint32_t reg_csc_r2y4_enmode:4;
		/*reg_csc_r2y4_enable;*/
		uint32_t reg_csc_r2y4_enable:1;
	};
	uint32_t val;
} IVE_TOP_REG_R2Y4_14_C;
typedef struct {
	volatile IVE_TOP_REG_0_C REG_0;
	volatile IVE_TOP_REG_1_C REG_1;
	volatile IVE_TOP_REG_2_C REG_2;
	volatile IVE_TOP_REG_3_C REG_3;
	volatile IVE_TOP_REG_h10_C REG_h10;
	volatile IVE_TOP_REG_11_C REG_11;
	volatile IVE_TOP_REG_12_C REG_12;
	volatile IVE_TOP_REG_CSC_COEFF_0_C REG_CSC_COEFF_0;
	volatile IVE_TOP_REG_CSC_COEFF_1_C REG_CSC_COEFF_1;
	volatile IVE_TOP_REG_CSC_COEFF_2_C REG_CSC_COEFF_2;
	volatile IVE_TOP_REG_CSC_COEFF_3_C REG_CSC_COEFF_3;
	volatile IVE_TOP_REG_CSC_COEFF_4_C REG_CSC_COEFF_4;
	volatile IVE_TOP_REG_CSC_COEFF_5_C REG_CSC_COEFF_5;
	volatile IVE_TOP_REG_CSC_COEFF_6_C REG_CSC_COEFF_6;
	volatile IVE_TOP_REG_CSC_COEFF_7_C REG_CSC_COEFF_7;
	volatile IVE_TOP_REG_CSC_COEFF_8_C REG_CSC_COEFF_8;
	volatile IVE_TOP_REG_CSC_COEFF_9_C REG_CSC_COEFF_9;
	volatile IVE_TOP_REG_CSC_COEFF_A_C REG_CSC_COEFF_A;
	volatile IVE_TOP_REG_CSC_COEFF_B_C REG_CSC_COEFF_B;
	volatile IVE_TOP_REG_14_C REG_14;
	volatile IVE_TOP_REG_15_C REG_15;
	volatile IVE_TOP_REG_h54_C REG_h54;
	volatile IVE_TOP_REG_h58_C REG_h58;
	volatile uint32_t _REG_16_0; // 0x5C
	volatile IVE_TOP_REG_16_C REG_16;
	volatile IVE_TOP_REG_h64_C REG_h64;
	volatile IVE_TOP_REG_h68_C REG_h68;
	volatile IVE_TOP_REG_h6c_C REG_h6c;
	volatile IVE_TOP_REG_h70_C REG_h70;
	volatile IVE_TOP_REG_h74_C REG_h74;
	volatile IVE_TOP_REG_20_C REG_20;
	volatile IVE_TOP_REG_21_C REG_21;
	volatile IVE_TOP_REG_h80_C REG_h80;
	volatile IVE_TOP_REG_84_C REG_84;
	volatile uint32_t _REG_90_0; // 0x88
	volatile uint32_t _REG_90_1; // 0x8C
	volatile IVE_TOP_REG_90_C REG_90;
	volatile IVE_TOP_REG_94_C REG_94;
	volatile IVE_TOP_REG_98_C REG_98;
	volatile uint32_t _REG_RS_SRC_SIZE_0; // 0x9C
	volatile uint32_t _REG_RS_SRC_SIZE_1; // 0xA0
	volatile uint32_t _REG_RS_SRC_SIZE_2; // 0xA4
	volatile uint32_t _REG_RS_SRC_SIZE_3; // 0xA8
	volatile uint32_t _REG_RS_SRC_SIZE_4; // 0xAC
	volatile uint32_t _REG_RS_SRC_SIZE_5; // 0xB0
	volatile uint32_t _REG_RS_SRC_SIZE_6; // 0xB4
	volatile uint32_t _REG_RS_SRC_SIZE_7; // 0xB8
	volatile uint32_t _REG_RS_SRC_SIZE_8; // 0xBC
	volatile uint32_t _REG_RS_SRC_SIZE_9; // 0xC0
	volatile uint32_t _REG_RS_SRC_SIZE_10; // 0xC4
	volatile uint32_t _REG_RS_SRC_SIZE_11; // 0xC8
	volatile uint32_t _REG_RS_SRC_SIZE_12; // 0xCC
	volatile uint32_t _REG_RS_SRC_SIZE_13; // 0xD0
	volatile uint32_t _REG_RS_SRC_SIZE_14; // 0xD4
	volatile uint32_t _REG_RS_SRC_SIZE_15; // 0xD8
	volatile uint32_t _REG_RS_SRC_SIZE_16; // 0xDC
	volatile uint32_t _REG_RS_SRC_SIZE_17; // 0xE0
	volatile uint32_t _REG_RS_SRC_SIZE_18; // 0xE4
	volatile uint32_t _REG_RS_SRC_SIZE_19; // 0xE8
	volatile uint32_t _REG_RS_SRC_SIZE_20; // 0xEC
	volatile uint32_t _REG_RS_SRC_SIZE_21; // 0xF0
	volatile uint32_t _REG_RS_SRC_SIZE_22; // 0xF4
	volatile uint32_t _REG_RS_SRC_SIZE_23; // 0xF8
	volatile uint32_t _REG_RS_SRC_SIZE_24; // 0xFC
	volatile IVE_TOP_REG_RS_SRC_SIZE_C REG_RS_SRC_SIZE;
	volatile IVE_TOP_REG_RS_DST_SIZE_C REG_RS_DST_SIZE;
	volatile IVE_TOP_REG_RS_H_SC_C REG_RS_H_SC;
	volatile IVE_TOP_REG_RS_V_SC_C REG_RS_V_SC;
	volatile IVE_TOP_REG_RS_PH_INI_C REG_RS_PH_INI;
	volatile IVE_TOP_REG_RS_NOR_C REG_RS_NOR;
	volatile IVE_TOP_REG_RS_CTRL_C REG_RS_CTRL;
	volatile IVE_TOP_REG_RS_dBG_H1_C REG_RS_dBG_H1;
	volatile IVE_TOP_REG_RS_dBG_H2_C REG_RS_dBG_H2;
	volatile IVE_TOP_REG_RS_dBG_V1_C REG_RS_dBG_V1;
	volatile IVE_TOP_REG_RS_dBG_V2_C REG_RS_dBG_V2;
	volatile uint32_t _REG_h130_0; // 0x12C
	volatile IVE_TOP_REG_h130_C REG_h130;
	volatile IVE_TOP_REG_h134_C REG_h134;
	volatile IVE_TOP_REG_h138_C REG_h138;
	volatile IVE_TOP_REG_h13c_C REG_h13c;
	volatile IVE_TOP_REG_h140_C REG_h140;
	volatile IVE_TOP_REG_h144_C REG_h144;
	volatile IVE_TOP_REG_h148_C REG_h148;
	volatile IVE_TOP_REG_h14c_C REG_h14c;
	volatile IVE_TOP_REG_h150_C REG_h150;
	volatile uint32_t _REG_h160_0; // 0x154
	volatile uint32_t _REG_h160_1; // 0x158
	volatile uint32_t _REG_h160_2; // 0x15C
	volatile IVE_TOP_REG_h160_C REG_h160;
	volatile IVE_TOP_REG_h164_C REG_h164;
	volatile IVE_TOP_REG_h168_C REG_h168;
	volatile IVE_TOP_REG_h16C_C REG_h16C;
	volatile IVE_TOP_REG_h170_C REG_h170;
	volatile IVE_TOP_REG_h174_C REG_h174;
	volatile uint32_t _REG_R2Y4_11_0; // 0x178
	volatile uint32_t _REG_R2Y4_11_1; // 0x17C
	volatile IVE_TOP_REG_R2Y4_11_C REG_R2Y4_11;
	volatile IVE_TOP_REG_R2Y4_12_C REG_R2Y4_12;
	volatile IVE_TOP_REG_R2Y4_COEFF_0_C REG_R2Y4_COEFF_0;
	volatile IVE_TOP_REG_R2Y4_COEFF_1_C REG_R2Y4_COEFF_1;
	volatile IVE_TOP_REG_R2Y4_COEFF_2_C REG_R2Y4_COEFF_2;
	volatile IVE_TOP_REG_R2Y4_COEFF_3_C REG_R2Y4_COEFF_3;
	volatile IVE_TOP_REG_R2Y4_COEFF_4_C REG_R2Y4_COEFF_4;
	volatile IVE_TOP_REG_R2Y4_COEFF_5_C REG_R2Y4_COEFF_5;
	volatile IVE_TOP_REG_R2Y4_COEFF_6_C REG_R2Y4_COEFF_6;
	volatile IVE_TOP_REG_R2Y4_COEFF_7_C REG_R2Y4_COEFF_7;
	volatile IVE_TOP_REG_R2Y4_COEFF_8_C REG_R2Y4_COEFF_8;
	volatile IVE_TOP_REG_R2Y4_COEFF_9_C REG_R2Y4_COEFF_9;
	volatile IVE_TOP_REG_R2Y4_COEFF_A_C REG_R2Y4_COEFF_A;
	volatile IVE_TOP_REG_R2Y4_COEFF_B_C REG_R2Y4_COEFF_B;
	volatile uint32_t _REG_R2Y4_14_0; // 0x1B8
	volatile IVE_TOP_REG_R2Y4_14_C REG_R2Y4_14;
} IVE_TOP_C;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_top_dump_ini(FILE* fp, IVE_TOP_C* p) {
	fprintf(fp, "reg_img_in_uv_swap = 0x%x\n",p->REG_0.reg_img_in_uv_swap);
	fprintf(fp, "reg_img_1_uv_swap = 0x%x\n",p->REG_0.reg_img_1_uv_swap);
	fprintf(fp, "reg_rdma_eigval_uv_swap = 0x%x\n",p->REG_0.reg_rdma_eigval_uv_swap);
	fprintf(fp, "reg_trig_cnt = 0x%x\n",p->REG_0.reg_trig_cnt);
	fprintf(fp, "reg_softrst = 0x%x\n",p->REG_1.reg_softrst);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->REG_1.reg_shdw_sel);
	fprintf(fp, "reg_fmt_vld_fg = 0x%x\n",p->REG_1.reg_fmt_vld_fg);
	fprintf(fp, "reg_fmt_vld_ccl = 0x%x\n",p->REG_1.reg_fmt_vld_ccl);
	fprintf(fp, "reg_fmt_vld_dmaf = 0x%x\n",p->REG_1.reg_fmt_vld_dmaf);
	fprintf(fp, "reg_fmt_vld_lk = 0x%x\n",p->REG_1.reg_fmt_vld_lk);
	fprintf(fp, "reg_cmdq_tsk_trig = 0x%x\n",p->REG_1.reg_cmdq_tsk_trig);
	fprintf(fp, "reg_cmdq_tsk_sel = 0x%x\n",p->REG_1.reg_cmdq_tsk_sel);
	fprintf(fp, "reg_cmdq_tsk_en = 0x%x\n",p->REG_1.reg_cmdq_tsk_en);
	fprintf(fp, "reg_dma_abort = 0x%x\n",p->REG_1.reg_dma_abort);
	fprintf(fp, "reg_wdma_abort_done = 0x%x\n",p->REG_1.reg_wdma_abort_done);
	fprintf(fp, "reg_rdma_abort_done = 0x%x\n",p->REG_1.reg_rdma_abort_done);
	fprintf(fp, "reg_img_in_axi_idle = 0x%x\n",p->REG_1.reg_img_in_axi_idle);
	fprintf(fp, "reg_odma_axi_idle = 0x%x\n",p->REG_1.reg_odma_axi_idle);
	fprintf(fp, "reg_img_widthm1 = 0x%x\n",p->REG_2.reg_img_widthm1);
	fprintf(fp, "reg_img_heightm1 = 0x%x\n",p->REG_2.reg_img_heightm1);
	fprintf(fp, "reg_imgmux_img0_sel = 0x%x\n",p->REG_3.reg_imgmux_img0_sel);
	fprintf(fp, "reg_mapmux_rdma_sel = 0x%x\n",p->REG_3.reg_mapmux_rdma_sel);
	fprintf(fp, "reg_ive_rdma_img1_en = 0x%x\n",p->REG_3.reg_ive_rdma_img1_en);
	fprintf(fp, "reg_ive_rdma_img1_mod_u8 = 0x%x\n",p->REG_3.reg_ive_rdma_img1_mod_u8);
	fprintf(fp, "reg_ive_rdma_eigval_en = 0x%x\n",p->REG_3.reg_ive_rdma_eigval_en);
	fprintf(fp, "reg_muxsel_gradfg = 0x%x\n",p->REG_3.reg_muxsel_gradfg);
	fprintf(fp, "reg_dma_share_mux_selgmm = 0x%x\n",p->REG_3.reg_dma_share_mux_selgmm);
	fprintf(fp, "reg_img_in_top_enable = 0x%x\n",p->REG_h10.reg_img_in_top_enable);
	fprintf(fp, "reg_resize_top_enable = 0x%x\n",p->REG_h10.reg_resize_top_enable);
	fprintf(fp, "reg_gmm_top_enable = 0x%x\n",p->REG_h10.reg_gmm_top_enable);
	fprintf(fp, "reg_csc_top_enable = 0x%x\n",p->REG_h10.reg_csc_top_enable);
	fprintf(fp, "reg_rdma_img1_top_enable = 0x%x\n",p->REG_h10.reg_rdma_img1_top_enable);
	fprintf(fp, "reg_bgm_top_enable = 0x%x\n",p->REG_h10.reg_bgm_top_enable);
	fprintf(fp, "reg_bgu_top_enable = 0x%x\n",p->REG_h10.reg_bgu_top_enable);
	fprintf(fp, "reg_r2y4_top_enable = 0x%x\n",p->REG_h10.reg_r2y4_top_enable);
	fprintf(fp, "reg_map_top_enable = 0x%x\n",p->REG_h10.reg_map_top_enable);
	fprintf(fp, "reg_rdma_eigval_top_enable = 0x%x\n",p->REG_h10.reg_rdma_eigval_top_enable);
	fprintf(fp, "reg_thresh_top_enable = 0x%x\n",p->REG_h10.reg_thresh_top_enable);
	fprintf(fp, "reg_hist_top_enable = 0x%x\n",p->REG_h10.reg_hist_top_enable);
	fprintf(fp, "reg_intg_top_enable = 0x%x\n",p->REG_h10.reg_intg_top_enable);
	fprintf(fp, "reg_ncc_top_enable = 0x%x\n",p->REG_h10.reg_ncc_top_enable);
	fprintf(fp, "reg_sad_top_enable = 0x%x\n",p->REG_h10.reg_sad_top_enable);
	fprintf(fp, "reg_filterop_top_enable = 0x%x\n",p->REG_h10.reg_filterop_top_enable);
	fprintf(fp, "reg_dmaf_top_enable = 0x%x\n",p->REG_h10.reg_dmaf_top_enable);
	fprintf(fp, "reg_ccl_top_enable = 0x%x\n",p->REG_h10.reg_ccl_top_enable);
	fprintf(fp, "reg_lk_top_enable = 0x%x\n",p->REG_h10.reg_lk_top_enable);
	fprintf(fp, "reg_csc_tab_sw_0 = 0x%x\n",p->REG_11.reg_csc_tab_sw_0);
	fprintf(fp, "reg_csc_tab_sw_1 = 0x%x\n",p->REG_11.reg_csc_tab_sw_1);
	fprintf(fp, "reg_csc_tab_sw_update = 0x%x\n",p->REG_12.reg_csc_tab_sw_update);
	fprintf(fp, "reg_csc_coeff_sw_update = 0x%x\n",p->REG_12.reg_csc_coeff_sw_update);
	fprintf(fp, "reg_csc_coeff_sw_00 = 0x%x\n",p->REG_CSC_COEFF_0.reg_csc_coeff_sw_00);
	fprintf(fp, "reg_csc_coeff_sw_01 = 0x%x\n",p->REG_CSC_COEFF_1.reg_csc_coeff_sw_01);
	fprintf(fp, "reg_csc_coeff_sw_02 = 0x%x\n",p->REG_CSC_COEFF_2.reg_csc_coeff_sw_02);
	fprintf(fp, "reg_csc_coeff_sw_03 = 0x%x\n",p->REG_CSC_COEFF_3.reg_csc_coeff_sw_03);
	fprintf(fp, "reg_csc_coeff_sw_04 = 0x%x\n",p->REG_CSC_COEFF_4.reg_csc_coeff_sw_04);
	fprintf(fp, "reg_csc_coeff_sw_05 = 0x%x\n",p->REG_CSC_COEFF_5.reg_csc_coeff_sw_05);
	fprintf(fp, "reg_csc_coeff_sw_06 = 0x%x\n",p->REG_CSC_COEFF_6.reg_csc_coeff_sw_06);
	fprintf(fp, "reg_csc_coeff_sw_07 = 0x%x\n",p->REG_CSC_COEFF_7.reg_csc_coeff_sw_07);
	fprintf(fp, "reg_csc_coeff_sw_08 = 0x%x\n",p->REG_CSC_COEFF_8.reg_csc_coeff_sw_08);
	fprintf(fp, "reg_csc_coeff_sw_09 = 0x%x\n",p->REG_CSC_COEFF_9.reg_csc_coeff_sw_09);
	fprintf(fp, "reg_csc_coeff_sw_10 = 0x%x\n",p->REG_CSC_COEFF_A.reg_csc_coeff_sw_10);
	fprintf(fp, "reg_csc_coeff_sw_11 = 0x%x\n",p->REG_CSC_COEFF_B.reg_csc_coeff_sw_11);
	fprintf(fp, "reg_csc_enmode = 0x%x\n",p->REG_14.reg_csc_enmode);
	fprintf(fp, "reg_csc_enable = 0x%x\n",p->REG_14.reg_csc_enable);
	fprintf(fp, "reg_lbp_u8bit_thr = 0x%x\n",p->REG_15.reg_lbp_u8bit_thr);
	fprintf(fp, "reg_lbp_s8bit_thr = 0x%x\n",p->REG_15.reg_lbp_s8bit_thr);
	fprintf(fp, "reg_lbp_enmode = 0x%x\n",p->REG_15.reg_lbp_enmode);
	fprintf(fp, "reg_ive_dma_idle = 0x%x\n",p->REG_h54.reg_ive_dma_idle);
	fprintf(fp, "reg_ive_gmm_dma_idle = 0x%x\n",p->REG_h58.reg_ive_gmm_dma_idle);
	fprintf(fp, "reg_dbg_en = 0x%x\n",p->REG_16.reg_dbg_en);
	fprintf(fp, "reg_dbg_sel = 0x%x\n",p->REG_16.reg_dbg_sel);
	fprintf(fp, "reg_dbg_col = 0x%x\n",p->REG_h64.reg_dbg_col);
	fprintf(fp, "reg_dbg_row = 0x%x\n",p->REG_h64.reg_dbg_row);
	fprintf(fp, "reg_dbg_status = 0x%x\n",p->REG_h68.reg_dbg_status);
	fprintf(fp, "reg_dbg_pix = 0x%x\n",p->REG_h6c.reg_dbg_pix);
	fprintf(fp, "reg_dbg_line = 0x%x\n",p->REG_h6c.reg_dbg_line);
	fprintf(fp, "reg_dbg_data = 0x%x\n",p->REG_h70.reg_dbg_data);
	fprintf(fp, "reg_dbg_perfmt = 0x%x\n",p->REG_h74.reg_dbg_perfmt);
	fprintf(fp, "reg_dbg_fmt = 0x%x\n",p->REG_h74.reg_dbg_fmt);
	fprintf(fp, "reg_frame2op_op_mode = 0x%x\n",p->REG_20.reg_frame2op_op_mode);
	fprintf(fp, "reg_frame2op_sub_mode = 0x%x\n",p->REG_20.reg_frame2op_sub_mode);
	fprintf(fp, "reg_frame2op_sub_change_order = 0x%x\n",p->REG_20.reg_frame2op_sub_change_order);
	fprintf(fp, "reg_frame2op_add_mode_rounding = 0x%x\n",p->REG_20.reg_frame2op_add_mode_rounding);
	fprintf(fp, "reg_frame2op_add_mode_clipping = 0x%x\n",p->REG_20.reg_frame2op_add_mode_clipping);
	fprintf(fp, "reg_frame2op_sub_switch_src = 0x%x\n",p->REG_20.reg_frame2op_sub_switch_src);
	fprintf(fp, "reg_fram2op_x_u0q16 = 0x%x\n",p->REG_21.reg_fram2op_x_u0q16);
	fprintf(fp, "reg_fram2op_y_u0q16 = 0x%x\n",p->REG_21.reg_fram2op_y_u0q16);
	fprintf(fp, "reg_frame2op_fg_op_mode = 0x%x\n",p->REG_h80.reg_frame2op_fg_op_mode);
	fprintf(fp, "reg_frame2op_fg_sub_mode = 0x%x\n",p->REG_h80.reg_frame2op_fg_sub_mode);
	fprintf(fp, "reg_frame2op_fg_sub_change_order = 0x%x\n",p->REG_h80.reg_frame2op_fg_sub_change_order);
	fprintf(fp, "reg_frame2op_fg_add_mode_rounding = 0x%x\n",p->REG_h80.reg_frame2op_fg_add_mode_rounding);
	fprintf(fp, "reg_frame2op_fg_add_mode_clipping = 0x%x\n",p->REG_h80.reg_frame2op_fg_add_mode_clipping);
	fprintf(fp, "reg_frame2op_fg_sub_switch_src = 0x%x\n",p->REG_h80.reg_frame2op_fg_sub_switch_src);
	fprintf(fp, "reg_fram2op_fg_x_u0q16 = 0x%x\n",p->REG_84.reg_fram2op_fg_x_u0q16);
	fprintf(fp, "reg_fram2op_fg_y_u0q16 = 0x%x\n",p->REG_84.reg_fram2op_fg_y_u0q16);
	fprintf(fp, "reg_frame_done_img_in = 0x%x\n",p->REG_90.reg_frame_done_img_in);
	fprintf(fp, "reg_frame_done_rdma_img1 = 0x%x\n",p->REG_90.reg_frame_done_rdma_img1);
	fprintf(fp, "reg_frame_done_rdma_eigval = 0x%x\n",p->REG_90.reg_frame_done_rdma_eigval);
	fprintf(fp, "reg_frame_done_resize = 0x%x\n",p->REG_90.reg_frame_done_resize);
	fprintf(fp, "reg_frame_done_gmm = 0x%x\n",p->REG_90.reg_frame_done_gmm);
	fprintf(fp, "reg_frame_done_csc = 0x%x\n",p->REG_90.reg_frame_done_csc);
	fprintf(fp, "reg_frame_done_hist = 0x%x\n",p->REG_90.reg_frame_done_hist);
	fprintf(fp, "reg_frame_done_intg = 0x%x\n",p->REG_90.reg_frame_done_intg);
	fprintf(fp, "reg_frame_done_sad = 0x%x\n",p->REG_90.reg_frame_done_sad);
	fprintf(fp, "reg_frame_done_ncc = 0x%x\n",p->REG_90.reg_frame_done_ncc);
	fprintf(fp, "reg_frame_done_bgm = 0x%x\n",p->REG_90.reg_frame_done_bgm);
	fprintf(fp, "reg_frame_done_bgu = 0x%x\n",p->REG_90.reg_frame_done_bgu);
	fprintf(fp, "reg_frame_done_r2y4 = 0x%x\n",p->REG_90.reg_frame_done_r2y4);
	fprintf(fp, "reg_frame_done_frame2op_bg = 0x%x\n",p->REG_90.reg_frame_done_frame2op_bg);
	fprintf(fp, "reg_frame_done_frame2op_fg = 0x%x\n",p->REG_90.reg_frame_done_frame2op_fg);
	fprintf(fp, "reg_frame_done_map = 0x%x\n",p->REG_90.reg_frame_done_map);
	fprintf(fp, "reg_frame_done_thresh16ro8 = 0x%x\n",p->REG_90.reg_frame_done_thresh16ro8);
	fprintf(fp, "reg_frame_done_thresh = 0x%x\n",p->REG_90.reg_frame_done_thresh);
	fprintf(fp, "reg_frame_done_filterop_odma = 0x%x\n",p->REG_90.reg_frame_done_filterop_odma);
	fprintf(fp, "reg_frame_done_filterop_wdma_y = 0x%x\n",p->REG_90.reg_frame_done_filterop_wdma_y);
	fprintf(fp, "reg_frame_done_filterop_wdma_c = 0x%x\n",p->REG_90.reg_frame_done_filterop_wdma_c);
	fprintf(fp, "reg_frame_done_dmaf = 0x%x\n",p->REG_90.reg_frame_done_dmaf);
	fprintf(fp, "reg_frame_done_ccl = 0x%x\n",p->REG_90.reg_frame_done_ccl);
	fprintf(fp, "reg_frame_done_lk = 0x%x\n",p->REG_90.reg_frame_done_lk);
	fprintf(fp, "reg_frame_done_filterop_wdma_yc = 0x%x\n",p->REG_90.reg_frame_done_filterop_wdma_yc);
	fprintf(fp, "reg_intr_en_hist = 0x%x\n",p->REG_94.reg_intr_en_hist);
	fprintf(fp, "reg_intr_en_intg = 0x%x\n",p->REG_94.reg_intr_en_intg);
	fprintf(fp, "reg_intr_en_sad = 0x%x\n",p->REG_94.reg_intr_en_sad);
	fprintf(fp, "reg_intr_en_ncc = 0x%x\n",p->REG_94.reg_intr_en_ncc);
	fprintf(fp, "reg_intr_en_filterop_odma = 0x%x\n",p->REG_94.reg_intr_en_filterop_odma);
	fprintf(fp, "reg_intr_en_filterop_wdma_y = 0x%x\n",p->REG_94.reg_intr_en_filterop_wdma_y);
	fprintf(fp, "reg_intr_en_filterop_wdma_c = 0x%x\n",p->REG_94.reg_intr_en_filterop_wdma_c);
	fprintf(fp, "reg_intr_en_dmaf = 0x%x\n",p->REG_94.reg_intr_en_dmaf);
	fprintf(fp, "reg_intr_en_ccl = 0x%x\n",p->REG_94.reg_intr_en_ccl);
	fprintf(fp, "reg_intr_en_lk = 0x%x\n",p->REG_94.reg_intr_en_lk);
	fprintf(fp, "reg_intr_en_filterop_wdma_yc = 0x%x\n",p->REG_94.reg_intr_en_filterop_wdma_yc);
	fprintf(fp, "reg_intr_status_hist = 0x%x\n",p->REG_98.reg_intr_status_hist);
	fprintf(fp, "reg_intr_status_intg = 0x%x\n",p->REG_98.reg_intr_status_intg);
	fprintf(fp, "reg_intr_status_sad = 0x%x\n",p->REG_98.reg_intr_status_sad);
	fprintf(fp, "reg_intr_status_ncc = 0x%x\n",p->REG_98.reg_intr_status_ncc);
	fprintf(fp, "reg_intr_status_filterop_odma = 0x%x\n",p->REG_98.reg_intr_status_filterop_odma);
	fprintf(fp, "reg_intr_status_filterop_wdma_y = 0x%x\n",p->REG_98.reg_intr_status_filterop_wdma_y);
	fprintf(fp, "reg_intr_status_filterop_wdma_c = 0x%x\n",p->REG_98.reg_intr_status_filterop_wdma_c);
	fprintf(fp, "reg_intr_status_dmaf = 0x%x\n",p->REG_98.reg_intr_status_dmaf);
	fprintf(fp, "reg_intr_status_ccl = 0x%x\n",p->REG_98.reg_intr_status_ccl);
	fprintf(fp, "reg_intr_status_lk = 0x%x\n",p->REG_98.reg_intr_status_lk);
	fprintf(fp, "reg_intr_status_filterop_wdma_yc = 0x%x\n",p->REG_98.reg_intr_status_filterop_wdma_yc);
	fprintf(fp, "reg_resize_src_wd = 0x%x\n",p->REG_RS_SRC_SIZE.reg_resize_src_wd);
	fprintf(fp, "reg_resize_src_ht = 0x%x\n",p->REG_RS_SRC_SIZE.reg_resize_src_ht);
	fprintf(fp, "reg_resize_dst_wd = 0x%x\n",p->REG_RS_DST_SIZE.reg_resize_dst_wd);
	fprintf(fp, "reg_resize_dst_ht = 0x%x\n",p->REG_RS_DST_SIZE.reg_resize_dst_ht);
	fprintf(fp, "reg_resize_h_sc_fac = 0x%x\n",p->REG_RS_H_SC.reg_resize_h_sc_fac);
	fprintf(fp, "reg_resize_v_sc_fac = 0x%x\n",p->REG_RS_V_SC.reg_resize_v_sc_fac);
	fprintf(fp, "reg_resize_h_ini_ph = 0x%x\n",p->REG_RS_PH_INI.reg_resize_h_ini_ph);
	fprintf(fp, "reg_resize_v_ini_ph = 0x%x\n",p->REG_RS_PH_INI.reg_resize_v_ini_ph);
	fprintf(fp, "reg_resize_h_nor = 0x%x\n",p->REG_RS_NOR.reg_resize_h_nor);
	fprintf(fp, "reg_resize_v_nor = 0x%x\n",p->REG_RS_NOR.reg_resize_v_nor);
	fprintf(fp, "reg_resize_ip_en = 0x%x\n",p->REG_RS_CTRL.reg_resize_ip_en);
	fprintf(fp, "reg_resize_dbg_en = 0x%x\n",p->REG_RS_CTRL.reg_resize_dbg_en);
	fprintf(fp, "reg_resize_area_fast = 0x%x\n",p->REG_RS_CTRL.reg_resize_area_fast);
	fprintf(fp, "reg_resize_blnr_mode = 0x%x\n",p->REG_RS_CTRL.reg_resize_blnr_mode);
	fprintf(fp, "reg_resize_sc_dbg_h1 = 0x%x\n",p->REG_RS_dBG_H1.reg_resize_sc_dbg_h1);
	fprintf(fp, "reg_resize_sc_dbg_h2 = 0x%x\n",p->REG_RS_dBG_H2.reg_resize_sc_dbg_h2);
	fprintf(fp, "reg_resize_sc_dbg_v1 = 0x%x\n",p->REG_RS_dBG_V1.reg_resize_sc_dbg_v1);
	fprintf(fp, "reg_resize_sc_dbg_v2 = 0x%x\n",p->REG_RS_dBG_V2.reg_resize_sc_dbg_v2);
	fprintf(fp, "reg_thresh_top_mod = 0x%x\n",p->REG_h130.reg_thresh_top_mod);
	fprintf(fp, "reg_thresh_thresh_en = 0x%x\n",p->REG_h130.reg_thresh_thresh_en);
	fprintf(fp, "reg_thresh_softrst = 0x%x\n",p->REG_h130.reg_thresh_softrst);
	fprintf(fp, "reg_thresh_16to8_mod = 0x%x\n",p->REG_h134.reg_thresh_16to8_mod);
	fprintf(fp, "reg_thresh_16to8_s8bias = 0x%x\n",p->REG_h134.reg_thresh_16to8_s8bias);
	fprintf(fp, "reg_thresh_16to8_u8Num_div_u16Den = 0x%x\n",p->REG_h134.reg_thresh_16to8_u8Num_div_u16Den);
	fprintf(fp, "reg_thresh_st_16to8_en = 0x%x\n",p->REG_h138.reg_thresh_st_16to8_en);
	fprintf(fp, "reg_thresh_st_16to8_u8Numerator = 0x%x\n",p->REG_h138.reg_thresh_st_16to8_u8Numerator);
	fprintf(fp, "reg_thresh_st_16to8_maxeigval = 0x%x\n",p->REG_h138.reg_thresh_st_16to8_maxeigval);
	fprintf(fp, "reg_thresh_s16_enmode = 0x%x\n",p->REG_h13c.reg_thresh_s16_enmode);
	fprintf(fp, "reg_thresh_s16_u8bit_min = 0x%x\n",p->REG_h13c.reg_thresh_s16_u8bit_min);
	fprintf(fp, "reg_thresh_s16_u8bit_mid = 0x%x\n",p->REG_h13c.reg_thresh_s16_u8bit_mid);
	fprintf(fp, "reg_thresh_s16_u8bit_max = 0x%x\n",p->REG_h13c.reg_thresh_s16_u8bit_max);
	fprintf(fp, "reg_thresh_s16_bit_thr_l = 0x%x\n",p->REG_h140.reg_thresh_s16_bit_thr_l);
	fprintf(fp, "reg_thresh_s16_bit_thr_h = 0x%x\n",p->REG_h140.reg_thresh_s16_bit_thr_h);
	fprintf(fp, "reg_thresh_u16_enmode = 0x%x\n",p->REG_h144.reg_thresh_u16_enmode);
	fprintf(fp, "reg_thresh_u16_u8bit_min = 0x%x\n",p->REG_h144.reg_thresh_u16_u8bit_min);
	fprintf(fp, "reg_thresh_u16_u8bit_mid = 0x%x\n",p->REG_h144.reg_thresh_u16_u8bit_mid);
	fprintf(fp, "reg_thresh_u16_u8bit_max = 0x%x\n",p->REG_h144.reg_thresh_u16_u8bit_max);
	fprintf(fp, "reg_thresh_u16_bit_thr_l = 0x%x\n",p->REG_h148.reg_thresh_u16_bit_thr_l);
	fprintf(fp, "reg_thresh_u16_bit_thr_h = 0x%x\n",p->REG_h148.reg_thresh_u16_bit_thr_h);
	fprintf(fp, "reg_thresh_u8bit_thr_l = 0x%x\n",p->REG_h14c.reg_thresh_u8bit_thr_l);
	fprintf(fp, "reg_thresh_u8bit_thr_h = 0x%x\n",p->REG_h14c.reg_thresh_u8bit_thr_h);
	fprintf(fp, "reg_thresh_enmode = 0x%x\n",p->REG_h14c.reg_thresh_enmode);
	fprintf(fp, "reg_thresh_u8bit_min = 0x%x\n",p->REG_h150.reg_thresh_u8bit_min);
	fprintf(fp, "reg_thresh_u8bit_mid = 0x%x\n",p->REG_h150.reg_thresh_u8bit_mid);
	fprintf(fp, "reg_thresh_u8bit_max = 0x%x\n",p->REG_h150.reg_thresh_u8bit_max);
	fprintf(fp, "reg_ncc_nemerator_l = 0x%x\n",p->REG_h160.reg_ncc_nemerator_l);
	fprintf(fp, "reg_ncc_nemerator_m = 0x%x\n",p->REG_h164.reg_ncc_nemerator_m);
	fprintf(fp, "reg_ncc_quadsum0_l = 0x%x\n",p->REG_h168.reg_ncc_quadsum0_l);
	fprintf(fp, "reg_ncc_quadsum0_m = 0x%x\n",p->REG_h16C.reg_ncc_quadsum0_m);
	fprintf(fp, "reg_ncc_quadsum1_l = 0x%x\n",p->REG_h170.reg_ncc_quadsum1_l);
	fprintf(fp, "reg_ncc_quadsum1_m = 0x%x\n",p->REG_h174.reg_ncc_quadsum1_m);
	fprintf(fp, "reg_csc_r2y4_tab_sw_0 = 0x%x\n",p->REG_R2Y4_11.reg_csc_r2y4_tab_sw_0);
	fprintf(fp, "reg_csc_r2y4_tab_sw_1 = 0x%x\n",p->REG_R2Y4_11.reg_csc_r2y4_tab_sw_1);
	fprintf(fp, "reg_csc_r2y4_tab_sw_update = 0x%x\n",p->REG_R2Y4_12.reg_csc_r2y4_tab_sw_update);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_update = 0x%x\n",p->REG_R2Y4_12.reg_csc_r2y4_coeff_sw_update);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_00 = 0x%x\n",p->REG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_01 = 0x%x\n",p->REG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_02 = 0x%x\n",p->REG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_03 = 0x%x\n",p->REG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_04 = 0x%x\n",p->REG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_05 = 0x%x\n",p->REG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_06 = 0x%x\n",p->REG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_07 = 0x%x\n",p->REG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_08 = 0x%x\n",p->REG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_09 = 0x%x\n",p->REG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_10 = 0x%x\n",p->REG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10);
	fprintf(fp, "reg_csc_r2y4_coeff_sw_11 = 0x%x\n",p->REG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11);
	fprintf(fp, "reg_csc_r2y4_enmode = 0x%x\n",p->REG_R2Y4_14.reg_csc_r2y4_enmode);
	fprintf(fp, "reg_csc_r2y4_enable = 0x%x\n",p->REG_R2Y4_14.reg_csc_r2y4_enable);

}
static void ive_top_print(IVE_TOP_C* p) {
    fprintf(stderr, "ive_top\n");
	fprintf(stderr, "\tREG_0.reg_img_in_uv_swap = 0x%x\n", p->REG_0.reg_img_in_uv_swap);
	fprintf(stderr, "\tREG_0.reg_img_1_uv_swap = 0x%x\n", p->REG_0.reg_img_1_uv_swap);
	fprintf(stderr, "\tREG_0.reg_rdma_eigval_uv_swap = 0x%x\n", p->REG_0.reg_rdma_eigval_uv_swap);
	fprintf(stderr, "\tREG_0.reg_trig_cnt = 0x%x\n", p->REG_0.reg_trig_cnt);
	fprintf(stderr, "\tREG_1.reg_softrst = 0x%x\n", p->REG_1.reg_softrst);
	fprintf(stderr, "\tREG_1.reg_shdw_sel = 0x%x\n", p->REG_1.reg_shdw_sel);
	fprintf(stderr, "\tREG_1.reg_fmt_vld_fg = 0x%x\n", p->REG_1.reg_fmt_vld_fg);
	fprintf(stderr, "\tREG_1.reg_fmt_vld_ccl = 0x%x\n", p->REG_1.reg_fmt_vld_ccl);
	fprintf(stderr, "\tREG_1.reg_fmt_vld_dmaf = 0x%x\n", p->REG_1.reg_fmt_vld_dmaf);
	fprintf(stderr, "\tREG_1.reg_fmt_vld_lk = 0x%x\n", p->REG_1.reg_fmt_vld_lk);
	fprintf(stderr, "\tREG_1.reg_cmdq_tsk_trig = 0x%x\n", p->REG_1.reg_cmdq_tsk_trig);
	fprintf(stderr, "\tREG_1.reg_cmdq_tsk_sel = 0x%x\n", p->REG_1.reg_cmdq_tsk_sel);
	fprintf(stderr, "\tREG_1.reg_cmdq_tsk_en = 0x%x\n", p->REG_1.reg_cmdq_tsk_en);
	fprintf(stderr, "\tREG_1.reg_dma_abort = 0x%x\n", p->REG_1.reg_dma_abort);
	fprintf(stderr, "\tREG_1.reg_wdma_abort_done = 0x%x\n", p->REG_1.reg_wdma_abort_done);
	fprintf(stderr, "\tREG_1.reg_rdma_abort_done = 0x%x\n", p->REG_1.reg_rdma_abort_done);
	fprintf(stderr, "\tREG_1.reg_img_in_axi_idle = 0x%x\n", p->REG_1.reg_img_in_axi_idle);
	fprintf(stderr, "\tREG_1.reg_odma_axi_idle = 0x%x\n", p->REG_1.reg_odma_axi_idle);
	fprintf(stderr, "\tREG_2.reg_img_widthm1 = 0x%x\n", p->REG_2.reg_img_widthm1);
	fprintf(stderr, "\tREG_2.reg_img_heightm1 = 0x%x\n", p->REG_2.reg_img_heightm1);
	fprintf(stderr, "\tREG_3.reg_imgmux_img0_sel = 0x%x\n", p->REG_3.reg_imgmux_img0_sel);
	fprintf(stderr, "\tREG_3.reg_mapmux_rdma_sel = 0x%x\n", p->REG_3.reg_mapmux_rdma_sel);
	fprintf(stderr, "\tREG_3.reg_ive_rdma_img1_en = 0x%x\n", p->REG_3.reg_ive_rdma_img1_en);
	fprintf(stderr, "\tREG_3.reg_ive_rdma_img1_mod_u8 = 0x%x\n", p->REG_3.reg_ive_rdma_img1_mod_u8);
	fprintf(stderr, "\tREG_3.reg_ive_rdma_eigval_en = 0x%x\n", p->REG_3.reg_ive_rdma_eigval_en);
	fprintf(stderr, "\tREG_3.reg_muxsel_gradfg = 0x%x\n", p->REG_3.reg_muxsel_gradfg);
	fprintf(stderr, "\tREG_3.reg_dma_share_mux_selgmm = 0x%x\n", p->REG_3.reg_dma_share_mux_selgmm);
	fprintf(stderr, "\tREG_h10.reg_img_in_top_enable = 0x%x\n", p->REG_h10.reg_img_in_top_enable);
	fprintf(stderr, "\tREG_h10.reg_resize_top_enable = 0x%x\n", p->REG_h10.reg_resize_top_enable);
	fprintf(stderr, "\tREG_h10.reg_gmm_top_enable = 0x%x\n", p->REG_h10.reg_gmm_top_enable);
	fprintf(stderr, "\tREG_h10.reg_csc_top_enable = 0x%x\n", p->REG_h10.reg_csc_top_enable);
	fprintf(stderr, "\tREG_h10.reg_rdma_img1_top_enable = 0x%x\n", p->REG_h10.reg_rdma_img1_top_enable);
	fprintf(stderr, "\tREG_h10.reg_bgm_top_enable = 0x%x\n", p->REG_h10.reg_bgm_top_enable);
	fprintf(stderr, "\tREG_h10.reg_bgu_top_enable = 0x%x\n", p->REG_h10.reg_bgu_top_enable);
	fprintf(stderr, "\tREG_h10.reg_r2y4_top_enable = 0x%x\n", p->REG_h10.reg_r2y4_top_enable);
	fprintf(stderr, "\tREG_h10.reg_map_top_enable = 0x%x\n", p->REG_h10.reg_map_top_enable);
	fprintf(stderr, "\tREG_h10.reg_rdma_eigval_top_enable = 0x%x\n", p->REG_h10.reg_rdma_eigval_top_enable);
	fprintf(stderr, "\tREG_h10.reg_thresh_top_enable = 0x%x\n", p->REG_h10.reg_thresh_top_enable);
	fprintf(stderr, "\tREG_h10.reg_hist_top_enable = 0x%x\n", p->REG_h10.reg_hist_top_enable);
	fprintf(stderr, "\tREG_h10.reg_intg_top_enable = 0x%x\n", p->REG_h10.reg_intg_top_enable);
	fprintf(stderr, "\tREG_h10.reg_ncc_top_enable = 0x%x\n", p->REG_h10.reg_ncc_top_enable);
	fprintf(stderr, "\tREG_h10.reg_sad_top_enable = 0x%x\n", p->REG_h10.reg_sad_top_enable);
	fprintf(stderr, "\tREG_h10.reg_filterop_top_enable = 0x%x\n", p->REG_h10.reg_filterop_top_enable);
	fprintf(stderr, "\tREG_h10.reg_dmaf_top_enable = 0x%x\n", p->REG_h10.reg_dmaf_top_enable);
	fprintf(stderr, "\tREG_h10.reg_ccl_top_enable = 0x%x\n", p->REG_h10.reg_ccl_top_enable);
	fprintf(stderr, "\tREG_h10.reg_lk_top_enable = 0x%x\n", p->REG_h10.reg_lk_top_enable);
	fprintf(stderr, "\tREG_11.reg_csc_tab_sw_0 = 0x%x\n", p->REG_11.reg_csc_tab_sw_0);
	fprintf(stderr, "\tREG_11.reg_csc_tab_sw_1 = 0x%x\n", p->REG_11.reg_csc_tab_sw_1);
	fprintf(stderr, "\tREG_12.reg_csc_tab_sw_update = 0x%x\n", p->REG_12.reg_csc_tab_sw_update);
	fprintf(stderr, "\tREG_12.reg_csc_coeff_sw_update = 0x%x\n", p->REG_12.reg_csc_coeff_sw_update);
	fprintf(stderr, "\tREG_CSC_COEFF_0.reg_csc_coeff_sw_00 = 0x%x\n", p->REG_CSC_COEFF_0.reg_csc_coeff_sw_00);
	fprintf(stderr, "\tREG_CSC_COEFF_1.reg_csc_coeff_sw_01 = 0x%x\n", p->REG_CSC_COEFF_1.reg_csc_coeff_sw_01);
	fprintf(stderr, "\tREG_CSC_COEFF_2.reg_csc_coeff_sw_02 = 0x%x\n", p->REG_CSC_COEFF_2.reg_csc_coeff_sw_02);
	fprintf(stderr, "\tREG_CSC_COEFF_3.reg_csc_coeff_sw_03 = 0x%x\n", p->REG_CSC_COEFF_3.reg_csc_coeff_sw_03);
	fprintf(stderr, "\tREG_CSC_COEFF_4.reg_csc_coeff_sw_04 = 0x%x\n", p->REG_CSC_COEFF_4.reg_csc_coeff_sw_04);
	fprintf(stderr, "\tREG_CSC_COEFF_5.reg_csc_coeff_sw_05 = 0x%x\n", p->REG_CSC_COEFF_5.reg_csc_coeff_sw_05);
	fprintf(stderr, "\tREG_CSC_COEFF_6.reg_csc_coeff_sw_06 = 0x%x\n", p->REG_CSC_COEFF_6.reg_csc_coeff_sw_06);
	fprintf(stderr, "\tREG_CSC_COEFF_7.reg_csc_coeff_sw_07 = 0x%x\n", p->REG_CSC_COEFF_7.reg_csc_coeff_sw_07);
	fprintf(stderr, "\tREG_CSC_COEFF_8.reg_csc_coeff_sw_08 = 0x%x\n", p->REG_CSC_COEFF_8.reg_csc_coeff_sw_08);
	fprintf(stderr, "\tREG_CSC_COEFF_9.reg_csc_coeff_sw_09 = 0x%x\n", p->REG_CSC_COEFF_9.reg_csc_coeff_sw_09);
	fprintf(stderr, "\tREG_CSC_COEFF_A.reg_csc_coeff_sw_10 = 0x%x\n", p->REG_CSC_COEFF_A.reg_csc_coeff_sw_10);
	fprintf(stderr, "\tREG_CSC_COEFF_B.reg_csc_coeff_sw_11 = 0x%x\n", p->REG_CSC_COEFF_B.reg_csc_coeff_sw_11);
	fprintf(stderr, "\tREG_14.reg_csc_enmode = 0x%x\n", p->REG_14.reg_csc_enmode);
	fprintf(stderr, "\tREG_14.reg_csc_enable = 0x%x\n", p->REG_14.reg_csc_enable);
	fprintf(stderr, "\tREG_15.reg_lbp_u8bit_thr = 0x%x\n", p->REG_15.reg_lbp_u8bit_thr);
	fprintf(stderr, "\tREG_15.reg_lbp_s8bit_thr = 0x%x\n", p->REG_15.reg_lbp_s8bit_thr);
	fprintf(stderr, "\tREG_15.reg_lbp_enmode = 0x%x\n", p->REG_15.reg_lbp_enmode);
	fprintf(stderr, "\tREG_h54.reg_ive_dma_idle = 0x%x\n", p->REG_h54.reg_ive_dma_idle);
	fprintf(stderr, "\tREG_h58.reg_ive_gmm_dma_idle = 0x%x\n", p->REG_h58.reg_ive_gmm_dma_idle);
	fprintf(stderr, "\tREG_16.reg_dbg_en = 0x%x\n", p->REG_16.reg_dbg_en);
	fprintf(stderr, "\tREG_16.reg_dbg_sel = 0x%x\n", p->REG_16.reg_dbg_sel);
	fprintf(stderr, "\tREG_h64.reg_dbg_col = 0x%x\n", p->REG_h64.reg_dbg_col);
	fprintf(stderr, "\tREG_h64.reg_dbg_row = 0x%x\n", p->REG_h64.reg_dbg_row);
	fprintf(stderr, "\tREG_h68.reg_dbg_status = 0x%x\n", p->REG_h68.reg_dbg_status);
	fprintf(stderr, "\tREG_h6c.reg_dbg_pix = 0x%x\n", p->REG_h6c.reg_dbg_pix);
	fprintf(stderr, "\tREG_h6c.reg_dbg_line = 0x%x\n", p->REG_h6c.reg_dbg_line);
	fprintf(stderr, "\tREG_h70.reg_dbg_data = 0x%x\n", p->REG_h70.reg_dbg_data);
	fprintf(stderr, "\tREG_h74.reg_dbg_perfmt = 0x%x\n", p->REG_h74.reg_dbg_perfmt);
	fprintf(stderr, "\tREG_h74.reg_dbg_fmt = 0x%x\n", p->REG_h74.reg_dbg_fmt);
	fprintf(stderr, "\tREG_20.reg_frame2op_op_mode = 0x%x\n", p->REG_20.reg_frame2op_op_mode);
	fprintf(stderr, "\tREG_20.reg_frame2op_sub_mode = 0x%x\n", p->REG_20.reg_frame2op_sub_mode);
	fprintf(stderr, "\tREG_20.reg_frame2op_sub_change_order = 0x%x\n", p->REG_20.reg_frame2op_sub_change_order);
	fprintf(stderr, "\tREG_20.reg_frame2op_add_mode_rounding = 0x%x\n", p->REG_20.reg_frame2op_add_mode_rounding);
	fprintf(stderr, "\tREG_20.reg_frame2op_add_mode_clipping = 0x%x\n", p->REG_20.reg_frame2op_add_mode_clipping);
	fprintf(stderr, "\tREG_20.reg_frame2op_sub_switch_src = 0x%x\n", p->REG_20.reg_frame2op_sub_switch_src);
	fprintf(stderr, "\tREG_21.reg_fram2op_x_u0q16 = 0x%x\n", p->REG_21.reg_fram2op_x_u0q16);
	fprintf(stderr, "\tREG_21.reg_fram2op_y_u0q16 = 0x%x\n", p->REG_21.reg_fram2op_y_u0q16);
	fprintf(stderr, "\tREG_h80.reg_frame2op_fg_op_mode = 0x%x\n", p->REG_h80.reg_frame2op_fg_op_mode);
	fprintf(stderr, "\tREG_h80.reg_frame2op_fg_sub_mode = 0x%x\n", p->REG_h80.reg_frame2op_fg_sub_mode);
	fprintf(stderr, "\tREG_h80.reg_frame2op_fg_sub_change_order = 0x%x\n", p->REG_h80.reg_frame2op_fg_sub_change_order);
	fprintf(stderr, "\tREG_h80.reg_frame2op_fg_add_mode_rounding = 0x%x\n", p->REG_h80.reg_frame2op_fg_add_mode_rounding);
	fprintf(stderr, "\tREG_h80.reg_frame2op_fg_add_mode_clipping = 0x%x\n", p->REG_h80.reg_frame2op_fg_add_mode_clipping);
	fprintf(stderr, "\tREG_h80.reg_frame2op_fg_sub_switch_src = 0x%x\n", p->REG_h80.reg_frame2op_fg_sub_switch_src);
	fprintf(stderr, "\tREG_84.reg_fram2op_fg_x_u0q16 = 0x%x\n", p->REG_84.reg_fram2op_fg_x_u0q16);
	fprintf(stderr, "\tREG_84.reg_fram2op_fg_y_u0q16 = 0x%x\n", p->REG_84.reg_fram2op_fg_y_u0q16);
	fprintf(stderr, "\tREG_90.reg_frame_done_img_in = 0x%x\n", p->REG_90.reg_frame_done_img_in);
	fprintf(stderr, "\tREG_90.reg_frame_done_rdma_img1 = 0x%x\n", p->REG_90.reg_frame_done_rdma_img1);
	fprintf(stderr, "\tREG_90.reg_frame_done_rdma_eigval = 0x%x\n", p->REG_90.reg_frame_done_rdma_eigval);
	fprintf(stderr, "\tREG_90.reg_frame_done_resize = 0x%x\n", p->REG_90.reg_frame_done_resize);
	fprintf(stderr, "\tREG_90.reg_frame_done_gmm = 0x%x\n", p->REG_90.reg_frame_done_gmm);
	fprintf(stderr, "\tREG_90.reg_frame_done_csc = 0x%x\n", p->REG_90.reg_frame_done_csc);
	fprintf(stderr, "\tREG_90.reg_frame_done_hist = 0x%x\n", p->REG_90.reg_frame_done_hist);
	fprintf(stderr, "\tREG_90.reg_frame_done_intg = 0x%x\n", p->REG_90.reg_frame_done_intg);
	fprintf(stderr, "\tREG_90.reg_frame_done_sad = 0x%x\n", p->REG_90.reg_frame_done_sad);
	fprintf(stderr, "\tREG_90.reg_frame_done_ncc = 0x%x\n", p->REG_90.reg_frame_done_ncc);
	fprintf(stderr, "\tREG_90.reg_frame_done_bgm = 0x%x\n", p->REG_90.reg_frame_done_bgm);
	fprintf(stderr, "\tREG_90.reg_frame_done_bgu = 0x%x\n", p->REG_90.reg_frame_done_bgu);
	fprintf(stderr, "\tREG_90.reg_frame_done_r2y4 = 0x%x\n", p->REG_90.reg_frame_done_r2y4);
	fprintf(stderr, "\tREG_90.reg_frame_done_frame2op_bg = 0x%x\n", p->REG_90.reg_frame_done_frame2op_bg);
	fprintf(stderr, "\tREG_90.reg_frame_done_frame2op_fg = 0x%x\n", p->REG_90.reg_frame_done_frame2op_fg);
	fprintf(stderr, "\tREG_90.reg_frame_done_map = 0x%x\n", p->REG_90.reg_frame_done_map);
	fprintf(stderr, "\tREG_90.reg_frame_done_thresh16ro8 = 0x%x\n", p->REG_90.reg_frame_done_thresh16ro8);
	fprintf(stderr, "\tREG_90.reg_frame_done_thresh = 0x%x\n", p->REG_90.reg_frame_done_thresh);
	fprintf(stderr, "\tREG_90.reg_frame_done_filterop_odma = 0x%x\n", p->REG_90.reg_frame_done_filterop_odma);
	fprintf(stderr, "\tREG_90.reg_frame_done_filterop_wdma_y = 0x%x\n", p->REG_90.reg_frame_done_filterop_wdma_y);
	fprintf(stderr, "\tREG_90.reg_frame_done_filterop_wdma_c = 0x%x\n", p->REG_90.reg_frame_done_filterop_wdma_c);
	fprintf(stderr, "\tREG_90.reg_frame_done_dmaf = 0x%x\n", p->REG_90.reg_frame_done_dmaf);
	fprintf(stderr, "\tREG_90.reg_frame_done_ccl = 0x%x\n", p->REG_90.reg_frame_done_ccl);
	fprintf(stderr, "\tREG_90.reg_frame_done_lk = 0x%x\n", p->REG_90.reg_frame_done_lk);
	fprintf(stderr, "\tREG_90.reg_frame_done_filterop_wdma_yc = 0x%x\n", p->REG_90.reg_frame_done_filterop_wdma_yc);
	fprintf(stderr, "\tREG_94.reg_intr_en_hist = 0x%x\n", p->REG_94.reg_intr_en_hist);
	fprintf(stderr, "\tREG_94.reg_intr_en_intg = 0x%x\n", p->REG_94.reg_intr_en_intg);
	fprintf(stderr, "\tREG_94.reg_intr_en_sad = 0x%x\n", p->REG_94.reg_intr_en_sad);
	fprintf(stderr, "\tREG_94.reg_intr_en_ncc = 0x%x\n", p->REG_94.reg_intr_en_ncc);
	fprintf(stderr, "\tREG_94.reg_intr_en_filterop_odma = 0x%x\n", p->REG_94.reg_intr_en_filterop_odma);
	fprintf(stderr, "\tREG_94.reg_intr_en_filterop_wdma_y = 0x%x\n", p->REG_94.reg_intr_en_filterop_wdma_y);
	fprintf(stderr, "\tREG_94.reg_intr_en_filterop_wdma_c = 0x%x\n", p->REG_94.reg_intr_en_filterop_wdma_c);
	fprintf(stderr, "\tREG_94.reg_intr_en_dmaf = 0x%x\n", p->REG_94.reg_intr_en_dmaf);
	fprintf(stderr, "\tREG_94.reg_intr_en_ccl = 0x%x\n", p->REG_94.reg_intr_en_ccl);
	fprintf(stderr, "\tREG_94.reg_intr_en_lk = 0x%x\n", p->REG_94.reg_intr_en_lk);
	fprintf(stderr, "\tREG_94.reg_intr_en_filterop_wdma_yc = 0x%x\n", p->REG_94.reg_intr_en_filterop_wdma_yc);
	fprintf(stderr, "\tREG_98.reg_intr_status_hist = 0x%x\n", p->REG_98.reg_intr_status_hist);
	fprintf(stderr, "\tREG_98.reg_intr_status_intg = 0x%x\n", p->REG_98.reg_intr_status_intg);
	fprintf(stderr, "\tREG_98.reg_intr_status_sad = 0x%x\n", p->REG_98.reg_intr_status_sad);
	fprintf(stderr, "\tREG_98.reg_intr_status_ncc = 0x%x\n", p->REG_98.reg_intr_status_ncc);
	fprintf(stderr, "\tREG_98.reg_intr_status_filterop_odma = 0x%x\n", p->REG_98.reg_intr_status_filterop_odma);
	fprintf(stderr, "\tREG_98.reg_intr_status_filterop_wdma_y = 0x%x\n", p->REG_98.reg_intr_status_filterop_wdma_y);
	fprintf(stderr, "\tREG_98.reg_intr_status_filterop_wdma_c = 0x%x\n", p->REG_98.reg_intr_status_filterop_wdma_c);
	fprintf(stderr, "\tREG_98.reg_intr_status_dmaf = 0x%x\n", p->REG_98.reg_intr_status_dmaf);
	fprintf(stderr, "\tREG_98.reg_intr_status_ccl = 0x%x\n", p->REG_98.reg_intr_status_ccl);
	fprintf(stderr, "\tREG_98.reg_intr_status_lk = 0x%x\n", p->REG_98.reg_intr_status_lk);
	fprintf(stderr, "\tREG_98.reg_intr_status_filterop_wdma_yc = 0x%x\n", p->REG_98.reg_intr_status_filterop_wdma_yc);
	fprintf(stderr, "\tREG_RS_SRC_SIZE.reg_resize_src_wd = 0x%x\n", p->REG_RS_SRC_SIZE.reg_resize_src_wd);
	fprintf(stderr, "\tREG_RS_SRC_SIZE.reg_resize_src_ht = 0x%x\n", p->REG_RS_SRC_SIZE.reg_resize_src_ht);
	fprintf(stderr, "\tREG_RS_DST_SIZE.reg_resize_dst_wd = 0x%x\n", p->REG_RS_DST_SIZE.reg_resize_dst_wd);
	fprintf(stderr, "\tREG_RS_DST_SIZE.reg_resize_dst_ht = 0x%x\n", p->REG_RS_DST_SIZE.reg_resize_dst_ht);
	fprintf(stderr, "\tREG_RS_H_SC.reg_resize_h_sc_fac = 0x%x\n", p->REG_RS_H_SC.reg_resize_h_sc_fac);
	fprintf(stderr, "\tREG_RS_V_SC.reg_resize_v_sc_fac = 0x%x\n", p->REG_RS_V_SC.reg_resize_v_sc_fac);
	fprintf(stderr, "\tREG_RS_PH_INI.reg_resize_h_ini_ph = 0x%x\n", p->REG_RS_PH_INI.reg_resize_h_ini_ph);
	fprintf(stderr, "\tREG_RS_PH_INI.reg_resize_v_ini_ph = 0x%x\n", p->REG_RS_PH_INI.reg_resize_v_ini_ph);
	fprintf(stderr, "\tREG_RS_NOR.reg_resize_h_nor = 0x%x\n", p->REG_RS_NOR.reg_resize_h_nor);
	fprintf(stderr, "\tREG_RS_NOR.reg_resize_v_nor = 0x%x\n", p->REG_RS_NOR.reg_resize_v_nor);
	fprintf(stderr, "\tREG_RS_CTRL.reg_resize_ip_en = 0x%x\n", p->REG_RS_CTRL.reg_resize_ip_en);
	fprintf(stderr, "\tREG_RS_CTRL.reg_resize_dbg_en = 0x%x\n", p->REG_RS_CTRL.reg_resize_dbg_en);
	fprintf(stderr, "\tREG_RS_CTRL.reg_resize_area_fast = 0x%x\n", p->REG_RS_CTRL.reg_resize_area_fast);
	fprintf(stderr, "\tREG_RS_CTRL.reg_resize_blnr_mode = 0x%x\n", p->REG_RS_CTRL.reg_resize_blnr_mode);
	fprintf(stderr, "\tREG_RS_dBG_H1.reg_resize_sc_dbg_h1 = 0x%x\n", p->REG_RS_dBG_H1.reg_resize_sc_dbg_h1);
	fprintf(stderr, "\tREG_RS_dBG_H2.reg_resize_sc_dbg_h2 = 0x%x\n", p->REG_RS_dBG_H2.reg_resize_sc_dbg_h2);
	fprintf(stderr, "\tREG_RS_dBG_V1.reg_resize_sc_dbg_v1 = 0x%x\n", p->REG_RS_dBG_V1.reg_resize_sc_dbg_v1);
	fprintf(stderr, "\tREG_RS_dBG_V2.reg_resize_sc_dbg_v2 = 0x%x\n", p->REG_RS_dBG_V2.reg_resize_sc_dbg_v2);
	fprintf(stderr, "\tREG_h130.reg_thresh_top_mod = 0x%x\n", p->REG_h130.reg_thresh_top_mod);
	fprintf(stderr, "\tREG_h130.reg_thresh_thresh_en = 0x%x\n", p->REG_h130.reg_thresh_thresh_en);
	fprintf(stderr, "\tREG_h130.reg_thresh_softrst = 0x%x\n", p->REG_h130.reg_thresh_softrst);
	fprintf(stderr, "\tREG_h134.reg_thresh_16to8_mod = 0x%x\n", p->REG_h134.reg_thresh_16to8_mod);
	fprintf(stderr, "\tREG_h134.reg_thresh_16to8_s8bias = 0x%x\n", p->REG_h134.reg_thresh_16to8_s8bias);
	fprintf(stderr, "\tREG_h134.reg_thresh_16to8_u8Num_div_u16Den = 0x%x\n", p->REG_h134.reg_thresh_16to8_u8Num_div_u16Den);
	fprintf(stderr, "\tREG_h138.reg_thresh_st_16to8_en = 0x%x\n", p->REG_h138.reg_thresh_st_16to8_en);
	fprintf(stderr, "\tREG_h138.reg_thresh_st_16to8_u8Numerator = 0x%x\n", p->REG_h138.reg_thresh_st_16to8_u8Numerator);
	fprintf(stderr, "\tREG_h138.reg_thresh_st_16to8_maxeigval = 0x%x\n", p->REG_h138.reg_thresh_st_16to8_maxeigval);
	fprintf(stderr, "\tREG_h13c.reg_thresh_s16_enmode = 0x%x\n", p->REG_h13c.reg_thresh_s16_enmode);
	fprintf(stderr, "\tREG_h13c.reg_thresh_s16_u8bit_min = 0x%x\n", p->REG_h13c.reg_thresh_s16_u8bit_min);
	fprintf(stderr, "\tREG_h13c.reg_thresh_s16_u8bit_mid = 0x%x\n", p->REG_h13c.reg_thresh_s16_u8bit_mid);
	fprintf(stderr, "\tREG_h13c.reg_thresh_s16_u8bit_max = 0x%x\n", p->REG_h13c.reg_thresh_s16_u8bit_max);
	fprintf(stderr, "\tREG_h140.reg_thresh_s16_bit_thr_l = 0x%x\n", p->REG_h140.reg_thresh_s16_bit_thr_l);
	fprintf(stderr, "\tREG_h140.reg_thresh_s16_bit_thr_h = 0x%x\n", p->REG_h140.reg_thresh_s16_bit_thr_h);
	fprintf(stderr, "\tREG_h144.reg_thresh_u16_enmode = 0x%x\n", p->REG_h144.reg_thresh_u16_enmode);
	fprintf(stderr, "\tREG_h144.reg_thresh_u16_u8bit_min = 0x%x\n", p->REG_h144.reg_thresh_u16_u8bit_min);
	fprintf(stderr, "\tREG_h144.reg_thresh_u16_u8bit_mid = 0x%x\n", p->REG_h144.reg_thresh_u16_u8bit_mid);
	fprintf(stderr, "\tREG_h144.reg_thresh_u16_u8bit_max = 0x%x\n", p->REG_h144.reg_thresh_u16_u8bit_max);
	fprintf(stderr, "\tREG_h148.reg_thresh_u16_bit_thr_l = 0x%x\n", p->REG_h148.reg_thresh_u16_bit_thr_l);
	fprintf(stderr, "\tREG_h148.reg_thresh_u16_bit_thr_h = 0x%x\n", p->REG_h148.reg_thresh_u16_bit_thr_h);
	fprintf(stderr, "\tREG_h14c.reg_thresh_u8bit_thr_l = 0x%x\n", p->REG_h14c.reg_thresh_u8bit_thr_l);
	fprintf(stderr, "\tREG_h14c.reg_thresh_u8bit_thr_h = 0x%x\n", p->REG_h14c.reg_thresh_u8bit_thr_h);
	fprintf(stderr, "\tREG_h14c.reg_thresh_enmode = 0x%x\n", p->REG_h14c.reg_thresh_enmode);
	fprintf(stderr, "\tREG_h150.reg_thresh_u8bit_min = 0x%x\n", p->REG_h150.reg_thresh_u8bit_min);
	fprintf(stderr, "\tREG_h150.reg_thresh_u8bit_mid = 0x%x\n", p->REG_h150.reg_thresh_u8bit_mid);
	fprintf(stderr, "\tREG_h150.reg_thresh_u8bit_max = 0x%x\n", p->REG_h150.reg_thresh_u8bit_max);
	fprintf(stderr, "\tREG_h160.reg_ncc_nemerator_l = 0x%x\n", p->REG_h160.reg_ncc_nemerator_l);
	fprintf(stderr, "\tREG_h164.reg_ncc_nemerator_m = 0x%x\n", p->REG_h164.reg_ncc_nemerator_m);
	fprintf(stderr, "\tREG_h168.reg_ncc_quadsum0_l = 0x%x\n", p->REG_h168.reg_ncc_quadsum0_l);
	fprintf(stderr, "\tREG_h16C.reg_ncc_quadsum0_m = 0x%x\n", p->REG_h16C.reg_ncc_quadsum0_m);
	fprintf(stderr, "\tREG_h170.reg_ncc_quadsum1_l = 0x%x\n", p->REG_h170.reg_ncc_quadsum1_l);
	fprintf(stderr, "\tREG_h174.reg_ncc_quadsum1_m = 0x%x\n", p->REG_h174.reg_ncc_quadsum1_m);
	fprintf(stderr, "\tREG_R2Y4_11.reg_csc_r2y4_tab_sw_0 = 0x%x\n", p->REG_R2Y4_11.reg_csc_r2y4_tab_sw_0);
	fprintf(stderr, "\tREG_R2Y4_11.reg_csc_r2y4_tab_sw_1 = 0x%x\n", p->REG_R2Y4_11.reg_csc_r2y4_tab_sw_1);
	fprintf(stderr, "\tREG_R2Y4_12.reg_csc_r2y4_tab_sw_update = 0x%x\n", p->REG_R2Y4_12.reg_csc_r2y4_tab_sw_update);
	fprintf(stderr, "\tREG_R2Y4_12.reg_csc_r2y4_coeff_sw_update = 0x%x\n", p->REG_R2Y4_12.reg_csc_r2y4_coeff_sw_update);
	fprintf(stderr, "\tREG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00 = 0x%x\n", p->REG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00);
	fprintf(stderr, "\tREG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01 = 0x%x\n", p->REG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01);
	fprintf(stderr, "\tREG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02 = 0x%x\n", p->REG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02);
	fprintf(stderr, "\tREG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03 = 0x%x\n", p->REG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03);
	fprintf(stderr, "\tREG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04 = 0x%x\n", p->REG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04);
	fprintf(stderr, "\tREG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05 = 0x%x\n", p->REG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05);
	fprintf(stderr, "\tREG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06 = 0x%x\n", p->REG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06);
	fprintf(stderr, "\tREG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07 = 0x%x\n", p->REG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07);
	fprintf(stderr, "\tREG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08 = 0x%x\n", p->REG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08);
	fprintf(stderr, "\tREG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09 = 0x%x\n", p->REG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09);
	fprintf(stderr, "\tREG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10 = 0x%x\n", p->REG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10);
	fprintf(stderr, "\tREG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11 = 0x%x\n", p->REG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11);
	fprintf(stderr, "\tREG_R2Y4_14.reg_csc_r2y4_enmode = 0x%x\n", p->REG_R2Y4_14.reg_csc_r2y4_enmode);
	fprintf(stderr, "\tREG_R2Y4_14.reg_csc_r2y4_enable = 0x%x\n", p->REG_R2Y4_14.reg_csc_r2y4_enable);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_TOP_C(X) \
	 IVE_TOP_C X = \
{\
	{	/* REG_0.reg_img_in_uv_swap = */0x0,\
	/*.REG_0.reg_img_1_uv_swap = */0x0,\
	/*.REG_0.reg_rdma_eigval_uv_swap = */0x0,\
	/*uint32_t rsv_3_3:1;=*/0,\
	/*.REG_0.reg_trig_cnt = */0x5,\
	},\
	{	/*.REG_1.reg_softrst = */0x0,\
	/*.REG_1.reg_shdw_sel = */0x1,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.REG_1.reg_fmt_vld_fg = */0x0,\
	/*.REG_1.reg_fmt_vld_ccl = */0x0,\
	/*.REG_1.reg_fmt_vld_dmaf = */0x0,\
	/*.REG_1.reg_fmt_vld_lk = */0x0,\
	/*.REG_1.reg_cmdq_tsk_trig = */0x0,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.REG_1.reg_cmdq_tsk_sel = */0x0,\
	/*.REG_1.reg_cmdq_tsk_en = */0x0,\
	/*.REG_1.reg_dma_abort = */0x0,\
	/*.REG_1.reg_wdma_abort_done = */0x0,\
	/*.REG_1.reg_rdma_abort_done = */0x0,\
	/*.REG_1.reg_img_in_axi_idle = */0x0,\
	/*.REG_1.reg_odma_axi_idle = */0x0,\
	},\
	{	/*.REG_2.reg_img_widthm1 = */0x77f,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.REG_2.reg_img_heightm1 = */0x437,\
	},\
	{	/*.REG_3.reg_imgmux_img0_sel = */0x1,\
	/*.REG_3.reg_mapmux_rdma_sel = */0x0,\
	/*.REG_3.reg_ive_rdma_img1_en = */0x0,\
	/*.REG_3.reg_ive_rdma_img1_mod_u8 = */0x1,\
	/*.REG_3.reg_ive_rdma_eigval_en = */0x0,\
	/*.REG_3.reg_muxsel_gradfg = */0x0,\
	/*.REG_3.reg_dma_share_mux_selgmm = */0x0,\
	},\
	{	/*.REG_h10.reg_img_in_top_enable = */0x1,\
	/*.REG_h10.reg_resize_top_enable = */0x1,\
	/*.REG_h10.reg_gmm_top_enable = */0x1,\
	/*.REG_h10.reg_csc_top_enable = */0x1,\
	/*.REG_h10.reg_rdma_img1_top_enable = */0x1,\
	/*.REG_h10.reg_bgm_top_enable = */0x1,\
	/*.REG_h10.reg_bgu_top_enable = */0x1,\
	/*.REG_h10.reg_r2y4_top_enable = */0x1,\
	/*.REG_h10.reg_map_top_enable = */0x1,\
	/*.REG_h10.reg_rdma_eigval_top_enable = */0x1,\
	/*.REG_h10.reg_thresh_top_enable = */0x1,\
	/*.REG_h10.reg_hist_top_enable = */0x1,\
	/*.REG_h10.reg_intg_top_enable = */0x1,\
	/*.REG_h10.reg_ncc_top_enable = */0x1,\
	/*.REG_h10.reg_sad_top_enable = */0x1,\
	/*.REG_h10.reg_filterop_top_enable = */0x1,\
	/*.REG_h10.reg_dmaf_top_enable = */0x1,\
	/*.REG_h10.reg_ccl_top_enable = */0x1,\
	/*.REG_h10.reg_lk_top_enable = */0x1,\
	},\
	{	/*.REG_11.reg_csc_tab_sw_0 = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.REG_11.reg_csc_tab_sw_1 = */0x0,\
	},\
	{	/*.REG_12.reg_csc_tab_sw_update = */0x0,\
	/*uint32_t rsv_1_15:15;=*/0,\
	/*.REG_12.reg_csc_coeff_sw_update = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_0.reg_csc_coeff_sw_00 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_1.reg_csc_coeff_sw_01 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_2.reg_csc_coeff_sw_02 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_3.reg_csc_coeff_sw_03 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_4.reg_csc_coeff_sw_04 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_5.reg_csc_coeff_sw_05 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_6.reg_csc_coeff_sw_06 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_7.reg_csc_coeff_sw_07 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_8.reg_csc_coeff_sw_08 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_9.reg_csc_coeff_sw_09 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_A.reg_csc_coeff_sw_10 = */0x0,\
	},\
	{	/*.REG_CSC_COEFF_B.reg_csc_coeff_sw_11 = */0x0,\
	},\
	{	/*.REG_14.reg_csc_enmode = */0x0,\
	/*.REG_14.reg_csc_enable = */0x0,\
	},\
	{	/*.REG_15.reg_lbp_u8bit_thr = */0x0,\
	/*.REG_15.reg_lbp_s8bit_thr = */0x0,\
	/*.REG_15.reg_lbp_enmode = */0x0,\
	},\
	{	/*.REG_h54.reg_ive_dma_idle = */0x0,\
	},\
	{	/*.REG_h58.reg_ive_gmm_dma_idle = */0x0,\
	},\
	{	/*.REG_16.reg_dbg_en = */0x1,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.REG_16.reg_dbg_sel = */0x6,\
	},\
	{	/*.REG_h64.reg_dbg_col = */0x0,\
	/*.REG_h64.reg_dbg_row = */0x0,\
	},\
	{	/*.REG_h68.reg_dbg_status = */0x0,\
	},\
	{	/*.REG_h6c.reg_dbg_pix = */0x0,\
	/*.REG_h6c.reg_dbg_line = */0x0,\
	},\
	{	/*.REG_h70.reg_dbg_data = */0x0,\
	},\
	{	/*.REG_h74.reg_dbg_perfmt = */0x0,\
	/*uint32_t rsv_1_15:15;=*/0,\
	/*.REG_h74.reg_dbg_fmt = */0x0,\
	},\
	{	/*.REG_20.reg_frame2op_op_mode = */0x0,\
	/*.REG_20.reg_frame2op_sub_mode = */0x0,\
	/*.REG_20.reg_frame2op_sub_change_order = */0x0,\
	/*.REG_20.reg_frame2op_add_mode_rounding = */0x0,\
	/*.REG_20.reg_frame2op_add_mode_clipping = */0x0,\
	/*.REG_20.reg_frame2op_sub_switch_src = */0x0,\
	},\
	{	/*.REG_21.reg_fram2op_x_u0q16 = */0x0,\
	/*.REG_21.reg_fram2op_y_u0q16 = */0x0,\
	},\
	{	/*.REG_h80.reg_frame2op_fg_op_mode = */0x0,\
	/*.REG_h80.reg_frame2op_fg_sub_mode = */0x0,\
	/*.REG_h80.reg_frame2op_fg_sub_change_order = */0x0,\
	/*.REG_h80.reg_frame2op_fg_add_mode_rounding = */0x0,\
	/*.REG_h80.reg_frame2op_fg_add_mode_clipping = */0x0,\
	/*.REG_h80.reg_frame2op_fg_sub_switch_src = */0x0,\
	},\
	{	/*.REG_84.reg_fram2op_fg_x_u0q16 = */0x0,\
	/*.REG_84.reg_fram2op_fg_y_u0q16 = */0x0,\
	},\
	{	/*.REG_90.reg_frame_done_img_in = */0x0,\
	/*.REG_90.reg_frame_done_rdma_img1 = */0x0,\
	/*.REG_90.reg_frame_done_rdma_eigval = */0x0,\
	/*.REG_90.reg_frame_done_resize = */0x0,\
	/*.REG_90.reg_frame_done_gmm = */0x0,\
	/*.REG_90.reg_frame_done_csc = */0x0,\
	/*.REG_90.reg_frame_done_hist = */0x0,\
	/*.REG_90.reg_frame_done_intg = */0x0,\
	/*.REG_90.reg_frame_done_sad = */0x0,\
	/*.REG_90.reg_frame_done_ncc = */0x0,\
	/*.REG_90.reg_frame_done_bgm = */0x0,\
	/*.REG_90.reg_frame_done_bgu = */0x0,\
	/*.REG_90.reg_frame_done_r2y4 = */0x0,\
	/*.REG_90.reg_frame_done_frame2op_bg = */0x0,\
	/*.REG_90.reg_frame_done_frame2op_fg = */0x0,\
	/*.REG_90.reg_frame_done_map = */0x0,\
	/*.REG_90.reg_frame_done_thresh16ro8 = */0x0,\
	/*.REG_90.reg_frame_done_thresh = */0x0,\
	/*.REG_90.reg_frame_done_filterop_odma = */0x0,\
	/*.REG_90.reg_frame_done_filterop_wdma_y = */0x0,\
	/*.REG_90.reg_frame_done_filterop_wdma_c = */0x0,\
	/*.REG_90.reg_frame_done_dmaf = */0x0,\
	/*.REG_90.reg_frame_done_ccl = */0x0,\
	/*.REG_90.reg_frame_done_lk = */0x0,\
	/*.REG_90.reg_frame_done_filterop_wdma_yc = */0x0,\
	},\
	{	/*.REG_94.reg_intr_en_hist = */0x0,\
	/*.REG_94.reg_intr_en_intg = */0x0,\
	/*.REG_94.reg_intr_en_sad = */0x0,\
	/*.REG_94.reg_intr_en_ncc = */0x0,\
	/*.REG_94.reg_intr_en_filterop_odma = */0x1,\
	/*.REG_94.reg_intr_en_filterop_wdma_y = */0x0,\
	/*.REG_94.reg_intr_en_filterop_wdma_c = */0x0,\
	/*.REG_94.reg_intr_en_dmaf = */0x0,\
	/*.REG_94.reg_intr_en_ccl = */0x0,\
	/*.REG_94.reg_intr_en_lk = */0x0,\
	/*.REG_94.reg_intr_en_filterop_wdma_yc = */0x0,\
	},\
	{	/*.REG_98.reg_intr_status_hist = */0x0,\
	/*.REG_98.reg_intr_status_intg = */0x0,\
	/*.REG_98.reg_intr_status_sad = */0x0,\
	/*.REG_98.reg_intr_status_ncc = */0x0,\
	/*.REG_98.reg_intr_status_filterop_odma = */0x0,\
	/*.REG_98.reg_intr_status_filterop_wdma_y = */0x0,\
	/*.REG_98.reg_intr_status_filterop_wdma_c = */0x0,\
	/*.REG_98.reg_intr_status_dmaf = */0x0,\
	/*.REG_98.reg_intr_status_ccl = */0x0,\
	/*.REG_98.reg_intr_status_lk = */0x0,\
	/*.REG_98.reg_intr_status_filterop_wdma_yc = */0x0,\
	},\
	{	/*.REG_RS_SRC_SIZE.reg_resize_src_wd = */0x77f,\
	/*.REG_RS_SRC_SIZE.reg_resize_src_ht = */0x437,\
	},\
	{	/*.REG_RS_DST_SIZE.reg_resize_dst_wd = */0x77f,\
	/*.REG_RS_DST_SIZE.reg_resize_dst_ht = */0x437,\
	},\
	{	/*.REG_RS_H_SC.reg_resize_h_sc_fac = */0x2000,\
	},\
	{	/*.REG_RS_V_SC.reg_resize_v_sc_fac = */0x2000,\
	},\
	{	/*.REG_RS_PH_INI.reg_resize_h_ini_ph = */0x0,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.REG_RS_PH_INI.reg_resize_v_ini_ph = */0x0,\
	},\
	{	/*.REG_RS_NOR.reg_resize_h_nor = */0x8000,\
	/*.REG_RS_NOR.reg_resize_v_nor = */0x8000,\
	},\
	{	/*.REG_RS_CTRL.reg_resize_ip_en = */0x0,\
	/*.REG_RS_CTRL.reg_resize_dbg_en = */0x0,\
	/*.REG_RS_CTRL.reg_resize_area_fast = */0x1,\
	/*.REG_RS_CTRL.reg_resize_blnr_mode = */0x0,\
	},\
	{	/*.REG_RS_dBG_H1.reg_resize_sc_dbg_h1 = */0,\
	},\
	{	/*.REG_RS_dBG_H2.reg_resize_sc_dbg_h2 = */0,\
	},\
	{	/*.REG_RS_dBG_V1.reg_resize_sc_dbg_v1 = */0,\
	},\
	{	/*.REG_RS_dBG_V2.reg_resize_sc_dbg_v2 = */0,\
	},\
	{	/*.REG_h130.reg_thresh_top_mod = */0x0,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.REG_h130.reg_thresh_thresh_en = */0x0,\
	/*uint32_t rsv_5_7:3;=*/0,\
	/*.REG_h130.reg_thresh_softrst = */0x0,\
	},\
	{	/*.REG_h134.reg_thresh_16to8_mod = */0x0,\
	/*uint32_t rsv_3_7:5;=*/0,\
	/*.REG_h134.reg_thresh_16to8_s8bias = */0x0,\
	/*.REG_h134.reg_thresh_16to8_u8Num_div_u16Den = */0x0,\
	},\
	{	/*.REG_h138.reg_thresh_st_16to8_en = */0x0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.REG_h138.reg_thresh_st_16to8_u8Numerator = */0x0,\
	/*.REG_h138.reg_thresh_st_16to8_maxeigval = */0x0,\
	},\
	{	/*.REG_h13c.reg_thresh_s16_enmode = */0x0,\
	/*uint32_t rsv_2_7:6;=*/0,\
	/*.REG_h13c.reg_thresh_s16_u8bit_min = */0x0,\
	/*.REG_h13c.reg_thresh_s16_u8bit_mid = */0x0,\
	/*.REG_h13c.reg_thresh_s16_u8bit_max = */0x0,\
	},\
	{	/*.REG_h140.reg_thresh_s16_bit_thr_l = */0x0,\
	/*.REG_h140.reg_thresh_s16_bit_thr_h = */0x0,\
	},\
	{	/*.REG_h144.reg_thresh_u16_enmode = */0x0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.REG_h144.reg_thresh_u16_u8bit_min = */0x0,\
	/*.REG_h144.reg_thresh_u16_u8bit_mid = */0x0,\
	/*.REG_h144.reg_thresh_u16_u8bit_max = */0x0,\
	},\
	{	/*.REG_h148.reg_thresh_u16_bit_thr_l = */0x0,\
	/*.REG_h148.reg_thresh_u16_bit_thr_h = */0x0,\
	},\
	{	/*.REG_h14c.reg_thresh_u8bit_thr_l = */0x0,\
	/*.REG_h14c.reg_thresh_u8bit_thr_h = */0x0,\
	/*.REG_h14c.reg_thresh_enmode = */0x0,\
	},\
	{	/*.REG_h150.reg_thresh_u8bit_min = */0x0,\
	/*.REG_h150.reg_thresh_u8bit_mid = */0x0,\
	/*.REG_h150.reg_thresh_u8bit_max = */0x0,\
	},\
	{	/*.REG_h160.reg_ncc_nemerator_l = */0x0,\
	},\
	{	/*.REG_h164.reg_ncc_nemerator_m = */0x0,\
	},\
	{	/*.REG_h168.reg_ncc_quadsum0_l = */0x0,\
	},\
	{	/*.REG_h16C.reg_ncc_quadsum0_m = */0x0,\
	},\
	{	/*.REG_h170.reg_ncc_quadsum1_l = */0x0,\
	},\
	{	/*.REG_h174.reg_ncc_quadsum1_m = */0x0,\
	},\
	{	/*.REG_R2Y4_11.reg_csc_r2y4_tab_sw_0 = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.REG_R2Y4_11.reg_csc_r2y4_tab_sw_1 = */0x0,\
	},\
	{	/*.REG_R2Y4_12.reg_csc_r2y4_tab_sw_update = */0x0,\
	/*uint32_t rsv_1_15:15;=*/0,\
	/*.REG_R2Y4_12.reg_csc_r2y4_coeff_sw_update = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10 = */0x0,\
	},\
	{	/*.REG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11 = */0x0,\
	},\
	{	/*.REG_R2Y4_14.reg_csc_r2y4_enmode = */0x8,\
	/*.REG_R2Y4_14.reg_csc_r2y4_enable = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_TOP_C \
{\
	.REG_0.reg_img_in_uv_swap = 0x0,\
	.REG_0.reg_img_1_uv_swap = 0x0,\
	.REG_0.reg_rdma_eigval_uv_swap = 0x0,\
	.REG_0.reg_trig_cnt = 0x5,\
	.REG_1.reg_softrst = 0x0,\
	.REG_1.reg_shdw_sel = 0x1,\
	.REG_1.reg_fmt_vld_fg = 0x0,\
	.REG_1.reg_fmt_vld_ccl = 0x0,\
	.REG_1.reg_fmt_vld_dmaf = 0x0,\
	.REG_1.reg_fmt_vld_lk = 0x0,\
	.REG_1.reg_cmdq_tsk_trig = 0x0,\
	.REG_1.reg_cmdq_tsk_sel = 0x0,\
	.REG_1.reg_cmdq_tsk_en = 0x0,\
	.REG_1.reg_dma_abort = 0x0,\
	.REG_1.reg_wdma_abort_done = 0x0,\
	.REG_1.reg_rdma_abort_done = 0x0,\
	.REG_1.reg_img_in_axi_idle = 0x0,\
	.REG_1.reg_odma_axi_idle = 0x0,\
	.REG_2.reg_img_widthm1 = 0x77f,\
	.REG_2.reg_img_heightm1 = 0x437,\
	.REG_3.reg_imgmux_img0_sel = 0x1,\
	.REG_3.reg_mapmux_rdma_sel = 0x0,\
	.REG_3.reg_ive_rdma_img1_en = 0x0,\
	.REG_3.reg_ive_rdma_img1_mod_u8 = 0x1,\
	.REG_3.reg_ive_rdma_eigval_en = 0x0,\
	.REG_3.reg_muxsel_gradfg = 0x0,\
	.REG_3.reg_dma_share_mux_selgmm = 0x0,\
	.REG_h10.reg_img_in_top_enable = 0x1,\
	.REG_h10.reg_resize_top_enable = 0x1,\
	.REG_h10.reg_gmm_top_enable = 0x1,\
	.REG_h10.reg_csc_top_enable = 0x1,\
	.REG_h10.reg_rdma_img1_top_enable = 0x1,\
	.REG_h10.reg_bgm_top_enable = 0x1,\
	.REG_h10.reg_bgu_top_enable = 0x1,\
	.REG_h10.reg_r2y4_top_enable = 0x1,\
	.REG_h10.reg_map_top_enable = 0x1,\
	.REG_h10.reg_rdma_eigval_top_enable = 0x1,\
	.REG_h10.reg_thresh_top_enable = 0x1,\
	.REG_h10.reg_hist_top_enable = 0x1,\
	.REG_h10.reg_intg_top_enable = 0x1,\
	.REG_h10.reg_ncc_top_enable = 0x1,\
	.REG_h10.reg_sad_top_enable = 0x1,\
	.REG_h10.reg_filterop_top_enable = 0x1,\
	.REG_h10.reg_dmaf_top_enable = 0x1,\
	.REG_h10.reg_ccl_top_enable = 0x1,\
	.REG_h10.reg_lk_top_enable = 0x1,\
	.REG_11.reg_csc_tab_sw_0 = 0x0,\
	.REG_11.reg_csc_tab_sw_1 = 0x0,\
	.REG_12.reg_csc_tab_sw_update = 0x0,\
	.REG_12.reg_csc_coeff_sw_update = 0x0,\
	.REG_CSC_COEFF_0.reg_csc_coeff_sw_00 = 0x0,\
	.REG_CSC_COEFF_1.reg_csc_coeff_sw_01 = 0x0,\
	.REG_CSC_COEFF_2.reg_csc_coeff_sw_02 = 0x0,\
	.REG_CSC_COEFF_3.reg_csc_coeff_sw_03 = 0x0,\
	.REG_CSC_COEFF_4.reg_csc_coeff_sw_04 = 0x0,\
	.REG_CSC_COEFF_5.reg_csc_coeff_sw_05 = 0x0,\
	.REG_CSC_COEFF_6.reg_csc_coeff_sw_06 = 0x0,\
	.REG_CSC_COEFF_7.reg_csc_coeff_sw_07 = 0x0,\
	.REG_CSC_COEFF_8.reg_csc_coeff_sw_08 = 0x0,\
	.REG_CSC_COEFF_9.reg_csc_coeff_sw_09 = 0x0,\
	.REG_CSC_COEFF_A.reg_csc_coeff_sw_10 = 0x0,\
	.REG_CSC_COEFF_B.reg_csc_coeff_sw_11 = 0x0,\
	.REG_14.reg_csc_enmode = 0x0,\
	.REG_14.reg_csc_enable = 0x0,\
	.REG_15.reg_lbp_u8bit_thr = 0x0,\
	.REG_15.reg_lbp_s8bit_thr = 0x0,\
	.REG_15.reg_lbp_enmode = 0x0,\
	.REG_h54.reg_ive_dma_idle = 0x0,\
	.REG_h58.reg_ive_gmm_dma_idle = 0x0,\
	.REG_16.reg_dbg_en = 0x1,\
	.REG_16.reg_dbg_sel = 0x6,\
	.REG_h64.reg_dbg_col = 0x0,\
	.REG_h64.reg_dbg_row = 0x0,\
	.REG_h68.reg_dbg_status = 0x0,\
	.REG_h6c.reg_dbg_pix = 0x0,\
	.REG_h6c.reg_dbg_line = 0x0,\
	.REG_h70.reg_dbg_data = 0x0,\
	.REG_h74.reg_dbg_perfmt = 0x0,\
	.REG_h74.reg_dbg_fmt = 0x0,\
	.REG_20.reg_frame2op_op_mode = 0x0,\
	.REG_20.reg_frame2op_sub_mode = 0x0,\
	.REG_20.reg_frame2op_sub_change_order = 0x0,\
	.REG_20.reg_frame2op_add_mode_rounding = 0x0,\
	.REG_20.reg_frame2op_add_mode_clipping = 0x0,\
	.REG_20.reg_frame2op_sub_switch_src = 0x0,\
	.REG_21.reg_fram2op_x_u0q16 = 0x0,\
	.REG_21.reg_fram2op_y_u0q16 = 0x0,\
	.REG_h80.reg_frame2op_fg_op_mode = 0x0,\
	.REG_h80.reg_frame2op_fg_sub_mode = 0x0,\
	.REG_h80.reg_frame2op_fg_sub_change_order = 0x0,\
	.REG_h80.reg_frame2op_fg_add_mode_rounding = 0x0,\
	.REG_h80.reg_frame2op_fg_add_mode_clipping = 0x0,\
	.REG_h80.reg_frame2op_fg_sub_switch_src = 0x0,\
	.REG_84.reg_fram2op_fg_x_u0q16 = 0x0,\
	.REG_84.reg_fram2op_fg_y_u0q16 = 0x0,\
	.REG_90.reg_frame_done_img_in = 0x0,\
	.REG_90.reg_frame_done_rdma_img1 = 0x0,\
	.REG_90.reg_frame_done_rdma_eigval = 0x0,\
	.REG_90.reg_frame_done_resize = 0x0,\
	.REG_90.reg_frame_done_gmm = 0x0,\
	.REG_90.reg_frame_done_csc = 0x0,\
	.REG_90.reg_frame_done_hist = 0x0,\
	.REG_90.reg_frame_done_intg = 0x0,\
	.REG_90.reg_frame_done_sad = 0x0,\
	.REG_90.reg_frame_done_ncc = 0x0,\
	.REG_90.reg_frame_done_bgm = 0x0,\
	.REG_90.reg_frame_done_bgu = 0x0,\
	.REG_90.reg_frame_done_r2y4 = 0x0,\
	.REG_90.reg_frame_done_frame2op_bg = 0x0,\
	.REG_90.reg_frame_done_frame2op_fg = 0x0,\
	.REG_90.reg_frame_done_map = 0x0,\
	.REG_90.reg_frame_done_thresh16ro8 = 0x0,\
	.REG_90.reg_frame_done_thresh = 0x0,\
	.REG_90.reg_frame_done_filterop_odma = 0x0,\
	.REG_90.reg_frame_done_filterop_wdma_y = 0x0,\
	.REG_90.reg_frame_done_filterop_wdma_c = 0x0,\
	.REG_90.reg_frame_done_dmaf = 0x0,\
	.REG_90.reg_frame_done_ccl = 0x0,\
	.REG_90.reg_frame_done_lk = 0x0,\
	.REG_90.reg_frame_done_filterop_wdma_yc = 0x0,\
	.REG_94.reg_intr_en_hist = 0x0,\
	.REG_94.reg_intr_en_intg = 0x0,\
	.REG_94.reg_intr_en_sad = 0x0,\
	.REG_94.reg_intr_en_ncc = 0x0,\
	.REG_94.reg_intr_en_filterop_odma = 0x1,\
	.REG_94.reg_intr_en_filterop_wdma_y = 0x0,\
	.REG_94.reg_intr_en_filterop_wdma_c = 0x0,\
	.REG_94.reg_intr_en_dmaf = 0x0,\
	.REG_94.reg_intr_en_ccl = 0x0,\
	.REG_94.reg_intr_en_lk = 0x0,\
	.REG_94.reg_intr_en_filterop_wdma_yc = 0x0,\
	.REG_98.reg_intr_status_hist = 0x0,\
	.REG_98.reg_intr_status_intg = 0x0,\
	.REG_98.reg_intr_status_sad = 0x0,\
	.REG_98.reg_intr_status_ncc = 0x0,\
	.REG_98.reg_intr_status_filterop_odma = 0x0,\
	.REG_98.reg_intr_status_filterop_wdma_y = 0x0,\
	.REG_98.reg_intr_status_filterop_wdma_c = 0x0,\
	.REG_98.reg_intr_status_dmaf = 0x0,\
	.REG_98.reg_intr_status_ccl = 0x0,\
	.REG_98.reg_intr_status_lk = 0x0,\
	.REG_98.reg_intr_status_filterop_wdma_yc = 0x0,\
	.REG_RS_SRC_SIZE.reg_resize_src_wd = 0x77f,\
	.REG_RS_SRC_SIZE.reg_resize_src_ht = 0x437,\
	.REG_RS_DST_SIZE.reg_resize_dst_wd = 0x77f,\
	.REG_RS_DST_SIZE.reg_resize_dst_ht = 0x437,\
	.REG_RS_H_SC.reg_resize_h_sc_fac = 0x2000,\
	.REG_RS_V_SC.reg_resize_v_sc_fac = 0x2000,\
	.REG_RS_PH_INI.reg_resize_h_ini_ph = 0x0,\
	.REG_RS_PH_INI.reg_resize_v_ini_ph = 0x0,\
	.REG_RS_NOR.reg_resize_h_nor = 0x8000,\
	.REG_RS_NOR.reg_resize_v_nor = 0x8000,\
	.REG_RS_CTRL.reg_resize_ip_en = 0x0,\
	.REG_RS_CTRL.reg_resize_dbg_en = 0x0,\
	.REG_RS_CTRL.reg_resize_area_fast = 0x1,\
	.REG_RS_CTRL.reg_resize_blnr_mode = 0x0,\
	.REG_h130.reg_thresh_top_mod = 0x0,\
	.REG_h130.reg_thresh_thresh_en = 0x0,\
	.REG_h130.reg_thresh_softrst = 0x0,\
	.REG_h134.reg_thresh_16to8_mod = 0x0,\
	.REG_h134.reg_thresh_16to8_s8bias = 0x0,\
	.REG_h134.reg_thresh_16to8_u8Num_div_u16Den = 0x0,\
	.REG_h138.reg_thresh_st_16to8_en = 0x0,\
	.REG_h138.reg_thresh_st_16to8_u8Numerator = 0x0,\
	.REG_h138.reg_thresh_st_16to8_maxeigval = 0x0,\
	.REG_h13c.reg_thresh_s16_enmode = 0x0,\
	.REG_h13c.reg_thresh_s16_u8bit_min = 0x0,\
	.REG_h13c.reg_thresh_s16_u8bit_mid = 0x0,\
	.REG_h13c.reg_thresh_s16_u8bit_max = 0x0,\
	.REG_h140.reg_thresh_s16_bit_thr_l = 0x0,\
	.REG_h140.reg_thresh_s16_bit_thr_h = 0x0,\
	.REG_h144.reg_thresh_u16_enmode = 0x0,\
	.REG_h144.reg_thresh_u16_u8bit_min = 0x0,\
	.REG_h144.reg_thresh_u16_u8bit_mid = 0x0,\
	.REG_h144.reg_thresh_u16_u8bit_max = 0x0,\
	.REG_h148.reg_thresh_u16_bit_thr_l = 0x0,\
	.REG_h148.reg_thresh_u16_bit_thr_h = 0x0,\
	.REG_h14c.reg_thresh_u8bit_thr_l = 0x0,\
	.REG_h14c.reg_thresh_u8bit_thr_h = 0x0,\
	.REG_h14c.reg_thresh_enmode = 0x0,\
	.REG_h150.reg_thresh_u8bit_min = 0x0,\
	.REG_h150.reg_thresh_u8bit_mid = 0x0,\
	.REG_h150.reg_thresh_u8bit_max = 0x0,\
	.REG_h160.reg_ncc_nemerator_l = 0x0,\
	.REG_h164.reg_ncc_nemerator_m = 0x0,\
	.REG_h168.reg_ncc_quadsum0_l = 0x0,\
	.REG_h16C.reg_ncc_quadsum0_m = 0x0,\
	.REG_h170.reg_ncc_quadsum1_l = 0x0,\
	.REG_h174.reg_ncc_quadsum1_m = 0x0,\
	.REG_R2Y4_11.reg_csc_r2y4_tab_sw_0 = 0x0,\
	.REG_R2Y4_11.reg_csc_r2y4_tab_sw_1 = 0x0,\
	.REG_R2Y4_12.reg_csc_r2y4_tab_sw_update = 0x0,\
	.REG_R2Y4_12.reg_csc_r2y4_coeff_sw_update = 0x0,\
	.REG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00 = 0x0,\
	.REG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01 = 0x0,\
	.REG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02 = 0x0,\
	.REG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03 = 0x0,\
	.REG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04 = 0x0,\
	.REG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05 = 0x0,\
	.REG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06 = 0x0,\
	.REG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07 = 0x0,\
	.REG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08 = 0x0,\
	.REG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09 = 0x0,\
	.REG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10 = 0x0,\
	.REG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11 = 0x0,\
	.REG_R2Y4_14.reg_csc_r2y4_enmode = 0x8,\
	.REG_R2Y4_14.reg_csc_r2y4_enable = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_TOP_STRUCT_H__

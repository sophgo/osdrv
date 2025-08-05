/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_csi_ctrl_top.h
 * Description:HW register description
 */

#ifndef _REG_CSI_CTRL_TOP_H_
#define _REG_CSI_CTRL_TOP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union reg_csi_ctrl_top_00 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_mode                   : 3;
		uint32_t csi_ignore_ecc                  : 1;
		uint32_t csi_vc_check                    : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_vc_set                      : 4;
		uint32_t csi_line_start_sent             : 1;
		uint32_t csi_format_frc                  : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t csi_format_set                  : 8;
	} bits;
};

union reg_csi_ctrl_top_04 {
	uint32_t raw;
	struct {
		uint32_t csi_intr_mask                   : 8;
		uint32_t csi_intr_clr                    : 8;
		uint32_t csi_hdr_en                      : 1;
		uint32_t csi_hdr_mode                    : 1;
		uint32_t csi_id_rm_else                  : 1;
		uint32_t csi_id_rm_ob                    : 1;
	} bits;
};

union reg_csi_ctrl_top_08 {
	uint32_t raw;
	struct {
		uint32_t csi_n0_ob_lef                   : 16;
		uint32_t csi_n0_ob_sef                   : 16;
	} bits;
};

union reg_csi_ctrl_top_0c {
	uint32_t raw;
	struct {
		uint32_t csi_n0_lef                      : 16;
		uint32_t csi_n1_ob_lef                   : 16;
	} bits;
};

union reg_csi_ctrl_top_10 {
	uint32_t raw;
	struct {
		uint32_t csi_n1_ob_sef                   : 16;
		uint32_t csi_n1_lef                      : 16;
	} bits;
};

union reg_csi_ctrl_top_14 {
	uint32_t raw;
	struct {
		uint32_t csi_blc_dt                      : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t csi_blc_en                      : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t csi_blc_format_set              : 3;
	} bits;
};

union reg_csi_ctrl_top_18 {
	uint32_t raw;
	struct {
		uint32_t csi_vc_map_ch00                 : 4;
		uint32_t csi_vc_map_ch01                 : 4;
		uint32_t csi_vc_map_ch10                 : 4;
		uint32_t csi_vc_map_ch11                 : 4;
	} bits;
};

union reg_csi_ctrl_top_1c {
	uint32_t raw;
	struct {
		uint32_t csi_n0_sef                      : 16;
		uint32_t csi_n1_sef                      : 16;
	} bits;
};

union reg_csi_ctrl_top_20 {
	uint32_t raw;
	struct {
		uint32_t csi_n0_sef2                     : 16;
		uint32_t csi_n1_sef2                     : 16;
	} bits;
};

union reg_csi_ctrl_top_24 {
	uint32_t raw;
	struct {
		uint32_t csi_n0_ob_sef2                  : 16;
		uint32_t csi_n1_ob_sef2                  : 16;
	} bits;
};

union reg_csi_ctrl_top_30 {
	uint32_t raw;
	struct {
		uint32_t csi_ecc_ph_dbg                  : 32;
	} bits;
};

union reg_csi_ctrl_top_34 {
	uint32_t raw;
	struct {
		uint32_t csi_ecc_clr_ph_dbg              : 1;
	} bits;
};

union reg_csi_ctrl_top_40 {
	uint32_t raw;
	struct {
		uint32_t csi_ecc_no_error                : 1;
		uint32_t csi_ecc_corrected_error         : 1;
		uint32_t csi_ecc_error                   : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t csi_crc_error                   : 1;
		uint32_t csi_wc_error                    : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t csi_fifo_full                   : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t csi_decode_format               : 6;
	} bits;
};

union reg_csi_ctrl_top_48 {
	uint32_t raw;
	struct {
		uint32_t short_pkt_rsv1                  : 16;
		uint32_t short_pkt_rsv2                  : 16;
	} bits;
};

union reg_csi_ctrl_top_4c {
	uint32_t raw;
	struct {
		uint32_t short_pkt_rsv3                  : 16;
		uint32_t short_pkt_rsv4                  : 16;
	} bits;
};

union reg_csi_ctrl_top_50 {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt1              : 16;
		uint32_t generic_short_pkt2              : 16;
	} bits;
};

union reg_csi_ctrl_top_54 {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt3              : 16;
		uint32_t generic_short_pkt4              : 16;
	} bits;
};

union reg_csi_ctrl_top_58 {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt5              : 16;
		uint32_t generic_short_pkt6              : 16;
	} bits;
};

union reg_csi_ctrl_top_5c {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt7              : 16;
		uint32_t generic_short_pkt8              : 16;
	} bits;
};

union reg_csi_ctrl_top_60 {
	uint32_t raw;
	struct {
		uint32_t csi_intr_status                 : 8;
	} bits;
};

union reg_csi_ctrl_top_64 {
	uint32_t raw;
	struct {
		uint32_t csi_dbg_sel                     : 8;
	} bits;
};

union reg_csi_ctrl_top_70 {
	uint32_t raw;
	struct {
		uint32_t csi_vs_gen_mode                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t csi_vs_gen_by_vcset             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_vs_delay_sel                : 2;
		uint32_t csi_hs_delay_sel                : 2;
	} bits;
};

union reg_csi_ctrl_top_74 {
	uint32_t raw;
	struct {
		uint32_t csi_hdr_dt_mode                 : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t csi_hdr_dt_format               : 6;
		uint32_t _rsv_10                         : 2;
		uint32_t csi_hdr_dt_lef                  : 6;
		uint32_t _rsv_18                         : 2;
		uint32_t csi_hdr_dt_sef                  : 6;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_csi_ctrl_top_t {
	union reg_csi_ctrl_top_00               reg_00;
	union reg_csi_ctrl_top_04               reg_04;
	union reg_csi_ctrl_top_08               reg_08;
	union reg_csi_ctrl_top_0c               reg_0c;
	union reg_csi_ctrl_top_10               reg_10;
	union reg_csi_ctrl_top_14               reg_14;
	union reg_csi_ctrl_top_18               reg_18;
	union reg_csi_ctrl_top_1c               reg_1c;
	union reg_csi_ctrl_top_20               reg_20;
	union reg_csi_ctrl_top_24               reg_24;
	uint32_t                                _resv_0x28[2];
	union reg_csi_ctrl_top_30               reg_30;
	union reg_csi_ctrl_top_34               reg_34;
	uint32_t                                _resv_0x38[2];
	union reg_csi_ctrl_top_40               reg_40;
	uint32_t                                _resv_0x44[1];
	union reg_csi_ctrl_top_48               reg_48;
	union reg_csi_ctrl_top_4c               reg_4c;
	union reg_csi_ctrl_top_50               reg_50;
	union reg_csi_ctrl_top_54               reg_54;
	union reg_csi_ctrl_top_58               reg_58;
	union reg_csi_ctrl_top_5c               reg_5c;
	union reg_csi_ctrl_top_60               reg_60;
	union reg_csi_ctrl_top_64               reg_64;
	uint32_t                                _resv_0x68[2];
	union reg_csi_ctrl_top_70               reg_70;
	union reg_csi_ctrl_top_74               reg_74;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_CSI_CTRL_TOP_H_ */

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_sublvds_ctrl_top.h
 * Description:HW register description
 */

#ifndef _REG_SUBLVDS_CTRL_TOP_H_
#define _REG_SUBLVDS_CTRL_TOP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union reg_sublvds_ctrl_top_00 {
	uint32_t raw;
	struct {
		uint32_t slvds_enable                    : 8;
		uint32_t slvds_bit_mode                  : 2;
		uint32_t slvds_data_reverse              : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t slvds_hdr_mode                  : 1;
		uint32_t slvds_hdr_pattern               : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_vfporch                   : 10;
	} bits;
};

union reg_sublvds_ctrl_top_04 {
	uint32_t raw;
	struct {
		uint32_t slvds_sync_1st                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_sync_2nd                  : 12;
	} bits;
};

union reg_sublvds_ctrl_top_08 {
	uint32_t raw;
	struct {
		uint32_t slvds_sync_3rd                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_norm_bk_sav               : 12;
	} bits;
};

union reg_sublvds_ctrl_top_0c {
	uint32_t raw;
	struct {
		uint32_t slvds_norm_bk_eav               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_norm_sav                  : 12;
	} bits;
};

union reg_sublvds_ctrl_top_10 {
	uint32_t raw;
	struct {
		uint32_t slvds_norm_eav                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_bk_sav                 : 12;
	} bits;
};

union reg_sublvds_ctrl_top_14 {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_bk_eav                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_bk_sav                 : 12;
	} bits;
};

union reg_sublvds_ctrl_top_18 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_bk_eav                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_lef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_1c {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_lef_eav                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_sef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_20 {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_sef_eav                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_lef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_24 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_lef_eav                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_sef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_28 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_sef_eav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_2c {
	uint32_t raw;
	struct {
		uint32_t vs_gen_sync_code                : 12;
		uint32_t vs_gen_by_sync_code             : 1;
	} bits;
};

union reg_sublvds_ctrl_top_30 {
	uint32_t raw;
	struct {
		uint32_t slvds_lane_mode                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t slvds_sync_source               : 8;
		uint32_t slvds_fifo_clr                  : 1;
	} bits;
};

union reg_sublvds_ctrl_top_40 {
	uint32_t raw;
	struct {
		uint32_t slvds_fifo_full                 : 1;
	} bits;
};

union reg_sublvds_ctrl_top_50 {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_lsef_sav               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_lsef_eav               : 12;
	} bits;
};

union reg_sublvds_ctrl_top_54 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_lsef_sav               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_lsef_eav               : 12;
	} bits;
};

union reg_sublvds_ctrl_top_58 {
	uint32_t raw;
	struct {
		uint32_t slvds_hdr_p2_hsize              : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_hdr_p2_hblank             : 14;
	} bits;
};

union reg_sublvds_ctrl_top_60 {
	uint32_t raw;
	struct {
		uint32_t hispi_mode                      : 1;
		uint32_t hispi_use_hsize                 : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t hispi_hdr_psp_mode              : 1;
	} bits;
};

union reg_sublvds_ctrl_top_64 {
	uint32_t raw;
	struct {
		uint32_t hispi_norm_sof                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_norm_eof                  : 12;
	} bits;
};

union reg_sublvds_ctrl_top_68 {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t1_sof                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t1_eof                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_6c {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t1_sol                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t1_eol                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_70 {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t2_sof                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t2_eof                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_74 {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t2_sol                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t2_eol                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_80 {
	uint32_t raw;
	struct {
		uint32_t dbg_sel                         : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_sublvds_ctrl_top_t {
	union reg_sublvds_ctrl_top_00           reg_00;
	union reg_sublvds_ctrl_top_04           reg_04;
	union reg_sublvds_ctrl_top_08           reg_08;
	union reg_sublvds_ctrl_top_0c           reg_0c;
	union reg_sublvds_ctrl_top_10           reg_10;
	union reg_sublvds_ctrl_top_14           reg_14;
	union reg_sublvds_ctrl_top_18           reg_18;
	union reg_sublvds_ctrl_top_1c           reg_1c;
	union reg_sublvds_ctrl_top_20           reg_20;
	union reg_sublvds_ctrl_top_24           reg_24;
	union reg_sublvds_ctrl_top_28           reg_28;
	union reg_sublvds_ctrl_top_2c           reg_2c;
	union reg_sublvds_ctrl_top_30           reg_30;
	uint32_t                                _resv_0x34[3];
	union reg_sublvds_ctrl_top_40           reg_40;
	uint32_t                                _resv_0x44[3];
	union reg_sublvds_ctrl_top_50           reg_50;
	union reg_sublvds_ctrl_top_54           reg_54;
	union reg_sublvds_ctrl_top_58           reg_58;
	uint32_t                                _resv_0x5c[1];
	union reg_sublvds_ctrl_top_60           reg_60;
	union reg_sublvds_ctrl_top_64           reg_64;
	union reg_sublvds_ctrl_top_68           reg_68;
	union reg_sublvds_ctrl_top_6c           reg_6c;
	union reg_sublvds_ctrl_top_70           reg_70;
	union reg_sublvds_ctrl_top_74           reg_74;
	uint32_t                                _resv_0x78[2];
	union reg_sublvds_ctrl_top_80           reg_80;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_SUBLVDS_CTRL_TOP_H_ */

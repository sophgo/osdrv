/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_phy_2l.h
 * Description:HW register description
 */

#ifndef _REG_PHY_2L_H_
#define _REG_PHY_2L_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union reg_sensor_phy_2l_00 {
	uint32_t raw;
	struct {
		uint32_t sensor_mode                     : 2;
	} bits;
};

union reg_sensor_phy_2l_04 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_d0_sel                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t csi_lane_d1_sel                 : 2;
	} bits;
};

union reg_sensor_phy_2l_08 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_ck_sel                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t csi_lane_ck_pnswap              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_lane_d0_pnswap              : 1;
		uint32_t csi_lane_d1_pnswap              : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t csi_ck_phase                    : 8;
	} bits;
};

union reg_sensor_phy_2l_0c {
	uint32_t raw;
	struct {
		uint32_t deskew_lane_en                  : 2;
		uint32_t _rsv_2                          : 14;
		uint32_t prbs9_test_period               : 16;
	} bits;
};

union reg_sensor_phy_2l_10 {
	uint32_t raw;
	struct {
		uint32_t t_hs_settle                     : 8;
		uint32_t t_all_zero                      : 8;
		uint32_t auto_ignore                     : 1;
		uint32_t auto_sync                       : 1;
	} bits;
};

union reg_sensor_phy_2l_20 {
	uint32_t raw;
	struct {
		uint32_t slvds_inv_en                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t slvds_bit_mode                  : 2;
		uint32_t slvds_lane_en                   : 2;
		uint32_t _rsv_6                          : 6;
		uint32_t slvds_force_resync              : 1;
		uint32_t slvds_resync                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_sav_1st                   : 12;
	} bits;
};

union reg_sensor_phy_2l_24 {
	uint32_t raw;
	struct {
		uint32_t slvds_sav_2nd                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_sav_3rd                   : 12;
	} bits;
};

union reg_sensor_phy_2l_28 {
	uint32_t raw;
	struct {
		uint32_t slvds_d0_sync_state             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t slvds_d1_sync_state             : 2;
	} bits;
};

union reg_sensor_phy_2l_30 {
	uint32_t raw;
	struct {
		uint32_t slvsec_lane_en                  : 2;
		uint32_t _rsv_2                          : 6;
		uint32_t slvsec_skew_cnt_en              : 1;
		uint32_t slvsec_train_seq_chk_en         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t slvsec_skew_cons                : 5;
		uint32_t slvsec_force_resync             : 1;
		uint32_t slvsec_resync                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t slvsec_unstable_skew_cnt        : 8;
	} bits;
};

union reg_sensor_phy_2l_34 {
	uint32_t raw;
	struct {
		uint32_t slvsec_sync_symbol              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t slvsec_standby_symbol           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t slvsec_deskew_symbol            : 9;
	} bits;
};

union reg_sensor_phy_2l_38 {
	uint32_t raw;
	struct {
		uint32_t slvsec_prbs9_test_period        : 16;
	} bits;
};

union reg_sensor_phy_2l_3c {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_clr             : 16;
	} bits;
};

union reg_sensor_phy_2l_40 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_mask            : 16;
	} bits;
};

union reg_sensor_phy_2l_44 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_status          : 16;
	} bits;
};

union reg_sensor_phy_2l_48 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_en           : 1;
		uint32_t slvsec_d0_clr_test_pat_err      : 1;
		uint32_t slvsec_d0_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_2l_4c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_err_cnt      : 16;
		uint32_t slvsec_d0_test_pat_err          : 1;
		uint32_t slvsec_d0_test_pat_pass         : 1;
		uint32_t slvsec_d0_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d0_start_code_err        : 1;
		uint32_t slvsec_d0_end_code_err          : 1;
		uint32_t slvsec_d0_deskew_code_err       : 1;
		uint32_t slvsec_d0_standby_code_err      : 1;
		uint32_t slvsec_d0_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_2l_50 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_en           : 1;
		uint32_t slvsec_d1_clr_test_pat_err      : 1;
		uint32_t slvsec_d1_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_2l_54 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_err_cnt      : 16;
		uint32_t slvsec_d1_test_pat_err          : 1;
		uint32_t slvsec_d1_test_pat_pass         : 1;
		uint32_t slvsec_d1_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d1_start_code_err        : 1;
		uint32_t slvsec_d1_end_code_err          : 1;
		uint32_t slvsec_d1_deskew_code_err       : 1;
		uint32_t slvsec_d1_standby_code_err      : 1;
		uint32_t slvsec_d1_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_2l_dbg_90 {
	uint32_t raw;
	struct {
		uint32_t ck_hs_state                     : 1;
		uint32_t ck_ulps_state                   : 1;
		uint32_t ck_stopstate                    : 1;
		uint32_t ck_err_state                    : 1;
		uint32_t deskew_state                    : 2;
	} bits;
};

union reg_sensor_phy_2l_dbg_94 {
	uint32_t raw;
	struct {
		uint32_t d0_datahs_state                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t d1_datahs_state                 : 3;
	} bits;
};

union reg_sensor_phy_2l_status_98 {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_clr                : 8;
		uint32_t d0_lp_status_clr                : 8;
		uint32_t d1_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_2l_status_9c {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_out                : 8;
		uint32_t d0_lp_status_out                : 8;
		uint32_t d1_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_2l_d0_0 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_en                     : 1;
		uint32_t d0_prbs9_clr_err                : 1;
		uint32_t d0_prbs9_source                 : 1;
		uint32_t d0_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d0_calib_max                    : 8;
		uint32_t d0_calib_step                   : 8;
		uint32_t d0_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_2l_d0_1 {
	uint32_t raw;
	struct {
		uint32_t d0_calib_en                     : 1;
		uint32_t d0_calib_source                 : 1;
		uint32_t d0_calib_mode                   : 1;
		uint32_t d0_calib_ignore                 : 1;
		uint32_t d0_calib_settle                 : 3;
		uint32_t d0_calib_phase_no_shift         : 1;
		uint32_t d0_calib_set_phase              : 8;
		uint32_t d0_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_2l_d0_2 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_rx_err                 : 1;
		uint32_t d0_prbs9_test_done              : 1;
		uint32_t d0_prbs9_test_pass              : 1;
		uint32_t d0_skew_calib_done              : 1;
		uint32_t d0_skew_calib_fail              : 1;
		uint32_t d0_datalp_state                 : 4;
		uint32_t d0_datalp_lpreq2err             : 1;
		uint32_t d0_datalp_dataesc2err           : 1;
		uint32_t d0_datalp_rsttri2err            : 1;
		uint32_t d0_datalp_hstest2err            : 1;
		uint32_t d0_datalp_esculp2err            : 1;
		uint32_t d0_datalp_hs2err                : 1;
		uint32_t d0_data_exist_1st_byte          : 1;
		uint32_t d0_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_2l_d0_3 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_4 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_5 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_6 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_7 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_8 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_9 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_a {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_b {
	uint32_t raw;
	struct {
		uint32_t d0_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d0_calib_threshold              : 8;
		uint32_t d0_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_2l_d1_0 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_en                     : 1;
		uint32_t d1_prbs9_clr_err                : 1;
		uint32_t d1_prbs9_source                 : 1;
		uint32_t d1_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d1_calib_max                    : 8;
		uint32_t d1_calib_step                   : 8;
		uint32_t d1_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_2l_d1_1 {
	uint32_t raw;
	struct {
		uint32_t d1_calib_en                     : 1;
		uint32_t d1_calib_source                 : 1;
		uint32_t d1_calib_mode                   : 1;
		uint32_t d1_calib_ignore                 : 1;
		uint32_t d1_calib_settle                 : 3;
		uint32_t d1_calib_phase_no_shift         : 1;
		uint32_t d1_calib_set_phase              : 8;
		uint32_t d1_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_2l_d1_2 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_rx_err                 : 1;
		uint32_t d1_prbs9_test_done              : 1;
		uint32_t d1_prbs9_test_pass              : 1;
		uint32_t d1_skew_calib_done              : 1;
		uint32_t d1_skew_calib_fail              : 1;
		uint32_t d1_datalp_state                 : 4;
		uint32_t d1_datalp_lpreq2err             : 1;
		uint32_t d1_datalp_dataesc2err           : 1;
		uint32_t d1_datalp_rsttri2err            : 1;
		uint32_t d1_datalp_hstest2err            : 1;
		uint32_t d1_datalp_esculp2err            : 1;
		uint32_t d1_datalp_hs2err                : 1;
		uint32_t d1_data_exist_1st_byte          : 1;
		uint32_t d1_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_2l_d1_3 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_4 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_5 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_6 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_7 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_8 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_9 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_a {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_b {
	uint32_t raw;
	struct {
		uint32_t d1_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d1_calib_threshold              : 8;
		uint32_t d1_calib_gp_count               : 9;
	} bits;
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

#endif /* _REG_PHY_2L_H_ */

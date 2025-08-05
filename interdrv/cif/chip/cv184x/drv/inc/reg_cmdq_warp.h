/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name:reg_cmdq_warp.h
 * Description:HW register description
 */

#ifndef _REG_CMDQ_WARP_H_
#define _REG_CMDQ_WARP_H_

#ifdef __cplusplus
extern "C" {
#endif


/******************************************/
/*          CMDSET Common Define          */
/******************************************/

struct cifcq_adma_desc_t {
	union {
		uint64_t    cmdset_addr;
		uint64_t    link_addr;
	};
	uint32_t    cmdset_size;
	union {
		uint32_t _rsv0;
		struct {
			uint32_t end    : 1;
			uint32_t _rsv1  : 1;
			uint32_t _rsv2  : 1;
			uint32_t link   : 1;
		} flag;
	};
};

union cmdset_field {
	uint32_t raw;
	struct {
		uint32_t reg_addr           : 20;
		uint32_t bwr_mask           : 4;
		uint32_t flag_end           : 1;
		uint32_t flag_int           : 1;
		uint32_t flag_last          : 1;
		uint32_t flag_rsv           : 1;
		uint32_t act                : 4;
	} bits;
};

struct vreg_resv {
	uint32_t                        resv;
	union cmdset_field              nop;
};


/******************************************/
/*           module definition            */
/******************************************/
struct vreg_sensor_mac_00 {
	union reg_sensor_mac_00                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_10 {
	union reg_sensor_mac_10                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_14 {
	union reg_sensor_mac_14                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_18 {
	union reg_sensor_mac_18                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_1c {
	union reg_sensor_mac_1c                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_20 {
	union reg_sensor_mac_20                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_24 {
	union reg_sensor_mac_24                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_28 {
	union reg_sensor_mac_28                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_30 {
	union reg_sensor_mac_30                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_34 {
	union reg_sensor_mac_34                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_40 {
	union reg_sensor_mac_40                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_44 {
	union reg_sensor_mac_44                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_48 {
	union reg_sensor_mac_48                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_4c {
	union reg_sensor_mac_4c                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_50 {
	union reg_sensor_mac_50                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_54 {
	union reg_sensor_mac_54                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_58 {
	union reg_sensor_mac_58                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_60 {
	union reg_sensor_mac_60                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_64 {
	union reg_sensor_mac_64                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_68 {
	union reg_sensor_mac_68                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_6c {
	union reg_sensor_mac_6c                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_70 {
	union reg_sensor_mac_70                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_74 {
	union reg_sensor_mac_74                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_80 {
	union reg_sensor_mac_80                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_84 {
	union reg_sensor_mac_84                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_88 {
	union reg_sensor_mac_88                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_8c {
	union reg_sensor_mac_8c                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_90 {
	union reg_sensor_mac_90                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_94 {
	union reg_sensor_mac_94                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_98 {
	union reg_sensor_mac_98                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_9c {
	union reg_sensor_mac_9c                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_a0 {
	union reg_sensor_mac_a0                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_a4 {
	union reg_sensor_mac_a4                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_a8 {
	union reg_sensor_mac_a8                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_b0 {
	union reg_sensor_mac_b0                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_b4 {
	union reg_sensor_mac_b4                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_b8 {
	union reg_sensor_mac_b8                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_bc {
	union reg_sensor_mac_bc                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_c0 {
	union reg_sensor_mac_c0                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_c4 {
	union reg_sensor_mac_c4                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_c8 {
	union reg_sensor_mac_c8                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_d0 {
	union reg_sensor_mac_d0                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_d4 {
	union reg_sensor_mac_d4                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_d8 {
	union reg_sensor_mac_d8                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_dc {
	union reg_sensor_mac_dc                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_e0 {
	union reg_sensor_mac_e0                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_e4 {
	union reg_sensor_mac_e4                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_e8 {
	union reg_sensor_mac_e8                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_ec {
	union reg_sensor_mac_ec                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_f0 {
	union reg_sensor_mac_f0                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_f4 {
	union reg_sensor_mac_f4                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_f8 {
	union reg_sensor_mac_f8                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_fc {
	union reg_sensor_mac_fc                 write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_100 {
	union reg_sensor_mac_100                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_104 {
	union reg_sensor_mac_104                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_108 {
	union reg_sensor_mac_108                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_110 {
	union reg_sensor_mac_110                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_114 {
	union reg_sensor_mac_114                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_118 {
	union reg_sensor_mac_118                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_11c {
	union reg_sensor_mac_11c                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_120 {
	union reg_sensor_mac_120                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_124 {
	union reg_sensor_mac_124                write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_mac_t {
	struct vreg_sensor_mac_00                reg_00;
	struct vreg_resv                        _resv_0x4[3];
	struct vreg_sensor_mac_10                reg_10;
	struct vreg_sensor_mac_14                reg_14;
	struct vreg_sensor_mac_18                reg_18;
	struct vreg_sensor_mac_1c                reg_1c;
	struct vreg_sensor_mac_20                reg_20;
	struct vreg_sensor_mac_24                reg_24;
	struct vreg_sensor_mac_28                reg_28;
	struct vreg_resv                        _resv_0x2c[1];
	struct vreg_sensor_mac_30                reg_30;
	struct vreg_sensor_mac_34                reg_34;
	struct vreg_resv                        _resv_0x38[2];
	struct vreg_sensor_mac_40                reg_40;
	struct vreg_sensor_mac_44                reg_44;
	struct vreg_sensor_mac_48                reg_48;
	struct vreg_sensor_mac_4c                reg_4c;
	struct vreg_sensor_mac_50                reg_50;
	struct vreg_sensor_mac_54                reg_54;
	struct vreg_sensor_mac_58                reg_58;
	struct vreg_resv                        _resv_0x5c[1];
	struct vreg_sensor_mac_60                reg_60;
	struct vreg_sensor_mac_64                reg_64;
	struct vreg_sensor_mac_68                reg_68;
	struct vreg_sensor_mac_6c                reg_6c;
	struct vreg_sensor_mac_70                reg_70;
	struct vreg_sensor_mac_74                reg_74;
	struct vreg_resv                        _resv_0x78[2];
	struct vreg_sensor_mac_80                reg_80;
	struct vreg_sensor_mac_84                reg_84;
	struct vreg_sensor_mac_88                reg_88;
	struct vreg_sensor_mac_8c                reg_8c;
	struct vreg_sensor_mac_90                reg_90;
	struct vreg_sensor_mac_94                reg_94;
	struct vreg_sensor_mac_98                reg_98;
	struct vreg_sensor_mac_9c                reg_9c;
	struct vreg_sensor_mac_a0                reg_a0;
	struct vreg_sensor_mac_a4                reg_a4;
	struct vreg_sensor_mac_a8                reg_a8;
	struct vreg_resv                        _resv_0xac[1];
	struct vreg_sensor_mac_b0                reg_b0;
	struct vreg_sensor_mac_b4                reg_b4;
	struct vreg_sensor_mac_b8                reg_b8;
	struct vreg_sensor_mac_bc                reg_bc;
	struct vreg_sensor_mac_c0                reg_c0;
	struct vreg_sensor_mac_c4                reg_c4;
	struct vreg_sensor_mac_c8                reg_c8;
	struct vreg_resv                        _resv_0xcc[1];
	struct vreg_sensor_mac_d0                reg_d0;
	struct vreg_sensor_mac_d4                reg_d4;
	struct vreg_sensor_mac_d8                reg_d8;
	struct vreg_sensor_mac_dc                reg_dc;
	struct vreg_sensor_mac_e0                reg_e0;
	struct vreg_sensor_mac_e4                reg_e4;
	struct vreg_sensor_mac_e8                reg_e8;
	struct vreg_sensor_mac_ec                reg_ec;
	struct vreg_sensor_mac_f0                reg_f0;
	struct vreg_sensor_mac_f4                reg_f4;
	struct vreg_sensor_mac_f8                reg_f8;
	struct vreg_sensor_mac_fc                reg_fc;
	struct vreg_sensor_mac_100               reg_100;
	struct vreg_sensor_mac_104               reg_104;
	struct vreg_sensor_mac_108               reg_108;
	struct vreg_resv                        _resv_0x10c[1];
	struct vreg_sensor_mac_110               reg_110;
	struct vreg_sensor_mac_114               reg_114;
	struct vreg_sensor_mac_118               reg_118;
	struct vreg_sensor_mac_11c               reg_11c;
	struct vreg_sensor_mac_120               reg_120;
	struct vreg_sensor_mac_124               reg_124;
};

/******************************************/
/*           module definition            */
/******************************************/
struct vreg_csi_ctrl_top_00 {
	union reg_csi_ctrl_top_00               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_04 {
	union reg_csi_ctrl_top_04               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_08 {
	union reg_csi_ctrl_top_08               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_0c {
	union reg_csi_ctrl_top_0c               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_10 {
	union reg_csi_ctrl_top_10               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_14 {
	union reg_csi_ctrl_top_14               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_18 {
	union reg_csi_ctrl_top_18               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_1c {
	union reg_csi_ctrl_top_1c               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_20 {
	union reg_csi_ctrl_top_20               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_24 {
	union reg_csi_ctrl_top_24               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_30 {
	union reg_csi_ctrl_top_30               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_34 {
	union reg_csi_ctrl_top_34               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_40 {
	union reg_csi_ctrl_top_40               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_48 {
	union reg_csi_ctrl_top_48               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_4c {
	union reg_csi_ctrl_top_4c               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_50 {
	union reg_csi_ctrl_top_50               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_54 {
	union reg_csi_ctrl_top_54               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_58 {
	union reg_csi_ctrl_top_58               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_5c {
	union reg_csi_ctrl_top_5c               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_60 {
	union reg_csi_ctrl_top_60               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_64 {
	union reg_csi_ctrl_top_64               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_70 {
	union reg_csi_ctrl_top_70               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_74 {
	union reg_csi_ctrl_top_74               write;
	union cmdset_field                      ctrl;
};

struct vreg_csi_ctrl_top_t {
	struct vreg_csi_ctrl_top_00              reg_00;
	struct vreg_csi_ctrl_top_04              reg_04;
	struct vreg_csi_ctrl_top_08              reg_08;
	struct vreg_csi_ctrl_top_0c              reg_0c;
	struct vreg_csi_ctrl_top_10              reg_10;
	struct vreg_csi_ctrl_top_14              reg_14;
	struct vreg_csi_ctrl_top_18              reg_18;
	struct vreg_csi_ctrl_top_1c              reg_1c;
	struct vreg_csi_ctrl_top_20              reg_20;
	struct vreg_csi_ctrl_top_24              reg_24;
	struct vreg_resv                        _resv_0x28[2];
	struct vreg_csi_ctrl_top_30              reg_30;
	struct vreg_csi_ctrl_top_34              reg_34;
	struct vreg_resv                        _resv_0x38[2];
	struct vreg_csi_ctrl_top_40              reg_40;
	struct vreg_resv                        _resv_0x44[1];
	struct vreg_csi_ctrl_top_48              reg_48;
	struct vreg_csi_ctrl_top_4c              reg_4c;
	struct vreg_csi_ctrl_top_50              reg_50;
	struct vreg_csi_ctrl_top_54              reg_54;
	struct vreg_csi_ctrl_top_58              reg_58;
	struct vreg_csi_ctrl_top_5c              reg_5c;
	struct vreg_csi_ctrl_top_60              reg_60;
	struct vreg_csi_ctrl_top_64              reg_64;
	struct vreg_resv                        _resv_0x68[2];
	struct vreg_csi_ctrl_top_70              reg_70;
	struct vreg_csi_ctrl_top_74              reg_74;
};

/******************************************/
/*           module definition            */
/******************************************/
struct vreg_sensor_phy_top_00 {
	union reg_sensor_phy_top_00             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_04 {
	union reg_sensor_phy_top_04             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_08 {
	union reg_sensor_phy_top_08             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_0c {
	union reg_sensor_phy_top_0c             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_10 {
	union reg_sensor_phy_top_10             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_20 {
	union reg_sensor_phy_top_20             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_24 {
	union reg_sensor_phy_top_24             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_2c {
	union reg_sensor_phy_top_2c             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_30 {
	union reg_sensor_phy_top_30             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_34 {
	union reg_sensor_phy_top_34             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_38 {
	union reg_sensor_phy_top_38             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_3c {
	union reg_sensor_phy_top_3c             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_40 {
	union reg_sensor_phy_top_40             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_44 {
	union reg_sensor_phy_top_44             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_48 {
	union reg_sensor_phy_top_48             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_70 {
	union reg_sensor_phy_top_70             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_74 {
	union reg_sensor_phy_top_74             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_7c {
	union reg_sensor_phy_top_7c             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_80 {
	union reg_sensor_phy_top_80             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_84 {
	union reg_sensor_phy_top_84             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_88 {
	union reg_sensor_phy_top_88             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_90 {
	union reg_sensor_phy_top_90             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_94 {
	union reg_sensor_phy_top_94             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_a0 {
	union reg_sensor_phy_top_a0             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_a4 {
	union reg_sensor_phy_top_a4             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_a8 {
	union reg_sensor_phy_top_a8             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_ac {
	union reg_sensor_phy_top_ac             write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_100 {
	union reg_sensor_phy_top_dft_100        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_104 {
	union reg_sensor_phy_top_dft_104        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_108 {
	union reg_sensor_phy_top_dft_108        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_10c {
	union reg_sensor_phy_top_dft_10c        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_110 {
	union reg_sensor_phy_top_dft_110        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_114 {
	union reg_sensor_phy_top_dft_114        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dft_11c {
	union reg_sensor_phy_top_dft_11c        write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dbg_12 {
	union reg_sensor_phy_top_dbg_120         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_0 {
	union reg_sensor_phy_top_test_0         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_1 {
	union reg_sensor_phy_top_test_1         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_2 {
	union reg_sensor_phy_top_test_2         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_3 {
	union reg_sensor_phy_top_test_3         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_4 {
	union reg_sensor_phy_top_test_4         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_5 {
	union reg_sensor_phy_top_test_5         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_6 {
	union reg_sensor_phy_top_test_6         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_7 {
	union reg_sensor_phy_top_test_7         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_8 {
	union reg_sensor_phy_top_test_8         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_9 {
	union reg_sensor_phy_top_test_9         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_a {
	union reg_sensor_phy_top_test_a         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_test_b {
	union reg_sensor_phy_top_test_b         write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d0 {
	union reg_sensor_phy_top_dummy_d0       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d1 {
	union reg_sensor_phy_top_dummy_d1       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d2 {
	union reg_sensor_phy_top_dummy_d2       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d3 {
	union reg_sensor_phy_top_dummy_d3       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d4 {
	union reg_sensor_phy_top_dummy_d4       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d5 {
	union reg_sensor_phy_top_dummy_d5       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d6 {
	union reg_sensor_phy_top_dummy_d6       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_dummy_d7 {
	union reg_sensor_phy_top_dummy_d7       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_top_t {
	struct vreg_sensor_phy_top_00            reg_00;
	struct vreg_sensor_phy_top_04            reg_04;
	struct vreg_sensor_phy_top_08            reg_08;
	struct vreg_sensor_phy_top_0c            reg_0c;
	struct vreg_sensor_phy_top_10            reg_10;
	struct vreg_resv                        _resv_0x14[3];
	struct vreg_sensor_phy_top_20            reg_20;
	struct vreg_sensor_phy_top_24            reg_24;
	struct vreg_resv                        _resv_0x28[1];
	struct vreg_sensor_phy_top_2c            reg_2c;
	struct vreg_sensor_phy_top_30            reg_30;
	struct vreg_sensor_phy_top_34            reg_34;
	struct vreg_sensor_phy_top_38            reg_38;
	struct vreg_sensor_phy_top_3c            reg_3c;
	struct vreg_sensor_phy_top_40            reg_40;
	struct vreg_sensor_phy_top_44            reg_44;
	struct vreg_sensor_phy_top_48            reg_48;
	struct vreg_resv                        _resv_0x4c[9];
	struct vreg_sensor_phy_top_70            reg_70;
	struct vreg_sensor_phy_top_74            reg_74;
	struct vreg_resv                        _resv_0x78[1];
	struct vreg_sensor_phy_top_7c            reg_7c;
	struct vreg_sensor_phy_top_80            reg_80;
	struct vreg_sensor_phy_top_84            reg_84;
	struct vreg_sensor_phy_top_88            reg_88;
	struct vreg_resv                        _resv_0x8c[1];
	struct vreg_sensor_phy_top_90            reg_90;
	struct vreg_sensor_phy_top_94            reg_94;
	struct vreg_resv                        _resv_0x98[2];
	struct vreg_sensor_phy_top_a0            reg_a0;
	struct vreg_sensor_phy_top_a4            reg_a4;
	struct vreg_sensor_phy_top_a8            reg_a8;
	struct vreg_sensor_phy_top_ac            reg_ac;
	struct vreg_resv                        _resv_0xb0[20];
	struct vreg_sensor_phy_top_dft_100       dft_100;
	struct vreg_sensor_phy_top_dft_104       dft_104;
	struct vreg_sensor_phy_top_dft_108       dft_108;
	struct vreg_sensor_phy_top_dft_10c       dft_10c;
	struct vreg_sensor_phy_top_dft_110       dft_110;
	struct vreg_sensor_phy_top_dft_114       dft_114;
	struct vreg_resv                        _resv_0x118[1];
	struct vreg_sensor_phy_top_dft_11c       dft_11c;
	struct vreg_sensor_phy_top_dbg_12        dbg_12;
	struct vreg_resv                        _resv_0x124[55];
	struct vreg_sensor_phy_top_test_0        test_0;
	struct vreg_sensor_phy_top_test_1        test_1;
	struct vreg_sensor_phy_top_test_2        test_2;
	struct vreg_sensor_phy_top_test_3        test_3;
	struct vreg_sensor_phy_top_test_4        test_4;
	struct vreg_sensor_phy_top_test_5        test_5;
	struct vreg_sensor_phy_top_test_6        test_6;
	struct vreg_sensor_phy_top_test_7        test_7;
	struct vreg_sensor_phy_top_test_8        test_8;
	struct vreg_sensor_phy_top_test_9        test_9;
	struct vreg_sensor_phy_top_test_a        test_a;
	struct vreg_sensor_phy_top_test_b        test_b;
	struct vreg_resv                        _resv_0x230[12];
	struct vreg_sensor_phy_top_dummy_d0      dummy_d0;
	struct vreg_sensor_phy_top_dummy_d1      dummy_d1;
	struct vreg_sensor_phy_top_dummy_d2      dummy_d2;
	struct vreg_sensor_phy_top_dummy_d3      dummy_d3;
	struct vreg_sensor_phy_top_dummy_d4      dummy_d4;
	struct vreg_sensor_phy_top_dummy_d5      dummy_d5;
	struct vreg_sensor_phy_top_dummy_d6      dummy_d6;
	struct vreg_sensor_phy_top_dummy_d7      dummy_d7;
};

/******************************************/
/*           module definition            */
/******************************************/
struct vreg_sensor_phy_4l_00 {
	union reg_sensor_phy_4l_00              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_04 {
	union reg_sensor_phy_4l_04              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_08 {
	union reg_sensor_phy_4l_08              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_0c {
	union reg_sensor_phy_4l_0c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_10 {
	union reg_sensor_phy_4l_10              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_20 {
	union reg_sensor_phy_4l_20              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_24 {
	union reg_sensor_phy_4l_24              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_28 {
	union reg_sensor_phy_4l_28              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_30 {
	union reg_sensor_phy_4l_30              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_34 {
	union reg_sensor_phy_4l_34              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_38 {
	union reg_sensor_phy_4l_38              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_3c {
	union reg_sensor_phy_4l_3c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_40 {
	union reg_sensor_phy_4l_40              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_44 {
	union reg_sensor_phy_4l_44              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_48 {
	union reg_sensor_phy_4l_48              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_4c {
	union reg_sensor_phy_4l_4c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_50 {
	union reg_sensor_phy_4l_50              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_54 {
	union reg_sensor_phy_4l_54              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_58 {
	union reg_sensor_phy_4l_58              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_5c {
	union reg_sensor_phy_4l_5c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_60 {
	union reg_sensor_phy_4l_60              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_64 {
	union reg_sensor_phy_4l_64              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_dbg_90 {
	union reg_sensor_phy_4l_dbg_90          write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_dbg_94 {
	union reg_sensor_phy_4l_dbg_94          write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_status_98 {
	union reg_sensor_phy_4l_status_98       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_status_9c {
	union reg_sensor_phy_4l_status_9c       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_status_a4 {
	union reg_sensor_phy_4l_status_a4       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_status_a8 {
	union reg_sensor_phy_4l_status_a8       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_0 {
	union reg_sensor_phy_4l_d0_0            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_1 {
	union reg_sensor_phy_4l_d0_1            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_2 {
	union reg_sensor_phy_4l_d0_2            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_3 {
	union reg_sensor_phy_4l_d0_3            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_4 {
	union reg_sensor_phy_4l_d0_4            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_5 {
	union reg_sensor_phy_4l_d0_5            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_6 {
	union reg_sensor_phy_4l_d0_6            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_7 {
	union reg_sensor_phy_4l_d0_7            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_8 {
	union reg_sensor_phy_4l_d0_8            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_9 {
	union reg_sensor_phy_4l_d0_9            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_a {
	union reg_sensor_phy_4l_d0_a            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d0_b {
	union reg_sensor_phy_4l_d0_b            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_0 {
	union reg_sensor_phy_4l_d1_0            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_1 {
	union reg_sensor_phy_4l_d1_1            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_2 {
	union reg_sensor_phy_4l_d1_2            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_3 {
	union reg_sensor_phy_4l_d1_3            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_4 {
	union reg_sensor_phy_4l_d1_4            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_5 {
	union reg_sensor_phy_4l_d1_5            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_6 {
	union reg_sensor_phy_4l_d1_6            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_7 {
	union reg_sensor_phy_4l_d1_7            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_8 {
	union reg_sensor_phy_4l_d1_8            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_9 {
	union reg_sensor_phy_4l_d1_9            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_a {
	union reg_sensor_phy_4l_d1_a            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d1_b {
	union reg_sensor_phy_4l_d1_b            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_0 {
	union reg_sensor_phy_4l_d2_0            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_1 {
	union reg_sensor_phy_4l_d2_1            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_2 {
	union reg_sensor_phy_4l_d2_2            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_3 {
	union reg_sensor_phy_4l_d2_3            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_4 {
	union reg_sensor_phy_4l_d2_4            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_5 {
	union reg_sensor_phy_4l_d2_5            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_6 {
	union reg_sensor_phy_4l_d2_6            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_7 {
	union reg_sensor_phy_4l_d2_7            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_8 {
	union reg_sensor_phy_4l_d2_8            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_9 {
	union reg_sensor_phy_4l_d2_9            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_a {
	union reg_sensor_phy_4l_d2_a            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d2_b {
	union reg_sensor_phy_4l_d2_b            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_0 {
	union reg_sensor_phy_4l_d3_0            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_1 {
	union reg_sensor_phy_4l_d3_1            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_2 {
	union reg_sensor_phy_4l_d3_2            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_3 {
	union reg_sensor_phy_4l_d3_3            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_4 {
	union reg_sensor_phy_4l_d3_4            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_5 {
	union reg_sensor_phy_4l_d3_5            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_6 {
	union reg_sensor_phy_4l_d3_6            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_7 {
	union reg_sensor_phy_4l_d3_7            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_8 {
	union reg_sensor_phy_4l_d3_8            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_9 {
	union reg_sensor_phy_4l_d3_9            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_a {
	union reg_sensor_phy_4l_d3_a            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_d3_b {
	union reg_sensor_phy_4l_d3_b            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_4l_t {
	struct vreg_sensor_phy_4l_00             reg_00;
	struct vreg_sensor_phy_4l_04             reg_04;
	struct vreg_sensor_phy_4l_08             reg_08;
	struct vreg_sensor_phy_4l_0c             reg_0c;
	struct vreg_sensor_phy_4l_10             reg_10;
	struct vreg_resv                        _resv_0x14[3];
	struct vreg_sensor_phy_4l_20             reg_20;
	struct vreg_sensor_phy_4l_24             reg_24;
	struct vreg_sensor_phy_4l_28             reg_28;
	struct vreg_resv                        _resv_0x2c[1];
	struct vreg_sensor_phy_4l_30             reg_30;
	struct vreg_sensor_phy_4l_34             reg_34;
	struct vreg_sensor_phy_4l_38             reg_38;
	struct vreg_sensor_phy_4l_3c             reg_3c;
	struct vreg_sensor_phy_4l_40             reg_40;
	struct vreg_sensor_phy_4l_44             reg_44;
	struct vreg_sensor_phy_4l_48             reg_48;
	struct vreg_sensor_phy_4l_4c             reg_4c;
	struct vreg_sensor_phy_4l_50             reg_50;
	struct vreg_sensor_phy_4l_54             reg_54;
	struct vreg_sensor_phy_4l_58             reg_58;
	struct vreg_sensor_phy_4l_5c             reg_5c;
	struct vreg_sensor_phy_4l_60             reg_60;
	struct vreg_sensor_phy_4l_64             reg_64;
	struct vreg_resv                        _resv_0x68[10];
	struct vreg_sensor_phy_4l_dbg_90         dbg_90;
	struct vreg_sensor_phy_4l_dbg_94         dbg_94;
	struct vreg_sensor_phy_4l_status_98      status_98;
	struct vreg_sensor_phy_4l_status_9c      status_9c;
	struct vreg_resv                        _resv_0xa0[1];
	struct vreg_sensor_phy_4l_status_a4      status_a4;
	struct vreg_sensor_phy_4l_status_a8      status_a8;
	struct vreg_resv                        _resv_0xac[21];
	struct vreg_sensor_phy_4l_d0_0           d0_0;
	struct vreg_sensor_phy_4l_d0_1           d0_1;
	struct vreg_sensor_phy_4l_d0_2           d0_2;
	struct vreg_sensor_phy_4l_d0_3           d0_3;
	struct vreg_sensor_phy_4l_d0_4           d0_4;
	struct vreg_sensor_phy_4l_d0_5           d0_5;
	struct vreg_sensor_phy_4l_d0_6           d0_6;
	struct vreg_sensor_phy_4l_d0_7           d0_7;
	struct vreg_sensor_phy_4l_d0_8           d0_8;
	struct vreg_sensor_phy_4l_d0_9           d0_9;
	struct vreg_sensor_phy_4l_d0_a           d0_a;
	struct vreg_sensor_phy_4l_d0_b           d0_b;
	struct vreg_resv                        _resv_0x130[4];
	struct vreg_sensor_phy_4l_d1_0           d1_0;
	struct vreg_sensor_phy_4l_d1_1           d1_1;
	struct vreg_sensor_phy_4l_d1_2           d1_2;
	struct vreg_sensor_phy_4l_d1_3           d1_3;
	struct vreg_sensor_phy_4l_d1_4           d1_4;
	struct vreg_sensor_phy_4l_d1_5           d1_5;
	struct vreg_sensor_phy_4l_d1_6           d1_6;
	struct vreg_sensor_phy_4l_d1_7           d1_7;
	struct vreg_sensor_phy_4l_d1_8           d1_8;
	struct vreg_sensor_phy_4l_d1_9           d1_9;
	struct vreg_sensor_phy_4l_d1_a           d1_a;
	struct vreg_sensor_phy_4l_d1_b           d1_b;
	struct vreg_resv                        _resv_0x170[4];
	struct vreg_sensor_phy_4l_d2_0           d2_0;
	struct vreg_sensor_phy_4l_d2_1           d2_1;
	struct vreg_sensor_phy_4l_d2_2           d2_2;
	struct vreg_sensor_phy_4l_d2_3           d2_3;
	struct vreg_sensor_phy_4l_d2_4           d2_4;
	struct vreg_sensor_phy_4l_d2_5           d2_5;
	struct vreg_sensor_phy_4l_d2_6           d2_6;
	struct vreg_sensor_phy_4l_d2_7           d2_7;
	struct vreg_sensor_phy_4l_d2_8           d2_8;
	struct vreg_sensor_phy_4l_d2_9           d2_9;
	struct vreg_sensor_phy_4l_d2_a           d2_a;
	struct vreg_sensor_phy_4l_d2_b           d2_b;
	struct vreg_resv                        _resv_0x1b0[4];
	struct vreg_sensor_phy_4l_d3_0           d3_0;
	struct vreg_sensor_phy_4l_d3_1           d3_1;
	struct vreg_sensor_phy_4l_d3_2           d3_2;
	struct vreg_sensor_phy_4l_d3_3           d3_3;
	struct vreg_sensor_phy_4l_d3_4           d3_4;
	struct vreg_sensor_phy_4l_d3_5           d3_5;
	struct vreg_sensor_phy_4l_d3_6           d3_6;
	struct vreg_sensor_phy_4l_d3_7           d3_7;
	struct vreg_sensor_phy_4l_d3_8           d3_8;
	struct vreg_sensor_phy_4l_d3_9           d3_9;
	struct vreg_sensor_phy_4l_d3_a           d3_a;
	struct vreg_sensor_phy_4l_d3_b           d3_b;
};

/******************************************/
/*           module definition            */
/******************************************/
struct vreg_sensor_phy_2l_00 {
	union reg_sensor_phy_2l_00              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_04 {
	union reg_sensor_phy_2l_04              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_08 {
	union reg_sensor_phy_2l_08              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_0c {
	union reg_sensor_phy_2l_0c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_10 {
	union reg_sensor_phy_2l_10              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_20 {
	union reg_sensor_phy_2l_20              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_24 {
	union reg_sensor_phy_2l_24              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_28 {
	union reg_sensor_phy_2l_28              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_30 {
	union reg_sensor_phy_2l_30              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_34 {
	union reg_sensor_phy_2l_34              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_38 {
	union reg_sensor_phy_2l_38              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_3c {
	union reg_sensor_phy_2l_3c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_40 {
	union reg_sensor_phy_2l_40              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_44 {
	union reg_sensor_phy_2l_44              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_48 {
	union reg_sensor_phy_2l_48              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_4c {
	union reg_sensor_phy_2l_4c              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_50 {
	union reg_sensor_phy_2l_50              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_54 {
	union reg_sensor_phy_2l_54              write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_dbg_90 {
	union reg_sensor_phy_2l_dbg_90          write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_dbg_94 {
	union reg_sensor_phy_2l_dbg_94          write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_status_98 {
	union reg_sensor_phy_2l_status_98       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_status_9c {
	union reg_sensor_phy_2l_status_9c       write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_0 {
	union reg_sensor_phy_2l_d0_0            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_1 {
	union reg_sensor_phy_2l_d0_1            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_2 {
	union reg_sensor_phy_2l_d0_2            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_3 {
	union reg_sensor_phy_2l_d0_3            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_4 {
	union reg_sensor_phy_2l_d0_4            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_5 {
	union reg_sensor_phy_2l_d0_5            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_6 {
	union reg_sensor_phy_2l_d0_6            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_7 {
	union reg_sensor_phy_2l_d0_7            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_8 {
	union reg_sensor_phy_2l_d0_8            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_9 {
	union reg_sensor_phy_2l_d0_9            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_a {
	union reg_sensor_phy_2l_d0_a            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d0_b {
	union reg_sensor_phy_2l_d0_b            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_0 {
	union reg_sensor_phy_2l_d1_0            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_1 {
	union reg_sensor_phy_2l_d1_1            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_2 {
	union reg_sensor_phy_2l_d1_2            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_3 {
	union reg_sensor_phy_2l_d1_3            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_4 {
	union reg_sensor_phy_2l_d1_4            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_5 {
	union reg_sensor_phy_2l_d1_5            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_6 {
	union reg_sensor_phy_2l_d1_6            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_7 {
	union reg_sensor_phy_2l_d1_7            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_8 {
	union reg_sensor_phy_2l_d1_8            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_9 {
	union reg_sensor_phy_2l_d1_9            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_a {
	union reg_sensor_phy_2l_d1_a            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_d1_b {
	union reg_sensor_phy_2l_d1_b            write;
	union cmdset_field                      ctrl;
};

struct vreg_sensor_phy_2l_t {
	struct vreg_sensor_phy_2l_00             reg_00;
	struct vreg_sensor_phy_2l_04             reg_04;
	struct vreg_sensor_phy_2l_08             reg_08;
	struct vreg_sensor_phy_2l_0c             reg_0c;
	struct vreg_sensor_phy_2l_10             reg_10;
	struct vreg_resv                        _resv_0x14[3];
	struct vreg_sensor_phy_2l_20             reg_20;
	struct vreg_sensor_phy_2l_24             reg_24;
	struct vreg_sensor_phy_2l_28             reg_28;
	struct vreg_resv                        _resv_0x2c[1];
	struct vreg_sensor_phy_2l_30             reg_30;
	struct vreg_sensor_phy_2l_34             reg_34;
	struct vreg_sensor_phy_2l_38             reg_38;
	struct vreg_sensor_phy_2l_3c             reg_3c;
	struct vreg_sensor_phy_2l_40             reg_40;
	struct vreg_sensor_phy_2l_44             reg_44;
	struct vreg_sensor_phy_2l_48             reg_48;
	struct vreg_sensor_phy_2l_4c             reg_4c;
	struct vreg_sensor_phy_2l_50             reg_50;
	struct vreg_sensor_phy_2l_54             reg_54;
	struct vreg_resv                        _resv_0x58[14];
	struct vreg_sensor_phy_2l_dbg_90         dbg_90;
	struct vreg_sensor_phy_2l_dbg_94         dbg_94;
	struct vreg_sensor_phy_2l_status_98      status_98;
	struct vreg_sensor_phy_2l_status_9c      status_9c;
	struct vreg_resv                        _resv_0xa0[24];
	struct vreg_sensor_phy_2l_d0_0           d0_0;
	struct vreg_sensor_phy_2l_d0_1           d0_1;
	struct vreg_sensor_phy_2l_d0_2           d0_2;
	struct vreg_sensor_phy_2l_d0_3           d0_3;
	struct vreg_sensor_phy_2l_d0_4           d0_4;
	struct vreg_sensor_phy_2l_d0_5           d0_5;
	struct vreg_sensor_phy_2l_d0_6           d0_6;
	struct vreg_sensor_phy_2l_d0_7           d0_7;
	struct vreg_sensor_phy_2l_d0_8           d0_8;
	struct vreg_sensor_phy_2l_d0_9           d0_9;
	struct vreg_sensor_phy_2l_d0_a           d0_a;
	struct vreg_sensor_phy_2l_d0_b           d0_b;
	struct vreg_resv                        _resv_0x130[4];
	struct vreg_sensor_phy_2l_d1_0           d1_0;
	struct vreg_sensor_phy_2l_d1_1           d1_1;
	struct vreg_sensor_phy_2l_d1_2           d1_2;
	struct vreg_sensor_phy_2l_d1_3           d1_3;
	struct vreg_sensor_phy_2l_d1_4           d1_4;
	struct vreg_sensor_phy_2l_d1_5           d1_5;
	struct vreg_sensor_phy_2l_d1_6           d1_6;
	struct vreg_sensor_phy_2l_d1_7           d1_7;
	struct vreg_sensor_phy_2l_d1_8           d1_8;
	struct vreg_sensor_phy_2l_d1_9           d1_9;
	struct vreg_sensor_phy_2l_d1_a           d1_a;
	struct vreg_sensor_phy_2l_d1_b           d1_b;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_CMDQ_WARP_H_ */

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: reg_blocks_csi_wrap.h
 * Description:
 */

#ifndef _REG_BLOCKS_CSI_WRAP_H_
#define _REG_BLOCKS_CSI_WRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_CSI_DPHY_4LANE_WRAP_T {
	union REG_CSI_DPHY_4LANE_WRAP_00        REG_00;
	union REG_CSI_DPHY_4LANE_WRAP_04        REG_04;
	union REG_CSI_DPHY_4LANE_WRAP_08        REG_08;
	union REG_CSI_DPHY_4LANE_WRAP_0C        REG_0C;
	union REG_CSI_DPHY_4LANE_WRAP_10        REG_10;
	union REG_CSI_DPHY_4LANE_WRAP_14        REG_14;
	union REG_CSI_DPHY_4LANE_WRAP_18        REG_18;
	uint32_t                                _resv_0x1c[1];
	union REG_CSI_DPHY_4LANE_WRAP_20        REG_20;
	union REG_CSI_DPHY_4LANE_WRAP_24        REG_24;
	union REG_CSI_DPHY_4LANE_WRAP_28        REG_28;
	union REG_CSI_DPHY_4LANE_WRAP_2C        REG_2C;
	union REG_CSI_DPHY_4LANE_WRAP_30        REG_30;
	union REG_CSI_DPHY_4LANE_WRAP_34        REG_34;
	union REG_CSI_DPHY_4LANE_WRAP_38        REG_38;
	union REG_CSI_DPHY_4LANE_WRAP_3C        REG_3C;
	union REG_CSI_DPHY_4LANE_WRAP_40        REG_40;
	union REG_CSI_DPHY_4LANE_WRAP_44        REG_44;
	union REG_CSI_DPHY_4LANE_WRAP_48        REG_48;
	union REG_CSI_DPHY_4LANE_WRAP_4C        REG_4C;
	union REG_CSI_DPHY_4LANE_WRAP_50        REG_50;
	union REG_CSI_DPHY_4LANE_WRAP_54        REG_54;
	union REG_CSI_DPHY_4LANE_WRAP_58        REG_58;
	uint32_t                                _resv_0x5c[1];
	union REG_CSI_DPHY_4LANE_WRAP_FRC_60    FRC_60;
	union REG_CSI_DPHY_4LANE_WRAP_FRC_64    FRC_64;
	union REG_CSI_DPHY_4LANE_WRAP_FRC_68    FRC_68;
	union REG_CSI_DPHY_4LANE_WRAP_DFT_6C    DFT_6C;
	union REG_CSI_DPHY_4LANE_WRAP_DFT_70    DFT_70;
	uint32_t                                _resv_0x74[1];
	union REG_CSI_DPHY_4LANE_WRAP_DFT_78    DFT_78;
	union REG_CSI_DPHY_4LANE_WRAP_DFT_7C    DFT_7C;
	uint32_t                                _resv_0x80[4];
	union REG_CSI_DPHY_4LANE_WRAP_DBG_90    DBG_90;
	uint32_t                                _resv_0x94[23];
	union REG_CSI_DPHY_4LANE_WRAP_LVDS_F0   LVDS_F0;
	union REG_CSI_DPHY_4LANE_WRAP_LVDS_F4   LVDS_F4;
	union REG_CSI_DPHY_4LANE_WRAP_LVDS_F8   LVDS_F8;
	union REG_CSI_DPHY_4LANE_WRAP_LVDS_FC   LVDS_FC;
	union REG_CSI_DPHY_4LANE_WRAP_D0_0      D0_0;
	union REG_CSI_DPHY_4LANE_WRAP_D0_1      D0_1;
	union REG_CSI_DPHY_4LANE_WRAP_D0_2      D0_2;
	union REG_CSI_DPHY_4LANE_WRAP_D0_3      D0_3;
	union REG_CSI_DPHY_4LANE_WRAP_D0_4      D0_4;
	union REG_CSI_DPHY_4LANE_WRAP_D0_5      D0_5;
	union REG_CSI_DPHY_4LANE_WRAP_D0_6      D0_6;
	union REG_CSI_DPHY_4LANE_WRAP_D0_7      D0_7;
	union REG_CSI_DPHY_4LANE_WRAP_D0_8      D0_8;
	union REG_CSI_DPHY_4LANE_WRAP_D0_9      D0_9;
	union REG_CSI_DPHY_4LANE_WRAP_D0_A      D0_A;
	union REG_CSI_DPHY_4LANE_WRAP_D0_B      D0_B;
	uint32_t                                _resv_0x130[4];
	union REG_CSI_DPHY_4LANE_WRAP_D1_0      D1_0;
	union REG_CSI_DPHY_4LANE_WRAP_D1_1      D1_1;
	union REG_CSI_DPHY_4LANE_WRAP_D1_2      D1_2;
	union REG_CSI_DPHY_4LANE_WRAP_D1_3      D1_3;
	union REG_CSI_DPHY_4LANE_WRAP_D1_4      D1_4;
	union REG_CSI_DPHY_4LANE_WRAP_D1_5      D1_5;
	union REG_CSI_DPHY_4LANE_WRAP_D1_6      D1_6;
	union REG_CSI_DPHY_4LANE_WRAP_D1_7      D1_7;
	union REG_CSI_DPHY_4LANE_WRAP_D1_8      D1_8;
	union REG_CSI_DPHY_4LANE_WRAP_D1_9      D1_9;
	union REG_CSI_DPHY_4LANE_WRAP_D1_A      D1_A;
	union REG_CSI_DPHY_4LANE_WRAP_D1_B      D1_B;
	uint32_t                                _resv_0x170[4];
	union REG_CSI_DPHY_4LANE_WRAP_D2_0      D2_0;
	union REG_CSI_DPHY_4LANE_WRAP_D2_1      D2_1;
	union REG_CSI_DPHY_4LANE_WRAP_D2_2      D2_2;
	union REG_CSI_DPHY_4LANE_WRAP_D2_3      D2_3;
	union REG_CSI_DPHY_4LANE_WRAP_D2_4      D2_4;
	union REG_CSI_DPHY_4LANE_WRAP_D2_5      D2_5;
	union REG_CSI_DPHY_4LANE_WRAP_D2_6      D2_6;
	union REG_CSI_DPHY_4LANE_WRAP_D2_7      D2_7;
	union REG_CSI_DPHY_4LANE_WRAP_D2_8      D2_8;
	union REG_CSI_DPHY_4LANE_WRAP_D2_9      D2_9;
	union REG_CSI_DPHY_4LANE_WRAP_D2_A      D2_A;
	union REG_CSI_DPHY_4LANE_WRAP_D2_B      D2_B;
	uint32_t                                _resv_0x1b0[4];
	union REG_CSI_DPHY_4LANE_WRAP_D3_0      D3_0;
	union REG_CSI_DPHY_4LANE_WRAP_D3_1      D3_1;
	union REG_CSI_DPHY_4LANE_WRAP_D3_2      D3_2;
	union REG_CSI_DPHY_4LANE_WRAP_D3_3      D3_3;
	union REG_CSI_DPHY_4LANE_WRAP_D3_4      D3_4;
	union REG_CSI_DPHY_4LANE_WRAP_D3_5      D3_5;
	union REG_CSI_DPHY_4LANE_WRAP_D3_6      D3_6;
	union REG_CSI_DPHY_4LANE_WRAP_D3_7      D3_7;
	union REG_CSI_DPHY_4LANE_WRAP_D3_8      D3_8;
	union REG_CSI_DPHY_4LANE_WRAP_D3_9      D3_9;
	union REG_CSI_DPHY_4LANE_WRAP_D3_A      D3_A;
	union REG_CSI_DPHY_4LANE_WRAP_D3_B      D3_B;
	uint32_t                                _resv_0x1f0[4];
	union REG_CSI_DPHY_4LANE_WRAP_TEST_0    TEST_0;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_1    TEST_1;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_2    TEST_2;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_3    TEST_3;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_4    TEST_4;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_5    TEST_5;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_6    TEST_6;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_7    TEST_7;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_8    TEST_8;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_9    TEST_9;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_A    TEST_A;
	union REG_CSI_DPHY_4LANE_WRAP_TEST_B    TEST_B;
	union REG_CSI_DPHY_4LANE_WRAP_DUMMY_D0  DUMMY_D0;
	union REG_CSI_DPHY_4LANE_WRAP_DUMMY_D1  DUMMY_D1;
	union REG_CSI_DPHY_4LANE_WRAP_DUMMY_D2  DUMMY_D2;
	union REG_CSI_DPHY_4LANE_WRAP_DUMMY_D3  DUMMY_D3;
	uint32_t                                _resv_0x240[48];
	union REG_CSI_DPHY_4LANE_WRAP_DBG_300   DBG_300;
	uint32_t                                _resv_0x304[3];
	union REG_CSI_DPHY_4LANE_WRAP_STATUS_310  STATUS_310;
	union REG_CSI_DPHY_4LANE_WRAP_STATUS_314  STATUS_314;
	union REG_CSI_DPHY_4LANE_WRAP_STATUS_318  STATUS_318;
	union REG_CSI_DPHY_4LANE_WRAP_STATUS_31C  STATUS_31C;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_BLOCKS_CSI_WRAP_H_ */

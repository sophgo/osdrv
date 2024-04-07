/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_blocks_csi_mac.h
 * Description:HW register description
 */

#ifndef _REG_BLOCKS_CSI_MAC_H_
#define _REG_BLOCKS_CSI_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_SENSOR_MAC_T {
	union REG_SENSOR_MAC_00                 REG_00;
	uint32_t                                _resv_0x4[3];
	union REG_SENSOR_MAC_10                 REG_10;
	union REG_SENSOR_MAC_14                 REG_14;
	union REG_SENSOR_MAC_18                 REG_18;
	union REG_SENSOR_MAC_1C                 REG_1C;
	union REG_SENSOR_MAC_20                 REG_20;
	union REG_SENSOR_MAC_24                 REG_24;
	union REG_SENSOR_MAC_28                 REG_28;
	uint32_t                                _resv_0x2c[1];
	union REG_SENSOR_MAC_30                 REG_30;
	union REG_SENSOR_MAC_34                 REG_34;
	uint32_t                                _resv_0x38[2];
	union REG_SENSOR_MAC_40                 REG_40;
	union REG_SENSOR_MAC_44                 REG_44;
	union REG_SENSOR_MAC_48                 REG_48;
	union REG_SENSOR_MAC_4C                 REG_4C;
	union REG_SENSOR_MAC_50                 REG_50;
	union REG_SENSOR_MAC_54                 REG_54;
	union REG_SENSOR_MAC_58                 REG_58;
	uint32_t                                _resv_0x5c[1];
	union REG_SENSOR_MAC_60                 REG_60;
	union REG_SENSOR_MAC_64                 REG_64;
	union REG_SENSOR_MAC_68                 REG_68;
	union REG_SENSOR_MAC_6C                 REG_6C;
	union REG_SENSOR_MAC_70                 REG_70;
	union REG_SENSOR_MAC_74                 REG_74;
	uint32_t                                _resv_0x78[2];
	union REG_SENSOR_MAC_80                 REG_80;
	union REG_SENSOR_MAC_84                 REG_84;
	union REG_SENSOR_MAC_88                 REG_88;
	union REG_SENSOR_MAC_8C                 REG_8C;
	union REG_SENSOR_MAC_90                 REG_90;
	union REG_SENSOR_MAC_94                 REG_94;
	union REG_SENSOR_MAC_98                 REG_98;
	union REG_SENSOR_MAC_9C                 REG_9C;
	union REG_SENSOR_MAC_A0                 REG_A0;
	union REG_SENSOR_MAC_A4                 REG_A4;
	union REG_SENSOR_MAC_A8                 REG_A8;
	uint32_t                                _resv_0xac[1];
	union REG_SENSOR_MAC_B0                 REG_B0;
	union REG_SENSOR_MAC_B4                 REG_B4;
	union REG_SENSOR_MAC_B8                 REG_B8;
	union REG_SENSOR_MAC_BC                 REG_BC;
	union REG_SENSOR_MAC_C0                 REG_C0;
	union REG_SENSOR_MAC_C4                 REG_C4;
	union REG_SENSOR_MAC_C8                 REG_C8;
	uint32_t                                _resv_0xcc[1];
	union REG_SENSOR_MAC_D0                 REG_D0;
	union REG_SENSOR_MAC_D4                 REG_D4;
	union REG_SENSOR_MAC_D8                 REG_D8;
	union REG_SENSOR_MAC_DC                 REG_DC;
	union REG_SENSOR_MAC_E0                 REG_E0;
	union REG_SENSOR_MAC_E4                 REG_E4;
	union REG_SENSOR_MAC_E8                 REG_E8;
	union REG_SENSOR_MAC_EC                 REG_EC;
	union REG_SENSOR_MAC_F0                 REG_F0;
	union REG_SENSOR_MAC_F4                 REG_F4;
	union REG_SENSOR_MAC_F8                 REG_F8;
	union REG_SENSOR_MAC_FC                 REG_FC;
	union REG_SENSOR_MAC_100                REG_100;
	union REG_SENSOR_MAC_104                REG_104;
	union REG_SENSOR_MAC_108                REG_108;
	uint32_t                                _resv_0x10c[1];
	union REG_SENSOR_MAC_110                REG_110;
	union REG_SENSOR_MAC_114                REG_114;
	union REG_SENSOR_MAC_118                REG_118;
	union REG_SENSOR_MAC_11C                REG_11C;
	union REG_SENSOR_MAC_120                REG_120;
	union REG_SENSOR_MAC_124                REG_124;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_SUBLVDS_CTRL_TOP_T {
	union REG_SUBLVDS_CTRL_TOP_00           REG_00;
	union REG_SUBLVDS_CTRL_TOP_04           REG_04;
	union REG_SUBLVDS_CTRL_TOP_08           REG_08;
	union REG_SUBLVDS_CTRL_TOP_0C           REG_0C;
	union REG_SUBLVDS_CTRL_TOP_10           REG_10;
	union REG_SUBLVDS_CTRL_TOP_14           REG_14;
	union REG_SUBLVDS_CTRL_TOP_18           REG_18;
	union REG_SUBLVDS_CTRL_TOP_1C           REG_1C;
	union REG_SUBLVDS_CTRL_TOP_20           REG_20;
	union REG_SUBLVDS_CTRL_TOP_24           REG_24;
	union REG_SUBLVDS_CTRL_TOP_28           REG_28;
	union REG_SUBLVDS_CTRL_TOP_2C           REG_2C;
	union REG_SUBLVDS_CTRL_TOP_30           REG_30;
	uint32_t                                _resv_0x34[3];
	union REG_SUBLVDS_CTRL_TOP_40           REG_40;
	uint32_t                                _resv_0x44[3];
	union REG_SUBLVDS_CTRL_TOP_50           REG_50;
	union REG_SUBLVDS_CTRL_TOP_54           REG_54;
	union REG_SUBLVDS_CTRL_TOP_58           REG_58;
	uint32_t                                _resv_0x5c[1];
	union REG_SUBLVDS_CTRL_TOP_60           REG_60;
	union REG_SUBLVDS_CTRL_TOP_64           REG_64;
	union REG_SUBLVDS_CTRL_TOP_68           REG_68;
	union REG_SUBLVDS_CTRL_TOP_6C           REG_6C;
	union REG_SUBLVDS_CTRL_TOP_70           REG_70;
	union REG_SUBLVDS_CTRL_TOP_74           REG_74;
	uint32_t                                _resv_0x78[2];
	union REG_SUBLVDS_CTRL_TOP_80           REG_80;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_CSI_CTRL_TOP_T {
	union REG_CSI_CTRL_TOP_00               REG_00;
	union REG_CSI_CTRL_TOP_04               REG_04;
	union REG_CSI_CTRL_TOP_08               REG_08;
	union REG_CSI_CTRL_TOP_0C               REG_0C;
	union REG_CSI_CTRL_TOP_10               REG_10;
	union REG_CSI_CTRL_TOP_14               REG_14;
	union REG_CSI_CTRL_TOP_18               REG_18;
	union REG_CSI_CTRL_TOP_1C               REG_1C;
	union REG_CSI_CTRL_TOP_20               REG_20;
	union REG_CSI_CTRL_TOP_24               REG_24;
	uint32_t                                _resv_0x28[2];
	union REG_CSI_CTRL_TOP_30               REG_30;
	union REG_CSI_CTRL_TOP_34               REG_34;
	uint32_t                                _resv_0x38[2];
	union REG_CSI_CTRL_TOP_40               REG_40;
	uint32_t                                _resv_0x44[1];
	union REG_CSI_CTRL_TOP_48               REG_48;
	union REG_CSI_CTRL_TOP_4C               REG_4C;
	union REG_CSI_CTRL_TOP_50               REG_50;
	union REG_CSI_CTRL_TOP_54               REG_54;
	union REG_CSI_CTRL_TOP_58               REG_58;
	union REG_CSI_CTRL_TOP_5C               REG_5C;
	union REG_CSI_CTRL_TOP_60               REG_60;
	union REG_CSI_CTRL_TOP_64               REG_64;
	uint32_t                                _resv_0x68[2];
	union REG_CSI_CTRL_TOP_70               REG_70;
	union REG_CSI_CTRL_TOP_74               REG_74;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_SENSOR_MAC_VI_T {
	union REG_SENSOR_MAC_VI_00              REG_00;
	uint32_t                                _resv_0x4[3];
	union REG_SENSOR_MAC_VI_10              REG_10;
	union REG_SENSOR_MAC_VI_14              REG_14;
	uint32_t                                _resv_0x18[2];
	union REG_SENSOR_MAC_VI_20              REG_20;
	union REG_SENSOR_MAC_VI_24              REG_24;
	union REG_SENSOR_MAC_VI_28              REG_28;
	union REG_SENSOR_MAC_VI_2C              REG_2C;
	union REG_SENSOR_MAC_VI_30              REG_30;
	union REG_SENSOR_MAC_VI_34              REG_34;
	union REG_SENSOR_MAC_VI_38              REG_38;
	union REG_SENSOR_MAC_VI_3C              REG_3C;
	union REG_SENSOR_MAC_VI_40              REG_40;
	union REG_SENSOR_MAC_VI_44              REG_44;
	union REG_SENSOR_MAC_VI_48              REG_48;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_BLOCKS_CSI_MAC_H_ */

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: reg_blocks_csi_mac.h
 * Description:
 */

#ifndef _REG_BLOCKS_CSI_MAC_H_
#define _REG_BLOCKS_CSI_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_SENSOR_MAC_1C4D_T {
	union REG_SENSOR_MAC_1C4D_00            REG_00;
	uint32_t                                _resv_0x4[3];
	union REG_SENSOR_MAC_1C4D_10            REG_10;
	union REG_SENSOR_MAC_1C4D_14            REG_14;
	union REG_SENSOR_MAC_1C4D_18            REG_18;
	union REG_SENSOR_MAC_1C4D_1C            REG_1C;
	union REG_SENSOR_MAC_1C4D_20            REG_20;
	union REG_SENSOR_MAC_1C4D_24            REG_24;
	union REG_SENSOR_MAC_1C4D_28            REG_28;
	uint32_t                                _resv_0x2c[1];
	union REG_SENSOR_MAC_1C4D_30            REG_30;
	union REG_SENSOR_MAC_1C4D_34            REG_34;
	uint32_t                                _resv_0x38[2];
	union REG_SENSOR_MAC_1C4D_40            REG_40;
	union REG_SENSOR_MAC_1C4D_44            REG_44;
	union REG_SENSOR_MAC_1C4D_48            REG_48;
	union REG_SENSOR_MAC_1C4D_4C            REG_4C;
	union REG_SENSOR_MAC_1C4D_50            REG_50;
	union REG_SENSOR_MAC_1C4D_54            REG_54;
	union REG_SENSOR_MAC_1C4D_58            REG_58;
	uint32_t                                _resv_0x5c[1];
	union REG_SENSOR_MAC_1C4D_60            REG_60;
	union REG_SENSOR_MAC_1C4D_64            REG_64;
	union REG_SENSOR_MAC_1C4D_68            REG_68;
	union REG_SENSOR_MAC_1C4D_6C            REG_6C;
	union REG_SENSOR_MAC_1C4D_70            REG_70;
	union REG_SENSOR_MAC_1C4D_74            REG_74;
	uint32_t                                _resv_0x78[2];
	union REG_SENSOR_MAC_1C4D_80            REG_80;
	union REG_SENSOR_MAC_1C4D_84            REG_84;
	union REG_SENSOR_MAC_1C4D_88            REG_88;
	union REG_SENSOR_MAC_1C4D_8C            REG_8C;
	union REG_SENSOR_MAC_1C4D_90            REG_90;
	union REG_SENSOR_MAC_1C4D_94            REG_94;
	union REG_SENSOR_MAC_1C4D_98            REG_98;
	union REG_SENSOR_MAC_1C4D_9C            REG_9C;
	union REG_SENSOR_MAC_1C4D_A0            REG_A0;
	union REG_SENSOR_MAC_1C4D_A4            REG_A4;
	union REG_SENSOR_MAC_1C4D_A8            REG_A8;
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

#ifdef __cplusplus
}
#endif

#endif /* _REG_BLOCKS_CSI_MAC_H_ */
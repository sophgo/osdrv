/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_dwa.h
 * Description:
 */

#ifndef _U_CVI_VIP_DWA_H_
#define _U_CVI_VIP_DWA_H_

#ifdef __cplusplus
	extern "C" {
#endif

enum cvi_dwa_op {
	CVI_DWA_OP_NONE,
	CVI_DWA_OP_XY_FLIP,
	CVI_DWA_OP_ROT_90,
	CVI_DWA_OP_ROT_270,
	CVI_DWA_OP_LDC,
	CVI_DWA_OP_MAX,
};

#ifdef __cplusplus
}
#endif

#endif	/* _U_CVI_VIP_DWA_H_ */

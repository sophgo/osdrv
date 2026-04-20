/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: motionsensor_cmd.h
 * Description: motionsensor user space command related code
 *
 */
#ifndef MOTIONSENSOR_CMD_H_
#define MOTIONSENSOR_CMD_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* end of #ifdef __cplusplus */

typedef enum {
	MOTIONSENSOR_NR_INIT = 0,
	MOTIONSENSOR_NR_DEINIT,
	MOTIONSENSOR_NR_START,
	MOTIONSENSOR_NR_STOP,
	MOTIONSENSOR_NR_GET_DATA,
	MOTIONSENSOR_NR_ZERO_CALIB,
	MOTIONSENSOR_NR_BUTT
} motionsensor_nr;

#define MSENSOR_TYPE 'g'
#define MSENSOR_CMD_INIT        _IOW(MSENSOR_TYPE, MOTIONSENSOR_NR_INIT, msensor_param)
#define MSENSOR_CMD_DEINIT      _IO(MSENSOR_TYPE, MOTIONSENSOR_NR_DEINIT)
#define MSENSOR_CMD_START       _IO(MSENSOR_TYPE, MOTIONSENSOR_NR_START)
#define MSENSOR_CMD_STOP        _IO(MSENSOR_TYPE, MOTIONSENSOR_NR_STOP)
#define MSENSOR_CMD_GET_DATA    _IOR(MSENSOR_TYPE, MOTIONSENSOR_NR_GET_DATA, msensor_data_info)
#define MSENSOR_CMD_ZERO_CALIB  _IO(MSENSOR_TYPE, MOTIONSENSOR_NR_ZERO_CALIB)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* end of #ifdef __cplusplus */

#endif


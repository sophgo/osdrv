/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: motionsensor_common.h
 * Description: motionsensor common related code
 *
 */
#ifndef MOTIONSENSOR_COMMON_H_
#define MOTIONSENSOR_COMMON_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define MSENSOR_GRADIENT            (0x1 << 10)

#define MSENSOR_TEMP_GYRO   0x1
#define MSENSOR_TEMP_ACC    0x2
#define MSENSOR_TEMP_MAGN   0x4
#define MSENSOR_TEMP_ALL    0x7

#define MSENSOR_DEVICE_GYRO 0x1
#define MSENSOR_DEVICE_ACC  0x2
#define MSENSOR_DEVICE_MAGN 0x4
#define MSENSOR_DEVICE_ALL  0x7

typedef struct {
	u32  device_mask;
	u32  temperature_mask;
} msensor_attr;

typedef struct {
	u64 phys_addr;
	u64 virt_addr;
	u32 buf_len;
} msensor_buf_attr;

typedef struct {
	s32 x;
	s32 y;
	s32 z;
	s32 temperature;
	s64 pts;
} msensor_sample_data_32bit;

typedef struct {
	s16 x;
	s16 y;
	s16 z;
	s16 temperature; // x 100
	s64 pts;
} msensor_sample_data_16bit;

typedef struct {
	u32 odr; // sample rate
	u32 fsr; // data range
	u8  data_width;
	s32  temperature_max;
	s32  temperature_min;
} msensor_gyro_config;

typedef struct {
	u32 odr; // sample rate
	u32 fsr; // data range
	u8  data_width;
	s32  temperature_max;
	s32  temperature_min;
} msensor_acc_config;

typedef struct {
	u32 odr;
	u32 fsr;
	u8  data_width;
	s32  temperature_max;
	s32  temperature_min;
} msensor_magn_config;

typedef struct {
	msensor_gyro_config  gyro_config;
	msensor_acc_config   acc_config;
	msensor_magn_config  magn_config;
} msensor_config;

typedef struct {
	msensor_buf_attr buf_attr;
	msensor_config   config;
	msensor_attr     attr;
} msensor_param;

typedef enum {
	MSENSOR_DATA_GYRO = 0,
	MSENSOR_DATA_ACC,
	MSENSOR_DATA_MAGN,
	MSENSOR_DATA_BUTT
} msensor_data_type;

typedef struct { // cyclic buffer
	u16 id;
	msensor_data_type  data_type;
	u32 buf_addr_offset;
	u32 buf_len;
	u16 element_num;
	u16 begin_index;  // sample data index
	u16 end_index;
} msensor_data_info;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* end of #ifdef __cplusplus */

#endif


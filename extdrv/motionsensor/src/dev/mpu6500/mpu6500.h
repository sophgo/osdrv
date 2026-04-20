/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: mpu6500.h
 * Description: mpu6500 motionsensor driver related code
 *
 */

#ifndef MPU6500_H_
#define MPU6500_H_

/* compatible: mpu9250 */

#include "motionsensor.h"
#include "motionsensor_common.h"
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>

#define REG_WHO_AM_I (0x75)

#define MSENSOR_FIFO_LEN    (512)

#define REG_GYRO_X_OFFS_H   (0X13) // 19
#define REG_GYRO_X_OFFS_L   (0X14) // 20
#define REG_GYRO_Y_OFFS_H   (0X15) // 21
#define REG_GYRO_Y_OFFS_L   (0X16) // 22
#define REG_GYRO_Z_OFFS_H   (0X17) // 23
#define REG_GYRO_Z_OFFS_L   (0X18) // 24

#define REG_SAMPLE_RATE_DIV (0X19) // 25
#define REG_CONFIGURATION   (0x1a) // 26
#define REG_GYRO_CONFIG     (0x1b) // 27
#define REG_ACC_CONFIG_01   (0x1c) // 28
#define REG_ACC_CONFIG_02   (0x1d) // 29

#define REG_FIFO_ENABLE     (0X23) // 35

#define REG_TEMP_OUT_H      (0X41) // 65
#define REG_TEMP_OUT_L      (0x42) // 66

#define REG_USER_CTRL       (0x6a) // 106
#define REG_PWR_MGMT_1      (0x6b)  // 107
#define REG_PWR_MGMT_2      (0x6c)  // 108

#define REG_FIFO_COUNTH     (0x72) // 114
#define REG_FIFO_COUNTL     (0x73) // 115
#define REG_FIFO_R_W        (0x74) // 116

#define MSENSOR_ROOM_TEMP_OFFSET_X100 (2100)
#define MSENSOR_TEMP_SENSITIVITY_X100 (33387)

#define MSENSOR_DEV_I2C_NODE "/dev/i2c-1"
#define MSENSOR_DEV_I2C_ADDR (0x68) // 7bit addr, use i2cdetect

typedef struct {
	msensor_gyro_config gyro_config;
	u32 band_width;
} msensor_gyro_status;

typedef struct {
	msensor_acc_config acc_config;
	u32 band_width;
} msensor_acc_status;

typedef struct {
	msensor_acc_status acc_status;
	msensor_gyro_status gyro_status;

	u8 fifo_en;
	u8 flag_acc_fifo_enabled;
	u8 flag_gyro_fifo_enabled;
	u8 *fifo_buf;
	u32 fifo_element_len;

	triger_config triger_data;

	struct hrtimer hrtimer;
	atomic_t workqueue_call_times;

	struct workqueue_struct *workqueue;
	struct work_struct work;
	struct mutex work_mutex; /* Mutex for work queue */

	struct mutex data_mutex; /* Mutex for data access */
	u8 data_buf_index;

	msensor_sample_data_16bit *gyro_data_buf[2];
	u32 gyro_data_index;
	u32 gyro_data_num;
	u32 gyro_data_num_max;

	msensor_sample_data_16bit *acc_data_buf[2];
	u32 acc_data_index;
	u32 acc_data_num;
	u32 acc_data_num_max;

	s32 ideal_time_interval;
	s32 time_interval;

	s64 last_pts;
	s64 now_pts;

	s64 temp_update_pts;
	s16 temperature;

	msensor_buf_attr buf_attr;

	s16 gyro_zero_calib_x;
	s16 gyro_zero_calib_y;
	s16 gyro_zero_calib_z;
} msensor_dev_info;

#endif


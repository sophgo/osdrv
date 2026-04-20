/* SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: motionsensor.h
 * Description: motionsensor header file
 *
 */
#ifndef MOTIONSENSOR_H_
#define MOTIONSENSOR_H_

#include <linux/cdev.h>

#define DATA_RATE_RESERVED 0x00
#define DATA_RATE_25HZ     0x06
#define DATA_RATE_50HZ     0x07
#define DATA_RATE_100HZ    0x08
#define DATA_RATE_200HZ    0x09
#define DATA_RATE_400HZ    0x0A
#define DATA_RATE_800HZ    0x0B
#define DATA_RATE_1600HZ   0x0C
#define DATA_RATE_3200HZ   0x0D

#define GYRO_FULL_SCALE_RANGE_250DPS (250 << 10)
#define GYRO_FULL_SCALE_RANGE_500DPS (500 << 10)
#define GYRO_FULL_SCALE_RANGE_1KDPS  (1000 << 10)
#define GYRO_FULL_SCALE_RANGE_2KDPS  (2000 << 10)
#define GYRO_FULL_SCALE_RANGE_31DPS  ((3125 << 10) / 100)
#define GYRO_FULL_SCALE_RANGE_62DPS  ((625 << 10) / 10)
#define GYRO_FULL_SCALE_RANGE_125DPS (125 << 10)

#define ACCEL_UI_FULL_SCALE_RANGE_2G  1
#define ACCEL_UI_FULL_SCALE_RANGE_4G  2
#define ACCEL_UI_FULL_SCALE_RANGE_8G  3
#define ACCEL_UI_FULL_SCALE_RANGE_16G 4

#define ACCEL_OIS_FULL_SCALE_RANGE_1G 5
#define ACCEL_OIS_FULL_SCALE_RANGE_2G 6
#define ACCEL_OIS_FULL_SCALE_RANGE_4G 7
#define ACCEL_OIS_FULL_SCALE_RANGE_8G 8

#define MOTIONSENSOR_MAX_TEMP 85
#define MOTIONSENSOR_MIN_TEMP (-40)

typedef enum {
	GYRO_OUTPUT_DATA_RATE_32KHZ = 32000,
	GYRO_OUTPUT_DATA_RATE_8KHZ = 8000,
	GYRO_OUTPUT_DATA_RATE_3200HZ = 3200,
	GYRO_OUTPUT_DATA_RATE_1600HZ = 1600,
	GYRO_OUTPUT_DATA_RATE_800HZ = 800,
	GYRO_OUTPUT_DATA_RATE_400HZ = 400,
	GYRO_OUTPUT_DATA_RATE_200HZ = 200,
	GYRO_OUTPUT_DATA_RATE_100HZ = 100,
	GYRO_OUTPUT_DATA_RATE_50HZ = 50,
	GYRO_OUTPUT_DATA_RATE_25HZ = 25,
	GYRO_OUTPUT_DATA_RATE_UNDER_1KHZ,
	GYRO_OUTPUT_DATA_RATE_BUTT
} msensor_gyro_output_data_rate;

typedef enum {
	GYRO_BAND_WIDTH_5HZ,
	GYRO_BAND_WIDTH_10HZ,
	GYRO_BAND_WIDTH_20HZ,
	GYRO_BAND_WIDTH_41HZ,
	GYRO_BAND_WIDTH_92HZ,
	GYRO_BAND_WIDTH_184HZ,
	GYRO_BAND_WIDTH_250HZ,
	GYRO_BAND_WIDTH_3600HZ,
	GYRO_BAND_WIDTH_8800HZ,
	GYRO_BAND_WIDTH_BUTT
} msensor_gyro_band_width;

typedef enum {
	ACCEL_OUTPUT_DATA_RATE_4KHZ = 4000,
	ACCEL_OUTPUT_DATA_RATE_1KHZ = 1000,
	ACCEL_OUTPUT_DATA_RATE_1600HZ = 1600,
	ACCEL_OUTPUT_DATA_RATE_800HZ = 800,
	ACCEL_OUTPUT_DATA_RATE_400HZ = 400,
	ACCEL_OUTPUT_DATA_RATE_200HZ = 200,
	ACCEL_OUTPUT_DATA_RATE_100HZ = 100,
	ACCEL_OUTPUT_DATA_RATE_50HZ = 50,
	ACCEL_OUTPUT_DATA_RATE_25HZ = 25,
	ACCEL_OUTPUT_DATA_RATE_UNDER_1KHZ,
	ACCEL_OUTPUT_DATA_RATE_BUTT
} msensor_accel_output_data_rate;

typedef enum {
	GYRO_FULL_SCALE_SET_250DPS = 250 << 10,         /* 250 dps */
	GYRO_FULL_SCALE_SET_500DPS = 500 << 10,         /* 500 dps */
	GYRO_FULL_SCALE_SET_1KDPS = 1000 << 10,         /* 1000 dps */
	GYRO_FULL_SCALE_SET_2KDPS = 2000 << 10,         /* 2000 dps */
	GYRO_FULL_SCALE_SET_31DPS = (3125 << 10) / 100, /* 31.25 dps : 3125 / 100 */
	GYRO_FULL_SCALE_SET_62DPS = (625 << 10) / 10,   /* 62.5 dps : 625 / 10 */
	GYRO_FULL_SCALE_SET_125DPS = 125 << 10,         /* 125 dps */
	GYRO_FULL_SCALE_SET_15DPS = 15 << 10,           /* 15 dps */
	GYRO_FULL_SCALE_SET_BUTT
} msensor_gyro_full_scale_range;

typedef enum {
	ACCEL_UI_FULL_SCALE_SET_2G = 2 << 10,
	ACCEL_UI_FULL_SCALE_SET_4G = 4 << 10,
	ACCEL_UI_FULL_SCALE_SET_8G = 8 << 10,
	ACCEL_UI_FULL_SCALE_SET_16G = 16 << 10,
	ACCEL_UI_FULL_SCALE_SET_BUTT
} msensor_accel_ui_full_scale_range;

typedef enum {
	ACCEL_OIS_FULL_SCALE_SET_1G = 1,
	ACCEL_OIS_FULL_SCALE_SET_2G = 2,
	ACCEL_OIS_FULL_SCALE_SET_4G = 4,
	ACCEL_OIS_FULL_SCALE_SET_8G = 8,
	ACCEL_OIS_FULL_SCALE_SET_BUTT
} msensor_accel_ois_full_scale_range;

typedef enum {
	ACCEL_BAND_WIDTH_5HZ,
	ACCEL_BAND_WIDTH_10HZ,
	ACCEL_BAND_WIDTH_21HZ,
	ACCEL_BAND_WIDTH_44HZ,
	ACCEL_BAND_WIDTH_99HZ,
	ACCEL_BAND_WIDTH_218HZ,
	ACCEL_BAND_WIDTH_420HZ,
	ACCEL_BAND_WIDTH_1046HZ,
	ACCEL_BAND_WIDTH_BUTT
} msensor_accel_band_width;

typedef enum {
	TRIGER_TIMER = 0,
	TRIGER_EXTERN_INTERRUPT,
	TRIGER_BUTT
} msensor_triger_mode;

typedef struct {
	u32 interval; /* unit :us */
} timer_param;

typedef struct {
	u32 interrupt_num;
} extern_interrupt_param;

typedef union {
	timer_param timer_config;
	extern_interrupt_param extern_interrupt_config;
} msensor_triger_info;

typedef struct {
	msensor_triger_mode triger_mode;
	msensor_triger_info triger_info;
} triger_config;

typedef struct {
	int cmd;
	int (*func)(unsigned long arg);
} msensor_ioctl_info;

typedef struct {
	int major;
	int minor;
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct proc_dir_entry *proc_dir;
} msensor_dev;

#endif


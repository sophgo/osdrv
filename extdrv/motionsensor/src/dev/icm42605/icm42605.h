/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: icm42605.h
 * Description: icm42605 motionsensor driver related code
 *
 */

#ifndef ICM42605_H_
#define ICM42605_H_

/* ICM42605 Motion Sensor Driver */

#include "motionsensor.h"
#include "motionsensor_common.h"
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

/* Device Identification */
#define REG_WHO_AM_I                 0x75  // 117
#define ICM42605_ID                  0x42  // 66

#define MSENSOR_DEV_I2C_NODE "/dev/i2c-1"
#define GPIO_IRQ_NAME "icm42605_fifo_irq"

#define MSENSOR_DEV_I2C_ADDR (0x69) // 7bit addr, use i2cdetect
#define GPIO_NUM              (391) // EVB pin-num 103
#define MSENSOR_DEV_I3C_EN    0x1F	// b'00011111

/* FIFO Configuration */
#define MSENSOR_FIFO_LEN             (2080)
#define FIFO_THR_PACKETS              (10)

// FIFO Packet Sizes
#define FIFO_PACKET_SIZE_ACCEL_ONLY  8     // Packet with only accelerometer data
#define FIFO_PACKET_SIZE_GYRO_ONLY   8     // Packet 2: Gyro + Temp
#define FIFO_PACKET_SIZE_ACCEL_GYRO  16    // Packet 3: Accel + Gyro + Temp + Timestamp

#define CALIB_NUM (128)

/* Scale Factors */
#define GYR_SSL                      32.8f  // 32.8
#define ACC_SSL                      0.244f  // 0.244

/* Temperature Constants */
#define MSENSOR_ROOM_TEMP_OFFSET_X100 (2100)
#define MSENSOR_TEMP_SENSITIVITY_X100 (33387)

/* Configuration Bits */
#define BIT_SOFT_RESET_CHIP_CONFIG   0x01  // 1
#define BIT_SPI_MODE                 0x10  // 16
#define BIT_ACCEL_MODE_LN            0x03  // b'00000011
#define BIT_GYRO_MODE_LN             0x0C  // b'00001100
#define BIT_GYRO_MODE_STANDBY        0x04  // b'00000100
#define BIT_ACCEL_UI_FS_SEL_8G      0x20  // 32
#define BIT_GYRO_UI_FS_SEL_1000DPS  0x20  // 32
#define BIT_ACCEL_ODR_50HZ           0x09  // 9
#define BIT_GYRO_ODR_50HZ            0x09  // 9
#define BIT_FIFO_MODE_STREAM         0x40  // b'01000000
#define BIT_FIFO_ACCEL_EN            0x01  // 1
#define BIT_FIFO_GYRO_EN             0x02  // 2
#define BIT_FIFO_TEMP_EN             0x04  // 4
#define BIT_FIFO_TMST_FSYNC_EN       0x08  // 8
#define BIT_FIFO_CNT_REC		     0x40  // b'01000000
#define BIT_FIFO_RESUME_PARTIAL_EN   0x40  // b'01000000
#define BIT_FIFO_FLUSH               0x02  // b'00000010

// FIFO Header Bit Definitions
#define FIFO_HEADER_EMPTY              0x80  // Bit 7: 1=FIFO empty, 0=Packet contains sensor data
#define FIFO_HEADER_ACCEL            0x40  // Bit 6: 1=Packet contains accel data, 0=No accel data
#define FIFO_HEADER_GYRO             0x20  // Bit 5: 1=Packet contains gyro data, 0=No gyro data
#define FIFO_HEADER_TIMESTAMP_FSYNC  0x0C  // Bits 3:2: 00=No timestamp/FSYNC, 10=ODR Timestamp, 11=FSYNC time
#define FIFO_HEADER_ODR_ACCEL        0x02  // Bit 1: 1=Accel ODR different from previous packet
#define FIFO_HEADER_ODR_GYRO         0x01  // Bit 0: 1=Gyro ODR different from previous packet

#define FIFO_WM_GT_TH_EN                0x20  // b'00100000
#define INT_ASYNC_RESET                 0x10  // b'00010000
#define FIFO_THS_INT1_EN                0x04  // b'00000100
#define INT1_LATCH                      0x04  // b'00000100
#define INT1_PUSHPULL                   0x02  // b'00000010
#define INT1_ACTIVE_HIGH                0x01  // b'00000001
#define FIFO_THS_INT_CLEAR_ON_RD        0x01  // b'00001000

/* Temperature Registers */
#define REG_TEMP_OUT_H               0x1D  // 29
#define REG_TEMP_OUT_L               0x1E  // 30

/* Accelerometer Data Registers */
#define REG_ACCEL_XOUT_H             0x1F  // 31
#define REG_ACCEL_XOUT_L             0x20  // 32
#define REG_ACCEL_YOUT_H             0x21  // 33
#define REG_ACCEL_YOUT_L             0x22  // 34
#define REG_ACCEL_ZOUT_H             0x23  // 35
#define REG_ACCEL_ZOUT_L             0x24  // 36

/* Gyroscope Data Registers */
#define REG_GYRO_XOUT_H              0x25  // 37
#define REG_GYRO_XOUT_L              0x26  // 38
#define REG_GYRO_YOUT_H              0x27  // 39
#define REG_GYRO_YOUT_L              0x28  // 40
#define REG_GYRO_ZOUT_H              0x29  // 41
#define REG_GYRO_ZOUT_L              0x2A  // 42

/* Bank Selection and Configuration */
#define REG_BANK_SEL                 0x76  // 118
#define REG_DEVICE_CONFIG            0x11  // 17

/* Interface Configuration */
#define REG_INTF_CONFIG0             0x4C  // 76
#define REG_INTF_CONFIG6             0x7C  // 124

/* Power Management */
#define REG_PWR_MGMT0                0x4E  // 78

/* Gyroscope Configuration */
#define REG_GYRO_CONFIG0             0x4F  // 79
/* Accelerometer Configuration */
#define REG_ACCEL_CONFIG0            0x50  // 80

/* Interrupt Configuration */
#define REG_INT_SOURCE0              0x65  // 101
#define REG_INT_CONFIG               0x14  // 20
#define REG_INT_CONFIG0              0x63  // 99
#define REG_INT_CONFIG1              0x64  // 100
#define REG_INT_STATUS               0x2D  // 45

/* FIFO Configuration */
#define REG_FIFO_CONFIG              0x16  // 22
#define REG_FIFO_CONFIG1             0x5F  // 95
#define REG_FIFO_WM_L                0x60  // 96
#define REG_FIFO_WM_H                0x61  // 97
/* FIFO Data and Count */
#define REG_FIFO_BYTE_COUNT_H        0x2E  // 46
#define REG_FIFO_BYTE_COUNT_L        0x2F  // 47
#define REG_FIFO_DATA                0x30  // 48

#define REG_SIGNAL_PATH_RESET        0x4B  // 75

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
	u32 fifo_num_packets;

	triger_config triger_data;

	struct hrtimer hrtimer;
	atomic_t workqueue_call_times;

	struct workqueue_struct *workqueue;
	struct work_struct work;

	struct mutex work_mutex;  /* Mutex for work queue */
	struct mutex data_mutex;  /* Mutex for data access */
	spinlock_t irq_timing_lock;    /* Lock for FIFO access */

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

	s64 irq_timing_fifo[16];  /* Simple array-based FIFO for IRQ timestamps */
	int irq_timing_head;           /* Head index for FIFO */
	int irq_timing_tail;           /* Tail index for FIFO */
	int irq_timing_count;          /* Number of timestamps in FIFO */

	int gpio_irq_num;
	bool gpio_irq_configured;

	s64 temp_update_pts;
	s16 temperature;

	msensor_buf_attr buf_attr;

	s16 gyro_zero_calib_x;
	s16 gyro_zero_calib_y;
	s16 gyro_zero_calib_z;

	u32 device_mask;
} msensor_dev_info;

#endif

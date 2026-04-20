// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: mpu6500.c
 * Description: mpu6500 motionsensor driver related code
 *
 */

#include "mpu6500.h"
#include "i2c_dev.h"
#include "msensor_dev.h"
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/types.h>

static msensor_dev_info * g_dev_info;

static int msensor_transfer_read(u8 reg_addr, u8 *reg_data, u32 cnt)
{
	int ret;

	ret = i2c_dev_read(reg_addr, reg_data, cnt);
	return ret;
}

static int msensor_transfer_write(u8 reg_addr, u8 *reg_data, u32 cnt)
{
	int ret;

	ret = i2c_dev_write(reg_addr, reg_data, cnt);
	return ret;
}

/*********************************************************************************************/
static int msensor_dev_read_id(void)
{
	int ret = 0;
	u8 dev_id = 0;

	ret = msensor_transfer_read(REG_WHO_AM_I, &dev_id, 1);
	if (ret < 0) {
		pr_err("read dev failed\n");
		return -1;
	}

	pr_info("%s: I AM 0x%x\n", __func__, dev_id);

	if (dev_id == 0x00) {
		pr_err("dev id error...\n");
		ret = -1;
	}

	return ret;
}

static int gyro_param_init(msensor_param *msensor_param)
{
	int ret = 0;

	if ((MSENSOR_DEVICE_GYRO & msensor_param->attr.device_mask) == 0)
		return ret;

	msensor_param->config.acc_config.odr =
			msensor_param->config.gyro_config.odr; // !!!

	g_dev_info->gyro_status.gyro_config = msensor_param->config.gyro_config;

	g_dev_info->gyro_status.gyro_config.odr /= MSENSOR_GRADIENT;
	g_dev_info->gyro_status.gyro_config.data_width = 16; // 16 bit

	/* keep 0 < dlpf_cfg < 7 */
	if (g_dev_info->gyro_status.gyro_config.odr <= 100) { /* 100 odr */
		g_dev_info->gyro_status.band_width = GYRO_BAND_WIDTH_20HZ;
	} else if (g_dev_info->gyro_status.gyro_config.odr <= 300) { /* 300 odr */
		g_dev_info->gyro_status.band_width = GYRO_BAND_WIDTH_41HZ;
	} else if (g_dev_info->gyro_status.gyro_config.odr <= 500) { /* 500 odr */
		g_dev_info->gyro_status.band_width = GYRO_BAND_WIDTH_92HZ;
	} else if (g_dev_info->gyro_status.gyro_config.odr <= 1000) { /* 1000 odr */
		g_dev_info->gyro_status.band_width = GYRO_BAND_WIDTH_184HZ;
	} else {
		pr_err("gyro param init failed, odr: %d not found...\n", g_dev_info->gyro_status.gyro_config.odr);
		return -1;
	}
	g_dev_info->gyro_status.band_width = GYRO_BAND_WIDTH_184HZ; // manually set for testing

	switch (msensor_param->config.gyro_config.fsr) {
	case GYRO_FULL_SCALE_SET_2KDPS:
	case GYRO_FULL_SCALE_SET_1KDPS:
	case GYRO_FULL_SCALE_SET_500DPS:
	case GYRO_FULL_SCALE_SET_250DPS:
	case GYRO_FULL_SCALE_SET_31DPS:
	case GYRO_FULL_SCALE_SET_62DPS:
	case GYRO_FULL_SCALE_SET_125DPS:
		g_dev_info->gyro_status.gyro_config.fsr = msensor_param->config.gyro_config.fsr;
		break;
	default:
		pr_err("gyro param init failed gyro_fsr:%d not found !\n", msensor_param->config.gyro_config.fsr);
		return -1;
	}

	return 0;
}

static int acc_param_init(msensor_param *msensor_param)
{
	int ret = 0;

	if ((MSENSOR_DEVICE_ACC & msensor_param->attr.device_mask) == 0)
		return ret;

	g_dev_info->acc_status.acc_config = msensor_param->config.acc_config;

	g_dev_info->acc_status.acc_config.odr /= MSENSOR_GRADIENT;
	g_dev_info->acc_status.acc_config.data_width = 16; // 16 bit

	/* keep 0 < dlpf_cfg < 7 */
	if (g_dev_info->acc_status.acc_config.odr <= 100) { /* 100 odr */
		g_dev_info->acc_status.band_width = ACCEL_BAND_WIDTH_21HZ;
	} else if (g_dev_info->acc_status.acc_config.odr <= 300) { /* 300 odr */
		g_dev_info->acc_status.band_width = ACCEL_BAND_WIDTH_44HZ;
	} else if (g_dev_info->acc_status.acc_config.odr <= 500) { /* 500 odr */
		g_dev_info->acc_status.band_width = ACCEL_BAND_WIDTH_99HZ;
	} else if (g_dev_info->acc_status.acc_config.odr <= 1000) { /* 1000 odr */
		g_dev_info->acc_status.band_width = ACCEL_BAND_WIDTH_218HZ;
	} else {
		pr_err("acc param init failed! acc_odr:%d not found !\n", g_dev_info->acc_status.acc_config.odr);
		return -1;
	}

	switch (msensor_param->config.acc_config.fsr) {
	case ACCEL_UI_FULL_SCALE_SET_2G:
	case ACCEL_UI_FULL_SCALE_SET_4G:
	case ACCEL_UI_FULL_SCALE_SET_8G:
	case ACCEL_UI_FULL_SCALE_SET_16G:
		g_dev_info->acc_status.acc_config.fsr =
				msensor_param->config.acc_config.fsr;
		break;
	default:
		pr_err("acc param init failed! accel_range:%d not found !\n", msensor_param->config.acc_config.fsr);
		return -1;
	}

	return 0;
}

static int msensor_param_init(msensor_param *msensor_param)
{
	int ret = 0;

	ret = gyro_param_init(msensor_param);
	if (ret < 0)
		return ret;

	ret = acc_param_init(msensor_param);
	if (ret < 0)
		return ret;

	return 0;
}

static int msensor_triger_mode_init(void)
{
	g_dev_info->triger_data.triger_mode = TRIGER_TIMER;
	g_dev_info->fifo_en = true;

	// i2c = 400khz,         512 x 9 x (1 / 400khz) = 11.52 ms
	// sample_rate = 1khz,   512 / (3 * 2 * 2) * (1 / 1khz) = 42.6 ms
	if (g_dev_info->triger_data.triger_mode == TRIGER_TIMER) {
		if (g_dev_info->gyro_status.gyro_config.odr <= 50) { /* 50 odr */
			g_dev_info->triger_data.triger_info.timer_config.interval = 1600000;
		} else if (g_dev_info->gyro_status.gyro_config.odr <= 200) { /* 200 odr */
			g_dev_info->triger_data.triger_info.timer_config.interval = 80000;
		} else if (g_dev_info->gyro_status.gyro_config.odr <= 500) { /* 500 odr */
			g_dev_info->triger_data.triger_info.timer_config.interval = 40000;
		} else if (g_dev_info->gyro_status.gyro_config.odr <= 1000) { /* 1000 odr */
			g_dev_info->triger_data.triger_info.timer_config.interval = 10000; // default: 20000
		} else {
			return -1;
		}
	} else if (g_dev_info->triger_data.triger_mode == TRIGER_EXTERN_INTERRUPT) {
		pr_err("triger mode not support yet...\n");
		return -1;
	}

	return 0;
}

static int msensor_config_power_reg(void)
{
	int ret = 0;
	u8 reg_value;

	/* reset */
	ret = msensor_transfer_read(REG_PWR_MGMT_1, &reg_value, 1);
	reg_value |= 0x80;
	pr_info("msensor rest: 0x%x\n", reg_value);
	ret = msensor_transfer_write(REG_PWR_MGMT_1, &reg_value, 1);
	msleep(100);

	ret = msensor_transfer_read(REG_PWR_MGMT_1, &reg_value, 1);
	pr_info("msensor rest end: 0x%x\n", reg_value);

	/*when using spi interface, should be set GYRO_RST ACCEL_RST TEMP_RST*/
	// set SIGNAL_PATH_RESET
	// msleep(100);

	/* enable pll */
	reg_value = 0x01;
	ret = msensor_transfer_write(REG_PWR_MGMT_1, &reg_value, 1);

	msleep(50);

	return ret;
}

static int msensor_config_gyro_reg(msensor_param *msensor_param)
{
	u8 reg_value = 0;

	if ((MSENSOR_DEVICE_GYRO & msensor_param->attr.device_mask) == 0) {
		// disable gyro
		msensor_transfer_read(REG_PWR_MGMT_2, &reg_value, 1);
		reg_value |= 0x07;
		msensor_transfer_write(REG_PWR_MGMT_2, &reg_value, 1);
		return 0;
	}

	msensor_transfer_read(REG_CONFIGURATION, &reg_value, 1);

	reg_value &= (~0x07);

	// limit max data rate 1khz
	// config DLPF reg
	switch (g_dev_info->gyro_status.band_width) {
	case GYRO_BAND_WIDTH_20HZ: // delay 9.9ms
		reg_value |= 0x04;
		break;
	case GYRO_BAND_WIDTH_41HZ: // delay 5.9ms
		reg_value |= 0x03;
		break;
	case GYRO_BAND_WIDTH_92HZ: // delay 3.9ms
		reg_value |= 0x02;
		break;
	case GYRO_BAND_WIDTH_184HZ: // delay 2.9ms
		reg_value |= 0x01;
		break;
	default:
		pr_err("band width error...\n");
		return -1;
	}

	msensor_transfer_write(REG_CONFIGURATION, &reg_value, 1);

	// config data range
	msensor_transfer_read(REG_GYRO_CONFIG, &reg_value, 1);

	reg_value &= (~(0x03 << 3));

	switch (g_dev_info->gyro_status.gyro_config.fsr) {
	case GYRO_FULL_SCALE_SET_2KDPS:
		reg_value |= (0x03 << 3);
		break;
	case GYRO_FULL_SCALE_SET_1KDPS:
		reg_value |= (0x02 << 3);
		break;
	case GYRO_FULL_SCALE_SET_500DPS:
		reg_value |= (0x01 << 3);
		break;
	case GYRO_FULL_SCALE_SET_250DPS:
		reg_value |= (0x00 << 3);
		break;
	default:
		pr_err("data range error...\n");
		return -1;
	}

	msensor_transfer_write(REG_GYRO_CONFIG, &reg_value, 1);

	return 0;
}

static int msensor_config_acc_reg(msensor_param *msensor_param)
{
	u8 reg_value = 0;

	if ((MSENSOR_DEVICE_ACC & msensor_param->attr.device_mask) == 0) {
		// disable acc
		msensor_transfer_read(REG_PWR_MGMT_2, &reg_value, 1);
		reg_value |= (0x07 << 3);
		msensor_transfer_write(REG_PWR_MGMT_2, &reg_value, 1);
		return 0;
	}

	msensor_transfer_read(REG_ACC_CONFIG_02, &reg_value, 1);

	reg_value &= (~0x07);

	// limit max data rate 1khz
	// config DLPF reg
	switch (g_dev_info->acc_status.band_width) {
	case ACCEL_BAND_WIDTH_21HZ: // 20hz delay 19.8ms
		reg_value |= 0x04;
		break;
	case ACCEL_BAND_WIDTH_44HZ: // 41hz delay 11.8ms
		reg_value |= 0x03;
		break;
	case ACCEL_BAND_WIDTH_99HZ: // 92hz delay 7.8ms
		reg_value |= 0x02;
		break;
	case ACCEL_BAND_WIDTH_218HZ: // 184hz delay 5.8ms
		reg_value |= 0x01;
		break;
	default:
		pr_err("band width error...\n");
		return -1;
	}

	msensor_transfer_write(REG_ACC_CONFIG_02, &reg_value, 1);

	msensor_transfer_read(REG_ACC_CONFIG_01, &reg_value, 1);

	reg_value &= (~(0x03 << 3));

	// config data range
	switch (g_dev_info->acc_status.acc_config.fsr) {
	case ACCEL_UI_FULL_SCALE_SET_2G:
		reg_value |= (0x00 << 3);
		break;
	case ACCEL_UI_FULL_SCALE_SET_4G:
		reg_value |= (0x01 << 3);
		break;
	case ACCEL_UI_FULL_SCALE_SET_8G:
		reg_value |= (0x02 << 3);
		break;
	case ACCEL_UI_FULL_SCALE_SET_16G:
		reg_value |= (0x03 << 3);
		break;
	default:
		pr_err("data range error...\n");
		return -1;
	}

	msensor_transfer_write(REG_ACC_CONFIG_01, &reg_value, 1);

	return 0;
}

static int msensor_config_sample_rate_reg(msensor_param *msensor_param)
{
	u8 reg_value = 0x00;
	u32 sample_rate = 0x00;

	if ((MSENSOR_DEVICE_GYRO & msensor_param->attr.device_mask) != 0)
		sample_rate = g_dev_info->gyro_status.gyro_config.odr;
	else if ((MSENSOR_DEVICE_ACC & msensor_param->attr.device_mask) != 0)
		sample_rate = g_dev_info->acc_status.acc_config.odr;
	else
		return 0;

	if (sample_rate == 0 || sample_rate > 1000 || 1000 % sample_rate > 0) {
		pr_err("sample rate error...\n");
		return -1;
	}

	g_dev_info->ideal_time_interval = (s64)(1 * 1000 * 1000 * 1000) / (s64)sample_rate; // ns

	reg_value = (1000 / sample_rate) - 1;
	// reg_value = 249;

	pr_info("set sample rate: %d, reg_value: %d\n", sample_rate, reg_value);

	msensor_transfer_write(REG_SAMPLE_RATE_DIV, &reg_value, 1);

	usleep_range(5000, 6000);

	return 0;
}

static int msensor_fifo_reset(void)
{
	u8 reg_value;

	if (!g_dev_info->fifo_en)
		return 0;

	/* reset fifo */
	msensor_transfer_read(REG_USER_CTRL, &reg_value, 1);
	reg_value |= 0x04;
	msensor_transfer_write(REG_USER_CTRL, &reg_value, 1);

	return 0;
}

static int msensor_config_fifo_reg(u32 dev_mode)
{
	int ret = 0;
	u8 reg_value;

	if (!g_dev_info->fifo_en)
		return ret;

	/* reset fifo */
	msensor_fifo_reset();

	usleep_range(1000, 2000);

	/* enable fifo */
	msensor_transfer_read(REG_USER_CTRL, &reg_value, 1);
	reg_value |= 0x40;
	msensor_transfer_write(REG_USER_CTRL, &reg_value, 1);

	msensor_transfer_read(REG_FIFO_ENABLE, &reg_value, 1);

	pr_info("dev_mode: %x\n", dev_mode);

	if (MSENSOR_DEVICE_ACC & dev_mode) {
		reg_value |= 0x08;
		g_dev_info->flag_acc_fifo_enabled = 1;
		pr_info("acc fifo enable\n");
	} else {
		g_dev_info->flag_acc_fifo_enabled = 0;
	}

	if (MSENSOR_DEVICE_GYRO & dev_mode) {
		reg_value |= 0x70;
		g_dev_info->flag_gyro_fifo_enabled = 1;
		pr_info("gyro fifo enable\n");
	} else {
		g_dev_info->flag_gyro_fifo_enabled = 0;
	}

	msensor_transfer_write(REG_FIFO_ENABLE, &reg_value, 1);

	g_dev_info->fifo_element_len = (g_dev_info->flag_acc_fifo_enabled + g_dev_info->flag_gyro_fifo_enabled) * 6;

	pr_info("fifo element len: %d bytes\n", g_dev_info->fifo_element_len);

	msensor_fifo_reset();

	usleep_range(1000, 2000);

	g_dev_info->fifo_buf = kmalloc(MSENSOR_FIFO_LEN, GFP_KERNEL);
	if (!g_dev_info->fifo_buf)
		return -ENOMEM;

	memset(g_dev_info->fifo_buf, 0, MSENSOR_FIFO_LEN);

	return 0;
}

static int msensor_buf_init(msensor_param *msensor_param)
{
	u32 data_num_temp = 0;
	u8 *buf_base = (u8 *)msensor_param->buf_attr.virt_addr;
	u32 data_num_max = msensor_param->buf_attr.buf_len / sizeof(msensor_sample_data_16bit);

	g_dev_info->data_buf_index = 0;

	g_dev_info->gyro_data_buf[0] = NULL;
	g_dev_info->gyro_data_buf[1] = NULL;
	g_dev_info->acc_data_buf[0] = NULL;
	g_dev_info->acc_data_buf[1] = NULL;

	if (msensor_param->attr.device_mask == (MSENSOR_DEVICE_GYRO | MSENSOR_DEVICE_ACC)) {
		data_num_temp = data_num_max / 2 / 2;

		g_dev_info->gyro_data_index = 0;
		g_dev_info->gyro_data_num = 0;
		g_dev_info->gyro_data_num_max = data_num_temp;
		g_dev_info->gyro_data_buf[0] = (msensor_sample_data_16bit *)buf_base;
		g_dev_info->gyro_data_buf[1] =
				(msensor_sample_data_16bit *)(buf_base + 1 * data_num_temp * sizeof(msensor_sample_data_16bit));

		g_dev_info->acc_data_index = 0;
		g_dev_info->acc_data_num = 0;
		g_dev_info->acc_data_num_max = data_num_temp;
		g_dev_info->acc_data_buf[0] =
				(msensor_sample_data_16bit *)(buf_base + 2 * data_num_temp * sizeof(msensor_sample_data_16bit));
		g_dev_info->acc_data_buf[1] =
				(msensor_sample_data_16bit *)(buf_base + 3 * data_num_temp * sizeof(msensor_sample_data_16bit));

	} else if (msensor_param->attr.device_mask == MSENSOR_DEVICE_GYRO) {
		data_num_temp = data_num_max / 2;

		g_dev_info->gyro_data_index = 0;
		g_dev_info->gyro_data_num = 0;
		g_dev_info->gyro_data_num_max = data_num_temp;
		g_dev_info->gyro_data_buf[0] = (msensor_sample_data_16bit *)buf_base;
		g_dev_info->gyro_data_buf[1] =
				(msensor_sample_data_16bit *)(buf_base + 1 * data_num_temp * sizeof(msensor_sample_data_16bit));

	} else if (msensor_param->attr.device_mask == MSENSOR_DEVICE_ACC) {
		data_num_temp = data_num_max / 2;

		g_dev_info->acc_data_index = 0;
		g_dev_info->acc_data_num = 0;
		g_dev_info->acc_data_num_max = data_num_temp;
		g_dev_info->acc_data_buf[0] = (msensor_sample_data_16bit *)buf_base;
		g_dev_info->acc_data_buf[1] =
				(msensor_sample_data_16bit *)(buf_base + 1 * data_num_temp * sizeof(msensor_sample_data_16bit));

	} else {
		pr_err("[ERR] not support device...\n");
		return -1;
	}

	g_dev_info->buf_attr = msensor_param->buf_attr;

	pr_info("data buffer init, max data number: %d, %d\n", data_num_temp, data_num_max);

	return 0;
}

int msensor_dev_init(msensor_param *msensor_param)
{
	int ret = 0;

	ret = msensor_param_init(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_triger_mode_init();
	if (ret < 0)
		return ret;

	ret = msensor_dev_read_id();
	if (ret < 0)
		return ret;

	ret = msensor_config_power_reg();
	if (ret < 0)
		return ret;

	ret = msensor_config_gyro_reg(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_config_acc_reg(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_config_sample_rate_reg(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_config_fifo_reg(msensor_param->attr.device_mask);
	if (ret < 0)
		return ret;

	ret = msensor_buf_init(msensor_param);
	if (ret < 0)
		return ret;

	return 0;
}

int msensor_dev_deinit(void)
{
	kfree(g_dev_info->fifo_buf);
	g_dev_info->fifo_buf = NULL;

	return 0;
}

/*********************************************************************************************/
int msensor_zero_calib(void)
{
	if (g_dev_info->gyro_zero_calib_x != 0 || g_dev_info->gyro_zero_calib_y != 0 || g_dev_info->gyro_zero_calib_z != 0) {
		pr_info("gyro zero calib already done\n");
	} else {
		#define __CALIB_NUM (768) // default: 32
		int i = 0;
		u16 index = 0;
		msensor_sample_data_16bit *data_base = NULL;
		msensor_sample_data_16bit *data = NULL;
		s32 x_sum = 0, y_sum = 0, z_sum = 0;
		s16 x, y, z;
		// uint8_t buffer[6];

		msensor_data_info data_info[2];

		memset(&data_info, 0, sizeof(msensor_data_info) * 2);

		msensor_get_data_info(data_info);

		if (data_info[0].data_type != MSENSOR_DATA_GYRO) {
			pr_err("[ERROR] gyro not enable...\n");
			return -1;
		}

		if (data_info[0].element_num < __CALIB_NUM) {
			pr_err("[ERROR] insufficient data for calib, ONLY %d data available.\n", data_info[0].element_num);
			return -1;
		}

		data_base = (msensor_sample_data_16bit *)(g_dev_info->buf_attr.virt_addr + data_info[0].buf_addr_offset);

		for (i = 0; i < __CALIB_NUM; i++) {
			data = data_base + index;

			pr_info("data->x: %d\n", data->x);

			x_sum += data->x;
			y_sum += data->y;
			z_sum += data->z;

			index++;

			if (index >= data_info[0].element_num)
				index = 0;
		}

		x = x_sum / __CALIB_NUM;
		y = y_sum / __CALIB_NUM;
		z = z_sum / __CALIB_NUM;

		pr_info("using %d gyro data for calibration\n", __CALIB_NUM);

		pr_info("x_sum: %d, y_sum: %d, z_sum: %d\n", x_sum, y_sum, z_sum);

		x = 0 - x;
		y = 0 - y;
		z = 0 - z;

		g_dev_info->gyro_zero_calib_x = x;
		g_dev_info->gyro_zero_calib_y = y;
		g_dev_info->gyro_zero_calib_z = z;

		pr_info("calib data: 0x%x, %d; 0x%x, %d; 0x%x, %d\n",
			g_dev_info->gyro_zero_calib_x, g_dev_info->gyro_zero_calib_x,
			g_dev_info->gyro_zero_calib_y, g_dev_info->gyro_zero_calib_y,
			g_dev_info->gyro_zero_calib_z, g_dev_info->gyro_zero_calib_z);

		// buffer[0] = (x & 0xFF00) >> 8;
		// buffer[1] = (x & 0xFF);
		// buffer[2] = (y & 0xFF00) >> 8;
		// buffer[3] = (y & 0xFF);
		// buffer[4] = (z & 0xFF00) >> 8;
		// buffer[5] = (z & 0xFF);

		// msensor_transfer_write(REG_GYRO_X_OFFS_H, buffer, 6); // !!!
	}
	return 0;
}

int msensor_get_data_info(msensor_data_info *data_info)
{
	u8 data_buf_index;

	u32 gyro_data_index;
	u32 gyro_data_num;

	u32 acc_data_index;
	u32 acc_data_num;

	mutex_lock(&g_dev_info->data_mutex); // lock !!!

	data_buf_index = g_dev_info->data_buf_index;

	if (g_dev_info->gyro_data_buf[data_buf_index]) {
		gyro_data_index = g_dev_info->gyro_data_index;
		gyro_data_num = g_dev_info->gyro_data_num;

		g_dev_info->gyro_data_index = 0; // reset
		g_dev_info->gyro_data_num = 0;
	}

	if (g_dev_info->acc_data_buf[data_buf_index]) {
		acc_data_index = g_dev_info->acc_data_index;
		acc_data_num = g_dev_info->acc_data_num;

		g_dev_info->acc_data_index = 0; // reset
		g_dev_info->acc_data_num = 0;
	}

	g_dev_info->data_buf_index =
			(g_dev_info->data_buf_index == 0) ? 1 : 0; // pingpong buffer

	mutex_unlock(&g_dev_info->data_mutex); // unlock !!!

	if (g_dev_info->gyro_data_buf[data_buf_index]) {
		data_info->id = data_buf_index;
		data_info->data_type = MSENSOR_DATA_GYRO;
		data_info->buf_addr_offset =
				((uint64_t)g_dev_info->gyro_data_buf[data_buf_index] -
				 g_dev_info->buf_attr.virt_addr);
		data_info->buf_len =
				g_dev_info->gyro_data_num_max * sizeof(msensor_sample_data_16bit);
		data_info->element_num = gyro_data_num;

		if (gyro_data_num >= g_dev_info->gyro_data_num_max) {
			data_info->begin_index = gyro_data_index;
			if (gyro_data_index == 0)
				data_info->end_index = g_dev_info->gyro_data_num_max - 1;
			else
				data_info->end_index = gyro_data_index - 1;
		} else {
			data_info->begin_index = 0;
			data_info->end_index = gyro_data_index - 1;
		}

		data_info++; // next...
	}

	if (g_dev_info->acc_data_buf[data_buf_index]) {
		data_info->id = data_buf_index;
		data_info->data_type = MSENSOR_DATA_ACC;
		data_info->buf_addr_offset =
				((uint64_t)g_dev_info->acc_data_buf[data_buf_index] -
				 g_dev_info->buf_attr.virt_addr);
		data_info->buf_len =
				g_dev_info->acc_data_num_max * sizeof(msensor_sample_data_16bit);
		data_info->element_num = acc_data_num;

		if (acc_data_num >= g_dev_info->acc_data_num_max) {
			data_info->begin_index = acc_data_index;
			if (acc_data_index == 0)
				data_info->end_index = g_dev_info->acc_data_num_max - 1;
			else
				data_info->end_index = acc_data_index - 1;
		} else {
			data_info->begin_index = 0;
			data_info->end_index = acc_data_index - 1;
		}

		data_info++; // next...
	}

	return 0;
}

static s16 msensor_read_temperature(void)
{
	u8 temp[2];
	int data = 0x0;

	msensor_transfer_read(REG_TEMP_OUT_H, temp, 2);

	data = temp[0] << 8 | temp[1];

	data = (data * 10000) / MSENSOR_TEMP_SENSITIVITY_X100 +
				 MSENSOR_ROOM_TEMP_OFFSET_X100;

	return (s16)data;
}

static void msensor_save_data(int fifo_count)
{
	int i = 0;
	u8 *buf = g_dev_info->fifo_buf;
	int num = fifo_count / g_dev_info->fifo_element_len;
	msensor_sample_data_16bit *gyro_buf_base =
			g_dev_info->gyro_data_buf[g_dev_info->data_buf_index];
	msensor_sample_data_16bit *acc_buf_base =
			g_dev_info->acc_data_buf[g_dev_info->data_buf_index];
	msensor_sample_data_16bit *gyro_buf = NULL;
	msensor_sample_data_16bit *acc_buf = NULL;

	for (i = 0; i < num; i++) {
		if (acc_buf_base && g_dev_info->flag_acc_fifo_enabled) {
			acc_buf = acc_buf_base + g_dev_info->acc_data_index;

			acc_buf->x = (buf[0] << 8) | buf[1];
			acc_buf->y = (buf[2] << 8) | buf[3];
			acc_buf->z = (buf[4] << 8) | buf[5];
			acc_buf->temperature = g_dev_info->temperature;
			acc_buf->pts = g_dev_info->last_pts + i * g_dev_info->time_interval;

			buf += 6;

			g_dev_info->acc_data_index++;
			if (g_dev_info->acc_data_index >= g_dev_info->acc_data_num_max)
				g_dev_info->acc_data_index = 0;

			if (g_dev_info->acc_data_num < g_dev_info->acc_data_num_max)
				g_dev_info->acc_data_num++;
		}

		if (gyro_buf_base && g_dev_info->flag_gyro_fifo_enabled) {
			gyro_buf = gyro_buf_base + g_dev_info->gyro_data_index;

			// pr_info("buf: %llx\n", (uint64_t) gyro_buf);

			// gyro_buf->x = (buf[0] << 8) | buf[1];
			// gyro_buf->y = (buf[2] << 8) | buf[3];
			// gyro_buf->z = (buf[4] << 8) | buf[5];
			gyro_buf->x = ((buf[0] << 8) | buf[1]) + g_dev_info->gyro_zero_calib_x;
			gyro_buf->y = ((buf[2] << 8) | buf[3]) + g_dev_info->gyro_zero_calib_y;
			gyro_buf->z = ((buf[4] << 8) | buf[5]) + g_dev_info->gyro_zero_calib_z;
			gyro_buf->temperature = g_dev_info->temperature;
			gyro_buf->pts = g_dev_info->last_pts + i * g_dev_info->time_interval;

			buf += 6;

			g_dev_info->gyro_data_index++;
			if (g_dev_info->gyro_data_index >= g_dev_info->gyro_data_num_max)
				g_dev_info->gyro_data_index = 0;

			if (g_dev_info->gyro_data_num < g_dev_info->gyro_data_num_max)
				g_dev_info->gyro_data_num++;
		}
	}
}

static void msensor_work_handler(struct work_struct *work)
{
	u8 temp[2];
	u16 fifo_count;
	s32 time_interval_temp;

	mutex_lock(&g_dev_info->work_mutex);

	g_dev_info->now_pts = ktime_get_ns();
	msensor_transfer_read(REG_FIFO_COUNTH, temp, 2);
	fifo_count = temp[0] << 8 | temp[1];

	if ((fifo_count % g_dev_info->fifo_element_len) != 0) {
		// pr_info("data align, %d, %d\n", fifo_count,
		//	g_dev_info->fifo_element_len);
		fifo_count = (fifo_count / g_dev_info->fifo_element_len) * g_dev_info->fifo_element_len;
		// pr_info("align: %d\n", fifo_count);
	}

	msensor_transfer_read(REG_FIFO_R_W, g_dev_info->fifo_buf, fifo_count);

	// pr_info("work sava data: %d\n", fifo_count);

	if (fifo_count >= MSENSOR_FIFO_LEN) {
		msensor_fifo_reset();

		g_dev_info->last_pts = ktime_get_ns();
		g_dev_info->time_interval = 0;

		pr_warn("[WARN] work fifo reset...\n");
	} else {
		time_interval_temp = (g_dev_info->now_pts - g_dev_info->last_pts) / (fifo_count / g_dev_info->fifo_element_len);

		if (g_dev_info->time_interval == 0)
			g_dev_info->time_interval = (time_interval_temp + g_dev_info->ideal_time_interval) / 2;
		else
			g_dev_info->time_interval = (time_interval_temp * 2 + g_dev_info->ideal_time_interval + g_dev_info->time_interval) / 4;

		// pr_info("time interval: %d, %d, %d\n", g_dev_info->ideal_time_interval,
		//	time_interval_temp, g_dev_info->time_interval);

		if ((g_dev_info->now_pts - g_dev_info->temp_update_pts) >= 1 * 1000 * 1000 * 1000) {
			g_dev_info->temp_update_pts = g_dev_info->now_pts;
			g_dev_info->temperature = msensor_read_temperature();
		}

		mutex_lock(&g_dev_info->data_mutex);
		msensor_save_data(fifo_count);
		mutex_unlock(&g_dev_info->data_mutex);

		g_dev_info->last_pts = g_dev_info->now_pts;
	}

	atomic_dec(&g_dev_info->workqueue_call_times);
	mutex_unlock(&g_dev_info->work_mutex);
}

int msensor_get_triger_mode(void)
{
	if (!g_dev_info)
		return -1;
	return g_dev_info->triger_data.triger_mode;
}

static enum hrtimer_restart timer_hr_interrupt(struct hrtimer *timer)
{
	ktime_t stime;

	stime = ktime_set(0, g_dev_info->triger_data.triger_info.timer_config.interval * NSEC_PER_USEC);
	hrtimer_forward_now(timer, stime);

	queue_work(g_dev_info->workqueue, &g_dev_info->work);

	if (atomic_read(&g_dev_info->workqueue_call_times) != 0)
		pr_warn("[WARN] msensor work delayed...\n");

	atomic_inc(&g_dev_info->workqueue_call_times);

	return HRTIMER_RESTART;
}

int msensor_timer_triger_run(void)
{
	ktime_t stime;

	/* init hrtimer */
	hrtimer_init(&g_dev_info->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_dev_info->hrtimer.function = timer_hr_interrupt;

	/* init workqueue */
	g_dev_info->workqueue = create_singlethread_workqueue("msensor");
	INIT_WORK(&g_dev_info->work, msensor_work_handler);

	/* start hrtimer */
	stime = ktime_set(0, g_dev_info->triger_data.triger_info.timer_config.interval * NSEC_PER_USEC);
	hrtimer_start(&g_dev_info->hrtimer, stime, HRTIMER_MODE_REL);

	pr_info("msensor timer triger run...\n");

	msensor_fifo_reset();

	g_dev_info->last_pts = ktime_get_ns();
	g_dev_info->time_interval = 0;

	return 0;
}

int msensor_timer_triger_stop(void)
{
	hrtimer_cancel(&g_dev_info->hrtimer);

	mutex_lock(&g_dev_info->work_mutex);
	// flush_workqueue(g_dev_info->workqueue);
	destroy_workqueue(g_dev_info->workqueue);
	mutex_unlock(&g_dev_info->work_mutex);

	pr_info("msensor timer triger stop...\n");

	return 0;
}

/*********************************************************************************************/
int msensor_get_debug_info(char *info, int info_len)
{
	int info_cnt = 0;
	char buf[256];

	info_cnt += snprintf(buf, sizeof(buf), "fifo info:\n");
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf),
											 "\tfifo_en: %d, acc_fifo_en: %d, gyro_fifo_en: %d\n",
											 g_dev_info->fifo_en, g_dev_info->flag_acc_fifo_enabled,
											 g_dev_info->flag_gyro_fifo_enabled);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tfifo_element_len: %d\n",
											 g_dev_info->fifo_element_len);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "data buffer info:\n");
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tgyro, index: %d, num: %d, num_max: %d\n",
							 g_dev_info->gyro_data_index, g_dev_info->gyro_data_num,
							 g_dev_info->gyro_data_num_max);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tacc, index: %d, num: %d, num_max: %d\n",
							 g_dev_info->acc_data_index, g_dev_info->acc_data_num,
							 g_dev_info->acc_data_num_max);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tbuffer length: %d\n",
											 g_dev_info->buf_attr.buf_len);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "other info:\n");
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\ttriger mode: %d, timer interval: %d us\n",
							 g_dev_info->triger_data.triger_mode,
							 g_dev_info->triger_data.triger_info.timer_config.interval);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tsample rate time interval: %d, real interval: %d\n",
							 g_dev_info->ideal_time_interval, g_dev_info->time_interval);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\ttemperature: %d\n",
											 g_dev_info->temperature);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tgyro zero calib data: %d, %d, %d\n",
							 g_dev_info->gyro_zero_calib_x, g_dev_info->gyro_zero_calib_y,
							 g_dev_info->gyro_zero_calib_z);
	strncat(info, buf, info_len - info_cnt - 1);

	pr_info("get debug info...%d, %d\n", info_len, info_cnt);

	return 0;
}

/*********************************************************************************************/
int msensor_module_init(void)
{
	int ret = 0;

	g_dev_info = kmalloc(sizeof(*g_dev_info), GFP_KERNEL);
	if (!g_dev_info)
		return -ENOMEM;

	memset(g_dev_info, 0, sizeof(msensor_dev_info));

	mutex_init(&g_dev_info->work_mutex);
	mutex_init(&g_dev_info->data_mutex);

	ret = i2c_dev_init(MSENSOR_DEV_I2C_NODE, MSENSOR_DEV_I2C_ADDR);
	if (ret < 0) {
		pr_err("i2c dev init err...\n");
		return -1;
	}

	return 0;
}

int msensor_module_deint(void)
{
	mutex_destroy(&g_dev_info->work_mutex);
	mutex_destroy(&g_dev_info->data_mutex);

	i2c_dev_deinit();

	kfree(g_dev_info);
	g_dev_info = NULL;

	return 0;
}

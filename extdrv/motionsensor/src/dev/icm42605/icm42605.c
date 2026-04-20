// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: icm42605.c
 * Description: icm42605 motionsensor driver related code
 *
 */

#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include "i2c_dev.h"
#include "icm42605.h"
#include "msensor_dev.h"

static msensor_dev_info * g_dev_info;

static int msensor_transfer_read(u8 reg_addr, u8 *reg_data, u32 cnt)
{
	int ret = 0;

	ret = i2c_dev_read(reg_addr, reg_data, cnt);
	if (ret < 0)
		pr_err("[ICM42605] i2c read failed: %d\n", ret);

	return ret;
}

static int msensor_transfer_write(u8 reg_addr, u8 *reg_data, u32 cnt)
{
	int ret = 0;

	ret = i2c_dev_write(reg_addr, reg_data, cnt);
	if (ret < 0)
		pr_err("[ICM42605] i2c write failed: %d\n", ret);

	return ret;
}

static int msensor_set_bits(u8 reg_addr, u8 bits_to_set)
{
	int ret = 0;
	u8 reg_value = 0;

	ret = msensor_transfer_read(reg_addr, &reg_value, 1);

	reg_value |= bits_to_set;

	ret = msensor_transfer_write(reg_addr, &reg_value, 1);

	return ret;
}

static int msensor_clear_bits(u8 reg_addr, u8 bits_to_clear)
{
	int ret = 0;
	u8 reg_value = 0;

	ret = msensor_transfer_read(reg_addr, &reg_value, 1);

	reg_value &= ~bits_to_clear;

	ret = msensor_transfer_write(reg_addr, &reg_value, 1);

	return ret;
}

/**************************** helper/config ***********************************/
static int gyro_param_init(msensor_param *msensor_param)
{
	// pass the gyro param from user space

	if ((msensor_param->attr.device_mask & MSENSOR_DEVICE_GYRO) == 0)
		return 0;

	g_dev_info->gyro_status.gyro_config = msensor_param->config.gyro_config;
	g_dev_info->gyro_status.gyro_config.odr /= MSENSOR_GRADIENT;
	g_dev_info->gyro_status.gyro_config.data_width = 16; // 16-bit

	return 0;
}

static int acc_param_init(msensor_param *msensor_param)
{
	// pass the acc param from user space

	if ((msensor_param->attr.device_mask & MSENSOR_DEVICE_ACC) == 0)
		return 0;

	g_dev_info->acc_status.acc_config = msensor_param->config.acc_config;
	g_dev_info->acc_status.acc_config.odr /= MSENSOR_GRADIENT;
	g_dev_info->acc_status.acc_config.data_width = 16; // 16-bit

	return 0;
}

static int msensor_param_init(msensor_param *msensor_param)
{
	int ret;

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
	g_dev_info->triger_data.triger_mode = TRIGER_EXTERN_INTERRUPT;
	g_dev_info->fifo_en = true;

	if (g_dev_info->triger_data.triger_mode == TRIGER_TIMER) {
		if (g_dev_info->gyro_status.gyro_config.odr <= 50)
			g_dev_info->triger_data.triger_info.timer_config.interval = 1600000;
		else if (g_dev_info->gyro_status.gyro_config.odr <= 200)
			g_dev_info->triger_data.triger_info.timer_config.interval = 80000;
		else if (g_dev_info->gyro_status.gyro_config.odr <= 500)
			g_dev_info->triger_data.triger_info.timer_config.interval = 40000;
		else if (g_dev_info->gyro_status.gyro_config.odr <= 1000)
			g_dev_info->triger_data.triger_info.timer_config.interval = 10000;
		else
			return -1;
		pr_info("[ICM42605] Timer trigger mode configured\n");
	} else if (g_dev_info->triger_data.triger_mode == TRIGER_EXTERN_INTERRUPT) {
		pr_info("[ICM42605] External interrupt trigger mode configured\n");
	} else {
		pr_err("[ICM42605] Unknown trigger mode: %d\n", g_dev_info->triger_data.triger_mode);
		return -1;
	}

	return 0;
}

static int msensor_soft_reset(void)
{
	int ret;

	ret = msensor_set_bits(REG_DEVICE_CONFIG, BIT_SOFT_RESET_CHIP_CONFIG);
	if (ret < 0) {
		pr_err("[ICM42605] soft reset chip config failed\n");
		return ret;
	}
	msleep(1000);	// 1s
	return 0;
}

static int msensor_sel_bank(u8 bank_sel)
{
	int ret;
	u8 reg_val;

	// Read current value of REG_BANK_SEL
	ret = msensor_transfer_read(REG_BANK_SEL, &reg_val, 1);
	if (ret < 0) {
		pr_err("[ICM42605] read register bank failed\n");
		return ret;
	}

	// Set only the 3 LSBs to bank_sel, preserve other bits
	reg_val = (reg_val & ~0x07) | (bank_sel & 0x07);

	ret = msensor_transfer_write(REG_BANK_SEL, &reg_val, 1);
	if (ret < 0) {
		pr_err("[ICM42605] select register bank failed\n");
		return ret;
	}

	return 0;
}

static int msensor_config_intf_reg(void)
{
	int ret;

	msensor_sel_bank(1);
	ret = msensor_clear_bits(REG_INTF_CONFIG6, MSENSOR_DEV_I3C_EN);
	if (ret < 0) {
		pr_err("[ICM42605] disable i3c failed\n");
		return ret;
	}
	msensor_sel_bank(0);  // always select bank 0
	usleep_range(1000, 2000);

	return 0;
}

static int msensor_config_power_reg(void)
{
	int ret;

	// Set gyro to low noise mode
	ret = msensor_set_bits(REG_PWR_MGMT0, BIT_GYRO_MODE_LN);
	if (ret < 0) {
		pr_err("[ICM42605] set gyro to low noise mode failed\n");
		return ret;
	}
	msleep(1000);
	return 0;
}

static uint8_t icm_map_gyro_fsr_bits(uint32_t fsr)
{
	switch (fsr) {
	case GYRO_FULL_SCALE_SET_2KDPS:
	return 0x00; // 2000 dps
	case GYRO_FULL_SCALE_SET_1KDPS:
	return 0x20; // 1000 dps
	case GYRO_FULL_SCALE_SET_500DPS:
	return 0x40; // 500 dps
	case GYRO_FULL_SCALE_SET_250DPS:
	return 0x60; // 250 dps
	case GYRO_FULL_SCALE_SET_125DPS:
	return 0x80; // 125 dps
	case GYRO_FULL_SCALE_SET_62DPS:
	return 0xA0; // 62 dps
	case GYRO_FULL_SCALE_SET_31DPS:
	return 0xC0; // 31 dps
	case GYRO_FULL_SCALE_SET_15DPS:
	return 0xE0; // 15 dps
	default:
	return 0x00; // default 2000 dps if unsupported
	}
}

static uint8_t icm_map_acc_fsr_bits(uint32_t fsr)
{
	switch (fsr) {
	case ACCEL_UI_FULL_SCALE_SET_16G:
	return 0x00; // 16g
	case ACCEL_UI_FULL_SCALE_SET_8G:
	return 0x20; // 8g
	case ACCEL_UI_FULL_SCALE_SET_4G:
	return 0x40; // 4g
	case ACCEL_UI_FULL_SCALE_SET_2G:
	return 0x60; // 2g
	default:
	return 0x00; // default 16g
	}
}

static uint8_t icm_map_odr_bits(uint32_t odr)
{
	switch (odr) {
	case 8000:
	return 0x03; // 8000Hz
	case 4000:
	return 0x04; // 4000Hz
	case 2000:
	return 0x05; // 2000Hz
	case 1000:
	return 0x06; // 1000Hz
	case 500:
	return 0x0F; // 500Hz
	case 200:
	return 0x07; // 200Hz
	case 100:
	return 0x08; // 100Hz
	case 50:
	return 0x09; // 50Hz
	case 25:
	return 0x0A; // 25Hz
	case 12:       // 12.5Hz rounded down to 12
	return 0x0B; // 12.5Hz
	default:
	return 0x06; // default 1000Hz
	}
}

static int msensor_config_gyro_reg(msensor_param *msensor_param)
{
	int ret;
	u8 reg_value = 0;

	if ((msensor_param->attr.device_mask & MSENSOR_DEVICE_GYRO) == 0)
		return 0;

	ret = msensor_transfer_read(REG_GYRO_CONFIG0, &reg_value, 1);
	if (ret < 0)
		return ret;

	// FS_SEL in upper bits, ODR in lower bits
	reg_value &= 0x00; // clear
	reg_value |= (icm_map_gyro_fsr_bits(g_dev_info->gyro_status.gyro_config.fsr));
	reg_value |= icm_map_odr_bits(g_dev_info->gyro_status.gyro_config.odr);

	ret = msensor_transfer_write(REG_GYRO_CONFIG0, &reg_value, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int msensor_config_acc_reg(msensor_param *msensor_param)
{
	int ret;
	u8 reg_value = 0;

	if ((msensor_param->attr.device_mask & MSENSOR_DEVICE_ACC) == 0)
		return 0;

	ret = msensor_transfer_read(REG_ACCEL_CONFIG0, &reg_value, 1);
	if (ret < 0)
		return ret;

	// FS_SEL in upper bits, ODR in lower bits
	reg_value &= 0x00; // clear
	reg_value |= icm_map_acc_fsr_bits(g_dev_info->acc_status.acc_config.fsr);
	reg_value |= icm_map_odr_bits(g_dev_info->acc_status.acc_config.odr);

	ret = msensor_transfer_write(REG_ACCEL_CONFIG0, &reg_value, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int msensor_config_sample_rate(msensor_param *msensor_param)
{
	u32 sample_rate = 0x00;

	if ((msensor_param->attr.device_mask & MSENSOR_DEVICE_GYRO) != 0)
		sample_rate = g_dev_info->gyro_status.gyro_config.odr;
	else if ((msensor_param->attr.device_mask & MSENSOR_DEVICE_ACC) != 0)
		sample_rate = g_dev_info->acc_status.acc_config.odr;
	else
		return 0;

	if (sample_rate == 0 || sample_rate > 8000) {
		pr_err("[ICM42605] sample rate invalid: %u\n", sample_rate);
		return -1;
	}

	g_dev_info->ideal_time_interval = (int64_t)(1 * 1000 * 1000 * 1000) / (int64_t)sample_rate; // by ns

	return 0;
}

static int msensor_fifo_reset(void)
{
	int ret;

	// use FIFO flush bit in SIGNAL_PATH_RESET register to reset FIFO
	if (!g_dev_info->fifo_en)
		return 0;

	ret = msensor_set_bits(REG_SIGNAL_PATH_RESET, BIT_FIFO_FLUSH);
	if (ret < 0) {
		pr_err("[ICM42605] fifo reset failed\n");
		return ret;
	}
	// usleep_range(1000, 2000);

	return 0;
}

static int msensor_config_fifo_reg(u32 dev_mask)
{
	int ret = 0;
	u8 reg_value;

	if (!g_dev_info->fifo_en)
		return 0;

	// Set FIFO to stream mode
	ret = msensor_set_bits(REG_FIFO_CONFIG, BIT_FIFO_MODE_STREAM);
	if (ret < 0) {
		pr_err("[ICM42605] set fifo to stream mode failed\n");
		return ret;
	}

	// Enable temp/gyro/accel into FIFO as requested
	reg_value = 0x00;
	if (MSENSOR_DEVICE_ACC & dev_mask) {
		reg_value |= BIT_FIFO_ACCEL_EN;
		g_dev_info->flag_acc_fifo_enabled = 1;
	} else {
		g_dev_info->flag_acc_fifo_enabled = 0;
	}
	if (MSENSOR_DEVICE_GYRO & dev_mask) {
		reg_value |= BIT_FIFO_GYRO_EN;
		g_dev_info->flag_gyro_fifo_enabled = 1;
	} else {
		g_dev_info->flag_gyro_fifo_enabled = 0;
	}

	reg_value |= BIT_FIFO_TEMP_EN;  // enable temperature

	ret = msensor_transfer_write(REG_FIFO_CONFIG1, &reg_value, 1);
	if (ret < 0) {
		pr_err("[ICM42605] set fifo config1 failed\n");
		return ret;
	}

	// msensor_fifo_reset();  // reset fifo

	if ((dev_mask & MSENSOR_DEVICE_ACC) && (dev_mask & MSENSOR_DEVICE_GYRO)) {
		g_dev_info->fifo_element_len = FIFO_PACKET_SIZE_ACCEL_GYRO;
	} else if (dev_mask & MSENSOR_DEVICE_ACC) {
		g_dev_info->fifo_element_len = FIFO_PACKET_SIZE_ACCEL_ONLY;
	} else if (dev_mask & MSENSOR_DEVICE_GYRO) {
		g_dev_info->fifo_element_len = FIFO_PACKET_SIZE_GYRO_ONLY;
	} else {
		pr_err("[ICM42605] device mask not supported\n");
		return -1;
	}

	/* Allocate the FIFO read buffer and clear it */
	g_dev_info->fifo_buf = kmalloc(MSENSOR_FIFO_LEN, GFP_KERNEL);
	if (!g_dev_info->fifo_buf)
		return -ENOMEM;
	memset(g_dev_info->fifo_buf, 0, MSENSOR_FIFO_LEN);

	return 0;
}

static int msensor_config_fifo_interrupt(void)
{
	int ret;
	u16 fifo_wm_gt_th = g_dev_info->fifo_element_len * FIFO_THR_PACKETS;  // 10 packets
	u8 fifo_wm_l, fifo_wm_h;

	// configure fifo watermark measured in bytes
	if (fifo_wm_gt_th <= 0) {
		pr_err("[ICM42605] fifo wm gt th is invalid: %u\n", fifo_wm_gt_th);
		return -1;
	}
	fifo_wm_l = fifo_wm_gt_th & 0xFF;
	fifo_wm_h = (fifo_wm_gt_th >> 8) & 0xFF;
	ret = msensor_transfer_write(REG_FIFO_WM_L, &fifo_wm_l, 1);
	if (ret < 0) {
		pr_err("[ICM42605] set fifo wm low byte failed\n");
		return ret;
	}
	ret = msensor_transfer_write(REG_FIFO_WM_H, &fifo_wm_h, 1);
	if (ret < 0) {
		pr_err("[ICM42605] set fifo wm high byte failed\n");
		return ret;
	}

	// interrupt async reset
	ret = msensor_clear_bits(REG_INT_CONFIG1, INT_ASYNC_RESET);
	if (ret < 0) {
		pr_err("[ICM42605] clear interrupt async reset failed\n");
		return ret;
	}

	// interrupt routed to INT1
	ret = msensor_set_bits(REG_INT_SOURCE0, FIFO_THS_INT1_EN);
	if (ret < 0) {
		pr_err("[ICM42605] set interrupt routed to INT1 failed\n");
		return ret;
	}

	return ret;
}

static int msensor_buf_init(msensor_param *msensor_param)
{
	u32 data_num_temp = 0;
	u8 *buf_base = (u8 *)msensor_param->buf_attr.virt_addr;
	u32 data_num_max =
		msensor_param->buf_attr.buf_len / sizeof(msensor_sample_data_16bit);

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
			(msensor_sample_data_16bit *)(buf_base +
											1 * data_num_temp *
												sizeof(msensor_sample_data_16bit));

		g_dev_info->acc_data_index = 0;
		g_dev_info->acc_data_num = 0;
		g_dev_info->acc_data_num_max = data_num_temp;
		g_dev_info->acc_data_buf[0] =
			(msensor_sample_data_16bit *)(buf_base +
											2 * data_num_temp *
												sizeof(msensor_sample_data_16bit));
		g_dev_info->acc_data_buf[1] =
			(msensor_sample_data_16bit *)(buf_base +
											3 * data_num_temp *
												sizeof(msensor_sample_data_16bit));
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
		pr_err("[ICM42605] device mask not supported\n");
		return -1;
	}

	g_dev_info->buf_attr = msensor_param->buf_attr;
	pr_info("[ICM42605] data buffer init, per-queue max: %u, total: %u\n", data_num_temp, data_num_max);
	return 0;
}

static int msensor_zero_calib_gyro_fifo(void)
{
	// calib with the mean of available gyro data in the fifo
	// budget IMU like mpu6500 should use this function

	u8 fifo_byte_count[2];
	u16 fifo_count; // measured by records
	int packets;
	int i;
	u8 *buf;
	s32 x_sum = 0, y_sum = 0, z_sum = 0;
	s16 x, y, z;
	int valid_samples = 0;
	u8 *calib_buf;
	int ret;

	if (g_dev_info->gyro_zero_calib_x != 0 || g_dev_info->gyro_zero_calib_y != 0 || g_dev_info->gyro_zero_calib_z != 0) {
		pr_warn("[ICM42605] gyro zero calib offset already set\n");
		return 0;
	}

	if (!g_dev_info->flag_gyro_fifo_enabled) {
		pr_warn("[ICM42605] fifo not enabled for gyro zero calib\n");
		return -1;
	}

	msleep(CALIB_NUM);

	// Read FIFO byte count
	ret = msensor_transfer_read(REG_FIFO_BYTE_COUNT_H, fifo_byte_count, 2);
	if (ret < 0)
		return ret;

	fifo_count = (fifo_byte_count[0] << 8) | fifo_byte_count[1];

	if (fifo_count > MSENSOR_FIFO_LEN)
		return -1;

	if ((fifo_count % g_dev_info->fifo_element_len) != 0)
		fifo_count = (fifo_count / g_dev_info->fifo_element_len) * g_dev_info->fifo_element_len;

	calib_buf = kmalloc(fifo_count, GFP_KERNEL);
	if (!calib_buf)
		return -ENOMEM;

	ret = msensor_transfer_read(REG_FIFO_DATA, calib_buf, fifo_count);
	if (ret < 0)
		return ret;

	if (fifo_count == 0) {
		pr_warn("[ICM42605] no data in FIFO for gyro zero calib\n");
		kfree(calib_buf);
		return -1;
	}

	// Calculate number of packets
	packets = fifo_count / g_dev_info->fifo_element_len;

	if (packets < 1) {
		pr_warn("[ICM42605] insufficient packets in FIFO for gyro zero calib\n");
		kfree(calib_buf);
		return -1;
	}

	buf = calib_buf;
	for (i = 0; i < packets; i++) {
		s16 gyro_x = (s16)((buf[1] << 8) | buf[2]);
		s16 gyro_y = (s16)((buf[3] << 8) | buf[4]);
		s16 gyro_z = (s16)((buf[5] << 8) | buf[6]);

		// Add to sum for mean calculation
		x_sum += gyro_x;
		y_sum += gyro_y;
		z_sum += gyro_z;
		valid_samples++;

		// Move to next packet
		buf += g_dev_info->fifo_element_len;
	}

	// Calculate mean (drift/offset)
	x = x_sum / valid_samples;
	y = y_sum / valid_samples;
	z = z_sum / valid_samples;

	// Set calibration offset (negative of mean to compensate for drift)
	g_dev_info->gyro_zero_calib_x = 0 - x;
	g_dev_info->gyro_zero_calib_y = 0 - y;
	g_dev_info->gyro_zero_calib_z = 0 - z;

	kfree(calib_buf);

	return 0;
}

/******************************* runtime **************************************/
int msensor_dev_init(msensor_param *msensor_param)
{
	int ret;

	ret = msensor_param_init(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_triger_mode_init();
	if (ret < 0)
		return ret;

	ret = msensor_config_sample_rate(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_buf_init(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_soft_reset();
	if (ret < 0)
		return ret;

	ret = msensor_config_intf_reg();
	if (ret < 0)
		return ret;

	ret = msensor_config_gyro_reg(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_config_acc_reg(msensor_param);
	if (ret < 0)
		return ret;

	ret = msensor_config_fifo_reg(msensor_param->attr.device_mask);
	if (ret < 0)
		return ret;

	ret = msensor_config_fifo_interrupt();
	if (ret < 0)
		return ret;

	ret = msensor_config_power_reg();
	if (ret < 0)
		return ret;

	mutex_init(&g_dev_info->work_mutex);
	mutex_init(&g_dev_info->data_mutex);

	return 0;
}

int msensor_dev_deinit(void)
{
	kfree(g_dev_info->fifo_buf);
	g_dev_info->fifo_buf = NULL;

	memset(g_dev_info, 0, sizeof(msensor_dev_info));

	mutex_destroy(&g_dev_info->work_mutex);
	mutex_destroy(&g_dev_info->data_mutex);
	return 0;
}

int msensor_zero_calib(void)
{
	int i;
	u16 index = 0;
	msensor_sample_data_16bit *data_base = NULL;
	msensor_sample_data_16bit *data = NULL;
	s32 x_sum = 0, y_sum = 0, z_sum = 0;
	s16 x, y, z;
	msensor_data_info data_info[2] = {0};

	if (g_dev_info->gyro_zero_calib_x != 0 || g_dev_info->gyro_zero_calib_y != 0 || g_dev_info->gyro_zero_calib_z != 0) {
		pr_warn("[ICM42605] gyro zero calib offset already set\n");
		return 0;
	}

	msensor_get_data_info(data_info);

	if (data_info[0].data_type != MSENSOR_DATA_GYRO) {
		pr_err("[ICM42605] gyro not enabled for calib\n");
		return -1;
	}

	if (data_info[0].element_num < CALIB_NUM) {
		pr_err("[ICM42605] insufficient data for calib, only %d available\n", data_info[0].element_num);
		return -1;
	}

	data_base = (msensor_sample_data_16bit *)(g_dev_info->buf_attr.virt_addr + data_info[0].buf_addr_offset);

	for (i = 0; i < CALIB_NUM; i++) {
		data = data_base + index;
		x_sum += data->x;
		y_sum += data->y;
		z_sum += data->z;
		index++;
		if (index >= data_info[0].element_num)
			index = 0;
	}

	x = x_sum / CALIB_NUM;
	y = y_sum / CALIB_NUM;
	z = z_sum / CALIB_NUM;

	g_dev_info->gyro_zero_calib_x = 0 - x;
	g_dev_info->gyro_zero_calib_y = 0 - y;
	g_dev_info->gyro_zero_calib_z = 0 - z;

	pr_info("[ICM42605] gyro calib offset: %d, %d, %d\n", g_dev_info->gyro_zero_calib_x, g_dev_info->gyro_zero_calib_y, g_dev_info->gyro_zero_calib_z);

	return 0;
}

int msensor_get_data_info(msensor_data_info *data_info)
{
	u8 data_buf_index;
	u32 gyro_data_index;
	u32 gyro_data_num;
	u32 acc_data_index;
	u32 acc_data_num;

	mutex_lock(&g_dev_info->data_mutex);
	data_buf_index = g_dev_info->data_buf_index;

	if (g_dev_info->gyro_data_buf[data_buf_index]) {
		gyro_data_index = g_dev_info->gyro_data_index;
		gyro_data_num = g_dev_info->gyro_data_num;
		g_dev_info->gyro_data_index = 0;
		g_dev_info->gyro_data_num = 0;
	}

	if (g_dev_info->acc_data_buf[data_buf_index]) {
		acc_data_index = g_dev_info->acc_data_index;
		acc_data_num = g_dev_info->acc_data_num;
		g_dev_info->acc_data_index = 0;
		g_dev_info->acc_data_num = 0;
	}

	g_dev_info->data_buf_index = (g_dev_info->data_buf_index == 0) ? 1 : 0;
	mutex_unlock(&g_dev_info->data_mutex);

	if (g_dev_info->gyro_data_buf[data_buf_index]) {
		data_info->id = data_buf_index;
		data_info->data_type = MSENSOR_DATA_GYRO;
		data_info->buf_addr_offset =
			((u64)g_dev_info->gyro_data_buf[data_buf_index] -
				g_dev_info->buf_attr.virt_addr);
		data_info->buf_len =
			g_dev_info->gyro_data_num_max * sizeof(msensor_sample_data_16bit);
		data_info->element_num = gyro_data_num;
		if (gyro_data_num >= g_dev_info->gyro_data_num_max) {
			data_info->begin_index = gyro_data_index;
			data_info->end_index = (gyro_data_index == 0)
										? (g_dev_info->gyro_data_num_max - 1)
										: (gyro_data_index - 1);
		} else {
			data_info->begin_index = 0;
			data_info->end_index = (gyro_data_index == 0) ? 0 : (gyro_data_index - 1);
		}
		data_info++;
	}

	if (g_dev_info->acc_data_buf[data_buf_index]) {
		data_info->id = data_buf_index;
		data_info->data_type = MSENSOR_DATA_ACC;
		data_info->buf_addr_offset =
			((u64)g_dev_info->acc_data_buf[data_buf_index] -
				g_dev_info->buf_attr.virt_addr);
		data_info->buf_len =
			g_dev_info->acc_data_num_max * sizeof(msensor_sample_data_16bit);
		data_info->element_num = acc_data_num;
		if (acc_data_num >= g_dev_info->acc_data_num_max) {
			data_info->begin_index = acc_data_index;
			data_info->end_index = (acc_data_index == 0)
										? (g_dev_info->acc_data_num_max - 1)
										: (acc_data_index - 1);
		} else {
			data_info->begin_index = 0;
			data_info->end_index = (acc_data_index == 0) ? 0 : (acc_data_index - 1);
		}
		data_info++;
	}

	return 0;
}

static void msensor_unpack_fifo(void)
{
	int i;
	u8 *buf = g_dev_info->fifo_buf;
	msensor_sample_data_16bit *gyro_buf_base =
		g_dev_info->gyro_data_buf[g_dev_info->data_buf_index]; // pingpong buffer index
	// msensor_sample_data_16bit *acc_buf_base =
	//     g_dev_info->acc_data_buf[g_dev_info->data_buf_index]; // pingpong
	//     buffer
	msensor_sample_data_16bit *gyro_buf = NULL;
	// msensor_sample_data_16bit *acc_buf = NULL;
	// int packet_size = 0;
	// int has_accel;
	int has_gyro;

	for (i = 0; i < g_dev_info->fifo_num_packets; i++) {
		u8 header = buf[0];

		if (header & FIFO_HEADER_EMPTY)
			break;  // if the fifo is empty, break

		has_gyro = (header & FIFO_HEADER_GYRO) ? 1 : 0;

		// Process gyroscope data if enabled
		if (gyro_buf_base && g_dev_info->flag_gyro_fifo_enabled && has_gyro) {
			gyro_buf = gyro_buf_base + g_dev_info->gyro_data_index;
			gyro_buf->x = ((buf[1] << 8) | buf[2]) +
						g_dev_info->gyro_zero_calib_x; // Gyro X [15:8] | [7:0]
			gyro_buf->y = ((buf[3] << 8) | buf[4]) +
						g_dev_info->gyro_zero_calib_y; // Gyro Y [15:8] | [7:0]
			gyro_buf->z = ((buf[5] << 8) | buf[6]) +
						g_dev_info->gyro_zero_calib_z; // Gyro Z [15:8] | [7:0]
			gyro_buf->temperature = buf[7];
			gyro_buf->pts = g_dev_info->last_pts + i * g_dev_info->time_interval;

			g_dev_info->gyro_data_index++;
			if (g_dev_info->gyro_data_index >= g_dev_info->gyro_data_num_max)
				g_dev_info->gyro_data_index = 0;
			if (g_dev_info->gyro_data_num < g_dev_info->gyro_data_num_max)
				g_dev_info->gyro_data_num++;

			buf += g_dev_info->fifo_element_len;
		} else {
			buf += 1;  // skip the header
		}
	}
}

static void msensor_work_handler(struct work_struct *work)
{
	// transfer the fifo data in the imu fifo to the buffer, and unpack the data

	u8 fifo_byte_count[2];
	u16 fifo_count; // measured by bytes
	s32 time_interval_temp;
	int ret;
	unsigned long flags;
	s64 irq_timestamp = 0;

	// ktime_t work_start_time = ktime_get_ns();
	// ktime_t fifo_read_start_time, fifo_read_end_time, work_end_time;

	mutex_lock(&g_dev_info->work_mutex);

	if (g_dev_info->triger_data.triger_mode == TRIGER_EXTERN_INTERRUPT) {
		// Dequeue timestamp from the IRQ FIFO
		spin_lock_irqsave(&g_dev_info->irq_timing_lock, flags);
		if (g_dev_info->irq_timing_count > 0) {
			irq_timestamp = g_dev_info->irq_timing_fifo[g_dev_info->irq_timing_head];
			g_dev_info->irq_timing_head = (g_dev_info->irq_timing_head + 1) % 16;
			g_dev_info->irq_timing_count--;
		}
		spin_unlock_irqrestore(&g_dev_info->irq_timing_lock, flags);

		fifo_count = FIFO_THR_PACKETS * g_dev_info->fifo_element_len;
		ret = msensor_transfer_read(REG_FIFO_DATA, g_dev_info->fifo_buf, fifo_count);
		g_dev_info->fifo_num_packets = FIFO_THR_PACKETS;

		// pr_warn("[ICM42605] IRQ interval: %lld\n", irq_timestamp - g_dev_info->now_pts);
		g_dev_info->last_pts = irq_timestamp - FIFO_THR_PACKETS * g_dev_info->ideal_time_interval;
		g_dev_info->time_interval = g_dev_info->ideal_time_interval;

	} else if (g_dev_info->triger_data.triger_mode == TRIGER_TIMER) {
		g_dev_info->now_pts = ktime_get_ns();

		ret = msensor_transfer_read(REG_FIFO_BYTE_COUNT_H, fifo_byte_count, 2);
		fifo_count = (fifo_byte_count[0] << 8) | fifo_byte_count[1]; // number of bytes read from device
		if (ret < 0 || fifo_count > MSENSOR_FIFO_LEN) {
			msensor_fifo_reset();
			usleep_range(1000, 2000);
			pr_warn("[ICM42605] fifo overflows or invalid, fifo_count: %d, reset fifo\n", fifo_count);
		} else {
			if ((fifo_count % g_dev_info->fifo_element_len) != 0)
				fifo_count = (fifo_count / g_dev_info->fifo_element_len) * g_dev_info->fifo_element_len;

			// fifo_read_start_time = ktime_get_ns();
			ret = msensor_transfer_read(REG_FIFO_DATA, g_dev_info->fifo_buf, fifo_count);
			// fifo_read_end_time = ktime_get_ns();

			g_dev_info->fifo_num_packets = fifo_count / g_dev_info->fifo_element_len; // get the number of packets

			time_interval_temp = (g_dev_info->fifo_num_packets > 0) ?
								((g_dev_info->now_pts - g_dev_info->last_pts) / g_dev_info->fifo_num_packets) : 0;
			// update the time interval of each packet with some kind of averaging
			if (g_dev_info->time_interval == 0)
				g_dev_info->time_interval = (time_interval_temp + g_dev_info->ideal_time_interval) / 2;
			else
				g_dev_info->time_interval = (time_interval_temp * 2 + g_dev_info->ideal_time_interval + g_dev_info->time_interval) / 4;
		}
	}

	mutex_lock(&g_dev_info->data_mutex);
	msensor_unpack_fifo();  // unpack the data from the fifo to the buffer
	mutex_unlock(&g_dev_info->data_mutex);

	g_dev_info->last_pts = g_dev_info->now_pts;

	atomic_dec(&g_dev_info->workqueue_call_times);
	mutex_unlock(&g_dev_info->work_mutex);

	// work_end_time = ktime_get_ns();
	// pr_warn("[ICM42605] msensor_work_handler execute time: %lld ns", (long long)(work_end_time - work_start_time));
	// pr_warn("[ICM42605] msensor_work_handler fifo read time: %lld ns", (long long)(fifo_read_end_time - fifo_read_start_time));
}

int msensor_get_triger_mode(void)
{
	if (!g_dev_info)
		return -1;
	return g_dev_info->triger_data.triger_mode;
}

static irqreturn_t icm42605_gpio_isr(int irq, void *dev_id)
{
	unsigned long flags;
	s64 irq_time;
	int call_times;

	irq_time = ktime_get_ns();

	// Add timestamp to the IRQ time FIFO
	spin_lock_irqsave(&g_dev_info->irq_timing_lock, flags);
	g_dev_info->irq_timing_fifo[g_dev_info->irq_timing_tail] = irq_time;
	g_dev_info->irq_timing_tail = (g_dev_info->irq_timing_tail + 1) % 16;
	if (g_dev_info->irq_timing_count < 16)
		g_dev_info->irq_timing_count++;
	else
		g_dev_info->irq_timing_head = (g_dev_info->irq_timing_head + 1) % 16;
	spin_unlock_irqrestore(&g_dev_info->irq_timing_lock, flags);

	queue_work(g_dev_info->workqueue, &g_dev_info->work);

	call_times = atomic_read(&g_dev_info->workqueue_call_times);
	if (call_times != 0)
		pr_warn("[ICM42605] work delayed, workqueue_call_times: %d\n", call_times);
	atomic_inc(&g_dev_info->workqueue_call_times);

	return IRQ_HANDLED;
}

/* GPIO Interrupt Setup Function */
static int msensor_config_gpio_interrupt(void)
{
	int ret;

	/* Safety check */
	if (!g_dev_info) {
		pr_err("[ICM42605] GPIO setup: device info not available\n");
		return -EINVAL;
	}

	/* Initialize GPIO interrupt fields */
	g_dev_info->gpio_irq_num = -1;
	g_dev_info->gpio_irq_configured = false;

	/* Step 1: Request GPIO */
	ret = gpio_request(GPIO_NUM, GPIO_IRQ_NAME);
	if (ret) {
		pr_err("[ICM42605] gpio_request(%d) failed: %d\n", GPIO_NUM, ret);
		return ret;
	}

	/* Step 2: Configure as input direction */
	ret = gpio_direction_input(GPIO_NUM);
	if (ret) {
		pr_err("[ICM42605] gpio_direction_input(%d) failed: %d\n", GPIO_NUM, ret);
		goto err_free_gpio;
	}

	/* Step 3: Map GPIO number to interrupt number */
	g_dev_info->gpio_irq_num = gpio_to_irq(GPIO_NUM);
	if (g_dev_info->gpio_irq_num < 0) {
		pr_err("[ICM42605] gpio_to_irq(%d) failed: %d\n", GPIO_NUM, g_dev_info->gpio_irq_num);
		ret = g_dev_info->gpio_irq_num;
		goto err_free_gpio;
	}

	/* Step 4: Request interrupt, configure as rising edge trigger */
	ret = request_irq(g_dev_info->gpio_irq_num,
			  icm42605_gpio_isr,
			  IRQF_TRIGGER_FALLING,
			  GPIO_IRQ_NAME,
			  (void *)&g_dev_info->gpio_irq_num);
	if (ret) {
		pr_err("[ICM42605] request_irq(%d) failed: %d\n", g_dev_info->gpio_irq_num, ret);
		goto err_free_gpio;
	}

	g_dev_info->gpio_irq_configured = true;

	return 0;

err_free_gpio:
	gpio_free(GPIO_NUM);
	return ret;
}

/* GPIO Interrupt Cleanup Function */
static void msensor_cleanup_gpio_interrupt(void)
{
	/* Safety check */
	if (!g_dev_info)
		return;

	if (g_dev_info->gpio_irq_configured && g_dev_info->gpio_irq_num >= 0) {
		free_irq(g_dev_info->gpio_irq_num, (void *)&g_dev_info->gpio_irq_num);
		g_dev_info->gpio_irq_configured = false;
		g_dev_info->gpio_irq_num = -1;
	}

	gpio_free(GPIO_NUM);
}

static enum hrtimer_restart timer_hr_interrupt(struct hrtimer *timer)
{
	ktime_t stime;
	int call_times;

	stime = ktime_set(0, g_dev_info->triger_data.triger_info.timer_config.interval * NSEC_PER_USEC);
	hrtimer_forward_now(timer, stime);
	queue_work(g_dev_info->workqueue, &g_dev_info->work);
	call_times = atomic_read(&g_dev_info->workqueue_call_times);
	if (call_times != 0)
		pr_warn("[ICM42605] work delayed, workqueue_call_times: %d\n", call_times);
	atomic_inc(&g_dev_info->workqueue_call_times);
	return HRTIMER_RESTART;
}

int msensor_timer_triger_run(void)
{
	ktime_t stime;

	hrtimer_init(&g_dev_info->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_dev_info->hrtimer.function = timer_hr_interrupt;

	atomic_set(&g_dev_info->workqueue_call_times, 0);
	g_dev_info->workqueue = create_singlethread_workqueue("msensor");
	INIT_WORK(&g_dev_info->work, msensor_work_handler);

	/* Initialize timing FIFO */
	spin_lock_init(&g_dev_info->irq_timing_lock);
	g_dev_info->irq_timing_head = 0;
	g_dev_info->irq_timing_tail = 0;
	g_dev_info->irq_timing_count = 0;

	stime = ktime_set(0, g_dev_info->triger_data.triger_info.timer_config.interval * NSEC_PER_USEC);
	hrtimer_start(&g_dev_info->hrtimer, stime, HRTIMER_MODE_REL);
	msensor_fifo_reset();

	g_dev_info->last_pts = ktime_get_ns();
	g_dev_info->time_interval = 0;

	pr_info("[ICM42605] timer trigger run\n");

	return 0;
}

int msensor_timer_triger_stop(void)
{
	hrtimer_cancel(&g_dev_info->hrtimer);
	mutex_lock(&g_dev_info->work_mutex);
	destroy_workqueue(g_dev_info->workqueue);
	mutex_unlock(&g_dev_info->work_mutex);
	pr_info("[ICM42605] timer trigger stop\n");
	return 0;
}

/* GPIO Interrupt Trigger Functions */
int msensor_gpio_triger_run(void)
{
	atomic_set(&g_dev_info->workqueue_call_times, 0);
	g_dev_info->workqueue = create_singlethread_workqueue("msensor");
	INIT_WORK(&g_dev_info->work, msensor_work_handler);

	/* Initialize timing FIFO */
	spin_lock_init(&g_dev_info->irq_timing_lock);
	g_dev_info->irq_timing_head = 0;
	g_dev_info->irq_timing_tail = 0;
	g_dev_info->irq_timing_count = 0;

	msensor_fifo_reset();

	/* Configure GPIO interrupt for FIFO watermark */
	if (msensor_config_gpio_interrupt() < 0) {
		pr_err("[ICM42605] GPIO interrupt configuration failed\n");
		return -1;
	}

	pr_info("[ICM42605] GPIO interrupt trigger run\n");

	return 0;
}

int msensor_gpio_triger_stop(void)
{
	msensor_cleanup_gpio_interrupt();
	mutex_lock(&g_dev_info->work_mutex);
	destroy_workqueue(g_dev_info->workqueue);
	mutex_unlock(&g_dev_info->work_mutex);
	pr_info("[ICM42605] GPIO interrupt trigger stop\n");
	return 0;
}

int msensor_get_debug_info(char *info, int info_len)
{
	int info_cnt = 0;
	char buf[256];
	int ret;
	u8 dev_id = 0;
	// uint16_t fifo_count;
	// uint8_t fifo_byte_count[2];
	// uint8_t gpio_value;
	// uint8_t int_status = 0;

	ret = msensor_transfer_read(REG_WHO_AM_I, &dev_id, 1);
	if (ret < 0)
		info_cnt += snprintf(buf, sizeof(buf), "read who_am_i failed\n");
	else
		info_cnt += snprintf(buf, sizeof(buf), "WHO_AM_I: 0x%x\n", dev_id);
	strncat(info, buf, info_len - info_cnt - 1);

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

	info_cnt += snprintf(buf, sizeof(buf), "\ttemperature: %d\n", g_dev_info->temperature);
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf), "\tgyro zero calib data: %d, %d, %d\n",
				 g_dev_info->gyro_zero_calib_x, g_dev_info->gyro_zero_calib_y,
				 g_dev_info->gyro_zero_calib_z);
	strncat(info, buf, info_len - info_cnt - 1);

	/* GPIO Interrupt Status */
	info_cnt += snprintf(buf, sizeof(buf), "GPIO interrupt info:\n");
	strncat(info, buf, info_len - info_cnt - 1);

	info_cnt += snprintf(buf, sizeof(buf),
			     "\tGPIO %d, IRQ %d, configured: %s\n",
			     GPIO_NUM,
			     g_dev_info->gpio_irq_num,
			     g_dev_info->gpio_irq_configured ? "yes" : "no");
	strncat(info, buf, info_len - info_cnt - 1);

	// gpio_value = gpio_get_value(GPIO_NUM);
	// info_cnt += snprintf(buf, sizeof(buf), "\tGPIO %d value = %d\n", GPIO_NUM, gpio_value);
	// strncat(info, buf, info_len - info_cnt - 1);

	// msensor_transfer_read(REG_FIFO_BYTE_COUNT_H, fifo_byte_count, 2);
	// fifo_count = (fifo_byte_count[0] << 8) | fifo_byte_count[1];
	// info_cnt += snprintf(buf, sizeof(buf), "\tfifo count: %d\n", fifo_count);
	// strncat(info, buf, info_len - info_cnt - 1);

	// msensor_transfer_read(REG_INT_STATUS, &int_status, 1);
	// info_cnt += snprintf(buf, sizeof(buf), "\tint status: 0x%x\n", int_status);
	// strncat(info, buf, info_len - info_cnt - 1);

	pr_info("get debug info...%d, %d\n", info_len, info_cnt);
	return 0;
}

int msensor_module_init(void)
{
	int ret = 0;

	g_dev_info = kmalloc(sizeof(*g_dev_info), GFP_KERNEL);
	if (!g_dev_info)
		return -ENOMEM;
	memset(g_dev_info, 0, sizeof(msensor_dev_info));

	ret = i2c_dev_init(MSENSOR_DEV_I2C_NODE, MSENSOR_DEV_I2C_ADDR);
	if (ret < 0) {
		pr_err("[ICM42605] i2c dev init err\n");
		return -1;
	}

	return 0;
}

int msensor_module_deint(void)
{
	i2c_dev_deinit();
	kfree(g_dev_info);
	g_dev_info = NULL;
	return 0;
}

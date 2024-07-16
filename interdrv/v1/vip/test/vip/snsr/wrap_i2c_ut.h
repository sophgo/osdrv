/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: wrap_i2c_ut.h
 * Description:
 */

#ifndef __WRAP_I2C_UT_H__
#define __WRAP_I2C_UT_H__

#include <linux/cvi_vip_snsr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CVI_SNS_I2C_IOC_MAGIC	'i'
#define CVI_SNS_I2C_WRITE	_IOWR(CVI_SNSR_IOC_MAGIC, 2, \
					struct isp_i2c_data)
#ifdef __cplusplus
}
#endif

#endif /* __WRAP_I2C_UT_H__ */

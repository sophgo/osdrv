/* SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: i2c_dev.h
 * Description: i2c transfer header file
 *
 */
#ifndef I2C_DEV_H_
#define I2C_DEV_H_

#include <linux/types.h>

int i2c_dev_init(const char *i2c_node, u16 i2c_addr);
int i2c_dev_deinit(void);

int i2c_dev_read(u8 reg_addr, u8 *reg_data, u32 cnt);
int i2c_dev_write(u8 reg_addr, u8 *reg_data, u32 cnt);

#endif


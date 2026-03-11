// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: i2c_dev.c
 * Description: i2c transfer related code
 *
 */
#include "i2c_dev.h"
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/slab.h>

struct file *g_fp;
struct i2c_client *g_client;

int i2c_dev_init(const char *i2c_node, u16 i2c_addr)
{
	g_fp = filp_open(i2c_node, O_RDWR, 0);
	if (IS_ERR(g_fp)) {
		pr_err("open %s err...\n", i2c_node);
		return -1;
	}

	g_client = (struct i2c_client *)g_fp->private_data;
	g_client->addr = i2c_addr;

	return 0;
}

int i2c_dev_deinit(void)
{
	if (g_fp) {
		filp_close(g_fp, NULL);
		g_fp = NULL;
	}

	return 0;
}

int i2c_dev_read(u8 reg_addr, u8 *reg_data, u32 cnt)
{
	int ret = 0;

	struct i2c_msg msg[2];

	msg[0].addr = g_client->addr;
	msg[0].flags = g_client->flags & I2C_M_TEN;
	msg[0].len = sizeof(u8);
	msg[0].buf = &reg_addr;

	msg[1].addr = g_client->addr;
	msg[1].flags = g_client->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = cnt;
	msg[1].buf = reg_data;

	ret = i2c_transfer(g_client->adapter, msg, 2);

	return ret;
}

int i2c_dev_write(u8 reg_addr, u8 *reg_data, u32 cnt)
{
	int ret = 0;

#define __WRITE_TEMP_BUFF (16)

	u32 i = 0;
	u8 buf[__WRITE_TEMP_BUFF];

	u8 *temp_buf = NULL;
	u32 temp_buf_cnt = 0;

	if (cnt < __WRITE_TEMP_BUFF - 1) {
		temp_buf = buf;
		temp_buf_cnt = __WRITE_TEMP_BUFF;
	} else {
		temp_buf = kmalloc(cnt + 1, GFP_KERNEL);
		temp_buf_cnt = cnt + 1;
	}

	if (!temp_buf) {
		pr_err("i2c dev write kmalloc fail...\n");
		return -1;
	}

	temp_buf[0] = reg_addr;

	for (i = 0; i < cnt; i++)
		temp_buf[i + 1] = reg_data[i];

	ret = i2c_master_send(g_client, temp_buf, cnt + 1);

	if (temp_buf != buf) {
		kfree(temp_buf);
		temp_buf = NULL;
	}

	return ret;
}

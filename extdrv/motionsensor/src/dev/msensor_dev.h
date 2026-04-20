/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: msensor_dev.h
 * Description: motionsensor device related code
 *
 */
#ifndef MSENSOR_DEV_H_
#define MSENSOR_DEV_H_

#include "motionsensor_common.h"

int msensor_dev_init(msensor_param *msensor_param);
int msensor_dev_deinit(void);

int msensor_module_init(void);
int msensor_module_deint(void);

int msensor_get_triger_mode(void);
int msensor_timer_triger_run(void);
int msensor_timer_triger_stop(void);
int msensor_gpio_triger_run(void);
int msensor_gpio_triger_stop(void);

/* max = 2 */
int msensor_get_data_info(msensor_data_info *data_info);
int msensor_zero_calib(void);

int msensor_get_debug_info(char *info, int info_len);

#endif


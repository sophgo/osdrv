/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: ut_instances.h
 * Description:
 */

#ifndef __UT_INSTANCES_H__
#define __UT_INSTANCES_H__

struct module_op *isp_get_instance(void);
struct module_op *sclr_get_instance(void);
struct module_op *cif_get_instance(void);
struct module_op *snsr_get_instance(void);
struct module_op *wrap_i2c_get_instance(void);

#endif /* __UT_INSTANCES_H__ */

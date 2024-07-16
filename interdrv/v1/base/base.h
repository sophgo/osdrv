/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: base.h
 * Description:
 */

#ifndef __CV183X_BASE_H__
#define __CV183X_BASE_H__

#include "uapi/cvi_base.h"

unsigned int cvi_base_read_chip_id(void);
extern unsigned int vb_max_pools;
extern unsigned int vb_pool_max_blk;

#endif /* __CV183X_BASE_H__ */

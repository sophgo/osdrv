/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: reg.h
 * Description:
 */

#ifndef _CVI_REG_H_
#define _CVI_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/io.h>

#define _reg_read(addr) readl((void __iomem *)addr)
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)

#ifdef __cplusplus
}
#endif

#endif /* _CVI_REG_H_ */

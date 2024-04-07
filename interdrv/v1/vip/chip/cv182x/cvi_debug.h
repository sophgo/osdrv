/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_debug.h
 * Description:
 */

#ifndef _CVI_DEBUG_H_
#define _CVI_DEBUG_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/debugfs.h>

extern int log_level;
extern u32 vip_log_lv;

#define vip_pr(level, fmt, arg...) \
	do { \
		if (vip_log_lv & level) { \
			if (level == CVI_ERR) \
				pr_err("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == CVI_WARN) \
				pr_warn("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == CVI_NOTICE) \
				pr_notice("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == CVI_INFO) \
				pr_info("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else if (level == CVI_DBG) \
				pr_debug("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
		} \
	} while (0)

enum vip_msg_pri {
	CVI_ERR = 3,
	CVI_WARN = 4,
	CVI_NOTICE = 5,
	CVI_INFO = 6,
	CVI_DBG = 7,
};

#define dprintk(level, fmt, arg...) \
	do { \
		if (log_level & level) { \
			if (level == VIP_ERR) \
				pr_err("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
			else \
				pr_debug("%d:%s(): " fmt, __LINE__, __func__, ## arg); \
		} \
	} while (0)

#define dpr_cont(level, fmt, arg...) \
	do { \
		if (log_level & level) { \
			if (level == VIP_ERR) \
				pr_err(fmt, ## arg); \
			else \
				pr_debug(fmt, ## arg); \
		} \
	} while (0)

enum vip_msg_prio {
	VIP_ERR		= 0x0001,
	VIP_WARN	= 0x0002,
	VIP_INFO	= 0x0004,
	VIP_DBG		= 0x0008,
	VIP_VB2		= 0x0010,
	VIP_ISP_IRQ	= 0x0020,
};

#ifdef __cplusplus
}
#endif

#endif /* _CVI_DEBUG_H_ */

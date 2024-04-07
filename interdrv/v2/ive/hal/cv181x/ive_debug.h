#ifndef _IVE_DEBUG_H_
#define _IVE_DEBUG_H_

#include <linux/debugfs.h>

extern u32 ive_log_lv;

#define CVI_DBG_ERR        1   /* error conditions                     */
#define CVI_DBG_WARN       2   /* warning conditions                   */
#define CVI_DBG_NOTICE     3   /* normal but significant condition     */
#define CVI_DBG_INFO       4   /* informational                        */
#define CVI_DBG_DEBUG      5   /* debug-level messages                 */

#define CVI_TRACE_IVE(level, fmt, ...) \
	do { \
		if (level <= ive_log_lv) { \
			if (level == CVI_DBG_ERR) \
				pr_err("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_WARN) \
				pr_warn("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_NOTICE) \
				pr_notice("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_INFO) \
				pr_info("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_DEBUG) \
				printk(KERN_DEBUG "%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

#endif /* _IVE_DEBUG_H_ */
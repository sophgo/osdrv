#ifndef __VO_COMMON_H__
#define __VO_COMMON_H__

#include <linux/debugfs.h>

#ifdef __cplusplus
	extern "C" {
#endif

extern int vo_log_lv;
extern int hide_vo;

#define DBG_ERR        1   /* error conditions                     */
#define DBG_WARN       2   /* warning conditions                   */
#define DBG_NOTICE     3   /* normal but significant condition     */
#define DBG_INFO       4   /* informational                        */
#define DBG_DEBUG      5   /* debug-level messages                 */

#if CONFIG_LOG
#define TRACE_VO(level, fmt, ...) \
	do { \
		if (level <= vo_log_lv) { \
			if (level == DBG_ERR) \
				pr_err("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == DBG_WARN) \
				pr_warn("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == DBG_NOTICE) \
				pr_notice("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == DBG_INFO) \
				pr_info("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == DBG_DEBUG) \
				pr_debug("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define TRACE_VO(level, fmt, ...) do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VO_COMMON_H__ */

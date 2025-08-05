#ifndef _TDE_DEBUG_H_
#define _TDE_DEBUG_H_

#include <linux/debugfs.h>
// #include <linux/comm_tde.h>

extern unsigned int tde_log_lv;

#define DBG_ERR        1   /* error conditions                     */
#define DBG_WARN       2   /* warning conditions                   */
#define DBG_NOTICE     3   /* normal but significant condition     */
#define DBG_INFO       4   /* informational                        */
#define DBG_DEBUG      5   /* debug-level messages                 */

#if CONFIG_LOG
#define TRACE_TDE(level, fmt, ...) \
	do { \
		if (level <= tde_log_lv) { \
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
#define TRACE_TDE(level, fmt, ...) do {} while (0)
#endif

#endif /* _TDE_DEBUG_H_ */

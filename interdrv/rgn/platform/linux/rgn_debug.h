#ifndef __RGN_DEBUG_H__
#define __RGN_DEBUG_H__

#ifdef __cplusplus
	extern "C" {
#endif


#define RGN_ERR        1   /* error conditions                     */
#define RGN_WARN       2   /* warning conditions                   */
#define RGN_NOTICE     3   /* normal but significant condition     */
#define RGN_INFO       4   /* informational                        */
#define RGN_DEBUG      5   /* debug-level messages                 */

extern unsigned int rgn_log_lv;

#if CONFIG_LOG
#define TRACE_RGN(level, fmt, ...) \
	do { \
		if (level <= rgn_log_lv) { \
			if (level == RGN_ERR) \
				pr_err("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_WARN) \
				pr_warn("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_NOTICE) \
				pr_notice("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_INFO) \
				pr_info("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == RGN_DEBUG) \
				pr_debug("%s:(%d): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define TRACE_RGN(level, fmt, ...) do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RGN_DEBUG_H__ */

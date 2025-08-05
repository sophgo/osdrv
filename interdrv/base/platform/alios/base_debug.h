#ifndef _BASE_DEBUG_H_
#define _BASE_DEBUG_H_


#include <debug/dbg.h>
#include <drv/tick.h>
#include "dmesg.h"

extern u32 base_log_lv;

#define DBG_ERR        1   /* error conditions                     */
#define DBG_WARN       2   /* warning conditions                   */
#define DBG_NOTICE     3   /* normal but significant condition     */
#define DBG_INFO       4   /* informational                        */
#define DBG_DEBUG      5   /* debug-level messages                 */

#if CONFIG_LOG
#if CONFIG_LOG_TO_DMESG
#define TRACE_BASE(level, fmt, ...) \
	do { \
		if (level <= base_log_lv) { \
			dmesg_printk("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define TRACE_BASE(level, fmt, ...) \
	do { \
		if (level <= base_log_lv) { \
			aos_debug_printf("[%lu]%s:%d(): " fmt, csi_tick_get_us(), __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#endif /*End of CONFIG_LOG_TO_DMESG */
#else
#define TRACE_BASE(level, fmt, ...) do {} while (0)
#endif /*End of CONFIG_LOG */


#endif /* _BASE_DEBUG_H_ */

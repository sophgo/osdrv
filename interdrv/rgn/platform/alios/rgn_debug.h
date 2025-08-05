#ifndef __RGN_DEBUG_H__
#define __RGN_DEBUG_H__

#include <debug/dbg.h>
#include <drv/tick.h>
#include "dmesg.h"

#define RGN_ERR        1   /* error conditions                     */
#define RGN_WARN       2   /* warning conditions                   */
#define RGN_NOTICE     3   /* normal but significant condition     */
#define RGN_INFO       4   /* informational                        */
#define RGN_DEBUG      5   /* debug-level messages                 */

extern int rgn_log_lv;

#if CONFIG_LOG
#if CONFIG_LOG_TO_DMESG
#define TRACE_RGN(level, fmt, ...) \
	do { \
		if (level <= rgn_log_lv) { \
			dmesg_printk("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define TRACE_RGN(level, fmt, ...) \
	do { \
		if (level <= rgn_log_lv) { \
			aos_debug_printf("[%lu]%s:%d(): " fmt, csi_tick_get_us(), __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#endif /*End of CONFIG_LOG_TO_DMESG */
#else
#define TRACE_RGN(level, fmt, ...) do {} while (0)
#endif /*End of CONFIG_LOG */


#endif /* __RGN_DEBUG_H__ */

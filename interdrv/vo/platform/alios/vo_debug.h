#ifndef __VO_DEBUG_H__
#define __VO_DEBUG_H__

#include <debug/dbg.h>
#include <drv/tick.h>
#include "dmesg.h"

extern int vo_log_lv;
extern int hide_vo;

#define DBG_ERR        1   /* error conditions                     */
#define DBG_WARN       2   /* warning conditions                   */
#define DBG_NOTICE     3   /* normal but significant condition     */
#define DBG_INFO       4   /* informational                        */
#define DBG_DEBUG      5   /* debug-level messages                 */


#if CONFIG_LOG
#if CONFIG_LOG_TO_DMESG
#define TRACE_VO(level, fmt, ...) \
	do { \
		if (level <= vo_log_lv) { \
			dmesg_printk("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define TRACE_VO(level, fmt, ...) \
	do { \
		if (level <= vo_log_lv) { \
			aos_debug_printf("[%lu]%s:%d(): " fmt, csi_tick_get_us(), __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#endif /*End of CONFIG_LOG_TO_DMESG */
#else
#define TRACE_VO(level, fmt, ...) do {} while (0)
#endif /*End of CONFIG_LOG */


#endif /* __VO_DEBUG_H__ */

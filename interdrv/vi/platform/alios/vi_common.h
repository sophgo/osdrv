#ifndef __VI_COMMON_H__
#define __VI_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal_types.h"
#include <debug/dbg.h>
#include <drv/tick.h>
#include "dmesg.h"

#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define VI_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment
#define VI_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define VI_256_ALIGN(x) (((x) + 0xFF) & ~0xFF)   // for 256byte alignment
#define VI_1K_ALIGN(x) (((x) + 0x3FF) & ~0x3FF)   // for 1kbyte alignment
#define VI_4K_ALIGN(x) (((x) + 0xFFF) & ~0xFFF)   // for 4kbyte alignment
#define ISP_ALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))   // for any bytes alignment
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#define CEIL(x, y) (((x) + ((1 << (y)))) >> (y))   // for alignment

extern u32 vi_log_lv;
extern u32 patgen_vblanking;

enum vi_msg_pri {
	VI_ERR		= 0x1,
	VI_WARN		= 0x2,
	VI_NOTICE	= 0x4,
	VI_INFO		= 0x8,
	VI_DBG		= 0x10,
};

#if CONFIG_LOG
#if CONFIG_LOG_TO_DMESG
#define vi_pr(level, fmt, ...) \
	do { \
		if (level <= vi_log_lv) {				\
			dmesg_printk("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define vi_pr(level, fmt, ...) \
	do { \
		if (level <= vi_log_lv) {				\
			aos_debug_printf("[%lu]%s:%d(): " fmt, csi_tick_get_us(), __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#endif /*End of CONFIG_LOG_TO_DMESG */
#else
#define vi_pr(level, fmt, ...) do {} while (0)
#endif /*End of CONFIG_LOG */

#ifdef __cplusplus
}
#endif

#endif /* __VI_COMMON_H__ */

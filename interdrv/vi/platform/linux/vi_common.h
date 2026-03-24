#ifndef __VI_COMMON_H__
#define __VI_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/printk.h>

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

#define vi_pr(level, fmt, arg...) \
	do { \
		if (vi_log_lv & level) { \
			if (level == VI_ERR) \
				pr_err("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_WARN) \
				pr_warn("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_NOTICE) \
				pr_notice("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_INFO) \
				pr_info("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == VI_DBG) \
				pr_debug("%s:%d(): " fmt, __func__, __LINE__, ## arg); \
		} \
	} while (0)

enum vi_msg_pri {
	VI_ERR		= 1,
	VI_WARN		= 2,
	VI_NOTICE	= 3,
	VI_INFO		= 4,
	VI_DBG		= 5,
};

#ifdef __cplusplus
}
#endif

#endif /* __VI_COMMON_H__ */

#ifndef __BASE_COMMON_H__
#define __BASE_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cvi_type.h>
#include <linux/cvi_common.h>

const uint8_t *sys_get_modname(MOD_ID_E id);

CVI_U32 get_diff_in_us(struct timespec64 t1, struct timespec64 t2);

#ifdef __cplusplus
}
#endif

#endif /* __BASE_CB_H__ */

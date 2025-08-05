#ifndef __BASE_COMMON_H__
#define __BASE_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "common.h"
#include "osal.h"

const char *sys_get_modname(mod_id_e id);

u32 get_diff_in_us(osal_timeval t1, osal_timeval t2);


#ifdef __cplusplus
}
#endif

#endif /* __BASE_CB_H__ */

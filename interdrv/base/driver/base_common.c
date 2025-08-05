#include "base_common.h"
#include "base_debug.h"

#define GENERATE_STRING(STRING) (#STRING),
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);


const char *sys_get_modname(mod_id_e id)
{
	return (id < ID_BUTT) ? MOD_STRING[id] : "UNDEF";
}
osal_module_export(sys_get_modname);

u32 get_diff_in_us(osal_timeval t1, osal_timeval t2)
{
	u64 ts_us;
	u64 ts_s;

	ts_s = t2.tv_sec - t1.tv_sec;

	if (t2.tv_usec < t1.tv_usec) {
		ts_us = 1000000 + t2.tv_usec - t1.tv_usec;
		ts_s--;
	} else {
		ts_us = t2.tv_usec - t1.tv_usec;
	}

	ts_us += 1000000 * ts_s;

	return (u32)ts_us;
}
osal_module_export(get_diff_in_us);



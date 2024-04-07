#include <linux/types.h>
#include <linux/module.h>
#include <linux/time.h>
#include "base_common.h"

#define GENERATE_STRING(STRING) (#STRING),
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
const uint8_t *sys_get_modname(MOD_ID_E id)
{
	return (id < CVI_ID_BUTT) ? MOD_STRING[id] : "UNDEF";
}
EXPORT_SYMBOL_GPL(sys_get_modname);

u32 get_diff_in_us(struct timespec64 t1, struct timespec64 t2)
{
	struct timespec64 ts_delta = timespec64_sub(t2, t1);
	u64 ts_ns;

	ts_ns = timespec64_to_ns(&ts_delta);
	do_div(ts_ns, 1000);
	return ts_ns;
}
EXPORT_SYMBOL_GPL(get_diff_in_us);


#ifndef __TDE_CB_H__
#define __TDE_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "comm_sys.h"
#include "comm_vb.h"
#include "base_ctx.h"

enum tde_usage_e {
        TDE_USAGE_ROTATION,
        TDE_USAGE_DRAW_LINE,
        TDE_USAGE_QUICK_COPY,
        TDE_USAGE_MAX
};

enum tde_cb_task_mode_e {
	TDE_CB_TASK_ROTATE_90, /* ratate 90 */
	TDE_CB_TASK_ROTATE_270, /* ratate 270 */
	TDE_CB_TASK_DRAW_LINE,
	TDE_CB_TASK_COPY,
	TDE_CB_TASK_MAX
};

struct tde_inter_cfg {
	enum tde_usage_e usage;
	const void *usage_param;
	unsigned int width;
	unsigned int height;
	unsigned long long src_addr;
	unsigned long long dst_addr;
	unsigned char sync_io;
	unsigned char block;
	enum tde_cb_task_mode_e task_mode;
};

enum tde_cb_cmd {
	TDE_CB_OP,
	TDE_CB_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __TDE_CB_H__ */

#ifndef __TDE_INTER_CB_H__
#define __TDE_INTER_CB_H__

#include "comm_tde.h"
#include "tde_sdk_layer.h"
#include "tde_cb.h"

int tde_do_op(struct tde_core *core, enum tde_usage_e usage
	, const void *usage_param, unsigned int width, unsigned int height
	, unsigned long long src_addr, unsigned long long dst_addr, unsigned char sync_io
	, unsigned char block, enum tde_task_mode_e task_mode);

#endif

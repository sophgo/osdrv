#ifndef _VI_DBG_PROC_H_
#define _VI_DBG_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <vi_defines.h>

extern struct cvi_vi_ctx *gViCtx;

int vi_dbg_proc_init(struct cvi_vi_dev *_vdev);
int vi_dbg_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif // _VI_DBG_PROC_H_


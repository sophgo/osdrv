#ifndef _CVI_VIP_STITCH_PROC_H_
#define _CVI_VIP_STITCH_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>

#include "stitch_ctx.h"
#include <linux/cvi_base_ctx.h>

#include "stitch_core.h"

int stitch_proc_init(struct cvi_stitch_dev *dev);
int stitch_proc_remove(struct cvi_stitch_dev *dev);

#endif // _CVI_VIP_STITCH_PROC_H_

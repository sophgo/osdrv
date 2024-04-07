#ifndef _DPU_PROC_H_
#define _DPU_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>
#include <linux/cvi_base_ctx.h>

#include "dpu_core.h"


int dpu_proc_init(struct cvi_dpu_dev *dev);
int dpu_proc_remove(struct cvi_dpu_dev *dev);

#endif // _CVI_VIP_DPU_PROC_H_

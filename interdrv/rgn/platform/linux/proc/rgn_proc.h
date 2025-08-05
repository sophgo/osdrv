#ifndef _RGN_PROC_H_
#define _RGN_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <generated/compile.h>

#include "comm_video.h"
#include "comm_region.h"
#include "rgn_ctx.h"

#define GDC_PROC_JOB_INFO_NUM      (500)
#define RGN_PROC_INFO_OFFSET       (sizeof(struct vpss_proc_ctx) * VPSS_MAX_GRP_NUM)

int rgn_proc_init(void);
int rgn_proc_remove(void);

#endif // _RGN_PROC_H_

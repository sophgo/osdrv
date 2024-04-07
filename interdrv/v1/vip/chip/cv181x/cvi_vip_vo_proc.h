#ifndef _CVI_VIP_VO_PROC_H_
#define _CVI_VIP_VO_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>
#include "mw/vpu_base.h"

int vo_proc_init(void *shm);
int vo_proc_remove(void);
int vo_proc_show(struct seq_file *m, void *v);

#endif // _CVI_VIP_VO_PROC_H_

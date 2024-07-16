#ifndef _CVI_VIP_VPSS_PROC_H_
#define _CVI_VIP_VPSS_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>
#include "mw/vpu_base.h"

int vpss_proc_init(void *shm);
int vpss_proc_remove(void);
int vpss_proc_show(struct seq_file *m, void *v);

#endif // _CVI_VIP_VPSS_PROC_H_

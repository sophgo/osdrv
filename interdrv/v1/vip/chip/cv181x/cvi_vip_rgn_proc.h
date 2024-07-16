#ifndef _CVI_VIP_RGN_PROC_H_
#define _CVI_VIP_RGN_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "mw/vpu_base.h"

int rgn_proc_init(void *shm);
int rgn_proc_remove(void);

#endif // _CVI_VIP_RGN_PROC_H_

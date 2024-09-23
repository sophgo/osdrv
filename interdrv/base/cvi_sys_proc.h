#ifndef _CVI_SYS_PROC_H_
#define _CVI_SYS_PROC_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include <linux/cvi_base_ctx.h>

int sys_proc_init(struct proc_dir_entry *_proc_dir, void *shared_mem);
int sys_proc_remove(struct proc_dir_entry *_proc_dir);

#endif


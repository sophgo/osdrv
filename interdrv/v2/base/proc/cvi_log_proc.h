#ifndef _CVI_LOG_PROC_H_
#define _CVI_LOG_PROC_H_


int log_proc_init(struct proc_dir_entry *_proc_dir, void *shared_mem);
int log_proc_remove(struct proc_dir_entry *_proc_dir);

#endif


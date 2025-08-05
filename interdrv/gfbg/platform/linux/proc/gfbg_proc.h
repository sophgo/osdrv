#ifndef GFBG_PROC_H
#define GFBG_PROC_H

#include <linux/proc_fs.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void gfbg_proc_add_module(const char *entry_name, void *data);
void gfbg_proc_remove_module(const char *entry_name);
void gfbg_proc_remove_all_module(void);
void gfbg_proc_init(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

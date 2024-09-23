
#ifndef __IPCM_DEV__
#define __IPCM_DEV__

#include "ipcm.h"

s32 dev_init(void);

void dev_cleanup(void);

s32 dev_recv_handle(void *data);

u32 dev_poll(struct file *file, struct poll_table_struct *table);

#endif

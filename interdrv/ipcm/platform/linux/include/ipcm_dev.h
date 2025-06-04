
#ifndef __IPCM_DEV__
#define __IPCM_DEV__

#include "ipcm.h"

s32 dev_init(void);

void dev_cleanup(void);

s32 dev_recv_handle(void *data);

u32 dev_poll(struct file *file, struct poll_table_struct *table);

int ipcm_register_dev(void);

void ipcm_deregister_dev(void);

int ipcm_msg_register_dev(void);

void ipcm_msg_deregister_dev(void);

int ipcm_sys_register_dev(void);

void ipcm_sys_deregister_dev(void);

int ipcm_cust_register_dev(void);

void ipcm_cust_deregister_dev(void);
#endif

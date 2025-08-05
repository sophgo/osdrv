#ifndef _CIF_IOCTL_H_
#define _CIF_IOCTL_H_

#include "cif_comm.h"

struct link *ctx_to_link(const struct cif_ctx *ctx);
struct cif_dev *file_cif_dev(struct file *file);
int cif_rm_cb(void);
int cif_register_cb(struct cif_dev *dev);
long cif_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
long cif_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#endif

#endif
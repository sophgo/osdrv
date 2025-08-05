#ifndef _CIF_L_H_
#define _CIF_L_H_
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/irq.h>
#include <linux/reset.h>
#include <generated/compile.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <defines.h>
#if defined(__CV184X__)
#include "pinctrl-cv184x.h"
#endif
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/compat.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#endif
#include <linux/miscdevice.h>

extern int proc_cif_show(struct seq_file *m, void *v);
extern ssize_t cif_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos);
extern int proc_cif_show(struct seq_file *m, void *v);

#endif

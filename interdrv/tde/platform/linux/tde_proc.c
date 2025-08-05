#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <generated/compile.h>

#include "tde_debug.h"
#include "tde_core.h"


#define TDE_PROC_NAME          "soph/tde"

// for proc info
static int proc_tde_mode;

/*************************************************************************
 *	TDE proc functions
 *************************************************************************/

int tde_ctx_proc_show(struct seq_file *m, void *v)
{
	//todo
	return 0;
}

/*************************************************************************
 *	Proc functions
 *************************************************************************/

static int tde_proc_show(struct seq_file *m, void *v)
{
	return tde_ctx_proc_show(m, v);
}

static ssize_t tde_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(cProcInputdata)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(cProcInputdata, 10, &proc_tde_mode))
		proc_tde_mode = 0;

	return count;
}

static int tde_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, tde_proc_show, PDE_DATA(inode));
}

static const struct proc_ops tde_proc_fops = {
	.proc_open = tde_proc_open,
	.proc_read = seq_read,
	.proc_write = tde_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int tde_proc_init(struct tde_core *core)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(TDE_PROC_NAME, 0644, NULL,
				 &tde_proc_fops, core);
	if (!entry) {
		TRACE_TDE(DBG_ERR, "tde proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int tde_proc_remove(struct tde_core *core)
{
	remove_proc_entry(TDE_PROC_NAME, NULL);
	return 0;
}

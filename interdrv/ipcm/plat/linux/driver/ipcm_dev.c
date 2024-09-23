
#include <asm/io.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/version.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

// #define _DEBUG

#include "mailbox.h"
#include "ipcm_dev.h"
#include "ipcmsg_dev.h"
#include "ipcm_drv_common.h"
#include "ipcm_port_msg.h"
#include "ipcm_port_sys.h"
#include "ipcm_port_anon.h"

#define IPCM_DEV_NAME "ipcm"

struct ipcm_device {
	struct device *dev;
	struct proc_dir_entry *proc_root;
};

int mailbox_irq;

unsigned long long t_recv;

typedef struct _ipcm_dev_ctx {
	u32 alios_stat;
	u8 open_recv_send_log;
	u8 test_case;
} ipcm_dev_ctx;

extern irqreturn_t prvQueueISR(int irq, void *dev_id);

static ipcm_dev_ctx s_ctx = {};

#ifdef IPCM_INFO_REC
extern void ipcm_info_record_start(void);
extern void ipcm_info_record_stop(void);
extern int ipcm_get_record_info_status(void);
extern void print_ring_snap(void);
#endif

static s32 _ipcm_irq_pre_process(u8 port_id, void *msg)
{
	if (s_ctx.open_recv_send_log || ipcm_log_level_debug()) {
		if (msg)
			ipcm_info("linux recv msg:%lx\n", *(unsigned long int *)msg);
		else {
			ipcm_warning("linux recved msg, but msg is null.\n");
		}
	}
	t_recv = timer_get_boot_us();
	return 0;
}

static s32 _ipcm_send_pre_process(u8 port_id, void *msg)
{
	if (s_ctx.open_recv_send_log || ipcm_log_level_debug()) {
		ipcm_info("linux send msg:%lx\n", *(unsigned long int *)msg);
	}
	return 0;
}

// static void ipcm_syscore_shutdown(void)
// {
// dev_cleanup();
// }

#ifndef IPCM_SHIRNK
static int cvi_get_log_level(void *data, u64 *val)
{
	*val = ipcm_get_log_level();
	return 0;
}

static int cvi_set_log_level(void *data, u64 val)
{
	ipcm_set_log_level(val);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(log_level, cvi_get_log_level, cvi_set_log_level, "%llu\n");

// static struct syscore_ops ipcm_syscore_ops = {
// .shutdown   = ipcm_syscore_shutdown
// };

#ifdef DRV_TEST
extern void _ipcm_test(void);
static ssize_t cvi_ipcm_test_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	s_ctx.test_case = input_param;
	switch(input_param){
		case 1:
			_ipcm_test();
		break;
		default:
		break;
	}
	return count;
}

static int cvi_ipcm_test_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", s_ctx.test_case);
	return 0;
}

static int cvi_ipcm_test_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cvi_ipcm_test_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops test_proc_ops = {
	.proc_open = cvi_ipcm_test_proc_open,
	.proc_read = seq_read,
	.proc_write = cvi_ipcm_test_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations test_proc_ops = {
	.owner = THIS_MODULE,
	.open = cvi_ipcm_test_proc_open,
	.read = seq_read,
	.write = cvi_ipcm_test_proc_write,
	.release = single_release,
};
#endif
#endif

static ssize_t cvi_ipcm_info_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
#ifdef IPCM_INFO_REC
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}
	if (input_param) {
		ipcm_info_record_start();
	} else {
		ipcm_info_record_stop();
	}
#endif

	return count;
}

static int cvi_ipcm_info_proc_show(struct seq_file *m, void *v)
{
	s32 cnt = mailbox_get_invalid_cnt();
	seq_printf(m, "mailbox not valid cnt:%d\n", cnt);
#ifdef IPCM_INFO_REC
	{
		int record_stat = ipcm_get_record_info_status();
		seq_printf(m, "ipcm info record status:%d\n", record_stat);
		print_ring_snap();
	}
#endif
	return 0;
}

static int cvi_ipcm_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cvi_ipcm_info_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops info_proc_ops = {
	.proc_open = cvi_ipcm_info_proc_open,
	.proc_read = seq_read,
	.proc_write = cvi_ipcm_info_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations info_proc_ops = {
	.owner = THIS_MODULE,
	.open = cvi_ipcm_info_proc_open,
	.read = seq_read,
	.write = cvi_ipcm_info_proc_write,
	.release = single_release,
};
#endif

static ssize_t cvi_ipcm_log0_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	s_ctx.open_recv_send_log = input_param;
	return count;
}

static int cvi_ipcm_log0_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", s_ctx.open_recv_send_log);
	return 0;
}

static int cvi_ipcm_log0_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cvi_ipcm_log0_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops log0_proc_ops = {
	.proc_open = cvi_ipcm_log0_proc_open,
	.proc_read = seq_read,
	.proc_write = cvi_ipcm_log0_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations log0_proc_ops = {
	.owner = THIS_MODULE,
	.open = cvi_ipcm_log0_proc_open,
	.read = seq_read,
	.write = cvi_ipcm_log0_proc_write,
	.release = single_release,
};
#endif

static ssize_t cvi_ipcm_loglevel_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	ipcm_set_log_level(input_param);
	return count;
}

static int cvi_ipcm_loglevel_proc_show(struct seq_file *m, void *v)
{
	int loglevel = ipcm_get_log_level();
	seq_printf(m, "%d\n", loglevel);
	return 0;
}

static int cvi_ipcm_loglevel_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cvi_ipcm_loglevel_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops loglevel_proc_ops = {
	.proc_open = cvi_ipcm_loglevel_proc_open,
	.proc_read = seq_read,
	.proc_write = cvi_ipcm_loglevel_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations loglevel_proc_ops = {
	.owner = THIS_MODULE,
	.open = cvi_ipcm_loglevel_proc_open,
	.read = seq_read,
	.write = cvi_ipcm_loglevel_proc_write,
	.release = single_release,
};
#endif

#endif

static int ipcm_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ipcm_device *ndev;
	int ret = 0;
	int err = -1;

	ipcm_info("%s start ---\n", __func__);
	ipcm_info("name=%s\n", pdev->name);

	ipcm_get_rtos_boot_status(&s_ctx.alios_stat);
	if (!(s_ctx.alios_stat & (1<<RTOS_IPCM_DONE))) {
		ipcm_err("alios ipcm not ready now, stat is %u maybe should resmod ipcm.ko\n",
			s_ctx.alios_stat);
		return -EFAULT;
	}

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->dev = dev;

	ret = ipcm_common_register_dev();
	if (ret < 0) {
		ipcm_err("ipcm_common_register_dev error ret:%d\n", ret);
		return ret;
	}

	ret = ipcmsg_register_dev();
	if (ret < 0) {
		ipcm_err("regsiter ipcmsg chrdev error\n");
	}

	// ret = ipcm_msg_register_dev();
	// if (ret < 0) {
	// 	ipcm_err("ipcm_msg_register_dev error\n");
	// 	return ret;
	// }

	// ret = ipcm_sys_register_dev();
	// if (ret) {
	// 	ipcm_err("ipcm_sys_register_dev fail.\n");
	// }

	ret = ipcm_anon_register_dev();
	if (ret) {
		ipcm_err("ipcm_anon_register_dev fail.\n");
	}

	mailbox_irq = platform_get_irq_byname(pdev, "mailbox");

	ret = ipcm_register_irq_handle(_ipcm_irq_pre_process);

	ret = ipcm_register_pre_send_handle(_ipcm_send_pre_process);

	ret = ipcm_port_common_init();
	if (ret) {
		ipcm_err("ipcm_port_common_init failed ret:%d.\n", ret);
		return ret;
	}

	// ret = ipcm_drv_msg_init();
	// if (ret) {
	// 	ipcm_err("ipcm_drv_msg_init failed.\n");
	// 	return ret;
	// }

	// ret = ipcm_drv_sys_init();
	// if (ret) {
	// 	ipcm_err("ipcm_drv_sys_init failed.\n");
	// }

	ret = ipcm_drv_anon_init();
	if (ret) {
		ipcm_err("ipcm_drv_anon_init failed.\n");
	}

	s_ctx.open_recv_send_log = 0;

#ifndef IPCM_SHIRNK
	ndev->proc_root = proc_mkdir("cvitek/ipcm", NULL);

#ifdef DRV_TEST
	if (proc_create_data("test", 0644, ndev->proc_root, &test_proc_ops, ndev) == NULL)
		pr_err("test proc creation failed\n");
#endif
	if (proc_create_data("ipcm_info", 0644, ndev->proc_root, &info_proc_ops, ndev) == NULL)
		pr_err("ipcm_info proc creation failed\n");

	if (proc_create_data("log0", 0644, ndev->proc_root, &log0_proc_ops, ndev) == NULL)
		pr_err("log0 proc creation failed\n");

	if (proc_create_data("loglevel", 0644, ndev->proc_root, &loglevel_proc_ops, ndev) == NULL)
		pr_err("loglevel proc creation failed\n");
#endif

	platform_set_drvdata(pdev, ndev);

	err = request_irq(mailbox_irq, prvQueueISR, 0, "mailbox", (void *)ndev);

	if (err) {
		ipcm_err("fail to register interrupt handler\n");
		return -1;
	}

	ipcm_info("%s DONE\n", __func__);
	return 0;

}

static int ipcm_dev_remove(struct platform_device *pdev)
{
	struct ipcm_device *ndev = platform_get_drvdata(pdev);

	if (!(s_ctx.alios_stat & (1<<RTOS_IPCM_DONE))) {
		ipcm_warning("ipcm driver not init, alios_stat is %u\n", s_ctx.alios_stat);
		return 0;
	}

#ifndef IPCM_SHIRNK
	proc_remove(ndev->proc_root);
#endif
	ipcm_anon_deregister_dev();
	// ipcm_sys_deregister_dev();
	ipcmsg_deregister_dev();
	// ipcm_msg_deregister_dev();
	ipcm_common_deregister_dev();
	platform_set_drvdata(pdev, NULL);
	/* remove irq handler*/
	free_irq(mailbox_irq, ndev);

	ipcm_drv_anon_uninit();
	// ipcm_drv_msg_uninit();
	// ipcm_drv_sys_uninit();
	ipcm_port_common_uninit();
	ipcm_uninit();
	ipcm_debug("%s DONE\n", __func__);

	return 0;
}

static const struct of_device_id ipcm_match[] = {
	{ .compatible = "cvitek,rtos_cmdqu" },
	{},
};

static struct platform_driver ipcm_driver = {
	.probe = ipcm_dev_probe,
	.remove = ipcm_dev_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = IPCM_DEV_NAME,
		.of_match_table = ipcm_match,
	},
};

static struct class *pbase_class;
static void ipcm_dev_exit(void)
{
	platform_driver_unregister(&ipcm_driver);
	class_destroy(pbase_class);
	ipcm_debug("%s DONE\n", __func__);
}

static int ipcm_dev_init(void)
{
	int rc;
	ipcm_debug("%s\n", __func__);
	pbase_class = class_create(THIS_MODULE, IPCM_DEV_NAME);
	if (IS_ERR(pbase_class)) {
		ipcm_err("create class failed\n");
		rc = PTR_ERR(pbase_class);
		goto cleanup;
	}

	platform_driver_register(&ipcm_driver);
	ipcm_debug("%s done\n", __func__);
	return 0;
cleanup:
	ipcm_dev_exit();
	return rc;
}

module_init(ipcm_dev_init);
module_exit(ipcm_dev_exit);

MODULE_AUTHOR("cvitek");
MODULE_DESCRIPTION("IPCM_DEV");
MODULE_LICENSE("GPL");

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/cvi_comm_video.h>
#include <base_cb.h>
#include <base_ctx.h>
//#include <vi_sys.h>

#include "../../common/dpu_debug.h"
#include "dpu_core.h"
#include "../../common/dpu_proc.h"
#include "../../common/dpu_ioctl.h"


#define DPU_CLASS_NAME "soph-dpu"
#define DPU_DEV_NAME "soph-dpu"
#define DEVICE_FROM_DTS 1
#define DPU_SHARE_MEM_SIZE (0x8000)

#define DPU_OSDRV_TEST 1

u32 dpu_log_lv = CVI_DBG_WARN/*CVI_DBG_INFO*/;

s32 hw_wait_time = 33;
//static atomic_t open_count = ATOMIC_INIT(0);

static const char *const CLK_DPU_NAME = "clk_dpu";

module_param(dpu_log_lv, int, 0644);
module_param(hw_wait_time, int, 0644);

int dpu_core_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	return 0;
}

static int dpu_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_DPU);
}

static int dpu_register_cb(struct cvi_dpu_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_DPU;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= dpu_core_cb;

	return base_reg_module_cb(&reg_cb);
}
//done
static int dpu_open(struct inode *inode, struct file *filp)
{
	u32 i;
	struct cvi_dpu_handle_info *h_info;
	struct cvi_dpu_dev *wdev =
		container_of(filp->private_data, struct cvi_dpu_dev, miscdev);

	if (!wdev) {
		pr_err("cannot find dpu private data\n");
		return -ENODEV;
	}

	h_info = kmalloc(sizeof(struct cvi_dpu_handle_info), GFP_KERNEL);
	if(!h_info){
		return -ENODEV;
	}

	h_info->file = filp;
	h_info->open_pid = current->pid;

	for(i = 0; i < DPU_MAX_GRP_NUM; i++)
		h_info->useGrp[i] = CVI_FALSE;

	mutex_lock(&wdev->dpuLock);
	list_add(&h_info->list, &wdev->handle_list);
	mutex_unlock(&wdev->dpuLock);

	return 0;
}

//done
static int dpu_release(struct inode *inode, struct file *filp)
{
	u32 i;
	struct cvi_dpu_dev *wdev = (struct cvi_dpu_dev*) filp->private_data;
	struct cvi_dpu_handle_info *h_info, *h_node;

	if (dpu_get_handle_info(wdev, filp, &h_info)) {
		pr_err("dpudrv: file list is not found!\n");
		return -EINVAL;
	}

	mutex_lock(&wdev->dpuLock);

	list_for_each_entry(h_node, &wdev->handle_list, list) {
		if(h_node->open_pid == h_info->open_pid){
			for(i = 0; i < DPU_MAX_GRP_NUM; i++){
				if(h_info->useGrp[i]){
					dpu_mode_deinit(i);
					h_info->useGrp[i] = CVI_FALSE;
				}
			}
		}
	}

	list_del(&h_info->list);
	kfree(h_info);
	mutex_unlock(&wdev->dpuLock);

	return 0;
}

//done
static const struct file_operations dpu_fops = {
	.owner = THIS_MODULE,
	.open = dpu_open,
	.release = dpu_release,
	.unlocked_ioctl = dpu_ioctl,

};

//done
static int _register_dev(struct cvi_dpu_dev *wdev)
{
	int rc;

	wdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	wdev->miscdev.name = DPU_DEV_NAME;
	wdev->miscdev.fops = &dpu_fops;

	rc = misc_register(&wdev->miscdev);
	if (rc) {
		pr_err("cvi_dpu: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

/*************************************************************************
 *	General functions
 *************************************************************************/
//done
int dpu_create_instance(struct platform_device *pdev)
{
	int i, rc = 0;
	struct cvi_dpu_dev *wdev;
	static const char * const clk_sys_name[] = {"clk_sys_1","clk_dpu_enable"};

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("dpu cannot get drv data for dpu\n");
		return -EINVAL;
	}
	mutex_init(&wdev->mutex);
	// spin_lock_init(&wdev->lock);

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		wdev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(wdev->clk_sys[i])) {
			pr_err("dpu cannot get clk for clk_sys_%d\n", i);
			wdev->clk_sys[i] = NULL;
		}
	}

	//wdev->align = DPU_ADDR_ALIGN;
	rc =_register_dev(wdev);
	if(rc){
		CVI_TRACE_DPU(CVI_DBG_ERR, "Failed to register dev\n");
		goto err_dev;
	}


	wdev->shared_mem = kzalloc(DPU_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!wdev->shared_mem) {
		pr_err("dpu shared mem alloc fail\n");
		return -ENOMEM;
	}

	rc = dpu_proc_init(wdev->shared_mem);
	if(rc){
		pr_err("dpu proc init failed\n");
		goto err_proc;
	}

	dpu_init(wdev);

	return 0;

err_proc:
	dpu_proc_remove(wdev->shared_mem);
	return rc;

err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	CVI_TRACE_DPU(CVI_DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

//done
int dpu_destroy_instance(struct platform_device *pdev)
{
	struct cvi_dpu_dev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("invalid data\n");
		return -EINVAL;
	}

	dpu_deinit(wdev);

	misc_deregister(&wdev->miscdev);

	dpu_proc_remove(wdev->shared_mem); //TODO

	kfree(wdev->shared_mem);

	return 0;
}

//done
//Get Reg Base And Irq Num
static int _init_resources(struct platform_device *pdev)
{
	uint64_t phyAddr;
	int rc = 0;
	int irq_num;
	static const char *const irq_name = "dpu";
	struct resource *res = NULL;
	void *reg_base;
	void *reg_base_sgbm_ld1_dma;
	void *reg_base_sgbm_ld2_dma;
	void *reg_base_sgbm_median_dma;
	void *reg_base_sgbm_bf_dma;
	void *reg_base_fgs_gx_dma;
	void *reg_base_fgs_chfh_ld_dma;
	void *reg_base_fgs_chfh_st_dma;
	void *reg_base_fgs_ux_dma;
	struct cvi_dpu_dev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		dev_err(&pdev->dev, "Can not get dpu drvdata\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "invalid resource\n");
		return -EINVAL;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	reg_base = devm_ioremap(&pdev->dev, res->start,
					res->end - res->start);
#else
	reg_base = devm_ioremap_nocache(&pdev->dev, res->start,
					res->end - res->start);
#endif
	phyAddr=(uint64_t)reg_base;
	CVI_TRACE_DPU(CVI_DBG_INFO, "  (%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%llx).\n",
	       1, res->start, res->end, phyAddr);

	dpu_set_base_addr(reg_base+0x600);
	reg_base_sgbm_ld1_dma    =reg_base;
	reg_base_sgbm_ld2_dma    =reg_base+0x100;
	reg_base_sgbm_median_dma =reg_base+0x1100;
	reg_base_sgbm_bf_dma     =reg_base+0x1000;
	reg_base_fgs_gx_dma      =reg_base+0x300;
	reg_base_fgs_chfh_ld_dma =reg_base+0x200;
	reg_base_fgs_chfh_st_dma =reg_base+0x1200;
	reg_base_fgs_ux_dma      =reg_base+0x1300;

	dpu_set_base_addr_sgbm_dma(reg_base_sgbm_ld1_dma , reg_base_sgbm_ld2_dma,\
								reg_base_sgbm_median_dma,reg_base_sgbm_bf_dma);

	dpu_set_base_addr_fgs_dma(reg_base_fgs_gx_dma , reg_base_fgs_chfh_ld_dma,\
								reg_base_fgs_chfh_st_dma,reg_base_fgs_ux_dma);
	/* Interrupt */
	irq_num = platform_get_irq_byname(pdev, irq_name);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", irq_name);
		return -ENODEV;
	}
	CVI_TRACE_DPU(CVI_DBG_INFO, "irq(%d) for %s get from platform driver.\n", irq_num,
		irq_name);

	wdev->irq_num = irq_num;

	return rc;
}

static irqreturn_t dpu_isr(int irq, void *dev)
{
	u8 intr_status ;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_isr        +\n");
	intr_status = dpu_intr_status();
	getsgbm_status();
	getfgs_status();
	dpu_irq_handler(intr_status,dev);
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_isr        -\n");
	return IRQ_HANDLED;
}

static int cvi_dpu_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_dpu_dev *wdev;

	/* allocate main structure */
	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	/* initialize locks */
	// spin_lock_init(&dev->lock);
	// mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, wdev);

	// get hw-resources
	rc = _init_resources(pdev);
	if (rc){
		CVI_TRACE_DPU(CVI_DBG_ERR, "Failed to init resource\n");
		goto err_dev;
	}

	// for dpu
	rc = dpu_create_instance(pdev);
	if (rc) {
		pr_err("Failed to create dpu instance\n");
		goto err_dev;
	}

	if (devm_request_irq(&pdev->dev, wdev->irq_num, dpu_isr,
			     IRQF_SHARED, "CVI_DPU", wdev)) {
		dev_err(&pdev->dev, "Unable to request dpu IRQ(%d)\n",
			wdev->irq_num);
		return -EINVAL;
	}

	/* dpu register cb */
	if (dpu_register_cb(wdev)) {
		dev_err(&pdev->dev, "Failed to register dpu cb, err %d\n", rc);
		return -EINVAL;
	}

	CVI_TRACE_DPU(CVI_DBG_INFO, "done with rc(%d).\n", rc);

	return rc;


err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * cvi_dpu_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
//done
static int cvi_dpu_remove(struct platform_device *pdev)
{
	struct cvi_dpu_dev *wdev;

	dpu_destroy_instance(pdev);

	/* dpu rm cb */
	if (dpu_core_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm dpu cb\n");
	}

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata");
		return 0;
	}

	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

//done
static const struct of_device_id cvi_dpu_dt_match[] = {
	{ .compatible = "cvitek,dpu" },
	{}
};

#if (!DEVICE_FROM_DTS)
static void cvi_dpu_pdev_release(struct device *dev)
{
}

static struct platform_device cvi_dpu_pdev = {
	.name		= "dpu",
	.dev.release	= cvi_dpu_pdev_release,
};
#endif

#ifdef CONFIG_PM_SLEEP
static int dpu_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int dpu_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	//VIP_CLK_RATIO_CONFIG(DPU, 0x10);

	return 0;
}

static SIMPLE_DEV_PM_OPS(dpu_pm_ops, dpu_suspend, dpu_resume);
#else
static SIMPLE_DEV_PM_OPS(dpu_pm_ops, NULL, NULL);
#endif

//done
static struct platform_driver cvi_dpu_driver = {
	.probe      = cvi_dpu_probe,
	.remove     = cvi_dpu_remove,
	.driver     = {
	.name		= "soph-dpu",
	.owner		= THIS_MODULE,
	.of_match_table	= cvi_dpu_dt_match,
	.pm		= &dpu_pm_ops,
	},
};

static int __init cvi_dpu_init(void)
{
	int rc;

	CVI_TRACE_DPU(CVI_DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_dpu_driver);
	#else
	rc = platform_device_register(&cvi_dpu_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_dpu_driver);
	if (rc)
		platform_device_unregister(&cvi_dpu_pdev);
	#endif

	return rc;
}

static void __exit cvi_dpu_exit(void)
{
	CVI_TRACE_DPU(CVI_DBG_INFO, " +\n");
	platform_driver_unregister(&cvi_dpu_driver);
	#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cvi_dpu_pdev);
	#endif
}

MODULE_DESCRIPTION("Cvitek dpu driver");
MODULE_AUTHOR("zhongbin.wang");
MODULE_LICENSE("GPL");
module_init(cvi_dpu_init);
module_exit(cvi_dpu_exit);


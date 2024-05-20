#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <linux/cvi_vip.h>
#include <linux/cvi_base_ctx.h>

#include <vi_sys.h>
#include <base_cb.h>
#include <base_ctx.h>
#include <ldc_cb.h>
#include <vi_cb.h>
#include <rgn_cb.h>

#include "base_common.h"
#include "vpss_common.h"
#include "scaler.h"
#include "vpss.h"
#include "vpss_debug.h"
#include "vpss_core.h"
#include "vo_cb.h"
#include "vpss_ioctl.h"
#include "vpss_rgn_ctrl.h"
#include "vpss_proc.h"

#define VPSS_DEV_NAME "soph-vpss"

u32 vpss_log_lv = CVI_DBG_WARN;
int hw_mask = 0x3ff; //default vpss_v + vpss_t + vpss_d

static atomic_t open_count = ATOMIC_INIT(0);
static const char *const vpss_name[] = {"vpss_v0", "vpss_v1", "vpss_v2",
										"vpss_v3", "vpss_t0", "vpss_t1",
										"vpss_t2", "vpss_t3", "vpss_d0",
										"vpss_d1"};
static const char *const vpss_clk_name[CVI_VPSS_MAX][3] = {
					{"clk_vi_sys3", NULL, "clk_vpss_v0"},
					{"clk_vi_sys3", NULL, "clk_vpss_v1"},
					{"clk_vi_sys3", NULL, "clk_vpss_v2"},
					{"clk_vi_sys3", NULL, "clk_vpss_v3"},
					{"clk_vd0_sys2", "clk_vd0_apb_vpss0", "clk_vpss_t0"},
					{"clk_vd0_sys2", "clk_vd0_apb_vpss1", "clk_vpss_t1"},
					{"clk_vd1_sys2", "clk_vd1_apb_vpss0", "clk_vpss_t2"},
					{"clk_vd1_sys2", "clk_vd1_apb_vpss1", "clk_vpss_t3"},
					{"clk_vo_sys1", NULL, "clk_vpss_d0"},
					{"clk_vo_sys1", NULL, "clk_vpss_d1"}};

module_param(vpss_log_lv, int, 0644);

module_param(hw_mask, int, 0644);


void vpss_timer_core_update(void *data)
{
	u8 i;
	u32 duration;
	struct cvi_vpss_device *dev = (struct cvi_vpss_device *)data;
	struct timespec64 curTime;
	static struct timespec64 preTime = {0};

	ktime_get_ts64(&curTime);
	duration = get_diff_in_us(preTime, curTime);
	preTime = curTime;

	if (duration > 2000000)
		return;

	for (i = CVI_VPSS_V0; i < CVI_VPSS_MAX; ++i) {
		dev->vpss_cores[i].u32DutyRatio = (dev->vpss_cores[i].u32HwDurationTotal * 100) / duration;
		dev->vpss_cores[i].u32HwDurationTotal = 0;
	}
}

int vpss_core_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	//struct cvi_vpss_device *vpss_dev = (struct cvi_vpss_device *)dev;
	int rc = -1;

	switch (cmd) {
	case VPSS_CB_VI_ONLINE_TRIGGER:
	{
		struct sc_cfg_cb *post_para = (struct sc_cfg_cb *)arg;
		struct cvi_vpss_online_cb param;

		param.snr_num = post_para->snr_num;
		param.is_tile = post_para->is_tile;
		param.is_left_tile = post_para->is_left_tile;
		param.l_in = post_para->l_in;
		param.l_out = post_para->l_out;
		param.r_in = post_para->r_in;
		param.r_out = post_para->r_out;

		vpss_set_mlv_info(post_para->snr_num, &post_para->m_lv_i);
		rc = cvi_vpss_hal_online_run(&param);
		break;
	}

	case VPSS_CB_SET_VIVPSSMODE:
	{
		const VI_VPSS_MODE_S *pstVIVPSSMode = (const VI_VPSS_MODE_S *)arg;

		CVI_TRACE_VPSS(CVI_DBG_INFO, "VPSS_CB_SET_VIVPSSMODE\n");
		rc = vpss_set_vivpss_mode(pstVIVPSSMode);
		break;
	}

	case VPSS_CB_ONLINE_ERR_HANDLE:
	{
		//struct sc_err_handle_cb *err_cb_para = (struct sc_err_handle_cb *)arg;

		//cvi_sc_vi_err_cb(err_cb_para, vdev);
		rc = 0;
		break;
	}

	case VPSS_CB_GET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;
		u32 layer = attr->layer;

		rc = vpss_get_rgn_hdls(VpssGrp, VpssChn, layer, enType, pstHandle);
		break;
	}

	case VPSS_CB_SET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;
		u32 layer = attr->layer;

		rc = vpss_set_rgn_hdls(VpssGrp, VpssChn, layer, enType, pstHandle);
		break;
	}

	case VPSS_CB_SET_RGN_CFG:
	{
		struct _rgn_cfg_cb_param *attr = (struct _rgn_cfg_cb_param *)arg;
		struct cvi_rgn_cfg *pstRgnCfg = &attr->rgn_cfg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;
		u32 layer = attr->layer;

		rc = vpss_set_rgn_cfg(VpssGrp, VpssChn, layer, pstRgnCfg);
		break;
	}

	case VPSS_CB_SET_RGNEX_CFG:
	{
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Not support rgn_ex\n");
		rc = -1;
		break;
	}

	case VPSS_CB_SET_COVEREX_CFG:
	{
		struct _rgn_coverex_cfg_cb_param *attr = (struct _rgn_coverex_cfg_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_set_rgn_coverex_cfg(VpssGrp, VpssChn, &attr->rgn_coverex_cfg);
		break;
	}

	case VPSS_CB_SET_MOSAIC_CFG:
	{
		struct _rgn_mosaic_cfg_cb_param *attr = (struct _rgn_mosaic_cfg_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_set_rgn_mosaic_cfg(VpssGrp, VpssChn, &attr->rgn_mosaic_cfg);
		break;
	}

	case VPSS_CB_SET_RGN_LUT_CFG:
	{
		struct _rgn_lut_cb_param *attr = (struct _rgn_lut_cb_param *)arg;
		struct cvi_rgn_lut_cfg *pstRgnLutCfg = &attr->lut_cfg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_set_rgn_lut_cfg(VpssGrp, VpssChn, pstRgnLutCfg);
		break;
	}

	case VPSS_CB_GET_RGN_OW_ADDR:
	{
		struct _rgn_get_ow_addr_cb_param *attr = (struct _rgn_get_ow_addr_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_get_rgn_ow_addr(VpssGrp, VpssChn, attr->layer, attr->handle, &attr->addr);
		break;
	}

	case VPSS_CB_GET_CHN_SIZE:
	{
		struct _rgn_chn_size_cb_param *attr = (struct _rgn_chn_size_cb_param *)arg;
		VPSS_CHN_ATTR_S stChnAttr;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_get_chn_attr(VpssGrp, VpssChn, &stChnAttr);
		attr->rect.u32Width = stChnAttr.u32Width;
		attr->rect.u32Height = stChnAttr.u32Height;
		break;
	}

	case VPSS_CB_GET_MLV_INFO:
	{
		struct vpss_grp_mlv_info *mlv_info = (struct vpss_grp_mlv_info *)arg;
		VPSS_GRP VpssGrp = mlv_info->vpss_grp;

		vpss_get_mlv_info(VpssGrp, &mlv_info->m_lv_i);
		break;
	}

	case VPSS_CB_STITCH:
	{
		struct vpss_stitch_cfg *stitch_cfg = (struct vpss_stitch_cfg *)arg;

		rc = cvi_vpss_hal_stitch_schedule(stitch_cfg);
		break;
	}

	case LDC_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg =
			(struct ldc_op_done_cfg *)arg;

		vpss_gdc_callback(cfg->pParam, cfg->blk);
		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

static int vpss_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VPSS);
}

static int vpss_core_register_cb(struct cvi_vpss_device *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VPSS;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vpss_core_cb;

	return base_reg_module_cb(&reg_cb);
}


static irqreturn_t vpss_isr(int irq, void *data)
{
	union sclr_intr intr_status;
	struct vpss_core *core = (struct vpss_core *)data;
	u8 vpss_idx = core->enVpssType;

	if (core->irq_num != irq) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "irq(%d) Error.\n", irq);
		return IRQ_HANDLED;
	}
	intr_status = sclr_intr_status(vpss_idx);
	sclr_intr_clr(vpss_idx, intr_status);

	if(intr_status.b.img_in_frame_end == 1)
		core->intr_status |= INTR_IMG_IN;
	if(intr_status.b.scl_frame_end == 1)
		core->intr_status |= INTR_SC;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "vpss(%d), irq(%d), status(0x%x)\n",
		vpss_idx, irq, intr_status.raw);

	if (intr_status.b.cmdq_end == 1) {
		sclr_clr_cmdq(vpss_idx);
		cvi_vpss_cmdq_irq_handler(core);
		return IRQ_HANDLED;
	}

	if ((core->intr_status & INTR_IMG_IN) &&
		(core->intr_status & INTR_SC)) {
		//struct sclr_img_checksum_status img_status;
		struct sclr_core_checksum_status sc_status;

		//sclr_img_get_checksum_status(vpss_idx, &img_status);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum en(%d)\n", img_status.checksum_base.b.enable);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum axi_read_data(0x%x)\n", img_status.axi_read_data);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum output_data(0x%x)\n", img_status.checksum_base.b.output_data);
		sclr_core_get_checksum_status(vpss_idx, &sc_status);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum en(%d)\n", sc_status.checksum_base.b.enable);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum axi_write_data(0x%x)\n", sc_status.axi_write_data);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum data_in(0x%x)\n", sc_status.checksum_base.b.data_in_from_img_in);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "checksum data_out(0x%x)\n", sc_status.checksum_base.b.data_out);

		core->intr_status = 0;
		core->checksum = sc_status.axi_write_data;
		cvi_vpss_irq_handler(core);
	}

	return IRQ_HANDLED;
}

static void vpss_dev_init(struct cvi_vpss_device *dev)
{
	u8 i;
	struct vpss_core *core;

	/* initialize locks */
	spin_lock_init(&dev->lock);

	for (i = CVI_VPSS_V0; i < CVI_VPSS_MAX; ++i) {
		core = &dev->vpss_cores[i];
		core->enVpssType = i;
		spin_lock_init(&core->vpss_lock);
		core->job = NULL;
		core->intr_status = 0;

		if (core->clk_src)
			clk_prepare_enable(core->clk_src);
		if (core->clk_apb)
			clk_prepare_enable(core->clk_apb);
		if (core->clk_vpss)
			clk_prepare_enable(core->clk_vpss);

		if (hw_mask & BIT(i)) {
			atomic_set(&core->state, CVI_VIP_IDLE);
			sclr_ctrl_init(i);
			CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss-%d init done\n", i);
		} else {
			atomic_set(&core->state, CVI_VIP_RUNNING);
		}

		if (core->clk_vpss)
			clk_disable(core->clk_vpss);
	}

	cvi_vpss_hal_init(dev);
	vpss_init();
	register_timer_fun(vpss_timer_core_update, (void *)dev);
}

static void vpss_dev_deinit(struct cvi_vpss_device *dev)
{
	u8 i;
	struct vpss_core *core;

	vpss_deinit();
	cvi_vpss_hal_deinit();

	for (i = CVI_VPSS_V0; i < CVI_VPSS_MAX; ++i) {
		core = &dev->vpss_cores[i];

		if (core->clk_src)
			clk_disable_unprepare(core->clk_src);
		if (core->clk_apb)
			clk_disable_unprepare(core->clk_apb);
		if (core->clk_vpss)
			clk_unprepare(core->clk_vpss);
	}
}

static int vpss_init_resources(struct platform_device *pdev)
{
	int rc = 0;
	int irq_num;
	struct resource *res = NULL;
	void *reg_base[4];
	struct cvi_vpss_device *dev;
	int i;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata\n");
		return -EINVAL;
	}

	/* register ioremap */
	for (i = 0; i < ARRAY_SIZE(reg_base); ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to get res[%d]\n", i);
			return -EINVAL;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		reg_base[i] = devm_ioremap(&pdev->dev, res->start,
						    res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start,
						    res->end - res->start);
#endif
		CVI_TRACE_VPSS(CVI_DBG_INFO, "(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			i, res->start, res->end, reg_base[i]);
		if (!reg_base[i]) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to get reg_base[%d]\n", i);
			return -EINVAL;
		}
	}
	sclr_set_base_addr(reg_base[0], reg_base[1], reg_base[2], reg_base[3]);

	/* Interrupt */
	for (i = 0; i < ARRAY_SIZE(vpss_name); ++i) {
		irq_num = platform_get_irq_byname(pdev, vpss_name[i]);
		if (irq_num < 0) {
			dev_err(&pdev->dev, "No IRQ resource for [%d]%s\n",
				i, vpss_name[i]);
			return -ENODEV;
		}
		CVI_TRACE_VPSS(CVI_DBG_INFO, "irq(%d) for %s get from platform driver.\n",
			irq_num, vpss_name[i]);
		dev->vpss_cores[i].irq_num = irq_num;
	}

	/* clk */
	for (i = 0; i < CVI_VPSS_MAX; ++i) {
		dev->vpss_cores[i].clk_src = devm_clk_get(&pdev->dev, vpss_clk_name[i][0]);
		if (IS_ERR(dev->vpss_cores[i].clk_src)) {
			pr_err("Cannot get source clk for vpss%d\n", i);
			dev->vpss_cores[i].clk_src = NULL;
		}

		if (vpss_clk_name[i][1]) {
			dev->vpss_cores[i].clk_apb = devm_clk_get(&pdev->dev, vpss_clk_name[i][1]);
			if (IS_ERR(dev->vpss_cores[i].clk_apb)) {
				pr_err("Cannot get apb clk for vpss%d\n", i);
				dev->vpss_cores[i].clk_apb = NULL;
			}
		} else {
			dev->vpss_cores[i].clk_apb = NULL;
		}

		dev->vpss_cores[i].clk_vpss = devm_clk_get(&pdev->dev, vpss_clk_name[i][2]);
		if (IS_ERR(dev->vpss_cores[i].clk_vpss)) {
			pr_err("Cannot get clk for vpss%d\n", i);
			dev->vpss_cores[i].clk_vpss = NULL;
		}
	}

	return rc;
}

static int vpss_open(struct inode *inode, struct file *filep)
{
	struct cvi_vpss_device *dev =
		container_of(filep->private_data, struct cvi_vpss_device, miscdev);
	int i;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss_open\n");

	if (!dev) {
		pr_err("Cannot find vpss private data\n");
		return -ENODEV;
	}

	i = atomic_inc_return(&open_count);
	if (i > 1) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss_open: open %d times\n", i);
		return 0;
	}
	vpss_mode_init();

	return 0;
}

static int vpss_release(struct inode *inode, struct file *filep)
{
	struct cvi_vpss_device *dev =
		container_of(filep->private_data, struct cvi_vpss_device, miscdev);
	int i;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss_release\n");

	if (!dev) {
		pr_err("Cannot find vpss private data\n");
		return -ENODEV;
	}

	i = atomic_dec_return(&open_count);
	if (i) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss_close: open %d times\n", i);
		return 0;
	}
	vpss_release_grp();
	vpss_mode_deinit();

	return 0;
}

static const struct file_operations vpss_fops = {
	.owner = THIS_MODULE,
	.open = vpss_open,
	.release = vpss_release,
	.unlocked_ioctl = vpss_ioctl,
};

static int register_vpss_dev(struct device *dev, struct cvi_vpss_device *vpss_dev)
{
	int rc;

	vpss_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	vpss_dev->miscdev.name =
		devm_kasprintf(dev, GFP_KERNEL, VPSS_DEV_NAME);
	vpss_dev->miscdev.fops = &vpss_fops;

	rc = misc_register(&vpss_dev->miscdev);
	if (rc)
		pr_err("vpss_dev: failed to register misc device.\n");

	return rc;
}

static int cvi_vpss_probe(struct platform_device *pdev)
{
	int rc = 0;
	int i;
	struct cvi_vpss_device *dev;

	/* allocate main structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to allocate resource\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, dev);

	// get hw-resources
	rc = vpss_init_resources(pdev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to init resource\n");
		goto err_dev;
	}

	rc = register_vpss_dev(&pdev->dev, dev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to register dev\n");
		goto err_dev;
	}

	rc = vpss_proc_init(dev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to init proc\n");
		goto err_proc;
	}

	for (i = 0; i < CVI_VPSS_MAX; ++i) {
		if (devm_request_irq(&pdev->dev, dev->vpss_cores[i].irq_num, vpss_isr, IRQF_SHARED,
			vpss_name[i], &dev->vpss_cores[i])) {
			dev_err(&pdev->dev, "Unable to request vpss IRQ(%d)\n",
					dev->vpss_cores[i].irq_num);
			goto err_irq;
		}
	}

	/* scaler register cb */
	if (vpss_core_register_cb(dev)) {
		dev_err(&pdev->dev, "Failed to register vpss cb, err %d\n", rc);
		goto err_irq;
	}

	vpss_dev_init(dev);

	CVI_TRACE_VPSS(CVI_DBG_WARN, "vpss probe done\n");

	return rc;

err_irq:
	vpss_proc_remove(dev);
err_proc:
	misc_deregister(&dev->miscdev);
err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	CVI_TRACE_VPSS(CVI_DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * bmd_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
static int cvi_vpss_remove(struct platform_device *pdev)
{
	struct cvi_vpss_device *dev;
	int i;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vpss drvdata");
		return -EINVAL;
	}

	vpss_dev_deinit(dev);
	/* scaler rm cb */
	if (vpss_core_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm vpss cb\n");
	}
	for (i = 0; i < CVI_VPSS_MAX; ++i)
		devm_free_irq(&pdev->dev, dev->vpss_cores[i].irq_num, &dev->vpss_cores[i]);

	vpss_proc_remove(dev);
	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	CVI_TRACE_VPSS(CVI_DBG_WARN, "vpss remove done\n");

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int vpss_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int vpss_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	//sclr_ctrl_init();
	//VIP_CLK_RATIO_CONFIG(ISP_TOP, 0x10);
	//VIP_CLK_RATIO_CONFIG(LDC, 0x10);
	//VIP_CLK_RATIO_CONFIG(IMG_D, 0x10);
	//VIP_CLK_RATIO_CONFIG(IMG_V, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_D, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V1, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V2, 0x10);

	//vip_sys_set_offline(VIP_SYS_AXI_BUS_SC_TOP, true);
	//vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_RAW, true);
	//vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_YUV, true);

	return 0;
}
#endif

static const struct of_device_id cvi_vpss_dt_match[] = {
	{.compatible = "cvitek,vpss"},
	{}
};

static SIMPLE_DEV_PM_OPS(vpss_pm_ops, vpss_suspend,
				vpss_resume);

#if (!DEVICE_FROM_DTS)
static void cvi_vpss_pdev_release(struct device *dev)
{
}

static struct platform_device cvi_vpss_pdev = {
	.name		= "vpss",
	.dev.release	= cvi_vpss_pdev_release,
};
#endif

static struct platform_driver cvi_vpss_pdrv = {
	.probe      = cvi_vpss_probe,
	.remove     = cvi_vpss_remove,
	.driver     = {
		.name		= "vpss",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cvi_vpss_dt_match,
#endif
		.pm		= &vpss_pm_ops,
	},
};

static int __init cvi_vpss_init(void)
{
	int rc;

	CVI_TRACE_VPSS(CVI_DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_vpss_pdrv);
	#else
	rc = platform_device_register(&cvi_vpss_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_vpss_pdrv);
	if (rc)
		platform_device_unregister(&cvi_vpss_pdev);
	#endif

	return rc;
}

static void __exit cvi_vpss_exit(void)
{
	CVI_TRACE_VPSS(CVI_DBG_INFO, " +\n");

	platform_driver_unregister(&cvi_vpss_pdrv);
	#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cvi_vpss_pdev);
	#endif
}

MODULE_DESCRIPTION("Cvitek Video Driver");
MODULE_AUTHOR("weiyong.luo");
MODULE_LICENSE("GPL");
module_init(cvi_vpss_init);
module_exit(cvi_vpss_exit);

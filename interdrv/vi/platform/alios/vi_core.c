#include "vi_core.h"
#include "base_cb.h"
#include "vi_common.h"
#include "vi_defines.h"
#include "vi_interfaces.h"
#include "vi_ioctl.h"
#include "vi_ctx.h"
#include "proc/vi_proc.h"
#include "drv/cvi_irq.h"
#include "driver_vi.h"
#include <aos/cli.h>

#define ISP_BASE_ADDR		(0x0A000000)
#define ISP_TOP_INT		20

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* control vi log level
 */
u32 vi_log_lv = VI_ERR | VI_WARN | VI_NOTICE | VI_INFO;
u32 patgen_vblanking = 112;

/*csi_be->clk_sys_3, clk_raw->clk_sys_1, clk_isp_top->clk_sys_1*/
const char * const clk_isp_name[] = {
	"reg_clk_csi_be_vip_en", "reg_clk_isp_top_vip_en"
};

/*clk_csi_mac0->clk_sys_3, clk_csi_mac1->clk_sys_2, clk_csi_mac2->clk_sys_0*/
const char * const clk_mac_name[] = {
	"reg_clk_csi_mac0_vip_en", "reg_clk_csi_mac1_vip_en", "reg_clk_csi_mac2_vip_en",
};

extern int tuning_dis[3];

static struct platform_vi_dev *g_vi_dev;

static int _vi_clk_ctrl(struct platform_vi_dev *vi_dev, u8 enable)
{
	u8 i = 0;
	int rc = 0;
	struct sop_vi_dev *vdev = &vi_dev->vdev;

	for (i = 0; i < ARRAY_SIZE(vdev->clk_isp); ++i) {
		if (vdev->clk_isp[i]) {
			if (enable) {
				osal_clk_prepare_enable(vdev->clk_isp[i]);
			} else {
				if (osal_clk_is_enabled(vdev->clk_isp[i]))
					osal_clk_disable_unprepare(vdev->clk_isp[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_isp(%d) is null\n", i);
			rc = OSAL_EAGAIN;
			goto EXIT;
		}
	}

	for (i = 0; i < ARRAY_SIZE(vdev->clk_mac); ++i) {
		if (vdev->clk_mac[i]) {
			if (enable) {
				osal_clk_prepare_enable(vdev->clk_mac[i]);
			} else {
				if (osal_clk_is_enabled(vdev->clk_mac[i]))
					osal_clk_disable_unprepare(vdev->clk_mac[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_mac(%d) is null\n", i);
			rc = OSAL_EAGAIN;
			goto EXIT;
		}
	}

	//Set axi_isp_top_clk_en 1
	//vi_sys_reg_write_mask(VI_SYS_REG_CLK_AXI_ISP_TOP_EN,
	//			VI_SYS_REG_CLK_AXI_ISP_TOP_EN_MASK,
	//			0x1 << VI_SYS_REG_CLK_AXI_ISP_TOP_EN_OFFSET);
EXIT:
	return rc;
}

int driver_vi_open(void)
{
	int ret = 0;
	struct platform_vi_dev *vi_dev = g_vi_dev;

	if (!vi_dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	if (!osal_atomic_read(&vi_dev->dev_open_cnt)) {

		_vi_clk_ctrl(vi_dev, true);

		vi_sw_init(&vi_dev->vdev);

		vi_pr(VI_INFO, "-\n");
	}

	osal_atomic_inc(&vi_dev->dev_open_cnt);

	return ret;
}

int driver_vi_release(void)
{
	int ret = 0;
	int open_cnt = 0;
	struct platform_vi_dev *vi_dev = g_vi_dev;

	if (!vi_dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	open_cnt = osal_atomic_dec_return(&vi_dev->dev_open_cnt);

	if (open_cnt < 0) {
		osal_atomic_set(&vi_dev->dev_open_cnt, 0);
	} else if (open_cnt == 0) {
		vi_sdk_release(&vi_dev->vdev);

		_vi_clk_ctrl(vi_dev, false);

		vi_pr(VI_INFO, "-\n");
	}

	return ret;
}

static int vi_event_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	struct isp_event_q *event_q = &vdev->event_q;

	return !osal_list_empty(&event_q->list);
}

static int vi_dbg_wait_cond_func(const void *param)
{
	int ret = 0;
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;

	if (osal_atomic_read(&vdev->isp_dbg_flag)) {
		osal_atomic_set(&vdev->isp_dbg_flag, 0);
		ret = 1;
	}

	return ret;
}

int driver_vi_poll(enum poll_type type)
{
	int ret = 0;
	struct platform_vi_dev *dev = g_vi_dev;
	struct sop_vi_dev *vdev = &dev->vdev;

	switch (type) {
	case POLL_TYPE_EVENT:
		/* code */
		if (vdev->isp_event_wait_q.wait) {
			ret = osal_wait_timeout_interruptible(&vdev->isp_event_wait_q,
							vi_event_wait_cond_func, vdev, 1000);
		}
		break;
	case POLL_TYPE_DBG:
		/* code */
		if (vdev->isp_dbg_wait_q.wait) {
			ret = osal_wait_uninterruptible(&vdev->isp_dbg_wait_q,
							vi_dbg_wait_cond_func, vdev);
		}

		break;
	default:
		break;
	}

	return ret ? 0 : OSAL_POLLIN | OSAL_POLLRDNORM;
}

static int vi_get_user(void *arg, void *parg, unsigned int cmd)
{
	struct vi_ext_control *ext_ctrl = (struct vi_ext_control *)arg;
	struct vi_ctrl *ctrl = (struct vi_ctrl *)parg;
	void *user_ptr = NULL;
	int size = 0;
	int ret = 0;
	int value = 0;

	switch (cmd) {
	case VI_IOC_S_CTRL:
	case VI_IOC_G_CTRL: {
		user_ptr = ext_ctrl->size ? ext_ctrl->ptr : NULL;
		size = ext_ctrl->size;
		value = ext_ctrl->value;
		ctrl->reserved[0] = ext_ctrl->reserved[0];
		break;
	}
	case VI_IOC_SDK_CTRL: {
		user_ptr = ext_ctrl->sdk_cfg.size ? ext_ctrl->sdk_cfg.ptr : NULL;
		size = ext_ctrl->sdk_cfg.size;
		value = ext_ctrl->sdk_cfg.value;
		ctrl->reserved[1] = ext_ctrl->sdk_cfg.reserved[0];
		break;
	}
	default:
		break;
	}

	if (size) {
		ctrl->ptr = user_ptr;
	} else {
		ctrl->val = value;
	}

	ctrl->dev = ext_ctrl->sdk_cfg.dev;
	ctrl->pipe = ext_ctrl->sdk_cfg.pipe;
	ctrl->chn = ext_ctrl->sdk_cfg.chn;
	ctrl->size = size;
	ctrl->id = ext_ctrl->id;

	return ret;
}

static int vi_put_user(void *arg, void *parg, unsigned int cmd)
{
	struct vi_ext_control *ext_ctrl = (struct vi_ext_control *)arg;
	struct vi_ctrl *ctrl = (struct vi_ctrl *)parg;
	int size = 0;
	int ret = 0;

	switch (cmd) {
	case VI_IOC_S_CTRL:
	case VI_IOC_G_CTRL: {
		size = ext_ctrl->size;
		ext_ctrl->value = ctrl->val;
		break;
	}
	case VI_IOC_SDK_CTRL: {
		size = ext_ctrl->sdk_cfg.size;
		ext_ctrl->sdk_cfg.value = ctrl->val;
		break;
	}
	default:
		break;
	}

	if (size && ctrl->ptr) {
		ctrl->ptr = NULL;
	}

	return ret;
}

long driver_vi_ioctl(unsigned int cmd, unsigned long arg)
{
	long	ret = 0;
	struct platform_vi_dev *dev = g_vi_dev;
	struct sop_vi_dev *vdev = &dev->vdev;
	struct vi_ext_control ext_ctrl;
	struct vi_ctrl ctrl;

	if (!dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	memset(&ext_ctrl, 0, sizeof(struct vi_ext_control));
	memset(&ctrl, 0, sizeof(struct vi_ctrl));

	osal_memcpy(&ext_ctrl, (void *)arg, sizeof(struct vi_ext_control));

	ret = vi_get_user(&ext_ctrl, &ctrl, cmd);
	if (ret)
		goto out;

	ret = vi_ioctl(vdev, cmd, &ctrl);

	vi_put_user(&ext_ctrl, &ctrl, cmd);

out:
	osal_memcpy((void *)arg, &ext_ctrl, sizeof(struct vi_ext_control));

	return ret;
}

int vi_core_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg)
{
	return vi_cb(dev, caller, cmd, arg);
}

static int vi_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VI);
}

static int vi_core_register_cb(struct sop_vi_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VI;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vi_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static void vi_core_isr(int irq, void *priv)
{
	struct sop_vi_dev *vdev = priv;

	vi_irq_handler(vdev);
}

static int vi_core_clk_init(struct platform_vi_dev *vi_dev)
{
	u8 i = 0;

	for (i = 0; i < ARRAY_SIZE(clk_isp_name); ++i) {
		vi_dev->vdev.clk_isp[i] = osal_clk_get(NULL, clk_isp_name[i]);
		if (vi_dev->vdev.clk_isp[i] == NULL) {
			vi_pr(VI_ERR, "Cannot get clk for %s\n", clk_isp_name[i]);
		}
	}

	for (i = 0; i < ARRAY_SIZE(clk_mac_name); ++i) {
		vi_dev->vdev.clk_mac[i] = osal_clk_get(NULL, clk_mac_name[i]);
		if (vi_dev->vdev.clk_mac[i] == NULL) {
			vi_pr(VI_ERR, "Cannot get clk for %s\n", clk_mac_name[i]);
		}
	}

	return 0;
}

static int vi_core_clk_deinit(struct platform_vi_dev *vi_dev)
{
	u8 i = 0;

	for (i = 0; i < ARRAY_SIZE(clk_isp_name); ++i) {
		osal_clk_put(NULL, vi_dev->vdev.clk_isp[i]);
		vi_dev->vdev.clk_isp[i] = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(clk_mac_name); ++i) {
		osal_clk_put(NULL, vi_dev->vdev.clk_mac[i]);
		vi_dev->vdev.clk_mac[i] = NULL;
	}

	return 0;
}

int driver_vi_init(void)
{
	struct sop_vi_dev *vdev;
	int ret = 0;

	vi_pr(VI_WARN, "+\n");

	if (g_vi_dev) {
		vi_pr(VI_ERR, "VI device is already initialized!\n");
		return OSAL_EINVAL;
	}

	g_vi_dev = osal_zalloc(sizeof(struct platform_vi_dev));
	if (!g_vi_dev)
		return OSAL_ENOMEM;

	vdev = &g_vi_dev->vdev;

	/* vi proc setup */
	vdev->shared_mem = osal_kzalloc(VI_SHARE_MEM_SIZE, OSAL_GFP_ATOMIC);
	if (!vdev->shared_mem)
		goto err_shared_mem;

	vdev->reg_base = (void *)ISP_BASE_ADDR;
	/* Interrupt */
	vdev->irq_num = ISP_TOP_INT;

	vi_pr(VI_INFO, "base_addr(%p). irq(%d)\n", vdev->reg_base, vdev->irq_num);

	ret = vi_core_clk_init(g_vi_dev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to init clk, err %d\n", ret);
		goto err_clk_init;
	}

	ret = vi_create_instance(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}
#if CONFIG_VI_SUPPORT_PROC
	ret = vi_create_proc(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create proc, err %d\n", ret);
		goto err_create_proc;
	}
#endif
	ret = request_irq(vdev->irq_num, vi_core_isr, 0, "vi_isr", vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to request irq_num(%d) ret(%d)\n",
				vdev->irq_num, ret);
		ret = OSAL_EINVAL;
		goto err_req_irq;
	}

	ret = vi_core_register_cb(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to register vi cb, err %d\n", ret);
		goto err_register_cb;
	}

	vi_pr(VI_WARN, "-\n");
	vi_pr(VI_INFO, "isp registered\n");

	return ret;
err_register_cb:
err_req_irq:
#if CONFIG_VI_SUPPORT_PROC
	vi_destroy_proc(vdev);
err_create_proc:
#endif
	vi_destroy_instance(vdev);
err_create_instance:
	vi_core_clk_deinit(g_vi_dev);
err_clk_init:
err_shared_mem:
	osal_free(g_vi_dev);
	g_vi_dev = NULL;
	return ret;
}

int driver_vi_exit(void)
{
	int ret = 0;
	struct platform_vi_dev *vi_dev = g_vi_dev;
	struct sop_vi_dev *vdev = &vi_dev->vdev;

	if (!vi_dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	ret = vi_destroy_instance(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vi_core_rm_cb();
	if (ret) {
		vi_pr(VI_ERR, "Failed to rm vi cb, err %d\n", ret);
	}

#if CONFIG_VI_SUPPORT_PROC
	vi_destroy_proc(vdev);
#endif

	osal_kfree(vdev->shared_mem);
	vdev->shared_mem = NULL;

	vi_core_clk_deinit(vi_dev);

	osal_free(g_vi_dev);
	g_vi_dev = NULL;

err_destroy_instance:
	vi_pr(VI_INFO, "%s -\n", __func__);

	return ret;
}

int vi_core_suspend(void)
{
	int ret = 0;
	struct platform_vi_dev *dev = g_vi_dev;

	if (!dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	osal_atomic_set(&dev->vdev.state, E_STATE_SUSPEND);

	ret = vi_suspend(&dev->vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to suspend vi, err %d\n", ret);
		goto err_suspend;
	}

	_vi_clk_ctrl(dev, false);

	vi_pr(VI_INFO, "-\n");

	return ret;

err_suspend:
	vi_resume(&dev->vdev);

	osal_atomic_set(&dev->vdev.state, E_STATE_DEFAULT);

	return ret;
}

int vi_core_resume(void)
{
	int ret = 0;
	struct platform_vi_dev *dev = g_vi_dev;
	struct sop_vi_dev *vdev = NULL;

	if (!dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	vdev = &dev->vdev;
	_vi_clk_ctrl(dev, true);

	ret = vi_resume(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to resume vi, err %d\n", ret);
		goto err_resume;
	}

	vi_pr(VI_INFO, "-\n");

	osal_atomic_set(&dev->vdev.state, E_STATE_DEFAULT);

	return ret;

err_resume:
	return ret;
}

static void vi_suspend_resume(int32_t argc, char **argv)
{
	int is_suspend = 0;

	if (argc < 2) {
		osal_printk("Usage: vi_suspend_resume [0|1]\n");
		return;
	}
	is_suspend = atoi(argv[1]);
	if (is_suspend < 0 || is_suspend > 3) {
		osal_printk("Invalid log level: %d\n", is_suspend);
		return;
	}

	if (is_suspend == 1) {
		vi_core_suspend();
	} else if (is_suspend == 0) {
		vi_core_resume();
	} else {
		osal_printk("Invalid log level: %d\n", is_suspend);
		return;
	}
}
ALIOS_CLI_CMD_REGISTER(vi_suspend_resume, vi_suspend_resume, vi_suspend_resume);

static void set_vi_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		osal_printk("Set vi_log_lv, (%d) -> (%d).\n", vi_log_lv, level);
		vi_log_lv = level;
	} else {
		osal_printk("vi_log_lv %d.\n", vi_log_lv);
	}
}

ALIOS_CLI_CMD_REGISTER(set_vi_log_level, vi_log_lv, set_vi_log_level);

static void set_tuning_dis(int32_t argc, char **argv)
{
	if (argc == 4) {
		tuning_dis[0] = atoi(argv[1]);
		tuning_dis[1] = atoi(argv[2]);
		tuning_dis[2] = atoi(argv[3]);
	} else {
		osal_printk("Usage: tuning_dis [pipe] [fe_ctrl] [post_ctrl] eg: tuning_dis 0 1 1.\n");
	}

	osal_printk("tuning_dis(%d %d %d).\n", tuning_dis[0], tuning_dis[1], tuning_dis[2]);
}

ALIOS_CLI_CMD_REGISTER(set_tuning_dis, tuning_dis, set_tuning_dis);

static void set_patgen_vblanking(int32_t argc, char **argv)
{
	if (argc == 2) {
		patgen_vblanking = atoi(argv[1]);
	} else {
		osal_printk("Usage: patgen_vblanking [num] eg: patgen_vblanking 120.\n");
	}

	osal_printk("patgen_vblanking(%d).\n", patgen_vblanking);
}

ALIOS_CLI_CMD_REGISTER(set_patgen_vblanking, patgen_vblanking, set_patgen_vblanking);


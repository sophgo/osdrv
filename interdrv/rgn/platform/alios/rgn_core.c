#include <stdlib.h>
#include <errno.h>
#include <aos/cli.h>
#include <rgn_core.h>
#include <base_cb.h>
#include "rgn_proc.h"

#define CVI_RGN_DEV_NAME            "soph-rgn"

static struct rgn_dev *gRdev;

int rgn_log_lv = RGN_WARN;

static int rgn_core_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg)
{
	return rgn_cb(dev, caller, cmd, arg);
}

static int rgn_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_RGN);
}

static int rgn_core_register_cb(struct rgn_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_RGN;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= rgn_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static int _rgn_create_proc(struct rgn_dev *rdev)
{
	int ret = OSAL_SUCCESS;

	(void)(rdev);
	return ret;
}

int rgn_create_instance(struct rgn_dev *prdev)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev = prdev;

	if (_rgn_create_proc(rdev)) {
		TRACE_RGN(RGN_ERR, "Failed to create proc\n");
		goto err;
	}
	gRdev = prdev;
	_rgn_sw_init(prdev);
	ret = OSAL_SUCCESS;

err:
	return ret;
}

// static int cvi_rgn_probe(struct platform_device *pdev)
int driver_rgn_init(void)
{
	int ret = 0;

	TRACE_RGN(RGN_WARN, "+\n");

	gRdev = malloc(sizeof(struct rgn_dev));
	if (!gRdev) {
		TRACE_RGN(RGN_ERR, "Failed to malloc size(%d)!\n", (int)sizeof(struct rgn_dev));
		return -ENOMEM;
	}
	memset(gRdev, 0, sizeof(struct rgn_dev));

	/* initialize locks */
	osal_spin_lock_init(&gRdev->lock);
	osal_mutex_init(&gRdev->mutex);

	ret = rgn_create_instance(gRdev);
	if (ret) {
		TRACE_RGN(RGN_ERR, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}

	if (rgn_core_register_cb(gRdev)) {
		TRACE_RGN(RGN_ERR, "Failed to register rgn cb, err %d\n", ret);
		goto err_create_instance;
	}

	TRACE_RGN(RGN_INFO, "done with ret(%d).\n", ret);
	TRACE_RGN(RGN_WARN, "-\n");
	return ret;

err_create_instance:
	TRACE_RGN(RGN_INFO, "failed with rc(%d).\n", ret);
	return ret;
}

static void _rgn_destroy_proc(struct rgn_dev *rdev)
{
}

int rgn_destroy_instance(struct rgn_dev *prdev)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev = prdev;

	_rgn_destroy_proc(rdev);
	_rgn_release_op(rdev);
	ret = OSAL_SUCCESS;

	return ret;
}

int driver_rgn_exit(void)
{
	int ret = 0;

	ret = rgn_destroy_instance(gRdev);
	if (ret) {
		TRACE_RGN(RGN_ERR, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	if (rgn_core_rm_cb()) {
		TRACE_RGN(RGN_ERR, "Failed to rm rgn cb, err %d\n", ret);
	}
    	free(gRdev);
err_destroy_instance:
	TRACE_RGN(RGN_INFO, "%s -\n", __func__);

	return ret;
}

void driver_rgn_release(void)
{
	TRACE_RGN(RGN_WARN, " +\n");
	_rgn_release_all_region();
	TRACE_RGN(RGN_WARN, " -\n");
}

long driver_rgn_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev = gRdev;
	struct rgn_ext_control p;

	if (memcpy(&p, (void *)arg, sizeof(struct rgn_ext_control)) == NULL)
		return ret;

	switch (cmd) {
	case RGN_IOC_S_CTRL:
		ret = _rgn_s_ctrl(rdev, &p);
		break;
	case RGN_IOC_G_CTRL:
		ret = _rgn_g_ctrl(rdev, &p);
		break;
	default:
		ret = -1;
		break;
	}

	if (memcpy((void *)arg, &p, sizeof(struct rgn_ext_control)) == NULL)
		return ret;

	return ret;
}

static void set_rgn_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		aos_debug_printf("Set rgn_log_lv, (%d) -> (%d).\n", rgn_log_lv, level);
		rgn_log_lv = level;
	} else {
		aos_debug_printf("rgn_log_lv = %d.\n", rgn_log_lv);
	}
}

ALIOS_CLI_CMD_REGISTER(set_rgn_log_level, rgn_log_lv, set_rgn_log_lv);

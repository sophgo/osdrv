#include "vo_ctx.h"
#include "vo_process.h"
#include "base_cb.h"
#include "vo_platform.h"
#include "dsi_mac.h"
#include "dsi_phy.h"
#include "vo_mac.h"
#include "vo_debug.h"
#include <aos/cli.h>
#include <stdlib.h>

#define REG_DISP_BASE(x) (0x0A094000)
#define REG_DSI_MAC_BASE(x) (0x0A099000)
#define REG_DSI_WRAP_BASE(x) (0x0A098000)
#define REG_VO_MAC_BASE(x) (0x0A0A8000)

int driver_vo_init()
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < DISP_MAX_INST; ++i) {
		disp_set_disp_base_addr(i, (void *)REG_DISP_BASE(i));
		dsi_mac_set_base_addr(i, (void *)REG_DSI_MAC_BASE(i));
		dphy_set_base_addr(i, (void *)REG_DSI_WRAP_BASE(i));
		vo_mac_set_base_addr(i, (void *)REG_VO_MAC_BASE(i));
	}

	ret = vo_create_instance();
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to create instance, err %d\n", ret);
		return ret;
	}

	ret = vo_core_register_cb(g_vo_ctx);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to register vo cb, err %d\n", ret);
		goto vo_core_register_cb_err;
	}

	TRACE_VO(DBG_ERR, "driver_vo_init\n");

	return ret;

vo_core_register_cb_err:
	vo_destroy_instance();
	return ret;
}

int driver_vo_exit()
{
	int ret = 0;

	ret = vo_destroy_instance();
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vo_core_rm_cb();
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to rm vo cb, err %d\n", ret);
	}

err_destroy_instance:
	TRACE_VO(DBG_INFO, "%s -\n", __func__);

	return ret;
}

int driver_vo_release()
{
	vo_release();
	return 0;
}

static void set_vo_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		aos_debug_printf("Set vo_log_lv, (%d) -> (%d).\n", vo_log_lv, level);
		vo_log_lv = level;
	} else {
		aos_debug_printf("vo_log_lv = %d.\n", vo_log_lv);
	}
}

ALIOS_CLI_CMD_REGISTER(set_vo_log_level, vo_log_lv, set_vo_log_lv);

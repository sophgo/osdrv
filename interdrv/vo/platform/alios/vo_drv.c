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

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static const char *const clk_vo_name[] = {
	"reg_clk_disp_vip_en", "reg_clk_dsi_mac_vip_en", "reg_clk_vo_mac_vip_en"
};

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

	for (i = 0; i < ARRAY_SIZE(clk_vo_name); ++i) {
		g_vo_ctx->clk_vo[i] = osal_clk_get(NULL, clk_vo_name[i]);
		if (g_vo_ctx->clk_vo[i] == NULL) {
			TRACE_VO(DBG_ERR, "Cannot get clk for %s\n", clk_vo_name[i]);
		}
		if (g_vo_ctx->clk_vo[i]) {
			osal_clk_prepare_enable(g_vo_ctx->clk_vo[i]);
		}
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
	int i = 0;

	ret = vo_destroy_instance();
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vo_core_rm_cb();
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to rm vo cb, err %d\n", ret);
	}

	for (i = 0; i < ARRAY_SIZE(g_vo_ctx->clk_vo); ++i) {
		if ((g_vo_ctx->clk_vo[i]) && osal_clk_is_enabled(g_vo_ctx->clk_vo[i])) {
			osal_clk_disable_unprepare(g_vo_ctx->clk_vo[i]);
			osal_clk_put(NULL, g_vo_ctx->clk_vo[i]);
			g_vo_ctx->clk_vo[i] = NULL;
		}
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

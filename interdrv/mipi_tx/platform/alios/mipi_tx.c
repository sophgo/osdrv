#include <debug/dbg.h>
#include <drv/tick.h>
#include "osal.h"
#include "comm_mipi_tx.h"
#include "base_ctx.h"
#include "disp.h"
#include "dsi_mac.h"
#include "dsi_phy.h"

/*
 * macro definition
 */
#define MIPI_TX_DEV_NAME "soph-mipi-tx"
#define MIPI_TX_PROC_NAME "soph_mipi_tx"

#define MIPI_TX_INFO(fmt, ...) \
		aos_debug_printf("[%d]%s:%d(): " fmt, csi_tick_get_ms(), __func__, __LINE__, ##__VA_ARGS__);

#define MIPI_TX_ERR(fmt, ...) \
		aos_debug_printf("[%d]%s:%d(): " fmt, csi_tick_get_ms(), __func__, __LINE__, ##__VA_ARGS__);

struct mipi_tx_dev {
	struct combo_dev_cfg_s dev_cfg[DISP_MAX_INST];
} mipi_tx_dev_ctx;

//to do: smooth from uboot
// static int smooth;
static int debug = 1;

/*
 * global variables definition
 */

/*
 * function definition
 */
static int mipi_tx_check_comb_dev_cfg(struct combo_dev_cfg_s *dev_cfg)
{
	if (dev_cfg->output_mode != OUTPUT_MODE_DSI_VIDEO) {
		MIPI_TX_ERR("output_mode(%d) not supported!\n", dev_cfg->output_mode);
		return OSAL_EINVAL;
	}

	if (dev_cfg->video_mode != BURST_MODE) {
		MIPI_TX_ERR("video_mode(%d) not supported!\n", dev_cfg->video_mode);
		return OSAL_EINVAL;
	}

	return 0;
}

static void _fill_disp_timing(struct disp_timing *timing, struct sync_info_s *sync_info)
{
	timing->vtotal = sync_info->vid_vsa_lines + sync_info->vid_vbp_lines +
			 sync_info->vid_active_lines + sync_info->vid_vfp_lines - 1;
	timing->htotal = sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels +
			 sync_info->vid_hline_pixels + sync_info->vid_hfp_pixels - 1;
	timing->vsync_start = 0;
	timing->vsync_end = timing->vsync_start + sync_info->vid_vsa_lines - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + sync_info->vid_vsa_lines + sync_info->vid_vbp_lines;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + sync_info->vid_active_lines - 1;
	timing->hsync_start = 0;
	timing->hsync_end = timing->hsync_start + sync_info->vid_hsa_pixels - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + sync_info->vid_hline_pixels - 1;
	timing->vsync_pol = sync_info->vid_vsa_pos_polarity;
	timing->hsync_pol = sync_info->vid_hsa_pos_polarity;
}

static void _get_sync_info(struct disp_timing timing, struct sync_info_s *sync_info)
{
	sync_info->vid_hsa_pixels = timing.hsync_end - timing.hsync_start + 1;
	sync_info->vid_hbp_pixels = timing.hfde_start - timing.hsync_start - sync_info->vid_hsa_pixels;
	sync_info->vid_hline_pixels = timing.hfde_end - timing.hfde_start + 1;
	sync_info->vid_hfp_pixels = timing.htotal - sync_info->vid_hsa_pixels -
				    sync_info->vid_hbp_pixels - sync_info->vid_hline_pixels + 1;
	sync_info->vid_vsa_lines = timing.vsync_end - timing.vsync_start + 1;
	sync_info->vid_vbp_lines = timing.vfde_start - timing.vsync_start - sync_info->vid_vsa_lines;
	sync_info->vid_active_lines = timing.vfde_end - timing.vfde_start + 1;
	sync_info->vid_vfp_lines = timing.vtotal - sync_info->vid_vsa_lines -
				   sync_info->vid_vbp_lines - sync_info->vid_active_lines + 1;
	sync_info->vid_vsa_pos_polarity = timing.vsync_pol;
	sync_info->vid_hsa_pos_polarity = timing.hsync_pol;
}

static int mipi_tx_set_combo_dev_cfg(struct mipi_tx_dev *tdev, struct combo_dev_cfg_s *dev_cfg, int devno)
{
	int ret, i;
	bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

	u8 lane_num = 0;
	struct disp_timing timing;
	enum dsi_fmt dsi_fmt;
	u8 bits;
	bool preamble_on = false;

	ret = mipi_tx_check_comb_dev_cfg(dev_cfg);
	if (ret < 0) {
		MIPI_TX_ERR("mipi_tx check combo_dev config failed!\n");
		return OSAL_EINVAL;
	}

	dphy_dsi_disable_lanes(devno);
	for (i = 0; i < LANE_MAX_NUM; i++) {
		if ((dev_cfg->lane_id[i] < 0) || (dev_cfg->lane_id[i] >= MIPI_TX_LANE_MAX)) {
			dphy_dsi_set_lane(devno, i, MIPI_TX_LANE_MAX, false, true);
			continue;
		}
		dphy_dsi_set_lane(devno, i, dev_cfg->lane_id[i], dev_cfg->lane_pn_swap[i], true);
		if (dev_cfg->lane_id[i] != MIPI_TX_LANE_CLK) {
			++lane_num;
			data_en[dev_cfg->lane_id[i] - 1] = true;
		}
	}
	if (lane_num == 0) {
		MIPI_TX_ERR("no active mipi-dsi lane\n");
		return OSAL_EINVAL;
	}

	disp_set_intf(devno, VO_DISP_INTF_DSI);
	_fill_disp_timing(&timing, &dev_cfg->sync_info);

	switch (dev_cfg->output_format) {
	case OUT_FORMAT_RGB_16_BIT:
		bits = 16;
		dsi_fmt = DSI_FMT_RGB565;
	break;

	case OUT_FORMAT_RGB_18_BIT:
		bits = 18;
		dsi_fmt = DSI_FMT_RGB666;
	break;

	case OUT_FORMAT_RGB_24_BIT:
		bits = 24;
		dsi_fmt = DSI_FMT_RGB888;
	break;

	case OUT_FORMAT_RGB_30_BIT:
		bits = 30;
		dsi_fmt = DSI_FMT_RGB101010;
	break;

	default:
	return OSAL_EINVAL;
	}

	preamble_on = (dev_cfg->pixel_clk * bits / lane_num) > 2500000;
	dphy_dsi_lane_en(devno, true, data_en, preamble_on);
	dphy_dsi_set_pll(devno, dev_cfg->pixel_clk, lane_num, bits);
	dsi_config(devno, lane_num, dsi_fmt, dev_cfg->sync_info.vid_hline_pixels);
	disp_set_timing(devno, &timing);
	// disp_clk_enable(devno, true);
	disp_tgen_enable(devno, true);

	MIPI_TX_INFO("lane_num(%d) preamble_on(%d) dsi_fmt(%d) bits(%d)\n", lane_num, preamble_on, dsi_fmt, bits);

	return ret;
}

int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg, int devno)
{
	int ret = 0;
	char data_lan_num = 0;
	struct disp_timing timing;

	if (dev_cfg == NULL) {
		MIPI_TX_ERR("ptr dev_cfg is NULL!\n");
		return OSAL_EINVAL;
	}

	data_lan_num = dphy_dsi_get_lane(devno, dev_cfg->lane_id);
	dev_cfg->video_mode = BURST_MODE;
	dev_cfg->output_mode = OUTPUT_MODE_DSI_VIDEO;
	dev_cfg->output_format = OUT_FORMAT_RGB_24_BIT;

	dev_cfg->devno = devno;
	disp_get_hw_timing(devno, &timing);
	_get_sync_info(timing, &dev_cfg->sync_info);
	dphy_dsi_get_pixclk(devno, &dev_cfg->pixel_clk, data_lan_num, 24);

	return ret;
}

static int mipi_tx_set_cmd(struct cmd_info_s *cmd_info, int devno)
{
	int i;
	char str[160];

	if (cmd_info->cmd_size > CMD_MAX_NUM) {
		MIPI_TX_ERR("cmd_size(%d) can't exceed %d!\n", cmd_info->cmd_size, CMD_MAX_NUM);
		return OSAL_EINVAL;
	} else if ((cmd_info->cmd_size != 0) && (cmd_info->cmd == NULL)) {
		MIPI_TX_ERR("cmd is NULL, but cmd_size(%d) isn't zero!\n", cmd_info->cmd_size);
		return OSAL_EINVAL;
	}

	snprintf(str, 160, "%s: ", __func__);
	for (i = 0; i < cmd_info->cmd_size && i < 16; ++i)
		snprintf(str + strlen(str), 160 - strlen(str), "%#x ", cmd_info->cmd[i]);
	MIPI_TX_INFO("%s\n", str);

	return dsi_dcs_write_buffer(devno, cmd_info->data_type, cmd_info->cmd, cmd_info->cmd_size, debug & 0x01);
}

static int mipi_tx_get_cmd(struct get_cmd_info_s *get_cmd_info, int devno)
{
	if (get_cmd_info->get_data_size > RX_MAX_NUM) {
		MIPI_TX_ERR("get_data_size(%d) can't exceed %d!\n", get_cmd_info->get_data_size, RX_MAX_NUM);
		return OSAL_EINVAL;
	} else if ((get_cmd_info->get_data_size != 0) && (get_cmd_info->get_data == NULL)) {
		MIPI_TX_ERR("cmd is NULL, but cmd_size(%d) isn't zero!\n", get_cmd_info->get_data_size);
		return OSAL_EINVAL;
	}

	return dsi_dcs_read_buffer(devno, get_cmd_info->data_type, get_cmd_info->data_param
		, get_cmd_info->get_data, get_cmd_info->get_data_size);
}

static void mipi_tx_enable(int devno)
{
	dsi_set_mode(devno, DSI_MODE_HS);
	osal_usleep_range(1000, 2000);
}

static void mipi_tx_disable(int devno)
{
	int ret = 0;
	int count = 0;

	dsi_set_mode(devno, DSI_MODE_IDLE);
	do {
		osal_usleep_range(1000, 2000);
		ret = dsi_chk_mode_done(devno, DSI_MODE_IDLE);
	} while ((ret != 0) && (count++ < 20));
}

int driver_mipi_tx_ioctl(unsigned int cmd, unsigned long arg)
{
	struct mipi_tx_dev *tdev = &mipi_tx_dev_ctx;
	int rc = 0;
	int devno = 0;

	switch (cmd) {
	case MIPI_TX_SET_DEV_CFG: {
		struct combo_dev_cfg_s stcombo_dev_cfg;
		// CHECK_IOCTL_CMD(cmd, struct combo_dev_cfg_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = OSAL_EINVAL;
			break;
		}
		if (osal_copy_from_user(&stcombo_dev_cfg, (void *)arg, sizeof(stcombo_dev_cfg))) {
			MIPI_TX_ERR("osal_copy_from_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}

		rc = mipi_tx_set_combo_dev_cfg(tdev, &stcombo_dev_cfg, devno);
		if (rc < 0) {
			MIPI_TX_ERR("mipi_tx set combo_dev config failed!\n");
		} else {
			tdev->dev_cfg[devno] = stcombo_dev_cfg;
		}
	}
	break;

	case MIPI_TX_GET_DEV_CFG: {
		struct combo_dev_cfg_s stcombo_dev_cfg;
		// CHECK_IOCTL_CMD(cmd, struct combo_dev_cfg_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = OSAL_EINVAL;
			break;
		}

		memset(&stcombo_dev_cfg, 0, sizeof(stcombo_dev_cfg));
		rc = mipi_tx_get_combo_dev_cfg(&stcombo_dev_cfg, devno);
		if (rc < 0)
			MIPI_TX_ERR("mipi_tx get combo_dev config failed!\n");

		if (osal_copy_to_user((void *)arg, &stcombo_dev_cfg, sizeof(stcombo_dev_cfg))) {
			MIPI_TX_ERR("osal_copy_to_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}
	}
	break;

	case MIPI_TX_SET_CMD: {
		struct cmd_info_s cmd_info;
		// CHECK_IOCTL_CMD(cmd, struct cmd_info_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = OSAL_EINVAL;
			break;
		}
		if (osal_copy_from_user(&cmd_info, (void *)arg, sizeof(cmd_info))) {
			MIPI_TX_ERR("osal_copy_from_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}
		if (cmd_info.cmd_size == 0) {
			MIPI_TX_ERR("cmd_size zero.\n");
			rc = OSAL_EINVAL;
			break;
		}

		// if cmd is NULL, use cmd_size as cmd.
		if (cmd_info.cmd == NULL) {
			cmd_info.cmd = osal_kmalloc(2, OSAL_GFP_KERNEL);
			if (cmd_info.cmd == NULL) {
				MIPI_TX_ERR("osal_kmalloc failed.\n");
				rc = OSAL_ENOMEM;
				break;
			}
			cmd_info.cmd[0] = cmd_info.cmd_size & 0xff;
			cmd_info.cmd[1] = (cmd_info.cmd_size >> 8) & 0xff;
			cmd_info.cmd_size = (cmd_info.data_type == 0x05) ? 1 : 2;
		} else {
			unsigned char *tmp_cmd = osal_kmalloc(cmd_info.cmd_size, OSAL_GFP_KERNEL);

			if (tmp_cmd == NULL) {
				MIPI_TX_ERR("osal_kmalloc failed.\n");
				rc = OSAL_ENOMEM;
				break;
			}

			if (osal_copy_from_user(tmp_cmd, (void *)cmd_info.cmd, cmd_info.cmd_size)) {
				MIPI_TX_ERR("cmd osal_copy_from_user failed.\n");
				rc = OSAL_ENOMEM;
				break;
			}
			cmd_info.cmd = tmp_cmd;
		}

		rc = mipi_tx_set_cmd(&cmd_info, cmd_info.devno);
		if (rc < 0)
			MIPI_TX_ERR("mipi_tx set cmd failed!\n");
		osal_kfree(cmd_info.cmd);
	}
	break;

	case MIPI_TX_GET_CMD: {
		struct get_cmd_info_s get_cmd_info;
		// CHECK_IOCTL_CMD(cmd, struct get_cmd_info_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = OSAL_EINVAL;
			break;
		}
		if (osal_copy_from_user(&get_cmd_info, (void *)arg, sizeof(get_cmd_info))) {
			MIPI_TX_ERR("osal_copy_from_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}

		rc = mipi_tx_get_cmd(&get_cmd_info, devno);
		if (rc < 0) {
			MIPI_TX_ERR("mipi_tx get cmd failed!\n");
			break;
		}
		if (osal_copy_to_user((void *)arg, &get_cmd_info, sizeof(get_cmd_info))) {
			MIPI_TX_ERR("osal_copy_to_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}
	}
	break;

	case MIPI_TX_ENABLE: {
		MIPI_TX_INFO("MIPI_TX_ENABLE devno(%d)\n", devno);
		mipi_tx_enable(devno);
	}
	break;

	case MIPI_TX_DISABLE: {
		mipi_tx_disable(devno);
	}
	break;

	case MIPI_TX_SET_HS_SETTLE: {
		struct hs_settle_s settle_cfg;
		// CHECK_IOCTL_CMD(cmd, struct hs_settle_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = OSAL_EINVAL;
			break;
		}
		if (osal_copy_from_user(&settle_cfg, (void *)arg, sizeof(settle_cfg))) {
			MIPI_TX_ERR("osal_copy_from_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}

		MIPI_TX_INFO("Set hs settle: prepare(%d) zero(%d) trail(%d)\n",
				     settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
		dphy_set_hs_settle(devno, settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
	}
	break;

	case MIPI_TX_GET_HS_SETTLE: {
		struct hs_settle_s settle_cfg;
		// CHECK_IOCTL_CMD(cmd, struct hs_settle_s);

		if (arg == 0) {
			MIPI_TX_ERR("NULL pointer.\n");
			rc = OSAL_EINVAL;
			break;
		}

		dphy_get_hs_settle(devno, &settle_cfg.prepare, &settle_cfg.zero, &settle_cfg.trail);
		MIPI_TX_INFO("Get hs settle: prepare(%d) zero(%d) trail(%d)\n",
				     settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
		if (osal_copy_to_user((void *)arg, &settle_cfg, sizeof(settle_cfg))) {
			MIPI_TX_ERR("osal_copy_to_user failed.\n");
			rc = OSAL_ENOMEM;
			break;
		}
	}
	break;

	case MIPI_TX_SUSPEND: {

	}
	break;

	case MIPI_TX_RESUME: {

	}
	break;

	default: {
		MIPI_TX_ERR("invalid mipi_tx ioctl cmd\n");
		rc = OSAL_EINVAL;
	}
	break;
	}

	return rc;
}

int driver_mipi_tx_init()
{
	return 0;
}

int driver_mipi_tx_exit()
{
	return 0;
}

#if ((CONFIG_MIPI_TX_SUPPORT_PROC) && (CONFIG_SUPPORT_VO))
#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "osal.h"
#include "comm_mipi_tx.h"
#include "driver_mipi_tx.h"

static int mipi_tx_devno = 0;

extern int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg, int devno);

/*************************************************************************
 *	MIPI_Tx proc functions
 *************************************************************************/
static int mipi_tx_proc_show_status()
{
	struct combo_dev_cfg_s mipi_tx_proc_ctx;
	enum mipi_tx_lane_id data_lane_id[LANE_MAX_NUM - 1] = {-1, -1, -1, -1};
	int phy_data_rate = 0, bits_per_pixel = 0;
	int i = 0, data_lane_num = 0;

	mipi_tx_get_combo_dev_cfg(&mipi_tx_proc_ctx, mipi_tx_devno);

	for (i = 0, data_lane_num = 0; i < LANE_MAX_NUM; ++i, ++data_lane_num) {
		if (mipi_tx_proc_ctx.lane_id[i] != MIPI_TX_LANE_CLK &&
		    mipi_tx_proc_ctx.lane_id[i] != -1)
			data_lane_id[mipi_tx_proc_ctx.lane_id[i] - 1] = data_lane_num + 1;
		else
			--data_lane_num;
	}

	switch (mipi_tx_proc_ctx.output_format) {
	case (OUT_FORMAT_RGB_16_BIT):
	case (OUT_FORMAT_YUV422_8_BIT):
		bits_per_pixel = 16;
		break;

	case (OUT_FORMAT_RGB_18_BIT):
	case (OUT_FORMAT_RGB_24_BIT):
		bits_per_pixel = 24;
		break;

	case (OUT_FORMAT_RGB_30_BIT):
		bits_per_pixel = 32;
		break;

	case (OUT_FORMAT_YUV420_8_BIT_NORMAL):
	case (OUT_FORMAT_YUV420_8_BIT_LEGACY):
		bits_per_pixel = 12;
		break;

	default:
		bits_per_pixel = 24;
		break;
	}

	if (data_lane_num == 0)
		memset(data_lane_id, 0, sizeof(data_lane_id));
	else {
		phy_data_rate = mipi_tx_proc_ctx.pixel_clk * bits_per_pixel * 10 / 8 / data_lane_num / 1000;
		phy_data_rate = phy_data_rate % 10 ? phy_data_rate / 10 + 1 : phy_data_rate / 10;
	}

	// printf("\nModule: [MIPI_TX], Build Time[%s]\n", UTS_VERSION);
	// MIPI_Tx DEV CONFIG
	printf("\n------MIPI_Tx%d DEV CONFIG----------------------------------------\n",
		   mipi_tx_proc_ctx.devno);
	printf("%10s%10s%10s%10s%10s%15s%15s%15s%15s%15s\n",
		   "devno", "lane0", "lane1", "lane2", "lane3", "output_mode",
		   "phy_data_rate", "pixel_clk(KHz)", "video_mode", "output_fmt");
	printf("%10d%10d%10d%10d%10d%15d%15d%15d%15d%15d\n",
		   mipi_tx_proc_ctx.devno,
		   data_lane_id[0],
		   data_lane_id[1],
		   data_lane_id[2],
		   data_lane_id[3],
		   mipi_tx_proc_ctx.output_mode,
		   phy_data_rate,
		   mipi_tx_proc_ctx.pixel_clk,
		   mipi_tx_proc_ctx.video_mode,
		   mipi_tx_proc_ctx.output_format);

	// MIPI_Tx SYNC CONFIG
	printf("\n------MIPI_Tx SYNC CONFIG---------------------------------------------\n");
	printf("%15s%15s%15s%15s%15s%15s%15s%15s%15s\n",
		   "pkt_size", "hsa_pixels", "hbp_pixels", "hline_pixels", "vsa_lines",
		   "vbp_lines", "vfp_lines", "active_lines", "edpi_cmd_size");
	printf("%15d%15d%15d%15d%15d%15d%15d%15d%15d\n",
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hfp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_vsa_lines,
		   mipi_tx_proc_ctx.sync_info.vid_vbp_lines,
		   mipi_tx_proc_ctx.sync_info.vid_vfp_lines,
		   mipi_tx_proc_ctx.sync_info.vid_active_lines,
		   mipi_tx_proc_ctx.sync_info.edpi_cmd_size);

	// MIPI_Tx DEV STATUS
	printf("\n------MIPI_Tx DEV STATUS----------------------------------------------\n");
	printf("%15s%15s%15s%15s%10s%10s%10s\n",
		   "width", "height", "HoriAll", "VertAll", "hbp", "hsa", "vsa");
	printf("%15d%15d%15d%15d%10d%10d%10d\n",
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_active_lines,
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hfp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_active_lines +
		   mipi_tx_proc_ctx.sync_info.vid_vfp_lines +
		   mipi_tx_proc_ctx.sync_info.vid_vbp_lines +
		   mipi_tx_proc_ctx.sync_info.vid_vsa_lines,
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_vsa_lines);

	return 0;
}

static void mipi_tx_proc_show(int32_t argc, char **argv)
{
	mipi_tx_proc_show_status();
}

ALIOS_CLI_CMD_REGISTER(mipi_tx_proc_show, proc_mipi_tx, mipi_tx info);
#endif
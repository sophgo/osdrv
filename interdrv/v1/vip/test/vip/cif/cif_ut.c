#include <ut_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_vip_cif.h>
#include <unistd.h>
#include <errno.h>
#include "../common/vip_msg_com.h"

#define CIF_DEFAULT_DEVNO 0

const char *dev_cif = "/dev/mipi-rx";
int ut_fd_cif;

struct combo_dev_attr_s imx290_rx_attr = {
	.input_mode = INPUT_MODE_SUBLVDS,
	.lvds_attr = {
		.wdr_mode = CVI_WDR_MODE_DOL_2F,
		.sync_mode = LVDS_SYNC_MODE_SAV,
		.raw_data_type = RAW_DATA_10BIT,
		.data_endian = LVDS_ENDIAN_BIG,
		.sync_code_endian = LVDS_ENDIAN_BIG,
		.lane_id = {0, 1, 2, -1, -1},
		.sync_code = {
			{
				{0x004, 0x1D4, 0x404, 0x5D4},
				{0x008, 0x1D8, 0x408, 0x5D8},
				{0x00C, 0x1DC, 0x40C, 0x5DC},
			},
		},
		.vsync_type = {
			.sync_type = LVDS_VSYNC_NORMAL,
		},
	},
	.devno = CIF_DEFAULT_DEVNO,
};

unsigned int mipi_devno = CIF_DEFAULT_DEVNO;

struct manual_wdr_s wdr_manual = {
	.devno = CIF_DEFAULT_DEVNO,
	.attr = {
		.manual_en = 1,
		.l2s_distance = 8,
		.lsef_length = 720,
		.discard_padding_lines = 1,
		.update = 1,
	},
};

/*************************************************************
 *	Private	functions
 *************************************************************/
static enum cvi_errno _cif_set_dev_attr(struct combo_dev_attr_s *attr)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(ut_fd_cif, CVI_MIPI_SET_DEV_ATTR, (void *)attr) < 0)
		ut_pr(UT_INFO, "errno %d\n", errno);
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_set_dev_attr(void)
{
	return _cif_set_dev_attr(&imx290_rx_attr);
}

static enum cvi_errno _cif_set_output_clk_edge(struct clk_edge_s *edge)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(ut_fd_cif, CVI_MIPI_SET_OUTPUT_CLK_EDGE,
			(void *)edge) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_set_output_clk_edge(void)
{
	struct clk_edge_s clk_edge;

	clk_edge.devno = CIF_DEFAULT_DEVNO;
	ut_pr(UT_INFO, "edge:");
	scanf("%u", (unsigned int *)&clk_edge.edge);

	return _cif_set_output_clk_edge(&clk_edge);
}

static enum cvi_errno _cif_reset_mipi(unsigned int devno)
{
	enum cvi_errno ret = ERR_NONE;

	mipi_devno = devno;

	if (ioctl(ut_fd_cif, CVI_MIPI_RESET_MIPI, (void *)&mipi_devno) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_reset_mipi(void)
{
	return _cif_reset_mipi(CIF_DEFAULT_DEVNO);
}

static enum cvi_errno _cif_set_manaul(struct manual_wdr_s *wdr_manu)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(ut_fd_cif, CVI_MIPI_SET_WDR_MANUAL,
			(void *)wdr_manu) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_set_manaul(void)
{
	return _cif_set_manaul(&wdr_manual);
}

static enum cvi_errno _cif_crop_top(struct crop_top_s *crop)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(ut_fd_cif, CVI_MIPI_SET_CROP_TOP,
			(void *)crop) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_crop_top(void)
{
	struct crop_top_s crop;

	crop.devno = CIF_DEFAULT_DEVNO;
	ut_pr(UT_INFO, "crop_top:");
	scanf("%d", (int *)&crop.crop_top);
	crop.update = 1;

	return _cif_crop_top(&crop);
}

static enum cvi_errno _cif_set_lvds_fp_vs(struct vsync_gen_s *fp_vs)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(ut_fd_cif, CVI_MIPI_SET_LVDS_FP_VS,
			(void *)fp_vs) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_set_lvds_fp_vs(void)
{
	struct vsync_gen_s fp_vs;

	fp_vs.devno = CIF_DEFAULT_DEVNO;
	ut_pr(UT_INFO, "distance_fp:");
	scanf("%d", (int *)&fp_vs.distance_fp);

	return _cif_set_lvds_fp_vs(&fp_vs);
}

static enum cvi_errno _cif_reset_snsr(unsigned int devno)
{
	enum cvi_errno ret = ERR_NONE;

	mipi_devno = devno;

	ut_pr(UT_INFO, "%u", mipi_devno);
	if (ioctl(ut_fd_cif, CVI_MIPI_RESET_SENSOR, &mipi_devno) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_reset_snsr(void)
{
	return _cif_reset_snsr(CIF_DEFAULT_DEVNO);
}

static enum cvi_errno _cif_unreset_snsr(unsigned int devno)
{
	enum cvi_errno ret = ERR_NONE;

	mipi_devno = devno;

	if (ioctl(ut_fd_cif, CVI_MIPI_UNRESET_SENSOR, &mipi_devno) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_unreset_snsr(void)
{
	return _cif_unreset_snsr(CIF_DEFAULT_DEVNO);
}

static enum cvi_errno _cif_enable_snsr_clk(unsigned int devno)
{
	enum cvi_errno ret = ERR_NONE;

	mipi_devno = devno;

	if (ioctl(ut_fd_cif, CVI_MIPI_ENABLE_SENSOR_CLOCK, &mipi_devno) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_enable_snsr_clk(void)
{
	return _cif_enable_snsr_clk(CIF_DEFAULT_DEVNO);
}

static enum cvi_errno _cif_disable_snsr_clk(unsigned int devno)
{
	enum cvi_errno ret = ERR_NONE;

	mipi_devno = devno;

	if (ioctl(ut_fd_cif, CVI_MIPI_DISABLE_SENSOR_CLOCK, &mipi_devno) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno cif_disable_snsr_clk(void)
{
	return _cif_disable_snsr_clk(CIF_DEFAULT_DEVNO);
}

static enum cvi_errno cif_manual(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	int cmd, run = 1;
	char str[20];

	while (run) {
		ut_pr(UT_INFO, "0-SET_DEV_ATTR\n");
		ut_pr(UT_INFO, "1-SET_OUTPUT_CLKC_EDGE\n");
		ut_pr(UT_INFO, "2-RESET_MIPI\n");
		ut_pr(UT_INFO, "3-SET_CROP_TOP\n");
		ut_pr(UT_INFO, "4-SET_WDR_MANUAL\n");
		ut_pr(UT_INFO, "5-SET_LVDS_FP_VS\n");
		ut_pr(UT_INFO, "6-RESET_SENSOR\n");
		ut_pr(UT_INFO, "7-UNRESET_SENSOR\n");
		ut_pr(UT_INFO, "8-ENABLE_SENSOR_CLOCK\n");
		ut_pr(UT_INFO, "9-DISABLE_SENSOR_CLOCK\n");
		ut_pr(UT_INFO, "255-Exit\n");
		scanf("%d", (int *)&cmd);
		fgets(str, 20, stdin);

		if (cmd == 255)
			break;

		switch (cmd) {
		case 0:
			ret = cif_set_dev_attr();
			break;
		case 1:
			ret = cif_set_output_clk_edge();
			fgets(str, 20, stdin);
			break;
		case 2:
			ret = cif_reset_mipi();
			break;
		case 3:
			ret = cif_crop_top();
			fgets(str, 20, stdin);
			break;
		case 4:
			ret = cif_set_manaul();
			break;
		case 5:
			ret = cif_set_lvds_fp_vs();
			fgets(str, 20, stdin);
			break;
		case 6:
			ret = cif_reset_snsr();
			break;
		case 7:
			ret = cif_unreset_snsr();
			break;
		case 8:
			ret = cif_enable_snsr_clk();
			break;
		case 9:
			ret = cif_disable_snsr_clk();
			break;
		default:
			ut_pr(UT_INFO, "Unsupport cmd\n");
			break;
		}
	}

	return ret;

}

/*************************************************************
 *	Public	functions
 *************************************************************/
enum cvi_errno cif_init(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	UT_DEV_INIT(dev_cif, ut_fd_cif);

	ut_pr(UT_INFO, "cif ver 1158\n");
	return ret;
}

enum cvi_errno cif_config(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno cif_start(void *priv)
{
	const struct inparam *param = (const struct inparam *)priv;
	enum cvi_errno ret = ERR_NONE;

	if (param->manual_mode)
		cif_manual(priv);

	return ret;
}

enum cvi_errno cif_stop(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno cif_msgrcv(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	struct msgpack *msg = (struct msgpack *)priv;
	struct cif_rx_ctx *msg_ctx = msg->msg_ctx;

	ut_pr(UT_INFO, "snsr rev frm src(%s) cmd(%d)\n",
			msg->src_name, msg_ctx->cmd);

	if (!ut_fd_cif)
		UT_DEV_INIT(dev_cif, ut_fd_cif);

	switch (msg_ctx->cmd) {
	case CIF_RESET_SENSOR:
		_cif_reset_snsr(*(unsigned int *)msg_ctx->indata);
		break;
	case CIF_RESET_MIPI:
		_cif_reset_mipi(*(unsigned int *)msg_ctx->indata);
		break;
	case CIF_SET_DEV_ATTR:
		_cif_set_dev_attr((struct combo_dev_attr_s *)msg_ctx->indata);
		break;
	case CIF_ENABLE_SNSR_CLK:
		_cif_enable_snsr_clk(*(unsigned int *)msg_ctx->indata);
		break;
	case CIF_UNRESET_SENSOR:
		_cif_unreset_snsr(*(unsigned int *)msg_ctx->indata);
		break;
	default:
		break;
	}
	return ret;
}

/*************************************************************
 *	Instance	Creation
 *************************************************************/
MODULE_DECL("cif", cif);

struct module_op *cif_get_instance(void)
{
	return &cif_op;
}


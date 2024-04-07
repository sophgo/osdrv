#include <ut_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_vip_snsr.h>
#include <linux/cvi_vip_cif.h>
#include <linux/v4l2-subdev.h>
#include "../common/vip_msg_com.h"
#include <unistd.h>
#include "wrap_i2c_ut.h"

//#define DUAL_SNSR

const char *dev_snsr = "/dev/sensor0";
int ut_fd_snsr;
static struct snsr_info_s snsr_info;
static struct snsr_rx_attr_s snsr_attr;
static struct wdr_size_s frm_info;
static struct snsr_cfg_node_s snsr_cfg;
static struct snsr_ae_def_s ae_info;
static struct snsr_awb_def_s awb_info;
static struct snsr_fps_s snsr_fps = {
	.fps_nume = 300000,
	.fps_denom = 10000,
};
static struct snsr_inttime_max_s snsr_intmax;
static struct snsr_inttime_s snsr_inttime;
static struct snsr_gain_update_s snsr_gain;
static enum cvi_errno snsr_auto(void *priv, unsigned int hdr_on);
static struct snr_csibdg_info isp_crop_info;
static struct cif_rx_ctx msg_ctx;
static struct vip_msg_ctx isp_msg_ctx;
static __u64 bayer_phy_addr[4];

#if defined(DUAL_SNSR)
const char *dev_snsr1 = "/dev/sensor1";
int ut_fd_snsr1;
static struct snsr_info_s snsr_info1;
static struct snsr_rx_attr_s snsr_attr1;
static struct wdr_size_s frm_info1;
static struct snsr_cfg_node_s snsr_cfg1;
static struct snsr_ae_def_s ae_info1;
static struct snsr_awb_def_s awb_info1;
static struct snsr_fps_s snsr_fps1 = {
	.fps_nume = 300000,
	.fps_denom = 10000,
};
static struct snsr_inttime_max_s snsr_intmax1;
static struct snsr_inttime_s snsr_inttime1;
static struct snsr_gain_update_s snsr_gain1;
static __u64 bayer_phy_addr1[2];
#endif
/*************************************************************
 *	Private	functions
 *************************************************************/
static enum cvi_errno snsr_snd_cif_msg(enum cif_rx_cmd cmd,
					void *indata, void *retdata)
{
	enum cvi_errno ret = ERR_NONE;

	msg_ctx.cmd = cmd;
	msg_ctx.indata = indata;
	msg_ctx.retdata = retdata;

	MSGSND("snsr", "cif", msg_ctx);

	return ret;
}

static enum cvi_errno snsr_snd_isp_msg(enum snr_isp_cmd cmd,
					void *indata, void *retdata)
{
	enum cvi_errno ret = ERR_NONE;

	isp_msg_ctx.cmd = cmd;
	isp_msg_ctx.indata = indata;
	isp_msg_ctx.retdata = retdata;

	MSGSND("snsr", "isp", isp_msg_ctx);

	return ret;
}

static enum cvi_errno _snsr_s_image_mode(int fd, struct snsr_mode_s *mode)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_S_IMAGE_MODE, (void *)mode) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_s_image_mode(void)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_mode_s *mode = &snsr_info.mode[0];
	int tmp;

	ut_pr(UT_INFO, "width:");
	scanf("%d", (int *)&tmp);
	mode->width = tmp;
	ut_pr(UT_INFO, "height:");
	scanf("%d", (int *)&tmp);
	mode->height = tmp;
	mode->fps_nume = 30*10000;
	mode->fps_denom = 10000;

	return _snsr_s_image_mode(ut_fd_snsr, mode);
}

static enum cvi_errno snsr_g_image_mode(int fd, struct snsr_info_s *info)
{
	enum cvi_errno ret = ERR_NONE;
	int i;

	if (ioctl(fd, CVI_SNSR_G_IMAGE_MODE, (void *)info) < 0)
		return ERR_IOCTL;

	ut_pr(UT_INFO, "clk2unrst_us %d\nunrst2init_us %d\n",
			info->clk2unrst_us,
			info->unrst2init_us);
	ut_pr(UT_INFO, "stable_ms %d\ncode %d\n",
			info->stable_ms,
			info->code);

	for (i = 0; i < info->mode_num; i++) {
		struct snsr_mode_s *mode = &info->mode[i];
		float fps = mode->fps_nume/mode->fps_denom;

		ut_pr(UT_INFO, "mode [%d] width %d height %d fps %f\n",
				i,
				mode->width,
				mode->height,
				fps);
	}

	return ret;

}

static enum cvi_errno _snsr_s_power(int fd, unsigned int on)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp_on = on;

	if (ioctl(fd, CVI_SNSR_S_POWER, (void *)&tmp_on) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_s_power(void)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int power;

	ut_pr(UT_INFO, "power:");
	scanf("%u", (unsigned int *)&power);

	return _snsr_s_power(ut_fd_snsr, power);
}

static enum cvi_errno _snsr_s_stream(int fd, unsigned int on)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp_on = on;

	if (ioctl(fd, CVI_SNSR_S_STREAM, (void *)&tmp_on) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_s_stream(void)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int stream;

	ut_pr(UT_INFO, "stream on:");
	scanf("%u", (unsigned int *)&stream);

	return _snsr_s_stream(ut_fd_snsr, stream);
}

static enum cvi_errno snsr_g_wdr_size(int fd, struct wdr_size_s *info)
{
	enum cvi_errno ret = ERR_NONE;
	int i;

	if (ioctl(fd, CVI_SNSR_G_WDR_SIZE, (void *)info) < 0)
		return ERR_IOCTL;

	for (i = 0; i < MAX_WDR_FRAME_NUM; i++) {
		struct active_size_s *size = &info->img_size[i];

		ut_pr(UT_INFO, "[%d] tw %d th %d, x %d, y %d, aw %d, ah %d\n",
			i,
			size->width,
			size->height,
			size->start_x,
			size->start_y,
			size->active_w,
			size->active_h);
	}

	return ret;
}

static enum cvi_errno _snsr_s_global_init(int fd)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp;

	if (ioctl(fd, CVI_SNSR_GLOBAL_INIT, (void *)&tmp) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno _snsr_g_rx_attr(int fd, struct snsr_rx_attr_s *attr)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_G_RX_ATTR, (void *)attr) < 0)
		ret = ERR_IOCTL;

	ut_pr(UT_INFO, "dev_no = %d\n", attr->dev_attr.devno);
	return ret;
}

static enum cvi_errno snsr_g_rx_attr(void)
{
	enum cvi_errno ret = ERR_NONE;

	ut_pr(UT_INFO, "is_hdr:");
	scanf("%u", (unsigned int *)&snsr_attr.is_hdr);

	return _snsr_g_rx_attr(ut_fd_snsr, &snsr_attr);
}

static enum cvi_errno _snsr_wdr(int fd, unsigned int wdr)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp_wdr = wdr;

	if (ioctl(fd, CVI_SNSR_S_WDR_MODE, (void *)&tmp_wdr) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_wdr(void)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp;

	ut_pr(UT_INFO, "wdr mode:");
	scanf("%u", (unsigned int *)&tmp);

	return _snsr_wdr(ut_fd_snsr, tmp);
}

static enum cvi_errno _snsr_again_calc(int fd, unsigned int *lin, unsigned int *db)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_gain_calc_s gain;

	gain.gain_lin = *lin;

	if (ioctl(fd, CVI_SNSR_AGAIN_CALC_TABLE, (void *)&gain) < 0)
		ret = ERR_IOCTL;

	*db = gain.gain_db;
	*lin = gain.gain_lin;

	return ret;
}

static enum cvi_errno snsr_again_calc(void)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int lin, db;

	ut_pr(UT_INFO, "again lin:");
	scanf("%u", (unsigned int *)&lin);

	ret = _snsr_again_calc(ut_fd_snsr, &lin, &db);

	ut_pr(UT_INFO, "lin = %u, db = %u\n", lin, db);

	return ret;
}

static enum cvi_errno _snsr_dgain_calc(int fd, unsigned int *lin, unsigned int *db)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_gain_calc_s gain;

	gain.gain_lin = *lin;

	if (ioctl(fd, CVI_SNSR_DGAIN_CALC_TABLE, (void *)&gain) < 0)
		ret = ERR_IOCTL;

	*db = gain.gain_db;
	*lin = gain.gain_lin;

	return ret;
}

static enum cvi_errno snsr_dgain_calc(void)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int lin, db;

	ut_pr(UT_INFO, "dgain lin:");
	scanf("%u", (unsigned int *)&lin);

	ret = _snsr_dgain_calc(ut_fd_snsr, &lin, &db);

	ut_pr(UT_INFO, "lin = %u, db = %u\n", lin, db);

	return ret;
}

static enum cvi_errno _snsr_gain_update(int fd, struct snsr_gain_update_s *gain)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_GAINS_UPDATE, (void *)gain) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_gain_update(void)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_gain_update_s gain;
	unsigned int tmp;

	ut_pr(UT_INFO, "again[0] db:");
	scanf("%u", (unsigned int *)&tmp);
	gain.again[0] = tmp;
	ut_pr(UT_INFO, "dgain[0] db:");
	scanf("%u", (unsigned int *)&tmp);
	gain.dgain[0] = tmp;

	ut_pr(UT_INFO, "again[1] db:");
	scanf("%u", (unsigned int *)&tmp);
	gain.again[1] = tmp;
	ut_pr(UT_INFO, "dgain[1] db:");
	scanf("%u", (unsigned int *)&tmp);
	gain.dgain[1] = tmp;

	ret = _snsr_gain_update(ut_fd_snsr, &gain);

	return ret;
}

static enum cvi_errno _snsr_g_cfg_node(int fd, struct snsr_cfg_node_s *cfg)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_G_CFG_NODE, (void *)cfg) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static struct wrap_i2c_ctx i2c_msg_ctx;

static enum cvi_errno snsr_snd_wrap_msg(enum wrap_i2c_cmd cmd,
					void *indata, void *retdata)
{
	enum cvi_errno ret = ERR_NONE;

	i2c_msg_ctx.cmd = cmd;
	i2c_msg_ctx.indata = indata;
	i2c_msg_ctx.retdata = retdata;

	MSGSND("snsr", "wrap_i2c", i2c_msg_ctx);

	return ret;
}

static enum cvi_errno snsr_g_cfg_node(void)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_regs_s *sns = &snsr_cfg.snsr;
	int i;

	/* get snsr cfg.  */
	ret = _snsr_g_cfg_node(ut_fd_snsr, &snsr_cfg);

	if (snsr_cfg.isp.need_update) {
		struct wdr_size_s *wdr = &snsr_cfg.isp.wdr;

		for (i = 0; i < wdr->frm_num; i++) {
			struct active_size_s *size = &wdr->img_size[i];

			ut_pr(UT_INFO, "[%d] tw %d th %d, x %d, y %d, aw %d, ah %d\n",
				i,
				size->width,
				size->height,
				size->start_x,
				size->start_y,
				size->active_w,
				size->active_h);
		}
		isp_crop_info.w = wdr->img_size[0].width;
		isp_crop_info.h = wdr->img_size[0].height;
		ret |= snsr_snd_isp_msg(CSIBDG_CTRL, &isp_crop_info,
				(void *)bayer_phy_addr);
		ut_pr(UT_INFO, "[%s][%d] phy_addr_1=0x%llx\n",
				__func__, __LINE__, bayer_phy_addr[0]);
		ut_pr(UT_INFO, "[%s][%d] phy_addr_2=0x%llx\n",
				__func__, __LINE__, bayer_phy_addr[1]);

	}

	/* configured the snsr. */
	for (i = 0; i < sns->regs_num; i++) {
		ut_pr(UT_INFO, "update %d, i2c_dev %d, dev_addr %d, reg_addr 0x%x, data 0x%x\n,",
				sns->i2c_data[i].update,
				sns->i2c_data[i].i2c_dev,
				sns->i2c_data[i].dev_addr,
				sns->i2c_data[i].reg_addr,
				sns->i2c_data[i].data);
		if (sns->i2c_data[i].update)
			snsr_snd_wrap_msg(I2C_WRITE, &sns->i2c_data[i], NULL);
	}
	snsr_cfg.configed = 1;

	return ret;
}

static enum cvi_errno _snsr_g_inttime_max(int fd, struct snsr_inttime_max_s *inttime)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_G_INTTIME_MAX, (void *)inttime) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_g_inttime_max(void)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_inttime_max_s inttime;
	unsigned int tmp;

	ut_pr(UT_INFO, "man_ratio_en:");
	scanf("%u", (unsigned int *)&tmp);
	inttime.man_ratio_en = tmp;

	ut_pr(UT_INFO, "ratio(x64):");
	scanf("%u", (unsigned int *)&tmp);
	inttime.ratio[0] = tmp;

	ret = _snsr_g_inttime_max(ut_fd_snsr, &inttime);

	ut_pr(UT_INFO, "lef min = %u, max = %u\n", inttime.inttime_min[1], inttime.inttime_max[1]);
	ut_pr(UT_INFO, "sef min = %u, max = %u\n", inttime.inttime_min[0], inttime.inttime_max[0]);
	return ret;
}

static enum cvi_errno _snsr_inttime_update(int fd, struct snsr_inttime_s *inttime)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_INTTIME_UPDATE, (void *)inttime) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_inttime_update(void)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_inttime_s inttime;
	unsigned int tmp;

	ut_pr(UT_INFO, "inttime[0]:");
	scanf("%u", (unsigned int *)&tmp);
	inttime.inttime[0] = tmp;

	ut_pr(UT_INFO, "inttime[1]:");
	scanf("%u", (unsigned int *)&tmp);
	inttime.inttime[1] = tmp;

	ret = _snsr_inttime_update(ut_fd_snsr, &inttime);

	return ret;
}

static enum cvi_errno _snsr_g_ae_def(int fd, struct snsr_ae_def_s *info)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_G_AE_DEF, (void *)info) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_g_ae_def(int fd, struct snsr_ae_def_s *info)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp;

	memset(info, 0, sizeof(*info));

	ret = _snsr_g_ae_def(fd, info);

	ut_pr(UT_INFO, "FL_std = %u, FL_max = %u\n",
			info->FL_std,
			info->FL_max);
	ut_pr(UT_INFO, "gain_mode = %u\n", info->gain_mode);
	ut_pr(UT_INFO, "max again = %u, min_again = %u\n",
			info->max_again,
			info->min_again);
	ut_pr(UT_INFO, "max dgain = %u, min dgain = %u\n",
			info->max_dgain, info->min_dgain);
	ut_pr(UT_INFO, "max inttime = %u, min inttime  = %u\n",
			info->max_inttime, info->min_inttime);
	ut_pr(UT_INFO, "man_ratio_en = %u, ratio = %u, %u, %u\n",
			info->man_ratio_en, info->ratio[0],
			info->ratio[1], info->ratio[2]);
	ut_pr(UT_INFO, "init_AESpeed = %u, init_AETolerace = %u, ae_compensation = %u\n",
			info->init_AESpeed,
			info->init_AETolerance,
			info->ae_compensation);

	return ret;
}

static enum cvi_errno _snsr_s_fps(int fd, struct snsr_fps_s *fps)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_S_FPS, (void *)fps) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_s_fps(void)
{
	enum cvi_errno ret = ERR_NONE;
	struct snsr_fps_s fps;
	unsigned int tmp;

	memset(&fps, 0, sizeof(fps));
	ut_pr(UT_INFO, "fps_nume:");
	scanf("%u", (unsigned int *)&tmp);
	fps.fps_nume = tmp;
	ut_pr(UT_INFO, "fps_denom:");
	scanf("%u", (unsigned int *)&tmp);
	fps.fps_denom = tmp;

	ret = _snsr_s_fps(ut_fd_snsr, &fps);

	ut_pr(UT_INFO, "max_inttime = %u, FL_std = %u\n",
			fps.max_inttime,
			fps.FL_std);

	return ret;
}

static enum cvi_errno _snsr_g_awb_def(int fd, struct snsr_awb_def_s *info)
{
	enum cvi_errno ret = ERR_NONE;

	if (ioctl(fd, CVI_SNSR_G_AWB_DEF, (void *)info) < 0)
		ret = ERR_IOCTL;

	return ret;
}

static enum cvi_errno snsr_g_awb_def(int fd, struct snsr_awb_def_s *info)
{
	enum cvi_errno ret = ERR_NONE;
	unsigned int tmp;

	memset(info, 0, sizeof(*info));

	ret = _snsr_g_awb_def(fd, info);

	ut_pr(UT_INFO, "init_Ggain = %u, awb_run_interval = %u\n",
			info->init_ggain,
			info->awb_run_interval);

	return ret;
}

static enum cvi_errno snsr_manual(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	int cmd, run = 1;
	char str[20];

	while (run) {
		ut_pr(UT_INFO, "0-S_IMAGE_MODE\n");
		ut_pr(UT_INFO, "1-G_IMAGE_MODE\n");
		ut_pr(UT_INFO, "2-S_POWER\n");
		ut_pr(UT_INFO, "3-S_STREAM\n");
		ut_pr(UT_INFO, "4-G_WDR_SIZE\n");
		ut_pr(UT_INFO, "5-G_CFG_NODE\n");
		ut_pr(UT_INFO, "6-G_RX_ATTR\n");
		ut_pr(UT_INFO, "7-WDR\n");
		ut_pr(UT_INFO, "8-GLOBAL_INIT\n");
		ut_pr(UT_INFO, "9-AGAIN_CALC\n");
		ut_pr(UT_INFO, "10-DGAIN_CALC\n");
		ut_pr(UT_INFO, "11-GAIN_UP\n");
		ut_pr(UT_INFO, "12-G_CFG_NODE\n");
		ut_pr(UT_INFO, "13-G_INTTIME_MAX\n");
		ut_pr(UT_INFO, "14-INTTIME_UP\n");
		ut_pr(UT_INFO, "15-G_AE_DEF\n");
		ut_pr(UT_INFO, "16-S_FPS\n");
		ut_pr(UT_INFO, "17-G_AWB_DEF\n");
		ut_pr(UT_INFO, "18-snsr_auto linear\n");
		ut_pr(UT_INFO, "19-snsr_auto wdr\n");
		ut_pr(UT_INFO, "255-Exit\n");
		scanf("%d", (int *)&cmd);
		fgets(str, 20, stdin);

		if (cmd == 255)
			break;

		switch (cmd) {
		case 0:
			ret = snsr_s_image_mode();
			break;
		case 1:
			ret = snsr_g_image_mode(ut_fd_snsr, &snsr_info);
			break;
		case 2:
			ret = snsr_s_power();
			break;
		case 3:
			ret = snsr_s_stream();
			break;
		case 4:
			ret = snsr_g_wdr_size(ut_fd_snsr, &frm_info);
			break;
		case 5:
			ret = snsr_g_cfg_node();
			break;
		case 6:
			ret = snsr_g_rx_attr();
			break;
		case 7:
			ret = snsr_wdr();
			break;
		case 8:
			ret = _snsr_s_global_init(ut_fd_snsr);
			break;
		case 9:
			ret = snsr_again_calc();
			break;
		case 10:
			ret = snsr_dgain_calc();
			break;
		case 11:
			ret = snsr_gain_update();
			break;
		case 12:
			ret = snsr_g_cfg_node();
			break;
		case 13:
			ret = snsr_g_inttime_max();
			break;
		case 14:
			ret = snsr_inttime_update();
			break;
		case 15:
			ret = snsr_g_ae_def(ut_fd_snsr, &ae_info);
			break;
		case 16:
			ret = snsr_s_fps();
			break;
		case 17:
			ret = snsr_g_awb_def(ut_fd_snsr, &awb_info);
			break;
		case 18:
			ret = snsr_auto(priv, 0);
			break;
		case 19:
			ret = snsr_auto(priv, 1);
			break;
		default:
			ut_pr(UT_INFO, "Unsupport cmd\n");
			break;
		}
		fgets(str, 20, stdin);
	}

	return ret;

}

static enum cvi_errno snsr_auto(void *priv, unsigned int hdr_on)
{
	enum cvi_errno ret = ERR_NONE;
	struct combo_dev_attr_s *combo = &snsr_attr.dev_attr;
	unsigned int cnt, devno;
	unsigned int tmp, tmp1;
#if defined(DUAL_SNSR)
	struct combo_dev_attr_s *combo1 = &snsr_attr1.dev_attr;
	unsigned int devno1;
#endif

	/* init-1. sensor global init. */
	ret = _snsr_s_global_init(ut_fd_snsr);
	/* init-1. get sensor image mode (g_isp_def). */
	ret |= snsr_g_image_mode(ut_fd_snsr, &snsr_info);
	/* init-2. set sensor image mode. */
	ret |= _snsr_s_image_mode(ut_fd_snsr, &snsr_info.mode[0]);
	/* init-2. set wdr mode. */
	ret |= _snsr_wdr(ut_fd_snsr, !!hdr_on ? SNS_WDR_MODE_2TO1_LINE : SNS_WDR_MODE_NONE);
	/* init-3. get mipi-rx attr. */
	snsr_attr.is_hdr = !!hdr_on;
	ret |= _snsr_g_rx_attr(ut_fd_snsr, &snsr_attr);
	devno = combo->devno;
	/* init-3. get ae def. */
	ret |= snsr_g_ae_def(ut_fd_snsr, &ae_info);
	/* init-3. get awb def. */
	ret |= snsr_g_awb_def(ut_fd_snsr, &awb_info);

	/* init-3. set user default (fps, inttime, gain). */
	if (hdr_on) {
		snsr_fps.fps_nume = ae_info.fps_nume;
		snsr_fps.fps_denom = ae_info.fps_denom;
		ret |= _snsr_s_fps(ut_fd_snsr, &snsr_fps);
		snsr_intmax.man_ratio_en = ae_info.man_ratio_en;
		snsr_intmax.ratio[0] = ae_info.ratio[0];
		snsr_intmax.ratio[1] = ae_info.ratio[1];
		snsr_intmax.ratio[2] = ae_info.ratio[2];
		_snsr_g_inttime_max(ut_fd_snsr, &snsr_intmax);
		ut_pr(UT_INFO, "sef min = %u, max = %u\n", snsr_intmax.inttime_min[0], snsr_intmax.inttime_max[0]);
		ut_pr(UT_INFO, "lef min = %u, max = %u\n", snsr_intmax.inttime_min[1], snsr_intmax.inttime_max[1]);
		snsr_inttime.inttime[0] = snsr_intmax.inttime_max[0];
		snsr_inttime.inttime[1] = snsr_intmax.inttime_max[1];
		ret |= _snsr_inttime_update(ut_fd_snsr, &snsr_inttime);
		tmp = ae_info.min_again;
		ret |= _snsr_again_calc(ut_fd_snsr, &tmp, &tmp1);
		snsr_gain.again[0] = tmp1;
		snsr_gain.again[1] = tmp1;
		tmp = ae_info.min_dgain;
		ret |= _snsr_dgain_calc(ut_fd_snsr, &tmp, &tmp1);
		snsr_gain.dgain[0] = tmp1;
		snsr_gain.dgain[1] = tmp1;
	} else {
		snsr_fps.fps_nume = ae_info.fps_nume;
		snsr_fps.fps_denom = ae_info.fps_denom;
		ret |= _snsr_s_fps(ut_fd_snsr, &snsr_fps);
		snsr_inttime.inttime[0] = snsr_fps.max_inttime/2;
		ret |= _snsr_inttime_update(ut_fd_snsr, &snsr_inttime);
		tmp = ae_info.min_again;
		ret |= _snsr_again_calc(ut_fd_snsr, &tmp, &tmp1);
		snsr_gain.again[0] = tmp1;
		tmp = ae_info.min_dgain;
		ret |= _snsr_dgain_calc(ut_fd_snsr, &tmp, &tmp1);
		snsr_gain.dgain[0] = tmp1;
	}
	ret |= _snsr_gain_update(ut_fd_snsr, &snsr_gain);
	/* init-3. update snsr config node. */
	ret |= _snsr_g_cfg_node(ut_fd_snsr, &snsr_cfg);
	/* init-3. get frame position. */
	ret |= snsr_g_wdr_size(ut_fd_snsr, &frm_info);

#if defined(DUAL_SNSR)
	/* init-1. sensor global init. */
	ret |= _snsr_s_global_init(ut_fd_snsr1);
	/* init-1. get sensor image mode (g_isp_def). */
	ret |= snsr_g_image_mode(ut_fd_snsr1, &snsr_info1);
	/* init-2. set sensor image mode. */
	ret |= _snsr_s_image_mode(ut_fd_snsr1, &snsr_info1.mode[0]);
	/* init-2. set wdr mode. */
	ret |= _snsr_wdr(ut_fd_snsr1, !!hdr_on ? SNS_WDR_MODE_2TO1_LINE : SNS_WDR_MODE_NONE);
	/* init-3. get mipi-rx attr. */
	snsr_attr1.is_hdr = !!hdr_on;
	ret |= _snsr_g_rx_attr(ut_fd_snsr1, &snsr_attr1);
	devno1 = combo1->devno;
	/* init-3. get ae def. */
	ret |= snsr_g_ae_def(ut_fd_snsr1, &ae_info1);
	/* init-3. get awb def. */
	ret |= snsr_g_awb_def(ut_fd_snsr1, &awb_info1);

	/* init-3. set user default (fps, inttime, gain). */
	if (hdr_on) {
		snsr_fps1.fps_nume = ae_info1.fps_nume;
		snsr_fps1.fps_denom = ae_info1.fps_denom;
		ret |= _snsr_s_fps(ut_fd_snsr1, &snsr_fps1);
		snsr_intmax1.man_ratio_en = ae_info1.man_ratio_en;
		snsr_intmax1.ratio[0] = ae_info1.ratio[0];
		snsr_intmax1.ratio[1] = ae_info1.ratio[1];
		snsr_intmax1.ratio[2] = ae_info1.ratio[2];
		_snsr_g_inttime_max(ut_fd_snsr1, &snsr_intmax1);
		ut_pr(UT_INFO, "sef min = %u, max = %u\n", snsr_intmax1.inttime_min[0], snsr_intmax1.inttime_max[0]);
		ut_pr(UT_INFO, "lef min = %u, max = %u\n", snsr_intmax1.inttime_min[1], snsr_intmax1.inttime_max[1]);
		snsr_inttime1.inttime[0] = snsr_intmax1.inttime_max[0];
		snsr_inttime1.inttime[1] = snsr_intmax1.inttime_max[1];
		ret |= _snsr_inttime_update(ut_fd_snsr1, &snsr_inttime1);
		tmp = ae_info1.min_again;
		ret |= _snsr_again_calc(ut_fd_snsr1, &tmp, &tmp1);
		snsr_gain1.again[0] = tmp1;
		snsr_gain1.again[1] = tmp1;
		tmp = ae_info1.min_dgain;
		ret |= _snsr_dgain_calc(ut_fd_snsr1, &tmp, &tmp1);
		snsr_gain1.dgain[0] = tmp1;
		snsr_gain1.dgain[1] = tmp1;
	} else {
		snsr_fps1.fps_nume = ae_info1.fps_nume;
		snsr_fps1.fps_denom = ae_info1.fps_denom;
		ret |= _snsr_s_fps(ut_fd_snsr1, &snsr_fps1);
		snsr_inttime1.inttime[0] = snsr_fps1.max_inttime/2;
		ret |= _snsr_inttime_update(ut_fd_snsr1, &snsr_inttime1);
		tmp = ae_info1.min_again;
		ret |= _snsr_again_calc(ut_fd_snsr1, &tmp, &tmp1);
		snsr_gain1.again[0] = tmp1;
		tmp = ae_info1.min_dgain;
		ret |= _snsr_dgain_calc(ut_fd_snsr1, &tmp, &tmp1);
		snsr_gain1.dgain[0] = tmp1;
	}
	ret |= _snsr_gain_update(ut_fd_snsr1, &snsr_gain1);
	/* init-3. update snsr config node. */
	ret |= _snsr_g_cfg_node(ut_fd_snsr1, &snsr_cfg1);
	/* init-3. get frame position. */
	ret |= snsr_g_wdr_size(ut_fd_snsr1, &frm_info1);

#endif

	/* init-4 isp clk ----> */
	isp_crop_info.w = frm_info.img_size[0].width;
	isp_crop_info.h = frm_info.img_size[0].height;
	isp_crop_info.hdr_on = !!hdr_on;

	ret |= snsr_snd_isp_msg(CSIBDG_CTRL, &isp_crop_info,
			(void *)bayer_phy_addr);

	ut_pr(UT_INFO, "[%s][%d] phy_addr_1=0x%llx\n",
			__func__, __LINE__, bayer_phy_addr[0]);
	ut_pr(UT_INFO, "[%s][%d] phy_addr_2=0x%llx\n",
			__func__, __LINE__, bayer_phy_addr[1]);
#if defined(DUAL_SNSR)
	ut_pr(UT_INFO, "[%s][%d] phy_addr_2=0x%llx\n",
			__func__, __LINE__, bayer_phy_addr[2]);
	ut_pr(UT_INFO, "[%s][%d] phy_addr_3=0x%llx\n",
			__func__, __LINE__, bayer_phy_addr[3]);
#endif
	/* init-5. enable sensor power. */
	ret |= _snsr_s_power(ut_fd_snsr, 1);
#if defined(DUAL_SNSR)
	ret |= _snsr_s_power(ut_fd_snsr1, 1);
#endif
	/* init-5. activate sensor reset. */
	ret |= snsr_snd_cif_msg(CIF_RESET_SENSOR,
			(void *)&devno, NULL);
#if defined(DUAL_SNSR)
	ret |= snsr_snd_cif_msg(CIF_RESET_SENSOR,
			(void *)&devno1, NULL);
#endif
	/* init-5. reset mipi-rx. */
	ret |= snsr_snd_cif_msg(CIF_RESET_MIPI,
			(void *)&devno, NULL);
#if defined(DUAL_SNSR)
	ret |= snsr_snd_cif_msg(CIF_RESET_MIPI,
			(void *)&devno1, NULL);
#endif
	/* init-5. set mipi attribute */
	ret |= snsr_snd_cif_msg(CIF_SET_DEV_ATTR,
			(void *)combo, NULL);
#if defined(DUAL_SNSR)
	ret |= snsr_snd_cif_msg(CIF_SET_DEV_ATTR,
			(void *)combo1, NULL);
#endif
	/* init-5. enable sensor clk */
	ret |= snsr_snd_cif_msg(CIF_ENABLE_SNSR_CLK,
			(void *)&devno, NULL);
#if defined(DUAL_SNSR)
	ret |= snsr_snd_cif_msg(CIF_ENABLE_SNSR_CLK,
			(void *)&devno1, NULL);
#endif
	/* init-5. clkout to unreset delay in snsr_info_s */
	usleep(snsr_info.clk2unrst_us);
	/* init-5. clear sensor reset */
	ret |= snsr_snd_cif_msg(CIF_UNRESET_SENSOR,
			(void *)&devno, NULL);
#if defined(DUAL_SNSR)
	ret |= snsr_snd_cif_msg(CIF_UNRESET_SENSOR,
			(void *)&devno1, NULL);
#endif
	/* init-5. unreset to init sequence delay in snsr_info_s */
	usleep(snsr_info.unrst2init_us);
	/* init-5. init sensor mode. */
	ret |= _snsr_s_stream(ut_fd_snsr, 1);
#if defined(DUAL_SNSR)
	ret |= _snsr_s_stream(ut_fd_snsr1, 1);
#endif
	/* init-5. wait sensor stable delay */
	cnt = snsr_info.stable_ms;
	while (cnt--)
		usleep(1000);
	return ret;
}

/*************************************************************
 *	Public	functions
 *************************************************************/
enum cvi_errno snsr_init(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	UT_DEV_INIT(dev_snsr, ut_fd_snsr);
#if defined(DUAL_SNSR)
	UT_DEV_INIT(dev_snsr1, ut_fd_snsr1);
#endif
	return ret;
}

enum cvi_errno snsr_config(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno snsr_start(void *priv)
{
	const struct inparam *param = (const struct inparam *)priv;
	enum cvi_errno ret = ERR_NONE;

	if (param->manual_mode)
		snsr_manual(priv);

	return ret;
}

enum cvi_errno snsr_stop(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno snsr_msgrcv(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	struct msgpack *msg = (struct msgpack *)priv;

	ut_pr(UT_INFO, "SNSR rev frm src(%s) val(%d)\n",
			msg->src_name, *(uint8_t *)msg->msg_ctx);

	return ret;
}

/*************************************************************
 *	Instance	Creation
 *************************************************************/
MODULE_DECL("snsr", snsr);

struct module_op *snsr_get_instance(void)
{
	return &snsr_op;
}


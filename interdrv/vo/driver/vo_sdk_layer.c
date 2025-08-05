#include "osal.h"
#include "vo_debug.h"
#include "vi_sys.h"
#include "dsi_mac.h"
#include "dsi_phy.h"
#include "vo_mac.h"
#include "vo_process.h"
#include "vo_sdk_layer.h"
#include "comm_buffer.h"

static inline int check_struct_size(unsigned int size, unsigned int type_size)
{
	if (size != type_size) {
		TRACE_VO(DBG_ERR, "data size error! size=%u, type_size=%u\n", size, type_size);
		return -1;
	}

	return 0;
}

/****************************************************************************
 * Global parameters
 ****************************************************************************/
struct vo_fmt vo_sdk_formats[] = {
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_420,
	.fmt		 = DISP_FMT_YUV420,
	.bit_depth	 = { 8, 4, 4 },
	.buffers	 = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_422,
	.fmt		 = DISP_FMT_YUV422,
	.bit_depth	 = { 8, 4, 4 },
	.buffers	 = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_444,
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV12,
	.fmt		 = DISP_FMT_NV12,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV21,
	.fmt		 = DISP_FMT_NV21,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV16,
	.fmt		 = DISP_FMT_YUV422SP1,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV61,
	.fmt		 = DISP_FMT_YUV422SP2,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUYV,
	.fmt		 = DISP_FMT_YUYV,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc		 = PIXEL_FORMAT_YVYU,
	.fmt		 = DISP_FMT_YVYU,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_UYVY,
	.fmt		 = DISP_FMT_UYVY,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_VYUY,
	.fmt		 = DISP_FMT_VYUY,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_BGR_888_PLANAR, /* rgb */
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_RGB_888_PLANAR, /* rgb */
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},

	{
	.fourcc	 = PIXEL_FORMAT_RGB_888, /* rgb */
	.fmt		 = DISP_FMT_RGB_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_BGR_888, /* bgr */
	.fmt		 = DISP_FMT_BGR_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_400, /* Y-Only */
	.fmt		 = DISP_FMT_Y_ONLY,
	.bit_depth	 = { 8 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_HSV_888, /* hsv */
	.fmt		 = DISP_FMT_RGB_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_HSV_888_PLANAR, /* hsv */
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
};

vo_sync_info_s sync_info[VO_OUTPUT_BUTT] = {
	[VO_OUTPUT_800x600_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 600, .vbb = 24, .vfb = 1
		, .hact = 800, .hbb = 88, .hfb = 40
		, .vpw = 4, .hpw = 128, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P24] = {.synm = 1, .iop = 1, .frame_rate = 24
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 638
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P25] = {.synm = 1, .iop = 1, .frame_rate = 25
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 528
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P30] = {.synm = 1, .iop = 1, .frame_rate = 30
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 88
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_720P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 720, .vbb = 20, .vfb = 5
		, .hact = 1280, .hbb = 220, .hfb = 440
		, .vpw = 5, .hpw = 40, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_720P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 720, .vbb = 20, .vfb = 5
		, .hact = 1280, .hbb = 220, .hfb = 110
		, .vpw = 5, .hpw = 40, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 528
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 88
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_576P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 576, .vbb = 39, .vfb = 5
		, .hact = 720, .hbb = 68, .hfb = 12
		, .vpw = 5, .hpw = 64, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_480P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 480, .vbb = 30, .vfb = 9
		, .hact = 720, .hbb = 60, .hfb = 16
		, .vpw = 6, .hpw = 62, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_720x1280_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1280, .vbb = 4, .vfb = 6
		, .hact = 720, .hbb = 36, .hfb = 128
		, .vpw = 16, .hpw = 64, .idv = 0, .ihs = 0, .ivs = 1},
	[VO_OUTPUT_1080x1920_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1920, .vbb = 36, .vfb = 6
		, .hact = 1080, .hbb = 148, .hfb = 88
		, .vpw = 16, .hpw = 64, .idv = 0, .ihs = 0, .ivs = 1},
	[VO_OUTPUT_480x800_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 800, .vbb = 20, .vfb = 20
		, .hact = 480, .hbb = 50, .hfb = 50
		, .vpw = 10, .hpw = 10, .idv = 0, .ihs = 0, .ivs = 1},
	[VO_OUTPUT_1440P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1440, .vbb = 33, .vfb = 3
		, .hact = 2560, .hbb = 80, .hfb = 32
		, .vpw = 5, .hpw = 48, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P24] = {.synm = 1, .iop = 1, .frame_rate = 24
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 1276
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P25] = {.synm = 1, .iop = 1, .frame_rate = 25
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 1056
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P30] = {.synm = 1, .iop = 1, .frame_rate = 30
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 176
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 1056
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 176
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_4096x2160P24] = {.synm = 1, .iop = 1, .frame_rate = 24
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 296, .hfb = 1020
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P25] = {.synm = 1, .iop = 1, .frame_rate = 25
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 968
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P30] = {.synm = 1, .iop = 1, .frame_rate = 30
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 88
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 968
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 88
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_640x480_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 480, .vbb = 12, .vfb = 20
		, .hact = 640, .hbb = 120, .hfb = 150
		, .vpw = 6, .hpw = 62, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_440x1920_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1920, .vbb = 30, .vfb = 150
		, .hact = 440, .hbb = 50, .hfb = 150
		, .vpw = 20, .hpw = 30, .idv = 0, .ihs = 1, .ivs = 0},
};

const struct disp_pattern patterns[VO_PAT_MAX] = {
	{.type = PAT_TYPE_OFF,	.color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_SNOW, .color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_AUTO, .color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_RED},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_GREEN},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_BLUE},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_BAR},
	{.type = PAT_TYPE_H_GRAD, .color = PAT_COLOR_WHITE},
	{.type = PAT_TYPE_V_GRAD, .color = PAT_COLOR_WHITE},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_USR,
	.rgb = {0, 0, 0} },
};

/****************************************************************************
 * SDK layer Defines
 ****************************************************************************/
#define DEFAULT_MESH_PADDR	0x80000000

#define VO_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))

#define GDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||           \
	 (fmt == PIXEL_FORMAT_YUV_400))


/****************************************************************************
 *internal APIs
 ****************************************************************************/
static int vo_get_panelstatus(vo_dev dev, unsigned int *is_init)
{
	if (vo_mac_mux_get(dev) == VO_MAC_SEL_I80)
		*is_init = vo_mac_check_i80_enable(dev);
	else
		*is_init = disp_check_tgen_enable(dev);

	return 0;
}

struct vo_fmt *vo_sdk_get_format(unsigned int pixelformat)
{
	struct vo_fmt *fmt;
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(vo_sdk_formats); k++) {
		fmt = &vo_sdk_formats[k];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}

	return NULL;
}

static void vo_sdk_fill_disp_cfg(struct disp_cfg *cfg, struct vo_video_format *video_format)
{
	struct vo_fmt *vo_sdk_fmt;
	struct vo_plane_info *plane_info = video_format->plane_info;

	vo_sdk_fmt = vo_sdk_get_format(video_format->pixelformat);

	cfg->fmt = vo_sdk_fmt->fmt;

	if (video_format->colorspace == VO_COLORSPACE_SRGB)
		cfg->in_csc = DISP_CSC_NONE;
	else if (video_format->colorspace == VO_COLORSPACE_SMPTE170M)
		cfg->in_csc = DISP_CSC_601_FULL_YUV2RGB;
	else
		cfg->in_csc = DISP_CSC_709_FULL_YUV2RGB;

	TRACE_VO(DBG_DEBUG, "bytesperline 0(%d))\n", plane_info[0].bytesperline);
	TRACE_VO(DBG_DEBUG, "bytesperline 1(%d))\n", plane_info[1].bytesperline);
	cfg->mem.pitch_y = plane_info[0].bytesperline;
	cfg->mem.pitch_c = plane_info[1].bytesperline;

	TRACE_VO(DBG_DEBUG, " width(%d), heigh(%d)\n", video_format->width, video_format->height);
	cfg->mem.width = video_format->width;
	cfg->mem.height = video_format->height;
	cfg->mem.start_x = 0;
	cfg->mem.start_y = 0;
}

static int vo_sdk_setfmt(int width, int height, unsigned int pxlfmt, vo_dev dev)
{
	int p = 0;
	struct vo_video_format video_fmt;
	const struct vo_fmt *vo_sdk_fmt;
	struct disp_cfg *cfg;
	unsigned int bytesperline;

	osal_memset(&video_fmt, 0, sizeof(struct vo_video_format));

	video_fmt.width = width;
	video_fmt.height = height;
	video_fmt.pixelformat = pxlfmt;
	video_fmt.field = 0;

	switch (pxlfmt) {
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:
		video_fmt.colorspace = VO_COLORSPACE_SMPTE170M;
		break;
	default:
		break;
	}
	switch (pxlfmt) {
	default:
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
		video_fmt.num_planes = 3;
		break;
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		video_fmt.num_planes = 2;
		break;
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:
		video_fmt.num_planes = 1;
		break;
	}

	vo_sdk_fmt = vo_sdk_get_format(pxlfmt);

	for (p = 0; p < video_fmt.num_planes; p++) {
		unsigned char plane_sub_v = (p == 0) ? 1 : vo_sdk_fmt->plane_sub_v;
		/* Calculate the minimum supported bytesperline value */
		bytesperline = ALIGN((video_fmt.width * vo_sdk_fmt->bit_depth[p]) >> 3, DISP_ALIGNMENT);

		if (video_fmt.plane_info[p].bytesperline < bytesperline)
			video_fmt.plane_info[p].bytesperline = bytesperline;

		video_fmt.plane_info[p].sizeimage = video_fmt.plane_info[p].bytesperline
		* video_fmt.height / plane_sub_v;

		TRACE_VO(DBG_DEBUG, "plane-%d: bytesperline(%d) sizeimage(%x)\n", p,
			 video_fmt.plane_info[p].bytesperline, video_fmt.plane_info[p].sizeimage);
	}

	cfg = disp_get_cfg(dev);
	vo_sdk_fill_disp_cfg(cfg, &video_fmt);
	disp_set_bw_cfg(dev, cfg->fmt);
	disp_set_cfg(dev, cfg);

	return 0;
}

static void release_buffer(struct vo_layer_ctx *layer_ctx, struct osal_list_head *head)
{
	unsigned long flags;
	struct disp_buffer *b = NULL;

	while (!osal_list_empty(head)) {
		osal_spin_lock_irqsave(&layer_ctx->list_lock, &flags);
		b = osal_list_first_entry(head,
			struct disp_buffer, list);
		osal_list_del_init(&b->list);
		osal_spin_unlock_irqrestore(&layer_ctx->list_lock, &flags);

		if (b == NULL)
			return;

		osal_vfree(b);
		b = NULL;
	}
}

int vo_set_interface(vo_dev dev, struct vo_disp_intf_cfg *cfg)
{
	int rc = 0;
	struct vo_dev_ctx *dev_ctx;
	union vi_sys_clk_ctrl2 vi_sys_clk_ctrl2;

	rc = check_vo_dev_valid(dev);
	if (rc != 0)
		return rc;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	// if (smooth[dev]) {
	//	TRACE_VO(DBG_DEBUG, "set_interface won't apply if smooth.\n");
	//	disp_reg_force_up(dev);
	//	vdev->vo_core[dev].disp_interface = cfg->intf_type;
	//	return 0;
	// }

	if (osal_atomic_read(&dev_ctx->disp_streamon) == 1) {
		TRACE_VO(DBG_INFO, "set_interface can't be control if streaming.\n");
		return 0;
	}

	if (cfg->intf_type == VO_DISP_INTF_DSI) {
		TRACE_VO(DBG_INFO, "MIPI use mipi_tx to control.\n");
		return 0;
	} else if (cfg->intf_type == VO_DISP_INTF_LVDS) {
		int i = 0;
		union dsi_lvdstx lvds_cfg;
		bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

		for (i = 0; i < LANE_MAX_NUM; i++) {
			if ((cfg->lvds_cfg.lane_id[i] < 0) ||
				(cfg->lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
				dphy_dsi_set_lane(dev, i, MIPI_TX_LANE_MAX, false, false);
				continue;
			}
			dphy_dsi_set_lane(dev, i, cfg->lvds_cfg.lane_id[i],
					  cfg->lvds_cfg.lane_pn_swap[i], false);
			if (cfg->lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
				data_en[cfg->lvds_cfg.lane_id[i] - 1] = true;
			}
		}
		dphy_dsi_lane_en(dev, true, data_en, false);

		disp_set_intf(dev, VO_DISP_INTF_LVDS);

		if (cfg->lvds_cfg.pixelclock == 0) {
			TRACE_VO(DBG_ERR, "lvds pixelclock 0 invalid\n");
			return -1;
		}
		lvds_cfg.b.out_bit = cfg->lvds_cfg.out_bits;
		lvds_cfg.b.vesa_mode = cfg->lvds_cfg.mode;
		if (cfg->lvds_cfg.chn_num == 1)
			lvds_cfg.b.dual_ch = 0;
		else if (cfg->lvds_cfg.chn_num == 2)
			lvds_cfg.b.dual_ch = 1;
		else {
			lvds_cfg.b.dual_ch = 0;
			TRACE_VO(DBG_ERR, "invalid lvds chn_num(%d). Use 1 instead.",
				 cfg->lvds_cfg.chn_num);
		}
		lvds_cfg.b.vs_out_en = cfg->lvds_cfg.vs_out_en;
		lvds_cfg.b.hs_out_en = cfg->lvds_cfg.hs_out_en;
		lvds_cfg.b.hs_blk_en = cfg->lvds_cfg.hs_blk_en;
		lvds_cfg.b.ml_swap = cfg->lvds_cfg.msb_lsb_data_swap;
		lvds_cfg.b.ctrl_rev = cfg->lvds_cfg.serial_msb_first;
		lvds_cfg.b.oe_swap = cfg->lvds_cfg.even_odd_link_swap;
		lvds_cfg.b.en = cfg->lvds_cfg.enable;
		dphy_lvds_analog_setting(dev, true);
		dphy_lvds_set_pll(dev, cfg->lvds_cfg.pixelclock, cfg->lvds_cfg.chn_num);
		dsi_lvdstx_set(dev, lvds_cfg);
	} else if (cfg->intf_type == VO_DISP_INTF_BT656 || cfg->intf_type == VO_DISP_INTF_BT1120) {
		char fmt_sel = 0;
		char bt_mode;
		union bt_enc enc;
		union bt_sync_code sync;

		if (cfg->bt_cfg.mode == BT_MODE_1120) {
			disp_set_intf(dev, VO_DISP_INTF_BT1120);
			bt_mode = VO_MAC_SEL_BT1120;
		} else if (cfg->bt_cfg.mode == BT_MODE_656) {
			disp_set_intf(dev, VO_DISP_INTF_BT656);
			bt_mode = VO_MAC_SEL_BT656;
		} else if (cfg->bt_cfg.mode == BT_MODE_601) {
			disp_set_intf(dev, VO_DISP_INTF_BT601);
			bt_mode = VO_MAC_SEL_BT601;
		} else {
			TRACE_VO(DBG_ERR, "invalid bt-mode(%d)\n", cfg->bt_cfg.mode);
			return -1;
		}

		if (cfg->bt_cfg.mode == BT_MODE_1120) {
			dphy_dsi_set_pll(dev, cfg->bt_cfg.pixelclock, 4, 24);
			vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();
			vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 1;
			vi_sys_clk_ctrl2.b.disp_div_cnt = 0;
			vi_sys_clk_ctrl2.b.disp_div_up = 1;
			vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
		} else if (cfg->bt_cfg.mode == BT_MODE_656) {
			dphy_dsi_set_pll(dev, cfg->bt_cfg.pixelclock * 2, 4, 24);
			vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();
			vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 0;
			vi_sys_clk_ctrl2.b.disp_div_cnt = 0;
			vi_sys_clk_ctrl2.b.disp_div_up = 1;
			vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
		} else if (cfg->bt_cfg.mode == BT_MODE_601) {
			dphy_dsi_set_pll(dev, cfg->bt_cfg.pixelclock * 2, 4, 24);
			vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();
			vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 0;
			vi_sys_clk_ctrl2.b.disp_div_cnt = 0;
			vi_sys_clk_ctrl2.b.disp_div_up = 1;
			vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
		}

		if (cfg->bt_cfg.mode == BT_MODE_656)
			fmt_sel = 0;
		else if (cfg->bt_cfg.mode == BT_MODE_601)
			fmt_sel = 2;
		else if (cfg->bt_cfg.mode == BT_MODE_1120)
			fmt_sel = 1;

		//set csc value
		disp_set_out_csc(dev, DISP_CSC_601_FULL_RGB2YUV);
		vo_mac_set_sel_type(dev, bt_mode);

		// to do
		// _disp_sel_pinmux(dev, cfg->intf_type, &cfg->bt_cfg);

		enc.raw = 0;
		enc.b.fmt_sel  = fmt_sel;
		enc.b.data_seq = cfg->bt_cfg.data_seq;
		enc.b.clk_inv  = cfg->bt_cfg.bt_clk_inv;
		enc.b.hs_inv   = cfg->bt_cfg.bt_hs_inv;
		enc.b.vs_inv   = cfg->bt_cfg.bt_vs_inv;
		sync.b.sav_vld = 0x80;
		sync.b.sav_blk = 0xab;
		sync.b.eav_vld = 0x9d;
		sync.b.eav_blk = 0xb6;
		vo_mac_bt_set(dev, enc, sync);
		vo_mac_bt_en(dev);
	} else if (cfg->intf_type == VO_DISP_INTF_HW_I80) {
		disp_set_intf(dev, VO_DISP_INTF_HW_I80);
		vo_mac_set_i80_if(dev, false, cfg->mcu_cfg.mode);
		if (cfg->mcu_cfg.mode == MCU_MODE_RGB565) {
			dphy_dsi_set_pll(dev, cfg->mcu_cfg.pixelclock * 4, 4, 24);
			vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();
			vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 0;
			vi_sys_clk_ctrl2.b.disp_div_cnt = 4;
			vi_sys_clk_ctrl2.b.disp_div_up = 1;
			vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
		} else if (cfg->mcu_cfg.mode == MCU_MODE_RGB888) {
			dphy_dsi_set_pll(dev, cfg->mcu_cfg.pixelclock * 6, 4, 24);
			vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();
			vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 0;
			vi_sys_clk_ctrl2.b.disp_div_cnt = 6;
			vi_sys_clk_ctrl2.b.disp_div_up = 1;
			vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
		}

		//vo_mux_sel
		vo_mac_sel_pinmux(dev, cfg->intf_type, &cfg->mcu_cfg);
		hw_mcu_cmd_send(dev, cfg->mcu_cfg.instrs.instr_cmd, cfg->mcu_cfg.instrs.instr_num);
		vo_mac_hw_i80_en(dev, true);
	} else {
		TRACE_VO(DBG_ERR, "invalid disp-intf(%d)\n", cfg->intf_type);
		return -1;
	}

	disp_reg_force_up(dev);

	return 0;
}

/****************************************************************************
 * SDK device APIs
 ****************************************************************************/
static int vo_set_pub_attr(vo_dev dev, vo_pub_attr_s *pub_attr)
{
	struct vo_disp_intf_cfg cfg;
	struct vo_dv_timings dv_timings;
	unsigned short rgb[3];
	unsigned int panel_status = 0;
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) should be disabled.\n", dev);
		return 0;
	}

	disp_timing_setup_from_reg(dev);

	memset(&cfg, 0, sizeof(cfg));

	if (pub_attr->intf_sync == VO_OUTPUT_USER) {
		dv_timings.bt.interlaced = !pub_attr->sync_info.iop;
		dv_timings.bt.height = pub_attr->sync_info.vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = pub_attr->sync_info.vbb;
		dv_timings.bt.vfrontporch = pub_attr->sync_info.vfb;
		dv_timings.bt.width = pub_attr->sync_info.hact;
		dv_timings.bt.hbackporch = pub_attr->sync_info.hbb;
		dv_timings.bt.hfrontporch = pub_attr->sync_info.hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = pub_attr->sync_info.hpw;
		dv_timings.bt.vsync = pub_attr->sync_info.vpw;
		dv_timings.bt.polarities = ((pub_attr->sync_info.ivs) ? 0 : 0x1) |
					   ((pub_attr->sync_info.ihs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = pub_attr->sync_info.frame_rate *
					   (dv_timings.bt.vbackporch + dv_timings.bt.height +
					   dv_timings.bt.vfrontporch + dv_timings.bt.vsync) *
					   (dv_timings.bt.hbackporch + dv_timings.bt.width +
					   dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
	} else if (pub_attr->intf_sync < VO_OUTPUT_USER) {
		dv_timings.bt.interlaced = !sync_info[pub_attr->intf_sync].iop;
		dv_timings.bt.height = sync_info[pub_attr->intf_sync].vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = sync_info[pub_attr->intf_sync].vbb;
		dv_timings.bt.vfrontporch = sync_info[pub_attr->intf_sync].vfb;
		dv_timings.bt.width = sync_info[pub_attr->intf_sync].hact;
		dv_timings.bt.hbackporch = sync_info[pub_attr->intf_sync].hbb;
		dv_timings.bt.hfrontporch = sync_info[pub_attr->intf_sync].hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = sync_info[pub_attr->intf_sync].hpw;
		dv_timings.bt.vsync = sync_info[pub_attr->intf_sync].vpw;
		dv_timings.bt.polarities = ((sync_info[pub_attr->intf_sync].ivs) ? 0 : 0x1) |
					   ((sync_info[pub_attr->intf_sync].ihs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = sync_info[pub_attr->intf_sync].frame_rate *
					   (dv_timings.bt.vbackporch + dv_timings.bt.height +
					   dv_timings.bt.vfrontporch + dv_timings.bt.vsync) *
					   (dv_timings.bt.hbackporch + dv_timings.bt.width +
					   dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
		pub_attr->sync_info = sync_info[pub_attr->intf_sync];
	} else {
		TRACE_VO(DBG_ERR, "VO Sync Info(%d) invalid.\n", pub_attr->intf_sync);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (dv_timings.bt.interlaced) {
		TRACE_VO(DBG_ERR, "VO not support interlaced timing.\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	if ((dv_timings.bt.pixelclock == 0) || (dv_timings.bt.height == 0) || (dv_timings.bt.width == 0)) {
		TRACE_VO(DBG_ERR, "VO Sync timing invalid. width(%d) height(%d) pixelclock(%llu)\n",
			 dv_timings.bt.width, dv_timings.bt.height, dv_timings.bt.pixelclock);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (pub_attr->intf_type == VO_INTF_MIPI) {
		cfg.intf_type = VO_DISP_INTF_DSI;
		TRACE_VO(DBG_INFO, "MIPI-DSI should be setup by mipi-tx.\n");
	} else if (pub_attr->intf_type == VO_INTF_HW_MCU) {
		const vo_hw_mcu_cfg_s *McuCfg = &pub_attr->stmcucfg;
		if (McuCfg->mode >= VO_MCU_MODE_MAX) {
			TRACE_VO(DBG_ERR, "VO DEV(%d) invalid MCU Format(%d).\n", dev, McuCfg->mode);
			return ERR_VO_ILLEGAL_PARAM;
		}

		cfg.intf_type = VO_DISP_INTF_HW_I80;
		cfg.mcu_cfg.mode = (enum mcu_mode)McuCfg->mode;
		cfg.mcu_cfg.pixelclock = osal_div_u64(dv_timings.bt.pixelclock, 1000);
		memcpy(&cfg.mcu_cfg.pins, &pub_attr->stmcucfg.pins, sizeof(struct vo_pins));
		memcpy(&cfg.mcu_cfg.instrs, &pub_attr->stmcucfg.instrs, sizeof(struct mcu_instrs));
		if (vo_set_interface(dev, &cfg) != 0) {
			TRACE_VO(DBG_ERR, "VO INTF configure failured.\n");
			return -1;
		}
	} else {
		TRACE_VO(DBG_ERR, "VO invalid INTF type(0x%x)\n", pub_attr->intf_type);
		return ERR_VO_ILLEGAL_PARAM;
	}

	vo_get_panelstatus(dev, &panel_status);
	TRACE_VO(DBG_INFO, "panel_status[%d], intf_type[%d]\n", panel_status, pub_attr->intf_type);

	if (pub_attr->intf_type != VO_INTF_MIPI && !panel_status) {
		struct disp_timing timing;
		vo_fill_disp_timing(&timing, &dv_timings.bt);
		disp_set_timing(dev, &timing);
	}

	rgb[2] = pub_attr->bgcolor & 0x3ff;
	rgb[1] = (pub_attr->bgcolor >> 10) & 0x3ff;
	rgb[0] = (pub_attr->bgcolor >> 20) & 0x3ff;

	disp_set_frame_bgcolor(dev, rgb[0], rgb[1], rgb[2]);

	osal_memcpy(&dev_ctx->pub_attr, pub_attr, sizeof(*pub_attr));

	return 0;
}

static int vo_get_pub_attr(vo_dev dev, vo_pub_attr_s *pub_attr)
{
	enum vo_mac_sel vo_mac_sel;
	struct disp_timing *timing = disp_get_timing(dev);
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	dev_ctx->pub_attr.sync_info.hact = timing->hfde_end - timing->hfde_start + 1;
	dev_ctx->pub_attr.sync_info.vact = timing->vfde_end - timing->vfde_start + 1;

	vo_mac_sel = vo_mac_mux_get(dev);

	switch (vo_mac_sel) {
	case VO_MAC_SEL_RGB:
		dev_ctx->pub_attr.intf_type = VO_INTF_PARALLEL_RGB;
		break;

	case VO_MAC_SEL_SERIAL_RGB:
		dev_ctx->pub_attr.intf_type = VO_INTF_SERIAL_RGB;
		break;

	case VO_MAC_SEL_HW_MCU:
		dev_ctx->pub_attr.intf_type = VO_INTF_HW_MCU;
		break;

	case VO_MAC_SEL_BT656:
		dev_ctx->pub_attr.intf_type = VO_INTF_BT656;
		break;

	case VO_MAC_SEL_BT1120:
		dev_ctx->pub_attr.intf_type = VO_INTF_BT1120;
		break;

	default:
		if (dphy_get_dsi_clk_lane_status(dev)) {
			if (dphy_is_lvds(dev))
				dev_ctx->pub_attr.intf_type = VO_INTF_LVDS;
			else
				dev_ctx->pub_attr.intf_type = VO_INTF_MIPI;
		} else {
			dev_ctx->pub_attr.intf_type = VO_INTF_BUTT;
		}
		break;
	}

	osal_memcpy(pub_attr, &dev_ctx->pub_attr, sizeof(vo_pub_attr_s));

	return 0;
}

static int vo_set_lvds_param(vo_dev dev, vo_lvds_attr_s *lvds_param)
{
	struct vo_dev_ctx *dev_ctx;
	struct vo_disp_intf_cfg cfg;
	struct vo_dv_timings dv_timings = {0};
	unsigned short i;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_sync == VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock = dev_ctx->pub_attr.sync_info.frame_rate *
					   (dev_ctx->pub_attr.sync_info.vbb +
					   (dev_ctx->pub_attr.sync_info.vact <<
					   !dev_ctx->pub_attr.sync_info.iop) +
					   dev_ctx->pub_attr.sync_info.vfb +
					   dev_ctx->pub_attr.sync_info.vpw) *
					   (dev_ctx->pub_attr.sync_info.hbb +
					   dev_ctx->pub_attr.sync_info.hact +
					   dev_ctx->pub_attr.sync_info.hfb +
					   dev_ctx->pub_attr.sync_info.hpw);
	} else if (dev_ctx->pub_attr.intf_sync < VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock = sync_info[dev_ctx->pub_attr.intf_sync].frame_rate *
					   (sync_info[dev_ctx->pub_attr.intf_sync].vbb +
					   (sync_info[dev_ctx->pub_attr.intf_sync].vact <<
					   !sync_info[dev_ctx->pub_attr.intf_sync].iop) +
					   sync_info[dev_ctx->pub_attr.intf_sync].vfb +
					   sync_info[dev_ctx->pub_attr.intf_sync].vpw) *
					   (sync_info[dev_ctx->pub_attr.intf_sync].hbb +
					   sync_info[dev_ctx->pub_attr.intf_sync].hact +
					   sync_info[dev_ctx->pub_attr.intf_sync].hfb +
					   sync_info[dev_ctx->pub_attr.intf_sync].hpw);
	}

	if (dev_ctx->pub_attr.intf_type == VO_INTF_LVDS) {
		cfg.intf_type = VO_DISP_INTF_LVDS;
		cfg.lvds_cfg.out_bits = lvds_param->out_bits;
		cfg.lvds_cfg.mode = lvds_param->lvds_vesa_mode;
		cfg.lvds_cfg.chn_num = lvds_param->chn_num;
		if (cfg.lvds_cfg.chn_num > 1) {
			TRACE_VO(DBG_ERR, "lvds only surpports single link!\n");
			return ERR_VO_ILLEGAL_PARAM;
		}
		cfg.lvds_cfg.vs_out_en = 1;
		cfg.lvds_cfg.hs_out_en = 1;
		cfg.lvds_cfg.hs_blk_en = 1;
		cfg.lvds_cfg.msb_lsb_data_swap = 1;
		cfg.lvds_cfg.serial_msb_first = lvds_param->data_big_endian;
		cfg.lvds_cfg.even_odd_link_swap = 0;
		cfg.lvds_cfg.enable = 1;

		osal_div_u64(dv_timings.bt.pixelclock, 1000);
		cfg.lvds_cfg.pixelclock = dv_timings.bt.pixelclock;

		for (i = 0; i < VO_LVDS_LANE_MAX; ++i) {
			cfg.lvds_cfg.lane_id[i] = lvds_param->lane_id[i];
			cfg.lvds_cfg.lane_pn_swap[i] = lvds_param->lane_pn_swap[i];
		}

		if (vo_set_interface(dev, &cfg) != 0) {
			TRACE_VO(DBG_ERR, "VO INTF configure failured.\n");
			return -1;
		}
	} else {
		TRACE_VO(DBG_ERR, "not working under th lvds interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_memcpy(&dev_ctx->lvds_param, lvds_param, sizeof(*lvds_param));

	return 0;
}

static int vo_get_lvds_param(vo_dev dev, vo_lvds_attr_s *lvds_param)
{
	struct vo_dev_ctx *dev_ctx;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_type != VO_INTF_LVDS) {
		TRACE_VO(DBG_ERR, "not working under th lvds interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_memcpy(lvds_param, &dev_ctx->lvds_param, sizeof(*lvds_param));

	return 0;
}

static int vo_set_bt_param(vo_dev dev, vo_bt_attr_s *bt_param)
{
	struct vo_dev_ctx *dev_ctx;
	struct vo_disp_intf_cfg cfg;
	struct vo_dv_timings dv_timings = {0};
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_sync == VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock = dev_ctx->pub_attr.sync_info.frame_rate *
					   (dev_ctx->pub_attr.sync_info.vbb +
					   (dev_ctx->pub_attr.sync_info.vact <<
					   !dev_ctx->pub_attr.sync_info.iop) +
					   dev_ctx->pub_attr.sync_info.vfb +
					   dev_ctx->pub_attr.sync_info.vpw) *
					   (dev_ctx->pub_attr.sync_info.hbb +
					   dev_ctx->pub_attr.sync_info.hact +
					   dev_ctx->pub_attr.sync_info.hfb +
					   dev_ctx->pub_attr.sync_info.hpw);
	} else if (dev_ctx->pub_attr.intf_sync < VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock = sync_info[dev_ctx->pub_attr.intf_sync].frame_rate *
					   (sync_info[dev_ctx->pub_attr.intf_sync].vbb +
					   (sync_info[dev_ctx->pub_attr.intf_sync].vact <<
					   !sync_info[dev_ctx->pub_attr.intf_sync].iop) +
					   sync_info[dev_ctx->pub_attr.intf_sync].vfb +
					   sync_info[dev_ctx->pub_attr.intf_sync].vpw) *
					   (sync_info[dev_ctx->pub_attr.intf_sync].hbb +
					   sync_info[dev_ctx->pub_attr.intf_sync].hact +
					   sync_info[dev_ctx->pub_attr.intf_sync].hfb +
					   sync_info[dev_ctx->pub_attr.intf_sync].hpw);
	}

	if (dev_ctx->pub_attr.intf_type == VO_INTF_BT656 ||
	    dev_ctx->pub_attr.intf_type == VO_INTF_BT1120) {
		cfg.bt_cfg.data_seq = bt_param->data_seq;
		cfg.bt_cfg.bt_clk_inv = bt_param->bt_clk_inv;
		cfg.bt_cfg.bt_vs_inv = bt_param->bt_vs_inv;
		cfg.bt_cfg.bt_hs_inv = bt_param->bt_hs_inv;
		osal_div_u64(dv_timings.bt.pixelclock, 1000);
		cfg.bt_cfg.pixelclock = dv_timings.bt.pixelclock;
		cfg.bt_cfg.pins.pin_num = bt_param->pin_num;
		osal_memcpy(&cfg.bt_cfg.pins.d_pins, bt_param->d_pins, sizeof(bt_param->d_pins));

		if (dev_ctx->pub_attr.intf_type == VO_INTF_BT656) {
			cfg.bt_cfg.mode = BT_MODE_656;
			cfg.intf_type = VO_DISP_INTF_BT656;
		} else if (dev_ctx->pub_attr.intf_type == VO_INTF_BT1120) {
			cfg.bt_cfg.mode = BT_MODE_1120;
			cfg.intf_type = VO_DISP_INTF_BT1120;
		}

		if (vo_set_interface(dev, &cfg) != 0) {
			TRACE_VO(DBG_ERR, "VO INTF configure failured.\n");
			return -1;
		}
	} else {
		TRACE_VO(DBG_ERR, "not working under th bt interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_memcpy(&dev_ctx->bt_param, bt_param, sizeof(*bt_param));

	return 0;
}

static int vo_get_bt_param(vo_dev dev, vo_bt_attr_s *bt_param)
{
	struct vo_dev_ctx *dev_ctx;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_type != VO_INTF_BT656 &&
	    dev_ctx->pub_attr.intf_type != VO_INTF_BT1120) {
		TRACE_VO(DBG_ERR, "not working under th bt interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_memcpy(bt_param, &dev_ctx->bt_param, sizeof(*bt_param));

	return 0;
}

static int vo_enable(vo_dev dev)
{
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	if (dev_ctx->pub_attr.intf_type == 0) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", dev);
		// return ERR_VO_DEV_NOT_CONFIG;
	}

	if (dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) should be disabled.\n", dev);
		// return ERR_VO_DEV_HAS_ENABLED;
	}

	dev_ctx->is_dev_enable = true;

	return 0;
}

int vo_disable(vo_dev dev)
{
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	if (!dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "vo_dev(%d) already disabled.\n", dev);
		return ERR_VO_DEV_NOT_ENABLED;
	}

	dev_ctx->is_dev_enable = false;

	return 0;
}


/****************************************************************************
 * SDK layer APIs
 ****************************************************************************/
static int vo_set_displaybuflen(vo_layer layer, unsigned int buflen)
{
	int ret = -1;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	if (buflen <= 2) {
		TRACE_VO(DBG_ERR, "layer(%d) buflen(%d) must be bigger than 2", layer, buflen);
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].display_buflen = buflen;
	osal_mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

static int vo_get_displaybuflen(vo_layer layer, unsigned int *buflen)
{
	int ret = -1;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	*buflen = g_vo_ctx->layer_ctx[layer].display_buflen;

	return 0;
}

static int vo_enablevideolayer(vo_layer layer)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_disable(layer);
	if (ret != 0)
		return 0;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	osal_mutex_lock(&layer_ctx->layer_lock);
	layer_ctx->is_layer_enable = true;
	layer_ctx->event = 0;
	osal_wait_init(&layer_ctx->wq);
	osal_mutex_unlock(&layer_ctx->layer_lock);

	return ret;
}

int vo_disablevideolayer(vo_layer layer)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!layer_ctx->is_layer_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't enabled yet.\n", layer);
		return ERR_VO_VIDEO_NOT_ENABLED;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	osal_wait_destroy(&layer_ctx->wq);
	layer_ctx->is_layer_enable = false;
	layer_ctx->frame_num = 0;
	layer_ctx->frame_rate = 0;
	layer_ctx->src_frame_num = 0;
	layer_ctx->src_frame_rate = 0;
	layer_ctx->frame_index = 0;
	layer_ctx->display_pts = 0;
	layer_ctx->predone_pts = 0;
	layer_ctx->bw_fail = 0;
	layer_ctx->vgop_bw_fail = 0;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_get_videolayerattr(vo_layer layer, vo_video_layer_attr_s *layer_attr)
{
	int ret = -1;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	osal_memcpy(layer_attr, &g_vo_ctx->layer_ctx[layer].layer_attr, sizeof(*layer_attr));

	return 0;
}

static int vo_set_videolayerattr(vo_layer layer, const vo_video_layer_attr_s *layer_attr)
{
	struct disp_rect rect;
	unsigned short rgb[3] = {0, 0, 0};
	int ret = -1;
	vo_dev dev;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	// ret = check_video_layer_disable(layer);
	// if (ret != 0)
	// 	return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!VO_SUPPORT_FMT(layer_attr->pixformat)) {
		TRACE_VO(DBG_ERR, "layer(%d) pixformat(%d) unsupported\n",
			 layer, layer_attr->pixformat);
		return ERR_VO_ILLEGAL_PARAM;
	}

	dev = layer_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_DEBUG, "layer(%d) unbind device", layer);
		return ERR_VO_SYS_NOTREADY;
	}

	if (layer_attr->img_size.width != layer_attr->disp_rect.width ||
	    layer_attr->img_size.height != layer_attr->disp_rect.height) {
		TRACE_VO(DBG_ERR, "layer(%d) img_size(%d %d) disp_rect(%d %d) isn't the same.\n",
			 layer, layer_attr->img_size.width, layer_attr->img_size.height,
			 layer_attr->disp_rect.width, layer_attr->disp_rect.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (layer_attr->img_size.width < VO_MIN_CHN_WIDTH ||
	    layer_attr->img_size.height < VO_MIN_CHN_HEIGHT) {
		TRACE_VO(DBG_ERR, "layer(%d) Size(%d %d) too small.\n",
			 layer, layer_attr->img_size.width, layer_attr->img_size.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	vo_sdk_setfmt(layer_attr->img_size.width, layer_attr->img_size.height,
		      layer_attr->pixformat, dev);

	disp_set_window_bgcolor(dev, rgb[0], rgb[1], rgb[2]);
	disp_enable_window_bgcolor(layer, true);

	rect.w = layer_attr->disp_rect.width;
	rect.h = layer_attr->disp_rect.height;
	rect.x = layer_attr->disp_rect.x;
	rect.y = layer_attr->disp_rect.y;
	disp_set_rect(dev, rect);

	osal_mutex_lock(&layer_ctx->layer_lock);
	layer_ctx->layer_attr.disp_rect = layer_attr->disp_rect;
	layer_ctx->layer_attr.img_size = layer_attr->img_size;
	//not use when 1 chn
	layer_ctx->layer_attr.frame_rate = layer_attr->frame_rate;
	layer_ctx->layer_attr.pixformat = layer_attr->pixformat;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	TRACE_VO(DBG_DEBUG, "layer(%d) framerate(%d) image-size(%d * %d) disp-rect(%d-%d-%d-%d).\n",
		 layer,
		 layer_ctx->layer_attr.frame_rate,
		 layer_attr->img_size.width, layer_attr->img_size.height,
		 layer_attr->disp_rect.x, layer_attr->disp_rect.y,
		 layer_attr->disp_rect.width, layer_attr->disp_rect.height);

	return 0;
}

static int vo_get_layer_proc_amp(vo_layer layer, int *proc_amp)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	osal_memcpy(proc_amp, layer_ctx->proc_amp, sizeof(layer_ctx->proc_amp));

	return 0;
}

static int vo_set_layer_proc_amp(vo_layer layer, const int *proc_amp)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!IS_FMT_YUV(layer_ctx->layer_attr.pixformat)) {
		TRACE_VO(DBG_ERR, "layer(%d) Only YUV format support.\n", layer);
		return ERR_VO_NOT_SUPPORT;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	osal_memcpy(layer_ctx->proc_amp, proc_amp, sizeof(layer_ctx->proc_amp));
	osal_mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

/****************************************************************************
 * SDK chn APIs
 ****************************************************************************/
static int vo_clear_chnbuf(vo_layer layer, vo_chn chn, bool clear)
{
	int ret = -1;
	vb_blk blk;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];

	osal_mutex_lock(&layer_ctx->layer_lock);
	//clear chn waitq vb
	while (!base_mod_jobs_waitq_empty(&chn_ctx->chn_jobs)) {
		blk = base_mod_jobs_waitq_pop(&chn_ctx->chn_jobs);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	//clear chn workq vb
	while (clear && !base_mod_jobs_workq_empty(&chn_ctx->chn_jobs)) {
		blk = base_mod_jobs_workq_pop(&chn_ctx->chn_jobs);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	release_buffer(layer_ctx, &layer_ctx->list_done);
	release_buffer(layer_ctx, &layer_ctx->list_work);
	release_buffer(layer_ctx, &layer_ctx->list_wait);

	osal_mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_send_frame(vo_layer layer, vo_chn chn, video_frame_info_s *video_frame, int millisec)
{
	mmf_chn_s mmf_chn = {.mod_id = ID_VO, .dev_id = layer, .chn_id = chn};
	vb_blk blk;
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	size_s size;

	UNUSED(millisec);

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];

	if (layer_ctx->layer_attr.pixformat != video_frame->video_frame.pixel_format) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) PixelFormat(%d) mismatch.\n",
			 layer, chn, video_frame->video_frame.pixel_format);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if ((chn_ctx->rotation == ROTATION_90) || (chn_ctx->rotation == ROTATION_270)) {
		size.width = layer_ctx->layer_attr.img_size.height;
		size.height = layer_ctx->layer_attr.img_size.width;
	} else
		size = layer_ctx->layer_attr.img_size;

	if ((size.width != (video_frame->video_frame.width -
		video_frame->video_frame.offset_left - video_frame->video_frame.offset_right)) ||
		(size.height != (video_frame->video_frame.height -
		video_frame->video_frame.offset_top - video_frame->video_frame.offset_bottom))) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) size(%d * %d) mismatch.\n",
			 layer, chn, video_frame->video_frame.width, video_frame->video_frame.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (IS_FRAME_OFFSET_INVALID(video_frame->video_frame)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) frame offset (%d %d %d %d) invalid\n",
			 layer, chn,
			 video_frame->video_frame.offset_left,
			 video_frame->video_frame.offset_right,
			 video_frame->video_frame.offset_top,
			 video_frame->video_frame.offset_bottom);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (IS_FMT_YUV420(layer_ctx->layer_attr.pixformat)) {
		if ((video_frame->video_frame.width -
		     video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) YUV420 can't accept odd frame valid width\n",
				 layer, chn);
			TRACE_VO(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				 video_frame->video_frame.width,
				 video_frame->video_frame.offset_left,
				 video_frame->video_frame.offset_right);
			return ERR_VO_ILLEGAL_PARAM;
		}
		if ((video_frame->video_frame.height -
		     video_frame->video_frame.offset_top -
		     video_frame->video_frame.offset_bottom) & 0x01) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) YUV420 can't accept odd frame valid height\n",
				 layer, chn);
			TRACE_VO(DBG_ERR, "height(%d) offset_top(%d) offset_bottom(%d)\n",
				 video_frame->video_frame.height, video_frame->video_frame.offset_top,
				 video_frame->video_frame.offset_bottom);
			return ERR_VO_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(layer_ctx->layer_attr.pixformat)) {
		if ((video_frame->video_frame.width -
		     video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) YUV422 can't accept odd frame valid width\n",
				 layer, chn);
			TRACE_VO(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				 video_frame->video_frame.width,
				 video_frame->video_frame.offset_left,
				 video_frame->video_frame.offset_right);
			return ERR_VO_ILLEGAL_PARAM;
		}
	}

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Invalid phy-addr(%llx). Can't locate vb_blk.\n",
			 layer, chn, video_frame->video_frame.phyaddr[0]);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (base_fill_videoframe2buffer(mmf_chn, video_frame, &((struct vb_s *)(uintptr_t)blk)->buf) != 0) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Invalid parameter\n", layer, chn);
		return ERR_VO_ILLEGAL_PARAM;
	}

	vo_recv_frame(mmf_chn, blk, NULL);

	return ret;
}

static int vo_send_logo_from_ion(vo_layer layer, vo_chn chn, video_frame_info_s *pstVideoFrame, int s32MilliSec)
{
	int ret = -1;
	extern int _vo_send_logo_from_ion(vo_layer layer, vo_chn chn, video_frame_info_s *pstVideoFrame, int s32MilliSec);
	ret = _vo_send_logo_from_ion(layer, chn, pstVideoFrame, s32MilliSec);
	if (ret != 0)
		return ret;

	return 0;
}

static int vo_get_chn_attr(vo_layer layer, vo_chn chn, vo_chn_attr_s *chn_attr)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	osal_memcpy(chn_attr, &chn_ctx->chn_attr, sizeof(*chn_attr));

	return 0;
}

static int vo_set_chn_attr(vo_layer layer, vo_chn chn, const vo_chn_attr_s *chn_attr)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (chn_attr->rect.width < VO_MIN_CHN_WIDTH || chn_attr->rect.height < VO_MIN_CHN_HEIGHT ||
	    (chn_attr->rect.width + chn_attr->rect.x > layer_ctx->layer_attr.img_size.width) ||
	    (chn_attr->rect.height + chn_attr->rect.y > layer_ctx->layer_attr.img_size.height) ||
	    chn_attr->rect.x < 0 || chn_attr->rect.y < 0) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) rect(%d %d %d %d) invalid.\n",
			 layer, chn, chn_attr->rect.x, chn_attr->rect.y,
			 chn_attr->rect.width, chn_attr->rect.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->chn_attr.priority = chn_attr->priority;
	chn_ctx->chn_attr.rect = chn_attr->rect;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	TRACE_VO(DBG_DEBUG, "layer(%d) chn(%d) priority(%d) chn-rect(%d-%d-%d-%d).\n",
		 layer, chn,
		 chn_attr->priority,
		 chn_attr->rect.x, chn_attr->rect.y,
		 chn_attr->rect.width, chn_attr->rect.height);

	return 0;
}

static int vo_set_chn_framerate(vo_layer layer, vo_chn chn, unsigned int frame_rate)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (frame_rate > chn_ctx->frame_rate) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) FrameRate(%d) invalid.\n",
			 layer, chn, frame_rate);
	}

	//ueser set framerate
	chn_ctx->frame_rate_user_set = frame_rate;

	return 0;
}

static int vo_get_chn_framerate(vo_layer layer, vo_chn chn, unsigned int *frame_rate)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	//realframerate
	*frame_rate = chn_ctx->frame_rate;

	return 0;
}

static int vo_get_chn_pts(vo_layer layer, vo_chn chn, unsigned long long *chn_pts)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	*chn_pts = chn_ctx->display_pts;

	return 0;
}

static int vo_get_chn_status(vo_layer layer, vo_chn chn, vo_query_status_s *status)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	//threshold + VO_CHN_WORKQ + depth
	status->chn_buf_used = chn_ctx->threshold;

	return 0;
}

static int vo_enable_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;
	struct vo_layer_ctx *layer_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];

	if (chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already enabled.\n",
			 layer, chn);
		return 0;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->is_chn_enable = true;
	chn_ctx->threshold = layer_ctx->display_buflen;
	base_mod_jobs_init(&chn_ctx->chn_jobs,
			   chn_ctx->threshold - VO_CHN_WORKQ,
			   VO_CHN_WORKQ,
			   0);
	osal_mutex_unlock(&layer_ctx->layer_lock);

	ret = vo_create_thread(layer);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to create thread, layer(%d).\n", layer);
		return ret;
	}

	ret = vo_start_streaming(layer_ctx->bind_dev_id);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to vo start streaming\n");
		return ret;
	}

	return ret;
}

int vo_disable_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;
	struct vo_layer_ctx *layer_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];

	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n",
			 layer, chn);
		return 0;
	}

	ret = vo_stop_streaming(layer_ctx->bind_dev_id);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to vo stop streaming\n");
		return ret;
	}

	ret = vo_destroy_thread(layer);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to destroy thread, layer(%d).\n", layer);
		return ret;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);

	release_buffer(layer_ctx, &layer_ctx->list_done);
	release_buffer(layer_ctx, &layer_ctx->list_work);
	release_buffer(layer_ctx, &layer_ctx->list_wait);

	base_mod_jobs_exit(&chn_ctx->chn_jobs);

	chn_ctx->is_chn_enable = false;
	chn_ctx->frame_num = 0;
	chn_ctx->frame_rate = 0;
	chn_ctx->src_frame_num = 0;
	chn_ctx->src_frame_rate = 0;
	chn_ctx->frame_index = 0;
	chn_ctx->frame_rate_user_set = 60;
	chn_ctx->display_pts = 0;
	chn_ctx->predone_pts = 0;
	chn_ctx->pause = false;
	chn_ctx->rotation = ROTATION_0;

	osal_mutex_unlock(&layer_ctx->layer_lock);

	TRACE_VO(DBG_INFO, "layer(%d) chn(%d) disabled.\n",
		 layer, chn);

	return 0;
}

static int vo_hide_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	unsigned short rgb[3] = {0, 0, 0};

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];
	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n",
			 layer, chn);
		return 0;
	}

	disp_set_pattern(layer_ctx->bind_dev_id, PAT_TYPE_FULL, PAT_COLOR_USR, rgb);
	// disp_set_frame_bgcolor(layer_ctx->bind_dev_id, 0, 0, 0);

	osal_mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->hide = true;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_show_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	unsigned short rgb[3] = {0, 0, 0};

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];
	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n",
			 layer, chn);
		return 0;
	}

	disp_set_pattern(layer_ctx->bind_dev_id, PAT_TYPE_OFF, PAT_COLOR_MAX, rgb);

	osal_mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->hide = false;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_pause_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &layer_ctx->chn_ctx[chn];
	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n",
			 layer, chn);
		return 0;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->pause = true;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	jobs = &chn_ctx->chn_jobs;
	osal_mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((vb_blk)(uintptr_t)vb);
	}
	osal_mutex_unlock(&jobs->lock);

	return 0;
}

static int vo_resume_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	if (!g_vo_ctx->layer_ctx[layer].chn_ctx[chn].is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}

	osal_mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].pause = false;
	osal_mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

int vo_get_chnrotation(vo_layer layer, vo_chn chn, rotation_e *rotation)
{
	int ret = -1;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	*rotation = g_vo_ctx->layer_ctx[layer].chn_ctx[chn].rotation;

	return 0;
}

static int vo_set_chnrotation(vo_layer layer, vo_chn chn, rotation_e rotation)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;
	struct vo_layer_ctx *layer_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (rotation >= ROTATION_MAX) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) invalid rotation(%d).\n", layer, chn, rotation);
		return ERR_VO_ILLEGAL_PARAM;
	} else if (rotation == ROTATION_0) {
		chn_ctx->rotation = rotation;
		return 0;
	}

	if (!GDC_SUPPORT_FMT(layer_ctx->layer_attr.pixformat)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) invalid PixFormat(%d).\n",
			 layer, chn, layer_ctx->layer_attr.pixformat);
		return ERR_VO_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	// TODO: dummy settings
	chn_ctx->mesh.paddr = DEFAULT_MESH_PADDR;
	chn_ctx->rotation = rotation;
	osal_mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

/*****************************************************************************
 *  SDK layer ioctl operations
 ****************************************************************************/
long vo_sdk_ctrl(struct vo_ext_control *p)
{
	unsigned int id = p->sdk_id;
	long rc = 0;

	switch (id) {
	case VO_SDK_SET_CHNATTR: {
		struct vo_chn_attr_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_attr_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_attr_cfg));

		rc = vo_set_chn_attr(cfg.layer, cfg.chn, &cfg.chn_attr);
	}
	break;

	case VO_SDK_GET_CHNATTR: {
		struct vo_chn_attr_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_attr_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_attr_cfg));

		rc = vo_get_chn_attr(cfg.layer, cfg.chn, &cfg.chn_attr);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_chn_attr_cfg));
	}
	break;

	case VO_SDK_SET_CHNFRAMERATE: {
		struct vo_chn_frmrate_cfg vo_chn_frame_rate;
		check_struct_size(p->size, sizeof(struct vo_chn_frmrate_cfg));
		osal_memcpy(&vo_chn_frame_rate, p->ptr, sizeof(struct vo_chn_frmrate_cfg));

		rc = vo_set_chn_framerate(vo_chn_frame_rate.layer, vo_chn_frame_rate.chn, vo_chn_frame_rate.frame_rate);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_set_chn_framerate failed with ret(%lx).\n", rc);
			break;
		}

		osal_memcpy(p->ptr, &vo_chn_frame_rate, sizeof(struct vo_chn_frmrate_cfg));
	}
	break;

	case VO_SDK_GET_CHNFRAMERATE: {
		struct vo_chn_frmrate_cfg vo_chn_frame_rate;
		check_struct_size(p->size, sizeof(struct vo_chn_frmrate_cfg));
		osal_memcpy(&vo_chn_frame_rate, p->ptr, sizeof(struct vo_chn_frmrate_cfg));

		rc = vo_get_chn_framerate(vo_chn_frame_rate.layer, vo_chn_frame_rate.chn,
					  &vo_chn_frame_rate.frame_rate);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_framerate failed with ret(%lx).\n", rc);
			break;
		}

		osal_memcpy(p->ptr, &vo_chn_frame_rate, sizeof(struct vo_chn_frmrate_cfg));
	}
	break;

	case VO_SDK_GET_CHNPTS: {
		struct vo_chn_pts_cfg vo_chn_pts;
		check_struct_size(p->size, sizeof(struct vo_chn_pts_cfg));
		osal_memcpy(&vo_chn_pts, p->ptr, sizeof(struct vo_chn_pts_cfg));

		rc = vo_get_chn_pts(vo_chn_pts.layer, vo_chn_pts.chn, &vo_chn_pts.chn_pts);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_pts failed with ret(%lx).\n", rc);
			break;
		}

		osal_memcpy(p->ptr, &vo_chn_pts, sizeof(struct vo_chn_pts_cfg));
	}
	break;

	case VO_SDK_GET_CHNSTATUS: {
		struct vo_chn_status_cfg vo_chn_status;
		check_struct_size(p->size, sizeof(struct vo_chn_status_cfg));
		osal_memcpy(&vo_chn_status, p->ptr, sizeof(struct vo_chn_status_cfg));

		rc = vo_get_chn_status(vo_chn_status.layer, vo_chn_status.chn, &vo_chn_status.status);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_status failed with ret(%lx).\n", rc);
			break;
		}

		osal_memcpy(p->ptr, &vo_chn_status, sizeof(struct vo_chn_status_cfg));
	}
	break;

	case VO_SDK_GET_PUBATTR: {
		struct vo_pub_attr_cfg *cfg;
		cfg = osal_kzalloc(sizeof(struct vo_pub_attr_cfg), OSAL_GFP_ATOMIC);

		check_struct_size(p->size, sizeof(struct vo_pub_attr_cfg));
		osal_memcpy(cfg, p->ptr, sizeof(struct vo_pub_attr_cfg));

		rc = vo_get_pub_attr(cfg->dev, &cfg->pub_attr);
		osal_memcpy(p->ptr, cfg, sizeof(struct vo_pub_attr_cfg));
		osal_kfree(cfg);
	}
	break;

	case VO_SDK_SET_PUBATTR: {
		struct vo_pub_attr_cfg *cfg;
		cfg = osal_kzalloc(sizeof(struct vo_pub_attr_cfg), OSAL_GFP_ATOMIC);

		check_struct_size(p->size, sizeof(struct vo_pub_attr_cfg));
		osal_memcpy(cfg, p->ptr, sizeof(struct vo_pub_attr_cfg));

		rc = vo_set_pub_attr(cfg->dev, &cfg->pub_attr);
		osal_kfree(cfg);
	}
	break;

	case VO_SDK_SET_LVDSPARAM: {
		struct vo_lvds_param_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_lvds_param_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_lvds_param_cfg));

		rc = vo_set_lvds_param(cfg.dev, &cfg.lvds_param);
	}
	break;

	case VO_SDK_GET_LVDSPARAM: {
		struct vo_lvds_param_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_lvds_param_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_lvds_param_cfg));

		rc = vo_get_lvds_param(cfg.dev, &cfg.lvds_param);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_lvds_param_cfg));
	}
	break;

	case VO_SDK_SET_BTPARAM: {
		struct vo_bt_param_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_bt_param_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_bt_param_cfg));

		rc = vo_set_bt_param(cfg.dev, &cfg.bt_param);
	}
	break;

	case VO_SDK_GET_BTPARAM: {
		struct vo_bt_param_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_bt_param_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_bt_param_cfg));

		rc = vo_get_bt_param(cfg.dev, &cfg.bt_param);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_bt_param_cfg));
	}
	break;

	case VO_SDK_GET_PANELSTATUE: {
		struct vo_panel_status_cfg cfg;
		vo_dev dev;

		check_struct_size(p->size, sizeof(struct vo_panel_status_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_panel_status_cfg));

		dev = g_vo_ctx->layer_ctx[cfg.layer].bind_dev_id;
		vo_get_panelstatus(dev, &cfg.is_init);

		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_panel_status_cfg));
	}
	break;

	case VO_SDK_ENABLE_CHN: {
		struct vo_chn_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_cfg));

		rc = vo_enable_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_DISABLE_CHN: {
		struct vo_chn_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_cfg));

		rc = vo_disable_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_SHOW_CHN: {
		struct vo_chn_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_cfg));

		rc = vo_show_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_HIDE_CHN: {
		struct vo_chn_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_cfg));

		rc = vo_hide_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_ENABLE: {
		struct vo_dev_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_dev_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_dev_cfg));

		rc = vo_enable(cfg.dev);
	}
	break;

	case VO_SDK_DISABLE: {
		struct vo_dev_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_dev_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_dev_cfg));

		rc = vo_disable(cfg.dev);
	}
	break;

	case VO_SDK_ISENABLE: {
		struct vo_dev_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_dev_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_dev_cfg));

		cfg.enable = g_vo_ctx->dev_ctx[cfg.dev].is_dev_enable;

		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_dev_cfg));
	}
	break;

	case VO_SDK_SEND_FRAME: {
		struct vo_snd_frm_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_snd_frm_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_snd_frm_cfg));

		rc = vo_send_frame(cfg.layer, cfg.chn, &cfg.video_frame, cfg.millisec);
	}
	break;

	case VO_SDK_SEND_LOGO_FROMION: {
		struct vo_snd_frm_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_snd_frm_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_snd_frm_cfg));

		rc = vo_send_logo_from_ion(cfg.layer, cfg.chn, &cfg.video_frame, cfg.millisec);
	}
	break;

	case VO_SDK_CLEAR_CHNBUF: {
		struct vo_clear_chn_buf_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_clear_chn_buf_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_clear_chn_buf_cfg));

		rc = vo_clear_chnbuf(cfg.layer, cfg.chn, cfg.clear);
	}
	break;

	case VO_SDK_SET_DISPLAYBUFLEN: {
		struct vo_display_buflen_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_display_buflen_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_display_buflen_cfg));

		rc = vo_set_displaybuflen(cfg.layer, cfg.buflen);
	}
	break;

	case VO_SDK_GET_DISPLAYBUFLEN: {
		struct vo_display_buflen_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_display_buflen_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_display_buflen_cfg));

		rc = vo_get_displaybuflen(cfg.layer, &cfg.buflen);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_display_buflen_cfg));
	}
	break;

	case VO_SDK_GET_CHNROTATION: {
		struct vo_chn_rotation_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_rotation_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg));

		rc = vo_get_chnrotation(cfg.layer, cfg.chn, &cfg.rotation);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_display_buflen_cfg));
	}
	break;

	case VO_SDK_SET_CHNROTATION: {
		struct vo_chn_rotation_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_chn_rotation_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg));

		rc = vo_set_chnrotation(cfg.layer, cfg.chn, cfg.rotation);
	}
	break;

	case VO_SDK_SET_VIDEOLAYERATTR: {
		struct vo_video_layer_attr_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_video_layer_attr_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg));

		rc = vo_set_videolayerattr(cfg.layer, &cfg.layer_attr);
	}
	break;

	case VO_SDK_GET_VIDEOLAYERATTR: {
		struct vo_video_layer_attr_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_video_layer_attr_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg));

		rc = vo_get_videolayerattr(cfg.layer, &cfg.layer_attr);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_video_layer_attr_cfg));
	}
	break;

	case VO_SDK_SET_LAYER_PROC_AMP: {
		struct vo_layer_proc_amp_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_layer_proc_amp_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_layer_proc_amp_cfg));

		rc = vo_set_layer_proc_amp(cfg.layer, cfg.proc_amp);
	}
	break;

	case VO_SDK_GET_LAYER_PROC_AMP: {
		struct vo_layer_proc_amp_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_layer_proc_amp_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_layer_proc_amp_cfg));

		rc = vo_get_layer_proc_amp(cfg.layer, cfg.proc_amp);
		osal_memcpy(p->ptr, &cfg, sizeof(struct vo_layer_proc_amp_cfg));
	}
	break;

	case VO_SDK_ENABLE_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_video_layer_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg));

		rc = vo_enablevideolayer(cfg.layer);
	}
	break;

	case VO_SDK_DISABLE_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;
		check_struct_size(p->size, sizeof(struct vo_video_layer_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg));

		rc = vo_disablevideolayer(cfg.layer);
	}
	break;

	case VO_SDK_PAUSE_CHN: {
		struct vo_chn_cfg cfg;

		check_struct_size(p->size, sizeof(struct vo_chn_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_cfg));
		rc = vo_pause_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_RESUME_CHN: {
		struct vo_chn_cfg cfg;

		check_struct_size(p->size, sizeof(struct vo_chn_cfg));
		osal_memcpy(&cfg, p->ptr, sizeof(struct vo_chn_cfg));

		rc = vo_resume_chn(cfg.layer, cfg.chn);
	}
	break;

	default:
		break;
	}

	return rc;
}

/*****************************************************************************
 *  custom ioctl operations
 ****************************************************************************/
long vo_custom_ctrl(struct vo_ext_control *p)
{
	long rc = 0;
	unsigned int id = p->id;

	switch (id) {
	case VO_IOCTL_SET_CUSTOM_CSC: {
		struct disp_csc_matrix cfg;
		vo_layer layer = p->reserved[0];
		vo_dev dev = g_vo_ctx->layer_ctx[layer].bind_dev_id;

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = OSAL_EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		check_struct_size(p->size, sizeof(struct disp_csc_matrix));

		osal_memcpy(&cfg, p->ptr, sizeof(struct disp_csc_matrix));

		_disp_set_in_csc(dev, &cfg);
	}
	break;

	case VO_IOCTL_GAMMA_LUT_UPDATE: {
		int i = 0;
		struct disp_gamma_attr gamma_attr_sclr;
		vo_gamma_info_s gamma_attr;
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = OSAL_EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		check_struct_size(p->size, sizeof(vo_gamma_info_s));

		osal_memcpy(&gamma_attr, (void *)p->ptr, sizeof(vo_gamma_info_s));

		gamma_attr_sclr.enable = gamma_attr.enable;
		gamma_attr_sclr.pre_osd = gamma_attr.osd_apply;

		for (i = 0; i < DISP_GAMMA_NODE; ++i) {
			gamma_attr_sclr.table[i] = gamma_attr.value[i];
		}

		disp_gamma_ctrl(dev, gamma_attr_sclr.enable, gamma_attr_sclr.pre_osd);
		disp_gamma_lut_update(dev, gamma_attr_sclr.table, gamma_attr_sclr.table, gamma_attr_sclr.table);
	}
	break;

	case VO_IOCTL_GAMMA_LUT_READ: {
		int i = 0;
		vo_gamma_info_s gamma_attr;
		struct disp_gamma_attr gamma_attr_sclr;
		vo_dev dev = p->reserved[0];

		disp_gamma_lut_read(dev, &gamma_attr_sclr);

		gamma_attr.enable = gamma_attr_sclr.enable;
		gamma_attr.osd_apply = gamma_attr_sclr.pre_osd;
		gamma_attr.dev = dev;

		for (i = 0; i < DISP_GAMMA_NODE; ++i) {
			gamma_attr.value[i] = gamma_attr_sclr.table[i];
		}

		osal_memcpy((void *)p->ptr, &gamma_attr, sizeof(vo_gamma_info_s));
	}
	break;

	case VO_IOCTL_PATTERN: {
		vo_layer dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = OSAL_EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		if (p->value >= VO_PAT_MAX) {
			TRACE_VO(DBG_ERR, "invalid disp-pattern(%d)\n", p->value);
			rc = OSAL_EINVAL;
			break;
		}

		disp_set_pattern(dev, patterns[p->value].type, patterns[p->value].color,
				 patterns[p->value].rgb);

		if (!disp_check_tgen_enable(dev))
			disp_tgen_enable(dev, true);
	}
	break;

	default:
		break;
	}

	return rc;
}

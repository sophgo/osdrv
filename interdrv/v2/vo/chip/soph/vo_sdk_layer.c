#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/cvi_buffer.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_defines.h>
#include <vb.h>
#include "sys.h"
#include "ion.h"
#include "vbq.h"
#include "base_common.h"
#include "disp.h"
#include "dsi_phy.h"
#include "vo.h"
#include "vo_sdk_layer.h"

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

VO_SYNC_INFO_S stSyncInfo[VO_OUTPUT_BUTT] = {
	[VO_OUTPUT_800x600_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 600, .u16Vbb = 24, .u16Vfb = 1
		, .u16Hact = 800, .u16Hbb = 88, .u16Hfb = 40
		, .u16Vpw = 4, .u16Hpw = 128, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P24] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 24
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 638
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P25] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 25
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 528
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P30] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 30
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 720, .u16Vbb = 20, .u16Vfb = 5
		, .u16Hact = 1280, .u16Hbb = 220, .u16Hfb = 440
		, .u16Vpw = 5, .u16Hpw = 40, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 720, .u16Vbb = 20, .u16Vfb = 5
		, .u16Hact = 1280, .u16Hbb = 220, .u16Hfb = 110
		, .u16Vpw = 5, .u16Hpw = 40, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 528
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_576P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 576, .u16Vbb = 39, .u16Vfb = 5
		, .u16Hact = 720, .u16Hbb = 68, .u16Hfb = 12
		, .u16Vpw = 5, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_480P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 480, .u16Vbb = 30, .u16Vfb = 9
		, .u16Hact = 720, .u16Hbb = 60, .u16Hfb = 16
		, .u16Vpw = 6, .u16Hpw = 62, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720x1280_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1280, .u16Vbb = 4, .u16Vfb = 6
		, .u16Hact = 720, .u16Hbb = 36, .u16Hfb = 128
		, .u16Vpw = 16, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_1080x1920_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1920, .u16Vbb = 36, .u16Vfb = 6
		, .u16Hact = 1080, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 16, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_480x800_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 800, .u16Vbb = 20, .u16Vfb = 20
		, .u16Hact = 480, .u16Hbb = 50, .u16Hfb = 50
		, .u16Vpw = 10, .u16Hpw = 10, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_1440P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1440, .u16Vbb = 33, .u16Vfb = 3
		, .u16Hact = 2560, .u16Hbb = 80, .u16Hfb = 32
		, .u16Vpw = 5, .u16Hpw = 48, .bIdv = 0, .bIhs = 1, .bIvs = 0},
	[VO_OUTPUT_2160P24] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 24
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 3840, .u16Hbb = 296, .u16Hfb = 1276
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 0},
	[VO_OUTPUT_2160P25] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 25
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 3840, .u16Hbb = 296, .u16Hfb = 1056
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 0},
	[VO_OUTPUT_2160P30] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 30
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 3840, .u16Hbb = 296, .u16Hfb = 176
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 0},
	[VO_OUTPUT_2160P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 3840, .u16Hbb = 296, .u16Hfb = 1056
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 0},
	[VO_OUTPUT_2160P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 3840, .u16Hbb = 296, .u16Hfb = 176
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 0},
	[VO_OUTPUT_4096x2160P24] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 24
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 4096, .u16Hbb = 296, .u16Hfb = 1020
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 1},
	[VO_OUTPUT_4096x2160P25] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 25
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 4096, .u16Hbb = 128, .u16Hfb = 968
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 1},
	[VO_OUTPUT_4096x2160P30] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 30
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 4096, .u16Hbb = 128, .u16Hfb = 88
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 1},
	[VO_OUTPUT_4096x2160P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 4096, .u16Hbb = 128, .u16Hfb = 968
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 1},
	[VO_OUTPUT_4096x2160P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 2160, .u16Vbb = 72, .u16Vfb = 8
		, .u16Hact = 4096, .u16Hbb = 128, .u16Hfb = 88
		, .u16Vpw = 10, .u16Hpw = 88, .bIdv = 0, .bIhs = 1, .bIvs = 1},
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
static s32 vo_get_panelstatus(VO_DEV VoDev, u32 *is_init)
{
	if (disp_mux_get(VoDev) == DISP_VO_SEL_I80) {
		*is_init = disp_check_i80_enable(VoDev);
	} else {
		*is_init = disp_check_tgen_enable(VoDev);
	}

	return CVI_SUCCESS;
}

struct vo_fmt *vo_sdk_get_format(u32 pixelformat)
{
	struct vo_fmt *fmt;
	u32 k;

	for (k = 0; k < ARRAY_SIZE(vo_sdk_formats); k++) {
		fmt = &vo_sdk_formats[k];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}

	return NULL;
}

static void _vo_sdk_fill_disp_cfg(struct disp_cfg *cfg,
		struct vo_video_format_mplane *mp)
{
	struct vo_fmt *fmt;
	struct vo_video_plane_pix_format *pfmt = mp->plane_fmt;

	fmt = vo_sdk_get_format(mp->pixelformat);

	cfg->fmt = fmt->fmt;

	if (mp->colorspace == VO_COLORSPACE_SRGB)
		cfg->in_csc = DISP_CSC_NONE;
	else if (mp->colorspace == VO_COLORSPACE_SMPTE170M)
		cfg->in_csc = DISP_CSC_601_FULL_YUV2RGB;
	else
		cfg->in_csc = DISP_CSC_709_FULL_YUV2RGB;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "bytesperline 0(%d))\n", pfmt[0].bytesperline);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "bytesperline 1(%d))\n", pfmt[1].bytesperline);
	cfg->mem.pitch_y = pfmt[0].bytesperline;
	cfg->mem.pitch_c = pfmt[1].bytesperline;

	CVI_TRACE_VO(CVI_DBG_DEBUG, " width(%d), heigh(%d)\n", mp->width, mp->height);
	cfg->mem.width = mp->width;
	cfg->mem.height = mp->height;
	cfg->mem.start_x = 0;
	cfg->mem.start_y = 0;
}

static s32 _vo_sdk_setfmt(s32 width, s32 height, u32 pxlfmt, VO_DEV VoDev)
{
	s32 p = 0;
	u8 align = 0;
	struct vo_video_format_mplane *mp;
	struct vo_video_format fmt;
	const struct vo_fmt *_vo_fmt;
	struct disp_cfg *cfg;
	u32 bytesperline;

	memset(&fmt, 0, sizeof(struct vo_video_format));

	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = pxlfmt;
	fmt.fmt.pix_mp.field = 0;

	if (align < VIP_ALIGNMENT)
		align = VIP_ALIGNMENT;

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
		fmt.fmt.pix_mp.colorspace = VO_COLORSPACE_SMPTE170M;
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
		fmt.fmt.pix_mp.num_planes = 3;
		break;
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		fmt.fmt.pix_mp.num_planes = 2;
		break;
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:
		fmt.fmt.pix_mp.num_planes = 1;
		break;
	}

	_vo_fmt = vo_sdk_get_format(pxlfmt);
	mp = &fmt.fmt.pix_mp;

	for (p = 0; p < mp->num_planes; p++) {
		u8 plane_sub_v = (p == 0) ? 1 : _vo_fmt->plane_sub_v;
		/* Calculate the minimum supported bytesperline value */
		bytesperline = ALIGN((mp->width * _vo_fmt->bit_depth[p]) >> 3, align);

		if (fmt.fmt.pix_mp.plane_fmt[p].bytesperline < bytesperline)
			fmt.fmt.pix_mp.plane_fmt[p].bytesperline = bytesperline;

		fmt.fmt.pix_mp.plane_fmt[p].sizeimage = fmt.fmt.pix_mp.plane_fmt[p].bytesperline
		* mp->height / plane_sub_v;

		CVI_TRACE_VO(CVI_DBG_DEBUG, "plane-%d: bytesperline(%d) sizeimage(%x)\n", p,
			fmt.fmt.pix_mp.plane_fmt[p].bytesperline, fmt.fmt.pix_mp.plane_fmt[p].sizeimage);
		memset(fmt.fmt.pix_mp.plane_fmt[p].reserved, 0, sizeof(fmt.fmt.pix_mp.plane_fmt[p].reserved));
	}

	cfg = disp_get_cfg(VoDev);
	_vo_sdk_fill_disp_cfg(cfg, mp);
	disp_set_bw_cfg(VoDev, cfg->fmt);
	disp_set_cfg(VoDev, cfg);

	return fmt.fmt.pix.sizeimage;
}

static void _release_buffer(struct cvi_vo_layer_ctx *pstLayerCtx, struct list_head *head)
{
	unsigned long flags;
	struct cvi_disp_buffer *b = NULL;

	while (!list_empty(head)) {
		spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
		b = list_first_entry(head,
			struct cvi_disp_buffer, list);
		list_del_init(&b->list);
		spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);

		if (b == NULL)
			return;

		if (b->blk != VB_INVALID_HANDLE)
			vb_release_block(b->blk);
		if (b->blk_i80 != VB_INVALID_HANDLE)
			vb_release_block(b->blk_i80);

		CVI_TRACE_VO(CVI_DBG_DEBUG, "relase vb(0x%llx).\n", b->buf.planes[0].addr);
		vfree(b);
		b = NULL;
	}

}

/****************************************************************************
 * SDK device APIs
 ****************************************************************************/
s32 vo_set_pub_attr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	struct cvi_disp_intf_cfg cfg;
	struct vo_dv_timings dv_timings;
	u16 rgb[3], i;
	u32  panel_status = 0;
	s32 ret = CVI_FAILURE;
	struct cvi_vo_dev_ctx *pstDevCtx;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	if (pstDevCtx->is_dev_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) should be disabled.\n", VoDev);
		return CVI_ERR_VO_DEV_HAS_ENABLED;
	}

	memset(&cfg, 0, sizeof(cfg));

	if (pstPubAttr->enIntfSync == VO_OUTPUT_USER) {
		//vo_get_dv_timings(d->fd, &dv_timings);
		dv_timings.bt.interlaced = !pstPubAttr->stSyncInfo.bIop;
		dv_timings.bt.height = pstPubAttr->stSyncInfo.u16Vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = pstPubAttr->stSyncInfo.u16Vbb;
		dv_timings.bt.vfrontporch = pstPubAttr->stSyncInfo.u16Vfb;
		dv_timings.bt.width = pstPubAttr->stSyncInfo.u16Hact;
		dv_timings.bt.hbackporch = pstPubAttr->stSyncInfo.u16Hbb;
		dv_timings.bt.hfrontporch = pstPubAttr->stSyncInfo.u16Hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = pstPubAttr->stSyncInfo.u16Hpw;
		dv_timings.bt.vsync = pstPubAttr->stSyncInfo.u16Vpw;
		dv_timings.bt.polarities = ((pstPubAttr->stSyncInfo.bIvs) ? 0 : 0x1)
					| ((pstPubAttr->stSyncInfo.bIhs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = pstPubAttr->stSyncInfo.u16FrameRate
					* (dv_timings.bt.vbackporch + dv_timings.bt.height
					   + dv_timings.bt.vfrontporch + dv_timings.bt.vsync)
					* (dv_timings.bt.hbackporch + dv_timings.bt.width
					   + dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
	} else if (pstPubAttr->enIntfSync < VO_OUTPUT_USER) {
		dv_timings.bt.interlaced = !stSyncInfo[pstPubAttr->enIntfSync].bIop;
		dv_timings.bt.height = stSyncInfo[pstPubAttr->enIntfSync].u16Vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = stSyncInfo[pstPubAttr->enIntfSync].u16Vbb;
		dv_timings.bt.vfrontporch = stSyncInfo[pstPubAttr->enIntfSync].u16Vfb;
		dv_timings.bt.width = stSyncInfo[pstPubAttr->enIntfSync].u16Hact;
		dv_timings.bt.hbackporch = stSyncInfo[pstPubAttr->enIntfSync].u16Hbb;
		dv_timings.bt.hfrontporch = stSyncInfo[pstPubAttr->enIntfSync].u16Hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = stSyncInfo[pstPubAttr->enIntfSync].u16Hpw;
		dv_timings.bt.vsync = stSyncInfo[pstPubAttr->enIntfSync].u16Vpw;
		dv_timings.bt.polarities = ((stSyncInfo[pstPubAttr->enIntfSync].bIvs) ? 0 : 0x1)
					| ((stSyncInfo[pstPubAttr->enIntfSync].bIhs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = stSyncInfo[pstPubAttr->enIntfSync].u16FrameRate
					* (dv_timings.bt.vbackporch + dv_timings.bt.height
					   + dv_timings.bt.vfrontporch + dv_timings.bt.vsync)
					* (dv_timings.bt.hbackporch + dv_timings.bt.width
					   + dv_timings.bt.hfrontporch + dv_timings.bt.hsync);

		pstPubAttr->stSyncInfo = stSyncInfo[pstPubAttr->enIntfSync];

	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO Sync Info(%d) invalid.\n", pstPubAttr->enIntfSync);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (dv_timings.bt.interlaced) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO not support interlaced timing.\n");
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	if ((dv_timings.bt.pixelclock == 0) || (dv_timings.bt.height == 0) || (dv_timings.bt.width == 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO Sync timing invalid. width(%d) height(%d) pixelclock(%llu)\n"
			, dv_timings.bt.width, dv_timings.bt.height, dv_timings.bt.pixelclock);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstPubAttr->enIntfType >= VO_INTF_LCD_18BIT) && (pstPubAttr->enIntfType <= VO_INTF_LCD_30BIT)) {
		cfg.intf_type = CVI_VIP_DISP_INTF_LVDS;
		if (pstPubAttr->enIntfType == VO_INTF_LCD_18BIT)
			cfg.lvds_cfg.out_bits = LVDS_OUT_6BIT;
		else if (pstPubAttr->enIntfType == VO_INTF_LCD_24BIT)
			cfg.lvds_cfg.out_bits = LVDS_OUT_8BIT;
		else if (pstPubAttr->enIntfType == VO_INTF_LCD_30BIT)
			cfg.lvds_cfg.out_bits = LVDS_OUT_10BIT;
		else
			cfg.lvds_cfg.out_bits = LVDS_OUT_8BIT;

		cfg.lvds_cfg.mode = (enum LVDS_MODE)pstPubAttr->stLvdsAttr.lvds_vesa_mode;
		cfg.lvds_cfg.chn_num = pstPubAttr->stLvdsAttr.chn_num;
		if (cfg.lvds_cfg.chn_num > 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "lvds only surpports single link!\n");
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		cfg.lvds_cfg.vs_out_en = 1;
		cfg.lvds_cfg.hs_out_en = 1;
		cfg.lvds_cfg.hs_blk_en = 1;
		cfg.lvds_cfg.msb_lsb_data_swap = 1;
		cfg.lvds_cfg.serial_msb_first = pstPubAttr->stLvdsAttr.data_big_endian;
		cfg.lvds_cfg.even_odd_link_swap = 0;
		cfg.lvds_cfg.enable = 1;
		do_div(dv_timings.bt.pixelclock, 1000);
		cfg.lvds_cfg.pixelclock = dv_timings.bt.pixelclock;
		for (i = 0; i < VO_LVDS_LANE_MAX; ++i) {
			cfg.lvds_cfg.lane_id[i] = pstPubAttr->stLvdsAttr.lane_id[i];
			cfg.lvds_cfg.lane_pn_swap[i] = pstPubAttr->stLvdsAttr.lane_pn_swap[i];
		}

		if (vo_set_interface(VoDev, &cfg) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO INTF configure failured.\n");
			return CVI_FAILURE;
		}
	} else if ((pstPubAttr->enIntfType == VO_INTF_MIPI) || (pstPubAttr->enIntfType == VO_INTF_MIPI_SLAVE)) {
		cfg.intf_type = CVI_VIP_DISP_INTF_DSI;
		CVI_TRACE_VO(CVI_DBG_DEBUG, "MIPI-DSI should be setup by mipi-tx.\n");
	} else if (pstPubAttr->enIntfType == VO_INTF_HDMI) {
		if (VoDev != VO_HDMI_DEVICE) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Only device 1 has hdmi interface!\n");
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		cfg.intf_type = CVI_VIP_DISP_INTF_HDMI;
		CVI_TRACE_VO(CVI_DBG_DEBUG, "HDMI should be setup by hdmi-tx.\n");
	} else if (pstPubAttr->enIntfType == VO_INTF_I80) {
		const VO_I80_CFG_S *psti80Cfg = &pstPubAttr->sti80Cfg;

		if ((psti80Cfg->lane_s.CS > 3) || (psti80Cfg->lane_s.RS > 3) ||
			(psti80Cfg->lane_s.WR > 3) || (psti80Cfg->lane_s.RD > 3)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) I80 lane should be less than 3.\n", VoDev);
			CVI_TRACE_VO(CVI_DBG_ERR, "CS(%d) RS(%d) WR(%d) RD(%d).\n",
					 psti80Cfg->lane_s.CS, psti80Cfg->lane_s.RS,
					 psti80Cfg->lane_s.WR, psti80Cfg->lane_s.RD);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if ((psti80Cfg->lane_s.CS == psti80Cfg->lane_s.RS) || (psti80Cfg->lane_s.CS == psti80Cfg->lane_s.WR) ||
			(psti80Cfg->lane_s.CS == psti80Cfg->lane_s.RD) || (psti80Cfg->lane_s.RS == psti80Cfg->lane_s.WR) ||
			(psti80Cfg->lane_s.CS == psti80Cfg->lane_s.RD) || (psti80Cfg->lane_s.WR == psti80Cfg->lane_s.RD)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) I80 lane can't duplicate CS(%d) RS(%d) WR(%d) RD(%d).\n",
					 VoDev, psti80Cfg->lane_s.CS, psti80Cfg->lane_s.RS,
					 psti80Cfg->lane_s.WR, psti80Cfg->lane_s.RD);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if (psti80Cfg->cycle_time > 250) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) cycle time %d > 250.\n",
					 VoDev, psti80Cfg->cycle_time);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if (psti80Cfg->fmt >= VO_I80_FORMAT_MAX) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) invalid I80 Format(%d).\n",
					 VoDev, psti80Cfg->fmt);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
#if 0 //TODO: I80
		cfg.intf_type = CVI_VIP_DISP_INTF_I80;
		if (vo_set_interface(gvdev, &cfg, VoDev) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO INTF configure failured.\n");
			//return CVI_FAILURE;
		}

		i80_ctrl[I80_CTRL_CMD] = BIT(psti80Cfg->lane_s.RD) |
					((BIT(psti80Cfg->lane_s.RD) | BIT(psti80Cfg->lane_s.WR)) << 4);
		i80_ctrl[I80_CTRL_DATA] = (BIT(psti80Cfg->lane_s.RD) | BIT(psti80Cfg->lane_s.RS)) |
			((BIT(psti80Cfg->lane_s.RD) | BIT(psti80Cfg->lane_s.WR) | BIT(psti80Cfg->lane_s.RS)) << 4);
		i80_ctrl[I80_CTRL_EOF] = 0xff;

		CVI_TRACE_VO(CVI_DBG_ERR, "VO I80 ctrl CMD(%#x) DATA(%#x)\n",
				 i80_ctrl[I80_CTRL_CMD], i80_ctrl[I80_CTRL_DATA]);



		d = get_dev_info(VDEV_TYPE_DISP, 0);
		if (vo_set_clk(d->fd, 1000000 / (psti80Cfg->cycle_time / 2)) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO I80 update cycle_time(%d) fail\n", psti80Cfg->cycle_time);
			return CVI_FAILURE;
		}
#endif
	} else if (pstPubAttr->enIntfType == VO_INTF_BT656) {
		cfg.intf_type = CVI_VIP_DISP_INTF_BT;
		cfg.bt_cfg.mode = BT_MODE_656;

		if (vo_set_interface(VoDev, &cfg) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO BT656 configure failured.\n");
			return CVI_FAILURE;
		}
	} else if (pstPubAttr->enIntfType == VO_INTF_BT1120) {
		cfg.intf_type = CVI_VIP_DISP_INTF_BT;
		cfg.bt_cfg.mode = BT_MODE_1120;

		if (vo_set_interface(VoDev, &cfg) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO BT1120 configure failured.\n");
			return CVI_FAILURE;
		}
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO invalid INTF type(0x%x)\n", pstPubAttr->enIntfType);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	vo_get_panelstatus(VoDev, &panel_status);

	CVI_TRACE_VO(CVI_DBG_INFO, "panel_status[%d], intf_type[%d]\n", panel_status, cfg.intf_type);
	if ((cfg.intf_type != CVI_VIP_DISP_INTF_DSI) && (cfg.intf_type != CVI_VIP_DISP_INTF_HDMI) && !panel_status) {
		struct disp_timing timing;

		vo_fill_disp_timing(&timing, &dv_timings.bt);
		disp_set_timing(VoDev, &timing);
	}

	rgb[2] = pstPubAttr->u32BgColor & 0x3ff;
	rgb[1] = (pstPubAttr->u32BgColor >> 10) & 0x3ff;
	rgb[0] = (pstPubAttr->u32BgColor >> 20) & 0x3ff;

	disp_set_frame_bgcolor(VoDev, rgb[0], rgb[1], rgb[2]);

	memcpy(&pstDevCtx->stPubAttr, pstPubAttr, sizeof(*pstPubAttr));

	return CVI_SUCCESS;
}

s32 vo_get_pub_attr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	enum disp_vo_sel vo_sel;
	struct disp_timing *timing = disp_get_timing(VoDev);
	s32 ret = CVI_FAILURE;
	struct cvi_vo_dev_ctx *pstDevCtx;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	pstDevCtx->stPubAttr.stSyncInfo.u16Hact = timing->hfde_end - timing->hfde_start + 1;
	pstDevCtx->stPubAttr.stSyncInfo.u16Vact = timing->vfde_end - timing->vfde_start + 1;

	vo_sel = disp_mux_get(VoDev);

	switch (vo_sel) {
	case DISP_VO_SEL_I80:
		pstDevCtx->stPubAttr.enIntfType = VO_INTF_I80;
		break;

	case DISP_VO_SEL_BT656:
		pstDevCtx->stPubAttr.enIntfType = VO_INTF_BT656;
		break;

	case DISP_VO_SEL_BT1120:
		pstDevCtx->stPubAttr.enIntfType = VO_INTF_BT1120;
		break;

	default:
		if (dphy_get_dsi_clk_lane_status(VoDev)) {
			if (dphy_is_lvds(VoDev))
				pstDevCtx->stPubAttr.enIntfType = VO_INTF_LCD;
			else
				pstDevCtx->stPubAttr.enIntfType = VO_INTF_MIPI;
		} else {
			pstDevCtx->stPubAttr.enIntfType = VO_INTF_HDMI;
		}
		break;
	}

	memcpy(pstPubAttr, &pstDevCtx->stPubAttr, sizeof(VO_PUB_ATTR_S));

	return CVI_SUCCESS;

}

s32 vo_set_hdmi_param(VO_DEV VoDev, VO_HDMI_PARAM_S *pstHDMIParam)
{
	struct cvi_vo_dev_ctx *pstDevCtx;

	if (VoDev != VO_HDMI_DEVICE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Only device 1 has hdmi interface!\n");
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	if (pstDevCtx->stPubAttr.enIntfType != VO_INTF_HDMI) {
		CVI_TRACE_VO(CVI_DBG_ERR, "not working under th hdmi interface!\n");
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (((pstHDMIParam->stHDMICSC.enCscMatrix >= VO_CSC_MATRIX_601_LIMIT_RGB2YUV)
		&& (pstHDMIParam->stHDMICSC.enCscMatrix <= VO_CSC_MATRIX_709_FULL_RGB2YUV))
		|| (pstHDMIParam->stHDMICSC.enCscMatrix == VO_CSC_MATRIX_IDENTITY)) {
		disp_set_out_csc(VoDev, pstHDMIParam->stHDMICSC.enCscMatrix);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) HDMI CscMatrix(%d) invalid.\n", VoDev, pstHDMIParam->stHDMICSC.enCscMatrix);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

s32 vo_get_hdmi_param(VO_DEV VoDev, VO_HDMI_PARAM_S *pstHDMIParam)
{
	struct disp_cfg *disp_cfg;
	struct cvi_vo_dev_ctx *pstDevCtx;

	if (VoDev != VO_HDMI_DEVICE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Only device 1 has hdmi interface!\n");
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	if (pstDevCtx->stPubAttr.enIntfType != VO_INTF_HDMI) {
		CVI_TRACE_VO(CVI_DBG_ERR, "not working under th hdmi interface!\n");
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	disp_cfg = disp_get_cfg(VoDev);
	pstHDMIParam->stHDMICSC.enCscMatrix = disp_cfg->out_csc;

	return CVI_SUCCESS;
}

s32 vo_enable(VO_DEV VoDev)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_dev_ctx *pstDevCtx;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	pstDevCtx = &gVoCtx->astDevCtx[VoDev];
	if (pstDevCtx->stPubAttr.enIntfType == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", VoDev);
		return CVI_ERR_VO_DEV_NOT_CONFIG;
	}
	if (pstDevCtx->s32BindLayerId == -1) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "VoDev(%d) unbind layer", VoDev);
		return CVI_ERR_VO_SYS_NOTREADY;
	}

	if (pstDevCtx->is_dev_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) should be disabled.\n", VoDev);
		return CVI_ERR_VO_DEV_HAS_ENABLED;
	}

	pstDevCtx->is_dev_enable = CVI_TRUE;

	if (vo_start_streaming(VoDev)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo start streaming\n");
		return CVI_ERR_VO_SYS_NOTREADY;
	}

	return CVI_SUCCESS;

}

s32 vo_disable(VO_DEV VoDev)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_dev_ctx *pstDevCtx;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	pstDevCtx = &gVoCtx->astDevCtx[VoDev];
	if (!pstDevCtx->is_dev_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO_DEV(%d) already disabled.\n", VoDev);
		return CVI_SUCCESS;
	}

	pstDevCtx->is_dev_enable = CVI_FALSE;

	if (vo_stop_streaming(VoDev)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo stop streaming\n");
		return CVI_ERR_VO_SYS_NOTREADY;
	}

	return CVI_SUCCESS;
}


/****************************************************************************
 * SDK layer APIs
 ****************************************************************************/
s32 vo_set_displaybuflen(VO_LAYER VoLayer, u32 u32BufLen)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].u32DisBufLen = u32BufLen;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return CVI_SUCCESS;
}

s32 vo_get_displaybuflen(VO_LAYER VoLayer, u32 *pu32BufLen)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	*pu32BufLen = gVoCtx->astLayerCtx[VoLayer].u32DisBufLen;

	return CVI_SUCCESS;
}

s32 vo_enablevideolayer(VO_LAYER VoLayer)
{
	s32 ret = CVI_FAILURE;
	s32 i;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	VB_CAL_CONFIG_S stVbCalConfig;
	VB_BLK blk = VB_INVALID_HANDLE;
	struct cvi_disp_buffer *pstDispBuf;
	unsigned long flags;
	VO_DEV VoDev;
	struct vb_s *vb;
	MMF_CHN_S chn;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_DISABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (pstLayerCtx->s32BindDevId == -1) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "VoLayer(%d) unbind device", VoLayer);
		return CVI_ERR_VO_SYS_NOTREADY;
	}

	VoDev = pstLayerCtx->s32BindDevId;
	chn.enModId = CVI_ID_VO;
	chn.s32DevId = VoDev;
	chn.s32ChnId = 0;

	COMMON_GetPicBufferConfig(pstLayerCtx->stLayerAttr.stImageSize.u32Width,
		pstLayerCtx->stLayerAttr.stImageSize.u32Height,
		pstLayerCtx->stLayerAttr.enPixFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

	base_mod_jobs_init(&pstLayerCtx->layer_jobs, 0, 0, pstLayerCtx->stLayerAttr.u32Depth);

	for (i = 0; i < pstLayerCtx->u32DisBufLen; i++) {
		blk = vb_get_block_with_id(VB_INVALID_POOLID, stVbCalConfig.u32VBSize, CVI_ID_VO);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VO(CVI_DBG_ERR, "get vb block fail.\n");
			ret = CVI_ERR_VO_NO_MEM;
			goto err;
		}

		vb = (struct vb_s *)blk;
		base_get_frame_info(pstLayerCtx->stLayerAttr.enPixFormat
					, pstLayerCtx->stLayerAttr.stImageSize
					, &vb->buf
					, vb_handle2phys_addr(blk)
					, DEFAULT_ALIGN);

		vb->buf.s16OffsetTop = 0;
		vb->buf.s16OffsetRight = 0;
		vb->buf.s16OffsetLeft = 0;
		vb->buf.s16OffsetBottom = 0;

		pstDispBuf = vzalloc(sizeof(struct cvi_disp_buffer));
		if (pstDispBuf == NULL) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vzalloc size(%zu) fail\n", sizeof(struct cvi_disp_buffer));
			vb_release_block(blk);
			ret = CVI_ERR_VO_NO_MEM;
			goto err;
		}

		if (gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType == VO_INTF_I80) {
			u8 byte_cnt = (gVoCtx->astDevCtx[VoDev].stPubAttr.sti80Cfg.fmt == VO_I80_FORMAT_RGB666) ? 3 : 2;
			u32 buf_size;
			VB_BLK blk_i80 = VB_INVALID_HANDLE;

			buf_size = ALIGN((pstLayerCtx->stLayerAttr.stImageSize.u32Width * byte_cnt + 1) * 3, 32) *
						pstLayerCtx->stLayerAttr.stImageSize.u32Height;
			blk_i80 = vb_get_block_with_id(VB_INVALID_POOLID, buf_size, CVI_ID_VO);
			if (blk_i80 == VB_INVALID_HANDLE) {
				CVI_TRACE_VO(CVI_DBG_INFO, "No more vb for i80 transform.\n");
				vb_release_block(blk);
				kfree(pstDispBuf);
				ret = CVI_ERR_VO_NO_MEM;
				goto err;
			}

			pstDispBuf->blk_i80 = blk_i80;
		} else {
			pstDispBuf->blk_i80 = VB_INVALID_HANDLE;
		}

		pstDispBuf->buf.length = 3;
		pstDispBuf->buf.index  = i;
		pstDispBuf->sequence = i;
		pstDispBuf->blk = blk;

		spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
		list_add_tail(&pstDispBuf->list, &pstLayerCtx->list_done);
		spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);
	}


	mutex_lock(&pstLayerCtx->layer_lock);
	pstLayerCtx->is_layer_enable = CVI_TRUE;
	for (i = 0; i < RGN_MAX_NUM_VO; ++i)
		pstLayerCtx->rgn_handle[i] = RGN_INVALID_HANDLE;
	for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
		pstLayerCtx->rgn_coverEx_handle[i] = RGN_INVALID_HANDLE;
	pstLayerCtx->event = 0;
	init_waitqueue_head(&pstLayerCtx->wq);
	mutex_unlock(&pstLayerCtx->layer_lock);

	ret = vo_create_thread(VoLayer);
	if (ret) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Failed to create thread, VoLayer(%d).\n", VoLayer);
		goto err;
	}

	return CVI_SUCCESS;

err:
	base_mod_jobs_exit(&pstLayerCtx->layer_jobs);
	_release_buffer(pstLayerCtx, &pstLayerCtx->list_done);

	return ret;
}

s32 vo_disablevideolayer(VO_LAYER VoLayer)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (!pstLayerCtx->is_layer_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't enabled yet.\n", VoLayer);
		return CVI_SUCCESS;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	pstLayerCtx->is_layer_enable = CVI_FALSE;
	pstLayerCtx->u32FrameNum = 0;
	pstLayerCtx->u32LayerFrameRate = 0;
	pstLayerCtx->u32SrcFrameNum = 0;
	pstLayerCtx->u32LayerSrcFrameRate = 0;
	pstLayerCtx->u32FrameIndex = 0;
	pstLayerCtx->u64DisplayPts = 0;
	pstLayerCtx->u64PreDonePts = 0;
	pstLayerCtx->u32BwFail = 0;
	pstLayerCtx->u32OsdBwFail = 0;
	mutex_unlock(&pstLayerCtx->layer_lock);

	vo_destroy_thread(VoLayer);

	base_mod_jobs_exit(&pstLayerCtx->layer_jobs);

	_release_buffer(pstLayerCtx, &pstLayerCtx->list_done);
	_release_buffer(pstLayerCtx, &pstLayerCtx->list_work);
	_release_buffer(pstLayerCtx, &pstLayerCtx->list_wait);

	return CVI_SUCCESS;
}

s32 vo_bind_videolayer(VO_LAYER VoLayer, VO_DEV VoDev)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_DISABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astDevCtx[VoDev].s32BindLayerId = VoLayer;
	gVoCtx->astLayerCtx[VoLayer].s32BindDevId = VoDev;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return ret;
}

s32 vo_unbind_videolayer(VO_LAYER VoLayer, VO_DEV VoDev)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_DISABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astDevCtx[VoDev].s32BindLayerId = -1;
	gVoCtx->astLayerCtx[VoLayer].s32BindDevId = -1;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return ret;
}

s32 vo_get_videolayerattr(VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstLayerAttr, &gVoCtx->astLayerCtx[VoLayer].stLayerAttr, sizeof(*pstLayerAttr));

	return CVI_SUCCESS;
}

s32 vo_set_videolayerattr(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	struct disp_rect rect;
	u16 rgb[3] = {0, 0, 0};
	s32 ret = CVI_FAILURE;
	VO_DEV VoDev;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_DISABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (!VO_SUPPORT_FMT(pstLayerAttr->enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "VoLayer(%d) enPixFormat(%d) unsupported\n"
			, VoLayer, pstLayerAttr->enPixFormat);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	VoDev = pstLayerCtx->s32BindDevId;
	if (VoDev == -1) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "VoLayer(%d) unbind device", VoLayer);
		return CVI_ERR_VO_SYS_NOTREADY;
	}

	if (gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType == VO_INTF_I80)
		if ((pstLayerAttr->enPixFormat != PIXEL_FORMAT_RGB_888)
		 && (pstLayerAttr->enPixFormat != PIXEL_FORMAT_BGR_888)
		 && (pstLayerAttr->enPixFormat != PIXEL_FORMAT_RGB_888_PLANAR)
		 && (pstLayerAttr->enPixFormat != PIXEL_FORMAT_BGR_888_PLANAR)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "I80 only accept RGB/BGR pixel format.\n");
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}

	if ((pstLayerAttr->stImageSize.u32Width != pstLayerAttr->stDispRect.u32Width)
	 || (pstLayerAttr->stImageSize.u32Height != pstLayerAttr->stDispRect.u32Height)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) stImageSize(%d %d) stDispRect(%d %d) isn't the same.\n"
			, VoLayer, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height
			, pstLayerAttr->stDispRect.u32Width, pstLayerAttr->stDispRect.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstLayerAttr->stImageSize.u32Width < VO_MIN_CHN_WIDTH)
	 || (pstLayerAttr->stImageSize.u32Height < VO_MIN_CHN_HEIGHT)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Size(%d %d) too small.\n"
			, VoLayer, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (pstLayerAttr->u32Depth > VO_MAX_LAYER_DEPTH) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Depth(%d) invalid.\n"
			, VoLayer, pstLayerAttr->u32Depth);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType != VO_INTF_I80) {
		_vo_sdk_setfmt(pstLayerAttr->stImageSize.u32Width,
					pstLayerAttr->stImageSize.u32Height, pstLayerAttr->enPixFormat, VoDev);
	}

	disp_set_window_bgcolor(VoDev, rgb[0], rgb[1], rgb[2]);

	rect.w = pstLayerAttr->stDispRect.u32Width;
	rect.h = pstLayerAttr->stDispRect.u32Height;
	rect.x = pstLayerAttr->stDispRect.s32X;
	rect.y = pstLayerAttr->stDispRect.s32Y;

	//vo_set_tgt_compose(d->fd, &area);
	disp_set_rect(VoDev, rect);

	mutex_lock(&pstLayerCtx->layer_lock);
	pstLayerCtx->stLayerAttr.stDispRect = pstLayerAttr->stDispRect;
	pstLayerCtx->stLayerAttr.stImageSize = pstLayerAttr->stImageSize;
	pstLayerCtx->stLayerAttr.u32DispFrmRt = pstLayerAttr->u32DispFrmRt;
	pstLayerCtx->stLayerAttr.enPixFormat = pstLayerAttr->enPixFormat;
	if (!pstLayerCtx->is_layer_enable)
		pstLayerCtx->stLayerAttr.u32Depth = pstLayerAttr->u32Depth;
	mutex_unlock(&pstLayerCtx->layer_lock);

	CVI_TRACE_VO(CVI_DBG_DEBUG, "VoLayer(%d) image-size(%d * %d) disp-rect(%d-%d-%d-%d).\n", VoLayer
		, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height
		, pstLayerAttr->stDispRect.s32X, pstLayerAttr->stDispRect.s32Y
		, pstLayerAttr->stDispRect.u32Width, pstLayerAttr->stDispRect.u32Height);

	return CVI_SUCCESS;
}

s32 vo_get_layer_proc_amp(VO_LAYER VoLayer, s32 *proc_amp)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	memcpy(proc_amp, pstLayerCtx->proc_amp, sizeof(pstLayerCtx->proc_amp));

	return CVI_SUCCESS;
}

s32 vo_set_layer_proc_amp(VO_LAYER VoLayer, const s32 *proc_amp)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (!IS_FMT_YUV(pstLayerCtx->stLayerAttr.enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Only YUV format support.\n", VoLayer);
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	memcpy(pstLayerCtx->proc_amp, proc_amp, sizeof(pstLayerCtx->proc_amp));
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

s32 vo_set_layer_csc(VO_LAYER VoLayer, VO_CSC_S stVideoCSC)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	VO_DEV VoDev;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	VoDev = pstLayerCtx->s32BindDevId;

	if (!IS_FMT_YUV(pstLayerCtx->stLayerAttr.enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Only YUV format support set csc.\n", VoLayer);
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (stVideoCSC.enCscMatrix >= VO_CSC_MATRIX_601_LIMIT_YUV2RGB
		&& stVideoCSC.enCscMatrix <= VO_CSC_MATRIX_709_FULL_YUV2RGB) {
		disp_set_in_csc(VoDev, stVideoCSC.enCscMatrix);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) CscMatrix(%d) invalid.\n", VoLayer, stVideoCSC.enCscMatrix);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

s32 vo_get_layer_csc(VO_LAYER VoLayer, VO_CSC_S *pstVideoCSC)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct disp_cfg *disp_cfg;
	VO_DEV VoDev;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	VoDev = pstLayerCtx->s32BindDevId;
	disp_cfg = disp_get_cfg(VoDev);
	pstVideoCSC->enCscMatrix = disp_cfg->in_csc;

	return CVI_SUCCESS;
}

s32 vo_get_screen_frame(VO_LAYER VoLayer, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	s32 ret = CVI_FAILURE;
	VB_BLK blk;
	VO_DEV VoDev = 0;
	MMF_CHN_S chn;
	struct vb_s *vb;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	s32 i = 0;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	VoDev = pstLayerCtx->s32BindDevId;

	chn.enModId = CVI_ID_VO;
	chn.s32DevId = VoDev;
	chn.s32ChnId = 0;
	memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));

	ret = base_get_chn_buffer(chn, &pstLayerCtx->layer_jobs, &blk, s32MilliSec);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo get screen buf fail\n");
		return ret;
	}

	vb = (struct vb_s *)blk;

	pstVideoFrame->stVFrame.enPixelFormat = vb->buf.enPixelFormat;
	pstVideoFrame->stVFrame.u32Width = vb->buf.size.u32Width;
	pstVideoFrame->stVFrame.u32Height = vb->buf.size.u32Height;
	pstVideoFrame->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstVideoFrame->stVFrame.u64PTS = vb->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pstVideoFrame->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstVideoFrame->stVFrame.u32Length[i] = vb->buf.length[i];
		pstVideoFrame->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstVideoFrame->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstVideoFrame->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstVideoFrame->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstVideoFrame->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstVideoFrame->stVFrame.pPrivateData = vb;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
			pstVideoFrame->stVFrame.enPixelFormat, pstVideoFrame->stVFrame.u32Width,
			pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.u64PTS,
			pstVideoFrame->stVFrame.u64PhyAddr[0], pstVideoFrame->stVFrame.u64PhyAddr[1],
			pstVideoFrame->stVFrame.u64PhyAddr[2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
			pstVideoFrame->stVFrame.u32Length[0], pstVideoFrame->stVFrame.u32Length[1],
			pstVideoFrame->stVFrame.u32Length[2], pstVideoFrame->stVFrame.u32Stride[0],
			pstVideoFrame->stVFrame.u32Stride[1], pstVideoFrame->stVFrame.u32Stride[2]);

	return CVI_SUCCESS;
}

s32 vo_release_screen_frame(VO_LAYER VoLayer, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	VB_BLK blk;
	s32 ret = CVI_SUCCESS;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;


	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Invalid phy-address(%llx) in pstVideoFrame. Can't find VB_BLK.\n"
			    , pstVideoFrame->stVFrame.u64PhyAddr[0]);
		return CVI_FAILURE;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "release layer frame, addr(0x%llx)\n",
			pstVideoFrame->stVFrame.u64PhyAddr[0]);

	return CVI_SUCCESS;
}

s32 vo_set_layer_toleration(VO_LAYER VoLayer, u32 u32Toleration)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	if ((u32Toleration < VO_MIN_TOLERATE) || (u32Toleration > VO_MAX_TOLERATE)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) u32Toleration(%d) invalid.\n", VoLayer, u32Toleration);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	pstLayerCtx->u32Toleration = u32Toleration;

	return CVI_SUCCESS;
}

s32 vo_get_layer_toleration(VO_LAYER VoLayer, u32 *pu32Toleration)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	*pu32Toleration = pstLayerCtx->u32Toleration;

	return CVI_SUCCESS;
}

/****************************************************************************
 * SDK chn APIs
 ****************************************************************************/
s32 vo_clear_chnbuf(VO_LAYER VoLayer, VO_CHN VoChn, bool bClrAll)
{
	s32 ret = CVI_FAILURE;
	VB_BLK blk;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	//clear chn waitq vb
	while (!base_mod_jobs_waitq_empty(&pstChnCtx->chn_jobs)) {
		blk = base_mod_jobs_waitq_pop(&pstChnCtx->chn_jobs);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	//clear chn workq vb
	while (bClrAll && !base_mod_jobs_workq_empty(&pstChnCtx->chn_jobs)) {
		blk = base_mod_jobs_workq_pop(&pstChnCtx->chn_jobs);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	return CVI_SUCCESS;
}

s32 vo_send_frame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = VoLayer, .s32ChnId = VoChn};
	VB_BLK blk;
	//SIZE_S stSize;
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	UNUSED(s32MilliSec);

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (pstLayerCtx->stLayerAttr.enPixFormat != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) PixelFormat(%d) mismatch.\n"
			, VoLayer, VoChn, pstVideoFrame->stVFrame.enPixelFormat);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

#if 0
	if ((pstChnCtx->enRotation == ROTATION_90)
		|| (pstChnCtx->enRotation == ROTATION_270)) {
		stSize.u32Width = pstChnCtx->stChnAttr.stRect.u32Height;
		stSize.u32Height = pstChnCtx->stChnAttr.stRect.u32Width;
	} else {
		stSize.u32Width = pstChnCtx->stChnAttr.stRect.u32Width;
		stSize.u32Height = pstChnCtx->stChnAttr.stRect.u32Height;
	}

	if ((stSize.u32Width != (pstVideoFrame->stVFrame.u32Width -
		pstVideoFrame->stVFrame.s16OffsetLeft - pstVideoFrame->stVFrame.s16OffsetRight))
	 || (stSize.u32Height != (pstVideoFrame->stVFrame.u32Height -
		pstVideoFrame->stVFrame.s16OffsetTop - pstVideoFrame->stVFrame.s16OffsetBottom))) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Size(%d * %d) frame width(%d %d %d) height(%d %d %d)mismatch.\n"
			, VoLayer, VoChn, stSize.u32Width, stSize.u32Height
			, pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight
			, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetTop
			, pstVideoFrame->stVFrame.s16OffsetBottom, pstVideoFrame->stVFrame.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
#endif

	if (IS_FRAME_OFFSET_INVALID(pstVideoFrame->stVFrame)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) frame offset (%d %d %d %d) invalid\n",
			VoLayer, VoChn,
			pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight,
			pstVideoFrame->stVFrame.s16OffsetTop, pstVideoFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (IS_FMT_YUV420(pstLayerCtx->stLayerAttr.enPixFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) YUV420 can't accept odd frame valid width\n",
				VoLayer, VoChn);
			CVI_TRACE_VO(CVI_DBG_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if ((pstVideoFrame->stVFrame.u32Height - pstVideoFrame->stVFrame.s16OffsetTop -
		     pstVideoFrame->stVFrame.s16OffsetBottom) & 0x01) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) YUV420 can't accept odd frame valid height\n",
				VoLayer, VoChn);
			CVI_TRACE_VO(CVI_DBG_ERR, "u32Height(%d) s16OffsetTop(%d) s16OffsetBottom(%d)\n",
				pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.s16OffsetTop,
				pstVideoFrame->stVFrame.s16OffsetBottom);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(pstLayerCtx->stLayerAttr.enPixFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) YUV422 can't accept odd frame valid width\n",
				VoLayer, VoChn);
			CVI_TRACE_VO(CVI_DBG_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
	}

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Invalid phy-addr(%llx). Can't locate VB_BLK.\n"
			      , VoLayer, VoChn, pstVideoFrame->stVFrame.u64PhyAddr[0]);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Invalid parameter\n", VoLayer, VoChn);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	vo_recv_frame(chn, blk);

	return ret;
}

s32 vo_get_chn_attr(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	memcpy(pstChnAttr, &pstChnCtx->stChnAttr, sizeof(*pstChnAttr));

	return CVI_SUCCESS;
}

s32 vo_set_chn_attr(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_ATTR_S *pstChnAttr)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	if ((pstChnAttr->stRect.u32Width < VO_MIN_CHN_WIDTH) || (pstChnAttr->stRect.u32Height < VO_MIN_CHN_HEIGHT)
	 || (pstChnAttr->stRect.u32Width + pstChnAttr->stRect.s32X > pstLayerCtx->stLayerAttr.stImageSize.u32Width)
	 || (pstChnAttr->stRect.u32Height + pstChnAttr->stRect.s32Y > pstLayerCtx->stLayerAttr.stImageSize.u32Height)
	 || (pstChnAttr->stRect.s32X < 0) || (pstChnAttr->stRect.s32Y < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) rect(%d %d %d %d) invalid.\n"
			, VoLayer, VoChn, pstChnAttr->stRect.s32X, pstChnAttr->stRect.s32Y
			, pstChnAttr->stRect.u32Width, pstChnAttr->stRect.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (pstChnAttr->u32Depth > VO_MAX_CHN_DEPTH) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Depth(%d) too big.\n"
			, VoLayer, VoChn, pstChnAttr->u32Depth);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	pstChnCtx->stChnAttr.u32Priority = pstChnAttr->u32Priority;
	pstChnCtx->stChnAttr.stRect = pstChnAttr->stRect;
	if (!pstChnCtx->is_chn_enable)
		pstChnCtx->stChnAttr.u32Depth = pstChnAttr->u32Depth;
	//when chn rect change need disable
	memset(&pstChnCtx->stChnZoomAttr, 0, sizeof(pstChnCtx->stChnZoomAttr));
	memset(&pstChnCtx->stChnParam, 0, sizeof(pstChnCtx->stChnParam));
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

s32 vo_set_chn_param(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_PARAM_S *pstChnParam)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_chn_ctx *pstChnCtx;
	VO_DEV VoDev;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	VoDev = pstLayerCtx->s32BindDevId;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstDevCtx->stPubAttr.enIntfType == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", VoDev);
		return CVI_ERR_VO_DEV_NOT_CONFIG;
	}

	if (!pstDevCtx->is_dev_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) should be enabled.\n", VoDev);
		return CVI_ERR_VO_DEV_NOT_ENABLED;
	}

	if (pstLayerCtx->stLayerAttr.stImageSize.u32Width == 0 || pstLayerCtx->stLayerAttr.stImageSize.u32Height == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't correctly configured.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_CONFIG;
	}

	if ((pstChnParam->stAspectRatio.enMode >= ASPECT_RATIO_MAX) || (pstChnParam->stAspectRatio.enMode < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) AspectRatio Mode(%d) invalid.\n"
			, VoLayer, VoChn, pstChnParam->stAspectRatio.enMode);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (((pstChnParam->stAspectRatio.stVideoRect.s32X < 0) || (pstChnParam->stAspectRatio.stVideoRect.s32Y < 0)
		|| ((pstChnParam->stAspectRatio.stVideoRect.s32X + pstChnParam->stAspectRatio.stVideoRect.u32Width)
		> pstChnCtx->stChnAttr.stRect.u32Width) || ((pstChnParam->stAspectRatio.stVideoRect.s32Y
		+ pstChnParam->stAspectRatio.stVideoRect.u32Height) > pstChnCtx->stChnAttr.stRect.u32Height))
		&& (pstChnParam->stAspectRatio.enMode == ASPECT_RATIO_MANUAL)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) AspectRatio rect(%d %d %d %d) invalid.\n"
			, VoLayer, VoChn, pstChnParam->stAspectRatio.stVideoRect.s32X, pstChnParam->stAspectRatio.stVideoRect.s32Y
			, pstChnParam->stAspectRatio.stVideoRect.u32Width, pstChnParam->stAspectRatio.stVideoRect.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	memcpy(&pstChnCtx->stChnParam, pstChnParam, sizeof(*pstChnParam));
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

s32 vo_get_chn_param(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_PARAM_S *pstChnParam)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	memcpy(pstChnParam, &pstChnCtx->stChnParam, sizeof(*pstChnParam));
	return CVI_SUCCESS;
}

s32 vo_set_chn_zoom(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_ZOOM_ATTR_S *pstChnZoomAttr)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_chn_ctx *pstChnCtx;
	VO_DEV VoDev;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	VoDev = pstLayerCtx->s32BindDevId;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstDevCtx->stPubAttr.enIntfType == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", VoDev);
		return CVI_ERR_VO_DEV_NOT_CONFIG;
	}

	if (!pstDevCtx->is_dev_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) should be enabled.\n", VoDev);
		return CVI_ERR_VO_DEV_NOT_ENABLED;
	}

	if (pstLayerCtx->stLayerAttr.stImageSize.u32Width == 0 || pstLayerCtx->stLayerAttr.stImageSize.u32Height == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't correctly configured.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_CONFIG;
	}

	//notice : stRect/stZoomRatio is for src pic
	if (pstChnZoomAttr->enZoomType == VO_CHN_ZOOM_IN_RECT) {
		if (((pstChnZoomAttr->stRect.u32Width < VO_MIN_CHN_WIDTH) && (pstChnZoomAttr->stRect.u32Width != 0))
			|| ((pstChnZoomAttr->stRect.u32Height < VO_MIN_CHN_HEIGHT) && (pstChnZoomAttr->stRect.u32Height != 0))
			|| (pstChnZoomAttr->stRect.u32Width & 0x01) || (pstChnZoomAttr->stRect.u32Height & 0x01)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Zoom rect(%d %d %d %d) invalid.\n"
			, VoLayer, VoChn, pstChnZoomAttr->stRect.s32X, pstChnZoomAttr->stRect.s32Y,
			pstChnZoomAttr->stRect.u32Width, pstChnZoomAttr->stRect.u32Height);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if (((pstChnCtx->stChnAttr.stRect.u32Width / pstChnZoomAttr->stRect.u32Width) > VO_MAX_CHN_SCALE)
			|| ((pstChnCtx->stChnAttr.stRect.u32Height / pstChnZoomAttr->stRect.u32Height) > VO_MAX_CHN_SCALE)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Zoom rect(%d %d %d %d) More than 16x scaling.\n"
			, VoLayer, VoChn, pstChnZoomAttr->stRect.s32X, pstChnZoomAttr->stRect.s32Y,
			pstChnZoomAttr->stRect.u32Width, pstChnZoomAttr->stRect.u32Height);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
	} else if (pstChnZoomAttr->enZoomType == VO_CHN_ZOOM_IN_RATIO) {
		if ((pstChnZoomAttr->stZoomRatio.u32Xratio > VO_MAX_CHN_ZOOM) || (pstChnZoomAttr->stZoomRatio.u32Yratio > VO_MAX_CHN_ZOOM)
			|| (pstChnZoomAttr->stZoomRatio.u32HeightRatio > VO_MAX_CHN_ZOOM) || (pstChnZoomAttr->stZoomRatio.u32HeightRatio > VO_MAX_CHN_ZOOM)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Zoom Ratio(%d %d %d %d) invalid.\n"
			, VoLayer, VoChn, pstChnZoomAttr->stZoomRatio.u32Xratio, pstChnZoomAttr->stZoomRatio.u32Yratio,
			pstChnZoomAttr->stZoomRatio.u32WidthRatio, pstChnZoomAttr->stZoomRatio.u32HeightRatio);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) ZoomType(%d) invalid.\n"
			, VoLayer, VoChn, pstChnZoomAttr->enZoomType);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	memcpy(&pstChnCtx->stChnZoomAttr, pstChnZoomAttr, sizeof(*pstChnZoomAttr));
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

s32 vo_get_chn_zoom(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ZOOM_ATTR_S *pstChnZoomAttr)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	memcpy(pstChnZoomAttr, &pstChnCtx->stChnZoomAttr, sizeof(*pstChnZoomAttr));
	return CVI_SUCCESS;
}

s32 vo_set_chn_border(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_BORDER_ATTR_S *pstChnBorder)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	if (pstLayerCtx->stLayerAttr.stImageSize.u32Width == 0 || pstLayerCtx->stLayerAttr.stImageSize.u32Height == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't correctly configured.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_CONFIG;
	}

	if ((pstChnBorder->stBorder.u32TopWidth > VO_MAX_CHN_BORDER) || (pstChnBorder->stBorder.u32BottomWidth > VO_MAX_CHN_BORDER)
		|| (pstChnBorder->stBorder.u32RightWidth > VO_MAX_CHN_BORDER) || (pstChnBorder->stBorder.u32RightWidth > VO_MAX_CHN_BORDER)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Border(%d %d %d %d) invalid.\n"
		, VoLayer, VoChn, pstChnBorder->stBorder.u32TopWidth, pstChnBorder->stBorder.u32BottomWidth,
		pstChnBorder->stBorder.u32LeftWidth, pstChnBorder->stBorder.u32RightWidth);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstChnBorder->stBorder.u32TopWidth & 0x01) || (pstChnBorder->stBorder.u32BottomWidth & 0x01)
		|| (pstChnBorder->stBorder.u32LeftWidth & 0x01) || (pstChnBorder->stBorder.u32RightWidth & 0x01)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Border(%d %d %d %d) 2-byte alignment required\n"
		, VoLayer, VoChn, pstChnBorder->stBorder.u32TopWidth, pstChnBorder->stBorder.u32BottomWidth,
		pstChnBorder->stBorder.u32LeftWidth, pstChnBorder->stBorder.u32RightWidth);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	memcpy(&pstChnCtx->stChnBorder, pstChnBorder, sizeof(*pstChnBorder));
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

s32 vo_get_chn_border(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_BORDER_ATTR_S *pstChnBorder)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	memcpy(pstChnBorder, &pstChnCtx->stChnBorder, sizeof(*pstChnBorder));
	return CVI_SUCCESS;
}

s32 vo_set_chn_mirror(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_MIRROR_TYPE enChnMirror)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	if (pstLayerCtx->stLayerAttr.stImageSize.u32Width == 0 || pstLayerCtx->stLayerAttr.stImageSize.u32Height == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't correctly configured.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_CONFIG;
	}

	if ((enChnMirror >= VO_CHN_MIRROR_BUTT) || (enChnMirror < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Mirror(%d) invalid.\n"
		, VoLayer, VoChn, enChnMirror);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	pstChnCtx->enChnMirror = enChnMirror ;
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

s32 vo_get_chn_mirror(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_MIRROR_TYPE *penChnMirror)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	*penChnMirror = pstChnCtx->enChnMirror;

	return CVI_SUCCESS;
}

s32 vo_get_chn_frame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	s32 ret = CVI_FAILURE;
	VB_BLK blk;
	VO_DEV VoDev = 0;
	MMF_CHN_S chn;
	struct vb_s *vb;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_chn_ctx *pstChnCtx;
	s32 i = 0;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	VoDev = pstLayerCtx->s32BindDevId;
	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	chn.enModId = CVI_ID_VO;
	chn.s32DevId = VoDev;
	chn.s32ChnId = VoChn;
	memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));

	ret = base_get_chn_buffer(chn, &pstChnCtx->chn_jobs, &blk, s32MilliSec);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo get chn buf fail\n");
		return ret;
	}

	vb = (struct vb_s *)blk;

	pstVideoFrame->stVFrame.enPixelFormat = vb->buf.enPixelFormat;
	pstVideoFrame->stVFrame.u32Width = vb->buf.size.u32Width;
	pstVideoFrame->stVFrame.u32Height = vb->buf.size.u32Height;
	pstVideoFrame->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstVideoFrame->stVFrame.u64PTS = vb->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pstVideoFrame->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstVideoFrame->stVFrame.u32Length[i] = vb->buf.length[i];
		pstVideoFrame->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstVideoFrame->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstVideoFrame->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstVideoFrame->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstVideoFrame->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstVideoFrame->stVFrame.pPrivateData = vb;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
			pstVideoFrame->stVFrame.enPixelFormat, pstVideoFrame->stVFrame.u32Width,
			pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.u64PTS,
			pstVideoFrame->stVFrame.u64PhyAddr[0], pstVideoFrame->stVFrame.u64PhyAddr[1],
			pstVideoFrame->stVFrame.u64PhyAddr[2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
			pstVideoFrame->stVFrame.u32Length[0], pstVideoFrame->stVFrame.u32Length[1],
			pstVideoFrame->stVFrame.u32Length[2], pstVideoFrame->stVFrame.u32Stride[0],
			pstVideoFrame->stVFrame.u32Stride[1], pstVideoFrame->stVFrame.u32Stride[2]);

	return CVI_SUCCESS;
}

s32 vo_release_chn_frame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	VB_BLK blk;
	s32 ret = CVI_SUCCESS;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Invalid phy-address(%llx) in pstVideoFrame. Can't find VB_BLK.\n"
			    , pstVideoFrame->stVFrame.u64PhyAddr[0]);
		return CVI_FAILURE;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "release chn frame, addr(0x%llx)\n",
			pstVideoFrame->stVFrame.u64PhyAddr[0]);

	return CVI_SUCCESS;
}

s32 vo_set_chn_framerate(VO_LAYER VoLayer, VO_CHN VoChn, u32 u32FrameRate)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	if (u32FrameRate > pstChnCtx->u32ChnFrameRate) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) FrameRate(%d) invalid.\n"
			, VoLayer, VoChn, u32FrameRate);
	}
	//ueser set framerate
	pstChnCtx->u32FrameRateUserSet = u32FrameRate;

	return CVI_SUCCESS;
}

s32 vo_get_chn_framerate(VO_LAYER VoLayer, VO_CHN VoChn, u32 *pu32FrameRate)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	//realframerate
	*pu32FrameRate = pstChnCtx->u32ChnFrameRate;

	return CVI_SUCCESS;
}

s32 vo_get_chn_pts(VO_LAYER VoLayer, VO_CHN VoChn, u64 *pu64ChnPTS)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	*pu64ChnPTS = pstChnCtx->u64DisplayPts;

	return CVI_SUCCESS;
}

s32 vo_get_chn_status(VO_LAYER VoLayer, VO_CHN VoChn, VO_QUERY_STATUS_S *pStstatus)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	//u32Threshold + VO_CHN_WORKQ + u32Depth
	pStstatus->u32ChnBufUsed = pstChnCtx->u32Threshold + pstChnCtx->stChnAttr.u32Depth;

	return CVI_SUCCESS;
}

s32 vo_set_chn_threshold(VO_LAYER VoLayer, VO_CHN VoChn, u32 u32Threshold)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) must be disable.\n", VoLayer, VoChn);
		return CVI_ERR_VO_CHN_NOT_DISABLED;
	}

	if ((u32Threshold < 2) || (u32Threshold > 8)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) u32Threshold(%d) invalid.\n", VoLayer, VoChn, u32Threshold);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	pstChnCtx->u32Threshold = u32Threshold;

	return CVI_SUCCESS;
}

s32 vo_get_chn_threshold(VO_LAYER VoLayer, VO_CHN VoChn, u32 *pu32Threshold)
{
	s32 ret = CVI_SUCCESS;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	*pu32Threshold = pstChnCtx->u32Threshold ? pstChnCtx->u32Threshold : VO_CHN_THRESHOLD;

	return CVI_SUCCESS;
}

s32 vo_enable_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already enabled.\n", VoLayer, VoChn);
		return CVI_ERR_VO_CHN_NOT_DISABLED;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	pstChnCtx->is_chn_enable = CVI_TRUE;
	if (pstChnCtx->u32Threshold)
		base_mod_jobs_init(&pstChnCtx->chn_jobs, pstChnCtx->u32Threshold - VO_CHN_WORKQ,
			VO_CHN_WORKQ, pstChnCtx->stChnAttr.u32Depth);
	else {
		base_mod_jobs_init(&pstChnCtx->chn_jobs, VO_CHN_THRESHOLD - VO_CHN_WORKQ, VO_CHN_WORKQ,
			pstChnCtx->stChnAttr.u32Depth);
		pstChnCtx->u32Threshold = VO_CHN_THRESHOLD;
	}
	mutex_unlock(&pstLayerCtx->layer_lock);

	return ret;
}

s32 vo_disable_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (!pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	base_mod_jobs_exit(&pstChnCtx->chn_jobs);
	pstChnCtx->is_chn_enable = CVI_FALSE;
	pstChnCtx->u32FrameNum = 0;
	pstChnCtx->u32ChnFrameRate = 0;
	pstChnCtx->u32SrcFrameNum = 0;
	pstChnCtx->u32ChnSrcFrameRate = 0;
	pstChnCtx->u32FrameIndex = 0;
	pstChnCtx->u32FrameRateUserSet = 0;
	pstChnCtx->u64DisplayPts = 0;
	pstChnCtx->u64PreDonePts = 0;
	pstChnCtx->enChnMirror = 0;
	pstChnCtx->bPause = CVI_FALSE;
	pstChnCtx->bRefresh = CVI_FALSE;
	pstChnCtx->bStep = CVI_FALSE;
	pstChnCtx->bStepTrigger = CVI_FALSE;
	memset(&pstChnCtx->stChnZoomAttr, 0, sizeof(pstChnCtx->stChnZoomAttr));
	memset(&pstChnCtx->stChnBorder, 0, sizeof(pstChnCtx->stChnBorder));
	memset(&pstChnCtx->stChnParam, 0, sizeof(pstChnCtx->stChnParam));
	mutex_unlock(&pstLayerCtx->layer_lock);
	CVI_TRACE_VO(CVI_DBG_INFO, "VoLayer(%d) VoChn(%d) disabled.\n", VoLayer, VoChn);

	return CVI_SUCCESS;
}

s32 vo_hide_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;
	struct vb_s *vb;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct vb_jobs_t *jobs;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	if (!pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bHide = CVI_TRUE;
	gVoCtx->astLayerCtx[VoLayer].bLayerUpdate = CVI_TRUE;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	jobs = &pstChnCtx->chn_jobs;
	mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	mutex_unlock(&jobs->lock);

	return CVI_SUCCESS;
}

s32 vo_show_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bHide = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].bLayerUpdate = CVI_TRUE;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return CVI_SUCCESS;
}

s32 vo_pause_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;
	struct vb_s *vb;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct vb_jobs_t *jobs;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	if (!pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bPause = CVI_TRUE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bStep = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bStepTrigger = CVI_FALSE;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	jobs = &pstChnCtx->chn_jobs;
	mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	mutex_unlock(&jobs->lock);

	return CVI_SUCCESS;
}

s32 vo_step_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	if (!pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}
	if (pstChnCtx->bStep && pstChnCtx->bStepTrigger) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) The last time step trigger was not finish.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bPause = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bRefresh = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bStep = CVI_TRUE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bStepTrigger = CVI_TRUE;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return CVI_SUCCESS;
}

s32 vo_resume_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bPause = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bRefresh = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bStep = CVI_FALSE;
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bStepTrigger = CVI_FALSE;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return CVI_SUCCESS;
}

s32 vo_refresh_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	struct cvi_vo_chn_ctx *pstChnCtx;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];

	if (!pstChnCtx->bPause) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) can only refresh in paused state.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	if (pstChnCtx->bRefresh) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) The last time refresh was not finish.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);
	gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].bRefresh = CVI_TRUE;
	mutex_unlock(&gVoCtx->astLayerCtx[VoLayer].layer_lock);

	return CVI_SUCCESS;
}

s32 vo_get_chnrotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E *penRotation)
{
	s32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*penRotation = gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].enRotation;

	return CVI_SUCCESS;
}

s32 vo_set_chnrotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E enRotation)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn];
	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (enRotation >= ROTATION_MAX) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) invalid rotation(%d).\n", VoLayer, VoChn, enRotation);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	} else if (enRotation == ROTATION_0) {
		pstChnCtx->enRotation = enRotation;
		return CVI_SUCCESS;
	}

	if (!GDC_SUPPORT_FMT(pstLayerCtx->stLayerAttr.enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) invalid PixFormat(%d).\n"
			, VoLayer, VoChn, pstLayerCtx->stLayerAttr.enPixFormat);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	// TODO: dummy settings
	pstChnCtx->mesh.paddr = DEFAULT_MESH_PADDR;
	pstChnCtx->enRotation = enRotation;
	mutex_unlock(&pstLayerCtx->layer_lock);

	return CVI_SUCCESS;
}

static int _vo_wbc_qbuf(struct cvi_vo_wbc_ctx *pstWbcCtx, struct cvi_buffer *buf)
{
	struct cvi_wbc_buffer *qbuf;

	qbuf = kzalloc(sizeof(struct cvi_wbc_buffer), GFP_ATOMIC);
	if (qbuf == NULL) {
		CVI_TRACE_VO(CVI_DBG_ERR, "qbuf kzalloc size(%zu) failed\n", sizeof(struct cvi_wbc_buffer));
		return -ENOMEM;
	}

	qbuf->buf.addr[0] = buf->phy_addr[0];
	qbuf->buf.addr[1] = buf->phy_addr[1];
	qbuf->buf.addr[2] = buf->phy_addr[2];

	qbuf->buf.pitch_y = buf->stride[0];
	qbuf->buf.pitch_c = buf->stride[1];

	qbuf->buf.width = buf->size.u32Width;
	qbuf->buf.height = buf->size.u32Height;

	vo_wbc_rdy_buf_queue(pstWbcCtx, qbuf);

	return CVI_SUCCESS;
}

s32 vo_wbc_qbuf(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	s32 ret;
	VB_BLK blk;
	struct vb_s *vb;
	VB_CAL_CONFIG_S stVbCalConfig;
	VO_DEV VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = VoDev, .s32ChnId = 0};
	SIZE_S stSize = pstWbcCtx->stWbcAttr.stTargetSize;
	PIXEL_FORMAT_E enPixFormat = pstWbcCtx->stWbcAttr.enPixFormat;
	// only support SDR8 COMPRESS_NONE
	COMMON_GetPicBufferConfig(stSize.u32Width, stSize.u32Height, enPixFormat
			, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, VIP_ALIGNMENT, &stVbCalConfig);
	// get vb for odma write
	blk = vb_get_block_with_id(VB_INVALID_POOLID, stVbCalConfig.u32VBSize, CVI_ID_VO);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Can't acquire vb block for wbc, size(%d)\n", stVbCalConfig.u32VBSize);
		return CVI_ERR_VO_NO_MEM;
	}

	vb = (struct vb_s *)blk;

	base_get_frame_info(enPixFormat
				, stSize
				, &vb->buf
				, vb_handle2phys_addr(blk)
				, DEFAULT_ALIGN);

	// not support scale/crop
	vb->buf.s16OffsetTop = 0;
	vb->buf.s16OffsetRight = 0;
	vb->buf.s16OffsetLeft = 0;
	vb->buf.s16OffsetBottom = 0;

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &pstWbcCtx->wbc_jobs, blk);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "wbc vb_qbuf failed\n");
		return ret;
	}

	ret = _vo_wbc_qbuf(pstWbcCtx, &vb->buf);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "_vo_wbc_qbuf failed\n");
		return ret;
	}

	ret = vb_release_block(blk);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "wbc vb_release_block failed\n");
		return ret;
	}

	return ret;
}

s32 vo_set_wbc_src(VO_WBC VoWbc, VO_WBC_SRC_S *pstWbcSrc)
{
	s32 ret = CVI_FAILURE;

	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];

	if ((pstWbcSrc->enSrcType < VO_WBC_SRC_DEV) || (pstWbcSrc->enSrcType >= VO_WBC_SRC_BUTT)
	 || (pstWbcSrc->u32SrcId >= VO_MAX_DEV_NUM)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) SrcType(%d) SrcId(%d) invalid.\n"
			, VoWbc, pstWbcSrc->enSrcType, pstWbcSrc->u32SrcId);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstWbcSrc->u32SrcId == 0) && (pstWbcSrc->enSrcType == VO_WBC_SRC_DEV)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) SrcType(%d) SrcId(%d) invalid. "
			"only device 1 support VO_WBC_SRC_DEV\n"
			, VoWbc, pstWbcSrc->enSrcType, pstWbcSrc->u32SrcId);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (pstWbcCtx->is_wbc_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) already enabled.\n", VoWbc);
		return CVI_ERR_VO_WBC_NOT_DISABLED;
	}

	mutex_lock(&pstWbcCtx->wbc_lock);
	memcpy(&pstWbcCtx->stWbcSrc, pstWbcSrc, sizeof(*pstWbcSrc));
	pstWbcCtx->is_wbc_src_cfg = CVI_TRUE;
	mutex_unlock(&pstWbcCtx->wbc_lock);

	return CVI_SUCCESS;
}

s32 vo_get_wbc_src(VO_WBC VoWbc, VO_WBC_SRC_S *pstWbcSrc)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	memcpy(pstWbcSrc, &pstWbcCtx->stWbcSrc, sizeof(*pstWbcSrc));

	return CVI_SUCCESS;
}

s32 vo_enable_wbc(VO_WBC VoWbc)
{
	s32 ret = CVI_FAILURE;
	VO_DEV VoDev;
	VO_LAYER VoLayer;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_wbc_ctx *pstWbcCtx;
	s32 i = 0;
	struct cvi_wbc_buffer *wbc_qbuf, *tmp;
	unsigned long flags;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];
	VoLayer = pstDevCtx->s32BindLayerId;

	if (pstDevCtx->s32BindLayerId == -1) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "VoDev(%d) unbind layer", VoDev);
		return CVI_ERR_VO_SYS_NOTREADY;
	}

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (!pstDevCtx->is_dev_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) isn't enabled yet.\n", VoDev);
		return CVI_ERR_VO_DEV_NOT_ENABLED;
	}

	if (!pstLayerCtx->is_layer_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't enabled yet.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_ENABLED;
	}

	if (!pstWbcCtx->is_wbc_attr_cfg) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) attr has not been set yet.\n", VoWbc);
		return CVI_ERR_VO_WBC_ATTR_NOT_CONFIG;
	}

	if (pstWbcCtx->is_wbc_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) already enabled.\n", VoWbc);
		return CVI_SUCCESS;
	}

	if (pstWbcCtx->stWbcSrc.enSrcType == VO_WBC_SRC_DEV) {
		base_mod_jobs_init(&pstWbcCtx->wbc_jobs, 0, VO_MAX_WBC_BUF, pstWbcCtx->u32Depth);
		INIT_LIST_HEAD(&pstWbcCtx->qbuf_list);
		INIT_LIST_HEAD(&pstWbcCtx->dqbuf_list);
		for (i = 0; i < VO_MAX_WBC_BUF; i++) {
			ret = vo_wbc_qbuf(pstWbcCtx);
			if (ret != CVI_SUCCESS) {
				CVI_TRACE_VO(CVI_DBG_ERR, "vo_wbc_sdk_qbuf error (%d)", ret);
				goto ERR_QBUF;
			}
		}
		pstWbcCtx->event = 0;
		init_waitqueue_head(&pstWbcCtx->wq);
		ret = vo_wbc_create_thread(VoWbc);
		if (ret) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Failed to create thread, VoWbc(%d).\n", VoWbc);
			return ret;
		}
	}

	mutex_lock(&pstWbcCtx->wbc_lock);
	pstWbcCtx->is_wbc_enable = CVI_TRUE;
	mutex_unlock(&pstWbcCtx->wbc_lock);

	return ret;

ERR_QBUF:
	spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
	list_for_each_entry_safe(wbc_qbuf, tmp, &(pstWbcCtx->qbuf_list), list) {
		kfree(wbc_qbuf);
	}
	pstWbcCtx->qbuf_num = 0;
	INIT_LIST_HEAD(&pstWbcCtx->qbuf_list);
	spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);

	spin_lock_irqsave(&pstWbcCtx->dqbuf_lock, flags);
	list_for_each_entry_safe(wbc_qbuf, tmp, &(pstWbcCtx->dqbuf_list), list) {
		kfree(wbc_qbuf);
	}
	INIT_LIST_HEAD(&pstWbcCtx->dqbuf_list);
	spin_unlock_irqrestore(&pstWbcCtx->dqbuf_lock, flags);

	base_mod_jobs_exit(&pstWbcCtx->wbc_jobs);

	return ret;
}

s32 vo_disable_wbc(VO_WBC VoWbc)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;
	struct cvi_wbc_buffer *wbc_qbuf, *tmp;
	unsigned long flags;
	VO_DEV VoDev;
	union disp_online_odma_intr_sel online_odma_mask;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	VoDev = pstWbcCtx->stWbcSrc.u32SrcId;

	if (!pstWbcCtx->is_wbc_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) already disabled.\n", VoWbc);
		return CVI_SUCCESS;
	}

	if (pstWbcCtx->stWbcSrc.enSrcType == VO_WBC_SRC_DEV) {
		vo_wbc_destroy_thread(VoWbc);

		spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
		list_for_each_entry_safe(wbc_qbuf, tmp, &(pstWbcCtx->qbuf_list), list) {
			kfree(wbc_qbuf);
		}
		pstWbcCtx->qbuf_num = 0;
		INIT_LIST_HEAD(&pstWbcCtx->qbuf_list);
		spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);

		spin_lock_irqsave(&pstWbcCtx->dqbuf_lock, flags);
		list_for_each_entry_safe(wbc_qbuf, tmp, &(pstWbcCtx->dqbuf_list), list) {
			kfree(wbc_qbuf);
		}
		INIT_LIST_HEAD(&pstWbcCtx->dqbuf_list);
		spin_unlock_irqrestore(&pstWbcCtx->dqbuf_lock, flags);

		base_mod_jobs_exit(&pstWbcCtx->wbc_jobs);

		if (pstWbcCtx->is_odma_enable == true) {
			// odma disable
			disp_get_odma_intr_mask(pstWbcCtx->stWbcSrc.u32SrcId, &online_odma_mask);
			online_odma_mask.b.disp_online_frame_end = false; //true means disable
			online_odma_mask.b.disp_odma_frame_end = true; //true means disable
			disp_set_odma_intr_mask(pstWbcCtx->stWbcSrc.u32SrcId, online_odma_mask);
			disp_odma_enable(pstWbcCtx->stWbcSrc.u32SrcId, false);
			pstWbcCtx->is_odma_enable = false;
		}
	}

	mutex_lock(&pstWbcCtx->wbc_lock);
	pstWbcCtx->is_wbc_enable = CVI_FALSE;
	pstWbcCtx->is_wbc_src_cfg = CVI_FALSE;
	pstWbcCtx->is_wbc_attr_cfg = CVI_FALSE;
	pstWbcCtx->stWbcSrc.enSrcType = VO_WBC_SRC_DEV;
	pstWbcCtx->stWbcSrc.u32SrcId = 0;
	pstWbcCtx->stWbcAttr.stTargetSize.u32Width = 0;
	pstWbcCtx->stWbcAttr.stTargetSize.u32Height = 0;
	pstWbcCtx->stWbcAttr.enPixFormat = PIXEL_FORMAT_NV21;
	pstWbcCtx->stWbcAttr.u32FrameRate = 0;
	pstWbcCtx->stWbcAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
	pstWbcCtx->stWbcAttr.enCompressMode = COMPRESS_MODE_NONE;
	pstWbcCtx->enWbcMode = VO_WBC_MODE_NORM;
	pstWbcCtx->u32Depth = VO_WBC_DONEQ;
	pstWbcCtx->u32DoneCnt = 0;
	pstWbcCtx->u32FrameNum = 0;
	pstWbcCtx->u32WbcFrameRate = 0;
	pstWbcCtx->u32OdmaFifoFull = 0;
	mutex_unlock(&pstWbcCtx->wbc_lock);

	return CVI_SUCCESS;
}

s32 vo_set_wbc_attr(VO_WBC VoWbc, VO_WBC_ATTR_S *pstWbcAttr)
{
	s32 ret = CVI_FAILURE;
	VO_DEV VoDev;
	VO_LAYER VoLayer;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];
	VoLayer = pstDevCtx->s32BindLayerId;
	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (!pstWbcCtx->is_wbc_src_cfg) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) src has not been set yet.\n", VoWbc);
		return CVI_ERR_VO_WBC_SRC_NOT_CONFIG;
	}

	if ((pstWbcAttr->stTargetSize.u32Width != pstLayerCtx->stLayerAttr.stImageSize.u32Width)
	 || (pstWbcAttr->stTargetSize.u32Height != pstLayerCtx->stLayerAttr.stImageSize.u32Height)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) Size (%d %d) must be same to Layer Size (%d %d).\n", VoWbc,
			pstWbcAttr->stTargetSize.u32Width, pstWbcAttr->stTargetSize.u32Height,
			pstLayerCtx->stLayerAttr.stImageSize.u32Width, pstLayerCtx->stLayerAttr.stImageSize.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (!VO_SUPPORT_FMT(pstWbcAttr->enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) enPixFormat(%d) unsupported\n"
			, VoWbc,  pstWbcAttr->enPixFormat);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (pstWbcAttr->enDynamicRange != DYNAMIC_RANGE_SDR8) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) enDynamicRange(%d) unsupported\n"
			, VoWbc,  pstWbcAttr->enDynamicRange);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (pstWbcAttr->enCompressMode != COMPRESS_MODE_NONE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) enCompressMode(%d) unsupported\n"
			, VoWbc, pstWbcAttr->enCompressMode);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstWbcCtx->wbc_lock);
 	if (pstWbcCtx->stWbcSrc.enSrcType == VO_WBC_SRC_VIDEO) {
		CVI_TRACE_VO(CVI_DBG_INFO, "VoWbc(%d) enSrcType(%d) use the LayerAttr\n"
			, VoWbc,  pstWbcCtx->stWbcSrc.enSrcType);
		pstWbcCtx->stWbcAttr.stTargetSize = pstLayerCtx->stLayerAttr.stImageSize;
		pstWbcCtx->stWbcAttr.enPixFormat = pstLayerCtx->stLayerAttr.enPixFormat;
		pstWbcCtx->stWbcAttr.u32FrameRate = pstLayerCtx->stLayerAttr.u32DispFrmRt;
		pstWbcCtx->stWbcAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
		pstWbcCtx->stWbcAttr.enCompressMode = COMPRESS_MODE_NONE;
	} else {
		pstWbcCtx->stWbcAttr.stTargetSize = pstWbcAttr->stTargetSize;
		pstWbcCtx->stWbcAttr.enPixFormat = pstWbcAttr->enPixFormat;
		pstWbcCtx->stWbcAttr.u32FrameRate = pstWbcAttr->u32FrameRate;
		pstWbcCtx->stWbcAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
		pstWbcCtx->stWbcAttr.enCompressMode = COMPRESS_MODE_NONE;
	}

	pstWbcCtx->is_wbc_attr_cfg = CVI_TRUE;

	mutex_unlock(&pstWbcCtx->wbc_lock);

	return CVI_SUCCESS;
}

s32 vo_get_wbc_attr(VO_WBC VoWbc, VO_WBC_ATTR_S *pstWbcAttr)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	memcpy(pstWbcAttr, &pstWbcCtx->stWbcAttr, sizeof(*pstWbcAttr));

	return CVI_SUCCESS;
}

s32 vo_set_wbc_mode(VO_WBC VoWbc, VO_WBC_MODE_E enWbcMode)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];

	if (pstWbcCtx->is_wbc_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) must be in disabled state.\n", VoWbc);
		return CVI_ERR_VO_WBC_NOT_DISABLED;
	}

	// if (enWbcMode < VO_WBC_MODE_NORM || enWbcMode >= VO_WBC_MODE_BUTT) {
	// 	CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) Mode(%d) not support.\n", VoWbc, enWbcMode);
	// 	return CVI_ERR_VO_ILLEGAL_PARAM;
	// }

	// now only support VO_WBC_MODE_NORM
	if (enWbcMode != VO_WBC_MODE_NORM) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) Mode(%d) not support.\n", VoWbc, enWbcMode);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&pstWbcCtx->wbc_lock);
	pstWbcCtx->enWbcMode = enWbcMode;
	mutex_unlock(&pstWbcCtx->wbc_lock);

	return CVI_SUCCESS;
}

s32 vo_get_wbc_mode(VO_WBC VoWbc, VO_WBC_MODE_E *penWbcMode)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];

	*penWbcMode = pstWbcCtx->enWbcMode;

	return CVI_SUCCESS;
}

s32 vo_set_wbc_depth(VO_WBC VoWbc, u32 u32Depth)
{
	s32 ret = CVI_FAILURE;
	VO_DEV VoDev;
	VO_LAYER VoLayer;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;


	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];
	VoLayer = pstDevCtx->s32BindLayerId;
	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (pstWbcCtx->is_wbc_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) must be in disabled state.\n", VoWbc);
		return CVI_ERR_VO_WBC_NOT_DISABLED;
	}

	mutex_lock(&pstWbcCtx->wbc_lock);
	if (pstWbcCtx->stWbcSrc.enSrcType == VO_WBC_SRC_VIDEO) {
		CVI_TRACE_VO(CVI_DBG_INFO, "VoWbc(%d) u32Depth(%d) is Useless for enSrcType(%d)\n"
			, VoWbc, u32Depth, pstWbcCtx->stWbcSrc.enSrcType);
		pstWbcCtx->u32Depth = pstLayerCtx->stLayerAttr.u32Depth;
	} else {
		pstWbcCtx->u32Depth = u32Depth;
	}
	mutex_unlock(&pstWbcCtx->wbc_lock);

	return CVI_SUCCESS;
}

s32 vo_get_wbc_depth(VO_WBC VoWbc, u32 *u32Depth)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];

	*u32Depth = pstWbcCtx->u32Depth;

	return CVI_SUCCESS;
}

s32 vo_get_wbc_frame(VO_WBC VoWbc, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	s32 ret = CVI_FAILURE;
	struct cvi_vo_wbc_ctx *pstWbcCtx;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	VB_BLK blk;
	VO_DEV VoDev;
	VO_LAYER VoLayer;
	struct vb_s *vb;
	s32 i = 0;
	MMF_CHN_S chn;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];
	VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	pstDevCtx = &gVoCtx->astDevCtx[VoDev];
	VoLayer = pstDevCtx->s32BindLayerId;
	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	chn.enModId = CVI_ID_VO;
	chn.s32DevId = VoDev;
	chn.s32ChnId = 0;

	if (!pstWbcCtx->is_wbc_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) isn't enabled yet.\n", VoWbc);
		return CVI_ERR_VO_WBC_NOT_ENABLED;
	}

	if (pstWbcCtx->u32Depth <= 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) depth not enough.\n", VoWbc);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));

	if (pstWbcCtx->stWbcSrc.enSrcType == VO_WBC_SRC_DEV) {
		ret = base_get_chn_buffer(chn, &pstWbcCtx->wbc_jobs, &blk, s32MilliSec);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "wbc get buf fail\n");
			return ret;
		}
	} else {
		ret = base_get_chn_buffer(chn, &pstLayerCtx->layer_jobs, &blk, s32MilliSec);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "wbc get buf fail\n");
			return ret;
		}
	}

	vb = (struct vb_s *)blk;

	pstVideoFrame->stVFrame.enPixelFormat = vb->buf.enPixelFormat;
	pstVideoFrame->stVFrame.u32Width = vb->buf.size.u32Width;
	pstVideoFrame->stVFrame.u32Height = vb->buf.size.u32Height;
	pstVideoFrame->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstVideoFrame->stVFrame.u64PTS = vb->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pstVideoFrame->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstVideoFrame->stVFrame.u32Length[i] = vb->buf.length[i];
		pstVideoFrame->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstVideoFrame->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstVideoFrame->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstVideoFrame->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstVideoFrame->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstVideoFrame->stVFrame.pPrivateData = vb;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
			pstVideoFrame->stVFrame.enPixelFormat, pstVideoFrame->stVFrame.u32Width,
			pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.u64PTS,
			pstVideoFrame->stVFrame.u64PhyAddr[0], pstVideoFrame->stVFrame.u64PhyAddr[1],
			pstVideoFrame->stVFrame.u64PhyAddr[2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
			pstVideoFrame->stVFrame.u32Length[0], pstVideoFrame->stVFrame.u32Length[1],
			pstVideoFrame->stVFrame.u32Length[2], pstVideoFrame->stVFrame.u32Stride[0],
			pstVideoFrame->stVFrame.u32Stride[1], pstVideoFrame->stVFrame.u32Stride[2]);

	return CVI_SUCCESS;
}

s32 vo_release_wbc_frame(VO_WBC VoWbc, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	VB_BLK blk;
	s32 ret = CVI_SUCCESS;

	ret = CHECK_VO_WBC_VALID(VoWbc);
	if (ret != CVI_SUCCESS)
		return ret;

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Invalid phy-address(%llx) in pstVideoFrame. Can't find VB_BLK.\n"
			    , pstVideoFrame->stVFrame.u64PhyAddr[0]);
		return CVI_FAILURE;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_VO(CVI_DBG_DEBUG, "release wbc frame, addr(0x%llx)\n",
			pstVideoFrame->stVFrame.u64PhyAddr[0]);

	return CVI_SUCCESS;
}

s32 vo_resume(void)
{
	s32 ret = CVI_FAILURE;
	VO_WBC VoWbc;
	VO_LAYER VoLayer;
	VO_DEV VoDev;

	for (VoDev = 0; VoDev < VO_MAX_DEV_NUM; ++VoDev)
		if (gVoCtx->astDevCtx[VoDev].is_dev_enable && gVoCtx->bSuspend) {
			ret = vo_start_streaming(VoDev);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo start streaming\n");
				return -EAGAIN;
			}
		}

	for (VoLayer = 0; VoLayer < VO_MAX_LAYER_NUM; ++VoLayer)
		if (gVoCtx->astLayerCtx[VoLayer].is_layer_enable && gVoCtx->bSuspend) {
			ret = vo_create_thread(VoLayer);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo create thread\n");
				ret = -EAGAIN;
			}
		}

	for (VoWbc = 0; VoWbc < VO_MAX_WBC_NUM; ++VoWbc)
		if (gVoCtx->astWbcCtx[VoWbc].is_wbc_enable && gVoCtx->bSuspend) {
			ret = vo_wbc_create_thread(VoWbc);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Failed to wbc create thread\n");
				ret = -EAGAIN;
			}
		}

	gVoCtx->bSuspend = CVI_FALSE;

	return ret;
}

s32 vo_suspend(void)
{
	s32 ret = CVI_FAILURE;
	VO_WBC VoWbc;
	VO_LAYER VoLayer;
	VO_DEV VoDev = 0;

	for (VoWbc = 0; VoWbc < VO_MAX_WBC_NUM; ++VoWbc)
		if (gVoCtx->astWbcCtx[VoWbc].is_wbc_enable) {
			ret = vo_wbc_destroy_thread(VoWbc);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Failed to wbc destory thread\n");
				ret = -EAGAIN;
			}
		}

	for (VoLayer = 0; VoLayer < VO_MAX_LAYER_NUM; ++VoLayer)
		if (gVoCtx->astLayerCtx[VoLayer].is_layer_enable) {
			ret = vo_destroy_thread(VoLayer);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo destory thread\n");
				ret = -EAGAIN;
			}
		}

	for (VoDev = 0; VoDev < VO_MAX_DEV_NUM; ++VoDev)
		if (gVoCtx->astDevCtx[VoDev].is_dev_enable) {
			ret = vo_stop_streaming(VoDev);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo stop streaming\n");
				return -EAGAIN;
			}
		}

	gVoCtx->bSuspend = CVI_TRUE;

	return ret;
}


/*****************************************************************************
 *  SDK layer ioctl operations for vi.c
 ****************************************************************************/
long vo_sdk_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->sdk_id;
	long rc = CVI_SUCCESS;

	switch (id) {
	case VO_SDK_SET_CHNATTR: {
		struct vo_chn_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_attr(cfg.VoLayer, cfg.VoChn, &cfg.stChnAttr);
	}
	break;

	case VO_SDK_GET_CHNATTR: {
		struct vo_chn_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_attr(cfg.VoLayer, cfg.VoChn, &cfg.stChnAttr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNATTR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNPARAM: {
		struct vo_chn_param_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_param_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_param(cfg.VoLayer, cfg.VoChn, &cfg.stChnParam);
	}
	break;

	case VO_SDK_GET_CHNPARAM: {
		struct vo_chn_param_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_param_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_param(cfg.VoLayer, cfg.VoChn, &cfg.stChnParam);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_param_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNPARAM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNZOOM: {
		struct vo_chn_zoom_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_zoom_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNZOOM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_zoom(cfg.VoLayer, cfg.VoChn, &cfg.stChnZoomAttr);
	}
	break;

	case VO_SDK_GET_CHNZOOM: {
		struct vo_chn_zoom_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_zoom_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNZOOM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_zoom(cfg.VoLayer, cfg.VoChn, &cfg.stChnZoomAttr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_zoom_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNZOOM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNBORDER: {
		struct vo_chn_border_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_border_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNBORDER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_border(cfg.VoLayer, cfg.VoChn, &cfg.stChnBorder);
	}
	break;

	case VO_SDK_GET_CHNBORDER: {
		struct vo_chn_border_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_border_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNBORDER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_border(cfg.VoLayer, cfg.VoChn, &cfg.stChnBorder);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_border_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNBORDER copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNMIRROR: {
		struct vo_chn_mirror_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_mirror_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNMIRROR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_mirror(cfg.VoLayer, cfg.VoChn, cfg.enChnMirror);
	}
	break;

	case VO_SDK_GET_CHNMIRROR: {
		struct vo_chn_mirror_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_mirror_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNMIRROR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_mirror(cfg.VoLayer, cfg.VoChn, &cfg.enChnMirror);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_mirror_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNMIRROR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNFRAME : {
		struct vo_chn_frame_cfg stVoChnFrame;

		if (copy_from_user(&stVoChnFrame, p->ptr, sizeof(struct vo_chn_frame_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_frame(stVoChnFrame.VoLayer, stVoChnFrame.VoChn, &stVoChnFrame.stVideoFrame,
						stVoChnFrame.s32MilliSec);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_get_chn_frame failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &stVoChnFrame, sizeof(struct vo_chn_frame_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNFRAME copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_RELEASE_CHNFRAME : {
		struct vo_chn_frame_cfg stVoChnFrame;

		if (copy_from_user(&stVoChnFrame, p->ptr, sizeof(struct vo_chn_frame_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_RELEASE_CHNFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_release_chn_frame(stVoChnFrame.VoLayer, stVoChnFrame.VoChn, &stVoChnFrame.stVideoFrame,
						stVoChnFrame.s32MilliSec);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_release_chn_frame failed with ret(%lx).\n", rc);
			break;
		}
	}
	break;

	case VO_SDK_SET_CHNFRAMERATE : {
		struct vo_chn_frmrate_cfg stVoChnFrameRate;

		if (copy_from_user(&stVoChnFrameRate, p->ptr, sizeof(struct vo_chn_frmrate_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNFRAMERATE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_framerate(stVoChnFrameRate.VoLayer, stVoChnFrameRate.VoChn, stVoChnFrameRate.u32FrameRate);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_set_chn_framerate failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &stVoChnFrameRate, sizeof(struct vo_chn_frmrate_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNFRAMERATE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNFRAMERATE : {
		struct vo_chn_frmrate_cfg stVoChnFrameRate;

		if (copy_from_user(&stVoChnFrameRate, p->ptr, sizeof(struct vo_chn_frmrate_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNFRAMERATE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_framerate(stVoChnFrameRate.VoLayer, stVoChnFrameRate.VoChn, &stVoChnFrameRate.u32FrameRate);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_get_chn_framerate failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &stVoChnFrameRate, sizeof(struct vo_chn_frmrate_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNFRAMERATE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNPTS : {
		struct vo_chn_pts_cfg stVoChnPts;

		if (copy_from_user(&stVoChnPts, p->ptr, sizeof(struct vo_chn_pts_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNPTS copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_pts(stVoChnPts.VoLayer, stVoChnPts.VoChn, &stVoChnPts.u64ChnPTS);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_get_chn_pts failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &stVoChnPts, sizeof(struct vo_chn_pts_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNPTS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNSTATUS : {
		struct vo_chn_status_cfg stVoChnStatus;

		if (copy_from_user(&stVoChnStatus, p->ptr, sizeof(struct vo_chn_status_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNSTATUS copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_status(stVoChnStatus.VoLayer, stVoChnStatus.VoChn, &stVoChnStatus.stStatus);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_get_chn_status failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &stVoChnStatus, sizeof(struct vo_chn_status_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNSTATUS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNTHRESHOLD: {
		struct vo_chn_threshold_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_threshold_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNTHRESHOLD copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_threshold(cfg.VoLayer, cfg.VoChn, cfg.u32Threshold);
	}
	break;

	case VO_SDK_GET_CHNTHRESHOLD: {
		struct vo_chn_threshold_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_threshold_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNTHRESHOLD copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_threshold(cfg.VoLayer, cfg.VoChn, &cfg.u32Threshold);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_threshold_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNTHRESHOLD copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_PUBATTR: {
		struct vo_pub_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_pub_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_PUBATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_pub_attr(cfg.VoDev, &cfg.stPubAttr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_pub_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_PUBATTR copy_to_user failed.\n");
			rc = -EFAULT;
		}

	}
	break;

	case VO_SDK_SET_PUBATTR: {
		struct vo_pub_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_pub_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_PUBATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_pub_attr(cfg.VoDev, &cfg.stPubAttr);
	}
	break;

	case VO_SDK_GET_HDMIPARAM: {
		struct vo_hdmi_param_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_hdmi_param_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_HDMIPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_hdmi_param(cfg.VoDev, &cfg.stHDMIParam);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_hdmi_param_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_HDMIPARAM copy_to_user failed.\n");
			rc = -EFAULT;
		}

	}
	break;

	case VO_SDK_SET_HDMIPARAM: {
		struct vo_hdmi_param_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_hdmi_param_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_HDMIPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_hdmi_param(cfg.VoDev, &cfg.stHDMIParam);
	}
	break;

	case VO_SDK_SUSPEND: {
		rc = vo_suspend();
	}
	break;

	case VO_SDK_RESUME: {
		rc = vo_resume();
	}
	break;

	case VO_SDK_GET_PANELSTATUE: {
		struct vo_panel_status_cfg cfg;
		VO_DEV VoDev;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_panel_status_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_PANELSTATUE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}
		VoDev = cfg.VoLayer;
		vo_get_panelstatus(VoDev, &cfg.is_init);

		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_PANELSTATUE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_ENABLE_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_ENABLE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enable_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_DISABLE_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_DISABLE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disable_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_SHOW_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SHOW_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_show_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_HIDE_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_HIDE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_hide_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_ENABLE: {
		struct vo_dev_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_ENABLE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enable(cfg.VoDev);
	}
	break;

	case VO_SDK_DISABLE: {
		struct vo_dev_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_DISABLE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disable(cfg.VoDev);
	}
	break;

	case VO_SDK_ISENABLE: {
		struct vo_dev_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_ISENABLE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}
		cfg.isEnable = gVoCtx->astDevCtx[cfg.VoDev].is_dev_enable;
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_dev_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_ISENABLE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SEND_FRAME: {
		struct vo_snd_frm_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_snd_frm_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SEND_FRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_send_frame(cfg.VoLayer, cfg.VoChn, &cfg.stVideoFrame, cfg.s32MilliSec);
	}
	break;

	case VO_SDK_CLEAR_CHNBUF: {
		struct vo_clear_chn_buf_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_clear_chn_buf_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_CLEAR_CHNBUF copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_clear_chnbuf(cfg.VoLayer, cfg.VoChn, cfg.bClrAll);
	}
	break;

	case VO_SDK_SET_DISPLAYBUFLEN: {
		struct vo_display_buflen_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_display_buflen_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_DISPLAYBUFLEN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_displaybuflen(cfg.VoLayer, cfg.u32BufLen);
	}
	break;

	case VO_SDK_GET_DISPLAYBUFLEN: {
		struct vo_display_buflen_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_display_buflen_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_DISPLAYBUFLEN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_displaybuflen(cfg.VoLayer, &cfg.u32BufLen);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_display_buflen_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_DISPLAYBUFLEN copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNROTATION: {
		struct vo_chn_rotation_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNROTATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chnrotation(cfg.VoLayer, cfg.VoChn, &cfg.enRotation);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_display_buflen_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_CHNROTATION copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNROTATION: {
		struct vo_chn_rotation_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_CHNROTATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chnrotation(cfg.VoLayer, cfg.VoChn, cfg.enRotation);
	}
	break;

	case VO_SDK_SET_VIDEOLAYERATTR: {
		struct vo_video_layer_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_VIDEOLAYERATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_videolayerattr(cfg.VoLayer, &cfg.stLayerAttr);
	}
	break;

	case VO_SDK_GET_VIDEOLAYERATTR: {
		struct vo_video_layer_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_VIDEOLAYERATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_videolayerattr(cfg.VoLayer, &cfg.stLayerAttr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_video_layer_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_VIDEOLAYERATTR copy_to_user failed.\n");
			rc = -EFAULT;

		}
	}
	break;

	case VO_SDK_SET_LAYER_PROC_AMP: {
		struct vo_layer_proc_amp_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_proc_amp_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_LAYER_PROC_AMP copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_proc_amp(cfg.VoLayer, cfg.proc_amp);
	}
	break;

	case VO_SDK_GET_LAYER_PROC_AMP: {
		struct vo_layer_proc_amp_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_proc_amp_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_LAYER_PROC_AMP copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_proc_amp(cfg.VoLayer, cfg.proc_amp);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_layer_proc_amp_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_LAYER_PROC_AMP copy_to_user failed.\n");
			rc = -EFAULT;

		}
	}
	break;

	case VO_SDK_SET_LAYERCSC: {
		struct vo_layer_csc_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_csc_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_LAYERCSC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_csc(cfg.VoLayer, cfg.stVideoCSC);
	}
	break;

	case VO_SDK_GET_LAYERCSC: {
		struct vo_layer_csc_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_csc_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_LAYERCSC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_csc(cfg.VoLayer, &cfg.stVideoCSC);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_layer_csc_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_LAYERCSC copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_ENABLE_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_ENABLE_VIDEOLAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enablevideolayer(cfg.VoLayer);
	}
	break;

	case VO_SDK_DISABLE_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_DISABLE_VIDEOLAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disablevideolayer(cfg.VoLayer);
	}
	break;

	case VO_SDK_SET_LAYERTOLERATION: {
		struct vo_layer_toleration_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_toleration_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_LAYERTOLERATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_toleration(cfg.VoLayer, cfg.u32Toleration);
	}
	break;

	case VO_SDK_GET_LAYERTOLERATION: {
		struct vo_layer_toleration_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_toleration_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_LAYERTOLERATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_toleration(cfg.VoLayer, &cfg.u32Toleration);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_layer_toleration_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_LAYERTOLERATION copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_SCREENFRAME : {
		struct vo_screen_frame stVoScreenFrame;

		if (copy_from_user(&stVoScreenFrame, p->ptr, sizeof(struct vo_screen_frame))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_SCREENFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_screen_frame(stVoScreenFrame.VoLayer, &stVoScreenFrame.stVideoFrame,
						stVoScreenFrame.s32MilliSec);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_get_screen_frame failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &stVoScreenFrame, sizeof(struct vo_screen_frame))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_SCREENFRAME copy_to_user failed.\n");
			rc = -EFAULT;
		}

	}
	break;

	case VO_SDK_RELEASE_SCREENFRAME : {
		struct vo_screen_frame stVoScreenFrame;

		if (copy_from_user(&stVoScreenFrame, p->ptr, sizeof(struct vo_screen_frame))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_RELEASE_SCREENFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_release_screen_frame(stVoScreenFrame.VoLayer, &stVoScreenFrame.stVideoFrame,
						stVoScreenFrame.s32MilliSec);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_release_screen_frame failed with ret(%lx).\n", rc);
			break;
		}
	}
	break;


	case VO_SDK_BIND_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_BIND_VIDEOLAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_bind_videolayer(cfg.VoLayer, cfg.VoLayer);
	}
	break;

	case VO_SDK_PAUSE_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_PAUSE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_pause_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_STEP_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_STEP_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_step_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_REFRESH_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_REFRESH_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_refresh_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_RESUME_CHN: {
		struct vo_chn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_RESUME_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_resume_chn(cfg.VoLayer, cfg.VoChn);
	}
	break;

	case VO_SDK_SET_WBCSRC: {
		struct vo_wbc_src_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_src_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_WBCSRC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_src(cfg.VoWbc, &cfg.stWbcSrc);
	}
	break;

	case VO_SDK_GET_WBCSRC: {
		struct vo_wbc_src_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_src_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCSRC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_src(cfg.VoWbc, &cfg.stWbcSrc);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_src_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCSRC copy_to_user failed.\n");
			rc = -EFAULT;
		}

	}
	break;

	case VO_SDK_ENABLE_WBC: {
		struct vo_wbc_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_ENABLE_WBC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enable_wbc(cfg.VoWbc);
	}
	break;

	case VO_SDK_DISABLE_WBC: {
		struct vo_wbc_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_DISABLE_WBC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disable_wbc(cfg.VoWbc);
	}
	break;

	case VO_SDK_SET_WBCATTR: {
		struct vo_wbc_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_WBCATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_attr(cfg.VoWbc, &cfg.stWbcAttr);
	}
	break;

	case VO_SDK_GET_WBCATTR: {
		struct vo_wbc_attr_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_attr(cfg.VoWbc, &cfg.stWbcAttr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_attr_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCATTR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_WBCMODE: {
		struct vo_wbc_mode_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_mode_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_WBCMODE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_mode(cfg.VoWbc, cfg.enWbcMode);
	}
	break;

	case VO_SDK_GET_WBCMODE: {
		struct vo_wbc_mode_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_mode_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCMODE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_mode(cfg.VoWbc, &cfg.enWbcMode);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_mode_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCMODE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_WBCDEPTH: {
		struct vo_wbc_depth_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_depth_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_SET_WBCDEPTH copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_depth(cfg.VoWbc, cfg.u32Depth);
	}
	break;

	case VO_SDK_GET_WBCDEPTH: {
		struct vo_wbc_depth_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_depth_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCDEPTH copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_depth(cfg.VoWbc, &cfg.u32Depth);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_depth_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCDEPTH copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_WBCFRAME: {
		struct vo_wbc_frame_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_frame_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_frame(cfg.VoWbc, &cfg.stVideoFrame,
						cfg.s32MilliSec);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_get_wbc_frame failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_frame_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_GET_WBCFRAME copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_RELEASE_WBCFRAME: {
		struct vo_wbc_frame_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_frame_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_SDK_RELEASE_WBCFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_release_wbc_frame(cfg.VoWbc, &cfg.stVideoFrame,
						cfg.s32MilliSec);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo_release_wbc_frame failed with ret(%lx).\n", rc);
			break;
		}
	}
	break;

	default:
		break;
	}

	return rc;
}

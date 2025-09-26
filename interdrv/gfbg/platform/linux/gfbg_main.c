#include <linux/version.h>
#include <linux/interrupt.h>
#include "gfbg_init.h"
#include "gfbg_main.h"
#include "gfbg_hal.h"
#include "gfbg_debug.h"
#include "gfbg_ctrl.h"
#include "gfbg_disp.h"
#include "gfbg_callback.h"
#include "ion.h"
#include "tde_cb.h"

#define VXRES_SIZE(xres, bpp) ALIGN((xres), GFBG_ALIGN / (bpp / 8))
#define FB_LINE_SIZE(vxres, bpp) (((vxres) * ((bpp) >> 3) + GFBG_ALIGNMENT) & (~GFBG_ALIGNMENT))

typedef int(*drv_gfbg_ioctl_func)(struct fb_info *info, unsigned long arg);

typedef struct {
	unsigned int cmd;
	drv_gfbg_ioctl_func func;
} drv_gfbg_ioctl_func_item;

#define DRV_GFBG_IOCTL_FUNC_ITEM_NUM_MAX 46

static int drv_gfbg_get_screen_origin_pos(struct fb_info *info, unsigned long arg);
static int drv_gfbg_set_screen_origin_pos(struct fb_info *info, unsigned long arg);
static int drv_gfbg_show_layer(struct fb_info *info, unsigned long arg);
static int drv_gfbg_get_layer_show_state(struct fb_info *info, unsigned long arg);
static int drv_gfbg_set_screen_size(struct fb_info *info, unsigned long arg);
static int drv_gfbg_get_screen_size(struct fb_info *info, unsigned long arg);
static int drv_gfbg_get_vblank(struct fb_info *info, unsigned long arg);
static int drv_gfbg_get_colorkey(struct fb_info *info, unsigned long arg);
static int drv_gfbg_set_colorkey(struct fb_info *info, unsigned long arg);
static int drv_gfbg_get_layer_info(struct fb_info *info, unsigned long arg);
static int drv_gfbg_set_layer_info(struct fb_info *info, unsigned long arg);
static int drv_gfbg_refresh_layer(struct fb_info *info, unsigned long arg);
static int drv_gfbg_get_canvas_buffer(struct fb_info *info, unsigned long arg);
static int drv_gfbg_flip_surface(struct fb_info *info, unsigned long arg);
static int drv_gfbg_compress_layer(struct fb_info *info, unsigned long arg);

static drv_gfbg_ioctl_func_item g_drv_gfbg_ioctl_func[DRV_GFBG_IOCTL_FUNC_ITEM_NUM_MAX] = {
	{FBIOGET_SCREEN_ORIGIN_GFBG, drv_gfbg_get_screen_origin_pos},
	{FBIOPUT_SCREEN_ORIGIN_GFBG, drv_gfbg_set_screen_origin_pos},
	{FBIOGET_SHOW_GFBG, drv_gfbg_get_layer_show_state},
	{FBIOPUT_SHOW_GFBG, drv_gfbg_show_layer},
	{FBIOGET_SCREEN_SIZE, drv_gfbg_get_screen_size},
	{FBIOPUT_SCREEN_SIZE, drv_gfbg_set_screen_size},
	{FBIOGET_VER_BLANK_GFBG, drv_gfbg_get_vblank},
	{FBIOGET_COLORKEY_GFBG, drv_gfbg_get_colorkey},
	{FBIOPUT_COLORKEY_GFBG, drv_gfbg_set_colorkey},
	{FBIOGET_LAYER_INFO, drv_gfbg_get_layer_info},
	{FBIOPUT_LAYER_INFO, drv_gfbg_set_layer_info},
	{FBIO_REFRESH, drv_gfbg_refresh_layer},
	{FBIOGET_CANVAS_BUF, drv_gfbg_get_canvas_buffer},
	{FBIOFLIP_SURFACE, drv_gfbg_flip_surface},
	{FBIOPUT_COMPRESSION_GFBG, drv_gfbg_compress_layer},
};

static const struct fb_fix_screeninfo g_default_fix = {
	.id = "gfbg",                  /* String identifierString identifier */
	.type = FB_TYPE_PACKED_PIXELS, /* FB type */
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 1,
	.ypanstep = 1,
	.ywrapstep = 0,
	.line_length = GFBG_DEF_STRIDE,
	.accel = FB_ACCEL_NONE,
	.mmio_len = 0,
	.mmio_start = 0,
};

static const struct fb_var_screeninfo g_default_var = {
	.xres           = GFBG_DEF_WIDTH,
	.yres           = GFBG_DEF_HEIGHT,
	.xres_virtual   = GFBG_DEF_WIDTH,
	.yres_virtual   = GFBG_DEF_HEIGHT,
	.xoffset        = 0,
	.yoffset        = 0,
	.bits_per_pixel = GFBG_DEF_DEPTH,
	.red            = {16, 8, 0},
	.green          = {8, 8, 0},
	.blue           = {0, 8, 0},
	.transp         = {24, 8, 0},
	.activate       = FB_ACTIVATE_NOW,
	.pixclock       = -1, /* pixel clock in ps (pico seconds) */
	.left_margin    = 0, /* time from sync to picture   */
	.right_margin   = 0, /* time from picture to sync   */
	.upper_margin   = 0, /* time from sync to picture   */
	.lower_margin   = 0,
	.hsync_len      = 0, /* length of horizontal sync   */
	.vsync_len      = 0, /* length of vertical sync */
};

static gfbg_argb_bitinfo g_argb_bit_field[] = {
	/* ARGB8888 */
	{
		.red	= {16, 8, 0},
		.green	= {8, 8, 0},
		.blue	= {0, 8, 0},
		.transp	= {24, 8, 0},
	},
	/* ARGB4444 */
	{
		.red	= {8, 4, 0},
		.green	= {4, 4, 0},
		.blue	= {0, 4, 0},
		.transp	= {12, 4, 0},
	},
	/* ARGB1555 */
	{
		.red	= {10, 5, 0},
		.green	= {5, 5, 0},
		.blue	= {0, 5, 0},
		.transp	= {15, 1, 0},
	},
	/* 256 LUT, pseudo color */
	{
		.red	= {0, 8, 0},
		.green	= {0, 8, 0},
		.blue	= {0, 8, 0},
		.transp	= {0, 0, 0},
	},
	/* 16 LUT, pseudo color */
	{
		.red	= {0, 4, 0},
		.green	= {0, 4, 0},
		.blue	= {0, 4, 0},
		.transp	= {0, 0, 0},
	},
};

phys_addr_t gfbg_get_smem_start(const struct fb_info *info)
{
	return (phys_addr_t)info->fix.smem_start;
}

char *gfbg_get_screen_base(const struct fb_info *info)
{
	return (char *)info->screen_base;
}

unsigned int gfbg_get_smem_len(const struct fb_info *info)
{
	return (unsigned int)info->fix.smem_len;
}

unsigned int gfbg_get_line_length(const struct fb_info *info)
{
	return (unsigned int)info->fix.line_length;
}

unsigned int gfbg_get_xres(const struct fb_info *info)
{
	return (unsigned int)info->var.xres;
}

unsigned int gfbg_get_yres(const struct fb_info *info)
{
	return (unsigned int)info->var.yres;
}

unsigned int gfbg_get_xres_virtual(const struct fb_info *info)
{
	return (unsigned int)info->var.xres_virtual;
}

unsigned int gfbg_get_yres_virtual(const struct fb_info *info)
{
	return (unsigned int)info->var.yres_virtual;
}

unsigned int gfbg_get_bits_per_pixel(const struct fb_info *info)
{
	return (unsigned int)info->var.bits_per_pixel;
}

unsigned int gfbg_get_yoffset(const struct fb_info *info)
{
	return (unsigned int)info->var.yoffset;
}

unsigned int gfbg_get_xoffset(const struct fb_info *info)
{
	return (unsigned int)info->var.xoffset;
}

static int gfbg_open_check_param(const struct fb_info *info)
{
	gfbg_par *par = NULL;
	if ((info == NULL) || (info->par == NULL)) {
		return -1;
	}

	par = (gfbg_par *)info->par;
	if (par->layer_id >= GFBG_MAX_LAYER_NUM) {
		TRACE_GFBG(DBG_ERR, "layer %u is not supported!\n", par->layer_id);
		return -1;
	}

	return 0;
}

static inline bool gfbg_get_show(const gfbg_par *par)
{
	if (par)
		return par->show;
	else
		return false;
}

static inline void gfbg_set_show(gfbg_par *par, bool show)
{
	if (par)
		par->show = show;
}

static inline void gfbg_get_fmt(const gfbg_par *par, fb_color_format *color_format)
{
	*color_format = par->color_format;
}

static inline void gfbg_set_fmt(gfbg_par *par, fb_color_format color_fmt)
{
	par->color_format = color_fmt;
}

static void gfbg_set_bufmode(vo_layer layer_id, fb_layer_buf layer_buf_mode)
{
	struct fb_info *info = g_layer[layer_id].info;
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_refresh_info *refresh_info = &par->refresh_info;

	/* in 0 buf mode ,maybe the stride or format will be changed! */
	if ((refresh_info->buf_mode == FB_LAYER_BUF_NONE) && (refresh_info->buf_mode != layer_buf_mode)) {
		par->modifying = true;

		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_STRIDE;

		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_FMT;

		par->modifying = false;
	}

	refresh_info->buf_mode = layer_buf_mode;
}

static void gfbg_set_dispbufinfo(vo_layer layer_id)
{
	struct fb_info *info = g_layer[layer_id].info;
	gfbg_par *par = (gfbg_par *)(info->par);
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	gfbg_dispbuf_info *disp_buf_info = &refresh_info->disp_buf_info;
	unsigned int buf_size;

	buf_size = gfbg_get_line_length(info) * gfbg_get_yres(info);

	if (gfbg_get_smem_len(info) == 0) {
		return;
	} else if ((gfbg_get_smem_len(info) >= buf_size) && (gfbg_get_smem_len(info) < buf_size * 2)) { /* 2 alg data */
		disp_buf_info->phys_addr[0] = gfbg_get_smem_start(info);
		disp_buf_info->phys_addr[1] = gfbg_get_smem_start(info);
	} else if (gfbg_get_smem_len(info) >= buf_size * 2) { /* 2 alg data */
		disp_buf_info->phys_addr[0] = gfbg_get_smem_start(info);
		disp_buf_info->phys_addr[1] = gfbg_get_smem_start(info) + buf_size;
	}
}

int gfbg_drv_set_layer_data_fmt(vo_layer layer_id, fb_color_format data_format)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;
	enum disp_gop_format pixel_format_for_hal;

	switch (data_format) {
		case FB_FORMAT_ARGB8888: {
			pixel_format_for_hal = DISP_GOP_FMT_ARGB8888;
			break;
		}

		case FB_FORMAT_ARGB4444: {
			pixel_format_for_hal = DISP_GOP_FMT_ARGB4444;
			break;
		}

		case FB_FORMAT_ARGB1555: {
			pixel_format_for_hal = DISP_GOP_FMT_ARGB1555;
			break;
		}

		case FB_FORMAT_LUT_256: {
			pixel_format_for_hal = DISP_GOP_FMT_256LUT;
			break;
		}

		case FB_FORMAT_LUT_16: {
			pixel_format_for_hal = DISP_GOP_FMT_16LUT;
			break;
		}
		default: {
			TRACE_GFBG(DBG_ERR, "layer (%d) GFBG does not support this color format\n", layer_id);
			return -1;
		}
	}

	gfbg_hal_set_layer_data_fmt(dev_id, hal_layer_id, pixel_format_for_hal);

	return 0;
}

int gfbg_drv_set_layer_stride(vo_layer layer_id, unsigned int stride)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;
	struct fb_info *info = NULL;

	if (g_layer[layer_id].rot) {
		// For rotated layers, the stride is set to the width of the display
		info = g_layer[layer_id].info;
		stride = gfbg_get_yres(info) * 4;
	}
	gfbg_hal_set_layer_stride(dev_id, hal_layer_id, stride);

	return 0;
}

int gfbg_drv_set_layer_zoom(vo_layer layer_id, const fb_rect *input_rect, const fb_rect *output_rect, bool enable)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;
	bool w_need_zoom;
	bool h_need_zoom;
	struct fb_info *info = NULL;
	gfbg_par *par = NULL;
	gfbg_display_info *display_info = NULL;
	info = g_layer[layer_id].info;
	par = (gfbg_par *)(info->par);
	display_info = &par->display_info;

	if (!enable) {
		gfbg_hal_set_layer_zoom(dev_id, hal_layer_id, false, false, display_info, g_layer[layer_id].rot);
		return -1;
	}

	if (input_rect == NULL || output_rect == NULL) {
		return -1;
	}

	w_need_zoom = (input_rect->width != output_rect->width);
	h_need_zoom = (input_rect->height != output_rect->height);

	gfbg_hal_set_layer_zoom(dev_id, hal_layer_id, w_need_zoom, h_need_zoom, display_info, g_layer[layer_id].rot);

	return 0;
}

int gfbg_drv_set_layer_rect(vo_layer layer_id, const fb_rect *input_rect, const fb_rect *output_rect)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;
	fb_rect rect;
	struct fb_info *info = NULL;
	gfbg_par *par = NULL;
	gfbg_display_info *display_info = NULL;
	info = g_layer[layer_id].info;
	par = (gfbg_par *)(info->par);
	display_info = &par->display_info;
	UNUSED(output_rect);

	if (input_rect == NULL) {
		return -1;
	}

	rect.x = input_rect->x;
	rect.y = input_rect->y;
	rect.width = input_rect->width;
	rect.height = input_rect->height;

	gfbg_hal_set_layer_rect(dev_id, hal_layer_id, &rect, display_info);

	return 0;
}

int gfbg_drv_set_layer_addr(vo_layer layer_id, phys_addr_t addr)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;

	gfbg_hal_set_layer_addr(dev_id, hal_layer_id, addr);

	return 0;
}

int gfbg_drv_set_layer_enable(vo_layer layer_id, bool enable)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;

	gfbg_hal_set_layer_enable(enable, dev_id, hal_layer_id);

	return 0;
}

static int gfbg_drv_set_layer_colorkey(vo_layer layer_id, const gfbg_colorkeyex *colorkey)
{
	vo_dev dev_id = 0;
	vo_layer hal_layer_id = 0;

	gfbg_hal_set_layer_colorkey(dev_id, hal_layer_id, colorkey);

	return 0;
}

int gfbg_drv_close_odec(vo_layer layer_id)
{
	vo_dev dev_id = 0;

	gfbg_hal_close_odec(dev_id, layer_id);

	return 0;
}

int gfbg_drv_set_oenc_cfg(fb_color_format color_format, vo_layer layer_id, phys_addr_t display_addr)
{
	struct oenc_cfg oenc_cfg = {0};
	struct fb_info *info = NULL;
	struct oenc_size img_size = {0};
	u16 mem_width = 0;
	int index = 1 - g_layer[layer_id].index;

	info = g_layer[layer_id].info;
	img_size.w = (unsigned int)info->var.xres;
	img_size.h = (unsigned int)info->var.yres;

	oenc_cfg.cfg.raw = 0x80000000;
	oenc_cfg.src_picture_size = img_size;
	oenc_cfg.src_mem_size.h = img_size.h;
	oenc_cfg.fmt = color_format;

	switch (oenc_cfg.fmt) {
	case OENC_GOP_FMT_ARGB8888:
		mem_width = img_size.w * 4;
		break;
	case OENC_GOP_FMT_ARGB4444:
	case OENC_GOP_FMT_ARGB1555:
		mem_width = img_size.w * 2;
		break;
	case OENC_GOP_FMT_256LUT:
		mem_width = img_size.w;
		break;
	case OENC_GOP_FMT_16LUT:
		mem_width = img_size.w >> 1;
		break;
	default:
	TRACE_GFBG(DBG_ERR, "invalid fmt(%d)\n", oenc_cfg.fmt);
		return -1;
	}
	oenc_cfg.src_pitch = ALIGN(mem_width, 16);
	oenc_cfg.src_mem_size.w = ALIGN(mem_width, 16);

	oenc_cfg.src_adr = display_addr;
	oenc_cfg.bso_adr = g_layer[layer_id].compre_info[index].compre_paddr;

	base_ion_cache_invalidate(oenc_cfg.bso_adr, g_layer[layer_id].compre_info[index].compre_vaddr,
				  g_layer[layer_id].compre_info[index].compre_size);
	gfbg_hal_set_oenc_cfg(oenc_cfg);
	return 0;
}

static void callback_init_rect(fb_rect *in_rect, fb_rect *out_rect, const gfbg_display_info *display_info)
{
	unsigned int ratio_w;
	unsigned int ratio_h;

	in_rect->x = display_info->pos.x_pos;
	in_rect->y = display_info->pos.y_pos;
	in_rect->width = (int)display_info->display_width;
	in_rect->height = (int)display_info->display_height;

	out_rect->x = display_info->pos.x_pos;
	out_rect->y = display_info->pos.y_pos;
	out_rect->width = display_info->screen_width;
	out_rect->height = display_info->screen_height;

	ratio_w = out_rect->width / in_rect->width;
	ratio_h = out_rect->height / in_rect->height;

	if (in_rect->x + in_rect->width > (int)display_info->max_screen_width) {
		in_rect->width = (int)(display_info->max_screen_width - in_rect->x);
	}

	if (in_rect->y + in_rect->height > (int)display_info->max_screen_height) {
		in_rect->height = (int)(display_info->max_screen_height - in_rect->y);
	}

	/* after cut off, the output rectangle keep rate with input rectangle */
	if ((display_info->screen_width != 0) && (display_info->screen_height != 0)) {
		out_rect->width = ratio_w * in_rect->width;
		out_rect->height = ratio_h * in_rect->height;
	}
}

/* Does the chip support scaling */
static bool gfbg_check_imagezoomenable(vo_layer layer_id, const fb_rect *in_rect, const fb_rect *out_rect)
{
	bool need_zoom;
	need_zoom = (out_rect->width != in_rect->width || out_rect->height != in_rect->height);
	if (need_zoom == false) {
		return false;
	}

	/* The chip only support W x2 Scale. */
	if (out_rect->width != in_rect->width && (out_rect->width != (in_rect->width * GFBG_MAX_ZOOMIN))) {
		TRACE_GFBG(DBG_ERR,
			   "GFBG layer%d in_size(%d, %d) and out_size(%d, %d) do out of ZoomRatio[1, %d]!!\n",
			   layer_id, in_rect->width, in_rect->height,
			   out_rect->width, out_rect->height, GFBG_MAX_ZOOMIN);
		return false;
	}

	/* The chip only support H x2 Scale. */
	if (out_rect->height != in_rect->height && (out_rect->height != (in_rect->height * GFBG_MAX_ZOOMIN))) {
		TRACE_GFBG(DBG_ERR,
			   "GFBG layer%d in_size(%d, %d) and out_size(%d, %d) do out of ZoomRatio[1, %d]!!\n",
			   layer_id, in_rect->width, in_rect->height,
			   out_rect->width, out_rect->height, GFBG_MAX_ZOOMIN);
		return false;
	}

	if (need_zoom && (in_rect->width > GFBG_LINE_BUF)) {
		TRACE_GFBG(DBG_ERR,
			   "GFBG layer%d in width: %u is bigger than %d, will not zoom in!!\n",
			   layer_id, in_rect->width, GFBG_LINE_BUF);
		return false;
	}

	return true;
}

static void callback_modify_colorkey(gfbg_par *par, vo_layer layer_id)
{
	if (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_COLORKEY) {
		gfbg_drv_set_layer_colorkey(layer_id, &par->ckey);
		par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_COLORKEY;
	}
}

static void callback_modify_format(gfbg_par *par, vo_layer layer_id)
{
	gfbg_refresh_info *refresh_info = NULL;
	refresh_info = &par->refresh_info;

	if (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_FMT) {
		if ((refresh_info->buf_mode == FB_LAYER_BUF_NONE) && refresh_info->user_buffer.canvas.phys_addr) {
			gfbg_drv_set_layer_data_fmt(layer_id, refresh_info->user_buffer.canvas.format);
		} else {
			gfbg_drv_set_layer_data_fmt(layer_id, par->color_format);
		}
		par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_FMT;
	}
}

static void callback_modify_stride(gfbg_par *par, vo_layer layer_id)
{
	struct fb_info *info = NULL;
	gfbg_refresh_info *refresh_info = NULL;
	refresh_info = &par->refresh_info;
	info = g_layer[layer_id].info;

	if (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_STRIDE) {
		if ((refresh_info->buf_mode == FB_LAYER_BUF_NONE) && refresh_info->user_buffer.canvas.phys_addr) {
			gfbg_drv_set_layer_stride(layer_id, refresh_info->user_buffer.canvas.pitch);
		} else {
			gfbg_drv_set_layer_stride(layer_id, gfbg_get_line_length(info));
		}
		par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_STRIDE;
	}
}

static void callback_modify_sizes(gfbg_par *par, vo_layer layer_id)
{
	gfbg_display_info *display_info = NULL;
	fb_rect in_rect = {0};
	fb_rect out_rect = {0};

	display_info = &par->display_info;
	/* Handles requests to modify input and output sizes. */
	if ((par->param_modify_mask & GFBG_LAYER_PARAMODIFY_INRECT) ||
	    (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_OUTRECT)) {
		/* for init rect */
		callback_init_rect(&in_rect, &out_rect, display_info);
		/* The chip only support W,H x2 Scale. */
		if (gfbg_check_imagezoomenable(layer_id, &in_rect, &out_rect) == true) {
			/*
			* If you want to go through the zoom module, you need to correct it to 2 alignment,
			* otherwise it will appear abnormal.
			*/

			gfbg_drv_set_layer_rect(layer_id, &in_rect, &out_rect);

			gfbg_drv_set_layer_zoom(layer_id, &in_rect, &out_rect, true);
		} else {
			/*
			* If scaling is not enabled,
			* the input size is used as the output size and the zoom module is closed.
			*/
			gfbg_drv_set_layer_rect(layer_id, &in_rect, &in_rect);

			gfbg_drv_set_layer_zoom(layer_id, &in_rect, &out_rect, false);
		}

		/* Processing completed, clear mask */
		par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_INRECT;
		par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_OUTRECT;
	}
}

static void callback_update_refresh_info(gfbg_par *par, vo_layer layer_id)
{
	// gfbg_refresh_info *refresh_info = NULL;
	// struct fb_info *info = NULL;
	// unsigned int index, buf_size;

	// refresh_info = &par->refresh_info;
	// index = refresh_info->disp_buf_info.index_for_int;
	// info = g_layer[layer_id].info;
	// buf_size = ((gfbg_get_line_length(info) * gfbg_get_yres(info)) + GFBG_ALIGNMENT) & (~GFBG_ALIGNMENT);
	// if ((refresh_info->buf_mode == FB_LAYER_BUF_DOUBLE) && (refresh_info->disp_buf_info.need_flip == true)) {
	/* Work buf to change to free buf. Take free buf to display */
		// index = 1 - index;
		// refresh_info->disp_buf_info.index_for_int = index;
		/*
		 * The display address is set to the address of the free buf,
		 * which is set to the screen buf address differently from 0buf
		 */
	//	gfbg_drv_set_layer_addr(layer_id, refresh_info->disp_buf_info.phys_addr[index]);
	//	refresh_info->screen_addr = refresh_info->disp_buf_info.phys_addr[index];

	//	info->var.yoffset = div_u64((refresh_info->disp_buf_info.phys_addr[index] - gfbg_get_smem_start(info)),
	//				    gfbg_get_line_length_ex(info));
	//	if ((gfbg_get_line_length(info) != 0) && ((gfbg_get_bits_per_pixel(info)>> 3) != 0)) { /* 3 is 8 bits */
	//	    info->var.xoffset = ((unsigned long)(refresh_info->disp_buf_info.phys_addr[index] -
	//				 gfbg_get_smem_start(info)) %
	//				 gfbg_get_line_length_ex(info)) /
	//				 (gfbg_get_bits_per_pixel(info)>> 3); /* 3 is 8 bits */
	//	}
	//	refresh_info->disp_buf_info.fliped = true;
	//	refresh_info->disp_buf_info.need_flip = false;
	//	refresh_info->disp_buf_info.int_pic_num++;
	// }
}

static void callback_modify_address(gfbg_par *par, vo_layer layer_id)
{
	gfbg_refresh_info *refresh_info = NULL;
	gfbg_display_info *display_info = NULL;
	struct fb_info *info = NULL;

	refresh_info = &par->refresh_info;
	display_info = &par->display_info;
	info = g_layer[layer_id].info;
	/* The display address is refreshed and the display address is modified. */
	if (!(par->param_modify_mask & GFBG_LAYER_PARAMODIFY_DISPLAYADDR)) {
		/* according to the index, decide which buf set to the screen */
		callback_update_refresh_info(par, layer_id);
		return;
	}

	gfbg_drv_close_odec(layer_id);
	gfbg_drv_set_layer_addr(layer_id, refresh_info->screen_addr);

	par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_DISPLAYADDR;

	if ((refresh_info->disp_buf_info.phys_addr[0] != refresh_info->disp_buf_info.phys_addr[1]) &&
	    (refresh_info->disp_buf_info.phys_addr[0])) {
		if (refresh_info->screen_addr >=  refresh_info->disp_buf_info.phys_addr[0] &&
		    refresh_info->screen_addr < refresh_info->disp_buf_info.phys_addr[1]) {
			refresh_info->disp_buf_info.index_for_int = 0;
		} else if ((refresh_info->screen_addr >= refresh_info->disp_buf_info.phys_addr[1]) &&
			   (refresh_info->screen_addr < (refresh_info->disp_buf_info.phys_addr[0] +
			   gfbg_get_smem_len(info)))) {
			refresh_info->disp_buf_info.index_for_int = 1;
		}
	}
	/* according to the index, decide which buf set to the screen */
	callback_update_refresh_info(par, layer_id);
}

static int vo_callback_process(gfbg_par *par, vo_layer layer_id, bool *is_continue)
{
	unsigned long lock_flag;

	spin_lock_irqsave(&par->lock, lock_flag);

	/* Non-modified status, modified, can be modified */
	if (!par->modifying) {
		/*
		* set graphic hw layer enable
		*/
		if (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_SHOW) {
			gfbg_drv_set_layer_enable(layer_id, par->show);
			par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_SHOW;
		}

		/* If not displayed, close the graphics layer and exit */
		if (par->show == false) {
			*is_continue = false;
			spin_unlock_irqrestore(&par->lock, lock_flag);
			return 0;
		}

		/*
		* set color key
		*/
		callback_modify_colorkey(par, layer_id);

		/*
		* set format
		*/
		callback_modify_format(par, layer_id);

		/*
		* set stride
		*/
		callback_modify_stride(par, layer_id);

		/*
		* Handles requests to modify input and output sizes
		*/
		callback_modify_sizes(par, layer_id);

		/*
		* The display address is refreshed and the display address is modified
		*/
		if (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_COMPRESS) {
			if (g_layer[layer_id].compre_info[g_layer[layer_id].index].oenc_cfg.bso_adr) {
				gfbg_drv_set_layer_enable(layer_id, true);
				gfbg_hal_gop_odec_set_cfg_from_oenc(layer_id,
					g_layer[layer_id].compre_info[g_layer[layer_id].index].oenc_cfg);
			} else {
				gfbg_drv_set_layer_enable(layer_id, false);
				spin_unlock_irqrestore(&par->lock, lock_flag);
				return 0;
			}
		} else {
			callback_modify_address(par, layer_id);
		}

	}

	*is_continue = par->show;
	spin_unlock_irqrestore(&par->lock, lock_flag);
	return 0;
}

/* Callback function for VO vertical timing interrupt */
int gfbg_interrupt_process(vo_layer layer_id)
{
	struct fb_info *info = NULL;
	gfbg_par *par = NULL;
	bool is_continue = false;

	info = g_layer[layer_id].info;
	if (info == NULL) {
		return -1;
	}
	if (info->par == NULL) {
		return -1;
	}
	par = (gfbg_par *)(info->par);
	if (vo_callback_process(par, layer_id, &is_continue) != 0) {
		return -1;
	}
	if (is_continue != true) {
		return 0;
	}

	/* Field blanking mark */
	par->vblflag = 1;
	wake_up_interruptible(&(par->vbl_event));
	return 0;
}

static void gfbg_open_init_display(struct fb_info *info, struct disp_timing *timing)
{
	gfbg_par *par = NULL;
	gfbg_display_info *display_info = NULL;
	par = (gfbg_par *)info->par;

	atomic_set(&par->ref_count, 0);

	info->var.xres = timing->hfde_end - timing->hfde_start + 1;
	info->var.yres = timing->vfde_end - timing->vfde_start + 1;
	if (g_layer[par->layer_id].rot) {
		info->var.xres = timing->vfde_end - timing->vfde_start + 1;
		info->var.yres = timing->hfde_end - timing->hfde_start + 1;
	}
	info->var.xres_virtual = VXRES_SIZE(info->var.xres, info->var.bits_per_pixel);
	info->var.yres_virtual = info->var.yres;
	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.activate |= FB_ACTIVATE_TEST;
	info->var.pixclock = timing->htotal * timing->vtotal * 60;
	info->var.left_margin = timing->hfde_start - timing->hsync_end;
	info->var.right_margin = timing->htotal - timing->hfde_end;
	info->var.upper_margin = timing->vfde_start - timing->vsync_end;
	info->var.lower_margin = timing->vtotal - timing->vfde_end;
	info->var.hsync_len = timing->hsync_end - timing->hsync_start;
	info->var.vsync_len = timing->vsync_end - timing->vsync_start;
	if (g_layer[par->layer_id].rot) {
		// Flip horizontal and vertical timing assignments for rotation
		info->var.left_margin = timing->vfde_start - timing->vsync_end;
		info->var.right_margin = timing->vtotal - timing->vfde_end;
		info->var.upper_margin = timing->hfde_start - timing->hsync_end;
		info->var.lower_margin = timing->htotal - timing->hfde_end;
		info->var.hsync_len = timing->vsync_end - timing->vsync_start;
		info->var.vsync_len = timing->hsync_end - timing->hsync_start;
	}
	info->var.sync &= ~(FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT);
	info->var.vmode = FB_VMODE_NONINTERLACED;
	info->var.activate &= ~FB_ACTIVATE_TEST;
	info->fix.line_length = FB_LINE_SIZE(info->var.xres_virtual, info->var.bits_per_pixel);

	display_info = &par->display_info;
	display_info->display_width    = gfbg_get_xres(info);
	display_info->display_height   = gfbg_get_yres(info);
	display_info->screen_width     = gfbg_get_xres(info);
	display_info->screen_height    = gfbg_get_yres(info);
	display_info->vir_x_res         = gfbg_get_xres_virtual(info);
	display_info->vir_y_res         = gfbg_get_yres_virtual(info);
	display_info->x_res            = gfbg_get_xres(info);
	display_info->y_res            = gfbg_get_yres(info);
	display_info->max_screen_width  = gfbg_get_xres(info);
	display_info->max_screen_height = gfbg_get_yres(info);

	TRACE_GFBG(DBG_INFO, "Devices bound to layer (%d) current resolution (%dx%d)\n", par->layer_id,
		   display_info->max_screen_width, display_info->max_screen_height);

	init_waitqueue_head(&(par->vbl_event));
	init_waitqueue_head(&(par->do_refresh_job));
}

static void gfbg_open_init_config(const struct fb_info *info, vo_layer layer_id)
{
	gfbg_par *par = NULL;
	unsigned long lock_flag;
	par = (gfbg_par *)info->par;

	spin_lock_irqsave(&par->lock, lock_flag);
	gfbg_set_bufmode(par->layer_id, FB_LAYER_BUF_BUTT);
	gfbg_set_dispbufinfo(layer_id);
	spin_unlock_irqrestore(&par->lock, lock_flag);
}

static void gfbg_open_init_finish(const struct fb_info *info)
{
	gfbg_par *par = NULL;
	unsigned long lock_flag;

	par = (gfbg_par *)info->par;

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;
	gfbg_set_show(par, true);
	par->param_modify_mask = GFBG_LAYER_PARAMODIFY_SHOW;
	par->modifying = false;
	par->layer_open = true;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	gfbg_drv_set_layer_enable(par->layer_id, true);
#if !defined(CONFIG_DUAL_OS)
	vo_gfbg_set_layer_enable(par->layer_id, true);
#endif
}

static int gfbg_open_start(struct fb_info *info)
{
	gfbg_par *par = NULL;
	vo_layer layer_id = 0;
	vo_dev dev_id = 0;
	// int ret = 0;
	bool is_enable = true;
	struct disp_timing timing;

	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	if (!atomic_read(&par->ref_count)) {
		// ret = vo_gfbg_get_bind_dev_id(layer_id, &dev_id);
		// if (ret != 0) {
		// 	TRACE_GFBG(DBG_ERR, "layer (%d) get bind dev id failure\n", layer_id);
		// 	return ret;
		// }

		/*The cvi_mpi layer controls the vo enable*/
		// ret = vo_gfbg_dev_is_enable(dev_id, &is_enable);
		// if (ret != 0) {
		// 	TRACE_GFBG(DBG_ERR, "check dev(%d) enable failure\n", dev_id);
		// 	return ret;
		// }

		if (is_enable) {
			gfbg_get_disp_hw_timing(dev_id, &timing);

			/* Initialize the display information in private data */
			gfbg_open_init_display(info, &timing);

			/* Initialization lock */
			spin_lock_init(&par->lock);

			/* gfbg set bufmode\alpha\displaybufinfo\fmt */
			gfbg_open_init_config(info, layer_id);

			gfbg_open_init_finish(info);
		} else {
			TRACE_GFBG(DBG_ERR, "vo_dev(%d) is not enable!\n", dev_id);
			return -1;
		}
	}

	return 0;
}

static int gfbg_free_canvas_buf(gfbg_par *par)
{
	fb_surface *canvas_sur = NULL;
	if (par == NULL) {
		return -1;
	}
	canvas_sur = &par->canvas_sur;

	if (canvas_sur->phys_addr != 0) {
		base_ion_free(canvas_sur->phys_addr);
	}
	canvas_sur->phys_addr = 0;

	return 0;
}

#if defined(CONFIG_DUAL_OS)
static int gfbg_vo_irq_handler(int irq, void *data)
{
    struct gfbg_vo_dev *gfbg_vdev = data;
    union disp_intr intr_status = {0};

    intr_status.b.disp_frame_end = 1;

    gfbg_vo_int_cb(gfbg_vdev->dev_id, gfbg_vdev->layer_id, intr_status);

    return IRQ_HANDLED;
}
#endif

static int gfbg_open(struct fb_info *info, int user)
{
	int ret = 0;
	gfbg_par *par = NULL;
	UNUSED(user);

	/* check input param */
	ret = gfbg_open_check_param(info);
	if (ret != 0) {
		return ret;
	}

	par = (gfbg_par *)info->par;

	/* open the layer first */
	ret = gfbg_open_start(info);
	if (ret != 0) {
		return ret;
	}

	/* increase reference count */
	atomic_inc(&par->ref_count);

#if defined(CONFIG_DUAL_OS)
	if (atomic_cmpxchg(&gfbg_vdev->irq_requested, 0, 1) == 0) {
		ret = osal_irq_request(gfbg_vdev->irq, gfbg_vo_irq_handler, 0,
			"disp", (void *)gfbg_vdev);
		if (ret) {
			TRACE_GFBG(DBG_ERR, "Failed to request IRQ %d\n", gfbg_vdev->irq);
			atomic_set(&gfbg_vdev->irq_requested, 0);
			return ret;
		}
	}
#endif
	return 0;
}

static int gfbg_release(struct fb_info *info, int user)
{
	gfbg_par *par = NULL;
	unsigned int mem_len;
	char *screen_base = NULL;
	int i = 0;
	UNUSED(user);

	if ((info == NULL) || (info->par == NULL)) {
		return -1;
	}

	par = (gfbg_par *)info->par;

	if (atomic_dec_and_test(&par->ref_count)) {
		gfbg_set_show(par, false);
		gfbg_drv_set_layer_enable(par->layer_id, false);
#if !defined(CONFIG_DUAL_OS)
		vo_gfbg_set_layer_enable(par->layer_id, false);
#endif
		gfbg_free_canvas_buf(par);
		par->layer_open = false;
		par->param_modify_mask = 0;

		screen_base = gfbg_get_screen_base(info);
		mem_len = gfbg_get_smem_len(info);
		if ((screen_base != NULL) && (mem_len != 0)) {
			(void)memset(screen_base, 0, mem_len);
		}
	}

	for (i = 0; i < 2; i++) {
		if (g_layer[par->layer_id].compre_info[i].compre_paddr != 0) {
			osal_usleep_range(50 * 1000, 50 * 2000); // wait for oenc finish
			base_ion_free(g_layer[par->layer_id].compre_info[i].compre_paddr);
			g_layer[par->layer_id].compre_info[i].compre_paddr = 0;
			g_layer[par->layer_id].compre_info[i].compre_vaddr = NULL;
			g_layer[par->layer_id].compre_info[i].oenc_cfg.bso_adr = 0;
		}
		if (g_layer[par->layer_id].tde_info[1].tde_paddr != 0) {
			base_ion_free(g_layer[par->layer_id].tde_info[i].tde_paddr);
			g_layer[par->layer_id].tde_info[i].tde_paddr = 0;
			g_layer[par->layer_id].tde_info[i].tde_vaddr = NULL;
			g_layer[par->layer_id].tde_info[i].tde_size = 0;
		}
	}

#if defined(CONFIG_DUAL_OS)
	if (atomic_cmpxchg(&gfbg_vdev->irq_requested, 1, 0) == 1) {
		osal_irq_free(gfbg_vdev->irq, (void *)gfbg_vdev);
	}
#endif
	return 0;
}

static int gfbg_bitfieldcmp(struct fb_bitfield x, struct fb_bitfield y)
{
	if ((x.offset == y.offset) && (x.length == y.length) && (x.msb_right == y.msb_right))
		return 0;
	else
		return -1;
}

static unsigned int gfbg_getbppbyfmt(fb_color_format color_fmt)
{
	switch (color_fmt) {
	case FB_FORMAT_ARGB4444:
	case FB_FORMAT_ARGB1555:
		return 16; /* 16 bit width */
	case FB_FORMAT_ARGB8888:
		return 32; /* 32 bit width */
	case FB_FORMAT_LUT_256:
		return 8; /* 8 bit width */
	case FB_FORMAT_LUT_16:
		return 4; /* 4 bit width */
	default:
		return 0;
	}
}

static fb_color_format gfbg_getfmtbyargb(const struct fb_bitfield *red, const struct fb_bitfield *green,
					 const struct fb_bitfield *blue, const struct fb_bitfield *transp,
					 unsigned int color_depth)
{
	unsigned int i;
	unsigned int bpp;

	if ((red == NULL) || (green == NULL) || (blue == NULL) || (transp == NULL)) {
		return  FB_FORMAT_BUTT;
	}

	/*
	* Find the pixel format (gfbg_argb_bitinfo) corresponding to the given red,
	* green, and blue bit field information and the number of bits per pixel (bpp)
	*/
	for (i = 0; i < sizeof(g_argb_bit_field) / sizeof(gfbg_argb_bitinfo); i++) {
		if ((gfbg_bitfieldcmp(*red, g_argb_bit_field[i].red) == 0) &&
		    (gfbg_bitfieldcmp(*green, g_argb_bit_field[i].green) == 0) &&
		    (gfbg_bitfieldcmp(*blue, g_argb_bit_field[i].blue) == 0) &&
		    (gfbg_bitfieldcmp(*transp, g_argb_bit_field[i].transp) == 0)) {
			bpp = gfbg_getbppbyfmt(i);
			if (bpp == color_depth) {
				return i;
			}
		}
	}
	i = FB_FORMAT_BUTT;
	return i;
}

#define TRACE_LAYER_FORMAT(par, var) \
	TRACE_GFBG(DBG_ERR, \
		   "layer (%d) Unknown format(offset, length) " \
		   "r:(%d, %d, %d) " \
		   "g:(%d, %d, %d) " \
		   "b:(%d, %d, %d) " \
		   "a:(%d, %d, %d) " \
		   "bpp:%d!\n", \
		   (par)->layer_id, \
		   (var)->red.offset, (var)->red.length, (var)->red.msb_right, \
		   (var)->green.offset, (var)->green.length, (var)->green.msb_right, \
		   (var)->blue.offset, (var)->blue.length, (var)->blue.msb_right, \
		   (var)->transp.offset, (var)->transp.length, (var)->transp.msb_right, \
		   (var)->bits_per_pixel)

static int gfbg_check_fmt(struct fb_var_screeninfo *var, const struct fb_info *info)
{
	fb_color_format format;
	gfbg_par *par = NULL;
	par = (gfbg_par *)info->par;

	format = gfbg_getfmtbyargb(&var->red, &var->green, &var->blue, &var->transp, var->bits_per_pixel);
	if (format == FB_FORMAT_BUTT) {
		TRACE_LAYER_FORMAT(par, var);
		return -EINVAL;
	}

	return 0;
}

static int gfbg_check_virtual_resolution(const struct fb_var_screeninfo *var, const struct fb_info *info)
{
	gfbg_par *par = NULL;
	vo_layer layer_id;
	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	if (var->xres < GFBG_MIN_WIDTH) {
		TRACE_GFBG(DBG_ERR, "xres(%d) of layer_id %d can't be less than min_width(%d)\n",
			   var->xres, layer_id, GFBG_MIN_WIDTH);
		return -1;
	}
	if (var->yres < GFBG_MIN_HEIGHT) {
		TRACE_GFBG(DBG_ERR, "yres(%d) of layer_id %d can't be less than min_height(%d)\n",
			   var->yres, layer_id, GFBG_MIN_HEIGHT);
		return -1;
	}

	if (var->xres > var->xres_virtual) {
		TRACE_GFBG(DBG_ERR, "xres(%d) of layer_id %d should be less than xres_virtual(%d)\n",
			   var->xres, layer_id, var->xres_virtual);
		return -1;
	}
	if (var->yres > var->yres_virtual) {
		TRACE_GFBG(DBG_ERR, "yres(%d) of layer_id %d should be less than yres_virtual(%d)\n",
			   var->yres, layer_id, var->yres_virtual);
		return -1;
	}
	return 0;
}

static int gfbg_check_offset(const struct fb_var_screeninfo *var, const struct fb_info *info)
{
	gfbg_par *par = NULL;
	vo_layer layer_id;
	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	if ((var->xoffset + var->xres > var->xres_virtual) || (var->xoffset > var->xres_virtual)) {
		TRACE_GFBG(DBG_ERR,
			   "the sum of layer%d's xoffset(%d) and xres(%d) should be less than xres_virtual(%d)\n",
			   layer_id, var->xoffset, var->xres, var->xres_virtual);
		return -EINVAL;
	}

	if ((var->yoffset + var->yres > var->yres_virtual) || (var->yoffset > var->yres_virtual)) {
		TRACE_GFBG(DBG_ERR,
			   "the sum of layer%d's yoffset(%d) and yres(%d) should be less than yres_virtual(%d)\n",
			   layer_id, var->yoffset, var->yres, var->yres_virtual);
		return -EINVAL;
	}
	return 0;
}

static int gfbg_check_total(const struct fb_var_screeninfo *var, const struct fb_info *info)
{
	unsigned int hor_total;
	unsigned int ver_total;
	gfbg_par *par = NULL;
	vo_layer layer_id;
	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	hor_total = var->left_margin + var->xres + var->right_margin + var->hsync_len;
	if (hor_total == 0) {
		TRACE_GFBG(DBG_ERR,
			   "the sum of layer%d's left_margin(%d),xres(%d),right_margin(%d),hsync_len(%d) can't be 0\n",
			   layer_id, var->left_margin, var->xres, var->right_margin, var->hsync_len);
		return -1;
	}
	ver_total = var->yres + var->lower_margin + var->vsync_len + var->upper_margin;
	if (ver_total == 0) {
		TRACE_GFBG(DBG_ERR,
			   "the sum of layer%d's left_margin(%d),xres(%d),right_margin(%d),hsync_len(%d) can't be 0\n",
			   layer_id, var->upper_margin, var->yres, var->lower_margin, var->vsync_len);
		return -1;
	}
	return 0;
}

static int gfbg_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	gfbg_par *par = NULL;
	unsigned int expected_len;
	vo_layer layer_id;

	TRACE_GFBG(DBG_INFO, "gfbg_check_var - 1\n");

	if ((info == NULL) || (var == NULL) || (info->par == NULL)) {
		return -1;
	}

	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	TRACE_GFBG(DBG_INFO, "gfbg_check_var - 2\n");

	if (gfbg_check_fmt(var, info) != 0) {
		TRACE_GFBG(DBG_ERR, "layer (%d) unsupport format!\n", layer_id);
		return -EINVAL;
	}

	TRACE_GFBG(DBG_INFO, "gfbg_check_var - 3\n");

	/*
	* for virtual resolution check
	* virtual resolution can't be less than minimal resolution
	*/
	if (gfbg_check_virtual_resolution(var, info) != 0) {
		return -1;
	}

	TRACE_GFBG(DBG_INFO, "gfbg_check_var - 4\n");
	/* check if the offset is valid */
	if (gfbg_check_offset(var, info) != 0) {
		return -1;
	}

	TRACE_GFBG(DBG_INFO, "gfbg_check_var - 5\n");
	/*
	* for hor_total and ver_total check
	* The FB driver in the Linux kernel will use u32HTotal and u32VTotal as divisors
	* so they cannot be 0
	*/
	if (gfbg_check_total(var, info) != 0) {
		return -1;
	}

	TRACE_GFBG(DBG_INFO, "layer (%d) xres:%d, yres:%d, xres_virtual:%d, yres_virtual:%d\n",
		   layer_id, var->xres, var->yres, var->xres_virtual, var->yres_virtual);
	/* for mem len check */
	expected_len = var->yres_virtual * ((((var->xres_virtual * var->bits_per_pixel) >> 3) + /* 8 bit (2^3) */
		GFBG_ALIGNMENT) & (~GFBG_ALIGNMENT));

	if (info->fix.smem_len && (expected_len > info->fix.smem_len)) {
		TRACE_GFBG(DBG_ERR, "layer (%d) don't has enough mem! expected: %d KBytes, real:%d KBytes\n",
			   layer_id, expected_len / 1024, info->fix.smem_len / 1024); /* 1024 for KB */
		return -EINVAL;
	}

	/* Interlaced mode not supported */
	if (var->vmode & FB_VMODE_INTERLACED)
		return -EINVAL;

	return 0;
}

static void set_par_stride(struct fb_info *info)
{
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_display_info *display_info = &par->display_info;
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	unsigned int stride;
	phys_addr_t display_addr;

	stride = (((gfbg_get_xres_virtual(info) * gfbg_get_bits_per_pixel(info)) >> 3) + GFBG_ALIGNMENT) &
		(~GFBG_ALIGNMENT);
	if (stride != gfbg_get_line_length(info) || (gfbg_get_yres(info) != display_info->y_res)) {
		info->fix.line_length = stride;
		gfbg_set_dispbufinfo(par->layer_id);
		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_STRIDE;
	}

	display_addr = (gfbg_get_smem_start(info) + stride * gfbg_get_yoffset(info) +
			gfbg_get_xoffset(info) * (gfbg_get_bits_per_pixel(info) >> 3)) &
			0xfffffff0; /* 0xfffffff0 16 align */
	if (display_addr != refresh_info->screen_addr) {
		refresh_info->screen_addr = display_addr;
		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_DISPLAYADDR;
	}
}

static void gfbg_get_maxscreensize(gfbg_par *par, unsigned int *width, unsigned int *height)
{
	gfbg_display_info *display_info = NULL;
	if (par == NULL) {
		return;
	}
	display_info = &par->display_info;

	if ((width != NULL) && (height != NULL)) {
		*width = display_info->max_screen_width;
		*height = display_info->max_screen_height;
	}
}

static int gfbg_check_mem_enough(const struct fb_info *info, unsigned int pitch, unsigned int height)
{
	unsigned int buffer_num = 0;
	unsigned int buffer_size;
	gfbg_par *par = NULL;
	gfbg_refresh_info *refresh_info = NULL;
	if (info == NULL) {
		return -1;
	}
	if (info->par == NULL) {
		return -1;
	}
	par = (gfbg_par *)info->par;
	refresh_info = &par->refresh_info;

	switch (refresh_info->buf_mode) {
	case FB_LAYER_BUF_DOUBLE:
	case FB_LAYER_BUF_DOUBLE_IMMEDIATE:
		buffer_num = 2; /* 2 buffer num */
		break;

	case FB_LAYER_BUF_ONE:
		buffer_num = 1;
		break;

	default:
		return 0;
	}
	/* The interface setting requires uBuffersize, the actual memory size info->fix.smem_len */
	buffer_size = buffer_num * pitch * height;
	if (gfbg_get_smem_len(info) >= buffer_size) {
		return 0;
	}
	TRACE_GFBG(DBG_ERR, "memory is not enough!  now is %d u32Pitch %d u32Height %d expect %d\n",
		   gfbg_get_smem_len(info), pitch, height, buffer_size);
	return -1;
}

static int gfbg_disp_check_param(const struct fb_info *info, unsigned int width, unsigned int height)
{
	gfbg_par *par = (gfbg_par *)info->par;
	fb_size max_screen_size = {0};
	unsigned int pitch;

	gfbg_get_maxscreensize(par, &max_screen_size.width, &max_screen_size.height);

	if (width % 2 || height % 2) { /* 2 for align */
		TRACE_GFBG(DBG_ERR, "layer (%d) display size(%u, %u) should align to 2!\n",
			   par->layer_id, width, height);
		return -1;
	}

	/* 3 is 8 bits */
	pitch = (((width * gfbg_get_bits_per_pixel(info)) >> 3) + GFBG_ALIGNMENT) & (~GFBG_ALIGNMENT);
	if (gfbg_check_mem_enough(info, pitch, height) == -1) {
		TRACE_GFBG(DBG_ERR, "layer (%d) memory is not enough!\n", par->layer_id);
		return -1;
	}
	return 0;
}

static void gfbg_fill_var_info(gfbg_display_info *display_info, struct fb_var_screeninfo *var)
{
	var->xres = display_info->display_width;
	var->yres = display_info->display_height;
	if (var->xres_virtual < display_info->display_width) {
		var->xres_virtual = display_info->display_width;
	}

	if (var->yres_virtual < display_info->display_height) {
		var->yres_virtual = display_info->display_height;
	}
}

static int gfbg_set_disp_size(vo_layer layer_id, unsigned int width, unsigned int height)
{
	struct fb_info *info = g_layer[layer_id].info;
	gfbg_par *par = (gfbg_par *)info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_fix_screeninfo *fix = &info->fix;
	gfbg_display_info *display_info = &par->display_info;
	unsigned int pitch;
	unsigned long lock_flag;

	spin_lock_irqsave(&par->lock, lock_flag);
	if ((display_info->display_width == width) && (display_info->display_height == height)) {
		spin_unlock_irqrestore(&par->lock, lock_flag);
		return 0;
	}
	/*
	* for width and height check
	* width and height should less than max_screen_size
	*/
	if (gfbg_disp_check_param(info, width, height) != 0) {
		spin_unlock_irqrestore(&par->lock, lock_flag);
		return -1;
	}

	display_info->display_width = width;
	display_info->display_height = height;
	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_INRECT;
	pitch = (((width * gfbg_get_bits_per_pixel(info)) >> 3) + GFBG_ALIGNMENT) & (~GFBG_ALIGNMENT);
	if (pitch > fix->line_length) {
		fix->line_length = pitch;
		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_STRIDE;
	}
	/*
	* If the user calls FBIOPUT_LAYER_INFO to set display_width and display_height,then sync to xres yres,
	* Otherwise, there will be an error in the memory address in gfbg_set_dispbufinfo.
	*/
	gfbg_fill_var_info(display_info, var);
	gfbg_set_dispbufinfo(layer_id);

	spin_unlock_irqrestore(&par->lock, lock_flag);

	return 0;
}

/*
 * we handle it by two case:
 * case 1 : if VO support Zoom, we only change screen size, g_display size keep not change
 * case 2: if VO can't support zoom, g_display size should keep the same as screen size
 */
static int gfbg_set_screen_size(vo_layer layer_id, unsigned int width, unsigned int height)
{
	struct fb_info *info = g_layer[layer_id].info;
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_display_info *display_info = &par->display_info;

	display_info->screen_width = width;
	display_info->screen_height = height;

	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_OUTRECT;

	return 0;
}

static int set_par_resolution(const struct fb_info *info)
{
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_display_info *display_info = &par->display_info;
	unsigned long lock_flag;

	spin_lock_irqsave(&par->lock, lock_flag);
	if (gfbg_get_xres(info) != display_info->x_res || gfbg_get_yres(info) != display_info->y_res) {
		if ((gfbg_get_xres(info) == 0) || (gfbg_get_yres(info) == 0)) {
			if (par->show == true) {
				par->show = false;
				par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_SHOW;
			}
		}

		/*
		* The following two functions have a sleep operation, you must unlock before calling,
		* and lock the global variable inside the function.
		*/
		spin_unlock_irqrestore(&par->lock, lock_flag);
		if (gfbg_set_disp_size(par->layer_id, gfbg_get_xres(info), gfbg_get_yres(info)) != 0) {
			return -1;
		}
		if (gfbg_set_screen_size(par->layer_id, gfbg_get_xres(info), gfbg_get_yres(info)) != 0) {
			return -1;
		}

		spin_lock_irqsave(&par->lock, lock_flag);
	}

	spin_unlock_irqrestore(&par->lock, lock_flag);
	return 0;
}

static int gfbg_set_par(struct fb_info *info)
{
	// int ret = 0;
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_display_info *display_info = &par->display_info;
	fb_color_format format;
	unsigned long lock_flag;

	spin_lock_irqsave(&par->lock, lock_flag);

	par->modifying = true;

	/* set the stride if stride change */
	set_par_stride(info);

	spin_unlock_irqrestore(&par->lock, lock_flag);

	/* If xres or yres change */
	if (set_par_resolution(info) != 0) {
		return -1;
	}

	format = gfbg_getfmtbyargb(&info->var.red, &info->var.green, &info->var.blue, &info->var.transp,
				   gfbg_get_bits_per_pixel(info));

	if (par->color_format != format) {
		gfbg_free_canvas_buf(par);
		gfbg_set_fmt(par, format);
		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_FMT;
	}

	spin_lock_irqsave(&par->lock, lock_flag);
	display_info->x_res = gfbg_get_xres(info);
	display_info->y_res = gfbg_get_yres(info);
	display_info->vir_x_res = gfbg_get_xres_virtual(info);
	display_info->vir_y_res = gfbg_get_yres_virtual(info);

	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	return 0;
}

static int refresh_0buf_process(gfbg_par *par, const fb_buf *canvas_buf)
{
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	unsigned long lock_flag;

	spin_lock_irqsave(&par->lock, lock_flag);

	par->modifying = true;
	/* modify by wxl : if change flush type between 2buffer and 0 buffer, the addr couldn't be changed */
	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_DISPLAYADDR;
	/*
	* The graphic address is taken from the canvas of the user data and
	* filled in the screen address of the refresh information.
	*/
	refresh_info->screen_addr = canvas_buf->canvas.phys_addr;

	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_STRIDE;
	refresh_info->user_buffer.canvas.pitch = canvas_buf->canvas.pitch;

	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_FMT;
	refresh_info->user_buffer.canvas.format = canvas_buf->canvas.format;

	/* vgop scaler */
	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_OUTRECT;

	spin_unlock_irqrestore(&par->lock, lock_flag);
	return 0;
}

static int gfbg_wait_regconfig_work(vo_layer layer_id)
{
	int ret = 0;
	gfbg_par *par = NULL;

	if (layer_id >= GFBG_MAX_LAYER_NUM) {
		return -1;
	}
	par = (gfbg_par *)g_layer[layer_id].info->par;
	if (par == NULL) {
		return -1;
	}
	par->vblflag = 0;
	/* Assuming TDE is fast enough, 40ms */
	ret = wait_event_interruptible_timeout(par->vbl_event, par->vblflag, (int)msecs_to_jiffies(40));
	if (ret < 0) {
		TRACE_GFBG(DBG_ERR, "Wait vblank failed!");
		return -1;
	}

	return 0;
}

static int gfbg_refresh_0buf(vo_layer layer_id, const fb_buf *canvas_buf)
{
	struct fb_info *info = g_layer[layer_id].info;
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	unsigned long lock_flag;
	int ret = 0;

	ret = refresh_0buf_process(par, canvas_buf);
	if (ret != 0) {
		return -1;
	}
	/*
	* In gfbg_set_disp_size, it is possible that kmalloc
	* allocates memory in a non-atomic manner, so the lock must be released first
	*/
	if (gfbg_set_disp_size(layer_id, canvas_buf->canvas.width, canvas_buf->canvas.height) != 0) {
		return -1;
	}

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = false;
	memcpy(&(refresh_info->user_buffer), canvas_buf, sizeof(fb_buf));
	par->vblflag = 0;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	/* if the flag "FB_ACTIVATE_VBL" has been set, we should wait for register update finish */
	if (info->var.activate & FB_ACTIVATE_VBL)
		gfbg_wait_regconfig_work(layer_id);

	return 0;
}

static s32 _tde_do_op_cb(enum tde_usage_e usage
			, const void *usage_param, unsigned int width, unsigned int height
			, unsigned long long src_addr, unsigned long long dst_addr, unsigned char sync_io
			, unsigned char block, enum tde_cb_task_mode_e task_mode)
{
	struct tde_inter_cfg cfg;
	struct base_exe_m_cb exe_cb;

	osal_memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.width = width;
	cfg.height = height;
	cfg.src_addr = src_addr;
	cfg.dst_addr = dst_addr;
	cfg.sync_io = sync_io;
	cfg.block = block;
	cfg.task_mode = task_mode;

	exe_cb.callee = E_MODULE_TDE;
	exe_cb.caller = E_MODULE_GFBG;
	exe_cb.cmd_id = TDE_CB_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

static int gfbg_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	phys_addr_t display_addr;
	unsigned int stride;
	fb_buf canvas_buf;
	int ret = 0;
	unsigned int len = 0;
	char name[16];
	unsigned long long paddr_tde = 0;
	void *ion_v_tde = NULL;
	enum tde_cb_task_mode_e task_mode;
	int tde_w, tde_h;
	int tde_index;

	/* set the stride and display start address */
	stride = gfbg_get_line_length(info);

	/* 3 is 8 bits */
	tde_w = ALIGN(var->xres, 16); /* 16 for align */
	tde_h = ALIGN(var->yres, 16); /* 16 for align */
	len = tde_w * tde_h * (gfbg_get_bits_per_pixel(info) >> 3);
	display_addr = (gfbg_get_smem_start(info) + (unsigned long long)stride *
			var->yoffset + (unsigned long long)var->xoffset *
			(gfbg_get_bits_per_pixel(info) >> 3)) & 0xfffffffffffffff0; /* 3 is 8 bits */

	// Handle rotation and TDE buffer allocation
	if (g_layer[par->layer_id].rot) {
		// Toggle tde_index for double buffering
		g_layer[par->layer_id].tde_index = 1 - g_layer[par->layer_id].tde_index;
		tde_index = g_layer[par->layer_id].tde_index;

		// Allocate TDE buffer if not already allocated
		if (!g_layer[par->layer_id].tde_info[tde_index].tde_paddr) {
			if (snprintf(name, sizeof(name), "gfbg_tde_%d", tde_index) < 0) {
				TRACE_GFBG(DBG_ERR, "%s:%d:snprintf failure\n", __func__, __LINE__);
				return -1;
			}

			ret = base_ion_alloc(&paddr_tde, &ion_v_tde, name, len, true);
			if (ret != 0) {
				TRACE_GFBG(DBG_ERR, "%s:failed to malloc video memory, size: %u KB!\n", name, len);
				return -1;
			}
			base_ion_cache_invalidate(paddr_tde, ion_v_tde, len);

			g_layer[par->layer_id].tde_info[tde_index].tde_paddr = paddr_tde;
			g_layer[par->layer_id].tde_info[tde_index].tde_vaddr = ion_v_tde;
			g_layer[par->layer_id].tde_info[tde_index].tde_size = len;
		}

		// Perform rotation if needed
		if (g_layer[par->layer_id].rot == 1 || g_layer[par->layer_id].rot == 2) {
			task_mode = (g_layer[par->layer_id].rot == 1) ? TDE_CB_TASK_ROTATE_90 : TDE_CB_TASK_ROTATE_270;
			_tde_do_op_cb(TDE_USAGE_ROTATION, NULL, var->xres, var->yres,
				display_addr, g_layer[par->layer_id].tde_info[tde_index].tde_paddr, 1, 1, task_mode);

			base_ion_cache_invalidate(
				g_layer[par->layer_id].tde_info[tde_index].tde_paddr,
				g_layer[par->layer_id].tde_info[tde_index].tde_vaddr,
				g_layer[par->layer_id].tde_info[tde_index].tde_size);

			display_addr = g_layer[par->layer_id].tde_info[tde_index].tde_paddr;
		}
	}

	canvas_buf.canvas.format = par->color_format;
	canvas_buf.canvas.phys_addr = display_addr;
	canvas_buf.canvas.pitch = stride;
	canvas_buf.update_rect.x = 0;
	canvas_buf.update_rect.y = 0;

	canvas_buf.canvas.width = gfbg_get_xres(info);
	canvas_buf.canvas.height = gfbg_get_yres(info);
	canvas_buf.update_rect.width = (int)gfbg_get_xres(info);
	canvas_buf.update_rect.height = (int)gfbg_get_yres(info);
	g_layer[par->layer_id].info->var.activate = info->var.activate;

	if (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_COMPRESS) {
		gfbg_drv_set_oenc_cfg(canvas_buf.canvas.format, par->layer_id, display_addr);
		if (osal_sem_down_timeout(&par->oenc_sem, 100)) {
			TRACE_GFBG(DBG_ERR, "get sem timeout, oenc not ready\n");
		}
	}

	refresh_info->buf_mode = FB_LAYER_BUF_BUTT;
	ret = gfbg_refresh_0buf(par->layer_id, &canvas_buf);

	return ret;
}

static int gfbg_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	unsigned char gfbg_cmd = _IOC_NR(cmd);

	if ((gfbg_cmd < DRV_GFBG_IOCTL_CMD_NUM_MIN) || (gfbg_cmd >= DRV_GFBG_IOCTL_CMD_NUM_MAX)) {
		return -1;
	}
	if (g_drv_gfbg_ioctl_func[gfbg_cmd - DRV_GFBG_IOCTL_CMD_NUM_MIN].func == NULL) {
		return -1;
	}
	if (cmd != g_drv_gfbg_ioctl_func[gfbg_cmd - DRV_GFBG_IOCTL_CMD_NUM_MIN].cmd) {
		TRACE_GFBG(DBG_ERR, "the command:0x%x is unsupported!\n", gfbg_cmd);
		return -1;
	}
	return g_drv_gfbg_ioctl_func[gfbg_cmd - DRV_GFBG_IOCTL_CMD_NUM_MIN].func(info, arg);
}

#ifdef CONFIG_COMPAT
static int gfbg_compat_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	return gfbg_ioctl(info, cmd, arg);
}
#endif

unsigned short gfbg_convert_color_to_argb4444(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char a1, r1, g1, b1;
	unsigned short pixel;

	pixel = a1 = r1 = g1 = b1 = 0;
	a1 = a >> 4;
	r1 = r >> 4;
	g1 = g >> 4;
	b1 = b >> 4;

	pixel = (b1 | (g1 << 4) | (r1 << 8 | (a1 << 12)));

	return pixel;
}

static int gfbg_dosetcolreg(const gfbg_cmp_reg *color_reg, const struct fb_info *info)
{
	// int ret = 0;
	gfbg_par *par = (gfbg_par *)info->par;
	vo_dev dev_id = 0;
	vo_layer layer_id, hal_layer_id;
	unsigned short argb4444;
	layer_id = par->layer_id;

	// ret = vo_gfbg_get_bind_dev_id(layer_id, &dev_id);
	// if (ret != 0) {
	// 	TRACE_GFBG(DBG_ERR, "layer (%d) get bind dev id failure\n", layer_id);
	// 	return ret;
	// }
	// ret = vo_gfbg_get_hw_layer_id(layer_id, &hal_layer_id);
	// if (ret != 0) {
	// 	TRACE_GFBG(DBG_ERR, "layer (%d) get hal layer id failure\n", layer_id);
	// 	return ret;
	// }

	// if (par->color_format != FB_FORMAT_LUT_256 && par->color_format != FB_FORMAT_LUT_16) {
	// 	TRACE_GFBG(DBG_WARN, "only supports setting color palettes when the format is LUT_256 or LUT_16!\n");
	// 	TRACE_GFBG(DBG_WARN, "now it is %d\n", par->color_format);
	// 	return -1;
	// }

	if (par->color_format == FB_FORMAT_LUT_256 && color_reg->regno > 255) { /* 255 is larger than */
		TRACE_GFBG(DBG_WARN, "regno: %d, larger than 255!\n", color_reg->regno);
		return -1;
	} else if (par->color_format == FB_FORMAT_LUT_16 && color_reg->regno > 15) { /* 16 is larger than */
		TRACE_GFBG(DBG_WARN, "regno: %d, larger than 15!\n", color_reg->regno);
		return -1;
	}

	// ARGB4444 only
	argb4444 = gfbg_convert_color_to_argb4444(color_reg->transp, color_reg->red, color_reg->green,
						  color_reg->blue);
	TRACE_GFBG(DBG_INFO, "dev_id:%d, hal_layer_id:%d, set color index %d to 0x%x\n",
		   dev_id, hal_layer_id, color_reg->regno, argb4444);

	return gfbg_hal_set_color_reg(dev_id, hal_layer_id, par->color_format,
				      color_reg->regno, argb4444);
}

static int gfbg_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
			   unsigned int blue, unsigned int transp, struct fb_info *info)
{
	gfbg_cmp_reg cmp_reg = {0};
	cmp_reg.regno = regno;
	cmp_reg.red = red;
	cmp_reg.green = green;
	cmp_reg.blue = blue;
	cmp_reg.transp = transp;
	return gfbg_dosetcolreg(&cmp_reg, info);
}

static int gfbg_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	unsigned int i;
	int start;
	unsigned short *red = NULL;
	unsigned short *green = NULL;
	unsigned short *blue = NULL;
	unsigned short *transp = NULL;
	gfbg_cmp_reg cmp_reg = {0};
	cmp_reg.transp = 0xffff;

	red = cmap->red;
	green = cmap->green;
	blue = cmap->blue;
	transp = cmap->transp;
	start = cmap->start;

	for (i = 0; i < cmap->len; i++) {
		cmp_reg.red   = *red++;
		cmp_reg.green = *green++;
		cmp_reg.blue  = *blue++;
		if (transp != NULL) {
			cmp_reg.transp = *transp++;
		}
		cmp_reg.regno = start;

		if (gfbg_dosetcolreg(&cmp_reg, info)) {
			break;
		}

		start++;
	}

	return 0;
}

#ifdef CONFIG_FB_CFB_IMAGEBLIT
static void gfbg_imageblit(struct fb_info *p, const struct fb_image *image)
{
	cfb_imageblit(p, image);
}
#endif

static struct fb_ops gfbg_ops = {
	.owner                  = THIS_MODULE,
	.fb_open                = gfbg_open,
	.fb_release             = gfbg_release,
	.fb_check_var           = gfbg_check_var,
	.fb_set_par             = gfbg_set_par,
	.fb_pan_display         = gfbg_pan_display,
	.fb_ioctl               = gfbg_ioctl,
	.fb_setcolreg           = gfbg_setcolreg,
	.fb_setcmap             = gfbg_setcmap,
#ifdef CONFIG_COMPAT
	.fb_compat_ioctl        = gfbg_compat_ioctl,
#endif
#ifdef CONFIG_FB_CFB_IMAGEBLIT
	.fb_imageblit		= gfbg_imageblit,
#endif
};

static int overlay_probe_alloc_mem(struct fb_info *info, struct fb_fix_screeninfo *fix,
				   struct fb_var_screeninfo *var)
{
	gfbg_par *par = NULL;
	vo_layer layer_id;
	char name[16];
	unsigned long long pAddr = 0;
	void *ion_v = NULL;
	int ret = -1;
	unsigned int len = 0;

	par = (gfbg_par *)(info->par);
	layer_id = par->layer_id;

	if (g_layer[layer_id].layer_size != 0) {
		/* initialize the fix screen info */
		*fix = g_default_fix;
		*var = g_default_var;

		if (snprintf(name, 12, "gfbg_layer%01u", layer_id) < 0) { /* 12:for char length */
			TRACE_GFBG(DBG_ERR, "%s:%d:snprintf_s failure\n", __func__, __LINE__);
			return -1;
		}

		len = g_layer[layer_id].layer_size * 1024;

		ret = base_ion_alloc(&pAddr, &ion_v, name, len, true);
		if (ret != 0) {
			TRACE_GFBG(DBG_ERR, "%s:failed to malloc the video memory, size: %u KBtyes!\n", name, len);
			return -1;
		}

		fix->smem_start = pAddr;
		fix->smem_len = len;
		info->screen_base = (char *)ion_v;

		// Clear the new dmabuf
		// TODO: Replace by TDE.
		memset(info->screen_base, 0, fix->smem_len);
		// Flush cache data into DRAM
		base_ion_cache_flush(fix->smem_start, (void *)info->screen_base, fix->smem_len);
	}

	return 0;
}

void gfbg_overlay_cleanup(vo_layer layer_id, bool unregister)
{
	struct fb_info *info = NULL;
	gfbg_par *par = NULL;

	/* get framebuffer info structure pointer */
	info = g_layer[layer_id].info;
	if (info != NULL) {
		if (gfbg_get_smem_start(info)) {
			base_ion_free(gfbg_get_smem_start(info));
		}

		if (unregister) {
			unregister_framebuffer(info);
		}

		par = (gfbg_par *)(info->par);
		osal_sem_destroy(&par->oenc_sem);
		framebuffer_release(info);
		g_layer[layer_id].info = NULL;
	}
}

int gfbg_overlay_probe(vo_layer layer_id)
{
	int ret = -1;
	struct fb_info *info = NULL;
	struct fb_fix_screeninfo *fix = NULL;
	struct fb_var_screeninfo *var = NULL;
	gfbg_par *par = NULL;

	info = framebuffer_alloc(sizeof(gfbg_par), NULL);
	if (info == NULL) {
		TRACE_GFBG(DBG_ERR, "failed to malloc the fb_info!\n");
		return -ENOMEM;
	}
	fix = &info->fix;
	var = &info->var;

	g_layer[layer_id].info = info;

	info->flags = FBINFO_FLAG_DEFAULT | FBINFO_HWACCEL_YPAN | FBINFO_HWACCEL_XPAN;
	info->fbops = &gfbg_ops;

	par = (gfbg_par *)(info->par);
	(void)memset(par, 0, sizeof(gfbg_par));
	par->layer_id = layer_id;

	if (snprintf(fix->id, 5, "ovl%01u", layer_id) < 0) {
		TRACE_GFBG(DBG_ERR, "%s:%d:snprintf failure\n", __func__, __LINE__);
		ret = -1;
		goto ERR;
	}

	ret = overlay_probe_alloc_mem(info, fix, var);
	if (ret != 0) {
		goto ERR;
	}

	ret = register_framebuffer(info);
	if (ret < 0) {
		TRACE_GFBG(DBG_ERR, "failed to register_framebuffer!layerid = %d, intRet = %d\n", layer_id, ret);
		ret = -EINVAL;
		goto ERR;
	}

	TRACE_GFBG(DBG_INFO, "succeed in registering the fb%d: %s frame buffer device\n", info->node, fix->id);

	osal_sem_init(&par->oenc_sem, 0);

	return 0;

ERR:
	gfbg_overlay_cleanup(layer_id, false);

	return ret;
}

void gfbg_alloc_cmap(vo_layer layer_id)
{
	const unsigned int cmap_len = 256;
	struct fb_info *info = g_layer[layer_id].info;

	if (info == NULL) {
		TRACE_GFBG(DBG_ERR, "gfbg_alloc_cmap failed\n");
		return;
	}

	if (fb_alloc_cmap(&info->cmap, cmap_len, 1) < 0) {
		info->cmap.len = 0;
		TRACE_GFBG(DBG_ERR, "fb_alloc_cmap failed\n");
		return;
	}

	info->cmap.len = cmap_len;
}

void gfbg_free_cmap(vo_layer layer_id)
{
	struct fb_cmap *cmap = NULL;
	struct fb_info *info = g_layer[layer_id].info;

	if (info == NULL) {
		return;
	}

	cmap = &info->cmap;
	if (cmap->len != 0) {
		fb_dealloc_cmap(cmap);
	}
}

static void gfbg_set_layerpos(gfbg_par *par, const fb_point *pos)
{
	int x_pos;
	int y_pos;
	fb_size max_screen_size = {0};
	vo_layer layer_id;
	gfbg_display_info *display_info = NULL;

	layer_id = par->layer_id;
	display_info = &par->display_info;

	gfbg_get_maxscreensize(par, &max_screen_size.width, &max_screen_size.height);
	x_pos = pos->x_pos;
	y_pos = pos->y_pos;
	if (x_pos >= max_screen_size.width) {
		TRACE_GFBG(DBG_INFO, "the sum of x_pos(%d) larger than Vodev screen width(%d)!\n",
			   x_pos, max_screen_size.width);
		// should - 1 or Horizontal stripes appear
		x_pos = max_screen_size.width - 1;
	}

	if (y_pos > max_screen_size.height) {
		TRACE_GFBG(DBG_INFO, "the sum of y_pos(%d) larger than Vodev screen height(%d)!\n",
			   y_pos, max_screen_size.height);
		y_pos = max_screen_size.height;
	}

	display_info->pos.x_pos = x_pos;
	display_info->pos.y_pos = y_pos;

	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_OUTRECT;
}

static inline void gfbg_get_layerpos(const gfbg_par *par, fb_point *pos)
{
	memcpy(pos, &par->display_info.pos, sizeof(fb_point));
}

static void gfbg_get_dispsize(const gfbg_par *par, unsigned int *width, unsigned int *height)
{
	const gfbg_display_info *display_info = NULL;

	display_info = &par->display_info;

	if ((width != NULL) && (height != NULL)) {
		*width = display_info->display_width;
		*height = display_info->display_height;
	}
}

static void gfbg_get_screensize(const gfbg_par *par, unsigned int *width, unsigned int *height)
{
	const gfbg_display_info *display_info = NULL;
	if (par == NULL) {
		return;
	}
	display_info = &par->display_info;

	if ((width != NULL) && (height != NULL)) {
		*width = display_info->screen_width;
		*height = display_info->screen_height;
	}
}

static inline void gfbg_get_key(const gfbg_par *par, gfbg_colorkeyex *key)
{
	memcpy(key, &par->ckey, sizeof(gfbg_colorkeyex));
}

static inline void gfbg_set_key(gfbg_par *par, const gfbg_colorkeyex *key)
{
	memcpy(&par->ckey, key, sizeof(gfbg_colorkeyex));
}

static inline void gfbg_get_bufmode(const gfbg_par *par, fb_layer_buf *buf_mode)
{
	*buf_mode = par->refresh_info.buf_mode;
}

static void gfbg_get_layerinfo(const gfbg_par *par, fb_layer_info *layer_info)
{
	fb_point pos = {0};
	if ((par != NULL) && (layer_info != NULL)) {
		gfbg_get_bufmode(par, &layer_info->buf_mode);
		gfbg_get_layerpos(par, &pos);
		layer_info->x_pos = pos.x_pos;
		layer_info->y_pos = pos.y_pos;
		gfbg_get_dispsize(par, &layer_info->display_width, &layer_info->display_height);
		gfbg_get_screensize(par, &layer_info->screen_width, &layer_info->screen_height);
		layer_info->canvas_width = par->canvas_sur.width;
		layer_info->canvas_height = par->canvas_sur.height;

		layer_info->mask = FB_LAYER_MASK_BUTT;
	}
}

/*
 * Name : gfbg_check_layerinfo
 * Desc :check layer information: buf refresh mode,position,canvas width
	 and height, display width and height, screen width and height.
 */
static int gfbg_check_layerinfo(const fb_layer_info *layer_info)
{
	if (layer_info->mask & FB_LAYER_MASK_BUF_MODE) {
		if (layer_info->buf_mode > FB_LAYER_BUF_DOUBLE_IMMEDIATE) {
			TRACE_GFBG(DBG_ERR, "buf_mode(%d) is error, should between %d and %d\n",
				   layer_info->buf_mode, FB_LAYER_BUF_DOUBLE, FB_LAYER_BUF_DOUBLE_IMMEDIATE);
			return -1;
		}
	}

	/* check the width and height */
	if (layer_info->mask & FB_LAYER_MASK_DISPLAY_SIZE) {
		if (layer_info->display_width % 2 || layer_info->display_height % 2) { /* 2 pixel align */
			TRACE_GFBG(DBG_ERR, "Disaplay W(%u) and H(%u) should align to 2!\n",
				   layer_info->display_width, layer_info->display_height);
			return -1;
		}
	}

	if (layer_info->mask & FB_LAYER_MASK_SCREEN_SIZE) {
		if (layer_info->screen_width % 2 || layer_info->screen_height % 2) { /* 2 pixel align */
			TRACE_GFBG(DBG_ERR, "Screenaplay W(%u) and H(%u) should align to 2!\n",
				   layer_info->screen_width, layer_info->screen_height);
			return -1;
		}
	}

	return 0;
}

static int check_display_size(const struct fb_info *info, const gfbg_par *par, const fb_layer_info *layer_info)
{
	unsigned int pitch;

	/* Modify the display size, the memory size has changed, limited by the size of the memory */
	if (layer_info->mask & FB_LAYER_MASK_DISPLAY_SIZE) {
		pitch = (layer_info->display_width * gfbg_get_bits_per_pixel(info)) >> 3; /* 3 for 8bit */
		pitch = (pitch + 0xf) & 0xfffffff0;
		if (gfbg_check_mem_enough(info, pitch, layer_info->display_height) == -1) {
			TRACE_GFBG(DBG_ERR, "memory is not enough!\n");
			return -1;
		}

		if (layer_info->display_width == 0 || layer_info->display_height == 0) {
			TRACE_GFBG(DBG_ERR, "display width/height shouldn't be 0!\n");
			return -1;
		}
	}
	return 0;
}

static int onputlayerinfo_check_size(const struct fb_info *info, const gfbg_par *par, const fb_layer_info *layer_info)
{
	/* Check the display size */
	if (check_display_size(info, par, layer_info) != 0) {
		return -1;
	}

	/* Check the canvas size */
	if (layer_info->mask & FB_LAYER_MASK_CANVAS_SIZE) {
		if ((layer_info->canvas_width == 0) || (layer_info->canvas_height == 0)) {
			TRACE_GFBG(DBG_ERR, "canvas width/height shouldn't be 0\n");
			return -1;
		}
	}

	/* Check the screen size */
	if (layer_info->mask & FB_LAYER_MASK_SCREEN_SIZE) {
		if ((layer_info->screen_width == 0) || (layer_info->screen_height == 0)) {
			TRACE_GFBG(DBG_ERR, "screen width/height shouldn't be 0\n");
			return -1;
		}
	}
	return 0;
}

static int onputlayerinfo_check_buf_mode(struct fb_info *info, gfbg_par *par, fb_layer_info *layer_info)
{
	unsigned int layer_size;

	/* Modify the display buf mode, the memory size has changed, limited by the size of the memory */
	if (layer_info->mask & FB_LAYER_MASK_BUF_MODE) {
		if (layer_info->buf_mode == FB_LAYER_BUF_ONE) {
			layer_size = gfbg_get_line_length(info) * gfbg_get_yres(info);
		} else if ((layer_info->buf_mode == FB_LAYER_BUF_DOUBLE) ||
			   (layer_info->buf_mode == FB_LAYER_BUF_DOUBLE_IMMEDIATE)) {
			layer_size = 2 * gfbg_get_line_length(info) * gfbg_get_yres(info); /* 2 buf */
		} else {
			layer_size = 0;
		}

		if (gfbg_get_smem_len(info) < layer_size) {
			/*
			* layer real memory size:%d KBytes, expected:%d KBtyes
			* real:gfbg_get_smem_len(info)/1024, expectde:layer_size/1024 or cmp_layer_size/1024
			*/
			TRACE_GFBG(DBG_ERR, "No enough mem!real:%dKB;expected:%dKB\n",
				   gfbg_get_smem_len(info) / 1024, layer_size / 1024); /* 1024 for KB */
			return -1;
		}
	}

	/* if x>width or y>height ,how to deal with: see nothing in screen or return failure. */
	if (layer_info->mask & FB_LAYER_MASK_POS) {
		if ((layer_info->x_pos < 0) || (layer_info->y_pos < 0)) {
			TRACE_GFBG(DBG_ERR, "It's not supported to set start pos of layer to negative!\n");
			return -1;
		}
	}
	return 0;
}

static void onputlayerinfo_set_with_mask(gfbg_par *par, const fb_layer_info *layer_info)
{
	fb_point pos;

	if (layer_info->mask & FB_LAYER_MASK_BUF_MODE) {
		gfbg_set_bufmode(par->layer_id, layer_info->buf_mode);
	}

	if (layer_info->mask & FB_LAYER_MASK_POS) {
		pos.x_pos = layer_info->x_pos;
		pos.y_pos = layer_info->y_pos;
		gfbg_set_layerpos(par, &pos);
	}
}

static int onputlayerinfo_process(gfbg_par *par, const fb_layer_info *layer_info)
{
	int ret = 0;
	unsigned long lock_flag;

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;

	/* deal with layer_info->mask */
	onputlayerinfo_set_with_mask(par, layer_info);

	spin_unlock_irqrestore(&par->lock, lock_flag);
	/*
	* The following two functions have a sleep operation inside,
	* you must unlock before calling, and lock the global amount inside the function.
	*/
	if (layer_info->mask & FB_LAYER_MASK_SCREEN_SIZE) {
		ret = gfbg_set_screen_size(par->layer_id, layer_info->screen_width, layer_info->screen_height);
		if (ret != 0) {
			return ret;
		}
	}
	if (layer_info->mask & FB_LAYER_MASK_DISPLAY_SIZE) {
		ret = gfbg_set_disp_size(par->layer_id, layer_info->display_width, layer_info->display_height);
		if (ret != 0) {
			return ret;
		}
	}
	spin_lock_irqsave(&par->lock, lock_flag);

	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);
	return 0;
}

static int alloc_new_canvas_buffer(const struct fb_info *info, const fb_layer_info *layer_info)
{
	gfbg_par *par = (gfbg_par *)info->par;
	fb_surface *canvas_surface = &par->canvas_sur;
	unsigned int layer_size;
	unsigned int pitch;
	unsigned long long pAddr = 0;
	void *buf = NULL;
	char name[16]; /* 16 name max length */

	/* 16 bytes alignment */
	pitch = (((layer_info->canvas_width * gfbg_get_bits_per_pixel(info)) >> 3) + 15) >> 4; /* 3 4 15 alg data */
	pitch = pitch << 4; /* 4 alg data */

	layer_size = pitch * layer_info->canvas_height;
	/* alloc new buffer */
	if (snprintf(name, 13, "gfbg_canvas%01u", par->layer_id) < 0) { /* 13:for char length */
		TRACE_GFBG(DBG_ERR, "%s:%d:snprintf failure\n", __func__, __LINE__);
		return -1;
	}

	if (base_ion_alloc(&pAddr, &buf, name, layer_size, true)) {
		TRACE_GFBG(DBG_ERR, "%s:failed to malloc the video memory, size: %u KBtyes!\n", name, layer_size);
		return -1;
	}

	canvas_surface->phys_addr = pAddr;
	if (canvas_surface->phys_addr == 0) {
		TRACE_GFBG(DBG_ERR, "alloc canvas buffer no mem");
		return -1;
	}

	// Clear the new dmabuf
	// TODO: Replace by TDE.
	memset(buf, 0, layer_size);
	// Flush cache data into DRAM
	base_ion_cache_flush(canvas_surface->phys_addr, buf, layer_size);

	TRACE_GFBG(DBG_INFO, "alloc new memory for canvas buffer success\n");
	canvas_surface->width = layer_info->canvas_width;
	canvas_surface->height = layer_info->canvas_height;
	canvas_surface->pitch = pitch;
	canvas_surface->format = gfbg_getfmtbyargb(&info->var.red, &info->var.green, &info->var.blue,
						   &info->var.transp, info->var.bits_per_pixel);
	return 0;
}

static int gfbg_alloc_canvas_buf(const struct fb_info *info, const fb_layer_info *layer_info)
{
	gfbg_par *par = NULL;
	fb_surface *canvas_surface = NULL;

	if ((info == NULL) || (layer_info == NULL)) {
		return -1;
	}
	par = (gfbg_par *)info->par;
	canvas_surface = &par->canvas_sur;

	if (!(layer_info->mask & FB_LAYER_MASK_CANVAS_SIZE)) {
		return 0;
	}

	/* if  with old canvas buffer */
	if (canvas_surface->phys_addr != 0) {
		/* if old is the same with new , then return, else free the old buffer */
		if ((layer_info->canvas_width == canvas_surface->width) &&
		    (layer_info->canvas_height == canvas_surface->height)) {
			TRACE_GFBG(DBG_INFO, "mem size is the same , no need alloc new memory");
			return 0;
		}

		/* free new old buffer */
		TRACE_GFBG(DBG_INFO, "free old canvas buffer\n");
		gfbg_free_canvas_buf(par);
	}
	if (layer_info->canvas_width > GFBG_DEF_WIDTH || layer_info->canvas_height > GFBG_DEF_HEIGHT) {
		TRACE_GFBG(DBG_INFO, "unsupported too large w(%d) and h(%d)\n",
			   layer_info->canvas_width, layer_info->canvas_height);
		return -1;
	}
	/* new canvas buffer */
	if (alloc_new_canvas_buffer(info, layer_info) != 0) {
		return -1;
	}
	return 0;
}

/*
 * Name : gfbg_refresh
 * Desc : It is refreshed according to the canvas information and the layer's buf refresh mode.
 *        It is called indirectly when setting the layer attr.
 * See  : references gfbg_refresh_again,gfbg_onrefresh
 *        calls gfbg_refresh_0buf,gfbg_refresh_1buf,gfbg_refresh_2buf
 */
static int gfbg_refresh(vo_layer layer_id, const fb_buf *canvas_buf, fb_layer_buf buf_mode)
{
	int ret = -1;

	if (canvas_buf == NULL) {
		return -1;
	}

	switch (buf_mode) {
	// case FB_LAYER_BUF_DOUBLE:
	//	ret = gfbg_refresh_2buf(layer_id, canvas_buf);
	//	break;
	// case FB_LAYER_BUF_ONE:
	//	ret = gfbg_refresh_1buf(layer_id, canvas_buf);
	//	break;
	case FB_LAYER_BUF_NONE:
		ret = gfbg_refresh_0buf(layer_id, canvas_buf);
		break;
	// case FB_LAYER_BUF_DOUBLE_IMMEDIATE:
	//	ret = gfbg_refresh_2buf_immediate_display(layer_id, canvas_buf);
	//	break;
	default:
		break;
	}

	return ret;
}

static void gfbg_refresh_again(vo_layer layer_id)
{
	struct fb_info *info = g_layer[layer_id].info;
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	fb_buf canvas;

	/* Prerequisites for the canvas to be refreshed */
	if (!(par->param_modify_mask & GFBG_LAYER_PARAMODIFY_INRECT)) {
		return;
	}

	if (refresh_info->user_buffer.canvas.phys_addr == 0) {
		return;
	}

	if (refresh_info->buf_mode == FB_LAYER_BUF_NONE) {
		return;
	}
	/* Fills the canvas object with refresh information from private data for refresh. */
	canvas = refresh_info->user_buffer;
	canvas.update_rect.x = 0;
	canvas.update_rect.y = 0;
	canvas.update_rect.width = canvas.canvas.width;
	canvas.update_rect.height = canvas.canvas.height;
	gfbg_refresh(layer_id, &canvas, refresh_info->buf_mode);
}

static int gfbg_onputlayerinfo(struct fb_info *info, gfbg_par *par, const void __user *argp)
{
	int ret = 0;
	fb_layer_info layer_info;

	if (argp == NULL) {
		TRACE_GFBG(DBG_ERR, "NULL arg!\n");
		return -EINVAL;
	}

	if (copy_from_user(&layer_info, argp, sizeof(fb_layer_info))) {
		return -EFAULT;
	}

	ret = gfbg_check_layerinfo(&layer_info);
	if (ret != 0) {
		return -1;
	}
	/*
	* Check the display size
	* Check the canvas size
	* Check the screen size
	*/
	if (onputlayerinfo_check_size(info, par, &layer_info) != 0) {
		return -1;
	}
	/*
	* Check when modify buf mode
	* Check when modify pos
	*/
	if (onputlayerinfo_check_buf_mode(info, par, &layer_info) != 0) {
		return -1;
	}

	/*
	* avoid modifying register in vo isr before all params has been recorded!
	* In vo irq,flag modifying will be checked.
	*/
	ret = onputlayerinfo_process(par, &layer_info);
	if (ret != 0) {
		return ret;
	}
	ret = gfbg_alloc_canvas_buf(info, &layer_info);
	if (ret != 0) {
		/*
		* There is no error returned here, because the user can also
		* specify this memory; in addition, even if the allocation is successful,
		* The user also needs to call FBIOGET_CANVAS_BUF to get it to operate.
		*/
		TRACE_GFBG(DBG_ERR, "alloc canvas buffer failed\n");
	}
	gfbg_refresh_again(par->layer_id);
	return ret;
}

static int onrefresh_check_param(const gfbg_par *par, const fb_buf *canvas_buf, fb_layer_buf buf_mode)
{
	UNUSED(par);
	if (canvas_buf->canvas.format >= FB_FORMAT_BUTT) {
		return -1;
	}

	if (buf_mode == FB_LAYER_BUF_BUTT) {
		TRACE_GFBG(DBG_ERR, "doesn't support FBIO_REFRESH operation when refresh mode is FB_LAYER_BUF_BUTT!\n");
		return -1;
	}

	if ((canvas_buf->update_rect.x >=  (int)canvas_buf->canvas.width) ||
	    (canvas_buf->update_rect.y >= (int)canvas_buf->canvas.height) ||
	    (canvas_buf->update_rect.width <= 0) || (canvas_buf->update_rect.height <= 0)) {
		TRACE_GFBG(DBG_ERR, "rect error: update rect:(%d,%d,%d,%d), canvas range:(%d,%d)\n",
			   canvas_buf->update_rect.x, canvas_buf->update_rect.y,
			   canvas_buf->update_rect.width, canvas_buf->update_rect.height,
			   canvas_buf->canvas.width, canvas_buf->canvas.height);
		return -1;
	}

	return 0;
}

static int onrefresh_get_canvas_buf(gfbg_par *par, void __user *argp, fb_buf *canvas_buf)
{
	fb_layer_buf buf_mode;
	if (copy_from_user(canvas_buf, argp, sizeof(fb_buf))) {
		return -EFAULT;
	}

	gfbg_get_bufmode(par, &buf_mode);
	/*
	* check canvas format
	* check canvas phyaddr
	* check buf_mode
	* check canvas update rect legality
	*/
	if (onrefresh_check_param(par, canvas_buf, buf_mode) != 0) {
		return -1;
	}

	/* update canvas update_rect */
	if (canvas_buf->update_rect.x + canvas_buf->update_rect.width > (int)canvas_buf->canvas.width) {
		canvas_buf->update_rect.width = canvas_buf->canvas.width - canvas_buf->update_rect.x;
	}

	if (canvas_buf->update_rect.y + canvas_buf->update_rect.height > (int)canvas_buf->canvas.height) {
		canvas_buf->update_rect.height =  canvas_buf->canvas.height - canvas_buf->update_rect.y;
	}
	if (buf_mode == FB_LAYER_BUF_NONE) {
		/* Check if the format of the canvas supported or not by gfbg */
		if ((par->layer_id >= GFBG_MAX_LAYER_NUM) || (canvas_buf->canvas.format >= FB_FORMAT_BUTT)) {
			return -1;
		}
	} else {
		/*
		* TDE to do
		*/
	}
	return 0;
}

static int gfbg_onrefresh(gfbg_par *par, void __user *argp)
{
	int ret = 0;
	fb_buf canvas_buf;
	fb_layer_buf buf_mode;

	if (argp == NULL) {
		TRACE_GFBG(DBG_ERR, "NULL arg!\n");
		return -EINVAL;
	}

	/* get canvas buffer and check legality */
	ret = onrefresh_get_canvas_buf(par, argp, &canvas_buf);
	if (ret != 0) {
		return -1;
	}

	gfbg_get_bufmode(par, &buf_mode);

	ret = gfbg_refresh(par->layer_id, &canvas_buf, buf_mode);

	return ret;
}

static int flip_surface_check_param(const struct fb_info *info, const fb_surfaceex *surface_ex)
{
	gfbg_par *par = NULL;
	gfbg_display_info *display_info = NULL;
	unsigned long addr;
	unsigned long smem_end;
	fb_color_format color_format;
	par = (gfbg_par *)info->par;
	display_info = &par->display_info;

	if (surface_ex->colorkey.enable != true && surface_ex->colorkey.enable != false) {
		TRACE_GFBG(DBG_ERR, "colorkey.enable(%d) should be true or false!\n", surface_ex->colorkey.enable);
		return -1;
	}

	gfbg_get_fmt(par, &color_format);

	if (color_format != FB_FORMAT_ARGB8888) {
		TRACE_GFBG(DBG_ERR, "layer(%d) colorkey only support argb8888 fmt!\n", par->layer_id);
		return -1;
	}

	addr = (unsigned long)surface_ex->phys_addr;
	smem_end = gfbg_get_smem_start(info) + gfbg_get_smem_len(info) - gfbg_get_yres(info) *
		gfbg_get_line_length(info);
	if ((addr < gfbg_get_smem_start(info)) || (addr > smem_end)) {
		TRACE_GFBG(DBG_ERR, "the addr(0x%lx) is out of range(0x%llx,0x%lx)!\n",
			   addr, gfbg_get_smem_start(info), smem_end);
		return -1;
	}

	if (gfbg_get_line_length(info) == 0) {
		return -1;
	}

	if (gfbg_get_bits_per_pixel(info) == 0) {
		return -1;
	}

	return 0;
}

static int flip_surface_pan_display(struct fb_info *info, const fb_surfaceex *surface_ex)
{
	unsigned long addr;
	unsigned int differ;
	unsigned int x_offset;
	unsigned int y_offset;
	struct fb_var_screeninfo var;
	addr = (unsigned long)surface_ex->phys_addr;
	differ = addr - gfbg_get_smem_start(info);
	y_offset = differ / gfbg_get_line_length(info);
	/* 8 bit (2^3) */
	x_offset = (((differ % gfbg_get_line_length(info)) << 3) / (gfbg_get_bits_per_pixel(info)));
	memcpy(&var, &info->var, sizeof(var));
	var.xoffset = x_offset;
	var.yoffset = y_offset;

	if (fb_pan_display(info, &var) < 0) {
		TRACE_GFBG(DBG_ERR, "pan_display error!\n");
		return -1;
	}

	return 0;
}

static int drv_gfbg_get_screen_origin_pos(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;

	fb_point pos = {0};

	par = (gfbg_par *)info->par;
	gfbg_get_layerpos(par, &pos);

	return copy_to_user(argp, &pos, sizeof(fb_point));
}

static int drv_gfbg_set_screen_origin_pos(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	fb_point expected_pos;
	vo_layer layer_id;
	unsigned long lock_flag;
	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	if (copy_from_user(&expected_pos, argp, sizeof(fb_point))) {
		return -EFAULT;
	}

	if (expected_pos.x_pos < 0 || expected_pos.y_pos < 0) {
		TRACE_GFBG(DBG_ERR, "It's not supported to set start pos of layer to negative!\n");
		return -1;
	}

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;

	/* Record the old location first */
	gfbg_set_layerpos(par, &expected_pos);
	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	return 0;
}

static int drv_gfbg_get_layer_show_state(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	bool is_show;
	par = (gfbg_par *)info->par;
	is_show = par->show;
	return copy_to_user(argp, &is_show, sizeof(bool));
}

static int drv_gfbg_show_layer(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	bool is_show = false;
	unsigned long lock_flag;
	vo_layer layer_id;
	int ret = 0;

	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	if (copy_from_user(&is_show, argp, sizeof(bool))) {
		return -EFAULT;
	}

	if ((is_show != true) && (is_show != false)) {
		TRACE_GFBG(DBG_ERR, "show(%d) should be true or false!\n", is_show);
		return -1;
	}

	if (is_show == gfbg_get_show(par)) {
		TRACE_GFBG(DBG_INFO, "The layer is show(%d) now!\n", par->show);
		return 0;
	}

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;

	gfbg_set_show(par, is_show);
	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_SHOW;

	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	return ret;
}

static int drv_gfbg_set_screen_size(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	const gfbg_display_info *display_info = NULL;
	fb_size  screen_size;
	fb_size  max_screen_size = {0};
	unsigned long lock_flag;

	par = (gfbg_par *)info->par;
	display_info = &par->display_info;

	if (copy_from_user(&screen_size, argp, sizeof(fb_size))) {
		return -EFAULT;
	}

	gfbg_get_maxscreensize(par, &max_screen_size.width, &max_screen_size.height);
	if (screen_size.width > max_screen_size.width) {
		TRACE_GFBG(DBG_ERR, "the width(%d) larger than dev screen width(%d)",
			   screen_size.width, max_screen_size.width);
		return -1;
	}
	if (screen_size.height > max_screen_size.height) {
		TRACE_GFBG(DBG_ERR, "the height(%d) larger than dev screen height(%d)",
			   screen_size.height, max_screen_size.height);
		return -1;
	}

	if ((screen_size.width == 0) || (screen_size.height == 0)) {
		TRACE_GFBG(DBG_ERR, "screen width(%u) height(%u) shouldn't be 0\n",
			   screen_size.width, screen_size.height);
		return -1;
	}
	if (screen_size.width % 2 || screen_size.height % 2) { /* 2 for align */
		TRACE_GFBG(DBG_ERR, "stScreenSize (%u, %u) should align to 2!\n",
			   screen_size.width, screen_size.height);
		return -1;
	}

	/* The chip only support W x2 Scale. */
	if (screen_size.width != display_info->display_width &&
	    (screen_size.width != (display_info->display_width * GFBG_MAX_ZOOMIN))) {
		TRACE_GFBG(DBG_ERR,
			   "GFBG layer%d display_size(%d, %d) and screen_size(%d, %d) do out of ZoomRatio[1, %d]!!\n",
			   par->layer_id, display_info->display_width, display_info->display_height,
			   screen_size.width, screen_size.height, GFBG_MAX_ZOOMIN);
		return -1;
	}

	/* The chip only support H x2 Scale. */
	if (screen_size.height != display_info->display_height &&
	    (screen_size.height != (display_info->display_height * GFBG_MAX_ZOOMIN))) {
		TRACE_GFBG(DBG_ERR,
			   "GFBG layer%d display_size(%d, %d) and screen_size(%d, %d) do out of ZoomRatio[1, %d]!!\n",
			   par->layer_id, display_info->display_width, display_info->display_height,
			   screen_size.width, screen_size.height, GFBG_MAX_ZOOMIN);
		return -1;
	}

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;
	if (gfbg_set_screen_size(par->layer_id, screen_size.width, screen_size.height) == 0) {
		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_OUTRECT;
	}
	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);
	return 0;
}

static int drv_gfbg_get_screen_size(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	fb_size screen_size = {0};
	par = (gfbg_par *)info->par;
	gfbg_get_screensize(par, &screen_size.width, &screen_size.height);
	return copy_to_user(argp, &screen_size, sizeof(fb_size));
}

static int drv_gfbg_get_vblank(struct fb_info *info, unsigned long arg)
{
	gfbg_par *par = NULL;
	par = (gfbg_par *)info->par;
	UNUSED(arg);

	if (gfbg_wait_regconfig_work(par->layer_id)) {
		TRACE_GFBG(DBG_ERR, "It is not support VBL!\n");
		return -EPERM;
	}

	return 0;
}

static int drv_gfbg_get_colorkey(struct fb_info *info, unsigned long arg)
{
	fb_colorkey colorkey = {0};
	gfbg_colorkeyex colorkey_ex = {0};
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	par = (gfbg_par *)info->par;

	gfbg_get_key(par, &colorkey_ex);

	colorkey.enable = colorkey_ex.key_enable;
	colorkey.value = colorkey_ex.key;

	return copy_to_user(argp, &colorkey, sizeof(fb_colorkey));
}

static int drv_gfbg_set_colorkey(struct fb_info *info, unsigned long arg)
{
	fb_colorkey colorkey;
	gfbg_colorkeyex colorkey_ex;
	unsigned long lock_flag;
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = (gfbg_par *)info->par;
	fb_color_format color_format;

	if (copy_from_user(&colorkey, argp, sizeof(fb_colorkey))) {
		return -EFAULT;
	}

	if ((colorkey.enable != true) && (colorkey.enable != false)) {
		TRACE_GFBG(DBG_ERR, "enable(%d) should be true or false!\n", colorkey.enable);
		return -1;
	}

	gfbg_get_fmt(par, &color_format);

	if (color_format != FB_FORMAT_ARGB8888) {
		TRACE_GFBG(DBG_ERR, "layer(%d) colorkey only support argb8888 fmt!\n", par->layer_id);
		return -1;
	}

	colorkey_ex.key = colorkey.value;
	colorkey_ex.key_enable = colorkey.enable;

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;
	gfbg_set_key(par, &colorkey_ex);
	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_COLORKEY;
	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);
	return 0;
}

static int drv_gfbg_get_layer_info(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	fb_layer_info layer_info = {0};
	par = (gfbg_par *)info->par;
	gfbg_get_layerinfo(par, &layer_info);
	return copy_to_user(argp, &layer_info, sizeof(fb_layer_info));
}

static int drv_gfbg_set_layer_info(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;

	par = (gfbg_par *)info->par;
	return gfbg_onputlayerinfo(info, par, argp);
}

static int drv_gfbg_refresh_layer(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	par = (gfbg_par *)info->par;
	return gfbg_onrefresh(par, argp);
}

static int drv_gfbg_get_canvas_buffer(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	fb_buf buf = {0};
	gfbg_refresh_info *refresh_info = NULL;
	par = (gfbg_par *)info->par;
	refresh_info = &par->refresh_info;
	memcpy(&(buf.canvas), &(par->canvas_sur), sizeof(fb_surface));
	memcpy(&(buf.update_rect), &(refresh_info->user_buffer.update_rect), sizeof(fb_rect));
	if (copy_to_user(argp, &(buf), sizeof(fb_buf))) {
		return -EFAULT;
	}
	return 0;
}

static int drv_gfbg_flip_surface(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	fb_surfaceex surface_ex;
	unsigned long lock_flag;
	gfbg_colorkeyex colorkey_ex = {0};

	par = (gfbg_par *)info->par;

	if (copy_from_user(&surface_ex, argp, sizeof(fb_surfaceex))) {
		return -EFAULT;
	}

	/* check surface is value or not */
	if (flip_surface_check_param(info, &surface_ex) != 0) {
		return -1;
	}

	/* refresh and color convert */
	if (flip_surface_pan_display(info, &surface_ex) != 0) {
		return -1;
	}

	colorkey_ex.key_enable = surface_ex.colorkey.enable;
	colorkey_ex.key = surface_ex.colorkey.value;

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;
	gfbg_set_key(par, &colorkey_ex);
	par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_COLORKEY;
	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	return 0;
}

static int drv_gfbg_compress_layer(struct fb_info *info, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	gfbg_par *par = NULL;
	bool is_compress = false;
	unsigned long lock_flag;
	vo_layer layer_id;
	char compre_name[16];
	unsigned long long compre_paddr = 0;
	void *compre_ion_v = NULL;
	int ret = 0;
	int i = 0;

	par = (gfbg_par *)info->par;
	layer_id = par->layer_id;

	if (copy_from_user(&is_compress, argp, sizeof(bool))) {
		return -EFAULT;
	}

	if ((is_compress != true) && (is_compress != false)) {
		TRACE_GFBG(DBG_ERR, "compress(%d) should be true or false!\n", is_compress);
		return -1;
	}

	if ((is_compress && (par->param_modify_mask & GFBG_LAYER_PARAMODIFY_COMPRESS)) ||
		(!is_compress && !(par->param_modify_mask & GFBG_LAYER_PARAMODIFY_COMPRESS))) {
		TRACE_GFBG(DBG_INFO, "Compression state is already %s!\n", is_compress ? "enabled" : "disabled");
		return 0;
	}

	spin_lock_irqsave(&par->lock, lock_flag);
	par->modifying = true;
	if (is_compress) {
		par->param_modify_mask |= GFBG_LAYER_PARAMODIFY_COMPRESS;
	} else {
		par->param_modify_mask &= ~GFBG_LAYER_PARAMODIFY_COMPRESS;
	}

	for (i = 0; i < 2; i++) {
		if (!g_layer[layer_id].compre_info[i].compre_paddr) {
			if (snprintf(compre_name, sizeof(compre_name), "compre_buffer%01u", i) < 0) {
				TRACE_GFBG(DBG_ERR, "%s:%d:snprintf failure\n", __func__, __LINE__);
				return -1;
			}
			ret = base_ion_alloc(&compre_paddr, &compre_ion_v, compre_name,
				g_layer[layer_id].compre_info[i].compre_size, true);
			if (ret != 0) {
				TRACE_GFBG(DBG_ERR, "%s:failed to malloc the oenc memory, size: %lu KBtyes!\n",
					compre_name, g_layer[layer_id].compre_info[i].compre_size);
				return -1;
			}

			memset(compre_ion_v, 0, g_layer[layer_id].compre_info[i].compre_size);
			g_layer[layer_id].compre_info[i].compre_paddr = compre_paddr;
			g_layer[layer_id].compre_info[i].compre_vaddr = compre_ion_v;
		}
	}

	par->modifying = false;
	spin_unlock_irqrestore(&par->lock, lock_flag);

	return ret;
}

int gfbg_oenc_irq_handler(int irq, void *data)
{
	struct fb_info *info = NULL;
	gfbg_par *par = NULL;
	struct gfbg_vo_dev *gfbg_vdev = data;
	int intr_status = oenc_intr_status();
	int index = 1 - g_layer[gfbg_vdev->layer_id].index;
	struct oenc_cfg oenc_cfg = {0};

	oenc_intr_clr(intr_status);

	base_ion_cache_flush(g_layer[gfbg_vdev->layer_id].compre_info[index].compre_paddr,
		(void *)g_layer[gfbg_vdev->layer_id].compre_info[index].compre_vaddr,
		g_layer[gfbg_vdev->layer_id].compre_info[index].compre_size);

	gfbg_hal_get_oenc(&oenc_cfg);
	g_layer[gfbg_vdev->layer_id].compre_info[index].oenc_cfg = oenc_cfg;

	/* get framebuffer info structure pointer */
	info = g_layer[gfbg_vdev->layer_id].info;
	if (info != NULL) {
		par = (gfbg_par *)(info->par);
		osal_sem_up(&par->oenc_sem);
	}
	g_layer[gfbg_vdev->layer_id].index = 1 - g_layer[gfbg_vdev->layer_id].index;
	return IRQ_HANDLED;
}
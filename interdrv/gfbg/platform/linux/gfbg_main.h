

#ifndef GFBG_MAIN_H
#define GFBG_MAIN_H

#include <linux/fb.h>
#include "comm_video.h"
#include "comm_gfbg.h"
#include "osal.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define GFBG_MAX_LAYER_NUM VO_MAX_GRAPHIC_LAYER_NUM
#define GFBG_DEF_WIDTH 1920
#define GFBG_DEF_HEIGHT 1080
#define GFBG_DEF_DEPTH 32
#define GFBG_DEF_STRIDE (GFBG_DEF_WIDTH * 2)
#define MAX_PALETTES 16
#define GFBG_MIN_WIDTH 16
#define GFBG_MIN_HEIGHT 16
#define GFBG_ALIGNMENT 0xf
#define GFBG_ALIGN 16
#define GFBG_MAX_ZOOMIN 2
#define GFBG_LINE_BUF 1920

/* mask bit */
typedef enum {
	GFBG_LAYER_PARAMODIFY_FMT              = 0x1,   /* color format */
	GFBG_LAYER_PARAMODIFY_STRIDE           = 0x2,   /* stride, line spacing */
	GFBG_LAYER_PARAMODIFY_COLORKEY         = 0x8,   /* colorkey */
	GFBG_LAYER_PARAMODIFY_INRECT           = 0x10,  /* input rect */
	GFBG_LAYER_PARAMODIFY_OUTRECT          = 0x20,  /* output rect */
	GFBG_LAYER_PARAMODIFY_DISPLAYADDR      = 0x40,  /* display addr */
	GFBG_LAYER_PARAMODIFY_SHOW             = 0x80,  /* show or hide */
	GFBG_LAYER_PARAMODIFY_COMPRESS         = 0x100, /* comp or not */
	GFBG_LAYER_PARAMODIFY_BUTT
} gfbg_layer_paramodify_maskbit;

typedef struct {
	struct fb_bitfield red;   /* bitfield in fb mem if true color, */
	struct fb_bitfield green; /* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp; /* transparency   */
} gfbg_argb_bitinfo;

typedef struct {
	fb_point pos;
	unsigned int display_width;
	unsigned int display_height;
	unsigned int screen_width;
	unsigned int screen_height;
	unsigned int vir_x_res;
	unsigned int vir_y_res;
	unsigned int x_res;
	unsigned int y_res;
	unsigned int max_screen_width;
	unsigned int max_screen_height;
} gfbg_display_info;

typedef struct {
	phys_addr_t phys_addr[2]; /* 2 for y or uv address */
	unsigned int stride;       /* buf stride */
	bool need_flip;
	unsigned int index_for_int;
	unsigned int int_pic_num;
	fb_rect union_rect;
	int refresh_handle;
} gfbg_dispbuf_info;

typedef struct {
	fb_layer_buf buf_mode; /* buffer mode */
	fb_buf user_buffer;
	phys_addr_t screen_addr;    /* screen buf addr */
	gfbg_dispbuf_info disp_buf_info;
	unsigned int refresh_num; /* refresh request num in 2 buf mode */
	bool do_refresh_job;
} gfbg_refresh_info;

typedef struct {
	bool key_enable;  /* colorkey */
	unsigned int key;
} gfbg_colorkeyex;

typedef struct {
	unsigned int regno;
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	unsigned int transp;
} gfbg_cmp_reg;

typedef struct {
	vo_layer layer_id;
	atomic_t ref_count;
	bool show;
	fb_color_format color_format;
	gfbg_colorkeyex ckey;
	gfbg_display_info display_info;
	gfbg_refresh_info refresh_info;
	unsigned int param_modify_mask;
	bool modifying;
	fb_surface canvas_sur;
	unsigned int vblflag;
	wait_queue_head_t vbl_event;
	wait_queue_head_t do_refresh_job;
	spinlock_t lock;
	bool layer_open;
	osal_semaphore oenc_sem;
} gfbg_par;

int gfbg_overlay_probe(vo_layer layer_id);
void gfbg_overlay_cleanup(vo_layer layer_id, bool unregister);
void gfbg_alloc_cmap(vo_layer layer_id);
void gfbg_free_cmap(vo_layer layer_id);
int gfbg_interrupt_process(vo_layer layer_id);
int gfbg_oenc_irq_handler(int irq, void *data);

phys_addr_t gfbg_get_smem_start(const struct fb_info *info);
char *gfbg_get_screen_base(const struct fb_info *info);
unsigned int gfbg_get_smem_len(const struct fb_info *info);
unsigned int gfbg_get_line_length(const struct fb_info *info);
unsigned int gfbg_get_xres(const struct fb_info *info);
unsigned int gfbg_get_yres(const struct fb_info *info);
unsigned int gfbg_get_xres_virtual(const struct fb_info *info);
unsigned int gfbg_get_yres_virtual(const struct fb_info *info);
unsigned int gfbg_get_bits_per_pixel(const struct fb_info *info);
unsigned int gfbg_get_yoffset(const struct fb_info *info);
unsigned int gfbg_get_xoffset(const struct fb_info *info);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

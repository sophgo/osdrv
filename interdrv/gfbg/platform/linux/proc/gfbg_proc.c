#include "gfbg_proc.h"
#include "gfbg_main.h"
#include "gfbg_debug.h"
#include "gfbg_ctrl.h"
#include "disp.h"

#define GFBG_ENTRY_NAME_RANGE 32
typedef struct {
	char entry_name[GFBG_ENTRY_NAME_RANGE];
	struct proc_dir_entry *entry;
} gfbg_proc_item;

static gfbg_proc_item g_proc_items[GFBG_MAX_LAYER_NUM];

// static const char *g_layer_name[] = {"layer_0", "layer_1", "layer_2", "layer_3", "layer_4", "layer_5"};
static const char * const g_layer_name[] = {"layer_0"};

static const char * const g_fmt_name[] = {
	"ARGB8888",
	"ARGB4444",
	"ARGB1555",
	"LUT256",
	"LUT16",
	"BUTT"
};

static void print_common_proc(struct seq_file *p, struct fb_info *info, const char *layer_name)
{
	gfbg_par *par = (gfbg_par *)info->par;
	gfbg_display_info *display_info = &par->display_info;
	gfbg_refresh_info *refresh_info = &par->refresh_info;
	static const char * const antiflicer_level[] = {"NONE", "LOW", "MIDDLE", "HIGH", "AUTO", "ERROR"};
	static const char * const mirror_mode[] = {"NONE", "HORIZONTAL", "VERTICAL", "BOTH", "unknown"};
	static const char * const dynamic_range[] = {"SDR8", "SDR10", "HDR10", "HLG", "SLF", "unknown"};
	static const char * const anti_mode[] = {"NONE", "TDE", "VOU", "ERROR"};
	static const char * const rotation_mode[] = {"0", "90", "180", "270", "-"};
	bool enable = false;
	if (atomic_read(&par->ref_count) > 0) {
		enable = true;
	}
	// (void)vo_gfbg_get_layer_enable(par->layer_id, &enable);

	seq_printf(p, "layer_name                 \t :%s\n", layer_name);
	seq_printf(p, "open_count                 \t :%d\n", atomic_read(&par->ref_count));
	seq_printf(p, "show_state                 \t :%s\n", (par->show) ? "ON" : "OFF");
	seq_printf(p, "graphic_enable             \t :%s\n", (enable == true) ? "ON" : "OFF");
	seq_printf(p, "start_position             \t :(%d, %d)\n", display_info->pos.x_pos,
		   display_info->pos.y_pos);
	seq_printf(p, "xres, yres                 \t :(%d, %d)\n", gfbg_get_xres(info), gfbg_get_yres(info));
	seq_printf(p, "xres_virtual, yres_virtual \t :(%d, %d)\n", gfbg_get_xres_virtual(info),
		gfbg_get_yres_virtual(info));
	seq_printf(p, "xoffset, yoffset           \t :(%d, %d)\n", gfbg_get_xoffset(info),
		gfbg_get_yoffset(info));
	seq_printf(p, "fix.line_length            \t :%d\n", gfbg_get_line_length(info));
	seq_printf(p, "mem_size:                  \t :%d KB\n", gfbg_get_smem_len(info) / 1024); /* 1024 1K */
	seq_printf(p, "layer_scale (hw):          \t :%s\n", "YES");
	seq_printf(p, "color_format:               \t :%s\n", g_fmt_name[par->color_format]);
	seq_printf(p, "colorkey_en                \t :%s\n", par->ckey.key_enable ? "ON" : "OFF");
	seq_printf(p, "colorkey_value             \t :0x%x\n", par->ckey.key);
	seq_printf(p, "mirror_mode:            \t :%s\n", mirror_mode[0]);
	seq_printf(p, "dynamic_range:            \t :%s\n", dynamic_range[0]);
	seq_printf(p, "deflicker_mode:            \t :%s\n", anti_mode[0]);
	seq_printf(p, "rotation_mode:             \t :%s\n", rotation_mode[0]);
	seq_printf(p, "deflicker_level:           \t :%s\n", antiflicer_level[0]);
	seq_printf(p, "gfbg_mode:                  \t :%s\n",
		   (refresh_info->buf_mode == FB_LAYER_BUF_BUTT) ? "STANDARD" : "EXTEND");
}

static void print_display_proc(struct seq_file *p, gfbg_par *par)
{
	static const char * const buf_mode[] = {"triple", "double", "single", "triple( no frame discarded)", "unknown"};
	gfbg_display_info *display_info = &par->display_info;
	gfbg_refresh_info *refresh_info = &par->refresh_info;

	seq_printf(p, "display_buffer_mode(+usr_buf)\t :%s\n", buf_mode[refresh_info->buf_mode]);
	seq_printf(p, "displaying_addr (register) \t :0x%lx\n",
		   (unsigned long)refresh_info->screen_addr);
	seq_printf(p, "display_buffer[0] addr     \t :0x%lx\n",
		   (unsigned long)refresh_info->disp_buf_info.phys_addr[0]);
	seq_printf(p, "display_buffer[1] addr     \t :0x%lx\n",
		   (unsigned long)refresh_info->disp_buf_info.phys_addr[1]);
	seq_printf(p, "is_premul_mode:            \t :%s\n", "NO");
	seq_printf(p, "display_rect                \t :(%d, %d)\n", display_info->display_width,
		display_info->display_height);
	seq_printf(p, "screen_rect                 \t :(%d, %d)\n", display_info->screen_width,
		display_info->screen_height);
	seq_printf(p, "device_max_resolution      \t :%d, %d\n", display_info->max_screen_width,
		display_info->max_screen_height);
	seq_printf(p, "is_need_flip(2buf)           \t :%s\n",
		refresh_info->disp_buf_info.need_flip ? "YES" : "NO");
	seq_printf(p, "buf_index_displaying(2buf)\t :%d\n", refresh_info->disp_buf_info.index_for_int);
	seq_printf(p, "refresh_request_num(2buf)  \t :%d\n", refresh_info->refresh_num);
	seq_printf(p, "switch_buf_num(2buf)       \t :%d\n", refresh_info->disp_buf_info.int_pic_num);
	seq_printf(p, "union_rect (2buf)          \t :(%d,%d,%d,%d)\n",
		   refresh_info->disp_buf_info.union_rect.x, refresh_info->disp_buf_info.union_rect.y,
		   refresh_info->disp_buf_info.union_rect.width, refresh_info->disp_buf_info.union_rect.height);
}

static void print_canvas_proc(struct seq_file *p, const gfbg_par *par)
{
	const gfbg_refresh_info *refresh_info = &par->refresh_info;

	seq_printf(p, "canavas_updated_addr       \t :0x%lx\n",
		   (unsigned long)refresh_info->user_buffer.canvas.phys_addr +
		   refresh_info->user_buffer.update_rect.y * refresh_info->user_buffer.update_rect.width +
		   refresh_info->user_buffer.update_rect.x);
	seq_printf(p, "canavas_updated (w, h)     \t :%d,%d\n",
		   refresh_info->user_buffer.update_rect.width,
		   refresh_info->user_buffer.update_rect.height);
	seq_printf(p, "canvas_width               \t :%d\n", refresh_info->user_buffer.canvas.width);
	seq_printf(p, "canvas_height              \t :%d\n", refresh_info->user_buffer.canvas.height);
	seq_printf(p, "canvas_pitch               \t :%d\n", refresh_info->user_buffer.canvas.pitch);
	seq_printf(p, "canvas_format              \t :%s\n", g_fmt_name[refresh_info->user_buffer.canvas.format]);
	seq_printf(p, "is_compress                 \t :%s\n", "NO");
}

static int gfbg_print_layer_proc(struct fb_info *info, struct seq_file *p, void *v)
{
	gfbg_par *par = (gfbg_par *)info->par;
	const char *layer_name = NULL;
	UNUSED(v);

	if (par->layer_id >= sizeof(g_layer_name) / sizeof(char *)) {
		layer_name = "unknown layer";
	} else {
		layer_name = g_layer_name[par->layer_id];
	}

	print_common_proc(p, info, layer_name);

	print_display_proc(p, par);

	print_canvas_proc(p, par);

	return 0;
}

static int gfbg_read_proc(struct seq_file *m, void *v)
{
	struct fb_info *info = NULL;
	gfbg_par *par = NULL;
	if (m == NULL) {
		return -1;
	}
	info = (struct fb_info *)(m->private);
	if (info == NULL) {
		return -1;
	}
	par = (gfbg_par *)info->par;
	if (par == NULL) {
		return -1;
	}

	return gfbg_print_layer_proc(info, m, NULL);
}

static int gfbg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, gfbg_read_proc, PDE_DATA(inode));
}

static const struct proc_ops gfbg_proc_fops = {
	.proc_open = gfbg_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

void gfbg_proc_add_module(const char *entry_name, void *data)
{
	int i;
	for (i = 0; i < GFBG_MAX_LAYER_NUM; i++) {
		if (!g_proc_items[i].entry) {
			break;
		}
	}

	if (i == GFBG_MAX_LAYER_NUM) {
		TRACE_GFBG(DBG_ERR, "gfbg proc num full.\n");
		return;
	}

	g_proc_items[i].entry = proc_create_data(entry_name, 0644, NULL, &gfbg_proc_fops, data);
	if (!g_proc_items[i].entry) {
		TRACE_GFBG(DBG_ERR, "gfbg proc create failed\n");
		return;
	}

	memcpy(g_proc_items[i].entry_name, entry_name, strlen(entry_name) + 1);
}

void gfbg_proc_remove_module(const char *entry_name)
{
	int i;

	if (entry_name == NULL) {
		return;
	}

	for (i = 0; i < GFBG_MAX_LAYER_NUM; i++) {
		if (!strcmp(g_proc_items[i].entry_name, entry_name)) {
			break;
		}
	}

	if (i == GFBG_MAX_LAYER_NUM) {
		return;
	}

	remove_proc_entry(entry_name, NULL);
	(void)memset(&g_proc_items[i], 0, sizeof(gfbg_proc_item));
}

void gfbg_proc_remove_all_module(void)
{
	int i;

	for (i = 0; i < GFBG_MAX_LAYER_NUM; i++) {
		if (g_proc_items[i].entry == NULL) {
			continue;
		}

		remove_proc_entry(g_proc_items[i].entry_name, NULL);
		(void)memset(&g_proc_items[i], 0, sizeof(gfbg_proc_item));
	}
}

void gfbg_proc_init(void)
{
	(void)memset(g_proc_items, 0, sizeof(g_proc_items));
}

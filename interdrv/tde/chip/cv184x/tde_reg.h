#ifndef __TDE_REG_H__
#define __TDE_REG_H__


enum tde_mode {
	TDE_MODE_ROTATE_90,
	TDE_MODE_ROTATE_270,
	TDE_MODE_DRAW_LINE,
	TDE_MODE_COPY,
	TDE_MODE_MAX
};

struct tde_surface_mem {
	u64 addr;
	u32 stride;
	u32 width;
	u32 height;
};

struct tde_line_cfg {
	u32 start_x;
	u32 start_y;
	u32 end_x;
	u32 end_y;
	u32 color;
	u32 thick;
};


void tde_set_base_addr(void *base_addr);
void tde_set_mode(enum tde_mode mode);
void tde_set_src_cfg(struct tde_surface_mem *cfg);
void tde_set_dst_cfg(struct tde_surface_mem *cfg);
void tde_set_draw_line_cfg(struct tde_line_cfg *cfg);

void tde_start(void);
void tde_clk_gate(bool enable);
u32 tde_intr_status(void);
void tde_clear_intr(void);
void tde_toggle_reset(void);

#endif

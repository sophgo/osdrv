#include "tde_debug.h"
#include "tde_reg.h"
#include "tde_ip_ctrl.h"


static void tde_run_general(enum tde_mode mode, struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer)
{
	struct tde_surface_mem src_mem, dst_mem;

	TRACE_TDE(DBG_INFO, "--------------------\n");
	TRACE_TDE(DBG_INFO, "mode(%d).\n", mode);
	TRACE_TDE(DBG_INFO, "src: w(%d) h(%d) stirde(%d) addr(%llx)\n",
		src_buffer->width, src_buffer->height, src_buffer->stride, src_buffer->addr);
	TRACE_TDE(DBG_INFO, "dst: w(%d) h(%d) stirde(%d) addr(%llx)\n",
		dst_buffer->width, dst_buffer->height, dst_buffer->stride, dst_buffer->addr);

	src_mem.addr = src_buffer->addr;
	src_mem.stride = src_buffer->stride;
	src_mem.width = src_buffer->width;
	src_mem.height = src_buffer->height;

	dst_mem.addr = dst_buffer->addr;
	dst_mem.stride = dst_buffer->stride;
	dst_mem.width = dst_buffer->width;
	dst_mem.height = dst_buffer->height;

	tde_set_mode(mode);
	tde_set_src_cfg(&src_mem);
	tde_set_dst_cfg(&dst_mem);
	tde_start();
}

void tde_run_rotate90(struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer)
{
	tde_run_general(TDE_MODE_ROTATE_90, src_buffer, dst_buffer);
}

void tde_run_rotate270(struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer)
{
	tde_run_general(TDE_MODE_ROTATE_270, src_buffer, dst_buffer);
}

void tde_run_draw_line(struct tde_line_attr *attr, struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer)
{
	struct tde_line_cfg cfg;
	struct tde_surface_mem src_mem, dst_mem;

	TRACE_TDE(DBG_INFO, "--------------------\n");
	TRACE_TDE(DBG_INFO, "mode: draw line.\n");
	TRACE_TDE(DBG_INFO, "src: w(%d) h(%d) stirde(%d) addr(%llx)\n",
		src_buffer->width, src_buffer->height, src_buffer->stride, src_buffer->addr);
	TRACE_TDE(DBG_INFO, "dst: w(%d) h(%d) stirde(%d) addr(%llx)\n",
		dst_buffer->width, dst_buffer->height, dst_buffer->stride, dst_buffer->addr);
	TRACE_TDE(DBG_INFO, "line: start(%d %d) end(%d %d) color(%x) thick(%d)\n",
		attr->start_x, attr->start_y, attr->end_x, attr->end_y, attr->color, attr->thick);

	tde_set_mode(TDE_MODE_DRAW_LINE);

	src_mem.addr = src_buffer->addr;
	src_mem.stride = src_buffer->stride;
	src_mem.width = src_buffer->width;
	src_mem.height = src_buffer->height;
	tde_set_src_cfg(&src_mem);

	dst_mem.addr = dst_buffer->addr;
	dst_mem.stride = dst_buffer->stride;
	dst_mem.width = dst_buffer->width;
	dst_mem.height = dst_buffer->height;
	tde_set_dst_cfg(&dst_mem);

	cfg.start_x = attr->start_x;
	cfg.start_y = attr->start_y;
	cfg.end_x = attr->end_x;
	cfg.end_y = attr->end_y;
	cfg.color = attr->color;
	cfg.thick = attr->thick;

	tde_set_draw_line_cfg(&cfg);
	tde_start();
}

void tde_run_copy(struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer)
{
	tde_run_general(TDE_MODE_COPY, src_buffer, dst_buffer);
}

void tde_ip_init(void)
{
	tde_clk_gate(true);
	tde_clear_intr();
}

void tde_ip_deinit(void)
{
	tde_clk_gate(false);
}

u32 tde_ip_clear_intr(void)
{
	u32 status = tde_intr_status();

	tde_clear_intr();
	TRACE_TDE(DBG_DEBUG, "intr status(%x).\n", status);

	return status;
}

void tde_ip_reset(void)
{
	tde_toggle_reset();
}


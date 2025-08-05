#include <linux/types.h>

#include "tde_debug.h"
#include "reg.h"
#include "osal.h"
#include "tde_reg.h"
#include "tde_reg_tab.h"
#include "comm_math.h"


#define REG_VI_SYS_BASE 0x0a090000

static uintptr_t reg_base;


void tde_set_base_addr(void *base_addr)
{
	reg_base = (uintptr_t)base_addr;
}

void tde_set_mode(enum tde_mode mode)
{
	u32 value;

	value = BIT(0); //enable
	value |= mode << 16; //mode

	_reg_write(reg_base + TDE_REG_SURFACE_REG0, value);
}

void tde_set_src_cfg(struct tde_surface_mem *cfg)
{
	_reg_write(reg_base + TDE_REG_SRC_SURFACE_ADDR_L, cfg->addr);
	_reg_write(reg_base + TDE_REG_SRC_SURFACE_ADDR_H, cfg->addr >> 32);
	_reg_write(reg_base + TDE_REG_SRC_SURFACE_STRIDE, cfg->stride);
	_reg_write(reg_base + TDE_REG_SRC_SURFACE_WIDTH, cfg->width);
	_reg_write(reg_base + TDE_REG_SRC_SURFACE_HEIGHT, cfg->height);

	_reg_write(reg_base + TDE_REG_SRC_SURFACE_PIXEL_4B, 0);
	_reg_write(reg_base + TDE_REG_SRC_SURFACE_PIXEL_2B, 0);
}

void tde_set_dst_cfg(struct tde_surface_mem *cfg)
{
	_reg_write(reg_base + TDE_REG_DST_SURFACE_ADDR_L, cfg->addr);
	_reg_write(reg_base + TDE_REG_DST_SURFACE_ADDR_H, cfg->addr >> 32);
	_reg_write(reg_base + TDE_REG_DST_SURFACE_STRIDE, cfg->stride);
	_reg_write(reg_base + TDE_REG_DST_SURFACE_WIDTH, cfg->width);
	_reg_write(reg_base + TDE_REG_DST_SURFACE_HEIGHT, cfg->height);
}

void tde_set_draw_line_cfg(struct tde_line_cfg *cfg)
{
	_reg_write(reg_base + TDE_REG_DRAW_LINE_CTRL0, cfg->start_x);
	_reg_write(reg_base + TDE_REG_DRAW_LINE_CTRL1, cfg->start_y);
	_reg_write(reg_base + TDE_REG_DRAW_LINE_CTRL2, cfg->end_x);
	_reg_write(reg_base + TDE_REG_DRAW_LINE_CTRL3, cfg->end_y);
	_reg_write(reg_base + TDE_REG_DRAW_LINE_CTRL4, cfg->color);
	_reg_write(reg_base + TDE_REG_DRAW_LINE_CTRL5, cfg->thick);
}

void tde_start(void)
{
	_reg_write(reg_base + TDE_REG_START_CTRL, 0x1);
}

void tde_clk_gate(bool enable)
{
	u32 value = enable ? 0x1 : 0x0;

	_reg_write(reg_base + TDE_REG_CLK_GATE, value);
}

u32 tde_intr_status(void)
{
	return _reg_read(reg_base + TDE_REG_INTR_STATUS) & 0x1;
}

void tde_clear_intr(void)
{
	_reg_write(reg_base + TDE_REG_INTR_CLEAR, 0x1);
}

void tde_toggle_reset(void)
{
	void *vi_sys_addr = NULL;
	u32 value = BIT(21);

	vi_sys_addr = osal_ioremap((unsigned long)REG_VI_SYS_BASE, 4);
	if (!vi_sys_addr) {
		TRACE_TDE(DBG_ERR, "osal_ioremap fail.\n");
		return;
	}
	_reg_write((uintptr_t)vi_sys_addr, value);
	osal_udelay(20);
	_reg_write((uintptr_t)vi_sys_addr, 0);
	osal_iounmap(vi_sys_addr, 4);
}



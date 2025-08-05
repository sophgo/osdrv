#ifdef ENVTEST
#include <common.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "system_common.h"
#include "timer.h"
#elif defined(ENV_EMU)
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "emu/command.h"
#else
#include "osal_types.h"
#endif  // ENVTEST

#include "reg.h"
#include "vi_sys.h"
#include "ldc_reg.h"
#include "ldc_cfg.h"
#include "ldc.h"
#include "cmdq.h"
#include "ldc_ctx.h"
#include "ldc_debug.h"
#include "ldc_common.h"

/****************************************************************************
 * Global parameters
 ***************************************************************************/
static uintptr_t reg_base[LDC_DEV_MAX_CNT];

/****************************************************************************
 * Initial info
 ***************************************************************************/
const unsigned char hcoeff_tap[COEFFICIENT_PHASE_NUM][4] = {
	{0x00, 0x40, 0x00, 0x00},
	{0xff, 0x40, 0x01, 0x00},
	{0xfe, 0x3f, 0x02, 0x01},
	{0xfe, 0x3f, 0x04, 0xff},
	{0xfd, 0x3e, 0x06, 0xff},
	{0xfc, 0x3c, 0x08, 0x00},
	{0xfc, 0x3b, 0x0a, 0xff},
	{0xfc, 0x39, 0x0c, 0xff},
	{0xfb, 0x38, 0x0f, 0xfe},
	{0xfb, 0x35, 0x11, 0xff},
	{0xfb, 0x33, 0x14, 0xfe},
	{0xfb, 0x31, 0x16, 0xfe},
	{0xfb, 0x2f, 0x19, 0xfd},
	{0xfb, 0x2c, 0x1c, 0xfd},
	{0xfc, 0x29, 0x1e, 0xfd},
	{0xfc, 0x27, 0x21, 0xfc},
	{0xfc, 0x24, 0x24, 0xfc},
	{0xfc, 0x21, 0x27, 0xfc},
	{0xfd, 0x1e, 0x29, 0xfc},
	{0xfd, 0x1c, 0x2c, 0xfb},
	{0xfd, 0x19, 0x2f, 0xfb},
	{0xfe, 0x16, 0x31, 0xfb},
	{0xfe, 0x14, 0x33, 0xfb},
	{0xfe, 0x11, 0x35, 0xfc},
	{0xfe, 0x0f, 0x38, 0xfb},
	{0xff, 0x0c, 0x39, 0xfc},
	{0xff, 0x0a, 0x3b, 0xfc},
	{0xff, 0x08, 0x3c, 0xfd},
	{0x00, 0x06, 0x3e, 0xfc},
	{0x00, 0x04, 0x3f, 0xfd},
	{0x00, 0x02, 0x3f, 0xff},
	{0x00, 0x01, 0x40, 0xff},
};
const unsigned char vcoeff_tap[COEFFICIENT_PHASE_NUM][4] = {
	{0x00, 0x40, 0x00, 0x00},
	{0xff, 0x40, 0x01, 0x00},
	{0xfe, 0x3f, 0x02, 0x01},
	{0xfe, 0x3f, 0x04, 0xff},
	{0xfd, 0x3e, 0x06, 0xff},
	{0xfc, 0x3c, 0x08, 0x00},
	{0xfc, 0x3b, 0x0a, 0xff},
	{0xfc, 0x39, 0x0c, 0xff},
	{0xfb, 0x38, 0x0f, 0xfe},
	{0xfb, 0x35, 0x11, 0xff},
	{0xfb, 0x33, 0x14, 0xfe},
	{0xfb, 0x31, 0x16, 0xfe},
	{0xfb, 0x2f, 0x19, 0xfd},
	{0xfb, 0x2c, 0x1c, 0xfd},
	{0xfc, 0x29, 0x1e, 0xfd},
	{0xfc, 0x27, 0x21, 0xfc},
	{0xfc, 0x24, 0x24, 0xfc},
	{0xfc, 0x21, 0x27, 0xfc},
	{0xfd, 0x1e, 0x29, 0xfc},
	{0xfd, 0x1c, 0x2c, 0xfb},
	{0xfd, 0x19, 0x2f, 0xfb},
	{0xfe, 0x16, 0x31, 0xfb},
	{0xfe, 0x14, 0x33, 0xfb},
	{0xfe, 0x11, 0x35, 0xfc},
	{0xfe, 0x0f, 0x38, 0xfb},
	{0xff, 0x0c, 0x39, 0xfc},
	{0xff, 0x0a, 0x3b, 0xfc},
	{0xff, 0x08, 0x3c, 0xfd},
	{0x00, 0x06, 0x3e, 0xfc},
	{0x00, 0x04, 0x3f, 0xfd},
	{0x00, 0x02, 0x3f, 0xff},
	{0x00, 0x01, 0x40, 0xff},

};

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void ldc_dump_register(int top_id);
extern bool ldc_dump_reg;

void ldc_set_base_addr(void *base, int top_id)
{
	reg_base[top_id] = (uintptr_t)base;
}

/**
 * ldc_init - setup ldc, mainly interpolation settings.
 *
 * @param cfg: settings for this ldc's operation
 */
void ldc_init(int top_id)
{
	(void)top_id;
}

void ldc_disable(int top_id)
{
	(void)top_id;
}

/**
 * ldc_reset - do reset. This can be activated only if dma stop to avoid hang
 *	       fabric.
 *
 */
void ldc_reset(int top_id)
{
	union vi_sys_reset mask;
	union vi_sys_reset_apb mask_apb;

	mask.raw = 0;
	mask_apb.raw = 0;

	mask.b.ldc = 1;
	mask_apb.b.ldc = 1;
	vi_sys_toggle_reset(mask);
	vi_sys_toggle_reset_apb(mask_apb);
}

/**
 * ldc_intr_ctrl - ldc's interrupt on(1)/off(0)
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void ldc_intr_ctrl(unsigned char intr_mask, int top_id)
{
	_reg_write(reg_base[top_id] + REG_LDC_IRQEN, intr_mask);
}

/**
 * ldc_intr_clr - clear ldc's interrupt
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void ldc_intr_clr(unsigned char intr_mask, int top_id)
{
	_reg_write(reg_base[top_id] + REG_LDC_IRQCLR, intr_mask);
}

/**
 * ldc_intr_status - ldc's interrupt status
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @return: The interrupt's status. 1 if active.
 */
unsigned char ldc_intr_status(int top_id)
{
	return _reg_read(reg_base[top_id] + REG_LDC_IRQSTAT);
}

/**
 * ldc_intr_sel - ldc's interrupt selection
 *
 * @param sel
 *      0: ldc_interrupt
 *      1: cmdq interrupt
 */
void ldc_intr_sel(unsigned char sel, int top_id)
{
	_reg_write(reg_base[top_id] + REG_LDC_INT_SEL, sel);
}

/**
 * ldc_check_param - check if config param is valid.
 *
 * @param cfg: settings for this ldc's operation
 * @return: true for valid.
 */
bool ldc_check_param(const struct ldc_cfg *cfg)
{
	if ((cfg->src_width > LDC_MAX_WIDTH) || (cfg->src_height > LDC_MAX_HEIGHT)) {
		return false;
	}
	if ((cfg->src_width & (LDC_SIZE_ALIGN - 1)) ||
	    (cfg->src_height & (LDC_SIZE_ALIGN - 1))) {
		return false;
	}
	if ((cfg->src_xstart > cfg->src_xend) ||
	    (cfg->src_xend - cfg->src_xstart < 32) ||
	    (cfg->src_xstart > cfg->src_width - 1) ||
	    (cfg->src_xend > cfg->src_width - 1)) {
		return false;
	}
	if (cfg->map_base & (LDC_ADDR_ALIGN - 1)) {
		return false;
	}
	if ((cfg->src_y_base & (LDC_ADDR_ALIGN - 1)) ||
	    (cfg->src_c_base & (LDC_ADDR_ALIGN - 1))) {
		return false;
	}
	if ((cfg->dst_y_base & (LDC_ADDR_ALIGN - 1)) ||
	    (cfg->dst_c_base & (LDC_ADDR_ALIGN - 1))) {
		return false;
	}
	return true;
}

/**
 * ldc_engine - start a ldc operation, wait frame_done intr after this.
 *              If output target is scaler, scaler's config should be done
 *              before this.
 *
 * @param cfg: settings for this ldc's operation
 */
void ldc_engine(const struct ldc_cfg *cfg, int top_id)
{
	unsigned char ras_mode = (cfg->dst_mode == LDC_DST_FLAT) ? 0 : 1;

	_reg_write(reg_base[top_id] + REG_LDC_DATA_FORMAT, cfg->pix_fmt);
	_reg_write(reg_base[top_id] + REG_LDC_RAS_MODE, ras_mode);
	_reg_write(reg_base[top_id] + REG_LDC_RAS_XSIZE, cfg->ras_width);
	_reg_write(reg_base[top_id] + REG_LDC_RAS_YSIZE, cfg->ras_height);

	_reg_write(reg_base[top_id] + REG_LDC_MAP_BASE, (cfg->map_base >> LDC_BASE_ADDR_SHIFT) & 0x7ffffff);
	_reg_write(reg_base[top_id] + REG_LDC_MAP_BYPASS, cfg->map_bypass);

	_reg_write(reg_base[top_id] + REG_LDC_SRC_BASE_Y, (cfg->src_y_base >> LDC_BASE_ADDR_SHIFT) & 0x7ffffff);
	_reg_write(reg_base[top_id] + REG_LDC_SRC_BASE_C, (cfg->src_c_base >> LDC_BASE_ADDR_SHIFT) & 0x7ffffff);
	_reg_write(reg_base[top_id] + REG_LDC_SRC_XSIZE, cfg->src_width);
	_reg_write(reg_base[top_id] + REG_LDC_SRC_YSIZE, cfg->src_height);
	_reg_write(reg_base[top_id] + REG_LDC_SRC_XSTART, cfg->src_xstart);
	_reg_write(reg_base[top_id] + REG_LDC_SRC_XEND, cfg->src_xend);
	_reg_write(reg_base[top_id] + REG_LDC_SRC_BG, cfg->bgcolor);

	_reg_write(reg_base[top_id] + REG_LDC_DST_BASE_Y, (cfg->dst_y_base >> LDC_BASE_ADDR_SHIFT) & 0x7ffffff);
	_reg_write(reg_base[top_id] + REG_LDC_DST_BASE_C, (cfg->dst_c_base >> LDC_BASE_ADDR_SHIFT) & 0x7ffffff);

	_reg_write(reg_base[top_id] + REG_LDC_EXTEND_HADDR, cfg->extend_haddr);

	_reg_write(reg_base[top_id] + REG_LDC_DST_MODE, cfg->dst_mode);

	_reg_write(reg_base[top_id] + REG_LDC_INT_SEL, 0); // 0: ldc intr, 1: cmd intr

	if ((ldc_dump_reg))
		ldc_dump_register(top_id);

	// start ldc
	_reg_write(reg_base[top_id] + REG_LDC_START, 1);

}

unsigned char ldc_cmdq_intr_status(unsigned char top_id)
{
	return cmdq_intr_status(reg_base[top_id] + REG_LDC_CMDQ_BASE);
}

void ldc_cmdq_intr_clr(unsigned char top_id, unsigned char intr_status)
{
	cmdq_intr_clr(reg_base[top_id] + REG_LDC_CMDQ_BASE, intr_status);
}

void ldc_cmdq_sw_restart(unsigned char top_id)
{
	cmdq_sw_restart(reg_base[top_id] + REG_LDC_CMDQ_BASE);
}

bool ldc_cmdq_is_sw_restart(unsigned char top_id)
{
	return cmdq_is_sw_restart(reg_base[top_id] + REG_LDC_CMDQ_BASE);
}

/**
 * ldc_is_finish - check if ldc's operation is finished.
 *              ldc_start can only be toggled if only dma done(frame_done intr),
 *              ow dma won't finished.
 */
bool ldc_is_finish(int top_id)
{
	return true;
}

void ldc_dump_register(int top_id)
{
	unsigned int val;

	TRACE_LDC(DBG_DEBUG, "LDC_FORMAT=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_DATA_FORMAT));
	TRACE_LDC(DBG_DEBUG, "LDC_RAS_MODE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_RAS_MODE));
	TRACE_LDC(DBG_DEBUG, "LDC_RAS_XSIZE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_RAS_XSIZE));
	TRACE_LDC(DBG_DEBUG, "LDC_RAS_YSIZE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_RAS_YSIZE));

	val = _reg_read(reg_base[top_id] + REG_LDC_MAP_BASE);
	TRACE_LDC(DBG_DEBUG, "LDC_MAP_BASE=0x%08x\n", val);
	TRACE_LDC(DBG_DEBUG, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	TRACE_LDC(DBG_DEBUG, "LDC_MAP_BYPASS=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_MAP_BYPASS));

	val = _reg_read(reg_base[top_id] + REG_LDC_SRC_BASE_Y);
	TRACE_LDC(DBG_DEBUG, "LDC_SRC_BASE_Y=0x%08x\n", val);
	TRACE_LDC(DBG_DEBUG, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	val = _reg_read(reg_base[top_id] + REG_LDC_SRC_BASE_C);
	TRACE_LDC(DBG_DEBUG, "LDC_SRC_BASE_C=0x%08x\n", val);
	TRACE_LDC(DBG_DEBUG, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	TRACE_LDC(DBG_DEBUG, "LDC_SRC_XSIZE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_SRC_XSIZE));
	TRACE_LDC(DBG_DEBUG, "LDC_SRC_YSIZE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_SRC_YSIZE));
	TRACE_LDC(DBG_DEBUG, "LDC_SRC_XSTR=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_SRC_XSTART));
	TRACE_LDC(DBG_DEBUG, "LDC_SRC_XEND=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_SRC_XEND));
	TRACE_LDC(DBG_DEBUG, "LDC_SRC_BG=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_SRC_BG));

	val = _reg_read(reg_base[top_id] + REG_LDC_DST_BASE_Y);
	TRACE_LDC(DBG_DEBUG, "LDC_DST_BASE_Y=0x%08x\n", val);
	TRACE_LDC(DBG_DEBUG, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	val = _reg_read(reg_base[top_id] + REG_LDC_DST_BASE_C);
	TRACE_LDC(DBG_DEBUG, "LDC_DST_BASE_C=0x%08x\n", val);
	TRACE_LDC(DBG_DEBUG, "    addr=0x%08x\n", val << LDC_BASE_ADDR_SHIFT);

	TRACE_LDC(DBG_DEBUG, "LDC_DST_MODE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_DST_MODE));
	TRACE_LDC(DBG_DEBUG, "LDC_IRQEN=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_IRQEN));
	TRACE_LDC(DBG_DEBUG, "LDC_START=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_START));
	TRACE_LDC(DBG_DEBUG, "LDC_IRQSTAT=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_IRQSTAT));
	TRACE_LDC(DBG_DEBUG, "LDC_IRQCLR=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_IRQCLR));
	TRACE_LDC(DBG_DEBUG, "LDC_LDC_DIR=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_DIR));
	TRACE_LDC(DBG_DEBUG, "LDC_CMDQ_IRQ_EN=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_INT_SEL));
	TRACE_LDC(DBG_DEBUG, "LDC_FORCE_IN_RANGE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_FORCE_IN_RANGE));
	TRACE_LDC(DBG_DEBUG, "LDC_OUT_RANGE=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_OUT_RANGE));
	TRACE_LDC(DBG_DEBUG, "LDC_OUT_RANGE_DST_X=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_OUT_RANGE_DST_X));
	TRACE_LDC(DBG_DEBUG, "LDC_OUT_RANGE_DST_Y=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_OUT_RANGE_DST_Y));
	TRACE_LDC(DBG_DEBUG, "LDC_OUT_RANGE_SRC_X=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_OUT_RANGE_SRC_X));
	TRACE_LDC(DBG_DEBUG, "LDC_OUT_RANGE_SRC_Y=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_OUT_RANGE_SRC_Y));
	TRACE_LDC(DBG_DEBUG, "LDC_DST_TI_CNT_X=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_DST_TI_CNT_X));
	TRACE_LDC(DBG_DEBUG, "LDC_DST_TI_CNT_Y=0x%08x\n", _reg_read(reg_base[top_id] + REG_LDC_DST_TI_CNT_Y));
}

void ldc_dump_cmdq(unsigned long long cmdq_addr, unsigned int num_cmd)
{
	unsigned int i;
	union cmdq_set *cmd_start = (union cmdq_set *)(uintptr_t)cmdq_addr;

	TRACE_LDC(DBG_DEBUG, "cmdq vir addr=0x%08llx, num=%d\n", cmdq_addr, num_cmd);
	for (i = 0; i < num_cmd; i++) {
		TRACE_LDC(DBG_DEBUG, "[%02d] [0x%08x]=0x%08x\n",
			i, cmd_start[i].reg.addr << 2,
			cmd_start[i].reg.data);
	}
}

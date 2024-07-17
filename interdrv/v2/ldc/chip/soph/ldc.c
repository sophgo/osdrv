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
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/dma-buf.h>
#include <asm/cacheflush.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif
#endif  // ENVTEST

#include "vi_sys.h"
#include "ldc_reg.h"
#include "ldc_cfg.h"
#include "ldc.h"
#include "cmdq.h"
#include "ldc_common.h"
#include "reg.h"
#include "ldc_debug.h"

/****************************************************************************
 * Global parameters
 ***************************************************************************/
static uintptr_t reg_base[LDC_DEV_MAX_CNT];

/****************************************************************************
 * Initial info
 ***************************************************************************/

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

	if (top_id) {
		mask.b.ldc1 = 1;
		mask_apb.b.ldc1 = 1;
	} else {
		mask.b.ldc0 = 1;
		mask_apb.b.ldc0 = 1;
	}
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

	if (unlikely(ldc_dump_reg))
		ldc_dump_register(top_id);

	// start ldc
	_reg_write(reg_base[top_id] + REG_LDC_START, 1);
}

/**
 * ldc_engine_cmdq - start a ldc operation, wait frame_done intr after this.
 *                   If output target is scaler, scaler's config should be done
 *                   before this.
 *
 * @param cfg: settings for cmdq
 * @param cnt: the number of settings
 * @param cmdq_addr: memory-address to put cmdq
 */
void ldc_engine_cmdq(int top_id, const void *cmdq_addr, struct ldc_cfg **cfgs, unsigned char cnt)
{
	unsigned char ras_mode;
	unsigned char cmd_idx = 0, i;
	union cmdq_set *cmd_start = (union cmdq_set *)cmdq_addr;
	unsigned long long ldc_apb_base = top_id ? REG_LDC1_BASE_FOR_CMDQ : REG_LDC0_BASE_FOR_CMDQ;

	//memset(cmd_start, 0, sizeof(union cmdq_set) * LDC_CMDQ_MAX_REG_CNT * cnt);
	for (i = 0; i < cnt; ++i) {
		struct ldc_cfg *cfg = cfgs[i];

		ras_mode = (cfg->dst_mode == LDC_DST_FLAT) ? 0 : 1;
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_DATA_FORMAT, cfg->pix_fmt);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_RAS_MODE, ras_mode);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_RAS_XSIZE, cfg->ras_width);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_RAS_YSIZE, cfg->ras_height);

		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_MAP_BASE, cfg->map_base >> LDC_BASE_ADDR_SHIFT);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_MAP_BYPASS, cfg->map_bypass);

		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_BASE_Y, cfg->src_y_base >> LDC_BASE_ADDR_SHIFT);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_BASE_C, cfg->src_c_base >> LDC_BASE_ADDR_SHIFT);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_XSIZE, cfg->src_width);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_YSIZE, cfg->src_height);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_XSTART, cfg->src_xstart);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_XEND, cfg->src_xend);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_SRC_BG, cfg->bgcolor);

		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_DST_BASE_Y, cfg->dst_y_base >> LDC_BASE_ADDR_SHIFT);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_DST_BASE_C, cfg->dst_c_base >> LDC_BASE_ADDR_SHIFT);
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_DST_MODE, cfg->dst_mode);

		// Enable cmdq interrupt
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_INT_SEL,
				 1); // 0: ldc intr, 1: cmd intr
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_IRQEN,
				 1);

		// start ldc
		cmdq_set_package(&cmd_start[cmd_idx++].reg, ldc_apb_base + REG_LDC_START, 1);

		cmdq_set_wait(&cmd_start[cmd_idx++], false, 1, 0x1);
#if 1
		// clear intr
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
			ldc_apb_base + REG_LDC_IRQCLR, 0x01);

		//cmdq_set_wait(&cmd_start[cmd_idx++], true, 0x200, 0x0);
#endif

#if 0
		s// wait end-inter(1)
		unsigned int flag_num = 1;   // 0: sc_str_flag, sc_stp_flag
		cmdq_set_wait(&cmd_start[cmd_idx++], /*is_timer*/false, flag_num, 0);

		// clear intr
		cmdq_set_package(&cmd_start[cmd_idx++].reg, REG_LDC_IRQCLR, 1);
#endif
	}

	cmd_start[cmd_idx-1].reg.intr_end = 1;
	cmd_start[cmd_idx-1].reg.intr_last = 1;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device((phys_addr_t)cmdq_addr
		, sizeof(union cmdq_set) * LDC_CMDQ_MAX_REG_CNT * cnt, DMA_TO_DEVICE);
#else
	__dma_map_area((void *)cmdq_addr
		, sizeof(union cmdq_set) * LDC_CMDQ_MAX_REG_CNT * cnt, DMA_TO_DEVICE);
#endif
	TRACE_LDC(DBG_DEBUG, "cmdq buf addr:%#lx\n", (uintptr_t)virt_to_phys((void *)cmdq_addr));

	cmdq_intr_ctrl(reg_base[top_id] + REG_LDC_CMDQ_BASE, 0x02);
	cmdq_engine(reg_base[top_id] + REG_LDC_CMDQ_BASE, (uintptr_t)virt_to_phys((void *)cmdq_addr), (ldc_apb_base) >> 22,
			true, false, cmd_idx);
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
	(void)top_id;
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
	union cmdq_set *cmd_start = (union cmdq_set *)cmdq_addr;

	TRACE_LDC(DBG_DEBUG, "cmdq vir addr=0x%08llx, num=%d\n", cmdq_addr, num_cmd);
	for (i = 0; i < num_cmd; i++) {
		TRACE_LDC(DBG_DEBUG, "[%02d] [0x%08x]=0x%08x\n",
			i, cmd_start[i].reg.addr << 2,
			cmd_start[i].reg.data);
	}
}

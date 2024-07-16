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
#include "dwa_reg.h"
#include "dwa_cfg.h"
#include "cmdq.h"
#include "reg.h"
#include "dwa_common.h"
#include "dwa_debug.h"

/****************************************************************************
 * Global parameters
 ***************************************************************************/
static uintptr_t dwa_reg_base[DWA_DEV_MAX_CNT];

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
void dwa_dump_register(int top_id);
extern bool dwa_dump_reg;

void dwa_set_base_addr(void *base, int top_id)
{
	dwa_reg_base[top_id] = (uintptr_t)base;
}

void dwa_disable(int top_id)
{
	_reg_write(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL, 0x00000000);
}

/**
 * dwa_init - setup dwa, mainly interpolation settings.
 *
 * @param cfg: settings for this dwa's operation
 */
void dwa_init(int top_id)
{
	uintptr_t i = 0;
	unsigned char interp_h_shift_num = 0, interp_v_shift_num = 0;

	/*
	union vi_sys_intr intr_mask;

	intr_mask = vi_sys_get_intr_status();
	if (top_id)
		intr_mask.b.dwa1 = 1;
	else
		intr_mask.b.dwa0 = 1;
	vi_set_intr_mask(intr_mask);
	*/

	// enable dwa
	_reg_write_mask(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL, BIT(0), BIT(0));

	if (INTERPOLATION_COEF_FBITS >= 5 && INTERPOLATION_COEF_FBITS <= 8) {
		interp_h_shift_num = INTERPOLATION_COEF_FBITS - 5;
		interp_v_shift_num = INTERPOLATION_COEF_FBITS - 5;
	} else {
		interp_h_shift_num = 0;
		interp_v_shift_num = 0;
	}
	_reg_write_mask(dwa_reg_base[top_id] + REG_DWA_INTERP_OUTPUT_CTRL0, 0x00000330,
			(interp_v_shift_num<<8) | (interp_h_shift_num<<4));

	for (i = 0 ; i < COEFFICIENT_PHASE_NUM ; ++i) {
		_reg_write(dwa_reg_base[top_id] + REG_DWA_INTERP_HCOEFF_PHASE0 + (i << 2),
			   (hcoeff_tap[i][3]<<24) | (hcoeff_tap[i][2]<<16) |
			   (hcoeff_tap[i][1]<<8) | hcoeff_tap[i][0]);
		_reg_write(dwa_reg_base[top_id] + REG_DWA_INTERP_VCOEFF_PHASE0 + (i << 2),
			   (vcoeff_tap[i][3]<<24) | (vcoeff_tap[i][2]<<16) |
			   (vcoeff_tap[i][1]<<8) | vcoeff_tap[i][0]);
	}
}

/**
 * dwa_reset - do reset. This can be activated only if dma stop to avoid hang
 *	       fabric.
 *
 */
void dwa_reset(int top_id)
{
#if 1
	union vi_sys_reset mask;
	union vi_sys_reset_apb mask_apb;

	mask.raw = 0;
	mask_apb.raw = 0;

	if (top_id) {
		mask.b.dwa1 = 1;
		mask_apb.b.dwa1 = 1;
	} else {
		mask.b.dwa0 = 1;
		mask_apb.b.dwa0 = 1;
	}
	vi_sys_toggle_reset(mask);
	vi_sys_toggle_reset_apb(mask_apb);
#endif
}

/**
 * dwa_intr_ctrl - dwa's interrupt on(1)/off(0)
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void dwa_intr_ctrl(unsigned char intr_mask, int top_id)
{
	_reg_write(dwa_reg_base[top_id] + REG_DWA_INT_EN, intr_mask);
}

/**
 * dwa_intr_clr - clear dwa's interrupt
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void dwa_intr_clr(unsigned char intr_mask, int top_id)
{
	_reg_write(dwa_reg_base[top_id] + REG_DWA_INT_CLR, intr_mask);
}

/**
 * dwa_intr_status - dwa's interrupt status
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @return: The interrupt's status. 1 if active.
 */
unsigned char dwa_intr_status(int top_id)
{
	return _reg_read(dwa_reg_base[top_id] + REG_DWA_INT_STATUS);
}

/**
 * dwa_engine - start a dwa operation, wait frame_done intr after this.
 *              If output target is scaler, scaler's config should be done
 *              before this.
 *
 * @param cfg: settings for this dwa's operation
 */
void dwa_engine(struct dwa_cfg *cfg, int top_id)
{
	// enable dwa
	//_reg_write(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL, 0x00000001);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_DATA_FORMAT,
		   (cfg->output_target<<4) | (cfg->pix_fmt&0x03));

	// mesh id addr
	_reg_write(dwa_reg_base[top_id] + REG_DWA_MESH_ID_BASE_ADDR_LOW, cfg->mesh_id);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_MESH_ID_BASE_ADDR_HIGH,
		   (cfg->mesh_id) >> 32);

	// src buffer
	_reg_write(dwa_reg_base[top_id] + REG_DWA_IMG_SRC_SIZE,
		   (cfg->src_height<<16) | cfg->src_width);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_IMG_BASE_ADDR_LOW,
		   cfg->src_buf[R_IDX].addrl);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_IMG_BASE_ADDR_HIGH,
		   cfg->src_buf[R_IDX].addrh);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_PITCH, cfg->src_buf[R_IDX].pitch);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_OFFSET,
		   (cfg->src_buf[R_IDX].offset_y<<16) |
		   cfg->src_buf[R_IDX].offset_x);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_IMG_BASE_ADDR_LOW,
		   cfg->src_buf[G_IDX].addrl);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_IMG_BASE_ADDR_HIGH,
		   cfg->src_buf[G_IDX].addrh);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_PITCH, cfg->src_buf[G_IDX].pitch);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_OFFSET,
		   (cfg->src_buf[G_IDX].offset_y<<16) |
		   cfg->src_buf[G_IDX].offset_x);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_IMG_BASE_ADDR_LOW,
		   cfg->src_buf[B_IDX].addrl);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_IMG_BASE_ADDR_HIGH,
		   cfg->src_buf[B_IDX].addrh);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_PITCH, cfg->src_buf[B_IDX].pitch);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_OFFSET,
		   (cfg->src_buf[B_IDX].offset_y<<16) |
		   cfg->src_buf[B_IDX].offset_x);

	// dst buffer
	_reg_write(dwa_reg_base[top_id] + REG_DWA_IMG_DST_SIZE,
		   (cfg->dst_height<<16) | cfg->dst_width);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_IMG_BASE_ADDR_LOW,
		   cfg->dst_buf[R_IDX].addrl);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_IMG_BASE_ADDR_HIGH,
		   cfg->dst_buf[R_IDX].addrh);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_PITCH, cfg->dst_buf[R_IDX].pitch);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_OFFSET,
		   (cfg->dst_buf[R_IDX].offset_y<<16) |
		   cfg->dst_buf[R_IDX].offset_x);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_DST_IMG_BASE_ADDR_LOW,
		   cfg->dst_buf[G_IDX].addrl);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_DST_IMG_BASE_ADDR_HIGH,
		   cfg->dst_buf[G_IDX].addrh);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_DST_PITCH, cfg->dst_buf[G_IDX].pitch);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_G_U_DST_OFFSET,
		   (cfg->dst_buf[G_IDX].offset_y<<16) |
		   cfg->dst_buf[G_IDX].offset_x);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_DST_IMG_BASE_ADDR_LOW,
		   cfg->dst_buf[B_IDX].addrl);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_DST_IMG_BASE_ADDR_HIGH,
		   cfg->dst_buf[B_IDX].addrh);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_DST_PITCH, cfg->dst_buf[B_IDX].pitch);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_B_V_DST_OFFSET,
		   (cfg->dst_buf[B_IDX].offset_y<<16) |
		   cfg->dst_buf[B_IDX].offset_x);

	_reg_write(dwa_reg_base[top_id] + REG_DWA_INTERP_OUTPUT_CTRL1, cfg->bgcolor);
	_reg_write(dwa_reg_base[top_id] + REG_DWA_INTERP_OUTPUT_CTRL2, cfg->bdcolor);

	//enable all intr
	//dwa_intr_ctrl(0x07, top_id);
	if (unlikely(dwa_dump_reg))
		dwa_dump_register(top_id);

	// start dwa
	_reg_write_mask(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL, BIT(1), BIT(1));

	// There should be a frame_down intr afterwards.
#ifdef ENV_EMU
	for (int i = 0 ; i < 0x400; i += 4) {
		if (i == 0x280)
			i = 0x300;
		else if (i == 0x380)
			break;
		printf("0x%x: 0x%x\n", i,
		       _reg_read(dwa_reg_base[top_id] + i + REG_DWA_BASE));
	}
#endif
}

unsigned char dwa_cmdq_intr_status(unsigned char top_id)
{
	return cmdq_intr_status(dwa_reg_base[top_id] + REG_DWA_CMDQ_BASE);
}

void dwa_cmdq_intr_clr(unsigned char top_id, unsigned char intr_status)
{
	cmdq_intr_clr(dwa_reg_base[top_id] + REG_DWA_CMDQ_BASE, intr_status);
}

void dwa_cmdq_sw_restart(unsigned char top_id)
{
	cmdq_sw_restart(dwa_reg_base[top_id] + REG_DWA_CMDQ_BASE);
}

bool dwa_cmdq_is_sw_restart(unsigned char top_id)
{
	return cmdq_is_sw_restart(dwa_reg_base[top_id] + REG_DWA_CMDQ_BASE);
}

/**
 * dwa_engine_cmdq - start multiple dwa operation based on given cfgs, wait
 *		     cmdq-end intr after this. If output target is scaler,
 *		     scaler's config should be done before this.
 *
 * @param cfgs: settings for these dwa's operations
 * @param cnt: number of dwa operations
 */
void dwa_engine_cmdq(int top_id, const void *cmdq_addr, struct dwa_cfg **cfgs, unsigned char cnt)
{
	unsigned char i = 0, cmd_idx = 0;
	union cmdq_set *cmd_start;
	unsigned long long dwa_apb_base = top_id ? REG_DWA1_BASE_FOR_CMDQ : REG_DWA0_BASE_FOR_CMDQ;

	cmd_start = (union cmdq_set *)cmdq_addr;
	TRACE_DWA(DBG_DEBUG, "cmd_start(%px)\n",cmd_start);

	//memset(cmd_start, 0, sizeof(union cmdq_set) * DWA_CMDQ_MAX_REG_CNT * cnt);
	//_reg_write(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL, 0x00000001);
	for (i = 0; i < cnt; ++i) {
		struct dwa_cfg *cfg = cfgs[i];

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_GLB_CTRL,
				0x00000001);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_DATA_FORMAT,
				(cfg->output_target<<4) | (cfg->pix_fmt&0x03));

		// mesh id addr
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_MESH_ID_BASE_ADDR_LOW, cfg->mesh_id);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_MESH_ID_BASE_ADDR_HIGH,
				(cfg->mesh_id) >> 32);

		// src buffer
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_IMG_SRC_SIZE,
				(cfg->src_height<<16) | cfg->src_width);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_SRC_IMG_BASE_ADDR_LOW,
				cfg->src_buf[R_IDX].addrl);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_SRC_IMG_BASE_ADDR_HIGH,
				cfg->src_buf[R_IDX].addrh);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_SRC_PITCH,
				cfg->src_buf[R_IDX].pitch);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_SRC_OFFSET,
				(cfg->src_buf[R_IDX].offset_y<<16) |
				 cfg->src_buf[R_IDX].offset_x);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_SRC_IMG_BASE_ADDR_LOW,
				cfg->src_buf[G_IDX].addrl);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_SRC_IMG_BASE_ADDR_HIGH,
				cfg->src_buf[G_IDX].addrh);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_SRC_PITCH,
				cfg->src_buf[G_IDX].pitch);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_SRC_OFFSET,
				(cfg->src_buf[G_IDX].offset_y<<16) |
				 cfg->src_buf[G_IDX].offset_x);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_SRC_IMG_BASE_ADDR_LOW,
				cfg->src_buf[B_IDX].addrl);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_SRC_IMG_BASE_ADDR_HIGH,
				cfg->src_buf[B_IDX].addrh);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_SRC_PITCH,
				cfg->src_buf[B_IDX].pitch);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_SRC_OFFSET,
				(cfg->src_buf[B_IDX].offset_y<<16) |
				 cfg->src_buf[B_IDX].offset_x);

		// dst buffer
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_IMG_DST_SIZE,
				(cfg->dst_height<<16) |
				 cfg->dst_width);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_DST_IMG_BASE_ADDR_LOW,
				cfg->dst_buf[R_IDX].addrl);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_DST_IMG_BASE_ADDR_HIGH,
				cfg->dst_buf[R_IDX].addrh);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_DST_PITCH,
				cfg->dst_buf[R_IDX].pitch);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_R_Y_DST_OFFSET,
				(cfg->dst_buf[R_IDX].offset_y<<16) |
				 cfg->src_buf[R_IDX].offset_x);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_DST_IMG_BASE_ADDR_LOW,
				cfg->dst_buf[G_IDX].addrl);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_DST_IMG_BASE_ADDR_HIGH,
				cfg->dst_buf[G_IDX].addrh);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_DST_PITCH,
				cfg->dst_buf[G_IDX].pitch);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_G_U_DST_OFFSET,
				(cfg->dst_buf[G_IDX].offset_y<<16) |
				 cfg->src_buf[G_IDX].offset_x);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_DST_IMG_BASE_ADDR_LOW,
				cfg->dst_buf[B_IDX].addrl);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_DST_IMG_BASE_ADDR_HIGH,
				cfg->dst_buf[B_IDX].addrh);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_DST_PITCH,
				cfg->dst_buf[B_IDX].pitch);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_B_V_DST_OFFSET,
				(cfg->dst_buf[B_IDX].offset_y<<16) |
				 cfg->src_buf[B_IDX].offset_x);

		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_INTERP_OUTPUT_CTRL1,
				cfg->bgcolor);
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_INTERP_OUTPUT_CTRL2,
				cfg->bdcolor);

		// start dwa
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
				dwa_apb_base + REG_DWA_GLB_CTRL, 0x03);

		cmdq_set_wait(&cmd_start[cmd_idx++], false, 1, 0x1);

#if 1
		// clear intr
		cmdq_set_package(&cmd_start[cmd_idx++].reg,
			dwa_apb_base + REG_DWA_INT_CLR, 0x07);

		//cmdq_set_wait(&cmd_start[cmd_idx++], true, 0x200, 0x0);
#endif
	}

#if 0
	// clear intr
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
		reg_base + REG_DWA_INT_CLR,
		0x07);

	cmdq_intr_status = cmdq_intr_status(reg_base + REG_DWA_CMDQ_BASE);
	if (cmdq_intr_status)
		cmdq_intr_clr(reg_base + REG_DWA_CMDQ_BASE, cmdq_intr_status);
#endif

	cmd_start[cmd_idx-1].reg.intr_end = 1;
	cmd_start[cmd_idx-1].reg.intr_last = 1;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device((phys_addr_t)cmdq_addr
		, sizeof(union cmdq_set) * DWA_CMDQ_MAX_REG_CNT * cnt, DMA_TO_DEVICE);
#else
	__dma_map_area((void *)cmdq_addr
		, sizeof(union cmdq_set) * DWA_CMDQ_MAX_REG_CNT * cnt, DMA_TO_DEVICE);
#endif
	TRACE_DWA(DBG_DEBUG, "cmdq buf addr:%#lx\n", (uintptr_t)virt_to_phys((void *)cmdq_addr));

	cmdq_intr_ctrl(dwa_reg_base[top_id] + REG_DWA_CMDQ_BASE, 0x02);
	cmdq_engine(dwa_reg_base[top_id] + REG_DWA_CMDQ_BASE, (uintptr_t)virt_to_phys((void *)cmdq_addr), (dwa_apb_base) >> 22,
			true, false, cmd_idx);
}

/**
 * dwa_is_busy - check if dwa's operation is finished.
 *              dwa_start can only be toggled if only dma done(frame_done intr),
 *              ow dma won't finished.
 */
bool dwa_is_finish(int top_id)
{
	unsigned int cycles = _reg_read(dwa_reg_base[top_id] + REG_DWA_FRAME_RUN_TIME);

	udelay(100);
	return (cycles == _reg_read(dwa_reg_base[top_id] + REG_DWA_FRAME_RUN_TIME));
}

unsigned int dwa_read_en_status(int top_id)
{
	return (u32)_reg_read(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL);
}

/**
 * dwa_clk_gating enable - set dwa's clk gating.
 * @param en: On/Off clk gating.
 */
void dwa_enable_clk_gating(int top_id, unsigned char en)
{
	unsigned char val = en ? 0xff : 0x0;

	_reg_write(dwa_reg_base[top_id] + REG_DWA_CG_EN, val);
}

void dwa_dump_register(int top_id)
{
	TRACE_DWA(DBG_DEBUG, "REG_DWA_GLB_CTRL                          =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_GLB_CTRL));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DATA_FORMAT                       =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DATA_FORMAT));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_AXIM                              =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_AXIM));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_CG_EN                             =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_CG_EN));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_INT_EN                            =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_INT_EN));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_INT_CLR                           =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_INT_CLR));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_INT_STATUS                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_INT_STATUS));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_SRC_DATA_RD_BW                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_SRC_DATA_RD_BW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DST_DATA_WR_BW		               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DST_DATA_WR_BW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_ID_BW	                       =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_ID_BW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_TABLE_BW		               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_TABLE_BW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_CACHE_HIT_NUM                =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_CACHE_HIT_NUM));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_CACHE_MISS_NUM               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_CACHE_MISS_NUM));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_CACHE_SLICE_NUM              =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_CACHE_SLICE_NUM));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_FRAME_RUN_TIME                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_FRAME_RUN_TIME));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_ID_RDMA_ERR_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_ID_RDMA_ERR_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_ID_RDMA_ERR_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_ID_RDMA_ERR_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_TBL_RDMA_ERR_ADDR_LOW        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_TBL_RDMA_ERR_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_TBL_RDMA_ERR_ADDR_HIGH       =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_TBL_RDMA_ERR_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_TBL_RDMA_ERR_CNT             =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_TBL_RDMA_ERR_CNT));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_SLICE_START_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_SLICE_START_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_SLICE_START_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_SLICE_START_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS0                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS0));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS1                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS1));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS2                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS2));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS3                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS3));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS4                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS4));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS5                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS5));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS6                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS6));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS7                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS7));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS8                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS8));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS9                        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS9));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS10                       =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS10));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_DEBUG_BUS11                       =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_DEBUG_BUS11));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_ID_BASE_ADDR_LOW             =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_ID_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_MESH_ID_BASE_ADDR_HIGH            =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_MESH_ID_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_IMG_SRC_SIZE                      =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_IMG_SRC_SIZE));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_SRC_IMG_BASE_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_IMG_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_SRC_IMG_BASE_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_IMG_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_SRC_PITCHW                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_PITCH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_SRC_OFFSET                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_SRC_OFFSET));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_SRC_IMG_BASE_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_IMG_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_SRC_IMG_BASE_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_IMG_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_SRC_PITCH                     =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_PITCH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_SRC_OFFSET                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_SRC_OFFSET));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_SRC_IMG_BASE_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_IMG_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_SRC_IMG_BASE_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_IMG_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_SRC_PITCH                     =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_PITCH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_SRC_OFFSET                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_SRC_OFFSET));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_IMG_DST_SIZE                      =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_IMG_DST_SIZE));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_DST_IMG_BASE_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_IMG_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_DST_IMG_BASE_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_IMG_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_DST_PITCH                     =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_PITCH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_R_Y_DST_OFFSET                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_R_Y_DST_OFFSET));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_DST_IMG_BASE_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_DST_IMG_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_DST_IMG_BASE_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_DST_IMG_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_DST_PITCH                     =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_DST_PITCH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_G_U_DST_OFFSET                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_G_U_DST_OFFSET));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_DST_IMG_BASE_ADDR_LOW         =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_DST_IMG_BASE_ADDR_LOW));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_DST_IMG_BASE_ADDR_HIGH        =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_DST_IMG_BASE_ADDR_HIGH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_DST_PITCH                     =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_DST_PITCH));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_B_V_DST_OFFSET                    =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_B_V_DST_OFFSET));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_INTERP_OUTPUT_CTRL0               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_INTERP_OUTPUT_CTRL0));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_INTERP_OUTPUT_CTRL1               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_INTERP_OUTPUT_CTRL1));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_INTERP_OUTPUT_CTRL2               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_INTERP_OUTPUT_CTRL2));
	TRACE_DWA(DBG_DEBUG, "REG_DWA_SRC_DATA_CACHE_CTRL               =0x%08x\n", _reg_read(dwa_reg_base[top_id] + REG_DWA_SRC_DATA_CACHE_CTRL));
}

void dwa_dump_cmdq(unsigned long long cmdq_addr, unsigned int num_cmd)
{
	unsigned int i;
	union cmdq_set *cmd_start = (union cmdq_set *)cmdq_addr;


	TRACE_DWA(DBG_DEBUG, "cmdq vir addr=0x%08llx, num=%d\n", cmdq_addr, num_cmd);
	for (i = 0; i < num_cmd; i++) {
		TRACE_DWA(DBG_DEBUG, "[%02d] [0x%08x]=0x%08x\n",
			i, cmd_start[i].reg.addr << 2,
			cmd_start[i].reg.data);
	}
}


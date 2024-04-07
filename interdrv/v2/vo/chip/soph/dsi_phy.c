#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "system_common.h"
#elif defined(ENV_EMU)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/log2.h>
#endif  // ENV_CVITEST

#include "vo_common.h"
#include "disp_reg.h"
#include "reg.h"
#include "dsi_phy.h"

void __iomem *reg_fpll_ctrl2;
void __iomem *reg_fpll_ctrl5;
void __iomem *reg_mpll_ctrl1;

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t reg_dsi_wrap_base[DISP_MAX_INST];
static u8 data_0_lane[DISP_MAX_INST];
static bool data_0_pn_swap[DISP_MAX_INST];

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void dphy_set_base_addr(u8 inst, void *base)
{
	reg_dsi_wrap_base[inst] = (uintptr_t)base;
}
EXPORT_SYMBOL_GPL(dphy_set_base_addr);

/**
 * dphy_dsi_lane_en - set dsi-lanes enable control.
 *                    setup before dphy_dsi_init().
 *
 * @param clk_en: clk lane enable
 * @param data_en: data lane[0-3] enable
 * @param preamble_en: preeamble enable
 */
void dphy_dsi_lane_en(u8 inst, bool clk_en, bool *data_en, bool preamble_en)
{
	u8 val = 0, i = 0;

	val |= clk_en;
	for (i = 0; i < 4; ++i)
		val |= (data_en[i] << (i + 1));
	if (preamble_en)
		val |= 0x20;
	_reg_write_mask(REG_DSI_PHY_EN(inst), 0x3f, val);
}
EXPORT_SYMBOL_GPL(dphy_dsi_lane_en);

/**
 * dphy_get_dsi_lane_status - get dsi-lanes status.
 *
 * @param data_en: to store data status of lane[0-3] and clk lane
 */
void dphy_get_dsi_lane_status(u8 inst, bool *data_en)
{
	u32 val = 0, i = 0;

	val = _reg_read(REG_DSI_PHY_EN(inst));
	for (i = 0; i < DSI_LANE_MAX; ++i) {
		if (val & (0x1 << i))
			data_en[i] = true;
		else
			data_en[i] = false;
	}
}
EXPORT_SYMBOL_GPL(dphy_get_dsi_lane_status);

bool dphy_get_dsi_clk_lane_status(u8 inst)
{
	return _reg_read(REG_DSI_PHY_EN(inst)) & 0x1;
}
EXPORT_SYMBOL_GPL(dphy_get_dsi_clk_lane_status);

/**
 * dphy_dsi_set_lane - dsi-lanes control.
 *                     setup before dphy_dsi_lane_en().
 *
 * @param lane_num: lane[0-4].
 * @param lane: the role of this lane.
 * @param pn_swap: if this lane positive/negative swap.
 * @param clk_phase_shift: if this clk lane phase shift 90 degree.
 * @return: 0 for success.
 */
int dphy_dsi_set_lane(u8 inst, u8 lane_num, enum lane_id lane, bool pn_swap, bool clk_phase_shift)
{
	if ((lane_num > 4) || (lane > DSI_LANE_MAX))
		return -1;

	_reg_write_mask(REG_DSI_PHY_LANE_SEL(inst), 0x7 << (4 * lane_num), lane << (4 * lane_num));
	_reg_write_mask(REG_DSI_PHY_LANE_PN_SWAP(inst), BIT(lane_num), pn_swap << lane_num);

	if (lane == DSI_LANE_CLK)
		_reg_write_mask(REG_DSI_PHY_LANE_SEL(inst), 0x1f << 24,
				clk_phase_shift ? ((1 << 24) << lane_num) : 0);
	if (lane == DSI_LANE_0) {
		data_0_lane[inst] = lane_num;
		data_0_pn_swap[inst] = pn_swap;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dphy_dsi_set_lane);

/**
 * dphy_dsi_get_lane - get dsi-lanes lane num.
 *
 * @param lane_num: to store lane num, -1 if disable.
 * @return: num of data lane.
 */
int dphy_dsi_get_lane(u8 inst, enum lane_id *lane_num)
{
	bool data_en[DSI_LANE_MAX] = {false, false, false, false, false};
	u32 val = 0, i = 0, j = 0;

	dphy_get_dsi_lane_status(inst, data_en);

	val = _reg_read(REG_DSI_PHY_LANE_SEL(inst));

	for (i = 0, j = 0; i < DSI_LANE_MAX; ++i) {
		if (data_en[i]) {
			lane_num[i] = (val >> i * 4) & 0x07;
			++j;
		} else {
			lane_num[i] = -1;
		}
	}
	return --j;
}
EXPORT_SYMBOL_GPL(dphy_dsi_get_lane);

/**
 * dphy_dsi_init - dphy init.
 *                 Invoked after dphy_dsi_set_lane() and dphy_dsi_lane_en().
 *
 */
void dphy_init(u8 inst, enum disp_vo_intf intf)
{
#if 0 //TODO: is it needed?
	if (intf == DISP_VO_INTF_MIPI)
		_reg_write(REG_DSI_PHY_HS_CFG2(inst), 0x10);
#endif

	_reg_write(REG_DSI_PHY_PD(inst), (intf == DISP_VO_INTF_MIPI || intf == DISP_VO_INTF_LVDS)
		   ? 0x0 : 0x1f1f);
	_reg_write(REG_DSI_PHY_POWER_DOWN_CFG(inst), (intf == DISP_VO_INTF_MIPI || intf == DISP_VO_INTF_LVDS ||
		 		intf == DISP_VO_INTF_HDMI) ? 0x0 : 0xffffffff);

	if(intf == DISP_VO_INTF_HDMI)
	   return;

	_reg_write(REG_DSI_PHY_ESC_INIT(inst), 0x100);
	_reg_write(REG_DSI_PHY_ESC_WAKE(inst), 0x100);

	if ((intf == DISP_VO_INTF_BT656) || (intf == DISP_VO_INTF_BT1120) || (intf == DISP_VO_INTF_I80))
		_reg_write(REG_DSI_PHY_EXT_GPIO(inst), 0x000fffff);
	else
		_reg_write(REG_DSI_PHY_EXT_GPIO(inst), 0x0);

	_reg_write(REG_DSI_PHY_LVDS_EN(inst), (intf == DISP_VO_INTF_LVDS));
	_reg_write(REG_DSI_PHY_EN_LVDS_CFG(inst), (intf == DISP_VO_INTF_LVDS) ? 0x1f1f : 0x0);
	_reg_write_mask(REG_DSI_PHY_TXPLL_SETUP(inst), 0x3 << 21
					, (intf == DISP_VO_INTF_LVDS) ? 0x3 << 21 : 0x0);
}
EXPORT_SYMBOL_GPL(dphy_init);

void _cal_pll_reg(u8 dsi_id, u32 clkkHz, u32 VCORx10000, u32 *reg_txpll, u32 *reg_set, u32 factor)
{
	u8 gain = 1 << ilog2(MAX(1, 25000000UL / VCORx10000));
	u32 VCOCx1000 = VCORx10000 * gain / 10;
	u8 reg_disp_div_sel = VCOCx1000 / clkkHz;
	u8 dig_dig = ilog2(gain);
	u8 reg_divout_sel = MIN(3, dig_dig);
	u8 reg_div_sel = dig_dig - reg_divout_sel;
	u32 loop_gain = VCOCx1000 / 133000;
	u32 loop_c = loop_gain & ~0x7;
	u8 reg_div_loop = (loop_c > 32 ? 3 : loop_c / 8) - 1;
	u8 loop_gain1 = (reg_div_loop + 1) * 8;

	*reg_set = ((u64)(factor * loop_gain1) << 26) / VCOCx1000;
	*reg_txpll = (reg_div_loop << 24) | (reg_div_sel << 10) | (reg_divout_sel << 8) | reg_disp_div_sel;

	CVI_TRACE_VO(CVI_DBG_INFO, "clkkHz(%d) VCORx10000(%d) gain(%d)\n", clkkHz, VCORx10000, gain);
	CVI_TRACE_VO(CVI_DBG_INFO, "VCOCx1000(%d) loop_gain(%d) loop_c(%d) loop_gain1(%d)\n",
			VCOCx1000, loop_gain, loop_c, loop_gain1);
	CVI_TRACE_VO(CVI_DBG_INFO, "regs: disp_div_sel(%d) div_loop(%d) divout_sel(%d) div_sel(%d), set(%#x)\n",
			reg_disp_div_sel, reg_div_loop, reg_divout_sel, reg_div_sel, *reg_set);
}

void dphy_lvds_set_pll(u8 inst, u32 clkkHz, u8 link)
{
	u32 VCORx10000 = clkkHz * 70 / link;
	u32 reg_txpll, reg_set;

	reg_fpll_ctrl2 = ioremap(0x28102488 + 0x100 * (inst), 4);
	reg_fpll_ctrl5 = ioremap(0x28102494 + 0x100 * (inst), 4);
	reg_mpll_ctrl1 = ioremap(0x281024c4 + 0x100 * (inst), 4);

	_cal_pll_reg(inst, clkkHz, VCORx10000, &reg_txpll, &reg_set, 1800000);

	_reg_write_mask(REG_DSI_PHY_TXPLL_SETUP(inst), 0x30007FF, reg_txpll);
	_reg_write_mask((uintptr_t)reg_mpll_ctrl1, BIT(12), 0);
	_reg_write((uintptr_t)reg_fpll_ctrl2, reg_set);
	// update
	_reg_write_mask((uintptr_t)reg_fpll_ctrl5, BIT(0), 1);
	_reg_write_mask((uintptr_t)reg_fpll_ctrl5, BIT(0), 0);

	iounmap(reg_fpll_ctrl2);
	iounmap(reg_fpll_ctrl5);
	iounmap(reg_mpll_ctrl1);
}
EXPORT_SYMBOL_GPL(dphy_lvds_set_pll);

void dphy_dsi_get_pixclk(u8 inst, u32 *clkkHz, u8 lane, u8 bits)
{
	u32 VCOCx1000, VCORx10000;
	u32 reg_txpll, reg_set;
	u8 reg_disp_div_sel;
	u32 factor = 1800000;
	u8 gain, loop_gain, loop_gain_tmp;
	u64 modifies = 0;

	reg_fpll_ctrl2 = ioremap(0x28102488 + 0x100 * (inst), 4);

	reg_set = _reg_read((uintptr_t)reg_fpll_ctrl2);
	reg_txpll = _reg_read(REG_DSI_PHY_TXPLL_SETUP(inst)) & 0x30007FF;

	reg_disp_div_sel = reg_txpll & 0x7f;
	gain = reg_disp_div_sel * lane / bits;

	for (loop_gain = 8; loop_gain < 255; loop_gain += 8) {
		modifies = ((u64)factor << 26) * loop_gain;
		do_div(modifies, reg_set);
		VCOCx1000 = (u32)modifies;
		loop_gain_tmp = (((VCOCx1000 / 133000) + 7) >> 3) << 3;
		if (loop_gain_tmp == loop_gain)
			break;
	}

	VCORx10000 = VCOCx1000 * 10 / gain;
	*clkkHz = VCORx10000 * lane / 10 / bits;

	iounmap(reg_fpll_ctrl2);
}
EXPORT_SYMBOL_GPL(dphy_dsi_get_pixclk);

void dphy_dsi_set_pll(u8 inst, u32 clkkHz, u8 lane, u8 bits)
{
	u32 VCORx10000 = clkkHz * bits * 10 / lane;
	u32 reg_txpll, reg_set;

	reg_fpll_ctrl2 = ioremap(0x28102488 + 0x100 * (inst), 4);
	reg_fpll_ctrl5 = ioremap(0x28102494 + 0x100 * (inst), 4);
	reg_mpll_ctrl1 = ioremap(0x281024c4 + 0x100 * (inst), 4);

	_cal_pll_reg(inst, clkkHz, VCORx10000, &reg_txpll, &reg_set, 1800000);
	_reg_write_mask(REG_DSI_PHY_TXPLL_SETUP(inst), 0x30007FF, reg_txpll);
	_reg_write_mask((uintptr_t)reg_mpll_ctrl1, BIT(12), 0);
	_reg_write((uintptr_t)reg_fpll_ctrl2, reg_set);
	// update
	_reg_write_mask((uintptr_t)reg_fpll_ctrl5, BIT(0), 1);
	_reg_write_mask((uintptr_t)reg_fpll_ctrl5, BIT(0), 0);

	iounmap(reg_fpll_ctrl2);
	iounmap(reg_fpll_ctrl5);
	iounmap(reg_mpll_ctrl1);
}
EXPORT_SYMBOL_GPL(dphy_dsi_set_pll);

bool dphy_is_lvds(u8 inst)
{
	return _reg_read(REG_DSI_PHY_LVDS_EN(inst)) & 0x1;
}
EXPORT_SYMBOL_GPL(dphy_is_lvds);

#define dcs_delay 1

enum LP_DATA {
	LP_DATA_00 = 0x00010001,
	LP_DATA_01 = 0x00010101,
	LP_DATA_10 = 0x01010001,
	LP_DATA_11 = 0x01010101,
	LP_DATA_MAX
};

static inline void _data_0_manual_data(u8 inst, enum LP_DATA data)
{
	if (data_0_pn_swap[inst]) {
		switch (data) {
		case LP_DATA_01:
			_reg_write(REG_DSI_PHY_DATA_OV(inst), LP_DATA_10 << data_0_lane[inst]);
			break;
		case LP_DATA_10:
			_reg_write(REG_DSI_PHY_DATA_OV(inst), LP_DATA_01 << data_0_lane[inst]);
			break;
		default:
			_reg_write(REG_DSI_PHY_DATA_OV(inst), data << data_0_lane[inst]);
			break;
		}
	} else {
		_reg_write(REG_DSI_PHY_DATA_OV(inst), data << data_0_lane[inst]);
	}
	udelay(dcs_delay);
}

// LP-11, LP-10, LP-00, LP-01, LP-00
static void _esc_entry(u8 inst)
{
	_data_0_manual_data(inst, LP_DATA_11);
	_data_0_manual_data(inst, LP_DATA_10);
	_data_0_manual_data(inst, LP_DATA_00);
	_data_0_manual_data(inst, LP_DATA_01);
	_data_0_manual_data(inst, LP_DATA_00);
}

// LP-00, LP-10, LP-11
static void _esc_exit(u8 inst)
{
	_data_0_manual_data(inst, LP_DATA_00);
	_data_0_manual_data(inst, LP_DATA_10);
	_data_0_manual_data(inst, LP_DATA_11);
}

static void _esc_data(u8 inst, u8 data)
{
	u8 i = 0;

	for (i = 0; i < 8; ++i) {
		_data_0_manual_data(inst, ((data & (1 << i)) ? LP_DATA_10 : LP_DATA_01));
		_data_0_manual_data(inst, LP_DATA_00);
	}
}

void dpyh_mipi_tx_manual_packet(u8 inst, const u8 *data, u8 count)
{
	u8 i = 0;

	_esc_entry(inst);
	_esc_data(inst, 0x87); // LPDT
	for (i = 0; i < count; ++i)
		_esc_data(inst, data[i]);
	_esc_exit(inst);
	_reg_write(REG_DSI_PHY_DATA_OV(inst), 0x0);
}

void dphy_set_hs_settle(u8 inst, u8 prepare, u8 zero, u8 trail)
{
	_reg_write_mask(REG_DSI_PHY_HS_CFG1(inst),
				0xffffff00, (trail << 24) | (zero << 16) | (prepare << 8));
}
EXPORT_SYMBOL_GPL(dphy_set_hs_settle);

void dphy_get_hs_settle(u8 inst, u8 *prepare, u8 *zero, u8 *trail)
{
	u32 value = _reg_read(REG_DSI_PHY_HS_CFG1(inst));

	if (prepare)
		*prepare = (value >> 8) & 0xff;
	if (zero)
		*zero = (value >> 16) & 0xff;
	if (trail)
		*trail = (value >> 24) & 0xff;
}
EXPORT_SYMBOL_GPL(dphy_get_hs_settle);

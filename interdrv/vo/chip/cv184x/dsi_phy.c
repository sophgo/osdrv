#include "osal.h"
#include "vo_debug.h"
#include "vo_reg.h"
#include "reg.h"
#include "dsi_phy.h"
#include "vi_sys.h"

#undef BIT
#define BIT(nr)      ((1U) << (nr))

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t reg_dsi_wrap_base[DISP_MAX_INST];
static unsigned char data_0_lane[DISP_MAX_INST];
static bool data_0_pn_swap[DISP_MAX_INST];

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void dphy_set_base_addr(unsigned char inst, void *base)
{
	reg_dsi_wrap_base[inst] = (uintptr_t)base;
}

/**
 * dphy_dsi_lane_en - set dsi-lanes enable control.
 *                    setup before dphy_dsi_init().
 *
 * @param clk_en: clk lane enable
 * @param data_en: data lane[0-3] enable
 * @param preamble_en: preeamble enable
 */
void dphy_dsi_lane_en(unsigned char inst, bool clk_en, bool *data_en, bool preamble_en)
{
	unsigned char val = 0, i = 0;

	val |= clk_en;

	for (i = 0; i < 4; ++i)
		val |= (data_en[i] << (i + 1));

	if (preamble_en)
		val |= 0x20;

	_reg_write_mask(REG_DSI_PHY_EN(inst), 0x3f, val);
}
osal_module_export(dphy_dsi_lane_en);

/**
 * dphy_get_dsi_lane_status - get dsi-lanes status.
 *
 * @param data_en: to store data status of lane[0-3] and clk lane
 */
void dphy_get_dsi_lane_status(unsigned char inst, bool *data_en)
{
	unsigned int val = 0, i = 0;
	val = _reg_read(REG_DSI_PHY_EN(inst));

	for (i = 0; i < MIPI_TX_LANE_MAX; ++i) {
		if (val & (0x1 << i))
			data_en[i] = true;
		else
			data_en[i] = false;
	}
}

void dphy_dsi_disable_lanes(unsigned char inst)
{
	_reg_write_mask(REG_DSI_PHY_EN(inst), 0x3f, 0);
	_reg_write_mask(REG_DSI_PHY_LANE_SEL(inst), 0xfffff, 0);
}
osal_module_export(dphy_dsi_disable_lanes);

bool dphy_get_dsi_clk_lane_status(unsigned char inst)
{
	return _reg_read(REG_DSI_PHY_EN(inst)) & 0x1;
}

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
int dphy_dsi_set_lane(unsigned char inst, unsigned char lane_num, enum mipi_tx_lane_id lane,
		      bool pn_swap, bool clk_phase_shift)
{
	if (lane_num > 4 || lane > MIPI_TX_LANE_MAX)
		return -1;

	_reg_write_mask(REG_DSI_PHY_LANE_SEL(inst), 0x7 << (4 * lane_num), lane << (4 * lane_num));
	_reg_write_mask(REG_DSI_PHY_LANE_PN_SWAP(inst), BIT(lane_num), pn_swap << lane_num);

	if (lane == MIPI_TX_LANE_CLK)
		_reg_write_mask(REG_DSI_PHY_LANE_SEL(inst), 0x1f << 24,
				clk_phase_shift ? ((1 << 24) << lane_num) : 0);

	if (lane == MIPI_TX_LANE_0) {
		data_0_lane[inst] = lane_num;
		data_0_pn_swap[inst] = pn_swap;
	}

	return 0;
}
osal_module_export(dphy_dsi_set_lane);

/**
 * dphy_dsi_get_lane - get dsi-lanes lane num.
 *
 * @param lane_num: to store lane num, -1 if disable.
 * @return: num of data lane.
 */
int dphy_dsi_get_lane(unsigned char inst, enum mipi_tx_lane_id *lane_num)
{
	bool data_en[MIPI_TX_LANE_MAX] = {false, false, false, false, false};
	unsigned int val = 0, i = 0, j = 0;

	dphy_get_dsi_lane_status(inst, data_en);

	val = _reg_read(REG_DSI_PHY_LANE_SEL(inst));

	for (i = 0, j = 0; i < MIPI_TX_LANE_MAX; ++i) {
		if (data_en[i]) {
			lane_num[i] = (val >> i * 4) & 0x07;
			++j;
		} else {
			lane_num[i] = -1;
		}
	}
	return --j;
}
osal_module_export(dphy_dsi_get_lane);

/**
 * dphy_dsi_init - dphy init.
 *                 Invoked after dphy_dsi_set_lane() and dphy_dsi_lane_en().
 *
 */
void dphy_init(unsigned char inst, enum vo_disp_intf intf)
{
	int lptrx = 0, lptx_rx = 0, hstx = 0, i;

	void *pll_reg = osal_ioremap(0x03002840, 4);
	u32 pll = _reg_read((uintptr_t)pll_reg) & 0xfd;
	_reg_write((uintptr_t)pll_reg, pll | 0x0a);
	osal_iounmap(pll_reg, 4);

	for (i = 0; i < MIPI_TX_LANE_MAX; ++i) {
		if (((_reg_read(REG_DSI_PHY_LANE_SEL(inst)) >> i * 4) & 0x0F) > 4) {
			lptrx |= ((1 << i) | (1 << (8 + i)));
			lptx_rx  |= ((1 << i) | (1 << (16 + i)));
			hstx  |= ((1 << i) | (1 << (8 + i)) | (1 << (16 + i)) | (1 << (24 + i)));
		}
	}

	_reg_write(REG_DSI_PHY_PD(inst), (intf == VO_DISP_INTF_DSI || intf == VO_DISP_INTF_LVDS)
		   ? lptrx : 0x1f1f);
	_reg_write(REG_DSI_PHY_LPTX_OV(inst), lptx_rx);
	_reg_write(REG_DSI_PHY_PD_EN_TX(inst), lptrx);
	_reg_write(REG_DSI_PHY_PD_TXDRV(inst), hstx);
	_reg_write(REG_DSI_PHY_GPO(inst), lptrx);
	_reg_write(REG_DSI_PHY_GPI(inst), lptrx);
	_reg_write(REG_DSI_PHY_ESC_INIT(inst), 0x100);
	_reg_write(REG_DSI_PHY_ESC_WAKE(inst), 0x100);

	if (intf == VO_DISP_INTF_DSI || intf == VO_DISP_INTF_LVDS)
		_reg_write(REG_DSI_PHY_EXT_GPIO(inst), 0x0);
	else
		_reg_write(REG_DSI_PHY_EXT_GPIO(inst), 0x3fffffff);

	_reg_write(REG_DSI_PHY_LVDS_EN(inst), (intf == VO_DISP_INTF_LVDS));
	// if lvds: 1. en txbias. 2. en sublvds. 3. set vsel to maximum
	_reg_write_mask(REG_DSI_PHY_EN_TXBIAS_OP(inst), 0x10000,
			(intf == VO_DISP_INTF_LVDS) ? 0x10000 : 0);
	_reg_write_mask(REG_DSI_PHY_EN_SUBLVDS(inst), 0x1F000000,
			(intf == VO_DISP_INTF_LVDS) ? 0x1F000000 : 0);
	_reg_write_mask(REG_DSI_PHY_VSEL(inst), 0xFFFFF,
			(intf == VO_DISP_INTF_LVDS) ? 0xFFFFF : 0);
}
osal_module_export(dphy_init);

static inline int _ilog2(unsigned long long x) {
	int result = 0;

	if (x == 0) return -1;

	if (x >= (1ULL << 32)) { x >>= 32; result += 32; }
	if (x >= (1ULL << 16)) { x >>= 16; result += 16; }
	if (x >= (1ULL << 8))  { x >>= 8;  result += 8; }
	if (x >= (1ULL << 4))  { x >>= 4;  result += 4; }
	if (x >= (1ULL << 2))  { x >>= 2;  result += 2; }
	if (x >= (1ULL << 1))  { result += 1; }
	return result;
}

void _cal_pll_reg(unsigned char dsi_id, unsigned int clk_khz, unsigned int vco_rx10000,
		  unsigned int *reg_txpll, unsigned int *reg_set, unsigned int factor)
{
	unsigned char gain = 1 << _ilog2(MAX(1, 25000000UL / vco_rx10000));
	unsigned int vco_cx1000 = vco_rx10000 * gain / 10;
	unsigned char reg_disp_div_sel = vco_cx1000 / clk_khz;
	unsigned char dig_dig = _ilog2(gain);
	unsigned char reg_divout_sel = MIN(3, dig_dig);
	unsigned char reg_div_sel = dig_dig - reg_divout_sel;

	unsigned int loop_gainx1000 = vco_cx1000 / 133;
	bool bt_div = reg_disp_div_sel > 0x7f;
	unsigned int loop_c = 8 * ((loop_gainx1000 / 8) / 1000);
	unsigned char div_loop = loop_c > 32 ? 3 : loop_c / 8;
	unsigned char loop_gain1 = div_loop * 8;
	unsigned long long modifies = (unsigned long long)(factor * loop_gain1) << 26;

	union vi_sys_clk_ctrl2 vi_sys_clk_ctrl2;
	unsigned char div_tmp = 1;

	modifies = osal_div_u64(modifies, vco_cx1000);
	*reg_set = (unsigned int)modifies;

	vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();

	if (bt_div) {
		while (reg_disp_div_sel > 0x7f) {
			reg_disp_div_sel >>= 1;
			div_tmp <<= 1;
		}
		vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 0;
		vi_sys_clk_ctrl2.b.disp_div_cnt = div_tmp;
		vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
	} else {
		vi_sys_clk_ctrl2.b.disp_sel_bt_div1 = 1;
		vi_sys_set_clk_ctrl2(vi_sys_clk_ctrl2);
	}

	_reg_write_mask(REG_DSI_PHY_TXPLL(dsi_id), 0x300000, div_loop << 20);

	*reg_txpll = (reg_div_sel << 10) | (reg_divout_sel << 8) | reg_disp_div_sel;

	TRACE_VO(DBG_INFO, "clk_khz(%d) vco_rx10000(%d) gain(%d)\n", clk_khz, vco_rx10000, gain);
	TRACE_VO(DBG_INFO, "vco_cx1000(%d) dig_dig(%d) loop_gain(%d)\n", vco_cx1000, dig_dig, loop_gainx1000);
	TRACE_VO(DBG_INFO, "loop_c(%d) div_loop(%d) loop_gain1(%d)\n", loop_c, div_loop, loop_gain1);
	TRACE_VO(DBG_INFO, "regs: disp_div_sel(%d), divout_sel(%d), div_sel(%d), set(%#x)\n",
			reg_disp_div_sel, reg_divout_sel, reg_div_sel, *reg_set);
	TRACE_VO(DBG_INFO, "vi_sys : bt_div(%d)\n", bt_div);
}

void dphy_lvds_set_pll(unsigned char inst, unsigned int clk_khz, unsigned char link)
{
	unsigned int vco_rx10000 = clk_khz * 70 / link;
	unsigned int reg_txpll, reg_set;

	_cal_pll_reg(inst, clk_khz, vco_rx10000, &reg_txpll, &reg_set, 900000);

	_reg_write_mask(REG_DSI_PHY_TXPLL(inst), 0x7FF, reg_txpll);
	_reg_write(REG_DSI_PHY_REG_SET(inst), reg_set);

	// update
	_reg_write_mask(REG_DSI_PHY_REG_8C(inst), BIT(0), 0);
	_reg_write_mask(REG_DSI_PHY_REG_8C(inst), BIT(0), 1);
	_reg_write_mask(REG_DSI_PHY_REG_8C(inst), BIT(0), 0);
}

void dphy_dsi_get_pixclk(unsigned char inst, unsigned int *clk_khz, unsigned char lane, unsigned char bits)
{
	unsigned int vco_cx1000, vco_rx10000;
	unsigned int reg_txpll, reg_set;
	unsigned char reg_disp_div_sel;
	unsigned int factor = 900000;
	unsigned char gain, loop_gain, loop_gain_tmp;
	unsigned long long modifies = 0;

	union vi_sys_clk_ctrl2 vi_sys_clk_ctrl2;
	bool bt_div;

	reg_set = _reg_read(REG_DSI_PHY_REG_SET(inst));

	vi_sys_clk_ctrl2 = vi_sys_get_clk_ctrl2();
	bt_div = !(vi_sys_clk_ctrl2.raw & 0x10);
	reg_txpll = _reg_read(REG_DSI_PHY_TXPLL(inst)) & 0x7ff;

	reg_disp_div_sel = reg_txpll & 0x7f;
	reg_disp_div_sel = bt_div ? reg_disp_div_sel << (vi_sys_clk_ctrl2.b.disp_div_cnt / 2) : reg_disp_div_sel;
	gain = reg_disp_div_sel * lane / bits;

	for (loop_gain = 8; loop_gain < 255; loop_gain += 8) {
		modifies = ((unsigned long long)factor << 26) * loop_gain;
		modifies = osal_div_u64(modifies, reg_set);
		vco_cx1000 = (unsigned int)modifies;
		loop_gain_tmp = (((vco_cx1000 / 266000) + 7) >> 3) << 3;
		if (loop_gain_tmp == loop_gain)
			break;
	}

	vco_rx10000 = vco_cx1000 * 10 / gain;
	*clk_khz = vco_rx10000 * lane / 10 / bits;
}
osal_module_export(dphy_dsi_get_pixclk);

void dphy_dsi_set_pll(unsigned char inst, unsigned int clk_khz, unsigned char lane, unsigned char bits)
{
	unsigned int vco_rx10000 = clk_khz * bits * 10 / lane;
	unsigned int reg_txpll, reg_set;

	_cal_pll_reg(inst, clk_khz, vco_rx10000, &reg_txpll, &reg_set, 900000);

	_reg_write_mask(REG_DSI_PHY_TXPLL(inst), 0x7ff, reg_txpll);
	_reg_write(REG_DSI_PHY_REG_SET(inst), reg_set);

	// update
	_reg_write_mask(REG_DSI_PHY_REG_8C(inst), BIT(0), 0);
	_reg_write_mask(REG_DSI_PHY_REG_8C(inst), BIT(0), 1);
	_reg_write_mask(REG_DSI_PHY_REG_8C(inst), BIT(0), 0);
}
osal_module_export(dphy_dsi_set_pll);

bool dphy_is_lvds(unsigned char inst)
{
	return _reg_read(REG_DSI_PHY_LVDS_EN(inst)) & 0x1;
}

void dphy_lvds_analog_setting(unsigned char inst, bool is_lvds)
{
	//mercury needs this analog setting while lvds tx mode
	if (is_lvds)
		_reg_write_mask(REG_DSI_PHY_REG_74(inst), 0x3ff, 0x2AA);
	else
		_reg_write_mask(REG_DSI_PHY_REG_74(inst), 0x3ff, 0x0);
}
osal_module_export(dphy_lvds_analog_setting);

#define dcs_delay 1

enum LP_DATA {
	LP_DATA_00 = 0x00010001,
	LP_DATA_01 = 0x00010101,
	LP_DATA_10 = 0x01010001,
	LP_DATA_11 = 0x01010101,
	LP_DATA_MAX
};

static inline void _data_0_manual_data(unsigned char inst, enum LP_DATA data)
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
	osal_udelay(dcs_delay);
}

// LP-11, LP-10, LP-00, LP-01, LP-00
static void _esc_entry(unsigned char inst)
{
	_data_0_manual_data(inst, LP_DATA_11);
	_data_0_manual_data(inst, LP_DATA_10);
	_data_0_manual_data(inst, LP_DATA_00);
	_data_0_manual_data(inst, LP_DATA_01);
	_data_0_manual_data(inst, LP_DATA_00);
}

// LP-00, LP-10, LP-11
static void _esc_exit(unsigned char inst)
{
	_data_0_manual_data(inst, LP_DATA_00);
	_data_0_manual_data(inst, LP_DATA_10);
	_data_0_manual_data(inst, LP_DATA_11);
}

static void _esc_data(unsigned char inst, unsigned char data)
{
	unsigned char i = 0;

	for (i = 0; i < 8; ++i) {
		_data_0_manual_data(inst, ((data & (1 << i)) ? LP_DATA_10 : LP_DATA_01));
		_data_0_manual_data(inst, LP_DATA_00);
	}
}

void dpyh_mipi_tx_manual_packet(unsigned char inst, const unsigned char *data, unsigned char count)
{
	unsigned char i = 0;

	_esc_entry(inst);
	_esc_data(inst, 0x87); // LPDT
	for (i = 0; i < count; ++i)
		_esc_data(inst, data[i]);
	_esc_exit(inst);
	_reg_write(REG_DSI_PHY_DATA_OV(inst), 0x0);
}

void dphy_set_hs_settle(unsigned char inst, unsigned char prepare, unsigned char zero, unsigned char trail)
{
	_reg_write_mask(REG_DSI_PHY_HS_CFG1(inst), 0xffffff00,
			(trail << 24) | (zero << 16) | (prepare << 8));
}
osal_module_export(dphy_set_hs_settle);

void dphy_get_hs_settle(unsigned char inst, unsigned char *prepare, unsigned char *zero, unsigned char *trail)
{
	unsigned int value = _reg_read(REG_DSI_PHY_HS_CFG1(inst));

	if (prepare)
		*prepare = (value >> 8) & 0xff;
	if (zero)
		*zero = (value >> 16) & 0xff;
	if (trail)
		*trail = (value >> 24) & 0xff;
}
osal_module_export(dphy_get_hs_settle);

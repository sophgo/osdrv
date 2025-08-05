#include "vo_reg.h"
#include "reg.h"
#include "disp.h"
#include "vo_mac.h"
#include "osal.h"
#include "vo_debug.h"
#include "vo_platform.h"

#undef BIT
#define BIT(nr)      ((1U) << (nr))

static uintptr_t reg_vo_mac_base[DISP_MAX_INST];

void vo_mac_set_base_addr(unsigned char inst, void *base)
{
	reg_vo_mac_base[inst] = (uintptr_t)base;
}

void vo_mac_set_data_mux(unsigned char inst, unsigned char vodata_selID, unsigned char value)
{
	uintptr_t vo_mux_addr[10] = {REG_VO_MAC_VO_MUX0(inst), REG_VO_MAC_VO_MUX1(inst),
				     REG_VO_MAC_VO_MUX2(inst), REG_VO_MAC_VO_MUX3(inst),
				     REG_VO_MAC_VO_MUX4(inst), REG_VO_MAC_VO_MUX5(inst),
				     REG_VO_MAC_VO_MUX6(inst), REG_VO_MAC_VO_MUX7(inst),
				     REG_VO_MAC_VO_MUX8(inst)};

	union vo_mac_mux vo_mac_mux;
	unsigned char vodata_muxidx = (vodata_selID + 2) / 4;
	unsigned char vodata_selidx = (vodata_selID + 2) % 4;

	vo_mac_mux.raw = _reg_read(vo_mux_addr[vodata_muxidx]);

	switch (vodata_selidx) {
	case 0:
		vo_mac_mux.b.vod_sel0 = value;
		break;

	case 1:
		vo_mac_mux.b.vod_sel1 = value;
		break;

	case 2:
		vo_mac_mux.b.vod_sel2 = value;
		break;

	case 3:
		vo_mac_mux.b.vod_sel3 = value;
		break;

	default:
		break;
	}
	_reg_write(vo_mux_addr[vodata_muxidx], vo_mac_mux.raw);
}

void vo_mac_set_sel_type(unsigned char inst, enum vo_mac_sel vo_mac_sel)
{
	union vo_mac_mux_sel vo_mac_mux;

	vo_mac_mux.raw = _reg_read(REG_VO_MAC_VO_MUX(inst));
	vo_mac_mux.b.vo_sel_type = vo_mac_sel;

	_reg_write(REG_VO_MAC_VO_MUX(inst), vo_mac_mux.raw);
}

enum vo_mac_sel vo_mac_mux_get(unsigned char inst)
{
	union vo_mac_mux_sel vo_mac_mux;

	vo_mac_mux.raw = _reg_read(REG_VO_MAC_VO_MUX(inst));

	return vo_mac_mux.b.vo_sel_type;
}
osal_module_export(vo_mac_mux_get);

/**
 * vo_mac_check_i80_enable - check whether mcu interface enable.
 *
 * @return: i80's enable status.
 */
bool vo_mac_check_i80_enable(unsigned char inst)
{
	bool is_enable = (_reg_read(REG_VO_MAC_MCU_IF_CTRL(inst)) & 0x01);

	return is_enable;
}

void vo_mac_bt_set(unsigned char inst, union bt_enc enc, union bt_sync_code sync)
{
	_reg_write(REG_VO_MAC_BT_ENC(inst), enc.raw);
	_reg_write(REG_VO_MAC_BT_SYNC_CODE(inst), sync.raw);
}

void vo_mac_bt_get(unsigned char inst, union bt_enc *enc, union bt_sync_code *sync)
{
	enc->raw = _reg_read(REG_VO_MAC_BT_ENC(inst));
	sync->raw = _reg_read(REG_VO_MAC_BT_SYNC_CODE(inst));
}

/****************************************************************************
 * serial RGB
 ****************************************************************************/
void vo_mac_srgb_ttl_en(unsigned char inst, bool enable)
{
	union  srgb_ctrl vo_srgb_ctrl;

	vo_srgb_ctrl.raw = _reg_read(REG_VO_MAC_SRGB_CTRL(inst));
	vo_srgb_ctrl.b.srgb_ttl_en = enable;

	_reg_write(REG_VO_MAC_SRGB_CTRL(inst), vo_srgb_ctrl.raw);
}

void vo_mac_srgb_ttl_4x(unsigned char inst, bool is_4x)
{
	union  srgb_ctrl vo_srgb_ctrl;

	vo_srgb_ctrl.raw = _reg_read(REG_VO_MAC_SRGB_CTRL(inst));
	vo_srgb_ctrl.b.srgb_ttl_4t = is_4x;

	_reg_write(REG_VO_MAC_SRGB_CTRL(inst), vo_srgb_ctrl.raw);
}

void vo_mac_bt_en(unsigned char inst)
{
	_reg_write(REG_VO_MAC_BT_CFG(inst), 0x1);
}
osal_module_export(vo_mac_bt_en);

/**
 * vo_mac_mux_sel - remap vo mux
 * @param vo_mac_mux_sel: origin vo mux
 * @param vo_mac_mux: mapped vo mux
 */
void vo_mac_mux_sel(unsigned char inst, int vo_mac_sel, int vo_mac_mux)
{
	unsigned int value = 0;
	uintptr_t reg_addr;
	unsigned int offset = 0;

	if (vo_mac_sel >= VO_D28)
		vo_mac_sel += 2;

	reg_addr = REG_VO_MAC_VO_MUX0(inst) +  (vo_mac_sel / 4) * 4;
	offset = (vo_mac_sel % 4) * 8;

	if (vo_mac_sel == VO_CLK0) {
		_reg_write_mask(REG_VO_MAC_VO_MUX7(inst), BIT(20), BIT(20));
	} else if (vo_mac_sel == VO_CLK1) {
		_reg_write_mask(REG_VO_MAC_VO_MUX7(inst), BIT(21), BIT(21));
	}

	value = _reg_read(reg_addr);
	value |= (vo_mac_mux << offset);
	_reg_write(reg_addr, value);
}
osal_module_export(vo_mac_mux_sel);

void vo_mac_ext_vo_pinmux_set(int vo_sel)
{
	void *ext_pin_addr = osal_ioremap(0x050270e4, 4);
	_reg_write_mask((uintptr_t)ext_pin_addr, 1 << (vo_sel - VO_D32), 1 << (vo_sel - VO_D32));
	osal_iounmap(ext_pin_addr, 4);
}
osal_module_export(vo_mac_ext_vo_pinmux_set);

void vo_mac_set_i80_if(unsigned char inst, unsigned char sw_mode, enum mcu_mode format)
{
	union mcu_if_ctrl mcu_if_ctrl;
	union hw_mcu_auto hw_mcu_auto;

	mcu_if_ctrl.raw = _reg_read(REG_VO_MAC_MCU_IF_CTRL(inst));
	hw_mcu_auto.raw = _reg_read(REG_VO_MAC_HW_MCU_AUTO(inst));

	if(sw_mode)
	{
	    //for sw mcu mode
		mcu_if_ctrl.b.i80_if_en = false;
		mcu_if_ctrl.b.i80_hw_if_en = false;
		mcu_if_ctrl.b.i80_sw_mode_en = true;
	}
	else//hw mcu
	{
		mcu_if_ctrl.b.i80_if_en = false;
		mcu_if_ctrl.b.i80_sw_mode_en = false;
		mcu_if_ctrl.b.i80_hw_if_en = true;
		if(MCU_MODE_RGB565 == format)
		{
			hw_mcu_auto.b.mcu_565 = true;
		}
		else if(MCU_MODE_RGB888 == format)
		{
			hw_mcu_auto.b.mcu_565 = false;
		}
	}

	_reg_write(REG_VO_MAC_MCU_IF_CTRL(inst), mcu_if_ctrl.raw);
	_reg_write(REG_VO_MAC_HW_MCU_AUTO(inst), hw_mcu_auto.raw);
}
osal_module_export(vo_mac_set_i80_if);

void vo_mac_hw_i80_en(unsigned char inst, bool enable)
{
	union hw_mcu_auto hw_mcu_auto;

	hw_mcu_auto.raw = _reg_read(REG_VO_MAC_HW_MCU_AUTO(inst));

	hw_mcu_auto.b.mcu_hw_trig = enable;

	_reg_write(REG_VO_MAC_HW_MCU_AUTO(inst), hw_mcu_auto.raw);
}
osal_module_export(vo_mac_hw_i80_en);

void i80_set_cmd0(unsigned char inst, unsigned int cmd)
{
	_reg_write(REG_VO_MAC_HW_MCU_CMD0(inst), cmd);
}

void i80_set_cmd_cnt(unsigned char inst, unsigned int cmdcnt)
{
	if (cmdcnt == 1) {
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(4), 0);//sw_tx_num=0
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(5), 0);
	}
	if (cmdcnt == 2) {
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(4), BIT(4));
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(5), 0);//sw_tx_num=1
	}
	if (cmdcnt == 3) {
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(4), 0);//sw_tx_num=2
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(5), BIT(5));
	}
	if (cmdcnt == 4) {
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(4), BIT(4));//sw_tx_num=3
		_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(5), BIT(5));
	}
}

void i80_trig(unsigned char inst)
{
	int cnt = 0;
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(0), 0);//rising edge to trig
	_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(0), 1);//rising edge to trig

	do {
		osal_udelay(1);
		if (_reg_read(REG_VO_MAC_HW_MCU_CMD(inst)) & BIT(3))//check sw_tx_done
		{
			_reg_write_mask(REG_VO_MAC_HW_MCU_CMD(inst), BIT(3), BIT(3));
			break;
		}
	} while (++cnt < 10);

	if (cnt == 10)
		TRACE_VO(DBG_ERR, "[I80]: hw mcu cmd not ready.\n");

}

void hw_mcu_cmd_send(unsigned char inst, void *cmds, int size)
{
	vo_i80_instr_s *instr = (vo_i80_instr_s *)cmds;
	int i = 0;
	unsigned int sw_cmd0;
	int cmd_cnt = 0;

	for (i = 0; i < size; i = i + 1) {
		cmd_cnt = 0;
		if (i < size) {
			cmd_cnt++;
			sw_cmd0 = (instr[i].data_type << 8) | instr[i].data;
			i80_set_cmd0(inst, sw_cmd0);
			osal_udelay(instr[i].delay * 1000);
		}
		i80_set_cmd_cnt(inst, cmd_cnt);
		i80_trig(inst);
	}
}

// to do
void vo_mac_sel_remux(unsigned char inst, struct vo_d_remap *pins, unsigned int pin_num)
{
	int i = 0;
	for (i = 0; i < pin_num; ++i) {
		switch (pins[i].sel) {
		case VO_MIPI_TXP2:
		case VO_VIVO_CLK:
		case VO_MIPI_TXM2:
		case VO_MIPI_TXP1:
		case VO_MIPI_TXM1:
		case VO_MIPI_TXP0:
		case VO_MIPI_TXM0:
		case VO_MIPI_RXP0:
		case VO_MIPI_RXN0:
		case VO_MIPI_RXP1:
		case VO_MIPI_RXN1:
		case VO_MIPI_RXP2:
		case VO_MIPI_RXN2:
		case VO_MIPI_RXP5:
		case VO_MIPI_RXN5:
		case VO_VIVO_D0:
		case VO_VIVO_D1:
		case VO_VIVO_D2:
		case VO_VIVO_D3:
		case VO_VIVO_D4:
		case VO_VIVO_D5:
		case VO_VIVO_D6:
		case VO_VIVO_D7:
		case VO_VIVO_D8:
		case VO_VIVO_D9:
		case VO_VIVO_D10:
		case VO_MIPI_TXM4:
		case VO_MIPI_TXP4:
		case VO_MIPI_TXM3:
		case VO_MIPI_TXP3:
		case VO_JTAG_CPU_TMS:
		case VO_JTAG_CPU_TCK:
		case VO_JTAG_CPU_TRST:
		case VO_AUX0:
			vo_config_pinmux(pins[i].sel);
		break;
		case VO_SD1_D3:
		case VO_SD1_D2:
		case VO_SD1_D1:
		case VO_SD1_D0:
		case VO_SD1_CMD:
		case VO_SD1_CLK:
			vo_config_pinmux(pins[i].sel);
			vo_mac_ext_vo_pinmux_set(pins[i].sel);
		break;
		default:
		break;
		}
		if (pins[i].mux != VO_MUX_BT_CLK)
			vo_mac_mux_sel(inst, pins[i].sel, pins[i].mux);
	}
}
osal_module_export(vo_mac_sel_remux);

void vo_mac_sel_pinmux(unsigned char inst, enum vo_disp_intf intf_type, void *param)
{
	if (intf_type == VO_DISP_INTF_BT656 || intf_type == VO_DISP_INTF_BT1120) {
		struct bt_intf_cfg *cfg = param;
		vo_mac_sel_remux(inst, cfg->pins.d_pins, cfg->pins.pin_num);
	} else if (intf_type == VO_DISP_INTF_HW_I80) {
		struct hw_mcu_intf_cfg *cfg = param;
		vo_mac_sel_remux(inst, cfg->pins.d_pins, cfg->pins.pin_num);
	}

}
osal_module_export(vo_mac_sel_pinmux);
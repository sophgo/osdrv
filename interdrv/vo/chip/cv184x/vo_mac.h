#ifndef _VO_MAC_H_
#define _VO_MAC_H_

#include "vo_define.h"

enum vo_mac_sel {
	VO_MAC_SEL_DISABLE,
	VO_MAC_SEL_RGB,
	VO_MAC_SEL_SW,
	VO_MAC_SEL_I80,
	VO_MAC_SEL_BT601,
	VO_MAC_SEL_BT656,
	VO_MAC_SEL_BT1120,
	VO_MAC_SEL_BT1120R,
	VO_MAC_SEL_SERIAL_RGB,
	VO_MAC_SEL_HW_MCU,
	VO_MAC_SEL_MAX,
};

union vo_mac_mux_sel {
	struct {
		unsigned int vo_sel_type	: 4;
	} b;
	unsigned int raw;
};

union vo_mac_mux {
	struct {
		unsigned int vod_sel0	: 8;
		unsigned int vod_sel1	: 8;
		unsigned int vod_sel2	: 8;
		unsigned int vod_sel3	: 8;
	} b;
	unsigned int raw;
};

enum hw_mcu_format {
	HW_MCU_FORMAT_RGB565 = 0,
	HW_MCU_FORMAT_RGB888,
	HW_MCU_FORMAT_MAX,
};

union mcu_if_ctrl {
	struct {
		unsigned int i80_if_en	: 1;
		unsigned int i80_sw_mode_en	: 1;
		unsigned int i80_hw_if_en	: 1;
		unsigned int resv	: 29;
	} b;
	unsigned int raw;
};

union hw_mcu_auto {
	struct {
		unsigned int mcu_hw_trig	: 1;
		unsigned int mcu_hw_stop	: 1;
		unsigned int cs_h_hw_blk	: 2;
		unsigned int mcu_565	: 1;
		unsigned int resv	: 27;
	} b;
	unsigned int raw;
};

/**
 * @fmt_sel: [0] clk select
 *		0: bt clock 2x of disp clock
 *		1: bt clock 2x of disp clock
 *	     [1] sync signal index
 *		0: with sync pattern
 *		1: without sync pattern
 * @hde_gate: gate output hde with vde
 * @data_seq: fmt_sel[0] = 0
 *		00: Cb0Y0Cr0Y1
 *		01: Cr0Y0Cb0Y1
 *		10: Y0Cb0Y1Cr0
 *		11: Y0Cr0Y1Cb0
 *	      fmt_sel[0] = 1
 *		0: Cb0Cr0
 *		1: Cr0Cb0
 * @clk_inv: clock rising edge at middle of data
 * @vs_inv: vs low active
 * @hs_inv: hs low active
 */
union bt_enc {
	struct {
		unsigned int fmt_sel	: 2;
		unsigned int resv_1	: 1;
		unsigned int hde_gate	: 1;
		unsigned int data_seq	: 2;
		unsigned int resv_2	: 2;
		unsigned int clk_inv	: 1;
		unsigned int hs_inv	: 1;
		unsigned int vs_inv	: 1;
	} b;
	unsigned int raw;
};

/**
 * @ sav_vld: sync pattern for start of valid data
 * @ sav_blk: sync pattern for start of blanking data
 * @ eav_vld: sync pattern for end of valid data
 * @ eav_blk: sync pattern for end of blanking data
 */
union bt_sync_code {
	struct {
		unsigned char sav_vld;
		unsigned char sav_blk;
		unsigned char eav_vld;
		unsigned char eav_blk;
	} b;
	unsigned int raw;
};

union srgb_ctrl {
	struct {
		unsigned int srgb_ttl_en	: 1;
		unsigned int srgb_ttl_4t	: 1;
		unsigned int resv	: 30;
	} b;
	unsigned int raw;
};

void vo_mac_set_base_addr(unsigned char inst, void *base);
void vo_mac_set_data_mux(unsigned char inst, unsigned char vodata_selID, unsigned char value);
enum vo_mac_sel vo_mac_mux_get(unsigned char inst);
void vo_mac_set_sel_type(unsigned char inst, enum vo_mac_sel vo_mac_sel);
bool vo_mac_check_i80_enable(unsigned char inst);
void vo_mac_bt_set(unsigned char inst, union bt_enc enc, union bt_sync_code sync);
void vo_mac_bt_get(unsigned char inst, union bt_enc *enc, union bt_sync_code *sync);
void vo_mac_bt_en(unsigned char inst);
void vo_mac_mux_sel(unsigned char inst, int vo_mac_sel, int vo_mac_mux);
void vo_mac_srgb_ttl_en(unsigned char inst, bool enable);
void vo_mac_srgb_ttl_4x(unsigned char inst, bool is_4x);
void vo_mac_ext_vo_pinmux_set(int vo_sel);
void vo_mac_set_i80_if(unsigned char inst, unsigned char sw_mode, enum mcu_mode format);
void vo_mac_hw_i80_en(unsigned char inst, bool enable);
void i80_set_cmd0(unsigned char inst, unsigned int cmd);
void i80_set_cmd_cnt(unsigned char inst, unsigned int cmdcnt);
void i80_trig(unsigned char inst);
void hw_mcu_cmd_send(unsigned char inst, void *cmds, int size);
void vo_mac_sel_remux(unsigned char inst, struct vo_d_remap *pins, unsigned int pin_num);
void vo_mac_sel_pinmux(unsigned char inst, enum vo_disp_intf intf_type, void *param);

#endif  //_VO_MAC_H_



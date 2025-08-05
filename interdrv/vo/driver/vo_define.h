#ifndef _VO_DEFINE_H_
#define _VO_DEFINE_H_

#ifndef LANE_MAX_NUM
#define LANE_MAX_NUM   5
#endif

#include "comm_vo.h"

enum vo_disp_intf {
	VO_DISP_INTF_DSI = 0,
	VO_DISP_INTF_LVDS,
	VO_DISP_INTF_BT601,
	VO_DISP_INTF_BT656,
	VO_DISP_INTF_BT1120,
	//not support yet
	VO_DISP_INTF_PARALLEL_RGB,
	VO_DISP_INTF_SERIAL_RGB,
	VO_DISP_INTF_SW_I80,
	VO_DISP_INTF_HW_I80,
	VO_DISP_INTF_MAX,
};

/*
 * @pixelclock: pixel clock in kHz
 */
struct lvds_intf_cfg {
	unsigned long long pixelclock;
	vo_lvds_out_bit_e out_bits;
	vo_lvds_mode_e mode;
	unsigned char chn_num;
	unsigned char vs_out_en;
	unsigned char hs_out_en;
	unsigned char hs_blk_en;
	unsigned char msb_lsb_data_swap;
	unsigned char serial_msb_first;
	unsigned char even_odd_link_swap;
	unsigned char enable;
	signed char lane_id[LANE_MAX_NUM];
	unsigned char lane_pn_swap[LANE_MAX_NUM];
};

struct vo_d_remap {
	vo_mac_d_sel_e sel;
	unsigned int mux;
};

struct vo_pins {
	unsigned char pin_num;
	struct vo_d_remap d_pins[MAX_VO_PINS];
};

enum bt_mode {
	BT_MODE_656 = 0,
	BT_MODE_1120,
	BT_MODE_601,
	BT_MODE_MAX,
};

struct bt_intf_cfg {
	unsigned long long pixelclock;
	unsigned char is_set_param;
	unsigned char bt_clk_inv;
	unsigned char bt_vs_inv;
	unsigned char bt_hs_inv;
	enum bt_mode mode;
	vo_bt_data_seq_e data_seq;
	struct vo_pins pins;
};

enum mcu_mode {
	MCU_MODE_RGB565 = 0,
	MCU_MODE_RGB888,
	MCU_MODE_MAX,
};

struct mcu_instrs {
	unsigned char instr_num;
	vo_i80_instr_s instr_cmd[MAX_MCU_INSTR];
};

struct hw_mcu_intf_cfg {
	enum mcu_mode mode;
	struct vo_pins pins;
	struct mcu_instrs instrs;
	unsigned long long pixelclock;
};

struct vo_disp_intf_cfg {
	enum vo_disp_intf intf_type;
	union {
		struct lvds_intf_cfg lvds_cfg;
		struct bt_intf_cfg bt_cfg;
		struct hw_mcu_intf_cfg mcu_cfg;
		//to do: prgb/srgb i80(sw_i80/hw_mcu)
	};
};

enum vo_dv_pos_pol {
	VO_DV_VSYNC_POS_POL = 1,
	VO_DV_HSYNC_POS_POL,
};

struct vo_plane {
	unsigned int length;
	unsigned long addr;
	unsigned int bytesused;
	union {
		unsigned int offset;
		unsigned long userptr;
	} m;
};

struct vo_buffer {
	unsigned int index;
	unsigned int length;
	unsigned int start_x;
	unsigned int start_y;
	struct vo_plane planes[3];
};

enum vo_colorspace {
	VO_COLORSPACE_SRGB = 0,
	VO_COLORSPACE_SMPTE170M,
	VO_COLORSPACE_MAX,
};

struct vo_fmt {
	unsigned int fourcc;
	unsigned char fmt;
	unsigned char buffers;
	unsigned int bit_depth[3];
	unsigned char plane_sub_h;
	unsigned char plane_sub_v;
};

struct vo_plane_info {
	unsigned int sizeimage;
	unsigned short bytesperline;
};

struct vo_video_format {
	unsigned int width;
	unsigned int height;
	unsigned int pixelformat;
	unsigned int field;
	unsigned int colorspace;
	struct vo_plane_info plane_info[3];
	unsigned char num_planes;
};

#endif	// _VO_DEFINE_H_

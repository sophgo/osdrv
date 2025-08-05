#ifndef _DSI_MAC_H_
#define _DSI_MAC_H_

#include "vo_define.h"

#define MAX_DSI_LP 16
#define MAX_DSI_SP 2

/**
 * @ out_bit: 0(6-bit), 1(8-bit), others(10-bit)
 * @ vesa_mode: 0(JEIDA), 1(VESA)
 * @ dual_ch: dual link
 * @ vs_out_en: vs output enable
 * @ hs_out_en: hs output enable
 * @ hs_blk_en: vertical blanking hs output enable
 * @ ml_swap: lvdstx hs data msb/lsb swap
 * @ ctrl_rev: serializer 0(msb first), 1(lsb first)
 * @ oe_swap: lvdstx even/odd link swap
 * @ en: lvdstx enable
 */
union dsi_lvdstx {
	struct {
		unsigned int out_bit	: 2;
		unsigned int vesa_mode	: 1;
		unsigned int dual_ch	: 1;
		unsigned int vs_out_en	: 1;
		unsigned int hs_out_en	: 1;
		unsigned int hs_blk_en	: 1;
		unsigned int resv_1	: 1;
		unsigned int ml_swap	: 1;
		unsigned int ctrl_rev	: 1;
		unsigned int oe_swap	: 1;
		unsigned int en		: 1;
		unsigned int resv	: 20;
	} b;
	unsigned int raw;
};

enum dsi_mode {
	DSI_MODE_IDLE = 0,
	DSI_MODE_SPKT = 1,
	DSI_MODE_ESC = 2,
	DSI_MODE_HS = 4,
	DSI_MODE_UNKNOWN,
	DSI_MODE_MAX = DSI_MODE_UNKNOWN,
};

enum dsi_fmt {
	DSI_FMT_RGB888 = 0,
	DSI_FMT_RGB666,
	DSI_FMT_RGB565,
	DSI_FMT_RGB101010,
	DSI_FMT_MAX,
};

void dsi_mac_set_base_addr(unsigned char inst, void *base);
enum dsi_mode dsi_get_mode(unsigned char inst);
int dsi_set_mode(unsigned char inst, enum dsi_mode mode);
void dsi_clr_mode(unsigned char inst);
int dsi_chk_mode_done(unsigned char inst, enum dsi_mode mode);
int dsi_long_packet(unsigned char inst, unsigned char di, const unsigned char *data, unsigned char count, bool sw_mode);
int dsi_long_packet_raw(unsigned char inst, const unsigned char *data, unsigned char count);
int dsi_short_packet(unsigned char inst, unsigned char di, const unsigned char *data,
		     unsigned char count, bool sw_mode);
int dsi_dcs_write_buffer(unsigned char inst, unsigned char di, const void *data, size_t len, bool sw_mode);
int dsi_dcs_read_buffer(unsigned char inst, unsigned char di, const unsigned short data_param,
			unsigned char *data, size_t len);
int dsi_config(unsigned char inst, unsigned char lane_num, enum dsi_fmt fmt, unsigned short width);
void dsi_lvdstx_set(unsigned char inst, union dsi_lvdstx cfg);
void dsi_lvdstx_get(unsigned char inst, union dsi_lvdstx *cfg);

#endif  //_DSI_MAC_H_
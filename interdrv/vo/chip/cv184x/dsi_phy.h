#ifndef _DSI_PHY_H_
#define _DSI_PHY_H_

#include "comm_mipi_tx.h"
#include "disp.h"

void dphy_set_base_addr(unsigned char inst, void *base);
void dphy_dsi_lane_en(unsigned char inst, bool clk_en, bool *data_en, bool preamble_en);
void dphy_get_dsi_lane_status(unsigned char inst, bool *data_en);
bool dphy_get_dsi_clk_lane_status(unsigned char inst);
int dphy_dsi_set_lane(unsigned char inst, unsigned char lane_num, enum mipi_tx_lane_id lane,
		      bool pn_swap, bool clk_phase_shift);
void dphy_dsi_disable_lanes(unsigned char inst);
int dphy_dsi_get_lane(unsigned char inst, enum mipi_tx_lane_id *data_lane_num);
void dphy_init(unsigned char inst, enum vo_disp_intf intf);
void dphy_dsi_get_pixclk(unsigned char inst, unsigned int *clk_khz, unsigned char lane, unsigned char bits);
void dphy_dsi_set_pll(unsigned char inst, unsigned int clk_khz, unsigned char lane, unsigned char bits);
void dpyh_mipi_tx_manual_packet(unsigned char inst, const unsigned char *data, unsigned char count);
void dphy_set_hs_settle(unsigned char inst, unsigned char prepare, unsigned char zero, unsigned char trail);
void dphy_get_hs_settle(unsigned char inst, unsigned char *prepare, unsigned char *zero, unsigned char *trail);

bool dphy_is_lvds(unsigned char inst);
void dphy_lvds_analog_setting(unsigned char inst, bool is_lvds);
void dphy_lvds_set_pll(unsigned char inst, unsigned int clk_khz, unsigned char link);

#endif	// _DSI_PHY_H

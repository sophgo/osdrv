/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: dsi_phy.h
 * Description:
 */

#ifndef _CVI_DSI_PHY_H_
#define _CVI_DSI_PHY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "scaler.h"

enum lane_id {
	DSI_LANE_CLK = 0,
	DSI_LANE_0,
	DSI_LANE_1,
	DSI_LANE_2,
	DSI_LANE_3,
	DSI_LANE_MAX,
};

void dphy_set_base_addr(void *base);
void dphy_dsi_lane_en(bool clk_en, bool *data_en, bool preamble_en);
void dphy_get_dsi_lane_status(bool *data_en);
int dphy_dsi_set_lane(u8 lane_num, enum lane_id lane, bool pn_swap, bool clk_phase_shift);
int dphy_dsi_get_lane(enum lane_id *data_lane_num);
void dphy_init(enum sclr_vo_intf intf);
void dphy_dsi_get_pixclk(u32 *clkkHz, u8 lane, u8 bits);
void dphy_dsi_set_pll(u32 clkkHz, u8 lane, u8 bits);
void dphy_dsi_analog_setting(bool is_lvds);
void dphy_lvds_set_pll(u32 clkkHz, u8 link);

void dpyh_mipi_tx_manual_packet(const u8 *data, u8 count);

void dphy_set_hs_settle(u8 prepare, u8 zero, u8 trail);
void dphy_get_hs_settle(u8 *prepare, u8 *zero, u8 *trail);

#ifdef __cplusplus
}
#endif

#endif	/* _CVI_DSI_PHY_H */

#ifndef _CIF_COMM_H_
#define _CIF_COMM_H_

#include "comm_cif.h"
#include "cif_drv.h"
#include <vi_sys.h>
#include <base_cb.h>
#include <cif_cb.h>

#define DBG_ERR        1   /* error conditions                     */
#define DBG_WARN       2   /* warning conditions                   */
#define DBG_NOTICE     3   /* normal but significant condition     */
#define DBG_INFO       4   /* informational                        */
#define DBG_DEBUG      5   /* debug-level messages                 */

struct link;

void write_value_range(
					u32 *data,
					unsigned int start_bit,
					unsigned int end_bit,
					unsigned int value);
void cif_reset_param(struct link *link);

const char *_to_string_input_mode(enum input_mode_e input_mode);
const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk);
const char *_to_string_mclk(enum cam_pll_freq_e freq);
const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type);
const char *_to_string_mipi_wdr_mode(enum mipi_wdr_mode_e wdr);
const char *_to_string_wdr_mode(enum wdr_mode_e wdr);
const char *_to_string_lvds_sync_mode(enum lvds_sync_mode_e mode);
const char *_to_string_bit_endian(enum lvds_bit_endian endian);
const char *_to_string_lvds_vsync_type(enum lvds_vsync_type_e type);
const char *_to_string_lvds_fid_type(enum lvds_fid_type_e type);
const char *_to_string_mclk(enum cam_pll_freq_e freq);
const char *_to_string_csi_decode(enum csi_decode_fmt_e fmt);
const char *_to_string_dlane_state(enum mipi_dlane_state_e state);
const char *_to_string_deskew_state(enum mipi_deskew_state_e state);
const char *_to_string_cmd(unsigned int cmd);

#endif

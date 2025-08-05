#include "cif_comm.h"
#include "cif.h"
// #include <string.h>
#include "osal_ioctl.h"

void cif_reset_param(struct link *link)
{
	link->is_on = 0;
	link->clk_edge = CLK_UP_EDGE;
	link->msb = OUTPUT_NORM_MSB;
	link->crop_top = 0;
	link->distance_fp = 0;
	osal_memset(&link->param, 0, sizeof(struct cif_param));
	osal_memset(&link->attr, 0, sizeof(struct combo_dev_attr_s));
	osal_memset(&link->sts_csi, 0, sizeof(struct csi_status));
	osal_memset(&link->sts_lvds, 0, sizeof(struct lvds_status));
}

 /*Replace the start bit to the end bit of data with value*/
void write_value_range(
					u32 *data,
					unsigned int start_bit,
					unsigned int end_bit,
					unsigned int value)
{
	unsigned int num_bits;
	unsigned int max_value;
	unsigned int mask;

	if (start_bit > end_bit) {
		TRACE_CIF(DBG_ERR, "start_bit must be less than or equal to end_bit.\n");
		return;
	}

	num_bits = end_bit - start_bit + 1;
	max_value = (1U << num_bits) - 1;

	if (value > max_value) {
		TRACE_CIF(DBG_ERR, "value exceeds the range defined by start_bit and end_bit.\n");
		return;
	}

	mask = max_value << start_bit;
	*data = (*data & ~mask) | ((value << start_bit) & mask);
}

const char *_to_string_input_mode(enum input_mode_e input_mode)
{
	switch (input_mode) {
	case INPUT_MODE_MIPI:
		return "MIPI";
	case INPUT_MODE_SUBLVDS:
		return "SUBLVDS";
	case INPUT_MODE_HISPI:
		return "HISPI";
	case INPUT_MODE_CMOS:
		return "CMOS";
	case INPUT_MODE_BT1120:
		return "BT1120";
	case INPUT_MODE_BT601:
		return "INPUT_MODE_BT601";
	case INPUT_MODE_BT656_9B:
		return "INPUT_MODE_BT656_9B";
	case INPUT_MODE_BT656_9B_DDR:
		return "INPUT_MODE_BT656_9B_DDR";
	case INPUT_MODE_CUSTOM_0:
		return "INPUT_MODE_CUSTOM_0";
	case INPUT_MODE_BT_DEMUX:
		return "INPUT_MODE_BT_DEMUX";
	default:
		return "unknown";
	}
}

const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk)
{
	switch (mac_clk) {
	case RX_MAC_CLK_150M:
		return "150MHZ";
	case RX_MAC_CLK_200M:
		return "200MHZ";
	case RX_MAC_CLK_300M:
		return "300MHZ";
	case RX_MAC_CLK_400M:
		return "400MHZ";
	case RX_MAC_CLK_500M:
		return "500MHZ";
	case RX_MAC_CLK_600M:
		return "600MHZ";
	case RX_MAC_CLK_900M:
		return "900MHZ";
	default:
		return "unknown";
	}
}

const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type)
{
	switch (raw_data_type) {
	case RAW_DATA_8BIT:
		return "RAW8";
	case RAW_DATA_10BIT:
		return "RAW10";
	case RAW_DATA_12BIT:
		return "RAW12";
	case YUV422_8BIT:
		return "YUV422_8BIT";
	case YUV422_10BIT:
		return "YUV422_10BIT";
	default:
		return "unknown";
	}
}

const char *_to_string_mipi_wdr_mode(enum mipi_wdr_mode_e wdr)
{
	switch (wdr) {
	case MIPI_WDR_MODE_NONE:
		return "NONE";
	case MIPI_WDR_MODE_VC:
		return "VC";
	case MIPI_WDR_MODE_DT:
		return "DT";
	case MIPI_WDR_MODE_DOL:
		return "DOL";
	case MIPI_WDR_MODE_MANUAL:
		return "MANUAL";
	default:
		return "unknown";
	}
}

const char *_to_string_wdr_mode(enum wdr_mode_e wdr)
{
	switch (wdr) {
	case CIF_WDR_MODE_NONE:
		return "NONE";
	case CIF_WDR_MODE_2F:
		return "2To1";
	case CIF_WDR_MODE_3F:
		return "3To1";
	case CIF_WDR_MODE_DOL_2F:
		return "DOL2To1";
	case CIF_WDR_MODE_DOL_3F:
		return "DOL3To1";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_sync_mode(enum lvds_sync_mode_e mode)
{
	switch (mode) {
	case LVDS_SYNC_MODE_SOF:
		return "SOF";
	case LVDS_SYNC_MODE_SAV:
		return "SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_bit_endian(enum lvds_bit_endian endian)
{
	switch (endian) {
	case LVDS_ENDIAN_LITTLE:
		return "LITTLE";
	case LVDS_ENDIAN_BIG:
		return "BIG";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_vsync_type(enum lvds_vsync_type_e type)
{
	switch (type) {
	case LVDS_VSYNC_NORMAL:
		return "NORMAL";
	case LVDS_VSYNC_SHARE:
		return "SHARE";
	case LVDS_VSYNC_HCONNECT:
		return "HCONNECT";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_fid_type(enum lvds_fid_type_e type)
{
	switch (type) {
	case LVDS_FID_NONE:
		return "FID_NONE";
	case LVDS_FID_IN_SAV:
		return "FID_IN_SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_mclk(enum cam_pll_freq_e freq)
{
	switch (freq) {
	case CAMPLL_FREQ_NONE:
		return "CAMPLL_FREQ_NONE";
	case CAMPLL_FREQ_37P125M:
		return "CAMPLL_FREQ_37P125M";
	case CAMPLL_FREQ_25M:
		return "CAMPLL_FREQ_25M";
	case CAMPLL_FREQ_27M:
		return "CAMPLL_FREQ_27M";
	case CAMPLL_FREQ_24M:
		return "CAMPLL_FREQ_24M";
	case CAMPLL_FREQ_26M:
		return "CAMPLL_FREQ_26M";
	default:
		return "unknown";
	}
}

const char *_to_string_csi_decode(enum csi_decode_fmt_e fmt)
{
	switch (fmt) {
	case DEC_FMT_YUV422_8:
		return "yuv422-8";
	case DEC_FMT_YUV422_10:
		return "yuv422-10";
	case DEC_FMT_RAW8:
		return "raw8";
	case DEC_FMT_RAW10:
		return "raw10";
	case DEC_FMT_RAW12:
		return "raw12";
	default:
		return "unknown";
	}
}

const char *_to_string_dlane_state(enum mipi_dlane_state_e state)
{
	switch (state) {
	case HS_IDLE:
		return "hs_idle";
	case HS_SYNC:
		return "hs_sync";
	case HS_SKEW_CAL:
		return "skew_cal";
	case HS_ALT_CAL:
		return "alt_cal";
	case HS_PREAMPLE:
		return "preample";
	case HS_HST:
		return "hs_hst";
	case HS_ERR:
		return "hs_err";
	default:
		return "unknown";
	}
}

const char *_to_string_deskew_state(enum mipi_deskew_state_e state)
{
	switch (state) {
	case DESKEW_IDLE:
		return "idle";
	case DESKEW_START:
		return "start";
	case DESKEW_DONE:
		return "done";
	default:
		return "unknown";
	}
}

const char *_to_string_cmd(unsigned int cmd)
{
	switch (cmd) {
	case MIPI_SET_DEV_ATTR:
		return "MIPI_SET_DEV_ATTR";
	case MIPI_SET_HS_MODE:
		return "MIPI_SET_HS_MODE";
	case MIPI_SET_OUTPUT_CLK_EDGE:
		return "MIPI_SET_OUTPUT_CLK_EDGE";
	case MIPI_RESET_MIPI:
		return "MIPI_RESET_MIPI";
	case MIPI_SET_CROP_TOP:
		return "MIPI_SET_CROP_TOP";
	case MIPI_SET_WDR_MANUAL:
		return "MIPI_SET_WDR_MANUAL";
	case MIPI_SET_LVDS_FP_VS:
		return "MIPI_SET_LVDS_FP_VS";
	case MIPI_RESET_SENSOR:
		return "MIPI_RESET_SENSOR";
	case MIPI_UNRESET_SENSOR:
		return "MIPI_UNRESET_SENSOR";
	case MIPI_ENABLE_SENSOR_CLOCK:
		return "MIPI_ENABLE_SENSOR_CLOCK";
	case MIPI_DISABLE_SENSOR_CLOCK:
		return "MIPI_DISABLE_SENSOR_CLOCK";
	case MIPI_RESET_LVDS:
		return "MIPI_RESET_LVDS";
	case MIPI_GET_CIF_ATTR:
		return "MIPI_GET_CIF_ATTR";
	case MIPI_SET_MAX_MAC_CLOCK:
		return "MIPI_SET_MAX_MAC_CLOCK";
	case MIPI_SET_CROP_WINDOW:
		return "MIPI_SET_CROP_WINDOW";
	default:
		return "unknown";
	}
	return "unknown";
}
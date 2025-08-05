#include "vo_reg.h"
#include "reg.h"
#include "disp.h"
#include "dsi_mac.h"
#include "dsi_phy.h"
#include "vo_debug.h"
#include "osal.h"

static uintptr_t reg_dsi_mac_base[DISP_MAX_INST];

void dsi_mac_set_base_addr(unsigned char inst, void *base)
{
	reg_dsi_mac_base[inst] = (uintptr_t)base;
}

#define POLY 0x8408
/*
 *                                      16   12   5
 * this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
 * This works out to be 0x1021, but the way the algorithm works
 * lets us use 0x8408 (the reverse of the bit pattern).  The high
 * bit is always assumed to be set, thus we only use 16 bits to
 * represent the 17 bit value.
 */
static unsigned short crc16(unsigned char *data_p, unsigned short length)
{
	unsigned char i, data;
	unsigned short crc = 0xffff;

	if (length == 0)
		return (~crc);

	do {
		for (i = 0, data = 0xff & *data_p++; i < 8; i++, data >>= 1) {
			if ((crc & 0x0001) ^ (data & 0x0001))
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
		}
	} while (--length);

	return crc;
}

static unsigned char ecc(unsigned char *data)
{
	char D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12;
	char D13, D14, D15, D16, D17, D18, D19, D20, D21, D22, D23;
	char P0, P1, P2, P3, P4, P5, P6, P7;

	D0  = data[0] & 0x01;
	D1  = (data[0] >> 1) & 0x01;
	D2  = (data[0] >> 2) & 0x01;
	D3  = (data[0] >> 3) & 0x01;
	D4  = (data[0] >> 4) & 0x01;
	D5  = (data[0] >> 5) & 0x01;
	D6  = (data[0] >> 6) & 0x01;
	D7  = (data[0] >> 7) & 0x01;

	D8  = data[1] & 0x01;
	D9  = (data[1] >> 1) & 0x01;
	D10 = (data[1] >> 2) & 0x01;
	D11 = (data[1] >> 3) & 0x01;
	D12 = (data[1] >> 4) & 0x01;
	D13 = (data[1] >> 5) & 0x01;
	D14 = (data[1] >> 6) & 0x01;
	D15 = (data[1] >> 7) & 0x01;

	D16 = data[2] & 0x01;
	D17 = (data[2] >> 1) & 0x01;
	D18 = (data[2] >> 2) & 0x01;
	D19 = (data[2] >> 3) & 0x01;
	D20 = (data[2] >> 4) & 0x01;
	D21 = (data[2] >> 5) & 0x01;
	D22 = (data[2] >> 6) & 0x01;
	D23 = (data[2] >> 7) & 0x01;

	P7 = 0;
	P6 = 0;
	P5 = (D10^D11^D12^D13^D14^D15^D16^D17^D18^D19^D21^D22^D23) & 0x01;
	P4 = (D4^D5^D6^D7^D8^D9^D16^D17^D18^D19^D20^D22^D23) & 0x01;
	P3 = (D1^D2^D3^D7^D8^D9^D13^D14^D15^D19^D20^D21^D23) & 0x01;
	P2 = (D0^D2^D3^D5^D6^D9^D11^D12^D15^D18^D20^D21^D22) & 0x01;
	P1 = (D0^D1^D3^D4^D6^D8^D10^D12^D14^D17^D20^D21^D22^D23) & 0x01;
	P0 = (D0^D1^D2^D4^D5^D7^D10^D11^D13^D16^D20^D21^D22^D23) & 0x01;

	return (P7 << 7) | (P6 << 6) | (P5 << 5) | (P4 << 4) |
		(P3 << 3) | (P2 << 2) | (P1 << 1) | P0;
}

/**
 * dsi_get_mode - get current dsi mode
 *
 * @return: current dsi mode
 */
enum dsi_mode dsi_get_mode(unsigned char inst)
{
	return (_reg_read(REG_DSI_MAC_EN(inst)) & 0x07);
}

/**
 * dsi_clr_mode - let dsi back to idle mode
 *
 */
void dsi_clr_mode(unsigned char inst)
{
	unsigned int mode = _reg_read(REG_DSI_MAC_EN(inst)) & 0x7;

	TRACE_VO(DBG_DEBUG, "%s: mac_en reg(%#x)\n", __func__, mode);
	if (mode != DSI_MODE_IDLE)
		_reg_write_mask(REG_DSI_MAC_EN(inst), 0x7, mode);
}

/**
 * dsi_set_mode - set dsi mode
 *
 * @param mode: new dsi mode except for idle
 * @return: 0 if success
 */
int dsi_set_mode(unsigned char inst, enum dsi_mode mode)
{
	if (mode >= DSI_MODE_MAX)
		return -1;

	if (mode == DSI_MODE_IDLE) {
		dsi_clr_mode(inst);
		return 0;
	}

	_reg_write_mask(REG_DSI_MAC_EN(inst), 0x7, mode);

	return 0;
}
osal_module_export(dsi_set_mode);

/**
 * dsi_chk_mode_done - check if dsi's work done.
 *
 * @param mode: the mode to check.
 * @return: 0 if success
 */
int dsi_chk_mode_done(unsigned char inst, enum dsi_mode mode)
{
	unsigned int val = 0;

	if ((mode == DSI_MODE_ESC) || (mode == DSI_MODE_SPKT)) {
		val = _reg_read(REG_DSI_MAC_EN(inst)) & 0x30;
		return (val ^ (mode << 4)) ? -1 : 0;
	}

	if ((mode == DSI_MODE_IDLE) || (mode == DSI_MODE_HS)) {
		val = _reg_read(REG_DSI_MAC_EN(inst)) & 0x07;
		return (val == (mode)) ? 0 : -1;
	}

	return -1;
}
osal_module_export(dsi_chk_mode_done);

int _dsi_chk_and_clean_mode(unsigned char inst, enum dsi_mode mode)
{
	int i, ret;

	for (i = 0; i < 5; ++i) {
		osal_udelay(20);
		ret = dsi_chk_mode_done(inst, mode);
		if (ret == 0) {
			dsi_clr_mode(inst);
			break;
		}
	}
	return ret;
}


/**
 * dsi_long_packet_raw - send dsi long packet by escapet-lpdt.
 *
 * @param data: long packet data including header and crc.
 * @param count: number of long packet data.
 * @return: 0 if success
 */
int dsi_long_packet_raw(unsigned char inst, const unsigned char *data, unsigned char count)
{
	uintptr_t addr = REG_DSI_ESC_TX0(inst);
	unsigned int val = 0;
	unsigned char i = 0, packet_count, data_offset = 0;
	int ret;
	char str[128];

	TRACE_VO(DBG_DEBUG, "%s; count(%d)\n", __func__, count);
	while (count != 0) {
		if (count <= MAX_DSI_LP) {
			packet_count = count;
		} else if (count == MAX_DSI_LP + 1) {
			// [HW WorkAround] LPDT over can't take just one byte
			packet_count = MAX_DSI_LP - 1;
		} else {
			packet_count = MAX_DSI_LP;
		}
		count -= packet_count;
		val = 0x01 | ((packet_count - 1) << 8) | (count ? 0 : 0x10000);
		_reg_write(REG_DSI_ESC(inst), val);
		TRACE_VO(DBG_DEBUG, "%s: esc reg(%#x)\n", __func__, val);

		snprintf(str, 128, "%s: packet_count(%d) data(", __func__, packet_count);
		for (i = 0; i < packet_count; i += 4) {
			if (packet_count - i < 4) {
				val = 0;
				memcpy(&val, &data[data_offset], packet_count - i);
				data_offset += packet_count - i;
				_reg_write(addr + i, val);
				snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
				break;
			}
			memcpy(&val, &data[data_offset], 4);
			data_offset += 4;
			_reg_write(addr + i, val);
			snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
		}
		TRACE_VO(DBG_DEBUG, "%s)\n", str);

		dsi_set_mode(inst, DSI_MODE_ESC);
		ret = _dsi_chk_and_clean_mode(inst, DSI_MODE_ESC);
		if (ret != 0) {
			TRACE_VO(DBG_ERR, "%s: packet_count(%d) data0(%#x)\n", __func__, packet_count, data[0]);
			break;
		}
	}
	return ret;
}

/**
 * dsi_long_packet - send dsi long packet by escapet-lpdt.
 *
 * @param di: data ID
 * @param data: long packet data
 * @param count: number of long packet data, 100 at most.
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: 0 if success
 */
int dsi_long_packet(unsigned char inst, unsigned char di, const unsigned char *data, unsigned char count, bool sw_mode)
{
	unsigned char packet[128] = {di, count & 0xff, count >> 8, 0};
	unsigned short crc;

	if (count > 128 - 6) {
		TRACE_VO(DBG_ERR, "%s: count(%d) invalid\n", __func__, count);
		return -1;
	}

	packet[3] = ecc(packet);
	memcpy(&packet[4], data, count);
	count += 4;
	crc = crc16(packet, count);
	packet[count++] = crc & 0xff;
	packet[count++] = crc >> 8;

	if (!sw_mode)
		return dsi_long_packet_raw(inst, packet, count);

	dpyh_mipi_tx_manual_packet(inst, packet, count);
	return 0;
}

/**
 * dsi_short_packet - send dsi short packet by escapet-lpdt.
 *   *NOTE*: ecc is hw-generated.
 *
 * @param di: data ID
 * @param data: short packet data
 * @param count: number of short packet data, 1 or 2.
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: 0 if success
 */
int dsi_short_packet(unsigned char inst, unsigned char di, const unsigned char *data, unsigned char count, bool sw_mode)
{
	unsigned int val = 0;

	if ((count > MAX_DSI_SP) || (count == 0))
		return -1;

	val = di;
	if (count == 2) {
		//val = 0x15;
		val |= (data[0] << 8) | (data[1] << 16);
	} else {
		//val = 0x05;
		val |= data[0] << 8;
	}

	TRACE_VO(DBG_DEBUG, "%s: dev(%d) val(0x%x)\n", __func__, inst, val);

	if (!sw_mode) {
		_reg_write_mask(REG_DSI_HS_0(inst), 0x00ffffff, val);
		dsi_set_mode(inst, DSI_MODE_SPKT);
		return _dsi_chk_and_clean_mode(inst, DSI_MODE_SPKT);
	}
	val |= (ecc((unsigned char *)&val) << 24);
	dpyh_mipi_tx_manual_packet(inst, (unsigned char *)&val, 4);
	return 0;
}

/**
 * dsi_dcs_write_buffer - send dsi packet by escapet-lpdt.
 *
 * @param di: data ID
 * @param data: packet data
 * @param count: number of packet data
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: Zero on success or a negative error code on failure.
 */
int dsi_dcs_write_buffer(unsigned char inst, unsigned char di, const void *data, size_t len, bool sw_mode)
{
	if (len == 0) {
		TRACE_VO(DBG_ERR, "[mipi_tx] %s: 0 param unacceptable.\n", __func__);
		return -1;
	}

	if ((di == 0x06) || (di == 0x05) || (di == 0x04) || (di == 0x03)) {
		if (len != 1) {
			TRACE_VO(DBG_ERR, "[mipi_tx] %s: cmd(0x%02x) should has 1 param.\n", __func__, di);
			return -1;
		}
		return dsi_short_packet(inst, di, data, len, sw_mode);
	}
	if ((di == 0x15) || (di == 0x37) || (di == 0x13) || (di == 0x14) || (di == 0x23)) {
		if (len != 2) {
			TRACE_VO(DBG_ERR, "[mipi_tx] %s: cmd(0x%02x) should has 2 param.\n", __func__, di);
			return -1;
		}
		return dsi_short_packet(inst, di, data, len, sw_mode);
	}
	if ((di == 0x29) || (di == 0x39))
		return dsi_long_packet(inst, di, data, len, sw_mode);

	return dsi_long_packet(inst, di, data, len, sw_mode);
}
osal_module_export(dsi_dcs_write_buffer);

#define ACK_WR       0x02
#define GEN_READ_LP  0x1A
#define GEN_READ_SP1 0x11
#define GEN_READ_SP2 0x12
#define DCS_READ_LP  0x1C
#define DCS_READ_SP1 0x21
#define DCS_READ_SP2 0x22

int dsi_dcs_read_buffer(unsigned char inst, unsigned char di, const unsigned short data_param,
			unsigned char *data, size_t len)
{
	int ret = 0;
	unsigned int rx_data;
	int i = 0;

	if (len > 4)
		len = 4;

	if (dsi_get_mode(inst) == DSI_MODE_HS) {
		TRACE_VO(DBG_ERR, "[mipi_tx] %s: not work in HS.\n", __func__);
		return -1;
	}

	// [2:0] reg_esc_mode
	// [7:4] reg_esc_trig
	// [11:8] reg_tx_bc
	// [15:12] reg_bta_rx_bc
	// [16:16] reg_tx_bc_over: TX LPDT transfer over,0: Extend to next trigger,1: Transfer over in this trigger
	// only set necessery bits
	_reg_write_mask(REG_DSI_ESC(inst), 0x07, 0x04);

	// send read cmd
	dsi_short_packet(inst, di, (unsigned char *)&data_param, 2, false);

	// goto BTA
	dsi_set_mode(inst, DSI_MODE_ESC);
	if (_dsi_chk_and_clean_mode(inst, DSI_MODE_ESC) != 0) {
		TRACE_VO(DBG_ERR, "[mipi_tx] %s: BTA error.\n", __func__);
		return ret;
	}

	// check result
	rx_data = _reg_read(REG_DSI_ESC_RX0(inst));
	switch (rx_data & 0xff) {
	case GEN_READ_SP1:
	case DCS_READ_SP1:
		data[0] = (rx_data >> 8) & 0xff;
		break;
	case GEN_READ_SP2:
	case DCS_READ_SP2:
		data[0] = (rx_data >> 8) & 0xff;
		data[1] = (rx_data >> 16) & 0xff;
		break;
	case GEN_READ_LP:
	case DCS_READ_LP:
		rx_data = _reg_read(REG_DSI_ESC_RX1(inst));
		for (i = 0; i < len; ++i)
			data[i] = (rx_data >> (i * 8)) & 0xff;
		break;
	case ACK_WR:
		TRACE_VO(DBG_ERR, "[mipi_tx] %s: dcs read, ack with error(%#x %#x).\n"
			, __func__, (rx_data >> 8) & 0xff, (rx_data >> 16) & 0xff);
		ret = -1;
		break;
	default:
		TRACE_VO(DBG_ERR, "[mipi_tx] %s: unknown DT, %#x.", __func__, rx_data);
		ret = -1;
		break;
	}

	// TRACE_VO(DBG_DEBUG, "%s: %#x %#x\n", __func__, rx_data0, rx_data1);
	return ret;
}
osal_module_export(dsi_dcs_read_buffer);

int dsi_config(unsigned char inst, unsigned char lane_num, enum dsi_fmt fmt, unsigned short width)
{
	unsigned int val = 0;
	unsigned char bit_depth[] = {24, 18, 16, 30};

	if ((lane_num != 1) && (lane_num != 2) && (lane_num != 4))
		return OSAL_EINVAL;
	if (fmt > DSI_FMT_MAX)
		return OSAL_EINVAL;

	lane_num >>= 1;
	val = (fmt << 30) | (lane_num << 24);
	_reg_write_mask(REG_DSI_HS_0(inst), 0xc3000000, val);
	val = (width / 10) << 16 | UPPER(width * bit_depth[fmt], 3);
	_reg_write(REG_DSI_HS_1(inst), val);

	return 0;
}
osal_module_export(dsi_config);


void dsi_lvdstx_set(unsigned char inst, union dsi_lvdstx cfg)
{
	_reg_write(REG_LVDSTX_MAC(inst), cfg.raw);
}

void dsi_lvdstx_get(unsigned char inst, union dsi_lvdstx *cfg)
{
	cfg->raw = _reg_read(REG_LVDSTX_MAC(inst));
}

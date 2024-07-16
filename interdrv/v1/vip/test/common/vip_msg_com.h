/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: vip_msg_com.h
 * Description:
 */

#ifndef __SNR_ISP_COM_H__
#define __SNR_ISP_COM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ut_common.h>

struct snr_csibdg_info {
	uint8_t  hdr_on;
	uint32_t w;
	uint32_t h;
};

enum snr_isp_cmd {
	CSIBDG_CTRL = 0,
	ISP_CTRL_RELEASE,
	CMD_MAX,
};

struct vip_msg_ctx {
	enum snr_isp_cmd cmd;
	void *indata;
	void *retdata;
};

enum cif_rx_cmd {
	CIF_RESET_SENSOR = 0,
	CIF_RESET_MIPI,
	CIF_SET_DEV_ATTR,
	CIF_ENABLE_SNSR_CLK,
	CIF_UNRESET_SENSOR,
};

struct cif_rx_ctx {
	enum cif_rx_cmd	cmd;
	void *indata;
	void *retdata;
};

enum wrap_i2c_cmd {
	I2C_WRITE = 0,
};

struct wrap_i2c_ctx {
	enum wrap_i2c_cmd cmd;
	void *indata;
	void *retdata;
};

#ifdef __cplusplus
}
#endif

#endif /* __SNR_ISP_COM_H__ */


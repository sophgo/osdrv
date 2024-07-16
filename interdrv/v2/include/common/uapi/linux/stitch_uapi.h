/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: stitch_uapi.h
 * Description:
 */

#ifndef _U_STITCH_UAPI_H_
#define _U_STITCH_UAPI_H_

#include "comm_stitch.h"

#ifdef __cplusplus
extern "C" {
#endif

struct stitch_src_frm_cfg {
	unsigned char src_id;
	video_frame_info_s video_frame;
	int milli_sec;
};

struct stitch_chn_frm_cfg {
	video_frame_info_s video_frame;
	int milli_sec;
};

struct stitch_vb_pool_cfg {
	void *reserved;
	unsigned int vb_pool;
};

/* Public */

#define STITCH_INIT          _IO('S',   0x00)
#define STITCH_DEINIT        _IO('S',   0x01)
#define STITCH_SET_SRC_ATTR  _IOW('S',  0x02, stitch_src_attr)
#define STITCH_GET_SRC_ATTR  _IOWR('S', 0x03, stitch_src_attr)
#define STITCH_SET_CHN_ATTR  _IOW('S',  0x04, stitch_chn_attr)
#define STITCH_GET_CHN_ATTR  _IOWR('S', 0x05, stitch_chn_attr)
#define STITCH_SET_OP_ATTR   _IOW('S',  0x06, stitch_op_attr)
#define STITCH_GET_OP_ATTR   _IOWR('S', 0x07, stitch_op_attr)
#define STITCH_SET_WGT_ATTR   _IOW('S',  0x08, stitch_bld_wgt_attr)
#define STITCH_GET_WGT_ATTR   _IOWR('S', 0x09, stitch_bld_wgt_attr)

#define STITCH_SET_REGX      _IOW('S',  0x0a, uint8_t)
#define STITCH_DEV_ENABLE    _IO('S',   0x0b)
#define STITCH_DEV_DISABLE   _IO('S',   0x0c)
#define STITCH_SEND_SRC_FRM  _IOW('S',  0x0d, struct stitch_src_frm_cfg)
#define STITCH_SEND_CHN_FRM  _IOW('S',  0x0e, struct stitch_chn_frm_cfg)
#define STITCH_GET_CHN_FRM   _IOWR('S', 0x0f, struct stitch_chn_frm_cfg)
#define STITCH_RLS_CHN_FRM   _IOW('S',  0x10, struct stitch_chn_frm_cfg)
#define STITCH_ATTACH_VB_POOL _IOW('S', 0x11, struct stitch_vb_pool_cfg)
#define STITCH_DETACH_VB_POOL _IOW('S', 0x12, struct stitch_vb_pool_cfg)
#define STITCH_DUMP_REGS _IO('S', 0x13)
#define STITCH_RST _IO('S', 0x14)

#define STITCH_SUSPEND _IO('S',0x15)
#define STITCH_RESUME _IO('S',0x16)

/* Internal use */

#ifdef __cplusplus
}
#endif

#endif /* _U_VPSS_UAPI_H_ */

#ifndef __VPSS_CB_H__
#define __VPSS_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "base_ctx.h"
#include "osal.h"
#include "base_cb.h"
#include "ldc_cb.h"
#include "comm_vpss.h"

#define VPSS_STITCH_MAX_CHN 64

struct vpss_online_cb_info {
	u8  snr_num;
	u8  is_tile;
	u8  is_left_tile;
	u64 frm_num;
	struct vip_line l_in;
	struct vip_line l_out;
	struct vip_line r_in;
	struct vip_line r_out;
	struct mlv_i_s m_lv_i;
	osal_timeval ts;
};

struct vpss_online_err_handle_info {
	u8  snr_num;
};

struct vpss_grp_mlv_info {
	u16 vpss_grp;
	struct mlv_i_s m_lv_i;
};

struct vpss_window {
	struct vip_rect rect_out; /*outside rectangle*/
	struct vip_rect rect_in; /*inside rectangle*/
	__u32 top_width;
	__u32 bottom_width;
	__u32 left_width;
	__u32 right_width;
	__u8 border_color[3]; /*border color RGB888*/
	__u8 bgcolor[3]; /*background color RGB888*/
	__u8 flip; /*0:normal 1:mirror 2:flip 3:mirror+flip*/
};

struct stitch_chn_cfg {
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	struct vip_frmsize src_size;
	struct vip_rect rect_crop;
	struct vpss_window window;
};

struct stitch_dst_cfg {
	__u8 color[3]; //background color RGB888
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	struct vip_frmsize dst_size;
};

typedef void (*stitch_job_cb)(void *data);

struct vpss_stitch_cfg {
	__u8 num;
	struct stitch_dst_cfg dst_cfg;
	struct stitch_chn_cfg *chn_cfg;
	stitch_job_cb job_cb;
	void *data;
};

struct vpss_sbm_cfg {
	s32 grp_id; //input
	s32 chn_id; //input
	struct sbm_cfg st_sbm_cfg;
};

struct vpss_ldc_chn_attr {
	vpss_grp vpss_grp;
	vpss_chn vpss_chn;
	size_s size;
};

struct vpss_ldc_internal_chn_attr {
	mod_id_e mod;
	struct vpss_ldc_chn_attr vpss_chn_attr;
};

struct vpss_cb_chn_ldc_cfg {
	vpss_grp vpss_grp;
	vpss_chn vpss_chn;
	rotation_e rotation;
	vpss_ldc_attr_s ldc_attr;
	uint64_t mesh_handle;
};

struct vpss_ldc_internal_chn_ldc_cfg {
	mod_id_e mod;
	struct vpss_cb_chn_ldc_cfg vpss_cfg;
};

enum vpss_cb_cmd {
	VPSS_CB_VI_ONLINE_TRIGGER,
	VPSS_CB_SET_VIVPSSMODE,
	VPSS_CB_GET_RGN_HDLS,
	VPSS_CB_SET_RGN_HDLS,
	VPSS_CB_SET_RGN_CFG,
	VPSS_CB_SET_RGNEX_CFG,
	VPSS_CB_SET_COVEREX_CFG,
	VPSS_CB_SET_MOSAIC_CFG,
	VPSS_CB_SET_RGN_LUT_CFG,
	VPSS_CB_GET_RGN_OW_ADDR,
	VPSS_CB_GET_CHN_SIZE,
	VPSS_CB_ONLINE_ERR_HANDLE,
	VPSS_CB_OVERFLOW_CHECK,
	VPSS_CB_GET_SBM_INFO,
	VPSS_CB_RESET_SBM,
	VPSS_CB_GET_MLV_INFO,
	VPSS_CB_STITCH,
	VPSS_CB_GDC_OP_DONE = LDC_CB_GDC_OP_DONE,
	VPSS_CB_GDC_GET_CHN_ATTR,
	VPSS_CB_GDC_SET_CHN_CFG,
	VPSS_CB_SBM_FRM_DONE,
	VPSS_CB_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __VPSS_CB_H__ */


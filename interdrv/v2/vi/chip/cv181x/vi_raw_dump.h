#ifndef __VI_RAW_DUMP_H__
#define __VI_RAW_DUMP_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <vi_isp_buf_ctrl.h>

extern struct isp_buffer *isp_byr[ISP_PRERAW_MAX], *isp_byr_se[ISP_PRERAW_MAX];

extern struct isp_queue raw_dump_b_q[ISP_PRERAW_MAX], raw_dump_b_se_q[ISP_PRERAW_MAX],
	raw_dump_b_dq[ISP_PRERAW_MAX], raw_dump_b_se_dq[ISP_PRERAW_MAX];

/*******************************************************************************
 *	rawdump interfaces
 ******************************************************************************/
void _isp_fe_be_raw_dump_cfg(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 chn_num);
int isp_raw_dump(struct cvi_vi_dev *vdev, struct cvi_vip_isp_raw_blk *dump);
void free_isp_byr(u8 raw_num);

int isp_start_smooth_raw_dump(struct cvi_vi_dev *vdev, struct cvi_vip_isp_smooth_raw_param *pstSmoothRawParam);
int isp_stop_smooth_raw_dump(struct cvi_vi_dev *vdev, struct cvi_vip_isp_smooth_raw_param *pstSmoothRawParam);
int isp_get_smooth_raw_dump(struct cvi_vi_dev *vdev, struct cvi_vip_isp_raw_blk *dump);
int isp_put_smooth_raw_dump(struct cvi_vi_dev *vdev, struct cvi_vip_isp_raw_blk *dump);

void _isp_raw_dump_chk(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const uint32_t frm_num);

#ifdef __cplusplus
}
#endif

#endif //__VI_RAW_DUMP_H__

#ifndef __VI_SDK_LAYER_H__
#define __VI_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "vi_defines.h"

struct vi_ctrl {
	__u32 id;
	__s32 dev;
	__s32 pipe;
	__s32 chn;
	__u32 size;
	__u32 reserved[2];

	union {
		__s32 val;
		__s64 value64;
		void *ptr;
	};
};

#define CHK_STRUCT_SIZE(size, expect_size)									\
	do {													\
		if ((size) != (expect_size)) {									\
			vi_pr(VI_ERR, "cmd_%d size error! expect size %zu, but %u\n", id, expect_size, size);	\
			return ERR_VI_INVALID_PARA;								\
		}												\
	} while (0)

/*****************************************************************************
 *  vi structure and enum for vi sdk layer
 ****************************************************************************/
struct sop_isp_buf {
	struct vi_buffer buf;
	struct osal_list_head list;
};

/*****************************************************************************
 *  vi function prototype for vi sdk layer
 ****************************************************************************/
void vi_sdk_release(struct sop_vi_dev *vdev);
int vi_create_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id);
void vi_destroy_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id);
void sop_isp_rdy_buf_queue(struct sop_vi_dev *vdev, struct sop_isp_buf *b);
int usr_pic_timer_init(struct sop_vi_dev *vdev);
int usr_pic_timer_start(struct sop_vi_dev *vdev);
int user_pic_trig(struct sop_vi_dev *vdev, bool is_frm_rst);
int usr_pic_time_remove(struct sop_vi_dev *vdev);
void vi_destory_dbg_thread(struct sop_vi_dev *vdev);

/*****************************************************************************
 *  vi sdk ioctl function prototype for vi layer
 ****************************************************************************/
int vi_disable_chn(struct sop_vi_dev *vdev, int pipe, int chn);
long vi_sdk_ctrl(struct sop_vi_dev *vdev, struct vi_ctrl *ctrl);
int vi_sdk_qbuf(mmf_chn_s chn, void *data);

int vi_get_chn_attr(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, vi_chn_attr_s *chn_attr);
int vi_set_chn_ldc_attr(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, const vi_ldc_attr_s *ldc_attr, u64 mesh_addr);

#ifdef __cplusplus
}
#endif

#endif //__VI_SDK_LAYER_H__

#ifndef __VI_H__
#define __VI_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal_def.h"
#include "vi_ctx.h"
#include "vi_common.h"

#include "vi_defines.h"
#include "vi_drv.h"
#include "base_cb.h"

/*******************************************************
 *  File operations for core
 ******************************************************/
void vi_sw_reset(struct vi_dev *vdev);
int vi_suspend(struct vi_dev *vdev);
int vi_resume(struct vi_dev *vdev);
void vi_irq_handler(struct vi_dev *vdev);
int vi_create_instance(struct vi_dev *vdev);
int vi_destroy_instance(struct vi_dev *vdev);
int vi_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg);

/*******************************************************
 *  Common interface for core
 ******************************************************/
//for isp ioctrl
void isp_snr_cfg_enq(struct isp_snr_cfg *snr_cfg_queue, struct sop_isp_snr_update *snr_node);

//for sdk_layer
int usr_pic_timer_init(struct vi_dev *vdev);
int usr_pic_timer_start(struct vi_dev *vdev);
int user_pic_trig(struct vi_dev *vdev, bool is_frm_rst);
int usr_pic_time_remove(struct vi_dev *vdev);

int vi_csi_start_streaming(struct vi_dev *vdev, uint8_t raw_num);
int vi_isp_start_streaming(struct vi_dev *vdev, uint8_t pipe, uint8_t chn);
int vi_csi_stop_streaming(struct vi_dev *vdev, uint8_t raw_num);
int vi_isp_stop_streaming(struct vi_dev *vdev, uint8_t pipe, uint8_t chn);

void vi_csi_ctrl_init(struct vi_dev *vdev, enum sop_isp_raw raw_num);
void vi_isp_ctrl_init(struct vi_dev *vdev, uint8_t pipe);
void vi_set_tuning_dis(int pipe, int fe_ctrl, int post_ctrl);
void vi_scene_ctrl(struct vi_dev *vdev);
void vi_preraw_trigger(struct vi_dev *vdev, const u8 pipe, const u8 chn);
void vi_destroy_dbg_thread(struct vi_dev *vdev);

int vi_update_ldc_mesh(struct vi_dev *vdev, vi_pipe pipe, vi_chn chn, const vi_ldc_attr_s *ldc_attr, u64 paddr);

int vi_create_thread(struct vi_dev *vdev, enum E_VI_TH th_id);
void vi_destroy_thread(struct vi_dev *vdev, enum E_VI_TH th_id);

#ifdef __cplusplus
}
#endif

#endif /* __VI_H__ */

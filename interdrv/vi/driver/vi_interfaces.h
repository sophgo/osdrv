#ifndef __VI_INTERFACES_H__
#define __VI_INTERFACES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "base_cb.h"
#include "vi_defines.h"

/*******************************************************
 *  File operations for core
 ******************************************************/
void vi_sw_init(struct sop_vi_dev *vdev);
void vi_sw_deinit(struct sop_vi_dev *vdev);
int vi_suspend(struct sop_vi_dev *vdev);
int vi_resume(struct sop_vi_dev *vdev);
void vi_irq_handler(struct sop_vi_dev *vdev);
int vi_create_instance(struct sop_vi_dev *vdev);
int vi_destroy_instance(struct sop_vi_dev *vdev);

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vi_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg);

void _vi_csi_ctrl_init(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num);
void _vi_isp_ctrl_init(struct sop_vi_dev *vdev, uint8_t pipe);
int vi_set_tuning_dis(int pipe, int fe_ctrl, int post_ctrl);
void _vi_scene_ctrl(struct sop_vi_dev *vdev);
int vi_csi_start_streaming(struct sop_vi_dev *vdev, uint8_t raw_num);
int vi_isp_start_streaming(struct sop_vi_dev *vdev, uint8_t pipe, uint8_t chn);
int vi_csi_stop_streaming(struct sop_vi_dev *vdev, uint8_t raw_num);
int vi_isp_stop_streaming(struct sop_vi_dev *vdev, uint8_t pipe, uint8_t chn);

void isp_snr_cfg_enq(struct sop_isp_snr_update *snr_node, const enum sop_isp_raw raw_num);

#ifdef __cplusplus
}
#endif

#endif /* __VI_INTERFACES_H__ */

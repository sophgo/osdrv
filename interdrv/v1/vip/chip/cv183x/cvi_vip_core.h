/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_core.h
 * Description:
 */

#ifndef __CVI_VIP_CORE_H__
#define __CVI_VIP_CORE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/pm_qos.h>
#include <linux/iommu.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>

#include "uapi/cvi_vip.h"
#include "vip/scaler.h"
#include "uapi/isp_reg.h"
#include "vip/isp_drv.h"
#include "uapi/cvi_vip_isp.h"
#include "cif/cif_drv.h"
#include "uapi/cvi_vip_disp.h"
#include "uapi/cvi_vip_sc.h"

#define CVI_VIP_DRV_NAME "cvi_vip_driver"
#define CVI_VIP_DVC_NAME "cvi_vip_1835"

#define DEVICE_FROM_DTS 1

#define ISP_DEVICE_IDX  100
#define DWA_DEVICE_IDX  102
#define IMG_DEVICE_IDX  103   // 103,104
#define SC_DEVICE_IDX   105   // 105~108
#define DISP_DEVICE_IDX 109

#define VIP_MAX_PLANES 3

#define vip_fill_rect_from_v4l2(vip_rect, v4l2_rect) \
	do { \
		vip_rect.x = v4l2_rect.left; \
		vip_rect.y = v4l2_rect.top; \
		vip_rect.w = v4l2_rect.width; \
		vip_rect.h = v4l2_rect.height; \
	} while (0)

/* Instance is already queued on the job_queue */
#define TRANS_QUEUED        (1 << 0)
/* Instance is currently running in hardware */
#define TRANS_RUNNING       (1 << 1)
/* Instance is currently aborting */
#define TRANS_ABORT         (1 << 2)

enum cvi_img_type {
	CVI_VIP_IMG_D = 0,
	CVI_VIP_IMG_V,
	CVI_VIP_IMG_MAX,
};

enum cvi_img_2_sc_bounding {
	CVI_VIP_IMG_2_SC_NONE = 0x00,
	CVI_VIP_IMG_2_SC_D = 0x01,
	CVI_VIP_IMG_2_SC_V = 0x02,
	CVI_VIP_IMG_2_SC_ALL = 0x03,
};

enum cvi_sc_type {
	CVI_VIP_SC_D = 0,
	CVI_VIP_SC_V0,
	CVI_VIP_SC_V1,
	CVI_VIP_SC_V2,
	CVI_VIP_SC_MAX,
};

enum cvi_vip_state {
	CVI_VIP_IDLE,
	CVI_VIP_RUNNING,
};

enum cvi_isp_postraw_state {
	ISP_POSTRAW_IDLE,
	ISP_POSTRAW_RUNNING,
};

enum cvi_isp_preraw_state {
	ISP_PRERAW_IDLE,
	ISP_PRERAW_RUNNING,
};

/* struct cvi_vip_fmt
 * @fourcc: v4l2 fourcc code.
 * @fmt: sclr driver's relative format.
 * @buffers: number of buffers.
 * @bit_depth: number of bits per pixel per plane.
 * @plane_sub_h: plane horizontal subsample.
 * @plane_sub_v: plane vertical subsample.
 */
struct cvi_vip_fmt {
	u32 fourcc;          /* v4l2 format id */
	enum sclr_format fmt;
	u8  buffers;
	u32 bit_depth[VIP_MAX_PLANES];
	u8 plane_sub_h;
	u8 plane_sub_v;
};

/* buffer for one video frame */
struct cvi_vip_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_v4l2_buffer          vb;
	struct list_head                list;
};

/**
 * @vdev: video_device instance
 * @opened: if device opened
 * @vb_q: vb2_queue
 * @rdy_queue: list of cvi_vip_buffer in the vb_q
 * @rdy_lock: spinlock for rdy_queue
 * @num_rdy: number of buffer in rdy_queue
 * @seq_count: the number of vb output from queue
 * @vid_caps: capabilities of the device
 * @fmt: format
 * @colorspace: V4L2 colorspace
 * @bytesperline: bytesperline of each plane
 * @sizeimage: size of the image per plane.
 * @frame_number: the number of vsync
 */
#define DEFINE_BASE_VDEV_PARAMS \
	struct video_device             vdev;       \
	atomic_t                        opened;     \
	struct vb2_queue                vb_q;       \
	struct list_head                rdy_queue;  \
	spinlock_t               rdy_lock;   \
	struct mutex                    mutex;	    \
	u8                              num_rdy;    \
	u32                             seq_count;  \
	u32                             vid_caps;   \
	const struct cvi_vip_fmt         *fmt;      \
	u32                             colorspace; \
	u32                             bytesperline[VIP_MAX_PLANES]; \
	u32                             sizeimage[VIP_MAX_PLANES]; \
	u32                             frame_number; \
	u8				align

#define DEFINE_ISP_VDEV_PARAMS \
	struct video_device             vdev;       \
	struct vb2_queue                vb_q;       \
	struct list_head                rdy_queue[ISP_PRERAW_MAX];  \
	spinlock_t                      rdy_lock;   \
	struct mutex                    mutex;    \
	u8                              num_rdy[ISP_PRERAW_MAX];    \
	u32                             seq_count;  \
	u32                             vid_caps;   \
	const struct cvi_vip_fmt         *fmt;      \
	u32                             colorspace; \
	u32                             bytesperline[VIP_MAX_PLANES]; \
	u32                             sizeimage[VIP_MAX_PLANES]; \
	u32                             frame_number; \
	u8				align; \
	u8				chn_id

#define DEFINE_SC_VDEV_PARAMS \
	struct video_device             vdev;       \
	atomic_t                        opened;     \
	spinlock_t                      rdy_lock;   \
	struct mutex                    mutex;	    \
	struct list_head                rdy_queue;  \
	struct list_head                ol_rdy_queue[ISP_PRERAW_MAX];  \
	u8                              num_rdy;    \
	u8                              ol_num_rdy[ISP_PRERAW_MAX];    \
	u32                             seq_count;  \
	u32                             vid_caps;   \
	const struct cvi_vip_fmt         *fmt;      \
	u32                             colorspace; \
	u32                             bytesperline[VIP_MAX_PLANES]; \
	u32                             sizeimage[VIP_MAX_PLANES]; \
	u8				align; \
	u8				job_grp

struct cvi_base_vdev {
	DEFINE_BASE_VDEV_PARAMS;
};

struct cvi_isp_base_vdev {
	DEFINE_ISP_VDEV_PARAMS;
};

struct cvi_isp_vdev {
	DEFINE_ISP_VDEV_PARAMS;

	// private data
	struct clk                      *isp_clk[4];
	struct clk                      *mac_clk[ISP_PRERAW_MAX];
	struct v4l2_mbus_framefmt	sns_fmt[ISP_PRERAW_MAX];
	struct v4l2_rect                sns_crop[ISP_PRERAW_MAX];
	struct v4l2_rect                sns_se_crop[ISP_PRERAW_MAX];
	struct v4l2_mbus_framefmt	usr_fmt;
	struct v4l2_rect                usr_crop;
	u64                             usr_pic_phy_addr;
	unsigned long			usr_pic_delay;
	enum cvi_isp_source		isp_source;
	struct cvi_isp_snr_info		snr_info[ISP_PRERAW_MAX];
	u32                             preraw_sof_count[ISP_PRERAW_MAX];
	u32                             preraw_frame_number[ISP_PRERAW_MAX];
	bool                            preraw_first_frm[ISP_PRERAW_MAX];
	u32                             postraw_frame_number[ISP_PRERAW_MAX];
	u32                             drop_frame_number[ISP_PRERAW_MAX];
	u8				postraw_proc_num;
	atomic_t                        preraw_state[ISP_PRERAW_MAX];
	atomic_t                        postraw_state;
	atomic_t                        isp_streamoff;
	struct v4l2_dv_timings          dv_timings;
	struct isp_ctx			ctx;
	u32				isp_err_times[ISP_PRERAW_MAX];
	atomic_t			isp_raw_dump_en[ISP_PRERAW_MAX];
	u32				isp_int_flag[ISP_PRERAW_MAX];
	wait_queue_head_t		isp_int_wait_q[ISP_PRERAW_MAX];
	u32				isp_pre_int_flag;
	wait_queue_head_t		isp_pre_wait_q;
	u32				isp_post_int_flag;
	wait_queue_head_t		isp_post_wait_q;
	void				*shared_mem;
};

struct cvi_dwa_vdev {
	struct video_device             vdev;
	struct v4l2_m2m_dev             *m2m_dev;
	struct list_head		ctx_list;
	u32                             ctx_num;

	u32                             vid_caps;
	struct mutex                    mutex;
	u8				align;

	// private data
	struct clk                      *clk;
	void                            *shared_mem;

	struct hrtimer                  timer;
};

/**
 * @dev_idx:  index of the device
 * @img_type: the type of this img_vdev
 * @sc_bounding: which sc instances are bounding with this img_vdev
 * @input_type: input type(isp, dwa, mem,...)
 * @job_flags: job status
 * @job_lock: lock of job
 * @src_size: img's input size
 * @crop_rect: img's output size, only work if input_type is mem
 */
struct cvi_img_vdev {
	DEFINE_BASE_VDEV_PARAMS;

	// private data
	struct clk                      *clk;
	u8                              dev_idx;
	enum sclr_img_in                img_type;
	enum cvi_img_2_sc_bounding      sc_bounding;
	enum cvi_input_type             input_type;
	unsigned long                   job_flags;
	spinlock_t                      job_lock;
	struct tasklet_struct           job_work;

	struct v4l2_frmsize_discrete    src_size;
	struct v4l2_rect                crop_rect;
	struct cvi_vpss_grp_cfg         vpss_grp_cfg;
	struct cvi_vpss_grp_cfg         ol_vpss_grp_cfg[2];
	u8                              job_grp;

	bool                            is_tile;
	bool                            is_work_on_r_tile;
	u8                              tile_mode;
	bool                            is_online_from_isp;
	atomic_t                        is_streaming;
	struct cvi_vpss_rgnex_cfg       rgnex_cfg;
	bool                            is_cmdq;
	u32                             irq_cnt;
	u32                             ol_irq_cnt[2];
	u32                             isp_trig_cnt[2];
	u32                             isp_trig_fail_cnt[2];
	u32                             user_trig_cnt;
	u32                             user_trig_fail_cnt;
	u8                              IntMask;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64               ts_start;
#else
	struct timespec                 ts_start;
#endif
};

/**
 * @dev_idx:  index of the device
 * @img_src:  bounding source of this sc_vdev
 * @sink_rect: sc's output rectangle, only if out-2-mem
 * @compose_out: sc's output size,
 * @crop_rect: sc's crop rectangle
 * @sc_coef: sc's scaling coefficient, such as bicubic, bilinear, ...
 * @tile_mode: sc's tile mode, both/left/right.
 * @is_streaming: to know if sc is stream-on.
 * @bind_fb: if sc use fb's gop.
 */
struct cvi_sc_vdev {
	DEFINE_SC_VDEV_PARAMS;

	// private data
	struct clk                      *clk;
	u8                              dev_idx;
	enum cvi_img_type               img_src;
	atomic_t                        job_state;

	struct v4l2_rect                sink_rect;
	struct v4l2_rect                compose_out;
	struct v4l2_rect                crop_rect;
	enum cvi_sc_scaling_coef        sc_coef;

	struct cvi_vpss_chn_cfg         vpss_chn_cfg;
	struct cvi_vpss_chn_cfg         ol_vpss_chn_cfg[2];

	u8                              tile_mode;
	void                            *shared_mem;
	atomic_t                        is_streaming;
	bool                            bind_fb;
	bool                            is_cmdq;
};

/**
 * @frame_number:  the number of vsync
 * @dv_timings: current v4l2 dv timing
 * @sink_rect: panel's resolution
 * @compose_out: output rectangle on frame
 * @disp_interface: display interface
 * @bgcolor_enable: if bgcolor enabled, no video will show except for disp-pattern
 */
struct cvi_disp_vdev {
	DEFINE_BASE_VDEV_PARAMS;

	// private data
	struct clk                      *clk_disp;
	struct clk                      *clk_bt;
	struct clk                      *clk_dsi;
	struct v4l2_dv_timings          dv_timings;
	struct v4l2_rect                sink_rect;
	struct v4l2_rect                compose_out;
	struct v4l2_rect                crop_rect;
	enum cvi_disp_intf              disp_interface;
	bool                            bgcolor_enable;
	void                            *shared_mem;
};

/**
 * @irq_num_scl:  irq_num of scl got from dts.
 * @irq_num_dwa:  irq_num of dwa got from dts.
 * @irq_num_isp:  irq_num of isp got from dts.
 * @dwa_vdev:     dev of dwa.
 * @img_vdev:     dev of sc-image.
 * @sc_vdev:      dev of sc-core.
 * @disp_vdev:    dev of sc-disp.
 * @isp_vdev:     dev of isp.
 * @disp_online:  if sc-core and sc-disp is online(no frame-buf).
 */
struct cvi_vip_dev {
	struct v4l2_device              v4l2_dev;
	spinlock_t               lock;
	struct mutex                    mutex;

	unsigned int                    irq_num_scl;
	unsigned int                    irq_num_dwa;
	unsigned int                    irq_num_isp;

	struct clk                      *clk_sys[3];
	struct clk                      *clk_sc_top;
	u32                             clk_sys1_freq;

	struct cvi_dwa_vdev              dwa_vdev;
	struct cvi_img_vdev              img_vdev[CVI_VIP_IMG_MAX];
	struct cvi_sc_vdev               sc_vdev[CVI_VIP_SC_MAX];
	struct cvi_disp_vdev             disp_vdev;
	struct cvi_isp_vdev              isp_vdev;

	bool                            disp_online;
};


struct cvi_sc_buf2 {
	struct cvi_vip_buffer2 buf;
	struct list_head       list;
};

const struct cvi_vip_fmt *cvi_vip_get_format(u32 pixelformat);
int cvi_vip_enum_fmt_vid(struct file *file, void *priv, struct v4l2_fmtdesc *f);
int cvi_vip_try_fmt_vid_mplane(struct v4l2_format *f, u8 align);
void cvi_vip_buf_queue(struct cvi_base_vdev *vdev, struct cvi_vip_buffer *b);
struct cvi_vip_buffer *cvi_vip_next_buf(struct cvi_base_vdev *vdev);
struct cvi_vip_buffer *cvi_vip_buf_remove(struct cvi_base_vdev *vdev);
void cvi_vip_buf_cancel(struct cvi_base_vdev *vdev);
int cvi_vip_try_schedule(struct cvi_img_vdev *idev, u8 grp_id, bool is_online);
void cvi_vip_job_finish(struct cvi_img_vdev *idev);
bool cvi_vip_job_is_queued(struct cvi_img_vdev *idev);
int cvi_vip_set_rgn_cfg(const u8 inst, const struct cvi_rgn_cfg *cfg, const struct sclr_size *size);

char *v4l2_fourcc2s(u32 fourcc, char *buf);

extern bool __clk_is_enabled(struct clk *clk);
extern int debug;

#ifdef __cplusplus
}
#endif

#endif /* __CVI_VIP_CORE_H__ */

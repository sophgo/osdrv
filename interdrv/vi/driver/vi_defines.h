#ifndef __VI_DEFINES_H__
#define __VI_DEFINES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal_types.h"
#include "module/osal_clk.h"
#include "module/osal_workqueue.h"
#include "defines.h"
#include "vbq.h"
#include "vi_uapi.h"
#include "vi_snsr.h"
#include "isp/vi_tun_cfg.h"
#include "isp/vi_isp.h"
#include "vi_drv.h"
#include "vi_isp_buf_ctrl.h"
#include "base_ctx.h"

enum E_VI_TH {
	E_VI_TH_PRERAW,
	E_VI_TH_ERR_HANDLER,
	E_VI_TH_EVENT_HANDLER,
	E_VI_TH_VBLANK_HANDLER,
	E_VI_TH_POSTRAW,
	E_VI_TH_AI_ISP,
	E_VI_TH_MAX
};

enum E_AI_WAKE_TYPE {
	E_AI_WAKE_TYPE_NONE,
	E_AI_WAKE_TYPE_MW,
	E_AI_WAKE_TYPE_AI_ISP_TH,
};

enum E_STATE_S {
	E_STATE_DEFAULT,
	E_STATE_SUSPEND,
	E_STATE_RESUME,
	E_STATE_MAX
};
struct vi_thread_attr {
	char			th_name[32];
	osal_task		*w_thread;
	osal_atomic		thread_exit;
	osal_wait		wq;
	osal_atomic		flag;
	osal_atomic		exit_flag;
	osal_tasklet 	tasklet;
	int			(*th_handler)(void *arg);
};

struct vi_event_k {
	struct vi_event		ev;
	struct osal_list_head	list;
};

struct isp_event_q {
	struct osal_list_head	list;
	osal_spinlock		lock;
	u8			count;
};

struct isp_mbus_framefmt {
	__u32	width;
	__u32	height;
	__u32	code;
};

struct isp_rect {
	__s32	left;
	__s32	top;
	__u32	width;
	__u32	height;
};

struct raw_dump_work {
	osal_workqueue worker;
	struct isp_queue raw_dump_vb_q;
};

struct raw_dump_s {
	struct isp_buffer		*isp_byr[ISP_FE_CHN_MAX];
	struct isp_queue		buf_q[ISP_FE_CHN_MAX];
	struct isp_queue		buf_dq[ISP_FE_CHN_MAX];

	osal_atomic			raw_dump_en[ISP_FE_CHN_MAX];
	osal_atomic			isp_smooth_raw_dump_en;
};

struct stream_state_s {
	osal_atomic			csi_streamon[ISP_PRERAW_MAX];
	osal_atomic			isp_streamon[VI_MAX_PIPE_NUM];
	osal_atomic			isp_init;
};

struct _isp_raw_num_n {
	enum sop_isp_raw raw_num;
	struct osal_list_head list;
};

struct isp_sof_raw_num_q {
	struct osal_list_head	list;
	osal_spinlock		lock;
	u8			count;
};

struct _isp_dqbuf_n {
	u8		pipe_id; // vi raw_num
	u8		chn_id; // vi_out buf_chn
	u32		frm_num;
	osal_timeval	tv;
	struct osal_list_head list;
};
struct _isp_snr_i2c_node {
	struct snsr_regs_s n;
	struct osal_list_head list;
};

struct _isp_crop_node {
	struct snsr_isp_s n;
	struct osal_list_head list;
};

struct isp_snr_queue {
	struct osal_list_head	list;
	u32			num_rdy;
};

struct isp_snr_cfg {
	struct isp_snr_queue i2c_queue;
	struct isp_snr_queue crop_queue;
	osal_spinlock lock;
};
/**
 * struct sop_vi - VI IP abstraction
 */
struct vi_dev {
	void				*reg_base;

	int				irq_num;
	osal_clk			*clk_isp[1];
	osal_clk			*clk_mac[3];
	void				*shared_mem;

	struct overflow_info		*overflow_info;
	struct isp_ctx			ctx;
	osal_timer			usr_pic_timer;
	struct isp_mbus_framefmt	usr_fmt;
	struct isp_rect			usr_crop;

	u8				gamma_tbl_idx;
	u8				timeout_cnt;
	u64				usr_pic_phy_addr[ISP_RAW_PATH_MAX];
	unsigned long			usr_pic_delay;
	enum sop_isp_source		isp_source;

	struct raw_dump_s		raw_dump[ISP_PRERAW_MAX];
	struct raw_dump_work		raw_dump_work;

	osal_atomic			isp_err_times[ISP_PRERAW_MAX];
	osal_atomic			isp_int_flag[ISP_PRERAW_MAX];
	osal_atomic			isp_ai_int_flag[ISP_PRERAW_MAX];
	osal_wait			isp_int_wait_q[ISP_PRERAW_MAX];
	osal_wait			isp_ai_wait_q[ISP_PRERAW_MAX];
	osal_wait			isp_event_wait_q;
	osal_wait			isp_dbg_wait_q;
	osal_atomic			isp_dbg_flag;
	osal_atomic			isp_err_handle_flag;

	struct isp_queue		pre_fe_out_q[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	struct isp_queue		pre_ai_isp_in_q[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	struct isp_queue		pre_ai_isp_out_q[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	struct isp_queue		postraw_wdr_in_q[ISP_RAW_PATH_MAX];
	struct isp_queue		postraw_in_q;

	struct mlv_i_s			mlv_i[VI_MAX_PIPE_NUM][MSP_BUF_MAX];

	struct vb_jobs_t		vi_jobs[VI_MAX_PIPE_NUM][VI_MAX_CHN_NUM];

	struct isp_snr_cfg		isp_snr_cfg[ISP_PRERAW_MAX];

	struct isp_buf_q		qbuf_q[VI_MAX_PIPE_NUM][VI_MAX_CHN_NUM];
	struct isp_buf_q		dqbuf_q;
	struct isp_sof_raw_num_q	pre_raw_num_q;

	vb_blk				(*vi_dqbuf)(mmf_chn_s mmf_chn, void *data);
	int				(*vi_qbuf)(mmf_chn_s mmf_chn, void *data);
	u32				pre_fe_sof_cnt[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	u32				pre_fe_frm_num[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	u32				postraw_frame_number[VI_MAX_PIPE_NUM];

	u32				drop_frame_number[ISP_PRERAW_MAX];
	u32				dump_frame_number[ISP_PRERAW_MAX];

	struct vi_ai_isp_info		ai_isp_info[VI_MAX_PIPE_NUM];
	struct gdc_mesh			mesh[VI_MAX_PIPE_NUM][VI_MAX_CHN_NUM];

	struct isp_event_q		event_q;

	struct stream_state_s		stream;

	osal_atomic			pre_fe_state[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	osal_atomic			postraw_state;
	osal_atomic			state;

	osal_atomic			is_drop;

	struct vi_thread_attr		vi_th[E_VI_TH_MAX];
};

#ifdef __cplusplus
}
#endif

#endif /* __VI_DEFINES_H__ */

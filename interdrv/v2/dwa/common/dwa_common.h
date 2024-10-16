#ifndef _DWA_COMMON_H_
#define _DWA_COMMON_H_

#include "linux/dwa_uapi.h"
#include "linux/comm_video.h"
#include "linux/comm_vb.h"
#include "base_ctx.h"

#define VIP_ALIGNMENT 0x40
#define DWA_ALIGNMENT 0x20

#define DWA_USE_WORKQUEUE 1
#define DWA_USE_THREADED_IRQ 0

#undef BIT
#undef BITMASK
#define BIT(nr)               (1U << (nr))
#define BITMASK(msb, lsb)     ((2U << (msb))-(1U << (lsb)))
#undef MIN
#define MIN(a, b) (((a) < (b))?(a):(b))
#undef MAX
#define MAX(a, b) (((a) > (b))?(a):(b))
#undef UPPER
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#undef VIP_ALIGN
#undef VIP_64_ALIGN
#define VIP_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define VIP_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment

#define DWA_DEV_MAX_CNT (2)
#define VIP_MAX_PLANES (3)
#define END_JOB_MAX_LEN (16)

#define DWA_IP_NUM                 (2)
#define DWA_PROC_JOB_INFO_NUM      (16)
#define DWA_MAX_JOB_NUM            (64)
#define DWA_JOB_MAX_TSK_NUM        (16)

#ifndef FPGA_PORTING
#define DWA_IDLE_WAIT_TIMEOUT_MS    1000
#define DWA_SYNC_IO_WAIT_TIMEOUT_MS 1000*2
#define DWA_EOF_WAIT_TIMEOUT_MS     200
#else
#define DWA_IDLE_WAIT_TIMEOUT_MS    1000*5
#define DWA_SYNC_IO_WAIT_TIMEOUT_MS 1000*10
#define DWA_EOF_WAIT_TIMEOUT_MS     1000*1
#endif

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

/***************** dwa callback param *************************/
typedef void (*dwa_cb)(void *, vb_blk);
typedef void (*dwa_tsk_cb)(void *data, int top_id);// tsk callback

enum dwa_usage {
	DWA_USAGE_ROTATION,
	DWA_USAGE_FISHEYE,
	DWA_USAGE_LDC,
	DWA_USAGE_MAX
};

enum dwa_task_type {
	DWA_TASK_TYPE_ROT = 0,
	DWA_TASK_TYPE_FISHEYE,
	DWA_TASK_TYPE_AFFINE,
	DWA_TASK_TYPE_LDC,
	DWA_TASK_TYPE_WARP,
	DWA_TASK_TYPE_MAX,
};

enum dwa_task_state {
	DWA_TASK_STATE_WAIT = 0,
	DWA_TASK_STATE_RUNNING,
	DWA_TASK_STATE_DONE,
	DWA_TASK_STATE_CANCLE,
	DWA_TASK_STATE_FAIL,
	DWA_TASK_STATE_MAX,
};

struct dwa_task {
	struct list_head node;// add to dwa_job task_list
	struct dwa_task_attr attr;
	unsigned long long mesh_id_addr;
	unsigned int bgcolor;
	unsigned int bdcolor;
	enum dwa_task_type type;
	rotation_e rotation;
	atomic_t state;//dwa_task_state
	int tsk_id;
	//char coreid;
};

struct dwa_data {
	unsigned int bytesperline[VIP_MAX_PLANES];
	unsigned int sizeimage[VIP_MAX_PLANES];
	unsigned short w;
	unsigned short h;
};

enum dwa_job_state {
	DWA_JOB_CREAT = 0,
	DWA_JOB_WAIT,
	DWA_JOB_WORKING,
	DWA_JOB_END,
	DWA_JOB_CANCLE,
	DWA_JOB_FAIL,
	DWA_JOB_INVALID,
};

struct dwa_job {
	spinlock_t lock;
	struct dwa_data cap_data, out_data;
	atomic_t job_state; //dwa_job_state
	struct list_head node;//add to dwa_vdev job_list
	struct list_head task_list;
	atomic_t task_num;
	gdc_identity_attr_s identity;
	bool use_cmdq;
	char coreid;
	wait_queue_head_t job_done_wq;
	bool job_done_evt;
	int proc_idx;
};

struct dwa_vb_doneq {
	struct semaphore sem;
	struct list_head doneq;
	spinlock_t lock;
};

struct dwa_vb_done {
	struct list_head node;//add to struct dwa_vb_doneq doneq
	video_frame_info_s img_out;
	struct dwa_job job;
};

struct mesh_dwa_cfg {
	enum dwa_usage usage;
	const void *usage_param;
	struct vb_s *vb_in;
	pixel_format_e pix_format;
	unsigned long long mesh_addr;
	unsigned char sync_io;
	void *cb_param;
	unsigned int cb_param_size;
	rotation_e rotation;
};

struct dwa_op_done_cfg {
	void *param;
	vb_blk blk;
};

enum DWA_CB_CMD {
	DWA_CB_MESH_GDC_OP,
	DWA_CB_DWA_OP_DONE = 100,	/* Skip VI/VPSS/VO self cmd */
	DWA_CB_MAX
};

/********************** proc status for dwa ***********************/
struct dwa_proc_tsk_status {
	unsigned int success;   //total doing success num
	unsigned int fail;      //total doing fail num
	unsigned int cancel;    //total doing cancel num
	unsigned int begin_num;   //total commit num
	unsigned int procing_num; //total hw start ProcingNum num
	unsigned int wait_num;    //waiting num
};

struct dwa_proc_tsk_info {
	unsigned int in_size;
	unsigned int out_size;
	enum dwa_task_type type;
	enum dwa_task_state state;
	unsigned char top_id;
	unsigned int hw_time; // HW cost time
	unsigned long long hw_start_time; // us
};

struct dwa_proc_job_status {
	unsigned int success;    //total doing success num
	unsigned int fail;       //total doing fail num
	unsigned int cancel;     //total doing cancel num
	unsigned int begin_num;   //total commit num
	unsigned int procing_num; //total hw start ProcingNum num
	unsigned int wait_num;    //job_list waiting num
};

struct dwa_proc_job_info {
	long long handle;
	gdc_identity_attr_s identity;
	unsigned int task_num; // number of tasks per job
	struct dwa_proc_tsk_info tsk_info[DWA_JOB_MAX_TSK_NUM];
	enum dwa_job_state state; // job state
	unsigned int cost_time; // From job submitted to job done
	unsigned int hw_time; // HW cost time
	unsigned int busy_time; // From job submitted to job commit to driver
	unsigned long long submit_time; // us
};

struct dwa_proc_operation_status {
	unsigned int add_task_suc;
	unsigned int add_task_fail;
	unsigned int end_job_suc;
	unsigned int end_job_fail;
	unsigned int cb_cnt;
};

struct dwa_proc_ctx {
	struct dwa_proc_job_info job_info[DWA_PROC_JOB_INFO_NUM];
	unsigned short job_idx; // latest job submitted
	struct dwa_proc_job_status job_status;
	struct dwa_proc_tsk_status tsk_status;
	struct dwa_proc_operation_status fisheye_status;
	spinlock_t lock;
};

#endif /* _DWA_COMMON_H_ */

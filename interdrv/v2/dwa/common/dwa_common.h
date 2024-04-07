#ifndef _DWA_COMMON_H_
#define _DWA_COMMON_H_

#include "linux/dwa_uapi.h"
#include "linux/cvi_comm_video.h"
#include "linux/cvi_comm_vb.h"
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

#define DWA_IP_NUM                 (2)
#define DWA_PROC_JOB_INFO_NUM      (16)
#define DWA_MAX_JOB_NUM            (64)
#define DWA_JOB_MAX_TSK_NUM        (16)

#ifndef FPGA_PORTING
#define DWA_IDLE_WAIT_TIMEOUT_MS    1000*5
#define DWA_SYNC_IO_WAIT_TIMEOUT_MS 1000*10
#define DWA_EOF_WAIT_TIMEOUT_MS     1000*1
#else
#define DWA_IDLE_WAIT_TIMEOUT_MS    1000*50
#define DWA_SYNC_IO_WAIT_TIMEOUT_MS 1000*100
#define DWA_EOF_WAIT_TIMEOUT_MS     1000*10
#endif

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

/***************** dwa callback param *************************/
typedef void (*dwa_cb)(void *, VB_BLK);
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
	struct list_head node;// add to cvi_dwa_job task_list
	struct dwa_task_attr attr;
	uint64_t mesh_id_addr;
	uint32_t bgcolor;
	uint32_t bdcolor;
	enum dwa_task_type type;
	ROTATION_E enRotation;
	atomic_t state;//dwa_task_state
	dwa_tsk_cb pfnTskCB;
	int tsk_id;
	struct semaphore sem;
	//int8_t coreid;
};

struct dwa_data {
	uint32_t bytesperline[VIP_MAX_PLANES];
	uint32_t sizeimage[VIP_MAX_PLANES];
	uint16_t w;
	uint16_t h;
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
	struct dwa_data cap_data, out_data;
	atomic_t enJobState; //dwa_job_state
	struct list_head node;//add to cvi_dwa_vdev job_list
	struct list_head task_list;
	atomic_t task_num;
	GDC_IDENTITY_ATTR_S identity;
	bool use_cmdq;
	int8_t coreid;
	wait_queue_head_t job_done_wq;
	bool job_done_evt;
};

struct dwa_vb_doneq {
	struct semaphore sem;
	struct list_head doneq;
};

struct dwa_vb_done {
	struct list_head node;//add to struct dwa_vb_doneq doneq
	VIDEO_FRAME_INFO_S stImgOut;
	struct dwa_job job;
};

struct mesh_dwa_cfg {
	enum dwa_usage usage;
	const void *pUsageParam;
	struct vb_s *vb_in;
	PIXEL_FORMAT_E enPixFormat;
	u64 mesh_addr;
	u8 sync_io;
	void *pcbParam;
	u32 cbParamSize;
	ROTATION_E enRotation;
};

struct dwa_op_done_cfg {
	void *pParam;
	VB_BLK blk;
};

enum DWA_CB_CMD {
	DWA_CB_MESH_GDC_OP,
	DWA_CB_DWA_OP_DONE = 100,	/* Skip VI/VPSS/VO self cmd */
	DWA_CB_MAX
};

/********************** proc status for dwa ***********************/
struct dwa_proc_tsk_status {
	uint32_t u32Success;   //total doing success num
	uint32_t u32Fail;      //total doing fail num
	uint32_t u32Cancel;    //total doing u32Cancel num
	uint32_t u32BeginNum;   //total commit num
	uint32_t u32ProcingNum; //total hw start ProcingNum num
	uint32_t u32waitNum;    //waiting num
};

struct dwa_proc_tsk_info {
	uint32_t u32InSize;
	uint32_t u32OutSize;
	enum dwa_task_type type;
	enum dwa_task_state state;
	u8 top_id;
	uint32_t u32HwTime; // HW cost time
	uint64_t u64HwStartTime; // us
};

struct dwa_proc_job_status {
	uint32_t u32Success;    //total doing success num
	uint32_t u32Fail;       //total doing fail num
	uint32_t u32Cancel;     //total doing cancel num
	uint32_t u32BeginNum;   //total commit num
	uint32_t u32ProcingNum; //total hw start ProcingNum num
	uint32_t u32waitNum;    //job_list waiting num
};

struct dwa_proc_job_info {
	int64_t hHandle;
	GDC_IDENTITY_ATTR_S identity;
	uint32_t u32TaskNum; // number of tasks per job
	struct dwa_proc_tsk_info tsk_info[DWA_JOB_MAX_TSK_NUM];
	enum dwa_job_state eState; // job state
	uint32_t u32CostTime; // From job submitted to job done
	uint32_t u32HwTime; // HW cost time
	uint32_t u32BusyTime; // From job submitted to job commit to driver
	uint64_t u64SubmitTime; // us
};

struct dwa_proc_operation_status {
	uint32_t u32AddTaskSuc;
	uint32_t u32AddTaskFail;
	uint32_t u32EndJobSuc;
	uint32_t u32EndJobFail;
	uint32_t u32CbCnt;
};

struct dwa_proc_ctx {
	struct dwa_proc_job_info stJobInfo[DWA_PROC_JOB_INFO_NUM];
	uint16_t u16JobIdx; // latest job submitted
	struct dwa_proc_job_status stJobStatus;
	struct dwa_proc_tsk_status stTskstatus;
	struct dwa_proc_operation_status stFishEyeStatus;
	spinlock_t lock;
};

#endif /* _DWA_COMMON_H_ */

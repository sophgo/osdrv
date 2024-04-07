#ifndef _LDC_COMMON_H_
#define _LDC_COMMON_H_

#include "linux/ldc_uapi.h"
#include "linux/cvi_comm_video.h"
#include "linux/cvi_comm_vb.h"
#include "base_ctx.h"

#define VIP_ALIGNMENT 0x40
#define LDC_ALIGNMENT (VIP_ALIGNMENT)

#define LDC_USE_WORKQUEUE 1
#define LDC_USE_THREADED_IRQ 0

#define LDC_YUV_BLACK 0x808000
#define LDC_RGB_BLACK 0x0

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

#define LDC_DEV_MAX_CNT (2)
#define VIP_MAX_PLANES (3)

#define LDC_IP_NUM                 (2)
#define LDC_PROC_JOB_INFO_NUM      (16)
#define LDC_MAX_JOB_NUM            (64)
#define LDC_JOB_MAX_TSK_NUM        (16)

#ifndef FPGA_PORTING
#define LDC_IDLE_WAIT_TIMEOUT_MS    1000*5
#define LDC_SYNC_IO_WAIT_TIMEOUT_MS 1000*10
#define LDC_EOF_WAIT_TIMEOUT_MS     1000*1
#else
#define LDC_IDLE_WAIT_TIMEOUT_MS    1000*50
#define LDC_SYNC_IO_WAIT_TIMEOUT_MS 1000*100
#define LDC_EOF_WAIT_TIMEOUT_MS     1000*10
#endif

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

/***************** ldc callback param *************************/
typedef void (*ldc_cb)(void *, VB_BLK);
typedef void (*ldc_tsk_cb)(void *data, int top_id);// tsk callback

enum ldc_usage {
	LDC_USAGE_ROTATION,
	LDC_USAGE_FISHEYE,
	LDC_USAGE_LDC,
	LDC_USAGE_MAX
};

enum ldc_task_type {
	LDC_TASK_TYPE_ROT = 0,
	LDC_TASK_TYPE_FISHEYE,
	LDC_TASK_TYPE_AFFINE,
	LDC_TASK_TYPE_LDC,
	LDC_TASK_TYPE_MAX,
};

enum ldc_task_state {
	LDC_TASK_STATE_WAIT = 0,
	LDC_TASK_STATE_RUNNING,
	LDC_TASK_STATE_DONE,
	LDC_TASK_STATE_CANCLE,
	LDC_TASK_STATE_FAIL,
	LDC_TASK_STATE_MAX,
};

struct ldc_task {
	struct list_head node;// add to cvi_ldc_job task_list
	struct gdc_task_attr attr;
	uint64_t mesh_id_addr;
	uint32_t bgcolor;
	uint32_t bdcolor;
	enum ldc_task_type type;
	ROTATION_E enRotation;
	atomic_t state;//ldc_task_state
	ldc_tsk_cb pfnTskCB;
	int tsk_id;
	struct semaphore sem;
	//int8_t coreid;
};

struct ldc_data {
	uint32_t bytesperline[VIP_MAX_PLANES];
	uint32_t sizeimage[VIP_MAX_PLANES];
	uint16_t w;
	uint16_t h;
};

enum ldc_job_state {
	LDC_JOB_CREAT = 0,
	LDC_JOB_WAIT,
	LDC_JOB_WORKING,
	LDC_JOB_END,
	LDC_JOB_CANCLE,
	LDC_JOB_FAIL,
	LDC_JOB_INVALID,
};

struct ldc_job {
	struct ldc_data cap_data, out_data;
	atomic_t enJobState; //ldc_job_state
	struct list_head node;//add to cvi_ldc_vdev job_list
	struct list_head task_list;
	atomic_t task_num;
	GDC_IDENTITY_ATTR_S identity;
	bool use_cmdq;
	int8_t coreid;
	wait_queue_head_t job_done_wq;
	bool job_done_evt;
};

struct ldc_vb_doneq {
	struct semaphore sem;
	struct list_head doneq;
};

struct ldc_vb_done {
	struct list_head node;//add to struct ldc_vb_doneq doneq
	VIDEO_FRAME_INFO_S stImgOut;
	struct ldc_job job;
};

struct mesh_ldc_cfg {
	enum ldc_usage usage;
	const void *pUsageParam;
	struct vb_s *vb_in;
	PIXEL_FORMAT_E enPixFormat;
	u64 mesh_addr;
	u8 sync_io;
	void *pcbParam;
	u32 cbParamSize;
	ROTATION_E enRotation;
};

/********************** proc status for ldc ***********************/
struct ldc_proc_tsk_status {
	uint32_t u32Success;   //total doing success num
	uint32_t u32Fail;      //total doing fail num
	uint32_t u32Cancel;    //total doing u32Cancel num
	uint32_t u32BeginNum;   //total commit num
	uint32_t u32ProcingNum; //total hw start ProcingNum num
	uint32_t u32waitNum;    //waiting num
};

struct ldc_proc_tsk_info {
	uint32_t u32InSize;
	uint32_t u32OutSize;
	enum ldc_task_type type;
	enum ldc_task_state state;
	u8 top_id;
	uint32_t u32HwTime; // HW cost time
	uint64_t u64HwStartTime; // us
};

struct ldc_proc_job_status {
	uint32_t u32Success;    //total doing success num
	uint32_t u32Fail;       //total doing fail num
	uint32_t u32Cancel;     //total doing cancel num
	uint32_t u32BeginNum;   //total commit num
	uint32_t u32ProcingNum; //total hw start ProcingNum num
	uint32_t u32waitNum;    //job_list waiting num
};

struct ldc_proc_job_info {
	int64_t hHandle;
	GDC_IDENTITY_ATTR_S identity;
	uint32_t u32TaskNum; // number of tasks per job
	struct ldc_proc_tsk_info tsk_info[LDC_JOB_MAX_TSK_NUM];
	enum ldc_job_state eState; // job state
	uint32_t u32CostTime; // From job submitted to job done
	uint32_t u32HwTime; // HW cost time
	uint32_t u32BusyTime; // From job submitted to job commit to driver
	uint64_t u64SubmitTime; // us
};

struct ldc_proc_operation_status {
	uint32_t u32AddTaskSuc;
	uint32_t u32AddTaskFail;
	uint32_t u32EndJobSuc;
	uint32_t u32EndJobFail;
	uint32_t u32CbCnt;
};

struct ldc_proc_ctx {
	struct ldc_proc_job_info stJobInfo[LDC_PROC_JOB_INFO_NUM];
	uint16_t u16JobIdx; // latest job submitted
	struct ldc_proc_job_status stJobStatus;
	struct ldc_proc_tsk_status stTskstatus;
	struct ldc_proc_operation_status stFishEyeStatus;
	spinlock_t lock;
};

#endif /* _LDC_COMMON_H_ */

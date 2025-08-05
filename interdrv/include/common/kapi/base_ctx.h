#ifndef __BASE_CTX_H__
#define __BASE_CTX_H__

#include "common.h"
#include "base_uapi.h"
#include "queue.h"
#include "osal.h"

#define MOTION_MASTER_CHN           0
#define MOTION_SLAVE_CHN            1

#define GDC_SHARE_MEM_SIZE          (0x8000)

#define NUM_OF_PLANES               3

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

#define MM_THREAD_PRIO 95

#define CHN_MATCH(x, y) (((x)->mod_id == (y)->mod_id) && ((x)->dev_id == (y)->dev_id)             \
		&& ((x)->chn_id == (y)->chn_id))

#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		int front, tail, capacity;				\
	}

#define FIFO_INIT(head, _capacity) do {						\
		if (_capacity > 0)						\
			(head)->fifo = osal_vmalloc(sizeof(*(head)->fifo) * _capacity);	\
		(head)->front = (head)->tail = -1;				\
		(head)->capacity = _capacity;					\
	} while (0)

#define FIFO_EXIT(head) do {						\
		(head)->front = (head)->tail = -1;			\
		(head)->capacity = 0; 					\
		if ((head)->fifo) 					\
			osal_vfree((head)->fifo);				\
		(head)->fifo = NULL;					\
	} while (0)

#define FIFO_EMPTY(head)    ((head)->front == -1)

#define FIFO_FULL(head)     (((head)->front == ((head)->tail + 1))	\
			|| (((head)->front == 0) && ((head)->tail == ((head)->capacity - 1))))

#define FIFO_CAPACITY(head) ((head)->capacity)

#define FIFO_SIZE(head)     (FIFO_EMPTY(head) ?\
		0 : ((((head)->tail + (head)->capacity - (head)->front) % (head)->capacity) + 1))

#define FIFO_PUSH(head, elm) do {						\
		if (FIFO_EMPTY(head))						\
			(head)->front = (head)->tail = 0;			\
		else								\
			(head)->tail = ((head)->tail == (head)->capacity - 1)	\
					? 0 : (head)->tail + 1;			\
		(head)->fifo[(head)->tail] = elm;				\
	} while (0)

#define FIFO_POP(head, pelm) do {						\
		*(pelm) = (head)->fifo[(head)->front];				\
		if ((head)->front == (head)->tail)				\
			(head)->front = (head)->tail = -1;			\
		else								\
			(head)->front = ((head)->front == (head)->capacity - 1)	\
					? 0 : (head)->front + 1;		\
	} while (0)

#define FIFO_FOREACH(var, head, idx)					\
	for (idx = (head)->front, var = (head)->fifo[idx];		\
		idx < (head)->front + FIFO_SIZE(head);			\
		idx = idx + 1, var = (head)->fifo[idx % (head)->capacity])

#define FIFO_GET_FRONT(head, pelm) (*(pelm) = (head)->fifo[(head)->front])

#define FIFO_GET_TAIL(head, pelm) (*(pelm) = (head)->fifo[(head)->tail])

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
		(var) && ((tvar) = TAILQ_NEXT((var), field), 1);	\
		(var) = (tvar))
#endif

#define CHECK_IOCTL_CMD(cmd, type) \
	if (_IOC_SIZE(cmd) != sizeof(type)) { \
		osal_printk("data size error!\n"); \
		return OSAL_EINVAL; \
	}

#define MO_TBL_SIZE 256


#define DEFAULT_MESH_PADDR	0x80000000
#define GDC_SUPPORT_FMT(fmt)                                                   \
	((fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||           \
	 (fmt == PIXEL_FORMAT_YUV_400))

enum MSP_IDX {
	MSP_MASTER_CHN,
	MSP_SLAVE_CHN,
	MSP_CHN_MAX,
};

struct vip_point {
	u16 x;
	u16 y;
};

struct vip_line {
	u16 start;
	u16 end;
};

struct vip_range {
	u16 start_x;
	u16 start_y;
	u16 end_x;
	u16 end_y;
};

struct vip_frmsize {
	u32 width;
	u32 height;
};

struct msp_chn_info {
	u32 width;
	u32 height;
	u64 phy_addr;
	u32 len;
	u8  chn;
};

struct mlv_i_s {
	u8 motion_lv;
	u16 dci_lv;
	u64 frm_num;

	struct msp_chn_info msp[MSP_CHN_MAX];
};

struct mod_ctx_s {
	mod_id_e modid;
	u8 ctx_num;
	void *ctx_info;
};

enum chn_type_e {
	CHN_TYPE_IN = 0,
	CHN_TYPE_OUT,
	CHN_TYPE_MAX
};

// start point is included.
// end point is excluded.
struct crop_size {
	uint16_t  start_x;
	uint16_t  start_y;
	uint16_t  end_x;
	uint16_t  end_y;
};

struct gdc_mesh {
	u64 paddr;
	void *vaddr;
	u32 meshSize;
	osal_mutex lock;
	osal_atomic gdc_flag;
};

enum gdc_job_state {
	GDC_JOB_SUCCESS = 0,
	GDC_JOB_FAIL,
	GDC_JOB_WORKING,
};

struct gdc_job_info {
	int64_t handle;
	mod_id_e mod_id; // the module submitted gdc job
	uint32_t task_num; // number of tasks
	enum gdc_job_state state; // job state
	uint32_t in_size;
	uint32_t out_size;
	uint32_t cost_time; // from job submitted to job done
	uint32_t hw_time; // hw cost time
	uint32_t busy_time; // from job submitted to job commit to driver
	uint64_t submit_time; // us
};

struct gdc_job_status {
	uint32_t success;
	uint32_t fail;
	uint32_t cancel;
	uint32_t begin_num;
	uint32_t busy_num;
	uint32_t procing_num;
};

struct gdc_task_status {
	uint32_t success;
	uint32_t fail;
	uint32_t cancel;
	uint32_t busy_num;
};

struct gdc_operation_status {
	uint32_t add_task_suc;
	uint32_t add_task_fail;
	uint32_t end_suc;
	uint32_t end_fail;
	uint32_t cb_cnt;
};

struct gdc_proc_ctx {
	struct gdc_job_info job_info[GDC_PROC_JOB_INFO_NUM];
	uint16_t job_info_idx; // latest job submitted
	struct gdc_job_status job_status;
	struct gdc_task_status task_status;
	struct gdc_operation_status fisheye_status;
};

struct csc_cfg {
	u16 coef[3][3];
	u8 sub[3];
	u8 add[3];
};

enum vip_input_type {
	VIP_INPUT_ISP = 0,
	VIP_INPUT_LDC,
	VIP_INPUT_SHARE,
	VIP_INPUT_MEM,
	VIP_INPUT_ISP_POST,
	VIP_INPUT_MAX,
};

struct rgn_canvas_ctx {
	u64 phy_addr;
	u8 *virt_addr;
	u32 len;
};

struct rgn_canvas_q {
	struct rgn_canvas_ctx **fifo;
	int  front, tail, capacity;
};

enum vip_pattern {
	VIP_PAT_OFF = 0,
	VIP_PAT_SNOW,
	VIP_PAT_AUTO,
	VIP_PAT_RED,
	VIP_PAT_GREEN,
	VIP_PAT_BLUE,
	VIP_PAT_COLORBAR,
	VIP_PAT_GRAY_GRAD_H,
	VIP_PAT_GRAY_GRAD_V,
	VIP_PAT_BLACK,
	VIP_PAT_MAX,
};

struct rgn_ex_cfg {
	struct rgn_param rgn_ex_param[16];
	struct rgn_odec odec;
	u8 num_of_rgn_ex;
	u8 hscale_x2;
	u8 vscale_x2;
	u8 colorkey_en;
	u32 colorkey;
};

struct vip_plane {
	u32 length;
	u64 addr;
};

struct venc_reg_info {
	u8 enable;
	u32 reg_00;
	u32 reg_08;
	u32 reg_88;
	u32 reg_90;
	u32 reg_94;
};

struct vpss_reg_info {
	u8 enable;
	u32 latched_line_cnt;
	u32 sc;
	struct {
		u8 isp2ip_y_in[2];
		u8 isp2ip_u_in[2];
		u8 isp2ip_v_in[2];
		u8 img_d_out[2];
		u8 img_v_out[2];
		u8 bld_sa[2];
		u8 bld_sb[2];
		u8 bld_m[2];
		u8 pri_sp[2];
		u8 pri_m[2];
		u8 sc_d[2];
		u8 sc_v1[2];
		u8 sc_v2[2];
		u8 sc_v3[2];
		u8 sc_d_out[2];
	} sc_top;
	struct {
		u8 sc_odma_axi_cmd_cs[4];
		u8 sc_odma_v_buf_empty;
		u8 sc_odma_v_buf_full;
		u8 sc_odma_u_buf_empty;
		u8 sc_odma_u_buf_full;
		u8 sc_odma_y_buf_empty;
		u8 sc_odma_y_buf_full;
		u8 sc_odma_axi_v_active;
		u8 sc_odma_axi_u_active;
		u8 sc_odma_axi_y_active;
		u8 sc_odma_axi_active;
		u8 reg_v_sb_empty;
		u8 reg_v_sb_full;
		u8 reg_u_sb_empty;
		u8 reg_u_sb_full;
		u8 reg_y_sb_empty;
		u8 reg_y_sb_full;
		u8 reg_sb_full;
	} odma;
	struct {
		u8 sb_mode;
		u8 sb_size;
		u8 sb_nb;
		u8 sb_full_nb;
		u8 sb_sw_wptr;
	} sb_ctrl;
	struct {
		u8 u_sb_wptr_ro;
		u8 u_sb_full;
		u8 u_sb_empty;
		u8 u_sb_dptr_ro;
		u8 v_sb_wptr_ro;
		u8 v_sb_full;
		u8 v_sb_empty;
		u8 v_sb_dptr_ro;
		u8 y_sb_wptr_ro;
		u8 y_sb_full;
		u8 y_sb_empty;
		u8 y_sb_dptr_ro;
		u8 sb_full;
	} sb_stat;
};

struct vi_reg_info {
	u8 enable;
	struct {
		u32 blk_idle;
		struct {
			u32 r_0;
			u32 r_4;
			u32 r_8;
			u32 r_c;
		} dbus_sel[7];
	} isp_top;
	struct {
		u32 preraw_info;
		u32 fe_idle_info;
	} preraw_fe;
	struct {
		u32 preraw_be_info;
		u32 be_dma_idle_info;
		u32 ip_idle_info;
		u32 stvalid_status;
		u32 stready_status;
	} preraw_be;
	struct {
		u32 stvalid_status;
		u32 stready_status;
		u32 dma_idle;
	} rawtop;
	struct {
		u32 ip_stvalid_status;
		u32 ip_stready_status;
		u32 dmi_stvalid_status;
		u32 dmi_stready_status;
		u32 xcnt_rpt;
		u32 ycnt_rpt;
	} rgbtop;
	struct {
		u32 debug_state;
		u32 stvalid_status;
		u32 stready_status;
		u32 xcnt_rpt;
		u32 ycnt_rpt;
	} yuvtop;
	struct {
		u32 dbg_sel;
		u32 status;
	} rdma28[2];
};

struct overflow_info {
	struct vi_reg_info vi_info;
	struct vpss_reg_info vpss_info;
	struct venc_reg_info vc_info;
};

#endif  /* __CVI_BASE_CTX_H__ */


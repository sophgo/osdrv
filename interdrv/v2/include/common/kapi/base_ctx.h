#ifndef __BASE_CTX_H__
#define __BASE_CTX_H__

#include <linux/cvi_base_ctx.h>
#include <linux/base_uapi.h>

#include <queue.h>


#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		int front, tail, capacity;				\
	}

#define FIFO_INIT(head, _capacity) do {						\
		if (_capacity > 0)						\
		(head)->fifo = vmalloc(sizeof(*(head)->fifo) * _capacity);	\
		(head)->front = (head)->tail = -1;				\
		(head)->capacity = _capacity;					\
	} while (0)

#define FIFO_EXIT(head) do {						\
		(head)->front = (head)->tail = -1;			\
		(head)->capacity = 0; 					\
		if ((head)->fifo) 					\
			vfree((head)->fifo);				\
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

#define MO_TBL_SIZE 256

struct mlv_i_s {
	u8 mlv_i_level;
	u8 mlv_i_table[MO_TBL_SIZE];
};

struct mod_ctx_s {
	MOD_ID_E modID;
	u8 ctx_num;
	void *ctx_info;
};

struct cvi_vi_info {
	CVI_BOOL enable;
	struct {
		CVI_U32 blk_idle;
		struct {
	       CVI_U32 r_0;
	       CVI_U32 r_4;
	       CVI_U32 r_8;
	       CVI_U32 r_c;
		} dbus_sel[7];
	} isp_top;
	struct {
		CVI_U32 preraw_info;
		CVI_U32 fe_idle_info;
	} preraw_fe;
	struct {
		CVI_U32 preraw_be_info;
		CVI_U32 be_dma_idle_info;
		CVI_U32 ip_idle_info;
		CVI_U32 stvalid_status;
		CVI_U32 stready_status;
	} preraw_be;
	struct {
		CVI_U32 stvalid_status;
		CVI_U32 stready_status;
		CVI_U32 dma_idle;
	} rawtop;
	struct {
		CVI_U32 ip_stvalid_status;
		CVI_U32 ip_stready_status;
		CVI_U32 dmi_stvalid_status;
		CVI_U32 dmi_stready_status;
		CVI_U32 xcnt_rpt;
		CVI_U32 ycnt_rpt;
	} rgbtop;
	struct {
		CVI_U32 debug_state;
		CVI_U32 stvalid_status;
		CVI_U32 stready_status;
		CVI_U32 xcnt_rpt;
		CVI_U32 ycnt_rpt;
	} yuvtop;
	struct {
		CVI_U32 dbg_sel;
		CVI_U32 status;
	} rdma28[2];
};


struct cvi_overflow_info {
	struct cvi_vi_info vi_info;
};

#endif  /* __CVI_BASE_CTX_H__ */

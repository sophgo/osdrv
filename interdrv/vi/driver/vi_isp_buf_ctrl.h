#ifndef __VI_ISP_BUF_CTRL_H__
#define __VI_ISP_BUF_CTRL_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal_list.h"
#include "vi_ip_comm.h"

/*inner buffer */
enum isp_buf_status {
	INTERNAL_BUFFER,
	EXTERNAL_BUFFER,
};

enum isp_buf_source {
	PRE_FE,
	PRE_AI_ISP,
};

struct isp_buffer {
	struct osal_list_head	list;
	uint8_t			is_ext;
	uint8_t			pipe;
	enum sop_isp_raw	raw_num;
	enum sop_isp_fe_chn_num chn_num;
	enum isp_buf_source	source;
	struct vi_rect		crop;
	struct _ai_cfg		ai_cfg;

	u64			vb_blk;
	u64			addr;
	u32			size;
	u32			byr_size;
	u32			frm_num;
	u32			ir_idx;
	osal_timeval		tv;
};

struct isp_queue {
	struct osal_list_head	rdy_queue;
	osal_spinlock		lock;
	u32			num_rdy;
	enum sop_isp_raw	raw_num;
	u8			depth;
};

struct vi2_plane {
	__u64 addr;
};

/*outer buffer */
struct vi2_buffer {
	__u32 index;
	__u32 length;
	struct vi2_plane planes[3];
	__u32 reserved;
};

struct sop_isp_buf {
	struct vi2_buffer buf;
	struct osal_list_head list;
};

struct isp_buf_q {
	struct osal_list_head	list;
	u8			num;
	osal_spinlock		lock;
};

void isp_buf_init(struct isp_queue *q);
void isp_buf_destroy(struct isp_queue *q);
struct isp_buffer *isp_buf_next(struct isp_queue *q);
struct isp_buffer *isp_buf_last(struct isp_queue *q);
void isp_buf_queue(struct isp_queue *q, struct isp_buffer *b);
struct isp_buffer *isp_buf_remove(struct isp_queue *q);
int isp_buf_empty(struct isp_queue *q);
int isp_buf_overflow(struct isp_queue *q);

void sop_isp_rdy_buf_queue(struct isp_buf_q *qbuf_q, struct sop_isp_buf *b);
struct sop_isp_buf *sop_isp_rdy_buf_next(struct isp_buf_q *qbuf_q);
int sop_isp_rdy_buf_empty(struct isp_buf_q *qbuf_q);
void sop_isp_rdy_buf_remove(struct isp_buf_q *qbuf_q);

#ifdef __cplusplus
}
#endif

#endif //__VI_ISP_BUF_CTRL_H__


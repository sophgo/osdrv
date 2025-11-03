#include "vi_isp_buf_ctrl.h"

/* inner buffer queue */
void isp_buf_init(struct isp_queue *q)
{
	OSAL_INIT_LIST_HEAD(&q->rdy_queue);
	osal_spin_lock_init(&q->lock);
	q->num_rdy = 0;
}

void isp_buf_destroy(struct isp_queue *q)
{
	if (q == NULL)
		return;

	osal_spin_lock_destroy(&q->lock);
}

struct isp_buffer *isp_buf_next(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	osal_spin_lock_irqsave(&q->lock, &flags);
	if (!osal_list_empty(&q->rdy_queue))
		b = osal_list_first_entry(&q->rdy_queue, struct isp_buffer, list);
	osal_spin_unlock_irqrestore(&q->lock, &flags);

	return b;
}

struct isp_buffer *isp_buf_last(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	osal_spin_lock_irqsave(&q->lock, &flags);
	if (!osal_list_empty(&q->rdy_queue))
		b = osal_list_last_entry(&q->rdy_queue, struct isp_buffer, list);
	osal_spin_unlock_irqrestore(&q->lock, &flags);

	return b;
}

void isp_buf_queue(struct isp_queue *q, struct isp_buffer *b)
{
	unsigned long flags;

	if (b == NULL)
		return;

	osal_spin_lock_irqsave(&q->lock, &flags);
	osal_list_add_tail(&b->list, &q->rdy_queue);
	++q->num_rdy;
	osal_spin_unlock_irqrestore(&q->lock, &flags);
}

struct isp_buffer *isp_buf_remove(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	osal_spin_lock_irqsave(&q->lock, &flags);
	if (!osal_list_empty(&q->rdy_queue)) {
		b = osal_list_first_entry(&q->rdy_queue, struct isp_buffer, list);
		osal_list_del_init(&b->list);
		--q->num_rdy;
	}
	osal_spin_unlock_irqrestore(&q->lock, &flags);

	return b;
}

int isp_buf_empty(struct isp_queue *q)
{
	unsigned long flags;
	int empty = 0;

	osal_spin_lock_irqsave(&q->lock, &flags);
	if (osal_list_empty(&q->rdy_queue)) {
		empty = 1;
	}
	osal_spin_unlock_irqrestore(&q->lock, &flags);

	return empty;
}

int isp_buf_overflow(struct isp_queue *q)
{
	unsigned long flags;
	int overflow = 0;

	osal_spin_lock_irqsave(&q->lock, &flags);
	if (q->num_rdy > q->depth) {
		overflow = 1;
	}
	osal_spin_unlock_irqrestore(&q->lock, &flags);

	return overflow;
}

/* outer buffer queue */
void sop_isp_rdy_buf_queue(struct isp_buf_q *qbuf_q, struct sop_isp_buf *b)
{
	unsigned long flags;

	osal_spin_lock_irqsave(&qbuf_q->lock, &flags);
	osal_list_add_tail(&b->list, &qbuf_q->list);
	++qbuf_q->num;
	osal_spin_unlock_irqrestore(&qbuf_q->lock, &flags);
}

struct sop_isp_buf *sop_isp_rdy_buf_next(struct isp_buf_q *qbuf_q)
{
	unsigned long flags;
	struct sop_isp_buf *b = NULL;

	osal_spin_lock_irqsave(&qbuf_q->lock, &flags);
	if (!osal_list_empty(&qbuf_q->list)) {
		b = osal_list_first_entry(&qbuf_q->list, struct sop_isp_buf, list);
	}
	osal_spin_unlock_irqrestore(&qbuf_q->lock, &flags);

	return b;
}

int sop_isp_rdy_buf_empty(struct isp_buf_q *qbuf_q)
{
	unsigned long flags;
	int empty = 0;

	osal_spin_lock_irqsave(&qbuf_q->lock, &flags);
	empty = (qbuf_q->num == 0);
	osal_spin_unlock_irqrestore(&qbuf_q->lock, &flags);

	return empty;
}

void sop_isp_rdy_buf_remove(struct isp_buf_q *qbuf_q)
{
	unsigned long flags;
	struct sop_isp_buf *b = NULL;

	osal_spin_lock_irqsave(&qbuf_q->lock, &flags);
	if (!osal_list_empty(&qbuf_q->list)) {
		b = osal_list_first_entry(&qbuf_q->list, struct sop_isp_buf, list);
		osal_list_del_init(&b->list);
		osal_kfree(b);
		qbuf_q->num--;
	}
	osal_spin_unlock_irqrestore(&qbuf_q->lock, &flags);
}

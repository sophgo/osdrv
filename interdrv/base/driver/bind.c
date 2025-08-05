#include "osal.h"
#include "comm_sys.h"
#include "comm_errno.h"
#include "base_uapi.h"
#include "base_debug.h"
#include "queue.h"
#include "base_ctx.h"

struct bind_t {
	TAILQ_ENTRY(bind_t) tailq;
	bind_node_s *node;
};

TAILQ_HEAD(bind_head, bind_t) binds;

static osal_spinlock bind_lock;
bind_node_s bind_nodes[BIND_NODE_MAXNUM];


int32_t bind(mmf_chn_s *src_chn, mmf_chn_s *dest_chn)
{
	struct bind_t *item, *item_tmp;
	int32_t ret = 0, i;
	unsigned long flags;

	TRACE_BASE(DBG_DEBUG, "%s: src(mId=%d, dId=%d, cId=%d), dst(mId=%d, dId=%d, cId=%d)\n",
		__func__,
		src_chn->mod_id, src_chn->dev_id, src_chn->chn_id,
		dest_chn->mod_id, dest_chn->dev_id, dest_chn->chn_id);

	osal_spin_lock_irqsave(&bind_lock, &flags);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, src_chn))
			continue;

		// check if dst already bind to src
		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->dsts.mmf_chn[i], dest_chn)) {
				TRACE_BASE(DBG_DEBUG, "Duplicate Dst(%d-%d-%d) to Src(%d-%d-%d)\n",
					dest_chn->mod_id, dest_chn->dev_id, dest_chn->chn_id,
					src_chn->mod_id, src_chn->dev_id, src_chn->chn_id);
				ret = -1;
				goto BIND_EXIT;
			}
		}
		// check if dsts have enough space for one more bind
		if (item->node->dsts.num >= BIND_DEST_MAXNUM) {
			TRACE_BASE(DBG_ERR, "Over max bind Dst number\n");
			ret = -1;
			goto BIND_EXIT;
		}
		item->node->dsts.mmf_chn[item->node->dsts.num++] = *dest_chn;

		goto BIND_SUCCESS;
	}

	// if src not found
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if (!bind_nodes[i].used) {
			memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
			bind_nodes[i].used = true;
			bind_nodes[i].src = *src_chn;
			bind_nodes[i].dsts.num = 1;
			bind_nodes[i].dsts.mmf_chn[0] = *dest_chn;
			break;
		}
	}

	if (i == BIND_NODE_MAXNUM) {
		TRACE_BASE(DBG_ERR, "No free bind node\n");
		ret = -1;
		goto BIND_EXIT;
	}

	item = osal_kzalloc(sizeof(*item), OSAL_GFP_ATOMIC);
	if (item == NULL) {
		memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
		ret = ERR_SYS_NOMEM;
		goto BIND_EXIT;
	}

	item->node = &bind_nodes[i];
	TAILQ_INSERT_TAIL(&binds, item, tailq);

BIND_SUCCESS:
	ret = 0;
BIND_EXIT:
	osal_spin_unlock_irqrestore(&bind_lock, &flags);

	return ret;
}

int32_t unbind(mmf_chn_s *src_chn, mmf_chn_s *dest_chn)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;
	unsigned long flags;

	osal_spin_lock_irqsave(&bind_lock, &flags);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, src_chn))
			continue;

		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->dsts.mmf_chn[i], dest_chn)) {
				if (--item->node->dsts.num) {
					for (; i < item->node->dsts.num; i++)
						item->node->dsts.mmf_chn[i] = item->node->dsts.mmf_chn[i + 1];
				} else {
					item->node->used = false;
					TAILQ_REMOVE(&binds, item, tailq);
					osal_kfree(item);
				}
				osal_spin_unlock_irqrestore(&bind_lock, &flags);
				return 0;
			}
		}
	}
	osal_spin_unlock_irqrestore(&bind_lock, &flags);
	return 0;
}

int32_t bind_get_dst(mmf_chn_s *src_chn, mmf_bind_dest_s *bind_dest)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;
	unsigned long flags;

	TRACE_BASE(DBG_DEBUG, "%s: src(.mod_id=%d, .dev_id=%d, .chn_id=%d)\n",
		__func__, src_chn->mod_id,
		src_chn->dev_id, src_chn->chn_id);

	osal_spin_lock_irqsave(&bind_lock, &flags);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->src, src_chn)) {
				*bind_dest = item->node->dsts;
				osal_spin_unlock_irqrestore(&bind_lock, &flags);
				return 0;
			}
		}
	}
	osal_spin_unlock_irqrestore(&bind_lock, &flags);
	return -1;
}
osal_module_export(bind_get_dst);

int32_t bind_get_src(mmf_chn_s *dest_chn, mmf_chn_s *src_chn)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;
	unsigned long flags;

	TRACE_BASE(DBG_DEBUG, "%s: dst(.mod_id=%d, .dev_id=%d, .chn_id=%d)\n",
		__func__, dest_chn->mod_id,
		dest_chn->dev_id, dest_chn->chn_id);

	osal_spin_lock_irqsave(&bind_lock, &flags);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->dsts.mmf_chn[i], dest_chn)) {
				*src_chn = item->node->src;
				osal_spin_unlock_irqrestore(&bind_lock, &flags);
				return 0;
			}
		}
	}
	osal_spin_unlock_irqrestore(&bind_lock, &flags);
	return -1;
}
osal_module_export(bind_get_src);


void bind_init(void)
{
	TAILQ_INIT(&binds);
	osal_spin_lock_init(&bind_lock);
	memset(bind_nodes, 0, sizeof(bind_nodes));
}

void bind_deinit(void)
{
	struct bind_t *item, *item_tmp;
	unsigned long flags;

	osal_spin_lock_irqsave(&bind_lock, &flags);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		TAILQ_REMOVE(&binds, item, tailq);
		osal_kfree(item);
	}
	memset(bind_nodes, 0, sizeof(bind_nodes));
	osal_spin_unlock_irqrestore(&bind_lock, &flags);
	osal_spin_lock_destroy(&bind_lock);
}


#include <linux/types.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_errno.h>
#include <linux/base_uapi.h>
#include <queue.h>


#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
		(var) && ((tvar) = TAILQ_NEXT((var), field), 1);	\
		(var) = (tvar))
#endif

#define CHN_MATCH(x, y) (((x)->enModId == (y)->enModId) && ((x)->s32DevId == (y)->s32DevId)             \
	&& ((x)->s32ChnId == (y)->s32ChnId))

struct bind_t {
	TAILQ_ENTRY(bind_t) tailq;
	BIND_NODE_S *node;
};

TAILQ_HEAD(bind_head, bind_t) binds;

static struct mutex bind_lock;
BIND_NODE_S bind_nodes[BIND_NODE_MAXNUM];


static int32_t bind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn)
{
	struct bind_t *item, *item_tmp;
	int32_t ret = 0, i;

	pr_debug("%s: src(mId=%d, dId=%d, cId=%d), dst(mId=%d, dId=%d, cId=%d)\n",
		__func__,
		pstSrcChn->enModId, pstSrcChn->s32DevId, pstSrcChn->s32ChnId,
		pstDestChn->enModId, pstDestChn->s32DevId, pstDestChn->s32ChnId);

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, pstSrcChn))
			continue;

		// check if dst already bind to src
		for (i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->dsts.astMmfChn[i], pstDestChn)) {
				pr_debug("Duplicate Dst(%d-%d-%d) to Src(%d-%d-%d)\n",
					pstDestChn->enModId, pstDestChn->s32DevId, pstDestChn->s32ChnId,
					pstSrcChn->enModId, pstSrcChn->s32DevId, pstSrcChn->s32ChnId);
				ret = -1;
				goto BIND_EXIT;
			}
		}
		// check if dsts have enough space for one more bind
		if (item->node->dsts.u32Num >= BIND_DEST_MAXNUM) {
			pr_err("Over max bind Dst number\n");
			ret = -1;
			goto BIND_EXIT;
		}
		item->node->dsts.astMmfChn[item->node->dsts.u32Num++] = *pstDestChn;

		goto BIND_SUCCESS;
	}

	// if src not found
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if (!bind_nodes[i].bUsed) {
			memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
			bind_nodes[i].bUsed = true;
			bind_nodes[i].src = *pstSrcChn;
			bind_nodes[i].dsts.u32Num = 1;
			bind_nodes[i].dsts.astMmfChn[0] = *pstDestChn;
			break;
		}
	}

	if (i == BIND_NODE_MAXNUM) {
		pr_err("No free bind node\n");
		ret = -1;
		goto BIND_EXIT;
	}

	item = vzalloc(sizeof(*item));
	if (item == NULL) {
		memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
		ret = CVI_ERR_SYS_NOMEM;
		goto BIND_EXIT;
	}

	item->node = &bind_nodes[i];
	TAILQ_INSERT_TAIL(&binds, item, tailq);

BIND_SUCCESS:
	ret = 0;
BIND_EXIT:
	mutex_unlock(&bind_lock);

	return ret;
}

static int32_t unbind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, pstSrcChn))
			continue;

		for (i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->dsts.astMmfChn[i], pstDestChn)) {
				if (--item->node->dsts.u32Num) {
					for (; i < item->node->dsts.u32Num; i++)
						item->node->dsts.astMmfChn[i] = item->node->dsts.astMmfChn[i + 1];
				} else {
					item->node->bUsed = CVI_FALSE;
					TAILQ_REMOVE(&binds, item, tailq);
					vfree(item);
				}
				mutex_unlock(&bind_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&bind_lock);
	return 0;
}

int32_t bind_get_dst(MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;

	pr_debug("%s: src(.enModId=%d, .s32DevId=%d, .s32ChnId=%d)\n",
		__func__, pstSrcChn->enModId,
		pstSrcChn->s32DevId, pstSrcChn->s32ChnId);

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->src, pstSrcChn)) {
				*pstBindDest = item->node->dsts;
				mutex_unlock(&bind_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&bind_lock);
	return -1;
}
EXPORT_SYMBOL_GPL(bind_get_dst);

int32_t bind_get_src(MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;

	pr_debug("%s: dst(.enModId=%d, .s32DevId=%d, .s32ChnId=%d)\n",
		__func__, pstDestChn->enModId,
		pstDestChn->s32DevId, pstDestChn->s32ChnId);

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.u32Num; ++i) {
			if (CHN_MATCH(&item->node->dsts.astMmfChn[i], pstDestChn)) {
				*pstSrcChn = item->node->src;
				mutex_unlock(&bind_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&bind_lock);
	return -1;
}
EXPORT_SYMBOL_GPL(bind_get_src);


void bind_init(void)
{
	TAILQ_INIT(&binds);
	mutex_init(&bind_lock);
	memset(bind_nodes, 0, sizeof(bind_nodes));
}

void bind_deinit(void)
{
	struct bind_t *item, *item_tmp;

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		TAILQ_REMOVE(&binds, item, tailq);
		vfree(item);
	}
	memset(bind_nodes, 0, sizeof(bind_nodes));
	mutex_unlock(&bind_lock);
}

int32_t bind_set_cfg_user(unsigned long arg)
{
	int32_t ret = 0;
	struct sys_bind_cfg ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_bind_cfg __user *)arg,
			     sizeof(struct sys_bind_cfg));
	if (ret) {
		pr_err("copy_from_user failed, sys_set_mod_cfg\n");
		return ret;
	}

	if (ioctl_arg.is_bind)
		ret = bind(&ioctl_arg.mmf_chn_src, &ioctl_arg.mmf_chn_dst);
	else
		ret = unbind(&ioctl_arg.mmf_chn_src, &ioctl_arg.mmf_chn_dst);

	return ret;
}

int32_t bind_get_cfg_user(unsigned long arg)
{
	int32_t ret = 0;
	struct sys_bind_cfg ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_bind_cfg __user *)arg,
			     sizeof(struct sys_bind_cfg));
	if (ret) {
		pr_err("copy_from_user failed, sys_set_mod_cfg\n");
		return ret;
	}

	if (ioctl_arg.get_by_src)
		ret = bind_get_dst(&ioctl_arg.mmf_chn_src, &ioctl_arg.bind_dst);
	else
		ret = bind_get_src(&ioctl_arg.mmf_chn_dst, &ioctl_arg.mmf_chn_src);

	if (ret)
		pr_err("sys_ctx_getbind failed\n");

	ret = copy_to_user((struct sys_bind_cfg __user *)arg,
			     &ioctl_arg,
			     sizeof(struct sys_bind_cfg));

	if (ret)
		pr_err("copy_to_user fail, sys_get_bind_cfg\n");

	return ret;
}



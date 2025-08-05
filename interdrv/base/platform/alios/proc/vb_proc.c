#include "osal.h"
#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "base_common.h"
#include "comm_math.h"
#include "vb.h"



/*************************************************************************
 *	VB proc functions
 *************************************************************************/
static int32_t _get_vb_mod_ids(struct vb_pool_ctx *pool, uint32_t blk_idx, uint64_t *modids)
{
	uint64_t phyaddr;
	vb_blk blk;
	struct vb_s *vb;

	phyaddr = pool->mem_base_align + (blk_idx * pool->blk_size);
	blk = vb_phys_addr2handle(phyaddr);
	if (blk == VB_INVALID_HANDLE)
		return -1;

	vb = (struct vb_s *)(uintptr_t)blk;
	*modids = osal_atomic_read(&vb->mod_ids);

	return 0;
}

static void _show_vb_status()
{
	uint32_t i, j, k, n;
	uint32_t mod_sum[ID_BUTT];
	int32_t ret;
	uint64_t modids;
	struct vb_pool_ctx *pool_ctx = NULL;
	uint32_t show_cnt;
	uint32_t max_pool_cnt, max_blk_cnt;
	char *buf = osal_malloc(8192);
	int pos = 0;
	uint32_t show_mod_ids[] = {ID_VI, ID_VPSS, ID_VO, ID_RGN, ID_GDC,
		ID_VENC, ID_VDEC, ID_USER};

	show_cnt = (uint32_t)sizeof(show_mod_ids) / (uint32_t)sizeof(show_mod_ids[0]);
	ret = vb_get_pool_info(&pool_ctx, &max_pool_cnt, &max_blk_cnt);
	if (ret != 0) {
		pos += sprintf(buf+pos,"vb_pool has not inited yet\n");
		return;
	}

	pos += sprintf(buf+pos, "-----VB PUB CONFIG-----------------------------------------------------------------------------------------------------------------\n");
	pos += sprintf(buf+pos, "%10s(%3d), %10s(%3d)\n", "MaxPoolCnt", max_pool_cnt, "MaxBlkCnt", max_blk_cnt);

	pos += sprintf(buf+pos,"\n-----COMMON POOL CONFIG------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < max_pool_cnt ; ++i) {
		if (pool_ctx[i].membase != 0) {
			pos += sprintf(buf+pos, "%10s(%3d)\t%10s(%12d)\t%10s(%3d)\n"
			, "PoolId", i, "Size", pool_ctx[i].blk_size, "Count", pool_ctx[i].blk_cnt);
		}
	}

	pos += sprintf(buf+pos,"\n-----------------------------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < max_pool_cnt; ++i) {
		if (pool_ctx[i].membase != 0) {
			osal_mutex_lock(&pool_ctx[i].lock);
			pos += sprintf(buf+pos, "%-10s: %s\n", "PoolName", pool_ctx[i].pool_name);
			pos += sprintf(buf+pos, "%-10s: %d\n", "PoolId", pool_ctx[i].poolid);
			pos += sprintf(buf+pos, "%-10s: 0x%llx\n", "PhysAddr", (unsigned long long)pool_ctx[i].membase);
			pos += sprintf(buf+pos, "%-10s: 0x%lx\n", "VirtAddr", (uintptr_t)pool_ctx[i].vmembase);
			pos += sprintf(buf+pos, "%-10s: %d\n", "IsComm", pool_ctx[i].is_comm_pool);
			pos += sprintf(buf+pos, "%-10s: %d\n", "Owner", pool_ctx[i].ownerid);
			pos += sprintf(buf+pos, "%-10s: %d\n", "BlkSz", pool_ctx[i].blk_size);
			pos += sprintf(buf+pos, "%-10s: %d\n", "BlkCnt", pool_ctx[i].blk_cnt);
			pos += sprintf(buf+pos, "%-10s: %d\n", "Free", pool_ctx[i].free_blk_cnt);
			pos += sprintf(buf+pos, "%-10s: %d\n", "MinFree", pool_ctx[i].min_free_blk_cnt);
			pos += sprintf(buf+pos,"\n");

			memset(mod_sum, 0, sizeof(mod_sum));
			pos += sprintf(buf+pos,"BLK");
			for (k = 0; k < show_cnt; k++)
				pos += sprintf(buf+pos, "\t%s", sys_get_modname(show_mod_ids[k]));

			for (j = 0; j < pool_ctx[i].blk_cnt; ++j) {
				pos += sprintf(buf+pos, "\n%s%d", "#", j);
				if (_get_vb_mod_ids(&pool_ctx[i], j, &modids) != 0) {
					for (k = 0; k < show_cnt; ++k) {
						pos += sprintf(buf+pos,"\te");
					}
					continue;
				}

				for (k = 0; k < show_cnt; ++k) {
					n = show_mod_ids[k];
					if (modids & BIT(n)) {
						pos += sprintf(buf+pos,"\t1");
						mod_sum[n]++;
					} else
						pos += sprintf(buf+pos,"\t0");
				}
			}

			pos += sprintf(buf+pos,"\nSum");
			for (k = 0; k < show_cnt; ++k) {
				n = show_mod_ids[k];
				pos += sprintf(buf+pos, "\t%d", mod_sum[n]);
			}

			osal_mutex_unlock(&pool_ctx[i].lock);
			pos += sprintf(buf+pos,"\n-----------------------------------------------------------------------------------------------------------------------------------\n");
		}
	}

	printf("%s",buf);
	osal_free(buf);
}

static void vb_proc_show(int32_t argc, char **argv)
{
	_show_vb_status();
}

ALIOS_CLI_CMD_REGISTER(vb_proc_show, proc_vb, vb info);

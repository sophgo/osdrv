/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vb_proc.c
 * Description: vb proc driver code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "cvi_vb_proc.h"

#define VB_PROC_NAME			"vb"
#define VB_PROC_PERMS			(0644)

static void *shared_mem;
/*************************************************************************
 *	VB proc functions
 *************************************************************************/
static void _show_vb_status(struct seq_file *m)
{
	int i, j, k;
	unsigned int modId_offset = 0;
	struct VB_POOL_S *pstVbPool = NULL;
	uint64_t *pModIds = NULL;
	int mod_sum[CVI_ID_BUTT];

	pstVbPool = (struct VB_POOL_S *)(shared_mem + BASE_VB_COMM_POOL_OFFSET);

	seq_printf(m, "\nModule: [VB], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "-----VB PUB CONFIG-----------------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "%10s(%3d), %10s(%3d)\n", "MaxPoolCnt", vb_max_pools, "MaxBlkCnt", vb_pool_max_blk);
	seq_puts(m, "\n-----COMMON POOL CONFIG------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < vb_max_pools ; ++i) {
		if (pstVbPool[i].memBase != 0) {
			seq_printf(m, "%10s(%3d)\t%10s(%12d)\t%10s(%3d)\n"
			, "PoolId", i, "Size", pstVbPool[i].config.u32BlkSize, "Count", pstVbPool[i].config.u32BlkCnt);
		}
	}

	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < vb_max_pools; ++i) {
		if (pstVbPool[i].memBase != 0) {
			seq_printf(m, "%-10s: %s\n", "PoolName", pstVbPool[i].acPoolName);
			seq_printf(m, "%-10s: %d\n", "PoolId", pstVbPool[i].poolID);
			seq_printf(m, "%-10s: 0x%llx\n", "PhysAddr", pstVbPool[i].memBase);
			seq_printf(m, "%-10s: 0x%lx\n", "VirtAddr", (uintptr_t)pstVbPool[i].vmemBase);
			seq_printf(m, "%-10s: %d\n", "IsComm", pstVbPool[i].bIsCommPool);
			seq_printf(m, "%-10s: %d\n", "Owner", pstVbPool[i].ownerID);
			seq_printf(m, "%-10s: %d\n", "BlkSz", pstVbPool[i].config.u32BlkSize);
			seq_printf(m, "%-10s: %d\n", "BlkCnt", pstVbPool[i].config.u32BlkCnt);
			seq_printf(m, "%-10s: %d\n", "Free", pstVbPool[i].u32FreeBlkCnt);
			seq_printf(m, "%-10s: %d\n", "MinFree", pstVbPool[i].u32MinFreeBlkCnt);
			seq_puts(m, "\n");

			memset(mod_sum, 0, sizeof(mod_sum));
			modId_offset = BASE_VB_BLK_MOD_ID_OFFSET + ((VB_BLK_MOD_ID_RSV_SIZE / vb_max_pools) * i);
			seq_puts(m, "BLK\tBASE\tVB\tSYS\tRGN\tCHNL\tVDEC\tVPSS\tVENC\tH264E\tJPEGE\tMPEG4E\tH265E\tJPEGD\tVO\tVI\tDIS\n");
			seq_puts(m, "RC\tAIO\tAI\tAO\tAENC\tADEC\tAUD\tVPU\tISP\tIVE\tUSER\tPROC\tLOG\tH264D\tGDC\tPHOTO\tFB\n");
			for (j = 0; j < pstVbPool[i].config.u32BlkCnt; ++j) {
				seq_printf(m, "%s%d\t", "#", j);
				pModIds = (uint64_t *)(shared_mem + modId_offset);
				for (k = 0; k < CVI_ID_BUTT; ++k) {
					if (*pModIds & BIT(k)) {
						seq_puts(m, "1\t");
						mod_sum[k]++;
					} else
						seq_puts(m, "0\t");

					if (k == CVI_ID_DIS || k == CVI_ID_FB)
						seq_puts(m, "\n");
				}
				modId_offset += sizeof(*pModIds);
			}

			seq_puts(m, "Sum\t");
			for (k = 0; k < CVI_ID_BUTT; ++k) {
				seq_printf(m, "%d\t", mod_sum[k]);
				if (k == CVI_ID_DIS || k == CVI_ID_FB)
					seq_puts(m, "\n");
			}
			seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
		}
	}
}

static int _vb_proc_show(struct seq_file *m, void *v)
{
	_show_vb_status(m);
	return 0;
}

static int _vb_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vb_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _vb_proc_fops = {
	.proc_open = _vb_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vb_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vb_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vb_proc_init(struct proc_dir_entry *_proc_dir, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VB_PROC_NAME, VB_PROC_PERMS, _proc_dir, &_vb_proc_fops, NULL) == NULL) {
		pr_err("vb proc creation failed\n");
		rc = -1;
	}

	shared_mem = shm;
	return rc;
}

int vb_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(VB_PROC_NAME, _proc_dir);
	shared_mem = NULL;

	return 0;
}

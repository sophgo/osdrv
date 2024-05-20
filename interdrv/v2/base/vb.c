#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include "vb.h"
#include "ion.h"
#include "queue.h"
#include "base_common.h"

uint32_t vb_max_pools = 512;
module_param(vb_max_pools, uint, 0644);
uint32_t vb_pool_max_blk = 128;
module_param(vb_pool_max_blk, uint, 0644);

static struct cvi_vb_cfg stVbConfig;
static struct vb_pool *vbPool;
static atomic_t ref_count = ATOMIC_INIT(0);

static DEFINE_MUTEX(g_lock);
static DEFINE_MUTEX(getVB_lock);
static DEFINE_MUTEX(poolLock);
static DEFINE_SPINLOCK(hashLock);

DEFINE_HASHTABLE(vb_hash, 8);

#define CHECK_VB_HANDLE_NULL(x)							\
	do {									\
		if ((x) == NULL) {						\
			pr_err(" NULL VB HANDLE\n");		\
			return -EINVAL;				\
		}								\
	} while (0)

#define CHECK_VB_HANDLE_VALID(x)						\
	do {									\
		if ((x)->magic != CVI_VB_MAGIC) {	\
			pr_err(" invalid VB Handle\n");	\
			return -EINVAL;				\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_WEAK(x)							\
	do {									\
		if ((x) == VB_STATIC_POOLID)					\
			break;							\
		if ((x) == VB_EXTERNAL_POOLID)					\
			break;							\
		if ((x) >= (vb_max_pools)) {					\
			pr_err(" invalid VB Pool(%d)\n", x);	\
			return -EINVAL;			\
		}								\
		if (!is_pool_inited(x)) {						\
			pr_err("VB_POOL(%d) isn't init yet.\n", x); \
			return -EINVAL;			\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_STRONG(x)							\
	do {									\
		if ((x) >= (vb_max_pools)) {					\
			pr_err(" invalid VB Pool(%d)\n", x);	\
			return -EINVAL; 		\
		}								\
		if (!is_pool_inited(x)) { 					\
			pr_err("VB_POOL(%d) isn't init yet.\n", x); \
			return -EINVAL; 		\
		}								\
	} while (0)

static inline bool is_pool_inited(VB_POOL poolId)
{
	return (vbPool[poolId].memBase == 0) ? CVI_FALSE : CVI_TRUE;
}


static bool _vb_hash_del(uint64_t u64PhyAddr)
{
	bool is_found = false;
	struct vb_s *obj;
	struct hlist_node *tmp;

	spin_lock(&hashLock);
	hash_for_each_possible_safe(vb_hash, obj, tmp, node, u64PhyAddr) {
		if (obj->phy_addr == u64PhyAddr) {
			hash_del(&obj->node);
			is_found = true;
			break;
		}
	}
	spin_unlock(&hashLock);

	return is_found;
}

static bool _vb_hash_find(uint64_t u64PhyAddr, struct vb_s **vb)
{
	bool is_found = false;
	struct vb_s *obj;

	spin_lock(&hashLock);
	hash_for_each_possible(vb_hash, obj, node, u64PhyAddr) {
		if (obj->phy_addr == u64PhyAddr) {
			is_found = true;
			break;
		}
	}
	spin_unlock(&hashLock);

	if (is_found)
		*vb = obj;
	return is_found;
}

static bool _is_comm_vb_released(void)
{
	uint32_t i;

	for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
		if (is_pool_inited(i)) {
			if (FIFO_CAPACITY(&vbPool[i].freeList) != FIFO_SIZE(&vbPool[i].freeList)) {
				pr_info("pool(%d) blk has not been all released yet\n", i);
				return false;
			}
		}
	}
	return true;
}

int32_t vb_print_pool(VB_POOL poolId)
{
	struct vb_s *vb;
	int bkt, i;
	char str[64];

	CHECK_VB_POOL_VALID_STRONG(poolId);

	spin_lock(&hashLock);
	hash_for_each(vb_hash, bkt, vb, node) {
		if (vb->vb_pool == poolId) {
			sprintf(str, "Pool[%d] vb paddr(%#llx) usr_cnt(%d) /",
				vb->vb_pool, vb->phy_addr, vb->usr_cnt.counter);

			for (i = 0; i < CVI_ID_BUTT; ++i) {
				if (atomic_long_read(&vb->mod_ids) & BIT(i)) {
					strncat(str, sys_get_modname(i), sizeof(str));
					strcat(str, "/");
				}
			}
			pr_info("%s\n", str);
		}
	}
	spin_unlock(&hashLock);

	return 0;
}
EXPORT_SYMBOL_GPL(vb_print_pool);

static int32_t _vb_set_config(struct cvi_vb_cfg *vb_cfg)
{
	int i;

	if (vb_cfg->comm_pool_cnt > VB_COMM_POOL_MAX_CNT
		|| vb_cfg->comm_pool_cnt == 0) {
		pr_err("Invalid comm_pool_cnt(%d)\n", vb_cfg->comm_pool_cnt);
		return -EINVAL;
	}

	for (i = 0; i < vb_cfg->comm_pool_cnt; ++i) {
		if (vb_cfg->comm_pool[i].blk_size == 0
			|| vb_cfg->comm_pool[i].blk_cnt == 0
			|| vb_cfg->comm_pool[i].blk_cnt > vb_pool_max_blk) {
			pr_err("Invalid pool cfg, pool(%d), blk_size(%d), blk_cnt(%d)\n",
				i, vb_cfg->comm_pool[i].blk_size,
				vb_cfg->comm_pool[i].blk_cnt);
			return -EINVAL;
		}
	}
	stVbConfig = *vb_cfg;

	return 0;
}

static void _vb_cleanup(void)
{
	struct vb_s *vb;
	int bkt, i;
	struct hlist_node *tmp;
	struct vb_pool *pstPool;
	struct vb_req *req, *req_tmp;


	// free vb pool
	for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
		if (is_pool_inited(i)) {
			pstPool = &vbPool[i];
			mutex_lock(&pstPool->lock);
			FIFO_EXIT(&pstPool->freeList);
			base_ion_free(pstPool->memBase);
			mutex_unlock(&pstPool->lock);
			mutex_destroy(&pstPool->lock);
			// free reqQ
			mutex_lock(&pstPool->reqQ_lock);
			if (!STAILQ_EMPTY(&pstPool->reqQ)) {
				STAILQ_FOREACH_SAFE(req, &pstPool->reqQ, stailq, req_tmp) {
					STAILQ_REMOVE(&pstPool->reqQ, req, vb_req, stailq);
					kfree(req);
				}
			}
			mutex_unlock(&pstPool->reqQ_lock);
			mutex_destroy(&pstPool->reqQ_lock);
			memset(pstPool, 0, sizeof(struct vb_pool));
		}
	}

	// free comm vb blk
	spin_lock(&hashLock);
	hash_for_each_safe(vb_hash, bkt, tmp, vb, node) {
		if ((vb->vb_pool >= VB_MAX_COMM_POOLS) && (vb->vb_pool < vb_max_pools))
			continue;
		if (vb->vb_pool == VB_STATIC_POOLID)
			base_ion_free(vb->phy_addr);
		hash_del(&vb->node);
	}
	spin_unlock(&hashLock);

}

static int32_t _vb_create_pool(struct cvi_vb_pool_cfg *config, bool isComm)
{
	uint32_t poolSize;
	struct vb_s *p;
	bool isCache;
	char ion_name[10];
	int32_t ret, i;
	VB_POOL poolId = config->pool_id;
	void *ion_v = NULL;

	poolSize = config->blk_size * config->blk_cnt;
	isCache = (config->remap_mode == VB_REMAP_MODE_CACHED);

	snprintf(ion_name, 10, "VbPool%d", poolId);
	ret = base_ion_alloc(&vbPool[poolId].memBase, &ion_v, (uint8_t *)ion_name, poolSize, isCache);
	if (ret) {
		pr_err("base_ion_alloc fail! ret(%d)\n", ret);
		return ret;
	}
	config->mem_base = vbPool[poolId].memBase;

	STAILQ_INIT(&vbPool[poolId].reqQ);
	mutex_init(&vbPool[poolId].reqQ_lock);
	mutex_init(&vbPool[poolId].lock);
	mutex_lock(&vbPool[poolId].lock);
	vbPool[poolId].poolID = poolId;
	vbPool[poolId].ownerID = (isComm) ? POOL_OWNER_COMMON : POOL_OWNER_PRIVATE;
	vbPool[poolId].vmemBase = ion_v;
	vbPool[poolId].blk_cnt = config->blk_cnt;
	vbPool[poolId].blk_size = config->blk_size;
	vbPool[poolId].remap_mode = config->remap_mode;
	vbPool[poolId].bIsCommPool = isComm;
	vbPool[poolId].u32FreeBlkCnt = config->blk_cnt;
	vbPool[poolId].u32MinFreeBlkCnt = vbPool[poolId].u32FreeBlkCnt;
	if (strlen(config->pool_name) != 0)
		strncpy(vbPool[poolId].acPoolName, config->pool_name,
			sizeof(vbPool[poolId].acPoolName));
	else
		strncpy(vbPool[poolId].acPoolName, "vbpool", sizeof(vbPool[poolId].acPoolName));
	vbPool[poolId].acPoolName[VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&vbPool[poolId].freeList, vbPool[poolId].blk_cnt);
	for (i = 0; i < vbPool[poolId].blk_cnt; ++i) {
		p = vzalloc(sizeof(*p));
		p->phy_addr = vbPool[poolId].memBase + (i * vbPool[poolId].blk_size);
		p->vir_addr = vbPool[poolId].vmemBase + (p->phy_addr - vbPool[poolId].memBase);
		p->vb_pool = poolId;
		atomic_set(&p->usr_cnt, 0);
		p->magic = CVI_VB_MAGIC;
		atomic_long_set(&p->mod_ids, 0);
		p->external = false;
		FIFO_PUSH(&vbPool[poolId].freeList, p);
		spin_lock(&hashLock);
		hash_add(vb_hash, &p->node, p->phy_addr);
		spin_unlock(&hashLock);
	}
	mutex_unlock(&vbPool[poolId].lock);

	return 0;
}

static int32_t _vb_destroy_pool(VB_POOL poolId)
{
	struct vb_pool *pstPool = &vbPool[poolId];
	struct vb_s *vb;
	struct vb_req *req, *req_tmp;

	pr_info("vb destroy pool, pool[%d]: capacity(%d) size(%d).\n"
		, poolId, FIFO_CAPACITY(&pstPool->freeList), FIFO_SIZE(&pstPool->freeList));
	if (FIFO_CAPACITY(&pstPool->freeList) != FIFO_SIZE(&pstPool->freeList)) {
		pr_info("pool(%d) blk should be all released before destroy pool\n", poolId);
		return -1;
	}

	mutex_lock(&pstPool->lock);
	while (!FIFO_EMPTY(&pstPool->freeList)) {
		FIFO_POP(&pstPool->freeList, &vb);
		_vb_hash_del(vb->phy_addr);
		vfree(vb);
	}
	FIFO_EXIT(&pstPool->freeList);
	base_ion_free(pstPool->memBase);
	mutex_unlock(&pstPool->lock);
	mutex_destroy(&pstPool->lock);

	// free reqQ
	mutex_lock(&pstPool->reqQ_lock);
	if (!STAILQ_EMPTY(&pstPool->reqQ)) {
		STAILQ_FOREACH_SAFE(req, &pstPool->reqQ, stailq, req_tmp) {
			STAILQ_REMOVE(&pstPool->reqQ, req, vb_req, stailq);
			kfree(req);
		}
	}
	mutex_unlock(&pstPool->reqQ_lock);
	mutex_destroy(&pstPool->reqQ_lock);

	memset(&vbPool[poolId], 0, sizeof(vbPool[poolId]));

	return 0;
}

static int32_t _vb_init(void)
{
	uint32_t i;
	int32_t ret;

	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		for (i = 0; i < stVbConfig.comm_pool_cnt; ++i) {
			stVbConfig.comm_pool[i].pool_id = i;
			ret = _vb_create_pool(&stVbConfig.comm_pool[i], true);
			if (ret) {
				pr_err("_vb_create_pool fail, ret(%d)\n", ret);
				goto VB_INIT_FAIL;
			}
		}

		pr_info("_vb_init -\n");
	}
	atomic_add(1, &ref_count);
	mutex_unlock(&g_lock);
	return 0;

VB_INIT_FAIL:
	for (i = 0; i < stVbConfig.comm_pool_cnt; ++i) {
		if (is_pool_inited(i))
			_vb_destroy_pool(i);
	}

	mutex_unlock(&g_lock);
	return ret;
}

static int32_t _vb_exit(void)
{
	int i;

	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		pr_info("vb has already exited\n");
		mutex_unlock(&g_lock);
		return 0;
	}
	if (atomic_sub_return(1, &ref_count) > 0) {
		mutex_unlock(&g_lock);
		return 0;
	}

	if (!_is_comm_vb_released()) {
		pr_info("vb has not been all released\n");
		for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
			if (is_pool_inited(i))
				vb_print_pool(i);
		}
	}
	_vb_cleanup();
	mutex_unlock(&g_lock);
	pr_info("_vb_exit -\n");
	return 0;
}

VB_POOL find_vb_pool(uint32_t u32BlkSize)
{
	VB_POOL Pool = VB_INVALID_POOLID;
	int i;

	for (i = 0; i < VB_COMM_POOL_MAX_CNT; ++i) {
		if (!is_pool_inited(i))
			continue;
		if (vbPool[i].ownerID != POOL_OWNER_COMMON)
			continue;
		if (vbPool[i].u32FreeBlkCnt == 0)
			continue;
		if (u32BlkSize > vbPool[i].blk_size)
			continue;
		if ((Pool == VB_INVALID_POOLID)
			|| (vbPool[Pool].blk_size > vbPool[i].blk_size))
			Pool = i;
	}
	return Pool;
}
EXPORT_SYMBOL_GPL(find_vb_pool);

static VB_BLK _vb_get_block_static(uint32_t u32BlkSize)
{
	int32_t ret = CVI_SUCCESS;
	uint64_t phy_addr = 0;
	void *ion_v = NULL;

	//allocate with ion
	ret = base_ion_alloc(&phy_addr, &ion_v, "static_pool", u32BlkSize, true);
	if (ret) {
		pr_err("base_ion_alloc fail! ret(%d)\n", ret);
		return VB_INVALID_HANDLE;
	}

	return vb_create_block(phy_addr, ion_v, VB_STATIC_POOLID, false);
}

/* _vb_get_block: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk.
 * @param u32BlkSize: the size of vb_blk to acquire.
 * @param modId: the Id of mod which acquire this blk
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
static VB_BLK _vb_get_block(struct vb_pool *pool, u32 u32BlkSize, MOD_ID_E modId)
{
	struct vb_s *p;

	if (u32BlkSize > pool->blk_size) {
		pr_err("PoolID(%#x) blksize(%d) > pool's(%d).\n"
			, pool->poolID, u32BlkSize, pool->blk_size);
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&pool->lock);
	if (FIFO_EMPTY(&pool->freeList)) {
		pr_info("VB_POOL owner(%#x) poolID(%#x) pool is empty.\n",
			pool->ownerID, pool->poolID);
		mutex_unlock(&pool->lock);
		vb_print_pool(pool->poolID);
		return VB_INVALID_HANDLE;
	}

	FIFO_POP(&pool->freeList, &p);
	pool->u32FreeBlkCnt--;
	pool->u32MinFreeBlkCnt =
		(pool->u32FreeBlkCnt < pool->u32MinFreeBlkCnt) ? pool->u32FreeBlkCnt : pool->u32MinFreeBlkCnt;
	atomic_set(&p->usr_cnt, 1);
	atomic_long_set(&p->mod_ids, BIT(modId));
	mutex_unlock(&pool->lock);
	pr_debug("Mod(%s) phy-addr(%#llx).\n", sys_get_modname(modId), p->phy_addr);
	return (VB_BLK)p;
}

static int32_t _vb_get_blk_info(struct cvi_vb_blk_info *blk_info)
{
	VB_BLK blk = (VB_BLK)blk_info->blk;
	struct vb_s *vb;

	vb = (struct vb_s *)blk;
	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	blk_info->phy_addr = vb->phy_addr;
	blk_info->pool_id = vb->vb_pool;
	blk_info->usr_cnt = vb->usr_cnt.counter;
	return 0;
}

static int32_t _vb_get_pool_cfg(struct cvi_vb_pool_cfg *pool_cfg)
{
	VB_POOL poolId = pool_cfg->pool_id;

	if (atomic_read(&ref_count) == 0) {
		pr_err("vb module hasn't inited yet.\n");
		return VB_INVALID_POOLID;
	}
	CHECK_VB_POOL_VALID_STRONG(poolId);

	pool_cfg->blk_cnt = vbPool[poolId].blk_cnt;
	pool_cfg->blk_size = vbPool[poolId].blk_size;
	pool_cfg->remap_mode = vbPool[poolId].remap_mode;
	pool_cfg->mem_base = vbPool[poolId].memBase;

	return 0;
}

/**************************************************************************
 *	 global APIs.
 **************************************************************************/
int32_t vb_get_pool_info(struct vb_pool **pool_info, uint32_t *pMaxPool, uint32_t *pMaxBlk)
{
	CHECK_VB_HANDLE_NULL(pool_info);
	CHECK_VB_HANDLE_NULL(vbPool);

	*pool_info = vbPool;
	*pMaxPool = vb_max_pools;
	*pMaxBlk = vb_pool_max_blk;

	return 0;
}

void vb_cleanup(void)
{
	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		pr_info("vb has already exited\n");
		mutex_unlock(&g_lock);
		return;
	}
	_vb_cleanup();
	atomic_set(&ref_count, 0);
	mutex_unlock(&g_lock);
	pr_info("vb_cleanup done\n");
}

int32_t vb_get_config(struct cvi_vb_cfg *pstVbConfig)
{
	if (!pstVbConfig) {
		pr_err("vb_get_config NULL ptr!\n");
		return -EINVAL;
	}

	*pstVbConfig = stVbConfig;
	return 0;
}
EXPORT_SYMBOL_GPL(vb_get_config);

int32_t vb_create_pool(struct cvi_vb_pool_cfg *config)
{
	uint32_t i;
	int32_t ret;

	config->pool_id = VB_INVALID_POOLID;
	if ((config->blk_size == 0) || (config->blk_cnt == 0)
		|| (config->blk_cnt > vb_pool_max_blk)) {
		pr_err("Invalid pool cfg, blk_size(%d), blk_cnt(%d)\n",
				config->blk_size, config->blk_cnt);
		return -EINVAL;
	}

	mutex_lock(&poolLock);
	for (i = VB_MAX_COMM_POOLS; i < vb_max_pools; ++i) {
		if (!is_pool_inited(i))
			break;
	}
	if (i >= vb_max_pools) {
		pr_err("Exceed vb_max_pools cnt: %d\n", vb_max_pools);
		mutex_unlock(&poolLock);
		return -ENOMEM;
	}

	config->pool_id = i;
	ret = _vb_create_pool(config, false);
	if (ret) {
		pr_err("_vb_create_pool fail, ret(%d)\n", ret);
		mutex_unlock(&poolLock);
		return ret;
	}
	mutex_unlock(&poolLock);
	return 0;
}
EXPORT_SYMBOL_GPL(vb_create_pool);


int32_t vb_destroy_pool(VB_POOL poolId)
{
	CHECK_VB_POOL_VALID_STRONG(poolId);

	return _vb_destroy_pool(poolId);
}
EXPORT_SYMBOL_GPL(vb_destroy_pool);

/* vb_create_block: create a vb blk per phy-addr given.
 *
 * @param phyAddr: phy-address of the buffer for this new vb.
 * @param virAddr: virtual-address of the buffer for this new vb.
 * @param VbPool: the pool of the vb belonging.
 * @param isExternal: if the buffer is not allocated by mmf
 */
VB_BLK vb_create_block(uint64_t phyAddr, void *virAddr, VB_POOL VbPool, bool isExternal)
{
	struct vb_s *p = NULL;

	p = vmalloc(sizeof(*p));
	if (!p) {
		pr_err("vmalloc failed.\n");
		return VB_INVALID_HANDLE;
	}

	p->phy_addr = phyAddr;
	p->vir_addr = virAddr;
	p->vb_pool = VbPool;
	atomic_set(&p->usr_cnt, 1);
	p->magic = CVI_VB_MAGIC;
	atomic_long_set(&p->mod_ids, 0);
	p->external = isExternal;
	spin_lock(&hashLock);
	hash_add(vb_hash, &p->node, p->phy_addr);
	spin_unlock(&hashLock);

	return (VB_BLK)p;
}
EXPORT_SYMBOL_GPL(vb_create_block);

VB_BLK vb_get_block_with_id(VB_POOL poolId, uint32_t u32BlkSize, MOD_ID_E modId)
{
	VB_BLK blk = VB_INVALID_HANDLE;

	mutex_lock(&getVB_lock);
	// common pool
	if (poolId == VB_INVALID_POOLID) {
		poolId = find_vb_pool(u32BlkSize);
		if (poolId == VB_INVALID_POOLID) {
			pr_err("No valid pool for size(%d).\n", u32BlkSize);
			goto get_vb_done;
		}
	} else if (poolId == VB_STATIC_POOLID) {
		blk = _vb_get_block_static(u32BlkSize);		//need not mapping pool, allocate vb block directly
		goto get_vb_done;
	} else if (poolId >= vb_max_pools) {
		pr_err(" invalid VB Pool(%d)\n", poolId);
		goto get_vb_done;
	} else {
		if (!is_pool_inited(poolId)) {
			pr_err("VB_POOL(%d) isn't init yet.\n", poolId);
			goto get_vb_done;
		}

		if (u32BlkSize > vbPool[poolId].blk_size) {
			pr_err("required size(%d) > pool(%d)'s blk-size(%d).\n", u32BlkSize, poolId,
					 vbPool[poolId].blk_size);
			goto get_vb_done;
		}
	}
	blk = _vb_get_block(&vbPool[poolId], u32BlkSize, modId);

get_vb_done:
	mutex_unlock(&getVB_lock);
	return blk;
}
EXPORT_SYMBOL_GPL(vb_get_block_with_id);

int32_t vb_release_block(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	struct vb_s *vb_tmp;
	struct vb_pool *pool;
	int cnt;
	int32_t result;
	bool bReq = false;
	struct vb_req *req, *tmp;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);
	CHECK_VB_POOL_VALID_WEAK(vb->vb_pool);

	cnt = atomic_sub_return(1, &vb->usr_cnt);
	if (cnt <= 0) {
		pr_debug("%p phy-addr(%#llx) release.\n",
			__builtin_return_address(0), vb->phy_addr);

		if (vb->external) {
			pr_debug("external buffer phy-addr(%#llx) release.\n", vb->phy_addr);
			_vb_hash_del(vb->phy_addr);
			vfree(vb);
			return 0;
		}

		//free VB_STATIC_POOLID
		if (vb->vb_pool == VB_STATIC_POOLID) {
			int32_t ret = 0;

			ret = base_ion_free(vb->phy_addr);
			_vb_hash_del(vb->phy_addr);
			vfree(vb);
			return ret;
		}

		if (cnt < 0) {
			int i = 0;

			pr_info("vb usr_cnt is zero.\n");
			pool = &vbPool[vb->vb_pool];
			mutex_lock(&pool->lock);
			FIFO_FOREACH(vb_tmp, &pool->freeList, i) {
				if (vb_tmp->phy_addr == vb->phy_addr) {
					mutex_unlock(&pool->lock);
					atomic_set(&vb->usr_cnt, 0);
					return 0;
				}
			}
			mutex_unlock(&pool->lock);
		}

		pool = &vbPool[vb->vb_pool];
		mutex_lock(&pool->lock);
		memset(&vb->buf, 0, sizeof(vb->buf));
		atomic_set(&vb->usr_cnt, 0);
		atomic_long_set(&vb->mod_ids, 0);
		FIFO_PUSH(&pool->freeList, vb);
		++pool->u32FreeBlkCnt;
		mutex_unlock(&pool->lock);

		mutex_lock(&pool->reqQ_lock);
		if (!STAILQ_EMPTY(&pool->reqQ)) {
			STAILQ_FOREACH_SAFE(req, &pool->reqQ, stailq, tmp) {
				if (req->VbPool != pool->poolID)
					continue;
				pr_info("pool(%d) vb(%#llx) release, Try acquire vb for %s\n", pool->poolID,
					vb->phy_addr, sys_get_modname(req->chn.enModId));
				STAILQ_REMOVE(&pool->reqQ, req, vb_req, stailq);
				bReq = true;
				break;
			}
		}
		mutex_unlock(&pool->reqQ_lock);
		if (bReq) {
			result = req->fp(req->chn, req->data);
			if (result) { // req->fp return fail
				mutex_lock(&pool->reqQ_lock);
				STAILQ_INSERT_TAIL(&pool->reqQ, req, stailq);
				mutex_unlock(&pool->reqQ_lock);
			} else
				kfree(req);
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(vb_release_block);

VB_BLK vb_phys_addr2handle(uint64_t u64PhyAddr)
{
	struct vb_s *vb = NULL;

	if (!_vb_hash_find(u64PhyAddr, &vb)) {
		pr_debug("Cannot find vb corresponding to phyAddr:%#llx\n", u64PhyAddr);
		return VB_INVALID_HANDLE;
	} else
		return (VB_BLK)vb;
}
EXPORT_SYMBOL_GPL(vb_phys_addr2handle);

uint64_t vb_handle2phys_addr(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != CVI_VB_MAGIC))
		return 0;
	return vb->phy_addr;
}
EXPORT_SYMBOL_GPL(vb_handle2phys_addr);

void *vb_handle2virt_addr(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != CVI_VB_MAGIC))
		return NULL;
	return vb->vir_addr;
}
EXPORT_SYMBOL_GPL(vb_handle2virt_addr);

VB_POOL vb_handle2pool_id(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != CVI_VB_MAGIC))
		return VB_INVALID_POOLID;
	return vb->vb_pool;
}
EXPORT_SYMBOL_GPL(vb_handle2pool_id);

int32_t vb_inquire_user_cnt(VB_BLK blk, uint32_t *pCnt)
{
	struct vb_s *vb = (struct vb_s *)blk;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	*pCnt = vb->usr_cnt.counter;
	return 0;
}
EXPORT_SYMBOL_GPL(vb_inquire_user_cnt);

/* vb_acquire_block: to register a callback to acquire vb_blk at CVI_VB_ReleaseBlock
 *						in case of CVI_VB_GetBlock failure.
 *
 * @param fp: callback to acquire blk for module.
 * @param chn: info of the module which needs this helper.
 */
void vb_acquire_block(vb_acquire_fp fp, MMF_CHN_S chn, VB_POOL VbPool, void *data)
{
	struct vb_req *req = NULL;

	if (VbPool == VB_INVALID_POOLID) {
		pr_err("invalid poolid.\n");
		return;
	}
	if (VbPool >= vb_max_pools) {
		pr_err(" invalid VB Pool(%d)\n", VbPool);
		return;
	}
	if (!is_pool_inited(VbPool)) {
		pr_err("VB_POOL(%d) isn't init yet.\n", VbPool);
		return;
	}

	req = kmalloc(sizeof(*req), GFP_ATOMIC);
	if (!req) {
		//pr_err("kmalloc failed.\n");	warning2error fail
		return;
	}

	req->fp = fp;
	req->chn = chn;
	req->VbPool = VbPool;
	req->data = data;

	mutex_lock(&vbPool[VbPool].reqQ_lock);
	STAILQ_INSERT_TAIL(&vbPool[VbPool].reqQ, req, stailq);
	mutex_unlock(&vbPool[VbPool].reqQ_lock);
}
EXPORT_SYMBOL_GPL(vb_acquire_block);

void vb_cancel_block(MMF_CHN_S chn, VB_POOL VbPool)
{
	struct vb_req *req, *tmp;

	if (VbPool == VB_INVALID_POOLID) {
		pr_err("invalid poolid.\n");
		return;
	}
	if (VbPool >= vb_max_pools) {
		pr_err(" invalid VB Pool(%d)\n", VbPool);
		return;
	}
	if (!is_pool_inited(VbPool)) {
		pr_err("VB_POOL(%d) isn't init yet.\n", VbPool);
		return;
	}

	mutex_lock(&vbPool[VbPool].reqQ_lock);
	if (!STAILQ_EMPTY(&vbPool[VbPool].reqQ)) {
		STAILQ_FOREACH_SAFE(req, &vbPool[VbPool].reqQ, stailq, tmp) {
			if (CHN_MATCH(&req->chn, &chn)) {
				STAILQ_REMOVE(&vbPool[VbPool].reqQ, req, vb_req, stailq);
				kfree(req);
			}
		}
	}
	mutex_unlock(&vbPool[VbPool].reqQ_lock);
}
EXPORT_SYMBOL_GPL(vb_cancel_block);

long vb_ctrl(unsigned long arg)
{
	long ret = 0;
	struct vb_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vb_ext_control)))
		return -EINVAL;

	switch (p.id) {
	case VB_IOCTL_SET_CONFIG: {
		struct cvi_vb_cfg cfg;

		if (atomic_read(&ref_count)) {
			pr_err("vb has already inited, set_config cmd has no effect\n");
			break;
		}

		memset(&cfg, 0, sizeof(struct cvi_vb_cfg));
		if (copy_from_user(&cfg, p.ptr, sizeof(struct cvi_vb_cfg))) {
			pr_err("VB_IOCTL_SET_CONFIG copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}
		ret = _vb_set_config(&cfg);
		break;
	}

	case VB_IOCTL_GET_CONFIG: {
		if (copy_to_user(p.ptr, &stVbConfig, sizeof(struct cvi_vb_cfg))) {
			pr_err("VB_IOCTL_GET_CONFIG copy_to_user failed.\n");
			ret = -ENOMEM;
		}
		break;
	}

	case VB_IOCTL_INIT:
		ret = _vb_init();
		break;

	case VB_IOCTL_EXIT:
		ret = _vb_exit();
		break;

	case VB_IOCTL_CREATE_POOL: {
		struct cvi_vb_pool_cfg cfg;

		memset(&cfg, 0, sizeof(struct cvi_vb_pool_cfg));
		if (copy_from_user(&cfg, p.ptr, sizeof(struct cvi_vb_pool_cfg))) {
			pr_err("VB_IOCTL_CREATE_POOL copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = vb_create_pool(&cfg);
		if (ret == 0) {
			if (copy_to_user(p.ptr, &cfg, sizeof(struct cvi_vb_pool_cfg))) {
				pr_err("VB_IOCTL_CREATE_POOL copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_DESTROY_POOL: {
		VB_POOL pool;

		pool = (VB_POOL)p.value;
		ret = vb_destroy_pool(pool);
		break;
	}

	case VB_IOCTL_GET_BLOCK: {
		struct cvi_vb_blk_cfg cfg;
		VB_BLK block;

		memset(&cfg, 0, sizeof(struct cvi_vb_blk_cfg));
		if (copy_from_user(&cfg, p.ptr, sizeof(struct cvi_vb_blk_cfg))) {
			pr_err("VB_IOCTL_GET_BLOCK copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		block = vb_get_block_with_id(cfg.pool_id, cfg.blk_size, CVI_ID_USER);
		if (block == VB_INVALID_HANDLE)
			ret = -ENOMEM;
		else {
			cfg.blk = (uint64_t)block;
			if (copy_to_user(p.ptr, &cfg, sizeof(struct cvi_vb_blk_cfg))) {
				pr_err("VB_IOCTL_GET_BLOCK copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_RELEASE_BLOCK: {
		VB_BLK blk = (VB_BLK)p.value64;

		ret = vb_release_block(blk);
		break;
	}

	case VB_IOCTL_PHYS_TO_HANDLE: {
		struct cvi_vb_blk_info blk_info;
		VB_BLK block;

		memset(&blk_info, 0, sizeof(struct cvi_vb_blk_info));
		if (copy_from_user(&blk_info, p.ptr, sizeof(struct cvi_vb_blk_info))) {
			pr_err("VB_IOCTL_PHYS_TO_HANDLE copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		block = vb_phys_addr2handle(blk_info.phy_addr);
		if (block == VB_INVALID_HANDLE)
			ret = -EINVAL;
		else {
			blk_info.blk = (uint64_t)block;
			if (copy_to_user(p.ptr, &blk_info, sizeof(struct cvi_vb_blk_info))) {
				pr_err("VB_IOCTL_PHYS_TO_HANDLE copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_BLK_INFO: {
		struct cvi_vb_blk_info blk_info;

		memset(&blk_info, 0, sizeof(struct cvi_vb_blk_info));
		if (copy_from_user(&blk_info, p.ptr, sizeof(struct cvi_vb_blk_info))) {
			pr_err("VB_IOCTL_GET_BLK_INFO copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = _vb_get_blk_info(&blk_info);
		if (ret == 0) {
			if (copy_to_user(p.ptr, &blk_info, sizeof(struct cvi_vb_blk_info))) {
				pr_err("VB_IOCTL_GET_BLK_INFO copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_POOL_CFG: {
		struct cvi_vb_pool_cfg pool_cfg;

		memset(&pool_cfg, 0, sizeof(struct cvi_vb_pool_cfg));
		if (copy_from_user(&pool_cfg, p.ptr, sizeof(struct cvi_vb_pool_cfg))) {
			pr_err("VB_IOCTL_GET_POOL_CFG copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = _vb_get_pool_cfg(&pool_cfg);
		if (ret == 0) {
			if (copy_to_user(p.ptr, &pool_cfg, sizeof(struct cvi_vb_pool_cfg))) {
				pr_err("VB_IOCTL_GET_POOL_CFG copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_POOL_MAX_CNT: {
		p.value = (int32_t)vb_max_pools;
		break;
	}

	case VB_IOCTL_PRINT_POOL: {
		VB_POOL pool;

		pool = (VB_POOL)p.value;
		ret = vb_print_pool(pool);
		break;
	}

	default:
		break;
	}
	if (copy_to_user((void __user *)arg, &p, sizeof(struct vb_ext_control)))
		return -EINVAL;
	return ret;
}

int32_t vb_create_instance(void)
{
	if (vb_max_pools < VB_MAX_COMM_POOLS) {
		pr_err("vb_max_pools is too small!\n");
		return -EINVAL;
	}

	vbPool = vzalloc(sizeof(struct vb_pool) * vb_max_pools);
	if (!vbPool) {
		pr_err("vbPool kzalloc fail!\n");
		return -ENOMEM;
	}
	pr_info("vb_max_pools(%d) vb_pool_max_blk(%d)\n", vb_max_pools, vb_pool_max_blk);

	return 0;
}

void vb_destroy_instance(void)
{
	if (vbPool) {
		vfree(vbPool);
		vbPool = NULL;
	}
}

void vb_release(void)
{
	_vb_exit();
}

#include "osal.h"
#include "vb.h"
#include "ion.h"
#include "queue.h"
#include "base_common.h"
#include "base_debug.h"
#include "comm_math.h"

#define VB_BASE_ADDR_ALIGN 4096

extern uint32_t vb_max_pools;
extern uint32_t vb_pool_max_blk;

static struct vb_cfg g_vb_config;
static struct vb_pool_ctx *g_vb_ctx;
static osal_atomic ref_count;

static osal_mutex g_lock;
static osal_mutex g_get_vb_lock;
static osal_mutex g_pool_lock;
static osal_mutex g_hash_lock;
static osal_hash *vb_hash;


static uint32_t g_show_mod_ids[] = {ID_VI, ID_VPSS, ID_VO, ID_RGN, ID_GDC,
	ID_VENC, ID_VDEC, ID_USER};


#define CHECK_VB_HANDLE_NULL(x)							\
	do {									\
		if ((x) == NULL) {						\
			TRACE_BASE(DBG_ERR, " NULL VB HANDLE\n");		\
			return ERR_VB_NULL_PTR;				\
		}								\
	} while (0)

#define CHECK_VB_HANDLE_VALID(x)						\
	do {									\
		if ((x)->magic != VB_MAGIC) {	\
			TRACE_BASE(DBG_ERR, " invalid VB Handle\n");	\
			return ERR_VB_INVALID;				\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_WEAK(x)							\
	do {									\
		if ((x) == VB_STATIC_POOLID)					\
			break;							\
		if ((x) == VB_EXTERNAL_POOLID)					\
			break;							\
		if ((x) >= (vb_max_pools)) {					\
			TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return ERR_VB_ILLEGAL_PARAM;			\
		}								\
		if (!is_pool_inited(x)) {						\
			TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", x); \
			return ERR_VB_NOTREADY;			\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_STRONG(x)							\
	do {									\
		if ((x) >= (vb_max_pools)) {					\
			TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return ERR_VB_ILLEGAL_PARAM; 		\
		}								\
		if (!is_pool_inited(x)) { 					\
			TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", x); \
			return ERR_VB_NOTREADY; 		\
		}								\
	} while (0)


static inline bool is_pool_inited(vb_pool poolid)
{
	return (g_vb_ctx[poolid].membase == 0) ? false : true;
}


static void _vb_hash_del(uint64_t phy_addr)
{
	osal_mutex_lock(&g_hash_lock);
	osal_hash_del(vb_hash, phy_addr);
	osal_mutex_unlock(&g_hash_lock);
}

static bool _vb_hash_find(uint64_t phy_addr, struct vb_s **vb)
{
	struct vb_s *obj = NULL;
	bool is_found = false;

	osal_mutex_lock(&g_hash_lock);
	obj = osal_hash_get(vb_hash, phy_addr);
	osal_mutex_unlock(&g_hash_lock);

	if (obj) {
		*vb = obj;
		is_found = true;
	}
	return is_found;
}

static bool _is_comm_vb_released(void)
{
	uint32_t i;

	for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
		if (is_pool_inited(i)) {
			if (FIFO_CAPACITY(&g_vb_ctx[i].freelist) != FIFO_SIZE(&g_vb_ctx[i].freelist)) {
				TRACE_BASE(DBG_INFO, "pool(%d) blk has not been all released yet\n", i);
				return false;
			}
		}
	}
	return true;
}

static int _hash_cb_print_pool(long long unsigned int key, void *value, void *context)
{
	int i;
	char str[64];
	vb_pool *poolid = (vb_pool *)context;
	struct vb_s *vb = (struct vb_s *)value;
	int id_max = ARRAY_SIZE(g_show_mod_ids);

	if (vb->poolid == *poolid) {
		sprintf(str, "Pool[%d] vb paddr:0x%llx usr_cnt(%d) /",
			vb->poolid, (unsigned long long)vb->phy_addr, osal_atomic_read(&vb->usr_cnt));

		for (i = 0; i < id_max; ++i) {
			if (osal_atomic_read(&vb->mod_ids) & BIT(g_show_mod_ids[i])) {
				strncat(str, sys_get_modname(g_show_mod_ids[i]), sizeof(str) - 1);
				strcat(str, "/");
			}
		}
		TRACE_BASE(DBG_INFO, "%s\n", str);
	}

	return OSAL_SUCCESS;
}

static int _hash_cb_free_vb(long long unsigned int key, void *value, void *context)
{
	struct vb_s *vb = (struct vb_s *)value;

	if ((vb->poolid >= VB_MAX_COMM_POOLS) && (vb->poolid < vb_max_pools))
		return OSAL_SUCCESS;
	if (vb->poolid == VB_STATIC_POOLID)
		base_ion_free(vb->phy_addr);
	osal_hash_del(vb_hash, key);
	osal_vfree(vb);

	return OSAL_SUCCESS;
}

int32_t vb_print_pool(vb_pool poolid)
{
	CHECK_VB_POOL_VALID_STRONG(poolid);

	osal_mutex_lock(&g_hash_lock);
	osal_hash_for_each(vb_hash, _hash_cb_print_pool, &poolid);
	osal_mutex_unlock(&g_hash_lock);

	return 0;
}
osal_module_export(vb_print_pool);

static void _vb_cleanup(void)
{
	int i;
	struct vb_pool_ctx *pool_ctx;
	struct vb_req *req, *req_tmp;

	// free vb pool
	for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
		if (is_pool_inited(i)) {
			pool_ctx = &g_vb_ctx[i];
			osal_mutex_lock(&pool_ctx->lock);
			FIFO_EXIT(&pool_ctx->freelist);
			base_ion_free(pool_ctx->membase);
			osal_mutex_unlock(&pool_ctx->lock);
			osal_mutex_destroy(&pool_ctx->lock);
			// free reqq
			osal_mutex_lock(&pool_ctx->reqq_lock);
			if (!STAILQ_EMPTY(&pool_ctx->reqq)) {
				STAILQ_FOREACH_SAFE(req, &pool_ctx->reqq, stailq, req_tmp) {
					STAILQ_REMOVE(&pool_ctx->reqq, req, vb_req, stailq);
					osal_kfree(req);
				}
			}
			osal_mutex_unlock(&pool_ctx->reqq_lock);
			osal_mutex_destroy(&pool_ctx->reqq_lock);
			memset(pool_ctx, 0, sizeof(struct vb_pool_ctx));
		}
	}

	// free comm vb blk
	osal_mutex_lock(&g_hash_lock);
	osal_hash_for_each_safe(vb_hash, _hash_cb_free_vb, NULL);
	osal_mutex_unlock(&g_hash_lock);

}

static int32_t _vb_create_pool(struct vb_pool_cfg *config, bool is_comm)
{
	uint32_t pool_size;
	struct vb_s *p;
	bool is_cache;
	char ion_name[10];
	int32_t ret, i;
	vb_pool pool_id = config->pool_id;
	void *ion_v = NULL;
	struct vb_pool_ctx *pool_ctx;
	uint32_t base_align_byte = VB_BASE_ADDR_ALIGN;
#ifdef __linux__
	uint32_t align_padding_byte = 0;
#else
	uint32_t align_padding_byte = VB_BASE_ADDR_ALIGN;
#endif


	pool_ctx = &g_vb_ctx[pool_id];
	pool_size = config->blk_size * config->blk_cnt + align_padding_byte;
	is_cache = (config->remap_mode == VB_REMAP_MODE_CACHED);

	snprintf(ion_name, 10, "VbPool%d", pool_id);
	ret = base_ion_alloc(&pool_ctx->membase, &ion_v, (uint8_t *)ion_name, pool_size, is_cache);
	if (ret) {
		TRACE_BASE(DBG_ERR, "base_ion_alloc fail! ret(%d)\n", ret);
		return ret;
	}

#ifdef __linux__
	if (pool_ctx->membase & (base_align_byte - 1)) {
		TRACE_BASE(DBG_ERR, "ion is not 4K align, addr(%#lx)\n", (unsigned long)pool_ctx->membase);
		base_ion_free(pool_ctx->membase);
		pool_ctx->membase = 0;
		return -1;
	}
	pool_ctx->mem_base_align = pool_ctx->membase;
#else
	pool_ctx->mem_base_align = ALIGN(pool_ctx->membase, base_align_byte);
	ion_v += pool_ctx->mem_base_align - pool_ctx->membase;
#endif
	config->mem_base = pool_ctx->mem_base_align;

	STAILQ_INIT(&pool_ctx->reqq);
	osal_mutex_init(&pool_ctx->reqq_lock);
	osal_mutex_init(&pool_ctx->lock);
	osal_mutex_lock(&pool_ctx->lock);
	pool_ctx->poolid = pool_id;
	pool_ctx->ownerid = (is_comm) ? POOL_OWNER_COMMON : POOL_OWNER_PRIVATE;
	pool_ctx->vmembase = ion_v;
	pool_ctx->blk_cnt = config->blk_cnt;
	pool_ctx->blk_size = config->blk_size;
	pool_ctx->remap_mode = config->remap_mode;
	pool_ctx->is_comm_pool = is_comm;
	pool_ctx->is_ex_pool = false;
	pool_ctx->free_blk_cnt = config->blk_cnt;
	pool_ctx->min_free_blk_cnt = pool_ctx->free_blk_cnt;
	if (strlen((char *)config->pool_name) != 0)
		strncpy((char *)pool_ctx->pool_name, (char *)config->pool_name,
			sizeof(pool_ctx->pool_name));
	else
		strncpy(pool_ctx->pool_name, "vbpool", sizeof(pool_ctx->pool_name));
	pool_ctx->pool_name[VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&pool_ctx->freelist, pool_ctx->blk_cnt);
	for (i = 0; i < pool_ctx->blk_cnt; ++i) {
		p = osal_vzalloc(sizeof(*p));
		memset(&p->buf, 0, sizeof(p->buf));
		p->phy_addr = pool_ctx->mem_base_align + (i * pool_ctx->blk_size);
		p->vir_addr = pool_ctx->vmembase + (p->phy_addr - pool_ctx->mem_base_align);
		p->poolid = pool_id;
		osal_atomic_set(&p->usr_cnt, 0);
		p->magic = VB_MAGIC;
		osal_atomic_set(&p->mod_ids, 0);
		p->external = false;
		FIFO_PUSH(&pool_ctx->freelist, p);
		osal_mutex_lock(&g_hash_lock);
		osal_hash_add(vb_hash, p->phy_addr, p);
		osal_mutex_unlock(&g_hash_lock);
	}
	osal_mutex_unlock(&pool_ctx->lock);

	return 0;
}

static int32_t _vb_create_ex_pool(struct vb_pool_ex_cfg *config)
{
	struct vb_s *p;
	int32_t i;
	vb_pool pool_id = config->pool_id;
	struct vb_pool_ctx *pool_ctx;

	pool_ctx = &g_vb_ctx[pool_id];

	STAILQ_INIT(&pool_ctx->reqq);
	osal_mutex_init(&pool_ctx->reqq_lock);
	osal_mutex_init(&pool_ctx->lock);
	osal_mutex_lock(&pool_ctx->lock);
	pool_ctx->poolid = pool_id;
	pool_ctx->ownerid = POOL_OWNER_PRIVATE;
	pool_ctx->membase = config->addr_p[0][0];
	pool_ctx->mem_base_align = ALIGN(pool_ctx->membase, VB_BASE_ADDR_ALIGN);
	pool_ctx->vmembase = 0;
	pool_ctx->blk_cnt = config->blk_cnt;
	pool_ctx->blk_size = 0xffffffff;
	pool_ctx->remap_mode = 0;
	pool_ctx->is_comm_pool = false;
	pool_ctx->is_ex_pool = true;
	pool_ctx->free_blk_cnt = config->blk_cnt;
	pool_ctx->min_free_blk_cnt = pool_ctx->free_blk_cnt;
	strncpy(pool_ctx->pool_name, "vbpoolex", sizeof(pool_ctx->pool_name));
	pool_ctx->pool_name[VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&pool_ctx->freelist, pool_ctx->blk_cnt);
	for (i = 0; i < pool_ctx->blk_cnt; ++i) {
		p = osal_vzalloc(sizeof(*p));
		memset(&p->buf, 0, sizeof(p->buf));
		p->phy_addr = config->addr_p[i][0];
		p->vir_addr = 0;
		p->poolid = pool_id;
		osal_atomic_set(&p->usr_cnt, 0);
		p->magic = VB_MAGIC;
		osal_atomic_set(&p->mod_ids, 0);
		p->external = true;
		p->buf.phy_addr[0] = config->addr_p[i][0];
		p->buf.phy_addr[1] = config->addr_p[i][1];
		p->buf.phy_addr[2] = config->addr_p[i][2];
		FIFO_PUSH(&pool_ctx->freelist, p);
		osal_mutex_lock(&g_hash_lock);
		osal_hash_add(vb_hash, p->phy_addr, p);
		osal_mutex_unlock(&g_hash_lock);
	}
	osal_mutex_unlock(&pool_ctx->lock);

	return 0;
}

static int32_t _vb_destroy_pool(vb_pool poolid)
{
	struct vb_pool_ctx *pool_ctx = &g_vb_ctx[poolid];
	struct vb_s *vb;
	struct vb_req *req, *req_tmp;

	TRACE_BASE(DBG_INFO, "vb destroy pool, pool[%d]: capacity(%d) size(%d).\n"
		, poolid, FIFO_CAPACITY(&pool_ctx->freelist), FIFO_SIZE(&pool_ctx->freelist));
	if (FIFO_CAPACITY(&pool_ctx->freelist) != FIFO_SIZE(&pool_ctx->freelist)) {
		TRACE_BASE(DBG_INFO, "pool(%d) blk should be all released before destroy pool\n", poolid);
		vb_print_pool(pool_ctx->poolid);
		return -1;
	}

	osal_mutex_lock(&pool_ctx->lock);
	while (!FIFO_EMPTY(&pool_ctx->freelist)) {
		FIFO_POP(&pool_ctx->freelist, &vb);
		_vb_hash_del(vb->phy_addr);
		osal_vfree(vb);
	}
	FIFO_EXIT(&pool_ctx->freelist);
	if (!pool_ctx->is_ex_pool)
		base_ion_free(pool_ctx->membase);
	osal_mutex_unlock(&pool_ctx->lock);
	osal_mutex_destroy(&pool_ctx->lock);

	// free reqq
	osal_mutex_lock(&pool_ctx->reqq_lock);
	if (!STAILQ_EMPTY(&pool_ctx->reqq)) {
		STAILQ_FOREACH_SAFE(req, &pool_ctx->reqq, stailq, req_tmp) {
			STAILQ_REMOVE(&pool_ctx->reqq, req, vb_req, stailq);
			osal_kfree(req);
		}
	}
	osal_mutex_unlock(&pool_ctx->reqq_lock);
	osal_mutex_destroy(&pool_ctx->reqq_lock);

	memset(pool_ctx, 0, sizeof(struct vb_pool_ctx));
	return 0;
}

int32_t vb_init(void)
{
	uint32_t i;
	int32_t ret;

	osal_mutex_lock(&g_lock);
	if (osal_atomic_read(&ref_count) == 0) {
		for (i = 0; i < g_vb_config.comm_pool_cnt; ++i) {
			g_vb_config.comm_pool[i].pool_id = i;
			ret = _vb_create_pool(&g_vb_config.comm_pool[i], true);
			if (ret) {
				TRACE_BASE(DBG_ERR, "_vb_create_pool fail, ret(%d)\n", ret);
				goto VB_INIT_FAIL;
			}
		}

		TRACE_BASE(DBG_INFO, "_vb_init -\n");
	}
	osal_atomic_inc_return(&ref_count);
	osal_mutex_unlock(&g_lock);
	return 0;

VB_INIT_FAIL:
	for (i = 0; i < g_vb_config.comm_pool_cnt; ++i) {
		if (is_pool_inited(i))
			_vb_destroy_pool(i);
	}

	osal_mutex_unlock(&g_lock);
	return ret;
}

int32_t vb_exit(void)
{
	int i;

	osal_mutex_lock(&g_lock);
	if (osal_atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_INFO, "vb has already exited\n");
		osal_mutex_unlock(&g_lock);
		return 0;
	}
	if (osal_atomic_dec_return(&ref_count) > 0) {
		osal_mutex_unlock(&g_lock);
		return 0;
	}

	if (!_is_comm_vb_released()) {
		TRACE_BASE(DBG_INFO, "vb has not been all released\n");
		for (i = 0; i < VB_MAX_COMM_POOLS; ++i) {
			if (is_pool_inited(i))
				vb_print_pool(i);
		}
	}
	_vb_cleanup();
	osal_mutex_unlock(&g_lock);
	TRACE_BASE(DBG_INFO, "_vb_exit -\n");
	return 0;
}

vb_pool find_vb_pool(uint32_t blk_size)
{
	vb_pool poolid = VB_INVALID_POOLID;
	int i;

	for (i = 0; i < VB_COMM_POOL_MAX_CNT; ++i) {
		if (!is_pool_inited(i))
			continue;
		if (g_vb_ctx[i].ownerid != POOL_OWNER_COMMON)
			continue;
		if (blk_size > g_vb_ctx[i].blk_size)
			continue;
		if ((poolid == VB_INVALID_POOLID)
			|| (g_vb_ctx[poolid].blk_size > g_vb_ctx[i].blk_size))
			poolid = i;
	}
	return poolid;
}
osal_module_export(find_vb_pool);

static vb_blk _vb_get_block_static(uint32_t blk_size)
{
	int32_t ret = 0;
	uint64_t phy_addr = 0;
	void *ion_v = NULL;

	//allocate with ion
	ret = base_ion_alloc(&phy_addr, &ion_v, (uint8_t *)"static_pool", blk_size, true);
	if (ret) {
		TRACE_BASE(DBG_ERR, "base_ion_alloc fail! ret(%d)\n", ret);
		return VB_INVALID_HANDLE;
	}

	return vb_create_block(phy_addr, ion_v, VB_STATIC_POOLID, false);
}

/* _vb_get_block: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk.
 * @param blk_size: the size of vb_blk to acquire.
 * @param modId: the Id of mod which acquire this blk
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
static vb_blk _vb_get_block(struct vb_pool_ctx *pool_ctx, u32 blk_size, mod_id_e mod_id)
{
	struct vb_s *p;

	if (blk_size > pool_ctx->blk_size) {
		TRACE_BASE(DBG_ERR, "PoolID(%#x) blksize(%d) > pool's(%d).\n"
			, pool_ctx->poolid, blk_size, pool_ctx->blk_size);
		return VB_INVALID_HANDLE;
	}

	osal_mutex_lock(&pool_ctx->lock);
	if (FIFO_EMPTY(&pool_ctx->freelist)) {
		TRACE_BASE(DBG_INFO, "vb_pool owner(%#x) poolid(%#x) pool is empty.\n",
			pool_ctx->ownerid, pool_ctx->poolid);
		osal_mutex_unlock(&pool_ctx->lock);
		vb_print_pool(pool_ctx->poolid);
		return VB_INVALID_HANDLE;
	}

	FIFO_POP(&pool_ctx->freelist, &p);
	pool_ctx->free_blk_cnt--;
	pool_ctx->min_free_blk_cnt =
		(pool_ctx->free_blk_cnt < pool_ctx->min_free_blk_cnt) ?
		pool_ctx->free_blk_cnt : pool_ctx->min_free_blk_cnt;
	osal_atomic_set(&p->usr_cnt, 1);
	vb_add_tag((vb_blk)(uintptr_t)p, mod_id);
	osal_mutex_unlock(&pool_ctx->lock);
	TRACE_BASE(DBG_DEBUG, "Mod(%s) phy-addr(%#llx).\n", sys_get_modname(mod_id), p->phy_addr);
	return (vb_blk)(uintptr_t)p;
}

int32_t vb_get_blk_info(struct vb_blk_info *blk_info)
{
	vb_blk blk = (vb_blk)blk_info->blk;
	struct vb_s *vb;

	vb = (struct vb_s *)(uintptr_t)blk;
	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	blk_info->phy_addr = vb->phy_addr;
	blk_info->pool_id = vb->poolid;
	blk_info->usr_cnt = osal_atomic_read(&vb->usr_cnt);
	return 0;
}

int32_t vb_get_pool_cfg(struct vb_pool_cfg *pool_cfg)
{
	vb_pool poolid = pool_cfg->pool_id;

	if (osal_atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_ERR, "vb module hasn't inited yet.\n");
		return VB_INVALID_POOLID;
	}
	CHECK_VB_POOL_VALID_STRONG(poolid);

	pool_cfg->blk_cnt = g_vb_ctx[poolid].blk_cnt;
	pool_cfg->blk_size = g_vb_ctx[poolid].blk_size;
	pool_cfg->remap_mode = g_vb_ctx[poolid].remap_mode;
	pool_cfg->mem_base = g_vb_ctx[poolid].membase;

	return 0;
}

uint32_t vb_get_pool_max_cnt(void)
{
	return vb_max_pools;
}

/**************************************************************************
 *	 global APIs.
 **************************************************************************/
int32_t vb_get_pool_info(struct vb_pool_ctx **pool_info, uint32_t *max_pool, uint32_t *max_blk)
{
	CHECK_VB_HANDLE_NULL(pool_info);
	CHECK_VB_HANDLE_NULL(g_vb_ctx);

	*pool_info = g_vb_ctx;
	*max_pool = vb_max_pools;
	*max_blk = vb_pool_max_blk;

	return 0;
}

void vb_cleanup(void)
{
	osal_mutex_lock(&g_lock);
	if (osal_atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_INFO, "vb has already exited\n");
		osal_mutex_unlock(&g_lock);
		return;
	}
	_vb_cleanup();
	osal_atomic_set(&ref_count, 0);
	osal_mutex_unlock(&g_lock);
	TRACE_BASE(DBG_INFO, "vb_cleanup done\n");
}

int32_t vb_set_config(struct vb_cfg *vb_cfg)
{
	int i;

	if (osal_atomic_read(&ref_count)) {
		TRACE_BASE(DBG_ERR, "vb has already inited, set_config cmd has no effect\n");
		return ERR_VB_NOT_PERM;
	}

	if (vb_cfg->comm_pool_cnt > VB_COMM_POOL_MAX_CNT) {
		TRACE_BASE(DBG_ERR, "Invalid comm_pool_cnt(%d)\n", vb_cfg->comm_pool_cnt);
		return ERR_VB_ILLEGAL_PARAM;
	}

	for (i = 0; i < vb_cfg->comm_pool_cnt; ++i) {
		if (vb_cfg->comm_pool[i].blk_size == 0
			|| vb_cfg->comm_pool[i].blk_cnt == 0
			|| vb_cfg->comm_pool[i].blk_cnt > vb_pool_max_blk) {
			TRACE_BASE(DBG_ERR, "Invalid pool cfg, pool(%d), blk_size(%d), blk_cnt(%d)\n",
				i, vb_cfg->comm_pool[i].blk_size,
				vb_cfg->comm_pool[i].blk_cnt);
			return ERR_VB_ILLEGAL_PARAM;
		}
	}
	g_vb_config = *vb_cfg;

	return 0;
}

int32_t vb_get_config(struct vb_cfg *vb_config)
{
	if (!vb_config) {
		TRACE_BASE(DBG_ERR, "vb_get_config NULL ptr!\n");
		return ERR_VB_NULL_PTR;
	}

	*vb_config = g_vb_config;
	return 0;
}
osal_module_export(vb_get_config);

int32_t vb_create_pool(struct vb_pool_cfg *config)
{
	uint32_t i;
	int32_t ret;

	config->pool_id = VB_INVALID_POOLID;
	if ((config->blk_size == 0) || (config->blk_cnt == 0)
		|| (config->blk_cnt > vb_pool_max_blk)) {
		TRACE_BASE(DBG_ERR, "Invalid pool cfg, blk_size(%d), blk_cnt(%d)\n",
				config->blk_size, config->blk_cnt);
		return ERR_VB_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&g_pool_lock);
	for (i = VB_MAX_COMM_POOLS; i < vb_max_pools; ++i) {
		if (!is_pool_inited(i))
			break;
	}
	if (i >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, "Exceed vb_max_pools cnt: %d\n", vb_max_pools);
		osal_mutex_unlock(&g_pool_lock);
		return ERR_VB_BUSY;
	}

	config->pool_id = i;
	ret = _vb_create_pool(config, false);
	if (ret) {
		TRACE_BASE(DBG_ERR, "_vb_create_pool fail, ret(%d)\n", ret);
		osal_mutex_unlock(&g_pool_lock);
		return ret;
	}
	osal_mutex_unlock(&g_pool_lock);
	return 0;
}
osal_module_export(vb_create_pool);

int32_t vb_create_ex_pool(struct vb_pool_ex_cfg *config)
{
	uint32_t i;
	int32_t ret;

	config->pool_id = VB_INVALID_POOLID;
	if ((config->blk_cnt == 0) || (config->blk_cnt > vb_pool_max_blk)) {
		TRACE_BASE(DBG_ERR, "Invalid pool cfg, blk_cnt(%d)\n", config->blk_cnt);
		return ERR_VB_ILLEGAL_PARAM;
	}

	if (osal_atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_ERR, "vb module hasn't inited yet.\n");
		return ERR_VB_NOTREADY;
	}

	osal_mutex_lock(&g_pool_lock);
	for (i = VB_MAX_COMM_POOLS; i < vb_max_pools; ++i) {
		if (!is_pool_inited(i))
			break;
	}
	if (i >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, "Exceed vb_max_pools cnt: %d\n", vb_max_pools);
		osal_mutex_unlock(&g_pool_lock);
		return ERR_VB_BUSY;
	}

	config->pool_id = i;
	ret = _vb_create_ex_pool(config);
	if (ret) {
		TRACE_BASE(DBG_ERR, "_vb_create_ex_pool fail, ret(%d)\n", ret);
		osal_mutex_unlock(&g_pool_lock);
		return ret;
	}
	osal_mutex_unlock(&g_pool_lock);
	return 0;
}
osal_module_export(vb_create_ex_pool);

int32_t vb_destroy_pool(vb_pool pool_id)
{
	CHECK_VB_POOL_VALID_STRONG(pool_id);

	return _vb_destroy_pool(pool_id);
}
osal_module_export(vb_destroy_pool);

/* vb_create_block: create a vb blk per phy-addr given.
 *
 * @param phy_addr: phy-address of the buffer for this new vb.
 * @param vir_addr: virtual-address of the buffer for this new vb.
 * @param pool_id: the pool of the vb belonging.
 * @param is_external: if the buffer is not allocated by mmf
 */
vb_blk vb_create_block(uint64_t phy_addr, void *vir_addr, vb_pool pool_id, bool is_external)
{
	struct vb_s *p = NULL;

	p = osal_vmalloc(sizeof(*p));
	if (!p) {
		TRACE_BASE(DBG_ERR, "vmalloc failed.\n");
		return VB_INVALID_HANDLE;
	}

	memset(&p->buf, 0, sizeof(p->buf));
	p->phy_addr = phy_addr;
	p->vir_addr = vir_addr;
	p->poolid = pool_id;
	osal_atomic_set(&p->usr_cnt, 1);
	p->magic = VB_MAGIC;
	osal_atomic_set(&p->mod_ids, 0);
	p->external = is_external;
	osal_mutex_lock(&g_hash_lock);
	osal_hash_add(vb_hash, p->phy_addr, p);
	osal_mutex_unlock(&g_hash_lock);

	return (vb_blk)(uintptr_t)p;
}
osal_module_export(vb_create_block);

vb_blk vb_get_block_with_id(vb_pool pool_id, uint32_t blk_size, mod_id_e mod_id)
{
	vb_blk blk = VB_INVALID_HANDLE;

	osal_mutex_lock(&g_get_vb_lock);
	// common pool
	if (pool_id == VB_INVALID_POOLID) {
		pool_id = find_vb_pool(blk_size);
		if (pool_id == VB_INVALID_POOLID) {
			TRACE_BASE(DBG_ERR, "No valid pool for size(%d).\n", blk_size);
			goto get_vb_done;
		}
	} else if (pool_id == VB_STATIC_POOLID) {
		blk = _vb_get_block_static(blk_size);		//need not mapping pool, allocate vb block directly
		goto get_vb_done;
	} else if (pool_id >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", pool_id);
		goto get_vb_done;
	} else {
		if (!is_pool_inited(pool_id)) {
			TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", pool_id);
			goto get_vb_done;
		}

		if (blk_size > g_vb_ctx[pool_id].blk_size) {
			TRACE_BASE(DBG_ERR, "required size(%d) > pool(%d)'s blk-size(%d).\n", blk_size, pool_id,
					 g_vb_ctx[pool_id].blk_size);
			goto get_vb_done;
		}
	}
	blk = _vb_get_block(&g_vb_ctx[pool_id], blk_size, mod_id);

get_vb_done:
	osal_mutex_unlock(&g_get_vb_lock);
	return blk;
}
osal_module_export(vb_get_block_with_id);

int32_t vb_release_block(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;
	struct vb_s *vb_tmp;
	struct vb_pool_ctx *pool;
	int cnt;
	int32_t result;
	bool bReq = false;
	struct vb_req *req, *tmp;

	if (osal_atomic_read(&ref_count) == 0) {
		TRACE_BASE(DBG_ERR, "vb module hasn't inited yet.\n");
		return ERR_VB_NOTREADY;
	}

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);
	CHECK_VB_POOL_VALID_WEAK(vb->poolid);

	cnt = osal_atomic_dec_return(&vb->usr_cnt);
	if (cnt <= 0) {
		TRACE_BASE(DBG_DEBUG, "%p phy-addr(%#llx) release.\n",
			__builtin_return_address(0), vb->phy_addr);

		if ((vb->poolid == VB_EXTERNAL_POOLID) && vb->external) {
			TRACE_BASE(DBG_DEBUG, "external buffer phy-addr(%#llx) release.\n", vb->phy_addr);
			_vb_hash_del(vb->phy_addr);
			osal_vfree(vb);
			return 0;
		}

		//free VB_STATIC_POOLID
		if (vb->poolid == VB_STATIC_POOLID) {
			int32_t ret = 0;

			ret = base_ion_free(vb->phy_addr);
			_vb_hash_del(vb->phy_addr);
			osal_vfree(vb);
			return ret >= 0 ? 0 : OSAL_EINVAL;
		}

		if (cnt < 0) {
			int i = 0;

			TRACE_BASE(DBG_WARN, "vb->phy_addr(%#llx) usr_cnt is zero.\n", vb->phy_addr);
			pool = &g_vb_ctx[vb->poolid];
			osal_mutex_lock(&pool->lock);
			FIFO_FOREACH(vb_tmp, &pool->freelist, i) {
				if (vb_tmp->phy_addr == vb->phy_addr) {
					osal_mutex_unlock(&pool->lock);
					osal_atomic_set(&vb->usr_cnt, 0);
					return 0;
				}
			}
			osal_mutex_unlock(&pool->lock);
		}

		pool = &g_vb_ctx[vb->poolid];
		osal_mutex_lock(&pool->lock);
		memset(&vb->buf, 0, sizeof(vb->buf));
		osal_atomic_set(&vb->usr_cnt, 0);
		osal_atomic_set(&vb->mod_ids, 0);
		FIFO_PUSH(&pool->freelist, vb);
		++pool->free_blk_cnt;
		osal_mutex_unlock(&pool->lock);

		osal_mutex_lock(&pool->reqq_lock);
		if (!STAILQ_EMPTY(&pool->reqq)) {
			STAILQ_FOREACH_SAFE(req, &pool->reqq, stailq, tmp) {
				if (req->poolid != pool->poolid)
					continue;

				TRACE_BASE(DBG_INFO, "pool(%d) vb(%#llx) release, Try acquire vb for %s\n", pool->poolid,
					vb->phy_addr, sys_get_modname(req->chn.mod_id));
				STAILQ_REMOVE(&pool->reqq, req, vb_req, stailq);
				bReq = true;
				break;
			}
		}
		osal_mutex_unlock(&pool->reqq_lock);
		if (bReq) {
			result = req->fp(req->chn, req->data);
			if (result) { // req->fp return fail
				osal_mutex_lock(&pool->reqq_lock);
				STAILQ_INSERT_TAIL(&pool->reqq, req, stailq);
				osal_mutex_unlock(&pool->reqq_lock);
			} else
				osal_kfree(req);
		}
	}

	return 0;
}
osal_module_export(vb_release_block);

vb_blk vb_phys_addr2handle(uint64_t phy_addr)
{
	struct vb_s *vb = NULL;

	if (!_vb_hash_find(phy_addr, &vb)) {
		TRACE_BASE(DBG_DEBUG, "Cannot find vb corresponding to phyAddr:%#llx\n", phy_addr);
		return VB_INVALID_HANDLE;
	} else
		return (vb_blk)(uintptr_t)vb;
}
osal_module_export(vb_phys_addr2handle);

uint64_t vb_handle2phys_addr(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != VB_MAGIC))
		return 0;
	return vb->phy_addr;
}
osal_module_export(vb_handle2phys_addr);

void *vb_handle2virt_addr(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != VB_MAGIC))
		return NULL;
	return vb->vir_addr;
}
osal_module_export(vb_handle2virt_addr);

vb_pool vb_handle2pool_id(vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	if ((!blk) || (blk == VB_INVALID_HANDLE) || (vb->magic != VB_MAGIC))
		return VB_INVALID_POOLID;
	return vb->poolid;
}
osal_module_export(vb_handle2pool_id);

int32_t vb_inquire_user_cnt(vb_blk blk, uint32_t *cnt)
{
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	*cnt = osal_atomic_read(&vb->usr_cnt);
	return 0;
}
osal_module_export(vb_inquire_user_cnt);

/* vb_acquire_block: to register a callback to acquire vb_blk at VB_ReleaseBlock
 *						in case of VB_GetBlock failure.
 *
 * @param fp: callback to acquire blk for module.
 * @param chn: info of the module which needs this helper.
 */
void vb_acquire_block(vb_acquire_fp fp, mmf_chn_s chn, vb_pool pool_id, void *data)
{
	struct vb_req *req = NULL;

	if (pool_id == VB_INVALID_POOLID) {
		TRACE_BASE(DBG_ERR, "invalid poolid.\n");
		return;
	}
	if (pool_id >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", pool_id);
		return;
	}
	if (!is_pool_inited(pool_id)) {
		TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", pool_id);
		return;
	}

	req = osal_kmalloc(sizeof(*req), OSAL_GFP_ATOMIC);
	if (!req) {
		//TRACE_BASE(DBG_ERR, "kmalloc failed.\n");	warning2error fail
		return;
	}

	req->fp = fp;
	req->chn = chn;
	req->poolid = pool_id;
	req->data = data;

	osal_mutex_lock(&g_vb_ctx[pool_id].reqq_lock);
	STAILQ_INSERT_TAIL(&g_vb_ctx[pool_id].reqq, req, stailq);
	osal_mutex_unlock(&g_vb_ctx[pool_id].reqq_lock);
}
osal_module_export(vb_acquire_block);

void vb_cancel_block(mmf_chn_s chn, vb_pool pool_id)
{
	struct vb_req *req, *tmp;

	if (pool_id == VB_INVALID_POOLID) {
		TRACE_BASE(DBG_ERR, "invalid poolid.\n");
		return;
	}
	if (pool_id >= vb_max_pools) {
		TRACE_BASE(DBG_ERR, " invalid VB Pool(%d)\n", pool_id);
		return;
	}
	if (!is_pool_inited(pool_id)) {
		TRACE_BASE(DBG_ERR, "vb_pool(%d) isn't init yet.\n", pool_id);
		return;
	}

	osal_mutex_lock(&g_vb_ctx[pool_id].reqq_lock);
	if (!STAILQ_EMPTY(&g_vb_ctx[pool_id].reqq)) {
		STAILQ_FOREACH_SAFE(req, &g_vb_ctx[pool_id].reqq, stailq, tmp) {
			if (CHN_MATCH(&req->chn, &chn)) {
				STAILQ_REMOVE(&g_vb_ctx[pool_id].reqq, req, vb_req, stailq);
				osal_kfree(req);
			}
		}
	}
	osal_mutex_unlock(&g_vb_ctx[pool_id].reqq_lock);
}
osal_module_export(vb_cancel_block);

void vb_add_tag(vb_blk blk, mod_id_e mod_id)
{
	int32_t i;
	int32_t id_max = ARRAY_SIZE(g_show_mod_ids);
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	for (i = 0; i < id_max; i++) {
		if (g_show_mod_ids[i] == mod_id)
			break;
	}

	if (i == id_max) {
		TRACE_BASE(DBG_ERR, "mod_id(%d) error.\n", mod_id);
		return;
	}

	osal_atomic_fetch_or(BIT(g_show_mod_ids[i]), &vb->mod_ids);
}
osal_module_export(vb_add_tag);

void vb_remove_tag(vb_blk blk, mod_id_e mod_id)
{
	int32_t i = 0;
	int32_t id_max = ARRAY_SIZE(g_show_mod_ids);
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	for (i = 0; i < id_max; i++) {
		if (g_show_mod_ids[i] == mod_id)
			break;
	}

	if (i == id_max) {
		TRACE_BASE(DBG_ERR, "mod_id(%d) error.\n", mod_id);
		return;
	}

	osal_atomic_fetch_and(~BIT(g_show_mod_ids[i]), &vb->mod_ids);
}
osal_module_export(vb_remove_tag);


int32_t vb_create_instance(void)
{
	if (vb_max_pools < VB_MAX_COMM_POOLS) {
		TRACE_BASE(DBG_ERR, "vb_max_pools is too small!\n");
		return OSAL_EINVAL;
	}

	g_vb_ctx = osal_vzalloc(sizeof(struct vb_pool_ctx) * vb_max_pools);
	if (!g_vb_ctx) {
		TRACE_BASE(DBG_ERR, "g_vb_ctx kzalloc fail!\n");
		return OSAL_ENOMEM;
	}
	TRACE_BASE(DBG_INFO, "vb_max_pools(%d) vb_pool_max_blk(%d)\n", vb_max_pools, vb_pool_max_blk);

	osal_atomic_set(&ref_count, 0);
	osal_mutex_init(&g_lock);
	osal_mutex_init(&g_get_vb_lock);
	osal_mutex_init(&g_pool_lock);
	osal_mutex_init(&g_hash_lock);

	vb_hash = osal_hash_create(64);
	if (!vb_hash) {
		TRACE_BASE(DBG_ERR, "osal_hash_create fail!\n");
	}

	return 0;
}

void vb_destroy_instance(void)
{
	osal_hash_destroy(vb_hash);
	vb_hash = NULL;
	osal_mutex_destroy(&g_hash_lock);
	osal_mutex_destroy(&g_pool_lock);
	osal_mutex_destroy(&g_get_vb_lock);
	osal_mutex_destroy(&g_lock);

	if (g_vb_ctx) {
		osal_vfree(g_vb_ctx);
		g_vb_ctx = NULL;
	}
}

void vb_release(void)
{
	vb_exit();
}


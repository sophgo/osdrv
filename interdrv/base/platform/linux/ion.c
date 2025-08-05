#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/module.h>
#include <asm/cacheflush.h>
#include <ion/ion.h>
#include <ion/cvitek/cvitek_ion_alloc.h>

#include "defines.h"
#include "base_uapi.h"
#include "base_debug.h"
#include "osal.h"

struct mem_mapping {
	uint64_t phy_addr;
	int32_t size;
	void *vir_addr;
	void *dmabuf;
	pid_t fd_tgid;
};

static int ion_debug_alloc_free;
module_param(ion_debug_alloc_free, int, 0644);

static osal_spinlock ion_lock;
static osal_hash *ion_hash;


static int32_t mem_put(struct mem_mapping *mem_info)
{
	struct mem_mapping *p;

	p = osal_kmalloc(sizeof(*p), OSAL_GFP_KERNEL);
	if (!p) {
		TRACE_BASE(DBG_ERR, "kmalloc failed\n");
		return -1;
	}
	memcpy(p, mem_info, sizeof(*p));

	osal_spin_lock(&ion_lock);
	osal_hash_add(ion_hash, p->phy_addr, p);
	osal_spin_unlock(&ion_lock);

	return 0;
}

static int _hash_cb_get_mem(long long unsigned int key, void *value, void *context)
{
	struct mem_mapping *obj = (struct mem_mapping *)value;
	struct mem_mapping *mem_info = (struct mem_mapping *)context;

	if (obj->phy_addr == mem_info->phy_addr) {
		memcpy(mem_info, obj, sizeof(*mem_info));
		osal_hash_del(ion_hash, mem_info->phy_addr);
		osal_kfree(obj);
		return OSAL_FAILURE;
	}

	return OSAL_SUCCESS;
}

static int32_t mem_get(uint64_t addr_p, struct mem_mapping *mem_info)
{
	int32_t ret = -1;

	memset(mem_info, 0, sizeof(struct mem_mapping));
	mem_info->phy_addr = addr_p;

	osal_spin_lock(&ion_lock);
	osal_hash_for_each_safe(ion_hash, _hash_cb_get_mem, mem_info);
	osal_spin_unlock(&ion_lock);

	return (mem_info->size == 0) ? ret : 0;
}

static int _hash_cb_mem_dump(long long unsigned int key, void *value, void *context)
{
	struct mem_mapping *obj = (struct mem_mapping *)value;
	int32_t *cnt = (int32_t *)context;

	TRACE_BASE(DBG_INFO, "ion addr=0x%llx, ion size=%d\n", obj->phy_addr, obj->size);
	(*cnt)++;

	return OSAL_SUCCESS;
}

static int32_t mem_dump(void)
{
	int32_t cnt = 0;

	osal_spin_lock(&ion_lock);
	osal_hash_for_each(ion_hash, _hash_cb_mem_dump, &cnt);
	osal_spin_unlock(&ion_lock);

	TRACE_BASE(DBG_INFO, "ion block total=%d\n", cnt);
	return cnt;
}

static int32_t _base_ion_alloc(uint64_t *addr_p, void **addr_v, uint32_t len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t dmabuf_fd = 0, ret = 0;
	struct dma_buf *dmabuf;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	//vpp heap
	dmabuf_fd = cvi_ion_alloc(ION_HEAP_TYPE_CARVEOUT, len, is_cached);
	if (dmabuf_fd < 0) {
		TRACE_BASE(DBG_ERR, "bm_ion_alloc len=0x%x failed\n", len);
		return -ENOMEM;
	}

	dmabuf = dma_buf_get(dmabuf_fd);
	if (!dmabuf) {
		TRACE_BASE(DBG_ERR, "allocated get dmabuf failed\n");
		return -ENOMEM;
	} else {
		//free fd if get dmabuf success.
		cvi_ion_free(current->tgid, dmabuf_fd);
	}

	ionbuf = (struct ion_buffer *)dmabuf->priv;
	owner_name = vmalloc(MAX_ION_BUFFER_NAME);
	if (name)
		strncpy(owner_name, name, MAX_ION_BUFFER_NAME);
	else
		strncpy(owner_name, "anonymous", MAX_ION_BUFFER_NAME);

	ionbuf->name = owner_name;

	ret = dma_buf_begin_cpu_access(dmabuf, DMA_TO_DEVICE);
	if (ret < 0) {
		TRACE_BASE(DBG_ERR, "dma_buf_begin_cpu_access failed\n");
		dma_buf_put(dmabuf);
		return ret;
	}

	vmap_addr = ionbuf->vaddr;
	if (IS_ERR(vmap_addr)) {
		ret = -EINVAL;
		return ret;
	}

	//push into memory manager
	mem_info.dmabuf = (void *)dmabuf;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.size = len;
	mem_info.fd_tgid = current->tgid;
	if (mem_put(&mem_info)) {
		TRACE_BASE(DBG_ERR, "allocate mm put failed\n");
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		TRACE_BASE(DBG_INFO, "ion alloc: pid=%d name=%s phy_addr=0x%llx size=%d\n",
			mem_info.fd_tgid, ionbuf->name, mem_info.phy_addr, mem_info.size);
	}

	*addr_p = ionbuf->paddr;
	*addr_v = vmap_addr;

	return ret;
}

static int32_t _base_ion_free(uint64_t addr_p)
{
	struct mem_mapping mem_info;
	struct ion_buffer *ionbuf;
	struct dma_buf *dmabuf;

	//get from memory manager
	if (mem_get(addr_p, &mem_info)) {
		TRACE_BASE(DBG_ERR, "mem_info get failed, addr:0x%llx\n", addr_p);
		return -ENOMEM;
	}

	dmabuf = (struct dma_buf *)(mem_info.dmabuf);
	ionbuf = (struct ion_buffer *)dmabuf->priv;

	if (ion_debug_alloc_free) {
		TRACE_BASE(DBG_INFO, "ion free: pid=%d name=%s phy_addr=0x%llx size=%d\n",
			mem_info.fd_tgid, ionbuf->name, mem_info.phy_addr, mem_info.size);
	}

	dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
	dma_buf_put(dmabuf);

	//return free size
	return mem_info.size;
}

int32_t base_ion_free(uint64_t phy_addr)
{
	return _base_ion_free(phy_addr);
}
osal_module_export(base_ion_free);

int32_t base_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	return _base_ion_alloc(p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
osal_module_export(base_ion_alloc);

int32_t base_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t len)
{
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, len, DMA_FROM_DEVICE);
#else
	__dma_map_area(phys_to_virt(addr_p), len, DMA_FROM_DEVICE);
#endif

	/*	*/
	smp_mb();
	return 0;
}
osal_module_export(base_ion_cache_invalidate);

int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t len)
{
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, len, DMA_TO_DEVICE);
#else
	__dma_map_area(phys_to_virt(addr_p), len, DMA_TO_DEVICE);
#endif

	/*	*/
	smp_mb();
	return 0;
}
osal_module_export(base_ion_cache_flush);

int32_t base_ion_dump(void)
{
	return mem_dump();
}
osal_module_export(base_ion_dump);

int32_t base_ion_init(void)
{
	osal_spin_lock_init(&ion_lock);

	ion_hash = osal_hash_create(128);
	if (!ion_hash) {
		TRACE_BASE(DBG_ERR, "osal_hash_create fail!\n");
	}

	return 0;
}

int32_t base_ion_deinit(void)
{
	osal_hash_destroy(ion_hash);
	ion_hash = NULL;
	osal_spin_lock_destroy(&ion_lock);

	return 0;
}


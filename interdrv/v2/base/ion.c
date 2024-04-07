#include <linux/dma-buf.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-map-ops.h>
#endif

#include <linux/cvi_defines.h>
#include "linux/base_uapi.h"
#include "ion.h"
#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"
#include <linux/dma-mapping.h>

struct mem_mapping {
	uint64_t phy_addr;
	int32_t dmabuf_fd;
	int32_t size;
	void *vir_addr;
	void *dmabuf;
	pid_t fd_pid;
};


#define MEM_MAPPING_MAX 4096
static struct mem_mapping ctx_mem_mgr[MEM_MAPPING_MAX] = {0};

static int ion_debug_alloc_free;
module_param(ion_debug_alloc_free, int, 0644);

static DEFINE_SPINLOCK(mem_lock);

static int32_t mem_put(struct mem_mapping *mem_info)
{
	int32_t i = 0;
	int8_t find_hit = 0;

	spin_lock(&mem_lock);
	for (i = 0; i < MEM_MAPPING_MAX; i++) {
		if (ctx_mem_mgr[i].phy_addr == 0) {
			memcpy(&ctx_mem_mgr[i], mem_info, sizeof(struct mem_mapping));

			pr_debug("sys_ctx_mem_put() p_addr=0x%llx, fd=%d, v_addr=%p, dmabuf=%p\n",
							ctx_mem_mgr[i].phy_addr,
							ctx_mem_mgr[i].dmabuf_fd,
							ctx_mem_mgr[i].vir_addr,
							ctx_mem_mgr[i].dmabuf);

			find_hit = 1;
			break;
		}
	}
	spin_unlock(&mem_lock);

	if (!find_hit) {
		pr_err("sys_ctx_mem_put() overflow\n");
		return -1;
	}
	return 0;
}

static int32_t mem_get(struct mem_mapping *mem_info)
{
	int32_t i = 0;
	int8_t find_hit = 0;

	spin_lock(&mem_lock);
	for (i = 0; i < MEM_MAPPING_MAX; i++) {
		if (ctx_mem_mgr[i].phy_addr == mem_info->phy_addr) {
			memcpy(mem_info, &ctx_mem_mgr[i], sizeof(struct mem_mapping));
			pr_debug("sys_ctx_mem_get() p_addr=0x%llx, fd=%d, v_addr=%p, dmabuf=%p\n",
							ctx_mem_mgr[i].phy_addr,
							ctx_mem_mgr[i].dmabuf_fd,
							ctx_mem_mgr[i].vir_addr,
							ctx_mem_mgr[i].dmabuf);

			memset(&ctx_mem_mgr[i], 0, sizeof(struct mem_mapping));

			find_hit = 1;
			break;
		}
	}
	spin_unlock(&mem_lock);

	if (!find_hit) {
		pr_err("sys_ctx_mem_get() can't find it\n");
		return -1;
	}

	return 0;
}

static int32_t mem_dump(void)
{
	int32_t i = 0, cnt = 0;

	spin_lock(&mem_lock);
	for (i = 0; i < MEM_MAPPING_MAX; i++) {
		if (ctx_mem_mgr[i].phy_addr) {
			pr_err("p_addr=0x%llx, dmabuf_fd=%d\n",
				ctx_mem_mgr[i].phy_addr, ctx_mem_mgr[i].dmabuf_fd);
			cnt++;
		}
	}
	spin_unlock(&mem_lock);

	pr_err("sys_ctx_mem_dump() total=%d\n", cnt);
	return cnt;
}

static int32_t _base_ion_alloc(uint64_t *addr_p, void **addr_v, uint32_t u32Len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t dmabuf_fd = 0, ret = 0;
	struct dma_buf *dmabuf;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	//vpp heap
	dmabuf_fd = bm_ion_alloc(0x1, u32Len, is_cached);
	if (dmabuf_fd < 0) {
		pr_err("bm_ion_alloc len=0x%x failed\n", u32Len);
		return -ENOMEM;
	}

	dmabuf = dma_buf_get(dmabuf_fd);
	if (!dmabuf) {
		pr_err("allocated get dmabuf failed\n");
		return -ENOMEM;
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
		pr_err("cvi_ion_alloc() dma_buf_begin_cpu_access failed\n");
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
	mem_info.dmabuf_fd = dmabuf_fd;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.fd_pid = current->pid;
	if (mem_put(&mem_info)) {
		pr_err("allocate mm put failed\n");
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		pr_info("%s: ion alloc: name=%s\n", __func__, ionbuf->name);
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_pid=%d\n", __func__, mem_info.fd_pid);
		pr_info("%s: current->pid=%d\n", __func__, current->pid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
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
	int ret = 0;

	//get from memory manager
	memset(&mem_info, 0, sizeof(struct mem_mapping));
	mem_info.phy_addr = addr_p;
	if (mem_get(&mem_info)) {
		pr_err("dmabuf_fd get failed, addr:0x%llx\n", addr_p);
		return -ENOMEM;
	} else {
		ret = mem_info.size;
	}

	dmabuf = (struct dma_buf *)(mem_info.dmabuf);
	ionbuf = (struct ion_buffer *)dmabuf->priv;

	if (ion_debug_alloc_free) {
		pr_info("%s: ion free: name=%s\n", __func__, ionbuf->name);
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_pid=%d\n", __func__, mem_info.fd_pid);
		pr_info("%s: current->pid=%d\n", __func__, current->pid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}

	dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
	dma_buf_put(dmabuf);

	bm_ion_free(mem_info.dmabuf_fd);

	return ret;
}

int32_t base_ion_free(uint64_t u64PhyAddr)
{
	return _base_ion_free(u64PhyAddr);
}
EXPORT_SYMBOL_GPL(base_ion_free);

int32_t base_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	return _base_ion_alloc(p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
EXPORT_SYMBOL_GPL(base_ion_alloc);

int32_t base_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len)
{
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, u32Len, DMA_FROM_DEVICE);
#else
	__dma_map_area(phys_to_virt(addr_p), u32Len, DMA_FROM_DEVICE);
#endif

	/*	*/
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(base_ion_cache_invalidate);

int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len)
{
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, u32Len, DMA_TO_DEVICE);
#else
	__dma_map_area(phys_to_virt(addr_p), u32Len, DMA_TO_DEVICE);
#endif

	/*	*/
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(base_ion_cache_flush);

int32_t base_ion_dump(void)
{
	return mem_dump();
}
EXPORT_SYMBOL_GPL(base_ion_dump);


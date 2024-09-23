#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "sys_context.h"
#include "sys.h"

#define MEM_MAPPING_MAX 100
static struct mem_mapping ctx_mem_mgr[MEM_MAPPING_MAX] = {0};

int32_t sys_ctx_init(void)
{
	memset(ctx_mem_mgr, 0, sizeof(struct mem_mapping) * MEM_MAPPING_MAX);

	return 0;
}

int32_t sys_ctx_mem_put(struct mem_mapping *mem_info)
{
	int32_t i = 0;
	int8_t find_hit = 0;

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

	if (!find_hit) {
		pr_err("sys_ctx_mem_put() overflow\n");
		return -1;
	}
	return 0;
}

int32_t sys_ctx_mem_get(struct mem_mapping *mem_info)
{
	int32_t i = 0;
	int8_t find_hit = 0;

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

	if (!find_hit) {
		pr_err("sys_ctx_mem_get() can't find it\n");
		return -1;
	}

	return 0;
}

int32_t sys_ctx_mem_dump(void)
{
	int32_t i = 0, cnt = 0;

	for (i = 0; i < MEM_MAPPING_MAX; i++) {
		if (ctx_mem_mgr[i].phy_addr) {
			pr_err("p_addr=0x%llx, dmabuf_fd=%d\n",
				ctx_mem_mgr[i].phy_addr, ctx_mem_mgr[i].dmabuf_fd);

			cnt ++;
		}
	}

	pr_err("sys_ctx_mem_dump() total=%d\n", cnt);
	return cnt;
}


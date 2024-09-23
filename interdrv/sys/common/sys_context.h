#ifndef __SYS_CONTEXT_H__
#define __SYS_CONTEXT_H__

#include <linux/types.h>


struct mem_mapping {
	uint64_t phy_addr;
	int32_t dmabuf_fd;
	void *vir_addr;
	void *dmabuf;
	pid_t fd_tgid;
	void *ionbuf;
};

int32_t sys_ctx_init(void);
int32_t sys_ctx_mem_put(struct mem_mapping *mem_config);
int32_t sys_ctx_mem_get(struct mem_mapping *mem_config);
int32_t sys_ctx_mem_dump(void);


#endif  /* __SYS_CONTEXT_H__ */


#ifndef MODULES_COMMON_INCLUDE_DEVMEM_H_
#define MODULES_COMMON_INCLUDE_DEVMEM_H_

#include <stdint.h>

int devm_open(void);
void devm_close(int fd);
void *devm_map(int fd, uint64_t addr, int len);
void devm_unmap(void *virt_addr, int len);

#endif				// MODULES_COMMON_INCLUDE_DEVMEM_H_

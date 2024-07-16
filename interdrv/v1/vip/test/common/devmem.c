/*
 ** read/write phy addr in userspace
 ** open /dev/mem
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

#include "devmem.h"

int devm_open(void)
{
	int fd;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1) {
		printf("cannot open '/dev/mem'\n");
		goto open_err;
	}

	return fd;
open_err:
	return -1;
}

void devm_close(int fd)
{
	if (fd)
		close(fd);
}

void *devm_map(int fd, uint64_t phy_addr, int len)
{
	off_t offset;
	void *map_base;

	offset = phy_addr & ~(sysconf(_SC_PAGE_SIZE) - 1);

	map_base = mmap(NULL, len + phy_addr - offset, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, offset);
	if (map_base == MAP_FAILED) {
		printf("mmap failed\n");
		goto mmap_err;
	}

	return (map_base + (phy_addr - offset));

mmap_err:
	return NULL;
}

void devm_unmap(void *virt_addr, int len)
{
	uint64_t addr;

	/* page align */
	addr = (((uint64_t)(uintptr_t)virt_addr) & ~(sysconf(_SC_PAGE_SIZE) - 1));
	munmap((void *)(uintptr_t)addr, len + (unsigned long) virt_addr - addr);
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <stdint.h>
#include <string.h>

#define u32 uint32_t

#include "sophon_spacc.h"

#define POOL_SIZE (100 * 1024)
#define DATA_SIZE (60 * 1024)

int main(int argc, char **args)
{
	int fd;
	unsigned int result_size;
	int ret;
	unsigned int pool_size = POOL_SIZE;
	char buf[POOL_SIZE] = {0};
	struct cvi_spacc_base64 b64 = {0, 1};

	fd = open("/dev/spacc", O_RDWR);
	if (fd < 0) {
		printf("open /dev/spacc failed\n");
		return -1;
	}

	if (ioctl(fd, IOCTL_SPACC_CREATE_POOL, &pool_size)) {
		printf("ioctl failed\n");
		return -1;
	}

	pool_size = 0;
	ioctl(fd, IOCTL_SPACC_GET_POOL_SIZE, &pool_size);
	printf("pool size: %d\n", pool_size);

	memcpy(buf, "hello", 5);

	// Encode
	write(fd, buf, DATA_SIZE);

	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	// Decode
	write(fd, buf, result_size);
	b64.action = 0;

	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);
	buf[result_size] = 0;

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	// Customer code Encode
	buf[0] = 0xFB;
	buf[1] = 0xEF;
	buf[2] = 0xBE;

	write(fd, buf, 3);

	b64.customer_code = ('!' << 8) | '#';
	b64.action = 1;
	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);
	buf[result_size] = 0;

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	// Customer code Decode
	memcpy(buf, "####", 4);

	write(fd, buf, 4);

	b64.customer_code = ('!' << 8) | '#';
	b64.action = 0;
	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);
	buf[result_size] = 0;

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	int i = 0;

	for (; i < 3; i++)
		printf("0x%x ", buf[i]);

	printf("\n");
	close(fd);
	return 0;
}


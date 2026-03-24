#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/* The following definitions are based on the kernel's uapi/linux/rpmsg.h */
struct rpmsg_endpoint_info {
	char name[32];
	unsigned int src;
	unsigned int dst;
};

#define RPMSG_CREATE_EPT_IOCTL _IOW(0xb5, 0x1, struct rpmsg_endpoint_info)

int main(int argc, char *argv[])
{
	int fd, rpmsg_fd, ret;
	struct rpmsg_endpoint_info ept_info;
	char rpmsg_dev_name[64];
	char *rpmsg_ctrl_dev = "/dev/rpmsg_ctrl0";
	char *endpoint_name = "rpmsg-client-sample";
	char *message = "hello world\0";

	printf("Opening %s\n", rpmsg_ctrl_dev);
	fd = open(rpmsg_ctrl_dev, O_RDWR);
	if (fd < 0) {
		perror("Failed to open rpmsg_ctrl0");
		return -1;
	}

	printf("Creating endpoint %s\n", endpoint_name);
	strcpy(ept_info.name, endpoint_name);
	ept_info.src = -1; /* Let the kernel assign a source address */
	ept_info.dst = 0; /* Let the kernel assign a destination address */

	ret = ioctl(fd, RPMSG_CREATE_EPT_IOCTL, &ept_info);
	if (ret < 0) {
		perror("Failed to create endpoint");
		close(fd);
		return -1;
	}

	sprintf(rpmsg_dev_name, "/dev/rpmsg%d", ret);
	printf("Opening %s\n", rpmsg_dev_name);

	rpmsg_fd = open(rpmsg_dev_name, O_RDWR);
	if (rpmsg_fd < 0) {
		perror("Failed to open rpmsg device");
		close(fd);
		return -1;
	}

	printf("Sending message: '%s'\n", message);
	for (int i = 0; i < 10; i++) {
		ret = write(rpmsg_fd, message, strlen(message) + 1);
		if (ret < 0) {
			perror("Failed to write to rpmsg device");
		} else {
			printf("Sent %d bytes\n", ret);
		}
	sleep(2);
	}
	printf("Closing devices\n");
	close(rpmsg_fd);
	close(fd);

	return 0;
}
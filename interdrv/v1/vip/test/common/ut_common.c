#include "ut_common.h"

static enum cvi_errno ioctl_errno(uint32_t req)
{
	enum cvi_errno ret = ERR_NONE;

	switch (req) {
	case VIDIOC_TRY_EXT_CTRLS:
		ret = ERR_IOCTL_VIDIOC_TRY_EXT_CTRLS;
		break;
	case VIDIOC_G_EXT_CTRLS:
		ret = ERR_IOCTL_VIDIOC_G_EXT_CTRLS;
		break;
	case VIDIOC_S_EXT_CTRLS:
		ret = ERR_IOCTL_VIDIOC_S_EXT_CTRLS;
		break;
	case VIDIOC_DQEVENT:
		ret = ERR_IOCTL_VIDIOC_DQEVENT;
		break;
	case VIDIOC_DQBUF:
		ret = ERR_IOCTL_VIDIOC_DQBUF;
		break;
	case VIDIOC_QBUF:
		ret = ERR_IOCTL_VIDIOC_QBUF;
		break;
	case VIDIOC_QUERYCAP:
		ret = ERR_IOCTL_VIDIOC_QUERYCAP;
		break;
	case VIDIOC_UNSUBSCRIBE_EVENT:
		ret = ERR_IOCTL_VIDIOC_UNSUBSCRIBE_EVENT;
		break;
	case VIDIOC_SUBSCRIBE_EVENT:
		ret = ERR_IOCTL_VIDIOC_SUBSCRIBE_EVENT;
		break;
	case VIDIOC_S_FMT:
		ret = ERR_IOCTL_VIDIOC_S_FMT;
		break;
	case VIDIOC_TRY_FMT:
		ret = ERR_IOCTL_VIDIOC_TRY_FMT;
		break;
	case VIDIOC_STREAMOFF:
		ret = ERR_IOCTL_VIDIOC_STREAMOFF;
		break;
	case VIDIOC_STREAMON:
		ret = ERR_IOCTL_VIDIOC_STREAMON;
		break;
	default:
		ut_pr(UT_ERR, "Can't find the ioctl_req(%d)", req);
		ret = ERR_IOCTL_NO_REQ;
		break;
	}

	return ret;
}

enum cvi_errno ut_v4l2_cmd(
	int fd,
	uint32_t req,
	void *data)
{
	enum cvi_errno ret_2 = ERR_NONE;
	int ret = 0;

	switch (req) {
	case VIDIOC_TRY_EXT_CTRLS:
	case VIDIOC_G_EXT_CTRLS:
	case VIDIOC_S_EXT_CTRLS: {
		struct v4l2_ext_controls *sub;

		sub = (struct v4l2_ext_controls *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	case VIDIOC_DQEVENT: {
		struct v4l2_event *sub;

		sub = (struct v4l2_event *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	case VIDIOC_DQBUF:
	case VIDIOC_QBUF: {
		struct v4l2_buffer *sub;

		sub = (struct v4l2_buffer *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	case VIDIOC_QUERYCAP: {
		struct v4l2_capability *sub;

		sub = (struct v4l2_capability *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	case VIDIOC_UNSUBSCRIBE_EVENT:
	case VIDIOC_SUBSCRIBE_EVENT: {
		struct v4l2_event_subscription *sub;

		sub = (struct v4l2_event_subscription *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	case VIDIOC_S_FMT:
	case VIDIOC_TRY_FMT: {
		struct v4l2_format *sub;

		sub = (struct v4l2_format *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	case VIDIOC_STREAMOFF:
	case VIDIOC_STREAMON: {
		int *sub;

		sub = (int *)data;
		ret = ioctl(fd, req, sub);
	}
	break;

	default:
		ut_pr(UT_ERR, "Can't find the ioctl_req(%d)", req);
		ret_2 = ERR_IOCTL_NO_REQ;
		break;
	}

	if (ret != 0) {
		ret_2 = ioctl_errno(req);
		ut_pr(UT_ERR, "IOCTL cmd fail req(0x%x), err_cmd(%d), %s\n",
		      req, ret_2, strerror(errno));
	}

	return ret_2;
}

void ut_errno_exit(const char *s)
{
	ut_pr(UT_ERR, "%s error %d, %s\n", s, errno, strerror(errno));

	exit(EXIT_FAILURE);
}

void ut_mem_init(
	int fd,
	int count,
	enum v4l2_buf_type type,
	enum v4l2_memory mem_type)
{
	struct v4l2_requestbuffers req;

	ZERO(req);

	req.count  = count;
	req.type   = type;
	req.memory = mem_type;

	if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (errno == EINVAL) {
			ut_pr(UT_ERR, "Not support memType(%d)\n", mem_type);
			exit(EXIT_FAILURE);
		} else {
			ut_errno_exit("VIDIOC_REQBUFS");
		}
	}
}

void ut_device_init(struct ut_dev_init *in)
{
	struct stat st;

	if (-1 == stat(in->dev_name, &st)) {
		ut_pr(UT_ERR, "Cannot identify '%s': %d, %s\n",
		      in->dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		ut_pr(UT_ERR, "%s is no device\n", in->dev_name);
		exit(EXIT_FAILURE);
	}

	*(in->fd) = open(in->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == *(in->fd)) {
		ut_pr(UT_ERR, "Cannot open '%s': %d, %s\n",
		      in->dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

enum cvi_errno ut_device_close(int fd)
{
	enum cvi_errno ret = ERR_NONE;

	if (-1 == close(fd)) {
		ut_pr(UT_ERR, "close device err: %d, %s\n", errno, strerror(errno));
		ret = ERR_DEVICE_CLOSE;
	}

	return ret;
}


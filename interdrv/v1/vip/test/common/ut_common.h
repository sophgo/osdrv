/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: ut_common.h
 * Description:
 */

#ifndef _UT_COMMON_H_
#define _UT_COMMON_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include <linux/videodev2.h>
#include <linux/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <ut_errno.h>

#define UT_INFO		0x1
#define UT_DBG		0x2
#define UT_NOTICE	0x4
#define UT_ERR		0x8
#define UT_ALL		0xF

#if defined(DEBUG_LOG)
#define UT_PR_LEVEL (UT_ALL)
#else
#define UT_PR_LEVEL (UT_ERR)
#endif

#define ut_pr(level, fmt, arg...) \
	do { \
		if (UT_PR_LEVEL & level) \
			printf(fmt, ##arg); \
	} while (0)

#define ALIGN(x, y)	((x + (y - 1)) & ~(y - 1))
#define ZERO(x)		memset(&(x), 0, sizeof(x))

/*******************************************
 *	Global	Definitions
 *******************************************/
enum FSM {
	INIT,
	CONFIG,
	START,
	STOP,
	ERROR,
};

struct ut_dev_init {
	const char *dev_name;
	int	   *fd;
};

struct size {
	uint16_t  w;
	uint16_t  h;
};

enum msg_type {
	RECEIVE,
	SEND,
};

struct msgpack {
	char		*src_name;
	char		*dst_name;
	enum msg_type	msg_id;
	void		*msg_ctx;
};

struct inparam {
	/* select fixed/manual mode */
	uint32_t	manual_mode;
	uint32_t	loop_num;
};

struct module_op {
	char		*name;
	enum FSM	state;
	struct inparam	incfg;
	enum cvi_errno (*init)(void *);
	enum cvi_errno (*config)(void *);
	enum cvi_errno (*start)(void *);
	enum cvi_errno (*stop)(void *);
	enum cvi_errno (*msgrcv)(void *);
};

#define MODULE_DECL(_str, mod)\
	static struct module_op mod##_op =\
	{\
		.name = _str,\
		.init = mod##_init,\
		.config = mod##_config,\
		.start = mod##_start,\
		.stop = mod##_stop,\
		.msgrcv = mod##_msgrcv,\
	}

void ut_device_init(struct ut_dev_init *in);
enum cvi_errno ut_device_close(int fd);

#define UT_DEV_INIT(x, y)\
	do {\
		struct ut_dev_init input = {\
			.dev_name = x,\
			.fd = &y\
		};\
		ut_device_init(&input);\
	} while (0)

void ut_mem_init(
	int fd,
	int count,
	enum v4l2_buf_type type,
	enum v4l2_memory mem_type);

enum cvi_errno ut_v4l2_cmd(
	int fd,
	uint32_t req,
	void *data);

void ut_errno_exit(const char *s);

enum cvi_errno ut_moduleCB(void *msg);

#define MSGSND(src, dst, val)\
	do {\
		struct msgpack msg;\
		char src_name[] = src;\
		char dst_name[] = dst;\
		msg.src_name = src_name;\
		msg.dst_name = dst_name;\
		msg.msg_id = SEND;\
		msg.msg_ctx = (void *)&val;\
		ret = ut_moduleCB((void *)&msg);\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _UT_COMMON_H_ */

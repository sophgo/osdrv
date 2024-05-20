/* SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef __SOPHON_SPACC_H__
#define __SOPHON_SPACC_H__

struct cvi_spacc_base64 {
	u32 customer_code;
	u32 action;  // 0: Decode, 1: Encode
};

struct cvi_spacc_base64_inner {
	u64 src;
	u64 dst;
	u64 len;
	u32 customer_code;
	u32 action;  // 0: Decode, 1: Encode
};

#define IOCTL_SPACC_BASE					'S'
#define IOCTL_SPACC_CREATE_POOL				_IOW(IOCTL_SPACC_BASE, 1, unsigned int)
#define IOCTL_SPACC_GET_POOL_SIZE			_IOR(IOCTL_SPACC_BASE, 2, unsigned int)

#define IOCTL_SPACC_BASE64					_IOW(IOCTL_SPACC_BASE, 5, struct cvi_spacc_base64)
#define IOCTL_SPACC_BASE64_INNER			_IOW(IOCTL_SPACC_BASE, 6, struct cvi_spacc_base64_inner)

#endif

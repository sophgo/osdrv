#ifndef __VI_SDK_LAYER_H__
#define __VI_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "vi_defines.h"

struct vi_ctrl {
	__u32 id;
	__s32 dev;
	__s32 pipe;
	__s32 chn;
	__u32 size;
	__u32 reserved[2];

	union {
		__s32 val;
		__s64 value64;
		void *ptr;
	};
};

#define CHK_STRUCT_SIZE(size, expect_size)									\
	do {													\
		if ((size) != (expect_size)) {									\
			vi_pr(VI_ERR, "cmd_%d size error! expect size %zu, but %u\n", id, expect_size, size);	\
			return ERR_VI_INVALID_PARA;								\
		}												\
	} while (0)

/*****************************************************************************
 *  vi function prototype for vi sdk layer
 ****************************************************************************/
void vi_sdk_release(struct vi_dev *vdev);
long vi_sdk_ctrl(struct vi_dev *vdev, struct vi_ctrl *ctrl);

#ifdef __cplusplus
}
#endif

#endif //__VI_SDK_LAYER_H__

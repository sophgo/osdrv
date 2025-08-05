#ifndef _VPSS_DEFINE_H_
#define _VPSS_DEFINE_H_

#include "defines.h"


#define SCL_MAX_INST VPSS_IP_NUM
#define SCL_MAX_DSI_LP 16
#define SCL_MAX_DSI_SP 2
#define SCL_MAX_GOP_INST 2
#define SCL_MAX_GOP_OW_INST 8
#define SCL_MAX_GOP_FB_INST 2
#define SCL_MAX_COVER_INST 4
#define SCL_MAX_BORDER_VPP 4
#define SCL_MAX_COVER 4

#define TILE_ON_IMG
#define TILE_GUARD_PIXEL 260

#define IMG_IN_PITCH_ALIGN 32

#define SCL_TILE_LEFT		(1 << 0)
#define SCL_TILE_RIGHT		(1 << 1)
#define SCL_TILE_BOTH		(SCL_TILE_LEFT | SCL_TILE_RIGHT)


enum vpss_dev {
	VPSS_V0 = 0,
	VPSS_V1,
	VPSS_V2,
	VPSS_V3,
	VPSS_MAX,
};

enum vpss_sb_line {
	SB_LINE_64 = 64,
	SB_LINE_128 = 128,
	SB_LINE_MAX,
};
enum vpss_sb_mode {
	SB_DISABLE,
	SB_FREE_RUN,
	SB_FRAME_BASE,
};

#endif

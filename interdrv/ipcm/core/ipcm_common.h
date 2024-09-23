
#ifndef __IPCM_COMMON_H__
#define __IPCM_COMMON_H__

#include "ipcm_env.h"

#ifndef NULL
#define NULL 0
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

unsigned long long timer_get_boot_us(void);

#define AP_SYSTEM_REG_BASE 0x1901000
#define AP_SYSTEM_REG_SIZE 0x1000
#define RTOS_BOOT_STATUS_OFFSET 0xFF0

#define RTOS_BOOT_STATUS_REG (AP_SYSTEM_REG_BASE + RTOS_BOOT_STATUS_OFFSET)

#define IPCM_CACHE_ALIGN_SIZE 64

#ifdef IPCM_SHIRNK
#define IPCM_PROC_SUPPORT 0
#else
#define IPCM_PROC_SUPPORT 1
#endif

#ifndef U32_MAX
#define U32_MAX ((u32)~0U)
#endif

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef int s32;

typedef enum _IPCM_LOG_LEVEL_E {
	IPCM_LOG_NONE,
	IPCM_LOG_ERR,
	IPCM_LOG_WARNING,
	IPCM_LOG_INFO,
	IPCM_LOG_DEBUG,
} IPCM_LOG_LEVEL_E;

typedef enum _RTOS_BOOT_STATUS_E {
    RTOS_SYS_BOOT_STAT = 0,
    RTOS_PANIC,
    RTOS_IPCM_DONE,
    RTOS_IPCMSG_DONE, // TODO
    RTOS_VI_DONE, // TODO
    RTOS_VPSS_DONE, // TODO
    RTOS_VENC_DONE, // TODO

    RTOS_BOOT_STATUS_BUTT,
} RTOS_BOOT_STATUS_E;

#define IPCM_LOG_LEVEL_DEFAULT IPCM_LOG_INFO
extern IPCM_LOG_LEVEL_E g_ipcm_log_level;

// errno
#define	EQFULL		200	/* queue is full */
#define	EQEMPTY		201	/* queue is empty */
#define	EMLOCK		202	/* mailbox lock fail */
#define	EMBUSY		203	/* mailbox busy */
#define EQNULL		204 /* queue not init , is null*/

#ifdef IPCM_SHIRNK
#define ipcm_err(x, args...)
#define ipcm_warning(x, args...)
#define ipcm_info(x, args...)
#define ipcm_debug(x, args...)
#else
#define ipcm_err(x, args...) do {if (g_ipcm_log_level >= IPCM_LOG_ERR) \
			PR("<err>[%s:%d]"x, __func__, __LINE__, ##args); } \
			while (0)

#define ipcm_warning(x, args...) do {if (g_ipcm_log_level >= IPCM_LOG_WARNING) \
			PR("<warning>[%s:%d]"x, __func__, __LINE__, ##args); } \
			while (0)

#define ipcm_info(x, args...) do {if (g_ipcm_log_level >= IPCM_LOG_INFO) \
			PR("<info>[%s:%d]"x, __func__, __LINE__, ##args); } \
			while (0)

#ifdef _DEBUG
#define ipcm_debug(x, args...) do { \
			PR("<debug>[%s:%d]"x, __func__, __LINE__, ##args); } \
			while (0)
#else
#define ipcm_debug(x, args...) do {if (g_ipcm_log_level >= IPCM_LOG_DEBUG) \
			PR("<debug>[%s:%d]"x, __func__, __LINE__, ##args); } \
			while (0)
#endif

#endif

#ifndef ALIGN
#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) ((x) & ~((a)-1))
#endif

#ifndef CVI_SUCCESS
#define CVI_SUCCESS 0
#endif

#ifndef CVI_FAIL
#define CVI_FAIL -1
#endif

#define IPCM_DATA_LOCK_NUM 10

// b equal to (SPIN_MAX - SPIN_DATA + 1)
#define IPCM_DATA_SPIN_MAX 5

// 7 bit for msg id
#define IPCM_MSG_ID_MAX 127

/**
 * 0 ~ 3: for system msg
 * 4 ~ 7: for virrtty msg
 * 8 ~ 11: for sharefs msg
 * 12 ~ 147: for anonymous msg
 * others: reserved
 * 
 */
#define IPCM_SYS_PORT_MAX 4
#define IPCM_VIRTTTY_PORT_MAX 4
#define IPCM_SHAREFS_PORT_MAX 4
#define IPCM_MSG_PORT_MAX 8
// 64 for userspace, 64 for kernel and userspace
#define IPCM_ANON_PORT_MAX 128
#define IPCM_ANON_KER_PORT_ST 64

#define IPCM_SYS_PORT_IDX 0
#define IPCM_VIRTTTY_PORT_IDX (IPCM_SYS_PORT_MAX)
#define IPCM_SHAREFS_PORT_IDX (IPCM_VIRTTTY_PORT_IDX + IPCM_VIRTTTY_PORT_MAX)
#define IPCM_MSG_PORT_IDX (IPCM_SHAREFS_PORT_IDX + IPCM_SHAREFS_PORT_MAX)
// 64 for userspace, 64 for kernel and userspace
#define IPCM_ANON_PORT_IDX (IPCM_MSG_PORT_IDX + IPCM_MSG_PORT_MAX)

// ipcm sturct
struct valid_t {
	unsigned char linux_valid;
	unsigned char rtos_valid;
} __attribute__((packed));

typedef enum _MsgType {
	MSG_TYPE_SHM = 0,	// msg_param is share memory addr
	MSG_TYPE_RAW_PARAM,	// msg_param is the param
} MsgType;

typedef union resv_t {
	struct valid_t valid;
	unsigned short mstime; // 0 : noblock, -1 : block infinite
} resv_t;

typedef struct _MsgPtr {
	u32 data_pos : 20;
	u32 remaining_rd_len : 12;
} MsgPtr;

typedef union _MsgParam {
	MsgPtr msg_ptr;
	unsigned int param;
} MsgParam;

typedef struct _MsgData {
	u8 grp_id;
	u8 msg_id : 7;
	u8 func_type : 1;
	union resv_t resv;
	MsgParam msg_param;
} __attribute__((packed)) __attribute__((aligned(0x8))) MsgData;

/**
 * PORT_SYSTEM: internal for system  TODO
 * PORT_VIRTTTY:
 * PORT_SHAREFS
 * PORT_MSG: internal for IPCMsg
 * PORT_ANON: custom api
 * if alios send msg, msg port_id >= IPCM_ANON_KER_PORT_ST, and linux kernel has been registered handler,
 * linux kernel handler will recv msg
 */
typedef enum _PortType {
	PORT_SYSTEM = 0,
	PORT_VIRTTTY,
	PORT_SHAREFS,
	PORT_MSG,
	PORT_ANON,

	PORT_BUTT,
} PortType;

typedef struct _IPCM_FLUSH_PARAM {
	u32 data_pos;
	u32 len;
} IPCM_FLUSH_PARAM;

// ipcm pool
typedef void *POOLHANDLE;

typedef struct _BlockConfig {
	u32 size;
	u32 num;
} BlockConfig;

s32 ipcm_get_grp_id(PortType type, u8 port_id, u8 *grp_id);

s32 ipcm_get_port_id(u8 grp_id, u8 *type, u8 *port_id);

int ipcm_get_log_level(void);

void ipcm_set_log_level(IPCM_LOG_LEVEL_E level);

int ipcm_log_level_debug(void);

#endif

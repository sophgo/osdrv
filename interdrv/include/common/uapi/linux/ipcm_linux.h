
#ifndef __IPCM_LINUX_H__
#define __IPCM_LINUX_H__

#include "linux/cvi_ipcmsg.h"

typedef enum _IPCM_BASE_CMD_E {
	IPCM_BASE_GET_DATA,
	IPCM_BASE_RLS_DATA,
	IPCM_BASE_FLUSH_DATA,
	IPCM_BASE_INV_DATA,
	IPCM_BASE_POOL_RESET,

	//get pool addr/size , freertos addr/size
	IPCM_BASE_POOL_ADDR,
	IPCM_BASE_POOL_SIZE,
	IPCM_BASE_FREERTOS_ADDR,
	IPCM_BASE_FREERTOS_SIZE,

	IPCM_BASE_GET_SHM_DATA_POS,

	IPCM_BASE_CMD_BUTT,
} IPCM_BASE_CMD_E;

typedef enum _IPCM_MSG_CMD_E {
	IPCM_MSG_ADD_SERVICE,
	IPCM_MSG_DEL_SERVICE,
	IPCM_MSG_CONNECT,
	IPCM_MSG_DISCONNECT,
	IPCM_MSG_IS_CONNECT,
	IPCM_MSG_SEND_ONLY,
	IPCM_MSG_SEND_ASYNC,
	IPCM_MSG_SEND_SYNC,
	IPCM_MSG_RUN,
	IPCM_MSG_GET_ID,
	IPCM_MSG_GET_SIG_DATA,
	IPCM_MSG_GET_USER_CNT,

	IPCM_MSG_LOCK,
	IPCM_MSG_UNLOCK,

	IPCM_MSG_CMD_BUTT,
} IPCM_MSG_CMD_E;

struct ipcm_add_service_cfg {
	CVI_CHAR aszServiceName[32];
	CVI_IPCMSG_CONNECT_S stConnectAttr;
};

struct ipcm_connect_cfg {
	CVI_CHAR aszServiceName[32];
	CVI_S32 s32Id;
	CVI_U8 isTry;
};

struct ipcm_send_only_cfg {
	CVI_S32 s32Id;
	CVI_IPCMSG_MESSAGE_S *pstRequest;
};

struct ipcm_send_cfg {
	CVI_S32 s32Id;
	CVI_IPCMSG_MESSAGE_S *pstRequest;
	CVI_IPCMSG_MESSAGE_S *pstResq;
	CVI_VOID *pRespBody;
	CVI_S32 s32TimeoutMs;
};

struct ipcm_signal_cfg {
	CVI_S32 s32Id;
	CVI_IPCMSG_MESSAGE_S *pstMsg;
	CVI_VOID *pBody;
};

typedef enum _IPCM_SYS_CMD_E {
	IPCM_SYS_GET_SYSINFO,
	IPCM_SYS_GET_LOG,

	IPCM_SYS_CMD_BUTT,
} IPCM_SYS_CMD_E;

typedef enum _IPCM_ANON_CMD_E {
	IPCM_ANON_TEST1,
	IPCM_ANON_TEST2,

	IPCM_ANON_CMD_BUTT,
} IPCM_ANON_CMD_E;

#define IPCM_IOC_MAGIC		'I'
#define IPCM_IOC_BASE		0x20
#define IPCM_IOC_MSG		0x40
#define IPCM_IOC_SYS		0x60
#define IPCM_IOC_ANON		0xD0

#define IPCM_IOC_GET_DATA		_IO(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_GET_DATA)
#define IPCM_IOC_RLS_DATA		_IO(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_RLS_DATA)
#define IPCM_IOC_FLUSH_DATA		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_FLUSH_DATA, IPCM_FLUSH_PARAM)
#define IPCM_IOC_INV_DATA		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_INV_DATA, IPCM_FLUSH_PARAM)
#define IPCM_IOC_POOL_RESET		_IO(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_POOL_RESET)
#define IPCM_GET_POOL_ADDR		_IOR(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_POOL_ADDR, unsigned long)
#define IPCM_GET_POOL_SIZE		_IOR(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_POOL_SIZE, unsigned int)
#define IPCM_GET_FREERTOS_ADDR	_IOR(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_FREERTOS_ADDR, unsigned long)
#define IPCM_GET_FREERTOS_SIZE	_IOR(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_FREERTOS_SIZE, unsigned int)
#define IPCM_GET_SHM_DATA_POS	_IOR(IPCM_IOC_MAGIC, IPCM_IOC_BASE + IPCM_BASE_GET_SHM_DATA_POS, unsigned int)

#define IPCM_IOC_ADD_SERVICE	_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_ADD_SERVICE, struct ipcm_add_service_cfg)
#define IPCM_IOC_DEL_SERVICE	_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_DEL_SERVICE, unsigned int)
#define IPCM_IOC_CONNECT		_IOWR(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_CONNECT, struct ipcm_connect_cfg)
#define IPCM_IOC_DISCONNECT		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_DISCONNECT, unsigned int)
#define IPCM_IOC_IS_CONNECT		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_IS_CONNECT, unsigned int)
#define IPCM_IOC_SEND_ONLY		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_SEND_ONLY, struct ipcm_send_only_cfg)
#define IPCM_IOC_SEND_ASYNC		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_SEND_ASYNC, struct ipcm_send_cfg)
#define IPCM_IOC_SEND_SYNC		_IOWR(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_SEND_SYNC, struct ipcm_send_cfg)
#define IPCM_IOC_RUN			_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_RUN, unsigned int)
#define IPCM_IOC_GET_ID			_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_GET_ID, unsigned int)
#define IPCM_IOC_SIG_DATA		_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_GET_SIG_DATA, struct ipcm_signal_cfg)
#define IPCM_IOC_GET_USER_CNT	_IOW(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_GET_USER_CNT, int)

#define IPCM_IOC_LOCK			_IO(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_LOCK)
#define IPCM_IOC_UNLOCK			_IO(IPCM_IOC_MAGIC, IPCM_IOC_MSG + IPCM_MSG_UNLOCK)

#define IPCM_IOC_GET_SYSINFO	_IOR(IPCM_IOC_MAGIC, IPCM_IOC_SYS + IPCM_SYS_GET_SYSINFO, MsgData)
#define IPCM_IOC_GET_LOG		_IOR(IPCM_IOC_MAGIC, IPCM_IOC_SYS + IPCM_SYS_GET_LOG, MsgData)

#define IPCM_IOC_ANON_TEST1		_IO(IPCM_IOC_MAGIC, IPCM_IOC_ANON + IPCM_ANON_TEST1)
#define IPCM_IOC_ANON_TEST2		_IO(IPCM_IOC_MAGIC, IPCM_IOC_ANON + IPCM_ANON_TEST2)

#endif
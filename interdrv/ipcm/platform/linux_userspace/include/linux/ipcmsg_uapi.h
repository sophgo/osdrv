#ifndef __IPCMSG_UAPI_H__
#define __IPCMSG_UAPI_H__

#include "cvi_comm_ipcmsg.h"


typedef enum _IPCMSG_MSG_CMD_E {
	IPCMSG_MSG_ADD_SERVICE,
	IPCMSG_MSG_DEL_SERVICE,
	IPCMSG_MSG_CONNECT,
	IPCMSG_MSG_DISCONNECT,
	IPCMSG_MSG_IS_CONNECT,
	IPCMSG_MSG_SEND_ONLY,
	IPCMSG_MSG_SEND_ASYNC,
	IPCMSG_MSG_SEND_SYNC,
	IPCMSG_MSG_RUN,
	IPCMSG_MSG_GET_ID,
	IPCMSG_MSG_GET_SIG_DATA,
	IPCMSG_MSG_GET_USER_CNT,
	IPCMSG_IOC_LOCK,
	IPCMSG_IOC_UNLOCK,
	IPCMSG_MSG_CMD_BUTT,
} IPCMSG_MSG_CMD_E;

struct ipcmsg_add_service_cfg {
	char aszServiceName[32];
	CVI_IPCMSG_CONNECT_S stConnectAttr;
};

struct ipcmsg_connect_cfg {
	char aszServiceName[32];
	int s32Id;
	unsigned char isTry;
};

struct ipcmsg_send_only_cfg {
	int s32Id;
	CVI_IPCMSG_MESSAGE_S *pstRequest;
};

struct ipcmsg_send_cfg {
	int s32Id;
	CVI_IPCMSG_MESSAGE_S *pstRequest;
	CVI_IPCMSG_MESSAGE_S *pstResq;
	void *pRespBody;
	int s32TimeoutMs;
};

struct ipcmsg_signal_cfg {
	int s32Id;
	CVI_IPCMSG_MESSAGE_S *pstMsg;
	void *pBody;
};


#define IPCMSG_IOC_MAGIC		'P'


#define IPCMSG_IOC_ADD_SERVICE	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_ADD_SERVICE, struct ipcmsg_add_service_cfg)
#define IPCMSG_IOC_DEL_SERVICE	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_DEL_SERVICE, unsigned int)
#define IPCMSG_IOC_CONNECT		_IOWR(IPCMSG_IOC_MAGIC, IPCMSG_MSG_CONNECT, struct ipcmsg_connect_cfg)
#define IPCMSG_IOC_DISCONNECT	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_DISCONNECT, unsigned int)
#define IPCMSG_IOC_IS_CONNECT	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_IS_CONNECT, unsigned int)
#define IPCMSG_IOC_SEND_ONLY	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_SEND_ONLY, struct ipcmsg_send_only_cfg)
#define IPCMSG_IOC_SEND_ASYNC	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_SEND_ASYNC, struct ipcmsg_send_cfg)
#define IPCMSG_IOC_SEND_SYNC	_IOWR(IPCMSG_IOC_MAGIC, IPCMSG_MSG_SEND_SYNC, struct ipcmsg_send_cfg)
#define IPCMSG_IOC_RUN			_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_RUN, unsigned int)
#define IPCMSG_IOC_GET_ID		_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_GET_ID, unsigned int)
#define IPCMSG_IOC_SIG_DATA		_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_GET_SIG_DATA, struct ipcmsg_signal_cfg)
#define IPCMSG_IOC_GET_USER_CNT	_IOW(IPCMSG_IOC_MAGIC, IPCMSG_MSG_GET_USER_CNT, int)


#endif

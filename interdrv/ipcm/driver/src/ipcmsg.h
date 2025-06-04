#ifndef __IPCMSG_H__
#define __IPCMSG_H__

int ipcmsg_proc_init(void);

int ipcmsg_proc_remove(void);

int ipcmsg_add_service(const char *pszServiceName, const CVI_IPCMSG_CONNECT_S *pstConnectAttr);

int ipcmsg_del_service(const char *pszServiceName);

int ipcmsg_tryconnect(int *ps32Id, const char *pszServiceName);

int ipcmsg_connect(int *ps32Id, const char *pszServiceName);

int ipcmsg_disconnect(int s32Id);

bool ipcmsg_is_connected(int s32Id);

int ipcmsg_send_only(int s32Id, CVI_IPCMSG_MESSAGE_S *pstRequest);

int ipcmsg_send_async(int s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg);

int ipcmsg_send_sync(int s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg,
	CVI_IPCMSG_MESSAGE_S **ppstMsg, int s32TimeoutMs);

void ipcmsg_run(int s32Id);

int ipcmsg_inquireUserCnt(int s32Id);

#endif

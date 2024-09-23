#ifndef __IPCMSG_H__
#define __IPCMSG_H__

int ipcmsg_proc_init(void);

int ipcmsg_proc_remove(void);

CVI_S32 ipcmsg_add_service(const CVI_CHAR *pszServiceName, const CVI_IPCMSG_CONNECT_S *pstConnectAttr);

CVI_S32 ipcmsg_del_service(const CVI_CHAR *pszServiceName);

CVI_S32 ipcmsg_tryconnect(CVI_S32 *ps32Id, const CVI_CHAR *pszServiceName);

CVI_S32 ipcmsg_connect(CVI_S32 *ps32Id, const CVI_CHAR *pszServiceName);

CVI_S32 ipcmsg_disconnect(CVI_S32 s32Id);

CVI_BOOL ipcmsg_is_connected(CVI_S32 s32Id);

CVI_S32 ipcmsg_send_only(CVI_S32 s32Id, CVI_IPCMSG_MESSAGE_S *pstRequest);

CVI_S32 ipcmsg_send_async(CVI_S32 s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg);

CVI_S32 ipcmsg_send_sync(CVI_S32 s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg,
	CVI_IPCMSG_MESSAGE_S **ppstMsg, CVI_S32 s32TimeoutMs);

CVI_VOID ipcmsg_run(CVI_S32 s32Id);

CVI_S32 ipcmsg_inquireUserCnt(CVI_S32 s32Id);

#endif

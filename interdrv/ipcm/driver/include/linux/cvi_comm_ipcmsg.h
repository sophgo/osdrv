#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <linux/types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus*/


#define CVI_IPCMSG_MAX_CONTENT_LEN (1024)
#define CVI_IPCMSG_PRIVDATA_NUM (8)
#define CVI_IPCMSG_INVALID_MSGID (0xFFFFFFFFFFFFFFFF)

/**Connection structure*/
typedef struct cviIPCMSG_CONNECT_S {
	unsigned int u32RemoteId;	 /**<Indicate the enumeration values for connecting to remote CPUs*/
	unsigned int u32Port;		 /**<Custom port number for message communication*/
	unsigned int u32Priority;	 /**<Priority of message transmission*/
} CVI_IPCMSG_CONNECT_S;

/**Message structure*/
typedef struct cviIPCMSG_MESSAGE_S {
	unsigned char bIsResp;	 /**<Identify the response messgae*/
	uint64_t u64Id;		 /**<Message ID*/
	unsigned int u32Module;	 /**<Module ID, user-defined*/
	unsigned int u32CMD;	     /**<CMD ID, user-defined*/
	int s32RetVal;	 /**<Retrun Value in response message*/
	unsigned int u32BodyLen;  /**<Length of pBody*/
	/**<Private data, can be modify directly after ::CVI_IPCMSG_CreateMessage
	or ::CVI_IPCMSG_CreateRespMessage*/
	int as32PrivData[CVI_IPCMSG_PRIVDATA_NUM];
	void *pBody;	 /**<Message body*/
#ifdef __arm__
	unsigned int u32VirAddrPadding;
#endif
} CVI_IPCMSG_MESSAGE_S;


/** Error number base */
#define CVI_IPCMSG_ERRNO_BASE 0x1900
/** Parameter is invalid */
#define CVI_IPCMSG_EINVAL (CVI_IPCMSG_ERRNO_BASE+1)
/** The function run timeout */
#define CVI_IPCMSG_ETIMEOUT (CVI_IPCMSG_ERRNO_BASE+2)
/** IPC driver open fail */
#define CVI_IPCMSG_ENOOP (CVI_IPCMSG_ERRNO_BASE+3)
/** Internal error */
#define CVI_IPCMSG_EINTER (CVI_IPCMSG_ERRNO_BASE+4)
/** Null pointer*/
#define CVI_IPCMSG_ENULL_PTR (CVI_IPCMSG_ERRNO_BASE+5)


#define CVI_IPCMSG_MAX_SERVICENAME_LEN (16)


/**
 * @brief Callback of receiving message.
 * @param[in] s32Id Handle of IPCMSG.
 * @param[in] pstMsg Received message.
 */
typedef void (*CVI_IPCMSG_HANDLE_FN_PTR)(int s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg);

/**
 * @brief Callback of receiving response message. used by CVI_IPCMSG_SendAsync
 * @param[in] pstMsg Response message.
 */
typedef void (*CVI_IPCMSG_RESPHANDLE_FN_PTR)(CVI_IPCMSG_MESSAGE_S *pstMsg);


/** @}*/  /** <!-- ==== IPCMSG End ====*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus*/

#endif

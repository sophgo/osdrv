/**
 * @brief Public IPCM interface for CVI platform
 *
 * Provides the main public APIs for inter-processor communication on CVI platform
 */

#ifndef __CVI_IPCM_H__
#define __CVI_IPCM_H__

#include "cvi_common.h"
#include "cvi_comm_ipcm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/**
 * @brief Initialize IPCM system
 * @return Status code
 */
CVI_S32 CVI_IPCM_Init(void);

/**
 * @brief Uninitialize IPCM system
 * @return Status code
 */
CVI_S32 CVI_IPCM_Uninit(void);

/**
 * @brief Invalidate data in cache
 * @param pData Data pointer
 * @param u32Size Data size
 * @return Status code
 */
CVI_S32 CVI_IPCM_InvData(CVI_VOID *pData, CVI_U32 u32Size);

/**
 * @brief Flush data from cache
 * @param pData Data pointer
 * @param u32Size Data size
 * @return Status code
 */
CVI_S32 CVI_IPCM_FlushData(CVI_VOID *pData, CVI_U32 u32Size);

/**
 * @brief Lock data access
 * @param u8LockID Lock identifier
 * @return Status code
 */
CVI_S32 CVI_IPCM_DataLock(CVI_U8 u8LockID);

/**
 * @brief Unlock data access
 * @param u8LockID Lock identifier
 * @return Status code
 */
CVI_S32 CVI_IPCM_DataUnlock(CVI_U8 u8LockID);

/**
 * @brief Get buffer from shared memory
 * @param u32Size Desired buffer size
 * @return Buffer pointer or NULL on failure
 */
CVI_VOID *CVI_IPCM_GetBuff(CVI_U32 u32Size);

/**
 * @brief Release buffer back to shared memory
 * @param pData Buffer pointer
 * @return Status code
 */
CVI_S32 CVI_IPCM_ReleaseBuff(CVI_VOID *pData);

/**
 * @brief Memory management functions
 */
CVI_VOID *CVI_IPCM_GetUserAddr(CVI_U32 paddr);

CVI_U32 CVI_IPCM_GetParamBinAddr(void);

CVI_U32 CVI_IPCM_GetParamBakBinAddr(void);

CVI_U32 CVI_IPCM_GetPQBinQddr(void);

/**
 * @brief Custom IPCM interface functions
 */
CVI_S32 CVI_IPCM_CustInit(void);

CVI_S32 CVI_IPCM_CustUninit(void);

CVI_S32 CVI_IPCM_CustPoolReset(void);

/**
 * @brief Custom message handling functions
 */
CVI_S32 CVI_IPCM_RegisterCustHandle(CVI_U8 u8PortID, IPCM_CUST_MSGPROC_FN pfnHandler, CVI_VOID *pData);

CVI_S32 CVI_IPCM_DeregisterCustHandle(CVI_U8 u8PortID);

// send msg if msg len > 4; max msg length is limited by pool block (2048?)
CVI_S32 CVI_IPCM_CustSendMsg(CVI_U8 u8PortID, CVI_U8 u8MsgID, CVI_VOID *pData, CVI_U32 u32Len);

// send param if msg len <= 4 or send 32 bits addr
CVI_S32 CVI_IPCM_CustSendParam(CVI_U8 u8PortID, CVI_U8 u8MsgID, CVI_U32 u32Param);

/**
 * @brief RTOS boot status management functions
 */
CVI_S32 CVI_IPCM_SetRtosSysBootStat(void);

CVI_S32 CVI_IPCM_ClrRtosSysBootStat(void);

CVI_S32 CVI_IPCM_GetRtosBootStat(void);

CVI_S32 CVI_IPCM_GetRtosIpcmStat(void);

CVI_S32 CVI_IPCM_SetRtosBootLogoStat(void);

CVI_S32 CVI_IPCM_ClrRtosBootLogoStat(void);

CVI_S32 CVI_IPCM_GetRtosBootLogoStat(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_IPCM_H__ */

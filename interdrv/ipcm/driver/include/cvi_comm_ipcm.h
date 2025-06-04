/**
 * @brief Common IPCM definitions and structures
 *
 * Defines shared constants and data structures for IPCM communication between processors
 */

#ifndef __CVI_COMM_IPCM_H__
#define __CVI_COMM_IPCM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "cvi_common.h"
#include "cvi_type.h"

/**
 * @brief Port definitions for custom IPCM communication
 */
#define CVI_IPCM_CUST_VUART_PORT 0  // Virtual UART port
#define CVI_IPCM_CUST_MSG_PORT 1    // Message port
#define CVI_IPCM_CUST_KER_PORT_ST 64 // Kernel port start index

/**
 * @brief Message type enumeration
 */
typedef enum _IPCM_MSG_TYPE_E {
	IPCM_MSG_TYPE_SHM = 0,	// Message uses shared memory for data
	IPCM_MSG_TYPE_RAW_PARAM,	// Message contains raw parameter data
} IPCM_MSG_TYPE_E;

/**
 * @brief Shared memory data structure
 */
typedef struct _IPCM_CUST_SHM_DATA_S {
	CVI_VOID *pData;  // Pointer to shared memory data
	CVI_U32 u32Size;  // Size of shared memory data
} IPCM_CUST_SHM_DATA_S;

/**
 * @brief Custom message structure
 */
typedef struct _IPCM_CUST_MSG_S {
	CVI_U8 u8PortID;        // Port identifier
	CVI_U8 u8MsgID : 7;     // Message identifier (7 bits)
	CVI_U8 u8DataType : 1;  // Data type flag (0: shared memory, 1: raw parameter)
	union {
		IPCM_CUST_SHM_DATA_S stData; // Shared memory data (when u8DataType is 0)
		CVI_U32 u32Param;            // Raw parameter (when u8DataType is 1)
	};
} IPCM_CUST_MSG_S;

/**
 * @brief Custom message processing callback function type
 * @param pPriv Private data passed to handler
 * @param pstData Message data structure
 * @return Status code
 */
typedef CVI_S32 (*IPCM_CUST_MSGPROC_FN)(CVI_VOID *pPriv, IPCM_CUST_MSG_S *pstData);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_COMM_IPCM_H__ */

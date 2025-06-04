/**
 * @file cvi_ipcm.c
 * @brief Main IPCM implementation for CVI platform
 * 
 * This file implements the core IPCM functionality including:
 * - Custom message handling
 * - Message processing registration
 * - Buffer management
 */

#include <errno.h>
#include "cvi_comm_ipcm.h"
#include "cvi_ipcm.h"
#include "ipcm_port.h"
#include "ipcm_custom.h"

// Magic number for custom message processor: A(0x41) N(0x4E) M(0xdD) G(0x47)
#define CUST_MSG_PROC_MAGIC 0x414E4D47

/**
 * @brief Custom message processor node structure
 */
typedef struct _IPCM_CUST_MSGPROC_S {
    CVI_U8 u8PortID;                    // Port identifier
    CVI_VOID *pPrivData;                // Private data for handler
    IPCM_CUST_MSGPROC_FN pfnCustProc;  // Message processing function
    struct _IPCM_CUST_MSGPROC_S *next;  // Next processor in list
} IPCM_CUST_MSGPROC_S;

// Head of custom processor linked list
static IPCM_CUST_MSGPROC_S *s_cust_proc_head = NULL;

/**
 * @brief Custom message processing handler
 * 
 * Processes incoming custom messages by:
 * 1. Validating magic number
 * 2. Converting message format
 * 3. Finding and calling registered handler
 *
 * @param priv Private data (magic number)
 * @param data Message data
 * @return Status code
 */
static s32 _CUST_MSGPROC_HANDLE(void *priv, ipcm_cust_msg_t *data)
{
    IPCM_CUST_MSGPROC_S *tmp = s_cust_proc_head;

    if (priv != (void *)CUST_MSG_PROC_MAGIC) {
        ipcm_err("unexcepted magic(%lx), excepted(%x)\n", (unsigned long)priv, CUST_MSG_PROC_MAGIC);
        return -EFAULT;
    }

    if (data) {
        IPCM_CUST_MSG_S stCust = {};
        stCust.u8PortID = data->port_id;
        stCust.u8MsgID = data->msg_id;
        stCust.u8DataType = data->data_type;
        if (MSG_TYPE_SHM == data->data_type) {
            stCust.stData.pData = data->data;
            stCust.stData.u32Size = data->size;
        } else {
            stCust.u32Param = (unsigned int)(unsigned long)data->data;
        }
        while (tmp != NULL) {
            if (tmp->u8PortID == stCust.u8PortID) {
                if (tmp->pfnCustProc) {
                    return tmp->pfnCustProc(tmp->pPrivData, &stCust);
                }
                else {
                    ipcm_warning("port id(%d) been found, but handle is null.\n", tmp->u8PortID);
                    return -EFAULT;
                }
            }
            tmp = tmp->next;
        }
        ipcm_warning("port id(%d) is not registered.\n", stCust.u8PortID);
        return 0;
    }

    ipcm_err("_CUST_MSGPROC_HANDLE data is null.\n");
    return -EFAULT;
}

CVI_S32 CVI_IPCM_Init(void)
{
    return ipcm_port_init();
}

CVI_S32 CVI_IPCM_Uninit(void)
{
    return ipcm_port_uninit();
}

CVI_S32 CVI_IPCM_InvData(CVI_VOID *pData, CVI_U32 u32Size)
{
    return ipcm_port_inv_data(pData, u32Size);
}

CVI_S32 CVI_IPCM_FlushData(CVI_VOID *pData, CVI_U32 u32Size)
{
    return ipcm_port_flush_data(pData, u32Size);
}

CVI_S32 CVI_IPCM_DataLock(CVI_U8 u8LockID)
{
    return ipcm_port_data_lock(u8LockID);
}

CVI_S32 CVI_IPCM_DataUnlock(CVI_U8 u8LockID)
{
    return ipcm_port_data_unlock(u8LockID);
}

CVI_VOID *CVI_IPCM_GetBuff(CVI_U32 u32Size)
{
    return ipcm_cust_get_buff(u32Size);
}

CVI_S32 CVI_IPCM_ReleaseBuff(CVI_VOID *pData)
{
    return ipcm_cust_release_buff(pData);
}

CVI_VOID *CVI_IPCM_GetUserAddr(CVI_U32 u32Paddr)
{
    return ipcm_port_get_user_addr(u32Paddr);
}

CVI_U32 CVI_IPCM_GetParamBinAddr(void)
{
    return ipcm_port_get_param_bin_addr();
}

CVI_U32 CVI_IPCM_GetParamBakBinAddr(void)
{
    return ipcm_port_get_param_bak_bin_addr();
}

CVI_U32 CVI_IPCM_GetPQBinQddr(void)
{
    return ipcm_port_get_pq_bin_addr();
}

CVI_S32 CVI_IPCM_CustInit(void)
{
    ipcm_cust_register_handle(_CUST_MSGPROC_HANDLE, (void *)CUST_MSG_PROC_MAGIC);
    return ipcm_cust_cli_init();
}

CVI_S32 CVI_IPCM_CustUninit(void)
{
    ipcm_cust_deregister_handle();
    return ipcm_cust_cli_uninit();
}

CVI_S32 CVI_IPCM_CustPoolReset(void)
{
    return ipcm_cust_pool_reset();
}

CVI_S32 CVI_IPCM_RegisterCustHandle(CVI_U8 u8PortID, IPCM_CUST_MSGPROC_FN pfnHandler, CVI_VOID *pData)
{
    IPCM_CUST_MSGPROC_S *proc = NULL;
    IPCM_CUST_MSGPROC_S *tmp;
    IPCM_CUST_MSGPROC_S *tmp_next;

    if (u8PortID >= IPCM_CUST_PORT_MAX) {
        ipcm_err("port id(%d) is invalid.\n", u8PortID);
        return -EINVAL;
    }

    if (NULL == pfnHandler) {
        ipcm_warning("pfnHandler is null, port id(%d) will be deregistered.\n", u8PortID);
        CVI_IPCM_DeregisterCustHandle(u8PortID);
        return 0;
    }

    proc = ipcm_alloc(sizeof(IPCM_CUST_MSGPROC_S));
    if (NULL == proc) {
        ipcm_err("proc malloc fail.\n");
        return -ENOMEM;
    }
    proc->u8PortID = u8PortID;
    proc->pPrivData = pData;
    proc->pfnCustProc = pfnHandler;
    proc->next = NULL;

    tmp = s_cust_proc_head;
    tmp_next = s_cust_proc_head;
    while(tmp_next != NULL) {
        tmp = tmp_next;
        if (tmp->u8PortID == u8PortID) { // u8PortID has been registered
            ipcm_warning("port id(%d) has been registered, update.\n", u8PortID);
            tmp->pPrivData = pData;
            tmp->pfnCustProc = pfnHandler;
            ipcm_free(proc);
            return 0;
        }
        tmp_next = tmp_next->next;
    }

    if (NULL == tmp) { // first handler
        s_cust_proc_head = proc;
    } else {
        tmp->next = proc;
    }

    return 0;
}

CVI_S32 CVI_IPCM_DeregisterCustHandle(CVI_U8 u8PortID)
{
    IPCM_CUST_MSGPROC_S *tmp;
    IPCM_CUST_MSGPROC_S *tmp_pre;

    if (u8PortID >= IPCM_CUST_PORT_MAX) {
        ipcm_err("port id(%d) is invalid.\n", u8PortID);
        return -EINVAL;
    }

    tmp = s_cust_proc_head;
    tmp_pre = s_cust_proc_head;
    while(tmp != NULL) {
        if (tmp->u8PortID == u8PortID) {
            if (tmp == s_cust_proc_head) {
                s_cust_proc_head = s_cust_proc_head->next;
            } else {
                tmp_pre->next = tmp->next;
            }
            ipcm_free(tmp);
            return 0;
        }
        tmp_pre = tmp;
        tmp = tmp->next;
    }

    ipcm_warning("port id(%d) not been registered.\n", u8PortID);
    return -EFAULT;
}

// send msg if msg len > 4; max msg length is limited by pool block (2048?)
CVI_S32 CVI_IPCM_CustSendMsg(CVI_U8 u8PortID, CVI_U8 u8MsgID, CVI_VOID *pData, CVI_U32 u32Len)
{
    if (pData == NULL) {
        ipcm_err("pData is null.\n");
        return -EINVAL;
    }

    if (u8MsgID > IPCM_MSG_ID_MAX) {
        ipcm_err("u8MsgID(%d) out of range, max(%d).\n", u8MsgID, IPCM_MSG_ID_MAX);
        return -EINVAL;
    }

    return ipcm_cust_send_msg(u8PortID, u8MsgID, pData, u32Len);
}

// send param if msg len <= 4 or send 32 bits addr
CVI_S32 CVI_IPCM_CustSendParam(CVI_U8 u8PortID, CVI_U8 u8MsgID, CVI_U32 u32Param)
{
    return ipcm_cust_send_param(u8PortID, u8MsgID, u32Param);
}

CVI_S32 CVI_IPCM_SetRtosSysBootStat(void)
{
    return ipcm_set_rtos_boot_bit(RTOS_SYS_BOOT_STAT, 1);
}

CVI_S32 CVI_IPCM_ClrRtosSysBootStat(void)
{
    return ipcm_set_rtos_boot_bit(RTOS_SYS_BOOT_STAT, 0);
}

CVI_S32 CVI_IPCM_GetRtosBootStat(void)
{
    u32 stat = 0;
    ipcm_get_rtos_boot_status(&stat);
    return (stat & RTOS_SYS_BOOT_STAT);
}

CVI_S32 CVI_IPCM_GetRtosIpcmStat(void)
{
    u32 stat = 0;
    ipcm_get_rtos_boot_status(&stat);
    return (stat & RTOS_IPCM_DONE);
}

CVI_S32 CVI_IPCM_SetRtosBootLogoStat(void)
{
    return ipcm_set_rtos_boot_bit(RTOS_BOOTLOGO_DONE, 1);
}

CVI_S32 CVI_IPCM_ClrRtosBootLogoStat(void)
{
    return ipcm_set_rtos_boot_bit(RTOS_BOOTLOGO_DONE, 0);
}

CVI_S32 CVI_IPCM_GetRtosBootLogoStat(void)
{
    u32 stat = 0;
    ipcm_get_rtos_boot_status(&stat);
    return (stat & RTOS_BOOTLOGO_DONE);
}

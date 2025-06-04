
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ipcm_port.h"
#include "ipcm_test_common.h"
#include "ipcm_test_common_cust.h"
#include "cvi_comm_ipcm.h"
#include "cvi_ipcm.h"

#define CUST_PRIV_DATA_MAGIC 0x443355aa
#define CUST_TEST_PARAM_DATA 0x4a5a2230

#define CUST_MSG_PROC_FUNCTION(ID) \
    static CVI_S32 cust_msg_proc_##ID(CVI_VOID *pPriv, IPCM_CUST_MSG_S *pstData) \
    { \
        return cust_msg_proc_common(pPriv, pstData); \
    }

static CUST_MSGPROC_FN _handler_hook = NULL;
static void *_hook_priv_data = NULL;

static CVI_S32 cust_msg_proc_common(CVI_VOID *pPriv, IPCM_CUST_MSG_S *pstData);

// declare handle function
CUST_MSG_PROC_FUNCTION(1)
CUST_MSG_PROC_FUNCTION(2)
CUST_MSG_PROC_FUNCTION(3)

static s32 _cust_msg_process(void *priv, ipcm_cust_msg_t *data)
{
    u8 port_id, msg_id, data_type;
    u32 data_len;
    u32 i = 0;
    s32 ret = 0;

    if (priv != (void *)CUST_PRIV_DATA_MAGIC) {
        ipcm_err("======cust test fail, reg handle magic err.\n");
        return -1;
    }
    if (data == NULL) {
        ipcm_err("======cust test fail, handle data is null.\n");
        return -1;
    }

    if (_handler_hook) {
        ret = _handler_hook(_hook_priv_data, data);
        if (ret == 1) // hook
            return 0;
    }

    port_id = data->port_id;
    msg_id = data->msg_id;
    data_type = data->data_type;
    data_len = data->size;

    ipcm_info("cust recv port_id(%u) msg_id(%u) data_type(%u) data(%lx) len(%u)\n",
        port_id, msg_id, data_type, (unsigned long)data->data, data_len);

    if (data_type == MSG_TYPE_SHM) {
        for (i=0; i<data_len; i++) {
            u8 tmp = *(u8 *)(data->data + i);
            if (tmp != (0x5a + msg_id)) {
                ipcm_err("======cust recv data fail, data_type(%d), data[%d](%x), except(%x)\n",
                    data_type, i, tmp, 0x5a + msg_id);
                ret = -1;
            }
        }
    }
    if (data_type == MSG_TYPE_RAW_PARAM) {
        if (data->data != (void *)(unsigned long)(CUST_TEST_PARAM_DATA + port_id + msg_id)) {
            ipcm_err("======cust recv data fail, data_type(%d), data(%lx) except(%x)\n",
                data_type, (unsigned long)data->data, CUST_TEST_PARAM_DATA + port_id + msg_id);
            ret = -1;
        }
    }

    return ret;
}

s32 cust_test_init(void)
{
    s32 ret = 0;

    ret = cust_test_plat_init();
    if (ret) {
        ipcm_err("======cust test ipcm_cust_srv_init fail:%d.\n",ret);
        cust_test_uninit();
        return ret;
    }

    ret = ipcm_cust_register_handle(_cust_msg_process, (void *)(unsigned long)CUST_PRIV_DATA_MAGIC);
    if (ret) {
        ipcm_err("======cust test ipcm_cust_register_handle fail:%d.\n",ret);
        cust_test_uninit();
        return ret;
    }

    return ret;
}

s32 cust_test_uninit(void)
{
    return cust_test_plat_uninit();
}

s32 cust_test_register_handle_hook(CUST_MSGPROC_FN hook, void *priv)
{
    if (hook) {
        _handler_hook = hook;
        _hook_priv_data = priv;
        return 0;
    }
    ipcm_err("register hook fail, hook is null.\n");
    return -1;
}

s32 cust_test_send_msg(u8 cust_msg_kernel)
{
    s32 ret = 0;
    void *data;
    int i;
    u32 buf_size;
    u8 port_id_st = 0;
    u8 msg_id;

    if (cust_msg_kernel)
        port_id_st = IPCM_CUST_KER_PORT_ST;

    for (i=0; i<_IPCM_TEST_LOOP_TIME;i++) {

        buf_size = g_test_size_buf[i % g_test_size_cnt];
        data = ipcm_cust_get_buff(buf_size);
        if (!data) {
            ipcm_err("ipcm get buf fail i:%d.\n", i);
            usleep(_IPCM_MSG_SND_INTERVAL);
            continue;
        }
        msg_id = i%128;
        memset(data, 0x5a+msg_id, buf_size);

        ret = ipcm_cust_send_msg(port_id_st, msg_id, data, buf_size);
        if (ret) {
            ipcm_err("======ipcm_cust_send_msg send fail ret:%d\n", ret);
        }
        // waiting for msg recvied and buffer release by alios
        usleep(_IPCM_MSG_SND_INTERVAL);
    }
    return ret;
}

s32 cust_test_send_param(u8 cust_msg_kernel)
{
    s32 ret = 0;
    int i=0, j=0;
    u8 port_id_st = 0;

    if (cust_msg_kernel)
        port_id_st = IPCM_CUST_KER_PORT_ST;

    // fail param
    // for (i=0; i<IPCM_CUST_PORT_MAX; i++) {
    //     for (j=0; j<IPCM_MSG_ID_MAX; j++) {
    //         ret = ipcm_cust_send_param(i, j, CUST_TEST_PARAM_DATA + i + j);
    //         if (CVI_SUCCESS == ret) {
    //             ipcm_err("cust send param fail, i(%d) j(%d) except(-1) ret(%d).\n",
    //                 i, j, ret);
    //         }
    //     }
    // }
    // normal param
    for (i=port_id_st; i<IPCM_CUST_PORT_MAX;) {
        for (j=0; j<=IPCM_MSG_ID_MAX; j++) {
            ret = ipcm_cust_send_param(i, j, CUST_TEST_PARAM_DATA + i + j);
            if (CVI_SUCCESS != ret) {
                ipcm_err("======cust send param fail, i(%d) j(%d) except(0) ret(%d).\n",
                    i, j, ret);
            }
            // waiting for msg recvied and buffer release by alios
            usleep(_IPCM_MSG_SND_INTERVAL);
            j += 8;
        }
        i += _IPCM_CUST_PORT_INTERVAL;
    }

    usleep(1000*1000);

    return ret;
}

static CVI_S32 cust_msg_proc_common(CVI_VOID *pPriv, IPCM_CUST_MSG_S *pstData)
{
    if (NULL == pstData) {
        ipcm_err("====test fail:pstData is null.\n");
        return -1;
    }

    if ((CVI_U8)(unsigned long)pPriv != pstData->u8PortID) {
        ipcm_err("====test fail:pPriv(%d) and PortID(%d) not equal.\n",
            (int)(unsigned long)pPriv, pstData->u8PortID);
        return -1;
    }

    if (pstData->u8DataType == IPCM_MSG_TYPE_SHM) {
        ipcm_info("cust recv msg port id:%d data:%llx size:%u\n",
            pstData->u8PortID, *(unsigned long long *)pstData->stData.pData, pstData->stData.u32Size);
    } else {
        ipcm_info("cust recv param port id:%d param:%x\n",
            pstData->u8PortID, (unsigned int)(unsigned long)pstData->u32Param);
    }

    return 0;
}

s32 cust_cvi_test_send_msg(u8 cust_msg_kernel)
{
    s32 ret = 0;
    void *data;
    int i;
    u32 buf_size;
    u8 port_id_st = 0;
    u8 msg_id;

    if (cust_msg_kernel)
        port_id_st = IPCM_CUST_KER_PORT_ST;

    for (i=0; i<_IPCM_TEST_LOOP_TIME;i++) {
        msg_id = i % 128;

        buf_size = g_test_size_buf[i % g_test_size_cnt];
        data = CVI_IPCM_GetBuff(buf_size);
        if (!data) {
            ipcm_err("ipcm get buf fail.\n");
            usleep(_IPCM_MSG_SND_INTERVAL);
            continue;
        }
        memset(data, 0x5a+msg_id, buf_size);

        ret = CVI_IPCM_CustSendMsg(port_id_st, msg_id, data, buf_size);
        if (ret) {
            ipcm_err("======ipcm_cust_send_msg send fail ret:%d\n", ret);
        }
        // waiting for msg recvied and buffer release by alios
        usleep(_IPCM_MSG_SND_INTERVAL);
    }
    return ret;
}

s32 cust_cvi_test_send_param(u8 cust_msg_kernel)
{
    s32 ret = 0;
    int i=0, j=0;
    u8 port_id_st = 0;

    if (cust_msg_kernel)
        port_id_st = IPCM_CUST_KER_PORT_ST;

    // normal param
    for (i=port_id_st; i<IPCM_CUST_PORT_MAX;) {
        for (j=0; j<=IPCM_MSG_ID_MAX; j++) {
            ret = CVI_IPCM_CustSendParam(i, j, CUST_TEST_PARAM_DATA + i + j);
            if (CVI_SUCCESS != ret) {
                ipcm_err("======cust send param fail, i(%d) j(%d) except(0) ret(%d).\n",
                    i, j, ret);
            }
            // waiting for msg recvied and buffer release by alios
            usleep(_IPCM_MSG_SND_INTERVAL);
            j += 8;
        }
        i += _IPCM_CUST_PORT_INTERVAL;
    }

    usleep(1000*1000);

    return ret;
}

s32 ipcm_cust_test_cvi(u8 cust_msg_kernel)
{
    s32 ret = 0;
    int i = 0;

    ret = CVI_IPCM_Init();
    if (ret) {
        ipcm_err("======cust test CVI_IPCM_Init fail:%d.\n",ret);
        return ret;
    }

    for (i=0; i<IPCM_CUST_PORT_MAX;) {
        if (i < 80) {
            ret = CVI_IPCM_RegisterCustHandle(i, cust_msg_proc_1, (CVI_VOID *)(unsigned long)i);
        } else if (i < 160) {
            ret = CVI_IPCM_RegisterCustHandle(i, cust_msg_proc_2, (CVI_VOID *)(unsigned long)i);
        } else {
            ret = CVI_IPCM_RegisterCustHandle(i, cust_msg_proc_3, (CVI_VOID *)(unsigned long)i);
        }
        i += _IPCM_CUST_PORT_INTERVAL;
    }

    ret = CVI_IPCM_CustInit();
    if (ret) {
        ipcm_err("======cust test CVI_IPCM_CustInit fail:%d.\n",ret);
        return ret;
    }

    cust_cvi_test_send_msg(cust_msg_kernel);
    cust_cvi_test_send_param(cust_msg_kernel);

    ret = CVI_IPCM_CustUninit();
    if (ret) {
        ipcm_err("======cust test CVI_IPCM_CustUninit fail:%d.\n",ret);
        return ret;
    }

    ret = CVI_IPCM_Uninit();
    if (ret) {
        ipcm_err("======cust test CVI_IPCM_Uninit fail:%d.\n",ret);
        return ret;
    }

    return 0;
}

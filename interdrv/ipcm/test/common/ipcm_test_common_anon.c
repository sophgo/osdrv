
#include <string.h>
#include <unistd.h>
#include "ipcm_port_common.h"
#include "ipcm_test_common.h"
#include "ipcm_test_common_anon.h"
#include "cvi_comm_ipcm.h"
#include "cvi_ipcm.h"

#define ANON_PRIV_DATA_MAGIC 0x443355aa
#define ANON_TEST_PARAM_DATA 0x4a5a2230

#define ANON_MSG_PROC_FUNCTION(ID) \
    static CVI_S32 anon_msg_proc_##ID(CVI_VOID *pPriv, IPCM_ANON_MSG_S *pstData) \
    { \
        return anon_msg_proc_common(pPriv, pstData); \
    }

static ANON_MSGPROC_FN _handler_hook = NULL;
static void *_hook_priv_data = NULL;

static CVI_S32 anon_msg_proc_common(CVI_VOID *pPriv, IPCM_ANON_MSG_S *pstData);

// declare handle function
ANON_MSG_PROC_FUNCTION(1)
ANON_MSG_PROC_FUNCTION(2)
ANON_MSG_PROC_FUNCTION(3)

static s32 _anon_msg_process(void *priv, ipcm_anon_msg_t *data)
{
    u8 port_id, msg_id, data_type;
    u32 data_len;
    u32 i = 0;
    s32 ret = 0;

    if (priv != (void *)ANON_PRIV_DATA_MAGIC) {
        ipcm_err("======anon test fail, reg handle magic err.\n");
        return -1;
    }
    if (data == NULL) {
        ipcm_err("======anon test fail, handle data is null.\n");
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

    ipcm_info("anon recv port_id(%u) msg_id(%u) data_type(%u) data(%lx) len(%u)\n",
        port_id, msg_id, data_type, (unsigned long)data->data, data_len);
    
    if (data_type == MSG_TYPE_SHM) {
        for (i=0; i<data_len; i++) {
            u8 tmp = *(u8 *)(data->data + i);
            if (tmp != (0x5a + msg_id)) {
                ipcm_err("======anon recv data fail, data_type(%d), data[%d](%x), except(%x)\n",
                    data_type, i, tmp, 0x5a + msg_id);
                ret = -1;
            }
        }
    }
    if (data_type == MSG_TYPE_RAW_PARAM) {
        if (data->data != (void *)(unsigned long)(ANON_TEST_PARAM_DATA + port_id + msg_id)) {
            ipcm_err("======anon recv data fail, data_type(%d), data(%lx) except(%x)\n",
                data_type, (unsigned long)data->data, ANON_TEST_PARAM_DATA + port_id + msg_id);
            ret = -1;
        }
    }

    return ret;
}

s32 anon_test_init(void)
{
    s32 ret = 0;

    ret = ipcm_anon_init();
    if (ret) {
        ipcm_err("======anon test ipcm_anon_init fail:%d.\n",ret);
        return ret;
    }

    ret = ipcm_anon_register_handle(_anon_msg_process, (void *)(unsigned long)ANON_PRIV_DATA_MAGIC);
    if (ret) {
        ipcm_err("======anon test ipcm_anon_register_handle fail:%d.\n",ret);
        ipcm_anon_uninit();
        return ret;
    }

    return ret;
}

s32 anon_test_uninit(void)
{
    return ipcm_anon_uninit();
}

s32 anon_test_register_handle_hook(ANON_MSGPROC_FN hook, void *priv)
{
    if (hook) {
        _handler_hook = hook;
        _hook_priv_data = priv;
        return 0;
    }
    ipcm_err("register hook fail, hook is null.\n");
    return -1;
}

s32 anon_test_send_msg(u8 anon_msg_kernel)
{
    s32 ret = 0;
    void *data;
    int i;
    u32 buf_size;
    u8 port_id_st = 0;
    u8 msg_id;

    if (anon_msg_kernel)
        port_id_st = IPCM_ANON_KER_PORT_ST;

    for (i=0; i<_IPCM_TEST_LOOP_TIME;i++) {

        buf_size = g_test_size_buf[i % g_test_size_cnt];
        data = ipcm_get_buff(buf_size);
        if (!data) {
            ipcm_err("ipcm get buf fail.\n");
            usleep(_IPCM_MSG_SND_INTERVAL);
            continue;
        }
        msg_id = i%128;
        memset(data, 0x5a+msg_id, buf_size);

        ret = ipcm_anon_send_msg(port_id_st, msg_id, data, buf_size);
        if (ret) {
            ipcm_err("======ipcm_anon_send_msg send fail ret:%d\n", ret);
        }
        // waiting for msg recvied and buffer release by alios
        usleep(_IPCM_MSG_SND_INTERVAL);
    }
    return ret;
}

s32 anon_test_send_param(u8 anon_msg_kernel)
{
    s32 ret = 0;
    int i=0, j=0;
    u8 port_id_st = 0;

    if (anon_msg_kernel)
        port_id_st = IPCM_ANON_KER_PORT_ST;

    // fail param
    // for (i=0; i<IPCM_ANON_PORT_MAX; i++) {
    //     for (j=0; j<IPCM_MSG_ID_MAX; j++) {
    //         ret = ipcm_anon_send_param(i, j, ANON_TEST_PARAM_DATA + i + j);
    //         if (CVI_SUCCESS == ret) {
    //             ipcm_err("anon send param fail, i(%d) j(%d) except(-1) ret(%d).\n",
    //                 i, j, ret);
    //         }
    //     }
    // }
    // normal param
    for (i=port_id_st; i<IPCM_ANON_PORT_MAX;) {
        for (j=0; j<=IPCM_MSG_ID_MAX; j++) {
            ret = ipcm_anon_send_param(i, j, ANON_TEST_PARAM_DATA + i + j);
            if (CVI_SUCCESS != ret) {
                ipcm_err("======anon send param fail, i(%d) j(%d) except(0) ret(%d).\n",
                    i, j, ret);
            }
            // waiting for msg recvied and buffer release by alios
            usleep(_IPCM_MSG_SND_INTERVAL);
            j += 8;
        }
        i += _IPCM_ANON_PORT_INTERVAL;
    }

    usleep(1000*1000);

    return ret;
}

static CVI_S32 anon_msg_proc_common(CVI_VOID *pPriv, IPCM_ANON_MSG_S *pstData)
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
        ipcm_info("anon recv msg port id:%d data:%llx size:%u\n",
            pstData->u8PortID, *(unsigned long long *)pstData->stData.pData, pstData->stData.u32Size);
    } else {
        ipcm_info("anon recv param port id:%d param:%x\n",
            pstData->u8PortID, (unsigned int)(unsigned long)pstData->u32Param);
    }

    return 0;
}

s32 anon_cvi_test_send_msg(u8 anon_msg_kernel)
{
    s32 ret = 0;
    void *data;
    int i;
    u32 buf_size;
    u8 port_id_st = 0;
    u8 msg_id;

    if (anon_msg_kernel)
        port_id_st = IPCM_ANON_KER_PORT_ST;

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

        ret = CVI_IPCM_AnonSendMsg(port_id_st, msg_id, data, buf_size);
        if (ret) {
            ipcm_err("======ipcm_anon_send_msg send fail ret:%d\n", ret);
        }
        // waiting for msg recvied and buffer release by alios
        usleep(_IPCM_MSG_SND_INTERVAL);
    }
    return ret;
}

s32 anon_cvi_test_send_param(u8 anon_msg_kernel)
{
    s32 ret = 0;
    int i=0, j=0;
    u8 port_id_st = 0;

    if (anon_msg_kernel)
        port_id_st = IPCM_ANON_KER_PORT_ST;

    // normal param
    for (i=port_id_st; i<IPCM_ANON_PORT_MAX;) {
        for (j=0; j<=IPCM_MSG_ID_MAX; j++) {
            ret = CVI_IPCM_AnonSendParam(i, j, ANON_TEST_PARAM_DATA + i + j);
            if (CVI_SUCCESS != ret) {
                ipcm_err("======anon send param fail, i(%d) j(%d) except(0) ret(%d).\n",
                    i, j, ret);
            }
            // waiting for msg recvied and buffer release by alios
            usleep(_IPCM_MSG_SND_INTERVAL);
            j += 8;
        }
        i += _IPCM_ANON_PORT_INTERVAL;
    }

    usleep(1000*1000);

    return ret;
}

s32 ipcm_anon_test_cvi(u8 anon_msg_kernel)
{
    s32 ret = 0;
    int i = 0;
    u32 boot_stat = 0;

    ret = CVI_IPCM_Init();
    if (ret) {
        ipcm_err("======anon test CVI_IPCM_Init fail:%d.\n",ret);
        return ret;
    }

    ret = CVI_IPCM_GetRtosBootStatus(&boot_stat);
    if (ret) {
        ipcm_err("======anon test CVI_IPCM_GetRtosBootStatus fail:%d.\n",ret);
    }
    ipcm_info("boot_stat:%d\n", boot_stat);

    for (i=0; i<IPCM_ANON_PORT_MAX;) {
        if (i < 80) {
            ret = CVI_IPCM_RegisterAnonHandle(i, anon_msg_proc_1, (CVI_VOID *)(unsigned long)i);
        } else if (i < 160) {
            ret = CVI_IPCM_RegisterAnonHandle(i, anon_msg_proc_2, (CVI_VOID *)(unsigned long)i);
        } else {
            ret = CVI_IPCM_RegisterAnonHandle(i, anon_msg_proc_3, (CVI_VOID *)(unsigned long)i);
        }
        i += _IPCM_ANON_PORT_INTERVAL;
    }

    ret = CVI_IPCM_AnonInit();
    if (ret) {
        ipcm_err("======anon test CVI_IPCM_AnonInit fail:%d.\n",ret);
        return ret;
    }

    anon_cvi_test_send_msg(anon_msg_kernel);
    anon_cvi_test_send_param(anon_msg_kernel);

    ret = CVI_IPCM_AnonUninit();
    if (ret) {
        ipcm_err("======anon test CVI_IPCM_AnonUninit fail:%d.\n",ret);
        return ret;
    }

    ret = CVI_IPCM_Uninit();
    if (ret) {
        ipcm_err("======anon test CVI_IPCM_Uninit fail:%d.\n",ret);
        return ret;
    }

    return 0;
}

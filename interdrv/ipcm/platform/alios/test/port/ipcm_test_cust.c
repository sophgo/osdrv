
#include <unistd.h>
#include <aos/kernel.h>
#include "ipcm_test_common_cust.h"

static int _cust_test_running = 0;
static PoolConfig *cust_config = NULL;

static void ipcm_cust_test_task(void *paras)
{
    u8 cust_msg_kernel = (u8)(unsigned long)paras;
    sleep(1);
    // test cust api
    cust_test_send_msg(cust_msg_kernel);
    cust_test_send_param(cust_msg_kernel);
    // test CVI_IPCM CUST api
    sleep(3);
    ipcm_cust_test_cvi(cust_msg_kernel);
    sleep(1);
    // restore ipcm cust handle
    cust_test_uninit();
    cust_test_init();

    _cust_test_running = 0;
}

static s32 _cust_test_handle_hook(void *priv, ipcm_cust_msg_t *data)
{
    u8 cust_msg_kernel = 0;
    if (_cust_test_running == 0) {
        _cust_test_running = 1;
        if (data && data->port_id >= IPCM_CUST_KER_PORT_ST)
            cust_msg_kernel = 1;

        aos_task_new("ipcm cust test task", ipcm_cust_test_task, (void *)(unsigned long)cust_msg_kernel, 4*1024);
    }

    return 0;
}

s32 cust_test_plat_init(void)
{
    s32 ret = 0;

	cust_config = (PoolConfig *)malloc(sizeof(PoolConfig) + 3 * sizeof(BlockConfig));
	cust_config->num = 3;
	cust_config->blk_conf[0].size = 256;
	cust_config->blk_conf[0].num = 4;
	cust_config->blk_conf[1].size = 512;
	cust_config->blk_conf[1].num = 3;
	cust_config->blk_conf[2].size = 1024;
	cust_config->blk_conf[2].num = 2;
    ret = ipcm_cust_srv_init(cust_config);
    if (cust_config) {
        free(cust_config);
        cust_config = NULL;
    }
    if (ret) {
        ipcm_err("======cust test ipcm_cust_srv_init fail:%d.\n",ret);
        return ret;
    }

    return ret;
}

s32 cust_test_plat_uninit(void)
{
    return ipcm_cust_srv_uninit();
}

s32 ipcm_cust_test_main(void)
{
    cust_test_init();
    cust_test_register_handle_hook(_cust_test_handle_hook, NULL);

    return 0;
}

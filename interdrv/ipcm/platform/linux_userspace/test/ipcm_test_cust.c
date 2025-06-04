
#include <unistd.h>
#include "ipcm_test_common.h"
#include "ipcm_test_common_cust.h"

s32 cust_test_plat_init(void)
{
    return ipcm_cust_cli_init();
}

s32 cust_test_plat_uninit(void)
{
    return ipcm_cust_cli_uninit();
}

s32 ipcm_cust_test_main(void)
{
    cust_test_init();
    cust_test_send_msg(0);
    cust_test_send_param(0);
	sleep(1); // wait for alios cust msg
    cust_test_uninit();
    return 0;
}

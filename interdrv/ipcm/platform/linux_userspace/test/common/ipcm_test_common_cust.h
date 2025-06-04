
#ifndef __IPCM_TEST_COMMON_CUST_H__
#define __IPCM_TEST_COMMON_CUST_H__

#include "ipcm_custom.h"

s32 cust_test_init(void);

s32 cust_test_uninit(void);

s32 cust_test_register_handle_hook(CUST_MSGPROC_FN hook, void *priv);

s32 cust_test_send_msg(u8 cust_msg_kernel);

s32 cust_test_send_param(u8 cust_msg_kernel);

s32 ipcm_cust_test_cvi(u8 cust_msg_kernel);

#endif

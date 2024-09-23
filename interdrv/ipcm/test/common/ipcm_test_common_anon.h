
#ifndef __IPCM_TEST_COMMON_ANON_H__
#define __IPCM_TEST_COMMON_ANON_H__

#include "ipcm_anonymous.h"

s32 anon_test_init(void);

s32 anon_test_uninit(void);

s32 anon_test_register_handle_hook(ANON_MSGPROC_FN hook, void *priv);

s32 anon_test_send_msg(u8 anon_msg_kernel);

s32 anon_test_send_param(u8 anon_msg_kernel);

s32 ipcm_anon_test_cvi(u8 anon_msg_kernel);

#endif

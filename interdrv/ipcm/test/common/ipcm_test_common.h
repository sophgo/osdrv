
#ifndef __IPCM_TEST_COMMON_H__
#define __IPCM_TEST_COMMON_H__

#include "ipcm_common.h"

#define _IPCM_TEST_LOOP_TIME 20
#define _IPCM_MSG_SND_INTERVAL (20*1000)
#define _IPCM_ANON_PORT_INTERVAL 15

extern unsigned long long t_recv;

extern u32 g_test_size_buf[];
extern int g_test_size_cnt;

typedef struct _IPC_TEST_DATA_SEND_T {
	unsigned long long t_send;
	u8 count;
	u8 resve[32];
} IPC_TEST_DATA_T;

int test_get_buff(void);

int test_send_msg(u8 msg_id, u8 msg_type, int count);

void ipcm_test_common(void);

#endif

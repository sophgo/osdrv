
#include <stdio.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <aos/kernel.h>
#include "ipcm_message.h"
#include "ipcm_test_msg.h"

typedef struct _IPCM_TEST_MSG_CTX {
	pthread_t recv_pid;
	u8 thread_status; // 0:stop 1:run
} IPCM_TEST_MSG_CTX;

static IPCM_TEST_MSG_CTX _msg_test_ctx = {};

static void *_test_recv_msg_proc(void *argv)
{
	int count = 0;
	int rcv_cont = 0;
	u8 *data;
	IPC_TEST_DATA_T *data_test;
	int len = 0;
	unsigned long long t_send, t4, t5;
	u8 msg_type;
	u8 msg_id;
	u32 remain_len;
	int ret = 0;

	_msg_test_ctx.thread_status = 1;
	while (_msg_test_ctx.thread_status) {
		ret = ipcm_msg_poll(0, 3000);
		if (ret)
			continue;
		t4 = timer_get_boot_us();
		len = ipcm_msg_read_data(0, (void **)&data, 4);
		if (len > 0) {
			ret = ipcm_msg_get_cur_msginfo(0, &msg_type, &msg_id, &remain_len);
			if (!ret) {
				if (msg_type == 0) {
					data_test = (IPC_TEST_DATA_T *)data;
					do {
						// printf("recv data %x len(%d)\n", data[0], len);
						len = ipcm_msg_read_data(0, (void **)&data, 4);
					} while (len > 0);
					t_send = data_test->t_send;
					t5 = timer_get_boot_us();
					ipcm_info("msg recv msg_id(%d) msg_type(%d) count(%d) t_send(%llu) t_recv(%llu)"
						" t4(%llu) t5(%llu) t4-t_recv(%llu) t5-t_send(%llu)\n",
						msg_id, 0, data_test->count, t_send, t_recv, t4, t5, t4-t_recv, t5-t_send);
					test_send_msg(count, 0, count);
				} else {
					t5 = timer_get_boot_us();
					rcv_cont = (unsigned long)data - 0xaabbccdd;
					ipcm_info("msg recv msg_id(%d) msg_type(%d) count(%d) t_recv(%llu) t4(%llu) t5(%llu)"
						" t4-t_recv(%llu)\n", msg_id, 1, rcv_cont, t_recv, t4, t5, t4-t_recv);
					test_send_msg(0x40 + count, 1, count);
				}
				count++;
			}

		}
		// sleep(1);
	}
	return NULL;
}

void test_resv_memory(void)
{
	int size[6] = {0x400, 0x100000, 0xA00000, 0x100000, 0x400, 0x500000}; // 1k  1m 10m 1m 1k 5m
	// int size[6] = {0x400, 0x800, 0x200, 0x400, 0x800, 0x200};
	void *data[6];
	int test_times = 3;
	int i, j;

	for (i = 0; i < test_times; i++) {
		ipcm_info("test times(%d):\n", i);
		for (j = 0; j < 6; j++) {
			data[j] = aos_ion_malloc(size[j]);
			ipcm_info("\tj(%d) addr(%p) size(%x)\n", j, data[j], size[j]);
		}
		for (j = 0; j < 6; j++) {
			aos_ion_free(data[j]);
		}
	}
}

int test_recv_msg(u8 msg_type)
{
	pthread_create(&_msg_test_ctx.recv_pid, NULL, _test_recv_msg_proc, (void *)(unsigned long)msg_type);
	return 0;
}

int test_recv_msg_stop(void)
{
	_msg_test_ctx.thread_status = 0;
	pthread_join(_msg_test_ctx.recv_pid, NULL);
	return 0;
}

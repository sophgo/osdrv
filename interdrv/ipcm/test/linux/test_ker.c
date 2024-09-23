

// #include <stdlib.h>
// #include <unistd.h>
#include <linux/delay.h>
#include "ipcm_message.h"
#include "ipcm_test_common.h"

int test_recv_msg_ker(void)
{
	int ret;
	int timeout;
	int count = 0;
	s32 len = 0;
	u8 msg_type;
	u8 msg_id;
	u32 remain_len;
	u8 *data;
	// IPC_TEST_DATA_T *data_test;
	// unsigned long long t_send, t4, t5;

	while (count < 20) {
		timeout = 3000;
        ret = ipcm_msg_poll(0, timeout);
		if (ret) {
			ipcm_info("SELECT timeout ret:%d\n", ret);
			// continue;
		}
		// t4 = timer_get_boot_us();
		len = ipcm_msg_read_data(0, (void **)&data, 128);
		// printf("recv1 len(%d)\n", len);
		if (len < 0) {
			ipcm_info("read ERR[%d]\n", ret);
			ret = -1;
			break;
		}
		ret = ipcm_msg_get_cur_msginfo(0, &msg_type, &msg_id, &remain_len);
		if (ret) {
			ipcm_info("ipcm_msg_get_cur_msginfo ERR[%d]\n", ret);
			break;
		}
		if (msg_type == 0) {
			// data_test = (IPC_TEST_DATA_T *)data;
			// t_send = data_test->t_send;
			// t5 = timer_get_boot_us();
			test_send_msg(count, 0, count);
		} else {
			ipcm_info("Recv msg:\n\tport_id(%d)\n\tmsg_id(%u)\n\tfunc_type(%u)\n\tparam(%lx)\n",
				0, msg_id, msg_type, (unsigned long)data);
			test_send_msg(0x40 + count, 1, count);
		}
		count++;
		msleep(1000);
	}
	return ret;
}

void _ipcm_test(void)
{
	int ret = 0;
	u8 msg_type = 0;

	ipcm_msg_init();
	ipcm_test_common();
	ipcm_msg_uninit();

	ret = ipcm_msg_init();
	ipcm_info("ipcm init\n");
	ipcm_info("sizeof u32(%zu) s32(%zu) u16(%zu) s16(%zu) u8(%zu) s8(%zu)\n", sizeof(u32),
		sizeof(s32), sizeof(u16), sizeof(s16), sizeof(u8), sizeof(u8));
	ipcm_info("data size:%zu\n", sizeof(IPC_TEST_DATA_T));
	// test_get_buff();

	test_send_msg(1, msg_type, 1);

	test_recv_msg_ker();

	msleep(1000);
	ipcm_msg_uninit();
	ipcm_info("ipcm uninit\n");
}


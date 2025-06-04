
#include "ipcm_plat_adapter.h"
#include "ipcm_message.h"
#include "ipcm_system.h"
#include "ipcm_test_common.h"

// case 1: ipcm msg init/ipcm msg uninit loop
int test_msg_init_loop(void)
{
	int loop_time = _IPCM_TEST_LOOP_TIME;
	int i = 0;
	s32 ret;
	PoolConfig *config = NULL;

	config = (PoolConfig *)ipcm_alloc(sizeof(PoolConfig) + 5 * sizeof(BlockConfig));
	config->num = 3;
	config->blk_conf[0].size = 64;
	config->blk_conf[0].num = 2;
	config->blk_conf[1].size = 128;
	config->blk_conf[1].num = 3;
	config->blk_conf[2].size = 256;
	config->blk_conf[2].num = 4;

	for (i=0; i<loop_time; i++) {
		ret = ipcm_msg_srv_init(config);
		if (ret < 0) {
			ipcm_err("UT IPCM ipcm_msg_srv_init fail:%d.\n", ret);
		}
		ret = ipcm_msg_srv_uninit();
		if (ret) {
			ipcm_err("UT IPCM ipcm_msg_srv_uninit fail:%d.\n", ret);
		}
	}

	for (i=0; i<loop_time; i++) {
		ret = ipcm_msg_cli_init();
		if (ret < 0) {
			ipcm_err("UT IPCM ipcm_msg_cli_init fail:%d.\n", ret);
		}
		ret = ipcm_msg_cli_uninit();
		if (ret) {
			ipcm_err("UT IPCM ipcm_msg_cli_uninit fail:%d.\n", ret);
		}
	}
	ipcm_free(config);
	return ret;
}

// case 2: ipcm_msg_get_buff/ipcm_msg_release_buff loop
int test_msg_get_buff_loop(void)
{
	int loop_time = _IPCM_TEST_LOOP_TIME;
	int i = 0;
	s32 ret;
	void *buf_data;

	for (i=0; i<loop_time; i++) {
		buf_data = ipcm_msg_get_buff(g_test_size_buf[i%g_test_size_cnt]);
		ret = ipcm_msg_release_buff(buf_data);
		if (ret) {
			ipcm_err("UT IPCM ipcm_msg_release_buff fail:%d\n", ret);
		}
	}
	return ret;
}

int test_get_buff(void)
{
	#define GET_BUF_TEST_NUM 8
	void *datap[GET_BUF_TEST_NUM] = {0};
	u32 sizea[GET_BUF_TEST_NUM] = {16, 32, 64, 128, 256, 512, 1024, 1536};
	int i = 0;
	int times = 0;

	for (times = 0; times < _IPCM_TEST_LOOP_TIME; times++) {
		for (i = 0; i < GET_BUF_TEST_NUM; i++) {
			datap[i] = ipcm_msg_get_buff(sizea[i]);
			ipcm_info("size(%d) addr(0x%p)\n", sizea[i], datap[i]);
		}
		for (i = 0; i < GET_BUF_TEST_NUM; i++) {
			ipcm_msg_release_buff(datap[i]);
			datap[i] = NULL;
		}
	}
	return 0;
}

int test_send_msg(u8 msg_id, u8 msg_type, int count)
{
	IPC_TEST_DATA_T *data_test;
	unsigned long long t1, t2;

	t1 = timer_get_boot_us();
	if (msg_type == 0) {
		data_test = (IPC_TEST_DATA_T *)ipcm_msg_get_buff(sizeof(IPC_TEST_DATA_T));

		memset(data_test, 0, sizeof(IPC_TEST_DATA_T));
		data_test->t_send = t1;
		data_test->count = count;
		ipcm_msg_send_msg(0, msg_id, data_test, sizeof(IPC_TEST_DATA_T));
	} else {
		ipcm_msg_send_param(0, msg_id, 0xaabbccdd+count);
	}
	t2 = timer_get_boot_us();
	UNUSED(t2);
	ipcm_info("msg send msgid(%d) msg_type(%d) count(%d) t1(%llu) t2(%llu) diff(%llu)\n",
		msg_id, msg_type, count, t1, t2, t2-t1);
	return 0;
}

void ipcm_test_common(void)
{
	int ret = 0;

	// ret = test_msg_init_loop();
	// if (ret) {
	// 	ipcm_err("IPCM UT test_msg_init_loop fail:%d\n", ret);
	// }

	ret = test_msg_get_buff_loop();
	if (ret) {
		ipcm_err("IPCM UT test_msg_get_buff_loop fail:%d\n", ret);
	}

	ret = test_get_buff();
	if (ret) {
		ipcm_err("IPCM UT test_get_buff fail:%d\n", ret);
	}
}


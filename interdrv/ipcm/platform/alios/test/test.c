
#if CONFIG_IPCM_MSG_TEST
#include <stdio.h>
#include <unistd.h>
#include <aos/cli.h>
#include <debug/dbg.h>
#include "ipcm_message.h"
#include "ipcm_system.h"
#include "ipcm_port.h"
#include "ipcm_test_msg.h"
#include "ipcm_test_sys.h"
#include "ipcm_test_common_cust.h"
#include "ipcm_test_cust.h"

extern s32 CVI_MSG_Deinit(void);

static u8 _msg_type = 0;

static void ipcm_test_print_help(int32_t argc, char **argv)
{
	ipcm_info("ipcm_test help:\r\n");
	ipcm_info("ipcm_test start        : restart ipcm server.\r\n");
	ipcm_info("ipcm_test stop         : stop ipcm server.\r\n");
	ipcm_info("ipcm_test ut           : run ipcm ut(should start server first).\r\n");
	ipcm_info("ipcm_test sndmsg 0xxxx : only send msg.\r\n");
}

static s32 ipcm_test_server_stop(void)
{
	cust_test_uninit();
	ipcm_sys_uninit();
	test_recv_msg_stop();
	ipcm_msg_srv_uninit();
	ipcm_port_uninit();
	return 0;
}

static s32 ipcm_test_server_restart(u8 msg_type)
{
	PoolConfig *config = NULL;

	CVI_MSG_Deinit();
	ipcm_test_server_stop();

	ipcm_port_init();
	ipcm_port_reset_pool_mgr();

	config = (PoolConfig *)malloc(sizeof(PoolConfig) + 5 * sizeof(BlockConfig));
	config->num = 5;
	config->blk_conf[0].size = 64;
	config->blk_conf[0].num = 2;
	config->blk_conf[1].size = 128;
	config->blk_conf[1].num = 3;
	config->blk_conf[2].size = 256;
	config->blk_conf[2].num = 4;
	config->blk_conf[3].size = 512;
	config->blk_conf[3].num = 5;
	config->blk_conf[4].size = 1024;
	config->blk_conf[4].num = 6;
	ipcm_msg_srv_init(config);
	free(config);
	test_recv_msg(msg_type);
	ipcm_sys_init();
	ipcm_cust_test_main();

	return 0;
}

static s32 ipcm_test_run_ut(u8 msg_type)
{
	test_resv_memory();

	test_recv_msg_stop();
	ipcm_test_common();
	ipcm_test_server_restart(msg_type); // should restart server after common test

	return 0;
}

static s32 ipcm_test_send_msg(unsigned long msg)
{
	printk("%s msg %x %lu\n", __func__, &msg, msg);
	return ipcm_port_send_msg((MsgData *)&msg);
}

static void ipcm_test_task(void *paras)
{
	u8 msg_type = (u8)(long)paras;
	ipcm_test_server_restart(msg_type);
	ipcm_test_run_ut(msg_type);
}

void ipcm_test_demaon(void)
{
	printf("enter ipcm_test_demaon\n");
	aos_task_new("ipcm test task", ipcm_test_task, (void *)(long)_msg_type, 8*1024);
	printf("ipcm_test_demaon done.\n");
}

void ipcm_test_cmd(char *buf, int32_t len, int32_t argc, char **argv)
{
	// if (argc >= 2) {
	// 	msg_type = strtoul(argv[1], NULL, 0);
	// }

	if (argc < 2) {
		ipcm_test_print_help(argc, argv);
		return;
	}

	/**
	 * cmdset 1
	 * ipcm_test serv_restart // server restart
	 * cmdset 2
	 * ipcm_test serv_stop // server stop
	 * cmdset 3
	 * ipcm_test ut_run // ut run
	 * cmdset 4
	 * ipcm_test snd_msg 0x // send msg
	 */

	if (0 == strcmp(argv[1], "start")) {
		ipcm_test_server_restart(_msg_type);
	}
	else if (0 == strcmp(argv[1], "stop")) {
		ipcm_test_server_stop();
	}
	else if (0 == strcmp(argv[1], "ut")) {
		ipcm_test_run_ut(_msg_type);
	}
	else if (0 == strcmp(argv[1], "sndmsg")) {
		if (argc >= 3) {
			unsigned long param = atol(argv[2]);
			printk("sndmsg param:%x\n", param);
			ipcm_test_send_msg(param);
		} else {
			ipcm_test_print_help(argc, argv);
		}
	} else {
		ipcm_test_print_help(argc, argv);
	}
}

static const struct cli_command test_ipcm_cmd[] = {
	{"ipcm_test", "ipcm driver test", ipcm_test_cmd},
};

void ipcm_driver_test_init(void)
{
	int32_t ret;

	ret = aos_cli_register_commands(test_ipcm_cmd, sizeof(test_ipcm_cmd) / sizeof(struct cli_command));
	if (ret) {
		printf("%s %d failed, ret = %d\r\n", __func__, __LINE__, ret);
	}
}
#else
void ipcm_driver_test_init(void)
{
}
#endif

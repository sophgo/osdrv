
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "ipcm_system.h"
#include "ipcm_port.h"
// #include "ipcm_test_msg.h"
#include "ipcm_test_sys.h"
#include "ipcm_test_cust.h"
#include "ipcm_test_common_cust.h"

int main(int argc, char **argv)
{
	(void)(argc);
	(void)(argv);
	// int fd = -1;
	// u8 msg_type = 0;
	// int ret = 0;
	// struct dump_uart_s *uart;
	// char *uart_ptr;

	ipcm_port_init();

	// ipcm_test_common();
#if 1
	// fd = ipcm_msg_init();
	printf("ipcm init\n");
	printf("sizeof u32(%zu) s32(%zu) u16(%zu) s16(%zu) u8(%zu) s8(%zu)\n", sizeof(u32),
		sizeof(s32), sizeof(u16), sizeof(s16), sizeof(u8), sizeof(u8));
	// test_get_buff();
	// if (argc >= 2) {
	// 	msg_type = atoi(argv[1]);
	// }

	printf("data size:%zu\n", sizeof(IPC_TEST_DATA_T));

	// ipcm_sys_init();
	// ret = ipcm_msg_sys_get_log(&uart);
	// if (0 == ret) {
	// 	printf("%p\n", uart);
	// 	uart_ptr = (char *)uart + sizeof(struct dump_uart_s);
	// 	printf("ptr:%p max:%d pos:%d e:%d o:%d\n", uart_ptr,
	// 		uart->dump_uart_max_size, uart->dump_uart_pos,
	// 		uart->dump_uart_enable, uart->dump_uart_overflow);
	// 	printf("%s\n", uart_ptr);
	// }
	// ipcm_sys_uninit();

	// test_send_msg(1, msg_type, 1);

	// ipcm_msg_uninit();

	ipcm_cust_test_main();
	ipcm_cust_test_cvi(0);

	ipcm_port_uninit();
	printf("ipcm uninit\n");
#endif
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <getopt.h>

#include "cvi_ipcm.h"
#include "ipcm_port.h"
#include <ctype.h>
#define TPU_SRAM_IPCM_BASE 0xE000000
#define BACKSPACE_ASCII	   8

static char *base = NULL;
static uint32_t log_paddr, log_size;
static int thread_stop = 0;
static struct termios old;

#define SEG_IDX_SAVE_PATH "/tmp/seg_idx"
/****
 * share memory map 
 * ------------ TAIL
 * ------------ TAIL - 1K ; 1k for param/param_bak/pq_bin/virtual log addr or size
 * ------------ TAIL - 2K ; 1k for virtual cmd while alios panic; usage: alios_cli panic
 * 
 * for ipcm msg
 * 
 * ----------- HEAD
*/
#define PANIC_CMD_BUF_LEN 1024
#define CMD_HEAD_MAGIC 0x7269
typedef struct _PANIC_LOG_CMD_HEAD {
	unsigned short magic;
	unsigned short cmd_len; // cmd buf len
	unsigned short wr_pos;
	unsigned short rd_pos;
	char cmd_content[0];
} __attribute__((__packed__)) PANIC_LOG_CMD_HEAD;

#define LOG_MODE_UART_MASK 0x1
#define LOG_MODE_FILE_MASK 0x2

static int b_alios_panic = 0;
static int log_mode = 1; // bit0: uart, bit1: log file; 0:close log
static int b_alios_cmd_support = 1;

static char *log_file = NULL;
static int log_fd = -1;

static PANIC_LOG_CMD_HEAD *painc_cmd_head = NULL;
static unsigned short painc_cmd_buf_len = 0;
static void *shm_base = NULL;
static uint32_t shm_size;

static int seg_idx_fd = -1;

static inline void print_log(char *base, const uint32_t start, const uint32_t end) {
	uint32_t i = 0;
	for (i = (start + 64); i < (end + 64); i++) {
		char c = base[i];
		if (isprint(c) || c == '\n' || c == '\r' || c == '\t') {
			if (log_mode & LOG_MODE_UART_MASK) {
				putchar(base[i]);
			}
			if (log_fd >= 0 && (log_mode & LOG_MODE_FILE_MASK)) {
				write(log_fd, &base[i], 1);
			}
		}
	}
}

static void config_termios(void)
{
	struct termios new_termios;

	tcgetattr(0, &new_termios);
	old = new_termios;

	// Enable char erase function
	new_termios.c_lflag = (ISIG | ICANON | ECHO | ECHOE | IEXTEN);
	new_termios.c_cc[VERASE] = BACKSPACE_ASCII;

	tcsetattr(0, TCSANOW, &new_termios);
}

static inline void revert_termios(void)
{
	tcsetattr(0, TCSANOW, &old);
}

/* Describe: get alios log addr and size from TPU SRAM
 * 
 */
int get_log_addr_and_size(int fd, uint32_t *log_paddr, uint32_t *log_size)
{
	uint32_t shm_paddr;
	uint32_t addr, size;

	ipcm_port_get_shm_info(&shm_paddr, &shm_size);

	// Return fail when shm addr isn't from ddr and size bigger than 128KB
	if ((shm_paddr < 0x80000000) || (shm_size > 0x20000)) {
		printf("Not in dual os platfrom, shm_paddr:%x shm_size:%x\n", shm_paddr, shm_size);
		return -1;
	}

	shm_base = (void *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, shm_paddr);
	if (NULL == shm_base) {
		perror("mmap alios log mem failed\n");
		return -1;
	}

	// printf("shm_paddr:%x shm_base:%lx\n", shm_paddr, (unsigned long)shm_base);
	painc_cmd_head = (PANIC_LOG_CMD_HEAD *)(shm_base + shm_size - 2*1024);
	painc_cmd_buf_len = PANIC_CMD_BUF_LEN - (unsigned short)(unsigned long)&(((PANIC_LOG_CMD_HEAD *)0)->cmd_content);
	// printf("painc_cmd_head:%lx painc_cmd_buf_len:%u\n", (unsigned long)painc_cmd_head, painc_cmd_buf_len);

	addr = *(uint32_t *)(shm_base + shm_size - 4*5);
	size = *(uint32_t *)(shm_base + shm_size - 4*6);

	//printf("log addr : 0x%x , size : 0x%x\n", addr, size);

	// Return fail when alios log addr isn't from ddr and size bigger than 128KB
	if ((addr < 0x80000000)
		|| (size > 0x20000)) {
		printf("Alios log addr or size is not availed, addr : 0x%x , size : 0x%x\n", addr, size);
		return -1;
	}

	*log_paddr = addr;
	*log_size = size;

	return 0;
}

/* 
 * Describe: redirect C906L log to C906B
 * Param:
 *      base : mmap base
 * 		log_size: log buffer size
 */
void *thread_get_log(void *arg)
{
	uint32_t read_segment_idx = 0, read_mirror_times = 0;
	uint32_t segment_idx, mirror_times;

	(void)(arg);

	if (log_file != NULL) {
		log_fd = open(log_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (log_fd < 0) {
			printf("open log file failed\n");
		}
	}

	if (read(seg_idx_fd, &read_segment_idx, 4) < 0){
		read_segment_idx = 0;
	}

	segment_idx = *(uint32_t *)(base + log_size - 0x4);
	mirror_times = *(uint32_t *)(base + log_size - 0x8);
	// printf("####### read_segment_idx:%d segment_idx:%d mirror_times:%d\n",
	// 	read_segment_idx, segment_idx, mirror_times);

	do {
		if (mirror_times == read_mirror_times) {
			print_log(base, (read_segment_idx << 6), (segment_idx << 6));
		} else if ((mirror_times == (read_mirror_times + 1))
					&& (read_segment_idx > segment_idx)) {
			print_log(base, (read_segment_idx << 6), (log_size - 128));

			if (segment_idx > 0) {
				print_log(base, 0, (segment_idx << 6));
			}
		} else {
			print_log(base, ((segment_idx + 1) << 6), (log_size - 128));

			if (segment_idx > 0) {
				print_log(base, 0, (segment_idx << 6));
			}
		}
		printf("\n");

		read_segment_idx = segment_idx;
		read_mirror_times = mirror_times;

		lseek(seg_idx_fd, 0, SEEK_SET);
		write(seg_idx_fd, &read_segment_idx, 4);
		fsync(seg_idx_fd);

		do {
			sleep(3);
			madvise((void *)(unsigned long)log_paddr, 0x20000, MADV_DONTNEED);
			segment_idx = *(uint32_t *)(base + log_size - 0x4);
			mirror_times = *(uint32_t *)(base + log_size - 0x8);
		} while ((segment_idx == read_segment_idx) && (mirror_times == read_mirror_times));
	} while (!thread_stop);

	if (log_fd >= 0) {
		close(log_fd);
		log_fd = -1;
	}
	return NULL;
}

int cli(void)
{
	int ret = 0;
	void *data;
	CVI_U32 buf_size;
	char cmd[1024] = {0};
	char *p;

	ret = CVI_IPCM_CustInit();
	if (ret) {
		printf("======cust test CVI_IPCM_CustInit fail:%d.\n",ret);
		return ret;
	}

	config_termios();

	do {
		if (0 == b_alios_panic) {
			printf("(cli-uart)# ");
		} else {
			printf("(panic)# ");
		}
		fflush(stdout);
		memset(cmd, 0, 1024);
		p = fgets(cmd, 1024, stdin);
		if (NULL == p) {
			continue;
		}

		ret = strlen(cmd);
		//printf("cmd len: %d , %s\n", ret, cmd);
		if ((ret == 1) && (cmd[0] == '\n')) {
			continue;
		} else if (strncmp(cmd, "exit", 4) == 0) {
			break;
		}

		if (0 == b_alios_panic) {
			buf_size = ret;
			data = CVI_IPCM_GetBuff(buf_size);
			if (!data) {
				printf("ipcm get buf fail.\n");
				break;
			}

			memset(data, 0, buf_size);
			memcpy(data, cmd, buf_size);

			ret = CVI_IPCM_CustSendMsg(0, 0, data, buf_size);
			if (ret) {
				printf("======CVI_IPCM_CustSendMsg send fail ret:%d\n", ret);
			}

		} else {
			int cnt = 0;
			char *p_cmd = painc_cmd_head->cmd_content;
			// invalid head
			CVI_IPCM_InvData(painc_cmd_head, 64);
            // printf("### head magic:%x wr:%u rd:%u len:%u!\r\n", painc_cmd_head->magic,
            //     painc_cmd_head->wr_pos, painc_cmd_head->rd_pos, painc_cmd_head->cmd_len);
			while (painc_cmd_head->rd_pos != painc_cmd_head->wr_pos) {
				usleep(100*1000); // wait alios read cmd complete
				CVI_IPCM_InvData(painc_cmd_head, 64);
				cnt++;
				if (cnt >= 10) {
					printf("(cli-uart)# no log 1s wr:%u rd:%u len:%u!\r\n",
						painc_cmd_head->wr_pos, painc_cmd_head->rd_pos, painc_cmd_head->cmd_len);
					break;
				}
			}
			if (cnt >= 10) {
				continue;
			}
			if ((painc_cmd_head->wr_pos+ret) <= painc_cmd_buf_len) {
				memcpy(p_cmd + painc_cmd_head->wr_pos, cmd, ret);
				// printf("####1 wr:%u ret:%d\n", painc_cmd_head->wr_pos, ret);
				painc_cmd_head->wr_pos += ret;
				// printf("####2 wr:%u ret:%d\n", painc_cmd_head->wr_pos, ret);
			} else {
				unsigned short len_pre = painc_cmd_buf_len - painc_cmd_head->wr_pos;
				unsigned short len_post = ret - len_pre;
				memcpy(p_cmd + painc_cmd_head->wr_pos, cmd, len_pre);
				memcpy(p_cmd, cmd + len_pre, len_post);
				painc_cmd_head->wr_pos = len_post;
			}
			painc_cmd_head->cmd_len = ret;
			// flush cache
			CVI_IPCM_FlushData(painc_cmd_head, PANIC_CMD_BUF_LEN);
		}
	} while (!thread_stop);

	CVI_IPCM_CustUninit();

	return 0;
}

static void _print_help(char *s)
{
	printf("Usage: %s [options]\n", s);
	printf("Options:\n");
	printf("  -h, --help           print this message\n");
	printf("  -p, --panic          panic mode\n");
	printf("  -m, --log_mode       log mode\n");
	printf("\t\t\tbit0: uart, bit1: log file; 0:close log\n");
	printf("  -c, --cmd_support    cmd support\n");
	printf("  -l, --log_file       log file\n");
}

/* main
background example:	./alios_cli -m 3 -l /tmp/log -c 0 &
foreground:	./alios_cli
*/
int main(int argc, char *argv[]) {
    int c = 0;
    int option_index = 0;
	int fd = 0;
	pthread_t tid;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"panic", no_argument, 0, 'p'},
		{"log_mode", required_argument, 0, 'm'},
		{"cmd_support", required_argument, 0, 'c'},
		{"log_file", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "hpm:c:l:", long_options, &option_index)) != -1) {
		switch(c) {
			case 'h':
				_print_help(argv[0]);
				return 0;
			case 'p':
				b_alios_panic = 1;
				break;
			case 'm':
				log_mode = atoi(optarg);
				break;
			case 'c':
				b_alios_cmd_support = atoi(optarg);
				break;
			case 'l':
				log_file = optarg;
				break;
			case '?':
				break;
			default:
				break;
		}
	}
	printf("b_alios_panic:%d, b_alios_cmd_support:%d, log_mode:%d\n",
		b_alios_panic, b_alios_cmd_support, log_mode);

	fd = open("/dev/mem", O_RDWR | O_NDELAY);
	if (fd < 0) {
		perror("open /dev/mem failed\n");
		return -1;
	}

	CVI_IPCM_Init();

	ipcm_port_get_log_info(&log_paddr, &log_size);
	// if (get_log_addr_and_size(fd, &log_paddr, &log_size) < 0) {
	// 	printf("get log addr and size failed\n");
	// 	close(fd);
	// 	return -1;
	// }

	base = (char *)mmap(NULL, log_size, PROT_READ, MAP_SHARED, fd, log_paddr);
	if (NULL == base) {
		perror("mmap alios log mem failed\n");
		close(fd);
		return -1;
	}

	seg_idx_fd = open(SEG_IDX_SAVE_PATH, O_RDWR | O_CREAT);
	if (seg_idx_fd < 0) {
		printf("open %s failed\n", SEG_IDX_SAVE_PATH);
	}

	if (log_mode != 0) {
		pthread_create(&tid, NULL, thread_get_log, NULL);
	}
	if (b_alios_cmd_support) {
		cli();
		revert_termios();
		thread_stop = 1;
	}

	pthread_join(tid, NULL);
	munmap(base, log_size);
	munmap(shm_base, shm_size);
	CVI_IPCM_Uninit();
	close(fd);
	close(seg_idx_fd);

	return 0;
}

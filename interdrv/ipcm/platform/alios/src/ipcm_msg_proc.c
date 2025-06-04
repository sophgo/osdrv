#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "cvi_mailbox.h"
#include "ipcm.h"
#include "ipcm_common.h"

extern void ipcm_port_set_rs_log_stat(u8 stat);

// Print IPCM pool information and backtrace
int ipcm_print_pool_proc(void)
{
#if IPCM_PROC_SUPPORT
	int i,j = 0;
	void **trace;

	// Print pool allocation traces
	if (_port_ctx.gtrace) {
		ipcm_info("pool get trace:");
		for (i=0; i<_port_ctx.pool_block_total; i++) {
			ipcm_info("\nid:%d", i);
			trace = (void **)((char *)_port_ctx.gtrace + (i * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
			for (j=0; j<IPCM_MSG_BACKTRACE_LVL; j++) {
				ipcm_info(" <- %p", trace[j]);
			}
		}
	}

	// Print pool free traces
	if (_port_ctx.ftrace) {
		ipcm_info("\npool free trace:");
		for (i=0; i<_port_ctx.pool_block_total; i++) {
			ipcm_info("\nid:%d", i);
			trace = (void **)((char *)_port_ctx.ftrace + (i * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
			for (j=0; j<IPCM_MSG_BACKTRACE_LVL; j++) {
				ipcm_info(" <- %p", trace[j]);
			}
		}
	}
	ipcm_info("\n");
#endif
	return 0;
}

#if IPCM_PROC_SUPPORT
// Record pool allocation backtrace
static int _ipcm_record_pool_bt(POOLHANDLE handle, void **trace_base, void *data)
{
	u32 pos;
	u32 block_idx;
	void **trace;

	if (data == NULL || trace_base == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}

	// Get block index and record backtrace
	pos = pool_get_data_offset(handle, data);
	block_idx = pool_get_block_idx_by_offset(handle, pos);
	trace = (void **)((char *)trace_base + (block_idx * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
	aos_mutex_lock(&_port_ctx.trace_mutex, AOS_WAIT_FOREVER);
	backtrace_now_get(trace, IPCM_MSG_BACKTRACE_LVL, 1);
	aos_mutex_unlock(&_port_ctx.trace_mutex);
	return 0;
}

// Record pool backtrace by position
static int _ipcm_record_pool_bt_bypos(POOLHANDLE handle, void **trace_base, u32 pos)
{
	u32 block_idx;
	void **trace;

	if (trace_base == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}

	// Get block index and record backtrace
	block_idx = pool_get_block_idx_by_offset(_port_ctx.pool_shm, pos);
	trace = (void **)((char *)trace_base + (block_idx * IPCM_MSG_BACKTRACE_LVL * sizeof(void *)));
	aos_mutex_lock(&_port_ctx.trace_mutex, AOS_WAIT_FOREVER);
	backtrace_now_get(trace, IPCM_MSG_BACKTRACE_LVL, 1);
	aos_mutex_unlock(&_port_ctx.trace_mutex);
	return 0;
}
#endif

// Print debug command help information
static void ipcm_dbg_print_help(void)
{
    ipcm_info("ipcm_dbg help:\r\n");
    ipcm_info("ipcm_dbg info : show ipcm info.\r\n");
    ipcm_info("ipcm_dbg pool_info : show pool get/rls callstack.\r\n");
    ipcm_info("ipcm_dbg log0_on  : open ipcm recv and send log.\r\n");
    ipcm_info("ipcm_dbg log0_off : close ipcm recv and send log.\r\n");
    ipcm_info("ipcm_dbg logall_on  : open all log(set log level to debug).\r\n");
    ipcm_info("ipcm_dbg logall_off : reset log level to default.\r\n");
}

// Show IPCM statistics
static int ipcm_info_show(void)
{
    ipcm_info("mailbox not valid cnt:%d\n", mailbox_get_invalid_cnt());
    ipcm_info("recv msg cnt:%d\n", ipcm_get_recv_msg_cnt());
    ipcm_info("send msg cnt:%d\n", ipcm_get_send_msg_cnt());
    return 0;
}

// Show pool information
static int ipcm_pool_info_show(void)
{
    ipcm_print_pool_proc();
    return 0;
}

// Debug command handler
static void ipcm_proc_debug(int32_t argc, char **argv)
{
    if (argc < 2) {
        ipcm_dbg_print_help();
        return;
    }

	// Handle different debug commands
    if (0 == strcmp(argv[1], "info")) {
	    ipcm_info_show();
    }else if (0 == strcmp(argv[1], "pool_info")) {
	    ipcm_pool_info_show();
    } else if (0 == strcmp(argv[1], "log0_on")) {
        ipcm_port_set_rs_log_stat(1);
    } else if (0 == strcmp(argv[1], "log0_off")) {
        ipcm_port_set_rs_log_stat(0);
    } else if (0 == strcmp(argv[1], "logall_on")) {
        ipcm_set_log_level(IPCM_LOG_DEBUG);
    } else if (0 == strcmp(argv[1], "logall_off")) {
        ipcm_set_log_level(IPCM_LOG_LEVEL_DEFAULT);
    } else {
        ipcm_dbg_print_help();
    }
}

// Register debug command with AliOS CLI
ALIOS_CLI_CMD_REGISTER(ipcm_proc_debug, ipcm_dbg, ipcm debug);

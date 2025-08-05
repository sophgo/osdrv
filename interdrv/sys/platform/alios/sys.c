#include <stdlib.h>
#include <aos/cli.h>
#include <debug/dbg.h>
#include "sys_uapi.h"
#include "comm_sys.h"
#include "comm_math.h"
#include "base_ctx.h"
#include "base_cb.h"
#include "vi_cb.h"
#include "vpss_cb.h"
#include "sys.h"
#include "sys_common.h"
#include "clk_list.h"
#include "vi_sys.h"
#include "sys_debug.h"
#include "osal.h"
#include "mmio.h"
#include "aos/kernel.h"

#define REG_VIP_BASE_ADDR		0x0a090000
#define TOP_BASE 0x3000000
#define RTC_BASE 0x05026000

struct sys_device {
	osal_mutex dev_lock;
	vi_vpss_mode_s vivpss_mode;
};


u32 sys_log_lv = DBG_WARN;
struct sys_device ndev;

#define WDT2_TOC_ADDR           (0x0301201C)

static void heartbeat_task_entry(void *arg)
{
	uint32_t count = 0;

	while (1) {
		count++;

		mmio_write_32(WDT2_TOC_ADDR, count);
		aos_msleep(500);
	}
}

int cvi_sys_heartbeatinit(void)
{
	static uint8_t heartbeat_inited = 0;
	aos_task_t task;

	if (heartbeat_inited != 0)
		return 0;

	aos_task_new_ext(&task, "heartbeat", heartbeat_task_entry, NULL,
					1024, AOS_DEFAULT_APP_PRI - 5);

	heartbeat_inited = 1;

	return 0;
}

static int _sys_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_SYS;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

long driver_sys_ioctl(unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct sys_device *dev = &ndev;

	switch (cmd) {
	case SYS_IOC_SET_VIVPSSMODE: {
		CHECK_IOCTL_CMD(cmd, vi_vpss_mode_s);

		osal_mutex_lock(&dev->dev_lock);
		osal_memcpy(&dev->vivpss_mode, (void *)arg, sizeof(vi_vpss_mode_s));
		osal_mutex_unlock(&dev->dev_lock);

		if (_sys_call_cb(E_MODULE_VI, VI_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			TRACE_SYS(DBG_ERR, "VI_CB_SET_VIVPSSMODE failed\n");
		}

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			TRACE_SYS(DBG_ERR, "VPSS_CB_SET_VIVPSSMODE failed\n");
		}

		break;
	}
	case SYS_IOC_GET_VIVPSSMODE: {
		CHECK_IOCTL_CMD(cmd, vi_vpss_mode_s);

		osal_mutex_lock(&dev->dev_lock);
		osal_memcpy((void *)arg, &dev->vivpss_mode, sizeof(vi_vpss_mode_s));
		osal_mutex_unlock(&dev->dev_lock);
		break;
		}
	case SYS_IOC_READ_CHIP_ID: {
		unsigned int chip_id = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		chip_id = sys_comm_read_chip_id();
		osal_memcpy((void *)arg, &chip_id, sizeof(unsigned int));
		break;
	}
	case SYS_IOC_READ_CHIP_VERSION: {
		unsigned int chip_version = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		chip_version = sys_comm_read_chip_version();
		osal_memcpy((void *)arg, &chip_version, sizeof(unsigned int));
		break;
	}
	case SYS_IOC_READ_CHIP_PWR_ON_REASON: {
		unsigned int reason = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		reason = sys_comm_read_chip_pwr_on_reason();
		osal_memcpy((void *)arg, &reason, sizeof(unsigned int));
		break;
	}
	default:
		TRACE_SYS(DBG_ERR, "cmd not find*****\n");
		return -1;
	}

	return ret;
}

int driver_sys_init(void)
{
	int i;

	TRACE_SYS(DBG_WARN, "+\n");
	cvi_sys_heartbeatinit();
	vi_sys_set_base_addr((void *)REG_VIP_BASE_ADDR);
	sys_comm_set_base_addr((void *)TOP_BASE, (void *)RTC_BASE);

	for (i = 0 ; i < ARRAY_SIZE(clk_list); i++) {
		osal_clk_register(&clk_list[i]);
	}

	osal_mutex_init(&ndev.dev_lock);
	TRACE_SYS(DBG_WARN, "-\n");

	return 0;
}

void driver_sys_exit(void)
{
	int i;

	for (i = 0 ; i < ARRAY_SIZE(clk_list); i++) {
		osal_clk_unregister(&clk_list[i]);
	}
	vi_sys_set_base_addr(NULL);
	osal_mutex_destroy(&ndev.dev_lock);
}

static void set_sys_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		aos_debug_printf("Set sys_log_lv, (%d) -> (%d).\n", sys_log_lv, level);
		sys_log_lv = level;
	} else {
		aos_debug_printf("sys_log_lv = %d.\n", sys_log_lv);
	}
}

ALIOS_CLI_CMD_REGISTER(set_sys_log_level, sys_log_lv, set sys_log_lv);

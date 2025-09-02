#include "vpss_debug.h"
#include "vpss_dev.h"
#include "vpss_ioctl.h"
#include "vpss_core.h"
#include "vpss_define.h"
#include <stdlib.h>
#include "drv/cvi_irq.h"
#include <aos/cli.h>

#define REG_VPSS_BASE_ADDR		(void *)0x0a080000
#define VPSS_INT0 21
#define VPSS_INT1 22
#define VPSS_INT2 23
#define VPSS_INT3 24

u32 vpss_log_lv = DBG_WARN;

static const char *const vpss_name[] = {"vpss_v0", "vpss_v1", "vpss_v2", "vpss_v3"};
static const char *const vpss_clk_name[VPSS_MAX] = {"reg_clk_vpss0_vip_en",
													"reg_clk_vpss1_vip_en",
													"reg_clk_vpss2_vip_en",
													"reg_clk_vpss3_vip_en"};

struct vpss_cores *vpss_cores;

struct vpss_cores *vpss_get_dev(void)
{
	return vpss_cores;
}

int vpss_isr(int irq, void *data)
{
	vpss_core_isr(irq, data);
	return 1;
}

long driver_vpss_ioctl(unsigned int cmd, unsigned long arg)
{
	return vpss_ioctl(vpss_cores, cmd, arg);
}

int driver_vpss_init(void)
{
	int rc = 0;
	int i;
	int irq_num[] = {VPSS_INT0, VPSS_INT1, VPSS_INT2, VPSS_INT3};

	TRACE_VPSS(DBG_WARN, "+\n");

	vpss_cores = osal_zalloc(sizeof(struct vpss_cores));
	if (!vpss_cores) {
		TRACE_VPSS(DBG_ERR, "Failed to allocate resource\n");
		return -1;
	}

	vpss_set_base_addr(REG_VPSS_BASE_ADDR);

	for (i = 0; i < VPSS_MAX; ++i) {
		vpss_cores->core[i].irq_num = irq_num[i];
		rc = osal_irq_request(vpss_cores->core[i].irq_num, vpss_isr, 0 , vpss_name[i], (void *)&vpss_cores->core[i]);
		if(rc != 0)
		{
			TRACE_VPSS(DBG_ERR, "Failed to request vpss irq:%d\n", vpss_cores->core[i].irq_num);
		}
		vpss_cores->core[i].clk = osal_clk_get(NULL, vpss_clk_name[i]);
		if (vpss_cores->core[i].clk == NULL) {
			TRACE_VPSS(DBG_ERR, "Cannot get source clk for vpss%d\n", i);
		}
	}

	vpss_core_init(vpss_cores);
	vpss_core_open(vpss_cores);
	TRACE_VPSS(DBG_WARN, "-\n");

	return rc;
}

void driver_vpss_exit(void)
{
	int i;

	TRACE_VPSS(DBG_INFO, " +\n");

	vpss_core_release(vpss_cores);
	vpss_core_deinit(vpss_cores);
	for (i = 0; i < VPSS_MAX; ++i) {
		osal_clk_put(NULL, vpss_cores->core[i].clk);
		vpss_cores->core[i].clk = NULL;
	}
}

void driver_vpss_release(void)
{
	TRACE_VPSS(DBG_WARN, " +\n");
	vpss_core_stop(vpss_cores);
	TRACE_VPSS(DBG_WARN, " -\n");
}

static void set_vpss_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		aos_debug_printf("Set vpss_log_lv, (%d) -> (%d).\n", vpss_log_lv, level);
		vpss_log_lv = level;
	} else {
		aos_debug_printf("vpss_log_lv = %d.\n", vpss_log_lv);
	}
}

ALIOS_CLI_CMD_REGISTER(set_vpss_log_level, vpss_log_lv, set_vpss_log_lv);

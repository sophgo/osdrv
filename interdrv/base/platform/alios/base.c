#include <stdlib.h>
#include <string.h>
#include <aos/cli.h>
#include <debug/dbg.h>
#include <time.h>
#include "osal.h"
#include "bind.h"
#include "ion.h"
#include "vb.h"
#include "base_debug.h"


u32 base_log_lv = DBG_WARN;
uint32_t vb_max_pools = 512;
uint32_t vb_pool_max_blk = 128;

// External log level variables from other modules
extern u32 sys_log_lv;
extern u32 vi_log_lv;
extern u32 vo_log_lv;
extern u32 vpss_log_lv;
extern u32 rgn_log_lv;
extern u32 ldc_log_lv;
extern u32 venc_log_lv;

// Log level control structure
struct log_module_info {
	const char *name;
	u32 *log_lv_ptr;
};

static struct log_module_info log_modules[] = {
	{"BASE",  &base_log_lv},
	{"SYS",   &sys_log_lv},
	{"VI",    &vi_log_lv},
	{"VO",    &vo_log_lv},
	{"VPSS",  &vpss_log_lv},
	{"RGN",   &rgn_log_lv},
	{"LDC",   &ldc_log_lv},
	{"VENC",  &venc_log_lv},
};

static long vb_ctrl(unsigned long arg)
{
	long ret = 0;
	struct vb_ext_control p;
	osal_memcpy(&p, (void *)arg, sizeof(struct vb_ext_control));

	switch (p.id) {
	case VB_IOCTL_SET_CONFIG: {
		struct vb_cfg cfg;

		osal_memset(&cfg, 0, sizeof(struct vb_cfg));
		osal_memcpy(&cfg, p.ptr, sizeof(struct vb_cfg));
		ret = vb_set_config(&cfg);
		break;
	}

	case VB_IOCTL_GET_CONFIG: {
		struct vb_cfg cfg;

		ret = vb_get_config(&cfg);
		osal_memcpy(p.ptr, &cfg, sizeof(struct vb_cfg));
		break;
	}

	case VB_IOCTL_INIT:
		ret = vb_init();
		break;

	case VB_IOCTL_EXIT:
		ret = vb_exit();
		break;

	case VB_IOCTL_CREATE_POOL: {
		struct vb_pool_cfg cfg;

		osal_memset(&cfg, 0, sizeof(struct vb_pool_cfg));
		osal_memcpy(&cfg, p.ptr, sizeof(struct vb_pool_cfg));

		ret = vb_create_pool(&cfg);
		if (ret == 0) {
			osal_memcpy(p.ptr, &cfg, sizeof(struct vb_pool_cfg));
		}
		break;
	}

	case VB_IOCTL_CREATE_EX_POOL: {
		struct vb_pool_ex_cfg *cfg;

		cfg = (struct vb_pool_ex_cfg *)osal_kmalloc(sizeof(struct vb_pool_ex_cfg), OSAL_GFP_ATOMIC);
		osal_memset(cfg, 0, sizeof(struct vb_pool_ex_cfg));
		osal_memcpy(cfg, p.ptr, sizeof(struct vb_pool_ex_cfg));

		ret = vb_create_ex_pool(cfg);
		if (ret == 0) {
			osal_memcpy(p.ptr, cfg, sizeof(struct vb_pool_ex_cfg));
		}
		osal_kfree(cfg);
		break;
	}

	case VB_IOCTL_DESTROY_POOL: {
		vb_pool pool_id;

		pool_id = (vb_pool)p.value;
		ret = vb_destroy_pool(pool_id);
		break;
	}

	case VB_IOCTL_GET_BLOCK: {
		struct vb_blk_cfg cfg;
		vb_blk block;

		osal_memset(&cfg, 0, sizeof(struct vb_blk_cfg));
		osal_memcpy(&cfg, p.ptr, sizeof(struct vb_blk_cfg));
		block = vb_get_block_with_id(cfg.pool_id, cfg.blk_size, ID_USER);
		if (block == VB_INVALID_HANDLE)
			ret = OSAL_ENOMEM;
		else {
			cfg.blk = (uint64_t)block;
			osal_memcpy(p.ptr, &cfg, sizeof(struct vb_blk_cfg));
		}
		break;
	}

	case VB_IOCTL_RELEASE_BLOCK: {
		vb_blk blk = (vb_blk)p.value64;

		ret = vb_release_block(blk);
		break;
	}

	case VB_IOCTL_PHYS_TO_HANDLE: {
		struct vb_blk_info blk_info;
		vb_blk block;

		osal_memset(&blk_info, 0, sizeof(struct vb_blk_info));
		osal_memcpy(&blk_info, p.ptr, sizeof(struct vb_blk_info));

		block = vb_phys_addr2handle(blk_info.phy_addr);
		if (block == VB_INVALID_HANDLE)
			ret = OSAL_EINVAL;
		else {
			blk_info.blk = (uint64_t)block;
			osal_memcpy(p.ptr, &blk_info, sizeof(struct vb_blk_info));
		}
		break;
	}

	case VB_IOCTL_GET_BLK_INFO: {
		struct vb_blk_info blk_info;

		osal_memset(&blk_info, 0, sizeof(struct vb_blk_info));
		osal_memcpy(&blk_info, p.ptr, sizeof(struct vb_blk_info));

		ret = vb_get_blk_info(&blk_info);
		if (ret == 0) {
			osal_memcpy(p.ptr, &blk_info, sizeof(struct vb_blk_info));
		}
		break;
	}

	case VB_IOCTL_GET_POOL_CFG: {
		struct vb_pool_cfg pool_cfg;

		osal_memset(&pool_cfg, 0, sizeof(struct vb_pool_cfg));
		osal_memcpy(&pool_cfg, p.ptr, sizeof(struct vb_pool_cfg));
		ret = vb_get_pool_cfg(&pool_cfg);
		if (ret == 0) {
			osal_memcpy(p.ptr, &pool_cfg, sizeof(struct vb_pool_cfg));
		}
		break;
	}

	case VB_IOCTL_GET_POOL_MAX_CNT: {
		p.value = vb_get_pool_max_cnt();
		break;
	}

	case VB_IOCTL_PRINT_POOL: {
		vb_pool pool_id;

		pool_id = (vb_pool)p.value;
		ret = vb_print_pool(pool_id);
		break;
	}

	default:
		break;
	}

	osal_memcpy((void *)arg, &p, sizeof(struct vb_ext_control));
	return ret;
}

long driver_base_ioctl(unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	switch (cmd) {
	case BASE_VB_CMD:
	{
		CHECK_IOCTL_CMD(cmd, struct vb_ext_control);
		ret = vb_ctrl(arg);
		break;
	}

	case BASE_SET_BINDCFG:
	{
		struct sys_bind_cfg bind_cfg;

		CHECK_IOCTL_CMD(cmd, struct sys_bind_cfg);
		osal_memcpy(&bind_cfg, (struct sys_bind_cfg *)arg, sizeof(struct sys_bind_cfg));
		if (bind_cfg.is_bind)
			ret = bind(&bind_cfg.mmf_chn_src, &bind_cfg.mmf_chn_dst);
		else
			ret = unbind(&bind_cfg.mmf_chn_src, &bind_cfg.mmf_chn_dst);
		break;
	}

	case BASE_GET_BINDCFG:
	{
		struct sys_bind_cfg bind_cfg;

		CHECK_IOCTL_CMD(cmd, struct sys_bind_cfg);
		osal_memcpy(&bind_cfg, (struct sys_bind_cfg *)arg, sizeof(struct sys_bind_cfg));
		if (bind_cfg.get_by_src)
			ret = bind_get_dst(&bind_cfg.mmf_chn_src, &bind_cfg.bind_dst);
		else
			ret = bind_get_src(&bind_cfg.mmf_chn_dst, &bind_cfg.mmf_chn_src);

		if (ret) {
			TRACE_BASE(DBG_ERR, "BASE_GET_BINDCFG failed\n");
			return ret;
		}
		osal_memcpy((struct sys_bind_cfg *)arg, &bind_cfg, sizeof(struct sys_bind_cfg));
		break;
	}

	case BASE_ION_ALLOC:
	{
		struct sys_ion_data stIonDate;
		void *addr_v = NULL;

		CHECK_IOCTL_CMD(cmd, struct sys_ion_data);
		osal_memcpy(&stIonDate, (struct sys_ion_data *)arg, sizeof(struct sys_ion_data));
		ret = base_ion_alloc((uint64_t *)&stIonDate.addr_p, &addr_v, stIonDate.name,
				stIonDate.size, stIonDate.cached);
		osal_memcpy((struct sys_ion_data *)arg, &stIonDate, sizeof(struct sys_ion_data));

		break;
	}

	case BASE_ION_FREE:
	{
		struct sys_ion_data stIonDate;

		CHECK_IOCTL_CMD(cmd, struct sys_ion_data);
		osal_memcpy(&stIonDate, (struct sys_ion_data *)arg, sizeof(struct sys_ion_data));

		ret = base_ion_free(stIonDate.addr_p);
		if (ret < 0) {
			TRACE_BASE(DBG_ERR, "base_ion_free fail\n");
			return OSAL_EINVAL;
		}
		osal_memcpy((struct sys_ion_data *)arg, &stIonDate, sizeof(struct sys_ion_data));
		break;
	}

	case BASE_CACHE_INVLD:
	{
		struct sys_cache_op stCacheOp;

		CHECK_IOCTL_CMD(cmd, struct sys_cache_op);
		osal_memcpy(&stCacheOp, (struct sys_cache_op *)arg, sizeof(struct sys_cache_op));

		ret = base_ion_cache_invalidate(stCacheOp.addr_p, stCacheOp.addr_v, stCacheOp.size);
		break;
	}

	case BASE_CACHE_FLUSH:
	{
		struct sys_cache_op stCacheOp;

		CHECK_IOCTL_CMD(cmd, struct sys_cache_op);
		osal_memcpy(&stCacheOp, (struct sys_cache_op *)arg, sizeof(struct sys_cache_op));

		ret = base_ion_cache_flush(stCacheOp.addr_p, stCacheOp.addr_v, stCacheOp.size);
		break;
	}

	case BASE_GET_TIMESTAMP:
	{
		uint64_t timestamp;
		struct timespec ts;

		CHECK_IOCTL_CMD(cmd, uint64_t);

		clock_gettime(CLOCK_MONOTONIC, &ts);
		timestamp = ts.tv_sec*1000000 + ts.tv_nsec/1000;
		osal_memcpy((uint64_t *)arg, &timestamp, sizeof(uint64_t));
		break;
	}

	default:
		TRACE_BASE(DBG_ERR, "Not support functions");
		return -1;
	}
	return ret;
}

int driver_base_init(void)
{
	TRACE_BASE(DBG_WARN, "+\n");
	base_ion_init();
	if (vb_create_instance()) {
		TRACE_BASE(DBG_ERR, "vb_create_instance failed\n");
		return OSAL_ENOMEM;
	}
	bind_init();
	TRACE_BASE(DBG_WARN, "-\n");

	return 0;
}

void driver_base_exit(void)
{
	bind_deinit();
	vb_cleanup();
	vb_destroy_instance();
	base_ion_deinit();
}

void driver_base_release(void)
{
	TRACE_BASE(DBG_WARN, "+\n");
	bind_deinit();
	bind_init();
	TRACE_BASE(DBG_WARN, "-\n");
}

static void set_base_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		aos_debug_printf("Set base_log_lv, (%d) -> (%d).\n", base_log_lv, level);
		base_log_lv = level;
	} else {
		aos_debug_printf("base_log_lv = %d.\n", base_log_lv);
	}
}

static void log_lv_control(int32_t argc, char **argv)
{
	int level;
	int module_count = sizeof(log_modules) / sizeof(log_modules[0]);
	int i;
	int found = 0;

	// Case 1: proc_log - show all module log levels
	if (argc == 1) {
		aos_debug_printf("Module Log Levels:\n");
		aos_debug_printf("------------------\n");
		for (i = 0; i < module_count; i++) {
			aos_debug_printf("%-10s: %d\n", log_modules[i].name, *(log_modules[i].log_lv_ptr));
		}
		aos_debug_printf("\nUsage:\n");
		aos_debug_printf("  proc_log               - Show all module log levels\n");
		aos_debug_printf("  proc_log ALL <level>   - Set all modules to <level>\n");
		aos_debug_printf("  proc_log <MOD> <level> - Set specific module to <level>\n");
		aos_debug_printf("  (Modules: BASE, SYS, VI, VO, VPSS, RGN, LDC, VENC)\n");
		return;
	}

	// Case 2: proc_log <module> <level>
	if (argc >= 3) {
		level = atoi(argv[2]);

		if (level < DBG_ERR || level > DBG_DEBUG) {
			aos_debug_printf("Error: log level out of range [%d-%d]\n", DBG_ERR, DBG_DEBUG);
			aos_debug_printf("  1-ERR, 2-WARN, 3-NOTICE, 4-INFO, 5-DEBUG\n");
			return;
		}

		// Check if setting ALL modules
		if (strncmp(argv[1], "ALL", 3) == 0) {
			aos_debug_printf("Setting all modules log level to %d\n", level);
			for (i = 0; i < module_count; i++) {
				aos_debug_printf("  %-10s: %d -> %d\n",
					log_modules[i].name,
					*(log_modules[i].log_lv_ptr),
					level);
				*(log_modules[i].log_lv_ptr) = level;
			}
			return;
		}

		// Find and set specific module
		for (i = 0; i < module_count; i++) {
			if (strncmp(argv[1], log_modules[i].name, strlen(log_modules[i].name)) == 0) {
				aos_debug_printf("Setting %s log level: %d -> %d\n",
					log_modules[i].name,
					*(log_modules[i].log_lv_ptr),
					level);
				*(log_modules[i].log_lv_ptr) = level;
				found = 1;
				break;
			}
		}

		if (!found) {
			aos_debug_printf("Error: Module '%s' not found\n", argv[1]);
			aos_debug_printf("Available modules: BASE, SYS, VI, VO, VPSS, RGN, LDC, VENC\n");
		}
	} else {
		aos_debug_printf("Error: Invalid arguments\n");
		aos_debug_printf("Usage:\n");
		aos_debug_printf("  proc_log               - Show all module log levels\n");
		aos_debug_printf("  proc_log ALL <level>   - Set all modules to <level>\n");
		aos_debug_printf("  proc_log <MOD> <level> - Set specific module to <level>\n");
	}
}

ALIOS_CLI_CMD_REGISTER(set_base_log_level, base_log_lv, set base_log_lv);
ALIOS_CLI_CMD_REGISTER(log_lv_control, proc_log, unified log level control);

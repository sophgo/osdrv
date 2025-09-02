#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/of_platform.h>
#include <linux/version.h>
#include "defines.h"
#include "base_cb.h"
#include "gfbg_callback.h"
#include "gfbg_init.h"
#include "gfbg_debug.h"
#include "proc/gfbg_proc.h"
#include "gfbg_disp.h"

int gfbg_log_lv = DBG_WARN;
module_param(gfbg_log_lv, int, 0644);
char gfbg_cfg[128] = "gfbg:vram0_size:8100";
module_param_string(video, gfbg_cfg, 128, 0000);
static int compre_size = 250;     //default 250KB
module_param(compre_size, int, 0444);
static int tde_rot = 0; //default 0, 1:90 degree, 2:270 degree
module_param(tde_rot, int, 0444);

gfbg_layer g_layer[GFBG_MAX_LAYER_NUM];

struct gfbg_vo_dev *gfbg_vdev;

const char *entry_name[GFBG_MAX_LAYER_NUM] = {
	"soph/gfbg0",
	// "soph/gfbg1",
	// "soph/gfbg2",
	// "soph/gfbg3",
	// "soph/gfbg4",
	// "soph/gfbg5",
};

#define PAGE_SIZE_ALIGN_MAX ((~0ul - PAGE_SIZE) / 1024)

const char *const disp_irq_name = {"disp"};

static unsigned long gfbg_get_vram_size(const char *pstr)
{
	int ret = 0;
	int str_is_valid = true;
	unsigned long vram_size = 0;
	unsigned long vram_size_temp;
	const char *ptr = pstr;

	if ((ptr == NULL) || (*ptr == '\0')) {
		return 0;
	}

	while (*ptr != '\0') {
		if (*ptr == ',') {
			break;
		} else if ((!((*ptr) >= '0' && (*ptr) <= '9')) && (*ptr != 'X') && (*ptr != 'x') &&
			   ((*ptr > 'f' && *ptr <= 'z') || (*ptr > 'F' && *ptr <= 'Z'))) {
			str_is_valid = false;
			break;
		}
		ptr++;
	}

	if (str_is_valid) {
		ret = kstrtoul(pstr, 0, &vram_size);
		if (ret) {
			TRACE_GFBG(DBG_ERR, "kstrtoul err!\n");
			return 0;
		}

		if (vram_size > PAGE_SIZE_ALIGN_MAX) {
			TRACE_GFBG(DBG_ERR, "vram_size(%lu)( > %lu) is overflow, it will be set to %u!\n",
				   vram_size, PAGE_SIZE_ALIGN_MAX, 0);
			vram_size = 0;
		}

		vram_size_temp = vram_size;
		/* make the size PAGE_SIZE align */
		vram_size = ((vram_size * 1024 + PAGE_SIZE - 1) & PAGE_MASK) / 1024;
		if (vram_size_temp != vram_size) {
			TRACE_GFBG(DBG_ERR, "vram_size(%lu) if not align in 4, it will be set to %lu!\n",
				   vram_size_temp, vram_size);
		}
	}

	return vram_size;
}

static int parse_cfg_change_layer_id(vo_layer *layer_id, const char *number, unsigned int length)
{
	int ret = 0;

	if (length > 4) {
		TRACE_GFBG(DBG_ERR, "length = %d; out of range!\n", length);
		return -1;
	}

	ret = kstrtoul(number, 0, (unsigned long *)layer_id);
	if (ret) {
		TRACE_GFBG(DBG_ERR, "kstrtoul err!\n");
		return -1;
	}

	if (*layer_id >= VO_MAX_GRAPHIC_LAYER_NUM) {
		TRACE_GFBG(DBG_ERR, "Layer %d is in module_param---video out of range!\n", *layer_id);
		return -1;
	}

	return 0;
}

static void parse_cfg_change_layer_size(const char *sc_str, vo_layer layer_id)
{
	unsigned long layer_size;

	layer_size = gfbg_get_vram_size(sc_str);
	g_layer[layer_id].layer_size = layer_size;
	g_layer[layer_id].compre_info[0].compre_size = compre_size * 1024; // convert to bytes
	g_layer[layer_id].compre_info[1].compre_size = compre_size * 1024; // convert to bytes
	g_layer[layer_id].rot = tde_rot;

	TRACE_GFBG(DBG_INFO, "layer_id = %u; layer_size = %lu, compre_size = %lu;\n",
		   layer_id, layer_size, g_layer[layer_id].compre_info[0].compre_size);
}

static void parse_cfg_start(char **sc_str)
{
	*sc_str = strstr(gfbg_cfg, "vram");
	TRACE_GFBG(DBG_INFO, "cfg:%s\n", gfbg_cfg);
}

static int gfbg_parse_cfg(void)
{
	char *sc_str = NULL;
	char number[4] = {0};
	unsigned int i;
	unsigned int j;
	vo_layer layer_id;
	char ac_param[12] = {0};
	char ac_temp[12] = {0};
	bool is_param_valid = false;

	parse_cfg_start(&sc_str);

	while (sc_str != NULL) {
		i = 0;
		for (j = 0; j < GFBG_MAX_LAYER_NUM; j++) {
			if (snprintf(ac_param, 11, "vram%01u_size", j) < 0) {
				TRACE_GFBG(DBG_ERR, "%s:%d:snprintf failure!\n", __func__, __LINE__);
				return -1;
			}
			if (strncpy(ac_temp, sc_str, 10) != ac_temp) {
				TRACE_GFBG(DBG_ERR, "%s:%d:strncpy failure!\n", __func__, __LINE__);
				return -1;
			}
			if (!strcmp(ac_param, ac_temp)) {
				is_param_valid = true;
			}
		}

		if (!is_param_valid) {
			TRACE_GFBG(DBG_ERR, "insmod parameter is invalid!\n");
			return -1;
		}

		sc_str += 4;
		while (*sc_str != '_') {
			if (i > 1) {
				TRACE_GFBG(DBG_ERR, "layer id is out of range!\n");
				return -1;
			}

			number[i] = *sc_str;
			i++;
			sc_str++;
		}

		number[i] = '\0';

		if (parse_cfg_change_layer_id(&layer_id, number, sizeof(number)) != 0)
			return -1;

		sc_str += sizeof("size") + i;
		parse_cfg_change_layer_size(sc_str, layer_id);
		sc_str = strstr(sc_str, "vram");
	}

	return 0;
}

static int gfbg_register(unsigned int index)
{
	int ret;
	ret = gfbg_overlay_probe(index);
	if (ret == 0) {
		/* create a proc entry in 'gfbg' for the layer */
		gfbg_proc_add_module(entry_name[index], g_layer[index].info);
		gfbg_alloc_cmap(index);
	} else {
		return -1;
	}

	return 0;
}

static void gfbg_unregister(unsigned int index)
{
	int j;
	for (j = index - 1; j >= 0; j--) {
		gfbg_free_cmap(j);
		/* destroy a proc entry in 'gfbg' for the layer */
		gfbg_proc_remove_module(entry_name[j]);
		/* unregister the layer */
		gfbg_overlay_cleanup(j, true);
	}
}

static int gfbg_cb_register(void)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_GFBG;
	reg_cb.dev		= (void *)g_layer;
	reg_cb.cb		= gfbg_cb;

	return base_reg_module_cb(&reg_cb);
}

static int gfbg_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_GFBG);
}

static int gfbg_init_register(void)
{
	unsigned int i;

	gfbg_proc_init();

	/* registe disp interrupt callback function */
	if (gfbg_cb_register() != 0) {
		TRACE_GFBG(DBG_ERR, "ERROR: Load gfbg.ko ....FAILED!\n");
		goto ERR1;
	}

	/* initialize fb file according the config */
	for (i = 0; i < GFBG_MAX_LAYER_NUM; i++) {
		if (gfbg_register(i) != 0) {
			gfbg_unregister(i);
			TRACE_GFBG(DBG_ERR, "ERROR: Load gfbg.ko ....FAILED!\n");
			goto ERR2;
		}
	}

	return 0;
ERR2:
	gfbg_rm_cb();
ERR1:
	gfbg_proc_remove_all_module();
	return -1;
}

void gfbg_cleanup(void)
{
	unsigned int i;

	/* delet disp interrupt callback function */
	gfbg_rm_cb();

	/* remove all entry under dir 'gfbg' */
	gfbg_proc_remove_all_module();

	for (i = 0; i < GFBG_MAX_LAYER_NUM; i++) {
		gfbg_free_cmap(i);
		gfbg_overlay_cleanup(i, true);
	}

	TRACE_GFBG(DBG_INFO, "unload gfbg.ko ....OK!\n");
}

static int gfbg_probe(struct platform_device *pdev)
{
	struct resource *res_disp, *res_oenc;
	void __iomem *virt_addr_disp, *virt_addr_oenc;
	unsigned long size_disp, size_oenc;
	int irq_disp = 0;
	int irq_oenc = 0;
	int ret = 0;

	TRACE_GFBG(DBG_INFO, "load gfbg.ko ....Start!\n");

	/* parse the cfg string */
	if (gfbg_parse_cfg() < 0) {
	    TRACE_GFBG(DBG_ERR, "Usage: insmod gfbg.ko video=\"gfbg: vrami_size:xxx, vramj_size:xxx, ...\"\n");
	    TRACE_GFBG(DBG_ERR, "i, j means layer id, xxx means layer size in kbytes!\n");
	    TRACE_GFBG(DBG_ERR, "ERROR: Load gfbg.ko ....FAILED!\n");
	    return -EINVAL;
	}

	/* initialize fb file according the config */
	if (gfbg_init_register() != 0) {
		return -EINVAL;
	}

	gfbg_vdev = devm_kzalloc(&pdev->dev, sizeof(*gfbg_vdev), GFP_KERNEL);
	if (!gfbg_vdev)
	    return -ENOMEM;

	res_disp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "disp");
	if (!res_disp) {
	    TRACE_GFBG(DBG_ERR, "Failed to get mem resource for disp\n");
	    return -ENODEV;
	}

	size_disp = resource_size(res_disp);
	if (size_disp == 0) {
	    TRACE_GFBG(DBG_ERR, "Invalid resource size for disp\n");
	    return -EINVAL;
	}

	virt_addr_disp = ioremap(res_disp->start, size_disp);
	if (!virt_addr_disp) {
		TRACE_GFBG(DBG_ERR, "ioremap failed!\n");
		return -ENOMEM;
	}

	gfbg_set_disp_base_addr(0, (void *)virt_addr_disp);

	gfbg_vdev->dev_id = 0;
	gfbg_vdev->layer_id = 0;
	irq_disp = platform_get_irq_byname(pdev, "disp");
	if (irq_disp < 0) {
		TRACE_GFBG(DBG_ERR, "Failed to vo request irq\n");
		return irq_disp;
	}
	gfbg_vdev->irq = irq_disp;

	atomic_set(&gfbg_vdev->irq_requested, 0);

	res_oenc = platform_get_resource_byname(pdev, IORESOURCE_MEM, "oenc");
	if (!res_oenc) {
	    TRACE_GFBG(DBG_ERR, "Failed to get mem resource for oenc\n");
	    return -ENODEV;
	}

	size_oenc = resource_size(res_oenc);
	if (size_oenc == 0) {
	    TRACE_GFBG(DBG_ERR, "Invalid resource size for oenc\n");
	    return -EINVAL;
	}

	virt_addr_oenc = ioremap(res_oenc->start, size_oenc);
	if (!virt_addr_oenc) {
		TRACE_GFBG(DBG_ERR, "ioremap failed!\n");
		return -ENOMEM;
	}

	gfbg_set_oenc_base_addr(0, (void *)virt_addr_oenc);

	irq_oenc = platform_get_irq_byname(pdev, "oenc");
	if (irq_oenc < 0) {
		TRACE_GFBG(DBG_ERR, "Failed to vo request irq\n");
		return irq_oenc;
	}
	gfbg_vdev->irq_oenc = irq_oenc;
	ret = osal_irq_request(gfbg_vdev->irq_oenc, gfbg_oenc_irq_handler, 0,
		"oenc", (void *)gfbg_vdev);

	TRACE_GFBG(DBG_INFO, "load gfbg.ko ....OK!\n");

	return ret;
}

static int gfbg_remove(struct platform_device *pdev)
{
	gfbg_cleanup();
	gfbg_set_disp_base_addr(0, NULL);
	gfbg_set_oenc_base_addr(0, NULL);
	osal_irq_free(gfbg_vdev->irq_oenc, (void *)gfbg_vdev);
	return 0;
}

static const struct of_device_id gfbg_match[] = {
	{ .compatible = "cvitek,fb" },
	{},
};
MODULE_DEVICE_TABLE(of, gfbg_match);

static struct platform_driver gfbg_driver = {
	.probe = gfbg_probe,
	.remove = gfbg_remove,
	.driver = {
		.name   = "gfbg",
		.owner	= THIS_MODULE,
		.of_match_table = gfbg_match,
	},
};

module_platform_driver(gfbg_driver);
MODULE_LICENSE("GPL");

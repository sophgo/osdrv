#include "drv/cvi_irq.h"
#include "cif_comm.h"
#include "driver_cif.h"
#include "cif.h"
#include "cif_ioctl.h"
#include "cif_cb.h"
#include <aos/cli.h>


static unsigned int irq_num[MAX_LINK_NUM] = {
	VIVO_INT_CSI_MAC0,
	VIVO_INT_CSI_MAC1,
	VIVO_INT_CSI_MAC2,
};

static char irq_name[MAX_LINK_NUM][20] = {
	"cif-irq0",
	"cif-irq1",
	"cif-irq2",
};

struct cif_dev *g_cif_pdev;

long driver_cif_ioctl(unsigned int cmd, unsigned long arg)
{
	return cif_ioctl(cmd, arg);
}

static int cif_core_cb(void *dev, enum _cb_modules_id caller, u32 cmd, void *arg)
{
	struct cif_dev *vdev = (struct cif_dev *)dev;
	int rc = -1;

	switch (cmd) {
	case CIF_CB_GET_CIF_ATTR:
	{
		struct cif_attr_s *cif_attr = (struct cif_attr_s *)arg;
		struct cif_param *param = &vdev->link[cif_attr->devno].param;
		struct combo_dev_attr_s *rx_attr = &vdev->link[cif_attr->devno].attr;

		if (param->type == CIF_TYPE_CSI && param->hdr_en) {
			struct param_csi *csi = &param->cfg.csi;
			cif_attr->stagger_vsync = (csi->hdr_mode == CSI_HDR_MODE_VC);
		} else {
			cif_attr->stagger_vsync = 0;
		}

		memcpy(&cif_attr->img_size, &rx_attr->img_size, sizeof(struct img_size_s));

		rc = 0;
		break;
	}
	case CIF_CB_RESET_LVDS:
	{
		//printf("[ WARN!! ]now do not support sublvds reset\n");
		break;
	}
	default:
		break;
	}

	return rc;
}

int cif_core_register_cb(struct cif_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_CIF;
	reg_cb.dev			= (void *)dev;
	reg_cb.cb			= cif_core_cb;

	return base_reg_module_cb(&reg_cb);
}

int cif_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_CIF);
}

static void cif_isr(int irq, void *_link)
{
	struct link *link = (struct link *)_link;
	struct cif_ctx *ctx = &link->cif_ctx;

	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_ECC_ERR_MASK))
		link->sts_csi.errcnt_ecc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_CRC_ERR_MASK))
		link->sts_csi.errcnt_crc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_WC_ERR_MASK))
		link->sts_csi.errcnt_wc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_HDR_ERR_MASK))
		link->sts_csi.errcnt_hdr++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_FIFO_FULL_MASK))
		link->sts_csi.fifo_full++;

	if (link->sts_csi.errcnt_ecc > 0xFFFF ||
		link->sts_csi.errcnt_crc > 0xFFFF ||
		link->sts_csi.errcnt_hdr > 0xFFFF ||
		link->sts_csi.fifo_full > 0xFFFF ||
		link->sts_csi.errcnt_wc > 0xFFFF) {

		cif_mask_csi_int_sts(ctx, 0x1F);
		TRACE_CIF(DBG_ERR, "mask the interrupt since err cnt is full\n");
		TRACE_CIF(DBG_ERR, "ecc = %u, crc = %u, wc = %u, hdr = %u, fifo_full = %u\n",
				link->sts_csi.errcnt_ecc,
				link->sts_csi.errcnt_crc,
				link->sts_csi.errcnt_wc,
				link->sts_csi.errcnt_hdr,
				link->sts_csi.fifo_full);
	}

	cif_clear_csi_int_sts(ctx);
}

#ifdef CONFIG_PROC_FS
void proc_mipi_rx(int32_t argc, char **argv)
{
	proc_cif_show();
}
ALIOS_CLI_CMD_REGISTER(proc_mipi_rx, proc_mipi_rx, proc mipi rx);
#endif

int driver_cif_init(void)
{
	int i, ret;
	struct link *link;
	/*alloc buf*/
	g_cif_pdev = osal_calloc(1,sizeof(struct cif_dev));
	if (!g_cif_pdev) {
		printf("[%s]alloc buf for cif fail.\n", __func__);
		return -1;
	}

	for (i = 0; i < MAX_LINK_NUM; i++) {
		struct cif_ctx *ctx = &g_cif_pdev->link[i].cif_ctx;
		ctx->mac_phys_regs = cif_get_mac_phys_reg_bases(i);
		ctx->wrap_phys_regs = cif_get_wrap_phys_reg_bases(i);
	}

	cif_set_base_addr(0, (uint32_t *)SENSOR_MAC0_BASE, (uint32_t *)DPHY_TOP_BASE);
	cif_set_base_addr(1, (uint32_t *)SENSOR_MAC1_BASE, (uint32_t *)DPHY_TOP_BASE);

	g_cif_pdev->max_mac_clk = 900;

	for (i = 0; i < MAX_LINK_NUM; ++i) {
		link = &g_cif_pdev->link[i];

		/*irq*/
		link->irq_num = irq_num[i];
		ret = request_irq(link->irq_num, cif_isr, 0, irq_name[i], (void *)link);
		if (ret < 0) {
			printf("request irq-%d fail\n", link->irq_num);
			return -1;
		}

		/* set the port id */
		link->cif_ctx.mac_num = i;
	}

	if (cif_core_register_cb(g_cif_pdev)) {
		printf("Failed to register cif cb, err!\n");
		return -1;
	}

	return 0;
}

int driver_cif_exit(void)
{
	int i;

	for (i = 0; i < MAX_LINK_NUM; ++i) {
		csi_irq_disable(irq_num[i]);
	}
	if (cif_core_rm_cb()) {
		printf("Failed to rm cif cb\n");
	}
	osal_free(g_cif_pdev);
	return 0;
}

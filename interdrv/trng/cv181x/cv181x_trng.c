#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#define DEVICE_NAME    "trng"

static int trng_major;
static struct class *trng_class;
static struct clk *efuse_clk;
static void __iomem *reg_base;

static void trng_init(void)
{
	uint32_t data;

	// wait idle
	do {
		data = ioread32(reg_base + 0xc);
	} while ((data & 0x80000000) == 0x80000000);

	// start seed generation
	iowrite32(0x1, reg_base);

	do {
		data = ioread32(reg_base + 0x14);
	} while (!(data & 0x10));

	// ack and clear the done bit flag
	iowrite32(0x10, reg_base + 0x14);

	// create state;
	iowrite32(0x3, reg_base);

	do {
		data = ioread32(reg_base + 0x14);
	} while (!(data & 0x10));

	// ack and clear the done bit flag
	iowrite32(0x10, reg_base + 0x14);
}

static int trng_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	static int bool_init;

	ret = clk_prepare_enable(efuse_clk);
	if (ret) {
		pr_err("%s: clock failed to prepare+enable: %lld\n", __func__,
			   (long long)ret);
		return ret;
	}

	if (bool_init == 0) {
		trng_init();
		bool_init = 1;
	}

	return 0;
}

static int trng_release(struct inode *inode, struct file *file)
{
	clk_disable_unprepare(efuse_clk);
	return 0;
}

static ssize_t trng_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int i, ret;
	uint32_t status;
	uint32_t index = 0;
	int availed = count;

	while (availed > 0) {
		// generation random number
		iowrite32(0x6, reg_base);

		do {
			status = ioread32(reg_base + 0x14);
		} while (!(status & 0x10));

		// ack and clear the done bit flag
		iowrite32(0x10, reg_base + 0x14);

		for (i = 0; i < 4; i++) {
			status = ioread32(reg_base + 0x24 + (4*i));

			if (availed < 4) {
				ret = copy_to_user(buf + index, &status, availed);
				if (ret != 0) {
					return -1;
				}

				return count;
			}

			ret = copy_to_user(buf + index, &status, 4);
			if (ret != 0) {
				return -1;
			}

			index += 4;
			availed -= 4;

			if (availed == 0)
				return count;
		}
	}

	return count;
}

const struct file_operations trng_fops = {
	.owner		=	THIS_MODULE,
	.open		=	trng_open,
	.release	=	trng_release,
	.read		=	trng_read,
};

static int trng_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res = NULL;
	//void __iomem *reg_base = NULL;

	// Get trng base address
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}

	efuse_clk = clk_get_sys(NULL, "clk_efuse");
	if (IS_ERR(efuse_clk)) {
		pr_err("%s: efuse clock not found %ld\n", __func__,
			   PTR_ERR(efuse_clk));
		return PTR_ERR(efuse_clk);
	}

	trng_major = register_chrdev(0, DEVICE_NAME, &trng_fops);
	if (trng_major < 0) {
		pr_err(DEVICE_NAME " can't register major number\n");
		return -1;
	}

	trng_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(trng_class)) {
		pr_err("Err: failed in trng class.\n");
		return -1;
	}

	device_create(trng_class, NULL, MKDEV(trng_major, 0), NULL, DEVICE_NAME);

	return 0;
}

static int trng_remove(struct platform_device *pdev)
{
	device_destroy(trng_class, MKDEV(trng_major, 0));
	class_destroy(trng_class);
	unregister_chrdev(trng_major, DEVICE_NAME);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dw_trng_of_match[] = {
	{ .compatible = "snps,dw-trng", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dw_trng_of_match);
#endif

static struct platform_driver trng_platform_driver = {
	.probe = trng_probe,
	.remove = trng_remove,
	.driver         = {
		.name   = "trng",
		.of_match_table = of_match_ptr(dw_trng_of_match),
		.owner  = THIS_MODULE,
	},
};

module_platform_driver(trng_platform_driver);

MODULE_AUTHOR("cvitek");
MODULE_DESCRIPTION("cvitek trng driver");
MODULE_LICENSE("GPL");

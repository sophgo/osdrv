// SPDX-License-Identifier: GPL-3.0-or-later

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/version.h>
#include <linux/compat.h>

#include <linux/init.h>
#include <linux/dma-buf.h>

#include <linux/arm-smccc.h>
#include <linux/uaccess.h>

#include <linux/dma-direction.h>
#include <linux/dma-map-ops.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "sophon_spacc.h"
#include <linux/cdev.h>  

#define DEVICE_NAME "spacc"


struct cvi_spacc {
	struct device *dev;
	struct cdev cdev;
	dev_t tdev;
	struct class *spacc_class;
#ifdef CONFIG_PM_SLEEP
	struct clk *efuse_clk;
#endif
};
struct cvi_spacc_private {
	struct mutex lock;
	void *buffer;
	u32 buffer_size;
	u32 used_size;
	u32 read_size;
	u32 state[8];
	u32 dma_handle;
	u32 data_size;
};
static void cvi_spacc_free_pool(struct cvi_spacc_private *pool)
{
    if (pool->buffer && pool->buffer_size) {
        pr_err("free pool: %p, size: %u\n", pool->buffer, pool->buffer_size);
        free_pages((unsigned long)pool->buffer, get_order(pool->buffer_size));
    }
    pool->buffer = NULL;
    pool->buffer_size = 0;
    pool->dma_handle = 0;
    pool->data_size = 0;
    pool->read_size = 0;
}

static int cvi_spacc_create_pool(struct cvi_spacc_private *pool, unsigned int size)
{
	// Free existing memory pool if any
	 cvi_spacc_free_pool(pool);

	size = PAGE_ALIGN(size);
	unsigned int order = get_order(size);

	pr_info(
		 "Attempting to allocate %u bytes (order %u)\n", size, order);

	// First try DMA zone allocation
	pool->buffer =
		(void *)__get_free_pages(GFP_DMA | __GFP_ZERO, order);
	if (pool->buffer) {
		pool->buffer_size = size;
		pool->dma_handle = virt_to_phys(pool->buffer);
		pr_info(
			 "DMA pages allocation succeeded at phys 0x%llx\n",
			 (unsigned long long)pool->dma_handle);
		return 0;
	}

	// If failed, wait and try DMA32
	msleep(100);
	pr_info( "DMA allocation failed, trying DMA32\n");

	pool->buffer =
		(void *)__get_free_pages(GFP_DMA32 | __GFP_ZERO, order);
	if (pool->buffer) {
		pool->buffer_size = size;
		pool->dma_handle = virt_to_phys(pool->buffer);
		pr_info(
			 "DMA32 pages allocation succeeded at phys 0x%llx\n",
			 (unsigned long long)pool->dma_handle);
		return 0;
	}

	// Finally try with retry mechanism
	int retry_count = 3;
	while (retry_count--) {
		
		pool->buffer =
			(void *)__get_free_pages(GFP_DMA | __GFP_ZERO |
							 __GFP_DIRECT_RECLAIM |
							 __GFP_RETRY_MAYFAIL,
						 order);

		if (pool->buffer) {
			pool->buffer_size = size;
			pool->dma_handle = virt_to_phys(pool->buffer);
			pr_info(
				 "Retry allocation succeeded at phys 0x%llx\n",
				 (unsigned long long)pool->dma_handle);
			return 0;
		}

		if (retry_count > 0) {
			pr_info(
				 "Allocation failed, retrying after delay\n");
			msleep(200);
		}
	}

	pr_err(
		"All allocation attempts failed for %u bytes\n"
		"Process: %s (PID: %d)\n"
		"Order required: %u\n",
		size, current->comm, current->pid, order);

	return -ENOMEM;
}

static int cvi_spacc_base64(u32 customer_code, u32 action, struct cvi_spacc_private *spacc_private)
{
	struct arm_smccc_res res = { 0 };
	phys_addr_t value_phys;

	value_phys = virt_to_phys(spacc_private->buffer);

	arch_sync_dma_for_device(value_phys, spacc_private->data_size, DMA_TO_DEVICE);

	arm_smccc_smc(OPTEE_SMC_CALL_CV_BASE64, (unsigned long)value_phys,
		      spacc_private->data_size, (unsigned long)value_phys,
		      customer_code, action, 0, 0, &res);
	pr_debug("res a0 : %lu\n", res.a0);

	return res.a0;
}

static int cvi_spacc_base64_inner(struct cvi_spacc_base64_inner *b64)
{
	struct arm_smccc_res res = { 0 };
	arch_sync_dma_for_device(b64->src, b64->len, DMA_TO_DEVICE);
	arm_smccc_smc(OPTEE_SMC_CALL_CV_BASE64, b64->src, b64->len, b64->dst,
		      b64->customer_code, b64->action, 0, 0, &res);

	arch_sync_dma_for_device(b64->dst, res.a0, DMA_FROM_DEVICE);
	pr_debug("res a0 : %lu\n", res.a0);

	return res.a0;
}


static int spacc_sha256(phys_addr_t buffer, unsigned int size)
{
	struct arm_smccc_res res = { 0 };

	arch_sync_dma_for_device(buffer, size, DMA_TO_DEVICE);
	arm_smccc_smc(OPTEE_SMC_CALL_CV_SHA256, (unsigned long)buffer, size,
		      (unsigned long)buffer, 0, 0, 0, 0, &res);
	printk("res a0 : %lu\n", res.a0);

	return res.a0;
}
static int spacc_sm3(phys_addr_t buffer, unsigned int size)
{
	struct arm_smccc_res res = { 0 };

	arch_sync_dma_for_device(buffer, size, DMA_TO_DEVICE);
	arm_smccc_smc(OPTEE_SMC_CALL_CV_SM3, (unsigned long)buffer, size,
		      (unsigned long)buffer, 0, 0, 0, 0, &res);
	printk("res a0 : %lu\n", res.a0);

	return res.a0;
}

static int spacc_aes(phys_addr_t src_phys, uint32_t len, phys_addr_t key_phys,
		     uint32_t key_len, phys_addr_t iv_phys,
		     spacc_aes_config_s *config)
{
	struct arm_smccc_res res = { 0 };
	uint64_t arg = 0;

	arch_sync_dma_for_device(src_phys, len, DMA_TO_DEVICE);
	arg = (u8)config->mode | ((u8)config->key_mode << 2) |
	      ((u8)config->action << 4) | ((u8)config->otp << 5);
	arm_smccc_smc(OPTEE_SMC_CALL_CV_AES, (unsigned long)src_phys,
		      (unsigned long)src_phys, len, key_phys, iv_phys, key_len,
		      (unsigned long)arg, &res);

	printk("res a0 : %lu\n", res.a0);

	return res.a0;
}
static int spacc_sm4(phys_addr_t src_phys, uint32_t len, phys_addr_t key_phys,
		     uint32_t key_len, phys_addr_t iv_phys,
		     spacc_aes_config_s *config)
{
	struct arm_smccc_res res = { 0 };
	uint64_t arg = 0;

	arch_sync_dma_for_device(src_phys, len, DMA_TO_DEVICE);
	arg = (u8)config->mode | ((u8)config->key_mode << 2) |
	      ((u8)config->action << 4) | ((u8)config->otp << 5);
	arm_smccc_smc(OPTEE_SMC_CALL_CV_SM4, (unsigned long)src_phys,
		      (unsigned long)src_phys, len, key_phys, iv_phys, key_len,
		      (unsigned long)arg, &res);

	printk("res a0 : %lu\n", res.a0);

	return res.a0;
}

static int spacc_des(phys_addr_t src_phys, uint32_t len, phys_addr_t key_phys,
		     uint32_t key_len, phys_addr_t iv_phys,
		     spacc_des_config_s *config, int tdes)
{
	struct arm_smccc_res res = { 0 };

	arch_sync_dma_for_device(src_phys, len, DMA_TO_DEVICE);
	arm_smccc_smc(tdes ? OPTEE_SMC_CALL_CV_TDES : OPTEE_SMC_CALL_CV_DES,
		      (unsigned long)src_phys, len, (unsigned long)src_phys,
		      key_phys, iv_phys, config->mode, config->action, &res);
	printk("res a0 : %lu\n", res.a0);
	return res.a0;
}

static int spacc_open(struct inode *inode, struct file *file)
{
	struct cvi_spacc_private *spacc_private;

	spacc_private = kzalloc(sizeof(*spacc_private), GFP_KERNEL);
	if (!spacc_private) {
		pr_err("kzalloc for spacc_private failed\n");
		return -ENOMEM;
	}
	mutex_init(&spacc_private->lock);
	spacc_private->used_size = 0;
	spacc_private->read_size = 0;
	spacc_private->buffer = NULL;
	spacc_private->buffer_size = 0;
	spacc_private->dma_handle = 0;
	spacc_private->data_size = 0;

	file->private_data = spacc_private;
	return 0;
}

static ssize_t spacc_read(struct file *filp, char *buf, size_t count,
			  loff_t *f_pos)
{
	struct cvi_spacc_private *spacc_private = filp->private_data;
	int ret;

	mutex_lock(&spacc_private->lock);

	if (!spacc_private->used_size) {
		mutex_unlock(&spacc_private->lock);
		return 0;
	}

	ret = spacc_private->used_size - spacc_private->read_size;
	count = (ret >= count) ? count : ret;
	ret = copy_to_user(
		buf, (char *)spacc_private->buffer + spacc_private->read_size,
		count);
	if (ret != 0) {
		mutex_unlock(&spacc_private->lock);
		return -1;
	}

	spacc_private->read_size += count;
	if (spacc_private->used_size == spacc_private->read_size) {
		spacc_private->used_size = 0;
		spacc_private->read_size = 0;
	}
	
	mutex_unlock(&spacc_private->lock);
	return count;
}

static ssize_t spacc_write(struct file *filp, const char *buf, size_t count,
			   loff_t *f_pos)
{
	struct cvi_spacc_private *spacc_private = filp->private_data;
	int ret;

	mutex_lock(&spacc_private->lock);

	ret = spacc_private->buffer_size - spacc_private->used_size;
	if (ret <= 0) {
		ret = spacc_private->used_size;
		mutex_unlock(&spacc_private->lock);
		return ret;
	}

	if (count > ret)
		count = ret;

	ret = copy_from_user(((unsigned char *)spacc_private->buffer +
			      spacc_private->used_size),
			     buf, count);
	if (ret != 0) {
		mutex_unlock(&spacc_private->lock);
		return -1;
	}

	spacc_private->used_size += count;
	
	mutex_unlock(&spacc_private->lock);
	return spacc_private->used_size;
}

static int spacc_release(struct inode *inode, struct file *file)
{
	struct cvi_spacc_private *spacc_private;
    
	if (!file || !file->private_data)
		return -EINVAL;
        
	spacc_private = file->private_data;
	mutex_destroy(&spacc_private->lock);
	cvi_spacc_free_pool(spacc_private);
	kfree(spacc_private);
	file->private_data = NULL;

	return 0;
}
static int handle_key_iv(spacc_aes_config_s *config, void **key_kernel_addr,
			 void **iv_kernel_addr, uint64_t *key_len)
{
	int ret = 0;
	
	*key_kernel_addr = NULL;
	*iv_kernel_addr = NULL;

	switch (config->key_mode) {
	case SPACC_KEY_SIZE_64BITS:
		*key_len = 8;
		break;
	case SPACC_KEY_SIZE_128BITS:
		*key_len = 16;
		break;
	case SPACC_KEY_SIZE_192BITS:
		*key_len = 24;
		break;
	case SPACC_KEY_SIZE_256BITS:
		*key_len = 32;
		break;
	default:
		return -EINVAL;
	}

	if (config->otp != SPACC_KEY_SOURCE_OTP) {
		*key_kernel_addr = kmalloc(*key_len, GFP_KERNEL);
		if (!*key_kernel_addr) {
			pr_err("kmalloc for key failed\n");
			ret = -ENOMEM;
			goto cleanup;
		}
		
		if (copy_from_user(*key_kernel_addr, config->key, *key_len) != 0) {
			pr_err("copy_from_user key failed\n");
			ret = -EFAULT;
			goto cleanup;
		}
	}

	if (config->mode != SPACC_ALGO_MODE_ECB) {
		*iv_kernel_addr = kmalloc(16, GFP_KERNEL);
		if (!*iv_kernel_addr) {
			pr_err("kmalloc for IV failed\n");
			ret = -ENOMEM;
			goto cleanup;
		}
		
		if (copy_from_user(*iv_kernel_addr, config->iv, 16) != 0) {
			pr_err("copy_from_user iv failed\n");
			ret = -EFAULT;
			goto cleanup;
		}
	}

	return 0;

cleanup:
	if (*key_kernel_addr) {
		kfree(*key_kernel_addr);
		*key_kernel_addr = NULL;
	}
	if (*iv_kernel_addr) {
		kfree(*iv_kernel_addr);
		*iv_kernel_addr = NULL;
	}
	return ret;
}
static int handle_src_phys(struct cvi_spacc_private *spacc_private,
			   spacc_aes_config_s *config, phys_addr_t *src_phys,
			   uint32_t *len)
{
	if (!spacc_private->buffer || spacc_private->used_size == 0) {
		printk(KERN_ERR "Memory pool is empty or uninitialized\n");
		return -EINVAL;
	}
	*src_phys = virt_to_phys(spacc_private->buffer);
	*len = spacc_private->used_size;
	return 0;
}
static long spacc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_spacc_private *spacc_private = filp->private_data;
	struct cvi_spacc *spacc = container_of(filp->f_inode->i_cdev, struct cvi_spacc, cdev);
	struct device *dev = spacc->dev;
	int ret = 0;
	void *key_kernel_addr = NULL;
	void *iv_kernel_addr = NULL;
	if(!spacc_private) {
		pr_err("spacc_private not initialized\n");
		return -ENODEV;
	}

	mutex_lock(&spacc_private->lock);

	switch (cmd) {
	case IOCTL_SPACC_CREATE_MEMPOOL: {
		unsigned int size = 0;
		
		ret = copy_from_user((unsigned char *)&size,
				     (unsigned char *)arg, sizeof(size));
		if (ret != 0){
			printk("copy_from_user size failed, ret: %d\n", ret);
			break;
		}
		ret = cvi_spacc_create_pool(spacc_private, size);
		// ret = cvi_spacc_init_buffer(spacc_private, size);
		if (ret != 0){
			printk("cvi_spacc_create_pool failed, ret: %d\n", ret);
			break;
		}
		break;
	}
	case IOCTL_SPACC_GET_MEMPOOL_SIZE: {
		ret = copy_to_user((unsigned char *)arg,
				   (unsigned char *)&spacc_private->buffer_size,
				   sizeof(spacc_private->buffer_size));
		if (ret != 0){
			printk("copy_to_user buffer_size failed, ret: %d\n", ret);
			break;
		}

		break;
	}
	case IOCTL_SPACC_BASE64: {
		struct cvi_spacc_base64 b64 = { 0 };
		u32 padding_size = 0;

		if (!arg) {
			pr_err("Invalid argument\n");
			break;
		}

		ret = copy_from_user((unsigned char *)&b64,
				     (unsigned char *)arg, sizeof(b64));
		if (ret != 0) {
			pr_err("Failed to copy base64 config from user\n");
			break;
		}

		if (!b64.action) {
			char *buf = spacc_private->buffer;
			if (!buf) {
				pr_err("Pool not allocated\n");
				break;
			}

			if (buf[spacc_private->data_size - 1] == '=')
				padding_size++;
			if (buf[spacc_private->data_size - 2] == '=')
				padding_size++;
		}

		ret = cvi_spacc_base64(b64.customer_code, b64.action, spacc_private);
		if (ret < 0) {
			dev_err(dev, "plat_cryptodma_do failed\n");
			break;
		}

		if (!b64.action)
			ret -= padding_size;

		spacc_private->data_size = ret;
		spacc_private->read_size = 0;
		break;
	}
	case IOCTL_SPACC_BASE64_INNER: {
		struct cvi_spacc_base64_inner b64;

		if (!arg) {
			pr_err("Invalid argument\n");
			break;
		}

		ret = copy_from_user((unsigned char *)&b64,
				     (unsigned char *)arg, sizeof(b64));
		if (ret != 0) {
			pr_err("Failed to copy base64 inner config from user\n");
			break;
		}

		ret = cvi_spacc_base64_inner(&b64);
		break;
	}
	case IOCTL_SPACC_AES_ACTION: {
		spacc_aes_config_s config = { 0 };
		phys_addr_t src_phys;
		uint32_t len;
		uint64_t key_len = 0;
		ret = copy_from_user((unsigned char *)&config,
				     (unsigned char *)arg, sizeof(config));
		if (ret != 0) {
			dev_err(dev, "copy_from_user config failed, ret: %d\n", ret);
			break;
		}
		ret = handle_src_phys(spacc_private, &config, &src_phys, &len);
		if (ret != 0) {
			dev_err(dev, "handle_src_phys failed, ret: %d\n", ret);
			break;
		}
		ret = handle_key_iv(&config, &key_kernel_addr, &iv_kernel_addr, &key_len);
		if (ret != 0) {
			dev_err(dev, "handle_key_iv failed, ret: %d\n", ret);
			break;
		}
		if (key_kernel_addr)
			arch_sync_dma_for_device(virt_to_phys(key_kernel_addr), key_len, DMA_TO_DEVICE);
		if (iv_kernel_addr)
			arch_sync_dma_for_device(virt_to_phys(iv_kernel_addr), 16, DMA_TO_DEVICE);
		ret = spacc_aes(src_phys, len,
				key_kernel_addr ? virt_to_phys(key_kernel_addr) : 0,
				key_len,
				iv_kernel_addr ? virt_to_phys(iv_kernel_addr) : 0,
				&config);
		if (ret < 0) {
			dev_err(dev, "spacc_aes failed, ret: %d\n", ret);
			break;
		}
		if (ret > 0) {
			arch_sync_dma_for_device(src_phys, ret, DMA_FROM_DEVICE);
			spacc_private->used_size = ret;
			spacc_private->read_size = 0;
		}

		break;
	}
	case IOCTL_SPACC_SM4_ACTION: {
		spacc_sm4_config_s config = { 0 };
		phys_addr_t src_phys;
		uint32_t len;
		uint64_t key_len = 0;
		ret = copy_from_user((unsigned char *)&config,
				     (unsigned char *)arg, sizeof(config));
		if (ret != 0) {
			dev_err(dev, "copy_from_user config failed, ret: %d\n", ret);
			break;
		}

		ret = handle_src_phys(spacc_private, &config, &src_phys, &len);
		if (ret != 0) {
			dev_err(dev, "copy_from_user config failed, ret: %d\n", ret);
			break;
		}

		ret = handle_key_iv((spacc_aes_config_s *)&config,
				    &key_kernel_addr, &iv_kernel_addr,
				    &key_len);
		if (ret != 0) {
			dev_err(dev, "handle_key_iv failed, ret: %d\n", ret);
			break;
		}

		if(key_kernel_addr)
			arch_sync_dma_for_device(virt_to_phys(key_kernel_addr), key_len, DMA_TO_DEVICE);
		if(iv_kernel_addr)
			arch_sync_dma_for_device(virt_to_phys(iv_kernel_addr), 16, DMA_TO_DEVICE);

		ret = spacc_sm4(src_phys, len,
                       key_kernel_addr ? virt_to_phys(key_kernel_addr) : 0,
                       key_len,
                       iv_kernel_addr ? virt_to_phys(iv_kernel_addr) : 0,
                       &config);
		if (ret < 0) {
			dev_err(dev, "spacc_sm4 failed\n");
			break;
		}

		arch_sync_dma_for_device(src_phys, spacc_private->used_size,
					 DMA_FROM_DEVICE);
		spacc_private->used_size = ret;
		spacc_private->read_size = 0;

		break;
	}
	case IOCTL_SPACC_DES_ACTION: {
		spacc_des_config_s config = { 0 };
		phys_addr_t src_phys;
		int key_len = 16;

		if (spacc_private->used_size == 0) {
			dev_err(dev, "spacc_dev->used_size : %d\n", spacc_private->used_size);
			ret = -EINVAL;
			break;
		}

		ret = copy_from_user((unsigned char *)&config,
					 (unsigned char *)arg, sizeof(config));
		if (ret != 0) {
			dev_err(dev, "copy_from_user config failed, ret: %d\n", ret);
			break;
		}

		src_phys = virt_to_phys(spacc_private->buffer);
		
		key_kernel_addr = kmalloc(key_len, GFP_KERNEL);
		if (!key_kernel_addr) {
			dev_err(dev, "kmalloc for key failed\n");
			ret = -ENOMEM;
			break;
		}
		
		if (copy_from_user(key_kernel_addr, config.key, key_len) != 0) {
			dev_err(dev, "copy_from_user key failed\n");
			ret = -EFAULT;
			break;
		}

		if (config.mode != SPACC_ALGO_MODE_ECB) {
			iv_kernel_addr = kmalloc(16, GFP_KERNEL);
			if (!iv_kernel_addr) {
				dev_err(dev, "kmalloc for IV failed\n");
				ret = -ENOMEM;
				break;
			}
			
			if (copy_from_user(iv_kernel_addr, config.iv, 16) != 0) {
				dev_err(dev, "copy_from_user iv failed\n");
				ret = -EFAULT;
				break;
			}
		}

		ret = spacc_des(src_phys, spacc_private->used_size,
					   virt_to_phys(key_kernel_addr), key_len,
					   iv_kernel_addr ? virt_to_phys(iv_kernel_addr) : 0,
					   &config, 0);
		if (ret < 0) {
			dev_err(dev, "spacc_des failed\n");
			break;
		}

		arch_sync_dma_for_device(src_phys, spacc_private->used_size,
					 DMA_FROM_DEVICE);
		spacc_private->used_size = ret;
		spacc_private->read_size = 0;

		break;
	}
	case IOCTL_SPACC_TDES_ACTION: {
		spacc_tdes_config_s config = { 0 };
		phys_addr_t src_phys;
		int key_len = 24;

		if (spacc_private->used_size == 0) {
			dev_err(dev, "spacc_dev->used_size : %d\n", spacc_private->used_size);
			ret = -EINVAL;
			break;
		}

		ret = copy_from_user((unsigned char *)&config,
					 (unsigned char *)arg, sizeof(config));
		if (ret != 0) {
			dev_err(dev, "copy_from_user config failed, ret: %d\n", ret);
			break;
		}

		src_phys = virt_to_phys(spacc_private->buffer);

		key_kernel_addr = kmalloc(key_len, GFP_KERNEL);
		if (!key_kernel_addr) {
			dev_err(dev, "kmalloc for key failed\n");
			ret = -ENOMEM;
			break;
		}

		if (copy_from_user(key_kernel_addr, config.key, key_len) != 0) {
			dev_err(dev, "copy_from_user key failed\n");
			ret = -EFAULT;
			break;
		}

		if (config.mode != SPACC_ALGO_MODE_ECB) {
			iv_kernel_addr = kmalloc(16, GFP_KERNEL);
			if (!iv_kernel_addr) {
				dev_err(dev, "kmalloc for IV failed\n");
				ret = -ENOMEM;
				break;
			}

			if (copy_from_user(iv_kernel_addr, config.iv, 16) != 0) {
				dev_err(dev, "copy_from_user iv failed\n");
				ret = -EFAULT;
				break;
			}
			if(iv_kernel_addr)
				arch_sync_dma_for_device(virt_to_phys(iv_kernel_addr), 16, DMA_TO_DEVICE);
		}
		if(key_kernel_addr)
		ret = spacc_des(src_phys, spacc_private->used_size,
					   virt_to_phys(key_kernel_addr), key_len,
					   iv_kernel_addr ? virt_to_phys(iv_kernel_addr) : 0,
					   &config, 1);
		if (ret < 0) {
			dev_err(dev, "spacc_tdes failed\n");
			break;
		}

		arch_sync_dma_for_device(src_phys, spacc_private->used_size,
					 DMA_FROM_DEVICE);
		spacc_private->used_size = ret;
		spacc_private->read_size = 0;

		break;
	}
	case IOCTL_SPACC_SHA256_ACTION: {
		if (spacc_private->used_size == 0) {
			printk("used_size : %d\n", spacc_private->used_size);
			ret = -EINVAL;
			break;
		}
		phys_addr_t src_phys = virt_to_phys(spacc_private->buffer);
		ret = spacc_sha256(src_phys, spacc_private->used_size);
		if (ret < 0) {
			dev_err(dev, "plat_cryptodma_do failed\n");
			break;
		}
		spacc_private->used_size = 32;
		spacc_private->read_size = 0;
		break;
	}
	case IOCTL_SPACC_SM3_ACTION: {
		if (spacc_private->used_size < 0) {
			dev_err(dev, "used_size : %d\n", spacc_private->used_size);
			ret = -EINVAL;
			break;
		}
		phys_addr_t src_phys = virt_to_phys(spacc_private->buffer);
		ret = spacc_sm3(src_phys, spacc_private->used_size);
		if (ret < 0) {
			dev_err(dev, "plat_cryptodma_do failed\n");
			break;
		}
		spacc_private->used_size = ret;
		spacc_private->read_size = 0;
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}
	
	if (key_kernel_addr) {
		kfree(key_kernel_addr);
		key_kernel_addr = NULL;
	}
	if (iv_kernel_addr) {
		kfree(iv_kernel_addr);
		iv_kernel_addr = NULL;
	}
	mutex_unlock(&spacc_private->lock);
	return ret;
}

#ifdef CONFIG_COMPAT
static long space_compat_ptr_ioctl(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd,
					  (unsigned long)compat_ptr(arg));
}
#endif

const struct file_operations spacc_fops = {
	.owner = THIS_MODULE,
	.open = spacc_open,
	.read = spacc_read,
	.write = spacc_write,
	.release = spacc_release,
	.unlocked_ioctl = spacc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = space_compat_ptr_ioctl,
#endif
};

static int cvitek_spacc_drv_probe(struct platform_device *pdev)
{
	struct cvi_spacc *spacc;
	struct device *dev = &pdev->dev;
	int ret = 0;

	spacc = devm_kzalloc(dev, sizeof(*spacc), GFP_KERNEL);
	if (!spacc)
		return -ENOMEM;

	spacc->dev = dev;

	ret = alloc_chrdev_region(&spacc->tdev, 0, 1, DEVICE_NAME);
	if (ret) {
		dev_err(dev, "Failed to allocate chrdev region\n");
		return ret;
	}

	cdev_init(&spacc->cdev, &spacc_fops);
	spacc->cdev.owner = THIS_MODULE;

	ret = cdev_add(&spacc->cdev, spacc->tdev, 1);
	if (ret) {
		dev_err(dev, "Failed to add cdev\n");
		goto failed_cdev;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
	spacc->spacc_class = class_create(THIS_MODULE, DEVICE_NAME);
#else
	spacc->spacc_class = class_create(DEVICE_NAME);
#endif
	if (IS_ERR(spacc->spacc_class)) {
		dev_err(dev, "Failed to create class\n");
		ret = PTR_ERR(spacc->spacc_class);
		goto failed_class;
	}

	if (IS_ERR(device_create(spacc->spacc_class, NULL, spacc->tdev, spacc,
				 DEVICE_NAME))) {
		dev_err(dev, "Failed to create device\n");
		ret = PTR_ERR(device_create(spacc->spacc_class, NULL,
					    spacc->tdev, spacc, DEVICE_NAME));
		goto failed_device;
	}

	platform_set_drvdata(pdev, spacc);
	dev_info(dev, "cvitek_spacc_drv_probe success\n");
	return 0;

failed_device:
	class_destroy(spacc->spacc_class);
failed_class:
	cdev_del(&spacc->cdev);
failed_cdev:
	unregister_chrdev_region(spacc->tdev, 1);
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
static int cvitek_spacc_drv_remove(struct platform_device *pdev)
#else
static void cvitek_spacc_drv_remove(struct platform_device *pdev)
#endif
{
	struct cvi_spacc *spacc = platform_get_drvdata(pdev);
	
	if (!spacc)
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
		return -EINVAL;
	
#else
		return;
#endif

	device_destroy(spacc->spacc_class, spacc->tdev);
	class_destroy(spacc->spacc_class);
	cdev_del(&spacc->cdev);
	unregister_chrdev_region(spacc->tdev, 1);
	platform_set_drvdata(pdev, NULL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
	return 0;
#else
	return;
#endif

}

#ifdef CONFIG_OF
static const struct of_device_id cvitek_spacc_of_match[] = {
	{
		.compatible = "cvitek,spacc",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cvitek_spacc_of_match);
#endif

#ifdef CONFIG_PM_SLEEP
static int cvitek_spacc_suspend(struct device *dev)
{
	// Release memory pool when system is suspended
	return 0;
}

static int cvitek_spacc_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops cvitek_spacc_pm_ops = { SET_SYSTEM_SLEEP_PM_OPS(
	cvitek_spacc_suspend, cvitek_spacc_resume) };
#endif

static struct platform_driver cvitek_spacc_driver = {
	.probe		= cvitek_spacc_drv_probe,
	.remove		= cvitek_spacc_drv_remove,
	.driver		= {
		.name	= "cvitek_spacc",
		.of_match_table = of_match_ptr(cvitek_spacc_of_match),
#ifdef CONFIG_PM_SLEEP
		.pm     = &cvitek_spacc_pm_ops,
#endif
	},
};
module_platform_driver(cvitek_spacc_driver);
MODULE_AUTHOR("Sophon");
MODULE_DESCRIPTION("Cvitek Spacc Driver");
MODULE_LICENSE("GPL");

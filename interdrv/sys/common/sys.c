#include <linux/dma-buf.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif
#include <linux/proc_fs.h>

#include "sys_context.h"
#include "sys.h"
#include <linux/sys_uapi.h>
#include "linux/cv180x_efuse.h"

enum enum_cache_op {
	enum_cache_op_invalid,
	enum_cache_op_flush,
};

#define CVI_SYS_DEV_NAME   "cvi-sys"
#define CVI_SYS_CLASS_NAME "cvi-sys"

struct cvi_sys_device {
	struct device *dev;
	struct miscdevice miscdev;

	struct mutex dev_lock;
	spinlock_t close_lock;
	int use_count;
};

static int ion_debug_alloc_free;
module_param(ion_debug_alloc_free, int, 0644);


#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define BASE_CLASS_NAME "cvi-base"
static struct class *pbase_class;
static struct proc_dir_entry *proc_dir = NULL;

static ssize_t base_efuse_shadow_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int ret = 0;
	UNUSED(class);
	UNUSED(attr);

	ret = cvi_efuse_read_buf(0, buf, PAGE_SIZE);

	return ret;
}

static ssize_t base_efuse_shadow_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	unsigned long addr;
	uint32_t value = 0xDEAFBEEF;
	int ret;
	UNUSED(class);
	UNUSED(attr);

	ret = kstrtoul(buf, 0, &addr);
	if (ret < 0) {
		pr_err("efuse_read: ret=%d\n", ret);
		return ret;
	}

	ret = cvi_efuse_read_buf(addr, &value, sizeof(value));
	pr_info("efuse_read: 0x%04lx=0x%08x ret=%d\n", addr, value, ret);

	return count;
}

static ssize_t base_efuse_prog_show(struct class *class,
				    struct class_attribute *attr, char *buf)
{
	UNUSED(class);
	UNUSED(attr);

	return scnprintf(buf, PAGE_SIZE, "%s\n", "PROG_SHOW");
}

static ssize_t base_efuse_prog_store(struct class *class,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int err;
	uint32_t addr = 0, value = 0;
	UNUSED(class);
	UNUSED(attr);

	if (sscanf(buf, "0x%x=0x%x", &addr, &value) != 2)
		return -ENOMEM;

	pr_info("%s: addr=0x%02x value=0x%08x\n", __func__, addr, value);
	err = cvi_efuse_write(addr, value);
	if (err < 0)
		return err;

	return count;
}

CLASS_ATTR_RW(base_efuse_shadow);
CLASS_ATTR_RW(base_efuse_prog);

static int32_t _sys_ion_alloc_nofd(uint64_t *addr_p, void **addr_v, uint32_t u32Len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t ret = 0;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	ionbuf = cvi_ion_alloc_nofd(ION_HEAP_TYPE_CARVEOUT, u32Len, is_cached);
	if (IS_ERR(ionbuf)) {
		pr_err("ion allocated len=0x%x failed\n", u32Len);
		return -ENOMEM;
	}

	owner_name = vmalloc(MAX_ION_BUFFER_NAME);
	if (name)
		strncpy(owner_name, name, MAX_ION_BUFFER_NAME);
	else
		strncpy(owner_name, "anonymous", MAX_ION_BUFFER_NAME);

	ionbuf->name = owner_name;

	ret = ion_buf_begin_cpu_access(ionbuf);
	if (ret < 0) {
		pr_err("cvi_ion_alloc() ion_buf_begin_cpu_access failed\n");
		cvi_ion_free_nofd(ionbuf);
		return ret;
	}

	vmap_addr = ionbuf->vaddr;
	pr_debug("_sys_ion_alloc_nofd v=%p\n", vmap_addr);
	if (IS_ERR(vmap_addr)) {
		ret = -EINVAL;
		ion_buf_end_cpu_access(ionbuf);
		cvi_ion_free_nofd(ionbuf);
		return ret;
	}

	//push into memory manager
	mem_info.dmabuf = NULL;
	mem_info.dmabuf_fd = -1;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.fd_tgid = current->tgid;
	mem_info.ionbuf = ionbuf;
	if (sys_ctx_mem_put(&mem_info)) {
		pr_err("allocate mm put failed\n");
		ion_buf_end_cpu_access(ionbuf);
		cvi_ion_free_nofd(ionbuf);
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		pr_info("%s: ionbuf->name=%s\n", __func__, ionbuf->name);
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_tgid=%d\n", __func__, mem_info.fd_tgid);
		pr_info("%s: current->tgid=%d\n", __func__, current->tgid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}
	*addr_p = ionbuf->paddr;
	*addr_v = vmap_addr;
	return ret;
}

static int32_t _sys_ion_alloc(int32_t *p_fd, uint64_t *addr_p, void **addr_v, uint32_t u32Len,
	uint32_t is_cached, uint8_t *name)
{
	int32_t dmabuf_fd = 0, ret = 0;
	struct dma_buf *dmabuf;
	struct ion_buffer *ionbuf;
	uint8_t *owner_name = NULL;
	void *vmap_addr = NULL;
	struct mem_mapping mem_info;

	dmabuf_fd = cvi_ion_alloc(ION_HEAP_TYPE_CARVEOUT, u32Len, is_cached);
	if (dmabuf_fd < 0) {
		pr_err("ion allocated len=0x%x failed\n", u32Len);
		return -ENOMEM;
	}

	dmabuf = dma_buf_get(dmabuf_fd);
	if (!dmabuf) {
		pr_err("allocated get dmabuf failed\n");
		cvi_ion_free(current->tgid, dmabuf_fd);
		return -ENOMEM;
	}

	ionbuf = (struct ion_buffer *)dmabuf->priv;
	owner_name = vmalloc(MAX_ION_BUFFER_NAME);
	if (name)
		strncpy(owner_name, name, MAX_ION_BUFFER_NAME);
	else
		strncpy(owner_name, "anonymous", MAX_ION_BUFFER_NAME);

	ionbuf->name = owner_name;

	ret = dma_buf_begin_cpu_access(dmabuf, DMA_TO_DEVICE);
	if (ret < 0) {
		pr_err("cvi_ion_alloc() dma_buf_begin_cpu_access failed\n");
		dma_buf_put(dmabuf);
		cvi_ion_free(current->tgid, dmabuf_fd);
		return ret;
	}

	vmap_addr = ionbuf->vaddr;
	pr_debug("_sys_ion_alloc v=%p\n", vmap_addr);
	if (IS_ERR(vmap_addr)) {
		dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
		dma_buf_put(dmabuf);
		cvi_ion_free(current->tgid, dmabuf_fd);
		ret = -EINVAL;
		return ret;
	}

	//push into memory manager
	mem_info.dmabuf = (void *)dmabuf;
	mem_info.dmabuf_fd = dmabuf_fd;
	mem_info.vir_addr = vmap_addr;
	mem_info.phy_addr = ionbuf->paddr;
	mem_info.fd_tgid = current->tgid;
	if (sys_ctx_mem_put(&mem_info)) {
		pr_err("allocate mm put failed\n");
		dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
		dma_buf_put(dmabuf);
		cvi_ion_free(current->tgid, dmabuf_fd);
		return -ENOMEM;
	}

	if (ion_debug_alloc_free) {
		pr_info("%s: ionbuf->name=%s\n", __func__, ionbuf->name);
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_tgid=%d\n", __func__, mem_info.fd_tgid);
		pr_info("%s: current->tgid=%d\n", __func__, current->tgid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}
	*p_fd = dmabuf_fd;
	*addr_p = ionbuf->paddr;
	*addr_v = vmap_addr;
	return ret;
}

static int32_t _sys_ion_free(uint64_t addr_p)
{
	struct mem_mapping mem_info;
	struct ion_buffer *ionbuf;
	struct dma_buf *dmabuf;

	//get from memory manager
	memset(&mem_info, 0, sizeof(struct mem_mapping));
	mem_info.phy_addr = addr_p;
	if (sys_ctx_mem_get(&mem_info)) {
		pr_err("dmabuf_fd get failed\n");
		return -ENOMEM;
	}

	dmabuf = (struct dma_buf *)(mem_info.dmabuf);
#if 1
	ionbuf = (struct ion_buffer *)dmabuf->priv;
	if (ionbuf->name && ion_debug_alloc_free) {
		pr_info("%s: ionbuf->name: %s\n", __func__, ionbuf->name);
		vfree(ionbuf->name);
		ionbuf->name = NULL;
	}
#endif

	if (ion_debug_alloc_free) {
		pr_info("%s: mem_info.dmabuf=%p\n", __func__, mem_info.dmabuf);
		pr_info("%s: mem_info.dmabuf_fd=%d\n", __func__, mem_info.dmabuf_fd);
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_tgid=%d\n", __func__, mem_info.fd_tgid);
		pr_info("%s: current->tgid=%d\n", __func__, current->tgid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}
	dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
	dma_buf_put(dmabuf);

	cvi_ion_free(mem_info.fd_tgid, mem_info.dmabuf_fd);
	return 0;
}

static int32_t _sys_ion_free_nofd(uint64_t addr_p)
{
	struct mem_mapping mem_info;
	struct ion_buffer *ionbuf;

	//get from memory manager
	memset(&mem_info, 0, sizeof(struct mem_mapping));
	mem_info.phy_addr = addr_p;
	if (sys_ctx_mem_get(&mem_info)) {
		pr_err("dmabuf_fd get failed\n");
		return -ENOMEM;
	}

	ionbuf = (struct ion_buffer *)(mem_info.ionbuf);
	pr_debug("%s: ionbuf->name: %s\n", __func__, ionbuf->name);

	if (ion_debug_alloc_free) {
		pr_info("%s: mem_info.vir_addr=%p\n", __func__, mem_info.vir_addr);
		pr_info("%s: mem_info.phy_addr=0x%llx\n", __func__, mem_info.phy_addr);
		pr_info("%s: mem_info.fd_tgid=%d\n", __func__, mem_info.fd_tgid);
		pr_info("%s: current->tgid=%d\n", __func__, current->tgid);
		pr_info("%s: current->comm=%s\n", __func__, current->comm);
	}

	if (ionbuf->name) {
		vfree(ionbuf->name);
		ionbuf->name = NULL;
	}

	ion_buf_end_cpu_access(ionbuf);
	cvi_ion_free_nofd(ionbuf);

	return 0;
}

int32_t sys_ion_free(uint64_t u64PhyAddr)
{
	return _sys_ion_free(u64PhyAddr);
}
EXPORT_SYMBOL_GPL(sys_ion_free);

int32_t sys_ion_free_nofd(uint64_t u64PhyAddr)
{
	return _sys_ion_free_nofd(u64PhyAddr);
}
EXPORT_SYMBOL_GPL(sys_ion_free_nofd);

static int32_t sys_ion_free_user(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0;
	struct sys_ion_data ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_ion_data __user *)arg,
			     sizeof(struct sys_ion_data));
	if (ret) {
		pr_err("copy_from_user failed, sys_ion_free_user\n");
		return ret;
	}

	return _sys_ion_free(ioctl_arg.addr_p);
}

int32_t sys_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	int32_t dma_fd = 0;

	return _sys_ion_alloc(&dma_fd, p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
EXPORT_SYMBOL_GPL(sys_ion_alloc);

int32_t sys_ion_alloc_nofd(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached)
{
	return _sys_ion_alloc_nofd(p_paddr, pp_vaddr, buf_len, is_cached, buf_name);
}
EXPORT_SYMBOL_GPL(sys_ion_alloc_nofd);

static int32_t sys_ion_alloc_user(struct cvi_sys_device *ndev, unsigned long arg)
{
	int32_t ret = 0, dma_fd = 0;
	uint64_t addr_p = 0;
	void *addr_v = NULL;
	struct sys_ion_data ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_ion_data __user *)arg,
			     sizeof(struct sys_ion_data));
	if (ret) {
		pr_err("copy_from_user failed, sys_ion_alloc_user\n");
		return ret;
	}

	ret = _sys_ion_alloc(&dma_fd, &addr_p, &addr_v,
					ioctl_arg.size, ioctl_arg.cached, ioctl_arg.name);
	if (ret < 0) {
		pr_err("_sys_ion_alloc failed\n");
		return ret;
	}

	ioctl_arg.addr_p = addr_p;
	ioctl_arg.dmabuf_fd = dma_fd;

	ret = copy_to_user((struct sys_ion_data __user *)arg,
			     &ioctl_arg,
			     sizeof(struct sys_ion_data));
	if (ret) {
		pr_err("copy_to_user fail, cvi_tpu_loadcmdbuf_sec\n");
		return ret;
	}

	return 0;
}

int32_t sys_ion_get_memory_statics(uint64_t *total_size,
	uint64_t *free_size, uint64_t *max_avail_size)
{
	return cvi_ion_get_memory_statics(total_size, free_size, max_avail_size);
}
EXPORT_SYMBOL_GPL(sys_ion_get_memory_statics);

int32_t sys_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, u32Len, DMA_FROM_DEVICE);
#else
	__dma_map_area(addr_v, u32Len, DMA_FROM_DEVICE);
#endif

	/*	*/
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(sys_cache_invalidate);

int32_t sys_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(addr_p, u32Len, DMA_TO_DEVICE);
#else
	__dma_map_area(addr_v, u32Len, DMA_TO_DEVICE);
#endif

	/*  */
	smp_mb();
	return 0;
}
EXPORT_SYMBOL_GPL(sys_cache_flush);

static int32_t sys_cache_op_userv(unsigned long arg, enum enum_cache_op op_code)
{
	int ret = 0;
	struct sys_cache_op ioctl_arg;

	ret = copy_from_user(&ioctl_arg, (struct sys_cache_op __user *)arg, sizeof(struct sys_cache_op));
	if (ret) {
		pr_err("copy_from_user faile, sys_cache_op_userv\n");
		return ret;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	if (op_code == enum_cache_op_invalid)
		arch_sync_dma_for_device(ioctl_arg.addr_p, ioctl_arg.size, DMA_FROM_DEVICE);
	else if (op_code == enum_cache_op_flush)
		arch_sync_dma_for_device(ioctl_arg.addr_p, ioctl_arg.size, DMA_TO_DEVICE);
#else
	if (op_code == enum_cache_op_invalid)
		__dma_map_area(ioctl_arg.addr_v, ioctl_arg.size, DMA_FROM_DEVICE);
	else if (op_code == enum_cache_op_flush)
		__dma_map_area(ioctl_arg.addr_v, ioctl_arg.size, DMA_TO_DEVICE);
#endif
	/*	*/
	smp_mb();
	return 0;
}

int32_t sys_ion_dump(void)
{
	return sys_ctx_mem_dump();
}
EXPORT_SYMBOL_GPL(sys_ion_dump);

static long cvi_sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_sys_device *ndev = filp->private_data;
	long ret = 0;
	struct sys_ion_mm_statics ion_statics = {};

	switch (cmd) {
	case SYS_ION_ALLOC:
		ret = sys_ion_alloc_user(ndev, arg);
		break;
	case SYS_ION_FREE:
		ret = sys_ion_free_user(ndev, arg);
		break;
	case SYS_CACHE_INVLD:
		ret = sys_cache_op_userv(arg, enum_cache_op_invalid);
		break;
	case SYS_CACHE_FLUSH:
		ret = sys_cache_op_userv(arg, enum_cache_op_flush);
		break;
	case SYS_ION_G_ION_MM_STATICS:
		sys_ion_get_memory_statics(&ion_statics.total_size, &ion_statics.free_size,
			&ion_statics.max_avail_size);
		ret = copy_to_user((struct sys_ion_mm_statics __user *)arg,
					&ion_statics,
					sizeof(struct sys_ion_mm_statics));
		if (ret) {
			pr_err("copy_to_user fail, total:%llu free:%lld avail:%lld ret:%ld\n",
				ion_statics.total_size,	ion_statics.free_size, ion_statics.max_avail_size, ret);
		}
		break;

	default:
		return -ENOTTY;
	}
	return ret;
}

static int cvi_sys_open(struct inode *inode, struct file *filp)
{
	struct cvi_sys_device *ndev = container_of(filp->private_data, struct cvi_sys_device, miscdev);

	spin_lock(&ndev->close_lock);
	ndev->use_count++;
	spin_unlock(&ndev->close_lock);
	filp->private_data = ndev;
	return 0;
}

static int cvi_sys_close(struct inode *inode, struct file *filp)
{
	struct cvi_sys_device *ndev = filp->private_data;
	int cnt = 0;

	spin_lock(&ndev->close_lock);
	cnt = --ndev->use_count;
	spin_unlock(&ndev->close_lock);

	filp->private_data = NULL;
	return 0;
}

#ifdef CONFIG_COMPAT
static long compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations cvi_sys_fops = {
	.owner = THIS_MODULE,
	.open = cvi_sys_open,
	.release = cvi_sys_close,
	.unlocked_ioctl = cvi_sys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ptr_ioctl,
#endif
};

int cvi_sys_register_misc(struct cvi_sys_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = CVI_SYS_DEV_NAME;
	ndev->miscdev.fops = &cvi_sys_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "cvi_sys: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static void sys_cleanup(void)
{
	class_remove_file(pbase_class, &class_attr_base_efuse_shadow);
	class_remove_file(pbase_class, &class_attr_base_efuse_prog);
	class_destroy(pbase_class);
}

static int cvi_sys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_sys_device *ndev;
	int32_t ret;
	struct file *filp = NULL;
	int rc;

	pr_debug("===cvitek_sys_probe start\n");
	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	pbase_class = class_create(THIS_MODULE, BASE_CLASS_NAME);
	if (IS_ERR(pbase_class)) {
		pr_err("create class failed\n");
		rc = PTR_ERR(pbase_class);
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_efuse_shadow);
	if (rc) {
		pr_err("sys: can't create sysfs base_efuse_shadow file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_efuse_prog);
	if (rc) {
		pr_err("sys: can't create sysfs base_efuse_prog) file\n");
		goto cleanup;
	}

	//create cvitek proc
	filp = filp_open("/proc/cvitek", O_RDONLY, 0);
	if (IS_ERR(filp)) {
		proc_dir = proc_mkdir("cvitek", NULL);
	} else {
		filp_close(filp, 0);
	}

	mutex_init(&ndev->dev_lock);
	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;

	ret = cvi_sys_register_misc(ndev);
	if (ret < 0) {
		pr_err("register misc error\n");
		return ret;
	}

	sys_ctx_init();

	platform_set_drvdata(pdev, ndev);

	pr_debug("===cvitek_sys_probe end\n");
	return 0;

cleanup:
	sys_cleanup();
	return rc;
}

static int cvi_sys_remove(struct platform_device *pdev)
{
	struct cvi_sys_device *ndev = platform_get_drvdata(pdev);

	sys_cleanup();

	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id cvitek_sys_match[] = {
	{ .compatible = "cvitek,sys" },
	{},
};
MODULE_DEVICE_TABLE(of, cvitek_sys_match);

static struct platform_driver cvitek_sys_driver = {
	.probe = cvi_sys_probe,
	.remove = cvi_sys_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = CVI_SYS_DEV_NAME,
			.of_match_table = cvitek_sys_match,
		},
};
module_platform_driver(cvitek_sys_driver);

MODULE_AUTHOR("Wellken Chen<wellken.chen@cvitek.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC SYS driver");
MODULE_LICENSE("GPL");


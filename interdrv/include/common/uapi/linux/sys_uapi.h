#ifndef __U_SYS_UAPI_H__
#define __U_SYS_UAPI_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/version.h>

struct sys_ion_mm_statics {
	__u64 total_size;
	__u64 free_size;
	__u64 max_avail_size;
};

#define SYS_IOC_MAGIC		'S'
#define SYS_ION_ALLOC     _IOWR(SYS_IOC_MAGIC, 0x01, unsigned long long)
#define SYS_ION_FREE      _IOW(SYS_IOC_MAGIC, 0x02, unsigned long long)
#define SYS_CACHE_INVLD   _IOW(SYS_IOC_MAGIC, 0x03, unsigned long long)
#define SYS_CACHE_FLUSH   _IOW(SYS_IOC_MAGIC, 0x04, unsigned long long)

#define SYS_INIT_USER     _IOW(SYS_IOC_MAGIC, 0x05, unsigned long long)
#define SYS_EXIT_USER     _IOW(SYS_IOC_MAGIC, 0x06, unsigned long long)
#define SYS_GET_SYSINFO   _IOR(SYS_IOC_MAGIC, 0x07, unsigned long long)

#define SYS_SET_MODECFG   _IOW(SYS_IOC_MAGIC, 0x08, unsigned long long)
#define SYS_GET_MODECFG   _IOR(SYS_IOC_MAGIC, 0x08, unsigned long long)
#define SYS_SET_BINDCFG   _IOW(SYS_IOC_MAGIC, 0x09, unsigned long long)
#define SYS_GET_BINDCFG   _IOR(SYS_IOC_MAGIC, 0x09, unsigned long long)

#define SYS_IOC_G_CTRL    _IOWR(SYS_IOC_MAGIC, 0x10, unsigned long long)
#define SYS_IOC_S_CTRL    _IOWR(SYS_IOC_MAGIC, 0x11, unsigned long long)

#define SYS_ION_G_ION_MM_STATICS _IOWR(SYS_IOC_MAGIC, 0x12, struct sys_ion_mm_statics)

enum SYS_IOCTL {
	SYS_IOCTL_SET_VIVPSSMODE,
	SYS_IOCTL_GET_VIVPSSMODE,
	SYS_IOCTL_SET_VPSSMODE,
	SYS_IOCTL_GET_VPSSMODE,
	SYS_IOCTL_SET_VPSSMODE_EX,
	SYS_IOCTL_GET_VPSSMODE_EX,
	SYS_IOCTL_SET_SYS_INIT,
	SYS_IOCTL_GET_SYS_INIT,
	SYS_IOCTL_MAX,
};

struct sys_ext_control {
	__u32 id;
	__u32 reserved[1];
	union {
		__s32 value;
		__s64 value64;
		void *ptr;
	};
} __attribute__ ((packed));

struct sys_cache_op {
	void *addr_v;
	__u64 addr_p;
	__u64 size;
	__s32 dma_fd;
};

#define MAX_ION_BUFFER_NAME 32
struct sys_ion_data {
	__u32 size;
	__u32 cached;
	__u32 dmabuf_fd;
	__u64 addr_p;
	__u8 name[MAX_ION_BUFFER_NAME];
};

#ifdef __cplusplus
	}
#endif

#endif /* __U_SYS_UAPI_H__ */

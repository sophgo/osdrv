
#include "ipcm_common.h"

#ifdef __LINUX_DRV__
#include <asm/io.h>
#else
#include <errno.h>
#endif

// #define __ASSEMBLY__
#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

#define SYS_COUNTER_FREQ_IN_SECOND 25000000

IPCM_LOG_LEVEL_E g_ipcm_log_level = IPCM_LOG_LEVEL_DEFAULT;

#ifdef __riscv
#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)	\
				: "=r" (__v) :			\
				: "memory");			\
	__v;							\
})

#define CSR_TIME 0xc01
#else
#ifdef __LINUX_DRV__
// arm linux driver
#include <linux/timex.h>
#include <linux/math64.h>
extern u64 (*arch_timer_read_counter)(void);
#else
// arm linux user space
unsigned long long timer_read_counter(void)
{
	unsigned long long cval = 0;
	// aarch64 not support now
	// asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (cval));
	return cval;
}
#endif
#endif

unsigned long long timer_get_boot_us(void)
{
	unsigned long long boot_us = 0;
#ifdef __riscv
	boot_us = csr_read(CSR_TIME) / (SYS_COUNTER_FREQ_IN_SECOND / 1000000);
#else
	#ifdef __LINUX_DRV__
	// arm linux driver
	boot_us = div_u64(arch_timer_read_counter(), (SYS_COUNTER_FREQ_IN_SECOND / 1000000));
	#else
	// arm linux user space
	boot_us = timer_read_counter()  / (SYS_COUNTER_FREQ_IN_SECOND / 1000000);
	#endif
#endif

	return boot_us;
}

s32 ipcm_get_grp_id(PortType type, u8 port_id, u8 *grp_id)
{
	if (grp_id == NULL) {
		ipcm_err("grp_id is null.\n");
		return -EFAULT;
	}

	switch(type) {
		case PORT_SYSTEM:
			if (port_id >= IPCM_SYS_PORT_MAX) {
				ipcm_err("type(%d) port_id(%d max:%d) is invalid.\n", type, port_id, IPCM_SYS_PORT_MAX);
				return -EINVAL;
			}
			*grp_id = IPCM_SYS_PORT_IDX + port_id;
			break;
		case PORT_VIRTTTY:
			if (port_id >= IPCM_VIRTTTY_PORT_MAX) {
				ipcm_err("type(%d) port_id(%d max:%d) is invalid.\n", type, port_id, IPCM_VIRTTTY_PORT_MAX);
				return -EINVAL;
			}
			*grp_id = IPCM_VIRTTTY_PORT_IDX + port_id;
			break;
		case PORT_SHAREFS:
			if (port_id >= IPCM_SHAREFS_PORT_MAX) {
				ipcm_err("type(%d) port_id(%d max:%d) is invalid.\n", type, port_id, IPCM_SHAREFS_PORT_MAX);
				return -EINVAL;
			}
			*grp_id = IPCM_SHAREFS_PORT_IDX + port_id;
			break;
		case PORT_MSG:
			if (port_id >= IPCM_MSG_PORT_MAX) {
				ipcm_err("type(%d) port_id(%d max:%d) is invalid.\n", type, port_id, IPCM_MSG_PORT_MAX);
				return -EINVAL;
			}
			*grp_id = IPCM_MSG_PORT_IDX + port_id;
			break;
		case PORT_ANON:
			if (port_id >= IPCM_ANON_PORT_MAX) {
				ipcm_err("type(%d) port_id(%d max:%d) is invalid.\n", type, port_id, IPCM_ANON_PORT_MAX);
				return -EINVAL;
			}
			*grp_id = IPCM_ANON_PORT_IDX + port_id;
			break;
		default:
			ipcm_err("type(%d) is invalid, max is %d.\n", type, PORT_ANON);
			return -EINVAL;
	}

	return 0;
}

s32 ipcm_get_port_id(u8 grp_id, u8 *type, u8 *port_id)
{
	if ((type == NULL) || (port_id == NULL)) {
		ipcm_err("type or port_id is null.\n");
		return -EFAULT;
	}

	do {
		if ((grp_id >= IPCM_MSG_PORT_IDX) && (grp_id < (IPCM_MSG_PORT_IDX + IPCM_MSG_PORT_MAX))) {
			*type = PORT_MSG;
			*port_id = grp_id - IPCM_MSG_PORT_IDX;
		}
		if ((grp_id >= IPCM_ANON_PORT_IDX) && (grp_id < (IPCM_ANON_PORT_IDX + IPCM_ANON_PORT_MAX))) {
			*type = PORT_ANON;
			*port_id = grp_id - IPCM_ANON_PORT_IDX;
		}
		// grp_id >= IPCM_SYS_PORT_IDX
		if (grp_id < (IPCM_SYS_PORT_IDX + IPCM_SYS_PORT_MAX)) {
			*type = PORT_SYSTEM;
			*port_id = grp_id - IPCM_SYS_PORT_IDX;
		}
		if ((grp_id >= IPCM_VIRTTTY_PORT_IDX) && (grp_id < (IPCM_VIRTTTY_PORT_IDX + IPCM_VIRTTTY_PORT_MAX))) {
			*type = PORT_VIRTTTY;
			*port_id = grp_id - IPCM_VIRTTTY_PORT_IDX;
		}
		if ((grp_id >= IPCM_SHAREFS_PORT_IDX) && (grp_id < (IPCM_SHAREFS_PORT_IDX + IPCM_SHAREFS_PORT_MAX))) {
			*type = PORT_SHAREFS;
			*port_id = grp_id - IPCM_SHAREFS_PORT_IDX;
		}

		return 0;
	} while(0);

	ipcm_err("grp_id(%d) out of range.\n", grp_id);
	return -EINVAL;
}

int ipcm_get_log_level(void)
{
	return g_ipcm_log_level;
}

void ipcm_set_log_level(IPCM_LOG_LEVEL_E level)
{
	g_ipcm_log_level = level;
}

inline int ipcm_log_level_debug(void)
{
	return g_ipcm_log_level >= IPCM_LOG_DEBUG;
}

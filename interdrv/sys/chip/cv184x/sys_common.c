#include "osal.h"
#include "defines.h"
#include "sys_uapi.h"
#include "reg.h"
#include "sys_common.h"
#include "sys_debug.h"

/* register bank */

#define RTC_ST_ON_REASON 0xF8

static uintptr_t top_base, rtc_base;

unsigned int sys_comm_read_chip_id(void)
{
	unsigned int cpu_bonding = _reg_read(top_base) >> 16;

	TRACE_SYS(DBG_DEBUG, "cpu_bonding=0x%x\n", cpu_bonding);

	switch (cpu_bonding) {
	case 0x1840:
		return E_CHIPID_CV184X;
	default:
		return E_CHIPID_UNKOWN;
	};

	return E_CHIPID_UNKOWN;
}
osal_module_export(sys_comm_read_chip_id);

unsigned int sys_comm_read_chip_version(void)
{
	unsigned int chip_version = 0;

	chip_version = _reg_read(top_base) & 0xffff;

	TRACE_SYS(DBG_DEBUG, "chip_version=0x%x\n", chip_version);

	switch (chip_version) {
	case 0x0:
		return E_CHIPVERSION_U01;
	default:
		return E_CHIPVERSION_U02;
	}
}
osal_module_export(sys_comm_read_chip_version);

unsigned int sys_comm_read_chip_pwr_on_reason(void)
{
	unsigned int reason = 0;

	reason = _reg_read(rtc_base + RTC_ST_ON_REASON);

	TRACE_SYS(DBG_DEBUG, "pwr on reason = 0x%x\n", reason);

	switch (reason) {
	case 0x800d0000:
	case 0x800f0000:
		return E_CHIP_PWR_ON_COLDBOOT;
	case 0x880d0003:
	case 0x880f0003:
		return E_CHIP_PWR_ON_WDT;
	case 0x80050009:
	case 0x800f0009:
		return E_CHIP_PWR_ON_SUSPEND;
	case 0x840d0003:
	case 0x840f0003:
		return E_CHIP_PWR_ON_WARM_RST;
	default:
		return E_CHIP_PWR_ON_COLDBOOT;
	}
}
osal_module_export(sys_comm_read_chip_pwr_on_reason);

int sys_comm_set_base_addr(void *top, void *rtc)
{
	top_base = (uintptr_t)top;
	rtc_base = (uintptr_t)rtc;

	TRACE_SYS(DBG_WARN, "CVITEK CHIP ID = %d\n", sys_comm_read_chip_id());

	return 0;
}


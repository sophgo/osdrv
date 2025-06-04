/**
 * @file ipcm_common.c
 * @brief Common utilities and helper functions for IPCM
 *
 * Implements shared functionality used across the IPCM system including:
 * - Timing functions
 * - Port ID management
 * - Logging control
 * - Debug level settings
 */

#include "ipcm_common.h"

#include "ipcm_plat_adapter.h"

// Default log level for IPCM system
IPCM_LOG_LEVEL_E g_ipcm_log_level = IPCM_LOG_LEVEL_DEFAULT;

/**
 * @brief Get system boot time in microseconds
 * 
 * Wrapper around platform-specific timer function
 *
 * @return Boot time in microseconds
 */
unsigned long long timer_get_boot_us(void)
{
	return ipcmpa_get_boot_us();
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
		case PORT_CUST:
			if (port_id >= IPCM_CUST_PORT_MAX) {
				ipcm_err("type(%d) port_id(%d max:%d) is invalid.\n", type, port_id, IPCM_CUST_PORT_MAX);
				return -EINVAL;
			}
			*grp_id = IPCM_CUST_PORT_IDX + port_id;
			break;
		default:
			ipcm_err("type(%d) is invalid, max is %d.\n", type, PORT_CUST);
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
		if ((grp_id >= IPCM_CUST_PORT_IDX) && (grp_id < (IPCM_CUST_PORT_IDX + IPCM_CUST_PORT_MAX))) {
			*type = PORT_CUST;
			*port_id = grp_id - IPCM_CUST_PORT_IDX;
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

/**
 * @brief Get current log level
 * @return Current log level setting
 */
int ipcm_get_log_level(void)
{
	return g_ipcm_log_level;
}

/**
 * @brief Set system log level
 * @param level New log level to set
 */
void ipcm_set_log_level(IPCM_LOG_LEVEL_E level)
{
	g_ipcm_log_level = level;
}

/**
 * @brief Check if debug logging is enabled
 * @return 1 if debug logging enabled, 0 otherwise
 */
inline int ipcm_log_level_debug(void)
{
	return g_ipcm_log_level >= IPCM_LOG_DEBUG;
}

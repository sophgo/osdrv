/**
 * @file ipcm_custom.c
 * @brief Custom IPCM implementation for shared memory communication
 *
 * Implements custom IPCM functionality including:
 * - Shared memory pool management
 * - Buffer allocation and release
 * - Message packing and sending
 */

#include "ipcm_plat_adapter.h"
#include "ipcm_dbg_rec_info.h"
#include "ipcm_port.h"
#include "ipcm_custom.h"
#include "ipcm.h"

extern s32 ipcm_cust_plat_init(void);
extern s32 ipcm_cust_plat_uninit(void);

static POOLHANDLE _pool_handle;
static u32 _pool_offset = 0; // Offset to pool manager

s32 ipcm_cust_srv_init(PoolConfig *config)
{
	s32 ret = -1;

	_pool_handle = pool_create(&_pool_offset, config, IPCM_CUST_POOL_ID);
	if (!_pool_handle) {
		ipcm_err("cust pool create fail,ret:%d\n", ret);
		return -1;
	}
	return ipcm_cust_plat_init();
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_srv_init);

s32 ipcm_cust_srv_uninit(void)
{
	s32 ret = -1;
	ret = pool_destroy(_pool_handle);
	if (ret) {
		ipcm_err("cust pool_destroy fail,ret:%d\n", ret);
	}
	return ipcm_cust_plat_uninit();
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_srv_uninit);

s32 ipcm_cust_cli_init(void)
{
	s32 ret = -1;

	_pool_handle = pool_mgr_get(&_pool_offset, IPCM_CUST_POOL_ID);
	if (!_pool_handle) {
		ipcm_err("cust pool get fail,ret:%d\n", ret);
		return -1;
	}
	return ipcm_cust_plat_init();
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_cli_init);

s32 ipcm_cust_cli_uninit(void)
{
	return ipcm_cust_plat_uninit();
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_cli_uninit);

/**
 * @brief Get pool offset for custom IPCM
 * @return Current pool offset
 */
s32 ipcm_cust_get_pool_offset(void)
{
	return _pool_offset;
}

/**
 * @brief Pack data into message structure
 *
 * @param data Data to pack
 * @param len Data length
 * @param msg Message structure to fill
 * @return Status code
 */
s32 ipcm_cust_data_packed(void *data, u32 len, MsgData *msg)
{
	return ipcm_port_data_packed(_pool_handle, data, len, msg);
}

/**
 * @brief Send custom message with data
 *
 * @param port_id Target port ID
 * @param msg_id Message identifier
 * @param data Message data
 * @param len Data length
 * @return Status code
 */
s32 ipcm_cust_send_msg(u8 port_id, u8 msg_id, void *data, u32 len)
{
	MsgData stMsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

    // Validate parameters
    if (data == NULL) {
        ipcm_err("data is null.\n");
        return -EFAULT;
    }
    if (port_id >= IPCM_CUST_PORT_MAX) {
        ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_CUST_PORT_MAX-1);
        return -EINVAL;
    }

    // Get group ID for custom port
    ret = ipcm_get_grp_id(PORT_CUST, port_id, &grp_id);
    if (ret) {
        ipcm_err("ipcm get grp id fail ret:%d.\n", ret);
        return ret;
    }

    // Pack message and send
    stMsg.grp_id = grp_id;
    stMsg.msg_id = msg_id;
    stMsg.func_type = MSG_TYPE_SHM;
    ret = ipcm_port_data_packed(_pool_handle, data, len, &stMsg);
    if (ret) {
        ipcm_err("ipcm_port_data_packed failed ret:%d.\n", ret);
        return ret;
    }

	return ipcm_send_msg(&stMsg);
}

/**
 * @brief Get buffer offset from shared memory pool
 * @param size Requested buffer size
 * @return Buffer offset or error code
 */
u32 ipcm_cust_get_buff_offset(u32 size)
{
    return pool_alloc_offset(_pool_handle, size);
}

/**
 * @brief Release buffer by its offset
 * @param pos Buffer offset to release
 * @return Status code
 */
s32 ipcm_cust_release_buff_by_offset(u32 pos)
{
    s32 ret = 0;

    ret = pool_free_by_offset(_pool_handle, pos);
	IPCM_DBG_R_BACKTRCE_BY_POS(_pool_handle, pos, 0);
	return ret;
}

/**
 * @brief Get data pointer from buffer offset
 * @param data_pos Buffer offset
 * @return Pointer to data or NULL
 */
void *ipcm_cust_get_data_by_offset(u32 data_pos)
{
    return pool_get_data_by_offset(_pool_handle, data_pos);
}

/**
 * @brief Allocate buffer from shared memory pool
 * @param size Requested buffer size
 * @return Pointer to allocated buffer or NULL
 */
void *ipcm_cust_get_buff(u32 size)
{
	void *data;
	data = pool_alloc_buffer(_pool_handle, size);
	IPCM_DBG_R_BACKTRCE(_pool_handle, data, 1);

	return data;
}

/**
 * @brief Release allocated buffer
 * @param data Pointer to buffer
 * @return Status code
 */
s32 ipcm_cust_release_buff(void *data)
{
	s32 ret;

	ret = pool_free_buffer(_pool_handle, data);
	IPCM_DBG_R_BACKTRCE(_pool_handle, data, 0);

	return ret;
}

/**
 * @brief Reset custom IPCM pool
 * @return Status code
 */
s32 ipcm_cust_pool_reset(void)
{
	return pool_reset(_pool_handle);
}

IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_send_msg);
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_get_buff_offset);
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_release_buff_by_offset);
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_get_data_by_offset);
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_get_buff);
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_release_buff);
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_pool_reset);

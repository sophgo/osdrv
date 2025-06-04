#include <linux/module.h>
#include <linux/delay.h>

#include "ipcm_port.h"
#include "ipcm_custom.h"

#define IPCM_CUST_TEST_NAME "cust_test"

#define CUST_PRIV_DATA_MAGIC 0x443355aa
#define CUST_TEST_PARAM_DATA 0x4a5a2230

static s32 _cust_msg_process(void *priv, ipcm_cust_msg_t *data)
{
	u8 port_id, msg_id, data_type;
	u32 data_len;
	u32 i = 0;
	s32 ret = 0;

	if (priv != (void *)CUST_PRIV_DATA_MAGIC) {
		ipcm_err("======cust test fail, reg handle magic err.\n");
		return -1;
	}
	if (data == NULL) {
		ipcm_err("======cust test fail, handle data is null.\n");
		return -1;
	}

	port_id = data->port_id;
	msg_id = data->msg_id;
	data_type = data->data_type;
	data_len = data->size;

	ipcm_info("cust recv port_id(%u) msg_id(%u) data_type(%u) data(%lx) len(%u)\n",
		port_id, msg_id, data_type, (unsigned long)data->data, data_len);

	if (data_type == MSG_TYPE_SHM) {
		for (i = 0; i < data_len; i++) {
			u8 tmp = *(u8 *)(data->data + i);
			if (tmp != (0x5a + msg_id)) {
				ipcm_err("======cust recv data fail, data_type(%d), data[%d](%x), except(%x)\n",
					data_type, i, tmp, 0x5a + msg_id);
				ret = -1;
			}
		}
	}
	if (data_type == MSG_TYPE_RAW_PARAM) {
		if (data->data != (void *)(unsigned long)(CUST_TEST_PARAM_DATA + port_id + msg_id)) {
			ipcm_err("======cust recv data fail, data_type(%d), data(%lx) except(%x)\n",
				data_type, (unsigned long)data->data, CUST_TEST_PARAM_DATA + port_id + msg_id);
			ret = -1;
		}
	}

	return 1; // 1 : has been handled by kernel
}

static int ipcm_cust_test_init(void)
{
	s32 ret = 0;
	int i = 0;
	u32 size_init = 77;

	u32 buf_size;
	void *buf_data;
	u8 port_id;
	u8 msg_id;

	ipcm_info("%s start ---\n", __func__);
	// ipcm_info("name=%s\n", pdev->name);

	ipcm_cust_cli_init();
	ipcm_cust_register_handle(_cust_msg_process, (void *)CUST_PRIV_DATA_MAGIC);

	// send IPCM_CUST_KER_PORT_ST msg to trigger alios cust test for test
	ipcm_cust_send_param(IPCM_CUST_KER_PORT_ST, 0, CUST_TEST_PARAM_DATA + IPCM_CUST_KER_PORT_ST);
	// send 20 msg
	for (i = 0; i < 20; i++) {
		port_id = i % IPCM_CUST_PORT_MAX;
		msg_id = i % 128;

		buf_size = size_init + 7 * i;
		buf_data = ipcm_cust_get_buff(buf_size);
		if (!buf_data) {
			ipcm_err("ipcm get buff fail.\n");
			continue;
		}
		memset(buf_data, 0x5a + msg_id, buf_size);
		ret = ipcm_cust_send_msg(port_id, msg_id, buf_data, buf_size);
		if (ret) {
			ipcm_err("ipcm cust send msg fail ret:%d\n", ret);
		}

		ret = ipcm_cust_send_param(port_id, msg_id, CUST_TEST_PARAM_DATA + port_id + msg_id);
		if (ret) {
			ipcm_err("ipcm cust send msg fail ret:%d\n", ret);
		}

		msleep(10);
    }

	ipcm_info("%s DONE\n", __func__);
	return 0;
}

static void ipcm_cust_test_exit(void)
{
	ipcm_info("%s start\n", __func__);
	ipcm_cust_deregister_handle();
	ipcm_cust_cli_uninit();
	ipcm_info("%s done\n", __func__);
}

module_init(ipcm_cust_test_init);
module_exit(ipcm_cust_test_exit);

MODULE_AUTHOR("cvitek");
MODULE_DESCRIPTION("IPCM_CUST_TEST");
MODULE_LICENSE("GPL");
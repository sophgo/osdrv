#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <uapi/linux/sched/types.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "linux/cvi_comm_ipcmsg.h"

#include "ipcm_message.h"
#include "ipcmsg_dev.h"

#define IPCMSG_PROC_NAME          "cvitek/ipcmsg"

#define CVI_DBG_ERR        1   /* error conditions                     */
#define CVI_DBG_WARN       2   /* warning conditions                   */
#define CVI_DBG_NOTICE     3   /* normal but significant condition     */
#define CVI_DBG_INFO       4   /* informational                        */
#define CVI_DBG_DEBUG      5   /* debug-level messages                 */

#define CVI_TRACE_IPCMSG(level, fmt, ...) \
	do { \
		if (level <= ipcmsg_log_lv) { \
			if (level == CVI_DBG_ERR) \
				pr_err("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_WARN) \
				pr_warn("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_NOTICE) \
				pr_notice("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_INFO) \
				pr_info("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
			else if (level == CVI_DBG_DEBUG) \
				printk(KERN_DEBUG "%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)


#define IPCMSG_CHECK_NULL_PTR(ptr)  \
	do {  \
		if (!(ptr)) {  \
			CVI_TRACE_IPCMSG(CVI_DBG_ERR, "NULL pointer.\n");  \
			return CVI_IPCMSG_ENULL_PTR;  \
		}  \
	} while (0)

#define MSG_MAGIC 0x9472
#define SERVICE_NAME_MAX_LEN 32
#define MSG_MAX_LEN 4096

extern void ipcmsg_signal_send(int s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg);

u32 ipcmsg_log_lv = CVI_DBG_INFO;
module_param(ipcmsg_log_lv, int, 0644);

struct ipcmsg_service {
	CVI_IPCMSG_CONNECT_S stConnect;
	char aszServiceName[SERVICE_NAME_MAX_LEN];
	int s32Id;
	bool isConnected;
	bool isRun;
	atomic_t user_cnt;
	struct task_struct *thread;
	struct list_head node;
};

struct msg_item {
	CVI_IPCMSG_MESSAGE_S stMsg;
	CVI_IPCMSG_MESSAGE_S *pstRespMsg;
	wait_queue_head_t wait;
	uint64_t u64PtsUs;
	uint64_t u64PtsTmp;
	bool avail;
	bool isSync;
	struct list_head node;
};

struct msg_head {
	unsigned short u16Magic;
	unsigned short u16MsgLen;
};

struct ipcmsg_status_info {
	atomic_t RecvCnt;
	atomic_t SendCnt;
	atomic_t SendSyncCnt;
	atomic_t SendASyncCnt;
	atomic_t SendOnlyCnt;
	atomic_t ThreadStep; //0:poll  1:read msg  2:message 3:resp message
};

static atomic_t s_service_id = ATOMIC_INIT(0);
static struct ipcmsg_status_info s_status;

DEFINE_MUTEX(s_Service_lock);
DEFINE_MUTEX(s_req_lock);
DEFINE_MUTEX(s_msg_rw_lock);


LIST_HEAD(s_ServiceList);
LIST_HEAD(s_ReqList);


static void ipcmsg_param_init(void)
{
	atomic_set(&s_status.RecvCnt, 0);
	atomic_set(&s_status.SendCnt, 0);
	atomic_set(&s_status.SendSyncCnt, 0);
	atomic_set(&s_status.SendASyncCnt, 0);
	atomic_set(&s_status.SendOnlyCnt, 0);
	atomic_set(&s_status.ThreadStep, 0);
}

static struct ipcmsg_service *_find_service_by_id(int s32Id)
{
	struct ipcmsg_service *pstService = NULL;

	mutex_lock(&s_Service_lock);
	if (list_empty(&s_ServiceList)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "s_ServiceList empty, s32Id(%d).\n", s32Id);
		mutex_unlock(&s_Service_lock);
		return NULL;
	}

	list_for_each_entry(pstService, &s_ServiceList, node) {
		if (s32Id == pstService->s32Id) {
			mutex_unlock(&s_Service_lock);
			return pstService;
		}
	}
	mutex_unlock(&s_Service_lock);

	return NULL;
}

uint64_t _GetCurUsPTS(void)
{
	struct timespec64 time;

	ktime_get_ts64(&time);
	return time.tv_sec*1000000 + time.tv_nsec/1000;
}

static int _send_msg(CVI_IPCMSG_MESSAGE_S *pstMsg)
{
	int s32Ret = 0;
	struct msg_head stHead;
	u8 msg_id = 0;
	void *p = NULL;
	unsigned int u32MsgLen = sizeof(*pstMsg) + pstMsg->u32BodyLen;
	unsigned int u32HeadLen = sizeof(stHead);
	unsigned int u32TotalLen = u32HeadLen + u32MsgLen;
	//uint64_t u64Time1, u64Time2;

	if (u32TotalLen > MSG_MAX_LEN) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "Message len(%d), exceeds the maximum value(%d)\n", u32TotalLen, MSG_MAX_LEN);
		return -1;
	}
	//u64Time1 = csi_tick_get_us();

	CVI_TRACE_IPCMSG(CVI_DBG_DEBUG, "[send] resp(%d) ID(%llu) mod(%x) cmd(%d), msg:%zu body_len:%d\n",
		pstMsg->bIsResp, pstMsg->u64Id, pstMsg->u32Module, pstMsg->u32CMD,
		sizeof(CVI_IPCMSG_MESSAGE_S), pstMsg->u32BodyLen);

	mutex_lock(&s_msg_rw_lock);
	p = ipcm_msg_get_buff(u32TotalLen);
	if (!p) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "ipcm_msg_get_buff failed.\n");
		mutex_unlock(&s_msg_rw_lock);
		return -1;
	}
	//CVI_TRACE_IPCMSG(CVI_DBG_DEBUG, "ipcm_msg_get_buff:%px.\n", p);

	stHead.u16Magic = MSG_MAGIC;
	stHead.u16MsgLen = u32MsgLen;
	memcpy(p, &stHead, u32HeadLen);
	memcpy(p + u32HeadLen, pstMsg, sizeof(*pstMsg));
	if (pstMsg->u32BodyLen && copy_from_user(p + u32HeadLen + sizeof(*pstMsg),
		(void __user *)pstMsg->pBody, pstMsg->u32BodyLen)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "copy_from_user failed.\n");
		mutex_unlock(&s_msg_rw_lock);
		return -1;
	}

	msg_id = pstMsg->u64Id & 0x7F;

	s32Ret = ipcm_msg_send_msg(0, msg_id, p, u32TotalLen);
	if (s32Ret) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "ipcm_msg_send_msg failed.\n");
		mutex_unlock(&s_msg_rw_lock);
		return -1;
	}
	mutex_unlock(&s_msg_rw_lock);
	atomic_fetch_add(1, &s_status.SendCnt);
	//u64Time2 = csi_tick_get_us();
	//printf("_send_msg cost:%ld\n", u64Time2 - u64Time1);

	return 0;
}

static CVI_IPCMSG_MESSAGE_S *_read_msg(void)
{
	struct msg_head *pstHead;
	CVI_IPCMSG_MESSAGE_S *pstMsg;
	unsigned char *buf = NULL;
	int s32RecvLen;
	unsigned int u32MsgLen;
	unsigned char *data = NULL;
	unsigned int u32HeadLen = sizeof(struct msg_head);
	//uint64_t u64Time1, u64Time2;

	//u64Time1 = csi_tick_get_us();
	mutex_lock(&s_msg_rw_lock);
	s32RecvLen = ipcm_msg_read_data(0, (void **)&data, u32HeadLen);
	if (s32RecvLen != u32HeadLen) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "read head failed, recv_len(%d)\n", s32RecvLen);
		mutex_unlock(&s_msg_rw_lock);
		return NULL;
	}

	//CVI_TRACE_IPCMSG(CVI_DBG_DEBUG, "ipcm_msg_read_data:%px\n", data);
	pstHead = (struct msg_head *)data;
	u32MsgLen = pstHead->u16MsgLen;
	CVI_TRACE_IPCMSG(CVI_DBG_DEBUG, "[recv] head u16MsgLen:%d,\n", u32MsgLen);

	if (pstHead->u16Magic != MSG_MAGIC) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "magic error, magic(%d), recv magic(%d).\n", MSG_MAGIC, pstHead->u16Magic);
		mutex_unlock(&s_msg_rw_lock);
		return NULL;
	}
	if (u32MsgLen > MSG_MAX_LEN) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "len error, len(%d).\n", u32MsgLen);
		mutex_unlock(&s_msg_rw_lock);
		return NULL;
	}

	s32RecvLen = ipcm_msg_read_data(0, (void **)&data, u32MsgLen);
	if (s32RecvLen != u32MsgLen) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "ipcm_msg_read_data failed, u16MsgLen(%d), recv_len(%d)\n",
			u32MsgLen, s32RecvLen);
		mutex_unlock(&s_msg_rw_lock);
		return NULL;
	}
	atomic_fetch_add(1, &s_status.RecvCnt);

	//buf = vzalloc(u32MsgLen);
	buf = kmalloc(u32MsgLen, GFP_KERNEL);
	if (!buf) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "kmalloc failed, size(%d)\n", u32MsgLen);
		mutex_unlock(&s_msg_rw_lock);
		return NULL;
	}
	memcpy(buf, data, u32MsgLen);
	pstMsg = (CVI_IPCMSG_MESSAGE_S *)buf;
	mutex_unlock(&s_msg_rw_lock);

	CVI_TRACE_IPCMSG(CVI_DBG_DEBUG, "[recv] resp(%d) ID(%llu) mod(%x) cmd(%d), msg:%zu body_len:%d\n",
		pstMsg->bIsResp, pstMsg->u64Id, pstMsg->u32Module, pstMsg->u32CMD,
		sizeof(CVI_IPCMSG_MESSAGE_S), pstMsg->u32BodyLen);

	if (pstMsg->u32BodyLen > u32MsgLen) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "u32BodyLen(%d) error.\n", pstMsg->u32BodyLen);
		kfree(buf);
		return NULL;
	}

	pstMsg->pBody = buf + sizeof(CVI_IPCMSG_MESSAGE_S);
	//u64Time2 = csi_tick_get_us();
	//printf("_read_msg cost:%ld\n", u64Time2 - u64Time1);

	return pstMsg;
}

static int _resp_proc(struct ipcmsg_service *pstService, CVI_IPCMSG_MESSAGE_S *pstMsg)
{
	struct msg_item *item, *tmp;

	mutex_lock(&s_req_lock);
	if (list_empty(&s_ReqList)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "s_ReqList empty, msg id(%llu).\n", pstMsg->u64Id);
		goto empty_err;
	}

	list_for_each_entry_safe(item, tmp, &s_ReqList, node) {
		if (item->stMsg.u64Id == pstMsg->u64Id) {
			mutex_unlock(&s_req_lock);

			if (item->isSync) {
				item->pstRespMsg = pstMsg;
				item->u64PtsTmp = _GetCurUsPTS();
				item->avail = 1;
				wake_up_interruptible(&item->wait);
			} else {
				ipcmsg_signal_send(pstService->s32Id, pstMsg);
				mutex_lock(&s_req_lock);
				list_del(&item->node);
				mutex_unlock(&s_req_lock);
				kfree(item);
				kfree(pstMsg);
			}
			return 0;
		}
	}

empty_err:
	mutex_unlock(&s_req_lock);
	kfree(pstMsg);

	return -1;
}

int ipcmsg_add_service(const char *pszServiceName, const CVI_IPCMSG_CONNECT_S *pstConnectAttr)
{
	int fd = 0;
	struct ipcmsg_service *pstService;

	IPCMSG_CHECK_NULL_PTR(pszServiceName);
	IPCMSG_CHECK_NULL_PTR(pstConnectAttr);

	mutex_lock(&s_Service_lock);
	list_for_each_entry(pstService, &s_ServiceList, node) {
		if (!strcmp(pszServiceName, pstService->aszServiceName)) {
			CVI_TRACE_IPCMSG(CVI_DBG_DEBUG, "Service(%s) ++.\n", pszServiceName);
			atomic_fetch_add(1, &pstService->user_cnt);
			mutex_unlock(&s_Service_lock);
			return 0;
		}
	}
	mutex_unlock(&s_Service_lock);

	fd = ipcm_msg_cli_init();
	if (fd < 0) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "ipcm_msg_init failed.\n");
		return CVI_IPCMSG_EINTER;
	}

	pstService = (struct ipcmsg_service *)kzalloc(sizeof(*pstService), GFP_KERNEL);
	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "calloc failed, size(%zu)\n", sizeof(*pstService));
		ipcm_msg_cli_uninit();
		return -1;
	}

	pstService->s32Id = atomic_fetch_add(1, &s_service_id);
	atomic_set(&pstService->user_cnt, 1);
	pstService->stConnect = *pstConnectAttr;
	strncpy(pstService->aszServiceName, pszServiceName, SERVICE_NAME_MAX_LEN - 1);
	mutex_lock(&s_Service_lock);
	list_add_tail(&pstService->node, &s_ServiceList);
	mutex_unlock(&s_Service_lock);
	CVI_TRACE_IPCMSG(CVI_DBG_INFO, "Add Service, name(%s) id(%d)\n", pszServiceName, pstService->s32Id);

	return 0;
}

int ipcmsg_del_service(const char *pszServiceName)
{
	int cnt;
	struct ipcmsg_service *pstService, *tmp;

	IPCMSG_CHECK_NULL_PTR(pszServiceName);

	mutex_lock(&s_Service_lock);
	list_for_each_entry_safe(pstService, tmp, &s_ServiceList, node) {
		if (!strcmp(pszServiceName, pstService->aszServiceName)) {
			cnt = atomic_sub_return(1, &pstService->user_cnt);
			if (cnt <= 0) {
				list_del(&pstService->node);
				ipcm_msg_cli_uninit();
				kfree(pstService);
				CVI_TRACE_IPCMSG(CVI_DBG_INFO, "Del Service, name(%s)\n", pszServiceName);
			}
			break;
		}
	}
	mutex_unlock(&s_Service_lock);

	return 0;
}

int ipcmsg_tryconnect(int *ps32Id, const char *pszServiceName)
{
	int s32Id = -1;
	struct ipcmsg_service *pstService = NULL;

	IPCMSG_CHECK_NULL_PTR(ps32Id);
	IPCMSG_CHECK_NULL_PTR(pszServiceName);

	mutex_lock(&s_Service_lock);
	list_for_each_entry(pstService, &s_ServiceList, node) {
		if (!strcmp(pszServiceName, pstService->aszServiceName)) {
			s32Id = pstService->s32Id;
			break;
		}
	}
	mutex_unlock(&s_Service_lock);

	if (s32Id < 0) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "%s, service not find.\n", pszServiceName);
		return -1;
	}

	//send message

	pstService->isConnected = 1;
	*ps32Id = s32Id;

	CVI_TRACE_IPCMSG(CVI_DBG_INFO, "connected, name(%s), id(%d)\n", pszServiceName, s32Id);

	return 0;
}

int ipcmsg_connect(int *ps32Id, const char *pszServiceName)
{
	return ipcmsg_tryconnect(ps32Id, pszServiceName);
}

int ipcmsg_disconnect(int s32Id)
{
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);

	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "service not find, ID(%d).\n", s32Id);
		return -1;
	}

	if (atomic_read(&pstService->user_cnt) > 1)
		return 0;

	pstService->isConnected = 0;

	ipcm_msg_up_blank_sem(0);
	if (pstService->thread && kthread_stop(pstService->thread))
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "fail to stop ipcmsg thread.\n");

	pstService->thread = NULL;
	pstService->isRun = 0;
	CVI_TRACE_IPCMSG(CVI_DBG_INFO, "disconnect, id(%d)\n", s32Id);

	return 0;
}

bool ipcmsg_is_connected(int s32Id)
{
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);

	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "service not find, ID(%d).\n", s32Id);
		return 0;
	}
	return pstService->isConnected;
}

int ipcmsg_send_only(int s32Id, CVI_IPCMSG_MESSAGE_S *pstRequest)
{
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);

	IPCMSG_CHECK_NULL_PTR(pstRequest);

	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "Invalid s32Id(%d).\n", s32Id);
		return -1;
	}

	//send msg
	if (_send_msg(pstRequest)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "_send_msg failed.");
		return -1;
	}
	atomic_fetch_add(1, &s_status.SendOnlyCnt);

	return 0;
}

int ipcmsg_send_async(int s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg)
{
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);
	struct msg_item *item = NULL;
	int s32Ret = -1;

	IPCMSG_CHECK_NULL_PTR(pstMsg);

	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "Invalid s32Id(%d).\n", s32Id);
		return -1;
	}

	item = (struct msg_item *)kzalloc(sizeof(struct msg_item), GFP_ATOMIC);
	if (!item) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "kzalloc failed, size(%zu)\n", sizeof(*item));
		return -1;
	}

	memcpy(&(item->stMsg), pstMsg, sizeof(*pstMsg));
	item->isSync = 0;
	item->u64PtsUs = _GetCurUsPTS();

	mutex_lock(&s_req_lock);
	list_add_tail(&item->node, &s_ReqList);
	mutex_unlock(&s_req_lock);

	//send msg
	if (_send_msg(pstMsg)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "_send_msg failed.");
		goto err;
	}
	atomic_fetch_add(1, &s_status.SendASyncCnt);

	return 0;
err:
	mutex_lock(&s_req_lock);
	list_del(&item->node);
	mutex_unlock(&s_req_lock);
	kfree(item);
	return s32Ret;
}

int ipcmsg_send_sync(int s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg,
		CVI_IPCMSG_MESSAGE_S **ppstMsg, int s32TimeoutMs)
{
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);
	struct msg_item *item;
	int s32Ret = -1;

	IPCMSG_CHECK_NULL_PTR(pstMsg);
	IPCMSG_CHECK_NULL_PTR(ppstMsg);

	*ppstMsg = NULL;
	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "Invalid s32Id(%d).\n", s32Id);
		return -1;
	}

	item = (struct msg_item *)kzalloc(sizeof(struct msg_item), GFP_ATOMIC);
	if (!item) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "kzalloc failed, size(%zu)\n", sizeof(*item));
		return -1;
	}

	init_waitqueue_head(&item->wait);
	memcpy(&(item->stMsg), pstMsg, sizeof(*pstMsg));
	item->isSync = 1;
	item->avail = 0;
	item->u64PtsUs = _GetCurUsPTS();

	mutex_lock(&s_req_lock);
	list_add_tail(&item->node, &s_ReqList);
	mutex_unlock(&s_req_lock);

	//send msg
	if (_send_msg(pstMsg)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "_send_msg failed.");
		goto err;
	}
	atomic_fetch_add(1, &s_status.SendSyncCnt);

	if (s32TimeoutMs < 0) {
		s32Ret = wait_event_interruptible(item->wait, item->avail);
		// s32Ret < 0, interrupt by a signal
		// s32Ret = 0, condition true
	} else {
		s32Ret = wait_event_interruptible_timeout(item->wait, item->avail, msecs_to_jiffies(s32TimeoutMs));
		// s32Ret < 0, interrupted by a signal
		// s32Ret = 0, timeout
		// s32Ret = 1, condition true
	}

	if (item->avail) {
		s32Ret = 0;
		*ppstMsg = item->pstRespMsg;
	} else {
		s32Ret = -1;
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "timeout %dms.", s32TimeoutMs);
	}

err:
	mutex_lock(&s_req_lock);
	list_del(&item->node);
	mutex_unlock(&s_req_lock);
	kfree(item);
	return s32Ret;
}

int service_run(void *arg)
{
	int s32Ret;
	CVI_IPCMSG_MESSAGE_S *pstMsg;
	struct ipcmsg_service *pstService = (struct ipcmsg_service *)arg;

	CVI_TRACE_IPCMSG(CVI_DBG_INFO, "ipcmsg service running, id(%d)\n", pstService->s32Id);
	while (pstService->isConnected) {
		atomic_set(&s_status.ThreadStep, 0);
		s32Ret = ipcm_msg_poll(0, 1000);
		if (s32Ret)
			continue;

		/* disconnect will set isConnected = 0 and up blank sem */
		if (pstService->isConnected == 0) {
			CVI_TRACE_IPCMSG(CVI_DBG_INFO, "ipcmsg service disconnect, id(%d)\n", pstService->s32Id);
			break;
		}

		atomic_set(&s_status.ThreadStep, 1);
		pstMsg = _read_msg();
		if (!pstMsg) {
			CVI_TRACE_IPCMSG(CVI_DBG_ERR, "read msg ERR\n");
			break;
		}

		if (pstMsg->bIsResp) {
			atomic_set(&s_status.ThreadStep, 3);
			s32Ret = _resp_proc(pstService, pstMsg);
			if (s32Ret) {
				CVI_TRACE_IPCMSG(CVI_DBG_ERR, "No corresponding message sender was found.\n");
			}
		} else {
			atomic_set(&s_status.ThreadStep, 2);
			ipcmsg_signal_send(pstService->s32Id, pstMsg);
			kfree(pstMsg);
		}
	}
	CVI_TRACE_IPCMSG(CVI_DBG_INFO, "ipcmsg service end, id(%d)\n", pstService->s32Id);

	return 0;
}

void ipcmsg_run(int s32Id)
{
	int s32Ret;
	struct sched_param tsk;
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);

	if (!pstService) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "Invalid s32Id(%d).\n", s32Id);
		return;
	}

	if (pstService->isRun)
		return;

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	pstService->isRun = 1;
	pstService->thread = kthread_run(service_run, pstService, "cvitask_ipcmsg");
	if (IS_ERR(pstService->thread)) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "failed to create ipcmsg kthread\n");
		return;
	}

	s32Ret = sched_setscheduler(pstService->thread, SCHED_RR, &tsk);
	if (s32Ret) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "ipcmsg thread priority update failed: %d\n", s32Ret);
		return;
	}
}

int ipcmsg_inquireUserCnt(int s32Id)
{
	struct ipcmsg_service *pstService = _find_service_by_id(s32Id);

	if (!pstService)
		return 0;

	return atomic_read(&pstService->user_cnt);
}


static int ipcmsg_proc_show(struct seq_file *m, void *v)
{
	struct ipcmsg_status_info *status = (struct ipcmsg_status_info *)m->private;

	seq_puts(m, "\n------ipcmsg status------\n");
	seq_printf(m, "---thread step(%d)---\n", atomic_read(&status->ThreadStep));
	seq_printf(m, "---recv(%d) send(%d)---\n", atomic_read(&status->RecvCnt),
		atomic_read(&status->SendCnt));
	seq_printf(m, "---SendSync(%d) SendASync(%d) SendOnly(%d)---\n",
		atomic_read(&status->SendSyncCnt),
		atomic_read(&status->SendASyncCnt),
		atomic_read(&status->SendOnlyCnt));

	return 0;
}

static int ipcmsg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ipcmsg_proc_show, PDE_DATA(inode));
}

static const struct proc_ops ipcmsg_proc_fops = {
	.proc_open = ipcmsg_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int ipcmsg_proc_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(IPCMSG_PROC_NAME, 0644, NULL,
				 &ipcmsg_proc_fops, &s_status);
	if (!entry) {
		CVI_TRACE_IPCMSG(CVI_DBG_ERR, "ipcmsg proc creation failed\n");
		return -1;
	}
	ipcmsg_param_init();

	return 0;
}

int ipcmsg_proc_remove(void)
{
	remove_proc_entry(IPCMSG_PROC_NAME, NULL);
	return 0;
}


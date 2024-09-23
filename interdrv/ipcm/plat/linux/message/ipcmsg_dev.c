#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ipcm_common.h"
#include "linux/ipcm_linux.h"
#include "ipcmsg.h"
#include "ipcm.h"


static struct fasync_struct *async_queue;
static struct ipcm_signal_cfg stSigData;
static atomic_t s_msg_id = ATOMIC_INIT(0);
static struct mutex data_lock[IPCM_DATA_SPIN_MAX];

void ipcmsg_signal_send(CVI_S32 s32Id, CVI_IPCMSG_MESSAGE_S *pstMsg)
{
	stSigData.s32Id = s32Id;
	stSigData.pstMsg = pstMsg;

	ipcm_err("kill_fasync\n");
	kill_fasync(&async_queue, SIGIO, POLL_IN);
}

static int dev_open(struct inode *inode, struct file *file)
{
	ipcm_debug("ipcmsg open\n");

	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	ipcm_debug("ipcmsg release\n");

	//==0
	fasync_helper(0, file, 0, &async_queue);

	return 0;
}

static int dev_fasync(int fd, struct file *file, int mode)
{
	return fasync_helper(fd, file, mode, &async_queue);
}


static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case IPCM_IOC_ADD_SERVICE:
	{
		struct ipcm_add_service_cfg add_service;

		if (copy_from_user(&add_service, (void __user *)arg, sizeof(add_service)))
			return -EINVAL;
		ret = ipcmsg_add_service(add_service.aszServiceName, &add_service.stConnectAttr);
		break;
	}

	case IPCM_IOC_DEL_SERVICE:
	{
		CVI_CHAR aszServiceName[32];

		if (copy_from_user(aszServiceName, (void __user *)arg, 32))
			return -EINVAL;
		ret = ipcmsg_del_service(aszServiceName);
		break;
	}

	case IPCM_IOC_CONNECT:
	{
		struct ipcm_connect_cfg connect_cfg;

		if (copy_from_user(&connect_cfg, (void __user *)arg, sizeof(connect_cfg)))
			return -EINVAL;

		if (connect_cfg.isTry)
			ret = ipcmsg_tryconnect(&connect_cfg.s32Id, connect_cfg.aszServiceName);
		else
			ret = ipcmsg_connect(&connect_cfg.s32Id, connect_cfg.aszServiceName);

		if (copy_to_user((void __user *)arg, &connect_cfg, sizeof(connect_cfg)))
			return -EINVAL;
		break;
	}

	case IPCM_IOC_DISCONNECT:
	{
		CVI_S32 s32Id;

		if (copy_from_user(&s32Id, (void __user *)arg, sizeof(s32Id)))
			return -EINVAL;
		ret = ipcmsg_disconnect(s32Id);
		break;
	}

	case IPCM_IOC_IS_CONNECT:
	{
		CVI_S32 s32Id;

		if (copy_from_user(&s32Id, (void __user *)arg, sizeof(s32Id)))
			return -EINVAL;
		ret = ipcmsg_is_connected(s32Id) ? 1 : 0;
		break;
	}

	case IPCM_IOC_SEND_ONLY:
	{
		struct ipcm_send_only_cfg send_only_cfg;
		CVI_IPCMSG_MESSAGE_S stMsg;

		if (copy_from_user(&send_only_cfg, (void __user *)arg, sizeof(send_only_cfg)))
			return -EINVAL;
		if (copy_from_user(&stMsg, (void __user *)send_only_cfg.pstRequest, sizeof(stMsg)))
			return -EINVAL;
		ret = ipcmsg_send_only(send_only_cfg.s32Id, &stMsg);
		break;
	}

	case IPCM_IOC_SEND_ASYNC:
	{
		struct ipcm_send_cfg send_cfg;
		CVI_IPCMSG_MESSAGE_S stMsg;

		if (copy_from_user(&send_cfg, (void __user *)arg, sizeof(send_cfg)))
			return -EINVAL;
		if (copy_from_user(&stMsg, (void __user *)send_cfg.pstRequest, sizeof(stMsg)))
			return -EINVAL;
		ret = ipcmsg_send_async(send_cfg.s32Id, &stMsg);
		break;
	}

	case IPCM_IOC_SEND_SYNC:
	{
		struct ipcm_send_cfg send_cfg;
		CVI_IPCMSG_MESSAGE_S stMsg;
		CVI_IPCMSG_MESSAGE_S *pstResq;

		if (copy_from_user(&send_cfg, (void __user *)arg, sizeof(send_cfg)))
			return -EINVAL;
		if (copy_from_user(&stMsg, (void __user *)send_cfg.pstRequest, sizeof(stMsg)))
			return -EINVAL;
		ret = ipcmsg_send_sync(send_cfg.s32Id, &stMsg,
			&pstResq, send_cfg.s32TimeoutMs);
		if (!ret) {
			if (copy_to_user((void __user *)send_cfg.pstResq, pstResq, sizeof(CVI_IPCMSG_MESSAGE_S)))
				return -EINVAL;
			if ((pstResq->u32BodyLen > 0) &&
				copy_to_user((void __user *)send_cfg.pRespBody, pstResq->pBody, pstResq->u32BodyLen))
				return -EINVAL;
			kfree(pstResq);
		}
		break;
	}

	case IPCM_IOC_RUN:
	{
		CVI_S32 s32Id;

		if (copy_from_user(&s32Id, (void __user *)arg, sizeof(s32Id)))
			return -EINVAL;
		ipcmsg_run(s32Id);
		ret = 0;
		break;
	}

	case IPCM_IOC_GET_ID:
	{
		CVI_S32 s32Id = atomic_fetch_add(1, &s_msg_id);

		if (copy_to_user((void __user *)arg, &s32Id, sizeof(s32Id)))
			return -EINVAL;
		ret = 0;
		break;
	}

	case IPCM_IOC_SIG_DATA:
	{
		struct ipcm_signal_cfg sig_cfg;

		if (copy_from_user(&sig_cfg, (void __user *)arg, sizeof(sig_cfg)))
			return -EINVAL;

		sig_cfg.s32Id = stSigData.s32Id;
		if (copy_to_user((void __user *)arg, &sig_cfg, sizeof(sig_cfg)))
			return -EINVAL;

		if (copy_to_user((void __user *)sig_cfg.pstMsg, stSigData.pstMsg, sizeof(CVI_IPCMSG_MESSAGE_S)))
			return -EINVAL;

		if ((stSigData.pstMsg->u32BodyLen > 0) &&
			copy_to_user((void __user *)sig_cfg.pBody,
			stSigData.pstMsg->pBody, stSigData.pstMsg->u32BodyLen))
			return -EINVAL;
		ret = 0;
		break;
	}

	case IPCM_IOC_GET_USER_CNT:
	{
		CVI_S32 s32Id;

		if (copy_from_user(&s32Id, (void __user *)arg, sizeof(s32Id)))
			return -EINVAL;

		ret = ipcmsg_inquireUserCnt(s32Id);
		break;
	}

	case IPCM_IOC_LOCK:
	{
		int id = arg % IPCM_DATA_SPIN_MAX;

		mutex_lock(&data_lock[id]);
		ret = ipcm_data_spin_lock(id);
		if (ret)
			mutex_unlock(&data_lock[id]);
		break;
	}

	case IPCM_IOC_UNLOCK:
	{
		int id = arg % IPCM_DATA_SPIN_MAX;

		ret = ipcm_data_spin_unlock(id);
		mutex_unlock(&data_lock[id]);
		break;
	}

	default:
		ret = -1;
		ipcm_err("cmd(%d) not support.\n", cmd);
		break;
	}

	return ret;
}

static const struct file_operations dev_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.unlocked_ioctl = dev_ioctl,
	.fasync = dev_fasync,
};

static struct miscdevice ipcmsg_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_fops,
	.name   = "ipcmsg"
};

int ipcmsg_register_dev(void)
{
	int rc, i;

	rc = misc_register(&ipcmsg_dev);
	if (rc) {
		ipcm_err("ipcmsg: failed to register misc device.\n");
		return rc;
	}
	ipcmsg_proc_init();

	for (i = 0; i < IPCM_DATA_SPIN_MAX; i++)
		mutex_init(&data_lock[i]);

	return 0;
}

void ipcmsg_deregister_dev(void)
{
	ipcmsg_proc_remove();

	misc_deregister(&ipcmsg_dev);
}


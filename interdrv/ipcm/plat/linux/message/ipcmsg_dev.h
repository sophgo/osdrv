#ifndef __IPCMSG_DEV__
#define __IPCMSG_DEV__

#include "linux/ipcm_linux.h"

#define MAX_SIG_MSG_NUM 8

struct ipcm_signal_ctl {
	struct ipcm_signal_cfg stSigData[MAX_SIG_MSG_NUM];
	atomic_t atUsed[MAX_SIG_MSG_NUM];
};

int ipcmsg_register_dev(void);
void ipcmsg_deregister_dev(void);


#endif

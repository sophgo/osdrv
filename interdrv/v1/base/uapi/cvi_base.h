/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_base.h
 * Description:
 */


#ifndef _U_CVI_BASE_H_
#define _U_CVI_BASE_H_

struct base_statesignal32 {
	unsigned int signr;
	unsigned int context;
};

struct base_statesignal {
	unsigned int signr;
	void *context;
};

enum base_state_e {
	BASE_STATE_NORMAL = 0,		/* init state or after user space resume is complete. */
	BASE_STATE_SUSPEND_PREPARE,	/* enter when suspend prepare is notified. */
	BASE_STATE_SUSPEND,		/* enter after user space suspend is done. */
	BASE_STATE_RESUME,		/* enter when suspend post is notified. */
	BASE_STATE_NUM
};

#define IOCTL_BASE_BASE		's'
#define IOCTL_READ_CHIP_ID	_IOR(IOCTL_BASE_BASE, 1, unsigned int)
#define IOCTL_READ_CHIP_VERSION	_IOR(IOCTL_BASE_BASE, 2, unsigned int)
#define IOCTL_STATESIG		_IOW(IOCTL_BASE_BASE, 3, struct base_statesignal)
#define IOCTL_STATESIG32	_IOW(IOCTL_BASE_BASE, 3, struct base_statesignal32)
#define IOCTL_READ_STATE	_IOR(IOCTL_BASE_BASE, 4, unsigned int)
#define IOCTL_USER_SUSPEND_DONE	_IOR(IOCTL_BASE_BASE, 5, unsigned int)
#define IOCTL_USER_RESUME_DONE	_IOR(IOCTL_BASE_BASE, 6, unsigned int)
#define IOCTL_READ_CHIP_PWR_ON_REASON	_IOR(IOCTL_BASE_BASE, 7, unsigned int)
#define IOCTL_GET_VB_POOLS_MAX_CNT	_IOR(IOCTL_BASE_BASE, 8, unsigned int)
#define IOCTL_GET_VB_BLK_MAX_CNT	_IOR(IOCTL_BASE_BASE, 9, unsigned int)

/* chip ID list */
enum ENUM_CHIP_ID {
	E_CHIPID_CV1822 = 0,	//0
	E_CHIPID_CV1832,	//1
	E_CHIPID_CV1835,	//2
	E_CHIPID_CV1838,	//3
	E_CHIPID_CV1829,
	E_CHIPID_CV1826,
	E_CHIPID_CV1821,
	E_CHIPID_CV1820,
	E_CHIPID_CV1823,
	E_CHIPID_CV1825
};

/* chip version list */
enum ENUM_CHIP_VERSION {
	E_CHIPVERSION_U01 = 1,	//1
	E_CHIPVERSION_U02,	//2
	E_CHIPVERSION_U03,
};

/* chip power on reason list */
enum ENUM_CHIP_PWR_ON_REASON {
	E_CHIP_PWR_ON_COLDBOOT = 1,	//1
	E_CHIP_PWR_ON_WDT,	//2
	E_CHIP_PWR_ON_SUSPEND,	//3
	E_CHIP_PWR_ON_WARM_RST,	//4
};
#endif // _U_CVI_BASE_H_

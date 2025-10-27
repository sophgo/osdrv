/* SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef __SOPHON_SPACC_H__
#define __SOPHON_SPACC_H__
#define OPTEE_SMC_CALL_CV_BASE64 0x0300000D
#define OPTEE_SMC_CALL_CV_SHA256 0x0300000E
#define OPTEE_SMC_CALL_CV_AES 0x0300000F
#define OPTEE_SMC_CALL_CV_SM4 0x03000010
#define OPTEE_SMC_CALL_CV_DES 0x03000011
#define OPTEE_SMC_CALL_CV_TDES 0x03000012
#define OPTEE_SMC_CALL_CV_SM3 0x03000013
typedef enum SPACC_ALGO {
	SPACC_ALGO_AES,
	SPACC_ALGO_DES,
	SPACC_ALGO_TDES,
	SPACC_ALGO_SM4,
	SPACC_ALGO_SHA1,
	SPACC_ALGO_SHA256,
	SPACC_ALGO_BASE64,
} SPACC_ALGO_E;

typedef enum SPACC_ALGO_MODE {
	SPACC_ALGO_MODE_ECB,
	SPACC_ALGO_MODE_CBC, //cv180x Not Supported
	SPACC_ALGO_MODE_CTR, //cv180x Not Supported
	SPACC_ALGO_MODE_OFB, //cv180x Not Supported
} SPACC_ALGO_MODE_E;

typedef enum SPACC_KEY_SIZE {
	SPACC_KEY_SIZE_64BITS,
	SPACC_KEY_SIZE_128BITS,
	SPACC_KEY_SIZE_192BITS,
	SPACC_KEY_SIZE_256BITS,
} SPACC_KEY_SIZE_E;

typedef enum SPACC_ACTION {
	SPACC_ACTION_ENCRYPTION,
	SPACC_ACTION_DECRYPT,
} SPACC_ACTION_E;

typedef enum SPACC_KEY_SOURCE {
	SPACC_KEY_SOURCE_DESCRIPTOR,
	SPACC_KEY_SOURCE_OTP, // key from otp or efuse
} SPACC_KEY_SOURCE_E;

typedef struct cvi_spacc_base64 {
	uint32_t customer_code;
	uint32_t action; // 0: Decode, 1: Encode
};

struct cvi_spacc_base64_inner {
	uint64_t src;
	uint64_t dst;
	uint64_t len;
	uint32_t customer_code;
	uint32_t action; // 0: Decode, 1: Encode
};

typedef struct spacc_aes_config {
	// data config
	void *src; //src phy address
	size_t len;

	// spacc config
	uintptr_t key;
	uintptr_t iv;
	SPACC_ALGO_MODE_E mode;
	SPACC_KEY_SIZE_E key_mode;
	SPACC_ACTION_E action;
	SPACC_KEY_SOURCE_E otp;
} spacc_aes_config_s;

typedef struct spacc_des_config {
	uintptr_t key;
	uintptr_t iv;
	SPACC_ALGO_MODE_E mode;
	SPACC_ACTION_E action;
} spacc_des_config_s;

typedef spacc_aes_config_s spacc_sm4_config_s;
typedef spacc_des_config_s spacc_tdes_config_s;

#define IOCTL_SPACC_BASE 'S'
#define IOCTL_SPACC_CREATE_MEMPOOL _IOW(IOCTL_SPACC_BASE, 1, unsigned int)
#define IOCTL_SPACC_GET_MEMPOOL_SIZE _IOR(IOCTL_SPACC_BASE, 2, unsigned int)

#define IOCTL_SPACC_BASE64 _IOW(IOCTL_SPACC_BASE, 5, struct cvi_spacc_base64)
#define IOCTL_SPACC_BASE64_INNER                                               \
	_IOW(IOCTL_SPACC_BASE, 6, struct cvi_spacc_base64_inner)
#define IOCTL_SPACC_SM3_ACTION _IO(IOCTL_SPACC_BASE, 7)
#define IOCTL_SPACC_AES_ACTION _IOW(IOCTL_SPACC_BASE, 8, spacc_aes_config_s)
#define IOCTL_SPACC_SM4_ACTION _IOW(IOCTL_SPACC_BASE, 9, spacc_sm4_config_s)
#define IOCTL_SPACC_DES_ACTION _IOW(IOCTL_SPACC_BASE, 10, spacc_des_config_s)
#define IOCTL_SPACC_TDES_ACTION _IOW(IOCTL_SPACC_BASE, 11, spacc_tdes_config_s)
#define IOCTL_SPACC_SHA256_ACTION _IO(IOCTL_SPACC_BASE, 12)



#endif

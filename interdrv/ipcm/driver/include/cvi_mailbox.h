/**
 * @brief Mailbox interface for inter-processor communication
 *
 * Provides hardware mailbox access and control for message passing between processors
 */

#ifndef __CVI_MAILBOX_H__
#define __CVI_MAILBOX_H__

#include "ipcm_plat_adapter.h"
#include "ipcm.h"

/**
 * @brief Hardware interrupt and register definitions
 */
#define MBOX_INT_C906_2ND 61
// #define MBOX_INT_C906_TOP 101
// #define MBOX_INT_C906_TOP 30

#define MAILBOX_REG_BASE                        0x01900000
#define MAILBOX_REG_BUFF                        (MAILBOX_REG_BASE + 0x0400)
#define SPINLOCK_REG_BASE                       (MAILBOX_REG_BASE + 0x00c0)

/**
 * @brief Mailbox register unions for hardware access
 */
union cpu_mailbox_info_offset {
	char mbox_info;
	int reserved;
};

union cpu_mailbox_int_clr_offset {
	char mbox_int_clr;
	int reserved;
};
union cpu_mailbox_int_mask_offset {
	char mbox_int_mask;
	int reserved;
};
union cpu_mailbox_int_offset {
	char mbox_int;
	int reserved;
};
union cpu_mailbox_int_raw_offset {
	char mbox_int_raw;
	int reserved;
};

union mailbox_set {
	char mbox_set;
	int reserved;
};
union mailbox_status {
	char mbox_status;
	int reserved;
};

union cpu_mailbox_status {
	char mbox_status;
	int reserved;
};

/* register mapping refers to mailbox user guide*/
struct cpu_mbox_int {
	union cpu_mailbox_int_clr_offset  cpu_mbox_int_clr;   // Interrupt clear
	union cpu_mailbox_int_mask_offset cpu_mbox_int_mask;  // Interrupt mask
	union cpu_mailbox_int_offset      cpu_mbox_int_int;   // Interrupt status
	union cpu_mailbox_int_raw_offset  cpu_mbox_int_raw;   // Raw interrupt status
};

/**
 * @brief Mailbox set register structure
 */
struct mailbox_set_register {
	union  cpu_mailbox_info_offset cpu_mbox_en[4];      //0x00, 0x04, 0x08, 0x0c
	struct cpu_mbox_int cpu_mbox_set[4];                //0x10~0x1C, 0x20~0x2C, 0x30~0x3C, 0x40~0x4C
	int    reserved[4];                                 //0x50~0x5C
	union  mailbox_set mbox_set;                        //0x60
	union  mailbox_status mbox_status;                  //0x64
	int    reserved2[2];                                //0x68~0x6C
	union  cpu_mailbox_status cpu_mbox_status[4];       //0x70
};

struct mailbox_done_register {
	union  cpu_mailbox_info_offset cpu_mbox_done_en[4];
	struct cpu_mbox_int cpu_mbox_done[4];
};

// volatile struct mailbox_set_register *mbox_reg;
// volatile struct mailbox_done_register *mbox_done_reg;
// volatile unsigned long *mailbox_context; // mailbox buffer context is 64 Bytess

#define MAILBOX_MAX_NUM         0x0008
#define MAILBOX_DONE_OFFSET     0x0002
#define MAILBOX_CONTEXT_OFFSET  0x0400

/**
 * @brief Command queue structure (8-byte aligned)
 */
typedef struct cmdqu_t cmdqu_t;
/* cmdqu size should be 8 bytes because of mailbox buffer size */
struct cmdqu_t {
	unsigned char ip_id;           // IP identifier
	unsigned char cmd_id : 7;      // Command ID
	unsigned char block : 1;       // Blocking flag
	union resv_t resv;            // Reserved data
	unsigned int  param_ptr;       // Parameter pointer
} __attribute__((packed)) __attribute__((aligned(0x8)));

/**
 * @brief Mailbox message handler function type
 */
typedef s32 (*mailbox_handle)(u8 port_id, void *msg, void *data);

// Function declarations
s32 mailbox_send(MsgData *msg);

s32 mailbox_init(mailbox_handle handle, void *data);

s32 mailbox_uninit(void);

s32 mailbox_get_invalid_cnt(void);

s32 mailbox_set_snd_cpu(int cpu_id);

#endif // end of__CVI_MAILBOX_H__


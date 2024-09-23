
#ifndef __MAILBOX__
#define __MAILBOX__

#include "cvi_mailbox.h"
#include "ipcm.h"

#define MBOX_INT_C906_2ND 61
// #define MBOX_INT_C906_TOP 101
// #define MBOX_INT_C906_TOP 30

#define MAILBOX_REG_BASE                        0x01900000
#define MAILBOX_REG_BUFF                        (MAILBOX_REG_BASE + 0x0400)
#define SPINLOCK_REG_BASE                       (MAILBOX_REG_BASE + 0x00c0)


typedef struct cmdqu_t cmdqu_t;
/* cmdqu size should be 8 bytes because of mailbox buffer size */
struct cmdqu_t {
	unsigned char ip_id;
	unsigned char cmd_id : 7;
	unsigned char block : 1;
	union resv_t resv;
	unsigned int  param_ptr;
} __attribute__((packed)) __attribute__((aligned(0x8)));

typedef s32 (*mailbox_handle)(u8 port_id, void *msg, void *data);

s32 mailbox_send(MsgData *msg);

s32 mailbox_init(mailbox_handle handle, void *data);

s32 mailbox_uninit(void);

s32 mailbox_get_invalid_cnt(void);

s32 mailbox_set_snd_cpu(int cpu_id);

#endif

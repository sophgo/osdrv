#ifndef _VI_DBG_PROC_H_
#define _VI_DBG_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>

#include "vi_defines.h"

int vi_dbg_proc_init(struct vi_dev *_vdev);
int vi_dbg_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif // _VI_DBG_PROC_H_


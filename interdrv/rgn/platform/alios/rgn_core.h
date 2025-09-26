#ifndef __RGN_CORE_H__
#define __RGN_CORE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal.h"
#include "rgn_debug.h"
#include "base_cb.h"
#include "rgn_uapi.h"

/**
 * struct cvi_rgn - RGN IP abstraction
 */
struct rgn_dev {
	osal_spinlock		lock;
	osal_spinlock		rdy_lock;
	osal_mutex		mutex;
	bool			bind_fb;
};

int rgn_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg);
long _rgn_s_ctrl(struct rgn_dev *rdev, struct rgn_ext_control *p);
long _rgn_g_ctrl(struct rgn_dev *rdev, struct rgn_ext_control *p);
int _rgn_sw_init(struct rgn_dev *rdev);
int _rgn_release_op(struct rgn_dev *rdev);
int _rgn_release_all_region(void);

#ifdef __cplusplus
}
#endif

#endif /* __RGN_CORE_H__ */

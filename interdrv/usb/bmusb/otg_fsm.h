#ifndef __BM_OTG_FSM_H
#define __BM_OTG_FSM_H

#include <linux/workqueue.h>

void bmusb_usb_otg_work(struct work_struct *work);
void otg_hnp_polling_work(struct work_struct *work);
void bmusb_set_state(struct otg_fsm *fsm, enum usb_otg_state new_state);

#endif /* __BM_OTG_FSM_H */

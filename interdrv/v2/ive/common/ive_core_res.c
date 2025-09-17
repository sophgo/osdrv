#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include "ive_core_res.h"
#include "ive_interface.h"

extern atomic_t dev_state[IVE_DEV_MAX];

typedef struct ive_drv_core_list_t {
    int id;
    bool is_used;
    struct list_head list;
} ive_drv_core_list_t;

static unsigned int s_max_num_core = 1;

static DEFINE_SPINLOCK(ive_spinlock);

static DEFINE_MUTEX(ive_drv_core_list_lock);

static DECLARE_WAIT_QUEUE_HEAD(ive_drv_core_wait_queue);
static LIST_HEAD(ive_drv_core_resource_list_head);
static int next_id = 1;
static bool isCoreIdle = false;
int ive_core_request_resource(int timeout) {
    ive_drv_core_list_t *res;
    unsigned long flags;
    int id = -1;
    unsigned long long elapse, cur;
    struct timespec64 ts;

    ktime_get_ts64(&ts);
    elapse = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

    while (id == -1) {
        spin_lock_irqsave(&ive_spinlock, flags);
        list_for_each_entry(res, &ive_drv_core_resource_list_head, list) {
            if (!res->is_used) {
                res->is_used = true;
                id = res->id;
                atomic_set(&dev_state[id], IVE_DEV_STATE_RUNNING);
                break;
            }
        }
        isCoreIdle = false;
        spin_unlock_irqrestore(&ive_spinlock, flags);

        if (id == -1) {
            if (timeout > 0 )
                wait_event_idle_exclusive_timeout(ive_drv_core_wait_queue, isCoreIdle, msecs_to_jiffies(timeout));
            else
                wait_event(ive_drv_core_wait_queue, isCoreIdle);
        }

        ktime_get_ts64(&ts);
        cur = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

        if (timeout > 0 && (cur - elapse) > timeout) {
            return -1;
        }
    }

    return id;
}

int ive_core_release_resource(int id) {
    ive_drv_core_list_t *res;
    unsigned long        flags;
    int ret = -1;

    spin_lock_irqsave(&ive_spinlock, flags);
    list_for_each_entry(res, &ive_drv_core_resource_list_head, list) {
        if (res->id == id) {
            res->is_used = false;
            ret = 0;
            isCoreIdle = true;
            atomic_set(&dev_state[id], IVE_DEV_STATE_END);
            wake_up(&ive_drv_core_wait_queue);
            break;
        }
    }
    spin_unlock_irqrestore(&ive_spinlock, flags);

    return ret;
}

int ive_core_init_resources(unsigned int core_num) {
    ive_drv_core_list_t *res;
    int i;

    s_max_num_core = core_num;
    for (i = 0; i < s_max_num_core; i++) {
        res = kzalloc(sizeof(*res), GFP_KERNEL);
        if (!res) {
            printk(KERN_ERR "Failed to allocate memory for resource\n");
            return -ENOMEM;
        }

        res->id = next_id--;
        res->is_used = false;
        INIT_LIST_HEAD(&res->list);

        mutex_lock(&ive_drv_core_list_lock);
        list_add_tail(&res->list, &ive_drv_core_resource_list_head);
        mutex_unlock(&ive_drv_core_list_lock);
    }

    return 0;
}

void ive_core_cleanup_resources(void) {
    ive_drv_core_list_t *res, *tmp;

    mutex_lock(&ive_drv_core_list_lock);
    list_for_each_entry_safe(res, tmp, &ive_drv_core_resource_list_head, list) {
        list_del(&res->list);
        kfree(res);
    }
    mutex_unlock(&ive_drv_core_list_lock);
}
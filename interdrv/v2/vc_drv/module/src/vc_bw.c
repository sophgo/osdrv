#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include "vc_drv_proc.h"

#ifdef MEDIA_V3

/* vcsys BWC (DDR bandwidth counters), X2 cv84x6 only.
 * vesys_top 0x22000000: VE + JPG0/1/2 r/w counters @0xB0-0xCC
 * vdsys_top 0x23000000+ n*0x100000, n=0..3: VD r/w counters @0xB0-0xC0
 * (VPSS0/1/2 counters @0xC4-0xDC belong to the vpss driver's proc).
 * Counters are free-running once enabled (0x08 bit31, 0->1 clears and
 * starts, 1 keeps counting); software windows virtualize the reset. */
#define VCBW_MAX_CNT        8
#define VCBW_VDSYS_NUM      4
#define VCBW_SYS_NUM        (1 + VCBW_VDSYS_NUM)
#define VCBW_CNT_UNIT       16      /* bytes per counter tick */
#define VCBW_WINDOW_MS      100

#define VCBW_VESYS_BASE     0x22000000UL
#define VCBW_VDSYS_BASE(n)  (0x23000000UL + (n) * 0x100000UL)
#define VCBW_REG_CTRL       0x08
#define VCBW_BWC_EN         (1U << 31)

struct vcbw_sys {
    void __iomem *regs;
    const u32 *offs;
    const char *const *names;
    int cnt;
    u32 last[VCBW_MAX_CNT];
    u64 total_bytes[VCBW_MAX_CNT];
    u32 cur_dmbps[VCBW_MAX_CNT];    /* 0.1 MB/s over the last window */
    u64 total_ms;
};

static const u32 vesys_offs[] = {0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC4, 0xC8, 0xCC};
static const char *const vesys_names[] = {
    "r_ve", "w_ve", "r_jpg0", "w_jpg0", "r_jpg1", "w_jpg1", "r_jpg2", "w_jpg2"
};
static const u32 vdsys_offs[] = {0xB0, 0xB4, 0xB8, 0xBC, 0xC0};
static const char *const vdsys_names[] = {
    "r_vd_m0", "r_vd_m1", "w_vd_m0", "w_vd_m1", "w_vd_m2"
};

static struct vcbw_sys vcbw_sys_tab[VCBW_SYS_NUM];
static DEFINE_SPINLOCK(vcbw_lock);
static struct delayed_work vcbw_work;
static bool vcbw_inited;

/* bytes accumulated over ms -> 0.1 MB/s */
static u32 vcbw_dmbps(u64 bytes, u64 ms)
{
    if (!ms)
        return 0;
    return (u32)div64_u64(bytes * 10, ms * 1000);
}

static void vcbw_sample_sys(struct vcbw_sys *s)
{
    int i;

    for (i = 0; i < s->cnt; i++) {
        u32 now = readl(s->regs + s->offs[i]);
        u32 delta = now - s->last[i];

        s->last[i] = now;
        s->total_bytes[i] += (u64)delta * VCBW_CNT_UNIT;
        s->cur_dmbps[i] = vcbw_dmbps((u64)delta * VCBW_CNT_UNIT, VCBW_WINDOW_MS);
    }
    s->total_ms += VCBW_WINDOW_MS;
}

static void vcbw_work_fn(struct work_struct *work)
{
    int i;

    spin_lock(&vcbw_lock);
    for (i = 0; i < VCBW_SYS_NUM; i++)
        vcbw_sample_sys(&vcbw_sys_tab[i]);
    spin_unlock(&vcbw_lock);

    schedule_delayed_work(&vcbw_work, msecs_to_jiffies(VCBW_WINDOW_MS));
}

static int vcbw_proc_show(struct seq_file *m, void *v)
{
    struct vcbw_sys snap[VCBW_SYS_NUM];
    unsigned long flags;
    int i, j;

    spin_lock_irqsave(&vcbw_lock, flags);
    memcpy(snap, vcbw_sys_tab, sizeof(snap));
    spin_unlock_irqrestore(&vcbw_lock, flags);

    seq_puts(m, "unit: 0.1MB/s, cur: last 100ms window, avg: since insmod\n");
    for (i = 0; i < VCBW_SYS_NUM; i++) {
        seq_printf(m, "[%s]", i == 0 ? "vesys" : "vdsys");
        for (j = 0; j < snap[i].cnt; j++)
            seq_printf(m, " %s avg=%u cur=%u", snap[i].names[j],
                       vcbw_dmbps(snap[i].total_bytes[j], snap[i].total_ms),
                       snap[i].cur_dmbps[j]);
        seq_putc(m, '\n');
    }
    return 0;
}

static int vcbw_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcbw_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
static const struct proc_ops vcbw_proc_fops = {
    .proc_open = vcbw_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations vcbw_proc_fops = {
    .open = vcbw_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int vc_bw_init(struct device *dev)
{
    struct vcbw_sys *s;
    int i, j;
    u32 ctrl;

    s = &vcbw_sys_tab[0];
    s->regs = ioremap(VCBW_VESYS_BASE, 0x1000);
    s->offs = vesys_offs;
    s->names = vesys_names;
    s->cnt = ARRAY_SIZE(vesys_offs);
    if (!s->regs)
        goto err_map;

    for (i = 0; i < VCBW_VDSYS_NUM; i++) {
        s = &vcbw_sys_tab[1 + i];
        s->regs = ioremap(VCBW_VDSYS_BASE(i), 0x1000);
        s->offs = vdsys_offs;
        s->names = vdsys_names;
        s->cnt = ARRAY_SIZE(vdsys_offs);
        if (!s->regs)
            goto err_map;
    }

    for (i = 0; i < VCBW_SYS_NUM; i++) {
        s = &vcbw_sys_tab[i];
        ctrl = readl(s->regs + VCBW_REG_CTRL);
        writel(ctrl | VCBW_BWC_EN, s->regs + VCBW_REG_CTRL);
        for (j = 0; j < s->cnt; j++)
            s->last[j] = readl(s->regs + s->offs[j]);
    }

    INIT_DEFERRABLE_WORK(&vcbw_work, vcbw_work_fn);
    schedule_delayed_work(&vcbw_work, msecs_to_jiffies(VCBW_WINDOW_MS));

    if (proc_create(VCBW_PROC_NAME, VIDEO_PROC_PERMS, NULL,
                    &vcbw_proc_fops) == NULL) {
        pr_err("vc_bw proc create failed\n");
        cancel_delayed_work_sync(&vcbw_work);
        goto err_map;
    }
    vcbw_inited = true;
    return 0;

err_map:
    for (i = 0; i < VCBW_SYS_NUM; i++)
        if (vcbw_sys_tab[i].regs) {
            iounmap(vcbw_sys_tab[i].regs);
            vcbw_sys_tab[i].regs = NULL;
        }
    return -ENOMEM;
}

void vc_bw_exit(void)
{
    int i;

    if (!vcbw_inited)
        return;
    cancel_delayed_work_sync(&vcbw_work);
    remove_proc_entry(VCBW_PROC_NAME, NULL);
    for (i = 0; i < VCBW_SYS_NUM; i++)
        if (vcbw_sys_tab[i].regs) {
            iounmap(vcbw_sys_tab[i].regs);
            vcbw_sys_tab[i].regs = NULL;
        }
    vcbw_inited = false;
}

#else /* !MEDIA_V3 */

int vc_bw_init(struct device *dev)
{
    return 0;
}

void vc_bw_exit(void)
{
}

#endif /* MEDIA_V3 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#include "scaler.h"
#include "scaler_reg.h"

#ifdef MEDIA_V3

/* vdsys BWC (DDR bandwidth counters) for the VPSS masters, X2 cv84x6 only.
 * vdsys_top 0x23000000+ n*0x100000, n=0..3, each has VPSS0/1/2 read/write
 * counters @0xC4-0xDC (unit 8 bytes, unlike the VD counters' 16).
 * 0x08 bit31 (reg_bwc_en) is shared with the VD counters exposed by the
 * vc_drv "vcbw" proc: both sides set it via read-modify-write and never
 * clear it, so whichever driver loads first or last keeps counting running
 * (0->1 edge clears and starts, 1->1 is a no-op). Counters free-run;
 * software windows virtualize the reset. */
#define VPSSBW_SYS_NUM      4
#define VPSSBW_CNT_PER_SYS  6
#define VPSSBW_CNT_UNIT     8       /* bytes per counter tick */
#define VPSSBW_WINDOW_MS    100

#define VPSSBW_REG_CTRL     0x08
#define VPSSBW_BWC_EN       (1U << 31)
#define VPSSBW_PROC_NAME    "vpssbw"

static const u32 vpssbw_offs[VPSSBW_CNT_PER_SYS] = {
    0xC4, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC
};
static const char *const vpssbw_names[VPSSBW_CNT_PER_SYS] = {
    "r_vpss0", "w_vpss0", "r_vpss1", "w_vpss1", "r_vpss2", "w_vpss2"
};

struct vpssbw_sys {
    void __iomem *regs;
    u32 last[VPSSBW_CNT_PER_SYS];
    u64 total_bytes[VPSSBW_CNT_PER_SYS];
    u32 cur_dmbps[VPSSBW_CNT_PER_SYS]; /* 0.1 MB/s over the last window */
    u64 total_ms;
};

static struct vpssbw_sys vpssbw_sys_tab[VPSSBW_SYS_NUM];
static DEFINE_SPINLOCK(vpssbw_lock);
static struct delayed_work vpssbw_work;
static bool vpssbw_inited;

/* bytes accumulated over ms -> 0.1 MB/s */
static u32 vpssbw_dmbps(u64 bytes, u64 ms)
{
    if (!ms)
        return 0;
    return (u32)div64_u64(bytes * 10, ms * 1000);
}

static void vpssbw_sample_sys(struct vpssbw_sys *s)
{
    int i;

    for (i = 0; i < VPSSBW_CNT_PER_SYS; i++) {
        u32 now = readl(s->regs + vpssbw_offs[i]);
        u32 delta = now - s->last[i];

        s->last[i] = now;
        s->total_bytes[i] += (u64)delta * VPSSBW_CNT_UNIT;
        s->cur_dmbps[i] =
            vpssbw_dmbps((u64)delta * VPSSBW_CNT_UNIT, VPSSBW_WINDOW_MS);
    }
    s->total_ms += VPSSBW_WINDOW_MS;
}

static void vpssbw_work_fn(struct work_struct *work)
{
    int i;

    spin_lock(&vpssbw_lock);
    for (i = 0; i < VPSSBW_SYS_NUM; i++)
        vpssbw_sample_sys(&vpssbw_sys_tab[i]);
    spin_unlock(&vpssbw_lock);

    schedule_delayed_work(&vpssbw_work, msecs_to_jiffies(VPSSBW_WINDOW_MS));
}

static int vpssbw_proc_show(struct seq_file *m, void *v)
{
    struct vpssbw_sys snap[VPSSBW_SYS_NUM];
    u64 tot_r = 0, tot_w = 0, tot_r_cur = 0, tot_w_cur = 0;
    unsigned long flags;
    int i, j;

    spin_lock_irqsave(&vpssbw_lock, flags);
    memcpy(snap, vpssbw_sys_tab, sizeof(snap));
    spin_unlock_irqrestore(&vpssbw_lock, flags);

    for (i = 0; i < VPSSBW_SYS_NUM; i++)
        for (j = 0; j < VPSSBW_CNT_PER_SYS; j += 2) {
            tot_r += snap[i].total_bytes[j];
            tot_w += snap[i].total_bytes[j + 1];
            tot_r_cur += snap[i].cur_dmbps[j];
            tot_w_cur += snap[i].cur_dmbps[j + 1];
        }

    seq_puts(m, "unit: 0.1MB/s, cur: last 100ms window, avg: since insmod\n");
    seq_printf(m, "[vpss_total] r avg=%u cur=%llu | w avg=%u cur=%llu\n",
               vpssbw_dmbps(tot_r, snap[0].total_ms), tot_r_cur,
               vpssbw_dmbps(tot_w, snap[0].total_ms), tot_w_cur);
    for (i = 0; i < VPSSBW_SYS_NUM; i++) {
        seq_printf(m, "[vdsys%d]", i);
        for (j = 0; j < VPSSBW_CNT_PER_SYS; j++)
            seq_printf(m, " %s avg=%u cur=%u", vpssbw_names[j],
                       vpssbw_dmbps(snap[i].total_bytes[j], snap[i].total_ms),
                       snap[i].cur_dmbps[j]);
        seq_putc(m, '\n');
    }
    return 0;
}

static int vpssbw_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, vpssbw_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
static const struct proc_ops vpssbw_proc_fops = {
    .proc_open = vpssbw_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations vpssbw_proc_fops = {
    .open = vpssbw_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int vpss_bw_init(void)
{
    void __iomem *regs;
    int i, j;
    u32 ctrl;

    for (i = 0; i < VPSSBW_SYS_NUM; i++) {
        regs = ioremap(REG_VD0_STS_BASE + i * 0x100000UL, 0x1000);
        if (!regs)
            goto err_map;
        vpssbw_sys_tab[i].regs = regs;
    }

    for (i = 0; i < VPSSBW_SYS_NUM; i++) {
        ctrl = readl(vpssbw_sys_tab[i].regs + VPSSBW_REG_CTRL);
        writel(ctrl | VPSSBW_BWC_EN,
               vpssbw_sys_tab[i].regs + VPSSBW_REG_CTRL);
        for (j = 0; j < VPSSBW_CNT_PER_SYS; j++)
            vpssbw_sys_tab[i].last[j] =
                readl(vpssbw_sys_tab[i].regs + vpssbw_offs[j]);
    }

    INIT_DEFERRABLE_WORK(&vpssbw_work, vpssbw_work_fn);
    schedule_delayed_work(&vpssbw_work, msecs_to_jiffies(VPSSBW_WINDOW_MS));

    if (proc_create(VPSSBW_PROC_NAME, 0644, NULL,
                    &vpssbw_proc_fops) == NULL) {
        pr_err("vpssbw proc create failed\n");
        cancel_delayed_work_sync(&vpssbw_work);
        goto err_map;
    }
    vpssbw_inited = true;
    return 0;

err_map:
    for (i = 0; i < VPSSBW_SYS_NUM; i++)
        if (vpssbw_sys_tab[i].regs) {
            iounmap(vpssbw_sys_tab[i].regs);
            vpssbw_sys_tab[i].regs = NULL;
        }
    return -ENOMEM;
}

void vpss_bw_exit(void)
{
    int i;

    if (!vpssbw_inited)
        return;
    cancel_delayed_work_sync(&vpssbw_work);
    remove_proc_entry(VPSSBW_PROC_NAME, NULL);
    for (i = 0; i < VPSSBW_SYS_NUM; i++)
        if (vpssbw_sys_tab[i].regs) {
            iounmap(vpssbw_sys_tab[i].regs);
            vpssbw_sys_tab[i].regs = NULL;
        }
    vpssbw_inited = false;
}

#else /* !MEDIA_V3 */

int vpss_bw_init(void)
{
    return 0;
}

void vpss_bw_exit(void)
{
}

#endif /* MEDIA_V3 */

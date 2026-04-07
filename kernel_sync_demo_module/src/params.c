// src/params.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/errno.h>

#include "ctx.h"
#include "sync.h"

/* lock_type (read/write) */
static int lock_type_set(const char *val, const struct kernel_param *kp)
{
    unsigned int v;

    if (kstrtouint(val, 10, &v))
        return -EINVAL;
    if (v > 2)
        return -EINVAL;
    if (atomic_read(&g_ctx.running))
        return -EBUSY;

    g_ctx.lock_type = v;
    return 0;
}

static int lock_type_get(char *buf, const struct kernel_param *kp)
{
    return scnprintf(buf, PAGE_SIZE, "%u\n", g_ctx.lock_type);
}

static const struct kernel_param_ops lock_type_ops = {
    .set = lock_type_set,
    .get = lock_type_get,
};

module_param_cb(lock_type, &lock_type_ops, NULL, 0644);
MODULE_PARM_DESC(lock_type, "0 spinlock, 1 mutex, 2 semaphore");

/* run (write) */
static int run_set(const char *val, const struct kernel_param *kp)
{
    unsigned int v;

    if (kstrtouint(val, 10, &v))
        return -EINVAL;
    if (v == 0)
        return 0;

    return workers_run(&g_ctx);
}

static const struct kernel_param_ops run_ops = {
    .set = run_set,
};

module_param_cb(run, &run_ops, NULL, 0220);
MODULE_PARM_DESC(run, "Write-only: echo 1 to run test");

/* result (read) */
static int result_get(char *buf, const struct kernel_param *kp)
{
    const char *name = lock_kind_name(g_ctx.lock_type);

    if (g_ctx.last_run_result < 0) {
        return scnprintf(buf, PAGE_SIZE,
                         "counter=%lld threads=%u iterations=%u lock=%s error=%d\n",
                         g_ctx.shared_counter, g_ctx.num_threads,
                         g_ctx.iterations, name, g_ctx.last_run_result);
    }

    return scnprintf(buf, PAGE_SIZE,
                     "counter=%lld threads=%u iterations=%u lock=%s ok\n",
                     g_ctx.shared_counter, g_ctx.num_threads,
                     g_ctx.iterations, name);
}

static const struct kernel_param_ops result_ops = {
    .get = result_get,
};

module_param_cb(result, &result_ops, NULL, 0444);
MODULE_PARM_DESC(result, "Read-only: last run result");

/* stats (read) */
static int stats_get(char *buf, const struct kernel_param *kp)
{
    s64 total_ns = ktime_to_ns(g_ctx.total_wait_time);
    unsigned int cont = g_ctx.contention_count;
    s64 avg = 0;

    if (cont)
        avg = div_s64(total_ns, cont);

    return scnprintf(buf, PAGE_SIZE,
                     "contention=%u total_wait_ns=%lld avg_wait_ns=%lld\n",
                     cont, (long long)total_ns, (long long)avg);
}

static const struct kernel_param_ops stats_ops = {
    .get = stats_get,
};

module_param_cb(stats, &stats_ops, NULL, 0444);
MODULE_PARM_DESC(stats, "Read-only: contention and wait times");

/* reset (write) */
static int reset_set(const char *val, const struct kernel_param *kp)
{
    unsigned int v;

    if (kstrtouint(val, 10, &v))
        return -EINVAL;
    if (v == 0)
        return 0;

    if (atomic_read(&g_ctx.running))
        return -EBUSY;

    g_ctx.shared_counter = 0;
    g_ctx.total_wait_time = 0;
    g_ctx.contention_count = 0;
    g_ctx.last_run_result = 0;

    return 0;
}

static const struct kernel_param_ops reset_ops = {
    .set = reset_set,
};

module_param_cb(reset, &reset_ops, NULL, 0220);
MODULE_PARM_DESC(reset, "Write-only: reset counter and stats");

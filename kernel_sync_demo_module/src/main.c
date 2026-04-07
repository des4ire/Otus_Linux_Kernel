// src/main.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "ctx.h"

static unsigned int num_threads = 4;
module_param(num_threads, uint, 0444);
MODULE_PARM_DESC(num_threads, "Number of worker threads (1..32)");

static unsigned int iterations = 1000;
module_param(iterations, uint, 0444);
MODULE_PARM_DESC(iterations, "Iterations per thread (1..1000000)");

/*
 * lock_type НЕ регистрируем здесь через module_param(),
 * потому что он уже зарегистрирован через module_param_cb() в params.c.
 * Значение по умолчанию останется 0, т.к. g_ctx статически инициализируется нулями.
 */

struct sync_ctx g_ctx;

static int __init kernel_sync_demo_init(void)
{
    if (num_threads < 1 || num_threads > 32)
        return -EINVAL;
    if (iterations < 1 || iterations > 1000000)
        return -EINVAL;
    if (g_ctx.lock_type > 2)
        return -EINVAL;

    g_ctx.num_threads = num_threads;
    g_ctx.iterations = iterations;

    g_ctx.shared_counter = 0;
    g_ctx.total_wait_time = 0;
    g_ctx.contention_count = 0;
    g_ctx.last_run_result = 0;

    atomic_set(&g_ctx.running, 0);
    init_completion(&g_ctx.done);

    pr_info("init threads=%u iterations=%u lock=%u\n",
            g_ctx.num_threads, g_ctx.iterations, g_ctx.lock_type);
    return 0;
}

static void __exit kernel_sync_demo_exit(void)
{
    workers_stop_if_running(&g_ctx);
    pr_info("exit\n");
}

module_init(kernel_sync_demo_init);
module_exit(kernel_sync_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Ogurcov");
MODULE_DESCRIPTION("kernel_sync_demo: spinlock vs mutex vs semaphore");

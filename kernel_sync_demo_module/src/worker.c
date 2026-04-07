// src/worker.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/kthread.h>

#include "ctx.h"
#include "sync.h"

#define WAIT_THRESHOLD_NS 100

static int worker_fn(void *data)
{
    struct worker_args *args = data;
    struct sync_ctx *ctx = args->ctx;
    unsigned int i;

    args->wait_time = 0;

    for (i = 0; i < ctx->iterations; i++) {
        ktime_t t1, t2;
        s64 ns;

        /* increment */
        t1 = ktime_get();
        sync_lock(ctx);
        t2 = ktime_get();

        ns = ktime_to_ns(ktime_sub(t2, t1));
        if (ns > WAIT_THRESHOLD_NS) {
            args->wait_time = ktime_add_ns(args->wait_time, ns);
            ctx->contention_count++;
        }

        ctx->shared_counter++;
        sync_unlock(ctx);

        /* decrement: отдельный lock/unlock */
        t1 = ktime_get();
        sync_lock(ctx);
        t2 = ktime_get();

        ns = ktime_to_ns(ktime_sub(t2, t1));
        if (ns > WAIT_THRESHOLD_NS) {
            args->wait_time = ktime_add_ns(args->wait_time, ns);
            ctx->contention_count++;
        }

        ctx->shared_counter--;
        sync_unlock(ctx);

        if (kthread_should_stop())
            break;
    }

    if (atomic_inc_return(&ctx->threads_done) == ctx->num_threads)
        complete(&ctx->done);

    return 0;
}

/* Освобождаем только массивы, БЕЗ kthread_stop() */
static void workers_release_arrays(struct sync_ctx *ctx)
{
    kfree(ctx->threads);
    ctx->threads = NULL;

    kfree(ctx->wargs);
    ctx->wargs = NULL;
}

/* Останавливаем только реально запущенные потоки */
static void workers_stop_threads(struct sync_ctx *ctx)
{
    unsigned int i;

    if (!ctx->threads)
        return;

    for (i = 0; i < ctx->num_threads; i++) {
        if (ctx->threads[i]) {
            kthread_stop(ctx->threads[i]);
            ctx->threads[i] = NULL;
        }
    }
}

int workers_run(struct sync_ctx *ctx)
{
    unsigned int i;
    int ret = 0;

    if (atomic_xchg(&ctx->running, 1))
        return SD_BUSY;

    reinit_completion(&ctx->done);
    atomic_set(&ctx->threads_done, 0);

    ctx->shared_counter = 0;
    ctx->total_wait_time = 0;
    ctx->contention_count = 0;
    ctx->last_run_result = 0;

    sync_init_lock(ctx);

    ctx->threads = kcalloc(ctx->num_threads, sizeof(*ctx->threads), GFP_KERNEL);
    if (!ctx->threads) {
        ret = SD_NOMEM;
        goto out_fail;
    }

    ctx->wargs = kcalloc(ctx->num_threads, sizeof(*ctx->wargs), GFP_KERNEL);
    if (!ctx->wargs) {
        ret = SD_NOMEM;
        goto out_fail;
    }

    for (i = 0; i < ctx->num_threads; i++) {
        ctx->wargs[i].ctx = ctx;
        ctx->wargs[i].thread_id = i;
        ctx->wargs[i].wait_time = 0;

        ctx->threads[i] = kthread_create(worker_fn, &ctx->wargs[i],
                                         "ksd_worker/%u", i);
        if (IS_ERR(ctx->threads[i])) {
            ret = PTR_ERR(ctx->threads[i]);
            ctx->threads[i] = NULL;
            goto out_fail;
        }

        wake_up_process(ctx->threads[i]);
    }

    /* Нормальный путь: просто ждём завершения всех потоков */
    wait_for_completion(&ctx->done);

    /* Собираем статистику */
    for (i = 0; i < ctx->num_threads; i++)
        ctx->total_wait_time = ktime_add(ctx->total_wait_time,
                                         ctx->wargs[i].wait_time);

        if (ctx->shared_counter != 0)
            ctx->last_run_result = -EIO;

    workers_release_arrays(ctx);
    atomic_set(&ctx->running, 0);
    return 0;

    out_fail:
    /*
     * Ошибка при создании: тут потоки могли уже стартовать,
     * поэтому их нужно останавливать через kthread_stop().
     */
    workers_stop_threads(ctx);
    workers_release_arrays(ctx);

    ctx->last_run_result = ret;
    atomic_set(&ctx->running, 0);
    return ret;
}

void workers_stop_if_running(struct sync_ctx *ctx)
{
    if (!atomic_read(&ctx->running))
        return;

    workers_stop_threads(ctx);
    workers_release_arrays(ctx);

    atomic_set(&ctx->running, 0);
}

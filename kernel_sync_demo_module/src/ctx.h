#ifndef KERNEL_SYNC_DEMO_CTX_H
#define KERNEL_SYNC_DEMO_CTX_H

#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/completion.h>

#include "sync.h"

#define SD_OK       0
#define SD_INVALID  -EINVAL
#define SD_NOMEM    -ENOMEM
#define SD_BUSY     -EBUSY

struct worker_args {
    struct sync_ctx *ctx;
    unsigned int thread_id;
    ktime_t wait_time;
};

struct sync_ctx {
    unsigned int num_threads;
    unsigned int iterations;
    unsigned int lock_type;

    long long shared_counter;

    spinlock_t slock;
    struct mutex mlock;
    struct semaphore sem;

    ktime_t total_wait_time;
    unsigned int contention_count;

    struct task_struct **threads;
    struct worker_args *wargs;

    atomic_t threads_done;
    int last_run_result;

    /* чтобы не запустить два теста параллельно */
    atomic_t running;

    /* сигнал завершения всех потоков */
    struct completion done;
};

extern struct sync_ctx g_ctx;

int workers_run(struct sync_ctx *ctx);
void workers_stop_if_running(struct sync_ctx *ctx);

#endif

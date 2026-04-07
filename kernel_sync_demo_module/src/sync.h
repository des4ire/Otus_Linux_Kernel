#ifndef KERNEL_SYNC_DEMO_SYNC_H
#define KERNEL_SYNC_DEMO_SYNC_H

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/ktime.h>

enum lock_kind {
    LOCK_SPINLOCK = 0,
    LOCK_MUTEX = 1,
    LOCK_SEMAPHORE = 2,
};

struct sync_ctx;

void sync_init_lock(struct sync_ctx *ctx);
void sync_lock(struct sync_ctx *ctx);
void sync_unlock(struct sync_ctx *ctx);

const char *lock_kind_name(unsigned int lock_type);

#endif

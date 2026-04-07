// src/sync.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include "ctx.h"
#include "sync.h"

void sync_init_lock(struct sync_ctx *ctx)
{
    switch (ctx->lock_type) {
        case LOCK_SPINLOCK:
            spin_lock_init(&ctx->slock);
            break;
        case LOCK_MUTEX:
            mutex_init(&ctx->mlock);
            break;
        case LOCK_SEMAPHORE:
            sema_init(&ctx->sem, 1);
            break;
        default:
            /* не должно случиться после валидации */
            spin_lock_init(&ctx->slock);
            break;
    }
}

void sync_lock(struct sync_ctx *ctx)
{
    switch (ctx->lock_type) {
        case LOCK_SPINLOCK:
            spin_lock(&ctx->slock);
            break;
        case LOCK_MUTEX:
            mutex_lock(&ctx->mlock);
            break;
        case LOCK_SEMAPHORE:
            down(&ctx->sem);
            break;
    }
}

void sync_unlock(struct sync_ctx *ctx)
{
    switch (ctx->lock_type) {
        case LOCK_SPINLOCK:
            spin_unlock(&ctx->slock);
            break;
        case LOCK_MUTEX:
            mutex_unlock(&ctx->mlock);
            break;
        case LOCK_SEMAPHORE:
            up(&ctx->sem);
            break;
    }
}

const char *lock_kind_name(unsigned int lock_type)
{
    switch (lock_type) {
        case LOCK_SPINLOCK:
            return "spinlock";
        case LOCK_MUTEX:
            return "mutex";
        case LOCK_SEMAPHORE:
            return "semaphore";
        default:
            return "unknown";
    }
}

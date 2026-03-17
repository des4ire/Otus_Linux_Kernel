// src/params.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
//#include <linux/mutex.h>

#include "ctx.h"

/* build.c */
int bs_build(struct bucket_search_ctx *ctx);
void bs_destroy(struct bucket_search_ctx *ctx);

/* search.c */
int bs_search(struct bucket_search_ctx *ctx, unsigned int x);
int bs_bucket_dump(struct bucket_search_ctx *ctx, char *buf, size_t buf_sz);

static inline unsigned int num_buckets(const struct bucket_search_ctx *ctx)
{
    return 1u << ctx->num_buckets_bits;
}

/* search (write) */
static int search_set(const char *val, const struct kernel_param *kp)
{
    unsigned int x;
    if (kstrtouint(val, 10, &x))
        return -EINVAL;
    return bs_search(&g_ctx, x);
}

static const struct kernel_param_ops search_ops = { .set = search_set };
module_param_cb(search, &search_ops, NULL, 0220);
MODULE_PARM_DESC(search, "Write-only: search X");

/* result (read) */
static int result_get(char *buf, const struct kernel_param *kp)
{
    int found;
    unsigned int v, b;

    mutex_lock(&g_ctx.lock);
    found = g_ctx.last_found;
    v = g_ctx.last_value;
    b = g_ctx.last_bucket;
    mutex_unlock(&g_ctx.lock);

    return scnprintf(buf, PAGE_SIZE, "found=%d value=%u bucket=%u\n", found, v, b);
}

static const struct kernel_param_ops result_ops = { .get = result_get };
module_param_cb(result, &result_ops, NULL, 0444);
MODULE_PARM_DESC(result, "Read-only: last search result");

/* rebuild (write) */
static int rebuild_set(const char *val, const struct kernel_param *kp)
{
    unsigned int x;

    if (kstrtouint(val, 10, &x))
        return -EINVAL;
    if (x == 0)
        return 0;

    mutex_lock(&g_ctx.lock);
    bs_destroy(&g_ctx);
    hash_init(g_ctx.htable);
    if (bs_build(&g_ctx)) {
        mutex_unlock(&g_ctx.lock);
        return -ENOMEM;
    }
    mutex_unlock(&g_ctx.lock);

    pr_info("rebuild done\n");
    return 0;
}

static const struct kernel_param_ops rebuild_ops = { .set = rebuild_set };
module_param_cb(rebuild, &rebuild_ops, NULL, 0220);
MODULE_PARM_DESC(rebuild, "Write-only: rebuild hash table");

/* bucket_id (write) */
static int bucket_id_set(const char *val, const struct kernel_param *kp)
{
    unsigned int b;

    if (kstrtouint(val, 10, &b))
        return -EINVAL;

    if (b >= num_buckets(&g_ctx))
        return -EINVAL;

    mutex_lock(&g_ctx.lock);
    g_ctx.current_bucket_id = b;
    mutex_unlock(&g_ctx.lock);


    return 0;
}

static const struct kernel_param_ops bucket_id_ops = { .set = bucket_id_set };
module_param_cb(bucket_id, &bucket_id_ops, NULL, 0220);
MODULE_PARM_DESC(bucket_id, "Write-only: select bucket for dump");

/* bucket_dump (read) */
static int bucket_dump_get(char *buf, const struct kernel_param *kp)
{
    unsigned int b;
    int ret;

    mutex_lock(&g_ctx.lock);
    b = g_ctx.current_bucket_id;
    mutex_unlock(&g_ctx.lock);

    if (b >= num_buckets(&g_ctx))
        return scnprintf(buf, PAGE_SIZE, "ERROR: invalid bucket_id\n");

    ret = bs_bucket_dump(&g_ctx, buf, PAGE_SIZE);
    if (ret < 0)
        return scnprintf(buf, PAGE_SIZE, "ERROR\n");
    return ret;
}

static const struct kernel_param_ops bucket_dump_ops = { .get = bucket_dump_get };
module_param_cb(bucket_dump, &bucket_dump_ops, NULL, 0444);
MODULE_PARM_DESC(bucket_dump, "Read-only: dump selected bucket");

/* для симметрии (ничего не делаем) */
void bs_params_init(void) {}
void bs_params_exit(void) {}

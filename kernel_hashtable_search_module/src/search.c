// src/search.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/hash.h>   /* hash_min */
#include <linux/kernel.h>
#include <linux/slab.h>
//#include <linux/hashtable.h>
#include <linux/sort.h>
#include <linux/bsearch.h>
#include <linux/mm.h>
//#include <linux/mutex.h>

#include "ctx.h"

static inline unsigned int bucket_key(unsigned int value, unsigned int bits)
{
    return value & ((1u << bits) - 1u);
}

/* sort(): сравниваем entry->value */
static int cmp_entry_ptrs(const void *a, const void *b)
{
    const struct hash_entry *ea = *(const struct hash_entry * const *)a;
    const struct hash_entry *eb = *(const struct hash_entry * const *)b;

    if (ea->value < eb->value)
        return -1;
    if (ea->value > eb->value)
        return 1;
    return 0;
}

/* bsearch(): key = unsigned int*, element = struct hash_entry* (через указатель в массиве) */
static int cmp_key_to_entry_ptr(const void *key, const void *element)
{
    unsigned int k = *(const unsigned int *)key;
    const struct hash_entry *e = *(const struct hash_entry * const *)element;

    if (k < e->value)
        return -1;
    if (k > e->value)
        return 1;
    return 0;
}

static int collect_bucket_sorted(struct bucket_search_ctx *ctx,
                                 unsigned int bucket,
                                 struct hash_entry ***out_arr,
                                 size_t *out_len)
{
    struct hash_entry *e;
    struct hash_entry **arr;
    size_t n = 0, i = 0;

    /* 1) посчитать */
    hlist_for_each_entry(e, &ctx->htable[bucket], node)
    n++;

    if (n == 0) {
        *out_arr = NULL;
        *out_len = 0;
        return 0;
    }

    /* 2) собрать указатели */
    arr = kmalloc_array(n, sizeof(*arr), GFP_KERNEL);
    if (!arr)
        return -ENOMEM;

    hlist_for_each_entry(e, &ctx->htable[bucket], node)
    arr[i++] = e;

    /* 3) sort() */
    sort(arr, n, sizeof(arr[0]), cmp_entry_ptrs, NULL);

    *out_arr = arr;
    *out_len = n;
    return 0;
}

int bs_search(struct bucket_search_ctx *ctx, unsigned int x)
{
    unsigned int bucket;
    struct hash_entry **arr = NULL;
    size_t n = 0;
    void *found;

    //bucket = bucket_key(x, ctx->num_buckets_bits);
     bucket = hash_min(x, ctx->num_buckets_bits);
    mutex_lock(&ctx->lock);

    ctx->last_value = x;
    ctx->last_bucket = bucket;
    ctx->last_found = 0;

    if (x >= ctx->array_size) {
        mutex_unlock(&ctx->lock);
        return 0; /* "не найдено" */
    }

    if (collect_bucket_sorted(ctx, bucket, &arr, &n)) {
        mutex_unlock(&ctx->lock);
        return -ENOMEM;
    }

    found = bsearch(&x, arr, n, sizeof(arr[0]), cmp_key_to_entry_ptr);
    ctx->last_found = (found != NULL);

    mutex_unlock(&ctx->lock);

    kfree(arr);

    pr_info("search x=%u bucket=%u found=%d\n", x, bucket, ctx->last_found);
    return 0;
}

int bs_bucket_dump(struct bucket_search_ctx *ctx, char *buf, size_t buf_sz)
{
    unsigned int bucket = ctx->current_bucket_id;
    struct hash_entry **arr = NULL;
    size_t n = 0;
    size_t pos = 0;
    size_t i;
    int ret;

    mutex_lock(&ctx->lock);

    ret = collect_bucket_sorted(ctx, bucket, &arr, &n);
    if (ret) {
        mutex_unlock(&ctx->lock);
        return ret;
    }

    pos += scnprintf(buf + pos, buf_sz - pos, "bucket=%u len=%zu:", bucket, n);

    for (i = 0; i < n; i++) {
        if (pos + 12 >= buf_sz) { /* место под " ...\n" */
            pos += scnprintf(buf + pos, buf_sz - pos, " ...");
            break;
        }
        pos += scnprintf(buf + pos, buf_sz - pos, " %u", arr[i]->value);
    }

    pos += scnprintf(buf + pos, buf_sz - pos, "\n");

    mutex_unlock(&ctx->lock);

    kfree(arr);
    return (int)pos;
}

int bs_rebuild(struct bucket_search_ctx *ctx)
{
    /* rebuild должен удалять и пересоздавать таблицу */
    mutex_lock(&ctx->lock);
    mutex_unlock(&ctx->lock);

    /* Реализацию удаления/построения делаем в params.c через build.c,
     *   чтобы не тянуть лишние include сюда. */
    return 0;
}

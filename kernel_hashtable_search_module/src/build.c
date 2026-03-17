// src/build.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/hash.h>   /* hash_min */
#include <linux/list.h>   /* hlist_add_head */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/random.h>
//#include <linux/hashtable.h>
#include "ctx.h"


static inline unsigned int bucket_key(unsigned int value, unsigned int bits)
{
    /* hash = value % num_buckets, num_buckets = 2^bits */
    return value & ((1u << bits) - 1u);
}

static void shuffle_u32(unsigned int *a, unsigned int n)
{
    /* Fisher–Yates */
    unsigned int i;
    for (i = n - 1; i > 0; i--) {
        unsigned int j = prandom_u32_max(i + 1);
        unsigned int tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

int bs_build(struct bucket_search_ctx *ctx)
{
    unsigned int *vals;
    unsigned int i;

    vals = kmalloc_array(ctx->array_size, sizeof(*vals), GFP_KERNEL);
    if (!vals)
        return -ENOMEM;

    for (i = 0; i < ctx->array_size; i++)
        vals[i] = i;

    /* "случайный массив" без выходов за диапазон (перемешанная 0..N-1) */
    shuffle_u32(vals, ctx->array_size);

    for (i = 0; i < ctx->array_size; i++) {
        struct hash_entry *e;
        unsigned int v = vals[i];
        unsigned int bucket = hash_min(v, ctx->num_buckets_bits); /* как в задании */

        e = kmalloc(sizeof(*e), GFP_KERNEL);
        if (!e) {
            kfree(vals);
            return -ENOMEM;
        }

        e->value = v;

        /* кладём напрямую в нужную корзину */
        hlist_add_head(&e->node, &ctx->htable[bucket]);
    }

    kfree(vals);
    pr_info("build done\n");
    return 0;
}

void bs_destroy(struct bucket_search_ctx *ctx)
{
    struct hash_entry *e;
    struct hlist_node *tmp;
    int bkt;

    hash_for_each_safe(ctx->htable, bkt, tmp, e, node) {
        hash_del(&e->node);
        kfree(e);
    }
}

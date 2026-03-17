// src/main.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/hashtable.h>
//#include <linux/mutex.h>
#include "ctx.h"


static unsigned int array_size = 1024;
module_param(array_size, uint, 0444);
MODULE_PARM_DESC(array_size, "Array size / max value + 1");

static unsigned int num_buckets_bits = 6;
module_param(num_buckets_bits, uint, 0444);
MODULE_PARM_DESC(num_buckets_bits, "Buckets bits (num buckets = 2^bits)");

/* Экспортируем контекст для остальных файлов */
struct bucket_search_ctx g_ctx;

/* build.c */
int bs_build(struct bucket_search_ctx *ctx);
void bs_destroy(struct bucket_search_ctx *ctx);

/* params.c */
void bs_params_init(void);
void bs_params_exit(void);

static int __init kernel_hashtable_search_init(void)
{
    if (array_size == 0 || num_buckets_bits == 0 || num_buckets_bits > MAX_BUCKET_BITS)
        return -EINVAL;

    mutex_init(&g_ctx.lock);

    g_ctx.array_size = array_size;
    g_ctx.num_buckets_bits = num_buckets_bits;
    g_ctx.last_found = 0;
    g_ctx.last_value = 0;
    g_ctx.last_bucket = 0;
    g_ctx.current_bucket_id = 0;

    hash_init(g_ctx.htable);

    if (bs_build(&g_ctx)) {
        pr_err("build failed\n");
        bs_destroy(&g_ctx);
        return -ENOMEM;
    }

    bs_params_init();

    pr_info("init: array_size=%u buckets=%u (bits=%u)\n",
            g_ctx.array_size, 1u << g_ctx.num_buckets_bits, g_ctx.num_buckets_bits);
    return 0;
}

static void __exit kernel_hashtable_search_exit(void)
{
    bs_params_exit();

    bs_destroy(&g_ctx);
    pr_info("exit\n");
}

module_init(kernel_hashtable_search_init);
module_exit(kernel_hashtable_search_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Ogurcov");
MODULE_DESCRIPTION("Hashtable + sort + bsearch via module_param_cb");

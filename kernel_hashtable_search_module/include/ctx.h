#ifndef KERNEL_HASHTABLE_SEARCH_CTX_H
#define KERNEL_HASHTABLE_SEARCH_CTX_H

#include <linux/hashtable.h>
#include <linux/mutex.h>

#define MAX_BUCKET_BITS 16
#warning "USING PROJECT include/ctx.h"
struct hash_entry {
    struct hlist_node node;
    unsigned int value;
};

struct bucket_search_ctx {
    unsigned int array_size;
    unsigned int num_buckets_bits;

    DECLARE_HASHTABLE(htable, MAX_BUCKET_BITS);

    int last_found;
    unsigned int last_value;
    unsigned int last_bucket;

    unsigned int current_bucket_id;

    struct mutex lock;
};

extern struct bucket_search_ctx g_ctx;

#endif

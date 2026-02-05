// src/allocator.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/string.h>

#include "allocator.h"

#define POOL_SIZE_BYTES (10u * 1024u * 1024u)  /* 10 MiB */   /* :contentReference[oaicite:6]{index=6} */
#define BLOCK_SIZE      4096u                  /* 4 KiB */    /* :contentReference[oaicite:7]{index=7} */
#define TOTAL_BLOCKS    (POOL_SIZE_BYTES / BLOCK_SIZE)        /* 2560 */ /* :contentReference[oaicite:8]{index=8} */
#define BITMAP_BYTES    ((TOTAL_BLOCKS + 7) / 8)              /* 320 */  /* :contentReference[oaicite:9]{index=9} */

struct memory_allocator {
    unsigned char *bitmap;
    void *memory_pool;
    size_t total_blocks;
    size_t block_size;
    spinlock_t lock;
};

struct alloc_node {
    struct list_head list;
    void *ptr;
    size_t start_block;
    size_t num_blocks;
};

static struct memory_allocator g_alloc;
static LIST_HEAD(g_alloc_list);

/* bitmap helpers (из bitmap.c) */
int bitmap_first_fit(const unsigned char *bm, size_t total_blocks,
                     size_t need, size_t *start_out);
size_t bitmap_count_free(const unsigned char *bm, size_t total_blocks);
size_t bitmap_largest_free_run(const unsigned char *bm, size_t total_blocks);
int bitmap_to_string(const unsigned char *bm, size_t total_blocks,
                     char *out, size_t out_sz);

/* локальные бит-операции для пометки диапазонов */
static inline int bm_test(const unsigned char *bm, size_t idx)
{
    size_t byte = idx / 8, bit = idx % 8;
    return (bm[byte] >> bit) & 1;
}
static inline void bm_set(unsigned char *bm, size_t idx)
{
    size_t byte = idx / 8, bit = idx % 8;
    bm[byte] |= (unsigned char)(1u << bit);
}
static inline void bm_clear(unsigned char *bm, size_t idx)
{
    size_t byte = idx / 8, bit = idx % 8;
    bm[byte] &= (unsigned char)~(1u << bit);
}

static size_t bytes_to_blocks(size_t bytes)
{
    size_t blocks = bytes / BLOCK_SIZE;
    if (bytes % BLOCK_SIZE)
        blocks++;
    return blocks;
}

static int ptr_to_block(void *ptr, size_t *block_out)
{
    uintptr_t base = (uintptr_t)g_alloc.memory_pool;
    uintptr_t p = (uintptr_t)ptr;

    if (!g_alloc.memory_pool || !block_out)
        return ALLOC_INVALID;

    if (p < base || p >= base + POOL_SIZE_BYTES)
        return ALLOC_INVALID;

    if ((p - base) % BLOCK_SIZE != 0)
        return ALLOC_INVALID;

    *block_out = (p - base) / BLOCK_SIZE;
    if (*block_out >= TOTAL_BLOCKS)
        return ALLOC_INVALID;

    return ALLOC_OK;
}

int allocator_init(void)
{
    g_alloc.total_blocks = TOTAL_BLOCKS;
    g_alloc.block_size = BLOCK_SIZE;
    spin_lock_init(&g_alloc.lock);

    g_alloc.bitmap = kzalloc(BITMAP_BYTES, GFP_KERNEL);
    if (!g_alloc.bitmap)
        return ALLOC_NOMEM;

    /* 10 MiB лучше через vzalloc/vmalloc (kmalloc может не дать большой contiguous) */
    g_alloc.memory_pool = vzalloc(POOL_SIZE_BYTES);
    if (!g_alloc.memory_pool) {
        kfree(g_alloc.bitmap);
        g_alloc.bitmap = NULL;
        return ALLOC_NOMEM;
    }

    pr_info("init: pool=%u bytes, block=%u, blocks=%zu, bitmap=%u bytes\n",
            POOL_SIZE_BYTES, BLOCK_SIZE, g_alloc.total_blocks, BITMAP_BYTES);
    return ALLOC_OK;
}

void allocator_cleanup(void)
{
    struct alloc_node *n, *tmp;
    unsigned long flags;

    /* освободим все активные аллокации */
    spin_lock_irqsave(&g_alloc.lock, flags);
    list_for_each_entry_safe(n, tmp, &g_alloc_list, list) {
        list_del(&n->list);
        /* сбрасываем биты */
        for (size_t i = 0; i < n->num_blocks; i++)
            bm_clear(g_alloc.bitmap, n->start_block + i);
        spin_unlock_irqrestore(&g_alloc.lock, flags);

        kfree(n);

        spin_lock_irqsave(&g_alloc.lock, flags);
    }
    spin_unlock_irqrestore(&g_alloc.lock, flags);

    if (g_alloc.memory_pool) {
        vfree(g_alloc.memory_pool);
        g_alloc.memory_pool = NULL;
    }

    kfree(g_alloc.bitmap);
    g_alloc.bitmap = NULL;

    pr_info("cleanup\n");
}

void *allocator_alloc(size_t bytes)
{
    size_t need, start;
    void *ptr = NULL;
    struct alloc_node *node;
    unsigned long flags;
    int ret;

    if (bytes == 0)
        return NULL;

    need = bytes_to_blocks(bytes);
    if (need > TOTAL_BLOCKS)
        return NULL;

    /* нельзя аллоцировать под spinlock (GFP_KERNEL может спать) */
    node = kmalloc(sizeof(*node), GFP_KERNEL);
    if (!node)
        return NULL;

    spin_lock_irqsave(&g_alloc.lock, flags);

    ret = bitmap_first_fit(g_alloc.bitmap, g_alloc.total_blocks, need, &start);
    if (ret) {
        spin_unlock_irqrestore(&g_alloc.lock, flags);
        kfree(node);
        return NULL;
    }

    /* помечаем блоки занятыми */
    for (size_t i = 0; i < need; i++)
        bm_set(g_alloc.bitmap, start + i);

    ptr = (void *)((char *)g_alloc.memory_pool + start * BLOCK_SIZE);

    node->ptr = ptr;
    node->start_block = start;
    node->num_blocks = need;
    list_add(&node->list, &g_alloc_list);

    spin_unlock_irqrestore(&g_alloc.lock, flags);

    /* печатаем адрес как число, чтобы не зависеть от %p/%px и kptr_restrict */
    pr_info("allocated %zu bytes (%zu blocks) at 0x%llx\n",
            bytes, need, (unsigned long long)(uintptr_t)ptr);

    return ptr;
}

int allocator_free(void *ptr)
{
    struct alloc_node *n, *found = NULL;
    size_t start;
    unsigned long flags;

    if (!ptr)
        return ALLOC_INVALID;

    if (ptr_to_block(ptr, &start) != ALLOC_OK)
        return ALLOC_INVALID;

    spin_lock_irqsave(&g_alloc.lock, flags);

    /* ищем именно этот указатель в списке активных аллокаций */
    list_for_each_entry(n, &g_alloc_list, list) {
        if (n->ptr == ptr) {
            found = n;
            break;
        }
    }

    if (!found) {
        spin_unlock_irqrestore(&g_alloc.lock, flags);
        return ALLOC_NOT_FOUND;
    }

    /* защита от "битых" состояний */
    if (found->start_block != start) {
        spin_unlock_irqrestore(&g_alloc.lock, flags);
        return ALLOC_INVALID;
    }

    /* сбрасываем биты */
    for (size_t i = 0; i < found->num_blocks; i++) {
        /* двойное освобождение: если бит уже 0 — состояние неверное */
        if (!bm_test(g_alloc.bitmap, found->start_block + i)) {
            spin_unlock_irqrestore(&g_alloc.lock, flags);
            return ALLOC_INVALID;
        }
        bm_clear(g_alloc.bitmap, found->start_block + i);
    }

    list_del(&found->list);
    spin_unlock_irqrestore(&g_alloc.lock, flags);

    pr_info("freed memory at 0x%llx (%zu blocks)\n",
            (unsigned long long)(uintptr_t)ptr, found->num_blocks);

    kfree(found);
    return ALLOC_OK;
}

struct stats_info allocator_get_stats(void)
{
    struct stats_info s;
    unsigned char snapshot[BITMAP_BYTES];
    unsigned long flags;
    size_t free_blocks, largest_run;

    memset(&s, 0, sizeof(s));

    s.total_blocks = TOTAL_BLOCKS;
    s.total_memory = POOL_SIZE_BYTES;

    /* снимем bitmap под локом, а считать будем без лока */
    spin_lock_irqsave(&g_alloc.lock, flags);
    memcpy(snapshot, g_alloc.bitmap, BITMAP_BYTES);
    spin_unlock_irqrestore(&g_alloc.lock, flags);

    free_blocks = bitmap_count_free(snapshot, TOTAL_BLOCKS);
    largest_run = bitmap_largest_free_run(snapshot, TOTAL_BLOCKS);

    s.free_blocks = free_blocks;
    s.allocated_blocks = TOTAL_BLOCKS - free_blocks;

    s.free_memory = free_blocks * BLOCK_SIZE;
    s.allocated_memory = s.allocated_blocks * BLOCK_SIZE;

    if (free_blocks == 0)
        s.fragmentation_percent = 0;
    else
        s.fragmentation_percent =
        100 - (largest_run * 100 / free_blocks);

    return s;
}

int allocator_bitmap_string(char *out, size_t out_sz)
{
    unsigned char snapshot[BITMAP_BYTES];
    unsigned long flags;

    spin_lock_irqsave(&g_alloc.lock, flags);
    memcpy(snapshot, g_alloc.bitmap, BITMAP_BYTES);
    spin_unlock_irqrestore(&g_alloc.lock, flags);

    return bitmap_to_string(snapshot, TOTAL_BLOCKS, out, out_sz);
}

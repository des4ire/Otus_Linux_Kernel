// src/params.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "allocator.h"

/* alloc (write-only) */
static int alloc_set(const char *val, const struct kernel_param *kp)
{
    unsigned long long bytes;
    void *p;
    int ret = kstrtoull(val, 10, &bytes);

    if (ret)
        return -EINVAL;

    if (bytes == 0)
        return -EINVAL;

    p = allocator_alloc((size_t)bytes);
    if (!p)
        return -ENOMEM;

    return 0;
}

static const struct kernel_param_ops alloc_ops = {
    .set = alloc_set,
};

module_param_cb(alloc, &alloc_ops, NULL, 0220);
MODULE_PARM_DESC(alloc, "Write-only: allocate N bytes");

/* free (write-only): ожидаем адрес как число (0x...) */
static int free_set(const char *val, const struct kernel_param *kp)
{
    unsigned long long addr;
    void *p;
    int ret;

    ret = kstrtoull(val, 0, &addr); /* base=0 => 0x/10 auto */
    if (ret)
        return -EINVAL;

    p = (void *)(uintptr_t)addr;

    ret = allocator_free(p);
    if (ret == ALLOC_OK)
        return 0;
    if (ret == ALLOC_NOT_FOUND)
        return -ENOENT;

    return -EINVAL;
}

static const struct kernel_param_ops free_ops = {
    .set = free_set,
};

module_param_cb(free, &free_ops, NULL, 0220);
MODULE_PARM_DESC(free, "Write-only: free by address (e.g. 0xffff...)");

/* stats (read-only) */
static int stats_get(char *buf, const struct kernel_param *kp)
{
    struct stats_info s = allocator_get_stats();

    /* формат похожий на пример из задания */
    return scnprintf(buf, PAGE_SIZE,
                     "Total: %zu KB | Free: %zu KB | Allocated: %zu KB | Fragmentation: %zu%%\n"
                     "Blocks: total=%zu free=%zu allocated=%zu\n",
                     s.total_memory / 1024,
                     s.free_memory / 1024,
                     s.allocated_memory / 1024,
                     s.fragmentation_percent,
                     s.total_blocks, s.free_blocks, s.allocated_blocks);
}

static const struct kernel_param_ops stats_ops = {
    .get = stats_get,
};

module_param_cb(stats, &stats_ops, NULL, 0444);
MODULE_PARM_DESC(stats, "Read-only: allocator statistics");

/* bitmap_info (read-only) */
static int bitmap_info_get(char *buf, const struct kernel_param *kp)
{
    int n = allocator_bitmap_string(buf, PAGE_SIZE);

    if (n < 0)
        return scnprintf(buf, PAGE_SIZE, "ERROR\n");

    return n;
}

static const struct kernel_param_ops bitmap_info_ops = {
    .get = bitmap_info_get,
};

module_param_cb(bitmap_info, &bitmap_info_ops, NULL, 0444);
MODULE_PARM_DESC(bitmap_info, "Read-only: bitmap visualization (X=used .=free)");

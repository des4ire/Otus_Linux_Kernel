#ifndef KERNEL_ALLOC_ALLOCATOR_H
#define KERNEL_ALLOC_ALLOCATOR_H

#include <linux/types.h>

#define ALLOC_OK        0
#define ALLOC_NOMEM     -1
#define ALLOC_INVALID   -2
#define ALLOC_NOT_FOUND -3

struct stats_info {
    size_t total_blocks;
    size_t free_blocks;
    size_t allocated_blocks;
    size_t total_memory;
    size_t free_memory;
    size_t allocated_memory;
    size_t fragmentation_percent;
};

int allocator_init(void);
void allocator_cleanup(void);

void *allocator_alloc(size_t bytes);
int allocator_free(void *ptr);

struct stats_info allocator_get_stats(void);

/* для params.c */
int allocator_bitmap_string(char *out, size_t out_sz);

#endif

// src/bitmap.c
#include <linux/kernel.h>
#include <linux/string.h>
#include "allocator.h"

/* 0 = free, 1 = used */
static inline int bm_test(const unsigned char *bm, size_t idx)
{
    size_t byte = idx / 8;
    size_t bit = idx % 8;
    return (bm[byte] >> bit) & 1;
}

static inline void bm_set(unsigned char *bm, size_t idx)
{
    size_t byte = idx / 8;
    size_t bit = idx % 8;
    bm[byte] |= (unsigned char)(1u << bit);
}

static inline void bm_clear(unsigned char *bm, size_t idx)
{
    size_t byte = idx / 8;
    size_t bit = idx % 8;
    bm[byte] &= (unsigned char)~(1u << bit);
}

/* first-fit: найти первую цепочку need свободных блоков */
int bitmap_first_fit(const unsigned char *bm, size_t total_blocks,
                     size_t need, size_t *start_out)
{
    size_t i = 0;

    if (!start_out || need == 0 || need > total_blocks)
        return -EINVAL;

    while (i + need <= total_blocks) {
        size_t j;

        /* пропускаем занятые */
        if (bm_test(bm, i)) {
            i++;
            continue;
        }

        /* проверяем непрерывный свободный отрезок */
        for (j = 0; j < need; j++) {
            if (bm_test(bm, i + j))
                break;
        }

        if (j == need) {
            *start_out = i;
            return 0;
        }

        i = i + j + 1;
    }

    return -ENOSPC;
}

size_t bitmap_count_free(const unsigned char *bm, size_t total_blocks)
{
    size_t i, free = 0;

    for (i = 0; i < total_blocks; i++)
        if (!bm_test(bm, i))
            free++;

    return free;
}

size_t bitmap_largest_free_run(const unsigned char *bm, size_t total_blocks)
{
    size_t i = 0;
    size_t best = 0;

    while (i < total_blocks) {
        size_t run = 0;

        while (i < total_blocks && bm_test(bm, i))
            i++;

        while (i < total_blocks && !bm_test(bm, i)) {
            run++;
            i++;
        }

        if (run > best)
            best = run;
    }

    return best;
}

/* визуализация: [X..XX....] где X=занято .=свободно */
int bitmap_to_string(const unsigned char *bm, size_t total_blocks,
                     char *out, size_t out_sz)
{
    size_t i, pos = 0;

    if (!out || out_sz < 4)
        return -EINVAL;

    out[pos++] = '[';

    for (i = 0; i < total_blocks; i++) {
        if (pos + 2 >= out_sz) /* место под ']' и '\n' */
            break;

        out[pos++] = bm_test(bm, i) ? 'X' : '.';

        /* небольшая группировка для читаемости */
        if ((i + 1) % 32 == 0 && pos + 2 < out_sz)
            out[pos++] = ' ';
    }

    if (pos + 2 >= out_sz)
        pos = out_sz - 2;

    out[pos++] = ']';
    out[pos++] = '\n';
    out[pos] = '\0';

    return (int)pos;
}

/* экспортируем символы для allocator.c */
int bitmap_first_fit(const unsigned char *bm, size_t total_blocks,
                     size_t need, size_t *start_out);
size_t bitmap_count_free(const unsigned char *bm, size_t total_blocks);
size_t bitmap_largest_free_run(const unsigned char *bm, size_t total_blocks);
int bitmap_to_string(const unsigned char *bm, size_t total_blocks,
                     char *out, size_t out_sz);

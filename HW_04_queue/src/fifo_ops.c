// src/fifo_ops.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/* Ошибки как в задании (можно подстроить под ваши константы) */
#define FIFO_OK       0
#define FIFO_EMPTY   -1
#define FIFO_FULL    -2
#define FIFO_NOMEM   -3
#define FIFO_INVALID -4

static struct {
    struct kfifo fifo;      /* хранит байты */
    spinlock_t lock;
    int max_size;           /* в элементах int */
    bool ready;
} g;

static inline int bytes_to_elems(unsigned int bytes)
{
    return bytes / sizeof(int);
}

int fifo_init(int max_size)
{
    int ret;
    unsigned int bytes;

    if (max_size <= 0)
        return FIFO_INVALID;

    spin_lock_init(&g.lock);
    g.max_size = max_size;
    bytes = (unsigned int)g.max_size * sizeof(int);

    ret = kfifo_alloc(&g.fifo, bytes, GFP_KERNEL);
    if (ret) {
        pr_err("kfifo_alloc failed: %d\n", ret);
        g.ready = false;
        return FIFO_NOMEM;
    }

    g.ready = true;
    pr_info("fifo init: capacity=%d elems\n", g.max_size);
    return FIFO_OK;
}

void fifo_free(void)
{
    if (!g.ready)
        return;

    kfifo_free(&g.fifo);
    g.ready = false;
    pr_info("fifo free\n");
}

int fifo_size(void)
{
    if (!g.ready)
        return 0;

    return bytes_to_elems(kfifo_len(&g.fifo));
}

int fifo_available(void)
{
    if (!g.ready)
        return 0;

    return bytes_to_elems(kfifo_avail(&g.fifo));
}

int fifo_is_empty(void)
{
    if (!g.ready)
        return 1;

    return kfifo_is_empty(&g.fifo) ? 1 : 0;
}

int fifo_is_full(void)
{
    if (!g.ready)
        return 0;

    /* “полна”, если не хватает места хотя бы на 1 int */
    return (kfifo_avail(&g.fifo) < sizeof(int)) ? 1 : 0;
}

void fifo_clear(void)
{
    unsigned long flags;

    if (!g.ready)
        return;

    spin_lock_irqsave(&g.lock, flags);
    kfifo_reset(&g.fifo);
    spin_unlock_irqrestore(&g.lock, flags);

    pr_info("clear (size=%d)\n", fifo_size());
}

int fifo_enqueue(int value)
{
    unsigned long flags;
    unsigned int written;

    if (!g.ready)
        return FIFO_INVALID;

    spin_lock_irqsave(&g.lock, flags);

    if (kfifo_avail(&g.fifo) < sizeof(int)) {
        spin_unlock_irqrestore(&g.lock, flags);
        return FIFO_FULL;
    }

    written = kfifo_in(&g.fifo, &value, sizeof(value));
    spin_unlock_irqrestore(&g.lock, flags);

    if (written != sizeof(value))
        return FIFO_INVALID;

    pr_info("enqueue %d (size=%d)\n", value, fifo_size());
    return FIFO_OK;
}

int fifo_dequeue(int *out)
{
    unsigned long flags;
    unsigned int read;

    if (!g.ready || !out)
        return FIFO_INVALID;

    spin_lock_irqsave(&g.lock, flags);

    if (kfifo_is_empty(&g.fifo)) {
        spin_unlock_irqrestore(&g.lock, flags);
        return FIFO_EMPTY;
    }

    read = kfifo_out(&g.fifo, out, sizeof(*out));
    spin_unlock_irqrestore(&g.lock, flags);

    if (read != sizeof(*out))
        return FIFO_INVALID;

    pr_info("dequeue -> %d (size=%d)\n", *out, fifo_size());
    return FIFO_OK;
}

int fifo_peek(int *out)
{
    unsigned long flags;
    unsigned int read;

    if (!g.ready || !out)
        return FIFO_INVALID;

    spin_lock_irqsave(&g.lock, flags);

    if (kfifo_is_empty(&g.fifo)) {
        spin_unlock_irqrestore(&g.lock, flags);
        return FIFO_EMPTY;
    }

    read = kfifo_out_peek(&g.fifo, out, sizeof(*out));
    spin_unlock_irqrestore(&g.lock, flags);

    if (read != sizeof(*out))
        return FIFO_INVALID;

    pr_info("peek -> %d (size=%d)\n", *out, fifo_size());
    return FIFO_OK;
}

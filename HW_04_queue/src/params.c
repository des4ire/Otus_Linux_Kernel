// src/params.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/errno.h>

/* внешние функции из fifo_ops.c */
int fifo_enqueue(int value);
int fifo_dequeue(int *out);
int fifo_peek(int *out);
int fifo_size(void);
int fifo_available(void);
int fifo_is_empty(void);
int fifo_is_full(void);
void fifo_clear(void);

/* те же коды, что и в fifo_ops.c */
#define FIFO_OK       0
#define FIFO_EMPTY   -1
#define FIFO_FULL    -2
#define FIFO_INVALID -4

/* enqueue (write-only) */
static int enqueue_set(const char *val, const struct kernel_param *kp)
{
    int v, ret;

    ret = kstrtoint(val, 10, &v);
    if (ret)
        return -EINVAL;

    ret = fifo_enqueue(v);
    if (ret == FIFO_FULL)
        return -ENOSPC;
    if (ret != FIFO_OK)
        return -EINVAL;

    return 0;
}

static const struct kernel_param_ops enqueue_ops = {
    .set = enqueue_set,
    .get = NULL,
};

module_param_cb(enqueue, &enqueue_ops, NULL, 0220);
MODULE_PARM_DESC(enqueue, "Write-only: enqueue int value");

/* dequeue (read-only): чтение делает pop */
static int dequeue_get(char *buf, const struct kernel_param *kp)
{
    int v, ret;

    ret = fifo_dequeue(&v);
    if (ret == FIFO_EMPTY)
        return scnprintf(buf, PAGE_SIZE, "EMPTY\n");
    if (ret != FIFO_OK)
        return scnprintf(buf, PAGE_SIZE, "ERROR\n");

    return scnprintf(buf, PAGE_SIZE, "%d\n", v);
}

static const struct kernel_param_ops dequeue_ops = {
    .get = dequeue_get, /* .set = NULL => RO */
};

module_param_cb(dequeue, &dequeue_ops, NULL, 0444);
MODULE_PARM_DESC(dequeue, "Read-only: dequeue and return head element");

/* peek (read-only) */
static int peek_get(char *buf, const struct kernel_param *kp)
{
    int v, ret;

    ret = fifo_peek(&v);
    if (ret == FIFO_EMPTY)
        return scnprintf(buf, PAGE_SIZE, "EMPTY\n");
    if (ret != FIFO_OK)
        return scnprintf(buf, PAGE_SIZE, "ERROR\n");

    return scnprintf(buf, PAGE_SIZE, "%d\n", v);
}

static const struct kernel_param_ops peek_ops = {
    .get = peek_get,
};

module_param_cb(peek, &peek_ops, NULL, 0444);
MODULE_PARM_DESC(peek, "Read-only: peek head element");

/* size (read-only) */
static int size_get(char *buf, const struct kernel_param *kp)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", fifo_size());
}

static const struct kernel_param_ops size_ops = {
    .get = size_get,
};

module_param_cb(size, &size_ops, NULL, 0444);
MODULE_PARM_DESC(size, "Read-only: number of elements");

/* available (read-only) */
static int available_get(char *buf, const struct kernel_param *kp)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", fifo_available());
}

static const struct kernel_param_ops available_ops = {
    .get = available_get,
};

module_param_cb(available, &available_ops, NULL, 0444);
MODULE_PARM_DESC(available, "Read-only: free slots");

/* is_empty (read-only) */
static int is_empty_get(char *buf, const struct kernel_param *kp)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", fifo_is_empty());
}

static const struct kernel_param_ops is_empty_ops = {
    .get = is_empty_get,
};

module_param_cb(is_empty, &is_empty_ops, NULL, 0444);
MODULE_PARM_DESC(is_empty, "Read-only: 1 if empty else 0");

/* is_full (read-only) */
static int is_full_get(char *buf, const struct kernel_param *kp)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", fifo_is_full());
}

static const struct kernel_param_ops is_full_ops = {
    .get = is_full_get,
};

module_param_cb(is_full, &is_full_ops, NULL, 0444);
MODULE_PARM_DESC(is_full, "Read-only: 1 if full else 0");

/* clear (write-only) */
static int clear_set(const char *val, const struct kernel_param *kp)
{
    fifo_clear();
    return 0;
}

static const struct kernel_param_ops clear_ops = {
    .set = clear_set,
    .get = NULL,
};

module_param_cb(clear, &clear_ops, NULL, 0220);
MODULE_PARM_DESC(clear, "Write-only: clear FIFO");

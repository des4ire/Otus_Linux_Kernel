#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "stack.h"
#include "stack_ops.h"

void stack_init(struct stack *s)
{
    INIT_LIST_HEAD(&s->elements);
    s->size = 0;
}

int stack_push(struct stack *s, int value)
{
    struct stack_entry *e = kmalloc(sizeof(*e), GFP_KERNEL);

    if (!e)
        return STACK_NOMEM;

    e->data = value;
    list_add(&e->list, &s->elements); /* push на вершину: в голову */
    s->size++;

    pr_info("push %d (size=%d)\n", value, s->size);
    return STACK_OK;
}

int stack_pop(struct stack *s, int *out)
{
    struct stack_entry *e;

    if (!out)
        return STACK_INVALID;

    if (list_empty(&s->elements))
        return STACK_EMPTY;

    e = list_first_entry(&s->elements, struct stack_entry, list);
    *out = e->data;

    list_del(&e->list);
    kfree(e);
    s->size--;

    pr_info("pop -> %d (size=%d)\n", *out, s->size);
    return STACK_OK;
}

int stack_peek(struct stack *s, int *out)
{
    struct stack_entry *e;

    if (!out)
        return STACK_INVALID;

    if (list_empty(&s->elements))
        return STACK_EMPTY;

    e = list_first_entry(&s->elements, struct stack_entry, list);
    *out = e->data;

    pr_info("peek -> %d (size=%d)\n", *out, s->size);
    return STACK_OK;
}

int stack_is_empty(struct stack *s)
{
    return list_empty(&s->elements) ? 1 : 0;
}

int stack_size(struct stack *s)
{
    return s->size;
}

void stack_clear(struct stack *s)
{
    struct stack_entry *e, *tmp;

    list_for_each_entry_safe(e, tmp, &s->elements, list) {
        list_del(&e->list);
        kfree(e);
    }
    s->size = 0;

    pr_info("clear (size=%d)\n", s->size);
}

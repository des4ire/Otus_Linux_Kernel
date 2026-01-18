#ifndef STACK_H
#define STACK_H

#include <linux/list.h>

struct stack {
    struct list_head elements; /* голова списка */
    int size;
};

struct stack_entry {
    struct list_head list;
    int data;
};

#endif

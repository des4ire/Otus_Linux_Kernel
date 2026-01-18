#ifndef STACK_OPS_H
#define STACK_OPS_H

#include "stack.h"

#define STACK_OK       0
#define STACK_EMPTY   -1
#define STACK_NOMEM   -2
#define STACK_INVALID -3

void stack_init(struct stack *s);
int  stack_push(struct stack *s, int value);
int  stack_pop(struct stack *s, int *out);
int  stack_peek(struct stack *s, int *out);
int  stack_is_empty(struct stack *s);
int  stack_size(struct stack *s);
void stack_clear(struct stack *s);

#endif

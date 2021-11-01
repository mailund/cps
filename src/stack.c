#include "cps.h"

#define INIT_STACK_SIZE 512 /* completely arbitrary */

void cps_init_stack(struct cps_stack *stack)
{
    stack->sp = 0;
    stack->data = malloc(INIT_STACK_SIZE);
    stack->size = INIT_STACK_SIZE;
}

void cps_resize_stack(struct cps_stack *stack, size_t new_size)
{
    // Don't make the stack too small...
    if (new_size < INIT_STACK_SIZE)
        new_size = INIT_STACK_SIZE;

    stack->data = realloc(stack->data, new_size);
    stack->size = new_size;
}

void cps_free_stack(struct cps_stack *stack)
{
    free(stack->data);
    stack->data = 0; // just in case.
}

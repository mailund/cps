//
//  closure.c
//  cps-iter
//
//  Created by Thomas Mailund on 26/10/2021.
//

#include "closure.h"
#include <assert.h>



cps_define_closure_type(fib, int, cps_args(int));

cps_define_closure_frame(ret, int, cps_args(int));
int cps_closure_function(ret, fib, int n) {
    cps_decref_frame(pool_, frame_);
    return n;
}

cps_define_closure_frame(fib1, int, cps_args(int),
                         int n; struct cps_closure k);
cps_define_closure_frame(fib2, int, cps_args(int),
                         int f1; struct cps_closure k);

int fib_cps(struct cps_stack *pool_, int n, struct cps_closure k)
{
    if (n <= 1)
    {
        return cps_call_closure(fib, k, n);
    }
    else
    {
        return fib_cps(pool_, n - 1,
                       cps_push_closure(fib1, .n = n, .k = k));
    }
}

int cps_closure_function(fib1, fib, int f1)
{
    cps_enter_closure(fib1);
    return fib_cps(pool_, _.n - 2,
                   cps_push_closure(fib2, .f1 = f1, .k = _.k));
}

int cps_closure_function(fib2, fib, int f2)
{
    cps_enter_closure(fib2);
    return cps_call_closure(fib, _.k, f2 + _.f1);
}

int fib(int n)
{
    struct cps_stack stack;
    cps_init_stack(&stack);

    int res = fib_cps(&stack, n, cps_push_closure_to_pool(ret, &stack, ));
    
    assert(stack.sp == 0); // Validation that all is freed.

    cps_free_stack(&stack);

    return res;
}


#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cps.h"

cps_define_closure_type(fib, int, cps_args(int));

cps_define_closure_frame(ret, int, cps_args(int));
int cps_closure_function(ret, int n)
{
    cps_decref_closure(cl_);
    return n;
}

cps_define_closure_frame(fib1, int, cps_args(int),
                         int n;
                         cps_closure k);
cps_define_closure_frame(fib2, int, cps_args(int),
                         int f1;
                         cps_closure k);

int fib_cps(struct cps_memory_pool pool, int n, cps_closure k)
{
    if (n <= 1)
    {
        return cps_call_closure(fib, k, n);
    }
    else
    {
        return fib_cps(pool, n - 1,
                       cps_push_closure_to_pool(fib1, pool, .n = n, .k = k));
    }
}

int cps_closure_function(fib1, int f1)
{
    cps_enter_closure(fib1);
    return fib_cps(cps_memory_pool_from_memory(cl_), _.n - 2,
                   cps_push_closure(fib2, .f1 = f1, .k = _.k));
}

int cps_closure_function(fib2, int f2)
{
    cps_enter_closure(fib2);
    return cps_call_closure(fib, _.k, f2 + _.f1);
}

int fib(int n)
{
    struct cps_stack stack;
    cps_init_stack(&stack);
    struct cps_memory_pool pool = cps_new_memory_pool(CPS_STACK, &stack);
        
    cps_closure k = cps_push_closure_to_pool(ret, pool, );
    int res = fib_cps(pool, n, k);

    assert(stack.sp == 0); // Validation that all is freed.

    cps_free_stack(&stack);

    return res;
}

int test_fib(int n)
{
    if (n <= 1)
        return n;
    return test_fib(n - 1) + test_fib(n - 2);
}

int main(int argc, const char *argv[])
{

    for (int i = 0; i < 15; i++)
    {
        printf("fib(%d) = %d\n", i, fib(i));
        assert(fib(i) == test_fib(i));
    }

    return 0;
}

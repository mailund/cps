//
//  main.c
//  cps-iter
//
//  Created by Thomas Mailund on 25/10/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "closure.h"
#include "stack.h"

int test_fib(int n)
{
    if (n <= 1) return n;
    return test_fib(n - 1) + test_fib(n - 2);
}

int main(int argc, const char * argv[]) {
    
    for (int i = 0; i < 15; i++) {
        printf("fib(%d) -> %d (should be %d)\n", i, fib(i), test_fib(i));
    }
    
    return 0;
}

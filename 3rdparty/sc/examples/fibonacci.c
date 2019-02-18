/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

/*
 * This sample implements a simple fibonacci context whose only purpose is to
 * repeatedly yield the next value in the fibonacci sequence. By repeatedly
 * switching to the fibonacci context, its parent context (the context that
 * created it, in this case the implicitly created main context) will receive
 * the next number in the sequence.
 *
 *   cc -I../include -L../build/lib fibonacci.c -lsc -o fibonacci
*/

#include <sc.h>

#include <stdio.h> /* printf */

static void fibonacci (void* param) {
    unsigned long a = 0;
    unsigned long b = 1;
    unsigned long fib;

    /* Yield the first two fibonacci numbers (0, 1) */
    sc_yield(&a);
    sc_yield(&b);

    /* Infinite loop that yields the rest */
    while (1) {
        fib = a + b;
        a = b;
        b = fib;

        /* By yielding to the parent context, we're switching control back to
           it, and the pointer we pass here will be returned from its call to
           `sc_switch`. */
        sc_yield(&fib);
    }
}

int main(int argc, char** argv) {
    int i;
    void* fib;

    /* Create the fibonacci context. Since the current context will not be
       leaving this function before the end of the fibonacci context's
       lifetime, it is safe to put the new context's stack on this stack. As
       long as it is small enough to not cause a stack overflow.  */
    char stack[SC_MIN_STACK_SIZE];
    sc_context_t context = sc_context_create(stack, sizeof(stack), &fibonacci);

    /* Print the first 10 numbers yielded by the fibonacci sequence
       generator. */
    for (i = 0; i < 10; ++i) {
        /* `sc_yield` returns control to the parent (or creator) context.
           Since the context we want to switch to is not our parent, we must
           instead use `sc_switch`. */
        fib = sc_switch(context, NULL);

        /* As with the stack of the new context, since the fibonacci stack
           still exists, it is safe to directly reference variables on its
           stack. */
        printf("%lu\n", *(unsigned long*)fib);
    }

    /* Clean up. */
    sc_context_destroy(context);
    return 0;
}

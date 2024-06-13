#include "src/MPC_Common/include/mpc_keywords.h"
#include <mpc_arch_context.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static sctk_ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
func1(void)
{
    printf("func1: started\n");
    printf("func1: swapcontext(&uctx_func1, &uctx_main)\n");
    if (mpc__swapcontext(&uctx_func1, &uctx_main) == -1)
        handle_error("swapcontext");
    assert(0);
}

static void
func2(void)
{
    printf("func2: started\n");
    printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
    if (mpc__swapcontext(&uctx_func2, &uctx_func1) == -1)
        handle_error("swapcontext");
    assert(0);
}

int
main(int argc, __UNUSED__ char *argv[])
{
    char func1_stack[16384];
    char func2_stack[16384];

    if (mpc__getcontext(&uctx_func1) == -1)
        handle_error("getcontext");
    uctx_func1.uc_stack.ss_sp = func1_stack;
    uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
    uctx_func1.uc_link = &uctx_main;
    mpc__makecontext(&uctx_func1, func1, 0);

    if (mpc__getcontext(&uctx_func2) == -1)
        handle_error("getcontext");
    uctx_func2.uc_stack.ss_sp = func2_stack;
    uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
    /* Successor context is f1(), unless argc > 1 */
    uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
    mpc__makecontext(&uctx_func2, func2, 0);

    printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
    if (mpc__swapcontext(&uctx_main, &uctx_func2) == -1)
        handle_error("swapcontext");

    printf("main: exiting\n");
    exit(EXIT_SUCCESS);
}

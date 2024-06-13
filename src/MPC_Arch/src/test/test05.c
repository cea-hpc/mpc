#include "src/MPC_Common/include/mpc_keywords.h"
#include <mpc_arch_context.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static sctk_ucontext_t uctx_main, uctx_func1, uctx_func2;

size_t i = 0;
#define MAX_ITER ((size_t)10)*((size_t)1000*1000)

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
func1(void)
{
    while(i < MAX_ITER){
    i++;
    if (mpc__swapcontext(&uctx_func1, &uctx_func2) == -1)
        handle_error("swapcontext");
    }
}

static void
func2(void)
{
    while(i < MAX_ITER){
    i++;
    if (mpc__swapcontext(&uctx_func2, &uctx_func1) == -1)
        handle_error("swapcontext");
    }
}


static double
rrrmpc_arch_get_timestamp_gettimeofday ()
{
  struct timeval tp;
  double res;
  gettimeofday (&tp, NULL);

  res = (double) tp.tv_sec * 1000000.0;
  res += tp.tv_usec;

  return res;
}


int
main(int argc, __UNUSED__ char* argv[])
{
    char func1_stack[16384];
    char func2_stack[16384];

    double start;
    double end;

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
    i++;
    start = rrrmpc_arch_get_timestamp_gettimeofday();
    if (mpc__swapcontext(&uctx_main, &uctx_func2) == -1)
        handle_error("swapcontext");
    end = rrrmpc_arch_get_timestamp_gettimeofday();

    printf("main: exiting average time %fus\n",(end-start)/i);
    exit(EXIT_SUCCESS);
}

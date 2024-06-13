#include <mpc_arch_context.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static sctk_ucontext_t uctx_func1, uctx_main;

size_t i = 0;
#define MAX_ITER ((size_t)10)*((size_t)1000*1000)

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
func1(void)
{
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
main()
{
    char func1_stack[16384];

    double start;
    double end;

    start = rrrmpc_arch_get_timestamp_gettimeofday();

    while(i < MAX_ITER){
    i++;
    if (mpc__getcontext(&uctx_func1) == -1)
        handle_error("getcontext");
    uctx_func1.uc_stack.ss_sp = func1_stack;
    uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
    uctx_func1.uc_link = &uctx_main;
    mpc__makecontext(&uctx_func1, func1, 0);

    if (mpc__swapcontext(&uctx_main, &uctx_func1) == -1)
        handle_error("swapcontext");
    }
    end = rrrmpc_arch_get_timestamp_gettimeofday();

    printf("main: exiting average time %fus\n",(end-start)/i);
    exit(EXIT_SUCCESS);
}

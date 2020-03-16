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
rrrsctk_get_time_stamp ()
{
  struct timeval tp;
  double res;
  gettimeofday (&tp, NULL);

  res = (double) tp.tv_sec * 1000000.0;
  res += tp.tv_usec;

  return res;
}


int
main(int argc, char *argv[])
{
    char func1_stack[16384];

    double start;
    double end;

    start = rrrsctk_get_time_stamp();

    while(i < MAX_ITER){
    i++;
    if (getcontext(&uctx_func1) == -1)
        handle_error("getcontext");
    uctx_func1.uc_stack.ss_sp = func1_stack;
    uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
    uctx_func1.uc_link = &uctx_main;
    makecontext(&uctx_func1, func1, 0);

    if (swapcontext(&uctx_main, &uctx_func1) == -1)
        handle_error("swapcontext");
    }
    end = rrrsctk_get_time_stamp();

    printf("main: exiting average time %fus\n",(end-start)/i);
    exit(EXIT_SUCCESS);
}

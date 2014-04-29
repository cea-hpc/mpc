#include <ucontext.h>

typedef ucontext_t sctk_ucontext_t;

#define mpc__makecontext makecontext
#define mpc__setcontext setcontext
#define mpc__getcontext getcontext
#define mpc__swapcontext swapcontext


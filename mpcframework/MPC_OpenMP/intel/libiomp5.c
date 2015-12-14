#include <stdio.h>
#include <stdlib.h>

extern void abort(void);

static void override()
{
    fprintf(stderr, "You should not be using this library. You should use MPC OpenMP symbols !\n");
    abort();
}

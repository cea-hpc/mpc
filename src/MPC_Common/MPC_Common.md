Module MPC_Common
======================

SUMMARY:
--------

This module contains all basic blocks of MPC framework, common to other modules.

CONTENTS:
---------
* **sctk_optim/**          : longjmp/setjmp (may be moved)
* **sctk_alloc/**          : POSIX binding when allocator is disabled
* **sctk_asm/**            : ASM part of MPC (test\_and\_set, atomics)
* **sctk_debug/**          : Debug interface definition (assert...)
* **libdict/**             :
* **sctk_launch/**         : MPC start definition (main())
* **sctk_datastructures/** : third-party data-structure (UT\*)
* **sctk_basic/**          : Misc. (stdint..)
* **sctk_aio/**            :
* **sctk_topology/**       : hardware topology management
* **sctk_spinlock/**       : lock interfaces for MPC
* **sctk_helpers/**        : MPC helpers (net,I/O...)


COMPONENTS:
-----------

### LIBC & Main() interposition

One challenge for MPC is to replace the application main() function by its own.
Up to now, MPC needed to be compiled with the application, a macro was in charge
or renaming `main` symbol to `mpc_user_main`. The idea is to provide a way to
replace at runtime the `main` symbol with needing a recompilation.

As the `main` symbol cannot be overriden because its address is hard-written in
the `start` function, pre-compiled in `crt1.o`, file appended when the
application is linked. the use of `LD_PRELOAD` is useless. But among other
program loading sequence function, one can be overriden because not contained
in the binary. `__libc_start_main` is a loader function (that is why it is not
included in the binary) and holds as first argument, the pointer to the main
function (actually transmitted by `_start`). The idea of
`sctk_launch/mpc_libc_main.c` is to override this function by preloading a
standalone library (built with MPC).

The library attempts to detect if the current program should be wrapped with
MPC. This has to be done, avoiding MPC to be loaded in each program started
between the command-line and the application (don't wrap grep, ls, cd...). There
is two ways to elect a program:

1. The user specifies through `MPC_USER_BINARY` the name of the binary to wrap.
   For example: `MPC_USER_BINARY="a.out" mpcrun ./a.out` will work.

2. The user defines a pattern of function that should be present in the wrapped
   binary. This can be performed with `MPC_SYMBOL_REGEX`, accepting any kind of
   regular expressions `grep` can use.
   For example: `MPC_SYMBOL_REGEX="MPI_Init" mpcrun ./mpi_helloworld`

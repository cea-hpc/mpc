Module MPC_Threads
======================

SUMMARY:
--------

MPC Threads implementation. Contains also Pthread overriding and multiple
schedulers designed to handle MPC threads.

CONTENTS:
---------
* **include/**     : exposed functions, to be called from other modules
* **sctk\_thread/** : sources for this module

COMPONENTS:
-----------

### TLS

MPC provides runtime mechanisms to reproduce the TLS approach for privatized
variables. The core model is given by the exTLS implementation, initially
located in MPC sources and now distributed as an independent components. ExTLS
can be found in extern-deps.

The privatized variables are considered as multi-level TLS:
* process-level : mimics the global variable scope.
* task-level: variables are global to each task;
* thread-level: variables are local to the calling user-threads;
* openmp-level: variables are in the lowest scope: OpenMP threads;

A functional runtime needs, prior to the execution, a compilation step
translating global variables to the desired TLS level (GCC+linker patches). In
this component, you will only find the runtime support. At runtime, TLS can be
from two kinds:
* dynamically resolved: through \_\_extls\_get_addr\_\* calls.
* statically resolved: trough GS register management;

In order to do that, the MPC TLS support needs to alter one base pointer, used
by exTLS library.

### Thread engine

MPC exposes its own thread scheduler. It means that when an application is built
against MPC, every thread is promoted to be a user-level thread, handled as
input by MPC. Any thread created that way will be referred as a user-level
thread. It includes:
* MPI 'task', evolving in a single MPC/UNIX process
* OpenMP thread, while being a little bit different, there are managed like
  standard user-level threads
* Any thread created by the user (trough pthread interface for instance). Any
  entry point in that interface are caught during runtime and re-routed to the
  user-level interface.

Once this is done, MPC can freely adjust its schedulign policy. There is
currently 3 different methods do span threads over resources :
1. pthread-mode: The most basic one, where each user-level thread is actually a
   real pthread. Note that this approach still takes care to expose right TLS
   levels when needed.
2. ethread:
3. ethread_mxn:

#### pthread

#### ethread\_*

Module MPC_Threads
======================

SUMMARY:
--------

MPC Threads implementation. Contains also Pthread overriding and multiple
schedulers designed to handle MPC threads.

CONTENTS:
---------
* **include/**     : exposed functions, to be called from other modules
* **src/** : sources for this module
* **src/accelerator/** : accelerator specific context-switching
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

### CUDA

CUDA support is provided to MPC for supporting user-level threads inside an
thread-based MPI application. The purpose of this work is to change the CUDA
context pushed on top of the CUDA stack, depending on the running thread.

MPC provides a new compiler wrapper named `mpc_nvcc`, in charge of compiling
application through the `nvcc` compiler but also by taking care of global
variable privatization. Be aware that a compilation through this wrapper can be
longer than with `mpc_cc` or `nvcc` becasue the need of a double compilation.
Actually it is the only way to detect CUDA-internal variables that should not
be disabled. (actually, variable names could be hard-coded, but it is a dirty
way, right ?). The found variable names are de-privatized when building the
application.

On most clusters, runtime libraries is not deployed on both login and compute
nodes. To let applications compiling successfully, a stub library is deployed on
compute nodes ( a `libuda` lib). The real library is then found when the
application is run over the compute nodes. The problem arises when MPC
dependencies (ex: ROMIO,GETOPT...) run their configure and try to execute some
tests (Autotools does such thins a lot). In that case, the mini-test is run on
the login node and tests failed because the lib is not present. To get rid of
this, MPC comes with a pair of options to enable CUDA support :

* `-cuda`: Compile-time option, to inform MPC that: (1) `mpc_nvcc` should be
  used and (2) that our own stub library should be linked to the application.
* `-accl`: Runtime-time option, asking mpcrun to context-switch CUDA context in
  its user-level threads. Without this option, MPC does not take care of CUDA
  environment. But application can still use CUDA by their own.

### OpenACC

Currently, MPC does not provide an OpenACC support (at least from runtime-side).
But the MPC_Accelerators has been designed for future evolution and handlers
have been configured to make the implementation easier.

### OpenCL

Exactly the same as the OpenACC thing.

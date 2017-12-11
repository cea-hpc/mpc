Module MPC_Accelerators
======================

SUMMARY:
--------

This module represent MPC Support of specialized hardware (GPGPU,etc...)

CONTENTS:
---------
* **include/**      : Contains generic header files (included by other modules)
* **src/**          : Global routines for MPC_Accelerators module
* **sctk_cuda/**    : CUDA-specific implementation for MPC (entry point: sctk_cuda.h)
* **sctk_openacc/** : OpenACC handling in MPC (entry point: sctk_openacc.h)
* **sctk_opencl/**  : OpenCL-specific implementation for MPC (entry point sctk_openacc.h)

COMPONENTS:
-----------

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

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
### OpenACC
### OpenCL

/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_CUDA

#include <cuda.h>

/* libcuda.so wrappers...
 * Add here each sctk_* CUDA wrappers to handle a new CUDA driver function
 * Don't forget to add a weak symbol in MPC too (sctk_cuda_wrap.c)
 */
CUresult sctk_cuInit(unsigned int flag) { return cuInit(flag); }
CUresult sctk_cuCtxCreate(CUcontext *c, unsigned int f, CUdevice d) {
  return cuCtxCreate(c, f, d);
}
CUresult sctk_cuCtxPopCurrent(CUcontext *c) { return cuCtxPopCurrent(c); }
CUresult sctk_cuCtxPushCurrent(CUcontext c) { return cuCtxPushCurrent(c); }
CUresult sctk_cuDeviceGetByPCIBusId(CUdevice *d, const char *b) {
  return cuDeviceGetByPCIBusId(d, b);
}

#endif /* MPC_USE_CUDA */

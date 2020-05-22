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

#include <mpc_thread_cuda_wrap.h>
#include <mpc_common_debug.h>


#ifdef MPC_USE_CUDA

/**
 * Weak symbol to let MPC know the cuInit() symbol without linking to it at compile time.
 * The real symbol will be found at run time.
 * @param[in] flag not used in this wrapper
 */
#pragma weak sctk_cuInit
CUresult sctk_cuInit(__UNUSED__ unsigned flag) {
  sctk_cuFatal();
  return -1;
}

/**
 * Weak symbol to let MPC know the cuCtxCreate() symbol without linking to it at compile time.
 * The real symbol will be found at run time.
 * @param[in] flag not used in this wrapper
 */
#pragma weak sctk_cuCtxCreate
CUresult sctk_cuCtxCreate(__UNUSED__ CUcontext *c, __UNUSED__ unsigned int f,__UNUSED__  CUdevice d) {
  sctk_cuFatal();
  return -1;
}

/**
 * Weak symbol to let MPC know the cuCtxPopCurrent() symbol without linking to it at compile time.
 * @param[in] flag not used in this wrapper
 */
#pragma weak sctk_cuCtxPopCurrent
CUresult sctk_cuCtxPopCurrent(__UNUSED__ CUcontext *c) {
  sctk_cuFatal();
  return -1;
}
/**
 * Weak symbol to let MPC know the ctxPushCurrent() symbol without linking to it at compile time.
 * @param[in] flag not used in this wrapper
 */

#pragma weak sctk_cuCtxPushCurrent
CUresult sctk_cuCtxPushCurrent(__UNUSED__ CUcontext c) {
  sctk_cuFatal();
  return -1;
}
/**
 * Weak symbol to let MPC know the cuDeviceGetByPCIBusId() symbol without linking to it at compile time.
 * @param[in] flag not used in this wrapper
 */

#pragma weak sctk_cuDeviceGetByPCIBusId
CUresult sctk_cuDeviceGetByPCIBusId(__UNUSED__ CUdevice *d, __UNUSED__ const char *b) {
  sctk_cuFatal();
  return -1;
}
#endif

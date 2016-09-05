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

#ifndef SCTK_CUDA_WRAP_H
#define SCTK_CUDA_WRAP_H
#include <sctk_debug.h>

/** Global FATAL CUDA routines to catch weak symbol call */
#define sctk_cuFatal()                                                         \
  do {                                                                         \
    sctk_error("You reached fake CUDA %s() call inside MPC", __func__);        \
    sctk_error("Please ensure a valid CUDA driver library is present and you " \
               "provided --cuda option to mpc_* compilers");                   \
    sctk_abort();                                                              \
  } while (0)

#ifdef MPC_USE_CUDA
/* As cuda.h is not included, create some bridges */
#define CUDA_SUCCESS 0
#define CU_CTX_SCHED_YIELD 0x02
typedef int CUresult;    /** CUDA driver return values*/
typedef int CUdevice;    /** CUDA device id (int) */
typedef void *CUcontext; /** CUDA ctx (opaque pointer) */

/* all weak symbols.
 * We should have as many protoptypes as contained in cuda_lib/cuda_lib.c
 */
CUresult sctk_cuInit(unsigned flag);
CUresult sctk_cuCtxCreate(CUcontext *c, unsigned int f, CUdevice d);
CUresult sctk_cuCtxPopCurrent(CUcontext *c);
CUresult sctk_cuCtxPushCurrent(CUcontext c);
CUresult sctk_cuDeviceGetByPCIBusId(CUdevice *d, const char *b);
#endif

#endif

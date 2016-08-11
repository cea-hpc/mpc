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

#ifndef SCTK_CUDA_H
#define SCTK_CUDA_H

#ifdef MPC_USE_CUDA
#include <sctk_cuda_wrap.h>
#include <cuda_runtime.h>
#include <sctk_debug.h>

/** in debug mode, check all CUDA APIs return codes */
#ifndef NDEBUG
#define safe_cudart(u) assume_m(((u) == cudaSuccess), "Runtime CUDA call failed with value %d", u)
#define safe_cudadv(u) assume_m(((u) == CUDA_SUCCESS), "Driver CUDA call failed with value %d", u)
#else
#define safe_cudart(u) u
#define safe_cudadv(u) u
#endif

/**
 * The CUDA context structure as handled by MPC.
 *
 * This structure is part of TLS bundle handled internally by thread context.
 */
typedef struct cuda_ctx_s {
  char pushed;       /**< Set to 1 when the ctx is currently pushed */
  int cpu_id;        /**< Register the cpu_id associated to the CUDA ctx */
  CUcontext context; /**< THE CUDA ctx */
} cuda_ctx_t;

/* CUDA libs init */
int sctk_accl_cuda_init();

/** create a new CUDA context for the current thread */
int sctk_accl_cuda_init_context();
/** Push the CUDA context of the current thread on the elected GPU */
int sctk_accl_cuda_push_context();
/** Remove the current CUDA context from the GPU */
int sctk_accl_cuda_pop_context();

#endif

#endif

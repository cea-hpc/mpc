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
#include <cuda_runtime.h>
#include <sctk_debug.h>
//#define safe_cucall(u) assume_m((u == cudaSuccess), "CUDA call return")

#ifdef NDEBUG
#define safe_cudart(u) assume_m(u == cudaSuccess, "Runtime CUDA call failed with value %d", u)
#define safe_cudadv(u) assume_m(u == CUDA_SUCCESS, "Driver CUDA call failed with value %d", u)
#else
#define safe_cudart(u) u
#define safe_cudadv(u) u
#endif
/* MPC CUDA context */

typedef struct tls_cuda_s{
      int is_ready; //when all thread are ready to use cuda contexts
      int cpu_id; //save cpu_id when context is created 
      CUcontext context; // thread user's cuda context 
} tls_cuda_t;

int sctk_accl_cuda_init(void** ptls_cuda);
int sctk_accl_cuda_push_context(void* tls_cuda);
int sctk_accl_cuda_pop_context(void** ptls_cuda);

#endif

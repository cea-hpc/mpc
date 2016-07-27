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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_TLS_H_
#define __SCTK_TLS_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_context.h"
#include <unistd.h>
#include <sys/syscall.h>

#if defined(MPC_Accelerators)
#include <sctk_accelerators.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif


#if defined(SCTK_USE_TLS)

  struct sctk_tls_dtors_s
  {
	  void * obj;
	  void (*dtor)(void *);
	  struct sctk_tls_dtors_s * next;
  };

  size_t sctk_extls_size();

  int sctk_locate_dynamic_initializers();
  int sctk_call_dynamic_initializers();

#if defined (MPC_Allocator)
  extern __thread struct sctk_alloc_chain * sctk_current_alloc_chain;
#endif


#if defined(MPC_USE_CUDA)
  extern __thread void* sctk_cuda_ctx;
#endif

#if defined (MPC_OpenMP)
  /* MPC OpenMP TLS */
  extern __thread void *sctk_openmp_thread_tls;
#endif

  extern __thread void *sctk_extls_storage;
  extern __thread char *mpc_user_tls_1;
  extern unsigned long mpc_user_tls_1_offset;
  extern unsigned long mpc_user_tls_1_entry_number;

#endif

#ifdef MPC_MPI
  extern __thread struct sctk_thread_specific_s * ___sctk_message_passing;
#endif

#define tls_save(a) ucp->a = a;
#define tls_restore(a) a = ucp->a;
#define tls_init(a) ucp->a = NULL;

  static inline void sctk_context_save_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
#if defined (MPC_PosixAllocator)
    ucp->sctk_current_alloc_chain_local = sctk_current_alloc_chain;
#endif

#if defined (MPC_OpenMP)
    /* MPC OpenMP TLS */
    tls_save(sctk_openmp_thread_tls);
#endif
    tls_save(mpc_user_tls_1);

#ifdef MPC_MPI
    tls_save (___sctk_message_passing);
#endif


#if defined(MPC_USE_CUDA)
	sctk_accl_cuda_pop_context();
	tls_save(sctk_cuda_ctx);
#endif
#endif

	/* the tls vector is restored by copy and cannot be changed
	 * It is then useless to save it at this time
	 */
	ucp->tls_ctx = (extls_ctx_t*)sctk_extls_storage;
	if(ucp->tls_ctx != NULL)
	    extls_ctx_save(ucp->tls_ctx);
  }

  static inline void sctk_context_restore_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
#if defined (MPC_PosixAllocator)
    sctk_current_alloc_chain = ucp->sctk_current_alloc_chain_local;
#endif

#if defined (MPC_OpenMP)
    tls_restore (sctk_openmp_thread_tls);
#endif
    tls_restore(mpc_user_tls_1);

    if(ucp->tls_ctx != NULL)
        extls_ctx_restore(ucp->tls_ctx);

#ifdef MPC_MPI
    tls_restore (___sctk_message_passing);
#endif

#if defined(MPC_USE_CUDA)
	tls_restore(sctk_cuda_ctx);
	if(sctk_cuda_ctx) /* if the thread to be scheduled has an attached cuda ctx: */
	{
		sctk_accl_cuda_push_context();
	}
#endif

#endif
  }

  static inline void sctk_context_init_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
    /* Create a new TLS context, probably not the place it has to be.
	 * more suited to be in sctk_thread.c w/ thread creation.
	 * Moreover, it should use ctx_herit instead of ctx_init (need to reference process level)
	 */
	  /* Nothing should have to be done here. The init is done per thread in sctk_thread.c */
    ucp->tls_ctx = (extls_ctx_t*)sctk_extls_storage;

#if defined (MPC_Allocator)
    ucp->sctk_current_alloc_chain_local = NULL;
#endif

#if defined (MPC_OpenMP)
    /* MPC OpenMP TLS */
    tls_init (sctk_openmp_thread_tls);
#endif

    ucp->mpc_user_tls_1 = mpc_user_tls_1;

#ifdef MPC_MPI
    tls_init (___sctk_message_passing);
#endif

#if defined(MPC_USE_CUDA)
    tls_init(sctk_cuda_ctx);
#endif

#endif
  }

  void sctk_tls_dtors_init(struct sctk_tls_dtors_s ** head);
  void sctk_tls_dtors_add(struct sctk_tls_dtors_s ** head, void * obj, void (*func)(void *));
  void sctk_tls_dtors_free(struct sctk_tls_dtors_s ** head);
  
#ifdef __cplusplus
}
#endif
#endif

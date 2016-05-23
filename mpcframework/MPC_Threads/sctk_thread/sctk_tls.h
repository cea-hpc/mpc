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
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_TLS_H_
#define __SCTK_TLS_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_context.h"
#include <unistd.h>
#include <sys/syscall.h>

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

  void sctk_extls_duplicate (void **new);
  void sctk_extls_keep (int *scopes);
  void sctk_extls_keep_with_specified_extls (void **extls, int *scopes);
  void sctk_extls_delete ();
  size_t sctk_extls_size();

  int sctk_locate_dynamic_initializers();
  int sctk_call_dynamic_initializers();

#if defined (MPC_Allocator)
  extern __thread struct sctk_alloc_chain * sctk_current_alloc_chain;
#endif

#if defined (MPC_OpenMP)
  /* MPC OpenMP TLS */
  extern __thread void *sctk_openmp_thread_tls;
#endif

  extern __thread void *sctk_extls;
  extern __thread void *sctk_extls_storage;

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

#ifdef MPC_MPI
    tls_save (___sctk_message_passing);
#endif

#endif

	/* the tls vector is restored by copy and cannot be changed
	 * It is then uselesse to save it at this time
	 */
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

    if(ucp->tls_ctx != NULL)
        extls_ctx_restore(ucp->tls_ctx);

#ifdef MPC_MPI
    tls_restore (___sctk_message_passing);
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

#ifdef MPC_MPI
    tls_init (___sctk_message_passing);
#endif
#endif
  }


#if defined (SCTK_USE_OPTIMIZED_TLS)
  void sctk_tls_module_set_gs_register ();
  void sctk_tls_module_alloc_and_fill ();
  void sctk_tls_module_alloc_and_fill_in_specified_tls_module_with_specified_extls ( void **_tls_module, void *_extls );
#endif


  void sctk_tls_dtors_init(struct sctk_tls_dtors_s ** head);
  void sctk_tls_dtors_add(struct sctk_tls_dtors_s ** head, void * obj, void (*func)(void *));
 void sctk_tls_dtors_free(struct sctk_tls_dtors_s ** head);
  
#ifdef __cplusplus
}
#endif
#endif

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
#include "sctk_config.h"
#include "sctk_context.h"
#include "sctk_spinlock.h"
#include "sctk_alloc.h"

#ifdef __cplusplus
extern "C"
{
#endif


#if defined(SCTK_USE_TLS)

  typedef enum
  { sctk_extls_process_scope = 0,
    sctk_extls_task_scope    = 1,
    sctk_extls_thread_scope  = 2,
#if defined (MPC_OpenMP)
    sctk_extls_openmp_scope  = 3,
    sctk_extls_max_scope     = 4,
#else
    sctk_extls_max_scope     = 3,
#endif
  } sctk_extls_scope_t;

  typedef enum
  { sctk_hls_node_scope          = 0,
    sctk_hls_numa_level_2_scope  = 1,
    sctk_hls_numa_level_1_scope  = 2,
    sctk_hls_socket_scope        = 3,
    sctk_hls_cache_level_3_scope = 4,
    sctk_hls_cache_level_2_scope = 5,
    sctk_hls_cache_level_1_scope = 6,
    sctk_hls_core_scope          = 7,
    sctk_hls_max_scope           = 8
  } sctk_hls_scope_t;
  /* this numbering should be kept in sync
	 with the argument given to hls_barrier
	 and hls_single in gcc */


  void sctk_extls_duplicate (void **new);
  void sctk_extls_keep (int *scopes);
  void sctk_extls_keep_non_current_thread (void **tls, int *scopes);
  void sctk_extls_delete ();

#if defined (MPC_Allocator)
  extern __thread void *sctk_tls_key;
#endif
#if defined (MPC_Profiler)
  extern __thread void *sctk_tls_trace;
#endif

  extern __thread char *mpc_user_tls_1;
  extern unsigned long mpc_user_tls_1_offset;
  extern unsigned long mpc_user_tls_1_entry_number;
  extern __thread void *sctk_extls;
  //profiling TLS
  extern __thread void *tls_trace_module;
  extern __thread void *tls_args;

  extern __thread void *sctk_hls_generation;

  extern __thread void *sctk_tls_module_vp[sctk_extls_max_scope+sctk_hls_max_scope] ;
  extern __thread void **sctk_tls_module ;

#endif

#ifdef MPC_Message_Passing
  extern __thread struct sctk_thread_specific_s *sctk_message_passing;
#endif

#define tls_save(a) ucp->a = a;
#define tls_restore(a) a = ucp->a;
#define tls_init(a) ucp->a = NULL;

  static inline void sctk_context_save_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
#if defined (MPC_Allocator)
    ucp->sctk_tls_key_local = sctk_tls_key;
#endif
#if defined (MPC_Profiler)
    ucp->sctk_tls_trace_local = sctk_tls_trace;
#endif
    tls_save (mpc_user_tls_1);
    tls_save (sctk_extls);
	tls_save (sctk_hls_generation);
	tls_save (sctk_tls_module);
#ifdef MPC_Message_Passing
    tls_save (sctk_message_passing);
#endif
    //profiling TLS
    tls_save (tls_args);
    tls_save (tls_trace_module);
#endif
  }

  static inline void sctk_context_restore_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
	int i ;
#if defined (MPC_Allocator)
    sctk_tls_key = ucp->sctk_tls_key_local;
#endif
#if defined (MPC_Profiler)
    sctk_tls_trace = ucp->sctk_tls_trace_local;
#endif
    tls_restore (mpc_user_tls_1);
    tls_restore (sctk_extls);
    tls_restore (sctk_hls_generation);
    tls_restore (sctk_tls_module);
	if ( sctk_tls_module != NULL ) {
		for ( i=0; i<sctk_extls_max_scope+sctk_hls_max_scope; ++i )
			sctk_tls_module_vp[i] = sctk_tls_module[i] ;
	}
#ifdef MPC_Message_Passing
    tls_restore (sctk_message_passing);
#endif
    //profiling TLS
    tls_restore (tls_args);
    tls_restore (tls_trace_module);
#endif
  }

  static inline void sctk_context_init_tls_without_extls (sctk_mctx_t *ucp)
  {
#if defined(SCTK_USE_TLS)
#if defined (MPC_Allocator)
    ucp->sctk_tls_key_local = NULL;
#endif
#if defined (MPC_Profiler)
    ucp->sctk_tls_trace_local = NULL;
#endif
    /* tls_init (mpc_user_tls_1); */
    ucp->mpc_user_tls_1 = mpc_user_tls_1 ;
#ifdef MPC_Message_Passing
    tls_init (sctk_message_passing);
#endif
    //profiling TLS
    tls_init (tls_args);
    tls_init (sctk_tls_module);
    tls_init (tls_trace_module);
    tls_init (sctk_hls_generation);
#endif
  }

  static inline void sctk_context_init_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
    tls_init (sctk_extls);
    sctk_extls_duplicate (&(ucp->sctk_extls));
    sctk_context_init_tls_without_extls (ucp);
#endif
  }

  static inline void sctk_context_init_tls_with_specified_extls (sctk_mctx_t * ucp, void * extls)
  {
#if defined(SCTK_USE_TLS)
	ucp->sctk_extls = extls ;
    sctk_context_init_tls_without_extls (ucp);
#endif
  }

  void sctk_hls_build_repository () ;
  void sctk_hls_checkout_on_vp () ;
  void sctk_hls_register_thread () ;

  void sctk_tls_module_set_gs_register ();
  void sctk_tls_module_alloc_and_fill ();


#ifdef __cplusplus
}
#endif
#endif

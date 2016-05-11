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
#include <unistd.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C"
{
#endif


#if defined(SCTK_USE_TLS)

  typedef enum
  { sctk_extls_process_scope = 0,
    sctk_extls_task_scope    = 1,
    sctk_extls_thread_scope  = 2,
    sctk_extls_openmp_scope  = 3,
    sctk_extls_max_scope     = 4,
  } sctk_extls_scope_t;

  typedef enum
  { sctk_hls_node_scope          = 0,
    sctk_hls_numa_level_2_scope  = 1,
    sctk_hls_numa_level_1_scope  = 2,
    sctk_hls_cache_level_3_scope = 3,
    sctk_hls_cache_level_2_scope = 4,
    sctk_hls_cache_level_1_scope = 5,
    sctk_hls_core_scope          = 6,
    sctk_hls_max_scope           = 7
  } sctk_hls_scope_t;
  /* this numbering should be kept in sync
	 with the argument given to hls_barrier
	 and hls_single in gcc */

  struct sctk_tls_dtors_s
  {
	  void * obj;
	  void (*dtor)(void *);
	  struct sctk_tls_dtors * next;
  };

  void sctk_extls_duplicate (void **new);
  void sctk_extls_keep (int *scopes);
  void sctk_extls_keep_with_specified_extls (void **extls, int *scopes);
  void sctk_extls_keep_non_current_thread (void **tls, int *scopes);
  void sctk_extls_delete ();
  size_t sctk_extls_size(); 

#if defined (MPC_Allocator)
  extern __thread struct sctk_alloc_chain * sctk_current_alloc_chain;
#endif
#if defined (MPC_Profiler)
  /* MPC Profiler TLS */
  extern __thread void *tls_mpc_profiler;
#endif
#if defined (MPC_OpenMP)
  /* MPC OpenMP TLS */
  extern __thread void *sctk_openmp_thread_tls;
#endif


  extern __thread char * sctk_optarg;
  extern __thread int sctk_optind;
  extern __thread int sctk_opterr;
  extern __thread int sctk_optopt;
  extern __thread int sctk_optreset;
  extern __thread int sctk_optpos;

  extern __thread char *mpc_user_tls_1;
  extern unsigned long mpc_user_tls_1_offset;
  extern unsigned long mpc_user_tls_1_entry_number;
  extern __thread void *sctk_extls;
  /* MPC Tracelib TLS */
  extern __thread void *tls_trace_module;
  extern __thread void *tls_args;


  extern __thread void *sctk_hls_generation;

#if defined (SCTK_USE_OPTIMIZED_TLS)
  extern __thread void *sctk_tls_module_vp[sctk_extls_max_scope+sctk_hls_max_scope] ;
  extern __thread void **sctk_tls_module ;
#endif

#endif

#ifdef MPC_MPI
  extern __thread struct sctk_thread_specific_s * ___sctk_message_passing;
#endif

#if defined (SCTK_USE_OPTIMIZED_TLS)
  static inline void sctk_context_restore_tls_module_vp () {
	  int i;
	  if ( sctk_tls_module != NULL ) {
		  for ( i=0; i<sctk_extls_max_scope+sctk_hls_max_scope; ++i )
			  sctk_tls_module_vp[i] = sctk_tls_module[i] ;
	  }
  }
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
#if defined (MPC_Profiler)
    /* MPC Profiler TLS */
    tls_save(tls_mpc_profiler);
#endif
#if defined (MPC_OpenMP)
    /* MPC OpenMP TLS */
    tls_save(sctk_openmp_thread_tls);
#endif
    tls_save (mpc_user_tls_1);
    tls_save (sctk_extls);
	tls_save (sctk_hls_generation);
	
#if defined (SCTK_USE_OPTIMIZED_TLS)
	tls_save (sctk_tls_module);
#endif

#ifdef MPC_MPI
    tls_save (___sctk_message_passing);
#endif
    /* MPC Tracelib TLS */
    tls_save (tls_args);
    tls_save (tls_trace_module);
#endif

	tls_save(sctk_optarg);
	tls_save(sctk_optind);
	tls_save(sctk_opterr);
	tls_save(sctk_optopt);
	tls_save(sctk_optreset);
	tls_save(sctk_optpos);
  }

  static inline void sctk_context_restore_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
#if defined (MPC_PosixAllocator)
    sctk_current_alloc_chain = ucp->sctk_current_alloc_chain_local;
#endif
#if defined (MPC_Profiler)
    /* MPC Profiler TLS */
    tls_restore (tls_mpc_profiler);
#endif
#if defined (MPC_OpenMP)
    /* MPC OpenMP TLS */
    tls_restore (sctk_openmp_thread_tls);
#endif
    tls_restore (mpc_user_tls_1);
    tls_restore (sctk_extls);
    tls_restore (sctk_hls_generation);

#if defined (SCTK_USE_OPTIMIZED_TLS)
	tls_restore (sctk_tls_module);
    sctk_context_restore_tls_module_vp () ;
#endif

#ifdef MPC_MPI
    tls_restore (___sctk_message_passing);
#endif
    /* MPC Tracelib TLS */
    tls_restore (tls_args);
    tls_restore (tls_trace_module);
#endif

	tls_restore(sctk_optarg);
	tls_restore(sctk_optind);
	tls_restore(sctk_opterr);
	tls_restore(sctk_optopt);
	tls_restore(sctk_optreset);
	tls_restore(sctk_optpos);
  }

  static inline void sctk_context_init_tls_without_extls (sctk_mctx_t *ucp)
  {
#if defined(SCTK_USE_TLS)
#if defined (MPC_Allocator)
    ucp->sctk_current_alloc_chain_local = NULL;
#endif
#if defined (MPC_Profiler)
    /* MPC Profiler TLS */
    tls_init (tls_mpc_profiler);
#endif
#if defined (MPC_OpenMP)
    /* MPC OpenMP TLS */
    tls_init (sctk_openmp_thread_tls);
#endif
    /* tls_init (mpc_user_tls_1); */
    ucp->mpc_user_tls_1 = mpc_user_tls_1 ;
#ifdef MPC_MPI
    tls_init (___sctk_message_passing);
#endif
    /* MPC Tracelib TLS */
    tls_init (tls_args);
    tls_init (tls_trace_module);

    tls_init (sctk_hls_generation);

#if defined (SCTK_USE_OPTIMIZED_TLS)
    tls_init (sctk_tls_module);
#endif

	ucp->sctk_optarg = NULL;
	ucp->sctk_optind = 1;
	ucp->sctk_opterr = 1;
	ucp->sctk_optopt = 0;
	ucp->sctk_optreset = 0;
	ucp->sctk_optpos= 0;
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

#if defined (SCTK_USE_OPTIMIZED_TLS)
  static inline void sctk_context_init_tls_with_specified_extls (sctk_mctx_t* ucp, void* extls, void* tls_module)
  {
#if defined(SCTK_USE_TLS)
    sctk_context_init_tls_without_extls (ucp);
	ucp->sctk_extls = extls ;
	ucp->sctk_tls_module = tls_module ;
#endif
  }
#else
  static inline void sctk_context_init_tls_with_specified_extls (sctk_mctx_t* ucp, void* extls )
  {
#if defined(SCTK_USE_TLS)
    sctk_context_init_tls_without_extls (ucp);
	ucp->sctk_extls = extls ;
#endif
  }
#endif

  void sctk_hls_build_repository () ;
  void sctk_hls_checkout_on_vp () ;
  void sctk_hls_register_thread () ;

#if defined (SCTK_USE_OPTIMIZED_TLS)
  void sctk_tls_module_set_gs_register ();
  void sctk_tls_module_alloc_and_fill ();
  void sctk_tls_module_alloc_and_fill_in_specified_tls_module_with_specified_extls ( void **_tls_module, void *_extls );
#endif

#ifdef __cplusplus
}
#endif
#endif

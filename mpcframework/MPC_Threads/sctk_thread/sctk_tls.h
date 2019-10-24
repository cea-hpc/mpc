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

	/**
	 * Element of destructor list, stored at task-level.
	 */
  struct sctk_tls_dtors_s
  {
	  void * obj;                    /**< the object address */
	  void (*dtor)(void *);          /**< the associated destructor */
	  struct sctk_tls_dtors_s * next;/**< the next registered item */
  };

  size_t sctk_extls_size();

  int sctk_locate_dynamic_initializers();
  int sctk_call_dynamic_initializers();

#if defined (MPC_Allocator)
  extern __thread struct sctk_alloc_chain * sctk_current_alloc_chain;
#endif

#if defined(MPC_USE_CUDA)
  extern __thread void *sctk_cuda_ctx;
#endif

#if defined (MPC_OpenMP)
  extern __thread void *sctk_openmp_thread_tls;
#endif

#if defined (MPC_Fault_Tolerance)
  extern __thread int sctk_ft_critical_section;
#endif

  extern __thread void *sctk_extls_storage;
  extern __thread char *mpc_user_tls_1;
  extern unsigned long mpc_user_tls_1_offset;
  extern unsigned long mpc_user_tls_1_entry_number;

#endif

#ifdef MPC_MPI
  extern __thread struct mpc_mpi_cl_per_thread_ctx_s * ___mpc_p_per_VP_comm_ctx;
  extern __thread int __mpc_task_rank;
#endif

/** macro copying the global value to the context */
#define tls_save(a) ucp->a = a;
/** macro copying the context value in the current environment */
#define tls_restore(a) a = ucp->a;
/** initialize the context field to NULL */
#define tls_init(a) ucp->a = NULL;

  /**
   * Save the current TLS environment in the given context.
   * This function is called in sctk_context.c.
   * @param[in,out] ucp the context where current data will be stored
   */
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
    tls_save (___mpc_p_per_VP_comm_ctx);
    tls_save(__mpc_task_rank);
#endif

#if defined(MPC_USE_CUDA)
    sctk_accl_cuda_pop_context();
    tls_save(sctk_cuda_ctx);
#endif

#if defined MPC_Fault_Tolerance
    tls_save(sctk_ft_critical_section);
#endif

    /* the tls vector is restored by copy and cannot be changed
     * It is then useless to save it at this time
     */
#ifdef MPC_USE_EXTLS
    ucp->tls_ctx = (extls_ctx_t *)sctk_extls_storage;
    if (ucp->tls_ctx != NULL)
      extls_ctx_save(ucp->tls_ctx);
#endif
#endif /* SCTK_USE_TLS */
  }

  /**
   * Set current TLS to the value found in the given context.
   * Called by sctk_context.c.
   * @param[in] ucp the context containing the data
   */
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

#if defined (MPC_USE_EXTLS)
    if(ucp->tls_ctx != NULL)
        extls_ctx_restore(ucp->tls_ctx);
#endif

#ifdef MPC_MPI
    tls_restore (___mpc_p_per_VP_comm_ctx);
    tls_restore(__mpc_task_rank);
#endif

#if defined MPC_Fault_Tolerance
    tls_restore(sctk_ft_critical_section);
#endif

#if defined(MPC_USE_CUDA)
    tls_restore(sctk_cuda_ctx);
    if (sctk_cuda_ctx) /* if the thread to be scheduled has an attached cuda
                          ctx: */
    {
      sctk_accl_cuda_push_context();
    }
#endif

#endif
  }

  /**
   * Initialize a new TLS context for the associated generic context.
   * Called by sctk_context.c.
   * @param[in,out] ucp the context containing TLS to initialize
   */
  static inline void sctk_context_init_tls (sctk_mctx_t * ucp)
  {
#if defined(SCTK_USE_TLS)
    /* Create a new TLS context, probably not the place it has to be.
	 * more suited to be in sctk_thread.c w/ thread creation.
	 * Moreover, it should use ctx_herit instead of ctx_init (need to reference process level)
	 */
	  /* Nothing should have to be done here. The init is done per thread in sctk_thread.c */
#ifdef MPC_USE_EXTLS
    ucp->tls_ctx = (extls_ctx_t*)sctk_extls_storage;
#endif

#if defined (MPC_Allocator)
    ucp->sctk_current_alloc_chain_local = NULL;
#endif

#if defined (MPC_OpenMP)
    /* MPC OpenMP TLS */
    tls_init (sctk_openmp_thread_tls);
#endif

    ucp->mpc_user_tls_1 = mpc_user_tls_1;

#ifdef MPC_MPI
    tls_init (___mpc_p_per_VP_comm_ctx);
    ucp->__mpc_task_rank = -2;
#endif

#if defined(MPC_USE_CUDA)
    tls_init(sctk_cuda_ctx);
#endif
    
#if defined MPC_Fault_Tolerance
    sctk_ft_critical_section = 0;
#endif

#endif
  }

  void sctk_tls_dtors_init(struct sctk_tls_dtors_s ** head);
  void sctk_tls_dtors_add(struct sctk_tls_dtors_s ** head, void * obj, void (*func)(void *));
  void sctk_tls_dtors_free(struct sctk_tls_dtors_s **head);
  void* sctk_get_ctx_addr(void); /* to be visible from launch.c */

#ifdef __cplusplus
}
#endif
#endif

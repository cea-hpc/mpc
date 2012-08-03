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
#ifndef __SCTK_ALLOC_API_H_
#define __SCTK_ALLOC_API_H_

#include <stdlib.h>
#include "sctk_config.h"


#ifndef MPC_PosixAllocator
#ifdef __cplusplus
extern "C"
{
#endif
  typedef enum
  {
    mpc_alloc_type_init,
    mpc_alloc_type_alloc_block,
    mpc_alloc_type_dealloc_block,
    mpc_alloc_type_alloc_mem,
    mpc_alloc_type_dealloc_mem,
    mpc_alloc_type_enable,
    mpc_alloc_type_disable,
    mpc_alloc_type_show
  } sctk_alloc_type_t;

  typedef sctk_alloc_type_t mpc_alloc_type_t;

  int mpc_alloc_stats_enable (void);
  int mpc_alloc_stats_disable (void);
  int mpc_alloc_stats_show (void);
  size_t mpc_alloc_get_total_memory_allocated (void);
  size_t mpc_alloc_get_total_memory_used (void);
  size_t mpc_alloc_get_total_nb_block (void);
  double mpc_alloc_get_fragmentation (void);
  void mpc_alloc_flush_alloc_buffers_long_life (void);
  void mpc_alloc_flush_alloc_buffers_short_life (void);
  void mpc_alloc_flush_alloc_buffers (void);
  void mpc_alloc_switch_to_short_life_alloc (void);
  void mpc_alloc_switch_to_long_life_alloc (void);
  int mpc_alloc_set_watchpoint (char
				*(*func_watchpoint) (size_t mem_alloc,
						     size_t mem_used,
						     size_t nb_block,
						     size_t size,
						     mpc_alloc_type_t
						     op_type));
  void mpc_dsm_prefetch (void *addr);
  void mpc_enable_dsm (void);
  void mpc_disable_dsm (void);

#ifdef __cplusplus
}
#endif
#else
#include "mpcalloc.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif
  void sctk_flush_alloc_buffers_long_life (void);
  void sctk_flush_alloc_buffers_short_life (void);
  void sctk_flush_alloc_buffers (void);

  void sctk_switch_to_short_life_alloc (void);
  void sctk_switch_to_long_life_alloc (void);

  int sctk_stats_enable (void);
  int sctk_stats_disable (void);

  int sctk_stats_show (void);
  size_t sctk_get_total_memory_allocated (void);
  size_t sctk_get_total_memory_used (void);
  size_t sctk_get_total_nb_block (void);
  double sctk_get_fragmentation (void);
  int sctk_reset_watchpoint (void);

  int sctk_set_watchpoint (char *(*func_watchpoint) (size_t mem_alloc,
						     size_t mem_used,
						     size_t nb_block,
						     size_t size,
						     mpc_alloc_type_t
						     op_type));
  void sctk_dsm_prefetch (void *addr);
  void sctk_enable_dsm (void);
  void sctk_disable_dsm (void);


  void *sctk_user_mmap (void *start, size_t length, int prot, int flags,
			int fd, off_t offset);
  int sctk_user_munmap (void *start, size_t length);
  void *sctk_user_mremap (void *old_address, size_t old_size, size_t new_size,
			  int flags);

#ifdef __cplusplus
}
#endif
#endif

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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_MMSRC_DEFAULT_H
#define SCTK_ALLOC_MMSRC_DEFAULT_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#include "sctk_alloc_lock.h"

/******************************** STRUCTURE *********************************/
/**
 * A simple memory source which use mmap to allocate memory. It will force mmap alignement to a
 * constant size : SCTK_MACRO_BLOC_SIZE to maintain macro bloc header alignement which permit
 * to find them quicly from a specific related chunk.
**/
struct sctk_alloc_mm_source_default
{
	struct sctk_alloc_mm_source source;
	void * heap_addr;
	sctk_size_t heap_size;
	struct sctk_thread_pool pool;
	sctk_alloc_spinlock_t spinlock;
};

/********************************* FUNCTION *********************************/
//default memory source functions
void sctk_alloc_mm_source_default_init(struct sctk_alloc_mm_source_default * source,sctk_addr_t heap_base,sctk_size_t heap_size);
extern struct sctk_alloc_macro_bloc *
sctk_alloc_mm_source_default_request_memory(struct sctk_alloc_mm_source *source,
                                            sctk_ssize_t size);
extern void
sctk_alloc_mm_source_default_free_memory(struct sctk_alloc_mm_source *source,
                                         struct sctk_alloc_macro_bloc *bloc);
extern void
sctk_alloc_mm_source_default_cleanup(struct sctk_alloc_mm_source *source);
extern struct sctk_alloc_macro_bloc *sctk_alloc_get_macro_bloc(void *ptr);
extern void
sctk_alloc_mm_source_insert_segment(struct sctk_alloc_mm_source_default *source,
                                    void *base, sctk_size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_MMSRC_DEFAULT_H */

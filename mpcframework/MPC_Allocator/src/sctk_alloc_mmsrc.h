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

#ifndef SCTK_ALLOC_MMSRC_H
#define SCTK_ALLOC_MMSRC_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#include "sctk_alloc_chunk.h"

/******************************** STRUCTURE *********************************/
/**
 * Define a macro bloc which serve for exchange between the thread pool and the memory source.
 * It reprensent the minimal bloc size which can be requested to the system by the memory source
 * via mmap.
 * Currently, the memory source manage the free list of macro bloc direcly by reusing the thread
 * pool functions, this is why this header start with a standard chunk header.
**/
struct sctk_alloc_macro_bloc
{
	/** Inherit from large header. **/
	struct sctk_alloc_chunk_header_large header;
	/** Pointer to the alloc chain which manage this bloc **/
	struct sctk_alloc_chain * chain;
	/** Some padding to maintain multiples**/
	char padding[8];
};

/******************************** STRUCTURE *********************************/
/**
 * Define a free macro bloc, keep same common header part, but contain extra informations
 * to be placed in free list and to know the mapping state.
**/
struct sctk_alloc_free_macro_bloc
{
	struct sctk_alloc_free_chunk header;
	enum sctk_alloc_mapping_state maping;
};

/******************************** STRUCTURE *********************************/
/**
 * Define a memory source which will be used to manage (and cache) memory macro blocs exchange
 * between the thread pool of the system. It can also be used to provide base for management of
 * a user memory segment.
 * This structure mainly provide function pointers to select the required function implementation
 * for init, request memory, free memory and final cleanup.
**/
struct sctk_alloc_mm_source
{
	struct sctk_alloc_macro_bloc * (*request_memory)(struct sctk_alloc_mm_source * source,sctk_ssize_t size);
	void (*free_memory)(struct sctk_alloc_mm_source * source,struct sctk_alloc_macro_bloc * bloc);
	void (*cleanup)(struct sctk_alloc_mm_source * source);
	struct sctk_alloc_macro_bloc * (*remap)(struct sctk_alloc_macro_bloc * old_macro_bloc,sctk_size_t new_size);
};

/************************* FUNCTION ************************/
//Base of memory source
void sctk_alloc_mm_source_base_init(struct sctk_alloc_mm_source * source);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_MMSRC_H */

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

#ifndef SCTK_ALLOC_REGION_H
#define SCTK_ALLOC_REGION_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"

/******************************** STRUCTURE *********************************/
struct sctk_alloc_region_entry
{
	struct sctk_alloc_macro_bloc * macro_bloc;
};

/******************************** STRUCTURE *********************************/
/**
 * Define an entry from region header. For now it simply contain a pointer to the related allocation
 * chain. NULL if not used.
**/
struct sctk_alloc_region
{
	struct sctk_alloc_region_entry entries[SCTK_REGION_HEADER_ENTRIES];
};

/************************* FUNCTION ************************/
//Region management
struct sctk_alloc_region *sctk_alloc_region_setup(void *addr);
struct sctk_alloc_region *sctk_alloc_region_get(void *addr);
void sctk_alloc_region_del(struct sctk_alloc_region *region);
SCTK_PUBLIC struct sctk_alloc_region_entry * sctk_alloc_region_get_entry(void* addr);
bool sctk_alloc_region_exist(void *addr);
void sctk_alloc_region_init(void);
void sctk_alloc_region_del_all(void);
void sctk_alloc_region_set_entry(struct sctk_alloc_chain *chain,
                                 struct sctk_alloc_macro_bloc *macro_bloc);
int sctk_alloc_region_get_id(void *addr);
bool sctk_alloc_region_has_ref(struct sctk_alloc_region *region);
void sctk_alloc_region_del_chain(struct sctk_alloc_region *region,
                                 struct sctk_alloc_chain *chain);
void sctk_alloc_region_unset_entry(struct sctk_alloc_macro_bloc *macro_bloc);
SCTK_PUBLIC struct sctk_alloc_macro_bloc * sctk_alloc_region_get_macro_bloc(void * ptr);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_REGION_H */

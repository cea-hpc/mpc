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

#ifndef SCTK_ALLOC_POSIX_H
#define SCTK_ALLOC_POSIX_H

/************************** HEADERS ************************/
#include <stdlib.h>
#include "sctk_alloc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************* FUNCTION ************************/
void sctk_alloc_posix_base_init(void);
struct sctk_alloc_chain * sctk_alloc_posix_create_new_tls_chain(void);
void sctk_alloc_posix_set_default_chain(struct sctk_alloc_chain * chain);
struct sctk_alloc_chain * sctk_alloc_posix_setup_tls_chain(void);
sctk_size_t sctk_alloc_posix_get_size(void *ptr);
void sctk_alloc_posix_plug_on_egg_allocator(void);

/************************* FUNCTION ************************/
void sctk_alloc_posix_mmsrc_numa_init_phase_numa(void);
SCTK_PUBLIC void sctk_alloc_posix_numa_migrate(void);
#ifdef MPC_ALLOCATOR_DISABLE_STATIC
SCTK_STATIC void sctk_alloc_posix_mmsrc_uma_init(void);
SCTK_STATIC void sctk_alloc_posix_mmsrc_numa_init(void);
SCTK_STATIC struct sctk_alloc_mm_source* sctk_alloc_posix_get_local_mm_source(void);
#endif

/************************* FUNCTION ************************/
SCTK_PUBLIC void * sctk_calloc (size_t nmemb, size_t size);
SCTK_PUBLIC void * sctk_malloc (size_t size);
SCTK_PUBLIC void sctk_free (void * ptr);
SCTK_PUBLIC void * sctk_realloc (void * ptr, size_t size);
SCTK_PUBLIC int sctk_posix_memalign(void **memptr, size_t boundary, size_t size);
SCTK_PUBLIC void * sctk_memalign(size_t boundary,size_t size);
struct sctk_alloc_chain * sctk_get_current_alloc_chain(void);
void * sctk_realloc_inter_chain (void * ptr, size_t size);
SCTK_STATIC void sctk_alloc_tls_chain();

/************************* FUNCTION ************************/
void sctk_alloc_posix_chain_print_stat(void);

#ifdef MPC_Allocator
	#include "sctk_alloc_to_recode.h"
#endif

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_POSIX_H

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

#ifndef SCTK_ALLOC_ON_NODE_H
#define SCTK_ALLOC_ON_NODE_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#include "sctk_alloc_common.h"

/************************* FUNCTION ************************/
//user entry point
SCTK_PUBLIC void * sctk_malloc_on_node (size_t size, int node);

/************************* FUNCTION ************************/
//internal functions
SCTK_INTERN void sctk_malloc_on_node_init(int numa_nodes);
void *sctk_malloc_on_node_uma(size_t size, int node);
void sctk_malloc_on_node_reset(void);

/************************* FUNCTION ************************/
//optional intern function depend on NUMA status
#ifdef HAVE_HWLOC
void *sctk_malloc_on_node_numa(size_t size, int node);
struct sctk_alloc_chain *sctk_malloc_on_node_get_chain(int node);
#endif //HAVE_HWLOC

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_ON_NODE_H

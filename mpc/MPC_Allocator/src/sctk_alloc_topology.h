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

#ifndef SCTK_ALLOC_TOPOLOGY_H
#define SCTK_ALLOC_TOPOLOGY_H

/************************** HEADERS ************************/
#if !defined(bool) && !defined(__cplusplus)
	#include <stdbool.h>
#endif

//optional header
#ifdef MPC_Threads
#include <sctk_topology.h>
#elif defined(HAVE_LIBNUMA)
#include <hwloc.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
bool sctk_is_numa_node(void);
int sctk_get_numa_node_number (void);
#endif

void sctk_alloc_init_topology(void);
int sctk_get_preferred_numa_node(void);
int sctk_alloc_init_on_numa_node(void);
void sctk_alloc_topology_bind_thread_on_core(int id);

#ifdef HAVE_LIBNUMA
hwloc_topology_t sctk_get_topology_object(void);
void sctk_alloc_migrate_numa_mem(void * addr,sctk_size_t size,int target_numa_node);
#endif //HAVE_LIBNUMA

#ifdef __cplusplus
};
#endif

#endif //SCTK_ALLOC_TOPOLOGY_H

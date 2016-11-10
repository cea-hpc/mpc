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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_CONFIG_H
#define SCTK_ALLOC_CONFIG_H

/************************** HEADERS ************************/
#include "sctk_alloc_common.h"

//select the config header (with or without MPC)
#ifdef MPC_Common
#include "sctk_runtime_config.h"
#else //MPC_Common

#ifdef __cplusplus
extern "C"
{
#endif

/******************************** STRUCTURE *********************************/
/**Options for MPC memory allocator.**/
struct sctk_runtime_config_struct_allocator
{	/**Enable or disable NUMA migration of allocator pages on thread migration.**/
	bool numa_migration;
	/**If the new segment is less than N time smaller than factor, realloc will allocate a new
	 * segment, otherwise it will keep the same one. Use 1 to force realloc every time (may be
	 * slower but consume less memory).**/
	int realloc_factor;
	/**If the new segment is smaller of N bytes than threashold, realloc will allocate a new
	 * segment, otherwise it will keep the same one. Use 0 to force realloc every time (may be
	 * slower but consume less memory).**/
	size_t realloc_threashold;
	/**Permit to enable of disable NUMA support in MPC Allocator.**/
	bool numa;
	/**Define the scope the posix allocator, can be : process | vp | thread.**/
	char * scope;
	/**If true, enable usage of abort() on free error, otherwise try to continue by skipping.**/
	bool strict;
	/**Maximum amount of memory to keep in memory sources (one per NUMA node).**/
	size_t keep_mem;
	/**Maximum size of macro blocs to keep in memory source for reuse.**/
	size_t keep_max;
	/** Print the config after load. **/
	bool print_config;
	bool numa_round_robin;
	int mm_sources;
};
#endif //MPC_Common

/************************* GLOBALS *************************/
#ifndef MPC_Common
extern struct sctk_runtime_config_struct_allocator sctk_alloc_global_config;
#endif //MPC_Common

/************************* FUNCTION ************************/
/**
 * Wrapper to get quick access to the allocator configuration structure.
 * It manage the case with or without MPC to ensure usability out of MPC.
**/
static __inline__ const struct sctk_runtime_config_struct_allocator * sctk_alloc_config(void)
{
	#ifdef MPC_Common
	return &sctk_runtime_config_get_nocheck()->modules.allocator;
	#else //MPC_Common
	return &sctk_alloc_global_config;
	#endif //MPC_Common
}

/************************* FUNCTION ************************/
void sctk_alloc_config_init(void);
void sctk_alloc_config_egg_init(void);
void sctk_alloc_config_init_static_defaults(
    struct sctk_runtime_config_struct_allocator *config);

#ifdef __cplusplus
};
#endif

#endif //SCTK_ALLOC_CONFIG_H

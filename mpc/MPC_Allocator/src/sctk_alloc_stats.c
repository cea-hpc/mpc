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

/************************** HEADERS ************************/
#include "sctk_alloc_stats.h"
#include "sctk_alloc_debug.h"

/************************* FUNCTION ************************/
/**
 * Permit to init the stat module related to an allocation chain.
 * @param chain_stats Define the module to init.
**/
void sctk_alloc_stats_chain_init(struct sctk_alloc_stats_chain* chain_stats)
{
	//check errors
	assert(chain_stats != NULL);

	//init values
	chain_stats->nb_macro_blocs    = 0;
	chain_stats->allocated_memory  = 0;
}

/************************* FUNCTION ************************/
/**
 * Method to call when deleting an allocation chain. For now it done nothing, but in future
 * it may be used to remove chain registration to global stat list for dynamic tracking.
 * @param chain_stats Define the module to delete.
**/
void sctk_alloc_stats_chain_destroy(struct sctk_alloc_stats_chain* chain_stats)
{
}

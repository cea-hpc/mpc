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

#ifndef SCTK_ALLOC_STATS_H
#define SCTK_ALLOC_STATS_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/

/************************** CONSTS *************************/

/************************** MACROS *************************/
#ifndef SCTK_ALLOC_STATS
#define SCTK_ALLOC_STATS_HOOK(x) x
#define SCTK_ALLOC_STATS_HOOK_INC(x) (x++)
#define SCTK_ALLOC_STATS_HOOK_DEC(x) (x--)
#else //SCTK_ALLOC_STAT
#define SCTK_ALLOC_STATS_HOOK(x) while(0){}
#define SCTK_ALLOC_STATS_HOOK_INC(x) while(0){}
#define SCTK_ALLOC_STATS_HOOK_DEC(x) while(0){}
#endif //SCTK_ALLOC_STAT

/************************** STRUCT *************************/
/**
 * Struct to store stats from each allocation chain.
**/
struct sctk_alloc_stats_chain
{
	/** Number of macro blocs allocated to the current allocation chain. **/
	unsigned long nb_macro_blocs;
	/** Memory size currently allocated an managed by the current allocation chain. **/
	unsigned long allocated_memory;
};

/************************* FUNCTION ************************/
void sctk_alloc_stats_chain_init(struct sctk_alloc_stats_chain * chain_stats);
void sctk_alloc_stats_chain_destroy(struct sctk_alloc_stats_chain * chain_stats);


#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_STATS_H


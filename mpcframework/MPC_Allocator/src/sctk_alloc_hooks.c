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

/********************************* INCLUDES *********************************/
#include <string.h>
#include "sctk_alloc_chunk.h"
#include "sctk_alloc_hooks.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_chain.h"
#include "sctk_alloc_inlined.h"

/********************************** GLOBALS *********************************/
#ifdef ENABLE_ALLOC_HOOKS
/**
 * Global hooks pointer.
 */
struct sctk_alloc_hooks sctk_alloc_gbl_hooks = {0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
#endif /* SCTK_ALLOC_HOOKS */

/********************************* FUNCTION *********************************/
/**
 * Default function to init the allocator hooks to NULL. This function is defined to be overloaded
 * by instrumentation libraries.
 * @param hooks Define the structure to update with function pointers.
 */
void sctk_alloc_hooks_init(struct sctk_alloc_hooks * hooks)
{
	//errors
	assert(hooks != NULL);
	assert(hooks->was_init == 0);

	memset(hooks,0,sizeof(*hooks));

	//define it as init
	hooks->was_init = 1;
}

/********************************* FUNCTION *********************************/
/**
 * As we cannot get access to the chain structure, this function permit to fetch the user data
 * member attached to the chain.
 * @param chain Define the chain from which to read the user data.
 */
void * sctk_alloc_hooks_get_user_data(__AL_UNUSED__ struct sctk_alloc_chain * chain)
{
	#ifdef ENABLE_ALLOC_HOOKS
		return chain->hook_user_data;
	#else
		return NULL;
	#endif
}

/********************************* FUNCTION *********************************/
/**
 * Permit to setup the user data member withour knowing the internal of the struct.
 * @param chain The chain on which to setup the user data.
 * @param data The user data to register on the chain.
 */
void sctk_alloc_hooks_set_user_data(__AL_UNUSED__ struct sctk_alloc_chain * chain, __AL_UNUSED__ void * data)
{
	#ifdef ENABLE_ALLOC_HOOKS
	chain->hook_user_data = data;
	#endif
}

/********************************* FUNCTION *********************************/
/**
 * Wrapper function to access to the macro bloc size without knowing the internals of the struct.
 */
size_t sctk_alloc_hooks_get_macro_bloc_size(struct sctk_alloc_macro_bloc * bloc)
{
	//TODO FIX THIS FOR WINDOWS
	return sctk_alloc_get_chunk_header_large_size(&bloc->header);
}

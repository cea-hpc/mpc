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


#ifndef SCTK_ALLOC_HOOKS_H
#define SCTK_ALLOC_HOOKS_H

/********************************* INCLUDES *********************************/
#include <stdlib.h>

/********************************* MACROS ***********************************/
/**
 * Enable notification of hooks to instrument the allocator.
**/
#ifdef ENABLE_ALLOC_HOOKS
	#define SCTK_ALLOC_HOOK(event_name,...) do {if (sctk_alloc_gbl_hooks.event_##event_name != NULL) sctk_alloc_gbl_hooks.event_##event_name(__VA_ARGS__); } while(0)
	#define SCTK_ALLOC_COND_HOOK(condition,event_name,...) do {if (sctk_alloc_gbl_hooks.event_##event_name != NULL && (condition)) sctk_alloc_gbl_hooks.event_##event_name(__VA_ARGS__); } while(0)
	#define SCTK_ALLOC_HAS_HOOK(event_name) (sctk_alloc_gbl_hooks.event_##event_name != NULL)
#else //ENABLE_ALLOC_HOOKS
	//we define with "while(0)..." because VCC doesn't support two semicolons.
	#define SCTK_ALLOC_HOOK(x,...) do {} while(0)
	#define SCTK_ALLOC_COND_HOOK(x,y,...) do {} while(0)
	#define SCTK_ALLOC_HAS_HOOK(x) 0
#endif //ENABLE_ALLOC_HOOKS

/******************************** STRUCTURE *********************************/
struct sctk_alloc_chain;
struct sctk_alloc_macro_bloc;

/********************************** TYPES ***********************************/
typedef void (*sctk_alloc_hook_chain_init)(struct sctk_alloc_chain * chain);
typedef void (*sctk_alloc_hook_chain_destroy)(struct sctk_alloc_chain * chain);
typedef void (*sctk_alloc_hook_chain_add_macro_bloc)(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * bloc);
typedef void (*sctk_alloc_hook_chain_free_macro_bloc)(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * bloc);
typedef void (*sctk_alloc_hook_chain_remote_free)(struct sctk_alloc_chain * local_chain,struct sctk_alloc_chain * distant_chain,void * ptr);
typedef void (*sctk_alloc_hook_chain_flush_rfq)(struct sctk_alloc_chain * chain,int nb_entries);
typedef void (*sctk_alloc_hook_chain_flush_rfq_entry)(struct sctk_alloc_chain * chain,void * ptr);
typedef void (*sctk_alloc_hook_chain_alloc)(struct sctk_alloc_chain * chain,size_t request,void * ptr,size_t size,size_t boundary);
typedef void (*sctk_alloc_hook_chain_free)(struct sctk_alloc_chain * chain,void * ptr,size_t size);
typedef void (*sctk_alloc_hook_chain_huge_alloc)(struct sctk_alloc_chain * chain,size_t request,void * ptr,size_t size,size_t boundary);
typedef void (*sctk_alloc_hook_chain_huge_free)(struct sctk_alloc_chain * chain,void * ptr,size_t size);
typedef void (*sctk_alloc_hook_chain_split)(struct sctk_alloc_chain * chain,void * base_addr,size_t chunk_size,size_t residut_size);
typedef void (*sctk_alloc_hook_chain_merge)(struct sctk_alloc_chain * chain, void * base_addr,size_t size);
typedef void (*sctk_alloc_hook_chain_next_is_realloc)(struct sctk_alloc_chain * chain, void * orig_ptr,size_t new_requested_size);

/******************************** STRUCTURE *********************************/
struct sctk_alloc_hooks
{
	int was_init;
	sctk_alloc_hook_chain_init event_chain_init;
	sctk_alloc_hook_chain_destroy event_chain_destroy;
	sctk_alloc_hook_chain_add_macro_bloc event_chain_add_macro_bloc;
	sctk_alloc_hook_chain_free_macro_bloc event_chain_free_macro_bloc;
	sctk_alloc_hook_chain_remote_free event_chain_remote_free;
	sctk_alloc_hook_chain_flush_rfq event_chain_flush_rfq;
	sctk_alloc_hook_chain_flush_rfq_entry event_chain_flush_rfq_entry;
	sctk_alloc_hook_chain_alloc event_chain_alloc;
	sctk_alloc_hook_chain_free event_chain_free;
	sctk_alloc_hook_chain_huge_alloc event_chain_huge_alloc;
	sctk_alloc_hook_chain_huge_free event_chain_huge_free;
	sctk_alloc_hook_chain_split event_chain_split;
	sctk_alloc_hook_chain_merge event_chain_merge;
	sctk_alloc_hook_chain_next_is_realloc event_chain_next_is_realloc;
};

/********************************** GLOBALS *********************************/
#ifdef ENABLE_ALLOC_HOOKS
extern struct sctk_alloc_hooks sctk_alloc_gbl_hooks;
#endif /* SCTK_ALLOC_HOOKS */

/********************************* FUNCTION *********************************/
void sctk_alloc_hooks_init(struct sctk_alloc_hooks * hooks);

/********************************* FUNCTION *********************************/
//tmp functions to remove when we got cleaner headers
void * sctk_alloc_hooks_get_user_data(struct sctk_alloc_chain * chain);
void sctk_alloc_hooks_set_user_data(struct sctk_alloc_chain * chain,void * data);
size_t sctk_alloc_hoks_get_macro_bloc_size(struct sctk_alloc_macro_bloc * bloc);

#endif /* SCTK_ALLOC_HOOKS_H */

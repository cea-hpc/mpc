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
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include "sctk_alloc_config.h"

#ifdef MPC_Common
#include "sctk_runtime_config_struct_defaults.h"
#endif //MPC_Common

/************************* GLOBALS *************************/
#ifndef MPC_Common
struct sctk_runtime_config_struct_allocator sctk_alloc_global_config;
#endif //MPC_Common

/************************* FUNCTION ************************/
/**
 * !!!! CAUTION !!!! This method has no allocator, so it musn't do any allocation.
 * @TODO find a way to avoid to recode this manually, import the one from MPC.
**/
SCTK_STATIC void sctk_alloc_config_init_static_defaults(struct sctk_runtime_config_struct_allocator * config)
{
	config->strict             = false;
	config->numa_migration     = false;
	config->realloc_factor     = 2;
	config->realloc_threashold = 50*1024*1024;//50MB
	config->keep_max           = 8*1024*1024;//8MB
	config->keep_mem           = 512*1024*1014;//512MB
}

/************************* FUNCTION ************************/
/**
 * This method is called to force config initialization just after egg_allocator init steps.
 * It carn perform allocation, so we can use the MPC config system here.
**/
#ifdef MPC_Common
void sctk_alloc_config_init(void)
{
	sctk_runtime_config_init();
}
#else //MPC_Common
void sctk_alloc_config_init(void)
{
	//nothing to do until we want to use getenv,
	//sctk_alloc_config_init_static_defaults already done the job
}
#endif //MPC_Common

/************************* FUNCTION ************************/
/**
 * This init method is used before egg allocator, it must not perform any allocation
 * as allocator is not ready when calling this. It must only put default fixed
 * values.
 * Values will be erased after egg initialization with final user values. This is
 * why we must only use parameters which can be replaced on fly or which didn't impact
 * egg_allocator.
**/
#ifdef MPC_Common
void sctk_alloc_config_egg_init(void)
{
	//TODO : Damn, with MPC, we may use sctk_runtime_config_struct_init_allocator() but can't due to
	//usage of strdup in sctk_runtime_config_map_entry_parse_size(), may be we can avoid this
	sctk_alloc_config_init_static_defaults(&__sctk_global_runtime_config__.modules.allocator);
}
#else //MPC_Common
void sctk_alloc_config_egg_init(void)
{
	//TODO : Damn, with MPC, we may use sctk_runtime_config_struct_init_allocator() but can't due to
	//usage of strdup in sctk_runtime_config_map_entry_parse_size(), may be we can avoid this
	sctk_alloc_config_init_static_defaults(&sctk_alloc_global_config);
}
#endif //MPC_Common

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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_RUNTIME_CONFIG_H
#define SCTK_RUNTIME_CONFIG_H

/********************************* INCLUDES *********************************/
#include <mpc_config_struct.h>
#include <mpc_keywords.h>
/********************************** GLOBALS *********************************/
/**
 * Global variable to store mpc runtime configuration loaded from XML.
 * Caution for quick static access, prefer usage of macro sctk_runtime_config_get().
**/
extern struct sctk_runtime_config __sctk_global_runtime_config__;
/** To know if already init. **/
extern bool __sctk_global_runtime_config_init__;
/** To avoid crash on symbol load when running mpc_print_config (mpc not linked). **/
extern bool sctk_crash_on_symbol_load;

/******************************** FUNCTION *********************************/
void sctk_runtime_config_init(void);
void sctk_runtime_config_runtime_display(void);

/******************************** FUNCTION *********************************/
/**
 * Return true if MPC configuration structure was initialized, false otherwise.
**/
static inline bool sctk_runtime_config_init_done(void)
{
	return __sctk_global_runtime_config_init__;
}

/******************************** FUNCTION *********************************/
/**
 * Function to use to get access to the configuration structure of MPC.
 * CAUTION, you must ensure that sctk_runtime_config_init() have been called before using
 * this function.
**/
static inline const struct sctk_runtime_config * sctk_runtime_config_get(void) {
	/* ensure we have called sctk_runtime_config_init */
	/* ok can return directly, so normally compiler will inline it and directly
	 * use the static address */
	return &__sctk_global_runtime_config__;
}

/******************************** FUNCTION *********************************/
/**
 * Function to use to get access to the configuration structure of MPC. It ensure that it was initialized
 * when the function return.
**/
static inline const struct sctk_runtime_config * sctk_runtime_config_get_checked(void) {
	if ( ! __sctk_global_runtime_config_init__ )
		sctk_runtime_config_init();

	return &__sctk_global_runtime_config__;
}

/******************************** FUNCTION *********************************/
/**
 * Function to use to get access to the configuration structure of MPC. It never check init state.
 * This function is DANGEROUS but required for allocator due to bootstrap issues.
 * Please AVOID to use it except in unsolvable situations.
**/
static inline const struct sctk_runtime_config * sctk_runtime_config_get_nocheck(void) {
	return &__sctk_global_runtime_config__;
}


/** Function used to register the MPIT variable registration callback */
void sctk_runtime_config_mpit_bind_variable_set_trampoline( void (*trampoline)(char *, size_t, void *));

#endif /*SCTK_RUNTIME_CONFIG_H*/

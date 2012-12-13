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

/********************************* INCLUDES *********************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sctk_debug.h>
#include "sctk_runtime_config_struct.h"
#include "sctk_runtime_config_validation.h"
#include "sctk_runtime_config_struct_defaults.h"

/********************************* FUNCTION *********************************/
/**
 * This function is called after loading the config to do more complexe validation on some values.
 * Create functions for each of you modules and call them here.
 * CAUTION, you must not depend on you module here as it was called at first initialisation step
 * before main and before the complete allocator initialisation.
**/
void sctk_runtime_config_validate(struct sctk_runtime_config * config)
{
	/* errors */
	assert(config != NULL);

	/* debug message */
	sctk_nodebug("Validator called on config...");

	/* call all post actions */
	sctk_runtime_config_old_getenv_compatibility(config);
	sctk_runtime_config_override_by_getenv(config);
	sctk_runtime_config_validate_allocator(config);
}

/********************************* FUNCTION *********************************/
/**
 * Compatility with old getenv() system. Some may be removed soon but
 * need some discutions about that.
 * @todo Cleanup non needed old support.
**/
void sctk_runtime_config_old_getenv_compatibility(struct sctk_runtime_config * config)
{
	/* vars */
	char * tmp;

	/* came from sctk_launch.c for banner disabling, also used into mpcomp.c
	   and sctk_thread.c */
	if (getenv ("MPC_DISABLE_BANNER") != NULL)
		config->modules.launcher.banner = false;
	//came from sctk_launch.c
	if ((tmp = getenv ("MPC_AUTO_KILL_TIMEOUT")) != NULL)
		config->modules.launcher.autokill = atoi(tmp);
}

/********************************* FUNCTION *********************************/
/**
 * This function may be used to defined all value overriding by environment
 * variables.
**/
void sctk_runtime_config_override_by_getenv(struct sctk_runtime_config * config)
{
	/* vars */
	char * tmp;

	/* User directory where to search extra launchers for mpcrun -l=... */
	if ((tmp = getenv("MPC_USER_LAUNCHERS")) != NULL)
		config->modules.launcher.user_launchers = tmp;
}

/********************************* FUNCTION *********************************/
/**
 * Function to validate MPC_Allocator configuration.
**/
void sctk_runtime_config_validate_allocator(struct sctk_runtime_config * config)
{
	/* vars */
	//const char * scope = config->modules.allocator.scope;

	/* check some integer options */
	/** @TODO Maybe add constrain checker in config-meta.xml format (field constrain='...') **/
	assume_m(config->modules.allocator.realloc_factor >= 1,
	         "Option modules.allocator.realloc_factor must be greater or equal to 1, get %d.",
	         config->modules.allocator.realloc_factor);

	/* check scope option */
	/* TODO re-enable this when implemented */
	/*assume_m(scope != NULL,"Invalid NULL or empty value for allocator.scope.");
	if (strcmp(scope,"process") != 0 && strcmp(scope,"thread") != 0 && strcmp(scope,"vp") != 0)
		sctk_fatal("Invalid configuration value for allocator.scope : %s, require (process | thread | vp)",scope);*/
}


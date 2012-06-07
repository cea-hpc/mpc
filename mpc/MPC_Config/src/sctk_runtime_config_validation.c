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

/********************  HEADERS  *********************/
#include "sctk_debug.h"
#include "sctk_runtime_config_struct.h"
#include "sctk_runtime_config_validation.h"
#include "sctk_runtime_config_struct_defaults.h"

/*******************  FUNCTION  *********************/
/**
 * This function is called after loading the config to do more complexe validation on some values.
 * Create functions for each of you modules and call them here.
 * CAUTION, you must not depend on you module here as it was called at first initialisation step
 * before main and before the complete allocator initialisation.
**/
void sctk_runtime_config_validate(struct sctk_runtime_config * config)
{
	sctk_debug("Validator called on config...\n");
	//sctk_runtime_config_validate_example(config);
}

/*******************  FUNCTION  *********************/
/**
 * This is a validator example.
**/
void sctk_runtime_config_validate_example(struct sctk_runtime_config * config)
{
	assume_m(config != NULL,"Config cannot be NULL.");
}

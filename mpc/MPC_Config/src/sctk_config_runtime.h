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

#ifndef SCTK_CONFIG_RUNTIME
#define SCTK_CONFIG_RUNTIME

/********************  HEADERS  *********************/
#include "sctk_config_struct.h"

/*********************  GLOBAL  *********************/
/**
 * Global variable to store mpc runtime configuration loaded from XML.
 * Caution for quick static access, prefer usage of macro sctk_config_runtime_get_fast().
**/
extern struct sctk_config __sctk_global_config_runtime__;
/** To know if aldready init. **/
extern bool __sctk_global_config_init__;


/*******************  FUNCTION  *********************/
void sctk_config_runtime_init(void);
void sctk_config_runtime_display(struct sctk_config * config);

/*******************  FUNCTION  *********************/
static inline bool sctk_config_runtime_init_done(void)
{
	return __sctk_global_config_init__;
}

/*******************  FUNCTION  *********************/
static inline const struct sctk_config * sctk_config_runtime_get(void)
{
	if ( ! __sctk_global_config_init__ )
		sctk_config_runtime_init();

	return &__sctk_global_config_runtime__;
}

#endif

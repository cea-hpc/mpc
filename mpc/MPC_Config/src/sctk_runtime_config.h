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

#ifndef SCTK_RUNTIME_CONFIG
#define SCTK_RUNTIME_CONFIG

/********************  HEADERS  *********************/
#include "sctk_runtime_config_struct.h"
#include "sctk_runtime_config.h"

/*********************  GLOBAL  *********************/
/**
 * Global variable to store mpc runtime configuration loaded from XML.
 * Caution for quick static access, prefer usage of macro sctk_runtime_config_get().
**/
extern struct sctk_runtime_config __sctk_global_runtime_config__;
/** To know if aldready init. **/
extern bool __sctk_global_runtime_config_init__;


/*******************  FUNCTION  *********************/
void sctk_runtime_config_init(void);
void sctk_runtime_config_runtime_display(struct sctk_runtime_config * config);

/*******************  FUNCTION  *********************/
static inline bool sctk_runtime_config_init_done(void)
{
	return __sctk_global_runtime_config_init__;
}

/*******************  FUNCTION  *********************/
static inline const struct sctk_runtime_config * sctk_config_runtime_get(void)
{
	if ( ! __sctk_global_runtime_config_init__ )
		sctk_runtime_config_init();

	return &__sctk_global_runtime_config__;
}

#endif

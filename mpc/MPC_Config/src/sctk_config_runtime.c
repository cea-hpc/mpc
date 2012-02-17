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
#include <assert.h>
#include "sctk_config_debug.h"
#include "sctk_config_runtime.h"
#include "sctk_config_sources.h"
#include "sctk_config_mapper.h"
#include "sctk_config_printer.h"

/*********************  GLOBAL  *********************/
/**
 * Global variable to store mpc runtime configuration loaded from XML.
**/
struct sctk_config __sctk_global_config_runtime__;
/** To know if aldready init. **/
bool __sctk_global_config_init__ = false;

/*******************  FUNCTION  *********************/
void sctk_config_runtime_init(void)
{
	//declare temp storage
	struct sctk_config_sources config_sources;

	//if already done do nothing
	if (__sctk_global_config_init__ == false)
	{
		//init libxml (safer to manually call it in multi-thread environnement as not threadsafe)
		xmlInitParser();

		//open
		sctk_config_sources_open(&config_sources,NULL);

		//map to c struct
		//sctk_config_reset(&config);
		sctk_config_map_sources_to_c_struct(&__sctk_global_config_runtime__,&config_sources);

		//close
		sctk_config_sources_close(&config_sources);

		//display
		sctk_config_display(&__sctk_global_config_runtime__);

		//mark as init
		__sctk_global_config_init__ = true;
	}
}

/*******************  FUNCTION  *********************/
void sctk_config_runtime_cleanup(void)
{
	//ceanlup struct
	sctk_config_do_cleanup(&__sctk_global_config_runtime__);
}

#endif

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
/* #   - AUTOMATIC GENERATION                                             # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdbool.h>
#include "sctk_runtime_config_struct_defaults.h"

#ifndef SCTK_RUNTIME_CONFIG_STRUCT_H
#define SCTK_RUNTIME_CONFIG_STRUCT_H

/*********************  STRUCT  *********************/
/****/
struct sctk_runtime_config_module_test
{	/****/
	int tt;
};

/*********************  STRUCT  *********************/
/****/
struct sctk_runtime_config_module_rail
{	/****/
	int tt;
	/****/
	int tt2;
};

/*********************  STRUCT  *********************/
/**Options for MPC allocator.**/
struct sctk_runtime_config_module_allocator
{	/**Permit to enable of disable NUMA support in MPC Allocator.**/
	bool numa;
	/**Enable of disable usage of allocator profiler.**/
	bool profile;
	/**Sample string**/
	char * alstring;
	/**Sample double**/
	double aldouble;
	/**Sample float**/
	float alfloat;
	/**Permit to enable of disable allocator warnings.**/
	bool warnings;
	/****/
	struct sctk_runtime_config_module_test test;
	/**Permit to list all available size classes.**/
	int * classes;
	/** Number of elements in classes array. **/
	int classes_size;
	/**Permit to list all available size classes.**/
	int * classes2;
	/** Number of elements in classes2 array. **/
	int classes2_size;
	/**blablabla**/
	struct sctk_runtime_config_module_rail * rails;
	/** Number of elements in rails array. **/
	int rails_size;
};

/*********************  STRUCT  *********************/
/**Options for MPC launcher.**/
struct sctk_runtime_config_module_launcher
{	/**blabla**/
	bool smt;
	/**blabla**/
	int cores;
	/**blabla**/
	int verbosity;
};

/*********************  STRUCT  *********************/
struct sctk_runtime_config_modules
{
	struct sctk_runtime_config_module_allocator allocator;
	struct sctk_runtime_config_module_launcher launcher;
};

/*********************  STRUCT  *********************/
struct sctk_runtime_config
{
	struct sctk_runtime_config_modules modules;
};

#endif // SCTK_RUNTIME_CONFIG_STRUCT_H


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

#include <sctk_bool.h>
#include "sctk_runtime_config_struct_defaults.h"

#ifndef SCTK_RUNTIME_CONFIG_STRUCT_H
#define SCTK_RUNTIME_CONFIG_STRUCT_H

/******************************** STRUCTURE *********************************/
/**Options for MPC memory allocator.**/
struct sctk_runtime_config_struct_allocator
{	/**Enable or disable NUMA migration of allocator pages on thread migration.**/
	bool numa_migration;
	/**If the new segment is less than N time smaller than factor, realloc will allocate a new segment, otherwise it will keep the same one. Use 1 to force realloc every time (may be slower but consume less memory).**/
	int realloc_factor;
	/**If the new segment is smaller of N bytes than threashold, realloc will allocate a new segment, otherwise it will keep the same one. Use 0 to force realloc every time (may be slower but consume less memory).**/
	size_t realloc_threashold;
	/**Permit to enable of disable NUMA support in MPC Allocator.**/
	bool numa;
	/**If true, enable usage of abort() on free error, otherwise try to continue by skipping.**/
	bool strict;
	/**Maximum amount of memory to keep in memory sources (one per NUMA node). Use 0 to disable cache, huge value to keep all.**/
	size_t keep_mem;
	/**Maximum size of macro blocs to keep in memory source for reuse. Use 0 to disable cache, huge value to keep all.**/
	size_t keep_max;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC launcher.**/
struct sctk_runtime_config_struct_launcher
{	/**Enable usage of hyperthreaded cores if available on current architecture.**/
	bool smt;
	/**Default number of cores if -c=X is not given to mpcrun.**/
	int cores;
	/**Default verbosity level from 0 to 3. Can be override by -vv on mpcrun.**/
	int verbosity;
	/**Display the MPC banner at launch time to print some informations about the topology. Can be override by MPC_DISABLE_BANNER.**/
	bool banner;
	/**Automatically kill the MPC processes after a given timeout. Use 0 to disable. Can be override by MPC_AUTO_KILL_TIMEOUT.**/
	int autokill;
	/**Permit to extend the launchers available via 'mpcrun -l=...' by providing scripts (named mpcrun_XXXX) in a user directory. Can be override by MPC_USER_LAUNCHERS.**/
	char * user_launchers;
};

/******************************** STRUCTURE *********************************/
/**Options for the internal MPC Profiler**/
struct sctk_runtime_config_struct_profiler
{	/**Prefix of MPC Profiler outputs**/
	char * file_prefix;
	/**Add a timestamp to profiles file names**/
	bool append_date;
	/**Profile in color when outputed to stdout**/
	bool color_stdout;
	/**Color for levels of profiler output**/
	char * * level_colors;
	/** Number of elements in level_colors array. **/
	int level_colors_size;
};

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config_modules
{
	struct sctk_runtime_config_struct_allocator allocator;
	struct sctk_runtime_config_struct_launcher launcher;
	struct sctk_runtime_config_struct_profiler profiler;
};

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config
{
	struct sctk_runtime_config_modules modules;
};

#endif /* SCTK_RUNTIME_CONFIG_STRUCT_H */


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

/*********************  STRUCT  *********************/
/**Options for MPC memory allocator (TODO not used up to now, need to be integrated).**/
struct sctk_runtime_config_struct_allocator
{	/**Enable of disable NUMA migration of allocator pages on thread migration.**/
	bool numa_migration;
	/**Permit to enable of disable NUMA support in MPC Allocator.**/
	bool numa;
	/**Define the scope the posix allocator, can be : process | thread | vp.**/
	char * scope;
	/**Larger bloc will be allocated directely with mmap. Recommended to be less than 2MB to avoid performance issues.**/
	size_t huge_bloc_limte;
	/**Enable failure on free error or try to continue by skipping.**/
	bool strict;
};

/*********************  STRUCT  *********************/
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
	/**Automatically kill the MPC processes after a given timeout. Use 0 to disable. Can be override by MPC_AUTO_KILL_TIMEOUT**/
	int autokill;
};

/*********************  STRUCT  *********************/
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_struct_net_driver_fake
{	/**Size of the buffer used for internal copies.**/
	int buffer;
	/**Enable stealing between threads.**/
	bool stealing;
};

/**********************  ENUM  **********************/
/**Define a specific configuration for a network driver to apply in rails.**/
enum sctk_runtime_config_struct_net_driver_type
{
	SCTK_RTCFG_net_driver_NONE,
	SCTK_RTCFG_net_driver_infiniband,
	SCTK_RTCFG_net_driver_tcp,
};

/*********************  STRUCT  *********************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct sctk_runtime_config_struct_net_driver
{
	enum sctk_runtime_config_struct_net_driver_type type;
	union {
		struct sctk_runtime_config_struct_net_driver_fake infiniband;
		struct sctk_runtime_config_struct_net_driver_fake tcp;
	} value;
};

/*********************  STRUCT  *********************/
/**Contain a list of driver configuration reused by rail definitions.**/
struct sctk_runtime_config_struct_net_driver_config
{	/**Name of the driver configuration to be referenced in rail definitions.**/
	char * name;
	/**Define the related driver to use and its configuration.**/
	struct sctk_runtime_config_struct_net_driver driver;
};

/*********************  STRUCT  *********************/
/**Define a rail which is a name, a device associate to a driver and a routing topology.**/
struct sctk_runtime_config_struct_net_rail
{	/**Define the name of current rail.**/
	char * name;
	/**Define the name of the device to use in this rail.**/
	char * device;
	/**Define the network topology to apply on this rail.**/
	char * topology;
	/**Define the driver config to use for this rail.**/
	char * config;
};

/*********************  STRUCT  *********************/
/**Base structure to contain the network configuration**/
struct sctk_runtime_config_struct_networks
{	/**Define the configuration driver list to reuse in rail definitions.**/
	struct sctk_runtime_config_struct_net_driver_config * configs;
	/** Number of elements in configs array. **/
	int configs_size;
	/**List of rails to declare in MPC.**/
	struct sctk_runtime_config_struct_net_rail * rails;
	/** Number of elements in rails array. **/
	int rails_size;
};

/*********************  STRUCT  *********************/
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

/*********************  STRUCT  *********************/
struct sctk_runtime_config_modules
{
	struct sctk_runtime_config_struct_allocator allocator;
	struct sctk_runtime_config_struct_launcher launcher;
	struct sctk_runtime_config_struct_profiler profiler;
};

/*********************  STRUCT  *********************/
struct sctk_runtime_config
{
	struct sctk_runtime_config_modules modules;
	struct sctk_runtime_config_struct_networks networks;
};

#endif // SCTK_RUNTIME_CONFIG_STRUCT_H


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
/**This is a test structure.**/
struct sctk_runtime_config_module_test
{	/**This is a test**/
	int tt;
};

/*********************  STRUCT  *********************/
/**This is a fake for rails**/
struct sctk_runtime_config_module_rail
{	/**This is a test**/
	int tt;
	/**This is a test**/
	int tt2;
};

/**********************  ENUM  **********************/
/**Define the list of drivers supported for MPC network.**/
enum sctk_runtime_config_module_driver_type
{
	SCTK_RTCFG_driver_NONE,
	SCTK_RTCFG_driver_test,
	SCTK_RTCFG_driver_rail,
};

/*********************  STRUCT  *********************/
/**Define the list of drivers supported for MPC network.**/
struct sctk_runtime_config_module_driver
{
	enum sctk_runtime_config_module_driver_type type;
	union {
		struct sctk_runtime_config_module_test test;
		struct sctk_runtime_config_module_rail rail;
	} value;
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
	/**Sample size entry**/
	size_t alsize;
	/**Permit to enable of disable allocator warnings.**/
	bool warnings;
	/**Try to use test structure.**/
	struct sctk_runtime_config_module_test test;
	/**This is a super doc**/
	struct sctk_runtime_config_module_driver driver;
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
	/**blabla**/
	struct sctk_runtime_config_module_driver * driverlist;
	/** Number of elements in driverlist array. **/
	int driverlist_size;
};

/*********************  STRUCT  *********************/
/**Options for MPC launcher.**/
struct sctk_runtime_config_module_launcher
{	/**blablabla**/
	bool smt;
	/**blabla**/
	int cores;
	/**blabla**/
	int verbosity;
};

/*********************  STRUCT  *********************/
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_module_net_driver_fake
{	/**Size of the buffer used for internal copies.**/
	int buffer;
	/**Enable stealing between threads.**/
	bool stealing;
};

/**********************  ENUM  **********************/
/**Define a specific configuration for a network driver to apply in rails.**/
enum sctk_runtime_config_module_net_driver_type
{
	SCTK_RTCFG_net_driver_NONE,
	SCTK_RTCFG_net_driver_infiniband,
	SCTK_RTCFG_net_driver_tcp,
};

/*********************  STRUCT  *********************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct sctk_runtime_config_module_net_driver
{
	enum sctk_runtime_config_module_net_driver_type type;
	union {
		struct sctk_runtime_config_module_net_driver_fake infiniband;
		struct sctk_runtime_config_module_net_driver_fake tcp;
	} value;
};

/*********************  STRUCT  *********************/
/**Contain a list of driver configuration reused by rail definitions.**/
struct sctk_runtime_config_module_net_driver_config
{	/**Name of the driver configuration to be referenced in rail definitions.**/
	char * name;
	/**Define the related driver to use and its configuration.**/
	struct sctk_runtime_config_module_net_driver driver;
};

/*********************  STRUCT  *********************/
/**Define a rail which is a name, a device associate to a driver and a routing topology.**/
struct sctk_runtime_config_module_net_rail
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
struct sctk_runtime_config_module_networks
{	/**Define the configuration driver list to reuse in rail definitions.**/
	struct sctk_runtime_config_module_net_driver_config * configs;
	/** Number of elements in configs array. **/
	int configs_size;
	/**List of rails to declare in MPC.**/
	struct sctk_runtime_config_module_net_rail * rails;
	/** Number of elements in rails array. **/
	int rails_size;
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
	struct sctk_runtime_config_module_networks networks;
};

#endif // SCTK_RUNTIME_CONFIG_STRUCT_H


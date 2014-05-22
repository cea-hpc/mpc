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

#include <stdlib.h>
#include <dlfcn.h>
#include "uthash.h"

#ifndef SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H
#define SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H

/******************************** VARIABLES *********************************/
void * sctk_handler;

struct enum_value {
	char name[50];
	int value;
	UT_hash_handle hh;
};

struct enum_type {
	char name[50];
	struct enum_value * values;
	UT_hash_handle hh;
};

struct enum_type * enums_types;

/******************************** STRUCTURE *********************************/
/* forward declaration functions */
struct sctk_runtime_config_funcptr;
struct sctk_runtime_config;

/********************************* FUNCTION *********************************/
/* reset functions */
void sctk_runtime_config_struct_init_allocator(void * struct_ptr);
void sctk_runtime_config_struct_init_launcher(void * struct_ptr);
void sctk_runtime_config_struct_init_debugger(void * struct_ptr);
void sctk_runtime_config_struct_init_net_driver_infiniband(void * struct_ptr);
void sctk_runtime_config_struct_init_net_driver_portals(void * struct_ptr);
void sctk_runtime_config_struct_init_net_driver_tcp(void * struct_ptr);
void sctk_runtime_config_struct_init_net_driver(void * struct_ptr);
void sctk_runtime_config_struct_init_net_driver_config(void * struct_ptr);
void sctk_runtime_config_struct_init_net_cli_option(void * struct_ptr);
void sctk_runtime_config_struct_init_net_rail(void * struct_ptr);
void sctk_runtime_config_struct_init_networks(void * struct_ptr);
void sctk_runtime_config_struct_init_inter_thread_comm(void * struct_ptr);
void sctk_runtime_config_struct_init_low_level_comm(void * struct_ptr);
void sctk_runtime_config_struct_init_mpc(void * struct_ptr);
void sctk_runtime_config_struct_init_openmp(void * struct_ptr);
void sctk_runtime_config_struct_init_profiler(void * struct_ptr);
void sctk_runtime_config_struct_init_thread(void * struct_ptr);
void sctk_runtime_config_reset(struct sctk_runtime_config * config);
void sctk_runtime_config_clean_hash_tables();
void* sctk_runtime_config_get_symbol();

#endif /* SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H */

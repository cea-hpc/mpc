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

#include "sctk_runtime_config_struct.h"
#include "../src/sctk_runtime_config_mapper.h"

/*********************  CONSTS  *********************/
const struct sctk_runtime_config_entry_meta sctk_runtime_config_db[] = {
	//sctk_runtime_config
	{"sctk_runtime_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config) , NULL , sctk_runtime_config_reset},
	{"modules"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config,modules)  , sizeof(struct sctk_runtime_config_modules) , "sctk_runtime_config_modules" , NULL},
	{"networks"    , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config,networks)  , sizeof(struct sctk_runtime_config_struct_networks), "sctk_runtime_config_struct_networks" , NULL},
	//sctk_runtime_config_modules
	{"sctk_runtime_config_modules" , SCTK_CONFIG_META_TYPE_STRUCT , 0 , sizeof(struct sctk_runtime_config) , NULL , NULL},
	{"allocator"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,allocator)  , sizeof(struct sctk_runtime_config_struct_allocator) , "sctk_runtime_config_struct_allocator" , sctk_runtime_config_struct_init_allocator},
	{"launcher"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,launcher)  , sizeof(struct sctk_runtime_config_struct_launcher) , "sctk_runtime_config_struct_launcher" , sctk_runtime_config_struct_init_launcher},
	{"profiler"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,profiler)  , sizeof(struct sctk_runtime_config_struct_profiler) , "sctk_runtime_config_struct_profiler" , sctk_runtime_config_struct_init_profiler},
	//struct
	{"sctk_runtime_config_struct_allocator" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_allocator) , NULL , sctk_runtime_config_struct_init_allocator},
	{"numa_migration"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,numa_migration)  , sizeof(bool) , "bool" , NULL},
	{"realloc_factor"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,realloc_factor)  , sizeof(int) , "int" , NULL},
	{"realloc_threashold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,realloc_threashold)  , sizeof(size_t) , "size_t" , NULL},
	{"numa"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,numa)  , sizeof(bool) , "bool" , NULL},
	{"scope"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,scope)  , sizeof(char *) , "char *" , NULL},
	{"strict"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,strict)  , sizeof(bool) , "bool" , NULL},
	//struct
	{"sctk_runtime_config_struct_launcher" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_launcher) , NULL , sctk_runtime_config_struct_init_launcher},
	{"smt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,smt)  , sizeof(bool) , "bool" , NULL},
	{"cores"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,cores)  , sizeof(int) , "int" , NULL},
	{"verbosity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,verbosity)  , sizeof(int) , "int" , NULL},
	{"banner"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,banner)  , sizeof(bool) , "bool" , NULL},
	{"autokill"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,autokill)  , sizeof(int) , "int" , NULL},
	//struct
	{"sctk_runtime_config_struct_net_driver_fake" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_fake) , NULL , sctk_runtime_config_struct_init_net_driver_fake},
	{"buffer"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_fake,buffer)  , sizeof(int) , "int" , NULL},
	{"stealing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_fake,stealing)  , sizeof(bool) , "bool" , NULL},
	//union
	{"sctk_runtime_config_struct_net_driver" , SCTK_CONFIG_META_TYPE_UNION , 0  , sizeof(struct sctk_runtime_config_struct_net_driver) , NULL , sctk_runtime_config_struct_init_net_driver},
	{"infiniband"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_infiniband  , sizeof(struct sctk_runtime_config_struct_net_driver_fake) , "sctk_runtime_config_struct_net_driver_fake" , sctk_runtime_config_struct_init_net_driver_fake},
	{"tcp"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcp  , sizeof(struct sctk_runtime_config_struct_net_driver_fake) , "sctk_runtime_config_struct_net_driver_fake" , sctk_runtime_config_struct_init_net_driver_fake},
	//struct
	{"sctk_runtime_config_struct_net_driver_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_config) , NULL , sctk_runtime_config_struct_init_net_driver_config},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,name)  , sizeof(char *) , "char *" , NULL},
	{"driver"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,driver)  , sizeof(struct sctk_runtime_config_struct_net_driver) , "sctk_runtime_config_struct_net_driver" , sctk_runtime_config_struct_init_net_driver},
	//struct
	{"sctk_runtime_config_struct_net_rail" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_rail) , NULL , sctk_runtime_config_struct_init_net_rail},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,name)  , sizeof(char *) , "char *" , NULL},
	{"device"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,device)  , sizeof(char *) , "char *" , NULL},
	{"topology"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,topology)  , sizeof(char *) , "char *" , NULL},
	{"config"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,config)  , sizeof(char *) , "char *" , NULL},
	//struct
	{"sctk_runtime_config_struct_networks" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_networks) , NULL , sctk_runtime_config_struct_init_networks},
	{"configs"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,configs) , sizeof(struct sctk_runtime_config_struct_net_driver_config) , "sctk_runtime_config_struct_net_driver_config" , "config"},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,rails) , sizeof(struct sctk_runtime_config_struct_net_rail) , "sctk_runtime_config_struct_net_rail" , "rail"},
	//struct
	{"sctk_runtime_config_struct_profiler" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_profiler) , NULL , sctk_runtime_config_struct_init_profiler},
	{"file_prefix"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,file_prefix)  , sizeof(char *) , "char *" , NULL},
	{"append_date"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,append_date)  , sizeof(bool) , "bool" , NULL},
	{"color_stdout"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,color_stdout)  , sizeof(bool) , "bool" , NULL},
	{"level_colors"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,level_colors) , sizeof(char *) , "char *" , "level"},
	//end marker
	{NULL , SCTK_CONFIG_META_TYPE_END , 0 , 0 , NULL,  NULL}
};


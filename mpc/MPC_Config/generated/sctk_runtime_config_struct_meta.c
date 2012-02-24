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
	{"modules"     , SCTK_CONFIG_META_TYPE_PARAM  , 0  , sizeof(struct sctk_runtime_config_modules) , "sctk_runtime_config_modules" , NULL},
	//sctk_runtime_config_modules
	{"sctk_runtime_config_modules" , SCTK_CONFIG_META_TYPE_STRUCT , 0 , sizeof(struct sctk_runtime_config) , NULL , NULL},
	{"allocator"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,allocator)  , sizeof(struct sctk_runtime_config_module_allocator) , "sctk_runtime_config_module_allocator" , sctk_runtime_config_module_init_allocator},
	{"launcher"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,launcher)  , sizeof(struct sctk_runtime_config_module_launcher) , "sctk_runtime_config_module_launcher" , sctk_runtime_config_module_init_launcher},
	//struct
	{"sctk_runtime_config_module_test" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_module_test) , NULL , sctk_runtime_config_module_init_test},
	{"tt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_test,tt)  , sizeof(int) , "int" , NULL},
	//struct
	{"sctk_runtime_config_module_rail" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_module_rail) , NULL , sctk_runtime_config_module_init_rail},
	{"tt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_rail,tt)  , sizeof(int) , "int" , NULL},
	{"tt2"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_rail,tt2)  , sizeof(int) , "int" , NULL},
	//union
	{"sctk_runtime_config_module_driver" , SCTK_CONFIG_META_TYPE_UNION , 0  , sizeof(struct sctk_runtime_config_module_driver) , NULL , sctk_runtime_config_module_init_driver},
	{"test"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_driver_test  , sizeof(struct sctk_runtime_config_module_test) , "sctk_runtime_config_module_test" , sctk_runtime_config_module_init_test},
	{"rail"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_driver_rail  , sizeof(struct sctk_runtime_config_module_rail) , "sctk_runtime_config_module_rail" , sctk_runtime_config_module_init_rail},
	//struct
	{"sctk_runtime_config_module_allocator" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_module_allocator) , NULL , sctk_runtime_config_module_init_allocator},
	{"numa"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,numa)  , sizeof(bool) , "bool" , NULL},
	{"profile"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,profile)  , sizeof(bool) , "bool" , NULL},
	{"alstring"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,alstring)  , sizeof(char *) , "char *" , NULL},
	{"aldouble"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,aldouble)  , sizeof(double) , "double" , NULL},
	{"alfloat"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,alfloat)  , sizeof(float) , "float" , NULL},
	{"warnings"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,warnings)  , sizeof(bool) , "bool" , NULL},
	{"test"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,test)  , sizeof(struct sctk_runtime_config_module_test) , "sctk_runtime_config_module_test" , sctk_runtime_config_module_init_test},
	{"driver"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,driver)  , sizeof(struct sctk_runtime_config_module_driver) , "sctk_runtime_config_module_driver" , sctk_runtime_config_module_init_driver},
	{"classes"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,classes) , sizeof(int) , "int" , "class"},
	{"classes2"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,classes2) , sizeof(int) , "int" , "class2"},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,rails) , sizeof(struct sctk_runtime_config_module_rail) , "sctk_runtime_config_module_rail" , "rail"},
	{"driverlist"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_allocator,driverlist) , sizeof(struct sctk_runtime_config_module_driver) , "sctk_runtime_config_module_driver" , "driver"},
	//struct
	{"sctk_runtime_config_module_launcher" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_module_launcher) , NULL , sctk_runtime_config_module_init_launcher},
	{"smt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_launcher,smt)  , sizeof(bool) , "bool" , NULL},
	{"cores"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_launcher,cores)  , sizeof(int) , "int" , NULL},
	{"verbosity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_module_launcher,verbosity)  , sizeof(int) , "int" , NULL},
	//end marker
	{NULL , SCTK_CONFIG_META_TYPE_END , 0 , 0 , NULL,  NULL}
};


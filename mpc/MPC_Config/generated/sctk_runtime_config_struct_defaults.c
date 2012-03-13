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
#include <string.h>
#include "sctk_runtime_config_struct.h"
#include "sctk_runtime_config_mapper.h"

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_test(void * struct_ptr)
{
	struct sctk_runtime_config_struct_test * obj = struct_ptr;
	//Simple params :
	obj->tt = 10;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_rail(void * struct_ptr)
{
	struct sctk_runtime_config_struct_rail * obj = struct_ptr;
	//Simple params :
	obj->tt = 10;
	obj->tt2 = 20;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_driver(void * struct_ptr)
{
	struct sctk_runtime_config_struct_driver * obj = struct_ptr;
	obj->type = SCTK_RTCFG_driver_NONE;
	memset(&obj->value,0,sizeof(obj->value));
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_allocator(void * struct_ptr)
{
	struct sctk_runtime_config_struct_allocator * obj = struct_ptr;
	//Simple params :
	obj->numa = false;
	obj->profile = false;
	obj->alstring = "TestTestTest";
	obj->aldouble = 42.42;
	obj->alfloat = 42.42;
	obj->alsize = sctk_runtime_config_map_entry_parse_size("2PB");
	obj->warnings = false;
	sctk_runtime_config_struct_init_test(&obj->test);
	sctk_runtime_config_struct_init_driver(&obj->driver);
	//array
	obj->classes = NULL;
	obj->classes_size = 0;
	//array
	obj->classes2 = calloc(3,sizeof(int));
	obj->classes2[0] = 0;
	obj->classes2[1] = 1;
	obj->classes2[2] = 10;
	obj->classes2_size = 3;
	//array
	obj->rails = NULL;
	obj->rails_size = 0;
	//array
	obj->driverlist = NULL;
	obj->driverlist_size = 0;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_launcher(void * struct_ptr)
{
	struct sctk_runtime_config_struct_launcher * obj = struct_ptr;
	//Simple params :
	obj->smt = false;
	obj->cores = 16;
	obj->verbosity = 0;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_fake(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_fake * obj = struct_ptr;
	//Simple params :
	obj->buffer = 1024;
	obj->stealing = true;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver * obj = struct_ptr;
	obj->type = SCTK_RTCFG_net_driver_NONE;
	memset(&obj->value,0,sizeof(obj->value));
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_config(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_config * obj = struct_ptr;
	//Simple params :
	obj->name = NULL;
	sctk_runtime_config_struct_init_net_driver(&obj->driver);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_rail(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_rail * obj = struct_ptr;
	//Simple params :
	obj->name = NULL;
	obj->device = NULL;
	obj->topology = NULL;
	obj->config = NULL;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_networks(void * struct_ptr)
{
	struct sctk_runtime_config_struct_networks * obj = struct_ptr;
	//Simple params :
	//array
	obj->configs = NULL;
	obj->configs_size = 0;
	//array
	obj->rails = NULL;
	obj->rails_size = 0;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset(struct sctk_runtime_config * config)
{
	sctk_runtime_config_struct_init_allocator(&config->modules.allocator);
	sctk_runtime_config_struct_init_launcher(&config->modules.launcher);
	sctk_runtime_config_struct_init_networks(&config->networks);
};


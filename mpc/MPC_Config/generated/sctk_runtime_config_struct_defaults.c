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
#include "sctk_runtime_config_struct_defaults.h"
#include "sctk_runtime_config_mapper.h"

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_allocator(void * struct_ptr)
{
	struct sctk_runtime_config_struct_allocator * obj = struct_ptr;
	/* Simple params : */
	obj->numa_migration = false;
	obj->realloc_factor = 2;
	obj->realloc_threashold = sctk_runtime_config_map_entry_parse_size("50MB");
	obj->numa = true;
	obj->strict = false;
	obj->keep_mem = sctk_runtime_config_map_entry_parse_size("500MB");
	obj->keep_max = sctk_runtime_config_map_entry_parse_size("8MB");
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_launcher(void * struct_ptr)
{
	struct sctk_runtime_config_struct_launcher * obj = struct_ptr;
	/* Simple params : */
	obj->smt = false;
	obj->cores = 1;
	obj->verbosity = 0;
	obj->banner = true;
	obj->autokill = 0;
	obj->user_launchers = "";
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_infiniband(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_infiniband * obj = struct_ptr;
	/* Simple params : */
	obj->network_type = 0;
	obj->adm_port = 0;
	obj->verbose_level = 0;
	obj->eager_limit = 12288;
	obj->buffered_limit = 262114;
	obj->qp_tx_depth = 15000;
	obj->qp_rx_depth = 0;
	obj->cq_depth = 40000;
	obj->max_sg_sq = 4;
	obj->max_sg_rq = 4;
	obj->max_inline = 128;
	obj->rdma_resizing = 0;
	obj->max_rdma_connections = 0;
	obj->max_rdma_resizing = 0;
	obj->init_ibufs = 1000;
	obj->max_srq_ibufs_posted = 1500;
	obj->max_srq_ibufs = 1000;
	obj->srq_credit_limit = 500;
	obj->srq_credit_thread_limit = 100;
	obj->size_ibufs_chunk = 100;
	obj->init_mr = 400;
	obj->size_mr_chunk = 200;
	obj->mmu_cache_enabled = 1;
	obj->mmu_cache_entries = 1;
	obj->steal = 2;
	obj->quiet_crash = 0;
	obj->async_thread = 0;
	obj->wc_in_number = 0;
	obj->wc_out_number = 0;
	obj->rdma_depth = 0;
	obj->rdma_dest_depth = 0;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_tcp(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_tcp * obj = struct_ptr;
	/* Simple params : */
	obj->fake_param = 0;
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
	/* Simple params : */
	obj->name = NULL;
	sctk_runtime_config_struct_init_net_driver(&obj->driver);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_rail(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_rail * obj = struct_ptr;
	/* Simple params : */
	obj->name = NULL;
	obj->device = NULL;
	obj->topology = NULL;
	obj->config = NULL;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_networks(void * struct_ptr)
{
	struct sctk_runtime_config_struct_networks * obj = struct_ptr;
	/* Simple params : */
	/* array */
	obj->configs = NULL;
	obj->configs_size = 0;
	/* array */
	obj->rails = NULL;
	obj->rails_size = 0;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_profiler(void * struct_ptr)
{
	struct sctk_runtime_config_struct_profiler * obj = struct_ptr;
	/* Simple params : */
	obj->file_prefix = "mpc_profile";
	obj->append_date = true;
	obj->color_stdout = true;
	/* array */
	obj->level_colors = calloc(6,sizeof(char *));
	obj->level_colors[0] = "#3A4D85";
	obj->level_colors[1] = "#82A2FF";
	obj->level_colors[2] = "#B8BDCB";
	obj->level_colors[3] = "#5D6782";
	obj->level_colors[4] = "#838383";
	obj->level_colors[5] = "#5A5757";
	obj->level_colors_size = 6;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset(struct sctk_runtime_config * config)
{
	sctk_runtime_config_struct_init_allocator(&config->modules.allocator);
	sctk_runtime_config_struct_init_launcher(&config->modules.launcher);
	sctk_runtime_config_struct_init_profiler(&config->modules.profiler);
	sctk_runtime_config_struct_init_networks(&config->networks);
};


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
#include <dlfcn.h>
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
	obj->verbosity = 0;
	obj->banner = true;
	obj->autokill = 0;
	obj->user_launchers = "default";
	obj->keep_rand_addr = true;
	obj->disable_rand_addr = false;
	obj->disable_mpc = false;
	obj->thread_init.name = "sctk_use_ethread_mxn";
	*(void **) &(obj->thread_init.value) = sctk_runtime_config_get_symbol("sctk_use_ethread_mxn");
	obj->nb_task = 1;
	obj->nb_process = 1;
	obj->nb_processor = 1;
	obj->nb_node = 1;
	obj->launcher = "none";
	obj->max_try = 10;
	obj->vers_details = false;
	obj->profiling = "none";
	obj->enable_smt = false;
	obj->share_node = false;
	obj->restart = false;
	obj->checkpoint = false;
	obj->migration = false;
	obj->report = false;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_debugger(void * struct_ptr)
{
	struct sctk_runtime_config_struct_debugger * obj = struct_ptr;
	/* Simple params : */
	obj->colors = true;
	obj->max_filename_size = 1024;
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
	obj->low_memory = false;
	obj->rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_enum_init_ibv_rdvz_protocol()
{
	struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));
	struct enum_value * current_value, * values = NULL;

	strncpy(current_enum->name, "enum ibv_rdvz_protocol", 50);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "IBV_RDVZ_WRITE_PROTOCOL", 50);
	current_value->value = IBV_RDVZ_WRITE_PROTOCOL;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "IBV_RDVZ_READ_PROTOCOL", 50);
	current_value->value = IBV_RDVZ_READ_PROTOCOL;
	HASH_ADD_STR(values, name, current_value);

	current_enum->values = values;
	HASH_ADD_STR(enums_types, name, current_enum);
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
void sctk_runtime_config_struct_init_net_cli_option(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_cli_option * obj = struct_ptr;
	/* Simple params : */
	obj->name = NULL;
	/* array */
	obj->rails = NULL;
	obj->rails_size = 0;
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
	/* array */
	obj->cli_options = NULL;
	obj->cli_options_size = 0;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_inter_thread_comm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_inter_thread_comm * obj = struct_ptr;
	/* Simple params : */
	obj->barrier_arity = 8;
	obj->broadcast_arity_max = 32;
	obj->broadcast_max_size = 1024;
	obj->broadcast_check_threshold = 512;
	obj->allreduce_arity_max = 8;
	obj->allreduce_max_size = 1024;
	obj->allreduce_check_threshold = 8192;
	obj->collectives_init_hook.name = "sctk_collectives_init_opt_messages";
	*(void **) &(obj->collectives_init_hook.value) = sctk_runtime_config_get_symbol("sctk_collectives_init_opt_messages");
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_low_level_comm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_low_level_comm * obj = struct_ptr;
	/* Simple params : */
	obj->checksum = true;
	obj->send_msg.name = "sctk_network_send_message_default";
	*(void **) &(obj->send_msg.value) = sctk_runtime_config_get_symbol("sctk_network_send_message_default");
	obj->notify_recv_msg.name = "sctk_network_notify_recv_message_default";
	*(void **) &(obj->notify_recv_msg.value) = sctk_runtime_config_get_symbol("sctk_network_notify_recv_message_default");
	obj->notify_matching_msg.name = "sctk_network_notify_matching_message_default";
	*(void **) &(obj->notify_matching_msg.value) = sctk_runtime_config_get_symbol("sctk_network_notify_matching_message_default");
	obj->notify_perform_msg.name = "sctk_network_notify_perform_message_default";
	*(void **) &(obj->notify_perform_msg.value) = sctk_runtime_config_get_symbol("sctk_network_notify_perform_message_default");
	obj->notify_idle_msg.name = "sctk_network_notify_idle_message_default";
	*(void **) &(obj->notify_idle_msg.value) = sctk_runtime_config_get_symbol("sctk_network_notify_idle_message_default");
	obj->notify_any_src_msg.name = "sctk_network_notify_any_source_message_default";
	*(void **) &(obj->notify_any_src_msg.value) = sctk_runtime_config_get_symbol("sctk_network_notify_any_source_message_default");
	obj->network_mode = "default";
	obj->dyn_reordering = false;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_mpc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_mpc * obj = struct_ptr;
	/* Simple params : */
	obj->log_debug = false;
	obj->hard_checking = false;
	obj->buffering = false;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_openmp(void * struct_ptr)
{
	struct sctk_runtime_config_struct_openmp * obj = struct_ptr;
	/* Simple params : */
	obj->vp = 0;
	obj->schedule = "static";
	obj->nb_threads = 0;
	obj->adjustment = false;
	obj->nested = false;
	obj->max_threads = 64;
	obj->max_alive_for_dyn = 7;
	obj->max_alive_for_guided = 3;
	obj->max_alive_sections = 3;
	obj->max_alive_single = 3;
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
void sctk_runtime_config_struct_init_thread(void * struct_ptr)
{
	struct sctk_runtime_config_struct_thread * obj = struct_ptr;
	/* Simple params : */
	obj->spin_delay = 10;
	obj->interval = 10;
	obj->kthread_stack_size = sctk_runtime_config_map_entry_parse_size("10MB");
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset(struct sctk_runtime_config * config)
{
	sctk_handler = dlopen(0, RTLD_LAZY | RTLD_GLOBAL);
	sctk_runtime_config_struct_init_allocator(&config->modules.allocator);
	sctk_runtime_config_struct_init_launcher(&config->modules.launcher);
	sctk_runtime_config_struct_init_debugger(&config->modules.debugger);
	sctk_runtime_config_struct_init_inter_thread_comm(&config->modules.inter_thread_comm);
	sctk_runtime_config_struct_init_low_level_comm(&config->modules.low_level_comm);
	sctk_runtime_config_struct_init_mpc(&config->modules.mpc);
	sctk_runtime_config_enum_init_ibv_rdvz_protocol();
	sctk_runtime_config_struct_init_openmp(&config->modules.openmp);
	sctk_runtime_config_struct_init_profiler(&config->modules.profiler);
	sctk_runtime_config_struct_init_thread(&config->modules.thread);
	sctk_runtime_config_struct_init_networks(&config->networks);
	dlclose(sctk_handler);
}


/*******************  FUNCTION  *********************/
void sctk_runtime_config_clean_hash_tables()
{
	struct enum_type * current_enum, * tmp_enum;
	struct enum_value * current_value, * tmp_value;

	HASH_ITER(hh, enums_types, current_enum, tmp_enum) {
		HASH_ITER(hh, current_enum->values, current_value, tmp_value) {
			HASH_DEL(current_enum->values, current_value);
			free(current_value);
		}
		HASH_DEL(enums_types, current_enum);
		free(current_enum);
	}
}


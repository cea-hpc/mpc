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
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->numa_migration = false;
	obj->realloc_factor = 2;
	obj->realloc_threashold = sctk_runtime_config_map_entry_parse_size("50MB");
	obj->numa = true;
	obj->strict = false;
	obj->keep_mem = sctk_runtime_config_map_entry_parse_size("500MB");
	obj->keep_max = sctk_runtime_config_map_entry_parse_size("8MB");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_launcher(void * struct_ptr)
{
	struct sctk_runtime_config_struct_launcher * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

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
	obj->nb_processor = 0;
	obj->nb_node = 1;
	obj->launcher = "none";
	obj->max_try = 10;
	obj->vers_details = false;
	obj->profiling = "stdout";
	obj->enable_smt = false;
	obj->share_node = false;
	obj->restart = false;
	obj->checkpoint = false;
	obj->migration = false;
	obj->report = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_debugger(void * struct_ptr)
{
	struct sctk_runtime_config_struct_debugger * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->colors = true;
	obj->max_filename_size = 1024;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_infiniband(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_infiniband * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

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
	obj->init_recv_ibufs = 200;
	obj->max_srq_ibufs_posted = 1500;
	obj->max_srq_ibufs = 1000;
	obj->srq_credit_limit = 500;
	obj->srq_credit_thread_limit = 100;
	obj->size_ibufs_chunk = 100;
	obj->init_mr = 400;
	obj->steal = 2;
	obj->quiet_crash = 0;
	obj->async_thread = 0;
	obj->wc_in_number = 0;
	obj->wc_out_number = 0;
	obj->rdma_depth = 0;
	obj->rdma_dest_depth = 0;
	obj->low_memory = false;
	obj->rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
	obj->rdma_min_size = 1024;
	obj->rdma_max_size = 4096;
	obj->rdma_min_nb = 8;
	obj->rdma_max_nb = 32;
	obj->rdma_resizing_min_size = 1024;
	obj->rdma_resizing_max_size = 4096;
	obj->rdma_resizing_min_nb = 8;
	obj->rdma_resizing_max_nb = 32;
	obj->size_recv_ibufs_chunk = 400;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_ib_global(void * struct_ptr)
{
	struct sctk_runtime_config_struct_ib_global * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->mmu_cache_enabled = 1;
	obj->mmu_cache_entry_count = 1000;
	obj->mmu_cache_maximum_size = sctk_runtime_config_map_entry_parse_size("10GB");
	obj->init_done = 1;
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
void sctk_runtime_config_struct_init_net_driver_portals(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_portals * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->fake_param = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_tcp(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_tcp * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->tcpoib = 1;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_tcp_rdma(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_tcp_rdma * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->tcpoib = 1;
	obj->init_done = 1;
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
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->name = NULL;
	sctk_runtime_config_struct_init_net_driver(&obj->driver);
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_boolean(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_boolean * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->value = 1;
	obj->gatefunc.name = "sctk_rail_gate_boolean";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_boolean");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_probabilistic(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_probabilistic * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->probability = 50;
	obj->gatefunc.name = "sctk_rail_gate_probabilistic";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_probabilistic");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_min_size(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_min_size * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->value = 0;
	obj->gatefunc.name = "sctk_rail_gate_minsize";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_minsize");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_max_size(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_max_size * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->value = 0;
	obj->gatefunc.name = "sctk_rail_gate_maxsize";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_maxsize");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_message_type(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_message_type * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->process = 1;
	obj->common = 1;
	obj->gatefunc.name = "sctk_rail_gate_maxsize";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_maxsize");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_user(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_user * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->gatefunc.name = "sctk_rail_gate_true";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_true");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_gate(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_gate * obj = struct_ptr;
	obj->type = SCTK_RTCFG_net_gate_NONE;
	memset(&obj->value,0,sizeof(obj->value));
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_rail(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_rail * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->name = NULL;
	obj->priority = 0;
	obj->device = NULL;
	obj->topology = NULL;
	obj->ondemand = 1;
	obj->config = NULL;
	/* array */
	obj->gates = NULL;
	obj->gates_size = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_cli_option(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_cli_option * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->name = NULL;
	/* array */
	obj->rails = NULL;
	obj->rails_size = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_networks(void * struct_ptr)
{
	struct sctk_runtime_config_struct_networks * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

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
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_inter_thread_comm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_inter_thread_comm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_arity = 8;
	obj->broadcast_arity_max = 32;
	obj->broadcast_max_size = 1024;
	obj->broadcast_check_threshold = 512;
	obj->allreduce_arity_max = 8;
	obj->allreduce_max_size = 4096;
	obj->allreduce_check_threshold = 8192;
	obj->ALLREDUCE_MAX_SLOT = 65536;
	obj->collectives_init_hook.name = "sctk_collectives_init_opt_noalloc_split_messages";
	*(void **) &(obj->collectives_init_hook.value) = sctk_runtime_config_get_symbol("sctk_collectives_init_opt_noalloc_split_messages");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_low_level_comm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_low_level_comm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->checksum = true;
	obj->send_msg.name = "sctk_network_send_message_default";
	*(void **) &(obj->send_msg.value) = sctk_runtime_config_get_symbol("sctk_network_send_message_default");
	obj->network_mode = "default";
	obj->dyn_reordering = false;
	obj->enable_idle_polling = false;
	sctk_runtime_config_struct_init_ib_global(&obj->ib_global);
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_mpc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_mpc * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->log_debug = false;
	obj->hard_checking = false;
	obj->buffering = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_openmp(void * struct_ptr)
{
	struct sctk_runtime_config_struct_openmp * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->vp = 0;
	obj->schedule = "static";
	obj->nb_threads = 0;
	obj->adjustment = false;
	obj->proc_bind = true;
	obj->nested = false;
	obj->stack_size = 0;
	obj->wait_policy = 0;
	obj->thread_limit = 0;
	obj->max_active_levels = 0;
	obj->tree = "";
	obj->max_threads = 64;
	obj->max_alive_for_dyn = 7;
	obj->max_alive_for_guided = 3;
	obj->max_alive_sections = 3;
	obj->max_alive_single = 3;
	obj->warn_nested = false;
	obj->mode = "simple-mixed";
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_profiler(void * struct_ptr)
{
	struct sctk_runtime_config_struct_profiler * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

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
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_thread(void * struct_ptr)
{
	struct sctk_runtime_config_struct_thread * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->spin_delay = 10;
	obj->interval = 10;
	obj->kthread_stack_size = sctk_runtime_config_map_entry_parse_size("10MB");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset(struct sctk_runtime_config * config)
{
	memset(config, 0, sizeof(struct sctk_runtime_config));
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


/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset_struct_default_if_needed(char * structname, void * ptr )
{
	if( !strcmp( structname , "sctk_runtime_config_struct_allocator") )
	{
		sctk_runtime_config_struct_init_allocator( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_launcher") )
	{
		sctk_runtime_config_struct_init_launcher( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_debugger") )
	{
		sctk_runtime_config_struct_init_debugger( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_infiniband") )
	{
		sctk_runtime_config_struct_init_net_driver_infiniband( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_ib_global") )
	{
		sctk_runtime_config_struct_init_ib_global( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_portals") )
	{
		sctk_runtime_config_struct_init_net_driver_portals( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_tcp") )
	{
		sctk_runtime_config_struct_init_net_driver_tcp( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_tcp_rdma") )
	{
		sctk_runtime_config_struct_init_net_driver_tcp_rdma( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_config") )
	{
		sctk_runtime_config_struct_init_net_driver_config( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_boolean") )
	{
		sctk_runtime_config_struct_init_gate_boolean( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_probabilistic") )
	{
		sctk_runtime_config_struct_init_gate_probabilistic( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_min_size") )
	{
		sctk_runtime_config_struct_init_gate_min_size( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_max_size") )
	{
		sctk_runtime_config_struct_init_gate_max_size( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_message_type") )
	{
		sctk_runtime_config_struct_init_gate_message_type( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_user") )
	{
		sctk_runtime_config_struct_init_gate_user( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_rail") )
	{
		sctk_runtime_config_struct_init_net_rail( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_cli_option") )
	{
		sctk_runtime_config_struct_init_net_cli_option( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_networks") )
	{
		sctk_runtime_config_struct_init_networks( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_inter_thread_comm") )
	{
		sctk_runtime_config_struct_init_inter_thread_comm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_low_level_comm") )
	{
		sctk_runtime_config_struct_init_low_level_comm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_mpc") )
	{
		sctk_runtime_config_struct_init_mpc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_openmp") )
	{
		sctk_runtime_config_struct_init_openmp( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_profiler") )
	{
		sctk_runtime_config_struct_init_profiler( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_thread") )
	{
		sctk_runtime_config_struct_init_thread( ptr );
		return;
	}

}


/*******************  FUNCTION  *********************/
void * sctk_runtime_config_get_union_value_offset(char * unionname, void * ptr )
{
	if( !strcmp( unionname , "sctk_runtime_config_struct_net_driver") )
	{
		struct sctk_runtime_config_struct_net_driver	* obj_net_driver = ptr;
		return &(obj_net_driver->value);
	}

	if( !strcmp( unionname , "sctk_runtime_config_struct_net_gate") )
	{
		struct sctk_runtime_config_struct_net_gate	* obj_net_gate = ptr;
		return &(obj_net_gate->value);
	}

	/* If not found assume sizeof (int) */
	return (char *)ptr + sizeof(int);
}


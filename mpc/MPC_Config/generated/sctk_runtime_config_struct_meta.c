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

/********************************  CONSTS  **********************************/
const struct sctk_runtime_config_entry_meta sctk_runtime_config_db[] = {
	/* sctk_runtime_config */
	{"sctk_runtime_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config) , NULL , sctk_runtime_config_reset},
	{"modules"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config,modules)  , sizeof(struct sctk_runtime_config_modules) , "sctk_runtime_config_modules" , NULL},
	{"networks"    , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config,networks)  , sizeof(struct sctk_runtime_config_struct_networks), "sctk_runtime_config_struct_networks" , NULL},
	/* sctk_runtime_config_modules */
	{"sctk_runtime_config_modules" , SCTK_CONFIG_META_TYPE_STRUCT , 0 , sizeof(struct sctk_runtime_config) , NULL , NULL},
	{"allocator"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,allocator)  , sizeof(struct sctk_runtime_config_struct_allocator) , "sctk_runtime_config_struct_allocator" , sctk_runtime_config_struct_init_allocator},
	{"launcher"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,launcher)  , sizeof(struct sctk_runtime_config_struct_launcher) , "sctk_runtime_config_struct_launcher" , sctk_runtime_config_struct_init_launcher},
	{"profiler"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,profiler)  , sizeof(struct sctk_runtime_config_struct_profiler) , "sctk_runtime_config_struct_profiler" , sctk_runtime_config_struct_init_profiler},
	/* struct */
	{"sctk_runtime_config_struct_allocator" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_allocator) , NULL , sctk_runtime_config_struct_init_allocator},
	{"numa_migration"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,numa_migration)  , sizeof(bool) , "bool" , NULL},
	{"realloc_factor"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,realloc_factor)  , sizeof(int) , "int" , NULL},
	{"realloc_threashold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,realloc_threashold)  , sizeof(size_t) , "size_t" , NULL},
	{"numa"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,numa)  , sizeof(bool) , "bool" , NULL},
	{"strict"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,strict)  , sizeof(bool) , "bool" , NULL},
	{"keep_mem"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,keep_mem)  , sizeof(size_t) , "size_t" , NULL},
	{"keep_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,keep_max)  , sizeof(size_t) , "size_t" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_launcher" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_launcher) , NULL , sctk_runtime_config_struct_init_launcher},
	{"smt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,smt)  , sizeof(bool) , "bool" , NULL},
	{"cores"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,cores)  , sizeof(int) , "int" , NULL},
	{"verbosity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,verbosity)  , sizeof(int) , "int" , NULL},
	{"banner"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,banner)  , sizeof(bool) , "bool" , NULL},
	{"autokill"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,autokill)  , sizeof(int) , "int" , NULL},
	{"user_launchers"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,user_launchers)  , sizeof(char *) , "char *" , NULL},
	{"keep_rand_addr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,keep_rand_addr)  , sizeof(bool) , "bool" , NULL},
	{"disable_rand_addr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,disable_rand_addr)  , sizeof(bool) , "bool" , NULL},
	{"disable_mpc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,disable_mpc)  , sizeof(bool) , "bool" , NULL},
	{"startup_args"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,startup_args)  , sizeof(char *) , "char *" , NULL},
	{"multithreading"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,multithreading)  , sizeof(char *) , "char *" , NULL},
	{"nb_task"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_task)  , sizeof(int) , "int" , NULL},
	{"nb_process"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_process)  , sizeof(int) , "int" , NULL},
	{"nb_processor"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_processor)  , sizeof(int) , "int" , NULL},
	{"nb_node"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_node)  , sizeof(int) , "int" , NULL},
	{"launcher"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,launcher)  , sizeof(char *) , "char *" , NULL},
	{"max_try"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,max_try)  , sizeof(int) , "int" , NULL},
	{"vers_details"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,vers_details)  , sizeof(bool) , "bool" , NULL},
	{"profiling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,profiling)  , sizeof(char *) , "char *" , NULL},
	{"enable_smt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,enable_smt)  , sizeof(bool) , "bool" , NULL},
	{"share_node"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,share_node)  , sizeof(bool) , "bool" , NULL},
	{"restart"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,restart)  , sizeof(bool) , "bool" , NULL},
	{"checkpoint"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,checkpoint)  , sizeof(bool) , "bool" , NULL},
	{"migration"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,migration)  , sizeof(bool) , "bool" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_infiniband" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_infiniband) , NULL , sctk_runtime_config_struct_init_net_driver_infiniband},
	{"network_type"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,network_type)  , sizeof(int) , "int" , NULL},
	{"adm_port"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,adm_port)  , sizeof(int) , "int" , NULL},
	{"verbose_level"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,verbose_level)  , sizeof(int) , "int" , NULL},
	{"eager_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,eager_limit)  , sizeof(int) , "int" , NULL},
	{"buffered_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,buffered_limit)  , sizeof(int) , "int" , NULL},
	{"qp_tx_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,qp_tx_depth)  , sizeof(int) , "int" , NULL},
	{"qp_rx_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,qp_rx_depth)  , sizeof(int) , "int" , NULL},
	{"cq_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,cq_depth)  , sizeof(int) , "int" , NULL},
	{"max_sg_sq"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_sg_sq)  , sizeof(int) , "int" , NULL},
	{"max_sg_rq"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_sg_rq)  , sizeof(int) , "int" , NULL},
	{"max_inline"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_inline)  , sizeof(int) , "int" , NULL},
	{"rdma_resizing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing)  , sizeof(int) , "int" , NULL},
	{"max_rdma_connections"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_rdma_connections)  , sizeof(int) , "int" , NULL},
	{"max_rdma_resizing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_rdma_resizing)  , sizeof(int) , "int" , NULL},
	{"init_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_ibufs)  , sizeof(int) , "int" , NULL},
	{"max_srq_ibufs_posted"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_srq_ibufs_posted)  , sizeof(int) , "int" , NULL},
	{"max_srq_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_srq_ibufs)  , sizeof(int) , "int" , NULL},
	{"srq_credit_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,srq_credit_limit)  , sizeof(int) , "int" , NULL},
	{"srq_credit_thread_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,srq_credit_thread_limit)  , sizeof(int) , "int" , NULL},
	{"size_ibufs_chunk"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,size_ibufs_chunk)  , sizeof(int) , "int" , NULL},
	{"init_mr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_mr)  , sizeof(int) , "int" , NULL},
	{"size_mr_chunk"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,size_mr_chunk)  , sizeof(int) , "int" , NULL},
	{"mmu_cache_enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,mmu_cache_enabled)  , sizeof(int) , "int" , NULL},
	{"mmu_cache_entries"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,mmu_cache_entries)  , sizeof(int) , "int" , NULL},
	{"steal"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,steal)  , sizeof(int) , "int" , NULL},
	{"quiet_crash"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,quiet_crash)  , sizeof(int) , "int" , NULL},
	{"async_thread"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,async_thread)  , sizeof(int) , "int" , NULL},
	{"wc_in_number"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,wc_in_number)  , sizeof(int) , "int" , NULL},
	{"wc_out_number"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,wc_out_number)  , sizeof(int) , "int" , NULL},
	{"rdma_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_depth)  , sizeof(int) , "int" , NULL},
	{"rdma_dest_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_dest_depth)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_tcp" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , NULL , sctk_runtime_config_struct_init_net_driver_tcp},
	{"fake_param"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_tcp,fake_param)  , sizeof(int) , "int" , NULL},
	/* union */
	{"sctk_runtime_config_struct_net_driver" , SCTK_CONFIG_META_TYPE_UNION , 0  , sizeof(struct sctk_runtime_config_struct_net_driver) , NULL , sctk_runtime_config_struct_init_net_driver},
	{"infiniband"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_infiniband  , sizeof(struct sctk_runtime_config_struct_net_driver_infiniband) , "sctk_runtime_config_struct_net_driver_infiniband" , sctk_runtime_config_struct_init_net_driver_infiniband},
	{"tcp"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcp  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , "sctk_runtime_config_struct_net_driver_tcp" , sctk_runtime_config_struct_init_net_driver_tcp},
	{"tcpoib"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcpoib  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , "sctk_runtime_config_struct_net_driver_tcp" , sctk_runtime_config_struct_init_net_driver_tcp},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_config) , NULL , sctk_runtime_config_struct_init_net_driver_config},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,name)  , sizeof(char *) , "char *" , NULL},
	{"driver"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,driver)  , sizeof(struct sctk_runtime_config_struct_net_driver) , "sctk_runtime_config_struct_net_driver" , sctk_runtime_config_struct_init_net_driver},
	/* struct */
	{"sctk_runtime_config_struct_net_cli_option" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_cli_option) , NULL , sctk_runtime_config_struct_init_net_cli_option},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_cli_option,name)  , sizeof(char *) , "char *" , NULL},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_cli_option,rails) , sizeof(char *) , "char *" , "rail"},
	/* struct */
	{"sctk_runtime_config_struct_net_rail" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_rail) , NULL , sctk_runtime_config_struct_init_net_rail},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,name)  , sizeof(char *) , "char *" , NULL},
	{"device"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,device)  , sizeof(char *) , "char *" , NULL},
	{"topology"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,topology)  , sizeof(char *) , "char *" , NULL},
	{"config"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,config)  , sizeof(char *) , "char *" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_networks" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_networks) , NULL , sctk_runtime_config_struct_init_networks},
	{"configs"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,configs) , sizeof(struct sctk_runtime_config_struct_net_driver_config) , "sctk_runtime_config_struct_net_driver_config" , "config"},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,rails) , sizeof(struct sctk_runtime_config_struct_net_rail) , "sctk_runtime_config_struct_net_rail" , "rail"},
	{"cli_options"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,cli_options) , sizeof(struct sctk_runtime_config_struct_net_cli_option) , "sctk_runtime_config_struct_net_cli_option" , "cli_option"},
	/* struct */
	{"sctk_runtime_config_struct_profiler" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_profiler) , NULL , sctk_runtime_config_struct_init_profiler},
	{"file_prefix"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,file_prefix)  , sizeof(char *) , "char *" , NULL},
	{"append_date"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,append_date)  , sizeof(bool) , "bool" , NULL},
	{"color_stdout"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,color_stdout)  , sizeof(bool) , "bool" , NULL},
	{"level_colors"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,level_colors) , sizeof(char *) , "char *" , "level"},
	/* end marker */
	{NULL , SCTK_CONFIG_META_TYPE_END , 0 , 0 , NULL,  NULL}
};


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
	{"debugger"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,debugger)  , sizeof(struct sctk_runtime_config_struct_debugger) , "sctk_runtime_config_struct_debugger" , sctk_runtime_config_struct_init_debugger},
	{"collectives_intra"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,collectives_intra)  , sizeof(struct sctk_runtime_config_struct_collectives_intra) , "sctk_runtime_config_struct_collectives_intra" , sctk_runtime_config_struct_init_collectives_intra},
	{"collectives_inter"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,collectives_inter)  , sizeof(struct sctk_runtime_config_struct_collectives_inter) , "sctk_runtime_config_struct_collectives_inter" , sctk_runtime_config_struct_init_collectives_inter},
	{"progress_thread"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,progress_thread)  , sizeof(struct sctk_runtime_config_struct_progress_thread) , "sctk_runtime_config_struct_progress_thread" , sctk_runtime_config_struct_init_progress_thread},
	{"mpc"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,mpc)  , sizeof(struct sctk_runtime_config_struct_mpc) , "sctk_runtime_config_struct_mpc" , sctk_runtime_config_struct_init_mpc},
	{"inter_thread_comm"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,inter_thread_comm)  , sizeof(struct sctk_runtime_config_struct_inter_thread_comm) , "sctk_runtime_config_struct_inter_thread_comm" , sctk_runtime_config_struct_init_inter_thread_comm},
	{"low_level_comm"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,low_level_comm)  , sizeof(struct sctk_runtime_config_struct_low_level_comm) , "sctk_runtime_config_struct_low_level_comm" , sctk_runtime_config_struct_init_low_level_comm},
	{"openmp"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,openmp)  , sizeof(struct sctk_runtime_config_struct_openmp) , "sctk_runtime_config_struct_openmp" , sctk_runtime_config_struct_init_openmp},
	{"profiler"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,profiler)  , sizeof(struct sctk_runtime_config_struct_profiler) , "sctk_runtime_config_struct_profiler" , sctk_runtime_config_struct_init_profiler},
	{"thread"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,thread)  , sizeof(struct sctk_runtime_config_struct_thread) , "sctk_runtime_config_struct_thread" , sctk_runtime_config_struct_init_thread},
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
	{"verbosity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,verbosity)  , sizeof(int) , "int" , NULL},
	{"banner"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,banner)  , sizeof(bool) , "bool" , NULL},
	{"autokill"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,autokill)  , sizeof(int) , "int" , NULL},
	{"user_launchers"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,user_launchers)  , sizeof(char *) , "char *" , NULL},
	{"keep_rand_addr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,keep_rand_addr)  , sizeof(bool) , "bool" , NULL},
	{"disable_rand_addr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,disable_rand_addr)  , sizeof(bool) , "bool" , NULL},
	{"disable_mpc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,disable_mpc)  , sizeof(bool) , "bool" , NULL},
	{"thread_init"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,thread_init)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
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
	{"report"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,report)  , sizeof(bool) , "bool" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_debugger" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_debugger) , NULL , sctk_runtime_config_struct_init_debugger},
	{"colors"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_debugger,colors)  , sizeof(bool) , "bool" , NULL},
	{"max_filename_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_debugger,max_filename_size)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_collectives_intra" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_collectives_intra) , NULL , sctk_runtime_config_struct_init_collectives_intra},
	{"barrier_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,barrier_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"bcast_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,bcast_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"allgather_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,allgather_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"allgatherv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,allgatherv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"alltoall_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,alltoall_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"alltoallv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,alltoallv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"alltoallw_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,alltoallw_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"gather_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,gather_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"gatherv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,gatherv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"scatter_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,scatter_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"scatterv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,scatterv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"scan_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,scan_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"exscan_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,exscan_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"reduce_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"allreduce_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,allreduce_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"reduce_scatter_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_scatter_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"reduce_scatter_block_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_scatter_block_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_collectives_inter" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_collectives_inter) , NULL , sctk_runtime_config_struct_init_collectives_inter},
	{"barrier_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,barrier_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"bcast_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,bcast_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"allgather_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,allgather_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"allgatherv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,allgatherv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"alltoall_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,alltoall_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"alltoallv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,alltoallv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"alltoallw_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,alltoallw_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"gather_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,gather_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"gatherv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,gatherv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"scatter_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,scatter_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"scatterv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,scatterv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"reduce_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,reduce_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"allreduce_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,allreduce_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"reduce_scatter_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,reduce_scatter_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"reduce_scatter_block_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,reduce_scatter_block_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_progress_thread" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_progress_thread) , NULL , sctk_runtime_config_struct_init_progress_thread},
	{"use_progress_thread"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_progress_thread,use_progress_thread)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_mpc" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_mpc) , NULL , sctk_runtime_config_struct_init_mpc},
	{"log_debug"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpc,log_debug)  , sizeof(bool) , "bool" , NULL},
	{"hard_checking"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpc,hard_checking)  , sizeof(bool) , "bool" , NULL},
	{"buffering"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpc,buffering)  , sizeof(bool) , "bool" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_topological" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_topological) , NULL , sctk_runtime_config_struct_init_net_driver_topological},
	{"dummy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_topological,dummy)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_infiniband" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_infiniband) , NULL , sctk_runtime_config_struct_init_net_driver_infiniband},
	{"pkey"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,pkey)  , sizeof(char *) , "char *" , NULL},
	{"network_type"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,network_type)  , sizeof(int) , "int" , NULL},
	{"adm_port"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,adm_port)  , sizeof(int) , "int" , NULL},
	{"verbose_level"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,verbose_level)  , sizeof(int) , "int" , NULL},
	{"eager_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,eager_limit)  , sizeof(int) , "int" , NULL},
	{"buffered_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,buffered_limit)  , sizeof(int) , "int" , NULL},
	{"qp_tx_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,qp_tx_depth)  , sizeof(int) , "int" , NULL},
	{"qp_rx_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,qp_rx_depth)  , sizeof(int) , "int" , NULL},
	{"cq_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,cq_depth)  , sizeof(int) , "int" , NULL},
	{"rdma_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_depth)  , sizeof(int) , "int" , NULL},
	{"max_sg_sq"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_sg_sq)  , sizeof(int) , "int" , NULL},
	{"max_sg_rq"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_sg_rq)  , sizeof(int) , "int" , NULL},
	{"max_inline"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_inline)  , sizeof(int) , "int" , NULL},
	{"rdma_resizing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing)  , sizeof(int) , "int" , NULL},
	{"max_rdma_connections"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_rdma_connections)  , sizeof(int) , "int" , NULL},
	{"max_rdma_resizing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_rdma_resizing)  , sizeof(int) , "int" , NULL},
	{"init_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_ibufs)  , sizeof(int) , "int" , NULL},
	{"init_recv_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_recv_ibufs)  , sizeof(int) , "int" , NULL},
	{"max_srq_ibufs_posted"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_srq_ibufs_posted)  , sizeof(int) , "int" , NULL},
	{"max_srq_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_srq_ibufs)  , sizeof(int) , "int" , NULL},
	{"srq_credit_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,srq_credit_limit)  , sizeof(int) , "int" , NULL},
	{"srq_credit_thread_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,srq_credit_thread_limit)  , sizeof(int) , "int" , NULL},
	{"size_ibufs_chunk"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,size_ibufs_chunk)  , sizeof(int) , "int" , NULL},
	{"init_mr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_mr)  , sizeof(int) , "int" , NULL},
	{"steal"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,steal)  , sizeof(int) , "int" , NULL},
	{"quiet_crash"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,quiet_crash)  , sizeof(int) , "int" , NULL},
	{"async_thread"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,async_thread)  , sizeof(int) , "int" , NULL},
	{"wc_in_number"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,wc_in_number)  , sizeof(int) , "int" , NULL},
	{"wc_out_number"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,wc_out_number)  , sizeof(int) , "int" , NULL},
	{"low_memory"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,low_memory)  , sizeof(bool) , "bool" , NULL},
	{"rdvz_protocol"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdvz_protocol)  , sizeof(enum ibv_rdvz_protocol) , "enum ibv_rdvz_protocol" , NULL},
	{"rdma_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_min_size)  , sizeof(int) , "int" , NULL},
	{"rdma_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_max_size)  , sizeof(int) , "int" , NULL},
	{"rdma_min_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_min_nb)  , sizeof(int) , "int" , NULL},
	{"rdma_max_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_max_nb)  , sizeof(int) , "int" , NULL},
	{"rdma_resizing_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_min_size)  , sizeof(int) , "int" , NULL},
	{"rdma_resizing_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_max_size)  , sizeof(int) , "int" , NULL},
	{"rdma_resizing_min_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_min_nb)  , sizeof(int) , "int" , NULL},
	{"rdma_resizing_max_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_max_nb)  , sizeof(int) , "int" , NULL},
	{"size_recv_ibufs_chunk"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,size_recv_ibufs_chunk)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_ib_global" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_ib_global) , NULL , sctk_runtime_config_struct_init_ib_global},
	{"mmu_cache_enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_enabled)  , sizeof(int) , "int" , NULL},
	{"mmu_cache_entry_count"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_entry_count)  , sizeof(int) , "int" , NULL},
	{"mmu_cache_maximum_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_maximum_size)  , sizeof(size_t) , "size_t" , NULL},
	{"mmu_cache_maximum_pin_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_maximum_pin_size)  , sizeof(size_t) , "size_t" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_portals" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_portals) , NULL , sctk_runtime_config_struct_init_net_driver_portals},
	{"fake_param"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_portals,fake_param)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_tcp" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , NULL , sctk_runtime_config_struct_init_net_driver_tcp},
	{"tcpoib"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_tcp,tcpoib)  , sizeof(int) , "int" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_tcp_rdma" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp_rdma) , NULL , sctk_runtime_config_struct_init_net_driver_tcp_rdma},
	{"tcpoib"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_tcp_rdma,tcpoib)  , sizeof(int) , "int" , NULL},
	/* union */
	{"sctk_runtime_config_struct_net_driver" , SCTK_CONFIG_META_TYPE_UNION , 0  , sizeof(struct sctk_runtime_config_struct_net_driver) , NULL , sctk_runtime_config_struct_init_net_driver},
	{"infiniband"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_infiniband  , sizeof(struct sctk_runtime_config_struct_net_driver_infiniband) , "sctk_runtime_config_struct_net_driver_infiniband" , sctk_runtime_config_struct_init_net_driver_infiniband},
	{"portals"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_portals  , sizeof(struct sctk_runtime_config_struct_net_driver_portals) , "sctk_runtime_config_struct_net_driver_portals" , sctk_runtime_config_struct_init_net_driver_portals},
	{"tcp"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcp  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , "sctk_runtime_config_struct_net_driver_tcp" , sctk_runtime_config_struct_init_net_driver_tcp},
	{"tcprdma"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcprdma  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp_rdma) , "sctk_runtime_config_struct_net_driver_tcp_rdma" , sctk_runtime_config_struct_init_net_driver_tcp_rdma},
	{"topological"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_topological  , sizeof(struct sctk_runtime_config_struct_net_driver_topological) , "sctk_runtime_config_struct_net_driver_topological" , sctk_runtime_config_struct_init_net_driver_topological},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_config) , NULL , sctk_runtime_config_struct_init_net_driver_config},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,name)  , sizeof(char *) , "char *" , NULL},
	{"driver"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,driver)  , sizeof(struct sctk_runtime_config_struct_net_driver) , "sctk_runtime_config_struct_net_driver" , sctk_runtime_config_struct_init_net_driver},
	/* struct */
	{"sctk_runtime_config_struct_gate_boolean" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_boolean) , NULL , sctk_runtime_config_struct_init_gate_boolean},
	{"value"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_boolean,value)  , sizeof(int) , "int" , NULL},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_boolean,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_gate_probabilistic" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_probabilistic) , NULL , sctk_runtime_config_struct_init_gate_probabilistic},
	{"probability"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_probabilistic,probability)  , sizeof(int) , "int" , NULL},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_probabilistic,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_gate_min_size" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_min_size) , NULL , sctk_runtime_config_struct_init_gate_min_size},
	{"value"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_min_size,value)  , sizeof(size_t) , "size_t" , NULL},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_min_size,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_gate_max_size" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_max_size) , NULL , sctk_runtime_config_struct_init_gate_max_size},
	{"value"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_max_size,value)  , sizeof(size_t) , "size_t" , NULL},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_max_size,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_gate_message_type" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_message_type) , NULL , sctk_runtime_config_struct_init_gate_message_type},
	{"process"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,process)  , sizeof(int) , "int" , NULL},
	{"common"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,common)  , sizeof(int) , "int" , NULL},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_gate_user" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_user) , NULL , sctk_runtime_config_struct_init_gate_user},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_user,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* union */
	{"sctk_runtime_config_struct_net_gate" , SCTK_CONFIG_META_TYPE_UNION , 0  , sizeof(struct sctk_runtime_config_struct_net_gate) , NULL , sctk_runtime_config_struct_init_net_gate},
	{"boolean"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_gate_boolean  , sizeof(struct sctk_runtime_config_struct_gate_boolean) , "sctk_runtime_config_struct_gate_boolean" , sctk_runtime_config_struct_init_gate_boolean},
	{"probabilistic"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_gate_probabilistic  , sizeof(struct sctk_runtime_config_struct_gate_probabilistic) , "sctk_runtime_config_struct_gate_probabilistic" , sctk_runtime_config_struct_init_gate_probabilistic},
	{"minsize"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_gate_minsize  , sizeof(struct sctk_runtime_config_struct_gate_min_size) , "sctk_runtime_config_struct_gate_min_size" , sctk_runtime_config_struct_init_gate_min_size},
	{"maxsize"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_gate_maxsize  , sizeof(struct sctk_runtime_config_struct_gate_max_size) , "sctk_runtime_config_struct_gate_max_size" , sctk_runtime_config_struct_init_gate_max_size},
	{"msgtype"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_gate_msgtype  , sizeof(struct sctk_runtime_config_struct_gate_message_type) , "sctk_runtime_config_struct_gate_message_type" , sctk_runtime_config_struct_init_gate_message_type},
	{"user"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_gate_user  , sizeof(struct sctk_runtime_config_struct_gate_probabilistic) , "sctk_runtime_config_struct_gate_probabilistic" , sctk_runtime_config_struct_init_gate_probabilistic},
	/* struct */
	{"sctk_runtime_config_struct_topological_polling" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_topological_polling) , NULL , sctk_runtime_config_struct_init_topological_polling},
	{"range"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_topological_polling,range)  , sizeof(enum rail_topological_polling_level) , "enum rail_topological_polling_level" , NULL},
	{"trigger"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_topological_polling,trigger)  , sizeof(enum rail_topological_polling_level) , "enum rail_topological_polling_level" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_net_rail" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_rail) , NULL , sctk_runtime_config_struct_init_net_rail},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,name)  , sizeof(char *) , "char *" , NULL},
	{"priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,priority)  , sizeof(int) , "int" , NULL},
	{"device"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,device)  , sizeof(char *) , "char *" , NULL},
	{"idle_polling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,idle_polling)  , sizeof(struct sctk_runtime_config_struct_topological_polling) , "sctk_runtime_config_struct_topological_polling" , sctk_runtime_config_struct_init_topological_polling},
	{"any_source_polling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,any_source_polling)  , sizeof(struct sctk_runtime_config_struct_topological_polling) , "sctk_runtime_config_struct_topological_polling" , sctk_runtime_config_struct_init_topological_polling},
	{"topology"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,topology)  , sizeof(char *) , "char *" , NULL},
	{"ondemand"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,ondemand)  , sizeof(int) , "int" , NULL},
	{"rdma"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,rdma)  , sizeof(int) , "int" , NULL},
	{"config"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,config)  , sizeof(char *) , "char *" , NULL},
	{"gates"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,gates) , sizeof(struct sctk_runtime_config_struct_net_gate) , "sctk_runtime_config_struct_net_gate" , "gate"},
	{"subrails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,subrails) , sizeof(struct sctk_runtime_config_struct_net_rail) , "sctk_runtime_config_struct_net_rail" , "subrail"},
	/* struct */
	{"sctk_runtime_config_struct_net_cli_option" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_cli_option) , NULL , sctk_runtime_config_struct_init_net_cli_option},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_cli_option,name)  , sizeof(char *) , "char *" , NULL},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_cli_option,rails) , sizeof(char *) , "char *" , "rail"},
	/* struct */
	{"sctk_runtime_config_struct_networks" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_networks) , NULL , sctk_runtime_config_struct_init_networks},
	{"configs"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,configs) , sizeof(struct sctk_runtime_config_struct_net_driver_config) , "sctk_runtime_config_struct_net_driver_config" , "config"},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,rails) , sizeof(struct sctk_runtime_config_struct_net_rail) , "sctk_runtime_config_struct_net_rail" , "rail"},
	{"cli_options"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,cli_options) , sizeof(struct sctk_runtime_config_struct_net_cli_option) , "sctk_runtime_config_struct_net_cli_option" , "cli_option"},
	/* struct */
	{"sctk_runtime_config_struct_inter_thread_comm" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_inter_thread_comm) , NULL , sctk_runtime_config_struct_init_inter_thread_comm},
	{"barrier_arity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,barrier_arity)  , sizeof(int) , "int" , NULL},
	{"broadcast_arity_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,broadcast_arity_max)  , sizeof(int) , "int" , NULL},
	{"broadcast_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,broadcast_max_size)  , sizeof(int) , "int" , NULL},
	{"broadcast_check_threshold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,broadcast_check_threshold)  , sizeof(int) , "int" , NULL},
	{"allreduce_arity_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,allreduce_arity_max)  , sizeof(int) , "int" , NULL},
	{"allreduce_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,allreduce_max_size)  , sizeof(int) , "int" , NULL},
	{"allreduce_check_threshold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,allreduce_check_threshold)  , sizeof(int) , "int" , NULL},
	{"ALLREDUCE_MAX_SLOT"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,ALLREDUCE_MAX_SLOT)  , sizeof(int) , "int" , NULL},
	{"collectives_init_hook"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,collectives_init_hook)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_low_level_comm" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_low_level_comm) , NULL , sctk_runtime_config_struct_init_low_level_comm},
	{"checksum"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,checksum)  , sizeof(bool) , "bool" , NULL},
	{"send_msg"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,send_msg)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL},
	{"network_mode"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,network_mode)  , sizeof(char *) , "char *" , NULL},
	{"dyn_reordering"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,dyn_reordering)  , sizeof(bool) , "bool" , NULL},
	{"enable_idle_polling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,enable_idle_polling)  , sizeof(bool) , "bool" , NULL},
	{"ib_global"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,ib_global)  , sizeof(struct sctk_runtime_config_struct_ib_global) , "sctk_runtime_config_struct_ib_global" , sctk_runtime_config_struct_init_ib_global},
	/* struct */
	{"sctk_runtime_config_struct_openmp" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_openmp) , NULL , sctk_runtime_config_struct_init_openmp},
	{"vp"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,vp)  , sizeof(int) , "int" , NULL},
	{"schedule"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,schedule)  , sizeof(char *) , "char *" , NULL},
	{"nb_threads"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,nb_threads)  , sizeof(int) , "int" , NULL},
	{"adjustment"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,adjustment)  , sizeof(bool) , "bool" , NULL},
	{"proc_bind"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,proc_bind)  , sizeof(bool) , "bool" , NULL},
	{"nested"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,nested)  , sizeof(bool) , "bool" , NULL},
	{"stack_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,stack_size)  , sizeof(int) , "int" , NULL},
	{"wait_policy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,wait_policy)  , sizeof(int) , "int" , NULL},
	{"thread_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,thread_limit)  , sizeof(int) , "int" , NULL},
	{"max_active_levels"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_active_levels)  , sizeof(int) , "int" , NULL},
	{"tree"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,tree)  , sizeof(char *) , "char *" , NULL},
	{"max_threads"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_threads)  , sizeof(int) , "int" , NULL},
	{"max_alive_for_dyn"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_for_dyn)  , sizeof(int) , "int" , NULL},
	{"max_alive_for_guided"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_for_guided)  , sizeof(int) , "int" , NULL},
	{"max_alive_sections"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_sections)  , sizeof(int) , "int" , NULL},
	{"max_alive_single"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_single)  , sizeof(int) , "int" , NULL},
	{"warn_nested"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,warn_nested)  , sizeof(bool) , "bool" , NULL},
	{"mode"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,mode)  , sizeof(char *) , "char *" , NULL},
	{"affinity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,affinity)  , sizeof(char *) , "char *" , NULL},
	/* struct */
	{"sctk_runtime_config_struct_profiler" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_profiler) , NULL , sctk_runtime_config_struct_init_profiler},
	{"file_prefix"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,file_prefix)  , sizeof(char *) , "char *" , NULL},
	{"append_date"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,append_date)  , sizeof(bool) , "bool" , NULL},
	{"color_stdout"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,color_stdout)  , sizeof(bool) , "bool" , NULL},
	{"level_colors"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,level_colors) , sizeof(char *) , "char *" , "level"},
	/* struct */
	{"sctk_runtime_config_struct_thread" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_thread) , NULL , sctk_runtime_config_struct_init_thread},
	{"spin_delay"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,spin_delay)  , sizeof(int) , "int" , NULL},
	{"interval"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,interval)  , sizeof(int) , "int" , NULL},
	{"kthread_stack_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,kthread_stack_size)  , sizeof(size_t) , "size_t" , NULL},
	/* end marker */
	{NULL , SCTK_CONFIG_META_TYPE_END , 0 , 0 , NULL,  NULL}
};


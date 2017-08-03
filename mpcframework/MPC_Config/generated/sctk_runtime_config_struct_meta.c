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
	{"accelerator"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,accelerator)  , sizeof(struct sctk_runtime_config_struct_accl) , "sctk_runtime_config_struct_accl" , sctk_runtime_config_struct_init_accl},
	{"allocator"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,allocator)  , sizeof(struct sctk_runtime_config_struct_allocator) , "sctk_runtime_config_struct_allocator" , sctk_runtime_config_struct_init_allocator},
	{"launcher"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,launcher)  , sizeof(struct sctk_runtime_config_struct_launcher) , "sctk_runtime_config_struct_launcher" , sctk_runtime_config_struct_init_launcher},
	{"debugger"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,debugger)  , sizeof(struct sctk_runtime_config_struct_debugger) , "sctk_runtime_config_struct_debugger" , sctk_runtime_config_struct_init_debugger},
	{"ft_system"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,ft_system)  , sizeof(struct sctk_runtime_config_struct_ft) , "sctk_runtime_config_struct_ft" , sctk_runtime_config_struct_init_ft},
	{"inter_thread_comm"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,inter_thread_comm)  , sizeof(struct sctk_runtime_config_struct_inter_thread_comm) , "sctk_runtime_config_struct_inter_thread_comm" , sctk_runtime_config_struct_init_inter_thread_comm},
	{"low_level_comm"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,low_level_comm)  , sizeof(struct sctk_runtime_config_struct_low_level_comm) , "sctk_runtime_config_struct_low_level_comm" , sctk_runtime_config_struct_init_low_level_comm},
	{"collectives_shm"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,collectives_shm)  , sizeof(struct sctk_runtime_config_struct_collectives_shm) , "sctk_runtime_config_struct_collectives_shm" , sctk_runtime_config_struct_init_collectives_shm},
	{"collectives_intra"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,collectives_intra)  , sizeof(struct sctk_runtime_config_struct_collectives_intra) , "sctk_runtime_config_struct_collectives_intra" , sctk_runtime_config_struct_init_collectives_intra},
	{"collectives_inter"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,collectives_inter)  , sizeof(struct sctk_runtime_config_struct_collectives_inter) , "sctk_runtime_config_struct_collectives_inter" , sctk_runtime_config_struct_init_collectives_inter},
	{"nbc"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,nbc)  , sizeof(struct sctk_runtime_config_struct_nbc) , "sctk_runtime_config_struct_nbc" , sctk_runtime_config_struct_init_nbc},
	{"mpc"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,mpc)  , sizeof(struct sctk_runtime_config_struct_mpc) , "sctk_runtime_config_struct_mpc" , sctk_runtime_config_struct_init_mpc},
	{"rma"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,rma)  , sizeof(struct sctk_runtime_config_struct_mpi_rma) , "sctk_runtime_config_struct_mpi_rma" , sctk_runtime_config_struct_init_mpi_rma},
	{"openmp"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,openmp)  , sizeof(struct sctk_runtime_config_struct_openmp) , "sctk_runtime_config_struct_openmp" , sctk_runtime_config_struct_init_openmp},
	{"profiler"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,profiler)  , sizeof(struct sctk_runtime_config_struct_profiler) , "sctk_runtime_config_struct_profiler" , sctk_runtime_config_struct_init_profiler},
	{"thread"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,thread)  , sizeof(struct sctk_runtime_config_struct_thread) , "sctk_runtime_config_struct_thread" , sctk_runtime_config_struct_init_thread},
	{"scheduler"     , SCTK_CONFIG_META_TYPE_PARAM , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_modules,scheduler)  , sizeof(struct sctk_runtime_config_struct_scheduler) , "sctk_runtime_config_struct_scheduler" , sctk_runtime_config_struct_init_scheduler},
	/* struct */
	{"sctk_runtime_config_struct_accl_cuda" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_accl_cuda) , NULL , sctk_runtime_config_struct_init_accl_cuda},
	{"enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl_cuda,enabled)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_accl_openacc" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_accl_openacc) , NULL , sctk_runtime_config_struct_init_accl_openacc},
	{"enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl_openacc,enabled)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_accl_opencl" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_accl_opencl) , NULL , sctk_runtime_config_struct_init_accl_opencl},
	{"enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl_opencl,enabled)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_accl" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_accl) , NULL , sctk_runtime_config_struct_init_accl},
	{"enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl,enabled)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"cuda"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl,cuda)  , sizeof(struct sctk_runtime_config_struct_accl_cuda) , "sctk_runtime_config_struct_accl_cuda" , sctk_runtime_config_struct_init_accl_cuda , 
				NULL
			},
	{"openacc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl,openacc)  , sizeof(struct sctk_runtime_config_struct_accl_openacc) , "sctk_runtime_config_struct_accl_openacc" , sctk_runtime_config_struct_init_accl_openacc , 
				NULL
			},
	{"opencl"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_accl,opencl)  , sizeof(struct sctk_runtime_config_struct_accl_opencl) , "sctk_runtime_config_struct_accl_opencl" , sctk_runtime_config_struct_init_accl_opencl , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_allocator" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_allocator) , NULL , sctk_runtime_config_struct_init_allocator},
	{"numa_migration"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,numa_migration)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"realloc_factor"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,realloc_factor)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"realloc_threashold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,realloc_threashold)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"numa"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,numa)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"strict"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,strict)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"keep_mem"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,keep_mem)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"keep_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_allocator,keep_max)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_launcher" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_launcher) , NULL , sctk_runtime_config_struct_init_launcher},
	{"verbosity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,verbosity)  , sizeof(int) , "int" , NULL , 
				"LN_VERBOSITY"
			},
	{"banner"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,banner)  , sizeof(int) , "int" , NULL , 
				"LN_BANNER"
			},
	{"autokill"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,autokill)  , sizeof(int) , "int" , NULL , 
				"LN_AUTOKILL"
			},
	{"user_launchers"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,user_launchers)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"keep_rand_addr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,keep_rand_addr)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"disable_rand_addr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,disable_rand_addr)  , sizeof(int) , "int" , NULL , 
				"LN_ADDR_RAND_DISABLE"
			},
	{"disable_mpc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,disable_mpc)  , sizeof(int) , "int" , NULL , 
				"LN_DISABLE_MPC"
			},
	{"thread_init"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,thread_init)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"nb_task"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_task)  , sizeof(int) , "int" , NULL , 
				"LN_MPI_TASK"
			},
	{"nb_process"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_process)  , sizeof(int) , "int" , NULL , 
				"LN_MPC_PROCESS"
			},
	{"nb_processor"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_processor)  , sizeof(int) , "int" , NULL , 
				"LN_MPC_VP"
			},
	{"nb_node"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,nb_node)  , sizeof(int) , "int" , NULL , 
				"LN_MPC_NODE"
			},
	{"launcher"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,launcher)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"max_try"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,max_try)  , sizeof(int) , "int" , NULL , 
				"LN_MAX_TOPO_TRY"
			},
	{"vers_details"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,vers_details)  , sizeof(int) , "int" , NULL , 
				"LN_DISPLAY_VERS"
			},
	{"profiling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,profiling)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"enable_smt"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,enable_smt)  , sizeof(int) , "int" , NULL , 
				"LN_ENABLE_SMT"
			},
	{"share_node"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,share_node)  , sizeof(int) , "int" , NULL , 
				"LN_SHARE_NODE"
			},
	{"restart"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,restart)  , sizeof(int) , "int" , NULL , 
				"LN_RESTART"
			},
	{"checkpoint"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,checkpoint)  , sizeof(int) , "int" , NULL , 
				"LN_CHECKPOINT"
			},
	{"migration"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,migration)  , sizeof(int) , "int" , NULL , 
				"LN_MIGRATION"
			},
	{"report"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_launcher,report)  , sizeof(int) , "int" , NULL , 
				"LN_REPORT"
			},
	/* struct */
	{"sctk_runtime_config_struct_debugger" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_debugger) , NULL , sctk_runtime_config_struct_init_debugger},
	{"colors"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_debugger,colors)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"max_filename_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_debugger,max_filename_size)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"mpc_bt_sig"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_debugger,mpc_bt_sig)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_ft" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_ft) , NULL , sctk_runtime_config_struct_init_ft},
	{"enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ft,enabled)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_topological" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_topological) , NULL , sctk_runtime_config_struct_init_net_driver_topological},
	{"dummy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_topological,dummy)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_infiniband" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_infiniband) , NULL , sctk_runtime_config_struct_init_net_driver_infiniband},
	{"pkey"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,pkey)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"adm_port"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,adm_port)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"verbose_level"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,verbose_level)  , sizeof(int) , "int" , NULL , 
				"IB_VERBOSE"
			},
	{"eager_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,eager_limit)  , sizeof(int) , "int" , NULL , 
				"IB_EAGER_THRESH"
			},
	{"buffered_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,buffered_limit)  , sizeof(int) , "int" , NULL , 
				"IB_BUFFERED_THRESH"
			},
	{"qp_tx_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,qp_tx_depth)  , sizeof(int) , "int" , NULL , 
				"IB_TX_DEPTH"
			},
	{"qp_rx_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,qp_rx_depth)  , sizeof(int) , "int" , NULL , 
				"IB_RX_DEPTH"
			},
	{"cq_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,cq_depth)  , sizeof(int) , "int" , NULL , 
				"IB_CQ_DEPTH"
			},
	{"rdma_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_depth)  , sizeof(int) , "int" , NULL , 
				"IB_RDMA_DEPTH"
			},
	{"max_sg_sq"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_sg_sq)  , sizeof(int) , "int" , NULL , 
				"IB_MAX_SG_SQ"
			},
	{"max_sg_rq"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_sg_rq)  , sizeof(int) , "int" , NULL , 
				"IB_MAX_SG_RQ"
			},
	{"max_inline"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_inline)  , sizeof(int) , "int" , NULL , 
				"IB_MAX_INLINE"
			},
	{"rdma_resizing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing)  , sizeof(int) , "int" , NULL , 
				"IB_RDMA_RESIZING"
			},
	{"max_rdma_connections"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_rdma_connections)  , sizeof(int) , "int" , NULL , 
				"IB_RDMA_CONN"
			},
	{"max_rdma_resizing"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_rdma_resizing)  , sizeof(int) , "int" , NULL , 
				"IB_RDMA_MAX_RESIZING"
			},
	{"init_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_ibufs)  , sizeof(int) , "int" , NULL , 
				"IB_INIT_IBUF"
			},
	{"init_recv_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_recv_ibufs)  , sizeof(int) , "int" , NULL , 
				"IB_INIT_RECV_IBUF"
			},
	{"max_srq_ibufs_posted"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_srq_ibufs_posted)  , sizeof(int) , "int" , NULL , 
				"IB_MAX_SRQ_IBUF_POSTED"
			},
	{"max_srq_ibufs"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,max_srq_ibufs)  , sizeof(int) , "int" , NULL , 
				"IB_MAX_SRQ_IBUF"
			},
	{"srq_credit_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,srq_credit_limit)  , sizeof(int) , "int" , NULL , 
				"IB_SRQ_CREDIT_LIMIT"
			},
	{"srq_credit_thread_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,srq_credit_thread_limit)  , sizeof(int) , "int" , NULL , 
				"IB_SRQ_CREDIT_LIMIT_THREAD"
			},
	{"size_ibufs_chunk"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,size_ibufs_chunk)  , sizeof(int) , "int" , NULL , 
				"IB_IBUF_CHUNK"
			},
	{"init_mr"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,init_mr)  , sizeof(int) , "int" , NULL , 
				"IB_MMU_INIT"
			},
	{"steal"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,steal)  , sizeof(int) , "int" , NULL , 
				"IB_CAN_STEAL"
			},
	{"quiet_crash"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,quiet_crash)  , sizeof(int) , "int" , NULL , 
				"IB_QUIET_CRASH"
			},
	{"async_thread"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,async_thread)  , sizeof(int) , "int" , NULL , 
				"IB_ASYNC_THREAD"
			},
	{"wc_in_number"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,wc_in_number)  , sizeof(int) , "int" , NULL , 
				"IB_WC_IN"
			},
	{"wc_out_number"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,wc_out_number)  , sizeof(int) , "int" , NULL , 
				"IB_WC_OUT"
			},
	{"low_memory"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,low_memory)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdvz_protocol"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdvz_protocol)  , sizeof(enum ibv_rdvz_protocol) , "enum ibv_rdvz_protocol" , NULL , 
				NULL
			},
	{"rdma_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_min_size)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_max_size)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_min_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_min_nb)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_max_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_max_nb)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_resizing_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_min_size)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_resizing_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_max_size)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_resizing_min_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_min_nb)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma_resizing_max_nb"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,rdma_resizing_max_nb)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"size_recv_ibufs_chunk"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_infiniband,size_recv_ibufs_chunk)  , sizeof(int) , "int" , NULL , 
				"IB_RECV_IBUF_CHUNK"
			},
	/* struct */
	{"sctk_runtime_config_struct_ib_global" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_ib_global) , NULL , sctk_runtime_config_struct_init_ib_global},
	{"mmu_cache_enabled"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_enabled)  , sizeof(int) , "int" , NULL , 
				"IB_MMU_CHACHE_ENABLED"
			},
	{"mmu_cache_entry_count"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_entry_count)  , sizeof(int) , "int" , NULL , 
				"IB_MMU_CHACHE_COUNT"
			},
	{"mmu_cache_maximum_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_maximum_size)  , sizeof(size_t) , "size_t" , NULL , 
				"IB_MMU_CHACHE_MAX_SIZE"
			},
	{"mmu_cache_maximum_pin_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_ib_global,mmu_cache_maximum_pin_size)  , sizeof(size_t) , "size_t" , NULL , 
				"IB_MMU_CHACHE_MAX_PIN_SIZE"
			},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_portals" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_portals) , NULL , sctk_runtime_config_struct_init_net_driver_portals},
	{"eager_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_portals,eager_limit)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"min_comms"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_portals,min_comms)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"block_cut"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_portals,block_cut)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_tcp" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , NULL , sctk_runtime_config_struct_init_net_driver_tcp},
	{"tcpoib"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_tcp,tcpoib)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_tcp_rdma" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp_rdma) , NULL , sctk_runtime_config_struct_init_net_driver_tcp_rdma},
	{"tcpoib"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_tcp_rdma,tcpoib)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_shm" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_shm) , NULL , sctk_runtime_config_struct_init_net_driver_shm},
	{"buffered_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,buffered_priority)  , sizeof(int) , "int" , NULL , 
				"SHM_BUFF_PRIORITY"
			},
	{"buffered_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,buffered_min_size)  , sizeof(int) , "int" , NULL , 
				"SHM_BUFF_MIN_SIZE"
			},
	{"buffered_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,buffered_max_size)  , sizeof(int) , "int" , NULL , 
				"SHM_BUFF_MAX_SIZE"
			},
	{"buffered_zerocopy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,buffered_zerocopy)  , sizeof(int) , "int" , NULL , 
				"SHM_BUFF_ZEROCOPY"
			},
	{"cma_enable"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,cma_enable)  , sizeof(int) , "int" , NULL , 
				"SHM_CMA_ENABLE"
			},
	{"cma_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,cma_priority)  , sizeof(int) , "int" , NULL , 
				"SHM_CMA_PRIORITY"
			},
	{"cma_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,cma_min_size)  , sizeof(int) , "int" , NULL , 
				"SHM_CMA_MIN_SIZE"
			},
	{"cma_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,cma_max_size)  , sizeof(int) , "int" , NULL , 
				"SHM_CMA_MAX_SIZE"
			},
	{"cma_zerocopy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,cma_zerocopy)  , sizeof(int) , "int" , NULL , 
				"SHM_CMA_ZEROCOPY"
			},
	{"frag_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,frag_priority)  , sizeof(int) , "int" , NULL , 
				"SHM_FRAG_PRIORITY"
			},
	{"frag_min_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,frag_min_size)  , sizeof(int) , "int" , NULL , 
				"SHM_FRAG_MIN_SIZE"
			},
	{"frag_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,frag_max_size)  , sizeof(int) , "int" , NULL , 
				"SHM_FRAG_MAX_SIZE"
			},
	{"frag_zerocopy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,frag_zerocopy)  , sizeof(int) , "int" , NULL , 
				"SHM_FRAG_ZEROCOPY"
			},
	{"shmem_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,shmem_size)  , sizeof(int) , "int" , NULL , 
				"SHM_SIZE"
			},
	{"cells_num"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_shm,cells_num)  , sizeof(int) , "int" , NULL , 
				"SHM_CELLS"
			},
	/* union */
	{"sctk_runtime_config_struct_net_driver" , SCTK_CONFIG_META_TYPE_UNION , 0  , sizeof(struct sctk_runtime_config_struct_net_driver) , NULL , sctk_runtime_config_struct_init_net_driver},
	{"infiniband"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_infiniband  , sizeof(struct sctk_runtime_config_struct_net_driver_infiniband) , "sctk_runtime_config_struct_net_driver_infiniband" , sctk_runtime_config_struct_init_net_driver_infiniband},
	{"portals"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_portals  , sizeof(struct sctk_runtime_config_struct_net_driver_portals) , "sctk_runtime_config_struct_net_driver_portals" , sctk_runtime_config_struct_init_net_driver_portals},
	{"tcp"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcp  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp) , "sctk_runtime_config_struct_net_driver_tcp" , sctk_runtime_config_struct_init_net_driver_tcp},
	{"tcprdma"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_tcprdma  , sizeof(struct sctk_runtime_config_struct_net_driver_tcp_rdma) , "sctk_runtime_config_struct_net_driver_tcp_rdma" , sctk_runtime_config_struct_init_net_driver_tcp_rdma},
	{"shm"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_shm  , sizeof(struct sctk_runtime_config_struct_net_driver_shm) , "sctk_runtime_config_struct_net_driver_shm" , sctk_runtime_config_struct_init_net_driver_shm},
	{"topological"     , SCTK_CONFIG_META_TYPE_UNION_ENTRY  , SCTK_RTCFG_net_driver_topological  , sizeof(struct sctk_runtime_config_struct_net_driver_topological) , "sctk_runtime_config_struct_net_driver_topological" , sctk_runtime_config_struct_init_net_driver_topological},
	/* struct */
	{"sctk_runtime_config_struct_net_driver_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_driver_config) , NULL , sctk_runtime_config_struct_init_net_driver_config},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,name)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"driver"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_driver_config,driver)  , sizeof(struct sctk_runtime_config_struct_net_driver) , "sctk_runtime_config_struct_net_driver" , sctk_runtime_config_struct_init_net_driver , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_gate_boolean" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_boolean) , NULL , sctk_runtime_config_struct_init_gate_boolean},
	{"value"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_boolean,value)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_boolean,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_gate_probabilistic" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_probabilistic) , NULL , sctk_runtime_config_struct_init_gate_probabilistic},
	{"probability"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_probabilistic,probability)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_probabilistic,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_gate_min_size" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_min_size) , NULL , sctk_runtime_config_struct_init_gate_min_size},
	{"value"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_min_size,value)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_min_size,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_gate_max_size" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_max_size) , NULL , sctk_runtime_config_struct_init_gate_max_size},
	{"value"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_max_size,value)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_max_size,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_gate_message_type" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_message_type) , NULL , sctk_runtime_config_struct_init_gate_message_type},
	{"process"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,process)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"task"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,task)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"emulated_rma"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,emulated_rma)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"common"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,common)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_message_type,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_gate_user" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_gate_user) , NULL , sctk_runtime_config_struct_init_gate_user},
	{"gatefunc"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_gate_user,gatefunc)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
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
	{"range"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_topological_polling,range)  , sizeof(enum rail_topological_polling_level) , "enum rail_topological_polling_level" , NULL , 
				NULL
			},
	{"trigger"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_topological_polling,trigger)  , sizeof(enum rail_topological_polling_level) , "enum rail_topological_polling_level" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_net_rail" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_rail) , NULL , sctk_runtime_config_struct_init_net_rail},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,name)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"device"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,device)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"idle_polling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,idle_polling)  , sizeof(struct sctk_runtime_config_struct_topological_polling) , "sctk_runtime_config_struct_topological_polling" , sctk_runtime_config_struct_init_topological_polling , 
				NULL
			},
	{"any_source_polling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,any_source_polling)  , sizeof(struct sctk_runtime_config_struct_topological_polling) , "sctk_runtime_config_struct_topological_polling" , sctk_runtime_config_struct_init_topological_polling , 
				NULL
			},
	{"topology"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,topology)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"ondemand"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,ondemand)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"rdma"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,rdma)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"config"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,config)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"gates"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,gates) , sizeof(struct sctk_runtime_config_struct_net_gate) , "sctk_runtime_config_struct_net_gate" , "gate"},
	{"subrails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_rail,subrails) , sizeof(struct sctk_runtime_config_struct_net_rail) , "sctk_runtime_config_struct_net_rail" , "subrail"},
	/* struct */
	{"sctk_runtime_config_struct_net_cli_option" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_net_cli_option) , NULL , sctk_runtime_config_struct_init_net_cli_option},
	{"name"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_cli_option,name)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_net_cli_option,rails) , sizeof(char *) , "char *" , "rail"},
	/* struct */
	{"sctk_runtime_config_struct_networks" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_networks) , NULL , sctk_runtime_config_struct_init_networks},
	{"configs"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,configs) , sizeof(struct sctk_runtime_config_struct_net_driver_config) , "sctk_runtime_config_struct_net_driver_config" , "config"},
	{"rails"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,rails) , sizeof(struct sctk_runtime_config_struct_net_rail) , "sctk_runtime_config_struct_net_rail" , "rail"},
	{"cli_options"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_networks,cli_options) , sizeof(struct sctk_runtime_config_struct_net_cli_option) , "sctk_runtime_config_struct_net_cli_option" , "cli_option"},
	/* struct */
	{"sctk_runtime_config_struct_inter_thread_comm" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_inter_thread_comm) , NULL , sctk_runtime_config_struct_init_inter_thread_comm},
	{"barrier_arity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,barrier_arity)  , sizeof(int) , "int" , NULL , 
				"ITT_BARRIER_ARITY"
			},
	{"broadcast_arity_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,broadcast_arity_max)  , sizeof(int) , "int" , NULL , 
				"ITT_BROADCAST_ARITY"
			},
	{"broadcast_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,broadcast_max_size)  , sizeof(int) , "int" , NULL , 
				"ITT_BROADCAST_MAX_SIZE"
			},
	{"broadcast_check_threshold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,broadcast_check_threshold)  , sizeof(int) , "int" , NULL , 
				"ITT_BROADCAST_CHECK_TH"
			},
	{"allreduce_arity_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,allreduce_arity_max)  , sizeof(int) , "int" , NULL , 
				"ITT_ALLREDUCE_ARITY_MAX"
			},
	{"allreduce_max_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,allreduce_max_size)  , sizeof(int) , "int" , NULL , 
				"ITT_ALLREDUCE_MAX_SIZE"
			},
	{"allreduce_check_threshold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,allreduce_check_threshold)  , sizeof(int) , "int" , NULL , 
				"ITT_ALLREDUCE_CHECK_TH"
			},
	{"ALLREDUCE_MAX_SLOT"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,ALLREDUCE_MAX_SLOT)  , sizeof(int) , "int" , NULL , 
				"ITT_ALLREDUCE_MAX_SLOT"
			},
	{"collectives_init_hook"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_inter_thread_comm,collectives_init_hook)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_low_level_comm" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_low_level_comm) , NULL , sctk_runtime_config_struct_init_low_level_comm},
	{"checksum"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,checksum)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"send_msg"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,send_msg)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"network_mode"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,network_mode)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"dyn_reordering"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,dyn_reordering)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"enable_idle_polling"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,enable_idle_polling)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"ib_global"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_low_level_comm,ib_global)  , sizeof(struct sctk_runtime_config_struct_ib_global) , "sctk_runtime_config_struct_ib_global" , sctk_runtime_config_struct_init_ib_global , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_collectives_shm" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_collectives_shm) , NULL , sctk_runtime_config_struct_init_collectives_shm},
	{"barrier_intra_shm"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,barrier_intra_shm)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"bcast_intra_shm"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,bcast_intra_shm)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoallv_intra_shm"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,alltoallv_intra_shm)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"gatherv_intra_shm"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,gatherv_intra_shm)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"scatterv_intra_shm"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,scatterv_intra_shm)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_intra_shm"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,reduce_intra_shm)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"topo_tree_arity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,topo_tree_arity)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_TOPO_TREE_ARITY"
			},
	{"topo_tree_dump"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,topo_tree_dump)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_TOPO_TREE_DUMP"
			},
	{"coll_force_nocommute"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,coll_force_nocommute)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_NO_COMMUTE"
			},
	{"reduce_pipelined_blocks"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,reduce_pipelined_blocks)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_REDUCE_PIPELINED_BLOCKS"
			},
	{"reduce_pipelined_tresh"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,reduce_pipelined_tresh)  , sizeof(size_t) , "size_t" , NULL , 
				"MPI_COLL_REDUCE_INTERLEAVE_TRSH"
			},
	{"reduce_interleave"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,reduce_interleave)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_REDUCE_INTERLEAVE"
			},
	{"bcast_interleave"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_shm,bcast_interleave)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_BCAST_INTERLEAVE"
			},
	/* struct */
	{"sctk_runtime_config_struct_collectives_intra" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_collectives_intra) , NULL , sctk_runtime_config_struct_init_collectives_intra},
	{"barrier_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,barrier_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"barrier_intra_for_trsh"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,barrier_intra_for_trsh)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_BARRIER_FOR_TRSH"
			},
	{"bcast_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,bcast_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"bcast_intra_for_trsh"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,bcast_intra_for_trsh)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_BCAST_FOR_TRSH"
			},
	{"bcast_intra_for_count_trsh"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,bcast_intra_for_count_trsh)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_BCAST_FOR_ELEM_TRSH"
			},
	{"allgather_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,allgather_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"allgatherv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,allgatherv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoall_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,alltoall_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoallv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,alltoallv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoallw_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,alltoallw_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"gather_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,gather_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"gatherv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,gatherv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"scatter_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,scatter_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"scatterv_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,scatterv_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"scan_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,scan_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"exscan_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,exscan_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_intra_for_trsh"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_intra_for_trsh)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_REDUCE_FOR_TRSH"
			},
	{"reduce_intra_for_count_trsh"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_intra_for_count_trsh)  , sizeof(int) , "int" , NULL , 
				"MPI_COLL_REDUCE_FOR_ELEM_TRSH"
			},
	{"allreduce_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,allreduce_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_scatter_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_scatter_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_scatter_block_intra"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_intra,reduce_scatter_block_intra)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_collectives_inter" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_collectives_inter) , NULL , sctk_runtime_config_struct_init_collectives_inter},
	{"barrier_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,barrier_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"bcast_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,bcast_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"allgather_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,allgather_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"allgatherv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,allgatherv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoall_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,alltoall_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoallv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,alltoallv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"alltoallw_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,alltoallw_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"gather_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,gather_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"gatherv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,gatherv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"scatter_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,scatter_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"scatterv_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,scatterv_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,reduce_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"allreduce_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,allreduce_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_scatter_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,reduce_scatter_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"reduce_scatter_block_inter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_collectives_inter,reduce_scatter_block_inter)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_nbc" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_nbc) , NULL , sctk_runtime_config_struct_init_nbc},
	{"use_progress_thread"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,use_progress_thread)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"progress_thread_binding"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,progress_thread_binding)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	{"use_egreq_bcast"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,use_egreq_bcast)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"use_egreq_scatter"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,use_egreq_scatter)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"use_egreq_gather"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,use_egreq_gather)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"use_egreq_reduce"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,use_egreq_reduce)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"use_egreq_barrier"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_nbc,use_egreq_barrier)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_mpi_rma" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_mpi_rma) , NULL , sctk_runtime_config_struct_init_mpi_rma},
	{"alloc_mem_pool_enable"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpi_rma,alloc_mem_pool_enable)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"alloc_mem_pool_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpi_rma,alloc_mem_pool_size)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"alloc_mem_pool_autodetect"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpi_rma,alloc_mem_pool_autodetect)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"alloc_mem_pool_force_process_linear"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpi_rma,alloc_mem_pool_force_process_linear)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"alloc_mem_pool_per_process_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpi_rma,alloc_mem_pool_per_process_size)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"win_thread_pool_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpi_rma,win_thread_pool_max)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_mpc" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_mpc) , NULL , sctk_runtime_config_struct_init_mpc},
	{"log_debug"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpc,log_debug)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"hard_checking"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpc,hard_checking)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"buffering"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_mpc,buffering)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_openmp" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_openmp) , NULL , sctk_runtime_config_struct_init_openmp},
	{"vp"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,vp)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"schedule"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,schedule)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"nb_threads"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,nb_threads)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"adjustment"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,adjustment)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"proc_bind"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,proc_bind)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"nested"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,nested)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"stack_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,stack_size)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"wait_policy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,wait_policy)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"thread_limit"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,thread_limit)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"max_active_levels"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_active_levels)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"tree"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,tree)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"max_threads"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_threads)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"max_alive_for_dyn"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_for_dyn)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"max_alive_for_guided"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_for_guided)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"max_alive_sections"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_sections)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"max_alive_single"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,max_alive_single)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"warn_nested"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,warn_nested)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"mode"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,mode)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"affinity"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,affinity)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"omp_new_task_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,omp_new_task_depth)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"omp_untied_task_depth"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,omp_untied_task_depth)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"omp_task_larceny_mode"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,omp_task_larceny_mode)  , sizeof(enum mpcomp_task_larceny_mode_t) , "enum mpcomp_task_larceny_mode_t" , NULL , 
				NULL
			},
	{"omp_task_nesting_max"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,omp_task_nesting_max)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"mpcomp_task_max_delayed"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_openmp,mpcomp_task_max_delayed)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_profiler" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_profiler) , NULL , sctk_runtime_config_struct_init_profiler},
	{"file_prefix"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,file_prefix)  , sizeof(char *) , "char *" , NULL , 
				NULL
			},
	{"append_date"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,append_date)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"color_stdout"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,color_stdout)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"level_colors"     , SCTK_CONFIG_META_TYPE_ARRAY  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_profiler,level_colors) , sizeof(char *) , "char *" , "level"},
	/* struct */
	{"sctk_runtime_config_struct_thread" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_thread) , NULL , sctk_runtime_config_struct_init_thread},
	{"spin_delay"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,spin_delay)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"interval"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,interval)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"kthread_stack_size"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,kthread_stack_size)  , sizeof(size_t) , "size_t" , NULL , 
				NULL
			},
	{"placement_policy"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_thread,placement_policy)  , sizeof(struct sctk_runtime_config_funcptr) , "funcptr" , NULL , 
				NULL
			},
	/* struct */
	{"sctk_runtime_config_struct_scheduler" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_runtime_config_struct_scheduler) , NULL , sctk_runtime_config_struct_init_scheduler},
	{"timestamp_threshold"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,timestamp_threshold)  , sizeof(double) , "double" , NULL , 
				NULL
			},
	{"task_polling_thread_basic_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,task_polling_thread_basic_priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"task_polling_thread_basic_priority_step"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,task_polling_thread_basic_priority_step)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"task_polling_thread_current_priority_step"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,task_polling_thread_current_priority_step)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"sched_NBC_Pthread_basic_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,sched_NBC_Pthread_basic_priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"sched_NBC_Pthread_basic_priority_step"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,sched_NBC_Pthread_basic_priority_step)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"sched_NBC_Pthread_current_priority_step"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,sched_NBC_Pthread_current_priority_step)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"mpi_basic_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,mpi_basic_priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"omp_basic_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,omp_basic_priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"posix_basic_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,posix_basic_priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	{"progress_basic_priority"     , SCTK_CONFIG_META_TYPE_PARAM  , sctk_runtime_config_get_offset_of_member(struct sctk_runtime_config_struct_scheduler,progress_basic_priority)  , sizeof(int) , "int" , NULL , 
				NULL
			},
	/* end marker */
	{NULL , SCTK_CONFIG_META_TYPE_END , 0 , 0 , NULL,  NULL}
};


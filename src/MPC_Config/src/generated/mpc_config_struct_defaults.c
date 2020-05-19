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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "mpc_config_struct.h"
#include "mpc_config_struct_defaults.h"
#include "runtime_config_mapper.h"

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
	obj->mpc_bt_sig = 1;
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

                        sctk_runtime_config_mpit_bind_variable( "LN_VERBOSITY",
                                                                sizeof(obj->verbosity ),
                                                                &obj->verbosity);
				obj->banner = true;

                        sctk_runtime_config_mpit_bind_variable( "LN_BANNER",
                                                                sizeof(obj->banner ),
                                                                &obj->banner);
				obj->autokill = 0;

                        sctk_runtime_config_mpit_bind_variable( "LN_AUTOKILL",
                                                                sizeof(obj->autokill ),
                                                                &obj->autokill);
				obj->user_launchers = "~/.mpc/";
	obj->disable_rand_addr = true;

                        sctk_runtime_config_mpit_bind_variable( "LN_ADDR_RAND_DISABLE",
                                                                sizeof(obj->disable_rand_addr ),
                                                                &obj->disable_rand_addr);
				obj->thread_init.name = "sctk_use_ethread_mxn";
	*(void **) &(obj->thread_init.value) = sctk_runtime_config_get_symbol("sctk_use_ethread_mxn");
	obj->nb_task = 1;

                        sctk_runtime_config_mpit_bind_variable( "LN_MPI_TASK",
                                                                sizeof(obj->nb_task ),
                                                                &obj->nb_task);
				obj->nb_process = 1;

                        sctk_runtime_config_mpit_bind_variable( "LN_MPC_PROCESS",
                                                                sizeof(obj->nb_process ),
                                                                &obj->nb_process);
				obj->nb_processor = 0;

                        sctk_runtime_config_mpit_bind_variable( "LN_MPC_VP",
                                                                sizeof(obj->nb_processor ),
                                                                &obj->nb_processor);
				obj->nb_node = 1;

                        sctk_runtime_config_mpit_bind_variable( "LN_MPC_NODE",
                                                                sizeof(obj->nb_node ),
                                                                &obj->nb_node);
				obj->launcher = "none";
	obj->profiling = "stdout";
	obj->enable_smt = false;

                        sctk_runtime_config_mpit_bind_variable( "LN_ENABLE_SMT",
                                                                sizeof(obj->enable_smt ),
                                                                &obj->enable_smt);
				obj->restart = false;

                        sctk_runtime_config_mpit_bind_variable( "LN_RESTART",
                                                                sizeof(obj->restart ),
                                                                &obj->restart);
				obj->checkpoint = false;

                        sctk_runtime_config_mpit_bind_variable( "LN_CHECKPOINT",
                                                                sizeof(obj->checkpoint ),
                                                                &obj->checkpoint);
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_shm_shared(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_shm_shared * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_intra_shared_node.name = "__INTERNAL__PMPI_Barrier_intra_shared_node";
	*(void **) &(obj->barrier_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_intra_shared_node");
	obj->bcast_intra_shared_node.name = "__INTERNAL__PMPI_Bcast_intra_shared_node";
	*(void **) &(obj->bcast_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_intra_shared_node");
	obj->alltoall_intra_shared_node.name = "__INTERNAL__PMPI_Alltoall_intra_shared_node";
	*(void **) &(obj->alltoall_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoall_intra_shared_node");
	obj->scatter_intra_shared_node.name = "__INTERNAL__PMPI_Scatter_intra_shared_node";
	*(void **) &(obj->scatter_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatter_intra_shared_node");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_shm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_shm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->bcast_intra_shm.name = "__INTERNAL__PMPI_Bcast_intra_shm";
	*(void **) &(obj->bcast_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_intra_shm");
	obj->alltoallv_intra_shm.name = "__INTERNAL__PMPI_Alltoallv_intra_shm";
	*(void **) &(obj->alltoallv_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallv_intra_shm");
	obj->gatherv_intra_shm.name = "__INTERNAL__PMPI_Gatherv_intra_shm";
	*(void **) &(obj->gatherv_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gatherv_intra_shm");
	obj->scatterv_intra_shm.name = "__INTERNAL__PMPI_Scatterv_intra_shm";
	*(void **) &(obj->scatterv_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatterv_intra_shm");
	obj->reduce_intra_shm.name = "__INTERNAL__PMPI_Reduce_shm";
	*(void **) &(obj->reduce_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_shm");
	obj->topo_tree_arity = -1;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_TOPO_TREE_ARITY",
                                                                sizeof(obj->topo_tree_arity ),
                                                                &obj->topo_tree_arity);
				obj->topo_tree_dump = false;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_TOPO_TREE_DUMP",
                                                                sizeof(obj->topo_tree_dump ),
                                                                &obj->topo_tree_dump);
				obj->coll_force_nocommute = false;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_NO_COMMUTE",
                                                                sizeof(obj->coll_force_nocommute ),
                                                                &obj->coll_force_nocommute);
				obj->reduce_pipelined_blocks = 16;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_REDUCE_PIPELINED_BLOCKS",
                                                                sizeof(obj->reduce_pipelined_blocks ),
                                                                &obj->reduce_pipelined_blocks);
				obj->reduce_pipelined_tresh = sctk_runtime_config_map_entry_parse_size("1KB");

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_REDUCE_INTERLEAVE_TRSH",
                                                                sizeof(obj->reduce_pipelined_tresh ),
                                                                &obj->reduce_pipelined_tresh);
				obj->reduce_interleave = 16;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_REDUCE_INTERLEAVE",
                                                                sizeof(obj->reduce_interleave ),
                                                                &obj->reduce_interleave);
				obj->bcast_interleave = 16;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_BCAST_INTERLEAVE",
                                                                sizeof(obj->bcast_interleave ),
                                                                &obj->bcast_interleave);
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_intra(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_intra * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_intra.name = "__INTERNAL__PMPI_Barrier_intra";
	*(void **) &(obj->barrier_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_intra");
	obj->barrier_intra_for_trsh = 33;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_BARRIER_FOR_TRSH",
                                                                sizeof(obj->barrier_intra_for_trsh ),
                                                                &obj->barrier_intra_for_trsh);
				obj->bcast_intra.name = "__INTERNAL__PMPI_Bcast_intra";
	*(void **) &(obj->bcast_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_intra");
	obj->bcast_intra_for_trsh = 33;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_BCAST_FOR_TRSH",
                                                                sizeof(obj->bcast_intra_for_trsh ),
                                                                &obj->bcast_intra_for_trsh);
				obj->bcast_intra_for_count_trsh = 1024;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_BCAST_FOR_ELEM_TRSH",
                                                                sizeof(obj->bcast_intra_for_count_trsh ),
                                                                &obj->bcast_intra_for_count_trsh);
				obj->allgather_intra.name = "__INTERNAL__PMPI_Allgather_intra";
	*(void **) &(obj->allgather_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgather_intra");
	obj->allgatherv_intra.name = "__INTERNAL__PMPI_Allgatherv_intra";
	*(void **) &(obj->allgatherv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgatherv_intra");
	obj->alltoall_intra.name = "__INTERNAL__PMPI_Alltoall_intra";
	*(void **) &(obj->alltoall_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoall_intra");
	obj->alltoallv_intra.name = "__INTERNAL__PMPI_Alltoallv_intra";
	*(void **) &(obj->alltoallv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallv_intra");
	obj->alltoallw_intra.name = "__INTERNAL__PMPI_Alltoallw_intra";
	*(void **) &(obj->alltoallw_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallw_intra");
	obj->gather_intra.name = "__INTERNAL__PMPI_Gather_intra";
	*(void **) &(obj->gather_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gather_intra");
	obj->gatherv_intra.name = "__INTERNAL__PMPI_Gatherv_intra";
	*(void **) &(obj->gatherv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gatherv_intra");
	obj->scatter_intra.name = "__INTERNAL__PMPI_Scatter_intra";
	*(void **) &(obj->scatter_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatter_intra");
	obj->scatterv_intra.name = "__INTERNAL__PMPI_Scatterv_intra";
	*(void **) &(obj->scatterv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatterv_intra");
	obj->scan_intra.name = "__INTERNAL__PMPI_Scan_intra";
	*(void **) &(obj->scan_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scan_intra");
	obj->exscan_intra.name = "__INTERNAL__PMPI_Exscan_intra";
	*(void **) &(obj->exscan_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Exscan_intra");
	obj->reduce_intra.name = "__INTERNAL__PMPI_Reduce_intra";
	*(void **) &(obj->reduce_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_intra");
	obj->reduce_intra_for_trsh = 33;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_REDUCE_FOR_TRSH",
                                                                sizeof(obj->reduce_intra_for_trsh ),
                                                                &obj->reduce_intra_for_trsh);
				obj->reduce_intra_for_count_trsh = 1024;

                        sctk_runtime_config_mpit_bind_variable( "MPI_COLL_REDUCE_FOR_ELEM_TRSH",
                                                                sizeof(obj->reduce_intra_for_count_trsh ),
                                                                &obj->reduce_intra_for_count_trsh);
				obj->allreduce_intra.name = "__INTERNAL__PMPI_Allreduce_intra";
	*(void **) &(obj->allreduce_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allreduce_intra");
	obj->reduce_scatter_intra.name = "__INTERNAL__PMPI_Reduce_scatter_intra";
	*(void **) &(obj->reduce_scatter_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_intra");
	obj->reduce_scatter_block_intra.name = "__INTERNAL__PMPI_Reduce_scatter_block_intra";
	*(void **) &(obj->reduce_scatter_block_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_block_intra");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_inter(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_inter * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_inter.name = "__INTERNAL__PMPI_Barrier_inter";
	*(void **) &(obj->barrier_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_inter");
	obj->bcast_inter.name = "__INTERNAL__PMPI_Bcast_inter";
	*(void **) &(obj->bcast_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_inter");
	obj->allgather_inter.name = "__INTERNAL__PMPI_Allgather_inter";
	*(void **) &(obj->allgather_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgather_inter");
	obj->allgatherv_inter.name = "__INTERNAL__PMPI_Allgatherv_inter";
	*(void **) &(obj->allgatherv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgatherv_inter");
	obj->alltoall_inter.name = "__INTERNAL__PMPI_Alltoall_inter";
	*(void **) &(obj->alltoall_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoall_inter");
	obj->alltoallv_inter.name = "__INTERNAL__PMPI_Alltoallv_inter";
	*(void **) &(obj->alltoallv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallv_inter");
	obj->alltoallw_inter.name = "__INTERNAL__PMPI_Alltoallw_inter";
	*(void **) &(obj->alltoallw_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallw_inter");
	obj->gather_inter.name = "__INTERNAL__PMPI_Gather_inter";
	*(void **) &(obj->gather_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gather_inter");
	obj->gatherv_inter.name = "__INTERNAL__PMPI_Gatherv_inter";
	*(void **) &(obj->gatherv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gatherv_inter");
	obj->scatter_inter.name = "__INTERNAL__PMPI_Scatter_inter";
	*(void **) &(obj->scatter_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatter_inter");
	obj->scatterv_inter.name = "__INTERNAL__PMPI_Scatterv_inter";
	*(void **) &(obj->scatterv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatterv_inter");
	obj->reduce_inter.name = "__INTERNAL__PMPI_Reduce_inter";
	*(void **) &(obj->reduce_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_inter");
	obj->allreduce_inter.name = "__INTERNAL__PMPI_Allreduce_inter";
	*(void **) &(obj->allreduce_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allreduce_inter");
	obj->reduce_scatter_inter.name = "__INTERNAL__PMPI_Reduce_scatter_inter";
	*(void **) &(obj->reduce_scatter_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_inter");
	obj->reduce_scatter_block_inter.name = "__INTERNAL__PMPI_Reduce_scatter_block_inter";
	*(void **) &(obj->reduce_scatter_block_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_block_inter");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_nbc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_nbc * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->use_progress_thread = 0;
	obj->progress_thread_binding.name = "sctk_get_progress_thread_binding_bind";
	*(void **) &(obj->progress_thread_binding.value) = sctk_runtime_config_get_symbol("sctk_get_progress_thread_binding_bind");
	obj->use_egreq_bcast = 0;
	obj->use_egreq_scatter = 0;
	obj->use_egreq_gather = 0;
	obj->use_egreq_reduce = 0;
	obj->use_egreq_barrier = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_mpi_rma(void * struct_ptr)
{
	struct sctk_runtime_config_struct_mpi_rma * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->alloc_mem_pool_enable = 1;
	obj->alloc_mem_pool_size = sctk_runtime_config_map_entry_parse_size("1MB");
	obj->alloc_mem_pool_autodetect = 1;
	obj->alloc_mem_pool_force_process_linear = 0;
	obj->alloc_mem_pool_per_process_size = sctk_runtime_config_map_entry_parse_size("1MB");
	obj->win_thread_pool_max = 2;
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
	obj->disable_message_buffering = false;

                        sctk_runtime_config_mpit_bind_variable( "MPC_DISABLE_BUFFERING",
                                                                sizeof(obj->disable_message_buffering ),
                                                                &obj->disable_message_buffering);
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_ft(void * struct_ptr)
{
	struct sctk_runtime_config_struct_ft * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_topological(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_topological * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->dummy = 0;
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
	obj->pkey = "undefined";
	obj->adm_port = 1;
	obj->verbose_level = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_VERBOSE",
                                                                sizeof(obj->verbose_level ),
                                                                &obj->verbose_level);
				obj->eager_limit = sctk_runtime_config_map_entry_parse_size("12KB");

                        sctk_runtime_config_mpit_bind_variable( "IB_EAGER_THRESH",
                                                                sizeof(obj->eager_limit ),
                                                                &obj->eager_limit);
				obj->buffered_limit = sctk_runtime_config_map_entry_parse_size("26KB");

                        sctk_runtime_config_mpit_bind_variable( "IB_BUFFERED_THRESH",
                                                                sizeof(obj->buffered_limit ),
                                                                &obj->buffered_limit);
				obj->qp_tx_depth = 15000;

                        sctk_runtime_config_mpit_bind_variable( "IB_TX_DEPTH",
                                                                sizeof(obj->qp_tx_depth ),
                                                                &obj->qp_tx_depth);
				obj->qp_rx_depth = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_RX_DEPTH",
                                                                sizeof(obj->qp_rx_depth ),
                                                                &obj->qp_rx_depth);
				obj->cq_depth = 40000;

                        sctk_runtime_config_mpit_bind_variable( "IB_CQ_DEPTH",
                                                                sizeof(obj->cq_depth ),
                                                                &obj->cq_depth);
				obj->rdma_depth = 16;

                        sctk_runtime_config_mpit_bind_variable( "IB_RDMA_DEPTH",
                                                                sizeof(obj->rdma_depth ),
                                                                &obj->rdma_depth);
				obj->max_sg_sq = 4;

                        sctk_runtime_config_mpit_bind_variable( "IB_MAX_SG_SQ",
                                                                sizeof(obj->max_sg_sq ),
                                                                &obj->max_sg_sq);
				obj->max_sg_rq = 4;

                        sctk_runtime_config_mpit_bind_variable( "IB_MAX_SG_RQ",
                                                                sizeof(obj->max_sg_rq ),
                                                                &obj->max_sg_rq);
				obj->max_inline = sctk_runtime_config_map_entry_parse_size("128B");

                        sctk_runtime_config_mpit_bind_variable( "IB_MAX_INLINE",
                                                                sizeof(obj->max_inline ),
                                                                &obj->max_inline);
				obj->rdma_resizing = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_RDMA_RESIZING",
                                                                sizeof(obj->rdma_resizing ),
                                                                &obj->rdma_resizing);
				obj->max_rdma_connections = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_RDMA_CONN",
                                                                sizeof(obj->max_rdma_connections ),
                                                                &obj->max_rdma_connections);
				obj->max_rdma_resizing = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_RDMA_MAX_RESIZING",
                                                                sizeof(obj->max_rdma_resizing ),
                                                                &obj->max_rdma_resizing);
				obj->init_ibufs = 1000;

                        sctk_runtime_config_mpit_bind_variable( "IB_INIT_IBUF",
                                                                sizeof(obj->init_ibufs ),
                                                                &obj->init_ibufs);
				obj->init_recv_ibufs = 200;

                        sctk_runtime_config_mpit_bind_variable( "IB_INIT_RECV_IBUF",
                                                                sizeof(obj->init_recv_ibufs ),
                                                                &obj->init_recv_ibufs);
				obj->max_srq_ibufs_posted = 1500;

                        sctk_runtime_config_mpit_bind_variable( "IB_MAX_SRQ_IBUF_POSTED",
                                                                sizeof(obj->max_srq_ibufs_posted ),
                                                                &obj->max_srq_ibufs_posted);
				obj->max_srq_ibufs = 1000;

                        sctk_runtime_config_mpit_bind_variable( "IB_MAX_SRQ_IBUF",
                                                                sizeof(obj->max_srq_ibufs ),
                                                                &obj->max_srq_ibufs);
				obj->srq_credit_limit = 500;

                        sctk_runtime_config_mpit_bind_variable( "IB_SRQ_CREDIT_LIMIT",
                                                                sizeof(obj->srq_credit_limit ),
                                                                &obj->srq_credit_limit);
				obj->srq_credit_thread_limit = 100;

                        sctk_runtime_config_mpit_bind_variable( "IB_SRQ_CREDIT_LIMIT_THREAD",
                                                                sizeof(obj->srq_credit_thread_limit ),
                                                                &obj->srq_credit_thread_limit);
				obj->size_ibufs_chunk = 100;

                        sctk_runtime_config_mpit_bind_variable( "IB_IBUF_CHUNK",
                                                                sizeof(obj->size_ibufs_chunk ),
                                                                &obj->size_ibufs_chunk);
				obj->init_mr = 400;

                        sctk_runtime_config_mpit_bind_variable( "IB_MMU_INIT",
                                                                sizeof(obj->init_mr ),
                                                                &obj->init_mr);
				obj->steal = 2;

                        sctk_runtime_config_mpit_bind_variable( "IB_CAN_STEAL",
                                                                sizeof(obj->steal ),
                                                                &obj->steal);
				obj->quiet_crash = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_QUIET_CRASH",
                                                                sizeof(obj->quiet_crash ),
                                                                &obj->quiet_crash);
				obj->async_thread = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_ASYNC_THREAD",
                                                                sizeof(obj->async_thread ),
                                                                &obj->async_thread);
				obj->wc_in_number = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_WC_IN",
                                                                sizeof(obj->wc_in_number ),
                                                                &obj->wc_in_number);
				obj->wc_out_number = 0;

                        sctk_runtime_config_mpit_bind_variable( "IB_WC_OUT",
                                                                sizeof(obj->wc_out_number ),
                                                                &obj->wc_out_number);
				obj->low_memory = false;
	obj->rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
	obj->rdma_min_size = sctk_runtime_config_map_entry_parse_size("1KB");
	obj->rdma_max_size = sctk_runtime_config_map_entry_parse_size("4KB");
	obj->rdma_min_nb = 8;
	obj->rdma_max_nb = 32;
	obj->rdma_resizing_min_size = sctk_runtime_config_map_entry_parse_size("1KB");
	obj->rdma_resizing_max_size = sctk_runtime_config_map_entry_parse_size("4KB");
	obj->rdma_resizing_min_nb = 8;
	obj->rdma_resizing_max_nb = 32;
	obj->size_recv_ibufs_chunk = 400;

                        sctk_runtime_config_mpit_bind_variable( "IB_RECV_IBUF_CHUNK",
                                                                sizeof(obj->size_recv_ibufs_chunk ),
                                                                &obj->size_recv_ibufs_chunk);
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

                        sctk_runtime_config_mpit_bind_variable( "IB_MMU_CHACHE_ENABLED",
                                                                sizeof(obj->mmu_cache_enabled ),
                                                                &obj->mmu_cache_enabled);
				obj->mmu_cache_entry_count = 1000;

                        sctk_runtime_config_mpit_bind_variable( "IB_MMU_CHACHE_COUNT",
                                                                sizeof(obj->mmu_cache_entry_count ),
                                                                &obj->mmu_cache_entry_count);
				obj->mmu_cache_maximum_size = sctk_runtime_config_map_entry_parse_size("4GB");

                        sctk_runtime_config_mpit_bind_variable( "IB_MMU_CHACHE_MAX_SIZE",
                                                                sizeof(obj->mmu_cache_maximum_size ),
                                                                &obj->mmu_cache_maximum_size);
				obj->mmu_cache_maximum_pin_size = sctk_runtime_config_map_entry_parse_size("1GB");

                        sctk_runtime_config_mpit_bind_variable( "IB_MMU_CHACHE_MAX_PIN_SIZE",
                                                                sizeof(obj->mmu_cache_maximum_pin_size ),
                                                                &obj->mmu_cache_maximum_pin_size);
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
void sctk_runtime_config_struct_init_offload_ops_t(void * struct_ptr)
{
	struct sctk_runtime_config_struct_offload_ops_t * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->ondemand = false;
	obj->collectives = false;
	obj->init_done = 1;
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
	obj->eager_limit = sctk_runtime_config_map_entry_parse_size("8 KB");
	obj->min_comms = 1;
	obj->block_cut = sctk_runtime_config_map_entry_parse_size("2 GB");
	sctk_runtime_config_struct_init_offload_ops_t(&obj->offloading);
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
void sctk_runtime_config_struct_init_net_driver_shm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_shm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->buffered_priority = 0;

                        sctk_runtime_config_mpit_bind_variable( "SHM_BUFF_PRIORITY",
                                                                sizeof(obj->buffered_priority ),
                                                                &obj->buffered_priority);
				obj->buffered_min_size = 0;

                        sctk_runtime_config_mpit_bind_variable( "SHM_BUFF_MIN_SIZE",
                                                                sizeof(obj->buffered_min_size ),
                                                                &obj->buffered_min_size);
				obj->buffered_max_size = 4096;

                        sctk_runtime_config_mpit_bind_variable( "SHM_BUFF_MAX_SIZE",
                                                                sizeof(obj->buffered_max_size ),
                                                                &obj->buffered_max_size);
				obj->buffered_zerocopy = false;

                        sctk_runtime_config_mpit_bind_variable( "SHM_BUFF_ZEROCOPY",
                                                                sizeof(obj->buffered_zerocopy ),
                                                                &obj->buffered_zerocopy);
				obj->cma_enable = true;

                        sctk_runtime_config_mpit_bind_variable( "SHM_CMA_ENABLE",
                                                                sizeof(obj->cma_enable ),
                                                                &obj->cma_enable);
				obj->cma_priority = 1;

                        sctk_runtime_config_mpit_bind_variable( "SHM_CMA_PRIORITY",
                                                                sizeof(obj->cma_priority ),
                                                                &obj->cma_priority);
				obj->cma_min_size = 4096;

                        sctk_runtime_config_mpit_bind_variable( "SHM_CMA_MIN_SIZE",
                                                                sizeof(obj->cma_min_size ),
                                                                &obj->cma_min_size);
				obj->cma_max_size = 0;

                        sctk_runtime_config_mpit_bind_variable( "SHM_CMA_MAX_SIZE",
                                                                sizeof(obj->cma_max_size ),
                                                                &obj->cma_max_size);
				obj->cma_zerocopy = false;

                        sctk_runtime_config_mpit_bind_variable( "SHM_CMA_ZEROCOPY",
                                                                sizeof(obj->cma_zerocopy ),
                                                                &obj->cma_zerocopy);
				obj->frag_priority = 2;

                        sctk_runtime_config_mpit_bind_variable( "SHM_FRAG_PRIORITY",
                                                                sizeof(obj->frag_priority ),
                                                                &obj->frag_priority);
				obj->frag_min_size = 4096;

                        sctk_runtime_config_mpit_bind_variable( "SHM_FRAG_MIN_SIZE",
                                                                sizeof(obj->frag_min_size ),
                                                                &obj->frag_min_size);
				obj->frag_max_size = 0;

                        sctk_runtime_config_mpit_bind_variable( "SHM_FRAG_MAX_SIZE",
                                                                sizeof(obj->frag_max_size ),
                                                                &obj->frag_max_size);
				obj->frag_zerocopy = false;

                        sctk_runtime_config_mpit_bind_variable( "SHM_FRAG_ZEROCOPY",
                                                                sizeof(obj->frag_zerocopy ),
                                                                &obj->frag_zerocopy);
				obj->shmem_size = 1024;

                        sctk_runtime_config_mpit_bind_variable( "SHM_SIZE",
                                                                sizeof(obj->shmem_size ),
                                                                &obj->shmem_size);
				obj->cells_num = 2048;

                        sctk_runtime_config_mpit_bind_variable( "SHM_CELLS",
                                                                sizeof(obj->cells_num ),
                                                                &obj->cells_num);
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
	obj->task = 1;
	obj->emulated_rma = 1;
	obj->common = 1;
	obj->gatefunc.name = "sctk_rail_gate_msgtype";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_msgtype");
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
void sctk_runtime_config_enum_init_rail_topological_polling_level()
{
	struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));
	struct enum_value * current_value, * values = NULL;

	strncpy(current_enum->name, "enum rail_topological_polling_level", 50);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_NONE", 50);
	current_value->value = RAIL_POLL_NONE;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_PU", 50);
	current_value->value = RAIL_POLL_PU;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_CORE", 50);
	current_value->value = RAIL_POLL_CORE;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_SOCKET", 50);
	current_value->value = RAIL_POLL_SOCKET;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_NUMA", 50);
	current_value->value = RAIL_POLL_NUMA;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_MACHINE", 50);
	current_value->value = RAIL_POLL_MACHINE;
	HASH_ADD_STR(values, name, current_value);

	current_enum->values = values;
	HASH_ADD_STR(enums_types, name, current_enum);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_topological_polling(void * struct_ptr)
{
	struct sctk_runtime_config_struct_topological_polling * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->range = RAIL_POLL_MACHINE;
	obj->trigger = RAIL_POLL_SOCKET;
	obj->init_done = 1;
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
	obj->priority = 1;
	obj->device = "default";
	sctk_runtime_config_struct_init_topological_polling(&obj->idle_polling);
	sctk_runtime_config_struct_init_topological_polling(&obj->any_source_polling);
	obj->topology = "ring";
	obj->ondemand = 1;
	obj->rdma = 0;
	obj->config = "topological";
	/* array */
	obj->gates = NULL;
	obj->gates_size = 0;
	/* array */
	obj->subrails = NULL;
	obj->subrails_size = 0;
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

                        sctk_runtime_config_mpit_bind_variable( "ITT_BARRIER_ARITY",
                                                                sizeof(obj->barrier_arity ),
                                                                &obj->barrier_arity);
				obj->broadcast_arity_max = 32;

                        sctk_runtime_config_mpit_bind_variable( "ITT_BROADCAST_ARITY",
                                                                sizeof(obj->broadcast_arity_max ),
                                                                &obj->broadcast_arity_max);
				obj->broadcast_max_size = 1024;

                        sctk_runtime_config_mpit_bind_variable( "ITT_BROADCAST_MAX_SIZE",
                                                                sizeof(obj->broadcast_max_size ),
                                                                &obj->broadcast_max_size);
				obj->broadcast_check_threshold = 512;

                        sctk_runtime_config_mpit_bind_variable( "ITT_BROADCAST_CHECK_TH",
                                                                sizeof(obj->broadcast_check_threshold ),
                                                                &obj->broadcast_check_threshold);
				obj->allreduce_arity_max = 8;

                        sctk_runtime_config_mpit_bind_variable( "ITT_ALLREDUCE_ARITY_MAX",
                                                                sizeof(obj->allreduce_arity_max ),
                                                                &obj->allreduce_arity_max);
				obj->allreduce_max_size = 4096;

                        sctk_runtime_config_mpit_bind_variable( "ITT_ALLREDUCE_MAX_SIZE",
                                                                sizeof(obj->allreduce_max_size ),
                                                                &obj->allreduce_max_size);
				obj->allreduce_check_threshold = 8192;

                        sctk_runtime_config_mpit_bind_variable( "ITT_ALLREDUCE_CHECK_TH",
                                                                sizeof(obj->allreduce_check_threshold ),
                                                                &obj->allreduce_check_threshold);
				obj->allreduce_max_slot = 65536;

                        sctk_runtime_config_mpit_bind_variable( "ITT_ALLREDUCE_MAX_SLOT",
                                                                sizeof(obj->allreduce_max_slot ),
                                                                &obj->allreduce_max_slot);
				obj->collectives_init_hook.name = "mpc_lowcomm_coll_init_noalloc";
	*(void **) &(obj->collectives_init_hook.value) = sctk_runtime_config_get_symbol("mpc_lowcomm_coll_init_noalloc");
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
void sctk_runtime_config_enum_init_mpcomp_task_larceny_mode_t()
{
	struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));
	struct enum_value * current_value, * values = NULL;

	strncpy(current_enum->name, "enum mpcomp_task_larceny_mode_t", 50);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_RANDOM", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_RANDOM;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_PRODUCER", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_PRODUCER;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_COUNT", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_COUNT;
	HASH_ADD_STR(values, name, current_value);

	current_enum->values = values;
	HASH_ADD_STR(enums_types, name, current_enum);
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
	obj->affinity = "balanced";
	obj->omp_new_task_depth = 10;
	obj->omp_untied_task_depth = 10;
	obj->omp_task_larceny_mode = MPCOMP_TASK_LARCENY_MODE_PRODUCER;
	obj->omp_task_nesting_max = 1000000;
	obj->mpcomp_task_max_delayed = 1024;
	obj->omp_task_steal_last_stolen_list = false;
	obj->omp_task_resteal_to_last_thief = false;
	obj->omp_task_use_lockfree_queue = true;
	obj->places = "cores";
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
	obj->placement_policy.name = "mpc_thread_get_task_placement_and_count_default";
	*(void **) &(obj->placement_policy.value) = sctk_runtime_config_get_symbol("mpc_thread_get_task_placement_and_count_default");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_scheduler(void * struct_ptr)
{
	struct sctk_runtime_config_struct_scheduler * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->timestamp_threshold = 0.0;
	obj->task_polling_thread_basic_priority = 20;
	obj->task_polling_thread_basic_priority_step = 20;
	obj->task_polling_thread_current_priority_step = 20;
	obj->sched_NBC_Pthread_basic_priority = 20;
	obj->sched_NBC_Pthread_basic_priority_step = 20;
	obj->sched_NBC_Pthread_current_priority_step = 20;
	obj->mpi_basic_priority = 20;
	obj->omp_basic_priority = 20;
	obj->posix_basic_priority = 20;
	obj->progress_basic_priority = 20;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl_cuda(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl_cuda * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl_openacc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl_openacc * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl_opencl(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl_opencl * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	sctk_runtime_config_struct_init_accl_cuda(&obj->cuda);
	sctk_runtime_config_struct_init_accl_openacc(&obj->openacc);
	sctk_runtime_config_struct_init_accl_opencl(&obj->opencl);
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset(struct sctk_runtime_config * config)
{
	memset(config, 0, sizeof(struct sctk_runtime_config));
	sctk_handler = dlopen(0, RTLD_LAZY | RTLD_GLOBAL);
	sctk_runtime_config_struct_init_debugger(&config->modules.debugger);
	sctk_runtime_config_struct_init_launcher(&config->modules.launcher);
	sctk_runtime_config_struct_init_collectives_shm_shared(&config->modules.collectives_shm_shared);
	sctk_runtime_config_struct_init_collectives_shm(&config->modules.collectives_shm);
	sctk_runtime_config_struct_init_collectives_intra(&config->modules.collectives_intra);
	sctk_runtime_config_struct_init_collectives_inter(&config->modules.collectives_inter);
	sctk_runtime_config_struct_init_nbc(&config->modules.nbc);
	sctk_runtime_config_struct_init_mpc(&config->modules.mpc);
	sctk_runtime_config_struct_init_mpi_rma(&config->modules.rma);
	sctk_runtime_config_struct_init_inter_thread_comm(&config->modules.inter_thread_comm);
	sctk_runtime_config_struct_init_low_level_comm(&config->modules.low_level_comm);
	sctk_runtime_config_struct_init_ft(&config->modules.ft_system);
	sctk_runtime_config_enum_init_ibv_rdvz_protocol();
	sctk_runtime_config_enum_init_rail_topological_polling_level();
	sctk_runtime_config_struct_init_openmp(&config->modules.openmp);
	sctk_runtime_config_enum_init_mpcomp_task_larceny_mode_t();
	sctk_runtime_config_struct_init_thread(&config->modules.thread);
	sctk_runtime_config_struct_init_scheduler(&config->modules.scheduler);
	sctk_runtime_config_struct_init_accl(&config->modules.accelerator);
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
void sctk_runtime_config_reset_struct_default_if_needed(const char * structname, void * ptr )
{
	if( !strcmp( structname , "sctk_runtime_config_struct_debugger") )
	{
		sctk_runtime_config_struct_init_debugger( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_launcher") )
	{
		sctk_runtime_config_struct_init_launcher( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_shm_shared") )
	{
		sctk_runtime_config_struct_init_collectives_shm_shared( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_shm") )
	{
		sctk_runtime_config_struct_init_collectives_shm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_intra") )
	{
		sctk_runtime_config_struct_init_collectives_intra( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_inter") )
	{
		sctk_runtime_config_struct_init_collectives_inter( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_nbc") )
	{
		sctk_runtime_config_struct_init_nbc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_mpi_rma") )
	{
		sctk_runtime_config_struct_init_mpi_rma( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_mpc") )
	{
		sctk_runtime_config_struct_init_mpc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_ft") )
	{
		sctk_runtime_config_struct_init_ft( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_topological") )
	{
		sctk_runtime_config_struct_init_net_driver_topological( ptr );
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

	if( !strcmp( structname , "sctk_runtime_config_struct_offload_ops_t") )
	{
		sctk_runtime_config_struct_init_offload_ops_t( ptr );
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

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_shm") )
	{
		sctk_runtime_config_struct_init_net_driver_shm( ptr );
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

	if( !strcmp( structname , "sctk_runtime_config_struct_topological_polling") )
	{
		sctk_runtime_config_struct_init_topological_polling( ptr );
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

	if( !strcmp( structname , "sctk_runtime_config_struct_openmp") )
	{
		sctk_runtime_config_struct_init_openmp( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_thread") )
	{
		sctk_runtime_config_struct_init_thread( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_scheduler") )
	{
		sctk_runtime_config_struct_init_scheduler( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl_cuda") )
	{
		sctk_runtime_config_struct_init_accl_cuda( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl_openacc") )
	{
		sctk_runtime_config_struct_init_accl_openacc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl_opencl") )
	{
		sctk_runtime_config_struct_init_accl_opencl( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl") )
	{
		sctk_runtime_config_struct_init_accl( ptr );
		return;
	}

}


/*******************  FUNCTION  *********************/
void * sctk_runtime_config_get_union_value_offset(const char * unionname, void * ptr )
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


#include "mpi_conf.h"

#include <dlfcn.h>

#include <mpc_common_debug.h>

/*********************
 * GLOBAL MPI CONFIG *
 *********************/

struct _mpc_mpi_config __mpc_mpi_config = { 0 };

struct _mpc_mpi_config *_mpc_mpi_config( void )
{
	return &__mpc_mpi_config;
}

/******************
 * NBC PARAMETERS *
 ******************/

static inline void __nbc_defaults( void )
{
	struct _mpc_mpi_config_nbc *nbc = &__mpc_mpi_config.nbc;

	nbc->progress_thread = 0;
	snprintf( nbc->thread_bind_function_name, MPC_CONF_STRING_SIZE, "sctk_get_progress_thread_binding_bind" );
	nbc->use_egreq_barrier = 0;
	nbc->use_egreq_bcast = 0;
	nbc->use_egreq_gather = 0;
	nbc->use_egreq_reduce = 0;
	nbc->use_egreq_scatter = 0;
}

static inline void __nbc_check( void )
{
	/* Ensure NBC binding function is resolved */
	struct _mpc_mpi_config_nbc *nbc = &__mpc_mpi_config.nbc;

	nbc->thread_bind_function = dlsym( NULL, nbc->thread_bind_function_name );

	if ( !nbc->thread_bind_function )
	{
		bad_parameter( "When loading mpcframework.mpi.nbc.progress.bindfunc = %s the function could not be resolved with dlsym", nbc->thread_bind_function_name );
	}
}

mpc_conf_config_type_t *__init_nbc_config( void )
{
	struct _mpc_mpi_config_nbc *nbc = &__mpc_mpi_config.nbc;

	mpc_conf_config_type_t *egreq = mpc_conf_config_type_init( "egreq",
															   PARAM( "barrier", &nbc->use_egreq_barrier, MPC_CONF_BOOL,
																	  "Enable EGREQ Barrier" ),
															   PARAM( "bcast", &nbc->use_egreq_bcast, MPC_CONF_BOOL,
																	  "Enable EGREQ Barrier" ),
															   PARAM( "scatter", &nbc->use_egreq_scatter, MPC_CONF_BOOL,
																	  "Enable EGREQ Barrier" ),
															   PARAM( "gather", &nbc->use_egreq_gather, MPC_CONF_BOOL,
																	  "Enable EGREQ Barrier" ),
															   PARAM( "reduce", &nbc->use_egreq_reduce, MPC_CONF_BOOL,
																	  "Enable EGREQ Barrier" ),
															   NULL );

	mpc_conf_config_type_t *progress = mpc_conf_config_type_init( "progress",
																  PARAM( "progress", &nbc->progress_thread, MPC_CONF_BOOL,
																		 "Enable progress thread for NBC" ),
																  PARAM( "bindfunc", nbc->thread_bind_function_name, MPC_CONF_STRING,
																		 "Function called to compute progress thread binding" ),
																  PARAM( "priority", &nbc->thread_basic_prio, MPC_CONF_INT,
																		 "Basic priority for progress threads" ),
																  NULL );

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init( "nbc",
															 PARAM( "egreq", egreq, MPC_CONF_TYPE,
																	"Extended Generic Request (EGREQ) based Collectives" ),
															 PARAM( "progress", progress, MPC_CONF_TYPE,
																	"NBC Progress thread configuration" ),
															 NULL );

	return ret;
}

/***************
 * COLLECTIVES *
 ***************/
/* Parameters */

static inline void __coll_param_defaults( void )
{
	struct _mpc_mpi_config_coll_opts *opts = &__mpc_mpi_config.coll_opts;

	/* INTRA */

	opts->force_nocommute = 0;

	opts->barrier_intra_for_trsh = 33;

	opts->reduce_intra_for_trsh = 33;
	opts->reduce_intra_count_trsh = 1024;

	opts->bcast_intra_for_trsh = 33;

	/* SHM */

	opts->reduce_pipelined_blocks = 16;
	opts->reduce_pipelined_tresh = 1024;
	opts->reduce_interleave = 16;

	opts->bcast_interleave = 16;

  /* TOPO */
  opts->topo_creation_allow_persistent = 0;
  opts->topo_creation_allow_non_blocking = 0;
  opts->topo_creation_allow_blocking = 0;
}

/* Coll array */

mpc_conf_config_type_t *_mpc_mpi_config_coll_array_conf( struct _mpc_mpi_config_coll_array *coll )
{
	mpc_conf_config_type_t *ret = mpc_conf_config_type_init( "pointers",
															   PARAM( "allgather", coll->allgather_name,
																	  MPC_CONF_STRING,
																	  "allgather function pointer" ),
															   PARAM( "allgatherv", coll->allgatherv_name,
																	  MPC_CONF_STRING,
																	  "allgatherv function pointer" ),
															   PARAM( "allreduce", coll->allreduce_name,
																	  MPC_CONF_STRING,
																	  "allreduce function pointer" ),
															   PARAM( "alltoall", coll->alltoall_name,
																	  MPC_CONF_STRING,
																	  "alltoall function pointer" ),
															   PARAM( "alltoallv", coll->alltoallv_name,
																	  MPC_CONF_STRING,
																	  "alltoallv function pointer" ),
															   PARAM( "alltoallw", coll->alltoallw_name,
																	  MPC_CONF_STRING,
																	  "alltoallw function pointer" ),
															   PARAM( "barrier", coll->barrier_name,
																	  MPC_CONF_STRING,
																	  "barrier function pointer" ),
															   PARAM( "bcast", coll->bcast_name,
																	  MPC_CONF_STRING,
																	  "bcast function pointer" ),
															   PARAM( "exscan", coll->exscan_name,
																	  MPC_CONF_STRING,
																	  "exscan function pointer" ),
															   PARAM( "gather", coll->gather_name,
																	  MPC_CONF_STRING,
																	  "gather function pointer" ),
															   PARAM( "gatherv", coll->gatherv_name,
																	  MPC_CONF_STRING,
																	  "gatherv function pointer" ),
															   PARAM( "scan", coll->scan_name,
																	  MPC_CONF_STRING,
																	  "scan function pointer" ),
															   PARAM( "scatter", coll->scatter_name,
																	  MPC_CONF_STRING,
																	  "scatter function pointer" ),
															   PARAM( "scatterv", coll->scatterv_name,
																	  MPC_CONF_STRING,
																	  "scatterv function pointer" ),
															   PARAM( "reduce", coll->reduce_name,
																	  MPC_CONF_STRING,
																	  "Reduce function pointer" ),
															   PARAM( "reducescatter", coll->reduce_scatter_name,
																	  MPC_CONF_STRING,
																	  "Reduce scatter function pointer" ),
															   PARAM( "reducescatterblock", coll->reduce_scatter_block_name,
																	  MPC_CONF_STRING,
																	  "Reduce scatter block function pointer" ),
															   NULL );

	return ret;
}

/* Coll algorithm array */

mpc_conf_config_type_t *_mpc_mpi_config_coll_algorithm_array_conf( struct _mpc_mpi_config_coll_algorithm_array *coll )
{
	mpc_conf_config_type_t *ret = mpc_conf_config_type_init( "algorithmpointers",
															   PARAM( "allgather", coll->allgather_name,
																	  MPC_CONF_STRING,
																	  "allgather function pointer" ),
															   PARAM( "allgatherv", coll->allgatherv_name,
																	  MPC_CONF_STRING,
																	  "allgatherv function pointer" ),
															   PARAM( "allreduce", coll->allreduce_name,
																	  MPC_CONF_STRING,
																	  "allreduce function pointer" ),
															   PARAM( "alltoall", coll->alltoall_name,
																	  MPC_CONF_STRING,
																	  "alltoall function pointer" ),
															   PARAM( "alltoallv", coll->alltoallv_name,
																	  MPC_CONF_STRING,
																	  "alltoallv function pointer" ),
															   PARAM( "alltoallw", coll->alltoallw_name,
																	  MPC_CONF_STRING,
																	  "alltoallw function pointer" ),
															   PARAM( "barrier", coll->barrier_name,
																	  MPC_CONF_STRING,
																	  "barrier function pointer" ),
															   PARAM( "bcast", coll->bcast_name,
																	  MPC_CONF_STRING,
																	  "bcast function pointer" ),
															   PARAM( "exscan", coll->exscan_name,
																	  MPC_CONF_STRING,
																	  "exscan function pointer" ),
															   PARAM( "gather", coll->gather_name,
																	  MPC_CONF_STRING,
																	  "gather function pointer" ),
															   PARAM( "gatherv", coll->gatherv_name,
																	  MPC_CONF_STRING,
																	  "gatherv function pointer" ),
															   PARAM( "scan", coll->scan_name,
																	  MPC_CONF_STRING,
																	  "scan function pointer" ),
															   PARAM( "scatter", coll->scatter_name,
																	  MPC_CONF_STRING,
																	  "scatter function pointer" ),
															   PARAM( "scatterv", coll->scatterv_name,
																	  MPC_CONF_STRING,
																	  "scatterv function pointer" ),
															   PARAM( "reduce", coll->reduce_name,
																	  MPC_CONF_STRING,
																	  "Reduce function pointer" ),
															   PARAM( "reducescatter", coll->reduce_scatter_name,
																	  MPC_CONF_STRING,
																	  "Reduce scatter function pointer" ),
															   PARAM( "reducescatterblock", coll->reduce_scatter_block_name,
																	  MPC_CONF_STRING,
																	  "Reduce scatter block function pointer" ),
															   NULL );

	return ret;
}


/* Intercomm */

static inline void __coll_intercomm_defaults( void )
{
	struct _mpc_mpi_config_coll_array *coll = &__mpc_mpi_config.coll_intercomm;

	snprintf( coll->barrier_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Barrier_inter" );
	snprintf( coll->bcast_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Bcast_inter" );
	snprintf( coll->allgather_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Allgather_inter" );
	snprintf( coll->allgatherv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Allgatherv_inter" );
	snprintf( coll->alltoall_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoall_inter" );
	snprintf( coll->alltoallv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoallv_inter" );
	snprintf( coll->alltoallw_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoallw_inter" );
	snprintf( coll->gather_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Gather_inter" );
	snprintf( coll->gatherv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Gatherv_inter" );
	snprintf( coll->scatter_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatter_inter" );
	snprintf( coll->scatterv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatterv_inter" );
	snprintf( coll->reduce_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_inter" );
	snprintf( coll->allreduce_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Allreduce_inter" );
	snprintf( coll->reduce_scatter_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_scatter_inter" );
	snprintf( coll->reduce_scatter_block_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_scatter_block_inter" );
	 coll->scan_name[0] = '\0';
	 coll->exscan_name[0] = '\0';
}

static inline void __coll_intracomm_defaults( void )
{
	struct _mpc_mpi_config_coll_array *coll = &__mpc_mpi_config.coll_intracomm;

	snprintf( coll->barrier_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Barrier_intra" );
	snprintf( coll->bcast_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Bcast_intra" );
	snprintf( coll->allgather_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Allgather_intra" );
	snprintf( coll->allgatherv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Allgatherv_intra" );
	snprintf( coll->alltoall_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoall_intra" );
	snprintf( coll->alltoallv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoallv_intra" );
	snprintf( coll->alltoallw_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoallw_intra" );
	snprintf( coll->gather_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Gather_intra" );
	snprintf( coll->gatherv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Gatherv_intra" );
	snprintf( coll->scatter_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatter_intra" );
	snprintf( coll->scatterv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatterv_intra" );
	snprintf( coll->reduce_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_intra" );
	snprintf( coll->allreduce_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Allreduce_intra" );
	snprintf( coll->reduce_scatter_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_scatter_intra" );
	snprintf( coll->reduce_scatter_block_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_scatter_block_intra" );
	snprintf( coll->scan_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scan_intra" );
	snprintf( coll->exscan_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Exscan_intra" );
}

static inline void __coll_algorithm_intracomm_defaults( void )
{
	struct _mpc_mpi_config_coll_algorithm_array *coll = &__mpc_mpi_config.coll_algorithm_intracomm;

	snprintf( coll->barrier_name              , MPC_CONF_STRING_SIZE, "___collectives_barrier_switch" );
	snprintf( coll->bcast_name                , MPC_CONF_STRING_SIZE, "___collectives_bcast_switch" );
	snprintf( coll->allgather_name            , MPC_CONF_STRING_SIZE, "___collectives_allgather_switch" );
	snprintf( coll->allgatherv_name           , MPC_CONF_STRING_SIZE, "___collectives_allgatherv_switch" );
	snprintf( coll->alltoall_name             , MPC_CONF_STRING_SIZE, "___collectives_alltoall_switch" );
	snprintf( coll->alltoallv_name            , MPC_CONF_STRING_SIZE, "___collectives_alltoallv_switch" );
	snprintf( coll->alltoallw_name            , MPC_CONF_STRING_SIZE, "___collectives_alltoallw_switch" );
	snprintf( coll->gather_name               , MPC_CONF_STRING_SIZE, "___collectives_gather_switch" );
	snprintf( coll->gatherv_name              , MPC_CONF_STRING_SIZE, "___collectives_gatherv_switch" );
	snprintf( coll->scatter_name              , MPC_CONF_STRING_SIZE, "___collectives_scatter_switch" );
	snprintf( coll->scatterv_name             , MPC_CONF_STRING_SIZE, "___collectives_scatterv_switch" );
	snprintf( coll->reduce_name               , MPC_CONF_STRING_SIZE, "___collectives_reduce_switch" );
	snprintf( coll->allreduce_name            , MPC_CONF_STRING_SIZE, "___collectives_allreduce_switch" );
	snprintf( coll->reduce_scatter_name       , MPC_CONF_STRING_SIZE, "___collectives_reduce_scatter_switch" );
	snprintf( coll->reduce_scatter_block_name , MPC_CONF_STRING_SIZE, "___collectives_reduce_scatter_block_switch" );
	snprintf( coll->scan_name                 , MPC_CONF_STRING_SIZE, "___collectives_scan_switch" );
	snprintf( coll->exscan_name               , MPC_CONF_STRING_SIZE, "___collectives_exscan_switch" );
}

static inline void __coll_intracomm_shm_defaults( void )
{
	struct _mpc_mpi_config_coll_array *coll = &__mpc_mpi_config.coll_intracomm_shm;

	coll->barrier_name[0] = '\0';
	snprintf( coll->bcast_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Bcast_intra_shm" );
	coll->allgather_name[0] = '\0';
	coll->allgatherv_name[0] = '\0';
	coll->alltoall_name[0] = '\0';
	snprintf( coll->alltoallv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoallv_intra_shm" );
	coll->alltoallw_name[0] = '\0';
	snprintf( coll->gather_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Gather_intra_shm" );
	snprintf( coll->gatherv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Gatherv_intra_shm" );
	snprintf( coll->scatter_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatter_intra_shm" );
	snprintf( coll->scatterv_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatterv_intra_shm" );
	snprintf( coll->reduce_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Reduce_shm" );
	coll->allreduce_name[0] = '\0';
	coll->reduce_scatter_name[0] = '\0';
	coll->reduce_scatter_block_name[0] = '\0';
	coll->scan_name[0] = '\0';
	coll->exscan_name[0] = '\0';
}

static inline void __coll_intracomm_shared_node_defaults( void )
{
	struct _mpc_mpi_config_coll_array *coll = &__mpc_mpi_config.coll_intracomm_shared_node;

	snprintf( coll->barrier_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Barrier_intra_shared_node" );
	snprintf( coll->bcast_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Bcast_intra_shared_node" );
	coll->allgather_name[0] = '\0';
	coll->allgatherv_name[0] = '\0';
	snprintf( coll->alltoall_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Alltoall_intra_shared_node" );
	coll->alltoallv_name[0] = '\0';
	coll->alltoallw_name[0] = '\0';
	coll->gather_name[0] = '\0';
	coll->gatherv_name[0] = '\0';
	snprintf( coll->scatter_name, MPC_CONF_STRING_SIZE, "__INTERNAL__PMPI_Scatter_intra_shared_node" );
	coll->scatterv_name[0] = '\0';
	coll->reduce_name[0] = '\0';
	coll->allreduce_name[0] = '\0';
	coll->reduce_scatter_name[0] = '\0';
	coll->reduce_scatter_block_name[0] = '\0';
	coll->scan_name[0] = '\0';
	coll->exscan_name[0] = '\0';
}

static inline void __load_coll_function( char *family, char *func_name, int ( **func )() )
{
	/* We allow functions not to be present
       in such case the pointer is set to null
       as the rest of the code just look at it */
	if ( !strlen( func_name ) )
	{
		*func = NULL;
	}
	else
	{
		*func = dlsym( NULL, func_name );

		if ( !( *func ) )
		{
			bad_parameter( "CONFIG: failed resolving collective '%s' for '%s'", func_name, family );
		}
		else
		{
			mpc_common_debug( "CONFIG %s == %p", func_name, *func );
		}
	}
}

void _mpc_mpi_config_coll_array_resolve( struct _mpc_mpi_config_coll_array *coll, char *family )
{
	__load_coll_function( family, coll->barrier_name, &coll->barrier );
	__load_coll_function( family, coll->bcast_name, &coll->bcast );
	__load_coll_function( family, coll->allgather_name, &coll->allgather );
	__load_coll_function( family, coll->allgatherv_name, &coll->allgatherv );
	__load_coll_function( family, coll->alltoall_name, &coll->alltoall );
	__load_coll_function( family, coll->alltoallv_name, &coll->alltoallv );
	__load_coll_function( family, coll->alltoallw_name, &coll->alltoallw );
	__load_coll_function( family, coll->gather_name, &coll->gather );
	__load_coll_function( family, coll->gatherv_name, &coll->gatherv );
	__load_coll_function( family, coll->scatter_name, &coll->scatter );
	__load_coll_function( family, coll->scatterv_name, &coll->scatterv );
	__load_coll_function( family, coll->reduce_name, &coll->reduce );
	__load_coll_function( family, coll->allreduce_name, &coll->allreduce );
	__load_coll_function( family, coll->reduce_scatter_name, &coll->reduce_scatter );
	__load_coll_function( family, coll->reduce_scatter_block_name, &coll->reduce_scatter_block );
	__load_coll_function( family, coll->exscan_name, &coll->exscan );
	__load_coll_function( family, coll->scan_name, &coll->scan );

}

void _mpc_mpi_config_coll_algorithm_array_resolve( struct _mpc_mpi_config_coll_algorithm_array *coll, char *family )
{
	__load_coll_function( family, coll->barrier_name, &coll->barrier );
	__load_coll_function( family, coll->bcast_name, &coll->bcast );
	__load_coll_function( family, coll->allgather_name, &coll->allgather );
	__load_coll_function( family, coll->allgatherv_name, &coll->allgatherv );
	__load_coll_function( family, coll->alltoall_name, &coll->alltoall );
	__load_coll_function( family, coll->alltoallv_name, &coll->alltoallv );
	__load_coll_function( family, coll->alltoallw_name, &coll->alltoallw );
	__load_coll_function( family, coll->gather_name, &coll->gather );
	__load_coll_function( family, coll->gatherv_name, &coll->gatherv );
	__load_coll_function( family, coll->scatter_name, &coll->scatter );
	__load_coll_function( family, coll->scatterv_name, &coll->scatterv );
	__load_coll_function( family, coll->reduce_name, &coll->reduce );
	__load_coll_function( family, coll->allreduce_name, &coll->allreduce );
	__load_coll_function( family, coll->reduce_scatter_name, &coll->reduce_scatter );
	__load_coll_function( family, coll->reduce_scatter_block_name, &coll->reduce_scatter_block );
	__load_coll_function( family, coll->exscan_name, &coll->exscan );
	__load_coll_function( family, coll->scan_name, &coll->scan );
}

static inline void __coll_check( void )
{
	_mpc_mpi_config_coll_array_resolve( &__mpc_mpi_config.coll_intercomm, "intercomm" );
	_mpc_mpi_config_coll_array_resolve( &__mpc_mpi_config.coll_intracomm, "intracomm" );
	_mpc_mpi_config_coll_algorithm_array_resolve( &__mpc_mpi_config.coll_algorithm_intracomm, "intracomm algorithm" );
	_mpc_mpi_config_coll_array_resolve( &__mpc_mpi_config.coll_intracomm_shm, "intracomm shm" );
	_mpc_mpi_config_coll_array_resolve( &__mpc_mpi_config.coll_intracomm_shared_node, "intracomm shared-node" );
}

static inline void __coll_defaults( void )
{
	__coll_param_defaults();
	__coll_intercomm_defaults();
	__coll_intracomm_defaults();
  __coll_algorithm_intracomm_defaults();
	__coll_intracomm_shm_defaults();
	__coll_intracomm_shared_node_defaults();
}


mpc_conf_config_type_t *__init_coll_config( void )
{
	/* Note the _mpc_mpi_config_coll_opts is split accross
	   collective types for readability */
	struct _mpc_mpi_config_coll_opts *opts = &__mpc_mpi_config.coll_opts;

	mpc_conf_config_type_t *shm = mpc_conf_config_type_init( "shm",
															PARAM( "reducepipelineblocks", &opts->reduce_pipelined_blocks, MPC_CONF_INT,
																	"Number of blocks for pipelined Reduce" ),
															PARAM( "reducepipelinetresh", &opts->reduce_pipelined_tresh, MPC_CONF_LONG_INT,
																	"Size required to rely on pipelined reduce" ),
															PARAM( "reduceinterleave", &opts->reduce_interleave, MPC_CONF_INT,
																	"Number of reduce slots to allocate (required to be power of 2)" ),
															PARAM( "bcastinterleave", &opts->bcast_interleave, MPC_CONF_INT,
																	"Number of bcast slots to allocate (required to be power of 2)" ),
															PARAM( "pointers", _mpc_mpi_config_coll_array_conf(&__mpc_mpi_config.coll_intracomm_shm), MPC_CONF_TYPE,
																	"Function pointers for shared memory collectives" ),
															NULL );

	mpc_conf_config_type_t *shared_node = mpc_conf_config_type_init( "sharednode",
															PARAM( "pointers", _mpc_mpi_config_coll_array_conf(&__mpc_mpi_config.coll_intracomm_shared_node), MPC_CONF_TYPE,
																	"Function pointers for shared-node collectives" ),
															NULL );

	mpc_conf_config_type_t *intercomm = mpc_conf_config_type_init( "intercomm",
															PARAM( "pointers", _mpc_mpi_config_coll_array_conf(&__mpc_mpi_config.coll_intercomm), MPC_CONF_TYPE,
																	"Function pointers for intercomm collectives" ),
															NULL );

	mpc_conf_config_type_t *intracomm = mpc_conf_config_type_init( "intracomm",
                              PARAM( "topopersistent", &opts->topo_creation_allow_persistent, MPC_CONF_BOOL,
																	"Allow the creation of topological communicators inside persistent collectives" ),
                              PARAM( "toponbc", &opts->topo_creation_allow_non_blocking, MPC_CONF_BOOL,
																	"Allow the creation of topological communicators inside non blocking collectives" ),
                              PARAM( "topoblocking", &opts->topo_creation_allow_blocking, MPC_CONF_BOOL,
																	"Allow the creation of topological communicators inside blocking collectives" ),
															PARAM( "nocommute", &opts->force_nocommute, MPC_CONF_BOOL,
																	"Force the use of deterministic algorithms" ),
															PARAM( "barrierfortresh", &opts->barrier_intra_for_trsh, MPC_CONF_INT,
																	"Maximum number of process for using a trivial for in the Barrier" ),
															PARAM( "reducefortresh", &opts->reduce_intra_for_trsh, MPC_CONF_INT,
																	"Maximum number of process for using a trivial for in the Reduce" ),
															PARAM( "reducecounttresh", &opts->reduce_intra_count_trsh, MPC_CONF_INT,
																	"Maximum number of elements for using a trivial for in the Reduce" ),
															PARAM( "bcastfortresh", &opts->bcast_intra_for_trsh, MPC_CONF_INT,
																	"Maximum number of process for using a trivial for in the Bcast" ),
															PARAM( "pointers", _mpc_mpi_config_coll_array_conf(&__mpc_mpi_config.coll_intracomm), MPC_CONF_TYPE,
																	"Function pointers for intracomm collectives" ),
                              PARAM("algorithmpointers", _mpc_mpi_config_coll_algorithm_array_conf(&__mpc_mpi_config.coll_algorithm_intracomm), MPC_CONF_TYPE,
                                  "Function pointers for intracomm collectives algorithms" ),
															NULL );

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init( "coll",
															PARAM( "shm", shm, MPC_CONF_TYPE,
																	"Shared-Memory (SHM) collectives configuration" ),
															PARAM( "sharednode", shared_node, MPC_CONF_TYPE,
																	"Shared-Node (process on the same node) collectives configuration" ),
															PARAM( "intercomm", intercomm, MPC_CONF_TYPE,
																	"Intercommunicator collectives configuration" ),
															PARAM( "intracomm", intracomm, MPC_CONF_TYPE,
																	"Intracommunicator collectives configuration" ),
															NULL );
	return ret;
}

/*********************
 * GLOBAL PARAMETERS *
 *********************/

void _mpc_mpi_config_check()
{
	__nbc_check();
	__coll_check();
}

static inline void __config_defaults( void )
{
	__nbc_defaults();
	__mpc_mpi_config.message_buffering = 1;
	__coll_defaults();
}

void _mpc_mpi_config_init( void )
{
	__config_defaults();

	mpc_conf_config_type_t *mpi = mpc_conf_config_type_init( "mpi",
															 PARAM( "buffering", &__mpc_mpi_config.message_buffering, MPC_CONF_BOOL,
																	"Enable small messages buffering" ),
															 PARAM( "nbc", __init_nbc_config(), MPC_CONF_TYPE,
																	"Configuration for Non-Blocking Collectives (NBC)" ),
															PARAM( "coll", __init_coll_config(), MPC_CONF_TYPE,
																	"Collective Communication Configuration" ),
															 NULL );

	mpc_conf_root_config_append( "mpcframework", mpi, "MPC MPI Configuration" );
}

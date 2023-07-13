#include "shm_coll.h"

#include <string.h>
#include <sctk_alloc.h>
#include <lowcomm_config.h>
#include <mpc_common_debug.h>

/************
 * PER NODE *
 ************/

int sctk_per_node_comm_context_init( struct sctk_per_node_comm_context *ctx,
									 __UNUSED__ mpc_lowcomm_communicator_t comm, int nb_task )
{
	return sctk_shared_mem_barrier_init( &ctx->shm_barrier, nb_task );
}

int sctk_per_node_comm_context_release( struct sctk_per_node_comm_context *ctx )
{
	return sctk_shared_mem_barrier_release( &ctx->shm_barrier );
}


/***************
 * PER PROCESS *
 ***************/

/************************************************************************/
/*Collective accessors                                                  */
/************************************************************************/


/************************************************************************/
/*INIT/DELETE                                                           */
/************************************************************************/

int sctk_shared_mem_barrier_sig_init( struct shared_mem_barrier_sig *shmb,
									  int nb_task )
{
	shmb->sig_points = sctk_malloc( sizeof( OPA_ptr_t ) * nb_task );
	assume( shmb->sig_points != NULL );
	shmb->tollgate = sctk_malloc( nb_task * sizeof( int ) );
	assume( shmb->tollgate != NULL );
	int i;

	for ( i = 0; i < nb_task; ++i )
	{
		shmb->tollgate[i] = 0;
	}

	OPA_store_int( &shmb->fare, 0 );
	OPA_store_int( &shmb->counter, nb_task );
	return 0;
}

int sctk_shared_mem_barrier_sig_release( struct shared_mem_barrier_sig *shmb )
{
	sctk_free( shmb->sig_points );
	shmb->sig_points = NULL;
	sctk_free( (void *) shmb->tollgate );
	shmb->tollgate = NULL;
	return 0;
}

int sctk_shared_mem_barrier_init( struct shared_mem_barrier *shmb, int nb_task )
{
	OPA_store_int( &shmb->counter, nb_task );
	OPA_store_int( &shmb->phase, 0 );
	return 0;
}

int sctk_shared_mem_barrier_release( __UNUSED__ struct shared_mem_barrier *shmb )
{
	return 0;
}

int sctk_shared_mem_reduce_init( struct shared_mem_reduce *shmr, int nb_task )
{
	shmr->buffer = sctk_malloc( sizeof( union shared_mem_buffer ) * nb_task );
	assume( shmr->buffer != NULL );
	OPA_store_int( &shmr->owner, -1 );
	OPA_store_int( &shmr->left_to_push, nb_task );
	shmr->target_buff = NULL;
	int pipelined_blocks = _mpc_lowcomm_coll_conf_get()->shm_reduce_pipelined_blocks;
	shmr->buff_lock = sctk_malloc( sizeof( mpc_common_spinlock_t ) * pipelined_blocks );
	assume( shmr->buff_lock != NULL );
	shmr->pipelined_blocks = pipelined_blocks;
	int i;

	for ( i = 0; i < pipelined_blocks; ++i )
	{
		mpc_common_spinlock_init( &shmr->buff_lock[i], 0 );
	}

	shmr->tollgate = sctk_malloc( nb_task * sizeof( int ) );
	assume( shmr->tollgate != NULL );

	for ( i = 0; i < nb_task; ++i )
	{
		shmr->tollgate[i] = 0;
	}

	OPA_store_int( &shmr->fare, 0 );
	return 0;
}

int sctk_shared_mem_reduce_release( struct shared_mem_reduce *shmr )
{
	sctk_free( shmr->buffer );
	shmr->buffer = NULL;
	sctk_free( (void *) shmr->buff_lock );
	shmr->buff_lock = NULL;
	sctk_free( (void *) shmr->tollgate );
	shmr->tollgate = NULL;
	return 0;
}

int sctk_shared_mem_bcast_init( struct shared_mem_bcast *shmb, int nb_task )
{
	OPA_store_int( &shmb->owner, -1 );
	OPA_store_int( &shmb->left_to_pop, nb_task );
	shmb->tollgate = sctk_malloc( nb_task * sizeof( int ) );
	assume( shmb->tollgate != NULL );
	int i;

	for ( i = 0; i < nb_task; ++i )
	{
		shmb->tollgate[i] = 0;
	}

	OPA_store_int( &shmb->fare, 0 );
	OPA_store_ptr( &shmb->to_free, 0 );
	shmb->scount = 0;
	shmb->stype_size = 0;
	shmb->root_in_buff = 0;
	return 0;
}

int sctk_shared_mem_bcast_release( struct shared_mem_bcast *shmb )
{
	sctk_free( (void *) shmb->tollgate );
	shmb->tollgate = NULL;
	return 0;
}

int sctk_shared_mem_gatherv_init( struct shared_mem_gatherv *shmgv,
								  int nb_task )
{
	OPA_store_int( &shmgv->owner, -1 );
	OPA_store_int( &shmgv->left_to_push, nb_task );
	/* Tollgate */
	shmgv->tollgate = sctk_malloc( nb_task * sizeof( int ) );
	assume( shmgv->tollgate != NULL );
	OPA_store_int( &shmgv->fare, 0 );
	/* Leaf CTX */
	shmgv->src_buffs = sctk_malloc( nb_task * sizeof( OPA_ptr_t ) );
	assume( shmgv->src_buffs != NULL );
	/* Root CTX */
	shmgv->target_buff = NULL;
	shmgv->counts = NULL;
	shmgv->disps = NULL;
	shmgv->rtype_size = 0;
	shmgv->rcount = 0;
	shmgv->let_me_unpack = 0;
	shmgv->send_type_size = sctk_malloc( nb_task * sizeof( size_t ) );
	assume( shmgv->send_type_size != NULL );
	shmgv->send_count = sctk_malloc( nb_task * sizeof( int ) );
	assume( shmgv->send_count != NULL );
	/* Fill it all */
	int i;

	for ( i = 0; i < nb_task; ++i )
	{
		shmgv->tollgate[i] = 0;
		shmgv->send_count[i] = 0;
		shmgv->send_type_size[i] = 0;
		OPA_store_ptr( &shmgv->src_buffs[i], 0 );
	}

	return 0;
}

int sctk_shared_mem_gatherv_release( struct shared_mem_gatherv *shmgv )
{
	sctk_free( (void *) shmgv->tollgate );
	shmgv->tollgate = NULL;
	sctk_free( shmgv->src_buffs );
	shmgv->src_buffs = NULL;
	sctk_free( shmgv->send_type_size );
	shmgv->send_type_size = NULL;
	sctk_free( shmgv->send_count );
	return 0;
}

int sctk_shared_mem_scatterv_init( struct shared_mem_scatterv *shmsv,
								   int nb_task )
{
	OPA_store_int( &shmsv->owner, -1 );
	OPA_store_int( &shmsv->left_to_pop, nb_task );
	/* Tollgate */
	shmsv->tollgate = sctk_malloc( nb_task * sizeof( int ) );
	assume( shmsv->tollgate != NULL );
	OPA_store_int( &shmsv->fare, 0 );
	shmsv->recv_types = sctk_malloc( nb_task * sizeof( mpc_lowcomm_datatype_t ) );

	/* Root CTX */
	shmsv->send_type = MPC_LOWCOMM_DATATYPE_NULL;

	shmsv->src_buffs = sctk_malloc( nb_task * sizeof( OPA_ptr_t ) );
	assume( shmsv->src_buffs != NULL );
	shmsv->was_packed = 0;
	shmsv->stype_size = 0;
	shmsv->counts = NULL;
	shmsv->disps = NULL;
	/* Fill it all */
	int i;

	for ( i = 0; i < nb_task; ++i )
	{
		shmsv->tollgate[i] = 0;
		OPA_store_ptr( &shmsv->src_buffs[i], 0 );
	}

	return 0;
}

int sctk_shared_mem_scatterv_release( struct shared_mem_scatterv *shmsv )
{
	sctk_free( (void *) shmsv->tollgate );
	shmsv->tollgate = NULL;
	sctk_free( shmsv->src_buffs );
	shmsv->src_buffs = NULL;
	sctk_free(shmsv->recv_types);
	shmsv->recv_types = NULL;
	return 0;
}

int sctk_shared_mem_a2a_init( struct shared_mem_a2a *shmaa, int nb_task )
{
	shmaa->infos =
		sctk_malloc( nb_task * sizeof( struct sctk_shared_mem_a2a_infos * ) );
	assume( shmaa->infos != NULL );
	/* Fill it all */
	int i;

	for ( i = 0; i < nb_task; ++i )
	{
		shmaa->infos[i] = NULL;
	}

	shmaa->has_in_place = 0;
	return 0;
}

int sctk_shared_mem_a2a_release( struct shared_mem_a2a *shmaa )
{
	sctk_free( shmaa->infos );
	shmaa->infos = NULL;
	return 0;
}

int powerof2( int x )
{
	while ( ( ( x % 2 ) == 0 ) && x > 1 )
	{
		x /= 2;
	}

	return ( x == 1 );
}

struct sctk_comm_coll * sctk_comm_coll_init(int nb_task )
{
    struct sctk_comm_coll *coll = sctk_malloc(sizeof(struct sctk_comm_coll));
    assume(coll != NULL);

	/* NB task for all */
	coll->comm_size = nb_task;
	/* Allocate coll id array */
	coll->coll_id = sctk_malloc( nb_task * sizeof( unsigned int ) );
	assume( coll->coll_id != NULL );
	memset( (void *) coll->coll_id, 0, sizeof( int ) * nb_task );
	int i;
	/* The barrier structure */
	sctk_shared_mem_barrier_init( &coll->shm_barrier, nb_task );
	/* The Signalled Barrier */
	sctk_shared_mem_barrier_sig_init( &coll->shm_barrier_sig, nb_task );
	/* The reduce structure */
	coll->reduce_interleave = _mpc_lowcomm_coll_conf_get()->shm_reduce_interleave;

	if ( !powerof2( coll->reduce_interleave ) )
	{
		mpc_common_debug_error( "INFO : Reduce interleave is required to be power of 2" );
		mpc_common_debug_error( "INFO : now default to 8" );
		coll->reduce_interleave = 8;
	}

	coll->shm_reduce =
		sctk_malloc( sizeof( struct shared_mem_reduce ) * coll->reduce_interleave );
	assume( coll->shm_reduce != NULL );

	for ( i = 0; i < coll->reduce_interleave; i++ )
	{
		sctk_shared_mem_reduce_init( &coll->shm_reduce[i], nb_task );
	}

	/* The reduce structure */
	coll->bcast_interleave = _mpc_lowcomm_coll_conf_get()->shm_bcast_interleave;

	if ( !powerof2( coll->bcast_interleave ) )
	{
		mpc_common_debug_error( "INFO : Bcast interleave is required to be power of 2" );
		mpc_common_debug_error( "INFO : now default to 8" );
		coll->bcast_interleave = 8;
	}

	coll->shm_bcast =
		sctk_malloc( sizeof( struct shared_mem_bcast ) * coll->bcast_interleave );
	assume( coll->shm_bcast != NULL );

	for ( i = 0; i < coll->bcast_interleave; i++ )
	{
		sctk_shared_mem_bcast_init( &coll->shm_bcast[i], nb_task );
	}

	/* The gatherv structure */
	sctk_shared_mem_gatherv_init( &coll->shm_gatherv, nb_task );
	/* The scatterv structure */
	sctk_shared_mem_scatterv_init( &coll->shm_scatterv, nb_task );
	/* The All2All structure */
	sctk_shared_mem_a2a_init( &coll->shm_a2a, nb_task );
	coll->node_ctx = NULL;
	/* Flag init done */
	coll->init_done = 1;

	return coll;
}

int sctk_comm_coll_release( struct sctk_comm_coll *coll )
{
	/* NB task for all */
	coll->comm_size = 0;
	/* Allocate coll id array */
	sctk_free( (void *) coll->coll_id );
	coll->coll_id = NULL;
	int i;
	/* The barrier structure */
	sctk_shared_mem_barrier_release( &coll->shm_barrier );
	/* The Signalled Barrier */
	sctk_shared_mem_barrier_sig_release( &coll->shm_barrier_sig );
	/* The reduce structure */

	for ( i = 0; i < coll->reduce_interleave; i++ )
	{
		sctk_shared_mem_reduce_release( &coll->shm_reduce[i] );
	}

	sctk_free( coll->shm_reduce );
	coll->shm_reduce = NULL;
	coll->reduce_interleave = 0;

	/* The reduce structure */
	for ( i = 0; i < coll->bcast_interleave; i++ )
	{
		sctk_shared_mem_bcast_release( &coll->shm_bcast[i] );
	}

	sctk_free( coll->shm_bcast );
	coll->shm_bcast = NULL;
	coll->bcast_interleave = 0;
	/* The gatherv structure */
	sctk_shared_mem_gatherv_release( &coll->shm_gatherv );
	/* The scatterv structure */
	sctk_shared_mem_scatterv_release( &coll->shm_scatterv );
	/* The All2All structure */
	sctk_shared_mem_a2a_release( &coll->shm_a2a );
	coll->init_done = 0;
	
    sctk_free(coll);
    
    return 0;
}

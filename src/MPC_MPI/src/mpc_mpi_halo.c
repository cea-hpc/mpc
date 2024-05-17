/* ############################# MPI License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPI Runtime.                                # */
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
/* #   - BESNARD Jean-Baptiste   jbbesnard@paratools.fr                   # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_mpi_halo.h"
#include "mpc_common_debug.h"
#include "mpc_lowcomm_msg.h"
#include <string.h>

#include <sctk_alloc.h>


void * sctk_malloc( size_t );
void * sctk_realloc( void * , size_t );
void sctk_free( void * );


/************************************************************************/
/* Halo context (storage and integer resolution)                        */
/************************************************************************/

static int halo_force_alloc = 0;

void sctk_mpi_halo_context_init( struct sctk_mpi_halo_context * ctx )
{
	ctx->halo_cells = NULL;
	ctx->halo_counter = 0;
	ctx->halo_size = 0;

	ctx->exchange_cells = NULL;
	ctx->exchange_counter = 0;
	ctx->exchange_size = 0;

	mpc_common_spinlock_init(&ctx->lock, 0);

	if( getenv("HALO_FORCE") )
	{
		halo_force_alloc = 1;
	}
}


void sctk_mpi_halo_context_relase( struct sctk_mpi_halo_context * ctx )
{
	memset(ctx, 0, sizeof(struct sctk_mpi_halo_context));
}

static inline int sctk_mpi_halo_context_resize_array( void ** ptr, int current_size )
{
	int new_size;

	if( current_size == 0 )
	{
		new_size = 10;
	}
	else
	{
		new_size = current_size * 2;
	}

	*ptr = sctk_realloc( *ptr, new_size * sizeof( void * ) );

	assume( *ptr != NULL );

	return new_size;
}

struct sctk_mpi_halo_s * sctk_mpi_halo_context_get( struct sctk_mpi_halo_context * ctx, int id )
{
	struct sctk_mpi_halo_s * ret = NULL;

	mpc_common_spinlock_lock( &ctx->lock );

	if( (id < ctx->halo_counter) && (0 <= id) )
		ret = ctx->halo_cells[id];

	mpc_common_spinlock_unlock( &ctx->lock );

	return ret;
}

int sctk_mpi_halo_context_set( struct sctk_mpi_halo_context * ctx,  int id, struct sctk_mpi_halo_s * halo )
{
	int ret = 0;
	mpc_common_spinlock_lock( &ctx->lock );

	if( (id < ctx->halo_counter) && (0 <= id) )
		ctx->halo_cells[id] = halo;
	else
		ret = 1;

	mpc_common_spinlock_unlock( &ctx->lock );

	return ret;
}

struct sctk_mpi_halo_s *  sctk_mpi_halo_context_new( struct sctk_mpi_halo_context * ctx,  int  * id )
{
	struct sctk_mpi_halo_s * ret = sctk_malloc( sizeof( struct sctk_mpi_halo_s ) );

	assume( ret != NULL );

	*id = -1;

	mpc_common_spinlock_lock( &ctx->lock );

	/* Search for free cells */
	int i;
	for( i = 0 ; i < ctx->halo_counter ; i++ )
	{
		if( ctx->halo_cells[i] == NULL )
		{
			 ctx->halo_cells[i] = ret;
			 *id = i;
			 break;
		}
	}

	/* Grow the array */
	if( *id == -1 )
	{
		ctx->halo_counter++;

		if( ctx->halo_size <= ctx->halo_counter )
		{
			ctx->halo_size = sctk_mpi_halo_context_resize_array( (void *)&(ctx->halo_cells), ctx->halo_size );
		}

		ctx->halo_cells[ctx->halo_counter - 1] = ret;
		*id = ctx->halo_counter - 1;

	}


	mpc_common_spinlock_unlock( &ctx->lock );

	return ret;
}

struct sctk_mpi_halo_exchange_s * sctk_mpi_halo_context_exchange_get( struct sctk_mpi_halo_context * ctx,  int id )
{
	struct sctk_mpi_halo_exchange_s * ret = NULL;

	mpc_common_spinlock_lock( &ctx->lock );

	if( (id < ctx->exchange_counter) && (0 <= id) )
		ret = ctx->exchange_cells[id];

	mpc_common_spinlock_unlock( &ctx->lock );

	return ret;
}

int sctk_mpi_halo_context_exchange_set( struct sctk_mpi_halo_context * ctx,  int id, struct sctk_mpi_halo_exchange_s * halo )
{
	int ret = 0;
	mpc_common_spinlock_lock( &ctx->lock );

	if( (id < ctx->exchange_counter) && (0 <= id) )
		ctx->exchange_cells[id] = halo;
	else
		ret = 1;

	mpc_common_spinlock_unlock( &ctx->lock );

	return ret;
}

struct sctk_mpi_halo_exchange_s *  sctk_mpi_halo_context_exchange_new( struct sctk_mpi_halo_context * ctx,  int  * id )
{
	struct sctk_mpi_halo_exchange_s * ret = sctk_malloc( sizeof(struct sctk_mpi_halo_exchange_s ) );

	assume( ret != NULL );

	*id = MPI_HALO_NULL;

	mpc_common_spinlock_lock( &ctx->lock );

	/* Search for free cells */
	int i;
	for( i = 0 ; i < ctx->exchange_counter ; i++ )
	{
		if( ctx->exchange_cells[i] == NULL )
		{
			 ctx->exchange_cells[i] = ret;
			 *id = i;
			 break;
		}
	}

	/* Grow the array */
	if( *id == -1 )
	{
		ctx->exchange_counter++;

		if( ctx->exchange_size <= ctx->exchange_counter )
		{
			ctx->exchange_size = sctk_mpi_halo_context_resize_array( (void *)&(ctx->exchange_cells), ctx->exchange_size );
		}

		ctx->exchange_cells[ctx->exchange_counter - 1] = ret;
		*id = ctx->exchange_counter - 1;

	}

	mpc_common_spinlock_unlock( &ctx->lock );

	return ret;
}


/************************************************************************/
/* Halo Cells                                                           */
/************************************************************************/

static volatile int __halo_local_tag = 25000;
mpc_common_spinlock_t  __halo_local_tag_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

int sctk_mpi_halo_init( struct sctk_mpi_halo_s * halo , char * label , MPI_Datatype type, int count )
{
	if( ! halo )
		return 1;

	halo->type = MPI_HALO_TYPE_NONE;
	halo->label = strdup( label );
	halo->is_committed = 0;

	halo->halo_buffer_type = MPI_HALO_BUFFER_NONE;
	halo->halo_buffer = NULL;
	halo->halo_remote = -1;
	halo->remote_label = NULL;

	halo->halo_count = count;

	halo->halo_type = type;

	mpc_common_spinlock_lock( &__halo_local_tag_lock );
	halo->halo_tag = __halo_local_tag++;
	mpc_common_spinlock_unlock( &__halo_local_tag_lock );

	MPI_Datatype cont_type;
	if( PMPI_Type_contiguous( count, type, &cont_type) != MPI_SUCCESS )
		return 1;
	MPI_Count element_size;
	if( PMPI_Type_size_x(cont_type , &element_size) != MPI_SUCCESS )
		return 1;

	halo->halo_extent = (size_t)element_size;

	PMPI_Type_free( &cont_type );

	return 0;
}


int sctk_mpi_halo_release( struct sctk_mpi_halo_s * halo )
{
	if( ! halo )
		return 1;

	if( halo->halo_buffer_type == MPI_HALO_BUFFER_ALLOCATED )
	{
		sctk_free( halo->halo_buffer );
	}

	halo->halo_buffer_type = MPI_HALO_BUFFER_NONE;
	halo->halo_buffer = NULL;

	free( halo->label );
	halo->label = NULL;
	halo->is_committed = 0;

	halo->halo_remote = -1;
	free( halo->remote_label );
	halo->remote_label = NULL;
	halo->halo_type = MPI_DATATYPE_NULL;
	halo->halo_count = 0;
	halo->halo_extent = 0;
	halo->type = MPI_HALO_TYPE_NONE;

	return 0;
}

int sctk_mpi_halo_set(  struct sctk_mpi_halo_s * halo, void * ptr )
{
	if( ! halo )
		return 1;

	if( halo->type != MPI_HALO_CELL_LOCAL )
	{
		mpc_common_debug_fatal("Could not set a buffer to a non-local halo\n");
	}

	if( !halo->is_committed )
	{
		mpc_common_debug_fatal("Could not set a buffer to a non-commited halo\n");
	}

	halo->halo_buffer = ptr;

	return 1;
}

int sctk_mpi_halo_get(  struct sctk_mpi_halo_s * halo, void ** ptr )
{
	if( ! halo || !ptr )
		return 1;

	if( halo->type != MPI_HALO_CELL_REMOTE )
	{
		mpc_common_debug_fatal("Could not get a buffer from a non-remote halo\n");
	}

	if( !halo->is_committed )
	{
		mpc_common_debug_fatal("Could not get a buffer from a non-commited halo\n");
	}

	*ptr = halo->halo_buffer;

	return 1;
}

void sctk_mpi_halo_print(  struct sctk_mpi_halo_s * halo )
{
	mpc_common_debug_warning("=============== Halo ================");
	mpc_common_debug_warning("type : %d", halo->type);
	mpc_common_debug_warning("label : %s", halo->label);
	mpc_common_debug_warning("committed : %d", halo->is_committed);
	mpc_common_debug_warning("halo_buffer : %p", halo->halo_buffer);
	mpc_common_debug_warning("halo_buffer_type : %d", halo->halo_buffer_type);
	mpc_common_debug_warning("halo_remote : %d", halo->halo_remote);
	mpc_common_debug_warning("halo_tag : %d", halo->halo_tag);
	mpc_common_debug_warning("remote_label : %s", halo->remote_label);
	mpc_common_debug_warning("halo_type : %d", halo->halo_type);
	mpc_common_debug_warning("halo_count : %d", halo->halo_count);
	mpc_common_debug_warning("halo_extent : %d", halo->halo_extent);
	mpc_common_debug_warning("=====================================");
}


/************************************************************************/
/* Halo Exchange                                                        */
/************************************************************************/

/*
 * Halo Exchange Actions
 */

struct sctk_mpi_halo_exchange_action_s * sctk_mpi_halo_exchange_new_action( struct sctk_mpi_halo_s * halo, sctk_mpi_halo_exchange_action_t action, int process, int tag, struct sctk_mpi_halo_exchange_action_s * prev )
{
	struct sctk_mpi_halo_exchange_action_s * ret = sctk_malloc( sizeof( struct sctk_mpi_halo_exchange_action_s ) );
	assume( ret != NULL );

	ret->process = process;
	ret->tag = tag;
	ret->request = MPI_REQUEST_NULL;
	ret->action = action;
	ret->halo = halo;
	ret->prev = prev;

	return ret;
}

int sctk_mpi_halo_exchange_action_free( struct sctk_mpi_halo_exchange_action_s * action )
{
	if( !action )
		return 0;

	action->request = MPI_REQUEST_NULL;
	action->action = MPI_HALO_EXCHANGE_NONE;
	action->halo = NULL;
	action->process = -1;
	action->tag = -1;

	sctk_free( action );

	return 0;
}



int sctk_mpi_halo_exchange_action_trigger( struct sctk_mpi_halo_exchange_action_s * action )
{
	struct sctk_mpi_halo_s * halo = action->halo;

	if( !halo->halo_buffer
	&& ( ( action->action == MPI_HALO_EXCHANGE_SEND ) || ( action->action == MPI_HALO_EXCHANGE_SEND_LOCAL ) ) )
	{
		sctk_mpi_halo_print(  halo );
		mpc_common_debug_fatal("Make sure all your local halos are bound and that data is set before doing an exchange");
	}

	int rank;
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );

	switch( action->action )
	{
		case MPI_HALO_EXCHANGE_NONE:
			break;
		case MPI_HALO_EXCHANGE_SEND:
			/* Send full buffer */
			mpc_common_debug( "Send %p (count %d type %d) from %d to %d tag %d", halo->halo_buffer, halo->halo_count, halo->halo_type, rank, action->process, action->tag );
			PMPI_Isend( halo->halo_buffer, halo->halo_count, halo->halo_type, action->process, action->tag, MPI_COMM_WORLD, &action->request );
			break;
		case MPI_HALO_EXCHANGE_SEND_LOCAL:
			/* Send pointer to buffer */
			mpc_common_debug( "Send Local %p (count %d type %d) from %d to %d tag %d", halo->halo_buffer, halo->halo_count, halo->halo_type, rank, action->process, action->tag );
			PMPI_Isend( & halo->halo_buffer, sizeof( void *), MPI_CHAR, action->process, action->tag, MPI_COMM_WORLD, &action->request );
			break;
		break;

		case MPI_HALO_EXCHANGE_RECV:
			PMPI_Irecv( halo->halo_buffer, halo->halo_count, halo->halo_type, action->process, action->tag, MPI_COMM_WORLD, &action->request );
			mpc_common_debug( "Recv %p (count %d type %d) from %d to %d tag %d", halo->halo_buffer, halo->halo_count, halo->halo_type, rank, action->process, action->tag );
			break;

		case MPI_HALO_EXCHANGE_RECV_LOCAL:
			PMPI_Irecv( & halo->halo_buffer, sizeof( void *), MPI_CHAR, action->process, action->tag, MPI_COMM_WORLD, &action->request );
			mpc_common_debug( "Recv Local %p (count %d type %d) from %d to %d tag %d", halo->halo_buffer, halo->halo_count, halo->halo_type, rank, action->process, action->tag );
			break;
		default:
			not_reachable();
	}

	return 0;
}


/*
 * Halo Exchange Init & Release
 */

int sctk_mpi_halo_exchange_init( struct sctk_mpi_halo_exchange_s *ex )
{
	ex->halo_count = 0;
	ex->halo_size = 0;
	ex->halo_actions = NULL;
	ex->halo = NULL;
	return 0;
}

int sctk_mpi_halo_exchange_release( struct sctk_mpi_halo_exchange_s *ex )
{
	sctk_free( ex->halo );
	ex->halo = NULL;
	ex->halo_size = 0;
	ex->halo_count = 0;

	struct sctk_mpi_halo_exchange_action_s * current = ex->halo_actions;
	struct sctk_mpi_halo_exchange_action_s * to_free;

	while( current )
	{
		to_free = current;
		current = current->prev;
		sctk_mpi_halo_exchange_action_free( to_free );
	}

	return 0;
}



/*
 * Halo Storage
 */

int sctk_mpi_halo_exchange_push_halo( struct sctk_mpi_halo_exchange_s *ex,  struct sctk_mpi_halo_s * halo )
{
	ex->halo_count++;

	if( ex->halo_size <= ex->halo_count )
	{
		ex->halo_size = sctk_mpi_halo_context_resize_array( (void *)&(ex->halo), ex->halo_size );
	}

	ex->halo[ex->halo_count - 1] = halo;

	return 0;
}

struct sctk_mpi_halo_s * sctk_mpi_halo_exchange_get_halo( struct sctk_mpi_halo_exchange_s *ex, char *label )
{
	int i;

	for( i = 0 ; i < ex->halo_count ; i++ )
	{
		if( !strcmp( label, ex->halo[i]->label ) )
		{
			return ex->halo[i];
		}
	}

	return NULL;
}

/*
 * Bind
 */

int sctk_mpi_halo_bind_remote( struct sctk_mpi_halo_exchange_s *ex, struct sctk_mpi_halo_s * halo, int remote, char * remote_label )
{
	/* First update the halo */
	halo->type = MPI_HALO_CELL_REMOTE;
	halo->remote_label = strdup( remote_label );
	halo->halo_remote = remote;

	/* Check if remote is local or not */
	if( mpc_lowcomm_is_remote_rank( remote ) || halo_force_alloc )
	{
		/* Remote process allocate a buffer */
		halo->halo_buffer_type = MPI_HALO_BUFFER_ALLOCATED;
		halo->halo_buffer = sctk_malloc( halo->halo_extent );
		assume( halo->halo_buffer != NULL );
	}
	else
	{
		/* Local process no need for a buffer */
		halo->halo_buffer_type = MPI_HALO_BUFFER_ALIAS;
	}

	sctk_mpi_halo_exchange_push_halo( ex,  halo );

	return 0;
}

int sctk_mpi_halo_bind_local( struct sctk_mpi_halo_exchange_s *ex, struct sctk_mpi_halo_s * halo )
{
	halo->type = MPI_HALO_CELL_LOCAL;
	sctk_mpi_halo_exchange_push_halo( ex,  halo );

	return 0;
}


/*
 * Exchange Request
 */


int sctk_mpi_halo_exchange_request_init( struct sctk_mpi_halo_exchange_request * req , struct sctk_mpi_halo_s * halo )
{
	PMPI_Comm_rank( MPI_COMM_WORLD, &req->source_process );
	req->dest_process = halo->halo_remote;
	req->tag = halo->halo_tag;
	snprintf( req->source_label, 512, "%s", halo->label );
	snprintf( req->remote_label, 512, "%s", halo->remote_label );

	req->extent = halo->halo_extent;

	return 0;
}

int sctk_mpi_halo_exchange_request_relase( struct sctk_mpi_halo_exchange_request * req  )
{
	req->source_process = -1;
	req->dest_process = -1;
	req->source_label[0] = '\0';
	req->remote_label[0] = '\0';
	req->extent = 0;

	return 0;
}

void sctk_mpi_halo_exchange_request_print( struct sctk_mpi_halo_exchange_request * req )
{
	mpc_common_debug_warning("======== HALO REQUEST ========");
	mpc_common_debug_warning("Source : %d", req->source_process);
	mpc_common_debug_warning("Dest : %d", req->dest_process);
	mpc_common_debug_warning("Source Label : %s", req->source_label);
	mpc_common_debug_warning("Remote Label : %s", req->remote_label);
	mpc_common_debug_warning("Extent : %ld", req->extent);
	mpc_common_debug_warning("==============================");
}

/*
 * Commit Helpers
 */


static inline int __sctk_mpi_halo_exchange_compute_valency( struct sctk_mpi_halo_exchange_s * ex )
{
	int rank, size;

	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
	PMPI_Comm_size( MPI_COMM_WORLD, &size );

	int * valency_array = sctk_calloc( size, sizeof( int ) );
	int * valency_array_recv = sctk_calloc( size, sizeof( int ) );

	assume( valency_array != NULL );
	assume( valency_array_recv != NULL );

	int i;

	for( i = 0 ; i < ex->halo_count ; i++ )
	{
		if( ex->halo[i]->type == MPI_HALO_CELL_REMOTE )
		{
			valency_array[ ex->halo[i]->halo_remote ]++;
		}
	}

	/* Allreduce It */
	PMPI_Allreduce( valency_array, valency_array_recv, size, MPI_INT, MPI_SUM, MPI_COMM_WORLD );

	/* Free send array */
	sctk_free( valency_array );
	/* Keep only received data */
	valency_array = valency_array_recv;

	/* How many incoming processes we expect ? */
	int expected_messages = valency_array[ rank ];

	/* We are now done with the valency array */
	sctk_free( valency_array );

	return expected_messages;
}

static inline struct sctk_mpi_halo_exchange_request * __sctk_mpi_halo_exchange_fill_outgoing_requests( struct sctk_mpi_halo_exchange_s * ex, int *outgoing_message_count )
{
	int local_req_remote_count = 0;

	int i;

	for( i = 0 ; i < ex->halo_count ; i++ )
	{
		if( ex->halo[i]->type == MPI_HALO_CELL_REMOTE )
		{
			local_req_remote_count++;
		}
	}

	struct sctk_mpi_halo_exchange_request * ret = sctk_malloc( local_req_remote_count * sizeof( struct sctk_mpi_halo_exchange_request ) );
	assume( ret );

	int counter = 0;
	for( i = 0 ; i < ex->halo_count ; i++ )
	{
		if( ex->halo[i]->type == MPI_HALO_CELL_REMOTE )
		{
			sctk_mpi_halo_exchange_request_init( &ret[counter] , ex->halo[i] );
			counter++;
		}
	}

	*outgoing_message_count = local_req_remote_count;

	return ret;
}


/*
 * Commit
 */


int sctk_mpi_halo_exchange_commit( struct sctk_mpi_halo_exchange_s * ex )
{
	/*  Compute the number of expected request for each task */
	int incoming_request_count = __sctk_mpi_halo_exchange_compute_valency( ex );
	/* Allocate storage for incoming requests */
	struct sctk_mpi_halo_exchange_request * incoming_requests = sctk_malloc( incoming_request_count * sizeof( struct sctk_mpi_halo_exchange_request ) );
	assume( incoming_requests != NULL );

	/* Fill in outgoing request information from local remote halo */
	int outgoing_request_count = 0;
	struct sctk_mpi_halo_exchange_request * outgoing_requests = __sctk_mpi_halo_exchange_fill_outgoing_requests( ex, &outgoing_request_count );

	MPI_Request * msg_req = sctk_malloc( ( outgoing_request_count + incoming_request_count ) * sizeof( MPI_Request) );
	assume( msg_req );

	/* Post request Exchange messages */
	int i;

	/* Incoming */
	for( i = 0 ; i < incoming_request_count ; i++ )
	{
		PMPI_Irecv( &incoming_requests[i], sizeof( struct sctk_mpi_halo_exchange_request ) , MPI_CHAR, MPI_ANY_SOURCE, 1987, MPI_COMM_WORLD, &msg_req[i] );
	}

	/* Outgoing */
	for( i = 0 ; i < outgoing_request_count ; i++ )
	{
		PMPI_Isend( &outgoing_requests[i], sizeof( struct sctk_mpi_halo_exchange_request ) , MPI_CHAR, outgoing_requests[i].dest_process, 1987, MPI_COMM_WORLD, &msg_req[incoming_request_count + i] );
	}

	/* Waitall */
	PMPI_Waitall( outgoing_request_count + incoming_request_count, msg_req, MPI_STATUSES_IGNORE );

	/* At this point end-point exchange has been done
	 * we now start validating and storing them */
	for( i = 0 ; i < incoming_request_count ; i++ )
	{
		struct sctk_mpi_halo_exchange_request * current_req = &incoming_requests[i];
		struct sctk_mpi_halo_s * target_halo =  sctk_mpi_halo_exchange_get_halo( ex, current_req->remote_label );

		if( ! target_halo )
		{
			sctk_mpi_halo_exchange_request_print( current_req );
			mpc_common_debug_error("Could not find a halo named %s in order to satify this request", current_req->remote_label );
			abort();
		}

		/* Check that it is a local one */
		if( target_halo->type != MPI_HALO_CELL_LOCAL )
		{
			sctk_mpi_halo_exchange_request_print( current_req );
			mpc_common_debug_error("Only locally bound halo cells can be requested by a remote buffer");
			abort();
		}


		/* Check extent */
		if( target_halo->halo_extent != current_req->extent )
		{
			sctk_mpi_halo_exchange_request_print( current_req );
			mpc_common_debug_error("Size mismatch between these buffers");
			abort();
		}

		/* Register a send to this buffer */

		/* Is request source in the same process ?
		 * Note that as this relationship is symetrical
		 * the buffer is allocated a the other end if actually remote
		 * */
		int is_remote = mpc_lowcomm_is_remote_rank( current_req->source_process );

		if( is_remote || halo_force_alloc )
		{
			/* Associate this buffer to an actual message */
			ex->halo_actions = sctk_mpi_halo_exchange_new_action( target_halo, MPI_HALO_EXCHANGE_SEND, current_req->source_process, current_req->tag, ex->halo_actions );
		}
		else
		{
			/* Associate this buffer to a pointer based message (shared-memory) */
			ex->halo_actions = sctk_mpi_halo_exchange_new_action( target_halo, MPI_HALO_EXCHANGE_SEND_LOCAL, current_req->source_process,  current_req->tag, ex->halo_actions );
		}

		/* Set halo as committed */
		target_halo->is_committed = 1;
	}

	/* We now push incoming recv action */
	for( i = 0 ; i < ex->halo_count ; i++ )
	{
		struct sctk_mpi_halo_s * current_halo = ex->halo[i];

		if( current_halo->type == MPI_HALO_CELL_REMOTE )
		{
			int is_remote = mpc_lowcomm_is_remote_rank( current_halo->halo_remote );

			if( is_remote || halo_force_alloc )
			{
				/* Associate this buffer to an actual message */
				ex->halo_actions = sctk_mpi_halo_exchange_new_action( current_halo, MPI_HALO_EXCHANGE_RECV, current_halo->halo_remote, current_halo->halo_tag, ex->halo_actions );
			}
			else
			{
				/* Associate this buffer to a pointer based message (shared-memory) */
				ex->halo_actions = sctk_mpi_halo_exchange_new_action( current_halo, MPI_HALO_EXCHANGE_RECV_LOCAL, current_halo->halo_remote,  current_halo->halo_tag, ex->halo_actions );
			}
		}

		/* Set halo as committed */
		current_halo->is_committed = 1;
	}

	sctk_free( incoming_requests );
	sctk_free( outgoing_requests );
	sctk_free( msg_req );

	return 0;
}






int sctk_mpi_halo_iexchange( struct sctk_mpi_halo_exchange_s *ex )
{
	/* Trigger All requests */
	struct sctk_mpi_halo_exchange_action_s * current_action = ex->halo_actions;

	while( current_action )
	{
		sctk_mpi_halo_exchange_action_trigger( current_action );
		current_action = current_action->prev;
	}

	return 0;
}

#define MPC_HALO_REQ_STATIC_ALLOC 30

int sctk_mpi_halo_exchange_wait( struct sctk_mpi_halo_exchange_s *ex )
{
	/* Count requests */
	struct sctk_mpi_halo_exchange_action_s * current_action = ex->halo_actions;

	int action_count = 0;

	while( current_action )
	{
		action_count++;
		current_action = current_action->prev;
	}

	/* Allocate requests */
	MPI_Request static_req[MPC_HALO_REQ_STATIC_ALLOC];
	MPI_Request * req = NULL;

	if( action_count < MPC_HALO_REQ_STATIC_ALLOC )
	{
		req = static_req;
	}
	else
	{
		req = sctk_malloc( action_count * sizeof( MPI_Request ) );
	}

	/* Fill requests */
	current_action = ex->halo_actions;

	int counter = 0;

	while( current_action )
	{
		req[ counter ] = current_action->request;
		counter++;
		current_action = current_action->prev;
	}

	/* Call wait */
	PMPI_Waitall( action_count, req, MPI_STATUSES_IGNORE );

	/* Free if needed */
	if( MPC_HALO_REQ_STATIC_ALLOC <= action_count )
	{
		sctk_free( req );
	}

	return 0;
}

int sctk_mpi_halo_exchange_blocking( struct sctk_mpi_halo_exchange_s *ex )
{
	sctk_mpi_halo_iexchange( ex );
	sctk_mpi_halo_exchange_wait( ex );
	return 0;
}

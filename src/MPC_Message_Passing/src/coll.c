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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "coll.h"

#include <math.h>
#include <mpc_launch_pmi.h>
#include <sctk_communicator.h>

#include <sctk_alloc.h>

/************************************************************************/
/*collective communication implementation                               */
/************************************************************************/

/* Overall Collective Configuration */

static int barrier_arity;

static int broadcast_arity_max;
static int broadcast_max_size;
static int broadcast_check_threshold;

static int allreduce_arity_max;
static int allreduce_max_size;
static int allreduce_check_threshold;
static unsigned int allreduce_max_slot;

/************************************
 * SIMPLE COLLECTIVE IMPLEMENTATION *
 ************************************/

/* Barrier */

static void _mpc_coll_barrier_simple( const mpc_lowcomm_communicator_t communicator,
                                      struct mpc_lowcomm_coll_s *tmp )
{
	int local;
	local = sctk_get_nb_task_local( communicator );
	mpc_thread_mutex_lock( &tmp->barrier.barrier_simple.lock );
	tmp->barrier.barrier_simple.done++;

	if ( tmp->barrier.barrier_simple.done == local )
	{
		tmp->barrier.barrier_simple.done = 0;
		mpc_thread_cond_broadcast( &tmp->barrier.barrier_simple.cond );
	}
	else
	{
		mpc_thread_cond_wait( &tmp->barrier.barrier_simple.cond,
		                       &tmp->barrier.barrier_simple.lock );
	}

	mpc_thread_mutex_unlock( &tmp->barrier.barrier_simple.lock );
}

void _mpc_coll_barrier_simple_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	if ( mpc_common_get_process_count() == 1 )
	{
		tmp->barrier.barrier_simple.done = 0;
		mpc_thread_mutex_init( &( tmp->barrier.barrier_simple.lock ), NULL );
		mpc_thread_cond_init( &( tmp->barrier.barrier_simple.cond ), NULL );
		tmp->barrier_func = _mpc_coll_barrier_simple;
	}
	else
	{
		not_reachable();
	}
}

/* Broadcast */

void _mpc_coll_bcast_simple( void *buffer, const size_t size,
                             const int root, const mpc_lowcomm_communicator_t com_id,
                             struct mpc_lowcomm_coll_s *tmp )
{
	int local;
	int id;
	local = sctk_get_nb_task_local( com_id );
	id = mpc_lowcomm_communicator_rank( com_id, mpc_common_get_task_rank() );
	mpc_thread_mutex_lock( &tmp->broadcast.broadcast_simple.lock );
	{
		if ( size > tmp->broadcast.broadcast_simple.size )
		{
			tmp->broadcast.broadcast_simple.buffer =
			    sctk_realloc( tmp->broadcast.broadcast_simple.buffer, size );
			tmp->broadcast.broadcast_simple.size = size;
		}

		tmp->broadcast.broadcast_simple.done++;

		if ( root == id )
		{
			memcpy( tmp->broadcast.broadcast_simple.buffer, buffer, size );
		}

		if ( tmp->broadcast.broadcast_simple.done == local )
		{
			tmp->broadcast.broadcast_simple.done = 0;
			mpc_thread_cond_broadcast( &tmp->broadcast.broadcast_simple.cond );
		}
		else
		{
			mpc_thread_cond_wait( &tmp->broadcast.broadcast_simple.cond,
			                       &tmp->broadcast.broadcast_simple.lock );
		}

		if ( root != id )
		{
			memcpy( buffer, tmp->broadcast.broadcast_simple.buffer, size );
		}
	}
	mpc_thread_mutex_unlock( &tmp->broadcast.broadcast_simple.lock );
	_mpc_coll_barrier_simple( com_id, tmp );
}

void _mpc_coll_bcast_simple_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	if ( mpc_common_get_process_count() == 1 )
	{
		tmp->broadcast.broadcast_simple.buffer = NULL;
		tmp->broadcast.broadcast_simple.size = 0;
		tmp->broadcast.broadcast_simple.done = 0;
		mpc_thread_mutex_init( &( tmp->broadcast.broadcast_simple.lock ), NULL );
		mpc_thread_cond_init( &( tmp->broadcast.broadcast_simple.cond ), NULL );
		tmp->broadcast_func = _mpc_coll_bcast_simple;
	}
	else
	{
		not_reachable();
	}
}

/* Allreduce Simple */

static void _mpc_coll_allreduce_simple( const void *buffer_in, void *buffer_out,
                                        const size_t elem_size,
                                        const size_t elem_number,
                                        void ( *func )( const void *, void *, size_t,
                                                mpc_lowcomm_datatype_t ),
                                        const mpc_lowcomm_communicator_t com_id,
                                        const mpc_lowcomm_datatype_t data_type,
                                        struct mpc_lowcomm_coll_s *tmp )
{
	int local;
	size_t size;
	size = elem_size * elem_number;
	local = sctk_get_nb_task_local( com_id );
	mpc_thread_mutex_lock( &tmp->allreduce.allreduce_simple.lock );
	{
		if ( size > tmp->allreduce.allreduce_simple.size )
		{
			tmp->allreduce.allreduce_simple.buffer =
			    sctk_realloc( tmp->allreduce.allreduce_simple.buffer, size );
			tmp->allreduce.allreduce_simple.size = size;
		}

		if ( tmp->allreduce.allreduce_simple.done == 0 )
		{
			if ( buffer_in != SCTK_IN_PLACE )
			{
				memcpy( tmp->allreduce.allreduce_simple.buffer, buffer_in, size );
			}
			else
			{
				memcpy( tmp->allreduce.allreduce_simple.buffer, buffer_out, size );
			}
		}
		else
		{
			func( buffer_in, tmp->allreduce.allreduce_simple.buffer, elem_number, data_type );
		}

		tmp->allreduce.allreduce_simple.done++;

		if ( tmp->allreduce.allreduce_simple.done == local )
		{
			tmp->allreduce.allreduce_simple.done = 0;
			mpc_thread_cond_broadcast( &tmp->allreduce.allreduce_simple.cond );
		}
		else
		{
			mpc_thread_cond_wait( &tmp->allreduce.allreduce_simple.cond,
			                       &tmp->allreduce.allreduce_simple.lock );
		}

		memcpy( buffer_out, tmp->allreduce.allreduce_simple.buffer, size );
	}
	mpc_thread_mutex_unlock( &tmp->allreduce.allreduce_simple.lock );
	_mpc_coll_barrier_simple( com_id, tmp );
}

void _mpc_coll_allreduce_simple_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	if ( mpc_common_get_process_count() == 1 )
	{
		tmp->allreduce.allreduce_simple.buffer = NULL;
		tmp->allreduce.allreduce_simple.size = 0;
		tmp->allreduce.allreduce_simple.done = 0;
		mpc_thread_mutex_init( &( tmp->allreduce.allreduce_simple.lock ), NULL );
		mpc_thread_cond_init( &( tmp->allreduce.allreduce_simple.cond ), NULL );
		tmp->allreduce_func = _mpc_coll_allreduce_simple;
	}
	else
	{
		not_reachable();
	}
}

/* Init */

void mpc_lowcomm_coll_init_simple( mpc_lowcomm_communicator_t id )
{
	if ( mpc_common_get_process_count() == 1 )
	{
		_mpc_coll_init( id,
		                _mpc_coll_barrier_simple_init,
		                _mpc_coll_bcast_simple_init,
		                _mpc_coll_allreduce_simple_init );
	}
	else
	{
		not_reachable();
	}
}

/*******************************
 * COMMON COLLECTIVE MESSAGING *
 *******************************/

/* Request data-structures */
static void _mpc_coll_free_message( __UNUSED__ void *ptr )
{
}

typedef struct
{
	mpc_lowcomm_request_t request;
	mpc_lowcomm_ptp_message_t msg;
} _mpc_coll_messages_t;

/* WARNING: if you change values below
   make sure the array in _mpc_coll_messages_table_t
   is large enough to handle the number of requests */

#define OPT_COLL_MAX_ASYNC 32
#define HETERO_COLL_MAX_ASYNC 1
#define OPT_NOALLOC_MAX_ASYNC 32

typedef struct
{
	_mpc_coll_messages_t msg_req[64 /* Update for ASYNC */];
	unsigned int nb_used;
} _mpc_coll_messages_table_t;

/* Internal functions */

static void _mpc_coll_message_send( const mpc_lowcomm_communicator_t communicator, int myself, int dest, int tag, void *buffer, size_t size,
                                    mpc_lowcomm_ptp_message_class_t message_class, _mpc_coll_messages_t *msg_req, int check_msg )
{
	mpc_lowcomm_ptp_message_header_clear( &( msg_req->msg ), MPC_LOWCOMM_MESSAGE_CONTIGUOUS,
	                                      _mpc_coll_free_message, mpc_lowcomm_ptp_message_copy );
	mpc_lowcomm_ptp_message_set_contiguous_addr( &( msg_req->msg ), buffer, size );
	mpc_lowcomm_ptp_message_header_init( &( msg_req->msg ), tag, communicator, myself, dest,
	                                     &( msg_req->request ), size, message_class,
	                                     SCTK_DATATYPE_IGNORE, REQUEST_SEND );
	_mpc_comm_ptp_message_send_check( &( msg_req->msg ), check_msg );
}

static void _mpc_coll_message_recv( const mpc_lowcomm_communicator_t communicator, int src, int myself, int tag, void *buffer, size_t size,
                                    mpc_lowcomm_ptp_message_class_t message_class, _mpc_coll_messages_t *msg_req, int check_msg )
{
	mpc_lowcomm_ptp_message_header_clear( &( msg_req->msg ), MPC_LOWCOMM_MESSAGE_CONTIGUOUS,
	                                      _mpc_coll_free_message, mpc_lowcomm_ptp_message_copy );
	mpc_lowcomm_ptp_message_set_contiguous_addr( &( msg_req->msg ), buffer, size );
	mpc_lowcomm_ptp_message_header_init( &( msg_req->msg ), tag, communicator, src, myself,
	                                     &( msg_req->request ), size, message_class,
	                                     SCTK_DATATYPE_IGNORE, REQUEST_RECV );
	_mpc_comm_ptp_message_recv_check( &( msg_req->msg ), check_msg );
}

static void _mpc_coll_messages_table_wait( _mpc_coll_messages_table_t *tab )
{
	unsigned int i;

	for ( i = 0; i < tab->nb_used; i++ )
	{
		sctk_nodebug( "Wait for messag %d", i );
		mpc_lowcomm_request_wait( &( tab->msg_req[i].request ) );
	}

	tab->nb_used = 0;
}

static _mpc_coll_messages_t *_mpc_coll_message_table_get_item( _mpc_coll_messages_table_t *tab, unsigned int max_async_request )
{
	_mpc_coll_messages_t *tmp;

	if ( tab->nb_used == max_async_request )
	{
		_mpc_coll_messages_table_wait( tab );
	}

	tmp = &( tab->msg_req[tab->nb_used] );
	sctk_nodebug( "tab->nb_used = %d", tab->nb_used );
	tab->nb_used++;
	return tmp;
}

static void _mpc_coll_message_table_init( _mpc_coll_messages_table_t *tab )
{
	tab->nb_used = 0;
}

/*********************************
 * OPT COLLECTIVE IMPLEMENTATION *
 *********************************/

/* Barrier */

static void _mpc_coll_opt_barrier( const mpc_lowcomm_communicator_t communicator, __UNUSED__ struct mpc_lowcomm_coll_s *tmp )
{
	if ( !sctk_is_inter_comm( communicator ) )
	{
		int myself;
		int total;
		int total_max;
		int i;
		_mpc_coll_messages_table_t table;
		char c = 'c';
		sctk_nodebug( "_mpc_coll_opt_barrier() begin:" ); //AMAHEO
		_mpc_coll_message_table_init( &table );
		total = mpc_lowcomm_communicator_size( communicator );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		sctk_nodebug( "enter barrier total = %d, myself = %d", total,
		              myself );
		total_max = log( total ) / log( barrier_arity );
		total_max = pow( barrier_arity, total_max );

		if ( total_max < total )
		{
			total_max = total_max * barrier_arity;
		}

		assume( total_max >= total );

		for ( i = barrier_arity; i <= total_max; i = i * barrier_arity )
		{
			{
				if ( myself % i == 0 )
				{
					int src;
					int j;
					src = myself;

					for ( j = 1; j < barrier_arity; j++ )
					{
						if ( ( src + ( j * ( i / barrier_arity ) ) ) < total )
						{
							_mpc_coll_message_recv(
							    communicator, src + ( j * ( i / barrier_arity ) ),
							    myself, 0, &c, 1, MPC_LOWCOMM_BARRIER_MESSAGE,
							    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
						}
					}

					_mpc_coll_messages_table_wait( &table );
				}
				else
				{
					int dest;
					dest = ( myself / i ) * i;

					if ( dest >= 0 )
					{
						_mpc_coll_message_send(
						    communicator, myself, dest, 0, &c, 1,
						    MPC_LOWCOMM_BARRIER_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
						_mpc_coll_message_recv(
						    communicator, dest, myself, 1, &c, 1,
						    MPC_LOWCOMM_BARRIER_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
						_mpc_coll_messages_table_wait( &table );
						break;
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );

		for ( ; i >= barrier_arity; i = i / barrier_arity )
		{
			if ( myself % i == 0 )
			{
				int dest;
				int j;
				dest = myself;

				for ( j = 1; j < barrier_arity; j++ )
				{
					if ( ( dest + ( j * ( i / barrier_arity ) ) ) < total )
					{
						_mpc_coll_message_send(
						    communicator, myself,
						    dest + ( j * ( i / barrier_arity ) ), 1, &c, 1,
						    MPC_LOWCOMM_BARRIER_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );
	}
	else
	{
		int i, j;
		int rsize;
		int myself;
		char c = 'c';
		_mpc_coll_messages_table_t table;
		_mpc_coll_message_table_init( &table );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		rsize = mpc_lowcomm_communicator_remote_size( communicator );

		for ( i = 0; i < rsize; i++ )
		{
			_mpc_coll_message_send( communicator, myself, i, 65536, &c, 1,
			                        MPC_LOWCOMM_BARRIER_MESSAGE,
			                        _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
		}

		for ( j = 0; j < rsize; j++ )
		{
			_mpc_coll_message_recv(
			    communicator, j, myself, 65536, &c, 1, MPC_LOWCOMM_BARRIER_MESSAGE,
			    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
		}

		_mpc_coll_messages_table_wait( &table );
	}
}

void _mpc_coll_opt_barrier_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	barrier_arity = sctk_runtime_config_get()->modules.inter_thread_comm.barrier_arity;
	tmp->barrier_func = _mpc_coll_opt_barrier;
}

/* Broadcast */

void mpc_lowcomm_bcast_opt_messages( void *buffer, const size_t size,
                                const int root, const mpc_lowcomm_communicator_t communicator,
                                struct mpc_lowcomm_coll_s *tmp )
{
	if ( size == 0 )
	{
		_mpc_coll_opt_barrier( communicator, tmp );
	}
	else
	{
		int myself;
		int related_myself;
		int total;
		int total_max;
		int i;
		_mpc_coll_messages_table_t table;
		int BROADCAST_ARRITY = 2;
		_mpc_coll_message_table_init( &table );
		BROADCAST_ARRITY = broadcast_max_size / size;

		if ( BROADCAST_ARRITY < 2 )
		{
			BROADCAST_ARRITY = 2;
		}

		if ( BROADCAST_ARRITY > broadcast_arity_max )
		{
			BROADCAST_ARRITY = broadcast_arity_max;
		}

		total = mpc_lowcomm_communicator_size( communicator );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		related_myself = ( myself + total - root ) % total;
		total_max = log( total ) / log( BROADCAST_ARRITY );
		total_max = pow( BROADCAST_ARRITY, total_max );

		if ( total_max < total )
		{
			total_max = total_max * BROADCAST_ARRITY;
		}

		assume( total_max >= total );
		sctk_nodebug( "enter broadcast total = %d, total_max = %d, "
		              "myself = %d, BROADCAST_ARRITY = %d",
		              total, total_max, myself, BROADCAST_ARRITY );

		for ( i = BROADCAST_ARRITY; i <= total_max;
		      i = i * BROADCAST_ARRITY )
		{
			if ( related_myself % i != 0 )
			{
				int dest;
				dest = ( related_myself / i ) * i;

				if ( dest >= 0 )
				{
					_mpc_coll_message_recv(
					    communicator, ( dest + root ) % total, myself, root,
					    buffer, size, MPC_LOWCOMM_BROADCAST_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
					_mpc_coll_messages_table_wait( &table );
					break;
				}
			}
		}

		for ( ; i >= BROADCAST_ARRITY; i = i / BROADCAST_ARRITY )
		{
			if ( related_myself % i == 0 )
			{
				int dest;
				int j;
				dest = related_myself;

				for ( j = 1; j < BROADCAST_ARRITY; j++ )
				{
					if ( ( dest + ( j * ( i / BROADCAST_ARRITY ) ) ) < total )
					{
						_mpc_coll_message_send(
						    communicator, myself,
						    ( dest + root + ( j * ( i / BROADCAST_ARRITY ) ) ) %
						    total,
						    root, buffer, size, MPC_LOWCOMM_BROADCAST_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );
	}
}

void _mpc_coll_opt_bcast_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	broadcast_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_arity_max;
	broadcast_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_max_size;
	broadcast_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_check_threshold;
	tmp->broadcast_func = mpc_lowcomm_bcast_opt_messages;
}

/* Allreduce */

static void _mpc_coll_opt_allreduce_intern( const void *buffer_in, void *buffer_out,
        const size_t elem_size,
        const size_t elem_number,
        void ( *func )( const void *, void *, size_t,
                        mpc_lowcomm_datatype_t ),
        const mpc_lowcomm_communicator_t communicator,
        const mpc_lowcomm_datatype_t data_type,
        struct mpc_lowcomm_coll_s *tmp )
{
	if ( elem_number == 0 )
	{
		_mpc_coll_opt_barrier( communicator, tmp );
	}
	else
	{
		int myself;
		int total;
		size_t size;
		int i;
		void *buffer_tmp;
		void **buffer_table;
		_mpc_coll_messages_table_t table;
		int ALLREDUCE_ARRITY = 2;
		int total_max;
		/*
		  MPI require that the result of the allreduce is the same on all MPI tasks.
		  This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating
		  point datatypes.
		*/
		_mpc_coll_message_table_init( &table );
		size = elem_size * elem_number;
		ALLREDUCE_ARRITY = allreduce_max_size / size;

		if ( ALLREDUCE_ARRITY < 2 )
		{
			ALLREDUCE_ARRITY = 2;
		}

		if ( ALLREDUCE_ARRITY > allreduce_arity_max )
		{
			ALLREDUCE_ARRITY = allreduce_arity_max;
		}

		buffer_tmp = sctk_malloc( size * ( ALLREDUCE_ARRITY - 1 ) );
		buffer_table = sctk_malloc( ( ALLREDUCE_ARRITY - 1 ) * sizeof( void * ) );
		{
			int j;

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				buffer_table[j - 1] = ( ( char * ) buffer_tmp ) + ( size * ( j - 1 ) );
			}
		}

		if ( buffer_in != SCTK_IN_PLACE )
		{
			memcpy( buffer_out, buffer_in, size );
		}

		assume( size > 0 );
		total = mpc_lowcomm_communicator_size( communicator );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		total_max = log( total ) / log( ALLREDUCE_ARRITY );
		total_max = pow( ALLREDUCE_ARRITY, total_max );

		if ( total_max < total )
		{
			total_max = total_max * ALLREDUCE_ARRITY;
		}

		assume( total_max >= total );

		for ( i = ALLREDUCE_ARRITY; i <= total_max;
		      i = i * ALLREDUCE_ARRITY )
		{
			if ( myself % i == 0 )
			{
				int src;
				int j;
				src = myself;

				for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
				{
					if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
					{
						sctk_nodebug( "Recv from %d",
						              src + ( j * ( i / ALLREDUCE_ARRITY ) ) );
						_mpc_coll_message_recv(
						    communicator, src + ( j * ( i / ALLREDUCE_ARRITY ) ),
						    myself, 0, buffer_table[j - 1], size,
						    MPC_LOWCOMM_ALLREDUCE_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
					}
				}

				_mpc_coll_messages_table_wait( &table );

				for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
				{
					if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
					{
						func( buffer_table[j - 1], buffer_out, elem_number,
						      data_type );
					}
				}
			}
			else
			{
				int dest;
				dest = ( myself / i ) * i;

				if ( dest >= 0 )
				{
					memcpy( buffer_tmp, buffer_out, size );
					sctk_nodebug( "Leaf send to %d", dest );
					_mpc_coll_message_send(
					    communicator, myself, dest, 0, buffer_tmp, size,
					    MPC_LOWCOMM_ALLREDUCE_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
					sctk_nodebug( "Leaf Recv from %d", dest );
					_mpc_coll_message_recv( communicator, dest, myself, 1,
					                        buffer_out, size,
					                        MPC_LOWCOMM_ALLREDUCE_MESSAGE,
					                        _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ),
					                        0 );
					_mpc_coll_messages_table_wait( &table );
					break;
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );

		for ( ; i >= ALLREDUCE_ARRITY; i = i / ALLREDUCE_ARRITY )
		{
			if ( myself % i == 0 )
			{
				int dest;
				int j;
				dest = myself;

				for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
				{
					if ( ( dest + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
					{
						sctk_nodebug( "send to %d",
						              dest + ( j * ( i / ALLREDUCE_ARRITY ) ) );
						_mpc_coll_message_send(
						    communicator, myself,
						    dest + ( j * ( i / ALLREDUCE_ARRITY ) ), 1, buffer_out,
						    size, MPC_LOWCOMM_ALLREDUCE_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_COLL_MAX_ASYNC ), 0 );
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );
		sctk_free( buffer_tmp );
		sctk_free( buffer_table );
	}
}

static void _mpc_coll_opt_allreduce( const void *buffer_in, void *buffer_out,
                                     const size_t elem_size,
                                     const size_t elem_number,
                                     void ( *func )( const void *, void *, size_t,
                                             mpc_lowcomm_datatype_t ),
                                     const mpc_lowcomm_communicator_t communicator,
                                     const mpc_lowcomm_datatype_t data_type,
                                     struct mpc_lowcomm_coll_s *tmp )
{
	_mpc_coll_opt_allreduce_intern( buffer_in, buffer_out, elem_size, elem_number, func, communicator, data_type, tmp );
}

void _mpc_coll_opt_allreduce_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	allreduce_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_arity_max;
	allreduce_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_max_size;
	allreduce_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_check_threshold;
	tmp->allreduce_func = _mpc_coll_opt_allreduce;
}

/* Init */

void mpc_lowcomm_coll_init_opt( mpc_lowcomm_communicator_t id )
{
	_mpc_coll_init( id,
	                _mpc_coll_opt_barrier_init,
	                _mpc_coll_opt_bcast_init,
	                _mpc_coll_opt_allreduce_init );
	/* if(get_process_rank() == 0){ */
	/*   mpc_common_debug_warning("Use low performances collectives"); */
	/* } */
}

/************************************
 * HETERO COLLECTIVE IMPLEMENTATION *
 ************************************/

/* Hetero Collective Parameters */

/* Barrier */

static int int_cmp( const void *a, const void *b )
{
	const int *ia = ( const int * ) a;
	const int *ib = ( const int * ) b;
	return *ia - *ib;
}

static void _mpc_coll_hetero_barrier_inter( const mpc_lowcomm_communicator_t communicator,
        __UNUSED__ struct mpc_lowcomm_coll_s *tmp )
{
	int myself;
	int *process_array;
	int total = sctk_get_process_nb_in_array( communicator );
	int *myself_ptr = NULL;
	int total_max;
	int i;
	_mpc_coll_messages_table_t table;
	char c = 'c';

	/* If only one process involved, we return */
	if ( total == 1 )
	{
		return;
	}

	sctk_nodebug( "Start inter" );
	_mpc_coll_message_table_init( &table );
	int process_rank = mpc_common_get_process_rank();
	process_array = sctk_get_process_array( communicator ),
	myself_ptr = ( ( int * ) bsearch( ( void * ) &process_rank,
	                                  process_array,
	                                  total, sizeof( int ), int_cmp ) );
	sctk_nodebug( "rank : %d, myself_ptr: %p", mpc_common_get_process_rank(), myself_ptr );
	assume( myself_ptr );
	myself = ( myself_ptr - process_array );
	total_max = log( total ) / log( barrier_arity );
	total_max = pow( barrier_arity, total_max );

	if ( total_max < total )
	{
		total_max = total_max * barrier_arity;
	}

	assume( total_max >= total );

	for ( i = barrier_arity; i <= total_max; i = i * barrier_arity )
	{
		if ( myself % i == 0 )
		{
			int src;
			int j;
			src = myself;

			for ( j = 1; j < barrier_arity; j++ )
			{
				if ( ( src + ( j * ( i / barrier_arity ) ) ) < total )
				{
					sctk_nodebug( "Recv %d to %d", src + ( j * ( i / barrier_arity ) ),
					              myself );
					_mpc_coll_message_recv(
					    communicator,
					    process_array[src + ( j * ( i / barrier_arity ) )],
					    process_array[myself], 0, &c, 1,
					    MPC_LOWCOMM_BARRIER_HETERO_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),  1 );
				}
			}

			_mpc_coll_messages_table_wait( &table );
		}
		else
		{
			int dest;
			dest = ( myself / i ) * i;

			if ( dest >= 0 )
			{
				sctk_nodebug( "send %d to %d", myself, dest );
				_mpc_coll_message_send(
				    communicator, process_array[myself], process_array[dest], 0,
				    &c, 1, MPC_LOWCOMM_BARRIER_HETERO_MESSAGE,
				    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ), 0 );
				sctk_nodebug( "recv %d to %d", dest, myself );
				_mpc_coll_message_recv(
				    communicator, process_array[dest], process_array[myself], 1,
				    &c, 1, MPC_LOWCOMM_BARRIER_HETERO_MESSAGE,
				    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),  0 );
				_mpc_coll_messages_table_wait( &table );
				break;
			}
		}
	}

	_mpc_coll_messages_table_wait( &table );

	for ( ; i >= barrier_arity; i = i / barrier_arity )
	{
		if ( myself % i == 0 )
		{
			int dest;
			int j;
			dest = myself;

			for ( j = 1; j < barrier_arity; j++ )
			{
				if ( ( dest + ( j * ( i / barrier_arity ) ) ) < total )
				{
					sctk_nodebug( "send %d to %d", myself,
					              dest + ( j * ( i / barrier_arity ) ) );
					_mpc_coll_message_send(
					    communicator, process_array[myself],
					    process_array[dest + ( j * ( i / barrier_arity ) )], 1, &c, 1,
					    MPC_LOWCOMM_BARRIER_HETERO_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ), 1 );
				}
			}
		}
	}

	_mpc_coll_messages_table_wait( &table );
	sctk_nodebug( "End inter" );
}

static void _mpc_coll_hetero_barrier( const mpc_lowcomm_communicator_t communicator,
                                      struct mpc_lowcomm_coll_s *tmp )
{
	int nb_tasks_in_node;
	_mpc_coll_hetero_barrier_t *barrier;
	unsigned int generation;
	int task_id_in_node;
	nb_tasks_in_node = sctk_get_nb_task_local( communicator );
	barrier = &tmp->barrier.barrier_hetero_messages;
	generation = barrier->generation;
	task_id_in_node =
	    OPA_fetch_and_incr_int( &barrier->tasks_entered_in_node );

	if ( task_id_in_node == nb_tasks_in_node - 1 )
	{
		_mpc_coll_hetero_barrier_inter( communicator, tmp );
		OPA_store_int( &barrier->tasks_entered_in_node, 0 );
		barrier->generation = generation + 1;
		OPA_write_barrier();
	}
	else
	{
		while ( barrier->generation < generation + 1 )
		{
			mpc_thread_yield();
		}
	}
}

void _mpc_coll_hetero_barrier_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	barrier_arity = sctk_runtime_config_get()->modules.inter_thread_comm.barrier_arity;
	tmp->barrier_func = _mpc_coll_hetero_barrier;
	_mpc_coll_hetero_barrier_t *barrier;
	barrier = &tmp->barrier.barrier_hetero_messages;
	OPA_store_int( &barrier->tasks_entered_in_node, 0 );
	barrier->generation = 0;
}

/* Broadcast */

void _mpc_coll_hetero_bcast_inter( void *buffer, const size_t size,
                                   const int root_process, const mpc_lowcomm_communicator_t communicator )
{
	/* If only one process involved, we return */
	int total = sctk_get_process_nb_in_array( communicator );

	if ( total == 1 )
	{
		return;
	}

	{
		int myself;
		int *myself_ptr = NULL;
		int related_myself;
		int total_max;
		int i;
		_mpc_coll_messages_table_t table;
		int BROADCAST_ARRITY;
		int specific_tag = MPC_LOWCOMM_BROADCAST_HETERO_MESSAGE;
		int *process_array;
		int root;
		_mpc_coll_message_table_init( &table );
		BROADCAST_ARRITY = broadcast_max_size / size;

		if ( BROADCAST_ARRITY < 2 )
		{
			BROADCAST_ARRITY = 2;
		}

		if ( BROADCAST_ARRITY > broadcast_arity_max )
		{
			BROADCAST_ARRITY = broadcast_arity_max;
		}

		int process_rank = mpc_common_get_process_rank();
		process_array = sctk_get_process_array( communicator );
		myself_ptr = ( ( int * ) bsearch( ( void * ) &process_rank,
		                                  process_array,
		                                  total, sizeof( int ), int_cmp ) );
		assume( myself_ptr );
		myself = ( myself_ptr - process_array );
		myself_ptr = ( ( int * ) bsearch( ( void * ) &root_process,
		                                  process_array,
		                                  total, sizeof( int ), int_cmp ) );
		assume( myself_ptr );
		root = ( myself_ptr - process_array );
		related_myself = ( myself + total - root ) % total;
		total_max = log( total ) / log( BROADCAST_ARRITY );
		total_max = pow( BROADCAST_ARRITY, total_max );

		if ( total_max < total )
		{
			total_max = total_max * BROADCAST_ARRITY;
		}

		assume( total_max >= total );

		for ( i = BROADCAST_ARRITY; i <= total_max;
		      i = i * BROADCAST_ARRITY )
		{
			if ( related_myself % i != 0 )
			{
				int dest;
				dest = ( related_myself / i ) * i;

				if ( dest >= 0 )
				{
					_mpc_coll_message_recv(
					    communicator, process_array[( dest + root ) % total],
					    process_array[myself], root_process, buffer, size,
					    specific_tag, _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),
					    1 );
					_mpc_coll_messages_table_wait( &table );
					break;
				}
			}
		}

		for ( ; i >= BROADCAST_ARRITY; i = i / BROADCAST_ARRITY )
		{
			if ( related_myself % i == 0 )
			{
				int dest;
				int j;
				dest = related_myself;

				for ( j = 1; j < BROADCAST_ARRITY; j++ )
				{
					if ( ( dest + ( j * ( i / BROADCAST_ARRITY ) ) ) < total )
					{
						_mpc_coll_message_send(
						    communicator, process_array[myself],
						    process_array[( dest + root +
						                    ( j * ( i / BROADCAST_ARRITY ) ) ) %
						                  total],
						    root_process, buffer, size, specific_tag,
						    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),
						    ( size < (size_t)broadcast_check_threshold ) );
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );
	}
}

void _mpc_coll_hetero_bcast( void *buffer, const size_t size,
                             const int root, const mpc_lowcomm_communicator_t communicator,
                             struct mpc_lowcomm_coll_s *tmp )
{
	int nb_tasks_in_node;
	int task_id_in_node;
	unsigned int generation;
	_mpc_coll_hetero_bcast_t *bcast;
	int myself;
	int is_root_on_node = 0;
	int root_process;

	if ( size == 0 )
	{
		_mpc_coll_hetero_barrier( communicator, tmp );
		return;
	}

	myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
	nb_tasks_in_node = sctk_get_nb_task_local( communicator );
	bcast = &tmp->broadcast.broadcast_hetero_messages;
	generation = bcast->generation;
	task_id_in_node = OPA_fetch_and_incr_int( &bcast->tasks_entered_in_node );
	/* Looking if root is on node */
	root_process = sctk_get_process_rank_from_task_rank( root );

	if ( root_process == mpc_common_get_process_rank() )
	{
		is_root_on_node = 1;
	}

	if ( ( ( is_root_on_node ) && ( root == myself ) ) ||
	     ( ( !is_root_on_node ) && task_id_in_node == 0 ) )
	{
		/* Begin inter node communications */
		_mpc_coll_hetero_bcast_inter( buffer, size,
		                              root_process, communicator );
		/* End inter node communications */
		OPA_store_ptr( &bcast->buff_root, buffer );
		OPA_write_barrier();

		while ( OPA_load_int( &bcast->tasks_exited_in_node ) != nb_tasks_in_node - 1 )
		{
			mpc_thread_yield();
		}

		/* Reinit all vars */
		OPA_store_int( &bcast->tasks_entered_in_node, 0 );
		OPA_store_int( &bcast->tasks_exited_in_node, 0 );
		OPA_store_ptr( &bcast->buff_root, NULL );
		bcast->generation = generation + 1;
		OPA_write_barrier();
	}
	else
	{
		void *buff_root = NULL;

		/* Wait until the buffer is ready */
		while ( ( buff_root = OPA_load_ptr( &bcast->buff_root ) ) == NULL )
		{
			mpc_thread_yield();
		}

		memcpy( buffer, buff_root, size );
		OPA_incr_int( &bcast->tasks_exited_in_node );

		while ( bcast->generation < generation + 1 )
		{
			mpc_thread_yield();
		}
	}
}

void _mpc_coll_hetero_bcast_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	broadcast_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_arity_max;
	broadcast_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_max_size;
	broadcast_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_check_threshold;
	tmp->broadcast_func = _mpc_coll_hetero_bcast;
	_mpc_coll_hetero_bcast_t *bcast;
	bcast = &tmp->broadcast.broadcast_hetero_messages;
	OPA_store_int( &bcast->tasks_entered_in_node, 0 );
	OPA_store_int( &bcast->tasks_exited_in_node, 0 );
	OPA_store_ptr( &bcast->buff_root, NULL );
	bcast->generation = 0;
}

/* Allreduce */
static void _mpc_coll_hetero_allreduce_intern_inter( const void *buffer_in, void *buffer_out,
        const size_t elem_size,
        const size_t elem_number,
        void ( *func )( const void *, void *, size_t,
                        mpc_lowcomm_datatype_t ),
        const mpc_lowcomm_communicator_t communicator,
        const mpc_lowcomm_datatype_t data_type,
        __UNUSED__ struct mpc_lowcomm_coll_s *tmp )
{
	int ALLREDUCE_ARRITY = 2;
	int total_max;
	_mpc_coll_messages_table_t table;
	void *buffer_tmp;
	void **buffer_table;
	int myself;
	int *myself_ptr;
	size_t size = elem_size * elem_number;
	int specific_tag = MPC_LOWCOMM_ALLREDUCE_HETERO_MESSAGE;
	int total = sctk_get_process_nb_in_array( communicator );
	int i;
	int *process_array;

	/* If only one process involved, we return */
	if ( total == 1 )
	{
		return;
	}

	/*
	   MPI require that the result of the allreduce is the same on all MPI tasks.
	   This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating
	   point datatypes.
	   */
	_mpc_coll_message_table_init( &table );
	ALLREDUCE_ARRITY = allreduce_max_size / size;

	if ( ALLREDUCE_ARRITY < 2 )
	{
		ALLREDUCE_ARRITY = 2;
	}

	if ( ALLREDUCE_ARRITY > allreduce_arity_max )
	{
		ALLREDUCE_ARRITY = allreduce_arity_max;
	}

	buffer_tmp = sctk_malloc( size * ( ALLREDUCE_ARRITY - 1 ) );
	buffer_table = sctk_malloc( ( ALLREDUCE_ARRITY - 1 ) * sizeof( void * ) );
	{
		int j;

		for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
		{
			buffer_table[j - 1] = ( ( char * ) buffer_tmp ) + ( size * ( j - 1 ) );
		}
	}
	int process_rank = mpc_common_get_process_rank();
	process_array = sctk_get_process_array( communicator ),
	myself_ptr = ( ( int * ) bsearch( ( void * ) &process_rank,
	                                  process_array,
	                                  total, sizeof( int ), int_cmp ) );
	assume( myself_ptr );
	myself = ( myself_ptr - process_array );
	total_max = log( total ) / log( ALLREDUCE_ARRITY );
	total_max = pow( ALLREDUCE_ARRITY, total_max );

	if ( total_max < total )
	{
		total_max = total_max * ALLREDUCE_ARRITY;
	}

	assume( total_max >= total );

	for ( i = ALLREDUCE_ARRITY; i <= total_max; i = i * ALLREDUCE_ARRITY )
	{
		if ( myself % i == 0 )
		{
			int src;
			int j;
			src = myself;

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
				{
					sctk_nodebug( "Recv from %d",
					              src + ( j * ( i / ALLREDUCE_ARRITY ) ) );
					_mpc_coll_message_recv(
					    communicator,
					    process_array[src + ( j * ( i / ALLREDUCE_ARRITY ) )],
					    process_array[myself], 0, buffer_table[j - 1], size,
					    specific_tag, _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),
					    0 );
				}
			}

			memcpy( buffer_out, buffer_in, size );
			_mpc_coll_messages_table_wait( &table );

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
				{
					func( buffer_table[j - 1], buffer_out, elem_number, data_type );
				}
			}
		}
		else
		{
			int dest;
			dest = ( myself / i ) * i;

			if ( dest >= 0 )
			{
				memcpy( buffer_tmp, buffer_out, size );
				sctk_nodebug( "src %d Leaf send to %d", myself, dest );
				_mpc_coll_message_send(
				    communicator, process_array[myself], process_array[dest], 0,
				    buffer_tmp, size, specific_tag,
				    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ), 1 );
				sctk_nodebug( "Leaf Recv from %d", dest );
				_mpc_coll_message_recv(
				    communicator, process_array[dest], process_array[myself], 1,
				    buffer_out, size, specific_tag,
				    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),  1 );
				_mpc_coll_messages_table_wait( &table );
				break;
			}
		}
	}

	_mpc_coll_messages_table_wait( &table );

	for ( ; i >= ALLREDUCE_ARRITY; i = i / ALLREDUCE_ARRITY )
	{
		if ( myself % i == 0 )
		{
			int dest;
			int j;
			dest = myself;

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				if ( ( dest + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
				{
					sctk_nodebug( "send to %d", dest + ( j * ( i / ALLREDUCE_ARRITY ) ) );
					_mpc_coll_message_send(
					    communicator, process_array[myself],
					    process_array[dest + ( j * ( i / ALLREDUCE_ARRITY ) )], 1,
					    buffer_out, size, specific_tag,
					    _mpc_coll_message_table_get_item( &table, HETERO_COLL_MAX_ASYNC ),
					    ( size < (size_t)allreduce_check_threshold ) );
				}
			}
		}
	}

	_mpc_coll_messages_table_wait( &table );
	sctk_free( buffer_tmp );
	sctk_free( buffer_table );
}

static void _mpc_coll_hetero_allreduce_hetero_intern( const void *buffer_in, void *buffer_out,
        const size_t elem_size,
        const size_t elem_number,
        void ( *func )( const void *, void *, size_t,
                        mpc_lowcomm_datatype_t ),
        const mpc_lowcomm_communicator_t communicator,
        const mpc_lowcomm_datatype_t data_type,
        struct mpc_lowcomm_coll_s *tmp )
{
	if ( elem_number == 0 )
	{
		_mpc_coll_hetero_barrier( communicator, tmp );
	}
	else
	{
		sctk_nodebug( "Starting allreduce" );
		int task_id_in_node;
		int nb_tasks_in_node;
		_mpc_coll_hetero_allreduce_t *allreduce;
		volatile void *volatile *buff_in;
		volatile void *volatile *buff_out;
		unsigned int generation;
		size_t size;
		size = elem_size * elem_number;
		assume( size );
		nb_tasks_in_node = sctk_get_nb_task_local( communicator );
		allreduce = &tmp->allreduce.allreduce_hetero_messages;
		generation = allreduce->generation;
		task_id_in_node =
		    OPA_fetch_and_incr_int( &allreduce->tasks_entered_in_node );
		buff_in = allreduce->buff_in;
		buff_out = allreduce->buff_out;
		/* Fill the buffer entry for all tasks */
		buff_in[task_id_in_node] = ( volatile void * ) buffer_in;
		buff_out[task_id_in_node] = buffer_out;
		OPA_write_barrier();

		/* Last entry */
		if ( task_id_in_node == nb_tasks_in_node - 1 )
		{
			int i;
			memcpy( buffer_out, buffer_in, size );

			/* Wait on all tasks and apply the reduction function.
			 * FIXME: reduction function must be done in parallel */
			for ( i = 0; i < nb_tasks_in_node - 1; ++i )
			{
				while ( buff_in[i] == NULL )
				{
					mpc_thread_yield();
				}

				sctk_nodebug( "Add content %d to buffer", *( ( int * ) buff_in[i] ) );
				func( ( const void * ) buff_in[i], buffer_out, elem_number, data_type );
			}

			sctk_nodebug( "Buffer content : %d", *( ( int * ) buffer_out ) );
			/* Begin allreduce inter-node */
			_mpc_coll_hetero_allreduce_intern_inter( buffer_in, buffer_out,
			        elem_size, elem_number, func,
			        communicator, data_type, tmp );
			/* End allreduce inter-node. Result is in buffer_out and must
			 * be propagate to all other tasks */
			allreduce->generation = generation + 1;
			OPA_write_barrier();

			while ( OPA_load_int( &allreduce->tasks_entered_in_node ) != 1 )
			{
				mpc_thread_yield();
			}

			buff_in[task_id_in_node] = NULL;
			buff_out[task_id_in_node] = NULL;
			OPA_store_int( &allreduce->tasks_entered_in_node, 0 );
			allreduce->generation = generation + 2;
		}
		else
		{
			while ( allreduce->generation < generation + 1 )
			{
				mpc_thread_yield();
			}

			memcpy( ( void * ) buffer_out,
			        ( void * ) buff_out[nb_tasks_in_node - 1], size );
			buff_in[task_id_in_node] = NULL;
			buff_out[task_id_in_node] = NULL;
			OPA_decr_int( &allreduce->tasks_entered_in_node );

			while ( allreduce->generation < generation + 2 )
			{
				mpc_thread_yield();
			}
		}

		sctk_nodebug( "End allreduce" );
	}
}

static void _mpc_coll_hetero_allreduce( const void *buffer_in, void *buffer_out,
                                        const size_t elem_size,
                                        const size_t elem_number,
                                        void ( *func )( const void *, void *, size_t,
                                                mpc_lowcomm_datatype_t ),
                                        const mpc_lowcomm_communicator_t communicator,
                                        const mpc_lowcomm_datatype_t data_type,
                                        struct mpc_lowcomm_coll_s *tmp )
{
	_mpc_coll_hetero_allreduce_hetero_intern( buffer_in, buffer_out, elem_size, elem_number, func, communicator, data_type, tmp );
}

void _mpc_coll_hetero_allreduce_init( struct mpc_lowcomm_coll_s *tmp, mpc_lowcomm_communicator_t id )
{
	allreduce_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_arity_max;
	allreduce_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_max_size;
	allreduce_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_check_threshold;
	tmp->allreduce_func = _mpc_coll_hetero_allreduce;
	_mpc_coll_hetero_allreduce_t *allreduce;
	int nb_tasks_in_node;
	nb_tasks_in_node = sctk_get_nb_task_local( id );
	allreduce = &tmp->allreduce.allreduce_hetero_messages;
	OPA_store_int( &allreduce->tasks_entered_in_node, 0 );
	allreduce->generation = 0;
	allreduce->buff_in = sctk_malloc( nb_tasks_in_node * sizeof( void * ) );
	allreduce->buff_out = sctk_malloc( nb_tasks_in_node * sizeof( void * ) );
	memset( ( void * ) allreduce->buff_in, 0, nb_tasks_in_node * sizeof( void * ) );
	memset( ( void * ) allreduce->buff_out, 0, nb_tasks_in_node * sizeof( void * ) );
}

/* Init */

void mpc_lowcomm_coll_init_hetero( mpc_lowcomm_communicator_t id )
{
	_mpc_coll_init( id,
	                _mpc_coll_hetero_barrier_init,
	                _mpc_coll_hetero_bcast_init,
	                _mpc_coll_hetero_allreduce_init );
	/* if(get_process_rank() == 0){ */
	/*   mpc_common_debug_warning("Use low performances collectives"); */
	/* } */
}

/******************************************
 * OPT NO ALLOC COLLECTIBE IMPLEMENTATION *
 ******************************************/

/* Barrier */

static void _mpc_coll_noalloc_barrier( const mpc_lowcomm_communicator_t communicator, __UNUSED__ struct mpc_lowcomm_coll_s *tmp )
{
	if ( !sctk_is_inter_comm( communicator ) )
	{
		int myself;
		int total;
		int total_max;
		int i;
		_mpc_coll_messages_table_t table;
		char c = 'c';
		_mpc_coll_message_table_init( &table );
		total = mpc_lowcomm_communicator_size( communicator );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		sctk_nodebug( "enter barrier total = %d, myself = %d", total,
		              myself );
		total_max = log( total ) / log( barrier_arity );
		total_max = pow( barrier_arity, total_max );

		if ( total_max < total )
		{
			total_max = total_max * barrier_arity;
		}

		assume( total_max >= total );

		for ( i = barrier_arity; i <= total_max; i = i * barrier_arity )
		{
			{
				if ( myself % i == 0 )
				{
					int src;
					int j;
					src = myself;

					for ( j = 1; j < barrier_arity; j++ )
					{
						if ( ( src + ( j * ( i / barrier_arity ) ) ) < total )
						{
							_mpc_coll_message_recv(
							    communicator, src + ( j * ( i / barrier_arity ) ),
							    myself, 0, &c, 1, MPC_LOWCOMM_BARRIER_MESSAGE,
							    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ),
							    0 );
						}
					}

					_mpc_coll_messages_table_wait( &table );
				}
				else
				{
					int dest;
					dest = ( myself / i ) * i;

					if ( dest >= 0 )
					{
						_mpc_coll_message_send(
						    communicator, myself, dest, 0, &c, 1,
						    MPC_LOWCOMM_BARRIER_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ), 0 );
						_mpc_coll_message_recv(
						    communicator, dest, myself, 1, &c, 1,
						    MPC_LOWCOMM_BARRIER_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ),
						    0 );
						_mpc_coll_messages_table_wait( &table );
						break;
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );

		for ( ; i >= barrier_arity; i = i / barrier_arity )
		{
			if ( myself % i == 0 )
			{
				int dest;
				int j;
				dest = myself;

				for ( j = 1; j < barrier_arity; j++ )
				{
					if ( ( dest + ( j * ( i / barrier_arity ) ) ) < total )
					{
						_mpc_coll_message_send(
						    communicator, myself,
						    dest + ( j * ( i / barrier_arity ) ), 1, &c, 1,
						    MPC_LOWCOMM_BARRIER_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ), 0 );
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );
	}
	else
	{
		int i, j;
		int rsize;
		int myself;
		char c = 'c';
		_mpc_coll_messages_table_t table;
		_mpc_coll_message_table_init( &table );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		rsize = mpc_lowcomm_communicator_remote_size( communicator );

		for ( i = 0; i < rsize; i++ )
		{
			_mpc_coll_message_send(
			    communicator, myself, i, 65536, &c, 1, MPC_LOWCOMM_BARRIER_MESSAGE,
			    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ), 0 );
		}

		for ( j = 0; j < rsize; j++ )
		{
			_mpc_coll_message_recv(
			    communicator, j, myself, 65536, &c, 1, MPC_LOWCOMM_BARRIER_MESSAGE,
			    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ),  0 );
		}

		_mpc_coll_messages_table_wait( &table );
	}
}

void _mpc_coll_noalloc_barrier_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	barrier_arity = sctk_runtime_config_get()->modules.inter_thread_comm.barrier_arity;
	tmp->barrier_func = _mpc_coll_noalloc_barrier;
}

/* Broadcast */

void _mpc_coll_noalloc_bcast( void *buffer, const size_t size,
                              const int root, const mpc_lowcomm_communicator_t communicator,
                              struct mpc_lowcomm_coll_s *tmp )
{
	if ( size == 0 )
	{
		_mpc_coll_noalloc_barrier( communicator, tmp );
	}
	else
	{
		int myself;
		int related_myself;
		int total;
		int total_max;
		int i;
		_mpc_coll_messages_table_t table;
		int BROADCAST_ARRITY;
		_mpc_coll_message_table_init( &table );
		BROADCAST_ARRITY = broadcast_max_size / size;

		if ( BROADCAST_ARRITY < 2 )
		{
			BROADCAST_ARRITY = 2;
		}

		if ( BROADCAST_ARRITY > broadcast_arity_max )
		{
			BROADCAST_ARRITY = broadcast_arity_max;
		}

		total = mpc_lowcomm_communicator_size( communicator );
		myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
		related_myself = ( myself + total - root ) % total;
		total_max = log( total ) / log( BROADCAST_ARRITY );
		total_max = pow( BROADCAST_ARRITY, total_max );

		if ( total_max < total )
		{
			total_max = total_max * BROADCAST_ARRITY;
		}

		assume( total_max >= total );
		sctk_nodebug( "enter broadcast total = %d, total_max = %d, "
		              "myself = %d, BROADCAST_ARRITY = %d",
		              total, total_max, myself, BROADCAST_ARRITY );

		for ( i = BROADCAST_ARRITY; i <= total_max;
		      i = i * BROADCAST_ARRITY )
		{
			if ( related_myself % i != 0 )
			{
				int dest;
				dest = ( related_myself / i ) * i;

				if ( dest >= 0 )
				{
					_mpc_coll_message_recv(
					    communicator, ( dest + root ) % total, myself, root,
					    buffer, size, MPC_LOWCOMM_BROADCAST_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ),
					    0 );
					_mpc_coll_messages_table_wait( &table );
					break;
				}
			}
		}

		for ( ; i >= BROADCAST_ARRITY; i = i / BROADCAST_ARRITY )
		{
			if ( related_myself % i == 0 )
			{
				int dest;
				int j;
				dest = related_myself;

				for ( j = 1; j < BROADCAST_ARRITY; j++ )
				{
					if ( ( dest + ( j * ( i / BROADCAST_ARRITY ) ) ) < total )
					{
						_mpc_coll_message_send(
						    communicator, myself,
						    ( dest + root + ( j * ( i / BROADCAST_ARRITY ) ) ) %
						    total,
						    root, buffer, size, MPC_LOWCOMM_BROADCAST_MESSAGE,
						    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ), 0 );
					}
				}
			}
		}

		_mpc_coll_messages_table_wait( &table );
	}
}

void _mpc_coll_noalloc_bcast_init( struct mpc_lowcomm_coll_s *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	broadcast_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_arity_max;
	broadcast_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_max_size;
	broadcast_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_check_threshold;
	tmp->broadcast_func = _mpc_coll_noalloc_bcast;
}

/* Allreduce */

#define ALLREDUCE_ALLOC_BUFFER_COMMUNICATORS 10

static void _mpc_coll_noalloc_allreduce_intern( const void *buffer_in, void *buffer_out,
        const size_t elem_size,
        const size_t elem_number,
        void ( *func )( const void *, void *, size_t,
                        mpc_lowcomm_datatype_t ),
        const mpc_lowcomm_communicator_t communicator,
        const mpc_lowcomm_datatype_t data_type )
{
	int myself;
	int total;
	size_t size;
	int i;
	void *buffer_tmp;
	void **buffer_table;
	_mpc_coll_messages_table_t table;
	int ALLREDUCE_ARRITY;
	int total_max;
	static __thread int buffer_used = 0;
	int need_free = 0;
	/*
	  MPI require that the result of the allreduce is the same on all MPI tasks.
	  This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating
	  point datatypes.
	*/
	_mpc_coll_message_table_init( &table );
	size = elem_size * elem_number;
	ALLREDUCE_ARRITY = allreduce_max_size / size;

	if ( ALLREDUCE_ARRITY < 2 )
	{
		ALLREDUCE_ARRITY = 2;
	}

	if ( ALLREDUCE_ARRITY > allreduce_arity_max )
	{
		ALLREDUCE_ARRITY = allreduce_arity_max;
	}

	if ( ( buffer_used == 1 ) || ( communicator < 0 ) || ( communicator >= ALLREDUCE_ALLOC_BUFFER_COMMUNICATORS ) )
	{
		buffer_tmp = sctk_malloc( size * ( ALLREDUCE_ARRITY - 1 ) );
		buffer_table = sctk_malloc( ( ALLREDUCE_ARRITY - 1 ) * sizeof( void * ) );
		need_free = 1;
	}
	else
	{
		static __thread void *buffer_tmp_loc[ALLREDUCE_ALLOC_BUFFER_COMMUNICATORS];
		static __thread void **buffer_table_loc[ALLREDUCE_ALLOC_BUFFER_COMMUNICATORS];
		static __thread size_t buffer_tmp_loc_size[ALLREDUCE_ALLOC_BUFFER_COMMUNICATORS];
		static __thread size_t buffer_table_loc_size[ALLREDUCE_ALLOC_BUFFER_COMMUNICATORS];
		need_free = 0;
		buffer_used = 1;

		if ( size * ( ALLREDUCE_ARRITY - 1 ) > buffer_tmp_loc_size[communicator] )
		{
			buffer_tmp_loc_size[communicator] = size * ( ALLREDUCE_ARRITY - 1 );
			sctk_free( buffer_tmp_loc[communicator] );
			buffer_tmp_loc[communicator] = sctk_malloc( buffer_tmp_loc_size[communicator] );
		}

		if ( ( ALLREDUCE_ARRITY - 1 ) * sizeof( void * ) > buffer_table_loc_size[communicator] )
		{
			buffer_table_loc_size[communicator] = ( ALLREDUCE_ARRITY - 1 ) * sizeof( void * );
			sctk_free( buffer_table_loc[communicator] );
			buffer_table_loc[communicator] = sctk_malloc( buffer_table_loc_size[communicator] );
		}

		buffer_tmp = buffer_tmp_loc[communicator];
		buffer_table = buffer_table_loc[communicator];
	}

	{
		int j;

		for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
		{
			buffer_table[j - 1] = ( ( char * ) buffer_tmp ) + ( size * ( j - 1 ) );
		}
	}

	if ( buffer_in != SCTK_IN_PLACE )
	{
		memcpy( buffer_out, buffer_in, size );
	}

	assume( size > 0 );
	total = mpc_lowcomm_communicator_size( communicator );
	myself = mpc_lowcomm_communicator_rank( communicator, mpc_common_get_task_rank() );
	total_max = log( total ) / log( ALLREDUCE_ARRITY );
	total_max = pow( ALLREDUCE_ARRITY, total_max );

	if ( total_max < total )
	{
		total_max = total_max * ALLREDUCE_ARRITY;
	}

	assume( total_max >= total );

	for ( i = ALLREDUCE_ARRITY; i <= total_max; i = i * ALLREDUCE_ARRITY )
	{
		if ( myself % i == 0 )
		{
			int src;
			int j;
			src = myself;

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
				{
					sctk_nodebug( "Recv from %d",
					              src + ( j * ( i / ALLREDUCE_ARRITY ) ) );
					_mpc_coll_message_recv(
					    communicator, src + ( j * ( i / ALLREDUCE_ARRITY ) ), myself, 0,
					    buffer_table[j - 1], size, MPC_LOWCOMM_ALLREDUCE_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ),
					    0 );
				}
			}

			_mpc_coll_messages_table_wait( &table );

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
				{
					func( buffer_table[j - 1], buffer_out, elem_number, data_type );
				}
			}
		}
		else
		{
			int dest;
			dest = ( myself / i ) * i;

			if ( dest >= 0 )
			{
				memcpy( buffer_tmp, buffer_out, size );
				sctk_nodebug( "Leaf send to %d", dest );
				_mpc_coll_message_send(
				    communicator, myself, dest, 0, buffer_tmp, size,
				    MPC_LOWCOMM_ALLREDUCE_MESSAGE,
				    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ), 0 );
				sctk_nodebug( "Leaf Recv from %d", dest );
				_mpc_coll_message_recv(
				    communicator, dest, myself, 1, buffer_out, size,
				    MPC_LOWCOMM_ALLREDUCE_MESSAGE,
				    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ),
				    0 );
				_mpc_coll_messages_table_wait( &table );
				break;
			}
		}
	}

	_mpc_coll_messages_table_wait( &table );

	for ( ; i >= ALLREDUCE_ARRITY; i = i / ALLREDUCE_ARRITY )
	{
		if ( myself % i == 0 )
		{
			int dest;
			int j;
			dest = myself;

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				if ( ( dest + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
				{
					sctk_nodebug( "send to %d", dest + ( j * ( i / ALLREDUCE_ARRITY ) ) );
					_mpc_coll_message_send(
					    communicator, myself, dest + ( j * ( i / ALLREDUCE_ARRITY ) ),
					    1, buffer_out, size, MPC_LOWCOMM_ALLREDUCE_MESSAGE,
					    _mpc_coll_message_table_get_item( &table, OPT_NOALLOC_MAX_ASYNC ), 0 );
				}
			}
		}
	}

	_mpc_coll_messages_table_wait( &table );

	if ( need_free == 1 )
	{
		sctk_free( buffer_tmp );
		sctk_free( buffer_table );
	}
	else
	{
		buffer_used = 0;
	}
}

static void _mpc_coll_noalloc_allreduce( const void *buffer_in, void *buffer_out,
        const size_t elem_size,
        const size_t elem_number,
        void ( *func )( const void *, void *, size_t,
                        mpc_lowcomm_datatype_t ),
        const mpc_lowcomm_communicator_t communicator,
        const mpc_lowcomm_datatype_t data_type,
        struct mpc_lowcomm_coll_s *tmp )
{
	if ( elem_number == 0 )
	{
		_mpc_coll_noalloc_barrier( communicator, tmp );
	}
	else
	{
#if 1

		if ( ( elem_size * elem_number > allreduce_max_slot ) && ( allreduce_max_slot > elem_size ) )
		{
			size_t elem_number_slot;
			size_t elem_number_remain;
			size_t elem_number_done = 0;
			size_t i;
			elem_number_slot = elem_number / ( ( elem_number ) / ( allreduce_max_slot / elem_size ) );

			if ( elem_number_slot < 1 )
			{
				elem_number_slot = 1;
			}

			elem_number_remain = elem_number % elem_number_slot;

			/* fprintf(stderr,"%lu %lu %lu %lu %lu\n",elem_number_slot,elem_number / elem_number_slot,elem_number_remain,elem_number,allreduce_max_slot / elem_size); */

			for ( i = 0; i < elem_number / elem_number_slot; i++ )
			{
				_mpc_coll_noalloc_allreduce_intern( ( ( char * ) buffer_in ) + ( elem_size * ( elem_number_done ) ),
				                                    ( ( char * ) buffer_out ) + ( elem_size * ( elem_number_done ) ),
				                                    elem_size, elem_number_slot, func, communicator, data_type );
				elem_number_done += elem_number_slot;
			}

			if ( elem_number_remain != 0 )
			{
				_mpc_coll_noalloc_allreduce_intern( ( ( char * ) buffer_in ) + ( elem_size * ( elem_number_done ) ),
				                                    ( ( char * ) buffer_out ) + ( elem_size * ( elem_number_done ) ),
				                                    elem_size, elem_number_remain, func, communicator, data_type );
			}
		}
		else
		{
			_mpc_coll_noalloc_allreduce_intern( buffer_in, buffer_out, elem_size, elem_number, func, communicator, data_type );
		}

#else
		_mpc_coll_noalloc_allreduce_intern( buffer_in, buffer_out, elem_size, elem_number, func, communicator, data_type );
#endif
	}
}

void _mpc_coll_noalloc_allreduce_allreduce_max_slot( int t )
{
	allreduce_max_slot = t;
}

void _mpc_coll_noalloc_allreduce_init( struct mpc_lowcomm_coll_s __UNUSED__ *tmp, __UNUSED__ mpc_lowcomm_communicator_t id )
{
	allreduce_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_arity_max;
	allreduce_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_max_size;
	allreduce_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_check_threshold;
	allreduce_max_slot = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_max_slot;
	tmp->allreduce_func = _mpc_coll_noalloc_allreduce;
}

/* Init */

void mpc_lowcomm_coll_init_noalloc( mpc_lowcomm_communicator_t id )
{
	_mpc_coll_init( id,
	                _mpc_coll_noalloc_barrier_init,
	                _mpc_coll_noalloc_bcast_init,
	                _mpc_coll_noalloc_allreduce_init );
	/* if(get_process_rank() == 0){ */
	/*   mpc_common_debug_warning("Use low performances collectives"); */
	/* } */
}

/**********************************************
 * GENERIC COLLECTIVE COMMUNICATION INTERFACE *
 **********************************************/

/************************************************************************/
/*Terminaison Barrier                                                   */
/************************************************************************/
void mpc_lowcomm_terminaison_barrier( void )
{
	int local;
	static volatile int done = 0;
	static mpc_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
	static mpc_thread_cond_t cond = SCTK_THREAD_COND_INITIALIZER;
	local = sctk_get_nb_task_local( SCTK_COMM_WORLD );
	mpc_thread_mutex_lock( &lock );
	done++;
	sctk_nodebug( "mpc_lowcomm_terminaison_barrier %d %d", done, local );

	if ( done == local )
	{
		done = 0;
#ifndef SCTK_LIB_MODE

		/* In libmode we do not want the pmi barrier
		 * to be called after MPI_Finalize (therefore we
		 * simply ignore it )*/
		if ( mpc_common_get_process_count() > 1 )
		{
			sctk_nodebug( "sctk_pmi_barrier" );
			mpc_launch_pmi_barrier();
		}

#endif
		sctk_nodebug( "WAKE ALL in mpc_lowcomm_terminaison_barrier" );
		mpc_thread_cond_broadcast( &cond );
	}
	else
	{
		sctk_nodebug( "WAIT in mpc_lowcomm_terminaison_barrier" );
		mpc_thread_cond_wait( &cond, &lock );
	}

	mpc_thread_mutex_unlock( &lock );
	sctk_nodebug( "mpc_lowcomm_terminaison_barrier %d %d DONE", done, local );
}

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/


int mpc_lowcomm_barrier_shm_on_context(struct shared_mem_barrier *barrier_ctx,
                                       int comm_size)
{
	int my_phase = !OPA_load_int(&barrier_ctx->phase);

	if(OPA_fetch_and_decr_int(&barrier_ctx->counter) == 1)
	{
		OPA_store_int(&barrier_ctx->counter, comm_size);
		OPA_store_int(&barrier_ctx->phase, my_phase);
	}
	else
	{
		while(OPA_load_int(&barrier_ctx->phase) != my_phase)
		{
	
				int cnt = 0;
				while(OPA_load_int(&barrier_ctx->phase) != my_phase)
				{
					if(128 < cnt++)
					{
						mpc_thread_yield();
					}
				}
		}
	}

	return SCTK_SUCCESS;
}


int __intercomm_barrier( const mpc_lowcomm_communicator_t communicator )
{
	mpc_lowcomm_barrier(sctk_get_local_comm_id(communicator));
	/* Todo understand intercomms */
	mpc_lowcomm_barrier(sctk_get_local_comm_id(communicator));
	return SCTK_SUCCESS;
}


int mpc_lowcomm_barrier( const mpc_lowcomm_communicator_t communicator )
{
	struct mpc_lowcomm_coll_s *tmp;

	if ( communicator == SCTK_COMM_SELF )
	{
		return SCTK_SUCCESS;
	}

	if(sctk_is_inter_comm(communicator))
	{
		return __intercomm_barrier(communicator);
	}

	if(sctk_is_shared_mem(communicator) )
	{
		/* Call the SHM version */
		struct sctk_comm_coll *    coll        = sctk_communicator_get_coll(communicator);

		if(!coll)
		{
			return SCTK_ERROR;
		}

		struct shared_mem_barrier *barrier_ctx = &coll->shm_barrier;
		return mpc_lowcomm_barrier_shm_on_context(barrier_ctx, coll->comm_size);
	}
	else
	{
		/* Call the inter-node version */
		tmp = _mpc_comm_get_internal_coll( communicator );
		tmp->barrier_func( communicator, tmp );
	}

	return SCTK_SUCCESS;
}

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/
void mpc_lowcomm_bcast( void *buffer, const size_t size,
                   const int root, const mpc_lowcomm_communicator_t communicator )
{
	struct mpc_lowcomm_coll_s *tmp;

	if ( communicator != SCTK_COMM_SELF )
	{
		tmp = _mpc_comm_get_internal_coll( communicator );
		tmp->broadcast_func( buffer, size, root, communicator, tmp );
	}
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
void mpc_lowcomm_allreduce( const void *buffer_in, void *buffer_out,
                       const size_t elem_size,
                       const size_t elem_number,
                       sctk_Op_f func,
                       const mpc_lowcomm_communicator_t communicator,
                       const mpc_lowcomm_datatype_t data_type )
{
	struct mpc_lowcomm_coll_s *tmp;

	if ( communicator != SCTK_COMM_SELF )
	{
		tmp = _mpc_comm_get_internal_coll( communicator );
		tmp->allreduce_func( buffer_in, buffer_out, elem_size,
		                     elem_number, func, communicator, data_type, tmp );
	}
	else
	{
		const size_t size = elem_size * elem_number;
		memcpy( buffer_out, buffer_in, size );
	}
}

/************************************************************************/
/*INIT                                                                  */
/************************************************************************/

void ( *mpc_lowcomm_coll_init_hook )( mpc_lowcomm_communicator_t id ) = NULL; //_mpc_coll_init_opt;

/*Init data structures used for task i*/
void _mpc_coll_init( mpc_lowcomm_communicator_t id,
                     void ( *barrier )( struct mpc_lowcomm_coll_s *, mpc_lowcomm_communicator_t id ),
                     void ( *broadcast )( struct mpc_lowcomm_coll_s *, mpc_lowcomm_communicator_t id ),
                     void ( *allreduce )( struct mpc_lowcomm_coll_s *, mpc_lowcomm_communicator_t id ) )
{
	struct mpc_lowcomm_coll_s *tmp;
	tmp = sctk_malloc( sizeof( struct mpc_lowcomm_coll_s ) );
	memset( tmp, 0, sizeof( struct mpc_lowcomm_coll_s ) );
	barrier( tmp, id );
	broadcast( tmp, id );
	allreduce( tmp, id );
	_mpc_comm_set_internal_coll( id, tmp );
}

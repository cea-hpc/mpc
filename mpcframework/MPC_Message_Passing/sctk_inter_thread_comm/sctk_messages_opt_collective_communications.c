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

#include <sctk_collective_communications.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <sctk_atomics.h>
#include <sctk_thread.h>
#include <string.h>
#include <math.h>

#include "sctk_runtime_config.h"

/************************************************************************/
/*PARAMETERS                                                            */
/************************************************************************/
static int barrier_arity;
static int broadcast_arity_max;
static int broadcast_max_size;
static int broadcast_check_threshold;
static int allreduce_arity_max;
static int allreduce_max_size;
static int allreduce_check_threshold;
#define SCTK_MAX_ASYNC 32
/* #warning "Change here !!!" */

/************************************************************************/
/*TOOLS                                                                 */
/************************************************************************/
static void sctk_free_opt_messages ( void *ptr )
{

}


typedef struct
{
	sctk_request_t request;
	sctk_thread_ptp_message_t msg;
} sctk_opt_messages_t;

typedef struct
{
	sctk_opt_messages_t msg_req[SCTK_MAX_ASYNC];
	int nb_used;
} sctk_opt_messages_table_t;


/************************************************************************/
/*FUNCTIONS                                                             */
/************************************************************************/

static void sctk_opt_messages_send ( const sctk_communicator_t communicator, int myself, int dest, int tag, void *buffer, size_t size,
                                     sctk_message_class_t message_class, sctk_opt_messages_t *msg_req, int check, int copy_in_send )
{
	sctk_init_header ( & ( msg_req->msg ), SCTK_MESSAGE_CONTIGUOUS, sctk_free_opt_messages, sctk_message_copy );
	sctk_add_adress_in_message ( & ( msg_req->msg ), buffer, size );
	msg_req->request.request_type = REQUEST_SEND_COLL;
	sctk_set_header_in_message ( & ( msg_req->msg ), tag, communicator, myself, dest, & ( msg_req->request ), size, message_class, SCTK_DATATYPE_IGNORE );

	sctk_send_message ( & ( msg_req->msg ) );
#if 0
	msg_req->msg.tail.need_check_in_wait = /* copy_in_send */1;
	sctk_send_message_try_check ( & ( msg_req->msg ), check );
#endif
}

static void sctk_opt_messages_recv ( const sctk_communicator_t communicator, int src, int myself, int tag, void *buffer, size_t size,
                                     sctk_message_class_t message_class, sctk_opt_messages_t *msg_req, struct sctk_internal_ptp_s *ptp_internal, int check,
                                     int copy_in_recv )
{

	sctk_init_header ( & ( msg_req->msg ), SCTK_MESSAGE_CONTIGUOUS, sctk_free_opt_messages, sctk_message_copy );
	sctk_add_adress_in_message ( & ( msg_req->msg ), buffer, size );
	msg_req->request.request_type = REQUEST_RECV_COLL;
	sctk_set_header_in_message ( & ( msg_req->msg ), tag, communicator,  src, myself, & ( msg_req->request ), size, message_class, SCTK_DATATYPE_IGNORE );

	sctk_recv_message ( & ( msg_req->msg ), ptp_internal, 1 );
#if 0
	msg_req->msg.tail.need_check_in_wait = /* copy_in_recv */1;
	sctk_recv_message_try_check ( & ( msg_req->msg ), ptp_internal, check );
#endif
}

static void sctk_opt_messages_wait ( sctk_opt_messages_table_t *tab )
{
	int i;

	for ( i = 0; i < tab->nb_used; i++ )
	{
		sctk_nodebug ( "Wait for messag %d", i );
		sctk_wait_message ( & ( tab->msg_req[i].request ) );
	}

	tab->nb_used = 0;
}



static sctk_opt_messages_t *sctk_opt_messages_get_item ( sctk_opt_messages_table_t *tab )
{
	sctk_opt_messages_t *tmp;

	if ( tab->nb_used == SCTK_MAX_ASYNC )
	{
		sctk_opt_messages_wait ( tab );
	}

	tmp = & ( tab->msg_req[tab->nb_used] );
	sctk_nodebug ( "tab->nb_used = %d", tab->nb_used );
	tab->nb_used++;
	return tmp;
}

static void sctk_opt_messages_init_items ( sctk_opt_messages_table_t *tab )
{
	tab->nb_used = 0;
}

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/

static void sctk_barrier_opt_messages ( const sctk_communicator_t communicator, sctk_internal_collectives_struct_t *tmp )
{
	if ( !sctk_is_inter_comm ( communicator ) )
	{
		int myself;
		int total;
		int total_max;
		int i;
		sctk_opt_messages_table_t table;
		char c = 'c';
		struct sctk_internal_ptp_s *ptp_internal;

		sctk_nodebug ( "sctk_barrier_opt_messages() begin:" ); //AMAHEO

		sctk_opt_messages_init_items ( &table );

		total = sctk_get_nb_task_total ( communicator );
		myself = sctk_get_rank ( communicator, sctk_get_task_rank() );
		ptp_internal = sctk_get_internal_ptp ( sctk_get_task_rank() );
		sctk_nodebug ( "enter barrier total = %d, myself = %d", total, myself );
		total_max = log ( total ) / log ( barrier_arity );
		total_max = pow ( barrier_arity, total_max );

		if ( total_max < total )
		{
			total_max = total_max * barrier_arity;
		}

		assume ( total_max >= total );

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
							sctk_opt_messages_recv ( communicator, src + ( j * ( i / barrier_arity ) ), myself, 0, &c, 1, SCTK_BARRIER_MESSAGE, sctk_opt_messages_get_item ( &table ), ptp_internal, 1, 1 );
						}
					}

					sctk_opt_messages_wait ( &table );
				}
				else
				{
					int dest;

					dest = ( myself / i ) * i;

					if ( dest >= 0 )
					{
						sctk_opt_messages_send ( communicator, myself, dest, 0, &c, 1, SCTK_BARRIER_MESSAGE, sctk_opt_messages_get_item ( &table ), 0, 1 );
						sctk_opt_messages_recv ( communicator, dest, myself, 1, &c, 1, SCTK_BARRIER_MESSAGE, sctk_opt_messages_get_item ( &table ), ptp_internal, 0, 1 );
						sctk_opt_messages_wait ( &table );
						break;
					}
				}
			}
		}

		sctk_opt_messages_wait ( &table );

		for ( ; i >= barrier_arity ; i = i / barrier_arity )
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
						sctk_opt_messages_send ( communicator, myself, dest + ( j * ( i / barrier_arity ) ), 1, &c, 1, SCTK_BARRIER_MESSAGE, sctk_opt_messages_get_item ( &table ), 1, 1 );
					}
				}
			}
		}

		sctk_opt_messages_wait ( &table );
	}
	else
	{
		int i, j;
		int dest;
		int size;
		int rsize;
		int total;
		int myself;
		char c = 'c';
		sctk_opt_messages_table_t table;
		struct sctk_internal_ptp_s *ptp_internal;

		sctk_opt_messages_init_items ( &table );

		total = sctk_get_nb_task_total ( communicator );
		myself = sctk_get_rank ( communicator, sctk_get_task_rank() );
		ptp_internal = sctk_get_internal_ptp ( sctk_get_task_rank() );

		rsize = sctk_get_nb_task_remote ( communicator );
		size = sctk_get_nb_task_total ( communicator );

		for ( i = 0 ; i < size ; i++ )
		{
			sctk_opt_messages_send (	communicator,
			                            myself,
			                            i,
			                            65536,
			                            &c,
			                            1,
			                            SCTK_BROADCAST_MESSAGE,
			                            sctk_opt_messages_get_item ( &table ),
			                            ( size < broadcast_check_threshold ),
			                            ( size < broadcast_check_threshold ) );
		}

		for ( j = 0 ; j < rsize ; j++ )
		{
			sctk_opt_messages_recv (	communicator,
			                            j,
			                            myself,
			                            65536,
			                            &c,
			                            1,
			                            SCTK_BROADCAST_MESSAGE,
			                            sctk_opt_messages_get_item ( &table ),
			                            ptp_internal,
			                            1,
			                            1 );
		}

		sctk_opt_messages_wait ( &table );
	}
}

void sctk_barrier_opt_messages_init ( sctk_internal_collectives_struct_t *tmp, sctk_communicator_t id )
{
	barrier_arity = sctk_runtime_config_get()->modules.inter_thread_comm.barrier_arity;

	tmp->barrier_func = sctk_barrier_opt_messages;
}

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/
void sctk_broadcast_opt_messages ( void *buffer, const size_t size,
                                   const int root, const sctk_communicator_t communicator,
                                   struct sctk_internal_collectives_struct_s *tmp )
{
	if ( size == 0 )
	{
		sctk_barrier_opt_messages ( communicator, tmp );
	}
	else
	{
		int myself;
		int related_myself;
		int total;
		int total_max;
		int i;
		sctk_opt_messages_table_t table;
		int BROADCAST_ARRITY = 2;
		struct sctk_internal_ptp_s *ptp_internal;

		sctk_opt_messages_init_items ( &table );

		BROADCAST_ARRITY = broadcast_max_size / size;

		if ( BROADCAST_ARRITY < 2 )
		{
			BROADCAST_ARRITY = 2;
		}

		if ( BROADCAST_ARRITY > broadcast_arity_max )
		{
			BROADCAST_ARRITY = broadcast_arity_max;
		}

		total = sctk_get_nb_task_total ( communicator );
		myself = sctk_get_rank ( communicator, sctk_get_task_rank() );
		related_myself = ( myself + total - root ) % total;
		ptp_internal = sctk_get_internal_ptp ( sctk_get_task_rank() );
		total_max = log ( total ) / log ( BROADCAST_ARRITY );
		total_max = pow ( BROADCAST_ARRITY, total_max );

		if ( total_max < total )
		{
			total_max = total_max * BROADCAST_ARRITY;
		}

		assume ( total_max >= total );

		sctk_nodebug ( "enter broadcast total = %d, total_max = %d, myself = %d, BROADCAST_ARRITY = %d", total, total_max, myself, BROADCAST_ARRITY );

		for ( i = BROADCAST_ARRITY; i <= total_max; i = i * BROADCAST_ARRITY )
		{
			if ( related_myself % i != 0 )
			{
				int dest;

				dest = ( related_myself / i ) * i;

				if ( dest >= 0 )
				{
					sctk_opt_messages_recv ( communicator, ( dest + root ) % total, myself, root, buffer, size, SCTK_BROADCAST_MESSAGE, sctk_opt_messages_get_item ( &table ), ptp_internal, 1, 1 );
					sctk_opt_messages_wait ( &table );
					break;
				}
			}
		}

		for ( ; i >= BROADCAST_ARRITY ; i = i / BROADCAST_ARRITY )
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
						sctk_opt_messages_send ( communicator, myself, ( dest + root + ( j * ( i / BROADCAST_ARRITY ) ) ) % total, root, buffer, size, SCTK_BROADCAST_MESSAGE,
						                         sctk_opt_messages_get_item ( &table ), ( size < broadcast_check_threshold ), ( size < broadcast_check_threshold ) );
					}
				}
			}
		}

		sctk_opt_messages_wait ( &table );
	}
}

void sctk_broadcast_opt_messages_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id )
{
	broadcast_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_arity_max;
	broadcast_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_max_size;
	broadcast_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.broadcast_check_threshold;

	tmp->broadcast_func = sctk_broadcast_opt_messages;
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
static void sctk_allreduce_opt_messages_intern ( const void *buffer_in, void *buffer_out,
                                                 const size_t elem_size,
                                                 const size_t elem_number,
                                                 void ( *func ) ( const void *, void *, size_t,
                                                                  sctk_datatype_t ),
                                                 const sctk_communicator_t communicator,
                                                 const sctk_datatype_t data_type,
                                                 struct sctk_internal_collectives_struct_s *tmp )
{
	if ( elem_number == 0 )
	{
		sctk_barrier_opt_messages ( communicator, tmp );
	}
	else
	{
		int myself;
		int total;
		size_t size;
		int i;
		void *buffer_tmp;
		void **buffer_table;
		sctk_opt_messages_table_t table;
		int ALLREDUCE_ARRITY = 2;
		int total_max;
		struct sctk_internal_ptp_s *ptp_internal;

		/*
		  MPI require that the result of the allreduce is the same on all MPI tasks.
		  This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating
		  point datatypes.
		*/
		sctk_opt_messages_init_items ( &table );

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

		buffer_tmp = sctk_malloc ( size * ( ALLREDUCE_ARRITY - 1 ) );
		buffer_table = sctk_malloc ( ( ALLREDUCE_ARRITY - 1 ) * sizeof ( void * ) );
		{
			int j;

			for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
			{
				buffer_table[j - 1] = ( ( char * ) buffer_tmp ) + ( size * ( j - 1 ) );
			}
		}

		if ( buffer_in != SCTK_IN_PLACE )
		{
			memcpy ( buffer_out, buffer_in, size );
		}

		assume ( size > 0 );

		total = sctk_get_nb_task_total ( communicator );
		myself = sctk_get_rank ( communicator, sctk_get_task_rank() );
		ptp_internal = sctk_get_internal_ptp ( sctk_get_task_rank() );

		total_max = log ( total ) / log ( ALLREDUCE_ARRITY );
		total_max = pow ( ALLREDUCE_ARRITY, total_max );

		if ( total_max < total )
		{
			total_max = total_max * ALLREDUCE_ARRITY;
		}

		assume ( total_max >= total );

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
						sctk_nodebug ( "Recv from %d", src + ( j * ( i / ALLREDUCE_ARRITY ) ) );
						sctk_opt_messages_recv ( communicator, src + ( j * ( i / ALLREDUCE_ARRITY ) ), myself, 0, buffer_table[j - 1], size, SCTK_ALLREDUCE_MESSAGE,
						                         sctk_opt_messages_get_item ( &table ), ptp_internal, 0, 0 );
					}
				}

				sctk_opt_messages_wait ( &table );

				for ( j = 1; j < ALLREDUCE_ARRITY; j++ )
				{
					if ( ( src + ( j * ( i / ALLREDUCE_ARRITY ) ) ) < total )
					{
						func ( buffer_table[j - 1], buffer_out, elem_number, data_type );
					}
				}
			}
			else
			{
				int dest;

				dest = ( myself / i ) * i;

				if ( dest >= 0 )
				{
					memcpy ( buffer_tmp, buffer_out, size );
					sctk_nodebug ( "Leaf send to %d", dest );
					sctk_opt_messages_send ( communicator, myself, dest, 0, buffer_tmp, size, SCTK_ALLREDUCE_MESSAGE, sctk_opt_messages_get_item ( &table ), 1, 1 );
					sctk_nodebug ( "Leaf Recv from %d", dest );
					sctk_opt_messages_recv ( communicator, dest, myself, 1, buffer_out, size, SCTK_ALLREDUCE_MESSAGE, sctk_opt_messages_get_item ( &table ), ptp_internal, 1, 1 );
					sctk_opt_messages_wait ( &table );
					break;
				}
			}
		}

		sctk_opt_messages_wait ( &table );

		for ( ; i >= ALLREDUCE_ARRITY ; i = i / ALLREDUCE_ARRITY )
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
						sctk_nodebug ( "send to %d", dest + ( j * ( i / ALLREDUCE_ARRITY ) ) );
						sctk_opt_messages_send ( communicator, myself, dest + ( j * ( i / ALLREDUCE_ARRITY ) ), 1, buffer_out, size, SCTK_ALLREDUCE_MESSAGE, sctk_opt_messages_get_item ( &table ),
						                         ( size < allreduce_check_threshold ), ( size < allreduce_check_threshold ) );
					}
				}
			}
		}

		sctk_opt_messages_wait ( &table );
		sctk_free ( buffer_tmp );
		sctk_free ( buffer_table );
	}
}

static void sctk_allreduce_opt_messages ( const void *buffer_in, void *buffer_out,
                                          const size_t elem_size,
                                          const size_t elem_number,
                                          void ( *func ) ( const void *, void *, size_t,
                                                           sctk_datatype_t ),
                                          const sctk_communicator_t communicator,
                                          const sctk_datatype_t data_type,
                                          struct sctk_internal_collectives_struct_s *tmp )
{
	TODO ( "Add buffer splitting" )
	sctk_allreduce_opt_messages_intern ( buffer_in, buffer_out, elem_size, elem_number, func, communicator, data_type, tmp );
}

void sctk_allreduce_opt_messages_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id )
{
	allreduce_arity_max = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_arity_max;
	allreduce_max_size = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_max_size;
	allreduce_check_threshold = sctk_runtime_config_get()->modules.inter_thread_comm.allreduce_check_threshold;

	tmp->allreduce_func = sctk_allreduce_opt_messages;
}

/************************************************************************/
/*Init                                                                  */
/************************************************************************/
void sctk_collectives_init_opt_messages ( sctk_communicator_t id )
{
	sctk_collectives_init ( id,
	                        sctk_barrier_opt_messages_init,
	                        sctk_broadcast_opt_messages_init,
	                        sctk_allreduce_opt_messages_init );
	/* if(sctk_process_rank == 0){ */
	/*   sctk_warning("Use low performances collectives"); */
	/* } */
}

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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_comm.h"

#include "sctk_inter_thread_comm.h"

/*  ###############
 *  Rank Querry 
 *  ############### */

int MPC_Net_get_rank()
{
	return sctk_get_task_rank();
}

int MPC_Net_get_comm_rank( const sctk_communicator_t communicator )
{
	return sctk_get_rank ( communicator, MPC_Net_get_rank() );
}

/*  ###############
 *  Communicators 
 *  ############### */

sctk_communicator_t MPC_Net_create_comm( const sctk_communicator_t origin_communicator,
										 const int nb_task_involved,
										 const int *task_list )
{
	return sctk_create_communicator ( origin_communicator, nb_task_involved, task_list );
}

sctk_communicator_t MPC_Net_delete_comm( const sctk_communicator_t comm )
{
	return sctk_delete_communicator ( comm );
}

/*  ###############
 *  Messages 
 *  ############### */

void MPC_Net_isend( int dest, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_request_t *req )
{
	sctk_message_isend( dest, data, size, tag, comm , req );
}

void MPC_Net_irecv( int src, void * buffer, size_t size, int tag, sctk_communicator_t comm , sctk_request_t *req )
{
	sctk_message_irecv( src,  buffer, size, tag, comm ,  req );
}

void MPC_Net_wait( sctk_request_t * request )
{
	sctk_wait_message( request );
}

void MPC_Net_send( int dest, void * data, size_t size, int tag, sctk_communicator_t comm )
{
	sctk_request_t req; 
	MPC_Net_isend( dest, data, size, tag, comm , &req );
	MPC_Net_wait( &req );
}

void MPC_Net_recv( int src, void * buffer, size_t size, int tag, sctk_communicator_t comm )
{
	sctk_request_t req; 
	MPC_Net_irecv( src, buffer, size, tag, comm, &req );
	MPC_Net_wait( &req );
}

void MPC_Net_sendrecv( void * sendbuf, size_t size, int dest, int tag, void * recvbuf, int src, int comm )
{
	sctk_sendrecv( sendbuf, size, dest, tag, recvbuf, src, comm );
}

/*  ###############
 *  Collectives 
 *  ############### */

void MPC_Net_barrier( sctk_communicator_t comm )
{
	sctk_barrier ( comm );
}

void MPC_Net_broadcast( void *buffer, const size_t size, 
                        const int root, const sctk_communicator_t communicator )
{
	sctk_broadcast ( buffer, size, root, communicator );
}

void MPC_Net_allreduce(const void *buffer_in, void *buffer_out,
                       const size_t elem_size,
                       const size_t elem_count,
                       sctk_Op_f func,
                       const sctk_communicator_t communicator,
                       sctk_datatype_t datatype )
{
	sctk_all_reduce ( buffer_in, buffer_out,elem_size, elem_count,
	                  func, communicator, datatype );
}

/*  ###############
 *  Setup & Teardown 
 *  ############### */
 
 void sctk_init_mpc_runtime();
 
 void MPC_Net_init()
 {
	 sctk_init_mpc_runtime();
	 MPC_Net_barrier( SCTK_COMM_WORLD );
 }

 void MPC_Net_release()
 {
	 MPC_Net_barrier( SCTK_COMM_WORLD );
 }

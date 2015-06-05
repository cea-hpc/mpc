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
#ifndef SCTK_COMM_H
#define SCTK_COMM_H
#include "sctk_types.h"
/*  ###############
 *  Rank Querry 
 *  ############### */
/** Get SCTK_COMM_WORLD rank **/
int MPC_Net_get_rank();
/** Get rank in a given communicator **/
int MPC_Net_get_comm_rank( const sctk_communicator_t communicator );
/*  ###############
 *  Communicators 
 *  ############### */
/** Create a new communicator **/
sctk_communicator_t MPC_Net_create_comm( const sctk_communicator_t origin_communicator,
										 const int nb_task_involved,
										 const int *task_list );
/** Delete a communicator **/
sctk_communicator_t MPC_Net_delete_comm( const sctk_communicator_t comm );
/*  ###############
 *  Messages 
 *  ############### */
/** Send an asynchronous message **/
void MPC_Net_isend( int dest, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_request_t *req );
/** Receive an asynchornous message **/
void MPC_Net_irecv( int src, void * buffer, size_t size, int tag, sctk_communicator_t comm , sctk_request_t *req );
/** Wait for a communication completion **/
void MPC_Net_wait( sctk_request_t * request );
/** Do a blocking send **/
void MPC_Net_send( int dest, void * data, size_t size, int tag, sctk_communicator_t comm );
/** Do a blocking recv **/
void MPC_Net_recv( int src, void * buffer, size_t size, int tag, sctk_communicator_t comm );
/** Do a blocking SendRecv **/
void MPC_Net_sendrecv( void * sendbuf, size_t size, int dest, int tag, void * recvbuf, int src, int comm );
/*  ###############
 *  Collectives 
 *  ############### */
/** Do a barrier on a communicator **/
void MPC_Net_barrier( sctk_communicator_t comm );
/** Do a broadcast on a communicator **/
void MPC_Net_broadcast( void *buffer, const size_t size, 
                        const int root, const sctk_communicator_t communicator );
/** Do a allreduce operation **/
void MPC_Net_allreduce(const void *buffer_in, void *buffer_out,
                       const size_t elem_size,
                       const size_t elem_count,
                       sctk_Op_f func,
                       const sctk_communicator_t communicator,
                       sctk_datatype_t datatype );
/*  ###############
 *  Setup & Teardown 
 *  ############### */
 /** Setup the MPC_Net environment (after MPI_Init) **/
 void MPC_Net_init();
 /** Release the MPC_Net environment **/
 void MPC_Net_release();
 /*  ###############
 *  MPI Hook Interface 
 *  ############### */
 /*
 int MPC_Net_hook_rank()
{
	int rank;

	MPI_Comm_rank( MPI_COMM_WORLD, & rank );
	return  rank;
}

int MPC_Net_hook_size()
{
	int size;

	MPI_Comm_size( MPI_COMM_WORLD, & size );

	return  size;
}

void MPC_Net_hook_barrier()
{
	MPI_Barrier( MPI_COMM_WORLD );
}

void MPC_Net_hook_send_to( void * data, size_t size, int target )
{
	MPI_Send( data, size, MPI_CHAR, target, 48895, MPI_COMM_WORLD );
}

void MPC_Net_hook_recv_from( void * data, size_t size, int source )
{
	MPI_Recv( data, size, MPI_CHAR, source, 48895, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
}
 */
#endif /* SCTK_COMM_H */

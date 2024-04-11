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
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #                                                                      # */
/* ######################################################################## */

// #define SCTK_DEBUG_SCHEDULER


#include "mpc_mpi.h"
#include <mpi_conf.h>

#include <stdbool.h>
#include <stddef.h>

#include <mpc_common_rank.h>
#include <mpc_mpi_internal.h>

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_nbc_weak.h"
#endif

#ifdef MPC_Threads
#include "mpc_thread_ng_engine.h"
#endif

#include "mpc_nbc_reduction_macros.h"

#include "egreq_nbc.h"
/* INTERNAL FOR NON-BLOCKING COLLECTIVES - CALL TO libNBC FUNCTIONS*/

/********************************************************************* *
 * The Following code comes from the libNBC, a library for the support *
 * of MPI-3 Non-Blocking Collective. The code was modified to ensure	 *
 * compatibility with the MPC framework.															 *
 * *********************************************************************/

static inline int NBC_Sched_create( NBC_Schedule *schedule );
static inline int NBC_Sched_send( void *buf, char tmpbuf, int count, MPI_Datatype datatype, int dest, NBC_Schedule *schedule );
static inline int NBC_Sched_recv( void *buf, char tmpbuf, int count, MPI_Datatype datatype, int source, NBC_Schedule *schedule );
static inline int NBC_Sched_op( void *buf3, char tmpbuf3, void *buf1, char tmpbuf1, void *buf2, char tmpbuf2, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule );
static inline int NBC_Sched_copy( void *src, char tmpsrc, int srccount, MPI_Datatype srctype, void *tgt, char tmptgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule );
static inline int NBC_Sched_unpack( void *inbuf, char tmpinbuf, int count, MPI_Datatype datatype, void *outbuf, char tmpoutbuf, NBC_Schedule *schedule );
static inline int NBC_Sched_barrier( NBC_Schedule *schedule );
static inline int NBC_Sched_commit( NBC_Schedule *schedule );
static inline int NBC_Progress( NBC_Handle *handle );
static inline int NBC_Start_round( NBC_Handle *handle );
static inline int NBC_Start_round_persistent( NBC_Handle *handle );
static inline int NBC_Free( NBC_Handle *handle );
static inline int NBC_Sched_send_pos( int pos, const void *buf, char tmpbuf, int count, MPI_Datatype datatype, int dest, NBC_Schedule *schedule );
static inline int NBC_Sched_recv_pos( int pos, void *buf, char tmpbuf, int count, MPI_Datatype datatype, int source, NBC_Schedule *schedule );
static inline int NBC_Sched_commit_pos( NBC_Schedule *schedule );
static inline int NBC_Sched_barrier_pos( int pos, NBC_Schedule *schedule );
static inline int NBC_Sched_op_pos( int pos, void *buf3, char tmpbuf3, void *buf1, char tmpbuf1, void *buf2, char tmpbuf2, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule );
static inline int NBC_Sched_copy_pos( int pos, void *src, char tmpsrc,
									  int srccount, MPI_Datatype srctype,
									  void *tgt, char tmptgt, int tgtcount,
									  MPI_Datatype tgttype,
									  NBC_Schedule *schedule );
int NBC_Sched_unpack_pos( int pos, void *inbuf, char tmpinbuf, int count,
						  MPI_Datatype datatype, void *outbuf, char tmpoutbuf,
						  NBC_Schedule *schedule );

/****************** *
 * NBC_IALLGATHER.C *
 * ******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* simple linear MPI_Iallgather
 * the algorithm uses p-1 rounds
 * each node sends the packet it received last round (or has in round 0) to it's right neighbor (modulo p)
 * each node receives from it's left (modulo p) neighbor */
#define RANK2VRANK( rank, vrank, root ) \
	{                                   \
		( vrank ) = rank;               \
		if ( ( rank ) == 0 )            \
			( vrank ) = root;           \
		if ( ( rank ) == ( root ) )     \
			( vrank ) = 0;              \
	}
#define VRANK2RANK( rank, vrank, root ) \
	{                                   \
		( rank ) = vrank;               \
		if ( ( vrank ) == 0 )           \
			( rank ) = root;            \
		if ( ( vrank ) == ( root ) )    \
			( rank ) = 0;               \
	}

int NBC_Iallgather( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int rank = 0;
	int p = 0;
	int res = 0;
	int r = 0;
	MPI_Aint rcvext = 0;
	MPI_Aint sndext = 0;
	NBC_Schedule *schedule = NULL;
	char *rbuf = NULL;
	char inplace = 0;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_IALLGATHER_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( recvtype, &rcvext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		if ( rank == 0 && !inplace )
		{ /*root contrib gather + bcast*/
			/* copy my data to receive buffer */
			rbuf = ( (char *) recvbuf ) + ( (MPI_Aint) rank * recvcount * rcvext );
			res = NBC_Copy( sendbuf, sendcount, sendtype, rbuf, recvcount,
							recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		int alloc_size = 0;
		int round_size = 0;
		int i = 0;
		int maxr = 0;
		int peer = 0;

		/* Algo Gather + Bcast */
		if ( rank == 0 )
		{
			alloc_size = (int) ( sizeof( int ) + sizeof( int ) +
								 ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
								 sizeof( char ) );
			round_size = p - 1;
		}
		else
		{
			alloc_size = sizeof( int ) + sizeof( int ) +
						 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						 sizeof( char );
			round_size = 1;
		}

		maxr = (int) ceil( ( log( p ) / LOG2 ) );
		int sends = 0;
		if ( rank == 0 )
		{
			sends = maxr;
		}
		else
		{
			int k = 0;
			sends = 0;
			for ( k = 0; k < maxr; k++ )
			{
				if ( ( ( rank + ( 1 << k ) < p ) && ( rank < ( 1 << k ) ) ) )
				{
					sends++;
				}
			}
			alloc_size += sizeof( int ) +
						  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  sizeof( char );
		}

		alloc_size += (int) ( sizeof( int ) +
							  sends * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							  sizeof( char ) );

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = round_size;

		/* Gather Part */

		int pos = sizeof( int );
		int pos_rounds = 0;

		/* send to root */
		if ( rank != 0 )
		{
			/* send msg to root */
			res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype,
									  0, schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
		else
		{
			for ( i = 0; i < p; i++ )
			{
				rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)i * recvcount * rcvext );
				if ( i != 0 )
				{
					/* root receives message to the right buffer */
					res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcount,
											  recvtype, i, schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_recv() "
								"(%i)\n",
								res );
						return res;
					}
				}
			}
		}

		NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );

		// receive from the right hosts
		if ( rank != 0 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			//          pos += sizeof(int);
			for ( r = 0; r < maxr; r++ )
			{
				if ( ( rank >= ( 1 << r ) ) && ( rank < ( 1 << ( r + 1 ) ) ) )
				{
					VRANK2RANK( peer, rank - ( 1 << r ), 0 );
					res = NBC_Sched_recv_pos( pos, recvbuf, 0, p * recvcount, recvtype, peer,
											  schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_recv() (%i)\n", res );
						return res;
					}
				}
			}
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
		}

		int cpt = 0;
		// now send to the right hosts
		for ( r = 0; r < maxr; r++ )
		{
			if ( ( ( rank + ( 1 << r ) < p ) && ( rank < ( 1 << r ) ) ) || ( rank == 0 ) )
			{
				VRANK2RANK( peer, rank + ( 1 << r ), 0 );
				cpt++;
				res = NBC_Sched_send_pos( pos, recvbuf, 0, p * recvcount, recvtype, peer,
										  schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
			}
		}
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = cpt;

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}

	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}

	if ( rank == 0 && !inplace )
	{ /*root contrib gather + bcast*/
		/* copy my data to receive buffer */
		rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)rank * recvcount * rcvext );
		res = NBC_Copy( sendbuf, sendcount, sendtype, rbuf, recvcount,
						recvtype, comm );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Copy() (%i)\n", res );
			return res;
		}
	}

	int i = 0;
	int maxr = 0;

	/* Gather Part */

	int pos = sizeof( int );
	int pos_rounds = 0;
	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	/* send to root */
	if ( rank != 0 )
	{
		/* send msg to root */
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
	}
	else
	{
		for ( i = 0; i < p; i++ )
		{
			if ( i != 0 )
			{
				/* root receives message to the right buffer */
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			}
		}
	}

	NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
	pos += sizeof( char );
	pos_rounds = pos;
	pos += sizeof( int );

	/// Bcast part

	// receive from the right hosts
	if ( rank != 0 )
	{
		*(int *) ( (char *) *handle->schedule + sizeof( int ) + pos_rounds ) = 1;
		//          pos += sizeof(int);
		for ( r = 0; r < maxr; r++ )
		{
			if ( ( rank >= ( 1 << r ) ) && ( rank < ( 1 << ( r + 1 ) ) ) )
			{
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
		}
		res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
	}

	handle->row_offset = sizeof( int ); /* have to reinit the offset */

	res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Start() (%i)\n", res );
		return res;
	}

	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Allgather using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcount Number of elements in sendbuf
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcount Number of elements in recvbuf
 *  \param recvtype Type of the data elements in recvbuf
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iallgather_init( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int rank = 0;
	int p = 0;
	int res = 0;
	int r = 0;
	MPI_Aint rcvext = 0;
	MPI_Aint sndext = 0;
	NBC_Schedule *schedule = NULL;
	char *rbuf = NULL;

	res = NBC_Init_handle( handle, comm, MPC_IALLGATHER_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( sendtype, &sndext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	int alloc_size = 0;
	int round_size = 0;
	int i = 0;
	int maxr = 0;
	int peer = 0;

	/* Algo Gather + Bcast */
	if ( rank == 0 )
	{
		alloc_size =
			(int)(sizeof( int ) + sizeof( int ) +
			( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
			sizeof( char ));
		round_size = p - 1;
	}
	else
	{
		alloc_size = sizeof( int ) + sizeof( int ) +
					 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					 sizeof( char );
		round_size = 1;
	}

	maxr = (int) ceil( ( log( p ) / LOG2 ) );
	//    alloc_size += sizeof(int);
	int sends = 0;
	if ( rank == 0 )
	{
		//      sends = (int)ceil((log(p) / LOG2));
		sends = maxr;
	}
	else
	{
		int k = 0;
		sends = 0;
		//      int maxr = (int)ceil((log(p) / LOG2));
		for ( k = 0; k < maxr; k++ )
		{
			if ( ( ( rank + ( 1 << k ) < p ) && ( rank < ( 1 << k ) ) ) )
			{
				sends++;
			}
		}
		alloc_size += sizeof( int ) +
					  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					  sizeof( char );
	}

	alloc_size += (int)(sizeof( int ) +
				  sends * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
				  sizeof( char ));

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;
	*( ( (int *) *schedule ) + 1 ) = round_size;

	/* Gather Part */

	int pos = sizeof( int );
	int pos_rounds = 0;

	/* send to root */
	if ( rank != 0 )
	{
		/* send msg to root */
		res = NBC_Sched_send_pos( pos, (void *) sendbuf, 0, sendcount, sendtype,
								  0, schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}
	}
	else
	{
		for ( i = 1; i < p; i++ )
		{
			rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)i * recvcount * rcvext );
			if ( i != 0 )
			{
				/* root receives message to the right buffer */
				res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcount,
										  recvtype, i, schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() "
							"(%i)\n",
							res );
					return res;
				}
			}
		}
	}

	NBC_Sched_barrier_pos( pos, schedule );
	pos += sizeof( char );
	pos_rounds = pos;
	pos += sizeof( int );

	/// Bcast part

	// receive from the right hosts
	if ( rank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		//          pos += sizeof(int);
		for ( r = 0; r < maxr; r++ )
		{
			if ( ( rank >= ( 1 << r ) ) && ( rank < ( 1 << ( r + 1 ) ) ) )
			{
				VRANK2RANK( peer, rank - ( 1 << r ), 0 );
				res = NBC_Sched_recv_pos( pos, recvbuf, 0, p * recvcount, recvtype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
			}
		}
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
	}

	int cpt = 0;
	//        pos_rounds = pos;
	//        pos += sizeof(int);
	// now send to the right hosts
	for ( r = 0; r < maxr; r++ )
	{
		if ( ( ( rank + ( 1 << r ) < p ) && ( rank < ( 1 << r ) ) ) || ( rank == 0 ) )
		{
			VRANK2RANK( peer, rank + ( 1 << r ), 0 );
			cpt++;
			res = NBC_Sched_send_pos( pos, recvbuf, 0, p * recvcount, recvtype, peer,
									  schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
	}
	*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = cpt;

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/******************* *
 * NBC_IALLGATHERV.C *
 * *******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* an allgatherv schedule can not be cached easily because the contents
 * ot the recvcounts array may change, so a comparison of the address
 * would not be sufficient ... we simply do not cache it */

/* simple linear MPI_Iallgatherv
 * the algorithm uses p-1 rounds
 * first round:
 *	 each node sends to it's left node (rank+1)%p sendcount elements
 *	 each node begins with it's right node (rank-11)%p and receives from it recvcounts[(rank+1)%p] elements
 * second round:
 *	 each node sends to node (rank+2)%p sendcount elements
 *	 each node receives from node (rank-2)%p recvcounts[(rank+2)%p] elements */
int NBC_Iallgatherv( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int rank = 0;
	int p = 0;
	int res = 0;
	int speer = 0;
	int rpeer = 0;
	MPI_Aint rcvext = 0;
	MPI_Aint sndext = 0;
	NBC_Schedule *schedule = NULL;
	char *rbuf = NULL;
	char inplace = 0;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );
	if ( !handle->is_persistent )
	{

		res = NBC_Init_handle( handle, comm, MPC_IALLGATHERV_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( recvtype, &rcvext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		if ( !inplace )
		{
			/* copy my data to receive buffer */
			rbuf = ( (char *) recvbuf ) + ( displs[rank] * rcvext );
			NBC_Copy( sendbuf, sendcount, sendtype, rbuf, recvcounts[rank], recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}
		if ( p >= 64 )
		{
			int alloc_size = 0;
			alloc_size += (int)(sizeof( int ) +
						  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  ( p - 1 ) * ( sizeof( int ) + sizeof( char ) ));
			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;
			*( ( (int *) *schedule ) + 1 ) = 2;
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}
			int pos = 0;
			int pos_rounds = pos;
			pos += sizeof( int );
			int r = 0;
			for ( r = 1; r < p; r++ )
			{

				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2;
				speer = ( rank + r ) % p;
				rpeer = ( rank - r + p ) % p;

				rbuf = ( (char *) recvbuf ) + ( displs[rpeer] * rcvext );
				res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rpeer], recvtype,
										  rpeer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype, speer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
			}
		}
		else
		{
			int alloc_size = 0;
			alloc_size += (int)(sizeof( int ) +
						  sizeof( int ) +
						  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  sizeof( char ));
			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;
			*( ( (int *) *schedule ) + 1 ) = 2 * ( p - 1 );
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}

			int pos = 0;
			int pos_rounds = pos;
			pos += sizeof( int );
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2 * ( p - 1 );
			int r = 0;
			for ( r = 1; r < p; r++ )
			{

				speer = ( rank + r ) % p;
				rpeer = ( rank - r + p ) % p;

				rbuf = ( (char *) recvbuf ) + ( displs[rpeer] * rcvext );
				res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rpeer], recvtype,
										  rpeer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype, speer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	
    res = _mpc_cl_comm_rank( comm, &rank );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
        return res;
    }
    res = _mpc_cl_comm_size( comm, &p );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
        return res;
    }
    res = PMPI_Type_extent( sendtype, &sndext );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
        return res;
    }
    res = PMPI_Type_extent( recvtype, &rcvext );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
        return res;
    }

    handle->tmpbuf = NULL;

    if ( !inplace )
    {
        /* copy my data to receive buffer */
        rbuf = ( (char *) recvbuf ) + ( displs[rank] * rcvext );
        NBC_Copy( sendbuf, sendcount, sendtype, rbuf, recvcounts[rank], recvtype, comm );
        if ( NBC_OK != res )
        {
            printf( "Error in NBC_Copy() (%i)\n", res );
            return res;
        }
    }

    handle->row_offset = sizeof( int ); /* have to reinit the offset */

    res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
    if ( NBC_OK != res )
    {
        printf( "Error in NBC_Start() (%i)\n", res );
        return res;
    }


	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Allgatherv using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcount Number of elements in sendbuf
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcounts Array (of length group size) containing the number of elements received from each process
 *  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
 *  \param recvtype Type of the data elements in recvbuf
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iallgatherv_init( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int rank = 0;
	int p = 0;
	int res = 0;
	int speer = 0;
	int rpeer = 0;
	MPI_Aint rcvext = 0;
	MPI_Aint sndext = 0;
	NBC_Schedule *schedule = NULL;
	char *rbuf = NULL;

	res = NBC_Init_handle( handle, comm, MPC_IALLGATHERV_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( sendtype, &sndext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	if ( p >= 64 )
	{
		int alloc_size = 0;
		alloc_size += (int)(sizeof( int ) +
					  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					  ( p - 1 ) * ( sizeof( int ) + sizeof( char ) ));
		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = 2;
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Sched_create (%i)\n", res );
			return res;
		}
		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		int r = 0;
		for ( r = 1; r < p; r++ )
		{

			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2;
			speer = ( rank + r ) % p;
			rpeer = ( rank - r + p ) % p;

			rbuf = ( (char *) recvbuf ) + ( displs[rpeer] * rcvext );
			res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rpeer], recvtype,
									  rpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype, speer,
									  schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
		}
	}
	else
	{
		int alloc_size = 0;
		alloc_size += (int)(sizeof( int ) +
					  sizeof( int ) +
					  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					  sizeof( char ));
		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = 2 * ( p - 1 );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Sched_create (%i)\n", res );
			return res;
		}

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2 * ( p - 1 );
		int r;
		for ( r = 1; r < p; r++ )
		{

			speer = ( rank + r ) % p;
			rpeer = ( rank - r + p ) % p;

			rbuf = ( (char *) recvbuf ) + ( displs[rpeer] * rcvext );
			res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rpeer], recvtype,
									  rpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype, speer,
									  schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/****************** *
 * NBC_IALLREDUCE.C *
 * ******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

static inline int ___allred_sched_diss( int rank, int p, int count, MPI_Datatype datatype, const void *sendbuf, void *recvbuf, MPI_Op op, NBC_Schedule *schedule, NBC_Handle *handle );
static inline int ___allred_sched_ring( int rank, int p, int count, MPI_Datatype datatype, const void *sendbuf, void *recvbuf, MPI_Op op, int size, int ext, NBC_Schedule *schedule, NBC_Handle *handle );


int NBC_Iallreduce( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res = -1, size;
	MPI_Aint ext;
	NBC_Schedule *schedule;
	enum
	{
		NBC_ARED_BINOMIAL,
		NBC_ARED_RING
	} alg;
	char inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_IALLREDUCE_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_size( datatype, &size );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_size() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = sctk_malloc( ext * count );
		if ( handle->tmpbuf == NULL )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return NBC_OOR;
		}

		if ( ( p == 1 ) && !inplace )
		{
			/* for a single node - copy data to receivebuf */
			res = NBC_Copy( sendbuf, count, datatype, recvbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		// TOFIX Segmentation Fault error when we choose the second algorithm
		// this problem is found and discribed in a mail to JJ in July 28, 2015
		/* algorithm selection */
		//	if(p < 4 || size*count < 65536*2)
		//	{
		alg = NBC_ARED_BINOMIAL;
		//	}
		//	else
		//	{
		//		alg = NBC_ARED_RING;
		//	}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		int recv_op_rounds = 0;

		int sends = 0;

		int maxr = (int) ceil( ( log( p ) / LOG2 ) );

		int root = 0;
		int alloc_size = 0;

		int vrank, vpeer, peer, r;
		switch ( alg )
		{
			case NBC_ARED_BINOMIAL:
				RANK2VRANK( rank, vrank, root );

				for ( r = 1; r <= maxr && ( vrank % ( 1 << r ) == 0 ); r++ )
				{
					vpeer = vrank + ( 1 << ( r - 1 ) );
					VRANK2RANK( peer, vpeer, root );
					if ( peer < p )
					{
						recv_op_rounds++;
					}
				}

				alloc_size =
					sizeof( int ) + sizeof( int ) +
					recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
									   sizeof( char ) + sizeof( int ) ) +
					recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
									   sizeof( char ) + sizeof( int ) );

				if ( rank != root )
					alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

				RANK2VRANK( rank, vrank, root );

				if ( vrank != 0 )
				{

					alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
								  sizeof( char ) + sizeof( int );
				}
				sends = 0;
				for ( r = 0; r < maxr; r++ )
				{
					if ( ( ( vrank + ( 1 << r ) < p ) && ( vrank < ( 1 << r ) ) ) ||
						 ( vrank == 0 ) )
					{
						sends++;
					}
				}
				alloc_size += sends * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							  sizeof( char );

				*schedule = sctk_malloc( alloc_size );
				*(int *) *schedule = alloc_size;

				res = ___allred_sched_diss( rank, p, count, datatype, sendbuf, recvbuf,
											op, schedule, handle );
				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}

				res = NBC_Sched_commit_pos( schedule );
				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_commit() (%i)\n", res );
					return res;
				}
				break;
			case NBC_ARED_RING:
				res = NBC_Sched_create( schedule );
				if ( res != NBC_OK )
				{
					printf( "Error in NBC_Sched_create (%i)\n", res );
					return res;
				}

				res = ___allred_sched_ring( rank, p, count, datatype, sendbuf, recvbuf,
											op, size, ext, schedule, handle );
				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}

				res = NBC_Sched_commit( schedule );
				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_commit() (%i)\n", res );
					return res;
				}
				break;
		}

		res = NBC_Start( handle, schedule );
		if ( res != NBC_OK )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_size( datatype, &size );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_size() (%i)\n", res );
			return res;
		}

		if ( ( p == 1 ) && !inplace )
		{
			/* for a single node - copy data to receivebuf */
			res = NBC_Copy( sendbuf, count, datatype, recvbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		alg = NBC_ARED_BINOMIAL;

		switch ( alg )
		{
			case NBC_ARED_BINOMIAL:

				break;
			case NBC_ARED_RING:
				/* not implemented */
				break;
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
		if ( res != NBC_OK )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
	}

	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Allreduce using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param count Number of elements in sendbuf
 *  \param datatype Type of the data elements in sendbuf
 *  \param op Reduction operation
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iallreduce_init( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, size;
	MPI_Aint ext;
	NBC_Schedule *schedule;
	enum
	{
		NBC_ARED_BINOMIAL,
		NBC_ARED_RING
	} alg;

	res = NBC_Init_handle( handle, comm, MPC_IALLREDUCE_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_size( datatype, &size );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_size() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = sctk_malloc( ext * count );
	if ( handle->tmpbuf == NULL )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return NBC_OOR;
	}

	// TOFIX Segmentation Fault error when we choose the second algorithm
	// this problem is found and discribed in a mail to JJ in July 28, 2015
	/* algorithm selection */
	//	if(p < 4 || size*count < 65536*2)
	//	{
	alg = NBC_ARED_BINOMIAL;
	//	}
	//	else
	//	{
	//		alg = NBC_ARED_RING;
	//	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	int recv_op_rounds = 0;

	int sends = 0;

	int maxr = (int) ceil( ( log( p ) / LOG2 ) );

	int root = 0;
	int alloc_size = 0;

	switch ( alg )
	{
		case NBC_ARED_BINOMIAL:;
			int vrank, vpeer, peer, r;
			RANK2VRANK( rank, vrank, root );

			for ( r = 1; r <= maxr && ( vrank % ( 1 << r ) == 0 ); r++ )
			{
				vpeer = vrank + ( 1 << ( r - 1 ) );
				VRANK2RANK( peer, vpeer, root );
				if ( peer < p )
				{
					recv_op_rounds++;
				}
			}

			alloc_size =
				sizeof( int ) + sizeof( int ) +
				recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
								   sizeof( char ) + sizeof( int ) ) +
				recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
								   sizeof( char ) + sizeof( int ) );

			if ( rank != root )
				alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

			RANK2VRANK( rank, vrank, root );

			if ( vrank != 0 )
			{

				alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							  sizeof( char ) + sizeof( int );
			}
			sends = 0;
			for ( r = 0; r < maxr; r++ )
			{
				if ( ( ( vrank + ( 1 << r ) < p ) && ( vrank < ( 1 << r ) ) ) ||
					 ( vrank == 0 ) )
				{
					sends++;
				}
			}
			alloc_size += sends * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  sizeof( char );

			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;

			res = ___allred_sched_diss( rank, p, count, datatype, sendbuf, recvbuf,
										op, schedule, handle );
			if ( NBC_OK != res )
			{
				printf( "Error in Schedule creation() (%i)\n", res );
				return res;
			}

			res = NBC_Sched_commit_pos( schedule );
			if ( res != NBC_OK )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_commit() (%i)\n", res );
				return res;
			}
			break;
		case NBC_ARED_RING:
			res = NBC_Sched_create( schedule );
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}

			res = ___allred_sched_ring( rank, p, count, datatype, sendbuf, recvbuf,
										op, size, ext, schedule, handle );
			if ( NBC_OK != res )
			{
				printf( "Error in Schedule creation() (%i)\n", res );
				return res;
			}

			res = NBC_Sched_commit( schedule );
			if ( res != NBC_OK )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_commit() (%i)\n", res );
				return res;
			}
			break;
	}

	handle->schedule = schedule;

	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/* binomial allreduce (binomial tree up and binomial bcast down)
 * working principle:
 * - each node gets a virtual rank vrank
 * - the 'root' node get vrank 0
 * - node 0 gets the vrank of the 'root'
 * - all other ranks stay identical (they do not matter)
 *
 * Algorithm:
 * pairwise exchange
 * round r:
 *	grp = rank % 2^r
 *	if grp == 0: receive from rank + 2^(r-1) if it exists and reduce value
 *	if grp == 1: send to rank - 2^(r-1) and exit function
 *
 * do this for R=log_2(p) rounds
 * followed by a Bcast:
 * Algorithm:
 * - each node with vrank > 2^r and vrank < 2^r+1 receives from node
 *	 vrank - 2^r (vrank=1 receives from 0, vrank 0 receives never)
 * - each node sends each round r to node vrank + 2^r
 * - a node stops to send if 2^r > commsize
 *
 */

static inline int ___allred_sched_diss( int rank, int p, int count, MPI_Datatype datatype, const void *sendbuf, void *recvbuf, MPI_Op op, NBC_Schedule *schedule, NBC_Handle *handle )
{
	int root, vrank, r, maxr, firstred, vpeer, peer, res;

	root = 0; /* this makes the code for ireduce and iallreduce nearly identical -
				 could be changed to improve performance */
	RANK2VRANK( rank, vrank, root );

	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	firstred = 1;
	int pos = 0;
	int pos_rounds = pos;
	pos += sizeof( int );
	for ( r = 1; r <= maxr; r++ )
	{
		if ( ( vrank % ( 1 << r ) ) == 0 )
		{
			/* we have to receive this round */
			vpeer = vrank + ( 1 << ( r - 1 ) );
			VRANK2RANK( peer, vpeer, root )
			if ( peer < p )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;

				res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, peer, schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				/* we have to wait until we have the data */
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				if ( firstred )
				{
					/* perform the reduce with the senbuf */
					res = NBC_Sched_op_pos( pos, recvbuf, 0, (void *) sendbuf, 0, 0, 1, count,
											datatype, op, schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					firstred = 0;
				}
				else
				{
					// perform the reduce in my local buffer
					res = NBC_Sched_op_pos( pos, recvbuf, 0, recvbuf, 0, 0, 1, count,
											datatype, op, schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				}

				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_op() (%i)\n", res );
					return res;
				}
				/* this cannot be done until handle->tmpbuf is unused :-( */
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;

				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
			}
		}
		else
		{
			/* we have to send this round */
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			vpeer = vrank - ( 1 << ( r - 1 ) );
			VRANK2RANK( peer, vpeer, root )
			if ( firstred )
			{
				// we have to use the sendbuf in the first round ..
				res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			else
			{
				// and the recvbuf in all remaining rounds
				res = NBC_Sched_send_pos( pos, recvbuf, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			if ( res != NBC_OK )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			// leave the game
			break;
		}
	}
	/* this is the Bcast part - copied with minor changes from nbc_ibcast.c

	 * changed: buffer -> recvbuf   */
	RANK2VRANK( rank, vrank, root );

	/* receive from the right hosts */
	if ( vrank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) += 1;

		for ( r = 0; r < maxr; r++ )
		{
			if ( ( vrank >= ( 1 << r ) ) && ( vrank < ( 1 << ( r + 1 ) ) ) )
			{
				VRANK2RANK( peer, vrank - ( 1 << r ), root );

				res = NBC_Sched_recv_pos( pos, recvbuf, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( res != NBC_OK )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
			}
		}
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
		;
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
	}
	int cpt = 0;
	/* now send to the right hosts */
	for ( r = 0; r < maxr; r++ )
	{
		if ( ( ( vrank + ( 1 << r ) < p ) && ( vrank < ( 1 << r ) ) ) || ( vrank == 0 ) )
		{
			VRANK2RANK( peer, vrank + ( 1 << r ), root );
			res =
				NBC_Sched_send_pos( pos, recvbuf, 0, count, datatype, peer, schedule );
			cpt++;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( res != NBC_OK )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
	}
	*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = cpt;

	/* end of the bcast */

	return NBC_OK;
}


static inline int ___allred_sched_ring( int r, int p, int count, MPI_Datatype datatype, const void *sendbuf, void *recvbuf, MPI_Op op, __UNUSED__ int size, int ext, NBC_Schedule *schedule, __UNUSED__ NBC_Handle *handle )
{
	int i;								 /* runner */
	int segsize, *segsizes, *segoffsets; /* segment sizes and offsets per segment (number of segments == number of nodes */
	int speer, rpeer;					 /* send and recvpeer */

	if ( count == 0 )
		return NBC_OK;

	{
		int mycount; /* temporary */
		segsizes = (int *) sctk_malloc( sizeof( int ) * p );
		segoffsets = (int *) sctk_malloc( sizeof( int ) * p );
		segsize = count / p; /* size of the segments */
		if ( count % p != 0 )
			segsize++;
		mycount = count;
		segoffsets[0] = 0;
		for ( i = 0; i < p; i++ )
		{
			mycount -= segsize;
			segsizes[i] = segsize;
			if ( mycount < 0 )
			{
				segsizes[i] = segsize + mycount;
				mycount = 0;
			}
			if ( i )
				segoffsets[i] = segoffsets[i - 1] + segsizes[i - 1];
		}
	}

	/* reduce peers */
	speer = ( r + 1 ) % p;
	rpeer = ( r - 1 + p ) % p;

	/*	+ -> reduced this round
	 *	/ -> sum (reduced in a previous step)
	 *
	 *		 *** round 0 ***
	 *		0				1				2
	 *
	 *	 00			 10			 20			0: [1] -> 1
	 *	 01			 11			 21			1: [2] -> 2
	 *	 02			 12			 22			2: [0] -> 0	--> send element (r+1)%p to node (r+1)%p
	 *
	 *			*** round 1 ***
	 *		0				1				2
	 *
	 *	 00+20		10			 20		 0: red(0), [0] -> 1
	 *	 01			 11+01		21		 1: red(1), [1] -> 2
	 *	 02			 12			 22+12	2: red(2), [2] -> 0 --> reduce and send element (r+0)%p to node (r+1)%p
	 *
	 *			*** round 2 ***
	 *		0				1				2
	 *
	 *	 00/20		all			20		 0: red(2), [2] -> 1
	 *	 01			 11/01		all		1: red(0), [0] -> 2
	 *	 all			12			 22/12	2: red(1), [1] -> 0 --> reduce and send (r-1)%p to node (r+1)%p
	 *
	 *			*** round 3 ***
	 *		0				1				2
	 *
	 *	 00/20		all			all		0: [1] -> 1
	 *	 all			11/01		all		1: [2] -> 2
	 *	 all			all			22/12	2: [0] -> 0 --> send element (r-2)%p to node (r+1)%p
	 *
	 *			*** round 4 ***
	 *		0				1				2
	 *
	 *	 all			all			all		0: done
	 *	 all			all			all		1: done
	 *	 all			all			all		2: done
	 *
	 * -> 4
	 *		 *** round 0 ***
	 *		0				1				2				3
	 *
	 *	 00			 10			 20			 30			 0: [1] -> 1
	 *	 01			 11			 21			 31			 1: [2] -> 2
	 *	 02			 12			 22			 32			 2: [3] -> 3
	 *	 03			 13			 23			 33			 3: [0] -> 0 --> send element (r+1)%p to node (r+1)%p
	 *
	 *			*** round 1 ***
	 *		0				1				2				3
	 *
	 *	 00+30		10			 20			 30			 0: red(0), [0] -> 1
	 *	 01			 11+01		21			 31			 1: red(1), [1] -> 2
	 *	 02			 12			 22+12		32			 2: red(2), [2] -> 3
	 *	 03			 13			 23			 33+23		3: red(3), [3] -> 0 --> reduce and send element (r+0)%p to node (r+1)%p
	 *
	 *			*** round 2 ***
	 *		0				1				2				3
	 *
	 *	 00/30		10+00/30 20			 30			 0: red(3), [3] -> 1
	 *	 01			 11/01		21+11/01 31			 1: red(0), [0] -> 2
	 *	 02			 12			 22/12		32+22/12 2: red(1), [1] -> 3
	 *	 03+33/23 13			 23			 33/23		3: red(2), [2] -> 0 --> reduce and send (r-1)%p to node (r+1)%p
	 *
	 *			*** round 3 ***
	 *		0				1				2				3
	 *
	 *	 00/30		10/00/30 all			30			 0: red(2), [2] -> 1
	 *	 01			 11/01		21/11/01 all			1: red(3), [3] -> 2
	 *	 all			12			 22/12		32/22/12 2: red(0), [0] -> 3
	 *	 03/33/23 all			23			 33/23		3: red(1), [1] -> 0 --> reduce and send (r-2)%p to node (r+1)%p
	 *
	 *			*** round 4 ***
	 *		0				1				2				3
	 *
	 *	 00/30		10/00/30 all			all			0: [1] -> 1
	 *	 all			11/01		21/11/01 all			1: [2] -> 2
	 *	 all			all			22/12		32/22/12 2: [3] -> 3
	 *	 03/33/23 all			all			33/23		3: [0] -> 0 --> receive and send element (r+1)%p to node (r+1)%p
	 *
	 *			*** round 5 ***
	 *		0				1				2				3
	 *
	 *	 all			10/00/30 all			all			0: [0] -> 1
	 *	 all			all			21/11/01 all			1: [1] -> 2
	 *	 all			all			all			32/22/12 2: [3] -> 3
	 *	 03/33/23 all			all			all			3: [4] -> 4 --> receive and send element (r-0)%p to node (r+1)%p
	 *
	 *			*** round 6 ***
	 *		0				1				2				3
	 *
	 *	 all			all			all			all
	 *	 all			all			all			all
	 *	 all			all			all			all
	 *	 all			all			all			all		 receive element (r-1)%p
	 *
	 *	 2p-2 rounds ... every node does p-1 reductions and p-1 sends
	 *
	 */
	{
		int round = 0;
		/* first p-1 rounds are reductions */
		do
		{
			int selement = ( r + 1 - round + 2 * p /*2*p avoids negative mod*/ ) % p; /* the element I am sending */
			int soffset = segoffsets[selement] * ext;
			int relement = ( r - round + 2 * p /*2*p avoids negative mod*/ ) % p; /* the element that I receive from my neighbor */
			int roffset = segoffsets[relement] * ext;

			/* first message come out of sendbuf */
			if ( round == 0 )
			{
				NBC_Sched_send( (char *) sendbuf + soffset, 0, segsizes[selement], datatype, speer, schedule );
			}
			else
			{
				NBC_Sched_send( (char *) recvbuf + soffset, 0, segsizes[selement], datatype, speer, schedule );
			}
			NBC_Sched_recv( (char *) recvbuf + roffset, 0, segsizes[relement], datatype, rpeer, schedule );

			NBC_Sched_barrier( schedule );
			NBC_Sched_op( (char *) recvbuf + roffset, 0, (char *) sendbuf + roffset, 0, (char *) recvbuf + roffset, 0, segsizes[relement], datatype, op, schedule );
			NBC_Sched_barrier( schedule );

			round++;
		} while ( round < p - 1 );

		do
		{
			int selement = ( r + 1 - round + 2 * p /*2*p avoids negative mod*/ ) % p; /* the element I am sending */
			int soffset = segoffsets[selement] * ext;
			int relement = ( r - round + 2 * p /*2*p avoids negative mod*/ ) % p; /* the element that I receive from my neighbor */
			int roffset = segoffsets[relement] * ext;

			NBC_Sched_send( (char *) recvbuf + soffset, 0, segsizes[selement], datatype, speer, schedule );
			NBC_Sched_recv( (char *) recvbuf + roffset, 0, segsizes[relement], datatype, rpeer, schedule );
			NBC_Sched_barrier( schedule );
			round++;
		} while ( round < 2 * p - 2 );
	}

	return NBC_OK;
}

/***************** *
 * NBC_IALLTOALL.C *
 * *****************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

static inline int ___a2a_sched_linear( int rank, int p, MPI_Aint sndext, MPI_Aint rcvext, NBC_Schedule *schedule, const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm );
static inline int ___a2a_sched_pairwise( int rank, int p, MPI_Aint sndext, MPI_Aint rcvext, NBC_Schedule *schedule, const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm );
static inline int ___a2a_sched_diss( int rank, int p, MPI_Aint sndext, MPI_Aint rcvext, NBC_Schedule *schedule, const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle );

/* simple linear MPI_Ialltoall the (simple) algorithm just sends to all nodes */
int NBC_Ialltoall( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, a2asize, sndsize, datasize;
	NBC_Schedule *schedule;
	MPI_Aint rcvext, sndext;
	char *rbuf, *sbuf, inplace;
	enum
	{
		NBC_A2A_LINEAR,
		NBC_A2A_PAIRWISE,
		NBC_A2A_DISS
	} alg;

	if ( !handle->is_persistent )
	{
		NBC_IN_PLACE( sendbuf, recvbuf, inplace );

		res = NBC_Init_handle( handle, comm, MPC_IALLTOALL_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}

		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}

		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( recvtype, &rcvext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_size( sendtype, &sndsize );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_size() (%i)\n", res );
			return res;
		}

		/* algorithm selection */
		a2asize = sndsize * sendcount * p;
		/* this number is optimized for TCP on odin.cs.indiana.edu */
		if ( ( p <= 8 ) && ( ( a2asize < 1 << 17 ) || ( sndsize * sendcount < 1 << 12 ) ) )
		{
			/* just send as fast as we can if we have less than 8 peers, if the
			 * total communicated size is smaller than 1<<17 *and* if we don't
			 * have eager messages (msgsize < 1<<13) */
			alg = /*NBC_A2A_LINEAR;*/ NBC_A2A_PAIRWISE;
		}
		else if ( a2asize < ( 1 << 12 ) * p )
		{

			alg = NBC_A2A_DISS;
		}
		else
		{
			alg = NBC_A2A_PAIRWISE;
		}
		if ( !inplace )
		{
			/* copy my data to receive buffer */
			rbuf = ( (char *) recvbuf ) + ( rank * recvcount * rcvext );
			sbuf = ( (char *) sendbuf ) + ( rank * sendcount * sndext );
			res = NBC_Copy( sbuf, sendcount, sendtype, rbuf, recvcount, recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		/* allocate temp buffer if we need one */
		if ( alg == NBC_A2A_DISS )
		{
			/* only A2A_DISS needs buffers */
			if ( NBC_Type_intrinsic( sendtype ) )
			{
				datasize = sndext * sendcount;
			}
			else
			{
				res = PMPI_Pack_size( sendcount, sendtype, comm, &datasize );
				if ( MPI_SUCCESS != res )
				{
					printf( "MPI Error in MPI_Pack_size() (%i)\n", res );
					return res;
				}
			}
			/* allocate temporary buffers */
			if ( p % 2 == 0 )
			{
				handle->tmpbuf = sctk_malloc( datasize * p * 2 );
			}
			else
			{
				/* we cannot divide p by two, so alloc more to be safe ... */
				handle->tmpbuf = sctk_malloc( datasize * ( p / 2 + 1 ) * 2 * 2 );
			}

			/* phase 1 - rotate n data blocks upwards into the tmpbuffer */
			if ( NBC_Type_intrinsic( sendtype ) )
			{
				/* contiguous - just copy (1st copy) */
				memcpy( handle->tmpbuf, (char *) sendbuf + datasize * rank, datasize * ( p - rank ) );
				if ( rank != 0 )
					memcpy( (char *) handle->tmpbuf + datasize * ( p - rank ), sendbuf, (long)datasize * ( rank ) );
			}
			else
			{
				int pos = 0;

				/* non-contiguous - pack */
				res = PMPI_Pack( (char *) sendbuf + (MPI_Aint)rank * sendcount * sndext, ( p - rank ) * sendcount, sendtype, handle->tmpbuf, ( p - rank ) * datasize, &pos, comm );
				if ( MPI_SUCCESS != res )
				{
					printf( "MPI Error in MPI_Pack() (%i)\n", res );
					return res;
				}
				if ( rank != 0 )
				{
					pos = 0;
					PMPI_Pack( sendbuf, rank * sendcount, sendtype, (char *) handle->tmpbuf + (ptrdiff_t)datasize * ( p - rank ), rank * datasize, &pos, comm );
					if ( MPI_SUCCESS != res )
					{
						printf( "MPI Error in MPI_Pack() (%i)\n", res );
						return res;
					}
				}
			}
		}
		else
		{
			handle->tmpbuf = NULL;
		}

		/* not found - generate new schedule */
		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		res = NBC_Sched_create( schedule );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Sched_create (%i)\n", res );
			return res;
		}

		int alloc_size = 0;

		switch ( alg )
		{
			case NBC_A2A_LINEAR:
				alloc_size = (int)(sizeof( int ) + sizeof( int ) +
							 ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
										   sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							 sizeof( char ));
				*schedule = sctk_malloc( alloc_size );
				*(int *) *schedule = alloc_size;
				res = ___a2a_sched_linear( rank, p, sndext, rcvext, schedule, sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
				if ( NBC_OK != res )
				{
					return res;
				}
				res = NBC_Sched_commit_pos( schedule );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_commit() (%i)\n", res );
					return res;
				}
				break;
			case NBC_A2A_DISS:
				res = NBC_Sched_create( schedule );
				if ( res != NBC_OK )
				{
					printf( "Error in NBC_Sched_create (%i)\n", res );
					return res;
				}
				res = ___a2a_sched_diss( rank, p, sndext, rcvext, schedule, sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, handle );
				if ( NBC_OK != res )
				{
					return res;
				}
				res = NBC_Sched_commit( schedule );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_commit() (%i)\n", res );
					return res;
				}
				break;
			case NBC_A2A_PAIRWISE:
				res = NBC_Sched_create( schedule );
				if ( res != NBC_OK )
				{
					printf( "Error in NBC_Sched_create (%i)\n", res );
					return res;
				}
				res = ___a2a_sched_pairwise( rank, p, sndext, rcvext, schedule, sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
				break;
		}

		if ( NBC_OK != res )
		{
			return res;
		}
		res = NBC_Sched_commit( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		return MPI_SUCCESS;
	}
	
        NBC_IN_PLACE( sendbuf, recvbuf, inplace );

    res = _mpc_cl_comm_rank( comm, &rank );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
        return res;
    }

    res = PMPI_Type_extent( sendtype, &sndext );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
        return res;
    }
    res = PMPI_Type_extent( recvtype, &rcvext );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
        return res;
    }
    if ( !inplace )
    {
        /* copy my data to receive buffer */
        rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)rank * recvcount * rcvext );
        sbuf = ( (char *) sendbuf ) + ( (MPI_Aint)rank * sendcount * sndext );
        res = NBC_Copy( sbuf, sendcount, sendtype, rbuf, recvcount, recvtype, comm );
        if ( NBC_OK != res )
        {
            printf( "Error in NBC_Copy() (%i)\n", res );
            return res;
        }
    }
    handle->row_offset = sizeof( int ); /* have to reinit the offset */
    res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
    if ( NBC_OK != res )
    {
        printf( "Error in NBC_Start() (%i)\n", res );
        return res;
    }

		return MPI_SUCCESS;

}

/** \brief Initialize NBC structures used in call of persistent Alltoall using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcount Number of elements in sendbuf
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcount Number of elements in recvbuf
 *  \param recvtype Type of the data elements in recvbuf
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ialltoall_init( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int rank = 0;
	int p = 0;
	int res = 0;
	int sndsize = 0;
	NBC_Schedule *schedule = NULL;
	MPI_Aint rcvext = 0;
	MPI_Aint sndext = 0;
	enum
	{
		NBC_A2A_LINEAR,
		NBC_A2A_PAIRWISE,
		NBC_A2A_DISS
	} alg;
	res = NBC_Init_handle( handle, comm, MPC_IALLTOALL_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( sendtype, &sndext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_size( sendtype, &sndsize );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_size() (%i)\n", res );
		return res;
	}

	/* algorithm selection */
	alg = /*NBC_A2A_LINEAR;*/ NBC_A2A_PAIRWISE;

	/* not found - generate new schedule */
	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	switch ( alg )
	{
		case NBC_A2A_PAIRWISE:
			res = NBC_Sched_create( schedule );
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}
			res = ___a2a_sched_pairwise( rank, p, sndext, rcvext, schedule, sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
			if ( NBC_OK != res )
			{
				return res;
			}
			res = NBC_Sched_commit( schedule );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_commit() (%i)\n", res );
				return res;
			}
			break;
		case NBC_A2A_LINEAR:
		case NBC_A2A_DISS:
			/* not implemented */
			break;
	}
	handle->schedule = schedule;

	return MPI_SUCCESS;
}

static inline int ___a2a_sched_pairwise( int rank, int p, MPI_Aint sndext, MPI_Aint rcvext, NBC_Schedule *schedule, const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm )
{
	int res = 0;
	int r = 0;
	int sndpeer = 0;
	int rcvpeer = 0;
	char *rbuf = NULL;
	char *sbuf = NULL;

	res = NBC_OK;
	if ( p < 2 )
		return res;

	for ( r = 1; r < p; r++ )
	{

		sndpeer = ( rank + r ) % p;
		rcvpeer = ( rank - r + p ) % p;

		rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)rcvpeer * recvcount * rcvext );
		res = NBC_Sched_recv( rbuf, 0, recvcount, recvtype, rcvpeer, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
		sbuf = ( (char *) sendbuf ) + ( (MPI_Aint)sndpeer * sendcount * sndext );
		res = NBC_Sched_send( sbuf, 0, sendcount, sendtype, sndpeer, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}

		if ( r < p )
		{
			res = NBC_Sched_barrier( schedule );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_barr() (%i)\n", res );
				return res;
			}
		}
	}

	return res;
}

static inline int ___a2a_sched_linear( int rank, int p, MPI_Aint sndext, MPI_Aint rcvext, NBC_Schedule *schedule, const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm )
{
	int res = 0;
	int r = 0;
	char *rbuf = NULL;
	char *sbuf = NULL;

	res = NBC_OK;

	int pos = sizeof( int );
	*(int *) ( (char *) *schedule + sizeof( int ) ) = ( p - 1 ) * 2;
	for ( r = 0; r < p; r++ )
	{
		/* easy algorithm */
		if ( r == rank )
		{
			continue;
		}
		rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)r * recvcount * rcvext );
		res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcount, recvtype, r, schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
		sbuf = ( (char *) sendbuf ) + ( (MPI_Aint)r * sendcount * sndext );
		res = NBC_Sched_send_pos( pos, sbuf, 0, sendcount, sendtype, r, schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}
	}

	return res;
}

static inline int ___a2a_sched_diss( int rank, int p, MPI_Aint sndext, MPI_Aint rcvext, NBC_Schedule *schedule, __UNUSED__ const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{
	int res = 0;
	int i = 0;
	int r = 0;
	int speer = 0;
	int rpeer = 0;
	int datasize = 0;
	int offset = 0;
	int virtp = 0;
	char *rbuf = NULL;
	char *rtmpbuf = NULL;
	char *stmpbuf = NULL;

	res = NBC_OK;
	if ( p < 2 )
		return res;

	if ( NBC_Type_intrinsic( sendtype ) )
	{
		datasize = (int)(sndext * sendcount);
	}
	else
	{
		res = PMPI_Pack_size( sendcount, sendtype, comm, &datasize );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Pack_size() (%i)\n", res );
			return res;
		}
	}

	/* allocate temporary buffers */
	if ( p % 2 == 0 )
	{
		rtmpbuf = (char *) handle->tmpbuf + (ptrdiff_t)datasize * p;
		stmpbuf = (char *) handle->tmpbuf + (ptrdiff_t)datasize * ( p + p / 2 );
	}
	else
	{
		/* we cannot divide p by two, so alloc more to be safe ... */
		virtp = ( p / 2 + 1 ) * 2;
		rtmpbuf = (char *) handle->tmpbuf + (ptrdiff_t)datasize * p;
		stmpbuf = (char *) handle->tmpbuf + (ptrdiff_t)datasize * ( p + virtp / 2 );
	}

	/* phase 2 - communicate */
	for ( r = 1; r < p; r <<= 1 )
	{
		offset = 0;
		for ( i = 1; i < p; i++ )
		{
			/* test if bit r is set in rank number i */
			if ( ( i & r ) == r )
			{
				/* copy data to sendbuffer (2nd copy) - could be avoided using iovecs */
				NBC_Sched_copy( (void *) (long) ( i * datasize ), 1, datasize, MPI_BYTE, stmpbuf + offset - (unsigned long) handle->tmpbuf, 1, datasize, MPI_BYTE, schedule );
				offset += datasize;
			}
		}

		speer = ( rank + r ) % p;
		/* add p because modulo does not work with negative values */
		rpeer = ( ( rank - r ) + p ) % p;

		res = NBC_Sched_recv( rtmpbuf - (unsigned long) handle->tmpbuf, 1, offset, MPI_BYTE, rpeer, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}

		res = NBC_Sched_send( stmpbuf - (unsigned long) handle->tmpbuf, 1, offset, MPI_BYTE, speer, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}

		res = NBC_Sched_barrier( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}

		/* unpack from buffer */
		offset = 0;
		for ( i = 1; i < p; i++ )
		{
			/* test if bit r is set in rank number i */
			if ( ( i & r ) == r )
			{
				/* copy data to tmpbuffer (3rd copy) - could be avoided using iovecs */
				NBC_Sched_copy( rtmpbuf + offset - (unsigned long) handle->tmpbuf, 1, datasize, MPI_BYTE, (void *) (long) ( i * datasize ), 1, datasize, MPI_BYTE, schedule );
				offset += datasize;
			}
		}
	}

	/* phase 3 - reorder - data is now in wrong order in handle->tmpbuf -
	 * reorder it into recvbuf */
	for ( i = 0; i < p; i++ )
	{
		rbuf = (char *) recvbuf + ( (MPI_Aint)( rank - i + p ) % p ) * recvcount * rcvext;
		res = NBC_Sched_unpack( (void *) (long) ( i * datasize ), 1, recvcount, recvtype, rbuf, 0, schedule );
		if ( NBC_OK != res )
		{
			printf( "MPI Error in NBC_Sched_pack() (%i)\n", res );
			return res;
		}
	}

	return NBC_OK;
}

/****************** *
 * NBC_IALLTOALLV.C *
 * ******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* an alltoallv schedule can not be cached easily because the contents
 * ot the recvcounts array may change, so a comparison of the address
 * would not be sufficient ... we simply do not cache it */

/* simple linear Alltoallv */
int NBC_Ialltoallv( const void *sendbuf, const int *sendcounts, const int *sdispls,
					MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls,
					MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{

	int rank, p, res;
	MPI_Aint sndext, rcvext;
	NBC_Schedule *schedule;
	char *rbuf, *sbuf, inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_IALLTOALLV_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( recvtype, &rcvext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		int alloc_size = (int)(sizeof( int ) +
						 ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
									   sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) + sizeof( int ) ));

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;

		// res = NBC_Sched_create(schedule);
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Sched_create (%i)\n", res );
			return res;
		}

		/* copy data to receivbuffer */
		if ( ( sendcounts[rank] != 0 ) && !inplace )
		{
			rbuf = ( (char *) recvbuf ) + ( rdispls[rank] * rcvext );
			sbuf = ( (char *) sendbuf ) + ( sdispls[rank] * sndext );
			res = NBC_Copy( sbuf, sendcounts[rank], sendtype, rbuf,
							recvcounts[rank], recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		// for (i = 0; i < p; i++)
		//{
		//     if (i == rank)
		//     {
		//         continue;
		//     }
		//     /* post all sends */
		//     if (sendcounts[i] != 0)
		//     {
		//         sbuf = ((char *)sendbuf) + (sdispls[i] * sndext);
		//         res = NBC_Sched_send(sbuf, 0, sendcounts[i], sendtype, i, schedule);
		//         if (NBC_OK != res)
		//         {
		//             printf("Error in NBC_Sched_send() (%i)\n", res);
		//             return res;
		//         }
		//     }
		//     /* post all receives */
		//     if (recvcounts[i] != 0)
		//     {
		//         rbuf = ((char *)recvbuf) + (rdispls[i] * rcvext);
		//         res = NBC_Sched_recv(rbuf, 0, recvcounts[i], recvtype, i, schedule);
		//         if (NBC_OK != res)
		//         {
		//             printf("Error in NBC_Sched_recv() (%i)\n", res);
		//             return res;
		//         }
		//     }
		// }
		int r = 0;
		int sndpeer = 0;
		int rcvpeer = 0;
		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		for ( r = 1; r < p; r++ )
		{

			sndpeer = ( rank + r ) % p;
			rcvpeer = ( rank - r + p ) % p;

			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2;
			// rbuf = ((char *) recvbuf) + (rdispls[rcvpeer]);
			rbuf = ( (char *) recvbuf ) + ( rdispls[rcvpeer] * rcvext );
			res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rcvpeer], recvtype, rcvpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			sbuf = ( (char *) sendbuf ) + ( sdispls[sndpeer] * sndext );
			res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[sndpeer], sendtype, sndpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			// sbuf = ((char *) sendbuf) + (sdispls[sndpeer]);
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}

			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			// res = NBC_Sched_barrier(schedule);
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_barr() (%i)\n", res );
				return res;
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	
        res = _mpc_cl_comm_rank( comm, &rank );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
        return res;
    }
    res = _mpc_cl_comm_size( comm, &p );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
        return res;
    }
    res = PMPI_Type_extent( sendtype, &sndext );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
        return res;
    }
    res = PMPI_Type_extent( recvtype, &rcvext );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
        return res;
    }

    /* copy data to receivbuffer */
    if ( ( sendcounts[rank] != 0 ) && !inplace )
    {
        rbuf = ( (char *) recvbuf ) + ( rdispls[rank] * rcvext );
        sbuf = ( (char *) sendbuf ) + ( sdispls[rank] * sndext );
        res = NBC_Copy( sbuf, sendcounts[rank], sendtype, rbuf,
                        recvcounts[rank], recvtype, comm );
        if ( NBC_OK != res )
        {
            printf( "Error in NBC_Copy() (%i)\n", res );
            return res;
        }
    }

    handle->row_offset = sizeof( int ); /* have to reinit the offset */
    res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
    if ( NBC_OK != res )
    {
        printf( "Error in NBC_Start() (%i)\n", res );
        return res;
    }


	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Alltoallv using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcounts Array (of length group size) containing the number of elements send to each process
 *  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcounts Array (of length group size) containing the number of elements received from each process
 *  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
 *  \param recvtype Type of the data elements in recvbuf
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ialltoallv_init( const void *sendbuf, const int *sendcounts, const int *sdispls,
						 MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls,
						 MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle )
{

	int rank, p, res;
	MPI_Aint sndext, rcvext;
	NBC_Schedule *schedule;
	char *rbuf, *sbuf;

	res = NBC_Init_handle( handle, comm, MPC_IALLTOALLV_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( sendtype, &sndext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	res = NBC_Sched_create( schedule );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Sched_create (%i)\n", res );
		return res;
	}

	// for (i = 0; i < p; i++)
	//{
	//   if (i == rank)
	//   {
	//     continue;
	//   }
	//   /* post all sends */
	//   if (sendcounts[i] != 0)
	//   {
	//     sbuf = ((char *)sendbuf) + (sdispls[i] * sndext);
	//     res = NBC_Sched_send(sbuf, 0, sendcounts[i], sendtype, i, schedule);
	//     if (NBC_OK != res)
	//     {
	//       printf("Error in NBC_Sched_send() (%i)\n", res);
	//       return res;
	//     }
	//   }
	//   /* post all receives */
	//   if (recvcounts[i] != 0)
	//   {
	//     rbuf = ((char *)recvbuf) + (rdispls[i] * rcvext);
	//     res = NBC_Sched_recv(rbuf, 0, recvcounts[i], recvtype, i, schedule);
	//     if (NBC_OK != res)
	//     {
	//       printf("Error in NBC_Sched_recv() (%i)\n", res);
	//       return res;
	//     }
	//   }
	// }
	//
	int r;
	int sndpeer, rcvpeer;
	for ( r = 1; r < p; r++ )
	{

		sndpeer = ( rank + r ) % p;
		rcvpeer = ( rank - r + p ) % p;

		// rbuf = ((char *) recvbuf) + (rdispls[rcvpeer]);
		rbuf = ( (char *) recvbuf ) + ( rdispls[rcvpeer] * rcvext );
		// res = NBC_Sched_recv(rbuf, 0, recvcounts[rcvpeer], recvtypes[rcvpeer], rcvpeer, schedule);
		res = NBC_Sched_recv( rbuf, 0, recvcounts[rcvpeer], recvtype, rcvpeer, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
		sbuf = ( (char *) sendbuf ) + ( sdispls[sndpeer] * sndext );
		res = NBC_Sched_send( sbuf, 0, sendcounts[sndpeer], sendtype, sndpeer, schedule );
		// sbuf = ((char *) sendbuf) + (sdispls[sndpeer]);
		// res = NBC_Sched_send(sbuf, 0, sendcounts[sndpeer], sendtypes[sndpeer], sndpeer, schedule);
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}

		res = NBC_Sched_barrier( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_barr() (%i)\n", res );
			return res;
		}
	}

	res = NBC_Sched_commit( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/******************* *
 * NBC_IBARRIER.C *
 * *******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* Dissemination implementation of MPI_Ibarrier */
int NBC_Ibarrier( MPI_Comm comm, NBC_Handle *handle )
{

	int round, rank, p, maxround, res, recvpeer, sendpeer;
	NBC_Schedule *schedule;

	res = NBC_Init_handle( handle, comm, MPC_IBARRIER_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = (void *) sctk_malloc( 2 * sizeof( char ) );

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	round = -1;
	//    res = NBC_Sched_create(schedule);
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Sched_create (%i)\n", res );
		return res;
	}

	maxround = (int) ceil( ( log( p ) / LOG2 ) - 1 );

	int alloc_size = (int)(
		sizeof( int ) + sizeof( int ) +
		( maxround + 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							 sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
		maxround * ( sizeof( char ) + sizeof( int ) ) + sizeof( char ));
	*schedule = sctk_malloc( alloc_size );

	*(int *) *schedule = alloc_size;
	*( ( (int *) *schedule ) + 1 ) = 2;
	//    int i;
	//    for(i=0; i<maxround+1; i++)
	//   {
	//        *(((int*)*schedule)+1)=(p-1)*2;
	//    }

	int pos = sizeof( int );
	int pos_rounds = 0;
	do
	{
		round++;
		sendpeer = ( rank + ( 1 << round ) ) % p;
		/* add p because modulo does not work with
		 * negative values */
		recvpeer = ( ( rank - ( 1 << round ) ) + p ) % p;

		/* send msg to sendpeer */
		res =
			NBC_Sched_send_pos( pos, (void *) 0, 1, 1, MPI_BYTE, sendpeer, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		if ( NBC_OK != res )
		{
			printf( "Error	in NBC_Sched_send() (%i)\n", res );
			return res;
		}

		/* recv msg from recvpeer */
		res =
			NBC_Sched_recv_pos( pos, (void *) 1, 1, 1, MPI_BYTE, recvpeer, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) += 1;
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}

		/* end communication round */
		if ( round < maxround )
		{
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
		}
	} while ( round < maxround );

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	res = NBC_Start( handle, schedule );

	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Start() (%i)\n", res );
		return res;
	}

	return MPI_SUCCESS;
}

/************** *
 * NBC_IBCAST.C *
 * **************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

static inline int ___bcast_sched_binomial( int rank, int p, int root, NBC_Schedule *schedule, void *buffer, int count, MPI_Datatype datatype );
static inline int ___bcast_sched_linear( int rank, int p, int root, NBC_Schedule *schedule, void *buffer, int count, MPI_Datatype datatype );

int NBC_Ibcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, size;
	NBC_Schedule *schedule;
	int alloc_size = 0;
	enum
	{
		NBC_BCAST_LINEAR,
		NBC_BCAST_BINOMIAL
	} alg;

	res = _mpc_cl_comm_rank( comm, &rank );
	res = _mpc_cl_comm_size( comm, &p );
	res = PMPI_Type_size( datatype, &size );
	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_IBCAST_TAG );

		/* algorithm selection */

		if ( p > 4 )
		{
			alg = NBC_BCAST_BINOMIAL;
		}
		else
		{
			alg = NBC_BCAST_LINEAR;
		}

		handle->tmpbuf = NULL;

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );

		switch ( alg )
		{
			case NBC_BCAST_LINEAR:
				if ( rank == root )
				{
					alloc_size =
						sizeof( int ) + sizeof( int ) +
						( ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) ) +
						sizeof( char );
					*schedule = sctk_malloc( alloc_size );
					if ( *schedule == NULL )
					{
						return NBC_OOR;
					}
					*(int *) *schedule = alloc_size;
					*( ( (int *) *schedule ) + 1 ) = p - 1;
				}
				else
				{
					alloc_size = sizeof( int ) + sizeof( int ) +
								 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
								 sizeof( char );
					*schedule = sctk_malloc( alloc_size );
					if ( *schedule == NULL )
					{
						return NBC_OOR;
					}
					*(int *) *schedule = alloc_size;
					*( ( (int *) *schedule ) + 1 ) = 1;
				}
				res = ___bcast_sched_linear( rank, p, root, schedule, buffer, count, datatype );
				break;

			case NBC_BCAST_BINOMIAL:;
				int vrank;

				RANK2VRANK( rank, vrank, root );
				alloc_size = sizeof( int );
				int sends;
				if ( vrank == 0 )
				{
					sends = (int) ceil( ( log( p ) / LOG2 ) );
				}
				else
				{
					int r = 0;
					sends = 0;
					int maxr = (int) ceil( ( log( p ) / LOG2 ) );
					while ( !( vrank & ( 1 << r ) ) && ( r < maxr ) )
					{
						if ( ( vrank + ( 1 << r ) < p ) )
						{
							sends++;
						}
						r++;
					}
					alloc_size += sizeof( int ) +
								  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
								  sizeof( char );
				}
				alloc_size += (int)(sizeof( int ) +
							  sends * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							  sizeof( char ));

				*schedule = sctk_malloc( alloc_size );
				if ( *schedule == NULL )
				{
					return NBC_OOR;
				}
				*(int *) *schedule = alloc_size;

				res =
					___bcast_sched_binomial( rank, p, root, schedule, buffer, count, datatype );
				break;
		}
		if ( NBC_OK != res )
		{
			printf( "Error in Schedule creation() (%i)\n", res );
			return res;
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	
        if ( p > 4 )
    {
        alg = NBC_BCAST_BINOMIAL;
    }
    else
    {
        alg = NBC_BCAST_LINEAR;
    }

    switch ( alg )
    {
        case NBC_BCAST_BINOMIAL:
        case NBC_BCAST_LINEAR:
            /* nothing to do for persistent */
            break;
    }

    handle->row_offset = sizeof( int ); /* have to reinit the offset */
    res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
    if ( NBC_OK != res )
    {
        printf( "Error in NBC_Start() (%i)\n", res );
        return res;
    }


	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Bcast using NBC interface
 *  \param buffer Address of the pointer to the buffer
 *  \param count Number of elements in buffer
 *  \param datatype Type of the data elements in sendbuf
 *  \param root Rank root of the broadcast
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ibcast_init( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, size;
	NBC_Schedule *schedule;
	int alloc_size = 0;
	enum
	{
		NBC_BCAST_LINEAR,
		NBC_BCAST_BINOMIAL
    } alg;

	res = NBC_Init_handle( handle, comm, MPC_IBCAST_TAG );
	res = _mpc_cl_comm_rank( comm, &rank );
	res = _mpc_cl_comm_size( comm, &p );
	res = PMPI_Type_size( datatype, &size );

	/* algorithm selection */

	if ( p > 4 )
	{
		alg = NBC_BCAST_BINOMIAL;
	}
	else
	{
		alg = NBC_BCAST_LINEAR;
	}

	handle->tmpbuf = NULL;

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );

	switch ( alg )
	{
		case NBC_BCAST_LINEAR:
			if ( rank == root )
			{
				alloc_size = (int)(
					sizeof( int ) + sizeof( int ) +
					( ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) ) +
					sizeof( char ));
				*schedule = sctk_malloc( alloc_size );
				if ( *schedule == NULL )
				{
					return NBC_OOR;
				}
				*(int *) *schedule = alloc_size;
				*( ( (int *) *schedule ) + 1 ) = p - 1;
			}
			else
			{
				alloc_size = (int)(sizeof( int ) + sizeof( int ) +
							 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							 sizeof( char ));
				*schedule = sctk_malloc( alloc_size );
				if ( *schedule == NULL )
				{
					return NBC_OOR;
				}
				*(int *) *schedule = alloc_size;
				*( ( (int *) *schedule ) + 1 ) = 1;
			}
			res = ___bcast_sched_linear( rank, p, root, schedule, buffer, count, datatype );
			break;

		case NBC_BCAST_BINOMIAL:;
			int vrank;

			RANK2VRANK( rank, vrank, root );
			alloc_size = sizeof( int );
			int sends;
			if ( vrank == 0 )
			{
				sends = (int) ceil( ( log( p ) / LOG2 ) );
			}
			else
			{
				int r = 0;
				sends = 0;
				int maxr = (int) ceil( ( log( p ) / LOG2 ) );
				while ( !( vrank & ( 1 << r ) ) && ( r < maxr ) )
				{
					if ( ( vrank + ( 1 << r ) < p ) )
					{
						sends++;
					}
					r++;
				}
				alloc_size += sizeof( int ) +
							  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							  sizeof( char );
			}
			alloc_size += (int)(sizeof( int ) +
						  sends * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  sizeof( char ));

			*schedule = sctk_malloc( alloc_size );
			if ( *schedule == NULL )
			{
				return NBC_OOR;
			}
			*(int *) *schedule = alloc_size;

			res =
				___bcast_sched_binomial( rank, p, root, schedule, buffer, count, datatype );
			break;
	}
	if ( NBC_OK != res )
	{
		printf( "Error in Schedule creation() (%i)\n", res );
		return res;
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/* better binomial bcast
 * working principle:
 * - each node gets a virtual rank vrank
 * - the 'root' node get vrank 0
 * - node 0 gets the vrank of the 'root'
 * - all other ranks stay identical (they do not matter)
 *
 * Algorithm:
 * - each node with vrank > 2^r and vrank < 2^r+1 receives from node
 *	 vrank - 2^r (vrank=1 receives from 0, vrank 0 receives never)
 * - each node sends each round r to node vrank + 2^r
 * - a node stops to send if 2^r > commsize
 */

static inline int ___bcast_sched_binomial( int rank, int p, int root, NBC_Schedule *schedule, void *buffer, int count, MPI_Datatype datatype )
{
	int maxr, vrank, peer, r, res;

	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	RANK2VRANK( rank, vrank, root );
	int pos = 0;
	int pos_rounds = pos;
	if ( vrank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		pos += sizeof( int );
		r = 0;
		for ( r = 0; r < maxr; r++ )
		{
			if ( ( vrank & ( 1 << r ) ) && ( r < maxr ) )
			{
				VRANK2RANK( peer, vrank - ( 1 << r ), root );
				res = NBC_Sched_recv_pos( pos, buffer, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				break;
			}
		}
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
	}

	int cpt = 0;
	pos_rounds = pos;
	pos += sizeof( int );
	r = 0;
	while ( !( vrank & ( 1 << r ) ) && ( r < maxr ) )
	{
		if ( ( vrank + ( 1 << r ) < p ) )
		{
			cpt++;
			VRANK2RANK( peer, vrank + ( 1 << r ), root );
			res = NBC_Sched_send_pos( pos, buffer, 0, count, datatype, peer,
									  schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
		r++;
	}
	*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = cpt;

	return NBC_OK;
}

/* simple linear MPI_Ibcast */
static inline int ___bcast_sched_linear( int rank, int p, int root, NBC_Schedule *schedule, void *buffer, int count, MPI_Datatype datatype )
{
	int peer, res;

	int pos = sizeof( int );
	/* send to all others */
	if ( rank == root )
	{
		for ( peer = 0; peer < p; peer++ )
		{
			if ( peer != root )
			{
				/* send msg to peer */
				res =
					NBC_Sched_send_pos( pos, buffer, 0, count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
			}
		}
	}
	else
	{
		/* recv msg from root */
		res = NBC_Sched_recv_pos( pos, buffer, 0, count, datatype, root, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
	}

	return NBC_OK;
}


/*************** *
 * NBC_IGATHER.C *
 * ***************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

static int ___power2( int p );
static int ___power2( int p )
{
	if ( p == 0 )
		return 1;
	int i = 0;
	int r = 1;
	for ( i = 0; i < p; i++ )
	{
		r *= 2;
	}

	return r;
}

static inline int ___binomial_gather( int rank, int p, int root, void *recvbuf, int sendcount, int recvcount, MPI_Datatype sendtype, MPI_Datatype recvtype, NBC_Schedule *schedule, NBC_Handle *handle );

static inline int ___binomial_gather( int rank, int p, int root, void *recvbuf, int sendcount, int recvcount, MPI_Datatype sendtype, MPI_Datatype recvtype, NBC_Schedule *schedule, NBC_Handle *handle )
{
	int vrank, vpeer, res, maxr, r;
	MPI_Aint rcvext;
	res = PMPI_Type_extent( recvtype, &rcvext );
	RANK2VRANK( rank, vrank, root );
	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	char *recv_temp = NULL;
	if ( ( vrank != 0 ) )
	{
		recv_temp = (char *) handle->tmpbuf_gather;
	}

	int pos = 0;
	int pos_rounds = pos;
	pos += sizeof( int );
	int nb_send = 0;
	for ( r = 1; r <= maxr; r++ )
	{
		if ( ( vrank % ( 1 << r ) ) == 0 )
		{
			/* we have to receive this round */
			vpeer = vrank + ( 1 << ( r - 1 ) );
			if ( vpeer < p )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				int nb_recv = ___power2( r - 1 );
				if ( vrank != 0 )
				{
					nb_send++;
					res = NBC_Sched_recv_pos( pos, recv_temp + rcvext * ( vpeer - vrank ) * recvcount, 0, recvcount * nb_recv, recvtype, vpeer, schedule );
					// res = NBC_Sched_recv_pos(pos, recv_temp, 0,recvcount*nb_recv, recvtype, vpeer, schedule);
				}
				else
				{
					if ( vpeer == ___power2( maxr - 1 ) )
					{
						nb_recv = ( p - vpeer );
					}
					res = NBC_Sched_recv_pos( pos, ( (char *) recvbuf ) + rcvext * vpeer * recvcount, 0, recvcount * nb_recv, recvtype, vpeer, schedule );
					// res = NBC_Sched_recv_pos(pos, recvbuf, 0,recvcount*nb_recv, recvtype, vpeer, schedule);
				}
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf_gather );
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				/* we have to wait until we have the data */

				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
			}
		}
		else
		{
			/* we have to send this round */
			vpeer = vrank - ( 1 << ( r - 1 ) );
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			int nb_send_p2 = ___power2( nb_send );
			// if(vrank == (p - 1)){
			//   nb_send_p2 = (p - vrank);
			// }
			res = NBC_Sched_send_pos( pos,
									  recv_temp,
									  0, nb_send_p2 * sendcount, sendtype, vpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}

			break;
		}
	}

	return NBC_OK;
}

int NBC_Igather( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	if ( !handle->is_persistent )
	{
		int rank, p, res, i;
		MPI_Aint rcvext;
		NBC_Schedule *schedule;
		int alloc_size = 0;
		int inplace;
		char *rbuf = NULL;

		NBC_IN_PLACE( sendbuf, recvbuf, inplace );

		res = NBC_Init_handle( handle, comm, MPC_IGATHER_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( recvtype, &rcvext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		if ( ( rank == root ) && ( !inplace ) )
		{
			rbuf = ( (char *) recvbuf ) + ( rank * recvcount * rcvext );
			/* if I am the root - just copy the message (only without MPI_IN_PLACE) */
			res = NBC_Copy( sendbuf, sendcount, sendtype, rbuf, recvcount, recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		/* only one node -> copy data */
		if ( ( p == 1 ) && !inplace )
		{
			res = NBC_Copy( sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
							comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		enum
		{
			NBC_BINOMIAL,
			NBC_LINEAR
		} alg;
		if ( p > 4 )
			alg = NBC_BINOMIAL;
		else
			alg = NBC_LINEAR;
		alg = NBC_BINOMIAL;

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}
		switch ( alg )
		{
			case NBC_BINOMIAL:;

				int maxr = (int) ceil( ( log( p ) / LOG2 ) );
				int recv_op_rounds = 0;

				int vrank, vpeer, peer, r;
				RANK2VRANK( rank, vrank, root );
				for ( r = 1; r <= maxr; r++ )
				{
					if ( ( vrank % ( 1 << r ) ) == 0 )
					{
						vpeer = vrank + ( 1 << ( r - 1 ) );
						VRANK2RANK( peer, vpeer, root )
						if ( peer < p )
						{
							recv_op_rounds++;
						}
					}
					else
					{
						break;
					}
				}
				alloc_size = (int)(
					sizeof( int ) +
					recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
									   sizeof( int ) + sizeof( char ) ));
				if ( rank != root )
				{
					alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) + sizeof( char ) + sizeof( int );
				}
				*schedule = sctk_malloc( alloc_size );
				*(int *) *schedule = alloc_size;

				if ( ( vrank != 0 ) )
				{
					int recv_tot_rounds = 0;
					for ( r = 1; r <= recv_op_rounds; r++ )
					{
						recv_tot_rounds += ___power2( r - 1 ); /*total count recv from other ranks in binomial algorithm */
					}
					handle->tmpbuf_gather = (char *) sctk_malloc( rcvext * recvcount * ( recv_tot_rounds + 1 ) );
					if ( NULL == handle->tmpbuf_gather )
					{
						printf( "Error in sctk_malloc() (%i)\n", res );
						return res;
					}

					res = NBC_Copy( sendbuf, sendcount, sendtype, (char *) handle->tmpbuf_gather, recvcount, recvtype, comm );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Copy() (%i)\n", res );
						return res;
					}
				}

				res = ___binomial_gather( rank, p, root, recvbuf, sendcount,
										  recvcount, sendtype, recvtype, schedule, handle );
				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}
				break;
			case NBC_LINEAR:;
				alloc_size = -1;
				int round_size = -1;
				if ( rank == root )
				{
					alloc_size = (int)(
						sizeof( int ) + sizeof( int ) +
						( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						sizeof( char ));
					round_size = p - 1;
				}
				else
				{
					alloc_size = sizeof( int ) + sizeof( int ) +
								 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
								 sizeof( char );
					round_size = 1;
				}

				*schedule = sctk_malloc( alloc_size );
				*(int *) *schedule = alloc_size;
				*( ( (int *) *schedule ) + 1 ) = round_size;

				int pos = sizeof( int );

				/* send to root */
				if ( rank != root )
				{
					/* send msg to root */
					res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype,
											  root, schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_send() (%i)\n", res );
						return res;
					}
				}
				else
				{
					for ( i = 0; i < p; i++ )
					{
						rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)i * recvcount * rcvext );
						if ( i != root )
						{
							/* root receives message to the right buffer */
							res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcount,
													  recvtype, i, schedule );
							pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
							if ( NBC_OK != res )
							{
								printf( "Error in NBC_Sched_recv() "
										"(%i)\n",
										res );
								return res;
							}
						}
					}
				}

				break;
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		return MPI_SUCCESS;
	}


    int inplace = 0;

    NBC_IN_PLACE( sendbuf, recvbuf, inplace );

    int rank = 0;
    int res = 0;
    res = _mpc_cl_comm_rank( comm, &rank );
    if ( ( rank == root ) && ( !inplace ) )
    {
        /* if I am the root - just copy the message (only without MPI_IN_PLACE) */
        res = NBC_Copy( sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
        if ( NBC_OK != res )
        {
            printf( "Error in NBC_Copy() (%i)\n", res );
            return res;
        }
    }

    int p = 0;
    res = _mpc_cl_comm_size( comm, &p );
    if ( MPI_SUCCESS != res )
    {
        printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
        return res;
    }
    enum
    {
        NBC_BINOMIAL,
        NBC_LINEAR
    } alg;
    if ( p > 4 )
        alg = NBC_BINOMIAL;
    else
        alg = NBC_LINEAR;
    alg = NBC_BINOMIAL;

    switch ( alg )
    {
        case NBC_BINOMIAL:;
            res = _mpc_cl_comm_rank( comm, &rank );
            if ( MPI_SUCCESS != res )
            {
                printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
                return res;
            }
            res = _mpc_cl_comm_size( comm, &p );
            if ( MPI_SUCCESS != res )
            {
                printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
                return res;
            }

            int vrank = 0;
            RANK2VRANK( rank, vrank, root );
            if ( ( vrank != root ) /*&& (!inplace)*/ )
            {
                /* if I am the root - just copy the message (only without MPI_IN_PLACE) */
                res = NBC_Copy( sendbuf, sendcount, sendtype, handle->tmpbuf_gather, recvcount, recvtype, comm );
                if ( NBC_OK != res )
                {
                    printf( "Error in NBC_Copy() (%i)\n", res );
                    return res;
                }
            }

            // res = binomial_gather_persistent(rank, p, root,
            //        (NBC_Schedule *)handle->schedule, handle);
            if ( NBC_OK != res )
            {
                printf( "Error in Schedule creation() (%i)\n", res );
                return res;
            }
            break;
        case NBC_LINEAR:;
            /* nothing to do */
            break;
    }

    handle->row_offset = sizeof( int ); /* have to reinit the offset */

    res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
    if ( NBC_OK != res )
    {
        printf( "Error in NBC_Start() (%i)\n", res );
        return res;
    }

	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Gather using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcount Number of elements in sendbuf
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcount Number of elements in recvbuf
 *  \param recvtype Type of the data elements in recvbuf
 *  \param root Rank root of the gather
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Igather_init( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
					  int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint rcvext;
	NBC_Schedule *schedule;
	int alloc_size = 0;
	char *rbuf = NULL;

	res = NBC_Init_handle( handle, comm, MPC_IGATHER_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	enum
	{
		NBC_BINOMIAL,
		NBC_LINEAR
	} alg;
	if ( p > 4 )
		alg = NBC_BINOMIAL;
	else
		alg = NBC_LINEAR;
	alg = NBC_BINOMIAL;

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}
	switch ( alg )
	{
		case NBC_BINOMIAL:;

			int maxr = (int) ceil( ( log( p ) / LOG2 ) );
			int recv_op_rounds = 0;

			int vrank, vpeer, peer, r;
			RANK2VRANK( rank, vrank, root );
			for ( r = 1; r <= maxr; r++ )
			{
				if ( ( vrank % ( 1 << r ) ) == 0 )
				{
					vpeer = vrank + ( 1 << ( r - 1 ) );
					VRANK2RANK( peer, vpeer, root )
					if ( peer < p )
					{
						recv_op_rounds++;
					}
				}
				else
				{
					break;
				}
			}
			alloc_size = (int)(
				sizeof( int ) +
				recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
								   sizeof( int ) + sizeof( char ) ));
			if ( rank != root )
			{
				alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) + sizeof( char ) + sizeof( int );
			}
			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;

			if ( ( vrank != 0 ) )
			{
				int recv_tot_rounds = 0;
				for ( r = 1; r <= recv_op_rounds; r++ )
				{
					recv_tot_rounds += ___power2( r - 1 ); /*total count recv from other ranks in binomial algorithm */
				}
				handle->tmpbuf_gather = (char *) sctk_malloc( rcvext * recvcount * ( recv_tot_rounds + 1 ) );
				if ( NULL == handle->tmpbuf_gather )
				{
					printf( "Error in sctk_malloc() (%i)\n", res );
					return res;
				}
			}

			res = ___binomial_gather( rank, p, root, recvbuf, sendcount,
									  recvcount, sendtype, recvtype, schedule, handle );
			if ( NBC_OK != res )
			{
				printf( "Error in Schedule creation() (%i)\n", res );
				return res;
			}
			break;
		case NBC_LINEAR:;
			alloc_size = -1;
			int round_size = -1;
			if ( rank == root )
			{
				alloc_size = (int)(
					sizeof( int ) + sizeof( int ) +
					( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					sizeof( char ));
				round_size = p - 1;
			}
			else
			{
				alloc_size = sizeof( int ) + sizeof( int ) +
							 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
							 sizeof( char );
				round_size = 1;
			}

			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;
			*( ( (int *) *schedule ) + 1 ) = round_size;

			int pos = sizeof( int );

			/* send to root */
			if ( rank != root )
			{
				/* send msg to root */
				res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype,
										  root, schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
			}
			else
			{
				for ( i = 0; i < p; i++ )
				{
					rbuf = ( (char *) recvbuf ) + ( (MPI_Aint)i * recvcount * rcvext );
					if ( i != root )
					{
						/* root receives message to the right buffer */
						res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcount,
												  recvtype, i, schedule );
						pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Sched_recv() "
									"(%i)\n",
									res );
							return res;
						}
					}
				}
			}

			break;
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/**************** *
 * NBC_IGATHERV.C *
 * ****************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* an gatherv schedule can not be cached easily because the contents
 * ot the recvcounts array may change, so a comparison of the address
 * would not be sufficient ... we simply do not cache it */

int NBC_Igatherv( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint rcvext;
	NBC_Schedule *schedule;
	char *rbuf, inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );
	if ( !handle->is_persistent )
	{

		res = NBC_Init_handle( handle, comm, MPC_IGATHERV_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( recvtype, &rcvext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		int alloc_size = -1;
		int round_size = -1;
		if ( rank == root )
		{
			alloc_size = (int)(sizeof( int ) + sizeof( int ) +
						 ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						 sizeof( char ));
			round_size = p - 1;
		}
		else
		{
			alloc_size = (int)(sizeof( int ) + sizeof( int ) +
						 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						 sizeof( char ));
			round_size = 1;
		}

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = round_size;

		int pos = sizeof( int );

		/* send to root */
		if ( rank != root )
		{
			/* send msg to root */
			res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype, root,
									  schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
		else
		{
			for ( i = 0; i < p; i++ )
			{
				rbuf = ( (char *) recvbuf ) + ( displs[i] * rcvext );
				if ( i == root )
				{
					if ( !inplace )
					{
						/* if I am the root - just copy the message */
						res = NBC_Copy( sendbuf, sendcount, sendtype, rbuf,
										recvcounts[i], recvtype, comm );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Copy() (%i)\n", res );
							return res;
						}
					}
				}
				else
				{
					/* root receives message to the right buffer */
					res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[i], recvtype, i,
											  schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_recv() (%i)\n", res );
						return res;
					}
				}
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	

	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	/* send to root */
	if ( rank == root )
	{
		for ( i = 0; i < p; i++ )
		{
			rbuf = ( (char *) recvbuf ) + ( displs[i] * rcvext );
			if ( i == root )
			{
				if ( !inplace )
				{
					/* if I am the root - just copy the message */
					res = NBC_Copy( sendbuf, sendcount, sendtype, rbuf,
									recvcounts[i], recvtype, comm );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Copy() (%i)\n", res );
						return res;
					}
				}
			}
		}
	}

	handle->row_offset = sizeof( int ); /* have to reinit the offset */
	res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Start() (%i)\n", res );
		return res;
	}


	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Gatherv using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcount Number of elements in sendbuf
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcounts Array (of length group size) containing the number of elements received from each process
 *  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
 *  \param recvtype Type of the data elements in recvbuf
 *  \param root Rank root of the gatherv
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Igatherv_init( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint rcvext;
	NBC_Schedule *schedule;
	char *rbuf;

	res = NBC_Init_handle( handle, comm, MPC_IGATHERV_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( recvtype, &rcvext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	int alloc_size = -1;
	int round_size = -1;
	if ( rank == root )
	{
		alloc_size = (int)(sizeof( int ) + sizeof( int ) +
					 ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					 sizeof( char ));
		round_size = p - 1;
	}
	else
	{
		alloc_size = (int)(sizeof( int ) + sizeof( int ) +
					 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					 sizeof( char ));
		round_size = 1;
	}

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;
	*( ( (int *) *schedule ) + 1 ) = round_size;

	int pos = sizeof( int );

	/* send to root */
	if ( rank != root )
	{
		/* send msg to root */
		res = NBC_Sched_send_pos( pos, sendbuf, 0, sendcount, sendtype, root,
								  schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}
	}
	else
	{
		for ( i = 0; i < p; i++ )
		{
			rbuf = ( (char *) recvbuf ) + ( displs[i] * rcvext );
			if ( i != root )
			{
				/* root receives message to the right buffer */
				res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[i], recvtype, i,
										  schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
			}
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/******************* *
 * NBC_IREDUCE.C *
 * *******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

static inline int ___red_sched_binomial( int rank, int p, int root, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, void *redbuf, NBC_Schedule *schedule, NBC_Handle *handle );
static inline int ___red_sched_chain( int rank, int p, int root, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Aint ext, int size, NBC_Schedule *schedule, NBC_Handle *handle, int fragsize );

/* the non-blocking reduce */
int NBC_Ireduce( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank = 0;
	int p = 0;
	int res = -1;
	int segsize = 0;
	int size = 0;
	MPI_Aint ext = 0;
	NBC_Schedule *schedule = NULL;
	int alloc_size = 0;
	char *redbuf = NULL;
	char inplace = 0;
	enum
	{
		NBC_RED_BINOMIAL,
		NBC_RED_CHAIN
	} alg;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );
	if ( !handle->is_persistent )
	{

		res = NBC_Init_handle( handle, comm, MPC_IREDUCE_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_size( datatype, &size );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_size() (%i)\n", res );
			return res;
		}

		/* only one node -> copy data */
		if ( ( p == 1 ) && !inplace )
		{
			res = NBC_Copy( sendbuf, count, datatype, recvbuf, count, datatype,
							comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		/* algorithm selection */
		//	if(p > 4 || size*count < 65536)
		//	{
		alg = NBC_RED_BINOMIAL;
		if ( rank == root )
		{
			/* root reduces in receivebuffer */
			handle->tmpbuf = sctk_malloc( ext * count );
		}
		else
		{
			/* recvbuf may not be valid on non-root nodes */
			handle->tmpbuf = sctk_malloc( ext * count * 2 );
			redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );
		}
		//	}
		//	else
		//	{
		//		handle->tmpbuf = sctk_malloc(ext*count);
		//		algint = NBC_RED_CHAIN;
		//		segintsize = 16384/2;
		//	}
		if ( NULL == handle->tmpbuf )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}

		int maxr = (int) ceil( ( log( p ) / LOG2 ) );
		int recv_op_rounds = 0;

		int vrank = 0;
		int vpeer = 0;
		int peer = 0;
		int r = 0;
		RANK2VRANK( rank, vrank, root );

		switch ( alg )
		{
			case NBC_RED_BINOMIAL:

				for ( r = 1; r <= maxr; r++ )
				{
					if ( ( vrank % ( 1 << r ) ) == 0 )
					{
						vpeer = vrank + ( 1 << ( r - 1 ) );
						VRANK2RANK( peer, vpeer, root )
						if ( peer < p )
						{
							recv_op_rounds++;
						}
					}
					else
					{
						break;
					}
				}

				alloc_size = (int)(
					sizeof( int ) +
					recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
									   sizeof( int ) + sizeof( char ) ) +
					recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
									   sizeof( int ) + sizeof( char ) ) +
					sizeof( char ) + sizeof( int ));
				if ( rank != root )
					alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				*schedule = sctk_malloc( alloc_size );
				*(int *) *schedule = alloc_size;

				res = ___red_sched_binomial( rank, p, root, sendbuf, recvbuf, count,
											 datatype, op, redbuf, schedule, handle );
				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}

				res = NBC_Sched_commit_pos( schedule );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_commit() (%i)\n", res );
					return res;
				}
				break;
			case NBC_RED_CHAIN:
				res = NBC_Sched_create( schedule );
				if ( res != NBC_OK )
				{
					printf( "Error in NBC_Sched_create (%i)\n", res );
					return res;
				}

				res =
					___red_sched_chain( rank, p, root, sendbuf, recvbuf, count, datatype,
										op, ext, size, schedule, handle, segsize );
				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}

				res = NBC_Sched_commit( schedule );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_commit() (%i)\n", res );
					return res;
				}
				break;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}

		/* only one node -> copy data */
		if ( ( p == 1 ) && !inplace )
		{
			res = NBC_Copy( sendbuf, count, datatype, recvbuf, count, datatype,
							comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		alg = NBC_RED_BINOMIAL;
		switch ( alg )
		{
			case NBC_RED_BINOMIAL:

				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}

				break;
			case NBC_RED_CHAIN:

				// res =
				//     ___red_sched_chain(rank, p, root, sendbuf, recvbuf, count, datatype,
				//             op, ext, size, handle->schedule, handle, segsize);
				if ( NBC_OK != res )
				{
					printf( "Error in Schedule creation() (%i)\n", res );
					return res;
				}

				break;
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */

		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
	}

	/* tmpbuf is freed with the handle */
	// return NBC_OK;
	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Reduce using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param count Number of elements in sendbuf
 *  \param datatype Type of the data elements in sendbuf
 *  \param op Reduction operation
 *  \param root Rank root of the reduce
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ireduce_init( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, segsize = 0, size;
	MPI_Aint ext;
	NBC_Schedule *schedule;
	int alloc_size = 0;
	char *redbuf = NULL;
	enum
	{
		NBC_RED_BINOMIAL,
		NBC_RED_CHAIN
	} alg;

	res = NBC_Init_handle( handle, comm, MPC_IREDUCE_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_size( datatype, &size );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_size() (%i)\n", res );
		return res;
	}

	/* algorithm selection */
	//	if(p > 4 || size*count < 65536)
	//	{
	alg = NBC_RED_BINOMIAL;
	if ( rank == root )
	{
		/* root reduces in receivebuffer */
		handle->tmpbuf = sctk_malloc( ext * count );
	}
	else
	{
		/* recvbuf may not be valid on non-root nodes */
		handle->tmpbuf = sctk_malloc( ext * count * 2 );
		redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );
	}
	//	}
	//	else
	//	{
	//		handle->tmpbuf = sctk_malloc(ext*count);
	//		algint = NBC_RED_CHAIN;
	//		segintsize = 16384/2;
	//	}
	if ( NULL == handle->tmpbuf )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}

	int maxr = (int) ceil( ( log( p ) / LOG2 ) );
	int recv_op_rounds = 0;

	int vrank, vpeer, peer, r;
	RANK2VRANK( rank, vrank, root );

	switch ( alg )
	{
		case NBC_RED_BINOMIAL:;

			for ( r = 1; r <= maxr; r++ )
			{
				if ( ( vrank % ( 1 << r ) ) == 0 )
				{
					vpeer = vrank + ( 1 << ( r - 1 ) );
					VRANK2RANK( peer, vpeer, root )
					if ( peer < p )
					{
						recv_op_rounds++;
					}
				}
				else
				{
					break;
				}
			}

			alloc_size =
				sizeof( int ) +
				recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
								   sizeof( int ) + sizeof( char ) ) +
				recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
								   sizeof( int ) + sizeof( char ) ) +
				sizeof( char ) + sizeof( int );
			if ( rank != root )
				alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;

			res = ___red_sched_binomial( rank, p, root, sendbuf, recvbuf, count,
										 datatype, op, redbuf, schedule, handle );
			if ( NBC_OK != res )
			{
				printf( "Error in Schedule creation() (%i)\n", res );
				return res;
			}

			res = NBC_Sched_commit_pos( schedule );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_commit() (%i)\n", res );
				return res;
			}
			break;
		case NBC_RED_CHAIN:
			res = NBC_Sched_create( schedule );
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}

			res =
				___red_sched_chain( rank, p, root, sendbuf, recvbuf, count, datatype,
									op, ext, size, schedule, handle, segsize );
			if ( NBC_OK != res )
			{
				printf( "Error in Schedule creation() (%i)\n", res );
				return res;
			}

			res = NBC_Sched_commit( schedule );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_commit() (%i)\n", res );
				return res;
			}
			break;
	}

	handle->schedule = schedule;

	/* tmpbuf is freed with the handle */
	// return NBC_OK;
	return MPI_SUCCESS;
}

/* binomial reduce
 * working principle:
 * - each node gets a virtual rank vrank
 * - the 'root' node get vrank 0
 * - node 0 gets the vrank of the 'root'
 * - all other ranks stay identical (they do not matter)
 *
 * Algorithm:
 * pairwise exchange
 * round r:
 *	grp = rank % 2^r
 *	if grp == 0: receive from rank + 2^(r-1) if it exists and reduce value
 *	if grp == 1: send to rank - 2^(r-1) and exit function
 *
 * do this for R=log_2(p) rounds
 *
 */

static inline int ___red_sched_binomial( int rank, int p, int root, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, void *redbuf, NBC_Schedule *schedule, NBC_Handle *handle )
{
	int firstred, vrank, vpeer, peer, res, maxr, r;

	RANK2VRANK( rank, vrank, root );
	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	firstred = 1;
	int pos = sizeof( int );
	int pos_rounds = 0;
	for ( r = 1; r <= maxr; r++ )
	{
		if ( ( vrank % ( 1 << r ) ) == 0 )
		{
			/* we have to receive this round */
			vpeer = vrank + ( 1 << ( r - 1 ) );
			VRANK2RANK( peer, vpeer, root )
			if ( peer < p )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				/* we have to wait until we have the data */
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				/* perform the reduce in my local buffer */
				if ( firstred )
				{
					if ( rank == root )
					{
						/* root is the only one who reduces in the receivebuffer
						 * take data from sendbuf in first round - save copy */
						res = NBC_Sched_op_pos( pos, recvbuf, 0, (void *) sendbuf, 0, 0, 1, count,
												datatype, op, schedule );
					}
					else
					{
						/* all others may not have a receive buffer
						 * take data from sendbuf in first round - save copy */
						res = NBC_Sched_op_pos(
							pos, (char *) redbuf - (unsigned long) handle->tmpbuf, 1, (void *) sendbuf,
							0, 0, 1, count, datatype, op, schedule );
					}
					firstred = 0;
				}
				else
				{
					if ( rank == root )
					{
						/* root is the only one who reduces in the receivebuffer */
						res = NBC_Sched_op_pos( pos, recvbuf, 0, recvbuf, 0, 0, 1, count,
												datatype, op, schedule );
					}
					else
					{
						/* all others may not have a
						 * receive buffer */
						res = NBC_Sched_op_pos(
							pos, (char *) redbuf - (unsigned long) handle->tmpbuf, 1,
							(char *) redbuf - (unsigned long) handle->tmpbuf, 1, 0, 1, count,
							datatype, op, schedule );
					}
				}
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_op() (%i)\n", res );
					return res;
				}
				/* this cannot be done until
				 * handle->tmpbuf is unused :-( */
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() "
							"(%i)\n",
							res );
					return res;
				}
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
			}
		}
		else
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			/* we have to send this round */
			vpeer = vrank - ( 1 << ( r - 1 ) );
			VRANK2RANK( peer, vpeer, root )
			if ( firstred )
			{
				/* we did not reduce anything */
				res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			else
			{
				/* we have to use the redbuf the root (which works in
				 * receivebuf) is never sending .. */
				res = NBC_Sched_send_pos( pos,
										  (char *) redbuf - (unsigned long) handle->tmpbuf,
										  1, count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			/* leave the game */
			break;
		}
	}

	return NBC_OK;
}

/* chain send ... */
static __inline__ int ___red_sched_chain( int rank, int p, int root, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Aint ext, int size, NBC_Schedule *schedule, __UNUSED__ NBC_Handle *handle, int fragsize )
{
	int res = 0;
	int vrank = 0;
	int rpeer = 0;
	int speer = 0;
	int numfrag = 0;
	int fragnum = 0;
	int fragcount = 0;
	int thiscount = 0;
	long offset = 0;

	//(void)fprintf( stderr, "RED_SCHED_CHAIN\n" );

	RANK2VRANK( rank, vrank, root );
	VRANK2RANK( rpeer, vrank + 1, root );
	VRANK2RANK( speer, vrank - 1, root );

	if ( count == 0 )
		return NBC_OK;

	numfrag = count * size / fragsize;
	if ( ( count * size ) % fragsize != 0 )
		numfrag++;
	fragcount = count / numfrag;
	/*printf("numfrag: %i, count: %i, size: %i, fragcount: %i\n", numfrag, count, size, fragcount);*/

	for ( fragnum = 0; fragnum < numfrag; fragnum++ )
	{
		offset = (MPI_Aint)fragnum * fragcount * ext;
		thiscount = fragcount;
		if ( fragnum == numfrag - 1 )
		{
			/* last fragment may not be full */
			thiscount = count - fragcount * fragnum;
		}

		/* last node does not recv */
		if ( vrank != p - 1 )
		{
			res = NBC_Sched_recv( (char *) offset, true, thiscount, datatype, rpeer, schedule );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			res = NBC_Sched_barrier( schedule );
			/* root reduces into receivebuf */
			if ( vrank == 0 )
			{
				res = NBC_Sched_op( (char *) recvbuf + offset, false, (char *) sendbuf + offset, false, (char *) offset, true, thiscount, datatype, op, schedule );
			}
			else
			{
				res = NBC_Sched_op( (char *) offset, true, (char *) sendbuf + offset, false, (char *) offset, true, thiscount, datatype, op, schedule );
			}
			res = NBC_Sched_barrier( schedule );
		}

		/* root does not send */
		if ( vrank != 0 )
		{
			/* rank p-1 has to send out of sendbuffer :) */
			if ( vrank == p - 1 )
			{
				res = NBC_Sched_send( (char *) sendbuf + offset, false, thiscount, datatype, speer, schedule );
			}
			else
			{
				res = NBC_Sched_send( (char *) offset, true, thiscount, datatype, speer, schedule );
			}
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			/* this barrier here seems awkward but isn't!!!! */
			res = NBC_Sched_barrier( schedule );
		}
	}

	return NBC_OK;
}
/*********************** *
 * NBC_IREDUCE_SCATTER.C *
 * ***********************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* an reduce_csttare schedule can not be cached easily because the contents
 * ot the recvcounts array may change, so a comparison of the address
 * would not be sufficient ... we simply do not cache it */

/* binomial reduce to rank 0 followed by a linear scatter ...
 *
 * Algorithm:
 * pairwise exchange
 * round r:
 *	grp = rank % 2^r
 *	if grp == 0: receive from rank + 2^(r-1) if it exists and reduce value
 *	if grp == 1: send to rank - 2^(r-1) and exit function
 *
 * do this for R=log_2(p) rounds
 *
 */

int NBC_Ireduce_scatter( const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int peer, rank, maxr, p, r, res, count, offset, firstred;
	MPI_Aint ext;
	char *redbuf, *sbuf, inplace;
	NBC_Schedule *schedule;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );
	if ( !handle->is_persistent )
	{

		res = NBC_Init_handle( handle, comm, MPC_IREDUCE_SCATTER_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return NBC_OOR;
		}

		maxr = (int) ceil( ( log( p ) / LOG2 ) );

		count = 0;
		for ( r = 0; r < p; r++ )
			count += recvcounts[r];

		handle->tmpbuf = sctk_malloc( ext * count * 2 );
		if ( handle->tmpbuf == NULL )
		{
			printf( "Error in sctk_malloc()\n" );
			return NBC_OOR;
		}

		redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );

		/* copy data to redbuf if we only have a single node */
		if ( ( p == 1 ) && !inplace )
		{
			res = NBC_Copy( sendbuf, count, datatype, redbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}
		int recv_op_rounds = 0;

		for ( r = 1; r <= maxr; r++ )
		{
			/* we have to receive this round */
			if ( ( rank % ( 1 << r ) ) == 0 )
			{
				peer = rank + ( 1 << ( r - 1 ) );
				if ( peer < p )
				{
					recv_op_rounds++;
				}
			}
			else
			{
				break;
			}
		}

		int alloc_size = (int)(
			sizeof( int ) +
			recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							   sizeof( int ) + sizeof( char ) ) +
			recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							   sizeof( int ) + sizeof( char ) ) +
			sizeof( char ) + sizeof( int ));
		if ( rank != 0 )
		{
			alloc_size += (int)( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			alloc_size += (int)( sizeof( char ) + sizeof( int ) );
		}

		if ( rank == 0 )
		{
			alloc_size += (int)(( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) + sizeof( int ) ) +
						  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ));
		}
		else
		{
			alloc_size += (int)( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		firstred = 1;
		for ( r = 1; r <= maxr; r++ )
		{
			if ( ( rank % ( 1 << r ) ) == 0 )
			{
				/* we have to receive this round */
				peer = rank + ( 1 << ( r - 1 ) );
				if ( peer < p )
				{
					*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
					res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, peer, schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_recv() (%i)\n", res );
						return res;
					}
					/* we have to wait until we have the data */

					res = NBC_Sched_barrier_pos( pos, schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;

					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_barrier() (%i)\n", res );
						return res;
					}
					if ( firstred )
					{
						/* take reduce data from the sendbuf in the first round -> save copy
						 */
						res =
							NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
											  (void *) sendbuf, 0, 0, 1, count, datatype, op, schedule );
						firstred = 0;
					}
					else
					{
						/* perform the reduce in my local buffer */
						res = NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
												redbuf - (unsigned long) handle->tmpbuf, 1, 0,
												1, count, datatype, op, schedule );
					}
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_op() (%i)\n", res );
						return res;
					}
					/* this cannot be done until handle->tmpbuf is unused :-( */
					res = NBC_Sched_barrier_pos( pos, schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_barrier() (%i)\n", res );
						return res;
					}
					//*(int *)((char *)*schedule + sizeof(int) + pos_rounds) = 0;
				}
			}
			else
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				/* we have to send this round */
				peer = rank - ( 1 << ( r - 1 ) );
				if ( firstred )
				{
					/* we have to send the senbuf */
					res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype, peer,
											  schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				}
				else
				{
					/* we send an already reduced value from redbuf */
					res = NBC_Sched_send_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
											  count, datatype, peer, schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				}
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
				/* leave the game */
				break;
			}
		}

		if ( rank != 0 )
		{
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
		}

		/* rank 0 is root and sends - all others receive */
		if ( rank != 0 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcounts[rank], datatype, 0,
									  schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
		}

		if ( rank == 0 )
		{
			//*(int *)((char *)*schedule + sizeof(int) + pos_rounds) = p - 1 + 1;
			offset = 0;
			for ( r = 1; r < p; r++ )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				offset += recvcounts[r - 1];
				sbuf = ( redbuf ) + ( offset * ext );
				/* root sends the right buffer to the right receiver */
				res = NBC_Sched_send_pos( pos, sbuf - (unsigned long) handle->tmpbuf, 1,
										  recvcounts[r], datatype, r, schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
			}
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_copy_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
									  recvcounts[0], datatype, recvbuf, 0, recvcounts[0],
									  datatype, schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_copy() (%i)\n", res );
				return res;
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}
		handle->is_persistent = 0;

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}

		res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	count = 0;
	for ( r = 0; r < p; r++ )
		count += recvcounts[r];

	redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );

	/* copy data to redbuf if we only have a single node */
	if ( ( p == 1 ) && !inplace )
	{
		res = NBC_Copy( sendbuf, count, datatype, redbuf, count, datatype, comm );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Copy() (%i)\n", res );
			return res;
		}
	}

	handle->row_offset = sizeof( int ); /* have to reinit the offset */
	res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Start() (%i)\n", res );
		return res;
	}


	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Reduce_scatter using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcounts Array (of length group size) containing the number of elements received from each process
 *  \param datatype Type of the data elements in sendbuf
 *  \param op Reduction operation
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ireduce_scatter_init( const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int peer, rank, maxr, p, r, res, count, offset, firstred;
	MPI_Aint ext;
	char *redbuf, *sbuf;
	NBC_Schedule *schedule;

	res = NBC_Init_handle( handle, comm, MPC_IREDUCE_SCATTER_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return NBC_OOR;
	}

	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	count = 0;
	for ( r = 0; r < p; r++ )
		count += recvcounts[r];

	handle->tmpbuf = sctk_malloc( ext * count * 2 );
	if ( handle->tmpbuf == NULL )
	{
		printf( "Error in sctk_malloc()\n" );
		return NBC_OOR;
	}

	redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );

	int recv_op_rounds = 0;

	for ( r = 1; r <= maxr; r++ )
	{
		/* we have to receive this round */
		if ( ( rank % ( 1 << r ) ) == 0 )
		{
			peer = rank + ( 1 << ( r - 1 ) );
			if ( peer < p )
			{
				recv_op_rounds++;
			}
		}
		else
		{
			break;
		}
	}

	int alloc_size =
		sizeof( int ) +
		recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						   sizeof( int ) + sizeof( char ) ) +
		recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						   sizeof( int ) + sizeof( char ) ) +
		sizeof( char ) + sizeof( int );
	if ( rank != 0 )
	{
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		alloc_size += ( sizeof( char ) + sizeof( int ) );
	}

	if ( rank == 0 )
	{
		alloc_size += ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) + sizeof( int ) ) +
					  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	}
	else
	{
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	}

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;

	int pos = 0;
	int pos_rounds = pos;
	pos += sizeof( int );
	firstred = 1;
	for ( r = 1; r <= maxr; r++ )
	{
		if ( ( rank % ( 1 << r ) ) == 0 )
		{
			/* we have to receive this round */
			peer = rank + ( 1 << ( r - 1 ) );
			if ( peer < p )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				/* we have to wait until we have the data */

				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;

				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				if ( firstred )
				{
					/* take reduce data from the sendbuf in the first round -> save copy
					 */
					res =
						NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
										  (void *) sendbuf, 0, 0, 1, count, datatype, op, schedule );
					firstred = 0;
				}
				else
				{
					/* perform the reduce in my local buffer */
					res = NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
											redbuf - (unsigned long) handle->tmpbuf, 1, 0,
											1, count, datatype, op, schedule );
				}
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_op() (%i)\n", res );
					return res;
				}
				/* this cannot be done until handle->tmpbuf is unused :-( */
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				//*(int *)((char *)*schedule + sizeof(int) + pos_rounds) = 0;
			}
		}
		else
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			/* we have to send this round */
			peer = rank - ( 1 << ( r - 1 ) );
			if ( firstred )
			{
				/* we have to send the senbuf */
				res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			else
			{
				/* we send an already reduced value from redbuf */
				res = NBC_Sched_send_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
										  count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			/* leave the game */
			break;
		}
	}

	if ( rank != 0 )
	{
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
	}

	/* rank 0 is root and sends - all others receive */
	if ( rank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcounts[rank], datatype, 0,
								  schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
	}

	if ( rank == 0 )
	{
		//*(int *)((char *)*schedule + sizeof(int) + pos_rounds) = p - 1 + 1;
		offset = 0;
		for ( r = 1; r < p; r++ )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			offset += recvcounts[r - 1];
			sbuf = ( (char *) redbuf ) + ( offset * ext );
			/* root sends the right buffer to the right receiver */
			res = NBC_Sched_send_pos( pos, sbuf - (unsigned long) handle->tmpbuf, 1,
									  recvcounts[r], datatype, r, schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
		}
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_copy_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
								  recvcounts[0], datatype, recvbuf, 0, recvcounts[0],
								  datatype, schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_copy() (%i)\n", res );
			return res;
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Start() (%i)\n", res );
		return res;
	}

	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/************* *
 * NBC_ISCAN.C *
 * *************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* linear iscan
 * working principle:
 * 1. each node (but node 0) receives from left neigbor
 * 2. performs op
 * 3. all but rank p-1 do sends to it's right neigbor and exits
 *
 */
int NBC_Iscan( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res;
	MPI_Aint ext;
	NBC_Schedule *schedule;
	char inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_ISCAN_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = sctk_malloc( ext * count );
		if ( handle->tmpbuf == NULL )
		{
			printf( "Error in sctk_malloc()\n" );
			return NBC_OOR;
		}

		if ( ( rank == 0 ) && !inplace )
		{
			/* copy data to receivebuf */
			res = NBC_Copy( sendbuf, count, datatype, recvbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		int alloc_size = sizeof( int ) + sizeof( int ) + sizeof( char );

		if ( rank != 0 )
			alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							sizeof( int ) + sizeof( char ) ) +
						  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							sizeof( int ) + sizeof( char ) );
		if ( rank != p - 1 )
			alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		if ( rank != 0 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, rank - 1,
									  schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			/* we have to wait until we have the data */
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );

			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			/* perform the reduce in my local buffer */

			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_op_pos( pos, recvbuf, 0, (void *) sendbuf, 0, 0, 1,
									count, datatype, op, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_op() (%i)\n", res );
				return res;
			}
			/* this cannot be done until handle->tmpbuf is unused :-( */
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
		}
		if ( rank != p - 1 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_send_pos( pos, recvbuf, 0, count, datatype,
									  rank + 1, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		/* tmpbuf is freed with the handle */
		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		if ( ( rank == 0 ) && !inplace )
		{
			/* copy data to receivebuf */
			res = NBC_Copy( sendbuf, count, datatype, recvbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		if ( rank != 0 )
		{
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			/* we have to wait until we have the data */
			res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );

			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			/* perform the reduce in my local buffer */

			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			/* this cannot be done until handle->tmpbuf is unused :-( */
			res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 0;
		}
		if ( rank != p - 1 )
		{
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		/* tmpbuf is freed with the handle */
		return MPI_SUCCESS;
	}
}

/** \brief Initialize NBC structures used in call of persistent Scan using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param count Number of elements in sendbuf
 *  \param datatype Type of the data elements in sendbuf
 *  \param op Reduction operation
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iscan_init( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res;
	MPI_Aint ext;
	NBC_Schedule *schedule;

	res = NBC_Init_handle( handle, comm, MPC_ISCAN_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = sctk_malloc( ext * count );
	if ( handle->tmpbuf == NULL )
	{
		printf( "Error in sctk_malloc()\n" );
		return NBC_OOR;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	int alloc_size = sizeof( int ) + sizeof( int ) + sizeof( char );

	if ( rank != 0 )
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						sizeof( int ) + sizeof( char ) ) +
					  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						sizeof( int ) + sizeof( char ) );
	if ( rank != p - 1 )
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;

	int pos = 0;
	int pos_rounds = pos;
	pos += sizeof( int );
	if ( rank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, rank - 1,
								  schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
		/* we have to wait until we have the data */
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );

		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
		/* perform the reduce in my local buffer */

		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_op_pos( pos, recvbuf, 0, (void *) sendbuf, 0, 0, 1,
								count, datatype, op, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_op() (%i)\n", res );
			return res;
		}
		/* this cannot be done until handle->tmpbuf is unused :-( */
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
	}
	if ( rank != p - 1 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_send_pos( pos, recvbuf, 0, count, datatype,
								  rank + 1, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;
	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/**************** *
 * NBC_ISCATTER.C *
 * ****************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* simple linear MPI_Iscatter */
int NBC_Iscatter( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint sndext;
	NBC_Schedule *schedule;
	char *sbuf, inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );
	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_ISCATTER_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		if ( ( rank == root ) && ( !inplace ) )
		{
			sbuf = ( (char *) sendbuf ) + ( rank * sendcount * sndext );
			/* if I am the root - just copy the message (not for MPI_IN_PLACE) */
			res = NBC_Copy( sbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		int alloc_size = -1;
		int round_size = -1;
		if ( rank == root )
		{
			if ( p >= 64 )
			{
				alloc_size =
					sizeof( int ) +
					( p - 1 ) * ( sizeof( int ) + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) );
				round_size = 1;
			}
			else
			{
				alloc_size =
					sizeof( int ) + sizeof( int ) +
					( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					sizeof( char );
				round_size = p - 1;
			}
		}
		else
		{
			alloc_size = sizeof( int ) + sizeof( int ) +
						 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						 sizeof( char );
			round_size = 1;
		}

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = round_size;

		/* receive from root */
		if ( rank != root )
		{
			int pos = sizeof( int );
			/* recv msg from root */
			res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcount, recvtype,
									  root, schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
		}
		else
		{
			if ( p >= 64 )
			{
				int pos = 0;
				int pos_rounds = pos;
				pos += sizeof( int );
				for ( i = 0; i < p; i++ )
				{
					sbuf = ( (char *) sendbuf ) + ( i * sendcount * sndext );
					if ( i != root )
					{
						*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
						/* root sends the right buffer to the right receiver */
						res = NBC_Sched_send_pos( pos, sbuf, 0, sendcount,
												  sendtype, i, schedule );
						pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
						res = NBC_Sched_barrier_pos( pos, schedule );
						pos += sizeof( char );
						pos_rounds = pos;
						pos += sizeof( int );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Sched_send() "
									"(%i)\n",
									res );
							return res;
						}
					}
				}
			}
			else
			{
				int pos = sizeof( int );
				for ( i = 0; i < p; i++ )
				{
					sbuf = ( (char *) sendbuf ) + ( i * sendcount * sndext );
					if ( i != root )
					{
						/* root sends the right buffer to the right receiver */
						res = NBC_Sched_send_pos( pos, sbuf, 0, sendcount,
												  sendtype, i, schedule );
						pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Sched_send() "
									"(%i)\n",
									res );
							return res;
						}
					}
				}
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		if ( ( rank == root ) && ( !inplace ) )
		{
			sbuf = ( (char *) sendbuf ) + ( rank * sendcount * sndext );
			/* if I am the root - just copy the message (not for MPI_IN_PLACE) */
			res = NBC_Copy( sbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
	}

	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Scatter using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcount Number of elements in sendbuf
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcount Number of elements in recvbuf
 *  \param recvtype Type of the data elements in recvbuf
 *  \param root Rank root of the broadcast
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iscatter_init( const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint sndext;
	NBC_Schedule *schedule;
	char *sbuf;

	res = NBC_Init_handle( handle, comm, MPC_ISCATTER_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( sendtype, &sndext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	int alloc_size = -1;
	int round_size = -1;
	if ( rank == root )
	{
		if ( p >= 64 )
		{
			alloc_size =
				sizeof( int ) +
				( p - 1 ) * ( sizeof( int ) + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) );
			round_size = 1;
		}
		else
		{
			alloc_size =
				sizeof( int ) + sizeof( int ) +
				( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
				sizeof( char );
			round_size = p - 1;
		}
	}
	else
	{
		alloc_size = sizeof( int ) + sizeof( int ) +
					 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					 sizeof( char );
		round_size = 1;
	}

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;
	*( ( (int *) *schedule ) + 1 ) = round_size;

	/* receive from root */
	if ( rank != root )
	{
		int pos = sizeof( int );
		/* recv msg from root */
		res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcount, recvtype,
								  root, schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
	}
	else
	{
		if ( p >= 64 )
		{
			int pos = 0;
			int pos_rounds = pos;
			pos += sizeof( int );
			for ( i = 0; i < p; i++ )
			{
				sbuf = ( (char *) sendbuf ) + ( i * sendcount * sndext );
				if ( i != root )
				{
					*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
					/* root sends the right buffer to the right receiver */
					res = NBC_Sched_send_pos( pos, sbuf, 0, sendcount,
											  sendtype, i, schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					res = NBC_Sched_barrier_pos( pos, schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_send() "
								"(%i)\n",
								res );
						return res;
					}
				}
			}
		}
		else
		{
			int pos = sizeof( int );
			for ( i = 0; i < p; i++ )
			{
				sbuf = ( (char *) sendbuf ) + ( i * sendcount * sndext );
				if ( i != root )
				{
					/* root sends the right buffer to the right receiver */
					res = NBC_Sched_send_pos( pos, sbuf, 0, sendcount,
											  sendtype, i, schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_send() "
								"(%i)\n",
								res );
						return res;
					}
				}
			}
		}
	}

	//                if (rank == root)
	//                {
	//                  alloc_size =
	//                      sizeof(int) + sizeof(int) +
	//                      (p - 1) * (sizeof(NBC_Args) + sizeof(NBC_Fn_type)) +
	//                      sizeof(char);
	//                  round_size = p - 1;
	//                }
	//                else
	//                {
	//                  alloc_size = sizeof(int) + sizeof(int) +
	//                               (sizeof(NBC_Args) + sizeof(NBC_Fn_type)) +
	//                               sizeof(char);
	//                  round_size = 1;
	//                }
	//
	//                *schedule = sctk_malloc(alloc_size);
	//                *(int *)*schedule = alloc_size;
	//                *(((int *)*schedule) + 1) = round_size;
	//
	//                int pos = sizeof(int);
	//
	//
	//                /* receive from root */
	//		if (rank != root)
	//		{
	//			/* recv msg from root */
	//			res = NBC_Sched_recv_pos(pos, recvbuf, 0, recvcount, recvtype,
	//					root, schedule);
	//			pos += sizeof(NBC_Args) + sizeof(NBC_Fn_type);
	//			if (NBC_OK != res)
	//			{
	//				printf("Error in NBC_Sched_recv() (%i)\n", res);
	//				return res;
	//			}
	//		}
	//                else
	//                {
	//			for (i = 0; i < p; i++)
	//			{
	//				sbuf = ((char *)sendbuf) + (i * sendcount * sndext);
	//				if (i != root)
	//				{
	//					/* root sends the right buffer to the right receiver */
	//					res = NBC_Sched_send_pos(pos, sbuf, 0, sendcount,
	//							sendtype, i, schedule);
	//					pos += sizeof(NBC_Args) + sizeof(NBC_Fn_type);
	//					if (NBC_OK != res)
	//					{
	//						printf("Error in NBC_Sched_send() "
	//								"(%i)\n",
	//								res);
	//						return res;
	//					}
	//				}
	//			}
	//		}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/***************** *
 * NBC_ISCATTERV.C *
 * *****************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* a scatterv schedule can not be cached easily because the contents
 * ot the recvcounts array may change, so a comparison of the address
 * would not be sufficient ... we simply do not cache it */

/* simple linear MPI_Iscatterv */
int NBC_Iscatterv( const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint sndext;
	NBC_Schedule *schedule;
	char *sbuf, inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_ISCATTERV_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		handle->tmpbuf = NULL;

		int alloc_size = -1;
		int round_size = -1;
		if ( rank == root )
		{
			if ( p >= 64 )
			{
				alloc_size =
					sizeof( int ) +
					( p - 1 ) * ( sizeof( int ) + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) );
				round_size = 1;
			}
			else
			{
				alloc_size =
					sizeof( int ) + sizeof( int ) +
					( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					sizeof( char );
				round_size = p - 1;
			}
		}
		else
		{
			alloc_size = sizeof( int ) + sizeof( int ) +
						 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						 sizeof( char );
			round_size = 1;
		}

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = round_size;

		/* receive from root */
		if ( rank != root )
		{
			int pos = sizeof( int );
			/* recv msg from root */
			res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcount, recvtype, root,
									  schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
		}
		else
		{
			if ( p >= 64 )
			{
				int pos = 0;
				int pos_rounds = pos;
				pos += sizeof( int );
				for ( i = 0; i < p; i++ )
				{
					sbuf = ( (char *) sendbuf ) + ( displs[i] * sndext );
					if ( i != root )
					{
						*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
						/* root sends the right buffer to the right receiver */
						res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[i], sendtype, i,
												  schedule );
						pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
						res = NBC_Sched_barrier_pos( pos, schedule );
						pos += sizeof( char );
						pos_rounds = pos;
						pos += sizeof( int );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Sched_send() "
									"(%i)\n",
									res );
							return res;
						}
					}
				}
			}
			else
			{
				int pos = sizeof( int );
				for ( i = 0; i < p; i++ )
				{
					sbuf = ( (char *) sendbuf ) + ( displs[i] * sndext );
					if ( i != root )
					{
						/* root sends the right buffer to the right receiver */
						res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[i], sendtype, i,
												  schedule );
						pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Sched_send() "
									"(%i)\n",
									res );
							return res;
						}
					}
				}
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( sendtype, &sndext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		/* receive from root */
		if ( rank == root )
		{
			for ( i = 0; i < p; i++ )
			{
				sbuf = ( (char *) sendbuf ) + ( displs[i] * sndext );
				if ( i == root )
				{
					if ( !inplace )
					{
						/* if I am the root - just copy the message */
						res = NBC_Copy( sbuf, sendcounts[i], sendtype, recvbuf,
										recvcount, recvtype, comm );
						if ( NBC_OK != res )
						{
							printf( "Error in NBC_Copy() (%i)\n", res );
							return res;
						}
					}
				}
			}
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
	}

	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Scatterv using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcounts Array (of length group size) containing the number of elements send to each process
 *  \param displs Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
 *  \param sendtype Type of the data elements in sendbuf
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcount Number of elements in recvbuf
 *  \param recvtype Type of the data elements in recvbuf
 *  \param root Rank root of the broadcast
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iscatterv_init( const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res, i;
	MPI_Aint sndext;
	NBC_Schedule *schedule;
	char *sbuf;

	res = NBC_Init_handle( handle, comm, MPC_ISCATTERV_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( sendtype, &sndext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	handle->tmpbuf = NULL;

	int alloc_size = -1;
	int round_size = -1;
	if ( rank == root )
	{
		if ( p >= 64 )
		{
			alloc_size =
				sizeof( int ) +
				( p - 1 ) * ( sizeof( int ) + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) );
			round_size = p - 1;
		}
		else
		{
			alloc_size =
				sizeof( int ) + sizeof( int ) +
				( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
				sizeof( char );
			round_size = p - 1;
		}
	}
	else
	{
		alloc_size = sizeof( int ) + sizeof( int ) +
					 ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					 sizeof( char );
		round_size = 1;
	}

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;
	*( ( (int *) *schedule ) + 1 ) = round_size;

	/* receive from root */
	if ( rank != root )
	{
		int pos = sizeof( int );
		/* recv msg from root */
		res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcount, recvtype, root,
								  schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
	}
	else
	{
		if ( p >= 64 )
		{
			int pos = 0;
			int pos_rounds = pos;
			pos += sizeof( int );
			for ( i = 0; i < p; i++ )
			{
				sbuf = ( (char *) sendbuf ) + ( displs[i] * sndext );
				if ( i != root )
				{
					*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
					/* root sends the right buffer to the right receiver */
					res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[i], sendtype, i,
											  schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					res = NBC_Sched_barrier_pos( pos, schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_send() "
								"(%i)\n",
								res );
						return res;
					}
				}
			}
		}
		else
		{
			int pos = sizeof( int );
			for ( i = 0; i < p; i++ )
			{
				sbuf = ( (char *) sendbuf ) + ( displs[i] * sndext );
				if ( i != root )
				{
					/* root sends the right buffer to the right receiver */
					res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[i], sendtype, i,
											  schedule );
					pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
					if ( NBC_OK != res )
					{
						printf( "Error in NBC_Sched_send() "
								"(%i)\n",
								res );
						return res;
					}
				}
			}
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/********************************************************************* *
 * The previous code came from the libNBC, a library for the support	 *
 * of MPI-3 Non-Blocking Collective. The code was modified to ensure	 *
 * compatibility with the MPC framework.															 *
 * *********************************************************************/

/********************************************************************* *
 * The Following code is a modified version of three functions from		*
 * the libNBC. It aims to support the three MPI-3 Non-Blocking				 *
 * Collective functions that are not initially present in the libNBC.	*
 * *********************************************************************/
/****************** *
 * NBC_IALLTOALLW.C *
 * ******************/

/* simple linear Alltoallv */
int NBC_Ialltoallw( const void *sendbuf, const int *sendcounts, const int *sdispls,
					const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls,
					const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle )
{

	int rank, p, res;

	NBC_Schedule *schedule;
	char *rbuf, *sbuf, inplace;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_IALLTOALLW_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = NULL;

		// res = NBC_Sched_create(schedule);
		// if(res != NBC_OK)
		//{ printf("Error in NBC_Sched_create (%i)\n", res); return res; }

		/* copy data to receivbuffer */
		if ( ( sendcounts[rank] != 0 ) && !inplace )
		{
			rbuf = ( (char *) recvbuf ) + ( rdispls[rank] );
			sbuf = ( (char *) sendbuf ) + ( sdispls[rank] );
			res = NBC_Copy( sbuf, sendcounts[rank], sendtypes[rank], rbuf, recvcounts[rank], recvtypes[rank], comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		// for (i = 0; i < p; i++)
		//{
		//     if (i == rank)
		//     { continue; }
		//     /* post all sends */
		//     if(sendcounts[i] != 0)
		//     {
		//         sbuf = ((char *) sendbuf) + (sdispls[i]);
		//         res = NBC_Sched_send(sbuf, 0, sendcounts[i], sendtypes[i], i, schedule);
		//         if (NBC_OK != res)
		//         { printf("Error in NBC_Sched_send() (%i)\n", res); return res; }
		//     }
		//     /* post all receives */
		//     if(recvcounts[i] != 0)
		//     {
		//         rbuf = ((char *) recvbuf) + (rdispls[i]);
		//         res = NBC_Sched_recv(rbuf, 0, recvcounts[i], recvtypes[i], i, schedule);
		//         if (NBC_OK != res)
		//         { printf("Error in NBC_Sched_recv() (%i)\n", res); return res; }
		//     }
		// }
		//
		//   int r;
		//    int sndpeer, rcvpeer;
		//	for(r=0; r<p; r++)
		//    {
		//        if(r == rank)
		//            continue;
		//
		//		sndpeer = (rank+r)%p;
		//		rcvpeer = (rank-r+p)%p;
		//
		//			rbuf = ((char *) recvbuf) + (rdispls[rcvpeer]);
		//			res = NBC_Sched_recv(rbuf, 0, recvcounts[rcvpeer], recvtypes[rcvpeer], rcvpeer, schedule);
		//		if (NBC_OK != res)
		//        { printf("Error in NBC_Sched_recv() (%i)\n", res); return res; }
		//			sbuf = ((char *) sendbuf) + (sdispls[sndpeer]);
		//			res = NBC_Sched_send(sbuf, 0, sendcounts[sndpeer], sendtypes[sndpeer], sndpeer, schedule);
		//		if (NBC_OK != res)
		//        { printf("Error in NBC_Sched_send() (%i)\n", res); return res; }
		//
		//			res = NBC_Sched_barrier(schedule);
		//			if (NBC_OK != res)
		//            { printf("Error in NBC_Sched_barr() (%i)\n", res); return res; }
		//	}
		//
		if ( p >= 64 )
		{
			int alloc_size = 0;
			alloc_size += sizeof( int ) +
						  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  ( p - 1 ) * ( sizeof( int ) + sizeof( char ) );
			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;
			*( ( (int *) *schedule ) + 1 ) = 2;
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}
			int pos = 0;
			int pos_rounds = pos;
			pos += sizeof( int );
			int r;
			int sndpeer, rcvpeer;
			for ( r = 1; r < p; r++ )
			{

				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2;
				sndpeer = ( rank + r ) % p;
				rcvpeer = ( rank - r + p ) % p;

				rbuf = ( (char *) recvbuf ) + ( rdispls[rcvpeer] );
				res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rcvpeer], recvtypes[rcvpeer], rcvpeer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				sbuf = ( (char *) sendbuf ) + ( sdispls[sndpeer] );
				res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[sndpeer], sendtypes[sndpeer], sndpeer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
			}
		}
		else
		{
			int alloc_size = 0;
			alloc_size += sizeof( int ) +
						  sizeof( int ) +
						  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
						  sizeof( char );
			*schedule = sctk_malloc( alloc_size );
			*(int *) *schedule = alloc_size;
			*( ( (int *) *schedule ) + 1 ) = 2 * ( p - 1 );
			if ( res != NBC_OK )
			{
				printf( "Error in NBC_Sched_create (%i)\n", res );
				return res;
			}

			int pos = 0;
			int pos_rounds = pos;
			pos += sizeof( int );
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2 * ( p - 1 );
			int r;
			int sndpeer, rcvpeer;
			for ( r = 1; r < p; r++ )
			{

				sndpeer = ( rank + r ) % p;
				rcvpeer = ( rank - r + p ) % p;

				rbuf = ( (char *) recvbuf ) + ( rdispls[rcvpeer] );
				res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rcvpeer], recvtypes[rcvpeer], rcvpeer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				sbuf = ( (char *) sendbuf ) + ( sdispls[sndpeer] );
				res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[sndpeer], sendtypes[sndpeer], sndpeer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}

		/* copy data to receivbuffer */
		if ( ( sendcounts[rank] != 0 ) && !inplace )
		{
			rbuf = ( (char *) recvbuf ) + ( rdispls[rank] );
			sbuf = ( (char *) sendbuf ) + ( sdispls[rank] );
			res = NBC_Copy( sbuf, sendcounts[rank], sendtypes[rank], rbuf, recvcounts[rank], recvtypes[rank], comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
		if ( NBC_OK != res )
		{
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		return MPI_SUCCESS;
	}
}

/** \brief Initialize NBC structures used in call of persistent Alltoallw using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param sendcounts Array (of length group size) containing the number of elements send to each process
 *  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
 *  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcounts Array (of length group size) containing the number of elements received from each process
 *  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
 *  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ialltoallw_init( const void *sendbuf, const int *sendcounts, const int *sdispls,
						 const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls,
						 const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle )
{

	int rank, p, res;

	NBC_Schedule *schedule;
	char *rbuf, *sbuf;

	res = NBC_Init_handle( handle, comm, MPC_IALLTOALLW_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = NULL;

	res = NBC_Sched_create( schedule );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Sched_create (%i)\n", res );
		return res;
	}

	// for (i = 0; i < p; i++)
	//{
	//	if (i == rank)
	//	{ continue; }
	//	/* post all sends */
	//	if(sendcounts[i] != 0)
	//	{
	//		sbuf = ((char *) sendbuf) + (sdispls[i]);
	//		res = NBC_Sched_send(sbuf, 0, sendcounts[i], sendtypes[i], i, schedule);
	//		if (NBC_OK != res)
	//		{ printf("Error in NBC_Sched_send() (%i)\n", res); return res; }
	//	}
	//	/* post all receives */
	//	if(recvcounts[i] != 0)
	//	{
	//		rbuf = ((char *) recvbuf) + (rdispls[i]);
	//		res = NBC_Sched_recv(rbuf, 0, recvcounts[i], recvtypes[i], i, schedule);
	//		if (NBC_OK != res)
	//		{ printf("Error in NBC_Sched_recv() (%i)\n", res); return res; }
	//	}
	// }
	if ( p >= 64 )
	{
		int alloc_size = 0;
		alloc_size += sizeof( int ) +
					  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					  ( p - 1 ) * ( sizeof( int ) + sizeof( char ) );
		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = 2;
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Sched_create (%i)\n", res );
			return res;
		}
		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		int r;
		int sndpeer, rcvpeer;
		for ( r = 1; r < p; r++ )
		{

			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2;
			sndpeer = ( rank + r ) % p;
			rcvpeer = ( rank - r + p ) % p;

			rbuf = ( (char *) recvbuf ) + ( rdispls[rcvpeer] );
			res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rcvpeer], recvtypes[rcvpeer], rcvpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			sbuf = ( (char *) sendbuf ) + ( sdispls[sndpeer] );
			res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[sndpeer], sendtypes[sndpeer], sndpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
		}
	}
	else
	{
		int alloc_size = 0;
		alloc_size += sizeof( int ) +
					  sizeof( int ) +
					  2 * ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					  sizeof( char );
		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;
		*( ( (int *) *schedule ) + 1 ) = 2 * ( p - 1 );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Sched_create (%i)\n", res );
			return res;
		}

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 2 * ( p - 1 );
		int r;
		int sndpeer, rcvpeer;
		for ( r = 1; r < p; r++ )
		{

			sndpeer = ( rank + r ) % p;
			rcvpeer = ( rank - r + p ) % p;

			rbuf = ( (char *) recvbuf ) + ( rdispls[rcvpeer] );
			res = NBC_Sched_recv_pos( pos, rbuf, 0, recvcounts[rcvpeer], recvtypes[rcvpeer], rcvpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			sbuf = ( (char *) sendbuf ) + ( sdispls[sndpeer] );
			res = NBC_Sched_send_pos( pos, sbuf, 0, sendcounts[sndpeer], sendtypes[sndpeer], sndpeer, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
	}

	res = NBC_Sched_commit( schedule );
	if ( NBC_OK != res )
	{
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	return MPI_SUCCESS;
}

/***************************** *
 * NBC_IREDUCE_SCATTER_BLOCK.C *
 * *****************************/
int NBC_Ireduce_scatter_block( const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int peer, rank, maxr, p, r, res, count, offset, firstred;
	MPI_Aint ext;
	char *redbuf, *sbuf, inplace;
	NBC_Schedule *schedule;

	NBC_IN_PLACE( sendbuf, recvbuf, inplace );

	if ( !handle->is_persistent )
	{
		res = NBC_Init_handle( handle, comm, MPC_IREDUCE_SCATTER_BLOCK_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return NBC_OOR;
		}

		maxr = (int) ceil( ( log( p ) / LOG2 ) );

		count = 0;
		for ( r = 0; r < p; r++ )
			count += recvcount;

		handle->tmpbuf = sctk_malloc( ext * count * 2 );
		if ( handle->tmpbuf == NULL )
		{
			printf( "Error in sctk_malloc()\n" );
			return NBC_OOR;
		}

		redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );

		/* copy data to redbuf if we only have a single node */
		if ( ( p == 1 ) && !inplace )
		{
			res = NBC_Copy( sendbuf, count, datatype, redbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}
		int recv_op_rounds = 0;
		for ( r = 1; r <= maxr && ( ( rank % ( 1 << r ) ) == 0 ); r++ )
		{
			/* we have to receive this round */
			peer = rank + ( 1 << ( r - 1 ) );
			if ( peer < p )
			{
				recv_op_rounds++;
			}
		}

		int alloc_size =
			sizeof( int ) +
			recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							   sizeof( int ) + sizeof( char ) ) +
			recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							   sizeof( int ) + sizeof( char ) ) +
			sizeof( char ) + sizeof( int );
		if ( rank != 0 )
		{
			alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			alloc_size += ( sizeof( char ) + sizeof( int ) );
			// alloc_size += (sizeof(char) + sizeof(int));
		}

		// alloc_size += (sizeof(char) + sizeof(int));
		if ( rank == 0 )
		{
			alloc_size += ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) + sizeof( char ) + sizeof( int ) ) +
						  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}
		else
		{
			alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		firstred = 1;
		for ( r = 1; r <= maxr; r++ )
		{
			if ( ( rank % ( 1 << r ) ) == 0 )
			{
				/* we have to receive this round */
				peer = rank + ( 1 << ( r - 1 ) );
				if ( peer < p )
				{
					*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
					res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, peer, schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_recv() (%i)\n", res );
						return res;
					}
					/* we have to wait until we have the data */

					res = NBC_Sched_barrier_pos( pos, schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;

					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_barrier() (%i)\n", res );
						return res;
					}
					if ( firstred )
					{
						/* take reduce data from the sendbuf in the first round -> save copy
						 */
						res =
							NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
											  (void *) sendbuf, 0, 0, 1, count, datatype, op, schedule );
						firstred = 0;
					}
					else
					{
						/* perform the reduce in my local buffer */
						res = NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
												redbuf - (unsigned long) handle->tmpbuf, 1, 0,
												1, count, datatype, op, schedule );
					}
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_op() (%i)\n", res );
						return res;
					}
					/* this cannot be done until handle->tmpbuf is unused :-( */
					res = NBC_Sched_barrier_pos( pos, schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_barrier() (%i)\n", res );
						return res;
					}
					//*(int *)((char *)*schedule + sizeof(int) + pos_rounds) = 0;
				}
			}
			else
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				/* we have to send this round */
				peer = rank - ( 1 << ( r - 1 ) );
				if ( firstred )
				{
					/* we have to send the senbuf */
					res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype, peer,
											  schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				}
				else
				{
					/* we send an already reduced value from redbuf */
					res = NBC_Sched_send_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
											  count, datatype, peer, schedule );
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				}
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
				/* leave the game */
				break;
			}
		}

		if ( rank != 0 )
		{
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
		}

		/* rank 0 is root and sends - all others receive */
		if ( rank != 0 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcount, datatype, 0, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
		}

		if ( rank == 0 )
		{
			//*(int *)((char *)*schedule + sizeof(int) + pos_rounds) = p - 1 + 1;
			offset = 0;
			for ( r = 1; r < p; r++ )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				offset += recvcount;
				sbuf = ( (char *) redbuf ) + ( offset * ext );
				/* root sends the right buffer to the right receiver */
				res = NBC_Sched_send_pos( pos, sbuf - (unsigned long) handle->tmpbuf, 1,
										  recvcount, datatype, r, schedule );
				pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_send() (%i)\n", res );
					return res;
				}
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
			}
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_copy_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
									  recvcount, datatype, recvbuf, 0, recvcount,
									  datatype, schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_copy() (%i)\n", res );
				return res;
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
		return MPI_SUCCESS;
	}
	else
	{
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		maxr = (int) ceil( ( log( p ) / LOG2 ) );

		count = 0;
		for ( r = 0; r < p; r++ )
			count += recvcount;

		redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );

		/* copy data to redbuf if we only have a single node */
		if ( ( p == 1 ) && !inplace )
		{
			res = NBC_Copy( sendbuf, count, datatype, redbuf, count, datatype, comm );
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Copy() (%i)\n", res );
				return res;
			}
		}

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		firstred = 1;
		for ( r = 1; r <= maxr; r++ )
		{
			if ( ( rank % ( 1 << r ) ) == 0 )
			{
				/* we have to receive this round */
				peer = rank + ( 1 << ( r - 1 ) );
				if ( peer < p )
				{
					*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					/* we have to wait until we have the data */

					res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;

					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_barrier() (%i)\n", res );
						return res;
					}
					if ( firstred )
					{
						firstred = 0;
					}
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
					/* this cannot be done until handle->tmpbuf is unused :-( */
					res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
					pos += sizeof( char );
					pos_rounds = pos;
					pos += sizeof( int );
					if ( NBC_OK != res )
					{
						sctk_free( handle->tmpbuf );
						printf( "Error in NBC_Sched_barrier() (%i)\n", res );
						return res;
					}
					*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 0;
				}
			}
			else
			{
				*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
				/* we have to send this round */
				peer = rank - ( 1 << ( r - 1 ) );
				if ( firstred )
				{
					/* we have to send the senbuf */
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				}
				else
				{
					/* we send an already reduced value from redbuf */
					pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				}
				/* leave the game */
				break;
			}
		}

		res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}

		/* rank 0 is root and sends - all others receive */
		if ( rank != 0 )
		{
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}
	}

	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/** \brief Initialize NBC structures used in call of persistent Reduce_scatter_block using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param recvcount Number of elements in recvbuf
 *  \param datatype Type of the data elements in sendbuf
 *  \param root Rank root of the broadcast
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Ireduce_scatter_block_init( const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int peer, rank, maxr, p, r, res, count, offset, firstred;
	MPI_Aint ext;
	char *redbuf, *sbuf;
	NBC_Schedule *schedule;

	res = NBC_Init_handle( handle, comm, MPC_IREDUCE_SCATTER_BLOCK_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return NBC_OOR;
	}

	maxr = (int) ceil( ( log( p ) / LOG2 ) );

	count = 0;
	for ( r = 0; r < p; r++ )
		count += recvcount;

	handle->tmpbuf = sctk_malloc( ext * count * 2 );
	if ( handle->tmpbuf == NULL )
	{
		printf( "Error in sctk_malloc()\n" );
		return NBC_OOR;
	}

	redbuf = ( (char *) handle->tmpbuf ) + ( ext * count );

	int recv_op_rounds = 0;
	for ( r = 1; r <= maxr && ( ( rank % ( 1 << r ) ) == 0 ); r++ )
	{
		/* we have to receive this round */
		peer = rank + ( 1 << ( r - 1 ) );
		if ( peer < p )
		{
			recv_op_rounds++;
		}
	}

	int alloc_size =
		sizeof( int ) +
		recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						   sizeof( int ) + sizeof( char ) ) +
		recv_op_rounds * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						   sizeof( int ) + sizeof( char ) ) +
		sizeof( char ) + sizeof( int );
	if ( rank != 0 )
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	alloc_size += ( sizeof( int ) + sizeof( char ) );
	if ( rank == 0 )
	{
		alloc_size += ( p - 1 ) * ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) ) +
					  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	}
	else
	{
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	}

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;

	int pos = 0;
	int pos_rounds = pos;
	pos += sizeof( int );
	firstred = 1;
	for ( r = 1; r <= maxr; r++ )
	{
		if ( ( rank % ( 1 << r ) ) == 0 )
		{
			/* we have to receive this round */
			peer = rank + ( 1 << ( r - 1 ) );
			if ( peer < p )
			{
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
				res = NBC_Sched_recv_pos( pos, 0, 1, count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_recv() (%i)\n", res );
					return res;
				}
				/* we have to wait until we have the data */

				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;

				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				if ( firstred )
				{
					/* take reduce data from the sendbuf in the first round -> save copy
					 */
					res =
						NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
										  (void *) sendbuf, 0, 0, 1, count, datatype, op, schedule );
					firstred = 0;
				}
				else
				{
					/* perform the reduce in my local buffer */
					res = NBC_Sched_op_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
											redbuf - (unsigned long) handle->tmpbuf, 1, 0,
											1, count, datatype, op, schedule );
				}
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_op() (%i)\n", res );
					return res;
				}
				/* this cannot be done until handle->tmpbuf is unused :-( */
				res = NBC_Sched_barrier_pos( pos, schedule );
				pos += sizeof( char );
				pos_rounds = pos;
				pos += sizeof( int );
				if ( NBC_OK != res )
				{
					sctk_free( handle->tmpbuf );
					printf( "Error in NBC_Sched_barrier() (%i)\n", res );
					return res;
				}
				*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
			}
		}
		else
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			/* we have to send this round */
			peer = rank - ( 1 << ( r - 1 ) );
			if ( firstred )
			{
				/* we have to send the senbuf */
				res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype, peer,
										  schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			else
			{
				/* we send an already reduced value from redbuf */
				res = NBC_Sched_send_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
										  count, datatype, peer, schedule );
				pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			}
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
			/* leave the game */
			break;
		}
	}

	res = NBC_Sched_barrier_pos( pos, schedule );
	pos += sizeof( char );
	pos_rounds = pos;
	pos += sizeof( int );
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Sched_barrier() (%i)\n", res );
		return res;
	}

	/* rank 0 is root and sends - all others receive */
	if ( rank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_recv_pos( pos, recvbuf, 0, recvcount, datatype, 0, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
	}

	if ( rank == 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = p - 1 + 1;
		offset = 0;
		for ( r = 1; r < p; r++ )
		{
			offset += recvcount;
			sbuf = ( (char *) redbuf ) + ( offset * ext );
			/* root sends the right buffer to the right receiver */
			res = NBC_Sched_send_pos( pos, sbuf - (unsigned long) handle->tmpbuf, 1,
									  recvcount, datatype, r, schedule );
			pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
		res = NBC_Sched_copy_pos( pos, redbuf - (unsigned long) handle->tmpbuf, 1,
								  recvcount, datatype, recvbuf, 0, recvcount,
								  datatype, schedule );
		pos += sizeof( NBC_Args ) + sizeof( NBC_Fn_type );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_copy() (%i)\n", res );
			return res;
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;
	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

/************* *
 * NBC_IEXSCAN.C *
 * *************/

int NBC_Iexscan( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res;
	MPI_Aint ext;
	NBC_Schedule *schedule;

	if ( !handle->is_persistent )
	{

		res = NBC_Init_handle( handle, comm, MPC_ISCAN_TAG );
		if ( res != NBC_OK )
		{
			printf( "Error in NBC_Init_handle(%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		handle->tmpbuf = sctk_malloc( ext * count );
		if ( handle->tmpbuf == NULL )
		{
			printf( "Error in sctk_malloc()\n" );
			return NBC_OOR;
		}

		schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
		if ( NULL == schedule )
		{
			printf( "Error in sctk_malloc()\n" );
			return res;
		}

		int alloc_size = sizeof( int ) + sizeof( int ) + sizeof( char );

		if ( rank != 0 )
			alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							sizeof( int ) + sizeof( char ) ) +
						  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
							sizeof( int ) + sizeof( char ) );
		if ( rank != p - 1 )
			alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

		*schedule = sctk_malloc( alloc_size );
		*(int *) *schedule = alloc_size;

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		if ( rank != 0 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_recv_pos( pos, recvbuf, 0, count, datatype, rank - 1,
									  schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_recv() (%i)\n", res );
				return res;
			}
			/* we have to wait until we have the data */
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );

			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			/* perform the reduce in my local buffer */

			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_op_pos( pos, 0, 1, (void *) sendbuf, 0, recvbuf, 0, count,
									datatype, op, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_op() (%i)\n", res );
				return res;
			}
			/* this cannot be done until handle->tmpbuf is unused :-( */
			res = NBC_Sched_barrier_pos( pos, schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
		}
		if ( rank != 0 && rank != p - 1 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_send_pos( pos, 0, 1, count, datatype, rank + 1,
									  schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}
		if ( rank == 0 )
		{
			*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
			res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype,
									  rank + 1, schedule );
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_send() (%i)\n", res );
				return res;
			}
		}

		res = NBC_Sched_commit_pos( schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_commit() (%i)\n", res );
			return res;
		}

		res = NBC_Start( handle, schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		/* tmpbuf is freed with the handle */
		return MPI_SUCCESS;
	}
	else
	{

		res = _mpc_cl_comm_rank( comm, &rank );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
			return res;
		}
		res = _mpc_cl_comm_size( comm, &p );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
			return res;
		}
		res = PMPI_Type_extent( datatype, &ext );
		if ( MPI_SUCCESS != res )
		{
			printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
			return res;
		}

		int pos = 0;
		int pos_rounds = pos;
		pos += sizeof( int );
		if ( rank != 0 )
		{
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			/* we have to wait until we have the data */
			res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );

			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			/* perform the reduce in my local buffer */

			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
			/* this cannot be done until handle->tmpbuf is unused :-( */
			res = NBC_Sched_barrier_pos( pos, (NBC_Schedule *) handle->schedule );
			pos += sizeof( char );
			pos_rounds = pos;
			pos += sizeof( int );
			if ( NBC_OK != res )
			{
				sctk_free( handle->tmpbuf );
				printf( "Error in NBC_Sched_barrier() (%i)\n", res );
				return res;
			}
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 0;
		}
		if ( rank != 0 && rank != p - 1 )
		{
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}
		if ( rank == 0 )
		{
			*(int *) ( (char *) *(NBC_Schedule *) handle->schedule + sizeof( int ) + pos_rounds ) = 1;
			pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		}

		handle->row_offset = sizeof( int ); /* have to reinit the offset */
		res = NBC_Start( handle, (NBC_Schedule *) handle->schedule );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Start() (%i)\n", res );
			return res;
		}

		/* tmpbuf is freed with the handle */
		return MPI_SUCCESS;
	}
}

/** \brief Initialize NBC structures used in call of persistent Scan using NBC interface
 *  \param sendbuf Adress of the pointer to the buffer used to send data
 *  \param recvbuf Adress of the pointer to the buffer used to receive data
 *  \param count Number of elements in sendbuf
 *  \param datatype Type of the data elements in sendbuf
 *  \param op Reduction operation
 *  \param comm Target communicator
 *  \param Handle
 */
int NBC_Iexscan_init( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle )
{
	int rank, p, res;
	MPI_Aint ext;
	NBC_Schedule *schedule;

	res = NBC_Init_handle( handle, comm, MPC_ISCAN_TAG );
	if ( res != NBC_OK )
	{
		printf( "Error in NBC_Init_handle(%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_rank( comm, &rank );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_rank() (%i)\n", res );
		return res;
	}
	res = _mpc_cl_comm_size( comm, &p );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Comm_size() (%i)\n", res );
		return res;
	}
	res = PMPI_Type_extent( datatype, &ext );
	if ( MPI_SUCCESS != res )
	{
		printf( "MPI Error in MPI_Type_extent() (%i)\n", res );
		return res;
	}

	handle->tmpbuf = sctk_malloc( ext * count );
	if ( handle->tmpbuf == NULL )
	{
		printf( "Error in sctk_malloc()\n" );
		return NBC_OOR;
	}

	schedule = (NBC_Schedule *) sctk_malloc( sizeof( NBC_Schedule ) );
	if ( NULL == schedule )
	{
		printf( "Error in sctk_malloc()\n" );
		return res;
	}

	int alloc_size = sizeof( int ) + sizeof( int ) + sizeof( char );

	if ( rank != 0 )
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						sizeof( int ) + sizeof( char ) ) +
					  ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) +
						sizeof( int ) + sizeof( char ) );
	if ( rank != p - 1 )
		alloc_size += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	*schedule = sctk_malloc( alloc_size );
	*(int *) *schedule = alloc_size;

	int pos = 0;
	int pos_rounds = pos;
	pos += sizeof( int );
	if ( rank != 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_recv_pos( pos, recvbuf, 0, count, datatype, rank - 1,
								  schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_recv() (%i)\n", res );
			return res;
		}
		/* we have to wait until we have the data */
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );

		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
		/* perform the reduce in my local buffer */

		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_op_pos( pos, 0, 1, (void *) sendbuf, 0, recvbuf, 0, count,
								datatype, op, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_op() (%i)\n", res );
			return res;
		}
		/* this cannot be done until handle->tmpbuf is unused :-( */
		res = NBC_Sched_barrier_pos( pos, schedule );
		pos += sizeof( char );
		pos_rounds = pos;
		pos += sizeof( int );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_barrier() (%i)\n", res );
			return res;
		}
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 0;
	}
	if ( rank != 0 && rank != p - 1 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_send_pos( pos, 0, 1, count, datatype, rank + 1,
								  schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}
	}
	if ( rank == 0 )
	{
		*(int *) ( (char *) *schedule + sizeof( int ) + pos_rounds ) = 1;
		res = NBC_Sched_send_pos( pos, sendbuf, 0, count, datatype,
								  rank + 1, schedule );
		pos += ( sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
		if ( NBC_OK != res )
		{
			sctk_free( handle->tmpbuf );
			printf( "Error in NBC_Sched_send() (%i)\n", res );
			return res;
		}
	}

	res = NBC_Sched_commit_pos( schedule );
	if ( NBC_OK != res )
	{
		sctk_free( handle->tmpbuf );
		printf( "Error in NBC_Sched_commit() (%i)\n", res );
		return res;
	}

	handle->schedule = schedule;

	/* tmpbuf is freed with the handle */
	return MPI_SUCCESS;
}

#if 0
static int JJ_NBC_Iexscan(void *sendbuf, void *recvbuf, int count,
                          MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                          NBC_Handle *handle)
                          {
  int rank, p, res;
  MPI_Aint ext;
  NBC_Schedule *schedule;
  char inplace;

  NBC_IN_PLACE(sendbuf, recvbuf, inplace);

  res = NBC_Init_handle(handle, comm, MPC_IEXSCAN_TAG);
  if (res != NBC_OK)
  {
    printf("Error in NBC_Init_handle(%i)\n", res);
    return res;
  }
  res = _mpc_cl_comm_rank(comm, &rank);
  if (MPI_SUCCESS != res)
  {
    printf("MPI Error in MPI_Comm_rank() (%i)\n", res);
    return res;
  }
  res = _mpc_cl_comm_size(comm, &p);
  if (MPI_SUCCESS != res)
  {
    printf("MPI Error in MPI_Comm_size() (%i)\n", res);
    return res;
  }
  res = PMPI_Type_extent(datatype, &ext);
  if (MPI_SUCCESS != res)
  {
    printf("MPI Error in MPI_Type_extent() (%i)\n", res);
    return res;
  }

  handle->tmpbuf = sctk_malloc(ext * count);
  if (handle->tmpbuf == NULL)
  {
    printf("Error in sctk_malloc()\n");
    return NBC_OOR;
  }

  if ((rank == 0) && !inplace)
  {
    /* copy data to receivebuf */
    res = NBC_Copy(sendbuf, count, datatype, recvbuf, count, datatype, comm);
    if (NBC_OK != res)
    {
      printf("Error in NBC_Copy() (%i)\n", res);
      return res;
    }
  }

  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));
  if (NULL == schedule)
  {
    printf("Error in sctk_malloc()\n");
    return res;
  }

  int alloc_size = sizeof(int) + sizeof(int) + sizeof(char);

  if (rank != 0)
    alloc_size += (sizeof(NBC_Args) + sizeof(NBC_Fn_type) + sizeof(int) +
                   sizeof(char)) +
                  (sizeof(NBC_Args) + sizeof(NBC_Fn_type) + sizeof(int) +
                   sizeof(char));
  if (rank != p - 1)
    alloc_size += (sizeof(NBC_Args) + sizeof(NBC_Fn_type));

  *schedule = sctk_malloc(alloc_size);
  *(int *)*schedule = alloc_size;

  int pos = 0;
  if (rank != 0)
  {
    *(int *)((char *)*schedule + sizeof(int) + pos) = 1;
    pos += sizeof(int);
    res = NBC_Sched_recv_pos(pos, recvbuf, 0, count, datatype, rank - 1,
                             schedule);
    pos += (sizeof(NBC_Args) + sizeof(NBC_Fn_type));
    if (NBC_OK != res)
    {
      sctk_free(handle->tmpbuf);
      printf("Error in NBC_Sched_recv() (%i)\n", res);
      return res;
    }
    /* we have to wait until we have the data */
    res = NBC_Sched_barrier_pos(pos, schedule);
    pos += sizeof(char);

    if (NBC_OK != res)
    {
      sctk_free(handle->tmpbuf);
      printf("Error in NBC_Sched_barrier() (%i)\n", res);
      return res;
    }
    /* perform the reduce in my local buffer */
    *(int *)((char *)*schedule + sizeof(int) + pos) = 1;
    pos += sizeof(int);
    res = NBC_Sched_op_pos(pos, 0, 1, sendbuf, 0, recvbuf, 0, count, datatype,
                           op, schedule);
    pos += (sizeof(NBC_Args) + sizeof(NBC_Fn_type));
    if (NBC_OK != res)
    {
      sctk_free(handle->tmpbuf);
      printf("Error in NBC_Sched_op() (%i)\n", res);
      return res;
    }
    /* this cannot be done until handle->tmpbuf is unused :-( */
    res = NBC_Sched_barrier_pos(pos, schedule);
    pos += sizeof(char);
    if (NBC_OK != res)
    {
      sctk_free(handle->tmpbuf);
      printf("Error in NBC_Sched_barrier() (%i)\n", res);
      return res;
    }
  }
  if (rank != 0 && rank != p - 1)
  {
    *(int *)((char *)*schedule + sizeof(int) + pos) = 1;
    pos += sizeof(int);
    res = NBC_Sched_send_pos(pos, 0, 1, count, datatype, rank + 1, schedule);
    pos += (sizeof(NBC_Args) + sizeof(NBC_Fn_type));
    if (NBC_OK != res)
    {
      sctk_free(handle->tmpbuf);
      printf("Error in NBC_Sched_send() (%i)\n", res);
      return res;
    }
  }
  if (rank == 0)
  {
    *(int *)((char *)*schedule + sizeof(int) + pos) = 1;
    pos += sizeof(int);
    res = NBC_Sched_send_pos(pos, recvbuf, 0, count, datatype, rank + 1,
                             schedule);
    pos += (sizeof(NBC_Args) + sizeof(NBC_Fn_type));
    if (NBC_OK != res)
    {
      sctk_free(handle->tmpbuf);
      printf("Error in NBC_Sched_send() (%i)\n", res);
      return res;
    }
  }

  res = NBC_Sched_commit_pos(schedule);
  if (NBC_OK != res)
  {
    sctk_free(handle->tmpbuf);
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
  {
    sctk_free(handle->tmpbuf);
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  /* tmpbuf is freed with the handle */
  return NBC_OK;
}
#endif

/********************************************************************* *
 * The previous code was a modified version of three functions from		*
 * the libNBC. It aimed to support the three MPI-3 Non-Blocking				*
 * Collective functions that are not initially present in the libNBC.	*
 * *********************************************************************/

/********************************************************************* *
 * The Following code comes from the libNBC, a library for the support *
 * of MPI-3 Non-Blocking Collective. The code was modified to ensure   *
 * compatibility with the MPC framework.                               *
 * *********************************************************************/

/******* *
 * NBC.C *
 * *******/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *                    rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

int __mpc_cl_egreq_progress_poll_id( int id );

void *NBC_Pthread_func( __UNUSED__ void *ptr )
{
	struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific;

	struct sctk_list_elem *list_handles;
	mpc_thread_mutex_t *lock;
	task_specific = mpc_cl_per_mpi_process_ctx_get();

	task_specific->mpc_mpi_data->nbc_initialized_per_task = 1;

	while ( 1 )
	{
		int cpt = 1;
		int size = 10;

		MPI_Request *requests;
		int *requests_locations;
		NBC_Handle **requests_handles;

		requests = (MPI_Request *) sctk_malloc( sizeof( MPI_Request ) * size );
		requests_locations = (int *) sctk_malloc( sizeof( int ) * size );
		requests_handles = (NBC_Handle **) sctk_malloc( sizeof( NBC_Handle * ) * size );

		/* re-compile list of requests */

		lock = &( task_specific->mpc_mpi_data->list_handles_lock );
		mpc_thread_sem_wait( &( task_specific->mpc_mpi_data->pending_req ) );
		struct sctk_list_elem *iter, *elem_tmp;

		NBC_Handle *tmp_handle;

		mpc_thread_mutex_lock( lock );
		list_handles = task_specific->mpc_mpi_data->NBC_Pthread_handles;

		DL_FOREACH_SAFE( list_handles, iter, elem_tmp )
		{
			tmp_handle = (NBC_Handle *) iter->elem;
			/* if the handle is not done but there are no requests, it must be
			 * a new one - start first round */
			if ( tmp_handle->req_count == 0 )
			{
				if ( tmp_handle->is_persistent )
				{
					NBC_Start_round_persistent( tmp_handle );
				}
				else
				{
					NBC_Start_round( tmp_handle );
				}
			}
			int i;
			for ( i = 0; i < tmp_handle->req_count; i++ )
			{
				if ( tmp_handle->req_array[i] != MPI_REQUEST_NULL )
				{
					if ( cpt == size )
					{
						size = size * 2;
						requests = (MPI_Request *) sctk_realloc( requests, sizeof( MPI_Request ) * size );
						requests_locations = (int *) sctk_realloc( requests_locations, sizeof( int ) * size );
						requests_handles = (NBC_Handle **) sctk_realloc( requests_handles, sizeof( NBC_Handle * ) * size );
					}
					requests[cpt] = tmp_handle->req_array[i];
					requests_locations[cpt] = i;
					requests_handles[cpt] = tmp_handle;

					cpt = cpt + 1;
				}
			}
		}

		mpc_thread_mutex_unlock( lock );

		int retidx = 0, res;
		NBC_DEBUG( 10, "waiting for %i elements\n", cpt );
		res = PMPI_Waitany( cpt, requests, &retidx, MPI_STATUS_IGNORE );
		NBC_DEBUG( 10, "elements %d is finished", retidx );
		if ( res != MPI_SUCCESS )
		{
			printf( "Error %i in MPI_Waitany()\n", res );
		}
		requests_handles[retidx]->req_array[requests_locations[retidx]] = MPI_REQUEST_NULL;
		NBC_Progress( requests_handles[retidx] );

		sctk_free( requests );
		sctk_free( requests_locations );
		sctk_free( requests_handles );
		if ( task_specific->mpc_mpi_data->nbc_initialized_per_task == -1 )
		{
			mpc_thread_exit( 0 );
		}
		mpc_thread_yield();
	}
}

/* allocates a new schedule array */
static inline int NBC_Sched_create( NBC_Schedule *schedule )
{

	*schedule = sctk_malloc( 2 * sizeof( int ) );
	if ( *schedule == NULL )
	{
		return NBC_OOR;
	}
	*(int *) *schedule = 2 * sizeof( int );
	*( ( (int *) *schedule ) + 1 ) = 0;

	return NBC_OK;
}

/* this function puts a send into the schedule */
static inline int NBC_Sched_send( void *buf, char tmpbuf, int count, MPI_Datatype datatype, int dest, NBC_Schedule *schedule )
{
	int size;
	NBC_Args *send_args;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + size ) = SEND;

	/* store the passed arguments */
	send_args = (NBC_Args *) ( (char *) *schedule + size + sizeof( NBC_Fn_type ) );
	send_args->buf = buf;
	send_args->tmpbuf = tmpbuf;
	send_args->count = count;
	send_args->datatype = datatype;
	send_args->dest = dest;
	send_args->comm = MPI_COMM_NULL;

	/* increase number of elements in schedule */
	NBC_INC_NUM_ROUND( *schedule );
	// NBC_DEBUG(10, "adding send - ends at byte %i\n", (int)(size+sizeof(NBC_Args)+sizeof(NBC_Fn_type)));

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	return NBC_OK;
}

static inline int NBC_Sched_send_pos( int pos, const void *buf, char tmpbuf, int count, MPI_Datatype datatype, int dest, NBC_Schedule *schedule )
{

	NBC_Args *send_args;

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + sizeof( int ) + pos ) = SEND;

	/* store the passed arguments */
	send_args = (NBC_Args *) ( (char *) *schedule + sizeof( int ) + pos +
							   sizeof( NBC_Fn_type ) );
	send_args->buf = (void *) buf;
	send_args->tmpbuf = tmpbuf;
	send_args->count = count;
	send_args->datatype = datatype;
	send_args->dest = dest;
	send_args->comm = MPI_COMM_NULL;

	return NBC_OK;
}

/* this function puts a receive into the schedule */
static inline int NBC_Sched_recv( void *buf, char tmpbuf, int count, MPI_Datatype datatype, int source, NBC_Schedule *schedule )
{
	int size;
	NBC_Args *recv_args;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + size ) = RECV;

	/* store the passed arguments */
	recv_args = (NBC_Args *) ( (char *) *schedule + size + sizeof( NBC_Fn_type ) );
	recv_args->buf = buf;
	recv_args->tmpbuf = tmpbuf;
	recv_args->count = count;
	recv_args->datatype = datatype;
	recv_args->source = source;
	recv_args->comm = MPI_COMM_NULL;

	/* increase number of elements in schedule */
	NBC_INC_NUM_ROUND( *schedule );

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	return NBC_OK;
}

static inline int NBC_Sched_recv_pos( int pos, void *buf, char tmpbuf, int count, MPI_Datatype datatype, int source, NBC_Schedule *schedule )
{

	NBC_Args *recv_args;

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + sizeof( int ) + pos ) = RECV;

	/* store the passed arguments */
	recv_args = (NBC_Args *) ( (char *) *schedule + sizeof( int ) + pos +
							   sizeof( NBC_Fn_type ) );
	recv_args->buf = buf;
	recv_args->tmpbuf = tmpbuf;
	recv_args->count = count;
	recv_args->datatype = datatype;
	recv_args->source = source;
	recv_args->comm = MPI_COMM_NULL;

	return NBC_OK;
}

/* this function puts an operation into the schedule */
static inline int NBC_Sched_op( void *buf3, char tmpbuf3, void *buf1, char tmpbuf1, void *buf2, char tmpbuf2, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule )
{
	int size;
	NBC_Args *op_args;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + size ) = OP;

	/* store the passed arguments */
	op_args = (NBC_Args *) ( (char *) *schedule + size + sizeof( NBC_Fn_type ) );
	op_args->buf1 = buf1;
	op_args->buf2 = buf2;
	op_args->buf3 = buf3;
	op_args->tmpbuf1 = tmpbuf1;
	op_args->tmpbuf2 = tmpbuf2;
	op_args->tmpbuf3 = tmpbuf3;
	op_args->count = count;
	op_args->op = op;
	op_args->datatype = datatype;

	/* increase number of elements in schedule */
	NBC_INC_NUM_ROUND( *schedule );

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	return NBC_OK;
}

static inline int NBC_Sched_op_pos( int pos, void *buf3, char tmpbuf3, void *buf1, char tmpbuf1, void *buf2, char tmpbuf2, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule )
{

	NBC_Args *op_args;

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + sizeof( int ) + pos ) = OP;

	/* store the passed arguments */
	op_args = (NBC_Args *) ( (char *) *schedule + sizeof( int ) + pos +
							 sizeof( NBC_Fn_type ) );
	op_args->buf1 = buf1;
	op_args->buf2 = buf2;
	op_args->buf3 = buf3;
	op_args->tmpbuf1 = tmpbuf1;
	op_args->tmpbuf2 = tmpbuf2;
	op_args->tmpbuf3 = tmpbuf3;
	op_args->count = count;
	op_args->op = op;
	op_args->datatype = datatype;

	return NBC_OK;
}

/* this function puts a copy into the schedule */
static inline int NBC_Sched_copy( void *src, char tmpsrc, int srccount, MPI_Datatype srctype, void *tgt, char tmptgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule )
{
	int size;
	NBC_Args *copy_args;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + size ) = COPY;

	/* store the passed arguments */
	copy_args = (NBC_Args *) ( (char *) *schedule + size + sizeof( NBC_Fn_type ) );
	copy_args->src = src;
	copy_args->tmpsrc = tmpsrc;
	copy_args->srccount = srccount;
	copy_args->srctype = srctype;
	copy_args->tgt = tgt;
	copy_args->tmptgt = tmptgt;
	copy_args->tgtcount = tgtcount;
	copy_args->tgttype = tgttype;
	copy_args->comm = MPI_COMM_NULL;

	/* increase number of elements in schedule */
	NBC_INC_NUM_ROUND( *schedule );

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	return NBC_OK;
}

/* this function puts a copy into the schedule */
static inline int NBC_Sched_copy_pos( int pos, void *src, char tmpsrc,
									  int srccount, MPI_Datatype srctype,
									  void *tgt, char tmptgt, int tgtcount,
									  MPI_Datatype tgttype,
									  NBC_Schedule *schedule )
{

	NBC_Args *copy_args;

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + sizeof( int ) + pos ) = COPY;

	/* store the passed arguments */
	copy_args = (NBC_Args *) ( (char *) *schedule + sizeof( int ) + pos +
							   sizeof( NBC_Fn_type ) );
	copy_args->src = src;
	copy_args->tmpsrc = tmpsrc;
	copy_args->srccount = srccount;
	copy_args->srctype = srctype;
	copy_args->tgt = tgt;
	copy_args->tmptgt = tmptgt;
	copy_args->tgtcount = tgtcount;
	copy_args->tgttype = tgttype;
	copy_args->comm = MPI_COMM_NULL;

	return NBC_OK;
}

/* this function puts a unpack into the schedule */
static inline int NBC_Sched_unpack( void *inbuf, char tmpinbuf, int count, MPI_Datatype datatype, void *outbuf, char tmpoutbuf, NBC_Schedule *schedule )
{
	int size;
	NBC_Args *unpack_args;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* adjust the function type */
	*(NBC_Fn_type *) ( (char *) *schedule + size ) = UNPACK;

	/* store the passed arguments */
	unpack_args = (NBC_Args *) ( (char *) *schedule + size + sizeof( NBC_Fn_type ) );
	unpack_args->inbuf = inbuf;
	unpack_args->tmpinbuf = tmpinbuf;
	unpack_args->count = count;
	unpack_args->datatype = datatype;
	unpack_args->outbuf = outbuf;
	unpack_args->tmpoutbuf = tmpoutbuf;
	unpack_args->comm = MPI_COMM_NULL;

	/* increase number of elements in schedule */
	NBC_INC_NUM_ROUND( *schedule );

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( NBC_Args ) + sizeof( NBC_Fn_type ) );

	return NBC_OK;
}

/* this function ends a round of a schedule */
static inline int NBC_Sched_barrier( NBC_Schedule *schedule )
{
	int size;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( char ) + sizeof( int ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* add the barrier char (1) because another round follows */
	*(char *) ( (char *) *schedule + size ) = 1;

	/* set round count elements = 0 for new round */
	*(int *) ( (char *) *schedule + size + sizeof( char ) ) = 0;

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( char ) + sizeof( int ) );

	return NBC_OK;
}

static inline int NBC_Sched_barrier_pos( int pos, NBC_Schedule *schedule )
{

	/* add the barrier char (1) because another round follows */
	*(char *) ( (char *) *schedule + sizeof( int ) + pos ) = 1;

	return NBC_OK;
}

/* this function ends a schedule */
static inline int NBC_Sched_commit( NBC_Schedule *schedule )
{
	int size;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );
	*schedule = (NBC_Schedule) sctk_realloc( *schedule, size + sizeof( char ) );
	if ( *schedule == NULL )
	{
		printf( "Error in sctk_realloc()\n" );
		return NBC_OOR;
	}

	/* add the barrier char (0) because this is the last round */
	*(char *) ( (char *) *schedule + size ) = 0;

	/* increase size of schedule */
	NBC_INC_SIZE( *schedule, sizeof( char ) );

	return NBC_OK;
}

static inline int NBC_Sched_commit_pos( NBC_Schedule *schedule )
{
	int size;

	/* get size of actual schedule */
	NBC_GET_SIZE( *schedule, size );

	/* add the barrier char (0) because this is the last round */
	*(char *) ( (char *) *schedule + size - sizeof( char ) ) = 0;

	return NBC_OK;
}

/* finishes a request
 *
 * to be called *only* from the progress thread !!! */
static inline int NBC_Free( NBC_Handle *handle )
{
	int use_progress_thread = 0;
	use_progress_thread = _mpc_mpi_config()->nbc.progress_thread;

	if ( use_progress_thread == 1 )
	{

		struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific;
		struct sctk_list_elem *list_handles;
		mpc_thread_mutex_t *lock;
		task_specific = mpc_cl_per_mpi_process_ctx_get();

		lock = &( task_specific->mpc_mpi_data->list_handles_lock );

		struct sctk_list_elem *elem_tmp;

		mpc_thread_mutex_lock( lock );
		list_handles = task_specific->mpc_mpi_data->NBC_Pthread_handles;

		DL_SEARCH_SCALAR( list_handles, elem_tmp, elem, handle );
		DL_DELETE( task_specific->mpc_mpi_data->NBC_Pthread_handles, elem_tmp );
		task_specific->mpc_mpi_data->NBC_Pthread_nb--;

		if ( mpc_common_get_flags()->new_scheduler_engine_enabled )
		{
#ifdef SCTK_DEBUG_SCHEDULER
			{
				char hostname[128];
				char hostname2[128];
				char current_vp[5];
				gethostname( hostname, 128 );
				strncpy( hostname2, hostname, 128 );
				sprintf( current_vp, "_%03d", mpc_topology_get_pu() );
				strcat( hostname2, current_vp );

				FILE *fd = fopen( hostname2, "a" );
				fprintf( fd, "NBC_DELETE %p %d\n", elem_tmp,
						 task_specific->mpc_mpi_data->NBC_Pthread_nb );
				fflush( fd );
				fclose( fd );
			}
#endif
			mpc_threads_generic_scheduler_decrease_prio();
		}

		mpc_thread_mutex_unlock( lock );

		mpc_thread_mutex_lock( &handle->lock );
	}
	if ( handle->schedule != NULL )
	{

		if ( handle->tmpbuf != NULL )
		{
			sctk_free( (void *) ( handle->tmpbuf ) );
			handle->tmpbuf = NULL;
		}
		if ( handle->req_array != NULL )
		{
			sctk_free( (void *) ( handle->req_array ) );
			handle->req_array = NULL;
			handle->req_count = 0;
		}

		/* free schedule */
		sctk_free( (void *) *( handle->schedule ) );
		handle->schedule = NULL;
	}
	if ( use_progress_thread == 1 )
	{
		mpc_thread_mutex_unlock( &handle->lock );
	}

	if ( NULL != handle->tmpbuf )
	{
		sctk_free( (void *) handle->tmpbuf );
		handle->tmpbuf = NULL;
	}

	if ( use_progress_thread == 1 )
	{
		if ( mpc_thread_sem_post( &handle->semid ) != 0 )
		{
			perror( "mpc_thread_sem_post()" );
		}
	}

	return NBC_OK;
}

static inline int __NBC_Start_round( NBC_Handle *handle, int depth );
static inline int __NBC_Start_round_persistent( NBC_Handle *handle, int depth );

/* progresses a request
 *
 * to be called *only* from the progress thread !!! */
static inline int __NBC_Progress( NBC_Handle *handle, int depth )
{
	int res, ret = NBC_CONTINUE;
	long size;
	char *delim;

	/* the handle is done if there is no schedule attached */
	if ( handle->schedule != NULL )
	{
		if ( handle->is_persistent )
		{
			if ( ( /*handle->req_count*/ handle->req_count_persistent[handle->num_rounds] > 0 ) && ( &handle->req_array[handle->array_offset] /*handle->req_array*/ != NULL ) )
			{
				// sctk_debug("INSIDE THE PROGRESS THREAD");

				MPI_request_struct_t *requests = __sctk_internal_get_MPC_requests();
				__sctk_convert_mpc_request_internal( &handle->req_array[handle->array_offset],
													 requests );
				res = PMPI_Waitall( handle->req_count_persistent[handle->num_rounds], &handle->req_array[handle->array_offset],
									/*&flag,*/ MPI_STATUSES_IGNORE );
				if ( res != MPI_SUCCESS )
				{
					printf( "MPI Error in MPI_Testall() (%i)\n", res );
					ret = res;
					goto error;
				}
			}
		}
		else
		{
			if ( ( handle->req_count > 0 ) && ( handle->req_array != NULL ) )
			{
				// sctk_debug("INSIDE THE PROGRESS THREAD");
				res = PMPI_Waitall( handle->req_count, handle->req_array,
									/*&flag,*/ MPI_STATUSES_IGNORE );
				if ( res != MPI_SUCCESS )
				{
					printf( "MPI Error in MPI_Testall() (%i)\n", res );
					ret = res;
					goto error;
				}
			}
		}
		/* a round is finished */
		/* adjust delim to start of current round */
		delim = (char *) *handle->schedule + handle->row_offset;
		NBC_GET_ROUND_SIZE( delim, size );
		/* adjust delim to end of current round -> delimiter */
		delim = delim + size;

		if ( *delim == 0 )
		{
			/* this was the last round - we're done */

			if ( !handle->is_persistent )
			{
				res = NBC_Free( handle );
				if ( ( NBC_OK != res ) )
				{
					printf( "Error in NBC_Free() (%i)\n", res );
					ret = res;
					goto error;
				}
			}
			if ( handle->is_persistent == 1 )
			{
				handle->is_persistent = 2;
			}
			handle->num_rounds = 0;
			handle->array_offset = 0;
			return NBC_OK;
		}
		else
		{
			/* move delim to start of next round */
			delim = delim + 1;
			/* initializing handle for new virgin round */
			handle->row_offset = (long) delim - (long) *handle->schedule;
			/* kick it off */
			if ( handle->is_persistent )
			{
				handle->array_offset += handle->req_count_persistent[handle->num_rounds];
				handle->num_rounds++;
				res = NBC_Start_round_persistent( handle );
			}
			else
			{
				res = __NBC_Start_round( handle, depth );
			}
			if ( NBC_OK != res )
			{
				printf( "Error in NBC_Start_round() (%i)\n", res );
				ret = res;
				goto error;
			}
		}
	}
	else
	{
		ret = NBC_OK;
	}

error:
	return ret;
}

static inline int NBC_Progress( NBC_Handle *handle )
{
	return __NBC_Progress( handle, 0 );
}

static inline int __NBC_Start_round_persistent( NBC_Handle *handle, int depth )
{
	int *numptr; /* number of operations */
	int i, res, ret = NBC_OK;
	NBC_Fn_type *typeptr;
	NBC_Args *sendargs;
	NBC_Args *recvargs;
	NBC_Args *opargs;
	NBC_Args *copyargs;
	NBC_Args *unpackargs;
	NBC_Schedule myschedule;
	void *buf1, *buf2, *buf3;
	MPI_Comm comm;

	MPI_request_struct_t *requests = __sctk_internal_get_MPC_requests();

	/* get schedule address */
	myschedule = (NBC_Schedule *) ( (char *) *handle->schedule + handle->row_offset );

	// mpc_common_debug_error("=> %p + %d", *handle->schedule,  handle->row_offset);

	numptr = (int *) myschedule;

	int req_cpt = 0;
	int old_req_count = handle->req_count;

	typeptr = (NBC_Fn_type *) ( numptr + 1 );
	if ( handle->is_persistent != 2 )
	{

		for ( i = 0; i < *numptr; i++ )
		{
			switch ( *typeptr )
			{
				case SEND:
					req_cpt++;
					break;
				case RECV:
					req_cpt++;
					break;
				case OP:
				case COPY:
				case MOVE:
				case UNPACK:
					break;
				default:
					printf( "--NBC_Start_round: bad type %li at offset %li rank %d\n", (long) *typeptr, (long) typeptr - (long) myschedule, mpc_common_get_task_rank() );
					ret = NBC_BAD_SCHED;
					goto error;
			}
			typeptr = (NBC_Fn_type *) ( ( (NBC_Args *) typeptr ) + 1 );
			/* increase ptr by size of fn_type enum */
			typeptr = (NBC_Fn_type *) ( (NBC_Fn_type *) typeptr + 1 );
		}

		handle->req_count += req_cpt;

		if ( old_req_count != handle->req_count )
		{
			if ( handle->actual_req_count <= handle->req_count )
			{
				if ( !handle->actual_req_count || req_cpt > 10 )
				{
					handle->actual_req_count = handle->req_count;
				}
				else
				{
					handle->actual_req_count += 10;
				}

				handle->req_array = (MPI_Request *) sctk_realloc( handle->req_array, ( handle->actual_req_count ) * sizeof( MPI_Request ) );

				for ( i = old_req_count; i < handle->actual_req_count; i++ )
					handle->req_array[i] = MPI_REQUEST_NULL;
			}
		}
	}

	req_cpt = 0;

	/* typeptr is increased by sizeof(int) bytes to point to type */
	typeptr = (NBC_Fn_type *) ( numptr + 1 );

	for ( i = 0; i < *numptr; i++ )
	{
		/* go sizeof op-data forward */
		switch ( *typeptr )
		{
			case SEND:
				sendargs = (NBC_Args *) ( typeptr + 1 );

				/* get an additional request */
				req_cpt++;
				/* get buffer */
				if ( sendargs->tmpbuf )
					buf1 = (char *) handle->tmpbuf + (long) sendargs->buf;
				else
					buf1 = sendargs->buf;

				if ( sendargs->comm == MPI_COMM_NULL )
					comm = handle->comm;
				else
					comm = sendargs->comm;

				NBC_CHECK_NULL( handle->req_array );
				if ( handle->is_persistent != 2 )
				{
					res = _mpc_cl_isend( buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, comm, __sctk_new_mpc_request( handle->req_array + ( old_req_count + req_cpt - 1 ), requests ) );
					/*mark new intermediate nbc request to keep it as long as collective isn't free*/
					requests->tab[*( handle->req_array + ( old_req_count + req_cpt - 1 ) )]->is_persistent = 1;
					if ( handle->is_persistent == 1 )
					{
						requests->tab[*( handle->req_array + req_cpt - 1 )]->is_persistent = 2;
						requests->tab[*( handle->req_array + req_cpt - 1 )]->used = 1;
						requests->tab[*( handle->req_array + req_cpt - 1 )]->nbc_handle.schedule = handle->schedule;
					}
				}
				else
				{
					requests->tab[*( handle->req_array + req_cpt - 1 )]->is_persistent = 2;
					requests->tab[*( handle->req_array + req_cpt - 1 )]->used = 1;
					requests->tab[*( handle->req_array + req_cpt - 1 )]->nbc_handle.schedule = handle->schedule;
					res = _mpc_cl_isend( buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, comm, &( ( requests->tab[(int) *( handle->req_array + ( handle->array_offset + req_cpt - 1 ) )] )->req ) );
				}
				if ( MPI_SUCCESS != res )
				{
					ret = res;
					goto error;
				}
				break;
			case RECV:
				recvargs = (NBC_Args *) ( typeptr + 1 );

				/* get an additional request - TODO: req_count NOT thread safe */
				req_cpt++;
				/* get buffer */
				if ( recvargs->tmpbuf )
				{
					buf1 = (char *) handle->tmpbuf + (long) recvargs->buf;
				}
				else
				{
					buf1 = recvargs->buf;
				}

				if ( recvargs->comm == MPI_COMM_NULL )
					comm = handle->comm;
				else
					comm = recvargs->comm;

				NBC_CHECK_NULL( handle->req_array );
				if ( handle->is_persistent != 2 )
				{
					res = _mpc_cl_irecv( buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, comm, __sctk_new_mpc_request( handle->req_array + ( old_req_count + req_cpt - 1 ), requests ) );
					/*mark new intermediate nbc request to keep it as long as collective isn't free*/
					requests->tab[*( handle->req_array + ( old_req_count + req_cpt - 1 ) )]->is_persistent = 1;
					if ( handle->is_persistent == 1 )
					{
						requests->tab[*( handle->req_array + handle->array_offset + req_cpt - 1 )]->is_persistent = 2;
						requests->tab[*( handle->req_array + handle->array_offset + req_cpt - 1 )]->used = 1;
						requests->tab[*( handle->req_array + handle->array_offset + req_cpt - 1 )]->nbc_handle.schedule = handle->schedule;
					}
				}
				else
				{
					requests->tab[*( handle->req_array + handle->array_offset + req_cpt - 1 )]->is_persistent = 2;
					requests->tab[*( handle->req_array + handle->array_offset + req_cpt - 1 )]->used = 1;
					requests->tab[*( handle->req_array + handle->array_offset + req_cpt - 1 )]->nbc_handle.schedule = handle->schedule;
					res = _mpc_cl_irecv( buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, comm, &( ( requests->tab[(int) *( handle->req_array + ( handle->array_offset + req_cpt - 1 ) )] )->req ) );
				}

				if ( MPI_SUCCESS != res )
				{
					printf( "Error in MPI_Irecv(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long) buf1, recvargs->count, (unsigned long) recvargs->datatype, recvargs->source, handle->tag, (unsigned long) handle->mycomm, res );
					ret = res;
					goto error;
				}
				break;
			case OP:
				opargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( opargs->tmpbuf1 )
					buf1 = (char *) handle->tmpbuf + (long) opargs->buf1;
				else
					buf1 = opargs->buf1;
				if ( opargs->tmpbuf2 )
					buf2 = (char *) handle->tmpbuf + (long) opargs->buf2;
				else
					buf2 = opargs->buf2;
				if ( opargs->tmpbuf3 )
					buf3 = (char *) handle->tmpbuf + (long) opargs->buf3;
				else
					buf3 = opargs->buf3;
				res = NBC_Operation( buf3, buf1, buf2, opargs->op, opargs->datatype, opargs->count );
				if ( res != NBC_OK )
				{
					printf( "NBC_Operation() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			case COPY:
				copyargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( copyargs->tmpsrc )
					buf1 = (char *) handle->tmpbuf + (long) copyargs->src;
				else
					buf1 = copyargs->src;

				if ( copyargs->comm == MPI_COMM_NULL )
					comm = handle->comm;
				else
					comm = copyargs->comm;

				if ( copyargs->tmptgt )
					buf2 = (char *) handle->tmpbuf + (long) copyargs->tgt;
				else
					buf2 = copyargs->tgt;

				res = NBC_Copy( buf1, copyargs->srccount, copyargs->srctype, buf2, copyargs->tgtcount, copyargs->tgttype, comm );
				if ( res != NBC_OK )
				{
					printf( "NBC_Copy() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			case MOVE:
				copyargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( copyargs->tmpsrc )
					buf1 = (char *) handle->tmpbuf + (long) copyargs->src;
				else
					buf1 = copyargs->src;

				if ( copyargs->comm == MPI_COMM_NULL )
					comm = handle->comm;
				else
					comm = copyargs->comm;

				if ( copyargs->tmptgt )
					buf2 = (char *) handle->tmpbuf + (long) copyargs->tgt;
				else
					buf2 = copyargs->tgt;

				res = NBC_Move( buf1, copyargs->srccount, copyargs->srctype, buf2, copyargs->tgtcount, copyargs->tgttype, comm );
				if ( res != NBC_OK )
				{
					printf( "NBC_Copy() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			case UNPACK:
				unpackargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( unpackargs->tmpinbuf )
					buf1 = (char *) handle->tmpbuf + (long) unpackargs->inbuf;
				else
					buf1 = unpackargs->outbuf;

				if ( unpackargs->comm == MPI_COMM_NULL )
					comm = handle->comm;
				else
					comm = unpackargs->comm;

				if ( unpackargs->tmpoutbuf )
					buf2 = (char *) handle->tmpbuf + (long) unpackargs->outbuf;
				else
					buf2 = unpackargs->outbuf;

				res = NBC_Unpack( buf1, unpackargs->count, unpackargs->datatype, buf2, comm );
				if ( res != NBC_OK )
				{
					printf( "NBC_Unpack() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			default:
				printf( "NBC_Start_round: bad type %li at offset %li rank %d\n", (long) *typeptr, (long) typeptr - (long) myschedule, mpc_common_get_task_rank() );
				ret = NBC_BAD_SCHED;
				goto error;
		}
		/* Move forward of both ARG and  TYPE size */
		typeptr = (NBC_Fn_type *) ( ( (NBC_Args *) typeptr ) + 1 ) + 1;
	}

	if ( handle->is_persistent == 1 )
	{
		if ( !handle->num_rounds )
		{
			handle->req_count_persistent = (int *) sctk_malloc( sizeof( int ) );
		}
		else
		{
			//            volatile int *tmp_reqs = handle->req_count_persistent;
			//            handle->req_count_persistent = (volatile int*)sctk_malloc((handle->num_rounds+1)*sizeof(volatile int));
			//            handle->req_count_persistent[handle->num_rounds] = req_cpt;
			//            for(i=0; i<handle->num_rounds; i++)
			//            {
			//                handle->req_count_persistent[i] = tmp_reqs[i];
			//            }
			//            sctk_free(tmp_reqs);

			handle->req_count_persistent = (int *) sctk_realloc( (int *) ( handle->req_count_persistent ), ( handle->num_rounds + 1 ) * sizeof( volatile int ) );
			handle->req_count_persistent[handle->num_rounds] = req_cpt;
		}
		handle->req_count_persistent[handle->num_rounds] = req_cpt;
	}

	/* check if we can make progress - not in the first round, this allows us to leave the
	 * initialization faster and to reach more overlap
	 *
	 * threaded case: calling progress in the first round can lead to a
	 * deadlock if NBC_Free is called in this round :-( */
	if ( ( handle->row_offset != sizeof( int ) ) && ( depth < 20 ) )
	{
		res = __NBC_Progress( handle, depth + 1 );
		if ( ( NBC_OK != res ) && ( NBC_CONTINUE != res ) )
		{
			printf( "Error in NBC_Progress() (%i)\n", res );
			ret = res;
			goto error;
		}
	}

error:
	return ret;
}
static inline int __NBC_Start_round( NBC_Handle *handle, int depth )
{
	int *numptr; /* number of operations */
	int i, res, ret = NBC_OK;
	NBC_Fn_type *typeptr;
	NBC_Args *sendargs;
	NBC_Args *recvargs;
	NBC_Args *opargs;
	NBC_Args *copyargs;
	NBC_Args *unpackargs;
	NBC_Schedule myschedule;
	void *buf1, *buf2, *buf3;
	MPI_Comm comm;

	MPI_request_struct_t *requests = __sctk_internal_get_MPC_requests();

	/* get schedule address */
	myschedule = (NBC_Schedule *) ( (char *) *handle->schedule + handle->row_offset );

	// mpc_common_debug_error("=> %p + %d", *handle->schedule,  handle->row_offset);

	numptr = (int *) myschedule;

	int req_cpt = 0;
	int old_req_count = handle->req_count;

	typeptr = (NBC_Fn_type *) ( numptr + 1 );
	// if( handle->tag== -29)
	//     fprintf(stderr,"start round rank %d *typeptr %d *numptr %d\n",mpc_common_get_task_rank(), *typeptr, *numptr);

	for ( i = 0; i < *numptr; i++ )
	{
		switch ( *typeptr )
		{
			case SEND:
				req_cpt++;
				break;
			case RECV:
				req_cpt++;
				break;
			case OP:
			case COPY:
			case MOVE:
			case UNPACK:
				break;
			default:
				printf( "1--NBC_Start_round: bad type %li at offset %li rank %d handle tag %d numptr %d myschedule %p handle->row_offset %ld *handle->schedule %p rank %d\n", (long) *typeptr, (long) typeptr - (long) myschedule, mpc_common_get_task_rank(), handle->tag, *numptr, myschedule, handle->row_offset, *handle->schedule, mpc_common_get_task_rank() );
				ret = NBC_BAD_SCHED;
				goto error;
		}
		typeptr = (NBC_Fn_type *) ( ( (NBC_Args *) typeptr ) + 1 );
		/* increase ptr by size of fn_type enum */
		typeptr = (NBC_Fn_type *) ( (NBC_Fn_type *) typeptr + 1 );
	}

	handle->req_count += req_cpt;

	if ( old_req_count != handle->req_count )
	{
		if ( handle->actual_req_count <= handle->req_count )
		{
			if ( !handle->actual_req_count || req_cpt > 10 )
			{
				handle->actual_req_count = handle->req_count;
			}
			else
			{
				handle->actual_req_count += 10;
			}

			handle->req_array = (MPI_Request *) sctk_realloc( handle->req_array, ( handle->actual_req_count ) * sizeof( MPI_Request ) );

			for ( i = old_req_count; i < handle->actual_req_count; i++ )
				handle->req_array[i] = MPI_REQUEST_NULL;
		}
	}

	req_cpt = 0;

	/* typeptr is increased by sizeof(int) bytes to point to type */
	typeptr = (NBC_Fn_type *) ( numptr + 1 );

	for ( i = 0; i < *numptr; i++ )
	{
		/* go sizeof op-data forward */
		switch ( *typeptr )
		{
			case SEND:
				sendargs = (NBC_Args *) ( typeptr + 1 );

				/* get an additional request */
				req_cpt++;
				/* get buffer */
				if ( sendargs->tmpbuf )
					buf1 = (char *) handle->tmpbuf + (long) sendargs->buf;
				else
					buf1 = sendargs->buf;

				if ( sendargs->comm == MPI_COMM_NULL )
					comm = handle->mycomm;
				else
					comm = sendargs->comm;

				NBC_CHECK_NULL( handle->req_array );
				// fprintf(stderr, "rank %d send to rank %d datatype %d count %d\n", mpc_common_get_task_rank(), sendargs->dest, sendargs->datatype, sendargs->count);
				res = _mpc_cl_isend( buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, comm, __sctk_new_mpc_request( handle->req_array + ( old_req_count + req_cpt - 1 ), requests ) );
				if ( MPI_SUCCESS != res )
				{
					printf( "Error in MPI_Isend(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long) buf1, sendargs->count, (unsigned long) sendargs->datatype, sendargs->dest, handle->tag, (unsigned long) handle->mycomm, res );
					ret = res;
					goto error;
				}
				break;
			case RECV:
				recvargs = (NBC_Args *) ( typeptr + 1 );

				/* get an additional request - TODO: req_count NOT thread safe */
				req_cpt++;
				/* get buffer */
				if ( recvargs->tmpbuf )
				{
					buf1 = (char *) handle->tmpbuf + (long) recvargs->buf;
				}
				else
				{
					buf1 = recvargs->buf;
				}

				if ( recvargs->comm == MPI_COMM_NULL )
					comm = handle->mycomm;
				else
					comm = recvargs->comm;

				NBC_CHECK_NULL( handle->req_array );
				// fprintf(stderr, "rank %d recv from rank %d datatype %d count %d\n", mpc_common_get_task_rank(), recvargs->source, recvargs->datatype, recvargs->count);
				res = _mpc_cl_irecv( buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, comm, __sctk_new_mpc_request( handle->req_array + ( old_req_count + req_cpt - 1 ), requests ) );

				if ( MPI_SUCCESS != res )
				{
					printf( "Error in MPI_Irecv(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long) buf1, recvargs->count, (unsigned long) recvargs->datatype, recvargs->source, handle->tag, (unsigned long) handle->mycomm, res );
					ret = res;
					goto error;
				}
				break;
			case OP:
				opargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( opargs->tmpbuf1 )
					buf1 = (char *) handle->tmpbuf + (long) opargs->buf1;
				else
					buf1 = opargs->buf1;

				if ( opargs->tmpbuf2 )
					buf2 = (char *) handle->tmpbuf + (long) opargs->buf2;
				else
					buf2 = opargs->buf2;

				if ( opargs->tmpbuf3 )
					buf3 = (char *) handle->tmpbuf + (long) opargs->buf3;
				else
					buf3 = opargs->buf3;

				res = NBC_Operation( buf3, buf1, buf2, opargs->op, opargs->datatype, opargs->count );
				if ( res != NBC_OK )
				{
					printf( "NBC_Operation() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			case COPY:
				copyargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( copyargs->tmpsrc )
					buf1 = (char *) handle->tmpbuf + (long) copyargs->src;
				else
					buf1 = copyargs->src;
				if ( copyargs->tmptgt )
					buf2 = (char *) handle->tmpbuf + (long) copyargs->tgt;
				else
					buf2 = copyargs->tgt;

				if ( copyargs->comm == MPI_COMM_NULL )
					comm = handle->mycomm;
				else
					comm = copyargs->comm;

				res = NBC_Copy( buf1, copyargs->srccount, copyargs->srctype, buf2, copyargs->tgtcount, copyargs->tgttype, comm );
				if ( res != NBC_OK )
				{
					printf( "NBC_Copy() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			case MOVE:
				copyargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( copyargs->tmpsrc )
					buf1 = (char *) handle->tmpbuf + (long) copyargs->src;
				else
					buf1 = copyargs->src;
				if ( copyargs->tmptgt )
					buf2 = (char *) handle->tmpbuf + (long) copyargs->tgt;
				else
					buf2 = copyargs->tgt;

				if ( copyargs->comm == MPI_COMM_NULL )
					comm = handle->mycomm;
				else
					comm = copyargs->comm;

				res = NBC_Move( buf1, copyargs->srccount, copyargs->srctype, buf2, copyargs->tgtcount, copyargs->tgttype, comm );
				if ( res != NBC_OK )
				{
					printf( "NBC_Copy() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			case UNPACK:
				unpackargs = (NBC_Args *) ( typeptr + 1 );

				/* get buffers */
				if ( unpackargs->tmpinbuf )
					buf1 = (char *) handle->tmpbuf + (long) unpackargs->inbuf;
				else
					buf1 = unpackargs->outbuf;
				if ( unpackargs->tmpoutbuf )
					buf2 = (char *) handle->tmpbuf + (long) unpackargs->outbuf;
				else
					buf2 = unpackargs->outbuf;

				if ( unpackargs->comm == MPI_COMM_NULL )
					comm = handle->mycomm;
				else
					comm = unpackargs->comm;

				res = NBC_Unpack( buf1, unpackargs->count, unpackargs->datatype, buf2, comm );
				if ( res != NBC_OK )
				{
					printf( "NBC_Unpack() failed (code: %i)\n", res );
					ret = res;
					goto error;
				}
				break;
			default:
				printf( "2NBC_Start_round: bad type %li at offset %li rank %d\n", (long) *typeptr, (long) typeptr - (long) myschedule, mpc_common_get_task_rank() );
				ret = NBC_BAD_SCHED;
				goto error;
		}
		/* Move forward of both ARG and  TYPE size */
		typeptr = (NBC_Fn_type *) ( ( (NBC_Args *) typeptr ) + 1 ) + 1;
	}

	/* check if we can make progress - not in the first round, this allows us to leave the
	 * initialization faster and to reach more overlap
	 *
	 * threaded case: calling progress in the first round can lead to a
	 * deadlock if NBC_Free is called in this round :-( */
	if ( ( handle->row_offset != sizeof( int ) ) && ( depth < 20 ) )
	{
		res = __NBC_Progress( handle, depth + 1 );
		if ( ( NBC_OK != res ) && ( NBC_CONTINUE != res ) )
		{
			printf( "Error in NBC_Progress() (%i)\n", res );
			ret = res;
			goto error;
		}
	}

error:
	return ret;
}

static inline int NBC_Start_round( NBC_Handle *handle )
{
	return __NBC_Start_round( handle, 0 );
}

static inline int NBC_Start_round_persistent( NBC_Handle *handle )
{
	return __NBC_Start_round_persistent( handle, 0 );
}

static inline int NBC_Initialize()
{
	if ( _mpc_mpi_config()->nbc.progress_thread == 1 )
	{
		struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific = mpc_cl_per_mpi_process_ctx_get();

		// _mpc_threads_ng_engine_attr_t attr;
		// _mpc_threads_ng_engine_attr_init(&attr);

		mpc_thread_attr_t attr;
		mpc_thread_attr_init( &attr );

		int ( *sctk_get_progress_thread_binding )( void ) =
			(int ( * )( void )) _mpc_mpi_config()->nbc.thread_bind_function;

		assume( sctk_get_progress_thread_binding != NULL );

		int cpu_id_to_bind_progress_thread = sctk_get_progress_thread_binding();

		mpc_thread_attr_setbinding( &attr,
									cpu_id_to_bind_progress_thread );

		////DEBUG
		// char hostname[1024];
		// gethostname(hostname,1024);
		// FILE *hostname_fd = fopen(strcat(hostname,".log"),"a");
		////if (hostname_fd == NULL) perror("FAILED FOPEN hostname_fd");

		// fprintf(hostname_fd,"task_id %03d, current_cpu=%03d,
		// cpu_id_to_bind_progress_thread = %03d\n",
		//        mpc_common_get_local_task_rank(),
		//        sctk_get_cpu(),
		//        cpu_id_to_bind_progress_thread
		//      );
		// fflush(hostname_fd);
		// fclose(hostname_fd);
		////DEBUG

		int ret = mpc_thread_core_thread_create( &( task_specific->mpc_mpi_data->NBC_Pthread ),
												 &attr,
												 NBC_Pthread_func, NULL );
		if ( 0 != ret )
		{
			printf( "Error in mpc_thread_core_thread_create() (%i)\n", ret );
			return NBC_OOR;
		}

		// _mpc_threads_ng_engine_attr_destroy(&attr);
		mpc_thread_attr_destroy( &attr );

		// task_specific->mpc_mpi_data->nbc_initialized_per_task = 1;
	}

	return NBC_OK;
}

inline int NBC_Init_handle( NBC_Handle *handle, MPI_Comm comm, int tag )
{
	int res = 0;

	// fprintf(stderr,"DEBUG######################
	// %d\n",_mpc_mpi_config()->nbc.progress_thread);

	if ( _mpc_mpi_config()->nbc.progress_thread == 1 )
	{
		struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific = NULL;
		task_specific = mpc_cl_per_mpi_process_ctx_get();

		mpc_thread_mutex_t *lock = &( task_specific->mpc_mpi_data->nbc_initializer_lock );
		mpc_thread_mutex_lock( lock );
		if ( task_specific->mpc_mpi_data->nbc_initialized_per_task == 0 )
		{
			res = NBC_Initialize();
			if ( res != NBC_OK )
			{
				mpc_thread_mutex_unlock( lock );
				return res;
			}
		}
		mpc_thread_mutex_unlock( lock );

		/* init locks */
		mpc_thread_mutex_init( &handle->lock, NULL );
		/* init semaphore */
		if ( mpc_thread_sem_init( &handle->semid, 0, 0 ) != 0 )
		{
			perror( "mpc_thread_sem_init()" );
		}
	}

	handle->tmpbuf = NULL;
	handle->req_count = 0;
	handle->actual_req_count = 0;
	handle->array_offset = 0;
	handle->num_rounds = 0;
	handle->req_array = NULL;
	handle->req_count_persistent = NULL;
	handle->comm = comm;
	handle->schedule = NULL;
	/* first int is the schedule size */
	handle->row_offset = sizeof( int );

	handle->tag = tag;
	handle->mycomm = comm;

	return NBC_OK;
}

inline int NBC_Start( NBC_Handle *handle, NBC_Schedule *schedule )
{
	int res = 0;

	handle->schedule = schedule;

	if ( _mpc_mpi_config()->nbc.progress_thread == 1 )
	{
		/* add handle to open handles - and give the control to the progress
		 * thread - the handle must not be touched by the user thread from now
		 * on !!! */

		struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific;

		mpc_thread_mutex_t *lock;
		task_specific = mpc_cl_per_mpi_process_ctx_get();

		lock = &( task_specific->mpc_mpi_data->list_handles_lock );

		struct sctk_list_elem *elem_tmp;

		elem_tmp = (struct sctk_list_elem *) sctk_malloc( sizeof( struct sctk_list_elem ) );
		elem_tmp->elem = handle;

		mpc_thread_wait_for_value_and_poll(
			&( task_specific->mpc_mpi_data->nbc_initialized_per_task ), 1, NULL,
			NULL );
		/* wake progress thread up */
		// _mpc_cl_send(&tmp_send, 1, MPI_INT, 0, 0, MPI_COMM_SELF);
		mpc_thread_mutex_lock( lock );

		DL_APPEND( task_specific->mpc_mpi_data->NBC_Pthread_handles, elem_tmp );
		task_specific->mpc_mpi_data->NBC_Pthread_nb++;
		mpc_thread_mutex_unlock( lock );
		mpc_thread_sem_post( &( task_specific->mpc_mpi_data->pending_req ) );

		mpc_threads_generic_scheduler_increase_prio();

		// mpc_thread_wait_for_value_and_poll(&(task_specific->mpc_mpi_data->nbc_initialized_per_task),
		// 1, NULL, NULL);
		///* wake progress thread up */
		// _mpc_cl_send(&tmp_send, 1, MPI_INT, 0, 0, MPI_COMM_SELF);

#ifdef SCTK_DEBUG_SCHEDULER
		{
			char hostname[128];
			char hostname2[128];
			char current_vp[5];
			gethostname( hostname, 128 );
			strncpy( hostname2, hostname, 128 );
			sprintf( current_vp, "_%03d", mpc_topology_get_pu() );
			strcat( hostname2, current_vp );

			FILE *fd = fopen( hostname2, "a" );
			fprintf( fd, "NBC_APPEND %p %d\n", elem_tmp,
					 task_specific->mpc_mpi_data->NBC_Pthread_nb );
			// printf("NBC_APPEND\n");
			// fflush(stdout);
			fflush( fd );
			fclose( fd );
		}
#endif

		// no progress thread
	}
	else
	{
		/* kick off first round */
		if ( handle->is_persistent )
		{
			res = NBC_Start_round_persistent( handle );
		}
		else
		{
			res = NBC_Start_round( handle );
		}
		if ( ( NBC_OK != res ) )
		{
			printf( "Error in NBC_Start_round() (%i)\n", res );
			return res;
		}
	}

	return NBC_OK;
}

int NBC_Wait( NBC_Handle *handle, MPI_Status *status )
{
	int use_progress_thread = 0;
	use_progress_thread = _mpc_mpi_config()->nbc.progress_thread;

	if ( status != MPI_STATUS_IGNORE )
	{
		memset( status, 0, sizeof( MPI_Status ) );
		status->MPI_ERROR = MPI_SUCCESS;
	}

	if ( use_progress_thread == 1 )
	{
		mpc_thread_mutex_lock( &handle->lock );
	}
	if ( handle->schedule == NULL )
	{
		if ( use_progress_thread == 1 )
		{
			mpc_thread_mutex_unlock( &handle->lock );
		}
		return NBC_OK;
	}
	if ( use_progress_thread == 1 )
	{
		mpc_thread_mutex_unlock( &handle->lock );

		/* wait on semaphore */
		int szcomm;
		MPI_Comm_size( handle->mycomm, &szcomm );
		if ( szcomm == 1 )
			mpc_thread_sem_post( &handle->semid );
		if ( mpc_thread_sem_wait( &handle->semid ) != 0 )
		{
			perror( "mpc_thread_sem_wait()" );
		}
		if ( mpc_thread_sem_destroy( &handle->semid ) != 0 )
		{
			perror( "mpc_thread_sem_destroy()" );
		}
	}
	else
	{
		/* poll */
		while ( NBC_OK != NBC_Progress( handle ) )
			;
	}

	return MPI_SUCCESS;
}

int NBC_Test( NBC_Handle *handle, int *flag, __UNUSED__ MPI_Status *status )
{
	int use_progress_thread = 0;
	use_progress_thread =
		_mpc_mpi_config()->nbc.progress_thread;
	int ret = NBC_CONTINUE;

	if ( status != MPI_STATUS_IGNORE )
	{
		status->MPI_ERROR = MPI_SUCCESS;
	}

	if ( use_progress_thread == 1 )
	{
		ret = NBC_CONTINUE;
		mpc_thread_mutex_lock( &handle->lock );
		if ( handle->schedule == NULL )
		{
			ret = NBC_OK;
		}
		mpc_thread_mutex_unlock( &handle->lock );
		//        printf("ProgressThread : ret = %d ------ NBC_OK: %d, NBC_CONTINUE:
		//        %d\n", ret, NBC_OK, NBC_CONTINUE); fflush(stdout);
	}
	else
	{
		ret = NBC_Progress( handle );
		//        printf("No PT :  ret = %d\n", ret); fflush(stdout);
	}
	if ( ret == NBC_OK )
	{
		*flag = 1;
	}
	else
	{
		*flag = 0;
	}
	return MPI_SUCCESS;
}

/******************* *
 * NBC_OP.C *
 * *******************/

/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *                    rights reserved.
 * Copyright (c) 2024 Commissariat a l'Energie Atomique et aux Energies Alternatives
 *                    All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *            Remi Dehenne <remi.dehenne@cea.fr>
 */


int NBC_Operation( void *buf3, void *buf1, void *buf2, MPI_Op op, MPI_Datatype type, int count )
{
	int i;
	
	/* C integer */
	if ( type == MPI_INT )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(int)
	}
	else if ( type == MPI_LONG )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(long)
	}
	else if ( type == MPI_SHORT )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(short)
	}
	else if ( type == MPI_CHAR || type == MPI_SIGNED_CHAR )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(char)
	}
	else if ( type == MPI_LONG_LONG_INT || type == MPI_LONG_LONG )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(long long int)
	}
	else if ( type == MPI_UNSIGNED )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(unsigned int)
	}
	else if ( type == MPI_UNSIGNED_LONG )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(unsigned long)
	}
	else if ( type == MPI_UNSIGNED_SHORT )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(unsigned short)
	}
	else if ( type == MPI_UNSIGNED_CHAR )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(unsigned char)
	}
	else if ( type == MPI_UNSIGNED_LONG_LONG )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(unsigned long long int)
	}
	else if ( type == MPI_INT8_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(int8_t)
	}
	else if ( type == MPI_INT16_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(int16_t)
	}
	else if ( type == MPI_INT32_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(int32_t)
	}
	else if ( type == MPI_INT64_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(int64_t)
	}
	else if ( type == MPI_UINT8_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(uint8_t)
	}
	else if ( type == MPI_UINT16_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(uint16_t)
	}
	else if ( type == MPI_UINT32_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(uint32_t)
	}
	else if ( type == MPI_UINT64_T )
	{
		MPC_NBC_OP_DEFINE_C_INTEGER(uint64_t)
	}

	/* Floating point */
	else if ( type == MPI_FLOAT )
	{
		MPC_NBC_OP_DEFINE_FLOATING_POINT(float)
	}
	else if ( type == MPI_DOUBLE )
	{
		MPC_NBC_OP_DEFINE_FLOATING_POINT(double)
	}
	else if ( type == MPI_LONG_DOUBLE )
	{
		MPC_NBC_OP_DEFINE_FLOATING_POINT(long double)
	}

	/* Logical */
	else if ( type == MPI_C_BOOL )
	{
		MPC_NBC_OP_DEFINE_LOGICAL(_Bool)
	}

	/* Complex */
	else if ( type == MPI_C_COMPLEX || type == MPI_C_FLOAT_COMPLEX)
	{
		MPC_NBC_OP_DEFINE_COMPLEX(float _Complex)
	}
	else if ( type == MPI_C_DOUBLE_COMPLEX)
	{
		MPC_NBC_OP_DEFINE_COMPLEX(double _Complex)
	}
	else if ( type == MPI_C_LONG_DOUBLE_COMPLEX)
	{
		MPC_NBC_OP_DEFINE_COMPLEX(long double _Complex)
	}

	/* Byte */
	else if ( type == MPI_BYTE )
	{
		MPC_NBC_OP_DEFINE_BYTE(char)
	}

	/* Multi-language types */
	else if ( type == MPI_AINT )
	{
		MPC_NBC_OP_DEFINE_MULTI_LANG(MPI_Aint)
	}
	else if ( type == MPI_OFFSET )
	{
		MPC_NBC_OP_DEFINE_MULTI_LANG(MPI_Offset)
	}
	else if ( type == MPI_COUNT )
	{
		MPC_NBC_OP_DEFINE_MULTI_LANG(MPI_Count)
	}

	/* Minloc / maxloc */
	else if ( type == MPI_FLOAT_INT )
	{
		MPC_NBC_OP_DEFINE_LOC(float)
	}
	else if ( type == MPI_DOUBLE_INT )
	{
		MPC_NBC_OP_DEFINE_LOC(double)
	}
	else if ( type == MPI_LONG_INT )
	{
		MPC_NBC_OP_DEFINE_LOC(long)
	}
	else if ( type == MPI_2INT )
	{
		MPC_NBC_OP_DEFINE_LOC(int)
	}
	else if ( type == MPI_SHORT_INT )
	{
		MPC_NBC_OP_DEFINE_LOC(short)
	}
	else if ( type == MPI_LONG_DOUBLE_INT )
	{
		MPC_NBC_OP_DEFINE_LOC_NAME(long double, long_double)
	}
	else
		return NBC_DATATYPE_NOT_SUPPORTED;

	return NBC_OK;
}

/********************************************************************* *
 * The previous code came from the libNBC, a library for the support   *
 * of MPI-3 Non-Blocking Collective. The code was modified to ensure   *
 * compatibility with the MPC framework.                               *
 * *********************************************************************/

int NBC_Finalize( __UNUSED__ mpc_thread_t *NBC_thread )
{
	if ( _mpc_mpi_config()->nbc.progress_thread == 1 )
	{
		int ret = 0;
		//  ret = mpc_thread_join( *NBC_thread, NULL);
		if ( 0 != ret )
		{
			printf( "Error in mpc_thread_join() (%i)\n", ret );
			return NBC_OOR;
		}
	}
	return NBC_OK;
}


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
/* #   - TABOADA Hugo hugo.taboada@cea.fr                                 # */
/* #   - DEHENNE Remi remi.dehenne@cea.fr                                 # */
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

#include "mpc_nbc_reduction_macros.h"
#include "egreq_nbc.h"

/* INTERNAL FOR NON-BLOCKING COLLECTIVES - CALL TO libNBC FUNCTIONS*/

/********************************************************************* *
 * The Following code comes from the libNBC, a library for the support *
 * of MPI-3 Non-Blocking Collective. The code was modified to ensure	 *
 * compatibility with the MPC framework.															 *
 * *********************************************************************/

static inline int NBC_Progress( NBC_Handle *handle );
static inline int NBC_Start_round( NBC_Handle *handle );
static inline int NBC_Start_round_persistent( NBC_Handle *handle );
static inline int NBC_Free( NBC_Handle *handle );

#define NBC_HANDLE_ALLOC_REQ(_nbc_handle, _offset) \
        ({ \
                MPI_internal_request_t *__mpi_req; \
                __mpi_req = (_nbc_handle)->req_array[_offset] = mpc_lowcomm_request_alloc(); \
                __mpi_req; \
         })

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
                                        MPI_Request req = NBC_HANDLE_ALLOC_REQ(handle,
                                                                               old_req_count + req_cpt -1);

					res = _mpc_cl_isend(buf1, sendargs->count, sendargs->datatype, sendargs->dest,
                                                            handle->tag, comm, _mpc_cl_get_lowcomm_request(req));
					/*mark new intermediate nbc request to keep it as long as collective isn't free*/
                                        req->is_persistent = 1;
					if ( handle->is_persistent == 1 )
					{
						req->is_persistent = 2;
						req->used = 1;
						req->nbc_handle.schedule = handle->schedule;

					}
				}
				else
				{
                                        MPI_Request req = NBC_HANDLE_ALLOC_REQ(handle,
                                                                               handle->array_offset + req_cpt -1);

					res = _mpc_cl_isend(buf1, sendargs->count, sendargs->datatype, sendargs->dest,
                                                            handle->tag, comm, _mpc_cl_get_lowcomm_request(req));
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
                                        MPI_Request mpi_req = NBC_HANDLE_ALLOC_REQ(handle, old_req_count + req_cpt - 1);
					res = _mpc_cl_irecv(buf1, recvargs->count, recvargs->datatype, recvargs->source,
                                                            handle->tag, comm, _mpc_cl_get_lowcomm_request(mpi_req));
					/*mark new intermediate nbc request to keep it as long as collective isn't free*/
                                        mpi_req->is_persistent = 1;
					if ( handle->is_persistent == 1 )
					{

						mpi_req->is_persistent = 2;
						mpi_req->used = 1;
						mpi_req->nbc_handle.schedule = handle->schedule;
					}
				}
				else
				{
                                        MPI_Request mpi_req = NBC_HANDLE_ALLOC_REQ(handle, handle->array_offset + req_cpt - 1);
					mpi_req->is_persistent = 2;
					mpi_req->used = 1;
					mpi_req->nbc_handle.schedule = handle->schedule;
					res = _mpc_cl_irecv( buf1, recvargs->count, recvargs->datatype, recvargs->source,
                                                             handle->tag, comm, _mpc_cl_get_lowcomm_request(mpi_req));
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
                MPI_Request mpi_req;
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
                                mpi_req = NBC_HANDLE_ALLOC_REQ(handle, old_req_count + req_cpt - 1);
				res = _mpc_cl_isend( buf1, sendargs->count, sendargs->datatype, sendargs->dest,
                                                     handle->tag, comm, _mpc_cl_get_lowcomm_request(mpi_req));
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
                                mpi_req = NBC_HANDLE_ALLOC_REQ(handle, old_req_count + req_cpt - 1);
				res = _mpc_cl_irecv( buf1, recvargs->count, recvargs->datatype, recvargs->source,
                                                     handle->tag, comm, _mpc_cl_get_lowcomm_request(mpi_req));

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

		mpc_thread_attr_t attr;
		mpc_thread_attr_init( &attr );

		int ( *sctk_get_progress_thread_binding )( void ) =
			(int ( * )( void )) _mpc_mpi_config()->nbc.thread_bind_function;

		assume( sctk_get_progress_thread_binding != NULL );

		int cpu_id_to_bind_progress_thread = sctk_get_progress_thread_binding();

		mpc_thread_attr_setbinding( &attr,
									cpu_id_to_bind_progress_thread );

		int ret = mpc_thread_core_thread_create( &( task_specific->mpc_mpi_data->NBC_Pthread ),
												 &attr,
												 NBC_Pthread_func, NULL );
		if ( 0 != ret )
		{
			printf( "Error in mpc_thread_core_thread_create() (%i)\n", ret );
			return NBC_OOR;
		}

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

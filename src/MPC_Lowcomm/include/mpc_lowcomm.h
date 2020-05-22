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

#include <mpc_lowcomm_types.h>

/** This file define the low-level communication layer for MPC
 * it also documents the libmode to bootstrap the communication layer
 * inside another MPI, in this case the full low-level interface
 * is available.
 */

/************************************************************************/
/* Rank Query                                                           */
/************************************************************************/

/** These functions can be used for rank manipulaton */

/** Get SCTK_COMM_WORLD rank
 * @return the comm world rank
 */
int mpc_lowcomm_get_rank();

/** Get SCTK_COMM_WORLD rank
 * @return the comm world size
 */
int mpc_lowcomm_get_size();

/** Get Rank in a communicator
 * @arg communicator Communicator from which to get the rank
 * @return the rank of the process in the given communicator
 */
int mpc_lowcomm_get_comm_rank(const mpc_lowcomm_communicator_t communicator);

/** Get Rank in a communicator
 * @arg communicator Communicator from which to get the process count
 * @return process count in the given communicator
 */
int mpc_lowcomm_get_comm_size(const mpc_lowcomm_communicator_t communicator);


/**
 * @brief Return the total number of UNIX processes
 * @return number of UNIX processes
 */
int mpc_lowcomm_get_process_count();

/**
 * @brief Return the UNIX process rank
 *
 * @return int UNIX process rank
 */
int mpc_lowcomm_get_process_rank();

/************************************************************************/
/* Communicators                                                        */
/************************************************************************/

/** These functions provide communicator primitives to the low-level interface */

/** Create a new communicator from a task list
 *
 * @arg origin_communicator Source communicator
 * @arg nb_task_involved Number of tasks
 * @arg task_list List of tasks to include is the new communicator
 *
 * @return Return a new communicator ID usable inside comm calls
 */
mpc_lowcomm_communicator_t mpc_lowcomm_create_comm(const mpc_lowcomm_communicator_t origin_communicator,
                                                   const int nb_task_involved,
                                                   const int *task_list);

/** Delete a given communicator
 * @param comm Communicator to be deleted
 *
 * @return SCTK_COMM_NULL if the comm has been deleted the comm otherwise (immutable comm)
 */
mpc_lowcomm_communicator_t mpc_lowcomm_delete_comm(const mpc_lowcomm_communicator_t comm);

/************************************************************************/
/* P2P Messages                                                         */
/************************************************************************/

/** This is the MPC low-level point to point interface */

/** Wait for a communication completion
 * @warning All communications issuing a request in the low-level
 * comm interface MUST be waited
 *
 * @param request The request to be waited
 */
void mpc_lowcomm_wait(mpc_lowcomm_request_t *request);

/** Send an asynchronous message
 *
 * @param dest Destination rank
 * @param data Data to be send
 * @param tag Message tag
 * @param comm Communicator of the message
 * @param req Returned request to be waited with @ref mpc_lowcomm_wait
 */
void mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req);

/** Receive an asynchornous message
 * @param src Source rank
 * @param buffer Buffer where to store the data
 * @param size Size of the data to be received
 * @param tag Message tag
 * @param comm Communicator of the message
 * @param req Returned request to be waited with @ref mpc_lowcomm_wait
 */
void mpc_lowcomm_irecv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req);

/** Send a synchronous message
 *
 * @param dest Destination rank
 * @param data Data to be send
 * @param tag Message tag
 * @param comm Communicator of the message
 */
void mpc_lowcomm_send(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm);

/** Receive a synchornous message
 * @param src Source rank
 * @param buffer Buffer where to store the data
 * @param size Size of the data to be received
 * @param tag Message tag
 * @param comm Communicator of the message
 */
void mpc_lowcomm_recv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm);

/** Do a synchronous SendRecv
 *
 * @param sendbuf Buffer to be sent
 * @param size Size of the buffer
 * @param dest Destination process
 * @param tag Message tag
 * @param recvbuf Receive buffer
 * @param src Source of the message
 * @param comm Communicator of the message
 */
void mpc_lowcomm_sendrecv(void *sendbuf, size_t size, int dest, int tag, void *recvbuf, int src, mpc_lowcomm_communicator_t comm);

/************************************************************************/
/* Collective Operations                                                */
/************************************************************************/

/** These functions are the low-level colletive operations */

/** Do a barrier on a communicator
 * @param comm Communicator to do a barrier on
 */
int mpc_lowcomm_barrier(mpc_lowcomm_communicator_t comm);

/* Internal shared memory version of the barrier */
struct shared_mem_barrier;
int mpc_lowcomm_barrier_shm_on_context(struct shared_mem_barrier *barrier_ctx,
                                       int comm_size);

/** Do a broadcast on a communicator
 * @param buffer Buffer to be broadcasted from root and filled on others
 * @param size Size of the buffer
 * @param root Root from which to read the buffer
 * @param communicator Communicator on which to broadcast
 */
void mpc_lowcomm_bcast(void *buffer, const size_t size,
                       const int root, const mpc_lowcomm_communicator_t communicator);

/** Do an allreduce on a communicator
 * @param buffer_in Input buffer (where the result is stored)
 * @param buffer_out Output buffer (where data is read)
 * @param elem_size Size of an element
 * @param elem_count Number of elements
 * @param func @ref sctk_Op_f to be used to do the operation (void (*sctk_Op_f) (const void *, void *, size_t, mpc_lowcomm_datatype_t);)
 * @param communicator Communicator on which to do the operation
 * @param datatype Datatype (not used in comms) but can be useful to swich in the reduce operation (passed in arg)
 */
void mpc_lowcomm_allreduce(const void *buffer_in, void *buffer_out,
                           const size_t elem_size,
                           const size_t elem_count,
                           sctk_Op_f func,
                           const mpc_lowcomm_communicator_t communicator,
                           mpc_lowcomm_datatype_t datatype);

void mpc_lowcomm_terminaison_barrier(void);

/***************************
* MPC LOWCOMM TRAMPOLINES *
***************************/

/**
 * @brief Trampoline function for polling EGREQ from low-level layers
 *
 * @param trampoline MPC_MPI polling function
 */
void mpc_lowcomm_egreq_poll_set_trampoline(void (*trampoline)(void) );

/**
 * @brief Trampoline function for MPC_MPI to check for allocmem boundaries
 *
 * @param trampoline pointer to trampoline function
 */
void mpc_lowcomm_rdma_allocmem_is_in_pool_set_trampoline(int (*trampoline)(void *) );

/**
 * @brief Set a flag to enable MPI window progress
 */
void mpc_lowcomm_rdma_MPI_windows_in_use(void);

/**
 * @brief Set the function to be called on request completion
 *
 * @param trampoline request completion callback
 */
void mpc_lowcomm_set_request_completion_trampoline(int trampoline(mpc_lowcomm_request_t *) );

/**
 * @brief Notify the completion of a request from lowcomm to MPC MPI
 *
 * @param req request to be completed
 * @return int 1 if request counter was handled
 */
int mpc_lowcomm_notify_request_completion(mpc_lowcomm_request_t *req);

/**
 * @brief Notify request on source context
 *
 * @param trampoline Function to notify to
 */
void mpc_lowcomm_rdma_MPC_MPI_notify_src_ctx_set_trampoline(void (*trampoline)(mpc_lowcomm_rdma_window_t) );

/**
 * @brief Notify request on dest context
 *
 * @param trampoline Function to notify to
 */
void mpc_lowcomm_rdma_MPC_MPI_notify_dest_ctx_set_trampoline(void (*trampoline)(mpc_lowcomm_rdma_window_t) );

/************************************************************************/
/* Setup and Teardow when running in libmode                            */
/************************************************************************/

/** Libmode allows MPC to run alongside another MPI instance
 * to do so simply compile mpc with ./installmpc --lib-mode.
 * Note that a TCP rail is required to bootstrap the comm ring
 * to enable on demand connections on high speed networks.
 *
 * The following hooks have to be defined in the host application (see example below):
 *
 * - int mpc_lowcomm_hook_rank() -> Return an unique rank
 * - int mpc_lowcomm_hook_size() -> Return the communicator size
 * - void mpc_lowcomm_hook_barrier() -> Implement a barrier between processes
 * - void mpc_lowcomm_hook_send_to( void * data, size_t size, int target ) -> Send data to a given rank
 * - void mpc_lowcomm_hook_recv_from( void * data, size_t size, int source ) -> Receive data from a given rank
 *
 * Here is a simple example of program running in libmode
 * as a guest inside another MPI note how the rank informations
 * are extracted from the host MPI and how the send and receive
 * hooks have to be defined to bootstrap MPC's comm:
 *
 * COMPILE this code on a libmode install with:
 *
 * mpicc ./code.c `mpc_cflags` `mpc_ldflags`
 *
 * code.c:
 *
 ***#include <stdio.h>
 ***#include <mpi.h>
 ***#include <mpc_lowcomm.h>
 *
 *      int mpc_lowcomm_hook_rank()
 *      {
 *              int rank;
 *
 *              MPI_Comm_rank( MPI_COMM_WORLD, & rank );
 *              return  rank;
 *      }
 *
 *      int mpc_lowcomm_hook_size()
 *      {
 *              int size;
 *
 *              MPI_Comm_size( MPI_COMM_WORLD, & size );
 *
 *              return  size;
 *      }
 *
 *      void mpc_lowcomm_hook_barrier()
 *      {
 *              MPI_Barrier( MPI_COMM_WORLD );
 *      }
 *
 *      void mpc_lowcomm_hook_send_to( void * data, size_t size, int target )
 *      {
 *              MPI_Send( data, size, MPI_CHAR, target, 48895, MPI_COMM_WORLD );
 *      }
 *
 *      void mpc_lowcomm_hook_recv_from( void * data, size_t size, int source )
 *      {
 *              MPI_Recv( data, size, MPI_CHAR, source, 48895, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
 *      }
 *
 *      int main( int argc, char ** argv )
 *      {
 *              MPI_Init( &argc, &argv );
 *              mpc_lowcomm_libmode_init();
 *
 *              int i;
 *
 *              for( i = 0 ; i < 1024 ; i++ )
 *              {
 *                      MPI_Barrier( MPI_COMM_WORLD );
 *                      mpc_lowcomm_barrier( SCTK_COMM_WORLD );
 *              }
 *
 *              mpc_lowcomm_libmode_release();
 *              MPI_Finalize();
 *
 *              return 0;
 *      }
 */

/** Setup the MPC_Net environment
 *
 * @warning This function has to be called when running MPC in libmode
 * it must be called after MPI_Init
 */
void mpc_lowcomm_libmode_init();

/** Release the MPC_Net environment
 * This function has to be called to release the MPC
 * net environment.
 *
 * @warning After this call it is invalid to emit any MPC_Net call
 */
void mpc_lowcomm_libmode_release();

#endif /* SCTK_COMM_H */

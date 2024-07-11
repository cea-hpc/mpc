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
#ifndef MPC_LOWCOMM_MAIN_HEADER_INC_H
#define MPC_LOWCOMM_MAIN_HEADER_INC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpc_lowcomm_types.h>

/** This file define the low-level communication layer for MPC */

/************************************************************************/
/* Rank Query                                                           */
/************************************************************************/

/** These functions can be used for rank manipulaton */

/** Get MPC_COMM_WORLD rank
 * @return the comm world rank
 */
int mpc_lowcomm_get_rank(void);

/** Get MPC_COMM_WORLD rank
 * @return the comm world size
 */
int mpc_lowcomm_get_size(void);

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
int mpc_lowcomm_get_process_count(void);

/**
 * @brief Return the UNIX process rank
 *
 * @return int UNIX process rank
 */
int mpc_lowcomm_get_process_rank(void);

/************
 * REQUESTS *
 ************/

/**
 * @brief Extract status information from a request
 *    @warning request should be completed or canceled
 *
 * @param request request to extract information from
 * @param status status to be filled
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_commit_status_from_request(mpc_lowcomm_request_t *request,
                                           mpc_lowcomm_status_t *status);

/**
 * @brief Wait for a request completion (or cancelation)
 *
 * @param request request to be waited
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_request_wait(mpc_lowcomm_request_t *request);

/**
 * @brief Cancel a request
 *
 * @param request the request to be cancelled
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_request_cancel(mpc_lowcomm_request_t *request);

/**
 * @brief Initialize a request (internal use)
 *
 * @param request request to be initialized
 * @param comm communicator to rely on
 * @param request_type request type
 */

void mpc_lowcomm_request_init_struct(mpc_lowcomm_request_t *request, 
                                     mpc_lowcomm_communicator_t comm, 
                                     int request_type, int src, int dest,
                                     int tag, lcp_send_callback_func_t cb);


/*********************
 * UNIVERSE MESSAGES *
 *********************/

/**
 * @brief Send a message to a remote set peer
 *
 * @param dest destination peer UID
 * @param data data to be sent
 * @param size size of data pointer
 * @param tag tag to be used
 * @param remote_world remote world communicator
 * @param req request to fill in
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_universe_isend(mpc_lowcomm_peer_uid_t dest,
                              const void *data,
                              size_t size,
                              int tag,
                              mpc_lowcomm_communicator_t remote_world,
                              mpc_lowcomm_request_t *req);

/**
 * @brief Recv a message from a remote set peer
 *
 * @param dest source peer UID
 * @param data data to be received
 * @param size size of data pointer
 * @param tag tag to be used
 * @param req request to fill in
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_universe_irecv(mpc_lowcomm_peer_uid_t src,
                               const void *data,
                               size_t size,
                               int tag,
                               mpc_lowcomm_request_t *req);

/*******************
 * PORT MANAGEMENT *
 *******************/

/**
 * @brief Open a new port for Connect accept
 *
 * @param id output string describing the new port
 * @param id_len the length of the string
 * @return int 0 on success
 */
int mpc_lowcomm_open_port(char * iport_name, int port_name_len);

/**
 * @brief Close a previously openned port
 *
 * @param id the port to be freed
 * @return int 0 on success
 */
int mpc_lowcomm_close_port(const char * port_name);

/*******************
 * NAME PUBLISHING *
 *******************/

/**
 * @brief Publish an open port as service name
 *
 * @param service_name the service to publish
 * @param port_name the port information to publish
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_publish_name(const char *service_name,
                             const char *port_name);

/**
 * @brief Unpublish a service name
 *
 * @param service_name the service to unpublish
 * @param port_name the port information to unpublish
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_unpublish_name(const char *service_name,
                               const char *port_name);

/**
 * @brief Retrieve information from a published service
 *
 * @param service_name the service name to resolve
 * @param port_name the corresponding port information
 * @param port_name_len the length of the port_name buffer
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_lookup_name(const char *service_name,
                            char *port_name,
                            int port_name_len);

/************************************************************************/
/* P2P Messages                                                         */
/************************************************************************/

/** This is the MPC low-level point to point interface */
/** Allocate a request from lower layer. */
void *mpc_lowcomm_request_alloc();
/** Free a request from lower layer. */
void mpc_lowcomm_request_free(mpc_lowcomm_request_t *request);
/** Pass to LCP layer the upper layer request information. */
int mpc_lowcomm_pass_mpi_request_info(size_t request_size, void (*request_init_func)(void *));
void mpc_lowcomm_set_packed_buf(mpc_lowcomm_request_t *request, void *buf,
                                size_t packed_size);

/** Wait for a communication completion
 * @warning All communications issuing a request in the low-level
 * comm interface MUST be waited
 *
 * @param request The request to be waited
 * @param status status to be filled (can be NULL)
 */
int mpc_lowcomm_wait(mpc_lowcomm_request_t *request, mpc_lowcomm_status_t *status);

/**
 * @brief Test and progress a request for completion
 *
 * @param request Request to be tested
 * @param completed 1 if the request did complete/was cancelled 0 if not
 * @param status status to be filled (only valid if completed can be NULL)
 *
 */
int mpc_lowcomm_test(mpc_lowcomm_request_t * request, int * completed, mpc_lowcomm_status_t *status);

/** Wait for a set of communication completion
 * @warning All communications issuing a request in the low-level
 * comm interface MUST be waited
 *
 * @param count number of requests in the array
 * @param requests The array of request to be waited
 * @param status array of statuses to be filled (can be NULL)
 */
int mpc_lowcomm_waitall(int count, mpc_lowcomm_request_t *requests, mpc_lowcomm_status_t *statuses);

/** Send an asynchronous message
 *
 * @param dest Destination rank
 * @param data Data to be send
 * @param tag Message tag
 * @param comm Communicator of the message
 * @param req Returned request to be waited with @ref mpc_lowcomm_wait
 */
int mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req);

/**
 * @brief wrapper for sending a rendez-vous message. The "i" is here for consistency with mpc_lowcomm_isend, even though the message is synchronous.
 *
 * @param dest Destination rank
 * @param data Data to be sent
 * @param size Size if the data to be received
 * @param tag Message tag
 * @param comm Communicator of the message
 * @param req Returned request to be waited with @ref mpc_lowcomm_wait
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int mpc_lowcomm_ssend(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req);

/**
 * @brief wrapped isend function. Send an "asynchronous" message (that is synchronous if it comes from `mpc_lowcomm_issend`).
 *
 * @param dest Destination rank
 * @param data Data to be sent
 * @param size Size if the data to be received
 * @param tag Message tag
 * @param comm Communicator of the message
 * @param req Returned request to be waited with @ref mpc_lowcomm_wait
 * @param protocol protocol to be forced into the lower layers. Only MPC_LOWCOMM_RDV is forced yet.
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int _mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req, int synchronized);

/** Receive an asynchornous message
 * @param src Source rank
 * @param buffer Buffer where to store the data
 * @param size Size of the data to be received
 * @param tag Message tag
 * @param comm Communicator of the message
 * @param req Returned request to be waited with @ref mpc_lowcomm_wait
 */
int mpc_lowcomm_irecv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req);

/** Send a synchronous message
 *
 * @param dest Destination rank
 * @param data Data to be send
 * @param tag Message tag
 * @param comm Communicator of the message
 */
int mpc_lowcomm_send(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm);

/** Receive a synchornous message
 * @param src Source rank
 * @param buffer Buffer where to store the data
 * @param size Size of the data to be received
 * @param tag Message tag
 * @param comm Communicator of the message
 */
int mpc_lowcomm_recv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm);

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
int mpc_lowcomm_sendrecv(const void *sendbuf, size_t size, int dest, int tag, void *recvbuf, int src, mpc_lowcomm_communicator_t comm);

/**
 * @brief Probe for a given message as both a given source and destination (advanced thread-based use)
 *
 * @param world_source Source IN COMM WORLD
 * @param world_destination Destination IN COMM WORLD
 * @param tag message tag
 * @param comm communicator to test for
 * @param flag set to 1 if the message is found
 * @param status status (similar to MPI filled with message infos)
 * @return int MPC_LOWCOMM_SUCCESS if no error
 */
int mpc_lowcomm_iprobe_src_dest(const int world_source, const int world_destination, const int tag,
                                const mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status);

/**
 * @brief Non-blocking probe for a message to local rank
 *
 * @param source message source (in comm)
 * @param tag message tag
 * @param comm message communicator
 * @param flag set to 1 if message is found
 * @param status status (similar to MPI filled with message infos)
 * @return int MPC_LOWCOMM_SUCCESS if no error
 */
int mpc_lowcomm_iprobe(int source, int tag, mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status);

/**
 * @brief Blocking probe for a message to local rank
 *
 * @param source message source (in comm)
 * @param tag message tag
 * @param comm message communicator
 * @param flag set to 1 if message is found
 * @param status status (similar to MPI filled with message infos)
 * @return int MPC_LOWCOMM_SUCCESS if no error
 */
int mpc_lowcomm_probe(int source, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_status_t *status);

/************************************************************************/
/* Collective Operations                                                */
/************************************************************************/

/** These functions are the low-level colletive operations */

/** Do a barrier on a communicator
 * @param comm Communicator to do a barrier on
 */
int mpc_lowcomm_barrier(mpc_lowcomm_communicator_t comm);

/** Do a barrier on a communicator avoiding any shm implementation
 * @param comm Communicator to do a barrier on
 */
int mpc_lowcomm_non_shm_barrier( const mpc_lowcomm_communicator_t communicator );

/* Internal shared memory version of the barrier */
struct shared_mem_barrier;
int mpc_lowcomm_barrier_shm_on_context(struct shared_mem_barrier *barrier_ctx,
                                       int comm_size);


int mpc_lowcomm_gather(void *sendbuf, void *recvbuf, const size_t size, int root, mpc_lowcomm_communicator_t comm);

int mpc_lowcomm_allgather(void *sendbuf,  void *recvbuf, size_t data_size, mpc_lowcomm_communicator_t comm);

/** Do a broadcast on a communicator
 * @param buffer Buffer to be broadcasted from root and filled on others
 * @param size Size of the buffer
 * @param root Root from which to read the buffer
 * @param communicator Communicator on which to broadcast
 */
int mpc_lowcomm_bcast(void *buffer, const size_t size,
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

void mpc_lowcomm_request_wait_all_msgs(const int task, const mpc_lowcomm_communicator_t com);

/***************************
* MEMORY POOL              *
***************************/

void *mpc_lowcomm_allocmem_pool_alloc(size_t size);
void *mpc_lowcomm_allocmem_pool_alloc_check(size_t size, int * is_shared);
int mpc_lowcomm_allocmem_pool_free(void *ptr);
int mpc_lowcomm_allocmem_pool_free_size(void *ptr, ssize_t known_size);
int mpc_lowcomm_allocmem_is_in_pool(void *ptr);
size_t mpc_lowcomm_allocmem_relative_addr(void *in_pool_addr);
void * mpc_lowcomm_allocmem_abs_addr(size_t relative_addr);
void mpc_lowcomm_accumulate_op_lock();
void mpc_lowcomm_accumulate_op_unlock();


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
 * @brief Set the function to be called on this request completion
 * Note: deactivated without MPI_MPC. Unused without RMA Windows
 *
 * @param callback request completion callback
 */
void mpc_lowcomm_set_request_completion_callback(mpc_lowcomm_request_t *req, int callback(mpc_lowcomm_request_t *) );

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
/* Setup and Teardown when running as a library                         */
/************************************************************************/

/* In this configuration MPC will rely on the PMI to bootstrap */

/* To compile for this library:
   * If mpc has threads: gcc `mpc_cflags` `mpc_ldflags` -include mpc.h ./a.c
   * If mpc is without threads: gcc `mpc_cflags` `mpc_ldflags` ./a.c
*/

void mpc_lowcomm_init(void);
void mpc_lowcomm_release(void);

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_MAIN_HEADER_INC_H */

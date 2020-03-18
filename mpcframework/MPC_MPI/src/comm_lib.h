
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
#ifndef MPC_MPI_MPC_CL_H
#define MPC_MPI_MPC_CL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpc_mpi_comm_lib.h>

#include "datatype.h"
#include "egreq_classes.h"
#include "mpc_info.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/****************************
 * SPECIALIZED MESSAGE TAGS *
 ****************************/

#define MPC_GATHERV_TAG -2
#define MPC_GATHER_TAG -3
#define MPC_SCATTERV_TAG -4
#define MPC_SCATTER_TAG -5
#define MPC_ALLTOALL_TAG -6
#define MPC_ALLTOALLV_TAG -7
#define MPC_ALLTOALLW_TAG -8
#define MPC_BROADCAST_TAG -9
#define MPC_BARRIER_TAG -10
#define MPC_ALLGATHER_TAG -11
#define MPC_ALLGATHERV_TAG -12
#define MPC_REDUCE_TAG -13
#define MPC_ALLREDUCE_TAG -14
#define MPC_REDUCE_SCATTER_TAG -15
#define MPC_REDUCE_SCATTER_BLOCK_TAG -16
#define MPC_SCAN_TAG -17
#define MPC_EXSCAN_TAG -18
#define MPC_IBARRIER_TAG -19
#define MPC_IBCAST_TAG -20
#define MPC_IGATHER_TAG -21
#define MPC_IGATHERV_TAG -22
#define MPC_ISCATTER_TAG -23
#define MPC_ISCATTERV_TAG -24
#define MPC_IALLGATHER_TAG -25
#define MPC_IALLGATHERV_TAG -26
#define MPC_IALLTOALL_TAG -27
#define MPC_IALLTOALLV_TAG -28
#define MPC_IALLTOALLW_TAG -29
#define MPC_IREDUCE_TAG -30
#define MPC_IALLREDUCE_TAG -31
#define MPC_IREDUCE_SCATTER_TAG -32
#define MPC_IREDUCE_SCATTER_BLOCK_TAG -33
#define MPC_ISCAN_TAG -34
#define MPC_IEXSCAN_TAG -35
#define MPC_COPY_TAG -36
/* TAG of minimum value */
#define MPC_LAST_TAG -37

/********************
 * INIT AND RELEASE *
 ********************/

int _mpc_cl_init( int *argc, char ***argv );
int _mpc_cl_init_thread( int *argc, char ***argv, int required, int *provided );
int _mpc_cl_initialized( int *flag );
int _mpc_cl_finalize( void );
int _mpc_cl_abort( mpc_lowcomm_communicator_t, int );
int _mpc_cl_query_thread( int *provided );

/********************
 * GROUP MANAGEMENT *
 ********************/

typedef struct
{
	int task_nb;
	/* Task list rank are valid in COMM_WORLD  */
	int *task_list_in_global_ranks;
} _mpc_cl_group_t;

extern const _mpc_cl_group_t mpc_group_empty;
extern const _mpc_cl_group_t mpc_group_null;

#define MPC_GROUP_EMPTY &mpc_group_empty
#define MPC_GROUP_NULL ( (_mpc_cl_group_t *) NULL )

int _mpc_cl_comm_group( mpc_lowcomm_communicator_t, _mpc_cl_group_t ** );
int _mpc_cl_comm_remote_group( mpc_lowcomm_communicator_t, _mpc_cl_group_t ** );
int _mpc_cl_group_free( _mpc_cl_group_t ** );
int _mpc_cl_group_incl( _mpc_cl_group_t *, int, int *, _mpc_cl_group_t ** );
int _mpc_cl_group_difference( _mpc_cl_group_t *, _mpc_cl_group_t *, _mpc_cl_group_t ** );


/***********************************
 * (EXTENDED) GENERALIZED REQUESTS *
 ***********************************/

/** \brief This function starts a generic request
*
* \param query_fn Query function called to fill the status object
* \param free_fn Free function which frees extra arg
* \param cancel_fn Function called when the request is canceled
* \param extra_state Extra context passed to every handlers
* \param request New request to be created
*
* \warning Generalized Requests are not progressed by MPI the user are
*          in charge of providing the progress mechanism.
*
* Once the operation completes, the user has to call \ref _mpc_cl_grequest_complete
*
*/
int _mpc_cl_grequest_start( sctk_Grequest_query_function *query_fr, sctk_Grequest_free_function *free_fn,
                            sctk_Grequest_cancel_function *cancel_fn, void *extra_state, mpc_lowcomm_request_t *request );

/** \brief Starts an extended generalized request with a polling function
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int _mpc_cl_egrequest_start( sctk_Grequest_query_function *query_fn,
                             sctk_Grequest_free_function *free_fn,
                             sctk_Grequest_cancel_function *cancel_fn,
                             sctk_Grequest_poll_fn *poll_fn,
                             void *extra_state,
                             mpc_lowcomm_request_t *request );

/** \brief Flag a Generalized Request as finished
* \param request Request we want to finish
*/
int _mpc_cl_grequest_complete( mpc_lowcomm_request_t request );

/** \brief Starts a generalized request with a polling function
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int _mpc_cl_grequest_start( sctk_Grequest_query_function *query_fn,
                            sctk_Grequest_free_function *free_fn,
                            sctk_Grequest_cancel_function *cancel_fn,
                            void *extra_state, mpc_lowcomm_request_t *request );

/** \brief This creates a request class which can be referred to later on
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime (CAN BE NULL)
 *  \param wait_fn Wait function called when the runtime waits several requests
 * of the same type (CAN BE NULL)
 *  \param new_class The identifier of the class we are creating (output)
 */
int _mpc_cl_grequest_class_create( sctk_Grequest_query_function *query_fn,
                                   sctk_Grequest_cancel_function *cancel_fn,
                                   sctk_Grequest_free_function *free_fn,
                                   sctk_Grequest_poll_fn *poll_fn,
                                   sctk_Grequest_wait_fn *wait_fn,
                                   sctk_Request_class *new_class );

/** \brief Create a request linked to an \ref sctk_Request_class type
 *
 *  \param target_class Identifier of the class of work we launch as created by
 * \ref PMPIX_GRequest_class_create
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int _mpc_cl_grequest_class_allocate( sctk_Request_class target_class, void *extra_state, mpc_lowcomm_request_t *request );

/******************************
 * MPC COMMON INFO MANAGEMENT *
 ******************************/

/* Matches the one of MPI_INFO_NULL @ mpc_mpi.h:207 */
#define MPC_INFO_NULL ( -1 )
/* Maximum length for keys and values
 * they are both defined for MPC and MPI variants */
/*1 MB */
#define MPC_MAX_INFO_VAL 1048576
#define MPC_MAX_INFO_KEY 256

int _mpc_cl_info_set( MPC_Info, const char *, const char * );
int _mpc_cl_info_free( MPC_Info * );
int _mpc_cl_info_create( MPC_Info * );
int _mpc_cl_info_delete( MPC_Info, const char * );
int _mpc_cl_info_get( MPC_Info, const char *, int, char *, int * );
int _mpc_cl_info_dup( MPC_Info, MPC_Info * );
int _mpc_cl_info_get_nkeys( MPC_Info, int * );
int _mpc_cl_info_get_count( MPC_Info, int, char * );
int _mpc_cl_info_get_valuelen( MPC_Info, char *, int *, int * );


/*********************************
 * REQUEST AND STATUS MANAGEMENT *
 *********************************/

extern mpc_lowcomm_request_t mpc_request_null;

#define MPC_REQUEST_NULL mpc_request_null

int _mpc_cl_request_get_status( mpc_lowcomm_request_t request, int *flag, mpc_lowcomm_status_t *status );

int _mpc_cl_status_set_elements_x( mpc_lowcomm_status_t *status, mpc_lowcomm_datatype_t datatype, size_t count );
int _mpc_cl_status_set_elements( mpc_lowcomm_status_t *status, mpc_lowcomm_datatype_t datatype, int count );
int _mpc_cl_status_get_count( mpc_lowcomm_status_t *, mpc_lowcomm_datatype_t, mpc_lowcomm_msg_count_t * );


/********************
 * TOPOLOGY GETTERS *
 ********************/

int _mpc_cl_comm_rank( mpc_lowcomm_communicator_t comm, int *rank );
int _mpc_cl_comm_size( mpc_lowcomm_communicator_t comm, int *size );
int _mpc_cl_comm_remote_size( mpc_lowcomm_communicator_t comm, int *size );
int _mpc_cl_node_rank( int *rank );
int _mpc_cl_get_processor_name( char *name, int *resultlen );

/*************************
 * COLLECTIVE OPERATIONS *
 *************************/

int _mpc_cl_barrier( mpc_lowcomm_communicator_t comm );
int _mpc_cl_bcast( void *buffer, mpc_lowcomm_msg_count_t count,
                   mpc_lowcomm_datatype_t datatype, int root, mpc_lowcomm_communicator_t comm );
int _mpc_cl_gather( void *sendbuf, mpc_lowcomm_msg_count_t sendcnt,
                    mpc_lowcomm_datatype_t sendtype, void *recvbuf,
                    mpc_lowcomm_msg_count_t recvcount, mpc_lowcomm_datatype_t recvtype,
                    int root, mpc_lowcomm_communicator_t comm );
int _mpc_cl_allgather( void *sendbuf, mpc_lowcomm_msg_count_t sendcount,
                       mpc_lowcomm_datatype_t sendtype, void *recvbuf,
                       mpc_lowcomm_msg_count_t recvcount,
                       mpc_lowcomm_datatype_t recvtype, mpc_lowcomm_communicator_t comm );

int _mpc_cl_op_create( sctk_Op_User_function *, int, sctk_Op * );
int _mpc_cl_op_free( sctk_Op * );

/*****************************
 * POINT TO POINT OPERATIONS *
 *****************************/

int _mpc_cl_isend( void *buf, mpc_lowcomm_msg_count_t count,
                   mpc_lowcomm_datatype_t datatype, int dest, int tag,
                   mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request );

int _mpc_cl_ibsend( void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype, int dest,
                    int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request );


int _mpc_cl_issend( void *buf, mpc_lowcomm_msg_count_t count,
                    mpc_lowcomm_datatype_t datatype, int dest, int tag,
                    mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request );

int _mpc_cl_irsend( void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype, int dest,
                    int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request );



int _mpc_cl_irecv( void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype,
                   int source, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request );

int _mpc_cl_send( void *buf, mpc_lowcomm_msg_count_t count,
                  mpc_lowcomm_datatype_t datatype, int dest, int tag, mpc_lowcomm_communicator_t comm );

int _mpc_cl_ssend( void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype,
                   int dest, int tag, mpc_lowcomm_communicator_t comm );

int _mpc_cl_recv( void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype, int source,
                  int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_status_t *status );


/*******************
 * WAIT OPERATIONS *
 *******************/

int _mpc_cl_wait( mpc_lowcomm_request_t *request, mpc_lowcomm_status_t *status );

/** \brief Internal MPI_Waitall implementation relying on pointer to requests
 *
 *  This function is needed in order to call the MPC interface from
 *  the MPI one without having to rely on a set of MPI_Tests as before
 *  now the progress from MPI_Waitall functions can also be polled
 *
 *  \warning This function is not MPI_Waitall (see the mpc_lowcomm_request_t *
 * parray_of_requests[] argument)
 *
 */
int _mpc_cl_waitallp( mpc_lowcomm_msg_count_t count, mpc_lowcomm_request_t *parray_of_requests[],
                      mpc_lowcomm_status_t array_of_statuses[] );

int _mpc_cl_waitall( mpc_lowcomm_msg_count_t count, mpc_lowcomm_request_t array_of_requests[],
                     mpc_lowcomm_status_t array_of_statuses[] );

int _mpc_cl_waitsome( mpc_lowcomm_msg_count_t incount, mpc_lowcomm_request_t array_of_requests[],
                      mpc_lowcomm_msg_count_t *outcount, mpc_lowcomm_msg_count_t array_of_indices[],
                      mpc_lowcomm_status_t array_of_statuses[] );

int _mpc_cl_waitany( mpc_lowcomm_msg_count_t count, mpc_lowcomm_request_t array_of_requests[],
                     mpc_lowcomm_msg_count_t *index, mpc_lowcomm_status_t *status );



int _mpc_cl_test( mpc_lowcomm_request_t *request, int *flag, mpc_lowcomm_status_t *status );

int _mpc_cl_iprobe( int source, int tag, mpc_lowcomm_communicator_t comm, int *flag,
                    mpc_lowcomm_status_t *status );

int _mpc_cl_probe( int source, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_status_t *status );


/*****************
 * COMMUNICATORS *
 *****************/

int _mpc_cl_comm_create( mpc_lowcomm_communicator_t comm, _mpc_cl_group_t *group, mpc_lowcomm_communicator_t *comm_out );

int _mpc_cl_intercomm_create( mpc_lowcomm_communicator_t local_comm,
                              int local_leader, mpc_lowcomm_communicator_t peer_comm,
                              int remote_leader, int tag, mpc_lowcomm_communicator_t *newintercomm );

int _mpc_cl_comm_create_from_intercomm( mpc_lowcomm_communicator_t comm,
                                        _mpc_cl_group_t *group,
                                        mpc_lowcomm_communicator_t *comm_out );

int _mpc_cl_comm_free( mpc_lowcomm_communicator_t *comm );

int _mpc_cl_comm_dup( mpc_lowcomm_communicator_t incomm, mpc_lowcomm_communicator_t *outcomm );

int _mpc_cl_comm_split( mpc_lowcomm_communicator_t comm, int color, int key, mpc_lowcomm_communicator_t *comm_out );

/******************
 * ERROR HANDLING *
 ******************/

void _mpc_cl_default_error( mpc_lowcomm_communicator_t *comm, int *error, char *msg, char *file, int line );

void _mpc_cl_return_error( mpc_lowcomm_communicator_t *comm, int *error, ... );

int _mpc_cl_errhandler_create( MPC_Handler_function *function,
                               MPC_Errhandler *errhandler );

int _mpc_cl_errhandler_set( mpc_lowcomm_communicator_t comm, MPC_Errhandler errhandler );

int _mpc_cl_errhandler_get( mpc_lowcomm_communicator_t comm, MPC_Errhandler *errhandler );

int _mpc_cl_errhandler_free( MPC_Errhandler *errhandler );

int _mpc_cl_error_string( int code, char *str, int *len );

int _mpc_cl_error_class( int errorcode, int *errorclass );

/************************
 * TIMING AND PROFILING *
 ************************/

double _mpc_cl_wtime( void );
double _mpc_cl_wtick( void );


/**************
 * CHECKPOINT *
 **************/

/**
 * Trigger a checkpoint for the whole application.
 *
 * We are aware this functino implements somehow a barrier through three "expensive" atomics
 * But keep in mind that creating a checkpoint for the whole application is far more expensive
 * than that.
 *
 * This function sets "state" depending on the application state:
 *   - If we reach this function, it is a simple checkpoint -> MPC_STATE_CHECKPOINT
 *   - If we restart from a snapshot, it is a restart -> MPC_STATE_RESTART
 *
 * \param[out] state The state after the procedure.
 */
int _mpc_cl_checkpoint( MPC_Checkpoint_state *st );


/************************************************************************/
/* Per communicator context                                             */
/************************************************************************/

struct mpc_mpi_per_communicator_s;

typedef struct
{
	mpc_lowcomm_communicator_t key;

	mpc_common_spinlock_t err_handler_lock;
	MPC_Handler_function  *err_handler;

	struct mpc_mpi_per_communicator_s *mpc_mpi_per_communicator;
	void ( *mpc_mpi_per_communicator_copy )( struct mpc_mpi_per_communicator_s **, struct mpc_mpi_per_communicator_s * );
	void ( *mpc_mpi_per_communicator_copy_dup )( struct mpc_mpi_per_communicator_s **, struct mpc_mpi_per_communicator_s * );

	UT_hash_handle hh;
} mpc_per_communicator_t;

/** \brief Retrieves a given per communicator context from task CTX
 */
mpc_per_communicator_t *_mpc_cl_per_communicator_get( struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific, mpc_lowcomm_communicator_t comm );


/************************************************************************/
/* Per task context                                                     */
/************************************************************************/

/**
 *  \brief This structure describes an atexit handler for a task
 *
 *  \warning atexit is rewitten in mpc_main.h generated by the configure
 *            this is just in case you were wondering.
 */
struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s
{
	void ( *func )(); /**< Function to be called when task exits */
	struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s *next;  /**< Following function to call */
};

/**
 *  \brief Describes the context of an MPI task
 *
 *  This data structure is initialised by \ref __mpc_cl_per_mpi_process_ctx_init and
 * 	released by \ref __mpc_cl_per_mpi_process_ctx_release. Initial setup is done
 *  in \ref __mpc_cl_per_mpi_process_ctx_init.
 *
 */
typedef struct mpc_mpi_cl_per_mpi_process_ctx_s
{
	/* TODO */
	struct mpc_mpi_data_s *mpc_mpi_data;

	/* Communicator handling */
	mpc_per_communicator_t *per_communicator;
	mpc_common_spinlock_t per_communicator_lock;

	/* ID */
	int task_id; /**< MPI comm rank of the task */

	/* Status */
	int init_done; /**< =1 if the task has called MPI_Init() 2
			     =2 if the task has called MPI_Finalize */
	int thread_level;

	/* Data-types */
	struct _mpc_dt_storage *datatype_array;

	/* Extended Request Class handling */
	struct _mpc_egreq_classes_storage grequest_context;

	/* MPI_Info handling */
	struct MPC_Info_factory info_fact; /**< This structure is used to store the association
                                                between MPI_Infos structs and their ID */

	/* At EXIT */
	struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s  *exit_handlers; /**< These functions are called when tasks leaves (atexit) */

	/* For disguisement */
	struct sctk_thread_data_s *thread_data;

	/* Progresss List */
	struct _mpc_egreq_progress_list *progress_list;
} mpc_mpi_cl_per_mpi_process_ctx_t;

/***********************
 * DATATYPE MANAGEMENT *
 ***********************/

#define MPC_TYPE_NULL_DELETE_FN ( NULL )
#define MPC_TYPE_NULL_COPY_FN ( NULL )
#define MPC_TYPE_NULL_DUP_FN ( (void *) 0x3 )

/** \brief Compute the size of a \ref mpc_lowcomm_datatype_t
 *  \param datatype target datatype
 *  \param size where to write the size of datatype
 */
int _mpc_cl_type_size( mpc_lowcomm_datatype_t datatype, size_t *size );

/** \brief Checks if a datatype has already been released
 *  \param datatype target datatype
 *  \param flag 1 if the type is allocated [OUT]
 */
int _mpc_cl_type_is_allocated( mpc_lowcomm_datatype_t datatype, int *flag );

/** \brief Set a Struct datatype as a padded one to return the extent instead of
 * the size
 *  \param datatype to be flagged as padded
 */
int _mpc_cl_type_flag_padded( mpc_lowcomm_datatype_t datatype );

/** \brief This function is the generic initializer for
 * _mpc_dt_contiguout_t
 *  Creates a contiguous datatypes of count data_in while checking for unicity
 *
 *  \param outtype Output datatype to be created
 *  \param count Number of entries of type data_in
 *  \param data_in Type of the entry to be created
 *
 */
int _mpc_cl_type_hcontiguous( mpc_lowcomm_datatype_t *outtype, size_t count, mpc_lowcomm_datatype_t *data_in );


int _mpc_cl_type_free( mpc_lowcomm_datatype_t *datatype );

/** \brief Duplicate a  datatype
 *  \param old_type Type to be duplicated
 *  \param new_type Copy of old_type
 */
int _mpc_cl_type_dup( mpc_lowcomm_datatype_t old_type, mpc_lowcomm_datatype_t *new_type );

/** \brief Set a name to an mpc_lowcomm_datatype_t
 *  \param datatype Datatype to be named
 *  \param name Name to be set
 */
int _mpc_cl_type_set_name( mpc_lowcomm_datatype_t datatype, char *name );

/** \brief Get a name to an mpc_lowcomm_datatype_t
 *  \param datatype Datatype to get the name of
 *  \param name Datatype name (OUT)
 *  \param resultlen Maximum length of the target buffer (OUT)
 */
int _mpc_cl_type_get_name( mpc_lowcomm_datatype_t datatype, char *name, int *resultlen );

/** \brief This call is used to fill the envelope of an MPI type
 *
 *  \param datatype Source datatype
 *  \param num_integers Number of input integers [OUT]
 *  \param num_addresses Number of input addresses [OUT]
 *  \param num_datatypes Number of input datatypes [OUT]
 *  \param combiner Combiner used to build the datatype [OUT]
 *
 *
 */
int _mpc_cl_type_get_envelope( mpc_lowcomm_datatype_t datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner );

/**
 * @brief Retrieve the internal layout of a datatype (see MPI_Get_contents)
 *
 * See the description of MPI_Get_contents to decode these arrays
 *
 * @param datatype input datatype to get layout of
 * @param max_integers size of the int array (from get_envelope)
 * @param max_addresses size of the addr array (from get_envelope)
 * @param max_datatypes size of the dt array (from get_envelope)
 * @param array_of_integers Pointer to array of int
 * @param array_of_addresses Pointer to array of addresses
 * @param array_of_datatypes Pointer to array of datatypes
 * @return int MPC_SUCCESS if OK
 */
int _mpc_cl_type_get_contents( mpc_lowcomm_datatype_t datatype,
                               int max_integers,
                               int max_addresses,
                               int max_datatypes,
                               int array_of_integers[],
                               size_t array_of_addresses[],
                               mpc_lowcomm_datatype_t array_of_datatypes[] );

/**
 * @brief Commit and optimize a datatype
 *
 * @param type Type to commit
 * @return int MPC_SUCCESS if OK
 */
int _mpc_cl_type_commit( mpc_lowcomm_datatype_t *type );

/** \brief Serialize a derived datatype in a contiguous segment
 *  \param type the derived data-type to be serialized
 *  \param size (OUT) the size of the serialize buffer
 *  \param header_pad offset to allocate for the header (data will be shifted)
 *  \return the allocated buffer of size (size) to be used and freed
 */
void *_mpc_cl_derived_type_serialize( mpc_lowcomm_datatype_t type, size_t *size,
                                      size_t header_pad );

/** \brief Deserialize a derived datatype from a contiguous segment
 *  \param buff the buffer from a previously serialized datatype
 *  \param size size of the input buffer (as generated during serialize)
 *  \param header_pad offset to skip as being the header
 *  \return a new datatype matching the serialized one (to be freed)
 */
mpc_lowcomm_datatype_t _mpc_cl_derived_type_deserialize( void *buff, size_t size,
        size_t header_pad );

/** \brief This function gets the basic type constituing a derived type for RMA
 *  \param type Derived type to be checked
 *  \return -1 if types are differing, the type if not
 */
mpc_lowcomm_datatype_t _mpc_cl_type_get_inner( mpc_lowcomm_datatype_t type );


/* Types Keyval handling */
int _mpc_cl_type_free_keyval( int *type_keyval );
int _mpc_cl_type_create_keyval( MPC_Type_copy_attr_function *copy,
                                MPC_Type_delete_attr_function *deletef,
                                int *type_keyval, void *extra_state );
int _mpc_cl_type_delete_attr( mpc_lowcomm_datatype_t datatype, int type_keyval );
int _mpc_cl_type_set_attr( mpc_lowcomm_datatype_t datatype, int type_keyval,
                           void *attribute_val );
int _mpc_cl_type_get_attr( mpc_lowcomm_datatype_t datatype, int attribute_val,
                           void *type_keyval, int *flag );

struct _mpc_dt_context;
int _mpc_cl_derived_datatype( mpc_lowcomm_datatype_t *datatype,
                              long *begins,
                              long *ends,
                              mpc_lowcomm_datatype_t *types,
                              unsigned long count,
                              long lb, int is_lb,
                              long ub, int is_ub,
                              struct _mpc_dt_context *ectx );

int _mpc_cl_type_get_true_extent( mpc_lowcomm_datatype_t datatype, size_t *true_lb, size_t *true_extent );

int _mpc_cl_type_convert_to_derived( mpc_lowcomm_datatype_t in_datatype, mpc_lowcomm_datatype_t *out_datatype );

int _mpc_cl_type_use( mpc_lowcomm_datatype_t datatype );

_mpc_dt_contiguout_t *_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
        mpc_lowcomm_datatype_t datatype );
_mpc_dt_contiguout_t *_mpc_cl_per_mpi_process_ctx_contiguous_datatype_get( mpc_lowcomm_datatype_t datatype );

_mpc_dt_derived_t *_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get(  mpc_mpi_cl_per_mpi_process_ctx_t *task_specific, mpc_lowcomm_datatype_t datatype );
_mpc_dt_derived_t *_mpc_cl_per_mpi_process_ctx_derived_datatype_get( mpc_lowcomm_datatype_t datatype );

int _mpc_cl_type_hcontiguous_ctx ( mpc_lowcomm_datatype_t *datatype, size_t count, mpc_lowcomm_datatype_t *data_in, struct _mpc_dt_context *ctx );

int _mpc_cl_derived_datatype_try_get_info ( mpc_lowcomm_datatype_t datatype, int *res, _mpc_dt_derived_t *output_datatype );

int _mpc_cl_type_ctx_set( mpc_lowcomm_datatype_t datatype,  struct _mpc_dt_context *dctx );

int _mpc_cl_derived_datatype_on_slot ( int id,
                                       long *begins,
                                       long *ends,
                                       mpc_lowcomm_datatype_t *types,
                                       unsigned long count,
                                       long lb, int is_lb,
                                       long ub, int is_ub );

int _mpc_cl_type_set_size( mpc_lowcomm_datatype_t datatype, size_t size );


#ifdef __cplusplus
}
#endif

#endif /* MPC_MPI_MPC_CL_H */

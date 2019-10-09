
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
#ifndef __mpc__H
#define __mpc__H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpc_mpi_messaging.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/** \brief Macro to obtain the total number of datatypes */
#define MPC_TYPE_COUNT ( SCTK_COMMON_DATA_TYPE_COUNT + 2 * SCTK_USER_DATA_TYPES_MAX )

typedef mpc_mp_msg_count_t mpc_mp_msg_count_t;

typedef unsigned int mpc_pack_indexes_t;
typedef long mpc_pack_absolute_indexes_t;

typedef struct
{
	int task_nb;
	/* Task list rank are valid in COMM_WORLD  */
	int *task_list_in_global_ranks;
} _mpc_m_group_t;

extern const _mpc_m_group_t mpc_group_empty;
extern const _mpc_m_group_t mpc_group_null;

#define MPC_GROUP_EMPTY &mpc_group_empty
#define MPC_GROUP_NULL ( (_mpc_m_group_t *) NULL )

/* #define MPC_STATUS_SIZE 80 */
/* #define MPC_REQUEST_SIZE 30 */

struct sctk_thread_ptp_message_s;

/** Generalized requests functions **/
typedef sctk_Grequest_query_function MPC_Grequest_query_function;
typedef sctk_Grequest_cancel_function MPC_Grequest_cancel_function;
typedef sctk_Grequest_free_function MPC_Grequest_free_function;
/** Extended Generalized requests functions **/
typedef sctk_Grequest_poll_fn MPCX_Grequest_poll_fn;
typedef sctk_Grequest_wait_fn MPCX_Grequest_wait_fn;
/** Generalized Request classes **/
typedef sctk_Request_class MPCX_Request_class;

/** MPC Requests */
extern mpc_mp_request_t mpc_request_null;

#define MPC_REQUEST_NULL mpc_request_null



/* Type Keyval Defines */





#define MPC_TYPE_NULL_DELETE_FN ( NULL )
#define MPC_TYPE_NULL_COPY_FN ( NULL )
#define MPC_TYPE_NULL_DUP_FN ( (void *) 0x3 )

/********************************************************************/
/*Special TAGS                                                      */
/********************************************************************/
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

#define MPC_CREATE_INTERN_FUNC( name ) extern const sctk_Op MPC_##name

MPC_CREATE_INTERN_FUNC( SUM );
MPC_CREATE_INTERN_FUNC( MAX );
MPC_CREATE_INTERN_FUNC( MIN );
MPC_CREATE_INTERN_FUNC( PROD );
MPC_CREATE_INTERN_FUNC( LAND );
MPC_CREATE_INTERN_FUNC( BAND );
MPC_CREATE_INTERN_FUNC( LOR );
MPC_CREATE_INTERN_FUNC( BOR );
MPC_CREATE_INTERN_FUNC( LXOR );
MPC_CREATE_INTERN_FUNC( BXOR );
MPC_CREATE_INTERN_FUNC( MINLOC );
MPC_CREATE_INTERN_FUNC( MAXLOC );

/*TYPES*/


/* MPI Info management */

/* Matches the one of MPI_INFO_NULL @ mpc_mpi.h:207 */
#define MPC_INFO_NULL ( -1 )
/* Maximum length for keys and values
 * they are both defined for MPC and MPI variants */
/*1 MB */
#define MPC_MAX_INFO_VAL 1048576
#define MPC_MAX_INFO_KEY 256

/* Profiling Functions */

typedef struct
{
	int virtual_cpuid;
	double usage;
} MPC_Activity_t;

/*Initialisation */
int _mpc_m_init( int *argc, char ***argv );
int _mpc_m_init_thread( int *argc, char ***argv, int required, int *provided );
int _mpc_m_initialized( int *flag );
int _mpc_m_finalize( void );
int _mpc_m_abort( mpc_mp_communicator_t, int );
int _mpc_m_query_thread( int *provided );

/*Topology informations */
int _mpc_m_comm_rank( mpc_mp_communicator_t comm, int *rank );
int _mpc_m_comm_size( mpc_mp_communicator_t comm, int *size );
int _mpc_m_comm_remote_size( mpc_mp_communicator_t comm, int *size );
int _mpc_m_node_rank( int *rank );
int _mpc_m_get_processor_name( char *name, int *resultlen );

/*Collective operations */
int _mpc_m_barrier( mpc_mp_communicator_t comm );
int _mpc_m_bcast( void *buffer, mpc_mp_msg_count_t count,
				  mpc_mp_datatype_t datatype, int root, mpc_mp_communicator_t comm );
int _mpc_m_gather( void *sendbuf, mpc_mp_msg_count_t sendcnt,
				   mpc_mp_datatype_t sendtype, void *recvbuf,
				   mpc_mp_msg_count_t recvcount, mpc_mp_datatype_t recvtype,
				   int root, mpc_mp_communicator_t comm );
int _mpc_m_allgather( void *sendbuf, mpc_mp_msg_count_t sendcount,
					  mpc_mp_datatype_t sendtype, void *recvbuf,
					  mpc_mp_msg_count_t recvcount,
					  mpc_mp_datatype_t recvtype, mpc_mp_communicator_t comm );

int _mpc_m_op_create( sctk_Op_User_function *, int, sctk_Op * );
int _mpc_m_op_free( sctk_Op * );

/*P-t-P Communications */
int _mpc_m_isend( void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype,
				  int dest, int tag, mpc_mp_communicator_t comm, mpc_mp_request_t *request );
int _mpc_m_ibsend( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
				   mpc_mp_request_t * );
int _mpc_m_issend( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
				   mpc_mp_request_t * );
int _mpc_m_irsend( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
				   mpc_mp_request_t * );

int _mpc_m_irecv( void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype,
				  int source, int tag, mpc_mp_communicator_t comm, mpc_mp_request_t *request );

int _mpc_m_wait( mpc_mp_request_t *request, mpc_mp_status_t *status );
int _mpc_m_waitall( mpc_mp_msg_count_t, mpc_mp_request_t *, mpc_mp_status_t * );
int _mpc_m_waitsome( mpc_mp_msg_count_t, mpc_mp_request_t *, mpc_mp_msg_count_t *,
					 mpc_mp_msg_count_t *, mpc_mp_status_t * );
int _mpc_m_waitany( mpc_mp_msg_count_t count, mpc_mp_request_t array_of_requests[],
					mpc_mp_msg_count_t *index, mpc_mp_status_t *status );
int _mpc_m_wait_pending( mpc_mp_communicator_t comm );
int _mpc_m_wait_pending_all_comm( void );

int _mpc_m_test( mpc_mp_request_t *request, int *flag, mpc_mp_status_t *status );

int PMPC_Iprobe( int, int, mpc_mp_communicator_t, int *, mpc_mp_status_t * );
int PMPC_Probe( int, int, mpc_mp_communicator_t, mpc_mp_status_t * );
int _mpc_m_status_get_count( mpc_mp_status_t *, mpc_mp_datatype_t, mpc_mp_msg_count_t * );

int _mpc_m_send( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t );

int _mpc_m_ssend( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t );

int _mpc_m_recv( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
				 mpc_mp_status_t * );

int recv( void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, void *,
		  mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
		  mpc_mp_status_t * );

/*Status */
int mpc_mp_comm_request_cancel( mpc_mp_request_t * );

/*Groups */
int _mpc_m_comm_group( mpc_mp_communicator_t, _mpc_m_group_t ** );
int _mpc_m_comm_remote_group( mpc_mp_communicator_t, _mpc_m_group_t ** );
int _mpc_m_group_free( _mpc_m_group_t ** );
int _mpc_m_group_incl( _mpc_m_group_t *, int, int *, _mpc_m_group_t ** );
int _mpc_m_group_difference( _mpc_m_group_t *, _mpc_m_group_t *, _mpc_m_group_t ** );

/*Communicators */
int _mpc_m_comm_create( mpc_mp_communicator_t, _mpc_m_group_t *, mpc_mp_communicator_t * );
int _mpc_m_intercomm_create( mpc_mp_communicator_t local_comm, int local_leader, mpc_mp_communicator_t peer_comm, int remote_leader, int tag, mpc_mp_communicator_t *newintercomm );
int _mpc_m_comm_create_from_intercomm( mpc_mp_communicator_t, _mpc_m_group_t *, mpc_mp_communicator_t * );
int _mpc_m_comm_free( mpc_mp_communicator_t * );
int _mpc_m_comm_dup( mpc_mp_communicator_t, mpc_mp_communicator_t * );
int _mpc_m_comm_split( mpc_mp_communicator_t, int, int, mpc_mp_communicator_t * );

/*Error_handler */
void _mpc_m_default_error( mpc_mp_communicator_t *comm, int *error, char *msg, char *file, int line );
void _mpc_m_return_error( mpc_mp_communicator_t *comm, int *error, ... );

int _mpc_m_errhandler_create( MPC_Handler_function *, MPC_Errhandler * );
int _mpc_m_errhandler_set( mpc_mp_communicator_t, MPC_Errhandler );
int _mpc_m_errhandler_get( mpc_mp_communicator_t, MPC_Errhandler * );
int _mpc_m_errhandler_free( MPC_Errhandler * );
int _mpc_m_error_string( int, char *, int * );
int _mpc_m_error_class( int, int * );

/*Timing */
double _mpc_m_wtime( void );
double _mpc_m_wtick( void );

/*Types */
int _mpc_m_type_size( mpc_mp_datatype_t, size_t * );
int _mpc_m_type_is_allocated( mpc_mp_datatype_t datatype, int *flag );
int _mpc_m_type_flag_padded( mpc_mp_datatype_t datatype );
int _mpc_m_type_hcontiguous( mpc_mp_datatype_t *outtype, size_t count, mpc_mp_datatype_t *data_in );
int _mpc_m_type_free( mpc_mp_datatype_t *datatype );
int _mpc_m_type_dup( mpc_mp_datatype_t old_type, mpc_mp_datatype_t *new_type );
int _mpc_m_type_set_name( mpc_mp_datatype_t datatype, char *name );
int _mpc_m_type_get_name( mpc_mp_datatype_t datatype, char *name, int *resultlen );
int _mpc_m_type_get_envelope( mpc_mp_datatype_t datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner );
int _mpc_m_type_get_contents( mpc_mp_datatype_t datatype,
							  int max_integers,
							  int max_addresses,
							  int max_datatypes,
							  int array_of_integers[],
							  size_t array_of_addresses[],
							  mpc_mp_datatype_t array_of_datatypes[] );
int _mpc_m_type_commit( mpc_mp_datatype_t *type );

/* Types Keyval handling */
int _mpc_m_type_free_keyval( int *type_keyval );
int _mpc_m_type_create_keyval( MPC_Type_copy_attr_function *copy,
							   MPC_Type_delete_attr_function *deletef,
							   int *type_keyval, void *extra_state );
int _mpc_m_type_delete_attr( mpc_mp_datatype_t datatype, int type_keyval );
int _mpc_m_type_set_attr( mpc_mp_datatype_t datatype, int type_keyval,
						  void *attribute_val );
int _mpc_m_type_get_attr( mpc_mp_datatype_t datatype, int attribute_val,
						  void *type_keyval, int *flag );

struct Datatype_External_context;
int _mpc_m_derived_datatype( mpc_mp_datatype_t *datatype,
							 mpc_pack_absolute_indexes_t *begins,
							 mpc_pack_absolute_indexes_t *ends,
							 mpc_mp_datatype_t *types,
							 unsigned long count,
							 mpc_pack_absolute_indexes_t lb, int is_lb,
							 mpc_pack_absolute_indexes_t ub, int is_ub,
							 struct Datatype_External_context *ectx );

int _mpc_m_type_get_true_extent( mpc_mp_datatype_t datatype, size_t *true_lb, size_t *true_extent );
int _mpc_m_type_convert_to_derived( mpc_mp_datatype_t in_datatype, mpc_mp_datatype_t *out_datatype );
int _mpc_m_type_use( mpc_mp_datatype_t datatype );

/*Requests */
int mpc_mp_comm_request_free( mpc_mp_request_t * );

/*Scheduling */
int _mpc_m_checkpoint( MPC_Checkpoint_state *st );
int _mpc_m_get_activity( int nb_item, MPC_Activity_t *tab, double *process_act );

/*Packs */
int _mpc_m_open_pack( mpc_mp_request_t *request );

int _mpc_m_add_pack( void *buf, mpc_mp_msg_count_t count,
					 mpc_pack_indexes_t *begins,
					 mpc_pack_indexes_t *ends, mpc_mp_datatype_t datatype,
					 mpc_mp_request_t *request );
int _mpc_m_add_pack_absolute( void *buf, mpc_mp_msg_count_t count,
							  mpc_pack_absolute_indexes_t *begins,
							  mpc_pack_absolute_indexes_t *ends,
							  mpc_mp_datatype_t datatype, mpc_mp_request_t *request );

int _mpc_m_isend_pack( int dest, int tag, mpc_mp_communicator_t comm,
					   mpc_mp_request_t *request );
int _mpc_m_irecv_pack( int source, int tag, mpc_mp_communicator_t comm,
					   mpc_mp_request_t *request );

/* MPI Info management */

int _mpc_m_info_set( MPC_Info, const char *, const char * );
int _mpc_m_info_free( MPC_Info * );
int _mpc_m_info_create( MPC_Info * );
int _mpc_m_info_delete( MPC_Info, const char * );
int _mpc_m_info_get( MPC_Info, const char *, int, char *, int * );
int _mpc_m_info_dup( MPC_Info, MPC_Info * );
int _mpc_m_info_get_nkeys( MPC_Info, int * );
int _mpc_m_info_get_count( MPC_Info, int, char * );
int _mpc_m_info_get_valuelen( MPC_Info, char *, int *, int * );

/* Generalized Requests */

int _mpc_m_grequest_start( MPC_Grequest_query_function *query_fr, MPC_Grequest_free_function *free_fn,
						   MPC_Grequest_cancel_function *cancel_fn, void *extra_state, mpc_mp_request_t *request );

int _mpc_m_grequest_complete( mpc_mp_request_t request );

/* Extended Generalized Requests */

int _mpc_m_egrequest_start( MPC_Grequest_query_function *query_fn,
							MPC_Grequest_free_function *free_fn,
							MPC_Grequest_cancel_function *cancel_fn,
							MPCX_Grequest_poll_fn *poll_fn,
							void *extra_state,
							mpc_mp_request_t *request );

/* Extended Generalized Requests Classes */
int _mpc_m_grequest_class_create( MPC_Grequest_query_function *query_fn,
								  MPC_Grequest_cancel_function *cancel_fn,
								  MPC_Grequest_free_function *free_fn,
								  MPCX_Grequest_poll_fn *poll_fn,
								  MPCX_Grequest_wait_fn *wait_fn,
								  MPCX_Request_class *new_class );

int _mpc_m_grequest_class_allocate( MPCX_Request_class target_class, void *extra_state, mpc_mp_request_t *request );

int _mpc_m_request_get_status( mpc_mp_request_t request, int *flag, mpc_mp_status_t *status );
int _mpc_m_status_set_elements_x( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, size_t count );
int _mpc_m_status_set_elements( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, int count );

#ifdef __cplusplus
}
#endif
#endif

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
#ifndef SCTK_TYPES_H
#define SCTK_TYPES_H
/************************** HEADERS **************************/
#include <stdlib.h>

#include <mpc_lowcomm_communicator.h>

/************************** TYPES **************************/
/** define a data-type **/
typedef int mpc_lowcomm_datatype_t;
/** Message count **/
typedef int mpc_lowcomm_msg_count_t;
/** A message header to be put in the request **/
typedef struct
{
	mpc_lowcomm_peer_uid_t source;
	mpc_lowcomm_peer_uid_t destination;
	int destination_task;
	int source_task;
	int message_tag;
	unsigned int communicator_id;
	mpc_lowcomm_msg_count_t msg_size;
} sctk_header_t;
/** Status Definition **/
typedef struct
{
	int MPC_SOURCE;		/**< Source of the Message */
	int MPC_TAG;		/**< Tag of the message */
	int MPC_ERROR;		/**< Did we encounter an error */
	int cancelled;		/**< Was the message canceled */
	mpc_lowcomm_msg_count_t size;	/**< Size of the message */
} mpc_lowcomm_status_t;

#define MPC_LOWCOMM_STATUS_NULL NULL
#define MPC_LOWCOMM_STATUS_INIT {MPC_ANY_SOURCE,MPC_ANY_TAG,MPC_LOWCOMM_SUCCESS,0,0}


/** Generalized requests functions **/
typedef int sctk_Grequest_query_function( void *extra_state, mpc_lowcomm_status_t *status );
typedef int sctk_Grequest_cancel_function( void *extra_state, int complete );
typedef int sctk_Grequest_free_function( void *extra_state );
/** Extended Generalized requests functions **/
typedef int sctk_Grequest_poll_fn( void *extra_state, mpc_lowcomm_status_t *status );
typedef int sctk_Grequest_wait_fn( int count, void **array_of_states, double timeout, mpc_lowcomm_status_t *status );
/** Generalized Request classes **/
typedef int sctk_Request_class;

/** Request definition **/
typedef struct mpc_lowcomm_request_t mpc_lowcomm_request_t;
struct mpc_lowcomm_request_t
{
	mpc_lowcomm_communicator_t comm;
	volatile int completion_flag;
    char pad[128];
	int request_type;
	sctk_header_t header;
	struct mpc_lowcomm_ptp_message_s *msg;
	mpc_lowcomm_datatype_t source_type; /**< Type in the remote message */
	mpc_lowcomm_datatype_t dest_type; /**< Type in the local message */
	int is_null;
	int truncated;
	int status_error;
	/* int ref rank */
	/* Generalized Request context  */
	int grequest_rank;
	void *progress_unit;
	sctk_Grequest_query_function *query_fn;
	sctk_Grequest_cancel_function *cancel_fn;
	sctk_Grequest_free_function *free_fn;
	void *extra_state;
	/* Extended Request */
	sctk_Grequest_poll_fn *poll_fn;
	sctk_Grequest_wait_fn *wait_fn;
	/* MPI_Grequest_complete takes a copy of the struct
	 * not a reference however we have to change a value
	 * in the source struct which is being pulled therefore
	 * we have to do this hack by saving a pointer to the
	 * request inside the request */
	void *pointer_to_source_request;
	/* This is a pointer to the shadow request */
	void *pointer_to_shadow_request;
	/* This is a pointer to the registered memory region
	 * in order to unpin when request completes */
	void *ptr_to_pin_ctx;
        int (*request_completion_fn)(mpc_lowcomm_request_t *);
};

mpc_lowcomm_request_t* mpc_lowcomm_request_null(void);
#define MPC_REQUEST_NULL (*mpc_lowcomm_request_null())

/** Reduction Operations **/
typedef void ( *sctk_Op_f ) ( const void *, void *, size_t, mpc_lowcomm_datatype_t );
typedef void ( sctk_Op_User_function ) ( void *, void *, int *, mpc_lowcomm_datatype_t * );

typedef struct
{
	sctk_Op_f func;
	sctk_Op_User_function *u_func;
} sctk_Op;

#define SCTK_OP_INIT {NULL,NULL}

/** RDMA windows */
typedef int mpc_lowcomm_rdma_window_t;

typedef enum
{
	RDMA_OP_NULL,
	RDMA_SUM,
	RDMA_INC,
	RDMA_DEC,
	RDMA_MIN,
	RDMA_MAX,
	RDMA_PROD,
	RDMA_LAND,
	RDMA_BAND,
	RDMA_LOR,
	RDMA_BOR,
	RDMA_LXOR,
	RDMA_BXOR
} RDMA_op;

typedef enum
{
	RDMA_TYPE_NULL,
	RDMA_TYPE_CHAR,
	RDMA_TYPE_DOUBLE,
	RDMA_TYPE_FLOAT,
	RDMA_TYPE_INT,
	RDMA_TYPE_LONG,
	RDMA_TYPE_LONG_DOUBLE,
	RDMA_TYPE_LONG_LONG,
	RDMA_TYPE_LONG_LONG_INT,
	RDMA_TYPE_SHORT,
	RDMA_TYPE_SIGNED_CHAR,
	RDMA_TYPE_UNSIGNED,
	RDMA_TYPE_UNSIGNED_CHAR,
	RDMA_TYPE_UNSIGNED_LONG,
	RDMA_TYPE_UNSIGNED_LONG_LONG,
	RDMA_TYPE_UNSIGNED_SHORT,
	RDMA_TYPE_WCHAR,
	RDMA_TYPE_AINT
} RDMA_type;

size_t RDMA_type_size( RDMA_type type );

/*******************
 * DataTypes        *
 *******************/

void mpc_lowcomm_register_type_is_common( int (*type_ptr)(mpc_lowcomm_datatype_t) );

#define MPC_PACKED 0
#define MPC_BYTE 1

/*********
 * ERROR *
 *********/

enum
{
	MPC_LOWCOMM_ERR_TRUNCATE, 	/* Message truncated on receive */
	MPC_LOWCOMM_ERR_TYPE,	/* Invalid datatype argument */
	MPC_LOWCOMM_ERR_PENDING, /* Pending request */

	MPC_LOWCOMM_ERR_LASTCODE
};

/*******************
 * FAULT TOLERANCE *
 *******************/

typedef enum sctk_ft_state_e
{
	MPC_STATE_NO_SUPPORT = 0,
	MPC_STATE_ERROR,
	MPC_STATE_CHECKPOINT,
	MPC_STATE_RESTART,
	MPC_STATE_IGNORE,
	MPC_STATE_COUNT
} mpc_lowcomm_checkpoint_state_t;



/************************** MACROS *************************/

#define MPC_ROOT -4

/** Define the NULL error handler */
#define SCTK_ERRHANDLER_NULL 0
/** Not using datatypes */
#define MPC_DATATYPE_IGNORE ((mpc_lowcomm_datatype_t)-1)
/** In place collectives **/
#define MPC_IN_PLACE ((void*)-1)
/** PROC_NULL **/
#define MPC_PROC_NULL -2
/** SUCCESS and ERROR **/
#define MPC_LOWCOMM_SUCCESS 0
#define MPC_LOWCOMM_ERROR 1

/** Wildcards **/
#define MPC_ANY_TAG -1
#define MPC_ANY_SOURCE -1

/****************************
 * SPECIALIZED MESSAGE TAGS *
 ****************************/

/* Ensure sync with comm_lib.h */
#define MPC_GATHER_TAG -3
#define MPC_BROADCAST_TAG -9
#define MPC_BARRIER_TAG -10
#define MPC_ALLGATHER_TAG -11


#endif /* SCTK_TYPES_H */

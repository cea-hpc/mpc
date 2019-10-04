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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_INTER_THREAD_COMMUNICATIONS_H_
#define __SCTK_INTER_THREAD_COMMUNICATIONS_H_

#include <sctk_config.h>
#include <sctk_debug.h>
#include <sctk_communicator.h>
#include <mpc_mp_coll.h>
#include <sctk_reorder.h>

#include <mpc_common_asm.h>
#include <sctk_types.h>

#include <sctk_ib.h>
#include <sctk_portals.h>
#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* sctk_request_t		                                                    */
/************************************************************************/
typedef enum {
	REQUEST_NULL = 0,
	REQUEST_SEND,
	REQUEST_RECV,
	REQUEST_GENERALIZED,
	REQUEST_PICKED,
	REQUEST_RDMA
} sctk_request_type_t;

/************************************************************************/
/* Messages Types                                               */
/************************************************************************/

/** This defines the type of a message */
typedef enum {
	SCTK_CLASS_NONE,
	SCTK_CANCELLED_SEND,
	SCTK_CANCELLED_RECV,

	SCTK_P2P_MESSAGE,
	SCTK_RDMA_MESSAGE,
	SCTK_RDMA_WINDOW_MESSAGES,  /**< These messages are used to exchange window  informations */
	SCTK_CONTROL_MESSAGE_FENCE, /**< This message is sent to create a fence on control messages */

	SCTK_BARRIER_MESSAGE,
	SCTK_BROADCAST_MESSAGE,
	SCTK_ALLREDUCE_MESSAGE,

	SCTK_ALLREDUCE_HETERO_MESSAGE,
	SCTK_BROADCAST_HETERO_MESSAGE,
	SCTK_BARRIER_HETERO_MESSAGE,

	SCTK_BARRIER_OFFLOAD_MESSAGE,
	SCTK_BROADCAST_OFFLOAD_MESSAGE,
	SCTK_REDUCE_OFFLOAD_MESSAGE,
	SCTK_ALLREDUCE_OFFLOAD_MESSAGE,

	SCTK_CONTROL_MESSAGE_INTERNAL, /**< This message is to be used inside a rail logic */
	SCTK_CONTROL_MESSAGE_RAIL,	 /**< This message goes to a rail */
	SCTK_CONTROL_MESSAGE_PROCESS,  /**< This message goes to a process (\ref sctk_control_message_process_level) */
	SCTK_CONTROL_MESSAGE_TASK,	 /**< This message goes to a task */
	SCTK_CONTROL_MESSAGE_USER,	 /**< This message goes to the application using an optionnal handler */
	SCTK_MESSAGE_CLASS_COUNT	   /**< This value allows to track the  number of control message types */
} sctk_message_class_t;

static const char *const sctk_message_class_name[SCTK_MESSAGE_CLASS_COUNT] = {
	"SCTK_CLASS_NONE",
	"SCTK_CANCELLED_SEND",
	"SCTK_CANCELLED_RECV",

	"SCTK_P2P_MESSAGE",
	"SCTK_RDMA_MESSAGE",
	"SCTK_RDMA_WINDOW_MESSAGES",
	"SCTK_CONTROL_MESSAGE_FENCE",

	"SCTK_BARRIER_MESSAGE",
	"SCTK_BROADCAST_MESSAGE",
	"SCTK_ALLREDUCE_MESSAGE",

	"SCTK_ALLREDUCE_HETERO_MESSAGE",
	"SCTK_BROADCAST_HETERO_MESSAGE",
	"SCTK_BARRIER_HETERO_MESSAGE",

	"SCTK_BARRIER_OFFLOAD_MESSAGE",
	"SCTK_BROADCAST_OFFLOAD_MESSAGE",
	"SCTK_REDUCE_OFFLOAD_MESSAGE",
	"SCTK_ALLREDUCE_OFFLOAD_MESSAGE",

	"SCTK_CONTROL_MESSAGE_INTERNAL",
	"SCTK_CONTROL_MESSAGE_RAIL",
	"SCTK_CONTROL_MESSAGE_PROCESS",
	"SCTK_CONTROL_MESSAGE_TASK",
	"SCTK_CONTROL_MESSAGE_USER"};

static inline int _mpc_comm_ptp_message_is_for_process( sctk_message_class_t type )
{
	switch ( type )
	{
		case SCTK_CLASS_NONE:
			not_implemented();
		case SCTK_CANCELLED_SEND:
		case SCTK_CANCELLED_RECV:
		case SCTK_P2P_MESSAGE:
		case SCTK_RDMA_MESSAGE:
		case SCTK_BARRIER_MESSAGE:
		case SCTK_BROADCAST_MESSAGE:
		case SCTK_ALLREDUCE_MESSAGE:
		case SCTK_CONTROL_MESSAGE_TASK:
		case SCTK_CONTROL_MESSAGE_FENCE:
		case SCTK_RDMA_WINDOW_MESSAGES: /* Note that the RDMA win message
                                           is not process specific to force
                                           on-demand connections between the
                                           RDMA peers prior to emitting RDMA */

			return 0;

		case SCTK_CONTROL_MESSAGE_INTERNAL:
		case SCTK_ALLREDUCE_HETERO_MESSAGE:
		case SCTK_BROADCAST_HETERO_MESSAGE:
		case SCTK_BARRIER_HETERO_MESSAGE:
		case SCTK_BARRIER_OFFLOAD_MESSAGE:
		case SCTK_BROADCAST_OFFLOAD_MESSAGE:
		case SCTK_REDUCE_OFFLOAD_MESSAGE:
		case SCTK_ALLREDUCE_OFFLOAD_MESSAGE:
		case SCTK_CONTROL_MESSAGE_RAIL:
		case SCTK_CONTROL_MESSAGE_PROCESS:
		case SCTK_CONTROL_MESSAGE_USER:
			return 1;

		case SCTK_MESSAGE_CLASS_COUNT:
			return 0;
	}

	return 0;
}

static inline int _mpc_comm_ptp_message_is_for_control( sctk_message_class_t type )
{
	switch ( type )
	{
		case SCTK_CLASS_NONE:
			not_implemented();

		case SCTK_CANCELLED_SEND:
		case SCTK_CANCELLED_RECV:
		case SCTK_P2P_MESSAGE:
		case SCTK_RDMA_MESSAGE:
		case SCTK_BARRIER_MESSAGE:
		case SCTK_BROADCAST_MESSAGE:
		case SCTK_ALLREDUCE_MESSAGE:
		case SCTK_ALLREDUCE_HETERO_MESSAGE:
		case SCTK_BROADCAST_HETERO_MESSAGE:
		case SCTK_BARRIER_HETERO_MESSAGE:
		case SCTK_BARRIER_OFFLOAD_MESSAGE:
		case SCTK_BROADCAST_OFFLOAD_MESSAGE:
		case SCTK_REDUCE_OFFLOAD_MESSAGE:
		case SCTK_ALLREDUCE_OFFLOAD_MESSAGE:
		case SCTK_RDMA_WINDOW_MESSAGES:
		case SCTK_CONTROL_MESSAGE_FENCE:
		case SCTK_CONTROL_MESSAGE_INTERNAL:
			return 0;

		case SCTK_CONTROL_MESSAGE_TASK:
		case SCTK_CONTROL_MESSAGE_RAIL:
		case SCTK_CONTROL_MESSAGE_PROCESS:
		case SCTK_CONTROL_MESSAGE_USER:
			return 1;

		case SCTK_MESSAGE_CLASS_COUNT:
			return 0;
	}

	return 0;
}

/************************************************************************/
/* Low Level Message Interface                                          */
/************************************************************************/

void mpc_mp_comm_isend_class_src( int src, int dest, void *data, size_t size, int tag, sctk_communicator_t comm, sctk_message_class_t class, sctk_request_t *req );
void mpc_mp_comm_irecv_class_dest( int src, int dest, void *buffer, size_t size, int tag, sctk_communicator_t comm, sctk_message_class_t class, sctk_request_t *req );
void mpc_mp_comm_isend_class( int dest, void *data, size_t size, int tag, sctk_communicator_t comm, sctk_message_class_t class, sctk_request_t *req );
void mpc_mp_comm_irecv_class( int src, void *data, size_t size, int tag, sctk_communicator_t comm, sctk_message_class_t class, sctk_request_t *req );
void mpc_mp_comm_isend( int dest, void *data, size_t size, int tag, sctk_communicator_t comm, sctk_request_t *req );
void mpc_mp_comm_irecv( int src, void *data, size_t size, int tag, sctk_communicator_t comm, sctk_request_t *req );
void mpc_mp_comm_sendrecv( void *sendbuf, size_t size, int dest, int tag, void *recvbuf, int src, int comm );

/************************************************************************/
/* Control Messages Header                                              */
/************************************************************************/

/** This is the content of a control message */
struct sctk_control_message_header
{
	char type;	/**< Type of the message determining the action */
	char subtype; /**< Subtype of the message (can be freely set -- usually to do a switch) */
	char param;   /**< Parameter value (depending on type and subtypes)
					             for rails it is used to store rail number */
	char rail_id; /**< The id of the rail sending the message (set during multirail selection \ref sctk_multirail_send_message)
					             it allows RAIL level messages to be routed accordingly */
};

/************************************************************************/
/* sctk_thread_message_header_t                                         */
/************************************************************************/

/** P2P message header */
typedef struct sctk_thread_message_header_s
{
	/* Process */
	int source;		 /**< Source Process */
	int destination; /**< Destination Process */
	/* Task */
	int source_task;	  /**< Source Task id */
	int destination_task; /**< Destination Task ID */
	/* Context */
	int message_tag;								 /**< Message TAG */
	sctk_communicator_t communicator;				 /**< Message communicator */
	struct sctk_control_message_header message_type; /**< Control Message Infos */
	/* Ordering */
	int message_number;			/**< Message order (for reorder) */
	char use_message_numbering; /**< Should this message be reordered */
	/* Content */
	sctk_datatype_t datatype; /**< Caried data-type (for matching check) */
	size_t msg_size;		  /**< Message size */
} sctk_thread_message_header_t;

void mpc_mp_comm_message_probe_any_tag( int destination, int source, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_mp_comm_message_probe_any_source_any_tag( int destination, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_mp_comm_message_probe( int destination, int source, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_mp_comm_message_probe_any_source( int destination, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_mp_comm_message_probe_any_source_class( int destination, int tag,
						sctk_message_class_t class,
						const sctk_communicator_t comm,
						int *status,
						sctk_thread_message_header_t *msg );

void mpc_mp_comm_message_probe_any_source_class_comm( int destination, int tag,
							sctk_message_class_t class,
							const sctk_communicator_t comm,
							int *status,
							sctk_thread_message_header_t *msg );

/************************************************/

/************************************************************************/
/* Message Content Descriptors                                          */
/************************************************************************/

typedef unsigned int sctk_pack_indexes_t;
typedef long sctk_pack_absolute_indexes_t;
typedef unsigned int sctk_count_t;

typedef enum {
	SCTK_MESSAGE_CONTIGUOUS,
	SCTK_MESSAGE_PACK,
	SCTK_MESSAGE_PACK_ABSOLUTE,
	SCTK_MESSAGE_PACK_UNDEFINED,
	SCTK_MESSAGE_NETWORK
} sctk_message_type_t;

/** SCTK_MESSAGE_CONTIGUOUS */
typedef struct
{
	size_t size;
	void *addr;
} sctk_message_contiguous_t;

/** SCTK_MESSAGE_PACK */
typedef struct
{
	sctk_count_t count;
	sctk_pack_indexes_t *begins;
	sctk_pack_indexes_t *ends;
	void *addr;
	size_t elem_size;
} sctk_message_pack_std_list_t;

/** SCTK_MESSAGE_PACK_ABSOLUTE */
typedef struct
{
	sctk_count_t count;
	sctk_pack_absolute_indexes_t *begins;
	sctk_pack_absolute_indexes_t *ends;
	void *addr;
	size_t elem_size;
} sctk_message_pack_absolute_list_t;

/** Content for list of packs */
typedef union {
	sctk_message_pack_absolute_list_t *absolute;
	sctk_message_pack_std_list_t *std;
} sctk_message_pack_list_t;

typedef struct
{
	size_t count;
	size_t max_count;
	sctk_message_pack_list_t list;
} sctk_message_pack_t;

/** Content single packed message */
typedef union {
	sctk_message_pack_absolute_list_t absolute;
	sctk_message_pack_std_list_t std;
} sctk_message_pack_default_t;

/** Message Content descriptor */
typedef union {
	sctk_message_contiguous_t contiguous; /** Contiguous case */
	sctk_message_pack_t pack;			  /** Packed case */
} sctk_message_t;

/************************************************************************/
/* Message List                                                         */
/************************************************************************/

struct sctk_thread_ptp_message_s;

typedef volatile struct sctk_msg_list_s
{
	struct sctk_thread_ptp_message_s *msg;
	volatile struct sctk_msg_list_s *prev, *next;
} sctk_msg_list_t;

/************************************************************************/
/* Message Copy                                                         */
/************************************************************************/

typedef struct sctk_message_to_copy_s
{
	struct sctk_thread_ptp_message_s *msg_send;
	struct sctk_thread_ptp_message_s *msg_recv;
	struct sctk_message_to_copy_s *prev, *next;
} sctk_message_to_copy_t;

void mpc_mp_comm_ptp_message_copy( sctk_message_to_copy_t *tmp );
void mpc_mp_comm_ptp_message_copy_pack( sctk_message_to_copy_t *tmp );
void mpc_mp_comm_ptp_message_copy_pack_absolute( sctk_message_to_copy_t *tmp );

/************************************************************************/
/* Message Content                                                      */
/************************************************************************/

typedef enum {
	SCTK_MESSAGE_PENDING = 0,
	SCTK_MESSAGE_DONE = 1,
	SCTK_MESSAGE_CANCELED = 2
} sctk_completion_flag_t;

/*Data to tranfers in inter-process communications*/
typedef struct
{
	sctk_thread_message_header_t header;
#ifdef SCTK_USE_CHECKSUM
	unsigned long checksum;
#endif
} sctk_thread_ptp_message_body_t;

struct mpc_buffered_msg_s;
struct mpc_comm_ptp_s;

/*Data not to tranfers in inter-process communications*/
typedef struct
{
	char remote_source;
	char remote_destination;

	volatile int *volatile completion_flag;

	sctk_request_t *request;

	/*Message data*/
	sctk_message_type_t message_type;
	sctk_message_t message;
	sctk_message_pack_default_t default_pack;

	/*Storage structs*/
	sctk_msg_list_t distant_list;
	sctk_message_to_copy_t copy_list;

	struct mpc_comm_ptp_s *internal_ptp;

	/*Destructor*/
	void ( *free_memory )( void * );

	/*Copy operator*/
	void ( *message_copy )( sctk_message_to_copy_t * );

	/*Reorder buffer struct*/
	sctk_reorder_buffer_t reorder;

	/* If the message has been buffered during the
	* Send function. If it is, we need to free the async
	* buffer when completing the message */
	struct mpc_buffered_msg_s *buffer_async;

	/* RDMA infos */
	void *rdma_src;
	void *route_table;

#ifdef MPC_USE_PORTALS
	/* Portals infos */
	struct sctk_ptl_tail_s ptl;
#endif

	/* Matchind ID used for MPROBE */
	OPA_int_t matching_id; /**< 0 By default unique id otherwise */

	/* XXX:Specific to IB */
	struct sctk_ib_msg_header_s ib;
} sctk_thread_ptp_message_tail_t;

/************************************************************************/
/* sctk_thread_ptp_message_t Point to Point message descriptor          */
/************************************************************************/

typedef struct sctk_thread_ptp_message_s
{
	sctk_thread_ptp_message_body_t body;
	sctk_thread_ptp_message_tail_t tail;

	/* Pointers for chaining elements */
	struct sctk_thread_ptp_message_s *prev;
	struct sctk_thread_ptp_message_s *next;

	/* If the entry comes from a buffered list */
	char from_buffered;
} sctk_thread_ptp_message_t;

void mpc_mp_comm_ptp_message_header_clear( sctk_thread_ptp_message_t *tmp, sctk_message_type_t msg_type, void ( *free_memory )( void * ),
										   void ( *message_copy )( sctk_message_to_copy_t * ) );

sctk_thread_ptp_message_t *mpc_mp_comm_ptp_message_header_create( sctk_message_type_t msg_type );

void mpc_mp_comm_ptp_message_set_contiguous_addr( sctk_thread_ptp_message_t *restrict msg, void *restrict addr, const size_t size );

void mpc_mp_comm_ptp_message_add_pack( sctk_thread_ptp_message_t *msg, void *adr, const sctk_count_t nb_items,
					const size_t elem_size,
					sctk_pack_indexes_t *begins,
					sctk_pack_indexes_t *ends );

void mpc_mp_comm_ptp_message_add_pack_absolute( sctk_thread_ptp_message_t *msg, void *adr,
						const sctk_count_t nb_items,
						const size_t elem_size,
						sctk_pack_absolute_indexes_t *begins,
						sctk_pack_absolute_indexes_t *ends );

void mpc_mp_comm_ptp_message_header_init( sctk_thread_ptp_message_t *msg, const int message_tag,
					const sctk_communicator_t communicator,
					const int source,
					const int destination,
					sctk_request_t *request,
					const size_t count,
					sctk_message_class_t message_class,
					sctk_datatype_t datatype,
					sctk_request_type_t request_type );

void mpc_mp_comm_ptp_message_send( sctk_thread_ptp_message_t *msg );

void mpc_mp_comm_ptp_message_recv( sctk_thread_ptp_message_t *msg );

void mpc_mp_comm_ptp_message_complete_and_free( sctk_thread_ptp_message_t *msg );

void _mpc_comm_ptp_message_send_check( sctk_thread_ptp_message_t *msg, int perform_check );
void _mpc_comm_ptp_message_recv_check( sctk_thread_ptp_message_t *msg, int perform_check );
void _mpc_comm_ptp_message_commit_request( sctk_thread_ptp_message_t *send, sctk_thread_ptp_message_t *recv );

static inline void _mpc_comm_ptp_message_clear_request( sctk_thread_ptp_message_t *msg )
{
	msg->tail.request = NULL;
	msg->tail.internal_ptp = NULL;
}

static inline void _mpc_comm_ptp_message_set_copy_and_free( sctk_thread_ptp_message_t *tmp,
							void ( *free_memory )( void * ),
							void ( *message_copy )( sctk_message_to_copy_t * ) )
{
	tmp->tail.free_memory = free_memory;
	tmp->tail.message_copy = message_copy;
	tmp->tail.buffer_async = NULL;

	memset( &tmp->tail.message.pack, 0, sizeof( tmp->tail.message.pack ) );
}

/** sctk_thread_message_header_t GETTERS and Setters */

#define SCTK_MSG_USE_MESSAGE_NUMBERING( msg ) msg->body.header.use_message_numbering
#define SCTK_MSG_USE_MESSAGE_NUMBERING_SET( msg, num ) \
	do                                                 \
	{                                                  \
		msg->body.header.use_message_numbering = num;  \
	} while ( 0 )

#define SCTK_MSG_SRC_PROCESS( msg ) msg->body.header.source
#define SCTK_MSG_SRC_PROCESS_SET( msg, src ) \
	do                                       \
	{                                        \
		msg->body.header.source = src;       \
	} while ( 0 )

#define SCTK_MSG_DEST_PROCESS( msg ) msg->body.header.destination
#define SCTK_MSG_DEST_PROCESS_SET( msg, dest ) \
	do                                         \
	{                                          \
		msg->body.header.destination = dest;   \
	} while ( 0 )

#define SCTK_MSG_SRC_TASK( msg ) msg->body.header.source_task
#define SCTK_MSG_SRC_TASK_SET( msg, src )   \
	do                                      \
	{                                       \
		msg->body.header.source_task = src; \
	} while ( 0 )

#define SCTK_MSG_DEST_TASK( msg ) msg->body.header.destination_task
#define SCTK_MSG_DEST_TASK_SET( msg, dest )       \
	do                                            \
	{                                             \
		msg->body.header.destination_task = dest; \
	} while ( 0 )

#define SCTK_MSG_COMMUNICATOR( msg ) msg->body.header.communicator
#define SCTK_MSG_COMMUNICATOR_SET( msg, comm ) \
	do                                         \
	{                                          \
		msg->body.header.communicator = comm;  \
	} while ( 0 )

#define SCTK_DATATYPE( msg ) msg->body.header.datatype
#define SCTK_MSG_DATATYPE_SET( msg, type ) \
	do                                     \
	{                                      \
		msg->body.header.datatype = type;  \
	} while ( 0 )

#define SCTK_MSG_TAG( msg ) msg->body.header.message_tag
#define SCTK_MSG_TAG_SET( msg, tag )        \
	do                                      \
	{                                       \
		msg->body.header.message_tag = tag; \
	} while ( 0 )

#define SCTK_MSG_NUMBER( msg ) msg->body.header.message_number
#define SCTK_MSG_NUMBER_SET( msg, number )        \
	do                                            \
	{                                             \
		msg->body.header.message_number = number; \
	} while ( 0 )

#define SCTK_MSG_MATCH( msg ) OPA_load_int( &msg->tail.matching_id )
#define SCTK_MSG_MATCH_SET( msg, match ) OPA_store_int( &msg->tail.matching_id, match )

#define SCTK_MSG_SPECIFIC_CLASS( msg ) msg->body.header.message_type.type
#define SCTK_MSG_SPECIFIC_CLASS_SET( msg, specific_tag )   \
	do                                                     \
	{                                                      \
		msg->body.header.message_type.type = specific_tag; \
	} while ( 0 )

#define SCTK_MSG_SPECIFIC_CLASS_SUBTYPE( msg ) msg->body.header.message_type.subtype
#define SCTK_MSG_SPECIFIC_CLASS_SET_SUBTYPE( msg, sub_type ) \
	do                                                       \
	{                                                        \
		msg->body.header.message_type.subtype = sub_type;    \
	} while ( 0 )

#define SCTK_MSG_SPECIFIC_CLASS_PARAM( msg ) msg->body.header.message_type.param
#define SCTK_MSG_SPECIFIC_CLASS_SET_PARAM( msg, param ) \
	do                                                  \
	{                                                   \
		msg->body.header.message_type.param = param;    \
	} while ( 0 )

#define SCTK_MSG_SPECIFIC_CLASS_RAILID( msg ) msg->body.header.message_type.rail_id
#define SCTK_MSG_SPECIFIC_CLASS_SET_RAILID( msg, raildid ) \
	do                                                     \
	{                                                      \
		msg->body.header.message_type.rail_id = raildid;   \
	} while ( 0 )

#define SCTK_MSG_HEADER( msg ) &msg->body.header

#define SCTK_MSG_SIZE( msg ) msg->body.header.msg_size
#define SCTK_MSG_SIZE_SET( msg, size )    \
	do                                    \
	{                                     \
		msg->body.header.msg_size = size; \
	} while ( 0 )

#define SCTK_MSG_REQUEST( msg ) msg->tail.request

#define SCTK_MSG_COMPLETION_FLAG( msg ) msg->tail.completion_flag
#define SCTK_MSG_COMPLETION_FLAG_SET( msg, completion ) \
	do                                                  \
	{                                                   \
		msg->tail.completion_flag = completion;         \
	} while ( 0 )

/** Buffered Messages **/
#define MAX_MPC_BUFFERED_SIZE ( 8 * sizeof( long ) )

typedef struct mpc_buffered_msg_s
{
	sctk_thread_ptp_message_t header;
	/* Completion flag to use if the user do not provide a valid request */
	int completion_flag;
	/* MPC_Request if the message is buffered  */
	sctk_request_t request;
	long buf[( MAX_MPC_BUFFERED_SIZE / sizeof( long ) ) + 1];
} mpc_buffered_msg_t;

/************************************************************************/
/* Message Progess                                                      */
/************************************************************************/

typedef struct mpc_mp_comm_ptp_msg_progress_s
{
	sctk_request_t *request;
	struct mpc_comm_ptp_s *recv_ptp;
	struct mpc_comm_ptp_s *send_ptp;
	int remote_process;
	int source_task_id;
	int dest_task_id;
	int polling_task_id;
	/* If we are blocked inside a function similar to MPI_Wait */
	int blocking;
} mpc_mp_comm_ptp_msg_progress_t;

void mpc_mp_comm_ptp_msg_wait_init( struct mpc_mp_comm_ptp_msg_progress_s *wait, sctk_request_t *request, int blocking );
void mpc_mp_comm_ptp_msg_progress( struct mpc_mp_comm_ptp_msg_progress_s *wait );

/************************************************************************/
/* General Functions                                                    */
/************************************************************************/

struct mpc_comm_ptp_s *_mpc_comm_ptp_array_get( sctk_communicator_t comm, int rank );
sctk_reorder_list_t *_mpc_comm_ptp_array_get_reorder( sctk_communicator_t communicator, int rank );

void mpc_mp_comm_request_wait_all_msgs( const int task, const sctk_communicator_t com );

int mpc_mp_comm_is_remote_rank( int dest );
void mpc_mp_comm_init_per_task( int i );

void mpc_mp_comm_perform_idle( volatile int *data, int value, void ( *func )( void * ), void *arg );

#define SCTK_PARALLEL_COMM_QUEUES_NUMBER 8

/************************************************************************/
/* Specific Message Tagging	                                          */
/************************************************************************/

/** Message for a process with ordering and a tag */
static inline int sctk_is_process_specific_message( sctk_thread_message_header_t *header )
{
	sctk_message_class_t class = header->message_type.type;
	return _mpc_comm_ptp_message_is_for_process( class );
}

/************************************************************************/
/* Thread-safe message probing	                                        */
/************************************************************************/

void sctk_m_probe_matching_init();
void sctk_m_probe_matching_set( int value );
void sctk_m_probe_matching_reset();
int sctk_m_probe_matching_get();

/************************************************************************/
/* sctk_request_t 		                                                    */
/************************************************************************/

void mpc_mp_comm_request_wait( sctk_request_t *request );
int mpc_mp_comm_request_cancel( sctk_request_t *msg );
void mpc_mp_comm_request_init( sctk_request_t *request, sctk_communicator_t comm, int request_type );

static inline int mpc_mp_comm_request_get_completion(sctk_request_t *request) {
  return request->completion_flag;
}

static inline void mpc_mp_comm_request_set_msg(sctk_request_t *request,
                                     sctk_thread_ptp_message_t *msg) {
  request->msg = msg;
}

static inline sctk_thread_ptp_message_t * mpc_mp_comm_request_get_msg(sctk_request_t *request) {
  return request->msg;
}

static inline void mpc_mp_comm_request_set_size(sctk_request_t *request) {
  request->SCTK_MSG_SIZE(msg) = 0;
}

static inline void mpc_mp_comm_request_inc_size(sctk_request_t *request,
                                                size_t size) {
  request->SCTK_MSG_SIZE(msg) += size;
}

static inline size_t mpc_mp_comm_request_get_size(sctk_request_t *request) {
  return request->SCTK_MSG_SIZE(msg);
}

static inline int mpc_mp_comm_request_get_source(sctk_request_t *request) {
  return request->header.source_task;
}

static inline int mpc_mp_comm_request_is_null(sctk_request_t *request) {
  return request->is_null;
}

static inline void mpc_mp_comm_request_set_null(sctk_request_t *request, int val) {
  request->is_null = val;
}

#ifdef __cplusplus
}
#endif

#endif

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
#include <sctk_collective_communications.h>
#include <sctk_reorder.h>
#include <sctk_ib.h>
#include <opa_primitives.h>
#include <sctk_types.h>
#include <sctk_portals.h>
#ifdef __cplusplus
extern "C"
{
#endif

/* Defines if we are in a full MPI mode */
//#define SCTK_ENABLE_SPINNING


/************************************************************************/
/* sctk_request_t		                                          */
/************************************************************************/
typedef enum
{
    REQUEST_NULL,
    REQUEST_SEND,
    REQUEST_RECV,
    REQUEST_SEND_COLL,
    REQUEST_RECV_COLL,
    REQUEST_GENERALIZED
} sctk_request_type_t;

void sctk_wait_message ( sctk_request_t *request );
int sctk_cancel_message ( sctk_request_t *msg );

/************************************************************************/
/* Messages Types                                               */
/************************************************************************/

/** This defines the type of a message */
typedef enum
{
	SCTK_CANCELLED_SEND,
	SCTK_CANCELLED_RECV,
	
	SCTK_P2P_MESSAGE,
	SCTK_RDMA_MESSAGE,
	SCTK_RDMA_WINDOW_MESSAGES, 		/**< These messages are used to exchange window informations */
	SCTK_CONTROL_MESSAGE_FENCE,     /**< This message is sent to create a fence on control messages */
	
	SCTK_BARRIER_MESSAGE,
	SCTK_BROADCAST_MESSAGE,
	SCTK_ALLREDUCE_MESSAGE,
	
	SCTK_ALLREDUCE_HETERO_MESSAGE,
	SCTK_BROADCAST_HETERO_MESSAGE,
	SCTK_BARRIER_HETERO_MESSAGE,
	
	SCTK_CONTROL_MESSAGE_INTERNAL, 		/**< This message is to be used inside a rail logic */
	SCTK_CONTROL_MESSAGE_RAIL, 		/**< This message goes to a rail */
	SCTK_CONTROL_MESSAGE_PROCESS,		/**< This message goes to a process (\ref sctk_control_message_process_level) */
	SCTK_CONTROL_MESSAGE_USER,		/**< This message goes to the application using an optionnal handler */
	SCTK_CONTROL_MESSAGE_COUNT		/**< Just in case this value allows to track the number of control message types */
}sctk_message_class_t;

static const char * const sctk_message_class_name[ SCTK_CONTROL_MESSAGE_COUNT ] = 
{   	"SCTK_CANCELLED_SEND",
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
	
	"SCTK_CONTROL_MESSAGE_INTERNAL",
	"SCTK_CONTROL_MESSAGE_RAIL", 		
	"SCTK_CONTROL_MESSAGE_PROCESS",		
	"SCTK_CONTROL_MESSAGE_USER"
 };

static inline int sctk_message_class_is_process_specific( sctk_message_class_t type )
{
	switch( type )
	{
		case SCTK_CANCELLED_SEND:
		case SCTK_CANCELLED_RECV:
		case SCTK_P2P_MESSAGE:
		case SCTK_RDMA_MESSAGE:
		case SCTK_BARRIER_MESSAGE:
		case SCTK_BROADCAST_MESSAGE:
		case SCTK_ALLREDUCE_MESSAGE:
		case SCTK_CONTROL_MESSAGE_FENCE:
		case SCTK_RDMA_WINDOW_MESSAGES: /* Note that the RDMA win message 
					   * is not process specific to force
					   * on-demand connections between the
					   * RDMA peers prior to emitting RDMA */
					    
			return 0;
		
		
		case SCTK_CONTROL_MESSAGE_INTERNAL:
		case SCTK_ALLREDUCE_HETERO_MESSAGE:
		case SCTK_BROADCAST_HETERO_MESSAGE:
		case SCTK_BARRIER_HETERO_MESSAGE:
		case SCTK_CONTROL_MESSAGE_RAIL:
		case SCTK_CONTROL_MESSAGE_PROCESS:
		case SCTK_CONTROL_MESSAGE_USER:
			return 1;
		
		case SCTK_CONTROL_MESSAGE_COUNT:
			return 0;
	}
	
	return 0;
}

static inline int sctk_message_class_is_control_message( sctk_message_class_t type )
{
	switch( type )
	{
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
		case SCTK_RDMA_WINDOW_MESSAGES:
		case SCTK_CONTROL_MESSAGE_FENCE:
		case SCTK_CONTROL_MESSAGE_INTERNAL:
			return 0;
		
		case SCTK_CONTROL_MESSAGE_RAIL:
		case SCTK_CONTROL_MESSAGE_PROCESS:
		case SCTK_CONTROL_MESSAGE_USER:
			return 1;
		
		case SCTK_CONTROL_MESSAGE_COUNT:
			return 0;
	}
	
	return 0;
}

/************************************************************************/
/* Low Level Messafe Interface                                          */
/************************************************************************/

void sctk_init_request (sctk_request_t * request, sctk_communicator_t comm, int request_type);
void sctk_message_isend_class_src( int src , int dest, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_message_class_t class, sctk_request_t *req );
void sctk_message_irecv_class_dest( int src, int dest, void * buffer, size_t size, int tag, sctk_communicator_t comm , sctk_message_class_t class, sctk_request_t *req );
void sctk_message_isend_class( int dest, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_message_class_t class, sctk_request_t *req );
void sctk_message_irecv_class( int src, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_message_class_t class, sctk_request_t *req );
void sctk_message_isend( int dest, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_request_t *req );
void sctk_message_irecv( int src, void * data, size_t size, int tag, sctk_communicator_t comm , sctk_request_t *req );
void sctk_sendrecv( void * sendbuf, size_t size, int dest, int tag, void * recvbuf, int src, int comm );

/************************************************************************/
/* Control Messages Header                                              */
/************************************************************************/

/** This is the content of a control message */
struct sctk_control_message_header
{
	char type;			/**< Type of the message determining the action */
	char subtype;			/**< Subtype of the message (can be freely set -- usually to do a switch) */
	char param;			/**< Parameter value (depending on type and subtypes)
					     for rails it is used to store rail number */
	char rail_id;			/**< The id of the rail sending the message (set during multirail selection \ref sctk_multirail_send_message)
					     it allows RAIL level messages to be routed accordingly */
};


/************************************************************************/
/* sctk_thread_message_header_t                                         */
/************************************************************************/

/** P2P message header */
typedef struct sctk_thread_message_header_s
{
	/* Process */
	int source; /**< Source Process */
	int destination; /**< Destination Process */
	/* Task */
	int source_task; /**< Source Task id */
	int destination_task; /**< Destination Task ID */
	/* Context */
	int message_tag; /**< Message TAG */
	sctk_communicator_t communicator; /**< Message communicator */
	struct sctk_control_message_header message_type; /**< Control Message Infos */
	/* Ordering */
	int message_number; /**< Message order (for reorder) */
	char use_message_numbering; /**< Should this message be reordered */
	/* Content */
	sctk_datatype_t datatype; /**< Caried data-type (for matching check) */
	size_t msg_size; /**< Message size */
} sctk_thread_message_header_t;

void sctk_probe_source_any_tag ( int destination, int source, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void sctk_probe_any_source_any_tag ( int destination, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void sctk_probe_source_tag ( int destination, int source, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void sctk_probe_any_source_tag ( int destination, const sctk_communicator_t comm, int *status, sctk_thread_message_header_t *msg );

/** sctk_thread_message_header_t GETTERS and Setters */

#define SCTK_MSG_USE_MESSAGE_NUMBERING( msg ) msg->body.header.use_message_numbering
#define SCTK_MSG_USE_MESSAGE_NUMBERING_SET( msg , num ) do{ msg->body.header.use_message_numbering = num; }while(0)

#define SCTK_MSG_SRC_PROCESS( msg ) msg->body.header.source
#define SCTK_MSG_SRC_PROCESS_SET( msg , src ) do{ msg->body.header.source = src; }while(0)

#define SCTK_MSG_DEST_PROCESS( msg ) msg->body.header.destination
#define SCTK_MSG_DEST_PROCESS_SET( msg , dest ) do{ msg->body.header.destination = dest; }while(0)

#define SCTK_MSG_SRC_TASK( msg ) msg->body.header.source_task
#define SCTK_MSG_SRC_TASK_SET( msg , src ) do{ msg->body.header.source_task = src; }while(0)

#define SCTK_MSG_DEST_TASK( msg ) msg->body.header.destination_task
#define SCTK_MSG_DEST_TASK_SET( msg , dest ) do{ msg->body.header.destination_task = dest; }while(0)

#define SCTK_MSG_COMMUNICATOR( msg ) msg->body.header.communicator
#define SCTK_MSG_COMMUNICATOR_SET( msg , comm ) do{ msg->body.header.communicator = comm; }while(0)

#define SCTK_MSG_TAG( msg ) msg->body.header.message_tag
#define SCTK_MSG_TAG_SET( msg , tag ) do{ msg->body.header.message_tag = tag; }while(0)

#define SCTK_MSG_NUMBER( msg ) msg->body.header.message_number
#define SCTK_MSG_NUMBER_SET( msg , number ) do{ msg->body.header.message_number = number; }while(0)

#define SCTK_MSG_SPECIFIC_CLASS( msg ) msg->body.header.message_type.type
#define SCTK_MSG_SPECIFIC_CLASS_SET( msg , specific_tag ) do{ msg->body.header.message_type.type = specific_tag; }while(0)

#define SCTK_MSG_SPECIFIC_CLASS_SUBTYPE( msg ) msg->body.header.message_type.subtype
#define SCTK_MSG_SPECIFIC_CLASS_SET_SUBTYPE( msg , sub_type ) do{ msg->body.header.message_type.subtype = sub_type; }while(0)

#define SCTK_MSG_SPECIFIC_CLASS_PARAM( msg ) msg->body.header.message_type.param
#define SCTK_MSG_SPECIFIC_CLASS_SET_PARAM( msg , param ) do{ msg->body.header.message_type.param = param; }while(0)

#define SCTK_MSG_SPECIFIC_CLASS_RAILID( msg ) msg->body.header.message_type.rail_id
#define SCTK_MSG_SPECIFIC_CLASS_SET_RAILID( msg , raildid ) do{ msg->body.header.message_type.rail_id = raildid; }while(0)

#define SCTK_MSG_HEADER( msg ) &msg->body.header

#define SCTK_MSG_SIZE( msg ) msg->body.header.msg_size
#define SCTK_MSG_SIZE_SET( msg , size ) do{ msg->body.header.msg_size = size; }while(0)

#define SCTK_MSG_COMPLETION_FLAG( msg ) msg->body.completion_flag
#define SCTK_MSG_COMPLETION_FLAG_SET( msg , completion ) do{ msg->body.completion_flag = completion; }while(0)

/************************************************/


/************************************************************************/
/* Message Content Descriptors                                          */
/************************************************************************/

typedef unsigned int sctk_pack_indexes_t;
typedef long sctk_pack_absolute_indexes_t;
typedef int sctk_count_t;

typedef enum
{
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
typedef union
{
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
typedef union
{
	sctk_message_pack_absolute_list_t absolute;
	sctk_message_pack_std_list_t std;
} sctk_message_pack_default_t;

/** Message Content descriptor */
typedef union
{
	sctk_message_contiguous_t contiguous; /** Contiguous case */
	sctk_message_pack_t pack; /** Packed case */
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

void sctk_message_copy ( sctk_message_to_copy_t *tmp );
void sctk_message_copy_pack ( sctk_message_to_copy_t *tmp );
void sctk_message_copy_pack_absolute ( sctk_message_to_copy_t *tmp );

/************************************************************************/
/* Message Content                                                      */
/************************************************************************/

typedef enum
{
    SCTK_MESSAGE_PENDING = 0,
    SCTK_MESSAGE_DONE = 1,
    SCTK_MESSAGE_CANCELED = 2
} sctk_completion_flag_t;


/*Data to tranfers in inter-process communications*/
typedef struct
{
	sctk_thread_message_header_t header;
	volatile int *volatile completion_flag;
	/* XXX:Specific to checksum */
	unsigned long checksum;
} sctk_thread_ptp_message_body_t;

int sctk_determine_src_process_from_header ( sctk_thread_ptp_message_body_t *body );
void sctk_determine_task_source_and_destination_from_header ( sctk_thread_ptp_message_body_t *body, int *source_task, int *destination_task );

struct mpc_buffered_msg_s;
struct sctk_internal_ptp_s;

/*Data not to tranfers in inter-process communications*/
typedef struct
{
	char remote_source;
	char remote_destination;

	int need_check_in_wait;

	sctk_request_t *request;

	/*Message data*/
	sctk_message_type_t message_type;
	sctk_message_t message;
	sctk_message_pack_default_t default_pack;

	/*Storage structs*/
	sctk_msg_list_t distant_list;
	sctk_message_to_copy_t copy_list;

	struct sctk_internal_ptp_s *internal_ptp;

	/*Destructor*/
	void ( *free_memory ) ( void * );

	/*Copy operator*/
	void ( *message_copy ) ( sctk_message_to_copy_t * );

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
	struct sctk_portals_msg_header_s portals;
#endif

	/* XXX:Specific to IB */
	struct sctk_ib_msg_header_s ib;
} sctk_thread_ptp_message_tail_t;


/************************************************************************/
/* sctk_thread_ptp_message_t Point to Point message descriptor          */
/************************************************************************/

typedef struct sctk_thread_ptp_message_s
{
	sctk_thread_ptp_message_body_t body;
	sctk_thread_ptp_message_tail_t  tail;

	/* Pointers for chaining elements */
	struct sctk_thread_ptp_message_s *prev;
	struct sctk_thread_ptp_message_s *next;
	/* If the entry comes from a buffered list */
	char from_buffered;
} sctk_thread_ptp_message_t;


void sctk_init_header ( sctk_thread_ptp_message_t *tmp, sctk_message_type_t msg_type, void ( *free_memory ) ( void * ),
                        void ( *message_copy ) ( sctk_message_to_copy_t * ) );
void sctk_reinit_header ( sctk_thread_ptp_message_t *tmp, void ( *free_memory ) ( void * ),
                          void ( *message_copy ) ( sctk_message_to_copy_t * ) );
sctk_thread_ptp_message_t *sctk_create_header ( const int myself, sctk_message_type_t msg_type );
void sctk_add_adress_in_message ( sctk_thread_ptp_message_t *restrict msg, void *restrict addr, const size_t size );
void sctk_add_pack_in_message ( sctk_thread_ptp_message_t *msg, void *adr, const sctk_count_t nb_items,
                                const size_t elem_size,
                                sctk_pack_indexes_t *begins,
                                sctk_pack_indexes_t *ends );
void sctk_add_pack_in_message_absolute ( sctk_thread_ptp_message_t *msg, void *adr,
                                         const sctk_count_t nb_items,
                                         const size_t elem_size,
                                         sctk_pack_absolute_indexes_t *begins,
                                         sctk_pack_absolute_indexes_t *ends );
void sctk_set_header_in_message ( sctk_thread_ptp_message_t *msg, const int message_tag,
                                  const sctk_communicator_t communicator,
                                  const int source,
                                  const int destination,
                                  sctk_request_t *request,
                                  const size_t count,
                                  sctk_message_class_t message_class,
                                  sctk_datatype_t datatype );
void sctk_send_message ( sctk_thread_ptp_message_t *msg );
void sctk_send_message_try_check ( sctk_thread_ptp_message_t *msg, int perform_check );
void sctk_recv_message ( sctk_thread_ptp_message_t *msg, struct sctk_internal_ptp_s *tmp, int need_check );
void sctk_recv_message_try_check ( sctk_thread_ptp_message_t *msg, struct sctk_internal_ptp_s *tmp, int perform_check );
void sctk_message_completion_and_free ( sctk_thread_ptp_message_t *send, sctk_thread_ptp_message_t *recv );
void sctk_complete_and_free_message ( sctk_thread_ptp_message_t *msg );
void sctk_rebuild_header ( sctk_thread_ptp_message_t *msg );

/** Buffered Messages **/
#define MAX_MPC_BUFFERED_SIZE (128 * sizeof(long))

typedef struct mpc_buffered_msg_s
{
	sctk_thread_ptp_message_t header;
	/* Completion flag to use if the user do not provide a valid request */
	int completion_flag;
	/* MPC_Request if the message is buffered  */
	sctk_request_t request;
	long buf[(MAX_MPC_BUFFERED_SIZE / sizeof (long)) + 1];
} mpc_buffered_msg_t;


/************************************************************************/
/* Message Progess                                                      */
/************************************************************************/

typedef struct sctk_perform_messages_s
{
	sctk_request_t *request;
	struct sctk_internal_ptp_s *recv_ptp;
	struct sctk_internal_ptp_s *send_ptp;
	int remote_process;
	int source_task_id;
	int polling_task_id;
	/* If we are blocked inside a function similar to MPI_Wait */
	int blocking;
} sctk_perform_messages_t;

void sctk_perform_messages_wait_init ( struct sctk_perform_messages_s *wait, sctk_request_t *request, int blocking );
/**
Check if the message if completed according to the message passed as a request
*/
void sctk_perform_messages ( struct sctk_perform_messages_s *wait );
void sctk_perform_messages_wait_init_request_type ( struct sctk_perform_messages_s *wait );

/************************************************************************/
/* General Functions                                                    */
/************************************************************************/

void sctk_wait_all ( const int task, const sctk_communicator_t com );
struct sctk_internal_ptp_s *sctk_get_internal_ptp(int glob_id,
                                                  sctk_communicator_t com);
int sctk_is_net_message ( int dest );
void sctk_ptp_per_task_init ( int i );
void sctk_unregister_thread ( const int i );
void sctk_notify_idle_message ();
void sctk_notify_idle_message_inter ();
sctk_reorder_list_t *
sctk_ptp_get_reorder_from_destination(int task,
                                      sctk_communicator_t communicator);
void sctk_inter_thread_perform_idle ( volatile int *data, int value, void ( *func ) ( void * ), void *arg );

#define SCTK_PARALLEL_COMM_QUEUES_NUMBER 8

/************************************************************************/
/* Specific Message Tagging	                                          */
/************************************************************************/

/** Message for a process with ordering and a tag */
static inline int sctk_is_process_specific_message( sctk_thread_message_header_t * header )
{
	sctk_message_class_t class = header->message_type.type;
	return sctk_message_class_is_process_specific( class );
}


#ifdef __cplusplus
}
#endif

#endif

#ifndef MPC_MESSAGE_PASSING_INCLUDE_COMM_H_
#define MPC_MESSAGE_PASSING_INCLUDE_COMM_H_

#include <sctk_types.h>
#include <sctk_reorder.h>
#include <sctk_ib.h>
#include <sctk_portals.h>

/************************************************************************/
/* mpc_lowcomm_request_t		                                                    */
/************************************************************************/
typedef enum
{
	REQUEST_NULL = 0,
	REQUEST_SEND,
	REQUEST_RECV,
	REQUEST_GENERALIZED,
	REQUEST_PICKED,
	REQUEST_RDMA
} mpc_lowcomm_request_type_t;


/************************************************************************/
/* Messages Types                                               */
/************************************************************************/

/** This defines the type of a message */
typedef enum
{
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
} mpc_lowcomm_ptp_message_class_t;

static const char *const sctk_message_class_name[SCTK_MESSAGE_CLASS_COUNT] =
{
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
	"SCTK_CONTROL_MESSAGE_USER"
};

/************************************************************************/
/* Low Level Message Interface                                          */
/************************************************************************/

void mpc_lowcomm_comm_isend_class_src( int src, int dest, void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req );
void mpc_lowcomm_comm_irecv_class_dest( int src, int dest, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req );
void mpc_lowcomm_comm_isend_class( int dest, void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req );
void mpc_lowcomm_comm_irecv_class( int src, void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req );
void mpc_lowcomm_comm_isend( int dest, void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req );
void mpc_lowcomm_comm_irecv( int src, void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req );
void mpc_lowcomm_comm_sendrecv( void *sendbuf, size_t size, int dest, int tag, void *recvbuf, int src, int comm );


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
	mpc_lowcomm_communicator_t communicator;				 /**< Message communicator */
	struct sctk_control_message_header message_type; /**< Control Message Infos */
	/* Ordering */
	int message_number;			/**< Message order (for reorder) */
	char use_message_numbering; /**< Should this message be reordered */
	/* Content */
	mpc_lowcomm_datatype_t datatype; /**< Caried data-type (for matching check) */
	size_t msg_size;		  /**< Message size */
} sctk_thread_message_header_t;

void mpc_lowcomm_comm_message_probe_any_tag( int destination, int source, const mpc_lowcomm_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_lowcomm_comm_message_probe_any_source_any_tag( int destination, const mpc_lowcomm_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_lowcomm_comm_message_probe( int destination, int source, const mpc_lowcomm_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_lowcomm_comm_message_probe_any_source( int destination, const mpc_lowcomm_communicator_t comm, int *status, sctk_thread_message_header_t *msg );
void mpc_lowcomm_comm_message_probe_any_source_class( int destination, int tag,
        mpc_lowcomm_ptp_message_class_t class,
        const mpc_lowcomm_communicator_t comm,
        int *status,
        sctk_thread_message_header_t *msg );

void mpc_lowcomm_comm_message_probe_any_source_class_comm( int destination, int tag,
        mpc_lowcomm_ptp_message_class_t class,
        const mpc_lowcomm_communicator_t comm,
        int *status,
        sctk_thread_message_header_t *msg );

/************************************************************************/
/* Message Copy                                                         */
/************************************************************************/

typedef struct mpc_lowcomm_ptp_message_content_to_copy_s
{
	struct sctk_thread_ptp_message_s *msg_send;
	struct sctk_thread_ptp_message_s *msg_recv;
	struct mpc_lowcomm_ptp_message_content_to_copy_s *prev, *next;
} mpc_lowcomm_ptp_message_content_to_copy_t;

void mpc_lowcomm_comm_ptp_message_copy( mpc_lowcomm_ptp_message_content_to_copy_t *tmp );
void mpc_lowcomm_comm_ptp_message_copy_pack( mpc_lowcomm_ptp_message_content_to_copy_t *tmp );
void mpc_lowcomm_comm_ptp_message_copy_pack_absolute( mpc_lowcomm_ptp_message_content_to_copy_t *tmp );

/************************************************************************/
/* Message Content Descriptors                                          */
/************************************************************************/

typedef enum
{
	SCTK_MESSAGE_CONTIGUOUS,
	SCTK_MESSAGE_PACK,
	SCTK_MESSAGE_PACK_ABSOLUTE,
	SCTK_MESSAGE_PACK_UNDEFINED,
	SCTK_MESSAGE_NETWORK
} mpc_lowcomm_ptp_message_type_t;

/** SCTK_MESSAGE_CONTIGUOUS */
typedef struct
{
	size_t size;
	void *addr;
} sctk_message_contiguous_t;

/** SCTK_MESSAGE_PACK */
typedef struct
{
	unsigned int count;
	unsigned long *begins;
	unsigned long *ends;
	void *addr;
	size_t elem_size;
} sctk_message_pack_std_list_t;

/** SCTK_MESSAGE_PACK_ABSOLUTE */
typedef struct
{
	unsigned int count;
	long *begins;
	long *ends;
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
	sctk_message_pack_t pack;			  /** Packed case */
} mpc_lowcomm_ptp_message_content_t;

/************************************************************************/
/* Message List                                                         */
/************************************************************************/

struct sctk_thread_ptp_message_s;

typedef volatile struct sctk_msg_list_s
{
	struct sctk_thread_ptp_message_s *msg;
	volatile struct sctk_msg_list_s *prev, *next;
} mpc_lowcomm_msg_list_t;

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
#ifdef SCTK_USE_CHECKSUM
	unsigned long checksum;
#endif
} mpc_lowcomm_ptp_message_body_t;

struct mpc_buffered_msg_s;
struct mpc_comm_ptp_s;

/*Data not to tranfers in inter-process communications*/
typedef struct
{
	char remote_source;

	volatile int *volatile completion_flag;

	/* Matchind ID used for MPROBE */
	OPA_int_t matching_id; /**< 0 By default unique id otherwise */

	mpc_lowcomm_request_t *request;

	/*Message data*/
	mpc_lowcomm_ptp_message_type_t message_type;
	mpc_lowcomm_ptp_message_content_t message;

	/*Storage structs*/
	mpc_lowcomm_msg_list_t distant_list;
	mpc_lowcomm_ptp_message_content_to_copy_t copy_list;

	struct mpc_comm_ptp_s *internal_ptp;

	/*Destructor*/
	void ( *free_memory )( void * );

	/*Copy operator*/
	void ( *message_copy )( mpc_lowcomm_ptp_message_content_to_copy_t * );

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

#ifdef MPC_USE_INFINIBAND
	struct sctk_ib_msg_header_s ib;
#endif
} mpc_lowcomm_ptp_message_tail_t;

/************************************************************************/
/* mpc_lowcomm_ptp_message_t Point to Point message descriptor          */
/************************************************************************/

typedef struct sctk_thread_ptp_message_s
{
	mpc_lowcomm_ptp_message_body_t body;
	mpc_lowcomm_ptp_message_tail_t tail;

	/* Pointers for chaining elements */
	struct sctk_thread_ptp_message_s *prev;
	struct sctk_thread_ptp_message_s *next;

	/* If the entry comes from a buffered list */
	char from_buffered;
} mpc_lowcomm_ptp_message_t;

void mpc_lowcomm_comm_ptp_message_header_clear( mpc_lowcomm_ptp_message_t *tmp, mpc_lowcomm_ptp_message_type_t msg_type, void ( *free_memory )( void * ),
        void ( *message_copy )( mpc_lowcomm_ptp_message_content_to_copy_t * ) );

mpc_lowcomm_ptp_message_t *mpc_lowcomm_comm_ptp_message_header_create( mpc_lowcomm_ptp_message_type_t msg_type );

void mpc_lowcomm_comm_ptp_message_set_contiguous_addr( mpc_lowcomm_ptp_message_t *restrict msg, void *restrict addr, const size_t size );

void mpc_lowcomm_comm_ptp_message_add_pack( mpc_lowcomm_ptp_message_t *msg, void *adr, const unsigned int nb_items,
                                       const size_t elem_size,
                                       unsigned long *begins,
                                       unsigned long *ends );

void mpc_lowcomm_comm_ptp_message_add_pack_absolute( mpc_lowcomm_ptp_message_t *msg, void *adr,
        const unsigned int nb_items,
        const size_t elem_size,
        long *begins,
        long *ends );

void mpc_lowcomm_comm_ptp_message_header_init( mpc_lowcomm_ptp_message_t *msg, const int message_tag,
        const mpc_lowcomm_communicator_t communicator,
        const int source,
        const int destination,
        mpc_lowcomm_request_t *request,
        const size_t count,
        mpc_lowcomm_ptp_message_class_t message_class,
        mpc_lowcomm_datatype_t datatype,
        mpc_lowcomm_request_type_t request_type );

void mpc_lowcomm_comm_ptp_message_send( mpc_lowcomm_ptp_message_t *msg );

void mpc_lowcomm_comm_ptp_message_recv( mpc_lowcomm_ptp_message_t *msg );

void mpc_lowcomm_comm_ptp_message_complete_and_free( mpc_lowcomm_ptp_message_t *msg );

/*********************
 * BUFFERED MESSAGES *
 *********************/

#define MAX_MPC_BUFFERED_SIZE ( 8 * sizeof( long ) )

typedef struct mpc_buffered_msg_s
{
	mpc_lowcomm_ptp_message_t header;
	/* Completion flag to use if the user do not provide a valid request */
	int completion_flag;
	/* mpc_lowcomm_request_t if the message is buffered  */
	mpc_lowcomm_request_t request;
	long buf[( MAX_MPC_BUFFERED_SIZE / sizeof( long ) ) + 1];
} mpc_buffered_msg_t;


/************************************************************************/
/* Message Progess                                                      */
/************************************************************************/

typedef struct mpc_lowcomm_comm_ptp_msg_progress_s
{
	mpc_lowcomm_request_t *request;
	struct mpc_comm_ptp_s *recv_ptp;
	struct mpc_comm_ptp_s *send_ptp;
	int remote_process;
	int source_task_id;
	int dest_task_id;
	int polling_task_id;
	/* If we are blocked inside a function similar to MPI_Wait */
	int blocking;
} mpc_lowcomm_comm_ptp_msg_progress_t;

void mpc_lowcomm_comm_ptp_msg_wait_init( struct mpc_lowcomm_comm_ptp_msg_progress_s *wait, mpc_lowcomm_request_t *request, int blocking );
void mpc_lowcomm_comm_ptp_msg_progress( struct mpc_lowcomm_comm_ptp_msg_progress_s *wait );


/************************************************************************/
/* General Functions                                                    */
/************************************************************************/

void mpc_lowcomm_comm_request_wait_all_msgs( const int task, const mpc_lowcomm_communicator_t com );

int mpc_lowcomm_comm_is_remote_rank( int dest );
void mpc_lowcomm_comm_init_per_task( int i );

void mpc_lowcomm_comm_perform_idle( volatile int *data, int value, void ( *func )( void * ), void *arg );

/************************************************************************/
/* mpc_lowcomm_request_t 		                                                    */
/************************************************************************/

void mpc_lowcomm_comm_request_wait( mpc_lowcomm_request_t *request );
int mpc_lowcomm_comm_request_cancel( mpc_lowcomm_request_t *msg );
void mpc_lowcomm_comm_request_init( mpc_lowcomm_request_t *request, mpc_lowcomm_communicator_t comm, int request_type );

static inline int mpc_lowcomm_comm_request_get_completion( mpc_lowcomm_request_t *request )
{
	return request->completion_flag;
}

static inline void mpc_lowcomm_comm_request_set_msg( mpc_lowcomm_request_t *request,
        mpc_lowcomm_ptp_message_t *msg )
{
	request->msg = msg;
}

static inline mpc_lowcomm_ptp_message_t *mpc_lowcomm_comm_request_get_msg( mpc_lowcomm_request_t *request )
{
	return request->msg;
}

static inline void mpc_lowcomm_comm_request_set_size( mpc_lowcomm_request_t *request )
{
	request->msg->body.header.msg_size = 0;
}

static inline void mpc_lowcomm_comm_request_inc_size( mpc_lowcomm_request_t *request,
        size_t size )
{
	request->msg->body.header.msg_size += size;
}

static inline size_t mpc_lowcomm_comm_request_get_size( mpc_lowcomm_request_t *request )
{
	return request->msg->body.header.msg_size;
}

static inline int mpc_lowcomm_comm_request_get_source( mpc_lowcomm_request_t *request )
{
	return request->header.source_task;
}

static inline int mpc_lowcomm_comm_request_is_null( mpc_lowcomm_request_t *request )
{
	return request->is_null;
}

static inline void mpc_lowcomm_comm_request_set_null( mpc_lowcomm_request_t *request, int val )
{
	request->is_null = val;
}

int mpc_lowcomm_comm_request_cancel( mpc_lowcomm_request_t *request );

int mpc_lowcomm_comm_request_free( mpc_lowcomm_request_t *request );

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/

static inline int mpc_lowcomm_comm_status_set_cancelled( mpc_lowcomm_status_t *status, int cancelled )
{
	status->cancelled = cancelled;
	return SCTK_SUCCESS;
}

static inline int mpc_lowcomm_comm_status_get_cancelled( mpc_lowcomm_status_t *status, int *flag )
{
	*flag = ( status->cancelled == 1 );
	return SCTK_SUCCESS;
}

#endif /* MPC_MESSAGE_PASSING_INCLUDE_COMM_H_ */
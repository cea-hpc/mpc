#ifndef MPC_MESSAGE_PASSING_INCLUDE_COMM_H_
#define MPC_MESSAGE_PASSING_INCLUDE_COMM_H_

#include <mpc_config.h>
#include <mpc_lowcomm_types.h>
#include <reorder.h>

#include <mpc_lowcomm_monitor.h>

#ifdef MPC_USE_INFINIBAND
#include "ib/ib.h"
#endif

#ifdef MPC_USE_PORTALS
#include "portals/sctk_portals.h"
#endif

#ifdef MPC_USE_OFI
#include "ofi/ofi_types.h"
#endif

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
	REQUEST_RDMA,
	REQUEST_TYPE_COUNT
} mpc_lowcomm_request_type_t;

static const char *const mpc_lowcomm_request_type_name[REQUEST_TYPE_COUNT] =
{
	"REQUEST_NULL",
	"REQUEST_SEND",
	"REQUEST_RECV",
	"REQUEST_GENERALIZED",
	"REQUEST_PICKED",
	"REQUEST_RDMA"
};

/************************************************************************/
/* Messages Types                                               */
/************************************************************************/

/** This defines the type of a message */
typedef enum
{
	MPC_LOWCOMM_CLASS_NONE,
	MPC_LOWCOMM_CANCELLED_SEND,
	MPC_LOWCOMM_CANCELLED_RECV,

	MPC_LOWCOMM_P2P_MESSAGE,
	MPC_LOWCOMM_RDMA_MESSAGE,
	MPC_LOWCOMM_RDMA_WINDOW_MESSAGES,  /**< These messages are used to exchange window  informations */
	MPC_LOWCOMM_CONTROL_MESSAGE_FENCE, /**< This message is sent to create a fence on control messages */

	MPC_LOWCOMM_BARRIER_MESSAGE,
	MPC_LOWCOMM_BROADCAST_MESSAGE,
	MPC_LOWCOMM_ALLREDUCE_MESSAGE,

	MPC_LOWCOMM_ALLREDUCE_HETERO_MESSAGE,
	MPC_LOWCOMM_BROADCAST_HETERO_MESSAGE,
	MPC_LOWCOMM_BARRIER_HETERO_MESSAGE,

	MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE,
	MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE,
	MPC_LOWCOMM_REDUCE_OFFLOAD_MESSAGE,
	MPC_LOWCOMM_ALLREDUCE_OFFLOAD_MESSAGE,

	MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, /**< This message is to be used inside a rail logic */


	MPC_LOWCOMM_MESSAGE_UNIVERSE,         /**< This message is for a given UID */

	MPC_LOWCOMM_MESSAGE_CLASS_COUNT       /**< This value allows to track the  number of control message types */
} mpc_lowcomm_ptp_message_class_t;

static const char *const mpc_lowcomm_ptp_message_class_name[MPC_LOWCOMM_MESSAGE_CLASS_COUNT] =
{
	"MPC_LOWCOMM_CLASS_NONE",
	"MPC_LOWCOMM_CANCELLED_SEND",
	"MPC_LOWCOMM_CANCELLED_RECV",

	"MPC_LOWCOMM_P2P_MESSAGE",
	"MPC_LOWCOMM_RDMA_MESSAGE",
	"MPC_LOWCOMM_RDMA_WINDOW_MESSAGES",
	"MPC_LOWCOMM_CONTROL_MESSAGE_FENCE",

	"MPC_LOWCOMM_BARRIER_MESSAGE",
	"MPC_LOWCOMM_BROADCAST_MESSAGE",
	"MPC_LOWCOMM_ALLREDUCE_MESSAGE",

	"MPC_LOWCOMM_ALLREDUCE_HETERO_MESSAGE",
	"MPC_LOWCOMM_BROADCAST_HETERO_MESSAGE",
	"MPC_LOWCOMM_BARRIER_HETERO_MESSAGE",

	"MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE",
	"MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE",
	"MPC_LOWCOMM_REDUCE_OFFLOAD_MESSAGE",
	"MPC_LOWCOMM_ALLREDUCE_OFFLOAD_MESSAGE",

	"MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL",


	"MPC_LOWCOMM_MESSAGE_UNIVERSE"
};

/************************************************************************/
/* Low Level Message Interface                                          */
/************************************************************************/

int mpc_lowcomm_isend_class_src(int src, int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req);
int mpc_lowcomm_irecv_class_dest(int src, int dest, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req);
int mpc_lowcomm_isend_class(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req);
int mpc_lowcomm_irecv_class(int src, void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req);

/************************************************************************/
/* mpc_lowcomm_ptp_message_header_t                                         */
/************************************************************************/

/** P2P message header */
typedef struct mpc_lowcomm_ptp_message_header_s
{
	/* Process */
	mpc_lowcomm_peer_uid_t                       source;                /**< Source Process */
	mpc_lowcomm_peer_uid_t                       destination;           /**< Destination Process */
	/* Task */
	int                                          source_task;           /**< Source Task id */
	int                                          destination_task;      /**< Destination Task ID */
	/* Context */
	char									     message_type;          /**< Message type */

	int                                          message_tag;           /**< Message TAG */
	mpc_lowcomm_communicator_id_t                communicator_id;       /**< Message communicator */
	/* Content */
	mpc_lowcomm_datatype_t                       datatype;              /**< Caried data-type (for matching check) */
	size_t                                       msg_size;              /**< Message size */
	/* Ordering */
	int                                          message_number;        /**< Message order (for reorder) */
	char                                         use_message_numbering; /**< Should this message be reordered */

} mpc_lowcomm_ptp_message_header_t;

void mpc_lowcomm_message_probe_any_tag(int destination, int source, const mpc_lowcomm_communicator_t comm, int *status, mpc_lowcomm_ptp_message_header_t *msg);
void mpc_lowcomm_message_probe_any_source_any_tag(int destination, const mpc_lowcomm_communicator_t comm, int *status, mpc_lowcomm_ptp_message_header_t *msg);
void mpc_lowcomm_message_probe(int destination, int source, const mpc_lowcomm_communicator_t comm, int *status, mpc_lowcomm_ptp_message_header_t *msg);
void mpc_lowcomm_message_probe_any_source(int destination, const mpc_lowcomm_communicator_t comm, int *status, mpc_lowcomm_ptp_message_header_t *msg);
void mpc_lowcomm_message_probe_any_source_class(int destination, int tag,
                                                mpc_lowcomm_ptp_message_class_t class,
                                                const mpc_lowcomm_communicator_t comm,
                                                int *status,
                                                mpc_lowcomm_ptp_message_header_t *msg);

void mpc_lowcomm_message_probe_any_source_class_comm(int destination, int tag,
                                                     mpc_lowcomm_ptp_message_class_t class,
                                                     const mpc_lowcomm_communicator_t comm,
                                                     int *status,
                                                     mpc_lowcomm_ptp_message_header_t *msg);

/************************************************************************/
/* Message Copy                                                         */
/************************************************************************/

typedef struct mpc_lowcomm_ptp_message_content_to_copy_s
{
	struct mpc_lowcomm_ptp_message_s *                msg_send;
	struct mpc_lowcomm_ptp_message_s *                msg_recv;
	struct mpc_lowcomm_ptp_message_content_to_copy_s *prev, *next;
} mpc_lowcomm_ptp_message_content_to_copy_t;

void mpc_lowcomm_ptp_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t *tmp);
void mpc_lowcomm_ptp_message_copy_pack(mpc_lowcomm_ptp_message_content_to_copy_t *tmp);
void mpc_lowcomm_ptp_message_copy_pack_absolute(mpc_lowcomm_ptp_message_content_to_copy_t *tmp);

/************************************************************************/
/* Message Content Descriptors                                          */
/************************************************************************/

typedef enum
{
	MPC_LOWCOMM_MESSAGE_CONTIGUOUS,
	MPC_LOWCOMM_MESSAGE_PACK,
	MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE,
	MPC_LOWCOMM_MESSAGE_PACK_UNDEFINED,
	MPC_LOWCOMM_MESSAGE_NETWORK
} mpc_lowcomm_ptp_message_type_t;

/** MPC_LOWCOMM_MESSAGE_CONTIGUOUS */
typedef struct
{
	size_t size;
	void * addr;
} mpc_lowcomm_ptp_message_contiguous_t;

/** MPC_LOWCOMM_MESSAGE_PACK */
typedef struct
{
	unsigned int   count;
	unsigned long *begins;
	unsigned long *ends;
	void *         addr;
	size_t         elem_size;
} mpc_lowcomm_ptp_message_pack_std_list_t;

/** MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE */
typedef struct
{
	unsigned int count;
	long *       begins;
	long *       ends;
	const void * addr;
	size_t       elem_size;
} mpc_lowcomm_ptp_message_pack_absolute_list_t;

/** Content for list of packs */
typedef union
{
	mpc_lowcomm_ptp_message_pack_absolute_list_t *absolute;
	mpc_lowcomm_ptp_message_pack_std_list_t *     std;
} mpc_lowcomm_ptp_message_pack_list_t;

typedef struct
{
	size_t                              count;
	size_t                              max_count;
	mpc_lowcomm_ptp_message_pack_list_t list;
} mpc_lowcomm_ptp_message_pack_t;

/** Content single packed message */
typedef union
{
	mpc_lowcomm_ptp_message_pack_absolute_list_t absolute;
	mpc_lowcomm_ptp_message_pack_std_list_t      std;
} mpc_lowcomm_ptp_message_pack_default_t;

/** Message Content descriptor */
typedef union
{
	mpc_lowcomm_ptp_message_contiguous_t contiguous; /** Contiguous case */
	mpc_lowcomm_ptp_message_pack_t       pack;       /** Packed case */
} mpc_lowcomm_ptp_message_content_t;

/************************************************************************/
/* Message List                                                         */
/************************************************************************/

struct mpc_lowcomm_ptp_message_s;

typedef volatile struct mpc_lowcomm_msg_list_s
{
	struct mpc_lowcomm_ptp_message_s *      msg;
	volatile struct mpc_lowcomm_msg_list_s *prev, *next;
} mpc_lowcomm_msg_list_t;

typedef struct
{
	mpc_common_spinlock_t            lock;
	mpc_lowcomm_msg_list_t *list;
} mpc_lowcomm_ptp_list_incomming_t;

typedef struct
{
	volatile mpc_lowcomm_msg_list_t *list;
} mpc_lowcomm_ptp_list_pending_t;

typedef struct
{
	mpc_common_spinlock_t         pending_lock;
	char                          pad[256];
	/* Messages are posted to the 'incoming' lists before being merged to the pending list. */
	mpc_lowcomm_ptp_list_incomming_t incomming_send;
	mpc_lowcomm_ptp_list_incomming_t incomming_recv;
	/* Messages in the 'pending' lists are waiting to be matched */
    mpc_lowcomm_ptp_list_pending_t   pending_send;
	mpc_lowcomm_ptp_list_pending_t   pending_recv;
} mpc_lowcomm_ptp_message_lists_t;

/**
 * @brief Get Message Lists for a given MPI process
 * 
 * @param rank rank to query for (in CW)
 * @param lists output array to hold list (number of local processes)
 * @param list_count IN max number of processes OUT actual number of processes
 * @return int MPC_LOWCOMM_SUCCESS if all OK MPC_LOWCOMM_ERR_TRUNCATE if array was too small
 */
int _mpc_lowcomm_comm_get_lists(int rank, mpc_lowcomm_ptp_message_lists_t **lists, int *list_count);

/************************************************************************/
/* Message Content                                                      */
/************************************************************************/

typedef enum
{
	MPC_LOWCOMM_MESSAGE_PENDING  = 0,
	MPC_LOWCOMM_MESSAGE_DONE     = 1,
	MPC_LOWCOMM_MESSAGE_CANCELED = 2
} mpc_lowcomm_completion_flag_t;

/*Data to tranfers in inter-process communications*/
typedef struct
{
	mpc_lowcomm_ptp_message_header_t header;
#ifdef SCTK_USE_CHECKSUM
	unsigned long                    checksum;
#endif
} mpc_lowcomm_ptp_message_body_t;

struct mpc_buffered_msg_s;
struct mpc_comm_ptp_s;

/*Data not to tranfers in inter-process communications*/
typedef struct
{
	volatile int *volatile                    completion_flag;

	/* Matchind ID used for MPROBE */
	OPA_int_t                                 matching_id;  /**< 0 By default unique id otherwise */
	mpc_lowcomm_ptp_message_type_t            message_type;

	mpc_lowcomm_communicator_t                communicator;

	/* Pointer to the local ptp list if local */
	struct mpc_comm_ptp_s *                   internal_ptp;

	mpc_lowcomm_request_t *                   request;

	/*Message data*/

	mpc_lowcomm_ptp_message_content_t         message;

	/*Storage structs*/
	mpc_lowcomm_msg_list_t                    distant_list;
	mpc_lowcomm_ptp_message_content_to_copy_t copy_list;

	/*Destructor*/
	void (*free_memory)(void *);
	/*Copy operator*/
	void (*message_copy)(mpc_lowcomm_ptp_message_content_to_copy_t *);

	/*Reorder buffer struct*/
	mpc_lowcomm_reorder_buffer_t              reorder;

	/* If the message has been buffered during the
	 * Send function. If it is, we need to free the async
	 * buffer when completing the message */
	struct mpc_buffered_msg_s *               buffer_async;

	/* RDMA infos */
	void *                                    rdma_src;
	void *                                    route_table;

#ifdef MPC_USE_PORTALS
	/* Portals infos */
	struct mpc_lowcomm_ptl_tail_s             ptl;
#endif

#ifdef MPC_USE_INFINIBAND
	struct mpc_lowcomm_ib_tail_s              ib;
#endif
#ifdef MPC_USE_OFI /* all of these should be replaced w/ union */
	struct mpc_lowcomm_ofi_tail_s ofi;
#endif
	char                                      remote_source;
} mpc_lowcomm_ptp_message_tail_t;

/************************************************************************/
/* mpc_lowcomm_ptp_message_t Point to Point message descriptor          */
/************************************************************************/

typedef struct mpc_lowcomm_ptp_message_s
{
	mpc_lowcomm_ptp_message_body_t    body;
	mpc_lowcomm_ptp_message_tail_t    tail;

	/* Pointers for chaining elements */
	struct mpc_lowcomm_ptp_message_s *prev;
	struct mpc_lowcomm_ptp_message_s *next;

	/* If the entry comes from a buffered list */
	char                              from_buffered;
} mpc_lowcomm_ptp_message_t;

void mpc_lowcomm_ptp_message_header_clear(mpc_lowcomm_ptp_message_t *tmp, mpc_lowcomm_ptp_message_type_t msg_type, void (*free_memory)(void *),
                                          void (*message_copy)(mpc_lowcomm_ptp_message_content_to_copy_t *) );

mpc_lowcomm_ptp_message_t *mpc_lowcomm_ptp_message_header_create(mpc_lowcomm_ptp_message_type_t msg_type);

void mpc_lowcomm_ptp_message_set_contiguous_addr(mpc_lowcomm_ptp_message_t *restrict msg, const void *restrict addr, const size_t size);

void mpc_lowcomm_ptp_message_add_pack(mpc_lowcomm_ptp_message_t *msg, void *adr, const unsigned int nb_items,
                                      const size_t elem_size,
                                      unsigned long *begins,
                                      unsigned long *ends);

void mpc_lowcomm_ptp_message_add_pack_absolute(mpc_lowcomm_ptp_message_t *msg, const void *adr,
                                               const unsigned int nb_items,
                                               const size_t elem_size,
                                               long *begins,
                                               long *ends);

void mpc_lowcomm_ptp_message_header_init(mpc_lowcomm_ptp_message_t *msg, const int message_tag,
                                         const mpc_lowcomm_communicator_t communicator,
                                         const mpc_lowcomm_peer_uid_t source,
                                         const mpc_lowcomm_peer_uid_t destination,
                                         mpc_lowcomm_request_t *request,
                                         const size_t count,
                                         mpc_lowcomm_ptp_message_class_t message_class,
                                         mpc_lowcomm_datatype_t datatype,
                                         mpc_lowcomm_request_type_t request_type);

void mpc_lowcomm_ptp_message_send(mpc_lowcomm_ptp_message_t *msg);

void mpc_lowcomm_ptp_message_recv(mpc_lowcomm_ptp_message_t *msg);

void mpc_lowcomm_ptp_message_complete_and_free(mpc_lowcomm_ptp_message_t *msg);

int mpc_lowcomm_check_type_compat(mpc_lowcomm_datatype_t src, mpc_lowcomm_datatype_t dest);

/*********************
* BUFFERED MESSAGES *
*********************/

#define MAX_MPC_BUFFERED_SIZE    (8 * sizeof(long) )

typedef struct mpc_buffered_msg_s
{
	mpc_lowcomm_ptp_message_t header;
	/* mpc_lowcomm_request_t if the message is buffered  */
	mpc_lowcomm_request_t     request;
	long                      buf[(MAX_MPC_BUFFERED_SIZE / sizeof(long) ) + 1];
} mpc_buffered_msg_t;

/************************************************************************/
/* Message Progess                                                      */
/************************************************************************/

typedef struct mpc_lowcomm_ptp_msg_progress_s
{
	mpc_lowcomm_request_t *request;
	struct mpc_comm_ptp_s *recv_ptp;
	struct mpc_comm_ptp_s *send_ptp;
	mpc_lowcomm_peer_uid_t remote_process;
	int                    source_task_id;
	int                    dest_task_id;
	int                    polling_task_id;
	/* If we are blocked inside a function similar to MPI_Wait */
	int                    blocking;
} mpc_lowcomm_ptp_msg_progress_t;

void mpc_lowcomm_ptp_msg_wait_init(struct mpc_lowcomm_ptp_msg_progress_s *wait, mpc_lowcomm_request_t *request, int blocking);
void mpc_lowcomm_ptp_msg_progress(struct mpc_lowcomm_ptp_msg_progress_s *wait);

/************************************************************************/
/* General Functions                                                    */
/************************************************************************/


int mpc_lowcomm_is_remote_rank(int dest);
void mpc_lowcomm_init_per_task(int i);

void mpc_lowcomm_perform_idle(volatile int *data, int value, void (*func)(void *), void *arg);

/************************************************************************/
/* mpc_lowcomm_request_t                                                                    */
/************************************************************************/

static inline int mpc_lowcomm_request_get_completion(mpc_lowcomm_request_t *request)
{
	return request->completion_flag;
}

static inline void mpc_lowcomm_request_set_msg(mpc_lowcomm_request_t *request,
                                               mpc_lowcomm_ptp_message_t *msg)
{
	request->msg = msg;
}

static inline mpc_lowcomm_ptp_message_t *mpc_lowcomm_request_get_msg(mpc_lowcomm_request_t *request)
{
	return request->msg;
}

static inline void mpc_lowcomm_request_set_size(mpc_lowcomm_request_t *request)
{
	request->msg->body.header.msg_size = 0;
}

static inline void mpc_lowcomm_request_inc_size(mpc_lowcomm_request_t *request,
                                                size_t size)
{
	request->msg->body.header.msg_size += size;
}

static inline size_t mpc_lowcomm_request_get_size(mpc_lowcomm_request_t *request)
{
	return request->msg->body.header.msg_size;
}

static inline int mpc_lowcomm_request_get_source(mpc_lowcomm_request_t *request)
{
	return request->header.source_task;
}

static inline int mpc_lowcomm_request_is_null(mpc_lowcomm_request_t *request)
{
	return request->is_null;
}

static inline void mpc_lowcomm_request_set_null(mpc_lowcomm_request_t *request, int val)
{
	request->is_null = val;
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/

static inline int mpc_lowcomm_status_set_cancelled(mpc_lowcomm_status_t *status, int cancelled)
{
	status->cancelled = cancelled;
	return MPC_LOWCOMM_SUCCESS;
}

static inline int mpc_lowcomm_status_get_cancelled(const mpc_lowcomm_status_t *status, int *flag)
{
	*flag = (status->cancelled == 1);
	return MPC_LOWCOMM_SUCCESS;
}

/************************************************************************/
/* Thread-safe message probing	                                        */
/************************************************************************/

void sctk_m_probe_matching_init();
void sctk_m_probe_matching_set(int value);
void sctk_m_probe_matching_reset();
int sctk_m_probe_matching_get();


#endif /* MPC_MESSAGE_PASSING_INCLUDE_COMM_H_ */

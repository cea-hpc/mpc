#ifndef LCP_H
#define LCP_H

#include <stdlib.h>

#include "lcp_def.h"

/**
 * @defgroup LCP_API LowComm Protocol API
 * @{
 * Description of the LCP API.
 * @}
 */

/**
 * @defgroup LCP_CONTEXT LCP Context
 * @ingroup LCP_API
 * @{
 * LCP Context is the communication manager. Its primary functions are: list,
 * initiate, configure and instanciate the communication interfaces (rails)
 * available. There can be a single communication context per application.
 * @}
 */

/**
 * @defgroup LCP_EP LCP Endpoint
 * @ingroup LCP_API
 * @{
 * LCP endpoints are containers for connection and communication management.
 * They are virtual communication channels associated with each process in an
 * MPI program. There can be only one endpoint per MPC UNIX process. It supports
 * multirail (data striping and multiplexing) transparently.
 * @}
 */

/**
 * @defgroup LCP_TASK LCP Task
 * @ingroup LCP_API
 * @{
 * LCP Tasks are containers for task data (ex: tag matching list). Any
 * communication primitives has the local task as argument.
 * @}
 */

/**
 * @defgroup LCP_COMM LCP Communication
 * @ingroup LCP_API
 * @{
 * LCP Communication primitives.
 * @}
 */

/**
 * @defgroup LCP_MEM LCP Memory
 * @ingroup LCP_API
 * @{
 * LCP Memory provides abstraction to interact with memory registration on the
 * Network Interface Cards. It is primarily used for one-sided communication
 * primitives.
 * @}
 */

/**
 * @ingroup LCP_CONTEXT
 * @brief LCP context field mask. It is used to specify which parameters has
 * been specified in @ref lcp_context_param_t.
 */

enum {
        LCP_CONTEXT_DATATYPE_OPS  = MPC_BIT(0), /**< datatype mask */
        LCP_CONTEXT_PROCESS_UID   = MPC_BIT(1), /**< process uid mask */
        LCP_CONTEXT_NUM_TASKS     = MPC_BIT(2), /**< task number mask */
        LCP_CONTEXT_REQUEST_SIZE  = MPC_BIT(3), /**< upper layer request size */
        LCP_CONTEXT_REQUEST_CB    = MPC_BIT(4), /**< request init callback */
};

/**
 * @ingroup LCP_CONTEXT
 * @brief Datatype operations. There used to support packing/unpacking of
 * derived MPI datatypes.
 */
typedef struct lcp_dt_ops {
        void (*pack)(void *context, void *buffer);
        void (*unpack)(void *context, void *buffer);
} lcp_dt_ops_t;

/**
 * @ingroup LCP_CONTEXT
 * @brief LCP context parameters. It is passed to the \ref lcp_context_create
 * routine to configure communication context initialization.
 */
typedef struct lcp_context_param {
        uint32_t     field_mask;  /**< context field mask */
        uint64_t     process_uid; /**< local process UID */
        int          num_tasks;   /**< local process UID */
        lcp_dt_ops_t dt_ops; /** datatype operations (pack/unpack) */
        int          request_size;
        lcp_request_init_callback_func_t request_init;
} lcp_context_param_t;

/**
 * @ingroup LCP_CONTEXT
 * @brief Context creation.
 *
 * List, init and instanciate communication inferfaces available. Based on the
 * components (tcp, ptl, ofi,...), list available devices, open interfaces based
 * on user configuration.
 *
 * @param [in] param   \ref lcp_context_param_t Parameters for context
 *                     creation.
 * @param [out] ctx_p  Pointer to context handle that will be allocated by the
 *                     routine.
 * @return Error code returned.
 */
int lcp_context_create(lcp_context_h *ctx_p, lcp_context_param_t *param);

/**
 * @ingroup LCP_CONTEXT
 * @brief Context release.
 *
 * Release and deallocate all the communication interface. Call at the end of
 * the execution when no more communication are needed.
 *
 * @param [in] ctx   Context handle to release.
 * @return Error code returned.
 */
int lcp_context_fini(lcp_context_h ctx);

/**
 * @ingroup LCP_TASK
 * @brief Create LCP Task. 
 *
 * Instanciate an LCP Task that will contain all communication related
 * information that should be own by a MPI task. There are as many tasks as MPI
 * ranks, each task is identified by its global MPI rank.
 *
 * @param [in] ctx  Context handle.
 * @param [in] tid  Task identifier or MPI comm world rank.
 * @param [out] task_p  Pointer to task handle that will be allocated by the
 *                     routine.
 * @return Error code returned.
 */

int lcp_task_create(lcp_context_h ctx, int tid, lcp_task_h *task_p);

/**
 * @ingroup LCP_COMM
 * @brief LCP Task associate to manager. 
 *
 * Associates a task to a communication context, or \ref lcp_manager_h.
 * Internal structures are instanciated to enable communication context
 * isolation. Type of structure instanciated may depend on the communication
 * model of the manager. For example, matching queues are only created for
 * two-sided communication model.
 *
 * @param [in] task  Task to be associated.
 * @param [in] mngr  Manager handle to be associated to.
 * @return Error code returned.
 */
int lcp_task_associate(lcp_task_h task, lcp_manager_h mngr);

/**
 * @ingroup LCP_COMM
 * @brief LCP Task dissociate from manager. 
 *
 * Remove all data structure used within the communication context, or \ref
 * lcp_manager_h.
 *
 * @param [in] task  Task to be dissociated.
 * @param [in] mngr  Manager handle to be dissociated to.
 * @return Error code returned.
 */
int lcp_task_dissociate(lcp_task_h task, lcp_manager_h mngr);

/**
 * @ingroup LCP_CONTEXT
 * @brief Get task handle. 
 *
 * Get the task handle based on the task identifier (TID). The TID is the MPI
 * rank in the MPI_COMM_WORLD communicator.
 *
 * @param [in] ctx  Context handle.
 * @param [in] tid  Task identifier or MPI comm world rank.
 * @return \ref lcp_task_h task handle for the specified tid.
 */
lcp_task_h lcp_context_task_get(lcp_context_h ctx, int tid);

/**
 * @ingroup LCP_COMM
 * @brief Manager fields 
 *
 * Specifies fields present in \ref lcp_manager_param_t during manager creation.
 */
enum {
        LCP_MANAGER_ESTIMATED_EPS = MPC_BIT(0),
        LCP_MANAGER_NUM_TASKS     = MPC_BIT(1),
        LCP_MANAGER_COMM_MODEL    = MPC_BIT(2),
};

/**
 * @ingroup LCP_COMM
 * @brief Manager instanciation flags. 
 *
 * Specifies a set of flags that are proposed to improve performances in some
 * situation.
 */
enum {
        LCP_MANAGER_TSC_MODEL = MPC_BIT(0),  /**< Init interface Two-Sided Communication capabilities. */
        LCP_MANAGER_OSC_MODEL = MPC_BIT(1),  /**< Init interface One-Sided Communication capabilities. */
};

/**
 * @ingroup LCP_COMM
 * @brief Manager parameters 
 *
 * Specifies the parameters that have been specified in \ref
 * lcp_manager_param_t.
 */

typedef struct lcp_manager_param {
        unsigned field_mask;
        int      estimated_eps; /**< Estimated number of endpoints. */
        int      num_tasks;     /**< num of tasks (MPI processes) */
        unsigned flags;         /**< communication model. */
} lcp_manager_param_t;

/**
 * @ingroup LCP_COMM
 * @brief Create LCP Manager. 
 *
 * An LCP manager act as a communication context that groups all outstanding
 * communications. It can be used to insulate different communication contexts
 * such as MPI windows or global contexts to allow fine grain progression,
 * endpoint flush, etc... 
 * Endpoints \ref lcp_ep_h are local to managers. An LCP manager is shared
 * between a group of LCP Tasks.
 *
 * @param [in] ctx  Context handle.
 * @param [out] mngr_p  Pointer to manager handle that will be allocated by the
 *                     routine.
 * @param [in] params  Manager parameters.
 * @return Error code returned.
 */
int lcp_manager_create(lcp_context_h ctx, lcp_manager_h *mngr_p, 
                       lcp_manager_param_t *params);

/**
 * @ingroup LCP_COMM
 * @brief Delete LCP Manager. 
 *
 * Deletes resources associated to a LCP manager.
 *
 * @param [in] mngr  Manager handle to be deleted.
 * @return Error code returned.
 */
int lcp_manager_fini(lcp_manager_h mngr);

/**
 * @ingroup LCP_CONTEXT
 * @brief Communication progress.
 *
 * Progress all outstanding communications.
 *
 * @param [in] ctx  Context handle.
 * @return Error code returned.
 */
//FIXME: change to lcp_manager_progress
int lcp_progress(lcp_manager_h mngr);

/**
 * @ingroup LCP_EP
 * @brief Create LCP Endpoint.
 *
 * Instanciate a protocol endpoint. Upon return, connection will have been
 * started and the endpoint can be used directly with communication primitives.
 *
 * @param [in] ctx  Manager handle.
 * @param [out] ep_p  Pointer to endpoint handle that will be allocated by the
 *                     routine.
 * @param [in] uid  Process identifier or MPC UNIX identifier.
 * @param [in] flags Flags to configure creation (unused for now).
 * @return Error code returned.
 */
int lcp_ep_create(lcp_manager_h mngr, 
                  lcp_ep_h *ep_p, 
		  uint64_t uid, 
                  unsigned flags);

/**
 * @ingroup LCP_EP
 * @brief Get LCP Endpoint.
 *
 * Get a protocol endpoint based on UNIX process identifier (UID).
 *
 * @param [in] ctx  Manager handle.
 * @param [in] uid  Process identifier or MPC UNIX identifier.
 * @return \ref lcp_ep_h endpoint handle for the specified UID.
 */
lcp_ep_h lcp_ep_get(lcp_manager_h mngr, 
                uint64_t uid);

/**
 * @ingroup LCP_EP
 * @brief Get LCP Endpoint or create it if it does not exist.
 *
 * Get a protocol endpoint based on UNIX process identifier (UID), creates it if
 * it to does exist yet.
 *
 * @param [in] ctx  Manager handle.
 * @param [in] uid  Process identifier or MPC UNIX identifier.
 * @param [out] ep_p  Pointer to endpoint handle that will be allocated by the
 *                     routine.
 * @param [in] flags Flags to configure creation (unused for now).
 * @return Error code returned.
 */
int lcp_ep_get_or_create(lcp_manager_h mngr, 
                uint64_t uid, lcp_ep_h *ep_p, 
                unsigned flags);

/**
 * @ingroup LCP_COMM
 * @brief Request field masks
 *
 * Specifies the parameters that have been specified in \ref
 * lcp_request_param_t.
 */
enum {
//FIXME: Some flags does not specify any field within lcp_request_param_t but
//       just a functionality (eg LCP_REQUEST_TRY_OFFLOAD or
//       LCP_REQUEST_TAG_SYNC). Maybe this should be divided in two enum for
//       clarity.
        LCP_REQUEST_TRY_OFFLOAD   = MPC_BIT(0), /**< Try offload send mask */
        LCP_REQUEST_USER_DATA     = MPC_BIT(1), /**< User data mask */
        LCP_REQUEST_USER_REQUEST  = MPC_BIT(2), /**< User request mask */
        LCP_REQUEST_TAG_SYNC      = MPC_BIT(3), /**< Sync request mask */
        LCP_REQUEST_AM_SYNC       = MPC_BIT(4), /**< AM sync request mask */
        LCP_REQUEST_AM_CALLBACK   = MPC_BIT(5), /**< AM callback mask */ 
        LCP_REQUEST_RMA_CALLBACK  = MPC_BIT(6), /**< Atomic callback mask */ 
        LCP_REQUEST_REPLY_BUFFER  = MPC_BIT(7), /**< Result buffer for Atomics */
        LCP_REQUEST_USER_MEMH     = MPC_BIT(8), /**< User-provided local Memory handle */
        LCP_REQUEST_USER_EPH      = MPC_BIT(9), /**< User-provided Endpoint handle */
        LCP_REQUEST_TAG_CALLBACK  = MPC_BIT(10), /**< TAG callback mask */ 
};

/**
 * @ingroup LCP_COMM
 * @brief Tag receive information
 *
 * Information to be returned to user whenever a request as been matched. The
 * structure is passed from upper layer and field by LCP with tag information
 * (length, tag, source and found in case of probing).
 * For example, this can be used in MPI_Status.
 */
typedef struct lcp_tag_recv_info {
        size_t length; /**< Length of the data received by the matched request */
        int32_t tag; /**< Tag of the matched request */
        int32_t src; /**< Source of the matched request */
        unsigned found; /**< Has request been found in matching list or not */
        unsigned flags; /**< Flags containing info from LCP (truncation, ...) */
} lcp_tag_recv_info_t;

/**
 * @ingroup LCP_COMM
 * @brief Tag information
 *
 * Information to be returned to user whenever a request as been matched. The
 * structure is passed from upper layer and field by LCP with tag information
 * (length, tag, source and found in case of probing).
 * For example, this can be used in MPI_Status.
 */
typedef struct lcp_tag_info {
        int32_t tag; /**< Tag of the matched request */
        int32_t src; /**< Source of the matched request */
        int32_t dest; /**< Source of the matched request */
        uint64_t src_uid; /**< Source UID */
        uint64_t dest_uid; /**< Destination UID */
        uint16_t comm_id; /**< Communicator ID */
        unsigned found; /**< Has request been found in matching list or not */
} lcp_tag_info_t;

/**
 * @ingroup LCP_COMM
 * @brief Datatype type 
 *
 * Specifies if the datat in the request is contiguous or not (derived)
 */
//FIXME: defined as a flags while it should be just an integer
enum lcp_dt_type {
        LCP_DATATYPE_CONTIGUOUS = MPC_BIT(0),
        LCP_DATATYPE_DERIVED    = MPC_BIT(1),
};

/**
 * @ingroup LCP_COMM
 * @brief Request parameters.
 *
 * Specifies a set of fields used to characterize how a request should be
 * handled by the communication primitives. It is primarily used to specify user
 * callback and to pass tag receive information (\ref lcp_tag_recv_info_t).
 */
//FIXME: union the callbacks.
typedef struct lcp_request_param {
        uint32_t                     field_mask; /**< Flags to indicate which parameter is used */
        lcp_send_callback_func_t     send_cb; /**< Completion callback for send requests */
        lcp_tag_recv_callback_func_t recv_cb; /**< Completion callback for recv requests */
        lcp_send_callback_func_t     am_cb; /**< Completion callback for recv AM requests */
        void                        *reply_buffer; /**< Location for returned value with atomic operations */
        lcp_datatype_t               datatype; /**< Contiguous or non-contiguous data */
        void                        *request; /**< Pointer to upper layer request for completion */
        lcp_mem_h                    mem; /**< Memory handle for RMA requests */
        lcp_ep_h                     ep; /**< Endpoint to be flushed. */
} lcp_request_param_t;

/**
 * @ingroup LCP_COMM
 * @brief Initialize a request.
 *
 * Returns an initialized pointer to a user request. Within the upper layer, it
 * is sometimes needed to initialize fields of upper layer request before
 * actually sending the request. 
 * The user is then responsible to free the request using \ref lcp_request_free
 *
 * @param [in]  task  Task handle of the task allocating the request.
 * @param [out] request Request handle containing the upper layer request.
 * @return Error code returned.
 */
void *lcp_request_alloc(lcp_task_h task);

/**
 * @ingroup LCP_COMM
 * @brief Free a request.
 *
 * Free a previously allocated request from \ref lcp_request_alloc
 *
 * @param [int] request Request handle to be freed.
 * @return Error code returned.
 */
void lcp_request_free(void *request);
/**
 * @ingroup LCP_COMM
 * @brief LCP send tag communication. 
 *
 * Communication primitive to send a tag message to the corresponding \ref
 * lcp_tag_recv_nb(). The routine is non-blocking and thus returns directly,
 * however @a request may be completed later depending on how the message is
 * sent. @a request is considered completed when @a buffer can be safely reused
 * by the user.
 * Actual completion is performed by setting the \ref mpc_lowcomm_request_t
 * COMPLETION_FLAG.
 *
 * @param [in] ep  Endpoint handle to the destination.
 * @param [in] task  Task handle of the source task.
 * @param [in] buffer  Address of source buffer.
 * @param [in] count  Size of source buffer.
 * @param [in] request  Lowcomm request.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Error code returned.
 */
int lcp_tag_send_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer,
                    size_t count, lcp_tag_info_t *tag_info, 
                    const lcp_request_param_t *param);

/**
 * @ingroup LCP_COMM
 * @brief LCP receive tag communication.
 *
 * Communication primitive to receive a tag message coming from \ref
 * lcp_tag_send_nb(). The routine is non-blocking and thus returns directly,
 * request completion depends on whether it has been matched. Completion is
 * performed internally by setting the \ref mpc_lowcomm_request_t
 * COMPLETION_FLAG, it can be done directly within the routine or later by
 * a call to \ref lcp_progress().
 *
 * @param [in] task  Task handle of the source task.
 * @param [in] buffer  Address of buffer where data is copied.
 * @param [in] count  Size of source buffer.
 * @param [in] request  Lowcomm request.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Error code returned.
 */
int lcp_tag_recv_nb(lcp_manager_h mngr, lcp_task_h task, void *buffer, 
                    size_t count, lcp_tag_info_t *tag_info, int32_t src_mask,
                    int32_t tag_mask, lcp_request_param_t *param);

/**
 * @ingroup LCP_COMM
 * @brief LCP probe tag.
 *
 * Communication primitive to probe internal matching lists to check if
 * a message has been received.
 * Information is filled within @a recv_info.
 *
 * @param [in] task  Task handle of the source task.
 * @param [in] src  MPI rank.
 * @param [in] tag  MPI tag.
 * @param [in] comm  Communicator identifier.
 * @param [in] recv_info  Receive information container that will be filled in
 *                        case a match is found.
 * @return Error code returned.
 */
//FIXME: comm should be a const uint16_t
int lcp_tag_probe_nb(lcp_manager_h mngr, lcp_task_h task, const int src, 
                     const int tag, const uint64_t comm,
                     lcp_tag_recv_info_t *recv_info);

/**
 * @ingroup LCP_COMM
 * @brief LCP Active Message flags. 
 *
 * Flags identifying the type of message returned by the callback \ref
 * lcp_am_callback_t. 
 */
enum {
        LCP_AM_EAGER = MPC_BIT(0), /**< Message received is of type eager. */
        LCP_AM_RNDV  = MPC_BIT(1), /**< Message received is of type rendez-vous. */
};

/**
 * @ingroup LCP_COMM
 * @brief LCP Active Message receive parameters.
 *
 * Parameters that are returned to the user through @a param argument of \ref
 * lcp_am_callback_t user callback. The callback has previously been defined by
 * the user and set to the local task through \ref lcp_am_set_handler_callback.
 */
typedef struct lcp_am_recv_param {
        uint32_t flags;
        lcp_ep_h reply_ep;
} lcp_am_recv_param_t;

/**
 * @ingroup LCP_COMM
 * @brief LCP register user handler for a specific AM identifier.
 *
 * Register a user handler with a user specified AM identifier. Handler will be
 * called when a message, that has been sent through \ref lcp_am_send_nb with the
 * specified AM ID, has been received. The user may use the opaque @a arg
 * pointer to pass custom data.
 *
 * @param [in] task  Task handle of the source task.
 * @param [in] am_id  Unique handler identifier.
 * @param [in] arg Opaque pointer to user data.
 * @param [in] cb  User-defined handler callback.
 * @param [in] flags  Flags (unused).
 * @return Error code returned.
 */
int lcp_am_set_handler_callback(lcp_manager_h mngr, lcp_task_h task, uint8_t am_id,
                                void *arg, lcp_am_callback_t cb,
                                unsigned flags);

/**
 * @ingroup LCP_COMM
 * @brief LCP send AM communication.
 *
 * Send data using the Active Message datapath. A user-defined handler must have
 * been set prior to calling this routine, see \ref lcp_am_set_handler_callback.
 *
 * In case of thread-based communication, we must specify the destination rank
 * through @a dest_tid.
 *
 * @param [in] ep  Endpoint to destination.
 * @param [in] task  Task handle of the local task.
 * @param [in] dest_tid  Unique handler identifier.
 * @param [in] am_id  Unique handler identifier.
 * @param [in] hdr  Pointer to user-defined header.
 * @param [in] hdr_size  Size of user-defined header.
 * @param [in] buffer  Pointer to buffer.
 * @param [in] count Size of buffer.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Error code returned.
 */
int lcp_am_send_nb(lcp_ep_h ep, lcp_task_h task, int32_t dest_tid,
                   uint8_t am_id, void *hdr, size_t hdr_size, const void *buffer,
                   size_t count, const lcp_request_param_t *param);

/**
 * @ingroup LCP_COMM
 * @brief LCP receive AM communication.
 *
 * This routine may be called to provide to LCP the necessary user buffer to
 * receive the data in case it is sent through the rendez-vous protocol. This
 * routine may thus be used in conjunction with \ref LCP_AM_RNDV flags that
 * would have been set within the user callback through \ref
 * lcp_am_recv_param_t.
 * To be notified for completion, user must provide set the completion callback
 * in \ref lcp_request_param_t.
 *
 * @param [in] task  Task handle of the source task.
 * @param [in] data_ctnr  Opaque pointer that must be @a data from \ref
 *                        lcp_am_callback_t.
 * @param [in] buffer  Buffer on which received data will be written.
 * @param [in] count  Size of buffer.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Error code returned.
 */
int lcp_am_recv_nb(lcp_manager_h mngr, lcp_task_h task, void *data_ctnr, 
                   void *buffer, size_t count, lcp_request_param_t *param);

/**
 * @ingroup LCP_COMM
 * @brief LCP Put RMA communication.
 *
 * This routine exposes RMA put capabilities with the same semantics.
 *
 * User may provide a callback through \ref lcp_request_param_t so that it can
 * be notified when the request has completed.
 * As a return value, a pointer to the upper layer is provided which can be used
 * to check progression of the communication.
 *
 * @param [in] ep  Endpoint to destination.
 * @param [in] task  Task handle of the source task.
 * @param [in] buffer  Buffer from which data will be sent.
 * @param [in] length  Length of data in buffer.
 * @param [in] remote_disp  displacement from start of the target buffer
 *                          described by \ref rkey.
 * @param [in] rkey  Remote memory key handle.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Pointer to upper layer request.
 */
lcp_status_ptr_t lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer,
                            size_t length, uint64_t remote_disp, lcp_mem_h rkey,
                            const lcp_request_param_t *param);

/**
 * @ingroup LCP_COMM
 * @brief LCP Get RMA communication. 
 *
 * This routine exposes RMA get capabilities with the same semantics.
 *
 * User may provide a callback through \ref lcp_request_param_t so that it can
 * be notified when the request has completed.
 * As a return value, a pointer to the upper layer is provided which can be used
 * to check progression of the communication.
 *
 * @param [in] ep  Endpoint to destination.
 * @param [in] task  Task handle of the source task.
 * @param [in] buffer  Buffer from which data will be sent.
 * @param [in] length  Length of data in buffer.
 * @param [in] remote_addr  Remote address where data is written.
 * @param [in] rkey  Remote memory key handle.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Error code returned.
 */
lcp_status_ptr_t lcp_get_nb(lcp_ep_h ep, lcp_task_h task, void *buffer, 
                            size_t length, uint64_t remote_addr, lcp_mem_h rkey,
                            const lcp_request_param_t *param); 

/**
 * @ingroup LCP_COMM
 * @brief LCP Flush RMA communication. 
 *
 * This routine flushes all outstanding RMA operations. Once the operation is
 * complete, all RMA operations issued before this call are locally and remotely
 * completed.
 *
 * User may provide a callback through \ref lcp_request_param_t so that it can
 * be notified when the request has completed.
 *
 * @param [in] mngr  Communication manager.
 * @param [in] task  Task handle of the source task.
 * @param [in] param  Request parameters \ref lcp_request_param_t.
 * @return Error code returned.
 */
int lcp_flush_nb(lcp_manager_h mngr, lcp_task_h task, 
                 const lcp_request_param_t *param);

typedef enum {
        LCP_ATOMIC_OP_ADD,
        LCP_ATOMIC_OP_AND,
        LCP_ATOMIC_OP_OR,
        LCP_ATOMIC_OP_XOR,
        LCP_ATOMIC_OP_SWAP,
        LCP_ATOMIC_OP_CSWAP,
} lcp_atomic_op_t;

int lcp_atomic_op_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer,
                     size_t length, uint64_t remote_addr, lcp_mem_h rkey,
                     lcp_atomic_op_t op_type, const lcp_request_param_t *param);

/**
 * @ingroup lcp_mem
 * @brief lcp memory flags. 
 *
 * Specifies wether the memory should be allocated or not by lcp_mem_register.
 *
 */

enum {
        LCP_MEM_REGISTER_ALLOCATE = MPC_BIT(0),
        LCP_MEM_REGISTER_DYNAMIC  = MPC_BIT(1),
        LCP_MEM_REGISTER_STATIC   = MPC_BIT(2),
};

/**
 * @ingroup lcp_mem
 * @brief lcp memory parameters. 
 *
 * Specifies a set of fields used to characterize how a memory should be
 * registered. for example, memory could be already allocated or, on the
 * contrary it should be allocated by lcp.
 *
 */

typedef struct lcp_mem_param {
        uint32_t    field_mask; 
        unsigned    flags;
        const void *address;
        size_t      size;
} lcp_mem_param_t;

/**
 * @ingroup LCP_MEM
 * @brief LCP register memory.
 *
 * Register memory to a NIC and get a memory handle from it.
 *
 * @param [in] mngr  Manager handle.
 * @param [in] mem_p  Pointer handle to registered memory.
 * @param [in] buffer  Buffer from which data will be registered.
 * @param [in] length  Length of data in buffer.
 * @param [in] flags  Flags to specify kind of registration.
 * @return Error code returned.
 */
int lcp_mem_register(lcp_manager_h mngr, lcp_mem_h *mem_p, 
                     lcp_mem_param_t *params);

/**
 * @ingroup LCP_MEM
 * @brief LCP deregister memory.
 *
 * Deregister memory from a NIC.
 *
 * @param [in] mngr  Manager handle.
 * @param [in] mem  Memory handle to be deregistered.
 * @return Error code returned.
 */
int lcp_mem_deregister(lcp_manager_h mngr, lcp_mem_h mem);

/**
 * @ingroup LCP_MEM
 * @brief LCP query registered memory. 
 *
 * Query attributes of memory that has been registered.
 *
 * @param [in] ctx  Context handle.
 * @param [in] mem  Memory handle to be deregistered.
 * @return Error code returned.
 */
int lcp_mem_query(lcp_mem_h mem, lcp_mem_attr_t *mem_attr);

/**
 * @ingroup LCP_MEM
 * @brief LCP pack memory key. 
 *
 * Pack a memory key so it can be sent to a remote peer.
 *
 * It is allocated by LCP and must be released appriopriately, see \ref
 * lcp_mem_release_rkey_buf.
 *
 * @param [in] mngr  Manager handle.
 * @param [in] mem  Memory handle to be packed.
 * @param [in] rkey_buf_p  Buffer where memory key data will be packed
 *                         (allocated by LCP).
 * @param [in] rkey_len  Length of packed memory key.
 * @return Error code returned.
 */
int lcp_mem_pack(lcp_manager_h mngr, lcp_mem_h mem, 
                    void **rkey_buf_p, int *rkey_len);

/**
 * @ingroup LCP_MEM
 * @brief LCP unpack memory key.
 *
 * Allocate a memory key and unpack a memory key so it can be used for RMA
 * communication.
 *
 *
 * @param [in] mngr  Manager handle.
 * @param [in] mem_p  Pointer where memory key will be unpacked.
 * @param [in] src  Buffer where memory key data has been packed
 * @param [in] size  Size of the packed key.
 * @return Error code returned.
 */
int lcp_mem_unpack(lcp_manager_h mngr, lcp_mem_h *mem_p, 
                   void *src, size_t size);

/**
 * @ingroup LCP_MEM
 * @brief LCP release memory key.
 *
 * Release packed memory key created through \ref lcp_mem_pack.
 *
 *
 * @param [in] rkey_buffer Pointer to packed memory key.
 * @return Error code returned.
 */
void lcp_mem_release_rkey_buf(void *rkey_buffer);

#endif

#ifndef LCP_H
#define LCP_H

#include <stdlib.h>

#include "lcp_def.h"
#include "lcp_common.h"

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

/* LCP return code */
typedef enum {
        LCP_SUCCESS     =  0,
        LCP_NO_RESOURCE = -1,
        LCP_ERROR       =  1,
} lcp_status_t;

/**
 * @ingroup LCP_CONTEXT 
 * @brief LCP context field mask. It is used to specify which parameters has
 * been specified in @ref lcp_context_param_t.
 */

enum {
        LCP_CONTEXT_DATATYPE_OPS  = LCP_BIT(0), /**< datatype mask */
        LCP_CONTEXT_PROCESS_UID   = LCP_BIT(1), /**< process uid mask */
        LCP_CONTEXT_NUM_PROCESSES = LCP_BIT(2), /**< num processes mask */
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
        uint32_t     flags;  /**< context field mask */
        uint64_t     process_uid; /**< local process UID */
        int          num_tasks; /**< num of tasks (MPI processes) */
        int          num_processes; /** num of processes (UNIX processes) */
        lcp_dt_ops_t dt_ops; /** datatype operations (pack/unpack) */
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
 * @ingroup LCP_CONTEXT
 * @brief Communication progress. 
 *
 * Progress all outstanding communications.
 *
 * @param [in] ctx  Context handle.
 * @return Error code returned.
 */
int lcp_progress(lcp_context_h ctx);

/**
 * @ingroup LCP_TASK
 * @brief Create LCP Task. 
 *
 * Instanciate an LCP Task that will contain all communication related
 * information that should be own by a MPI task.
 *
 * @param [in] ctx  Context handle.
 * @param [in] tid  Task identifier or MPI comm world rank.
 * @param [out] task_p  Pointer to task handle that will be allocated by the
 *                     routine.
 * @return Error code returned.
 */
int lcp_task_create(lcp_context_h ctx, int tid, lcp_task_h *task_p);

/**
 * @ingroup LCP_EP
 * @brief Create LCP Endpoint. 
 *
 * Instanciate a protocol endpoint. Upon return, connection will have been
 * started and the endpoint can be used directly with communication primitives.
 *
 * @param [in] ctx  Context handle.
 * @param [out] ep_p  Pointer to endpoint handle that will be allocated by the
 *                     routine.
 * @param [in] uid  Process identifier or MPC UNIX identifier.
 * @param [in] flags Flags to configure creation (unused for now).
 * @return Error code returned.
 */
int lcp_ep_create(lcp_context_h ctx, 
                  lcp_ep_h *ep_p, 
		  uint64_t uid, 
                  unsigned flags);

/**
 * @ingroup LCP_EP
 * @brief Get LCP Endpoint. 
 *
 * Get a protocol endpoint based on UNIX process identifier (UID).
 *
 * @param [in] ctx  Context handle.
 * @param [in] uid  Process identifier or MPC UNIX identifier.
 * @return \ref lcp_ep_h endpoint handle for the specified UID.
 */
lcp_ep_h lcp_ep_get(lcp_context_h ctx, 
                uint64_t uid);

/**
 * @ingroup LCP_EP
 * @brief Get LCP Endpoint or create it if it does not exist. 
 *
 * Get a protocol endpoint based on UNIX process identifier (UID), creates it if
 * it to does exist yet.
 *
 * @param [in] ctx  Context handle.
 * @param [in] uid  Process identifier or MPC UNIX identifier.
 * @param [out] ep_p  Pointer to endpoint handle that will be allocated by the
 *                     routine.
 * @param [in] flags Flags to configure creation (unused for now).
 * @return Error code returned.
 */
int lcp_ep_get_or_create(lcp_context_h ctx, 
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
        LCP_REQUEST_TRY_OFFLOAD   = LCP_BIT(0), /**< Try offload send mask */
        LCP_REQUEST_USER_DATA     = LCP_BIT(1), /**< User data mask */
        LCP_REQUEST_USER_REQUEST  = LCP_BIT(2), /**< User request mask */
        LCP_REQUEST_TAG_SYNC      = LCP_BIT(3), /**< Sync request mask */
        LCP_REQUEST_AM_SYNC       = LCP_BIT(4), /**< AM sync request mask */
        LCP_REQUEST_AM_CALLBACK   = LCP_BIT(5), /**< AM callback mask */ 
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
typedef struct lcp_tag_info {
        size_t length; /**< Length of the data received by the matched request */
        int32_t tag; /**< Tag of the matched request */
        int32_t src; /**< Source of the matched request */
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
        LCP_DATATYPE_CONTIGUOUS = LCP_BIT(0),
        LCP_DATATYPE_DERIVED    = LCP_BIT(1),
};

/**
 * @ingroup LCP_COMM
 * @brief Request parameters.
 *
 * Specifies a set of fields used to characterize how a request should be
 * handled by the communication primitives. It is primarily used to specify user
 * callback and to pass tag information (\ref lcp_tag_info_t).
 */
typedef struct lcp_request_param {
        uint32_t                     flags; /**< Flags to indicate which parameter is used */
        lcp_tag_info_t         *tag_info; /**< Receive info field upon matching completion */
        lcp_rma_completion_func_t   on_rma_completion; /**< Completion callback for send RMA requests */
        lcp_am_completion_func_t    on_am_completion; /**< Completion callback for recv AM requests */
        void                        *user_request; /**< User data attached with the AM callback 
                                                     \ref lcp_am_completion_func_t */
        lcp_datatype_t               datatype; /**< Contiguous or non-contiguous data */
        mpc_lowcomm_request_t       *request; /**< Pointer to lowcomm request for completion */
        lcp_mem_h                    memh; /**< Memory handle for RMA requests */
} lcp_request_param_t;

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
                    size_t count, mpc_lowcomm_request_t *request, 
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
int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param);

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
int lcp_tag_probe_nb(lcp_task_h task, const int src, 
                     const int tag, const uint64_t comm,
                     lcp_tag_info_t *recv_info);

enum {
        LCP_AM_EAGER = LCP_BIT(0),
        LCP_AM_RNDV  = LCP_BIT(1),
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
int lcp_am_set_handler_callback(lcp_task_h task, uint8_t am_id,
                                void *arg, lcp_am_user_func_t cb,
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
int lcp_am_recv_nb(lcp_task_h task, void *data_ctnr, void *buffer, 
                   size_t count, lcp_request_param_t *param);

/**
 * @ingroup LCP_COMM
 * @brief LCP Put RMA communication. 
 *
 * This routine exposes RMA put capabilities with the same semantics.
 *
 * User may provide a callback through \ref lcp_request_param_t so that it can
 * be notified when the request has completed.
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
int lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, 
               size_t length, uint64_t remote_addr, lcp_mem_h rkey,
               const lcp_request_param_t *param); 

/**
 * @ingroup LCP_MEM
 * @brief LCP register memory. 
 *
 * Register memory to a NIC and get a memory handle from it.
 *
 * @param [in] ctx  Context handle.
 * @param [in] mem_p  Pointer handle to registered memory.
 * @param [in] buffer  Buffer from which data will be registered.
 * @param [in] length  Length of data in buffer.
 * @return Error code returned.
 */
int lcp_mem_register(lcp_context_h ctx, lcp_mem_h *mem_p, void *buffer, 
                     size_t length);

/**
 * @ingroup LCP_MEM
 * @brief LCP deregister memory. 
 *
 * Deregister memory from a NIC.
 *
 * @param [in] ctx  Context handle.
 * @param [in] mem  Memory handle to be deregistered.
 * @return Error code returned.
 */
int lcp_mem_deregister(lcp_context_h ctx, lcp_mem_h mem);

/**
 * @ingroup LCP_MEM
 * @brief LCP pack memory key. 
 *
 * Pack a memory key so it can be sent to a remote peer.
 *
 * It is allocated by LCP and must be released appriopriately, see \ref
 * lcp_mem_release_rkey_buf.
 *
 * @param [in] ctx  Context handle.
 * @param [in] mem  Memory handle to be packed.
 * @param [in] rkey_buf_p  Buffer where memory key data will be packed
 *                         (allocated by LCP).
 * @param [in] rkey_len  Length of packed memory key.
 * @return Error code returned.
 */
int lcp_mem_pack(lcp_context_h ctx, lcp_mem_h mem, 
                    void **rkey_buf_p, size_t *rkey_len);

/**
 * @ingroup LCP_MEM
 * @brief LCP unpack memory key. 
 *
 * Allocate a memory key and unpack a memory key so it can be used for RMA
 * communication.
 *
 *
 * @param [in] ctx  Context handle.
 * @param [in] mem_p  Pointer where memory key will be unpacked.
 * @param [in] src  Buffer where memory key data has been packed
 * @param [in] size  Size of the packed key.
 * @return Error code returned.
 */
int lcp_mem_unpack(lcp_context_h ctx, lcp_mem_h *mem_p, 
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

#ifndef LCP_REQUEST_H
#define LCP_REQUEST_H

#include "lcp_types.h"

#include "lcp.h"
//FIXME: try to remove this dependency. Currently needed to make progress on
//       endpoint connection.
#include "lcp_ep.h"
#include "lcp_context.h"

#include <sctk_rail.h>
#include <sctk_alloc.h>

enum {
	LCP_REQUEST_RNDV_TAG            = LCP_BIT(0),
	LCP_REQUEST_DELETE_FROM_PENDING = LCP_BIT(1),
	LCP_REQUEST_RECV_TRUNC          = LCP_BIT(2), /* flags if request is truncated */
        LCP_REQUEST_MPI_COMPLETE        = LCP_BIT(3), /* call MPI level completion callback, 
                                                         see ucp_request_complete */
        LCP_REQUEST_RMA_COMPLETE        = LCP_BIT(4),
        LCP_REQUEST_OFFLOADED_RNDV      = LCP_BIT(5),
        LCP_REQUEST_RECV                = LCP_BIT(6),
        LCP_REQUEST_SEND                = LCP_BIT(7),
        LCP_REQUEST_NEED_PROGRESS       = LCP_BIT(8),
};

enum {
	LCP_RECV_CONTAINER_UNEXP_RNDV_TAG       = LCP_BIT(0),
	LCP_RECV_CONTAINER_UNEXP_EAGER_TAG      = LCP_BIT(1),
	LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC = LCP_BIT(2),
};

/* Store data for unexpected am messages
 * Data is stored after (at lcp_unexp_cntr + 1)
 */
struct lcp_unexp_ctnr {
	size_t length;
	unsigned flags;
};

/* Generatic send function */
typedef int (*lcp_send_func_t)(lcp_request_t *req);

struct lcp_request {
	uint64_t       flags;
	lcp_context_h  ctx; 
	lcp_task_h     task; 
        lcp_datatype_t datatype;
        mpc_lowcomm_ptp_message_class_t message_type;
	union {
		struct {
			lcp_ep_h ep;
			size_t length; /* Total length, in bytes */
			void *buffer;
			lcr_tag_context_t t_ctx;
                        lcp_complete_callback_func_t cb;
                        size_t (*pack_function)(void *dest, void *data);
                        uint64_t ack_msg_key;
			union {
				struct {
					uint16_t comm;
					int32_t  src_pid;
					int32_t  dest_pid;
                                        int32_t  src_tid;
                                        int32_t  dest_tid;
					int32_t  tag;
				} tag;

                                struct {
                                        uint64_t  addr;
                                        lcp_mem_h rkey;
                                } rndv;

                                struct {
                                        uint64_t remote_addr;
                                        lcp_mem_h rkey;
                                } rma;
			};

                        int rndv_proto;
			lcp_send_func_t func;
		} send;

		struct {
			size_t length;
                        size_t send_length;
			void *buffer;
			lcr_tag_context_t t_ctx;

                        struct {
                                uint16_t comm;
                                int32_t  src_pid;
                                int32_t  dest_pid;
                                int32_t  src_tid;
                                int32_t  dest_tid;
                                int32_t  tag;
                        } tag;

		} recv;
	};

	//NOTE: break LCP modularity
	mpc_lowcomm_request_t *request; /* Upper layer request */
        lcp_tag_recv_info_t   *info;
        void                  *user_data;
	int16_t                seqn;    /* Sequence number */
	uint64_t               msg_id;  /* Unique message identifier */
        struct lcp_request    *super;   /* master request */
        mpc_queue_elem_t       queue;   /* element in pending queue */
        
        struct {
                lcr_tag_t imm;
                int mid; /* matching id */
        } tm;

	struct {
                void *           pack_buf;
                size_t           offset;
                size_t           remaining;
                lcr_completion_t comp;
                lcp_mem_h        lmem; 
                int              offloaded;
	} state;
};

#define LCP_REQUEST_INIT_TAG_SEND(_req, _ctx, _task, _mpi_req, _info, _length, \
                                  _ep, _buf, _seqn, _msg_id, _dt) \
{ \
	(_req)->request           = _mpi_req;                                \
        (_req)->send.tag.src_tid  = (_mpi_req)->header.source_task;          \
        (_req)->send.tag.dest_tid = (_mpi_req)->header.destination_task;     \
        (_req)->send.tag.dest_pid = (_mpi_req)->header.destination;          \
        (_req)->send.tag.src_pid  = (_mpi_req)->header.source;               \
        (_req)->send.tag.tag      = (_mpi_req)->header.message_tag;          \
        (_req)->send.tag.comm     = (_mpi_req)->header.communicator_id;      \
	(_req)->info              = _info;                                   \
	(_req)->msg_id            = _msg_id;                                 \
	(_req)->seqn              = _seqn;                                   \
	(_req)->ctx               = _ctx;                                    \
	(_req)->task              = _task;                                   \
	(_req)->datatype          = _dt;                                     \
	\
	(_req)->send.buffer       = _buf;                                    \
	(_req)->send.ep           = _ep;                                     \
	(_req)->send.length       = _length;                                 \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

#define LCP_REQUEST_INIT_RMA_SEND(_req, _ctx, _task, _mpi_req, _info, _length, \
                                  _ep, _buf, _seqn, _msg_id, _dt) \
{ \
	(_req)->request           = _mpi_req;                                \
	(_req)->info              = _info;                                   \
	(_req)->msg_id            = _msg_id;                                 \
	(_req)->seqn              = _seqn;                                   \
	(_req)->ctx               = _ctx;                                    \
	(_req)->task              = _task;                                   \
	(_req)->datatype          = _dt;                                     \
	\
	(_req)->send.buffer       = _buf;                                    \
	(_req)->send.ep           = _ep;                                     \
	(_req)->send.length       = _length;                                 \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

#define LCP_REQUEST_INIT_TAG_RECV(_req, _ctx, _task, _mpi_req, _info, _length, \
                              _buf, _dt) \
{ \
	(_req)->request           = _mpi_req;                                \
        (_req)->recv.tag.src_tid  = (_mpi_req)->header.source_task;          \
        (_req)->recv.tag.dest_tid = (_mpi_req)->header.destination_task;     \
        (_req)->recv.tag.dest_pid = (_mpi_req)->header.destination;          \
        (_req)->recv.tag.src_pid  = (_mpi_req)->header.source;               \
        (_req)->recv.tag.tag      = (_mpi_req)->header.message_tag;          \
        (_req)->recv.tag.comm     = (_mpi_req)->header.communicator_id;      \
	(_req)->info              = _info;                                   \
	(_req)->msg_id            = 0;                                       \
	(_req)->seqn              = 0;                                       \
	(_req)->ctx               = _ctx;                                    \
	(_req)->task              = _task;                                   \
	(_req)->datatype          = _dt;                                     \
	\
	(_req)->recv.buffer       = _buf;                                    \
	(_req)->recv.length       = _length;                                 \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

static inline void lcp_request_init_rma_put(lcp_request_t *req, 
                                            uint64_t remote_addr,
                                            lcp_mem_h rkey,
                                            lcp_request_param_t *param)
{
        req->send.rma.remote_addr = remote_addr;
        req->send.rma.rkey        = rkey; 
        req->send.cb              = param->cb;
        req->request              = param->request;
};

static inline int lcp_request_send(lcp_request_t *req)
{
        int rc;

        /* First, check endpoint availability */
        if (req->send.ep->state == LCP_EP_FLAG_CONNECTING) {
                lcp_ep_progress_conn(req->ctx, req->send.ep);
                if (req->send.ep->state == LCP_EP_FLAG_CONNECTING) {
                        mpc_queue_push(&req->ctx->pending_queue, &req->queue);
                        return MPC_LOWCOMM_SUCCESS;
                } else if (lcp_pending_get_request(req->ctx->pend, 
                                                   req->msg_id) != NULL) {
                        //FIXME: modify request progression managment.
                        req->flags |= ~LCP_REQUEST_NEED_PROGRESS;
                        // req->flags &= ~LCP_REQUEST_NEED_PROGRESS;
                        lcp_pending_delete(req->ctx->pend, req->msg_id);
                }
        }

        switch((rc = req->send.func(req))) {
        case MPC_LOWCOMM_SUCCESS:
                break;
        case MPC_LOWCOMM_NO_RESOURCE:
                mpc_queue_push(&req->ctx->pending_queue, &req->queue);
                break;
        case MPC_LOWCOMM_ERROR:
                mpc_common_debug_error("LCP: could not send request.");
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown send error.");
                break;
        }
        return rc;
}

int lcp_request_create(lcp_request_t **req_p);
int lcp_request_complete(lcp_request_t *req);
int lcp_request_init_unexp_ctnr(lcp_unexp_ctnr_t **ctnr_p, void *data, 
				size_t length, unsigned flags);

#endif

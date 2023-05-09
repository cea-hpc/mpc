#ifndef LCP_REQUEST_H
#define LCP_REQUEST_H

#include "lcp_types.h"

#include "lcp.h"
#include "lcp_ep.h"
#include "lcp_context.h"
#include "lcp_header.h" 
#include "lcp_pending.h"

#include "sctk_rail.h" 

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
};

enum {
	LCP_RECV_CONTAINER_UNEXP_RGET           = LCP_BIT(0),
	LCP_RECV_CONTAINER_UNEXP_RPUT           = LCP_BIT(1),
	LCP_RECV_CONTAINER_UNEXP_TAG            = LCP_BIT(2),
	LCP_RECV_CONTAINER_UNEXP_SM             = LCP_BIT(3),
	LCP_RECV_CONTAINER_UNEXP_TAG_SYNC       = LCP_BIT(4)
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
                                        uint64_t remote_addr;
                                        lcp_mem_h rkey;
                                } rma;
			};

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
                bmap_t           mem_map;
                size_t           offset;
                void *           pack_buf;
                size_t           remaining;
                lcp_chnl_idx_t   cc;
                lcr_completion_t comp;
                uint8_t          comp_stg;
                lcp_mem_h        lmem; 
                lcp_mem_h        rmem; 
                int              offloaded;
	} state;
};

#define LCP_REQUEST_INIT_SEND(_req, _ctx, _task, _mpi_req, _info, _length, \
                              _ep, _buf, _seqn, _msg_id, _dt) \
{ \
	(_req)->request         = _mpi_req; \
	(_req)->info            = _info;    \
	(_req)->msg_id          = _msg_id;  \
	(_req)->seqn            = _seqn;    \
	(_req)->ctx             = _ctx;     \
	(_req)->task            = _task;    \
	(_req)->datatype        = _dt;      \
	\
	(_req)->send.buffer     = _buf;     \
	(_req)->send.ep         = _ep;      \
	(_req)->send.length     = _length;  \
	\
	(_req)->state.cc        = (_ep)->priority_chnl; \
	(_req)->state.remaining = _length;              \
	(_req)->state.offset    = 0;                    \
}

#define LCP_REQUEST_INIT_RECV(_req, _ctx, _task, _mpi_req, _info, _length, \
                              _buf, _dt) \
{ \
	(_req)->request         = _mpi_req; \
	(_req)->info            = _info;    \
	(_req)->msg_id          = 0;        \
	(_req)->seqn            = 0;        \
	(_req)->ctx             = _ctx;     \
	(_req)->task            = _task;    \
	(_req)->datatype        = _dt;      \
	\
	(_req)->recv.buffer     = _buf;     \
	(_req)->recv.length     = _length;  \
	\
	(_req)->state.remaining = _length;  \
	(_req)->state.offset    = 0;        \
}

static inline void lcp_request_init_tag_send(lcp_request_t *req)
{
        req->send.tag.src_tid  = req->request->header.source_task;
        req->send.tag.dest_tid = req->request->header.destination_task;
        req->send.tag.dest_pid = req->request->header.destination;
        req->send.tag.src_pid  = req->request->header.source; 
        req->send.tag.tag      = req->request->header.message_tag;
        req->send.tag.comm     = req->request->header.communicator_id; 
};


static inline void lcp_request_init_tag_recv(lcp_request_t *req, lcp_tag_recv_info_t *info)
{
        req->recv.tag.src_tid  = req->request->header.source_task;
        req->recv.tag.dest_tid = req->request->header.destination_task;
        req->recv.tag.dest_pid = req->request->header.destination;
        req->recv.tag.src_pid  = req->request->header.source; 
        req->recv.tag.tag      = req->request->header.message_tag;
        req->recv.tag.comm     = req->request->header.communicator_id;
        req->info              = info;
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

static inline void lcp_request_init_ack(lcp_request_t *ack_req, lcp_ep_h ep, 
                                        int seqn, uint64_t msg_id)
{
        ack_req->send.ep            = ep;

        ack_req->msg_id             = msg_id;

        ack_req->state.remaining    = sizeof(lcp_ack_hdr_t);
        ack_req->state.offset       = 0; 
        ack_req->state.cc           = ep->priority_chnl;

	ack_req->msg_id             = msg_id;
	ack_req->seqn               = seqn;
}

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
                req->info->length = req->send.length;
                req->info->src    = req->send.tag.src_tid;
                req->info->tag    = req->send.tag.tag;
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

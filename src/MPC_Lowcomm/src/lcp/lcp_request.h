/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef LCP_REQUEST_H
#define LCP_REQUEST_H

#include "lcp_def.h"

#include "lcp.h"
//FIXME: try to remove lcp_ep.h. Currently needed to make progress on
//       endpoint connection.
#include "lcp_ep.h"
//FIXME: rail.h is needed here for lcr_tag_context_t struct in request.
//       See if we could use a pointer instead to avoid including rail.
#include "rail.h"
#include "lcp_context.h"
#include "lcp_task.h"
#include "lcp_header.h"

#include <sctk_alloc.h>

//TODO: LCP_REQUEST prefix confusing with those defined in lcp.h
enum {
	LCP_REQUEST_RECV_TRUNC          = LCP_BIT(1), /* flags if request is truncated */
        LCP_REQUEST_MPI_COMPLETE        = LCP_BIT(2), /* call MPI level completion callback,
                                                         see ucp_request_complete */
        LCP_REQUEST_RMA_COMPLETE        = LCP_BIT(3),
        LCP_REQUEST_OFFLOADED_RNDV      = LCP_BIT(4),
        LCP_REQUEST_LOCAL_COMPLETED     = LCP_BIT(5),
        LCP_REQUEST_REMOTE_COMPLETED    = LCP_BIT(6),
        LCP_REQUEST_USER_COMPLETE       = LCP_BIT(7),
};

//TODO: rename "unexpected container" to "receive descriptor". It was initially
//      used only for receiving unexpected data but it was then extended to
//      receive data for inter-thread communication.
enum {
	LCP_RECV_CONTAINER_UNEXP_RNDV_TAG       = LCP_BIT(0),
	LCP_RECV_CONTAINER_UNEXP_EAGER_TAG      = LCP_BIT(1),
	LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC = LCP_BIT(2),
	LCP_RECV_CONTAINER_UNEXP_RNDV_AM        = LCP_BIT(3),
	LCP_RECV_CONTAINER_UNEXP_TASK_TAG_BCOPY = LCP_BIT(4),
	LCP_RECV_CONTAINER_UNEXP_TASK_TAG_SYNC  = LCP_BIT(5),
	LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY = LCP_BIT(6),
};

/* Store data for unexpected am messages
 * Data is stored after (at lcp_unexp_cntr + 1)
 */
struct lcp_unexp_ctnr {
	ssize_t length;
	unsigned flags;
        mpc_queue_elem_t elem; /* element in the list of unexpected list */
};

typedef struct lcp_task_completion lcp_task_completion_t;
typedef void (*lcp_task_completion_callback_t)(lcp_task_completion_t *self);

//NOTE: ctnr member must be first;
struct lcp_task_completion {
        struct lcp_unexp_ctnr ctnr;
        lcp_tag_hdr_t  hdr;
        lcp_request_t *sreq; /* Send request */
        lcp_request_t *mreq; /* Matched request */
        void *buffer;        /* Address in case of bcopy */
        lcp_task_completion_callback_t comp_cb;
};

/* Generatic send function */
typedef int (*lcp_send_func_t)(lcp_request_t *req);

struct lcp_request {
        uint64_t       flags;
        lcp_context_h  ctx;
        lcp_task_h     task;
        lcp_datatype_t datatype;
        int is_sync;
        union {
                struct {
                        lcp_ep_h ep;
                        size_t length; /* Total length, in bytes */
			void *buffer;
			lcr_tag_context_t t_ctx;
			union {
				struct {
					uint16_t comm;
					uint64_t src_uid;
					uint64_t dest_uid;
                                        int32_t  src_tid;
                                        int32_t  dest_tid;
					int32_t  tag;
				} tag;

                                struct {
                                        uint8_t  am_id;
                                        void    *hdr;
                                        size_t   hdr_size;
                                        uint64_t src_uid;
                                        int32_t  dest_tid;
                                        lcp_am_completion_func_t on_completion;
                                        void    *request;
                                } am;

                                struct {
                                        uint64_t  addr;
                                        lcp_mem_h rkey;
                                } rndv;

                                struct {
                                        int32_t src_tid;
                                } ack;

                                struct {
                                        uint64_t remote_addr;
                                        lcp_mem_h rkey;
                                        /*
                                         * TODO: merge RMA callback with the AM callback
                                         * into a top-level struct field.
                                         * Requires code refactoring to have compatible
                                         * function prototypes.
                                         */
                                        lcp_rma_completion_func_t on_completion;
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

                        union {
                                struct {
                                        uint16_t comm;
                                        uint64_t src_uid;
                                        uint64_t dest_uid;
                                        int32_t  src_tid;
                                        int32_t  smask;
                                        int32_t  dest_tid;
                                        int32_t  tag;
                                        int32_t  tmask;
                                } tag;

                                struct {
                                        uint8_t  am_id;
                                        uint64_t src_uid;
                                        lcp_unexp_ctnr_t *ctnr;
                                        lcp_am_completion_func_t cb;
                                        void    *request;
                                } am;
                        };
		} recv;
	};

	//NOTE: break LCP modularity
	mpc_lowcomm_request_t *request;   /* Upper layer request */
        lcp_tag_info_t   *info;
        void                  *user_data;
	uint16_t                seqn;      /* Sequence number */
	uint64_t               msg_id;    /* Unique message identifier */
        struct lcp_request    *super;     /* master request */
        struct lcp_request    *rndv_req;  /* rndv request */ //FIXME: try avoid
                                                             //       having this req
        mpc_queue_elem_t       queue;     /* element in pending queue */
        mpc_queue_elem_t       match;     /* element in matching queue */

        struct {
                lcr_tag_t imm;
                int mid; /* matching id */
        } tm;

	struct {
                void *                pack_buf;
                size_t                offset;
                size_t                remaining;
                lcr_completion_t      comp;  /* Completion for rail send */
                lcp_task_completion_t tcomp; /* Completion for task send */
                lcp_mem_h             lmem;
                int                   offloaded;
	} state;
};

#define LCP_REQUEST_INIT_TAG_SEND(_req, _ctx, _task, _mpi_req, _info, _length, \
                                  _ep, _buf, _seqn, _dt, _is_sync) \
{ \
	(_req)->request           = _mpi_req;                                \
        (_req)->send.tag.src_tid  = (_mpi_req)->header.source_task;          \
        (_req)->send.tag.dest_tid = (_mpi_req)->header.destination_task;     \
        (_req)->send.tag.dest_uid = (_mpi_req)->header.destination;          \
        (_req)->send.tag.src_uid  = (_mpi_req)->header.source;               \
        (_req)->send.tag.tag      = (_mpi_req)->header.message_tag;          \
        (_req)->send.tag.comm     = (_mpi_req)->header.communicator_id;      \
	(_req)->info              = _info;                                   \
	(_req)->seqn              = _seqn;                                   \
	(_req)->ctx               = _ctx;                                    \
	(_req)->task              = _task;                                   \
	(_req)->msg_id            = (uint64_t)(_req);                        \
	(_req)->datatype          = _dt;                                     \
	(_req)->is_sync           = _is_sync;                                \
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
	(_req)->ctx               = _ctx;                                    \
	(_req)->task              = _task;                                   \
	(_req)->datatype          = _dt;                                     \
	\
	(_req)->recv.buffer       = _buf;                                    \
	(_req)->recv.length       = _length;                                 \
        (_req)->recv.tag.comm     = (_mpi_req)->header.communicator_id;      \
        (_req)->recv.tag.src_uid  = (_mpi_req)->header.source;               \
        (_req)->recv.tag.dest_uid = (_mpi_req)->header.destination;          \
        (_req)->recv.tag.src_tid  = (_mpi_req)->header.source_task;          \
        (_req)->recv.tag.dest_tid = (_mpi_req)->header.destination_task;     \
        (_req)->recv.tag.smask    = (_mpi_req)->header.source_task == MPC_ANY_SOURCE ? 0 : ~0; \
        (_req)->recv.tag.tag      = (_mpi_req)->header.message_tag;          \
        (_req)->recv.tag.tmask    = (_mpi_req)->header.message_tag == MPC_ANY_TAG ? 0 : ~0; \
        \
	(_req)->request           = _mpi_req;                                \
	(_req)->info              = _info;                                   \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

#define LCP_REQUEST_INIT_AM_SEND(_req, _ctx, _task, _am_id,                  \
                                 _src_uid, _dest_tid, _info, _length, _ep,   \
                                 _buf, _seqn, _msg_id, _dt, _is_sync)                  \
{ \
        (_req)->send.am.am_id     = _am_id;                                  \
	(_req)->info              = _info;                                   \
	(_req)->msg_id            = _msg_id;                                 \
	(_req)->seqn              = _seqn;                                   \
	(_req)->ctx               = _ctx;                                    \
	(_req)->task              = _task;                                   \
	(_req)->datatype          = _dt;                                     \
	(_req)->is_sync           = _is_sync;                                \
	\
	(_req)->send.buffer       = _buf;                                    \
	(_req)->send.ep           = _ep;                                     \
	(_req)->send.length       = _length;                                 \
	(_req)->send.am.hdr       = NULL;                                    \
	(_req)->send.am.hdr_size  = 0;                                       \
	(_req)->send.am.src_uid   = _src_uid;                                \
	(_req)->send.am.dest_tid  = _dest_tid;                               \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

#define LCP_REQUEST_INIT_AM_RECV(_req, _ctx, _task, _info, _length, _buf, _dt) \
{ \
	(_req)->info              = _info;                                   \
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

#define LCP_REQUEST_SET_MSGID(_msg_id, _tid, _seqn) \
        _msg_id  = 0;                               \
        _msg_id |= (_tid & 0xffffffffull);          \
        _msg_id  = (_msg_id << 32);                 \
        _msg_id |= (_seqn & 0xffffffffull);

#define lcp_request_get(_task) \
        ({ \
                lcp_request_t *__req = mpc_mpool_pop((_task)->req_mp); \
                if (__req != NULL) { \
                        lcp_request_reset(__req); \
                } \
                __req; \
         })

#define lcp_request_put(_req) \
        mpc_mpool_push(_req);

#define lcp_container_get(_task) \
        ({ \
                lcp_unexp_ctnr_t *__ctnr = mpc_mpool_pop((_task)->unexp_mp); \
                if (__ctnr != NULL) { \
                        lcp_container_reset(__ctnr); \
                } \
                __ctnr; \
         })

#define lcp_container_put(_ctnr) \
        mpc_mpool_push(_ctnr);

static inline void lcp_request_reset(lcp_request_t *req)
{
        req->flags = 0;
}

static inline void lcp_container_reset(lcp_unexp_ctnr_t *ctnr)
{
        ctnr->flags = 0;
}

static inline void lcp_request_init_rma_put(lcp_request_t *req,
                                            uint64_t remote_addr,
                                            lcp_mem_h rkey,
                                            const lcp_request_param_t *param)
{
        req->send.rma.remote_addr = remote_addr;
        req->send.rma.rkey        = rkey;
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
                        return LCP_SUCCESS;
                }
        }

        switch((rc = req->send.func(req))) {
        case LCP_SUCCESS:
                break;
        case MPC_LOWCOMM_NO_RESOURCE:
                //TODO: implement thread-safe pending queue
                mpc_queue_push(&req->ctx->pending_queue, &req->queue);
                break;
        case LCP_ERROR:
                mpc_common_debug_error("LCP: could not send request.");
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown send error.");
                break;
        }
        return rc;
}

void lcp_request_storage_init();
void lcp_request_storage_release();

//NOTE: we keep request creation call until we have more experience with it. It
//      has been replaced by lcp_request_get which uses a memory pool.
int lcp_request_create(lcp_request_t **req_p);
int lcp_request_complete(lcp_request_t *req);
int lcp_request_init_unexp_ctnr(lcp_task_h task, lcp_unexp_ctnr_t **ctnr_p, void *data,
				size_t length, unsigned flags);
void *lcp_request_get_unexp_ctnr_payload(lcp_unexp_ctnr_t *ctnr);

#endif

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

#include "lcp.h"
//FIXME: try to remove lcp_ep.h. Currently needed to make progress on
//       endpoint connection.
#include "lcp_ep.h"
//FIXME: rail.h is needed here for lcr_tag_context_t struct in request.
//       See if we could use a pointer instead to avoid including rail.
#include "rail.h"
#include "lcp_manager.h"
#include "lcp_header.h"
#include <mpc_common_bit.h>
#include <sctk_alloc.h>
#include "lcp_task.h"

//TODO: LCP_REQUEST prefix confusing with those defined in lcp.h
enum {
	LCP_REQUEST_RECV_TRUNC         = MPC_BIT(1), /* flags if request is truncated */
        LCP_REQUEST_MPI_COMPLETE       = MPC_BIT(2), /* call MPI level completion callback, 
                                                        see ucp_request_complete */
        LCP_REQUEST_RMA_COMPLETE       = MPC_BIT(3),
        LCP_REQUEST_OFFLOADED_RNDV     = MPC_BIT(4),
        LCP_REQUEST_LOCAL_COMPLETED    = MPC_BIT(5),
        LCP_REQUEST_REMOTE_COMPLETED   = MPC_BIT(6),
        LCP_REQUEST_USER_CALLBACK      = MPC_BIT(7),
        LCP_REQUEST_USER_PROVIDED_MEMH = MPC_BIT(8),
        LCP_REQUEST_USER_PROVIDED_EPH  = MPC_BIT(9),
        LCP_REQUEST_USER_ALLOCATED     = MPC_BIT(10),
        LCP_REQUEST_RELEASE_REMOTE_KEY = MPC_BIT(11),
};

//TODO: rename "unexpected container" to "receive descriptor". It was initially
//      used only for receiving unexpected data but it was then extended to
//      receive data for inter-thread communication.
enum {
	LCP_RECV_CONTAINER_UNEXP_RNDV_TAG       = MPC_BIT(0),
	LCP_RECV_CONTAINER_UNEXP_EAGER_TAG      = MPC_BIT(1),
	LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC = MPC_BIT(2),
	LCP_RECV_CONTAINER_UNEXP_RNDV_AM        = MPC_BIT(3),
	LCP_RECV_CONTAINER_UNEXP_TASK_TAG_BCOPY = MPC_BIT(4),
	LCP_RECV_CONTAINER_UNEXP_TASK_TAG_SYNC  = MPC_BIT(5),
	LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY = MPC_BIT(6),
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
        int            status;
        uint64_t       flags;
        lcp_manager_h  mngr;
        lcp_task_h     task; 
        lcp_datatype_t datatype;
        int is_sync;
        union {
                struct {
                        lcp_ep_h ep;
                        size_t length; /* Total length, in bytes */
			void *buffer; //FIXME: set to const
			lcr_tag_context_t t_ctx;
                        lcp_send_callback_func_t send_cb;
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
                                        int       is_get;
                                        uint64_t  remote_disp;
                                        lcp_mem_h rkey;
                                        void     *user_data;
                                } rma;

                                struct {
                                        uint64_t        remote_offset;
                                        lcp_atomic_op_t op;
                                        lcp_mem_h       rkey;
                                        void           *reply_buffer;
                                        uint64_t        value;
                                } ato;

                                struct {
                                        uint64_t flush;
                                } flush;
			};

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
                                        lcp_tag_recv_callback_func_t recv_cb;
                                        lcp_tag_recv_info_t info;
                                } tag;

                                struct {
                                        uint8_t  am_id;
                                        uint64_t src_uid;
                                        lcp_unexp_ctnr_t *ctnr;
                                        lcp_send_callback_func_t cb; //FIXME: rename
                                        void    *request;
                                } am;
                        };
		} recv;
	};

        void                  *request;   /* Pointer to upper layer request. */
        lcp_tag_recv_info_t    info;      /* Tag info transmitted to upper layer. */
	uint16_t               seqn;      /* Sequence number */
	uint64_t               msg_id;    /* Unique message identifier */
        struct lcp_request    *super;     /* master request */
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

#define LCP_REQUEST_INIT_TAG_SEND(_req, _mngr, _task, _request, _tag_info, _length, \
                                  _ep, _buf, _seqn, _dt, _is_sync) \
{ \
        (_req)->send.tag.src_tid  = (_tag_info)->src;                        \
        (_req)->send.tag.dest_tid = (_tag_info)->dest;                       \
        (_req)->send.tag.dest_uid = (_tag_info)->dest_uid;                   \
        (_req)->send.tag.src_uid  = (_tag_info)->src_uid;                    \
        (_req)->send.tag.tag      = (_tag_info)->tag;                        \
        (_req)->send.tag.comm     = (_tag_info)->comm_id;                    \
	(_req)->request           = _request;                                \
	(_req)->seqn              = _seqn;                                   \
	(_req)->status            = MPC_LOWCOMM_SUCCESS;                     \
	(_req)->mngr              = _mngr;                                   \
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

#define LCP_REQUEST_INIT_TAG_RECV(_req, _mngr, _task, _length, _request,      \
                                  _buf, _tag_info, _src_mask, _tag_mask, _dt) \
{ \
	(_req)->status            = MPC_LOWCOMM_SUCCESS;                     \
	(_req)->mngr              = _mngr;                                   \
	(_req)->task              = _task;                                   \
	(_req)->request           = _request;                                \
	(_req)->datatype          = _dt;                                     \
	\
	(_req)->recv.buffer       = _buf;                                    \
	(_req)->recv.length       = _length;                                 \
        (_req)->recv.tag.comm     = (_tag_info)->comm_id;                    \
        (_req)->recv.tag.src_uid  = (_tag_info)->src_uid;                    \
        (_req)->recv.tag.dest_uid = (_tag_info)->dest_uid;                   \
        (_req)->recv.tag.src_tid  = (_tag_info)->src;                        \
        (_req)->recv.tag.dest_tid = (_tag_info)->dest;                       \
        (_req)->recv.tag.tag      = (_tag_info)->tag;                        \
        (_req)->recv.tag.smask    = _src_mask;                               \
        (_req)->recv.tag.tmask    = _tag_mask;                               \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

#define LCP_REQUEST_INIT_AM_SEND(_req, _mngr, _task, _am_id, _request,       \
                                 _src_uid, _dest_tid, _length, _ep,          \
                                 _buf, _seqn, _msg_id, _dt, _is_sync)        \
{ \
        (_req)->send.am.am_id     = _am_id;                                  \
	(_req)->msg_id            = _msg_id;                                 \
	(_req)->seqn              = _seqn;                                   \
	(_req)->mngr              = _mngr;                                   \
	(_req)->task              = _task;                                   \
	(_req)->request           = _request;                                \
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

#define LCP_REQUEST_INIT_AM_RECV(_req, _mngr, _task, _length, _request, \
                                 _buf, _dt) \
{ \
	(_req)->mngr               = _mngr;                                  \
	(_req)->task              = _task;                                   \
	(_req)->datatype          = _dt;                                     \
	(_req)->request           = _request;                                \
	\
	(_req)->recv.buffer       = _buf;                                    \
	(_req)->recv.length       = _length;                                 \
	\
	(_req)->state.remaining   = _length;                                 \
	(_req)->state.offset      = 0;                                       \
}

#define LCP_REQUEST_INIT_RMA_PUT(_req, _mngr, _task, _length, _request, \
                                 _ep, _buf, _seqn, _msg_id, _dt, \
                                 _rkey, _remote_disp) \
{ \
	(_req)->msg_id               = _msg_id;                                 \
	(_req)->seqn                 = _seqn;                                   \
	(_req)->mngr                 = _mngr;                                    \
	(_req)->task                 = _task;                                   \
	(_req)->datatype             = _dt;                                     \
	(_req)->request              = _request;                                \
	\
	(_req)->send.buffer          = _buf;                                    \
	(_req)->send.ep              = _ep;                                     \
	(_req)->send.length          = _length;                                 \
	(_req)->send.rma.is_get      = 0;                                 \
	(_req)->send.rma.rkey        = _rkey;                                 \
	(_req)->send.rma.remote_disp = _remote_disp;                                 \
	\
	(_req)->state.remaining      = _length;                                 \
	(_req)->state.offset         = 0;                                       \
}

#define LCP_REQUEST_INIT_RMA_GET(_req, _mngr, _task, _length, _request, \
                                 _ep, _buf, _seqn, _msg_id, _dt, \
                                 _rkey, _remote_disp) \
{ \
	(_req)->msg_id               = _msg_id;                                 \
	(_req)->seqn                 = _seqn;                                   \
	(_req)->mngr                 = _mngr;                                   \
	(_req)->task                 = _task;                                   \
	(_req)->datatype             = _dt;                                     \
	(_req)->request              = _request;                                \
	\
	(_req)->send.buffer          = _buf;                                    \
	(_req)->send.ep              = _ep;                                     \
	(_req)->send.length          = _length;                                 \
	(_req)->send.rma.is_get      = 1;                                 \
	(_req)->send.rma.rkey        = _rkey;                                 \
	(_req)->send.rma.remote_disp = _remote_disp;                                 \
	\
	(_req)->state.remaining      = _length;                                 \
	(_req)->state.offset         = 0;                                       \
}

#define LCP_REQUEST_INIT_RMA_FLUSH(_req, _mngr, _task, _length, _request, \
                                   _ep, _buf, _seqn, _msg_id, _dt) \
{ \
	(_req)->msg_id               = _msg_id;                                 \
	(_req)->seqn                 = _seqn;                                   \
	(_req)->mngr                 = _mngr;                                   \
	(_req)->task                 = _task;                                   \
	(_req)->request              = _request;                                \
	(_req)->datatype             = _dt;                                     \
	\
	(_req)->send.buffer          = _buf;                                    \
	(_req)->send.ep              = _ep;                                     \
	(_req)->send.length          = _length;                                 \
	\
	(_req)->state.remaining      = _length;                                 \
	(_req)->state.offset         = 0;                                       \
}

#define LCP_REQUEST_INIT_ATO_SEND(_req, _mngr, _task, _length, _request, \
                                  _ep, _buf, _seqn, _msg_id, _dt, \
                                  _rkey, _remote_offset, _op) \
{ \
	(_req)->msg_id                 = _msg_id;        \
	(_req)->seqn                   = _seqn;          \
	(_req)->mngr                   = _mngr;          \
	(_req)->task                   = _task;          \
	(_req)->datatype               = _dt;            \
	(_req)->request                = _request;       \
	\
	(_req)->send.buffer            = _buf;           \
	(_req)->send.ep                = _ep;            \
	(_req)->send.length            = _length;        \
	(_req)->send.ato.rkey          = _rkey;          \
	(_req)->send.ato.remote_offset = _remote_offset; \
	(_req)->send.ato.op            = _op;            \
	\
	(_req)->state.remaining        = _length;        \
	(_req)->state.offset           = 0;              \
}

#define LCP_REQUEST_SET_MSGID(_msg_id, _tid, _seqn) \
        _msg_id  = 0;                               \
        _msg_id |= (_tid & 0xffffffffull);          \
        _msg_id  = (_msg_id << 32);                 \
        _msg_id |= (_seqn & 0xffffffffull);

#define lcp_request_get_param(_task, _param) \
        ({ \
                lcp_request_t *__req; \
                if ((_param)->field_mask & LCP_REQUEST_USER_REQUEST) {\
                        __req = (lcp_request_t *)((_param)->request) - 1; \
                } else { \
                        __req = mpc_mpool_pop(&(_task)->req_mp); \
                        if (__req != NULL) { \
                                lcp_request_reset(__req); \
                        } \
                } \
                __req; \
         }) 

#define lcp_request_get(_task) \
        ({ \
                lcp_request_t *__req = mpc_mpool_pop(&(_task)->req_mp); \
                if (__req != NULL) { \
                        lcp_request_reset(__req); \
                } \
                __req; \
         })

#define lcp_request_put(_req) \
        mpc_mpool_push(_req);

#define lcp_request_complete(_req, _cb, _status, ...) \
        mpc_common_debug("LCP: complete req=%p, msg_id=%llu, " \
                         "seqn=%d", _req, (_req)->msg_id, \
                         (_req)->seqn); \
        if ((_req)->flags & LCP_REQUEST_USER_CALLBACK) { \
                (_req)->_cb(_status, (_req)->request, ## __VA_ARGS__); \
        }\
        if (!((_req)->flags & LCP_REQUEST_USER_ALLOCATED)) {\
                lcp_request_put(_req); \
        }

#define lcp_container_get(_task) \
        ({ \
                lcp_unexp_ctnr_t *__ctnr = mpc_mpool_pop(&(_task)->unexp_mp); \
                if (__ctnr != NULL) { \
                        lcp_container_reset(__ctnr); \
                } \
                __ctnr; \
         })

#define lcp_container_put(_ctnr) \
        mpc_mpool_push(_ctnr);

// NOLINTBEGIN(clang-diagnostic-unused-function)

static inline void lcp_request_reset(lcp_request_t *req)
{
        req->flags = 0;
}

static inline void lcp_container_reset(lcp_unexp_ctnr_t *ctnr)
{
        ctnr->flags = 0;
}

static inline int lcp_request_send(lcp_request_t *req)
{
        int rc;

        /* First, check endpoint availability */
        if (req->send.ep->state == LCP_EP_FLAG_CONNECTING) {
                lcp_ep_progress_conn(req->mngr, req->send.ep);
                if (req->send.ep->state == LCP_EP_FLAG_CONNECTING) {
                        mpc_queue_push(&req->mngr->pending_queue, &req->queue);
                        return LCP_SUCCESS;
                }
        }

        switch((rc = req->send.func(req))) {
        case LCP_SUCCESS:
                break;
        case MPC_LOWCOMM_NO_RESOURCE:
                //TODO: implement thread-safe pending queue
                mpc_queue_push(&req->mngr->pending_queue, &req->queue);
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

// NOLINTEND(clang-diagnostic-unused-function)

//FIXME: remove?
void lcp_request_storage_init();
//FIXME: remove?
void lcp_request_storage_release();

int lcp_request_init_unexp_ctnr(lcp_task_h task, lcp_unexp_ctnr_t **ctnr_p, void *data, 
				size_t length, unsigned flags);

#endif

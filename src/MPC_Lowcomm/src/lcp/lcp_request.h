#ifndef LCP_REQUEST_H
#define LCP_REQUEST_H

#include "lcp_types.h"

#include "lcp.h"
#include "lcp_ep.h"
#include "lcp_header.h" 

#include "sctk_rail.h" 

#include <sctk_alloc.h>

enum {
	LCP_REQUEST_SEND_TAG           = LCP_BIT(0),
	LCP_REQUEST_SEND_FRAG          = LCP_BIT(1),
	LCP_REQUEST_RECV_FRAG          = LCP_BIT(2),
	LCP_REQUEST_SEND_CTRL          = LCP_BIT(3),
	LCP_REQUEST_RECV_TRUNC         = LCP_BIT(4),
        LCP_REQUEST_MPI_COMPLETE       = LCP_BIT(5),
        LCP_REQUEST_OFFLOADED          = LCP_BIT(6)
};

enum {
	LCP_RECV_CONTAINER_UNEXP_RNDV = LCP_BIT(0),
	LCP_RECV_CONTAINER_UNEXP_TAG  = LCP_BIT(1)
};


typedef enum {
	MPC_LOWCOMM_LCP_TAG = 0, /* request send eager */
	MPC_LOWCOMM_LCP_PEND, /* request pending */
	MPC_LOWCOMM_LCP_RPUT_SYNC, /* waiting ack from rput protocol*/
	MPC_LOWCOMM_LCP_RPUT_FRAG, /* fragmentation in progress */
	MPC_LOWCOMM_LCP_DONE
} lcp_request_status;

/* Store data for unexpected messages
 * Data is stored after (at lcp_unexp_cntr + 1)
 */
struct lcp_unexp_ctnr {
	size_t length;
	unsigned flags;
};

typedef int (*lcp_send_func_t)(lcp_request_t *req);

struct lcp_request {
	uint64_t flags;
	union {
		struct {
			lcp_ep_h ep;
			size_t length; /* Total length, in bytes */
			void *buffer;
                        lcp_context_h ctx;       //NOTE: needed by lcp_request_complete
			lcr_tag_context_t t_ctx;
			lcp_chnl_idx_t cc;

			union {
				struct {
					uint64_t comm_id;
					uint64_t src;
					int32_t src_tsk;
					uint64_t dest;
					int32_t dest_tsk;
					int tag;
					int dt;
				} tag;

				struct {
					uint64_t comm_id;
					uint8_t  am_id;
					uint64_t dest;
				} am;
			};

			lcp_send_func_t func;
		} send;

		struct {
			size_t length;
                        size_t send_length;
			lcp_chnl_idx_t cc;
			void *buffer;
                        lcp_context_h ctx;       //NOTE: needed by lcp_request_complete
			lcr_tag_context_t t_ctx;

			struct {
				uint64_t comm_id;
				uint64_t src;
				int32_t src_tsk;
				uint64_t dest;
				int32_t dest_tsk;
				int tag;
				int dt;
			} tag;
			
		} recv;
	};

	mpc_lowcomm_request_t *request; /* Upper layer request */
	int64_t msg_number;
	uint64_t msg_id;

        struct {
                uint64_t comm_id;
                lcp_context_h ctx;
                lcr_tag_context_t t_ctx;
        } tm;

	struct {
		lcp_request_status status;
		size_t remaining;
		size_t offset;
		int f_id;
		lcp_chnl_idx_t cur;
		lcr_completion_t comp;
	} state;
};

static inline void lcp_request_init_send(lcp_request_t *req, lcp_ep_h ep,
					 mpc_lowcomm_request_t *request,
					 void *buffer, int64_t seqn, 
					 uint64_t msg_id)
{
        req->send.tag.dest      = request->header.destination;
        req->send.tag.dest_tsk  = request->header.destination_task; 
        req->send.tag.src       = request->header.source; 
        req->send.tag.src_tsk   = request->header.source_task;	
        req->send.tag.tag       = request->header.message_tag;
        req->send.tag.comm_id   = request->header.communicator_id; 
        req->send.length        = request->header.msg_size; 
        req->send.buffer        = buffer;
	req->send.ep            = ep;
	req->send.cc            = ep->priority_chnl;
	req->send.ctx           = ep->ctx; 

        req->state.remaining    = request->header.msg_size; 
        req->state.offset       = 0; 
        req->state.f_id         = 0; 

        req->request            = request;
	req->msg_id             = msg_id;
	req->msg_number         = seqn;
        req->tm.comm_id         = request->header.communicator_id;
};

static inline void lcp_request_init_ack(lcp_request_t *ack_req, lcp_ep_h ep, 
                                        uint64_t comm_id, uint64_t dest, 
                                        int seqn, uint64_t msg_id)
{
        ack_req->send.am.am_id      = MPC_LOWCOMM_ACK_MESSAGE;
        ack_req->send.am.comm_id    = comm_id;
        ack_req->send.am.dest       = dest;
        ack_req->send.ep            = ep;
        ack_req->send.cc            = ep->priority_chnl;

        ack_req->msg_id             = msg_id;

        ack_req->state.remaining    = sizeof(lcp_rndv_ack_hdr_t);
        ack_req->state.offset       = 0; 
        ack_req->state.f_id         = 0; 

	ack_req->msg_id             = msg_id;
	ack_req->msg_number         = seqn;
}

static inline void lcp_request_init_recv(lcp_request_t *req, 
					mpc_lowcomm_request_t *request,
                                        lcp_context_h ctx,
					void *buffer, size_t length, 
					uint64_t msg_id)
{
        req->recv.tag.dest      = request->header.destination;
        req->recv.tag.dest_tsk  = request->header.destination_task;
        req->recv.tag.src       = request->header.source;     
        req->recv.tag.src_tsk   = request->header.source_task;
        req->recv.tag.tag       = request->header.message_tag;
        req->recv.tag.comm_id   = request->header.communicator_id;
        req->recv.length        = request->header.msg_size; 
        req->recv.ctx           = ctx;
        req->state.remaining    = request->header.msg_size; 
        req->state.offset       = 0; 
        req->state.f_id         = 0; 
        req->recv.buffer        = buffer;
        req->recv.length        = length;
        req->request            = request; 

	req->msg_id             = msg_id;
        req->tm.comm_id         = request->header.communicator_id;
}

int lcp_request_create(lcp_request_t **req_p);
void lcp_request_update_state(lcp_request_t *req, size_t length);
int lcp_request_complete(lcp_request_t *req);

int lcp_request_init_unexp_ctnr(lcp_unexp_ctnr_t **ctnr_p, void *data, 
				size_t length, unsigned flags);

#endif

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

static size_t lcp_proto_rndv_pack(void *dest, void *data)
{
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.rndv.comm_id;
	hdr->base.dest         = req->send.rndv.dest;
	hdr->base.src          = req->send.rndv.src;
	hdr->base.tag          = req->send.rndv.tag;
	hdr->base.seqn         = req->msg_number;
	
	hdr->dest        = req->send.rndv.dest;
	hdr->msg_id      = req->msg_id;
	hdr->total_size  = req->send.length;
        hdr->remote_addr = 0; //FIXME: check how to differenciate with rndv put

	return sizeof(*hdr);
}

static size_t lcp_proto_ack_pack(void *dest, void *data)
{
	lcp_rndv_ack_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.am.comm_id;
	hdr->base.am_id = req->send.am.am_id;
	
	hdr->dest        = req->send.am.dest;
	hdr->msg_id      = req->msg_id;

	return sizeof(*hdr);
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

//NOTE: completion callback are used:
//      1) for send zcopy (tag-matching and no tag-matching)
//      2) for send bcopy (tag-matching)
//      But not used for recv since handler is called and will
//      handle request completion. 
void lcp_request_complete_rndv(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

	//NOTE: completion may be called after ack is received 
	//      overriding thus MPC_LOWCOMM_LCP_RPUT_FRAG state.
	//      Thus it blocks the rendez-vous in SYNC state and 
	//      deadlock.
	//      Therefore, there is nothing to be done here.
        mpc_common_debug("LCP: rndv request completed. req=%p, msg_id=%llu",
			req, req->msg_id);
}

void lcp_request_complete_ack(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        req->state.status = MPC_LOWCOMM_LCP_DONE;
	lcp_request_complete(req);
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

int lcp_send_am_eager_rndv_bcopy(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_rndv 
	};
	req->send.t_ctx.comp = cp;

	payload = lcp_send_do_am_bcopy(lcr_ep, 
			MPC_LOWCOMM_RDV_MESSAGE, 
			lcp_proto_rndv_pack, req);
	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}

	return rc;
}

int lcp_send_am_eager_ack_bcopy(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	int payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	payload = lcp_send_do_am_bcopy(lcr_ep,
			MPC_LOWCOMM_ACK_RDV_MESSAGE,
			lcp_proto_ack_pack, req);
	if (payload == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug_error("LCP: no resource available.");
		rc = MPC_LOWCOMM_NO_RESOURCE;
	} else if ((unsigned long)payload < sizeof(lcp_rndv_ack_hdr_t)) {
		mpc_common_debug_error("LCP: could not pack all data.");
		rc = MPC_LOWCOMM_ERROR;
	} 

	return rc;
}

void lcp_request_complete_rsend(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_complete(req);
        } 
}

void lcp_request_complete_rput_final(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        lcp_request_complete(req);
}

void lcp_request_complete_rput(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_mem_deregister(req->send.ep->ctx, req->send.rndv.lmem);

                lcp_ep_h ep;
                struct iovec iov_fin[1];

                iov_fin[0].iov_base = NULL;
                iov_fin[0].iov_len = 0; 

                ep = req->send.ep;
                req->send.t_ctx.comp.comp_cb = lcp_request_complete_rput_final; 

                lcr_tag_t tag = { 0 };
                LCP_TM_SET_MATCHBITS(tag.t, 
                                     req->send.rndv.src, 
                                     req->send.rndv.tag, 
                                     req->msg_number);

                lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                      tag,
                                      0,
                                      iov_fin,
                                      1,
                                      &(req->send.t_ctx));
        }

        return;
}

void lcp_request_rput_ack_callback(lcr_completion_t *comp)
{
        int rc, f_id;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_mem_h rmem; 
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
	_mpc_lowcomm_endpoint_t *lcr_ep;

        lcp_request_t *ack_req = mpc_container_of(comp, lcp_request_t, 
                                                  recv.t_ctx.comp);

        lcp_request_t *super = ack_req->super;

        /* first create and unpack remote key */
        rc = lcp_mem_create(super->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        //NOTE: we use here pointer from tag interface, could be either from
        //      overflow or priority.
        rc = lcp_mem_unpack(super->ctx, rmem, ack_req->recv.t_ctx.start);
        if (rc < 0) {
                goto err;
        }
        sctk_free(ack_req->recv.buffer);
        lcp_request_complete(ack_req);

        ep = super->send.ep;

        super->send.t_ctx.comp.comp_cb = lcp_request_complete_rput;

        remaining   = super->send.length;
        f_id        = 0;
        offset      = 0;
	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;
		length = remaining < frag_length ? remaining : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, remaining=%llu, "
				 "len=%d", f_id, super->send.tag.src_tsk, 
				 super->send.tag.dest_tsk, super->msg_id, remaining, length,
				 remaining);

                rc = lcp_send_do_put_zcopy(lcr_ep,
                                           offset, 
                                           offset, 
                                           &(super->send.rndv.lmem->mems[ep->current_chnl]),
                                           &(rmem->mems[ep->current_chnl]),
                                           length,
                                           &(super->send.t_ctx.comp));

                offset    += length; remaining -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
        }
err:
        return;

}

void lcp_request_rput_callback(lcr_completion_t *comp)
{
        int rc;
        lcp_request_t *ack;
        lcp_ep_h ep;
        struct iovec iov_ack[1];
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);
        lcr_memp_t fake_mem[req->ctx->num_resources];

        rc = lcp_mem_register(req->ctx, 
                              &(req->send.rndv.lmem), 
                              req->send.buffer, 
                              req->send.length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        ep = req->send.ep;

        lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super = req;
        ack->recv.length = lcp_mem_pack(req->ctx, (void *)fake_mem, req->send.rndv.lmem);
        ack->recv.buffer = sctk_malloc(ack->recv.length);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->msg_number);

        lcr_tag_t ign_tag = {
                .t = 0
        };

        iov_ack[0].iov_base = ack->recv.buffer;
        iov_ack[0].iov_len  = ack->recv.length,

        ack->recv.t_ctx.comm_id      = req->send.rndv.comm_id;
        ack->recv.t_ctx.req          = ack;
        ack->recv.t_ctx.comp.comp_cb = lcp_request_rput_ack_callback;

        rc = lcp_recv_do_tag_zcopy(ep->lct_eps[ep->priority_chnl]->rail,
                                   tag,
                                   ign_tag,
                                   iov_ack,
                                   1,
                                   &(ack->recv.t_ctx));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
err:
        return;
}

int lcp_send_rsend_start(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_RSEND_TM_MESSAGE, req->send.length);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->msg_number);

        struct iovec iov[2];
        iov[0].iov_base = NULL;
        iov[0].iov_len  = 0;
        iov[1].iov_base = req->send.buffer;
        iov[1].iov_len  = req->send.length;

        req->send.t_ctx.req          = req;
        req->send.t_ctx.comm_id      = req->send.rndv.comm_id;
        req->send.t_ctx.comp.comp_cb = lcp_request_complete_rsend;

        rc = lcp_send_do_tag_rndv_zcopy(ep->lct_eps[ep->priority_chnl],
                                        tag,
                                        imm,
                                        iov,
                                        2,
                                        &(req->send.t_ctx));
        return rc;
}

void lcp_request_complete_srget(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        if (req->send.rndv.ack) {
                lcp_mem_deregister(req->ctx, req->send.rndv.lmem);
                lcp_request_complete(req);
        } else {
                req->send.rndv.ack = 1;
        }
}

int lcp_send_rget_start(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;
        uint64_t imm = 0;
        struct iovec iov_send[1], iov_fin[1];
        lcr_memp_t fake_mem[req->ctx->num_resources];

        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_RGET_TM_MESSAGE, req->send.length);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->msg_number);

        rc = lcp_mem_register(req->send.ep->ctx, 
                              &(req->send.rndv.lmem), 
                              req->send.buffer, 
                              req->send.length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        iov_send[0].iov_base = sctk_malloc(sizeof(fake_mem));
        iov_send[0].iov_len  = lcp_mem_pack(req->send.ep->ctx, iov_send[0].iov_base, req->send.rndv.lmem);

        lcr_tag_t ign_tag = {
                .t = 0
        };

        iov_fin[0].iov_base = NULL;
        iov_fin[0].iov_len = 0; 

        req->send.t_ctx.req          = req;
        req->send.t_ctx.comm_id      = req->send.rndv.comm_id;
        req->send.t_ctx.comp.comp_cb = lcp_request_complete_srget;

        rc = lcp_recv_do_tag_zcopy(ep->lct_eps[ep->priority_chnl]->rail,
                                   tag,
                                   ign_tag,
                                   iov_fin,
                                   1,
                                   &(req->send.t_ctx));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->send.rndv.ack = 0;
        rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                   tag,
                                   imm,
                                   iov_send,
                                   1,
                                   &(req->send.t_ctx));

err:
        return rc;
}

int lcp_send_rput_start(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;
        uint64_t imm = 0;
        struct iovec iov_send[1];

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->msg_number);

        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_RPUT_TM_MESSAGE, req->send.length);

        iov_send[0].iov_base = NULL;
        iov_send[0].iov_len  = 0;

        req->send.t_ctx.req          = req;
        req->send.t_ctx.comp.comp_cb = lcp_request_rput_callback;
        req->send.t_ctx.comm_id      = req->send.rndv.comm_id;

        rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                   tag,
                                   imm,
                                   iov_send,
                                   1,
                                   &(req->send.t_ctx));

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
err:
        return rc;
}

int lcp_send_rndv_start(lcp_request_t *req) 
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_ep_h ep = req->send.ep;
	lcp_chnl_idx_t cc = ep->priority_chnl;

	mpc_common_debug_info("LCP: send rndv req=%p, comm_id=%lu, tag=%d, "
			      "src=%d, dest=%d, msg_id=%d.", req, req->send.rndv.comm_id, 
			      req->send.rndv.tag, req->send.rndv.src, 
			      req->send.rndv.dest, req->msg_id);

	if (LCR_IFACE_IS_TM(ep->lct_eps[cc]->rail)) {
                lcr_rail_attr_t attr; 
                ep->lct_eps[cc]->rail->iface_get_attr(ep->lct_eps[cc]->rail, &attr);

                switch(ep->ctx->config.rndv_mode) {
                case LCP_RNDV_SEND:
                        req->send.func = lcp_send_rsend_start;
                        break;
                case LCP_RNDV_GET:
                        req->send.func = lcp_send_rget_start;
                        break;
                case LCP_RNDV_PUT:
                        req->send.func = lcp_send_rput_start;
                        break;
                default:
                        mpc_common_debug_fatal("LCP: unknown protocol");
                } 	
        } else {
		req->send.func = lcp_send_am_eager_rndv_bcopy;

                if (lcp_pending_create(ep->ctx->pend_send_req, req, 
                                       req->msg_id) == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
	}

err:
	return rc;
}

int lcp_rndv_matched(lcp_context_h ctx, 
		lcp_request_t *rreq,
		lcp_rndv_hdr_t *hdr) 
{
	int rc;
	lcp_ep_ctx_t *ctx_ep;
	lcp_ep_h ep;
	lcp_request_t *ack_req;

	/* Init all protocol data */
	rreq->msg_id            = hdr->msg_id;
	rreq->recv.send_length  = hdr->total_size; 
        rreq->state.remaining   = hdr->total_size;
	rreq->state.offset      = 0;
	rreq->msg_number        = hdr->base.seqn;
	rreq->state.status      = MPC_LOWCOMM_LCP_RPUT_FRAG;

	/* Get LCP endpoint if exists */
	HASH_FIND(hh, ctx->ep_ht, &hdr->base.src, sizeof(uint64_t), ctx_ep);
	if (ctx_ep == NULL) {
		rc = lcp_ep_create(ctx, &ep, hdr->base.src, 0);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not create ep after match.");
			goto err;
		}
	} else {
		ep = ctx_ep->ep;
	}

        if (rreq->flags & LCP_REQUEST_RNDV_TAG) {
                /* Send ack */
                rc = lcp_request_create(&ack_req);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                lcp_request_init_ack(ack_req, ep, 
                                     hdr->base.base.comm_id, 
                                     hdr->base.dest,
                                     hdr->base.seqn,
                                     hdr->msg_id);

                mpc_common_debug("LCP: send ack comm_id=%llu, dest=%d, msg_id=%llu",
                                 ack_req->send.am.comm_id, ack_req->send.am.dest, 
                                 ack_req->msg_id);
                rc = lcp_send_am_eager_ack_bcopy(ack_req);
        }

err:
	return rc;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
void lcp_request_complete_rrsend(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      recv.t_ctx.comp);

        lcp_mem_deregister(req->ctx, req->state.lmem);

        lcp_request_complete(req);
}

void lcp_request_complete_rget_final(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      recv.t_ctx.comp);

        lcp_request_complete(req);
}

void lcp_request_complete_rrget(lcr_completion_t *comp)
{
        int rc;
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      recv.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_mem_deregister(req->ctx, req->state.lmem);

                lcp_ep_h ep;
                lcp_ep_ctx_t *ctx_ep;
                struct iovec iov_fin[1];

                iov_fin[0].iov_base = NULL;
                iov_fin[0].iov_len = 0; 
                /* Get LCP endpoint if exists */
                HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
                assert(ctx_ep);

                ep = ctx_ep->ep;
                req->recv.t_ctx.comp.comp_cb = lcp_request_complete_rget_final; //FIXME: should we also complete fin ?

                rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                           req->recv.t_ctx.tag,
                                           0,
                                           iov_fin,
                                           1,
                                           &(req->recv.t_ctx));
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }
err:
        return;
}

void lcp_request_complete_rrput(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      recv.t_ctx.comp);

        lcp_mem_deregister(req->ctx, req->state.lmem);

        lcp_request_complete(req);
}

void lcp_request_complete_rrack(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      recv.t_ctx.comp);

        lcp_request_complete(req);
}

void lcp_request_complete_srack(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        lcp_request_complete(req);
}

//TODO: use lcp_request_send(req)
int lcp_recv_rsend(lcp_request_t *req, void *hdr)
{
        int rc;
        lcp_mem_h rmem; 
        lcp_ep_h ep;
	lcp_ep_ctx_t *ctx_ep;

	/* Get LCP endpoint if exists */
	HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
	if (ctx_ep == NULL) {
		rc = lcp_ep_create(req->ctx, &ep, req->recv.tag.src, 0);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not create ep after match.");
			goto err;
		}
	} else {
		ep = ctx_ep->ep;
	}


        /* first create and unpack remote key */
        rc = lcp_mem_create(req->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        rc = lcp_mem_unpack(req->ctx, rmem, hdr);
        if (rc < 0) {
                goto err;
        }

        rc = lcp_mem_register(req->ctx, 
                              &(req->state.lmem), 
                              req->recv.buffer, 
                              req->recv.send_length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->recv.t_ctx.comp.comp_cb = lcp_request_complete_rrsend;
        rc = lcp_send_do_get_zcopy(ep->lct_eps[ep->priority_chnl],
                                   0,
                                   0,
                                   &req->state.lmem->mems[ep->priority_chnl],
                                   &rmem->mems[ep->priority_chnl],
                                   req->recv.send_length,
                                   &(req->recv.t_ctx.comp));

err:
        return rc;
}


int lcp_recv_rget(lcp_request_t *req, void *hdr)
{
        int rc, f_id, i;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_mem_h rmem; 
        lcp_ep_h ep;
	lcp_ep_ctx_t *ctx_ep;
        lcr_rail_attr_t attr;
	_mpc_lowcomm_endpoint_t *lcr_ep;

	/* Get LCP endpoint if exists */
	HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
	if (ctx_ep == NULL) {
		rc = lcp_ep_create(req->ctx, &ep, req->recv.tag.src, 0);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not create ep after match.");
			goto err;
		}
	} else {
		ep = ctx_ep->ep;
	}

        /* first create and unpack remote key */
        rc = lcp_mem_create(req->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_mem_unpack(req->ctx, rmem, hdr);
        if (rc < 0) {
                goto err;
        }

        rc = lcp_mem_register(req->ctx, 
                              &(req->state.lmem), 
                              req->recv.buffer, 
                              req->recv.send_length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->recv.t_ctx.comp.comp_cb = lcp_request_complete_rrget;

        req->state.remaining = remaining = req->recv.send_length;
        f_id                 = 0;
        offset               = 0;

        per_ep_length        = sctk_malloc(ep->num_chnls * sizeof(size_t));
        for (i=0; i<ep->num_chnls; i++) {
                per_ep_length[i] = (size_t)req->recv.send_length / ep->num_chnls;
        }
        per_ep_length[0] += req->recv.send_length % ep->num_chnls;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;
		length = per_ep_length[ep->current_chnl] < frag_length ? 
                        per_ep_length[ep->current_chnl] : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, remaining=%llu", 
                                 f_id, req->send.tag.src_tsk, req->send.tag.dest_tsk, req->msg_id, 
                                 remaining, length);

                rc = lcp_send_do_get_zcopy(lcr_ep,
                                           offset, 
                                           offset, 
                                           &(req->state.lmem->mems[ep->current_chnl]),
                                           &(rmem->mems[ep->current_chnl]),
                                           length,
                                           &(req->recv.t_ctx.comp));

                per_ep_length[ep->current_chnl] -= length;
                offset    += length; remaining  -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
        }

        sctk_free(per_ep_length);
err:
        return rc;
}

int lcp_recv_rput(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep;
	lcp_ep_ctx_t *ctx_ep;
        lcp_request_t *ack;
        struct iovec iov_ack[1];
        struct iovec iov_fin[1];
        lcr_memp_t max_mem[req->ctx->num_resources];

	HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
	if (ctx_ep == NULL) {
		rc = lcp_ep_create(req->ctx, &ep, req->recv.tag.src, 0);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not create ep after match.");
			goto err;
		}
	} else {
		ep = ctx_ep->ep;
	}

        rc = lcp_mem_register(req->ctx, 
                              &(req->state.lmem), 
                              req->recv.buffer, 
                              req->recv.send_length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super = req;
        ack->send.buffer = sctk_malloc(sizeof(max_mem));
        ack->send.length = lcp_mem_pack(ep->ctx, ack->send.buffer, req->state.lmem);
        ack->msg_number  = req->msg_number;

        iov_ack[0].iov_base = ack->send.buffer;
        iov_ack[0].iov_len  = ack->send.length,

        ack->send.t_ctx.comm_id      = req->recv.tag.comm_id;
        ack->send.t_ctx.req          = ack;
        ack->send.t_ctx.comp.comp_cb = lcp_request_complete_srack; 

        rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                   req->recv.t_ctx.tag,
                                   0,
                                   iov_ack,
                                   1,
                                   &(ack->send.t_ctx));

        lcr_tag_t ign_tag = {
                .t = 0
        };

        iov_fin[0].iov_base = NULL;
        iov_fin[0].iov_len = 0; 

        req->recv.t_ctx.comp.comp_cb = lcp_request_complete_rrput;

        rc = lcp_recv_do_tag_zcopy(ep->lct_eps[ep->priority_chnl]->rail,
                                   req->recv.t_ctx.tag,
                                   ign_tag,
                                   iov_fin,
                                   1,
                                   &(req->recv.t_ctx));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
err:
        return rc;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

//NOTE: flags could be used to avoid duplicating functions for
//      AM and TAG
int lcp_rndv_am_tag_handler(void *arg, void *data,
                            size_t length, 
                            __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_rndv_hdr_t *hdr = data;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;

	/* No payload in rndv message for now */
	assert(length == sizeof(lcp_rndv_hdr_t));

	LCP_CONTEXT_LOCK(ctx);

	mpc_common_debug_info("LCP: recv rndv header src=%d, dest=%d, msg_id=%llu",
			      hdr->base.src_tsk, hdr->base.dest_tsk, hdr->msg_id);

	req = lcp_match_prq(ctx->prq_table, 
			    hdr->base.base.comm_id, 
			    hdr->base.tag, hdr->base.src);
	if (req != NULL) {
		mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
				 "tag=%d, src=%lu.", req, req->recv.tag.comm_id, 
				 req->recv.tag.tag, req->recv.tag.src);
		LCP_CONTEXT_UNLOCK(ctx); //NOTE: unlock context to enable endpoint creation.
		rc = lcp_rndv_matched(ctx, req, hdr);
	} else {
		lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
					    LCP_RECV_CONTAINER_UNEXP_RNDV);
		lcp_append_umq(ctx->umq_table, (void *)ctnr, hdr->base.base.comm_id, 
			       hdr->base.tag, hdr->base.src);
		LCP_CONTEXT_UNLOCK(ctx);
		rc = MPC_LOWCOMM_SUCCESS;
	}


	return rc;
}

static int lcp_ack_am_tag_handler(void *arg, void *data,
				__UNUSED__ size_t size, 
				__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_rndv_ack_hdr_t *hdr = data;
	lcp_request_t *req;

	mpc_common_debug_info("LCP: recv tag ack header comm_id=%llu, src=%d, dest=%d, "
			      "msg_id=%llu.", hdr->base.base.comm_id, hdr->src, hdr->dest, 
			      hdr->msg_id);

	/* Retrieve request */
        req = lcp_pending_get_request(ctx->pend_send_req, hdr->msg_id);
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not find ctrl msg: comm id=%llu, "
				       "msg id=%llu, src=%d, dest=%d.", 
				       hdr->base.base.comm_id, hdr->msg_id,
				       hdr->src, hdr->dest);
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Set send function that will be called in lcp_progress */
	req->send.func    = lcp_send_am_zcopy_multi;

	/* Update request state */
	req->state.status = MPC_LOWCOMM_LCP_RPUT_FRAG;
err:
	return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_ACK_RDV_MESSAGE, lcp_ack_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_MESSAGE, lcp_rndv_am_tag_handler, 0);

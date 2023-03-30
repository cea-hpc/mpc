#include "lcp.h"

#include "lcp_context.h"
#include "lcp_task.h"
#include "lcp_def.h"
#include "lcp_prototypes.h"
#include "lcp_request.h"
#include "lcp_mem.h"

#include "sctk_alloc.h"

size_t lcp_rma_put_pack(void *dest, void *arg) {
        lcp_request_t *req = (lcp_request_t *)arg;

        memcpy(dest, req->send.buffer, req->send.length);

        return req->send.length;
}

static inline int build_rma_memory_registration_bitmap(size_t length, 
                                                       size_t min_frag_size, 
                                                       int max_iface,
                                                       bmap_t *bmap_p)
{
        bmap_t bmap = MPC_BITMAP_INIT;
        int num_used_ifaces = 0;

        while ((length > num_used_ifaces * min_frag_size) &&
               (num_used_ifaces < max_iface)) {
                MPC_BITMAP_SET(bmap, num_used_ifaces);
                num_used_ifaces++;
        }

        *bmap_p = bmap;

        return num_used_ifaces;
}

int lcp_rma_reg_send_buffer(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;
        sctk_rail_info_t *rail = ep->lct_eps[ep->priority_chnl]->rail;
        lcr_rail_attr_t attr;

        /* build registration bitmap */
        rail->iface_get_attr(rail, &attr);
        build_rma_memory_registration_bitmap(req->send.length, 
                                             attr.iface.cap.rma.min_frag_size,
                                             req->ctx->num_resources,
                                             &req->state.mem_map);

        /* Register and pack memory pin context that will be sent to remote */
        rc = lcp_mem_register(req->send.ep->ctx, 
                              &(req->state.lmem), 
                              req->send.buffer, 
                              req->send.length);

        return rc;
}

void lcp_rma_request_complete_put(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_complete(req);
        } 
}

int lcp_rma_put_bcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = ep->priority_chnl;

	mpc_common_debug_info("LCP: send put bcopy remote addr=%p, dest=%d",
			      req->send.rma.remote_addr, ep->uid);
        payload_size = lcp_send_do_put_bcopy(ep->lct_eps[cc], 
                                             lcp_rma_put_pack, req, 
                                             req->send.rma.remote_addr,
                                             &req->send.rma.rkey->mems[cc]);

	//FIXME: handle error
	if (payload_size < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}

        lcp_request_complete(req);

        return rc;
}

//FIXME: code very similar with lcp_send_rput_common
int lcp_rma_put_zcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i, num_used_ifaces;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        /* Set the length to be sent on each interface. The minimum size that
         * can be sent on a interface is defined by min_frag_size */
        num_used_ifaces = req->send.rma.rkey->num_ifaces;
        per_ep_length   = sctk_malloc(num_used_ifaces * sizeof(size_t));
        for (i=0; i<num_used_ifaces; i++) {
                per_ep_length[i] = (size_t)req->send.length / num_used_ifaces;
        }
        per_ep_length[0] += req->send.length % num_used_ifaces;

        remaining   = req->send.length;
        offset      = 0;
        ep          = req->send.ep;

        req->state.comp.comp_cb = lcp_rma_request_complete_put;

        while (remaining > 0) {
                lcr_ep = ep->lct_eps[req->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                /* length is min(max_frag_size, per_ep_length) */
                frag_length = attr.iface.cap.rma.max_put_zcopy;
                length = per_ep_length[req->state.cc] < frag_length ? 
                        per_ep_length[req->state.cc] : frag_length;

                //FIXME: error managment => NO_RESOURCE not handled
                rc = lcp_send_do_put_zcopy(lcr_ep,
                                           (uint64_t)req->send.buffer + offset,
                                           offset,
                                           &(req->send.rma.rkey->mems[req->state.cc]),
                                           length,
                                           &(req->state.comp));

                offset  += length; remaining -= length;
                per_ep_length[req->state.cc] -= length;

                req->state.cc = (req->state.cc + 1) % num_used_ifaces;
        }

        sctk_free(per_ep_length);

        return rc;
}

int lcp_rma_put_start(lcp_ep_h ep, lcp_request_t *req) 
{
        int rc = MPC_LOWCOMM_SUCCESS;

        if (req->send.length <= ep->ep_config.rma.max_put_bcopy) {
                req->send.func = lcp_rma_put_bcopy;
        } else if (req->send.length <= ep->ep_config.rma.max_put_zcopy) {
                rc = lcp_rma_reg_send_buffer(req);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP RMA: could not register send buffer");
                        goto err;
                }
                req->send.func = lcp_rma_put_zcopy;
        }

err:
        return rc;
}

int lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, size_t length,
               uint64_t remote_addr, lcp_mem_h rkey,
               lcp_request_param_t *param) 
{
        int rc;
        UNUSED(length);
        lcp_request_t *req;

        assert(param->flags & LCP_REQUEST_USER_REQUEST);

        uint64_t msg_id = lcp_rand_uint64();

        rc = lcp_request_create(&req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        req->flags |= LCP_REQUEST_RMA_COMPLETE;
        LCP_REQUEST_INIT_SEND(req, ep->ctx, task, NULL, NULL, length, ep, (void *)buffer, 
                              0 /* no ordering for rma */, msg_id);
        lcp_request_init_rma_put(req, remote_addr, rkey, param);

        rc = lcp_rma_put_start(ep, req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;  
        }

        rc = lcp_request_send(req);
err:
        return rc;
}

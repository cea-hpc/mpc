#include "lcp_ep.h"

#include "sctk_alloc.h"

#include "lcp_context.h"
#include "lcp_common.h"

#include <limits.h>
#include "uthash.h"

static int lcp_ep_check_if_valid(lcp_ep_h ep) 
{
        int is_valid;

        /* Endpoint must have at least one transport interface */
        is_valid = (uint64_t)ep->map.bits != 0;

        /* Endpoint must have a single channel if local comm */
        if (ep->uid == ep->ctx->process_uid) {
                is_valid = is_valid && (ep->num_chnls == 1);
                if (!is_valid) {
                        mpc_common_debug_warning("LCP: ep is self but no SM "
                                                 "transport found. Check TBSM.");
                }
        }

        return is_valid;
}

//FIXME: Cannot be used by TAG offload data path since multiplexing not allowed.
int lcp_ep_get_next_cc(lcp_ep_h ep) 
{
        int i;
        lcp_chnl_idx_t cc_idx;

        /* Set ep comm channel to next. Both are first set as priority channel
         * during request initialization. */
        ep->cc = ep->next_cc;
        
        /* Set next comm channel */
        if (ep->num_chnls > 1 && ep->ctx->config.multirail_enabled) {
                cc_idx = ep->cc + 1;
                for (i = 0; i < ep->ctx->num_resources; i++) {
                        if (MPC_BITMAP_GET(ep->map, cc_idx)) {
                                ep->next_cc = cc_idx;
                                break;
                        }
                        cc_idx = (cc_idx + 1) % ep->ctx->num_resources;
                }
        } else {
                ep->next_cc = ep->cc;
        }
        
        return ep->cc;
}

int lcp_ep_create_base(lcp_context_h ctx, lcp_ep_h *ep_p)
{
	int rc        = MPC_LOWCOMM_SUCCESS;
        bmap_t ep_map = MPC_BITMAP_INIT;
	lcp_ep_h       ep;

	ep = sctk_malloc(sizeof(struct lcp_ep));
	if (ep == NULL) {
		mpc_common_debug_error("LCP: failed to allocate endpoint");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ep, 0, sizeof(struct lcp_ep));

        /* Allocate as many transport endpoints as resources */
	ep->lct_eps = sctk_malloc(ctx->num_resources * sizeof(_mpc_lowcomm_endpoint_t *));
	if (ep->lct_eps == NULL) {
		mpc_common_debug_error("LCP: could not allocate endpoint rails.");
		rc = MPC_LOWCOMM_ERROR;
		goto err_alloc_eps;
	}
	memset(ep->lct_eps, 0, ctx->num_resources * sizeof(_mpc_lowcomm_endpoint_t *));

        /* Init channel endpoint bitmap */
        ep->map = ep_map;
	
	*ep_p   = ep;

	return rc;
err_alloc_eps:
        sctk_free(ep); 
err:
	return rc;
}

int lcp_ep_init_config(lcp_context_h ctx, lcp_ep_h ep)
{
	int i;
	int max_prio = 0, prio_idx = 0;

	ep->ep_config.am.max_bcopy       = SIZE_MAX;
	ep->ep_config.am.max_zcopy       = SIZE_MAX;
        ep->ep_config.am.max_iovecs      = SIZE_MAX;
	ep->ep_config.tag.max_bcopy      = SIZE_MAX;
	ep->ep_config.tag.max_zcopy      = SIZE_MAX;
        ep->ep_config.tag.max_iovecs     = SIZE_MAX;
	ep->ep_config.rndv.max_put_zcopy = SIZE_MAX;
	ep->ep_config.rndv.max_get_zcopy = SIZE_MAX;
	ep->ep_config.rma.max_put_bcopy  = SIZE_MAX;
	ep->ep_config.rma.max_put_zcopy  = SIZE_MAX;
        ep->ep_config.offload            = 0;

	for (i=0; i<ctx->num_resources; i++) {	
                /* Only append config of used endpoint interfaces */
                if (!MPC_BITMAP_GET(ep->map, i)) 
                        continue;

		lcp_rsc_desc_t if_desc  = ctx->resources[i];
                sctk_rail_info_t *iface = ctx->resources[i].iface;
                lcr_rail_attr_t attr;

		if (max_prio < if_desc.priority) {
			max_prio = if_desc.priority;
			prio_idx = i;
		}
                
                iface->iface_get_attr(iface, &attr);
                if (iface->runtime_config_rail->offload) {
                        ep->ep_config.offload = 1;
                        ep->ep_config.tag.max_bcopy = LCP_MIN(ep->ep_config.tag.max_bcopy,
                                                              attr.iface.cap.tag.max_bcopy);
                        ep->ep_config.tag.max_zcopy = LCP_MIN(ep->ep_config.tag.max_zcopy,
                                                              attr.iface.cap.tag.max_zcopy);
                        ep->ep_config.tag.max_iovecs = LCP_MIN(ep->ep_config.tag.max_iovecs,
                                                               attr.iface.cap.tag.max_iovecs);
                }
                ep->ep_config.am.max_bcopy = LCP_MIN(ep->ep_config.am.max_bcopy,
                                                  attr.iface.cap.am.max_bcopy);
                ep->ep_config.am.max_zcopy = LCP_MIN(ep->ep_config.am.max_zcopy,
                                                  attr.iface.cap.am.max_zcopy);
                ep->ep_config.am.max_iovecs = LCP_MIN(ep->ep_config.am.max_iovecs,
                                                   attr.iface.cap.am.max_iovecs);
                ep->ep_config.rndv.max_get_zcopy = LCP_MIN(ep->ep_config.rndv.max_get_zcopy,
                                                           attr.iface.cap.rndv.max_get_zcopy);
                ep->ep_config.rndv.max_put_zcopy = LCP_MIN(ep->ep_config.rndv.max_put_zcopy,
                                                           attr.iface.cap.rndv.max_put_zcopy);

                if (iface->runtime_config_rail->rdma) {
                        ep->ep_config.rma.max_put_bcopy = LCP_MIN(ep->ep_config.rma.max_put_bcopy,
                                                                  attr.iface.cap.rma.max_put_bcopy);
                        ep->ep_config.rma.max_put_bcopy = LCP_MIN(ep->ep_config.rma.max_put_zcopy,
                                                                  attr.iface.cap.rma.max_put_zcopy);
                }
	}

        //FIXME: should it be two distinct threshold? One for tag, one for am?
        ep->ep_config.rndv_threshold = ep->ep_config.am.max_zcopy;

        //FIXME: priority chnl is used only for offload channels, for am channel
        //       cc and next_cc are used to support multiplexing.
        //       Could be simplified probably.
	ep->priority_chnl = ep->cc = ep->next_cc = prio_idx; 

	return MPC_LOWCOMM_SUCCESS;
}

int lcp_ep_insert(lcp_context_h ctx, lcp_ep_h ep)
{
	int rc;

	lcp_ep_ctx_t *elem = sctk_malloc(sizeof(lcp_ep_ctx_t));
	if (elem == NULL) {
		mpc_common_debug_error("LCP: could not allocate endpoint table entry.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(elem, 0, sizeof(lcp_ep_ctx_t));

	/* Init table entry */
	elem->ep_key = ep->uid;
	elem->ep = ep;
	HASH_ADD(hh, ctx->ep_ht, ep_key, sizeof(mpc_lowcomm_peer_uid_t), elem);

	/* Update context */
	ctx->num_eps++;

	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}

int lcp_ep_init_channels(lcp_context_h ctx, lcp_ep_h ep)
{
	int rc, i;
	int connected = 0;
        int is_self = (ep->uid == ctx->process_uid);

	//NOTE: by default all resources are used except if communications are
        //      shared, in which cases only shared memory resource is used
	for (i=0; i<ctx->num_resources; i++) {
		_mpc_lowcomm_endpoint_t *lcr_ep;
		lcp_rsc_desc_t if_desc = ctx->resources[i];

                /* Use shared mem exclusively only when self */
                if ((is_self && !if_desc.iface->is_self) ||
                    (!is_self && if_desc.iface->is_self))
                        continue;

		/* Check transport endpoint availability */
		lcr_ep = sctk_rail_get_any_route_to_process(if_desc.iface, ep->uid);
		/* If endpoint does not exist yet, connect on demand */
		if (lcr_ep == NULL) {
			if_desc.iface->connect_on_demand(if_desc.iface, ep->uid);
			/* Get newly created endpoint */
			lcr_ep = sctk_rail_get_any_route_to_process(if_desc.iface, 
                                                                    ep->uid);
			if (lcr_ep == NULL) {
				continue;
			}
		}

		/* Add endpoint to list and set bitmap entry */
		ep->lct_eps[i] = lcr_ep;
                MPC_BITMAP_SET(ep->map, i);
                ep->num_chnls++;

                //FIXME: means that only one transport need be connected for the
                //       protocol endpoint to be connected.
                connected = 1;
	}

	if (!connected) {
		ep->state = LCP_EP_FLAG_CONNECTING;
	} else {
		ep->state = LCP_EP_FLAG_CONNECTED;
	}
	mpc_common_debug("LCP: ep state=%s.", ep->state ? "CONNECTING" : "CONNECTED");

	rc = MPC_LOWCOMM_SUCCESS;

	return rc;
}

int lcp_ep_progress_conn(lcp_context_h ctx, lcp_ep_h ep)
{
	int i;
	int connecting = 0;

	for (i=0; i<ctx->num_resources; i++) {
		_mpc_lowcomm_endpoint_t *lcr_ep;
		lcp_rsc_desc_t if_desc = ctx->resources[i];

		lcr_ep = ep->lct_eps[i];
		if (lcr_ep == NULL) {
			/* Check transport endpoint availability */
			lcr_ep = sctk_rail_get_any_route_to_process(if_desc.iface, ep->uid);
			if (lcr_ep == NULL) {
				connecting = 1;
				continue;
			}
			ep->lct_eps[i] = lcr_ep;
		} 
	}

	if (connecting) {
		ep->state =  LCP_EP_FLAG_CONNECTING;
		return 0;
	} else {
		ep->state =  LCP_EP_FLAG_CONNECTED;
		return 1;
	}
}

int lcp_context_ep_create(lcp_context_h ctx, lcp_ep_h *ep_p, 
                          uint64_t uid, __UNUSED__ unsigned flags)
{
	int rc;
	lcp_ep_h ep;	

	/* Allocation endpoint */
	rc = lcp_ep_create_base(ctx, &ep);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto err;
	}

	ep->uid  = uid;
	ep->ctx  = ctx;
        OPA_store_int(&ep->seqn, 0);

	/* Create all transport endpoints */
	rc = lcp_ep_init_channels(ctx, ep);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto err_unalloc;
	}

	/* Init endpoint config */
	lcp_ep_init_config(ctx, ep);

        /* Check if protocol endpoint is valid */ 
        if (!lcp_ep_check_if_valid(ep)) {
                mpc_common_debug_error("LCP: endpoint not valid.");
                rc= MPC_LOWCOMM_ERROR;
                goto err_unalloc;
        }
	
	*ep_p = ep;
	mpc_common_debug_info("LCP: created ep dest=%llu, dst_tsk=%d.",
			      uid, uid);

        return rc;
err_unalloc:
        lcp_ep_delete(ep);
err:
	return rc;
}


int lcp_ep_create(lcp_context_h ctx, lcp_ep_h *ep_p, 
		  mpc_lowcomm_peer_uid_t uid, unsigned flags)
{
	lcp_ep_h ep = NULL;	
	int rc = MPC_LOWCOMM_SUCCESS;

	LCP_CONTEXT_LOCK(ctx);
	lcp_ep_get(ctx, uid, &ep);
	if (ep != NULL) {
		mpc_common_debug_warning("LCP: ep already exists. uid=%llu.", 
                                         uid);
		*ep_p = ep;
                goto err_and_unlock;
	}
	
	/* Create and connect endpoint */
	rc = lcp_context_ep_create(ctx, &ep, uid, flags);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create endpoint. "
				       "uid=%llu.", uid);
		goto err_and_unlock;
	}

	/* Insert endpoint in the context table */
	rc = lcp_ep_insert(ctx, ep);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto err_and_unlock;
	}

	*ep_p = ep;

err_and_unlock:
	LCP_CONTEXT_UNLOCK(ctx);

	return rc;
}

//FIXME: should it be the users responsability to delete it ?
void lcp_ep_delete(lcp_ep_h ep)
{
        int i;

        for (i=0; i<ep->num_chnls; i++) {
                if (!MPC_BITMAP_GET(ep->map, i))
                        continue;
                sctk_free(ep->lct_eps[i]);
        }

        sctk_free(ep->lct_eps);
        sctk_free(ep);

        ep = NULL;
}


void lcp_ep_get(lcp_context_h ctx, mpc_lowcomm_peer_uid_t uid, lcp_ep_h *ep_p)
{
	lcp_ep_ctx_t *elem;

	HASH_FIND(hh, ctx->ep_ht, &uid, sizeof(mpc_lowcomm_peer_uid_t), elem);
	if (elem == NULL) {
		*ep_p = NULL;
	} else {
		*ep_p = elem->ep;
	}
}

__UNUSED__ static int lcp_ep_connection_handler(void *arg, void *data,
				     __UNUSED__ size_t length, 
				     __UNUSED__ unsigned flags)
{
	int rc;
	lcp_context_h ctx = arg;
	mpc_lowcomm_peer_uid_t *dest = data;
	lcp_ep_ctx_t *ctx_ep = NULL;
	lcp_ep_h *ep = NULL;

	mpc_common_spinlock_lock(&(ctx->ctx_lock));

	HASH_FIND(hh, ctx->ep_ht, dest, sizeof(mpc_lowcomm_peer_uid_t), ctx_ep);

	if (ctx_ep != NULL) {
		rc = MPC_LOWCOMM_SUCCESS;
		goto out;
	}

	rc = lcp_context_ep_create(ctx, ep, *dest, 0);

	/* Insert endpoint in the context table */
	rc = lcp_ep_insert(ctx, *ep);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto out;
	}
out:
	mpc_common_spinlock_unlock(&(ctx->ctx_lock));
	return rc;
}

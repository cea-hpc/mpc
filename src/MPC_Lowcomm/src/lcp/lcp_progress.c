#include "lcp.h"

#include "sctk_rail.h"

#include "lcp_context.h"
#include "lcp_prototypes.h"
#include <uthash.h>

int lcp_progress(lcp_context_h ctx)
{
	int i, rc = MPC_LOWCOMM_SUCCESS;

	for (i=0; i<ctx->num_resources; i++) {
		sctk_rail_info_t *iface = ctx->resources[i].iface;
		if (iface->notify_idle_message) {
			iface->notify_idle_message(iface);
		}
	}

	lcp_pending_entry_t *entry_e, *entry_tmp;
        HASH_ITER(hh, ctx->pend_send_req->table, entry_e, entry_tmp) {
                lcp_request_t *req = entry_e->req;
		lcp_ep_h ep        = req->send.ep;

                switch(req->state.status) {
                case MPC_LOWCOMM_LCP_TAG:
                        break;
                case MPC_LOWCOMM_LCP_PEND:
                        if(lcp_ep_progress_conn(ctx, req->send.ep)) {
                                //FIXME: remove this call
                                rc = lcp_send_start(req->send.ep, req);
                        }
                        break;
                case MPC_LOWCOMM_LCP_RPUT_SYNC:
                        break;
                case MPC_LOWCOMM_LCP_RPUT_FRAG:
			if (LCR_IFACE_IS_TM(ep->lct_eps[ep->priority_chnl]->rail)) {
				rc = lcp_send_tag_zcopy_multi(req);
			} else {
				rc = lcp_send_am_zcopy_multi(req);
			}

                        if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                                mpc_common_debug("LCP: progress req=%p", req);
                        } else if (rc == MPC_LOWCOMM_ERROR) {
                                mpc_common_debug_error("LCP: could not progress req=%p", req);
                        }
                        break;
                case MPC_LOWCOMM_LCP_DONE:
                        break;
                }
        }

	return rc;
}

#include "lcp.h"

#include "sctk_rail.h"

#include "lcp_context.h"
#include "lcp_request.h"

#include <uthash.h>

int lcp_progress(lcp_context_h ctx)
{
	int i, rc = MPC_LOWCOMM_SUCCESS;

	for (i=0; i<ctx->num_resources; i++) {
		sctk_rail_info_t *iface = ctx->resources[i].iface;
                iface->iface_progress(iface);
	}

	lcp_pending_entry_t *entry_e, *entry_tmp;
        HASH_ITER(hh, ctx->pend_send_req->table, entry_e, entry_tmp) {
                lcp_request_t *req = entry_e->req;

                switch(req->state.status) {
                case MPC_LOWCOMM_LCP_TAG:
                case MPC_LOWCOMM_LCP_RPUT_SYNC:
                case MPC_LOWCOMM_LCP_DONE:
                        break;
                case MPC_LOWCOMM_LCP_PEND:
                case MPC_LOWCOMM_LCP_RPUT_FRAG:
			rc = lcp_request_send(req);

                        if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                                mpc_common_debug("LCP: progress req=%p", req);
                        } else if (rc == MPC_LOWCOMM_ERROR) {
                                mpc_common_debug_error("LCP: could not progress req=%p", req);
                        }
                        break;
                }
        }

	return rc;
}

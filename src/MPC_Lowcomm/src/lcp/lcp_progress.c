#include "lcp.h"

#include "mpc_common_spinlock.h"
#include "rail.h"

#include "lcp_context.h"
#include "lcp_request.h"

#include <uthash.h>

/**
 * @brief progress in the pending request list
 * 
 * @param ctx 
 * @return int 
 */
int lcp_progress(lcp_context_h ctx)
{
	int i, rc = LCP_SUCCESS;

        for (i=0; i<ctx->num_resources; i++) {
                sctk_rail_info_t *iface = ctx->resources[i].iface;
                if (iface->iface_progress != NULL)
                        iface->iface_progress(iface);
        }

        /* Loop to try sending requests within the pending queue only once. */
        size_t nb_pending = mpc_queue_length(&ctx->pending_queue); 
        while (nb_pending > 0) {
                LCP_CONTEXT_LOCK(ctx);
                /* One request is pulled from pending queue. */
                lcp_request_t *req = mpc_queue_pull_elem(&ctx->pending_queue,
                                                         lcp_request_t, queue);
                LCP_CONTEXT_UNLOCK(ctx);

                /* Send request which will be pushed back in pending queue if 
                 * it could not be sent */
                lcp_request_send(req);

                nb_pending--; 
        }

	return rc;
}

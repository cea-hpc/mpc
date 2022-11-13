#ifndef LCP_EP_H
#define LCP_EP_H

#include "lcp.h"
#include "lcp_types.h"

#include "sctk_rail.h"
#include "opa_primitives.h"

typedef uint16_t lcp_ep_flags_t;

enum {
	LCP_EP_FLAG_CONNECTED = 0,
	LCP_EP_FLAG_CONNECTING,
	LCP_EP_FLAG_USED,
	LCP_EP_FLAG_CLOSED 
};

typedef struct lcp_ep_rndv_config {
	size_t frag_thresh;
} lcp_ep_rndv_config_t;

typedef struct lcp_ep_config {
        size_t max_bcopy;
        size_t max_zcopy;

        struct {
                size_t max_put_zcopy;
                size_t max_get_zcopy;
        } rndv;

        size_t rndv_threshold;

} lcp_ep_config_t;

typedef struct lcp_ep {
	lcp_ep_config_t ep_config;

	lcp_chnl_idx_t priority_chnl;
	lcp_chnl_idx_t current_chnl; /* scheduling channel */

	lcp_ep_flags_t flags;
	int state;

	lcp_context_h ctx; /* Back reference to context */

	uint64_t uid; /* Remote peer uid */
        OPA_int_t seqn;

	int num_chnls; /* Number of channels */
	_mpc_lowcomm_endpoint_t **lct_eps;
} lcp_ep_t;

int lcp_context_ep_create(lcp_context_h ctx, lcp_ep_h *ep_p, 
			  uint64_t uid, unsigned flags);
int lcp_ep_progress_conn(lcp_context_h ctx, lcp_ep_h ep);

#endif

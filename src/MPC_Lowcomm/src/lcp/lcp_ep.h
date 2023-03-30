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

/**
 * @brief configuration of rendez-vous for a given endpoint (specifies fragmentation threshold)
 * 
 */
typedef struct lcp_ep_rndv_config {
	size_t frag_thresh;
} lcp_ep_rndv_config_t;

/**
 * @brief configuration of endpoint
 * 
 */
typedef struct lcp_ep_config {
        struct {
                size_t max_bcopy;
                size_t max_zcopy;
                size_t max_iovecs;
        } am;

        struct {
                size_t max_bcopy;
                size_t max_zcopy;
                size_t max_iovecs;
        } tag;
        int offload;

        struct {
                size_t max_put_zcopy;
                size_t max_get_zcopy;
        } rndv;

        struct {
                size_t max_put_bcopy;
                size_t max_put_zcopy;
        } rma;

        size_t rndv_threshold;

} lcp_ep_config_t;

struct lcp_ep {
        lcp_ep_config_t ep_config;

        lcp_chnl_idx_t priority_chnl;
        lcp_chnl_idx_t cc; /* Round-Robin Communication Chanel */
        lcp_chnl_idx_t next_cc; /* Next cc to be used */

        lcp_ep_flags_t flags;
        int state;

        lcp_context_h ctx; /* Back reference to context */

        uint64_t  uid; /* Remote peer uid */
        OPA_int_t seqn;

        int num_chnls; /* Number of channels */
        _mpc_lowcomm_endpoint_t **lct_eps; //FIXME: rename (lct not ok)
        bmap_t avail_map; /* Bitmap of usable transport endpoints */
        bmap_t conn_map;  /* Bitmap of connected transport endpoints */
};


int lcp_context_ep_create(lcp_context_h ctx, lcp_ep_h *ep_p, 
			  uint64_t uid, unsigned flags);
int lcp_ep_progress_conn(lcp_context_h ctx, lcp_ep_h ep);
void lcp_ep_delete(lcp_ep_h ep);
int lcp_ep_get_next_cc(lcp_ep_h ep);

int lcp_ep_get_or_create(lcp_context_h ctx, uint64_t comm_id, uint32_t destination, lcp_ep_h *ep_p, unsigned flags);

#endif

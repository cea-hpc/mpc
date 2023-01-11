#ifndef LCP_H
#define LCP_H

#include "lcp_def.h"
#include "lcp_common.h"

#include <mpc_common_types.h>

/* Context */
lcp_context_h lcp_context_get();
int lcp_context_create(lcp_context_h *ctx_p, unsigned flags);
int lcp_context_fini(lcp_context_h ctx);
//FIXME: hack for portals pte entry
int lcp_context_has_comm(lcp_context_h ctx, uint64_t comm_key);
int lcp_context_add_comm(lcp_context_h ctx, uint64_t comm_key);

/* Endpoint */
int lcp_ep_create(lcp_context_h ctx, 
                  lcp_ep_h *ep, 
		  uint64_t uid, 
                  unsigned flags);
void lcp_ep_get(lcp_context_h ctx, 
                uint64_t uid, 
		lcp_ep_h *ep);

typedef struct lcp_tag_recv_info {
        size_t length;
        int32_t tag;
        int32_t src;
} lcp_tag_recv_info_t;

enum {
        LCP_REQUEST_TRY_OFFLOAD = LCP_BIT(0)
};

typedef struct {
        uint32_t flags;
        lcp_tag_recv_info_t *recv_info;
        lcp_mem_h memh;
} lcp_request_param_t;

/* Send/Receive */
int lcp_tag_send_nb(lcp_ep_h ep, const void *buffer,
                    size_t count, mpc_lowcomm_request_t *request, 
                    const lcp_request_param_t *param);

int lcp_tag_recv_nb(lcp_context_h ctx, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param);

/* Progress */
int lcp_progress(lcp_context_h ctx);

#endif

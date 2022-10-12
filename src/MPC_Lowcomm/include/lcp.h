#ifndef LCP_H
#define LCP_H

#include "lcp_def.h"
#include "lcp_common.h"

#include <mpc_lowcomm_types.h>
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

/* Send/Receive */
int lcp_send(lcp_ep_h ep, 
             mpc_lowcomm_request_t *request, 
	     void *buffer, 
             uint64_t seqn);
int lcp_recv(lcp_context_h ctx, 
             mpc_lowcomm_request_t *request,
	     void *buffer);

/* Progress */
int lcp_progress(lcp_context_h ctx);

#endif

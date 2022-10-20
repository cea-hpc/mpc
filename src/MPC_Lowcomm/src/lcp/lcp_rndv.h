#ifndef LCP_RNDV_H
#define LCP_RNDV_H

#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_context.h"

int lcp_send_rndv_start(lcp_request_t *req);
int lcp_rndv_matched(lcp_context_h ctx, 
		lcp_request_t *rreq,
		lcp_rndv_hdr_t *hdr);

#endif

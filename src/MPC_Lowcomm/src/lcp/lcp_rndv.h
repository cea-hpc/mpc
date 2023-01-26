#ifndef LCP_RNDV_H
#define LCP_RNDV_H

#include "lcp_def.h"
#include "lcp_types.h"

#include "lcp_header.h"


int lcp_send_rput_common(lcp_request_t *super, lcp_mem_h rmem);
int lcp_send_rndv_am_start(lcp_request_t *req);
int lcp_rndv_matched(lcp_request_t *req, 
                     lcp_rndv_hdr_t *hdr, size_t length,
                     lcp_rndv_mode_t rndv_mode);
int lcp_recv_rsend(lcp_request_t *req, void *hdr);
int lcp_recv_rget(lcp_request_t *req, void *hdr, size_t hdr_length);
int lcp_recv_rput(lcp_request_t *req, void *hdr);
#endif

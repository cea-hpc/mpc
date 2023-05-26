#ifndef LCP_RNDV_H
#define LCP_RNDV_H

#include "lcp_def.h"
#include "lcp_types.h"

size_t lcp_rndv_rts_pack(lcp_request_t *req, void *dest);
int lcp_send_rndv_start(lcp_request_t *req);
int lcp_rndv_reg_send_buffer(lcp_request_t *req);
int lcp_rndv_process_rts(lcp_request_t *req, void *data, size_t length);

#endif

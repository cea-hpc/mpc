#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_def.h"

int lcp_tag_send_ack(lcp_request_t *parent_request, lcp_tag_hdr_t *hdr);
int lcp_send_eager_tag_zcopy(lcp_request_t *req);
int lcp_send_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_rndv_tag_start(lcp_request_t *req);

#endif

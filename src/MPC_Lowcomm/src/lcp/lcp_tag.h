#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_def.h"
#include "lcp_types.h"

int lcp_send_tag_ack(lcp_request_t *super);
int lcp_send_eager_tag_zcopy(lcp_request_t *req);
int lcp_send_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_rndv_tag_start(lcp_request_t *req);

int lcp_recv_eager_tag_data(lcp_request_t *req, void *hdr, size_t length);

#endif

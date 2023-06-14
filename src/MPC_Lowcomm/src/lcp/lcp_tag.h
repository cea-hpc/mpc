#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_def.h"
#include "lcp_types.h"

//FIXME: move tag header over here.

int lcp_send_eager_sync_ack(lcp_request_t *super, void *data);
int lcp_send_eager_tag_zcopy(lcp_request_t *req);
int lcp_send_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_rndv_tag_start(lcp_request_t *req);

int lcp_recv_eager_tag_data(lcp_request_t *req, void *hdr, void *data, size_t length);
void lcp_recv_rndv_tag_data(lcp_request_t *req, void *data);

#endif

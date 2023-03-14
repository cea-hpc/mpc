#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_header.h"

int lcp_tag_send_ack(lcp_request_t *req);
size_t lcp_send_tag_pack(void *dest, void *data);
#endif

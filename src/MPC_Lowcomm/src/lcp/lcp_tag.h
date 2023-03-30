#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_def.h"
#include "lcp_header.h"

int lcp_tag_send_ack(lcp_ep_h ep, lcp_tag_hdr_t *hdr);
size_t lcp_send_tag_pack(void *dest, void *data);
#endif

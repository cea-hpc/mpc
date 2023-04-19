#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_def.h"
#include "lcp_header.h"

int lcp_tag_send_ack(lcp_request_t *parent_request, lcp_tag_hdr_t *hdr);
size_t lcp_send_tag_pack(void *dest, void *data);
uint64_t lcp_msg_id(uint16_t peer_uid, uint16_t sequence);
int lcp_recv_copy(lcp_request_t *req, lcp_tag_hdr_t *hdr, size_t length);
#endif

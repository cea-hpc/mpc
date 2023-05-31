#ifndef LCP_TAG_OFFLOAD_H
#define LCP_TAG_OFFLOAD_H

#include "lcp_def.h"
#include "lcr/lcr_def.h"

int lcp_send_tag_offload_eager_bcopy(lcp_request_t *req);
int lcp_send_tag_offload_eager_zcopy(lcp_request_t *req);

int lcp_send_rndv_offload_start(lcp_request_t *req);

int lcp_recv_tag_zcopy(lcp_request_t *req, sctk_rail_info_t *iface);
int lcp_recv_tag_probe(sctk_rail_info_t *rail, const int src, const int tag, 
                       const uint64_t comm, lcp_tag_recv_info_t *recv_info);

#endif

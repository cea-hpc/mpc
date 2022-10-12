#ifndef LCP_PROTO_H
#define LCP_PROTO_H

#include "lcp_ep.h"
#include "lcp_def.h"
#include "lcp_header.h"

typedef struct lcp_context_multi {
	size_t length;
	size_t offset;
	lcp_request_t *req;
} lcp_context_multi_t;


/* Active message */
int lcp_send_do_am_ack(lcp_request_t *req);
int lcp_send_do_am_frag_zcopy(lcp_request_t *req, size_t offset, 
			      size_t length);
int lcp_send_do_am_zcopy(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req);
int lcp_send_do_am(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req);

/* Tag message */
int lcp_send_do_tag_bcopy(lcp_ep_h ep, uint8_t am_id, lcr_pack_callback_t pack, 
			  lcp_request_t *req);
int lcp_send_do_tag(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req);

/* Start send */
int lcp_send_start(lcp_ep_h ep, lcp_request_t *req);

/* Protocol and tag offload management */
int lcp_proto_recv_do_ack(lcp_request_t *req, sctk_rail_info_t *iface);
int lcp_proto_send_do_ack(lcp_ep_h ep, lcp_request_t *req);
int lcp_proto_send_do_rndv(lcp_request_t *req);

int lcp_send_do_zcopy_multi(lcp_ep_h ep, lcp_request_t *req);

/* Completion management */
void lcp_rndv_complete_frag(lcr_completion_t *comp);

/* Fragmentation message */
int lcp_rndv_matched(lcp_context_h ctx, lcp_request_t *req, 
		     lcp_rndv_hdr_t *hdr);
/* Normal message */
int lcp_tag_matched(lcp_request_t *req, lcp_tag_hdr_t *hdr, 
		    size_t length);
#endif

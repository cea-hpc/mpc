#ifndef LCP_TAG_OFFLOAD_H
#define LCP_TAG_OFFLOAD_H

#include "lcp_def.h"
#include "lcp_ep.h"


/* lcp_tag_t: for matching
 *
 * 64                                              0
 * <-----------------><-----------------><--------->
 *       src (24)             tag (24)     seqn (16) 
 */

#define LCP_TM_SEQN_MASK 0x800000000000fffful 
#define LCP_TM_SRC_MASK 0x800000ffffff0000ul
#define LCP_TM_TAG_MASK 0x7fffff0000000000ul

/* lcp_tag_t: for matched request
 *
 * 64                                              0
 * <------------------------------------><--------->
 *       msg_id  52                        f_id (12) 
 */
#define LCP_TM_FRAG_FID_MASK 0x7000000000000ffful
#define LCP_TM_FRAG_MID_MASK 0x7ffffffffffff000ul

#define LCP_TM_INIT_TAG_CONTEXT(t_ctx, _ctx, _req, _comm_id) \
{ \
	(t_ctx)->arg = _ctx; \
	(t_ctx)->req = _req; \
	(t_ctx)->comm_id = _comm_id; \
}

/* immediate data
 *
 * 64                                    14        0
 * <------------------------------------><--------->
 *               seqn                       prot
 */ 
typedef union {
	uint64_t raw;
	struct {
		uint64_t prot:12;
		uint64_t seqn:52;
	} hdr;
} lcp_tm_imm_t;

typedef struct lcp_tm {
	sctk_rail_info_t *iface; /* main tm interface */
	uint8_t num_tm_iface;    /* number of tm interfaces */
} lcp_tm_t;

/* Send post */
ssize_t lcp_send_do_tag_offload_bcopy(lcp_request_t *req, uint64_t imm, 
				      lcr_pack_callback_t pack);
int lcp_send_do_tag_offload_post(lcp_ep_h ep, lcp_request_t *req);
int lcp_send_do_frag_offload_post(lcp_request_t *req, size_t offset, 
				  unsigned frag_id, size_t length);
int lcp_send_do_ack_offload_post(lcp_request_t *req, 
				 lcr_pack_callback_t pack);

/* Recv post */
int lcp_recv_frag_offload_post(lcp_request_t *req, sctk_rail_info_t *iface,
			       size_t offset, unsigned frag_id, size_t length);
int lcp_recv_tag_offload_post(lcp_request_t *req, sctk_rail_info_t *iface);
int lcp_recv_ack_offload_post(lcp_request_t *req, sctk_rail_info_t *iface);

#endif

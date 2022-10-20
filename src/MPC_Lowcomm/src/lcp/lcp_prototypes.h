#ifndef LCP_AM_H
#define LCP_AM_H

#include "lcp_def.h"
#include "lcp_request.h"
#include "lcr/lcr_def.h"

/* ============================================== */
/* Offload utils                                  */
/* ============================================== */
/* lcp_tag_t: for matching
 *
 * 64                                              0
 * <-----------------><-----------------><--------->
 *       src (24)             tag (24)     seqn (16) 
 */

#define LCP_TM_SEQN_MASK 0x800000000000fffful 
#define LCP_TM_TAG_MASK 0x800000ffffff0000ul
#define LCP_TM_SRC_MASK 0x7fffff0000000000ul

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
 * 64                                    12        0
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

//FIXME: to be removed
int lcp_send_start(lcp_ep_h ep, lcp_request_t *req);

int lcp_send_am_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_am_eager_tag_zcopy(lcp_request_t *req);
int lcp_send_am_zcopy_multi(lcp_request_t *req);

int lcp_send_tag_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_tag_eager_tag_zcopy(lcp_request_t *req);
int lcp_recv_tag_zcopy(lcp_request_t *req, sctk_rail_info_t *iface);

int lcp_send_tag_zcopy_multi(lcp_request_t *req);
int lcp_recv_tag_zcopy_multi(lcp_ep_h ep, lcp_request_t *rreq);

static inline int lcp_send_do_am_bcopy(_mpc_lowcomm_endpoint_t *lcr_ep, 
		uint8_t am_id, 
		lcr_pack_callback_t pack,
		void *arg)
{
	return lcr_ep->rail->send_am_bcopy(lcr_ep, am_id, pack, arg, 0);
}

static inline int lcp_send_do_am_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep, 
		uint8_t am_id, 
		void *header,
		int hdr_size,
		struct iovec *iov,
		int iovcnt,
		lcr_completion_t *comp)
{
	return lcr_ep->rail->send_am_zcopy(lcr_ep, am_id, header,
					 hdr_size, iov,
					 iovcnt, 0, comp);
}

static inline int lcp_send_do_tag_bcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
		lcr_tag_t tag,
		uint64_t imm,
		lcr_pack_callback_t pack,
		void *arg,
		lcr_tag_context_t *tag_ctx)
{
	return lcr_ep->rail->send_tag_bcopy(lcr_ep, tag, imm, pack, arg, 0, tag_ctx);
}

static inline int lcp_send_do_tag_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
		lcr_tag_t tag,
		uint64_t imm,
		const struct iovec *iov,
		size_t iovcnt,
		lcr_tag_context_t *tag_ctx)
{
	return lcr_ep->rail->send_tag_zcopy(lcr_ep, tag, imm, iov, iovcnt, 0, tag_ctx);
}

static inline int lcp_recv_do_tag_zcopy(sctk_rail_info_t *iface,
		lcr_tag_t tag,
		lcr_tag_t ign_tag,
		const struct iovec *iov,
		size_t iovcnt,
		lcr_tag_context_t *tag_ctx) 
{
	return iface->recv_tag_zcopy(iface, tag, ign_tag, iov, iovcnt, 
				     tag_ctx);
}
#endif

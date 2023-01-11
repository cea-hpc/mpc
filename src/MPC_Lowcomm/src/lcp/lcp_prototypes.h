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
 *       src (24)             tag (24)     comm (16)
 */

#define LCP_TM_COMM_MASK 0x800000000000ffffull
#define LCP_TM_TAG_MASK  0x800000ffffff0000ull
#define LCP_TM_SRC_MASK  0x7fffff0000000000ull

#define LCP_TM_SET_MATCHBITS(_matchbits, _src, _tag, _comm) \
        _matchbits |= (_src & 0xffffffull); \
        _matchbits  = (_matchbits << 24); \
        _matchbits |= (_tag & 0xffffffull); \
        _matchbits  = (_matchbits << 16); \
        _matchbits |= (_comm & 0xffffull);

#define LCP_TM_GET_SRC(_matchbits) \
        ((int)((_matchbits & LCP_TM_SRC_MASK) >> 40))
#define LCP_TM_GET_TAG(_matchbits) \
        ((int)((_matchbits & LCP_TM_TAG_MASK) >> 16))
#define LCP_TM_GET_COMM(_matchbits) \
        ((uint16_t)(_matchbits & LCP_TM_COMM_MASK))

/* lcp_tag_t: for matched request
 *
 * 64                                              0
 * <------------------------------------><--------->
 *       msg_id  52                        f_id (12) 
 */
#define LCP_TM_FRAG_FID_MASK 0x7000000000000ffful
#define LCP_TM_FRAG_MID_MASK 0x7ffffffffffff000ul

/* immediate data
 
 * 64    56                                16        0
 * <-----><--------------------------------><-------->
 *   op                  size                  seqn 
 */ 
#define LCP_TM_HDR_OP_MASK     0x7f00000000000000ull
#define LCP_TM_HDR_LENGTH_MASK 0x80ffffffffff0000ull
#define LCP_TM_HDR_SEQN_MASK   0x800000000000ffffull

#define LCP_TM_SET_HDR_DATA(_hdr, _op, _length, _seqn) \
        _hdr |= (_op & 0xffull); \
        _hdr  = (_hdr << 40); \
        _hdr |= (_length & 0x000000ffffffffffull); \
        _hdr  = (_hdr << 16); \
        _hdr |= (_seqn & 0x000000000000ffffull);

#define LCP_TM_GET_HDR_OP(_hdr) \
        ((uint8_t)((_hdr & LCP_TM_HDR_OP_MASK) >> 56))
#define LCP_TM_GET_HDR_LENGTH(_hdr) \
        ((size_t)((_hdr & LCP_TM_HDR_LENGTH_MASK) >> 16))
#define LCP_TM_GET_HDR_SEQN(_hdr) \
        ((size_t)(_hdr & LCP_TM_HDR_SEQN_MASK))

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

static inline int lcp_send_do_tag_rndv_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                             lcr_tag_t tag,
                                             uint64_t imm,
                                             const struct iovec *iov,
                                             size_t iovcnt,
                                             lcr_tag_context_t *tag_ctx)
{
        return lcr_ep->rail->send_tag_rndv_zcopy(lcr_ep, tag, imm, iov, iovcnt, 0, tag_ctx);
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

static inline int lcp_send_do_get_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        uint64_t local_addr,
                                        uint64_t remote_addr,
                                        lcr_memp_t *remote_key,
                                        size_t size,
                                        lcr_tag_context_t *ctx) 
{
        return lcr_ep->rail->send_get(lcr_ep,
                                      local_addr,
                                      remote_addr,
                                      remote_key,
                                      size,
                                      ctx);
}

static inline int lcp_send_do_put_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        uint64_t local_addr,
                                        uint64_t remote_addr,
                                        lcr_memp_t *remote_key,
                                        size_t size,
                                        lcr_tag_context_t *ctx) 
{
        return lcr_ep->rail->send_put(lcr_ep,
                                      local_addr,
                                      remote_addr,
                                      remote_key,
                                      size,
                                      ctx);
}

#endif

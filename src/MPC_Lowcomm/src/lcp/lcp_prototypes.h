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

/* immediate data

 * 64 63   60   56                          16        0
 * <--><---><---><--------------------------><-------->
 * sync  op  bitmap        size                 uid 
 */ 
#define LCP_TM_HDR_SYNC_MASK   0x8000000000000000ull
#define LCP_TM_HDR_OP_MASK     0x7000000000000000ull
#define LCP_TM_HDR_BITMAP_MASK 0x0f00000000000000ull
#define LCP_TM_HDR_LENGTH_MASK 0x00ffffffffff0000ull
#define LCP_TM_HDR_UID_MASK    0x000000000000ffffull

#define LCP_TM_SET_HDR_DATA(_hdr, _sync, _op, _bm, _length, _uid) \
        _hdr |= (_sync ? 0 : 1); \
        _hdr  = (_hdr << 3); \
        _hdr |= (_op & 0x7ull); \
        _hdr  = (_hdr << 4); \
        _hdr |= (_bm & 0xfull); \
        _hdr  = (_hdr << 40); \
        _hdr |= (_length & 0x000000ffffffffffull); \
        _hdr  = (_hdr << 16); \
        _hdr |= (_uid & 0x000000000000ffffull);

#define LCP_TM_GET_HDR_SYNC(_hdr) \
        ((int)((_hdr & LCP_TM_HDR_SYNC_MASK) >> 63))
#define LCP_TM_GET_HDR_OP(_hdr) \
        ((uint8_t)((_hdr & LCP_TM_HDR_OP_MASK) >> 60))
#define LCP_TM_GET_HDR_BITMAP(_hdr) \
        ((uint64_t)((_hdr & LCP_TM_HDR_BITMAP_MASK) >> 56))
#define LCP_TM_GET_HDR_LENGTH(_hdr) \
        ((size_t)((_hdr & LCP_TM_HDR_LENGTH_MASK) >> 16))
#define LCP_TM_GET_HDR_UID(_hdr) \
        ((size_t)(_hdr & LCP_TM_HDR_UID_MASK))

int lcp_send_rndv_offload_start(lcp_request_t *req);

int lcp_send_am_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_am_eager_tag_zcopy(lcp_request_t *req);

int lcp_send_tag_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_tag_eager_tag_zcopy(lcp_request_t *req);
int lcp_recv_tag_zcopy(lcp_request_t *req, sctk_rail_info_t *iface);
int lcp_recv_tag_probe(sctk_rail_info_t *rail, const int src, const int tag, 
                       const uint64_t comm, lcp_tag_recv_info_t *recv_info);

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
                                        void *arg)
{
        return lcr_ep->rail->send_tag_bcopy(lcr_ep, tag, imm, pack, arg, 0);
}

static inline int lcp_send_do_tag_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        lcr_tag_t tag,
                                        uint64_t imm,
                                        const struct iovec *iov,
                                        size_t iovcnt,
                                        lcr_completion_t *comp)
{
        return lcr_ep->rail->send_tag_zcopy(lcr_ep, tag, imm, iov, 
                                            iovcnt, 0, comp);
}

static inline int lcp_post_do_tag_zcopy(sctk_rail_info_t *iface,
                                        lcr_tag_t tag,
                                        lcr_tag_t ign_tag,
                                        const struct iovec *iov,
                                        size_t iovcnt,
                                        unsigned flags,
                                        lcr_tag_context_t *tag_ctx) 
{
        return iface->post_tag_zcopy(iface, tag, ign_tag, iov, iovcnt, 
                                     flags, tag_ctx);
}

static inline int lcp_send_do_get_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        uint64_t local_addr,
                                        uint64_t remote_addr,
                                        lcr_memp_t *remote_key,
                                        size_t size,
                                        lcr_completion_t *comp) 
{
        return lcr_ep->rail->get_zcopy(lcr_ep, local_addr,
                                       remote_addr, remote_key,
                                       size, comp);
}

static inline int lcp_send_do_get_tag_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                            lcr_tag_t tag,
                                            uint64_t local_offset,
                                            uint64_t remote_offset,
                                            size_t size,
                                            lcr_completion_t *comp) 
{
        return lcr_ep->rail->get_tag_zcopy(lcr_ep, tag, local_offset,
                                           remote_offset, size, comp);
}

static inline int lcp_send_do_put_bcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        lcr_pack_callback_t pack,
                                        void *arg,
                                        uint64_t remote_addr,
                                        lcr_memp_t *remote_key)
{
        return lcr_ep->rail->put_bcopy(lcr_ep, pack, arg, 
                                       remote_addr, remote_key);
}

static inline int lcp_send_do_put_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        uint64_t local_addr,
                                        uint64_t remote_addr,
                                        lcr_memp_t *remote_key,
                                        size_t size,
                                        lcr_completion_t *comp) 
{
        return lcr_ep->rail->put_zcopy(lcr_ep, local_addr,
                                       remote_addr, remote_key,
                                       size, comp);
}

static inline int lcp_iface_do_progress(sctk_rail_info_t *rail)
{
        return rail->iface_progress(rail);
}

#endif

#ifndef LCP_PROTOTYPES_H
#define LCP_PROTOTYPES_H

#include "lcp_def.h"
#include "lcr/lcr_def.h"

#include <rail.h>

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

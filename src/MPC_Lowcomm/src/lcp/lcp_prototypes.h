/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef LCP_PROTOTYPES_H
#define LCP_PROTOTYPES_H

#include "lcp_def.h"
#include "lcr/lcr_def.h"

#include <rail.h>

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */

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
                                        lcr_memp_t *local_key,
                                        lcr_memp_t *remote_key)
{
        return lcr_ep->rail->put_bcopy(lcr_ep, pack, arg, 
                                       remote_addr, local_key, remote_key);
}

static inline int lcp_send_do_get_bcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        lcr_pack_callback_t pack,
                                        void *arg,
                                        uint64_t remote_addr,
                                        lcr_memp_t *local_key,
                                        lcr_memp_t *remote_key)
{
        return lcr_ep->rail->get_bcopy(lcr_ep, pack, arg, 
                                       remote_addr, local_key, remote_key);
}

static inline int lcp_send_do_put_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        uint64_t local_addr,
                                        uint64_t remote_addr,
                                        lcr_memp_t *local_key,
                                        lcr_memp_t *remote_key,
                                        size_t size,
                                        lcr_completion_t *comp)
{
        return lcr_ep->rail->put_zcopy(lcr_ep, local_addr,
                                       remote_addr, local_key,
                                       remote_key, size, comp);
}

static inline int lcp_send_do_get_zcopy(_mpc_lowcomm_endpoint_t *lcr_ep,
                                        uint64_t local_addr,
                                        uint64_t remote_addr,
                                        lcr_memp_t *local_key,
                                        lcr_memp_t *remote_key,
                                        size_t size,
                                        lcr_completion_t *comp) 
{
        return lcr_ep->rail->get_zcopy(lcr_ep, local_addr,
                                       remote_addr, local_key,
                                       remote_key, size, comp);
}

static inline int lcp_atomic_do_post(_mpc_lowcomm_endpoint_t *lcr_ep,
                                     uint64_t *value,
                                     uint64_t remote_offset,
                                     lcr_atomic_op_t op_type,
                                     lcr_memp_t *local_key,
                                     lcr_memp_t *remote_key,
                                     size_t size,
                                     lcr_completion_t *comp)
{
        return lcr_ep->rail->atomic_post(lcr_ep, (uint64_t)value, 
                                         remote_offset, op_type,
                                         local_key, remote_key,
                                         size, comp);
}

static inline int lcp_atomic_do_fetch(_mpc_lowcomm_endpoint_t *lcr_ep,
                                      uint64_t *result,
                                      uint64_t *value,
                                      uint64_t remote_offset,
                                      lcr_atomic_op_t op_type,
                                      lcr_memp_t *local_key,
                                      lcr_memp_t *remote_key,
                                      size_t size,
                                      lcr_completion_t *comp)
{
        return lcr_ep->rail->atomic_fetch(lcr_ep, (uint64_t)result, 
                                          (uint64_t)value,
                                          remote_offset, op_type,
                                          local_key, remote_key,
                                          size, comp);
}

static inline int lcp_atomic_do_cswap(_mpc_lowcomm_endpoint_t *lcr_ep,
                                      uint64_t *result,
                                      uint64_t *value,
                                      uint64_t compare,
                                      uint64_t remote_offset,
                                      lcr_atomic_op_t op_type,
                                      lcr_memp_t *local_key,
                                      lcr_memp_t *remote_key,
                                      size_t size,
                                      lcr_completion_t *comp) 
{
        return lcr_ep->rail->atomic_cswap(lcr_ep, (uint64_t)result,
                                          (uint64_t)value,
                                          remote_offset, op_type,
                                          local_key, remote_key,
                                          compare, size, comp);
}

static inline int lcp_iface_do_progress(sctk_rail_info_t *rail)
{
        return rail->iface_progress(rail);
}

/* NOLINTEND(clang-diagnostic-unused-function) */

#endif

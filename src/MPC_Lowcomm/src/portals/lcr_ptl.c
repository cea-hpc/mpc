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

#include "lcr_ptl.h"

#include "sctk_ptl_iface.h"
#include "sctk_ptl_offcoll.h"

#include <sctk_alloc.h>
#include <alloca.h>
#include <uthash.h>

//FIXME: add safeguard for msg size
ssize_t lcr_ptl_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              unsigned flags)
{
        UNUSED(flags);
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = NULL;
        size_t size                    = 0;
        sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        lcr_ptl_send_comp_t *ptl_comp;

        //FIXME: use a memory pool.
        start = sctk_malloc(srail->eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        assert(size <= srail->eager_limit);
        remote = infos->dest;

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                size = -1;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->type = LCR_PTL_COMP_AM_BCOPY;
        ptl_comp->bcopy_buf = start;

        sctk_ptl_chk(PtlPut(srail->ptl_info.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.am_pte,
                            match.raw,
                            0, /* remote offset */
                            ptl_comp,
                            id
                           ));

err:
        return size;
}

int lcr_ptl_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
                          uint8_t id,
                          const void *header,
                          unsigned header_length,
                          const struct iovec *iov,
                          size_t iovcnt,
                          unsigned flags,
                          lcr_completion_t *comp)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        UNUSED(flags);
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        size_t size                    = 0;
        sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        ptl_iovec_t *ptl_iovec         = sctk_malloc((iovcnt + 1)
                                                * sizeof(ptl_iovec_t));
	ptl_md_t iov_md;
        lcr_ptl_send_comp_t *ptl_comp;

        assert(iovcnt <= (size_t)srail->ptl_info.max_iovecs);
        ptl_iovec[0].iov_len  = header_length;
        ptl_iovec[0].iov_base = (void *)header;
        size += header_length;

        for (i=0; i<(int)iovcnt; i++) {
                ptl_iovec[1 + i].iov_base = iov[i].iov_base;
                size += ptl_iovec[1 + i].iov_len = iov[i].iov_len;
        }

        remote = infos->dest;

        assert(iovcnt + 1 <= (size_t)srail->ptl_info.max_iovecs);

        iov_md = (ptl_md_t){
                .start = ptl_iovec,
                .length = iovcnt + 1,
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->ptl_info.eqh,
                .options = PTL_MD_EVENT_SEND_DISABLE | PTL_IOVEC,
        };

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate ptl coop");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));

	sctk_ptl_chk(PtlMDBind(
		srail->iface,      /* the NI handler */
		&iov_md,           /* the MD to bind with memory region */
		&ptl_comp->iov_mdh /* out: the MD handler */
	));

        ptl_comp->comp = comp;
        ptl_comp->iov  = ptl_iovec;
        ptl_comp->type = LCR_PTL_COMP_AM_ZCOPY;

        sctk_ptl_chk(PtlPut(ptl_comp->iov_mdh,
                            0,
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.am_pte,
                            match.raw,
                            0,
                            ptl_comp,
                            id
                           ));

err:
        return rc;
}

ssize_t lcr_ptl_send_tag_bcopy(_mpc_lowcomm_endpoint_t *ep,
                               lcr_tag_t tag,
                               uint64_t imm,
                               lcr_pack_callback_t pack,
                               void *arg,
                               __UNUSED__ unsigned cflags)
{
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = NULL;
        size_t size                    = 0;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        lcr_ptl_send_comp_t *ptl_comp;

        start = sctk_malloc(srail->eager_limit);
        size = pack(start, arg);

        assert(size <= srail->eager_limit);

        remote = infos->dest;

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate ptl coop");
                size = -1;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));

        ptl_comp->type      = LCR_PTL_COMP_TAG_BCOPY;
        ptl_comp->bcopy_buf = start;

        mpc_common_debug_info("lcr ptl: send tag bcopy to %d (iface=%llu, match=[%d,%d,%d], "
                              "remote=%llu, idx=%d, sz=%llu)", ep->dest, srail->iface,
                              tag.t_tag.tag, tag.t_tag.src, tag.t_tag.comm,
                              remote, srail->ptl_info.tag_pte, size);
        sctk_ptl_chk(PtlPut(srail->ptl_info.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.tag_pte,
                            tag.t,
                            0, /* remote offset */
                            ptl_comp,
                            imm
                           ));

err:
        return size;
}

int lcr_ptl_send_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_tag_t tag,
                           uint64_t imm,
                           const struct iovec *iov,
                           size_t iovcnt,
                           __UNUSED__ unsigned cflags,
                           lcr_completion_t *comp)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        size_t size                    = iov->iov_len;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        lcr_ptl_send_comp_t *ptl_comp;

        //FIXME: support for only one iovec
        assert(iovcnt == 1);

        /* prepare the Put() MD */
        remote                 = infos->dest;

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate ptl coop");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));

        ptl_comp->type      = LCR_PTL_COMP_TAG_ZCOPY;
        ptl_comp->comp      = comp;

        mpc_common_debug_info("LCR PTL: send tag zcopy to %d (iface=%llu, match=[%d,%d,%d], "
                              "remote=%llu, idx=%d, sz=%llu, user_ptr=%p)",
                              ep->dest, srail->iface,
                              tag.t_tag.tag, tag.t_tag.src, tag.t_tag.comm,
                              remote, srail->ptl_info.tag_pte, size, ptl_comp);
        sctk_ptl_chk(PtlPut(srail->ptl_info.mdh,
                            (ptl_size_t) iov[0].iov_base, /* local offset */
                            iov[0].iov_len,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.tag_pte,
                            tag.t,
                            0, /* remote offset */
                            ptl_comp,
                            imm
                           ));

err:
        return rc;
}

int lcr_ptl_post_tag_zcopy(sctk_rail_info_t *rail,
                           lcr_tag_t tag, lcr_tag_t ign_tag,
                           const struct iovec *iov,
                           size_t iovcnt, /* only one iov supported */
                           unsigned flags,
                           lcr_tag_context_t *ctx)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ptl_me_t me;
        ptl_handle_me_t meh;
        unsigned int persistant = flags & LCR_IFACE_TM_PERSISTANT_MEM ?
                0 : SCTK_PTL_ONCE;
        unsigned int search = flags & LCR_IFACE_TM_SEARCH;
        lcr_ptl_persistant_post_t *persistant_post;
        sctk_ptl_rail_info_t* srail     = &rail->network.ptl;
        lcr_ptl_send_comp_t *ptl_comp;

        assert(iov && iovcnt == 1);

        /* complete the ME data, this ME will be appended to the PRIORITY_LIST */
        me = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                        .ignore_bits = ign_tag.t,
                        .match_bits  = tag.t,
                        .match_id = SCTK_PTL_ANY_PROCESS,
                        .min_free = 0,
                        .length = iov[iovcnt-1].iov_len,
                        .start = iov[iovcnt-1].iov_base,
                        .uid = PTL_UID_ANY,
                        .options = SCTK_PTL_ME_PUT_FLAGS    |
                                SCTK_PTL_ME_GET_FLAGS       |
                                persistant
        };

        mpc_common_debug_info("LCR PTL: post recv zcopy to %p (idx=%d, tag=%llu, "
                              "sz=%llu, buf=%p)", rail, tag.t,
                              srail->ptl_info.tag_pte, iov[0].iov_len,
                              iov[0].iov_base);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate ptl coop");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));

        ptl_comp->type    = search ? LCR_PTL_COMP_TAG_SEARCH : LCR_PTL_COMP_TAG_ZCOPY;
        ptl_comp->tag_ctx = ctx;

        if (search) {
                sctk_ptl_chk(PtlMESearch(srail->iface,
                                         srail->ptl_info.tag_pte,
                                         &me,
                                         PTL_SEARCH_ONLY,
                                         ptl_comp));
        } else {
                sctk_ptl_chk(PtlMEAppend(srail->iface,
                                         srail->ptl_info.tag_pte,
                                         &me,
                                         SCTK_PTL_PRIORITY_LIST,
                                         ptl_comp,
                                         &meh
                                        ));
        }

        if (flags & LCR_IFACE_TM_PERSISTANT_MEM) {
                persistant_post = sctk_malloc(sizeof(lcr_ptl_persistant_post_t));
                if (persistant_post == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate "
                                               "persistant post");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                persistant_post->tag_key = tag.t;
                persistant_post->meh = meh;
                HASH_ADD(hh, srail->ptl_info.persistant_ht, tag_key, sizeof(uint64_t),
                         persistant_post);
        }

err:
        return rc;
}

int lcr_ptl_unpost_tag_zcopy(sctk_rail_info_t *rail, lcr_tag_t tag)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_persistant_post_t *persistant_post;
        sctk_ptl_rail_info_t* srail     = &rail->network.ptl;

        HASH_FIND(hh, srail->ptl_info.persistant_ht, &tag.t, sizeof(uint64_t),
                  persistant_post);
        if (persistant_post == NULL) {
                mpc_common_debug_error("LCR PTL: could not find posted "
                                       "element with tag: %llu", tag.t);
                rc = MPC_LOWCOMM_ERROR;
                return rc;
        }

        sctk_ptl_chk(PtlMEUnlink(persistant_post->meh));

        return rc;
}

int lcr_ptl_send_put_bcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_pack_callback_t pack,
                           void *arg,
                           uint64_t remote_addr,
                           lcr_memp_t *remote_key)
{
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = NULL;
        size_t size                    = 0;
        sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        lcr_ptl_send_comp_t *ptl_comp;

        start = sctk_malloc(srail->eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        remote = infos->dest;

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                size = -1;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->type = LCR_PTL_COMP_AM_BCOPY; //FIXME: assign proper type even though
                                                //       behabior is same as AM
        ptl_comp->bcopy_buf = start;

        mpc_common_debug_info("lcr ptl: send am bcopy to %d ("
                              "remote=%llu, sz=%llu, pte=%d)", ep->dest,
                              remote, size, srail->ptl_info.am_pte);

        //FIXME: there are questions around the completion of bcopy requests.
        //       With Portals, how should a request be completed?
        sctk_ptl_chk(PtlPut(srail->ptl_info.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.rma_pte,
                            match.raw,
                            (uint64_t)remote_key->pin.ptl.start + remote_addr,
                            ptl_comp,
                            0
                           ));

err:
        return size;
}

int lcr_ptl_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote           = infos->dest;
        sctk_ptl_matchbits_t rdv_match = SCTK_PTL_MATCH_INIT;
        lcr_ptl_send_comp_t *ptl_comp  = NULL;

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p, "
                              "remote addr=%p",
                              __sctk_ptl_match_str(sctk_malloc(32), 32,
                                                   remote_key->pin.ptl.match.raw),
                              remote, remote_offset, srail->ptl_info.rma_pte,
                              local_addr, remote_key->pin.ptl.start);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->type = LCR_PTL_COMP_RMA_PUT;
        ptl_comp->comp = comp;

        sctk_ptl_chk(PtlPut(srail->ptl_info.mdh, local_addr,
                            size, PTL_ACK_REQ, remote,
                            srail->ptl_info.rma_pte, rdv_match.raw,
                            (uint64_t)remote_key->pin.ptl.start + remote_offset,
                            ptl_comp, 0
                           ));
err:
        return rc;
}

int lcr_ptl_send_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote           = infos->dest;
        sctk_ptl_matchbits_t rdv_match = SCTK_PTL_MATCH_INIT;
        lcr_ptl_send_comp_t *ptl_comp  = NULL;

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p, "
                              "remote addr=%p, iface=%d",
                              __sctk_ptl_match_str(sctk_malloc(32), 32,
                                                   remote_key->pin.ptl.match.raw),
                              remote, remote_offset, srail->ptl_info.rma_pte,
                              local_addr, remote_key->pin.ptl.start, srail->iface);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->comp = comp;

        sctk_ptl_chk(PtlGet(srail->ptl_info.mdh,
                            local_addr,
                            size,
                            remote,
                            srail->ptl_info.rma_pte,
                            rdv_match.raw,
                            (uint64_t)remote_key->pin.ptl.start + remote_offset,
                            ptl_comp
                           ));
err:
        return rc;
}

int lcr_ptl_get_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
                          lcr_tag_t tag,
                          uint64_t local_offset,
                          uint64_t remote_offset,
                          size_t size,
                          lcr_completion_t *comp)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail   = &ep->rail->network.ptl;
        sctk_ptl_id_t remote          = infos->dest;
        lcr_ptl_send_comp_t *ptl_comp = NULL;

        mpc_common_debug_info("PTL: get tag remote key. "
                              "iface=%p, remote=%llu, remote off=%llu, pte idx=%d, "
                              "local addr=%p", ep->rail, remote, remote_offset,
                              srail->ptl_info.tag_pte, local_offset);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->comp = comp;

        sctk_ptl_chk(PtlGet(srail->ptl_info.mdh,
                            local_offset,
                            size,
                            remote,
                            srail->ptl_info.tag_pte,
                            tag.t,
                            remote_offset,
                            ptl_comp
                           ));

err:
        return rc;

}

void lcr_ptl_mem_register(struct sctk_rail_info_s *rail,
                          struct sctk_rail_pin_ctx_list *list,
                          void * addr, size_t size)
{
        UNUSED(rail);
        UNUSED(addr);
        UNUSED(size);
        list->pin.ptl.start = addr;
}

void lcr_ptl_mem_unregister(struct sctk_rail_info_s *rail,
                            struct sctk_rail_pin_ctx_list *list)
{
        UNUSED(rail);
        UNUSED(list);
        return;
}

int lcr_ptl_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;

        *(uint64_t *)p = (uint64_t)memp->pin.ptl.start;

        return sizeof(uint64_t);
}

int lcr_ptl_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;

        /* deserialize data */
        memp->pin.ptl.start = *(uint64_t *)p;

        return sizeof(uint64_t);
}

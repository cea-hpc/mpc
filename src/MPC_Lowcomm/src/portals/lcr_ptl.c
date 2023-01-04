#include "lcr_ptl.h"

#include "sctk_ptl_iface.h"
#include "sctk_ptl_offcoll.h"

#include <sctk_alloc.h>
#include <alloca.h>

//TODO: not used yet. Should it be ?
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

        start = sctk_malloc(srail->eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        remote = infos->dest;

        assert(size <= srail->eager_limit);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                size = -1;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->type = LCR_PTL_COMP_BCOPY;
        ptl_comp->bcopy_buf = start;

        mpc_common_debug_info("lcr ptl: send bcopy to %d (iface=%llu, "
                              "remote=%llu, sz=%llu, pte=%d)", ep->dest, srail->iface, 
                              remote, size, srail->ptl_info.eager_pt_idx);

        //FIXME: there are questions around the completion of bcopy requests.
        //       With Portals, how should a request be completed?
        sctk_ptl_chk(PtlPut(srail->ptl_info.md_req->slot_h.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.eager_pt_idx,
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
        size_t ptl_iovcnt = 0;
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
                .eq_handle = srail->ptl_info.me_eq,
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
        ptl_comp->type = LCR_PTL_COMP_ZCOPY;

        mpc_common_debug_info("lcr ptl: send am zcopy to %d (iface=%llu, "
                              "remote=%llu, sz=%llu, pte=%d)", ep->dest, srail->iface, 
                              remote, size, srail->ptl_info.eager_pt_idx);
        sctk_ptl_chk(PtlPut(ptl_comp->iov_mdh,
                            0,
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.eager_pt_idx,
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
                               __UNUSED__ unsigned cflags,
                               lcr_tag_context_t *ctx)
{
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = NULL;
        size_t size                    = 0;
        sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
        sctk_ptl_pte_t* pte            = NULL;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_imm_data_t hdr;

        //TODO(HIGH PRIORITY): free start...
        start = sctk_malloc(srail->eager_limit);
        size = pack(start, arg);

        assert(size <= srail->eager_limit);

        /* prepare the Put() MD */
        match.raw              = tag.t;
        hdr.raw                = imm;
        pte                    = mpc_common_hashtable_get(&srail->pt_table, ctx->comm_id);
        remote                 = infos->dest;

        assert(pte);

        mpc_common_debug_info("lcr ptl: send bcopy to %d (iface=%llu, match=%s, "
                              "remote=%llu, idx=%d, sz=%llu)", ep->dest, srail->iface, 
                              __sctk_ptl_match_str(malloc(32), 32, tag.t), 
                              remote, pte->idx, size);
        sctk_ptl_chk(PtlPut(srail->ptl_info.md_req->slot_h.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote,
                            pte->idx,
                            match.raw,
                            0, /* remote offset */
                            ctx,
                            hdr.raw	
                           ));

        return size;
}

int lcr_ptl_send_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_tag_t tag,
                           uint64_t imm,
                           const struct iovec *iov,
                           size_t iovcnt,
                           __UNUSED__ unsigned cflags,
                           lcr_tag_context_t *ctx)
{
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        size_t size                    = iov->iov_len;
        sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
        sctk_ptl_pte_t* pte            = NULL;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_imm_data_t hdr;

        //FIXME: support for only one iovec
        assert(iovcnt == 1);

        /* prepare the Put() MD */
        match.raw              = tag.t;
        hdr.raw                = imm;
        pte                    = mpc_common_hashtable_get(&srail->pt_table, ctx->comm_id);
        remote                 = infos->dest;

        assert(pte);

        mpc_common_debug_info("LCR PTL: send zcopy to %d (iface=%llu, match=%s, "
                              "remote=%llu, idx=%d, sz=%llu, user_ptr=%p)", 
                              ep->dest, srail->iface, 
                              __sctk_ptl_match_str(malloc(32), 32, tag.t), 
                              remote, pte->idx, size, ctx);
        sctk_ptl_chk(PtlPut(srail->ptl_info.md_req->slot_h.mdh,
                            (ptl_size_t) iov[0].iov_base, /* local offset */
                            iov[0].iov_len,
                            PTL_ACK_REQ,
                            remote,
                            pte->idx,
                            match.raw,
                            0, /* remote offset */
                            ctx,
                            hdr.raw	
                           ));

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_send_tag_rndv_zcopy(_mpc_lowcomm_endpoint_t *ep,
                                lcr_tag_t tag,
                                uint64_t imm,
                                const struct iovec *iov,
                                size_t iovcnt,
                                __UNUSED__ unsigned cflags,
                                lcr_tag_context_t *ctx)
{
        sctk_ptl_local_data_t* me_request = NULL;
        sctk_ptl_rail_info_t* srail       = &ep->rail->network.ptl;
        int me_flags                      = 0;
        sctk_ptl_matchbits_t match        = SCTK_PTL_MATCH_INIT;
        sctk_ptl_matchbits_t ign          = SCTK_PTL_MATCH_INIT;
        sctk_ptl_pte_t* pte, *rdma_pte    = NULL;
        sctk_ptl_id_t remote              = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;

        assert(iovcnt == 2);

        match.raw              = tag.t;
        pte                    = mpc_common_hashtable_get(&srail->pt_table, ctx->comm_id);
        remote                 = infos->dest;

        /* complete the ME data, this ME will be appended to the PRIORITY_LIST */
        rdma_pte   = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);
        me_flags   = SCTK_PTL_ME_GET_FLAGS | SCTK_PTL_ONCE;
        me_request = sctk_ptl_me_create(iov[1].iov_base, iov[1].iov_len, remote, match, ign, me_flags); 

        assert(me_request);
        assert(pte);

        me_request->msg  = ctx;
        me_request->list = SCTK_PTL_PRIORITY_LIST;
        me_request->type = SCTK_PTL_TYPE_STD;

        mpc_common_debug_info("LCR PTL: post recv rndv zcopy (iface=%llu, idx=%d, "
                              "sz=%llu, buf=%p, match=%s)", srail->iface, rdma_pte->idx, 
                              iov[1].iov_len, iov[1].iov_base,
                              __sctk_ptl_match_str(sctk_malloc(32), 32, match.raw));
        sctk_ptl_chk(PtlMEAppend(srail->iface,
                                 rdma_pte->idx,
                                 &me_request->slot.me,
                                 me_request->list,
                                 me_request,
                                 &me_request->slot_h.meh
                                ));

        mpc_common_debug_info("LCR PTL: send zcopy to %d (iface=%llu, remote=%llu, "
                              "idx=%d, sz=%llu match=%s)", ep->dest, srail->iface, 
                              remote, pte->idx, iov[0].iov_len,
                              __sctk_ptl_match_str(sctk_malloc(32), 32, match.raw));

        sctk_ptl_chk(PtlPut(srail->ptl_info.md_req->slot_h.mdh,
                            0, /* local offset */
                            0,
                            PTL_ACK_REQ,
                            remote,
                            pte->idx,
                            match.raw,
                            0, /* remote offset */
                            ctx,
                            imm	
                           ));

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_recv_tag_zcopy(sctk_rail_info_t *rail,
                           lcr_tag_t tag, lcr_tag_t ign_tag,
                           const struct iovec *iov, 
                           __UNUSED__ size_t iovcnt, /* only one iov supported */
                           lcr_tag_context_t *ctx) 
{
        void* start                     = NULL;
        unsigned flags                  = 0;
        size_t size                     = 0;
        sctk_ptl_matchbits_t match      = SCTK_PTL_MATCH_INIT;
        sctk_ptl_matchbits_t ign        = SCTK_PTL_MATCH_INIT;
        sctk_ptl_pte_t* pte             = NULL;
        sctk_ptl_local_data_t* user_ptr = NULL;
        sctk_ptl_id_t remote            = SCTK_PTL_ANY_PROCESS;
        sctk_ptl_rail_info_t* srail     = &rail->network.ptl;

        start = iov[0].iov_base;
        match.raw = tag.t;
        ign.raw = ign_tag.t;

        /* complete the ME data, this ME will be appended to the PRIORITY_LIST */
        size     = iov[0].iov_len;
        pte      = mpc_common_hashtable_get(&srail->pt_table, ctx->comm_id);
        flags    = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE;
        user_ptr = sctk_ptl_me_create(start, size, remote, match, ign, flags); 

        assert(user_ptr);
        assert(pte);

        user_ptr->msg  = ctx;
        user_ptr->list = SCTK_PTL_PRIORITY_LIST;
        user_ptr->type = SCTK_PTL_TYPE_STD;
        sctk_ptl_me_register(srail, user_ptr, pte);

        mpc_common_debug_info("LCR PTL: post recv zcopy to %p (iface=%llu, idx=%d, "
                              "sz=%llu, buf=%p)", rail, srail->iface, pte->idx, size, start);
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_send_mput(_mpc_lowcomm_endpoint_t *ep,
                      uint64_t local_addr,
                      uint64_t remote_offset,
                      lcr_memp_t *remote_key,
                      size_t size,
                      lcr_tag_context_t *ctx) 
{
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote = infos->dest;
        sctk_ptl_pte_t *rdma_pte; 

        rdma_pte = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, 
                                                   remote_key->pin.ptl.match.raw),
                              remote, remote_offset, rdma_pte->idx, local_addr);

        sctk_ptl_chk(PtlPut(srail->ptl_info.md_req->slot_h.mdh,
                            local_addr, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote,
                            rdma_pte->idx,
                            remote_key->pin.ptl.match.raw,
                            remote_offset, /* remote offset */
                            ctx,
                            0
                           ));

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_send_put(_mpc_lowcomm_endpoint_t *ep,
                     uint64_t local_addr,
                     uint64_t remote_offset,
                     lcr_memp_t *remote_key,
                     size_t size,
                     lcr_tag_context_t *ctx) 
{
        int rc;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote = infos->dest;
        sctk_ptl_matchbits_t rdv_match = SCTK_PTL_MATCH_INIT;
        lcr_ptl_send_comp_t *ptl_comp;

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p, "
                              "remote addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, 
                                                   remote_key->pin.ptl.match.raw),
                              remote, remote_offset, srail->ptl_info.rndv_pt_idx, 
                              local_addr, remote_key->pin.ptl.start);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->type = LCR_PTL_COMP_PUT;
        ptl_comp->tag_ctx = ctx;

        sctk_ptl_chk(PtlPut(srail->ptl_info.md_req->slot_h.mdh,
                            local_addr, 
                            size,
                            PTL_ACK_REQ,
                            remote,
                            srail->ptl_info.rndv_pt_idx,
                            rdv_match.raw,
                            (uint64_t)remote_key->pin.ptl.start + remote_offset,
                            ptl_comp,
                            0
                           ));

        rc = MPC_LOWCOMM_SUCCESS;
err:
        return rc;
}

int lcr_ptl_send_mget(_mpc_lowcomm_endpoint_t *ep,
                      uint64_t local_addr,
                      uint64_t remote_offset,
                      lcr_memp_t *remote_key,
                      size_t size,
                      lcr_tag_context_t *ctx) 
{
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote = infos->dest;
        sctk_ptl_pte_t *rdma_pte; 

        rdma_pte = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, 
                                                   remote_key->pin.ptl.match.raw),
                              remote, remote_offset, rdma_pte->idx, local_addr);

        sctk_ptl_chk(PtlGet(srail->ptl_info.md_req->slot_h.mdh,
                            local_addr,
                            size,
                            remote,
                            rdma_pte->idx,
                            remote_key->pin.ptl.match.raw,
                            remote_offset,
                            ctx	
                           ));

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_send_get(_mpc_lowcomm_endpoint_t *ep,
                     uint64_t local_addr,
                     uint64_t remote_offset,
                     lcr_memp_t *remote_key,
                     size_t size,
                     lcr_tag_context_t *ctx) 
{
        int rc;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote = infos->dest;
        sctk_ptl_matchbits_t rdv_match = SCTK_PTL_MATCH_INIT;
        lcr_ptl_send_comp_t *ptl_comp;

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p, "
                              "remote addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, 
                                                   remote_key->pin.ptl.match.raw),
                              remote, remote_offset, srail->ptl_info.rndv_pt_idx, 
                              local_addr, remote_key->pin.ptl.start);

        ptl_comp = sctk_malloc(sizeof(lcr_ptl_send_comp_t));
        if (ptl_comp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy comp");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ptl_comp, 0, sizeof(lcr_ptl_send_comp_t));
        ptl_comp->tag_ctx = ctx;

        sctk_ptl_chk(PtlGet(srail->ptl_info.md_req->slot_h.mdh,
                            local_addr,
                            size,
                            remote,
                            srail->ptl_info.rndv_pt_idx,
                            rdv_match.raw,
                            (uint64_t)remote_key->pin.ptl.start + remote_offset,
                            ptl_comp	
                           ));

        rc = MPC_LOWCOMM_SUCCESS;
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

int lcr_ptl_pack_mrkey(sctk_rail_info_t *rail,
                       lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        *(sctk_ptl_matchbits_t *)p = (sctk_ptl_matchbits_t)memp->pin.ptl.match;

        return sizeof(sctk_ptl_matchbits_t);
}

//FIXME: add const dest
int lcr_ptl_unpack_mrkey(sctk_rail_info_t *rail,
                         lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        /* deserialize data */
        //FIXME: warning generated by following line, start is void * while
        //       uint64_t 
        memp->pin.ptl.match = *(sctk_ptl_matchbits_t *)p;

        return sizeof(sctk_ptl_matchbits_t);
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
        //FIXME: warning generated by following line, start is void * while
        //       uint64_t 
        memp->pin.ptl.start = *(uint64_t *)p;

        return sizeof(uint64_t);
}

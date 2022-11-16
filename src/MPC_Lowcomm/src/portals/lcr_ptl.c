#include "lcr_ptl.h"

#include "sctk_ptl_iface.h"

#include <sctk_alloc.h>

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

        mpc_common_debug_info("lcr ptl: send bcopy to %d (iface=%llu, match=%s, remote=%llu, idx=%d, sz=%llu)", 
                              ep->dest, srail->iface, __sctk_ptl_match_str(malloc(32), 32, tag.t), remote, pte->idx, size);
        sctk_ptl_chk(PtlPut(srail->md_req->slot_h.mdh,
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

        mpc_common_debug_info("LCR PTL: send zcopy to %d (iface=%llu, match=%s, remote=%llu, idx=%d, sz=%llu, user_ptr=%p)", 
                              ep->dest, srail->iface, __sctk_ptl_match_str(malloc(32), 32, tag.t), remote, pte->idx, size, 
                              ctx);
        sctk_ptl_chk(PtlPut(srail->md_req->slot_h.mdh,
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

        mpc_common_debug_info("LCR PTL: send zcopy to %d (iface=%llu, remote=%llu, idx=%d, sz=%llu "
                              "match=%s)", ep->dest, srail->iface, remote, pte->idx, iov[0].iov_len,
                              __sctk_ptl_match_str(sctk_malloc(32), 32, match.raw));
        sctk_ptl_chk(PtlPut(srail->md_req->slot_h.mdh,
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

int lcr_ptl_send_put(_mpc_lowcomm_endpoint_t *ep,
                     uint64_t local_offset,
                     uint64_t remote_offset,
                     lcr_memp_t *local_key,
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
                              "remote off=%llu, local off=%llu, pte idx=%d, local addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, remote_key->pin.ptl.match.raw),
                              remote, local_offset, remote_offset, rdma_pte->idx, local_key->pin.ptl.start);

        sctk_ptl_chk(PtlPut(srail->md_req->slot_h.mdh,
                            (ptl_size_t)local_key->pin.ptl.start + local_offset, /* local offset */
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

int lcr_ptl_send_get(_mpc_lowcomm_endpoint_t *ep,
                     uint64_t local_offset,
                     uint64_t remote_offset,
                     lcr_memp_t *local_key,
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
                              "remote off=%llu, local off=%llu, pte idx=%d, local addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, remote_key->pin.ptl.match.raw),
                              remote, local_offset, remote_offset, rdma_pte->idx, local_key->pin.ptl.start);
	sctk_ptl_chk(PtlGet(
		srail->md_req->slot_h.mdh,
		(ptl_size_t)local_key->pin.ptl.start + local_offset,
		size,
		remote,
		rdma_pte->idx,
		remote_key->pin.ptl.match.raw,
		remote_offset,
	        ctx	
	));

        return MPC_LOWCOMM_SUCCESS;
}

//FIXME: add const dest
int lcr_ptl_pack_memp(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        *(sctk_ptl_matchbits_t *)p = (sctk_ptl_matchbits_t)memp->pin.ptl.match;

        return sizeof(sctk_ptl_matchbits_t);
}

//FIXME: add const dest
int lcr_ptl_unpack_memp(sctk_rail_info_t *rail,
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

int lcr_ptl_iface_progress(sctk_rail_info_t *rail)
{
	int did_poll = 0;
	int ret;
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
        sctk_ptl_local_data_t *user_ptr;
        lcr_tag_context_t *tag_ctx;
        sctk_ptl_pte_t *pte;

	ret = PtlEQGet(srail->mes_eq, &ev);
	sctk_ptl_chk(ret);

        if (ret == PTL_OK) {
		mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, match=%s,  sz=%llu, "
                                      "user=%p, start=%p", sctk_ptl_event_decode(ev), ev.pt_index, 
                                      __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), 
                                      ev.mlength, ev.user_ptr, ev.start);
                did_poll = 1;

                switch (ev.type) {
                case PTL_EVENT_SEND:
			not_reachable();
			break;
                case PTL_EVENT_REPLY:
                case PTL_EVENT_ACK:
                        tag_ctx  = (lcr_tag_context_t *)ev.user_ptr;
			tag_ctx->comp.sent = ev.mlength;
			tag_ctx->comp.comp_cb(&tag_ctx->comp);
                        break;
                case PTL_EVENT_PUT_OVERFLOW:
                case PTL_EVENT_GET_OVERFLOW:
                        pte = lcr_ptl_pte_idx_to_pte(srail, ev.pt_index);
			sctk_ptl_me_feed(srail, pte, srail->eager_limit, 1, 
                                         SCTK_PTL_OVERFLOW_LIST, 
                                         SCTK_PTL_TYPE_STD, 
                                         SCTK_PTL_PROT_NONE);
                        /* set up context */
                        user_ptr           = (sctk_ptl_local_data_t *)ev.user_ptr;
                        tag_ctx            = (lcr_tag_context_t *)user_ptr->msg;
                        tag_ctx->tag       = (lcr_tag_t)ev.match_bits;
                        tag_ctx->imm       = ev.hdr_data;
                        tag_ctx->start     = ev.start;
                        tag_ctx->comp.sent = ev.mlength;
                        tag_ctx->flags    |= LCR_IFACE_TM_OVERFLOW;

                        /* call completion callback */
			tag_ctx->comp.comp_cb(&tag_ctx->comp);

			sctk_free(user_ptr);
			break;
                case PTL_EVENT_PUT:
                case PTL_EVENT_GET:
                        user_ptr = (sctk_ptl_local_data_t *)ev.user_ptr;
                        tag_ctx  = (lcr_tag_context_t *)user_ptr->msg;
                        assert(tag_ctx);
                        /* set up context */
                        tag_ctx->tag       = (lcr_tag_t)ev.match_bits;
                        tag_ctx->imm       = ev.hdr_data;
                        tag_ctx->start     = ev.start;
                        tag_ctx->comp.sent = ev.mlength;
                        tag_ctx->flags    |= LCR_IFACE_TM_NOVERFLOW;

                        /* callback */
                        tag_ctx->comp.comp_cb(&tag_ctx->comp);

                        sctk_free(user_ptr);
                        break;
                case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
                case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
                        break;
                case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
                case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
                case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
                case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
                        /* probably nothing to do here */
                case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
                case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
                case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
                        not_reachable();              /* have been disabled */
                        break;
                default:
                        break;
                }
        }

        return did_poll;
}

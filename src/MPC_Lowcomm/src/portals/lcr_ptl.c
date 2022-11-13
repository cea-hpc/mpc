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
        int rc;
        sctk_ptl_local_data_t* request = NULL;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = NULL;
        size_t size                    = 0;
        int flags                      = 0;
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
        flags                  = SCTK_PTL_MD_PUT_FLAGS;
        match.raw              = tag.t;
        hdr.raw                = imm;
        pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
        remote                 = infos->dest;
        request                = sctk_ptl_md_create(srail, start, size, flags);

        assert(request);
        assert(pte);

        /* double-linking */
        request->msg           = ctx;
        request->type          = SCTK_PTL_TYPE_STD;

        /* emit the request */
        rc = lcr_ptl_md_register(srail, request);
        if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                mpc_common_debug("LCR PTL: bcopy no resource to %d (iface=%llu, "
                                 "remote=%llu, idx=%d, sz=%llu)", ep->dest, srail->iface, 
                                 remote, pte->idx, size);
                return MPC_LOWCOMM_NO_RESOURCE;
        }
        mpc_common_debug_info("LCR PTL: send bcopy to %d (iface=%llu, match=%s, remote=%llu, idx=%d, sz=%llu)", 
                              ep->dest, srail->iface, __sctk_ptl_match_str(malloc(32), 32, tag.t), remote, pte->idx, size);
        sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);

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
        int rc;
        sctk_ptl_local_data_t* request = NULL;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = iov->iov_base;
        size_t size                    = iov->iov_len;
        int flags                      = 0;
        sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
        sctk_ptl_pte_t* pte            = NULL;
        sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_imm_data_t hdr;

        //FIXME: support for only one iovec
        assert(iovcnt == 1);

        /* prepare the Put() MD */
        flags                  = SCTK_PTL_MD_PUT_FLAGS;
        match.raw              = tag.t;
        hdr.raw                = imm;
        pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
        remote                 = infos->dest;
        request                = sctk_ptl_md_create(srail, start, size, flags);

        assert(request);
        assert(pte);

        /* double-linking */
        request->msg           = ctx;
        request->type          = SCTK_PTL_TYPE_STD;

        /* emit the request */
        rc = lcr_ptl_md_register(srail, request);
        if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                mpc_common_debug("LCR PTL: zcopy in progress to %d (iface=%llu, remote=%llu, "
                                 "idx=%d, sz=%llu)", ep->dest, srail->iface, remote,  pte->idx, 
                                 size);
                return MPC_LOWCOMM_NO_RESOURCE;
        }
        mpc_common_debug_info("LCR PTL: send zcopy to %d (iface=%llu, match=%s, remote=%llu, idx=%d, sz=%llu, user_ptr=%p)", 
                              ep->dest, srail->iface, __sctk_ptl_match_str(malloc(32), 32, tag.t), remote, pte->idx, size, 
                              request);
        sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);

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
        int rc;
        sctk_ptl_local_data_t* md_request = NULL;
        sctk_ptl_local_data_t* me_request = NULL;
        sctk_ptl_rail_info_t* srail       = &ep->rail->network.ptl;
        int md_flags, me_flags            = 0;
        sctk_ptl_matchbits_t match        = SCTK_PTL_MATCH_INIT;
        sctk_ptl_matchbits_t ign          = SCTK_PTL_MATCH_INIT;
        sctk_ptl_pte_t* pte, *rdma_pte    = NULL;
        sctk_ptl_id_t remote              = SCTK_PTL_ANY_PROCESS;
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_imm_data_t hdr;

        assert(iovcnt == 2);

        /* prepare the Put() MD */
        md_flags               = SCTK_PTL_MD_PUT_FLAGS;
        match.raw              = tag.t;
        hdr.raw                = imm;
        pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
        remote                 = infos->dest;
        md_request             = sctk_ptl_md_create(srail, NULL, 0, md_flags);

        assert(md_request);
        assert(pte);

        /* emit the request */
        md_request->msg = ctx;
        md_request->type = SCTK_PTL_TYPE_STD;
        rc = lcr_ptl_md_register(srail, md_request);
        if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                mpc_common_debug("LCR PTL: zcopy in progress to %d (iface=%llu, remote=%llu, "
                                 "idx=%d, sz=%llu)", ep->dest, srail->iface, remote,  pte->idx, 
                                 iov[1].iov_len);
                return MPC_LOWCOMM_NO_RESOURCE;
        }

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
        sctk_ptl_chk(PtlPut(md_request->slot_h.mdh,
                            0, /* local offset */
                            iov[0].iov_len,
                            PTL_ACK_REQ,
                            remote,
                            pte->idx,
                            match.raw,
                            0, /* remote offset */
                            md_request,
                            hdr.raw	
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
        pte      = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
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
                     uint64_t local_off,
                     uint64_t remote_off,
                     lcr_memp_t *local_key,
                     lcr_memp_t *remote_key,
                     size_t size,
                     lcr_completion_t *comp) 
{
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote = infos->dest;
        sctk_ptl_pte_t *rdma_pte; 

        rdma_pte = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);

        local_key->pin.ptl.md_data->msg = comp; 

        sctk_ptl_emit_put(local_key->pin.ptl.md_data, /* The base MD */
                          size,                      /* request size */
                          remote,                    /* target process */
                          rdma_pte,                  /* Portals entry */
                          remote_key->pin.ptl.match,  /* match bits */
                          local_off, remote_off,     /* offsets */
                          0,                         /* Number of bytes sent */
                          local_key->pin.ptl.md_data 
                         );

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_send_get(_mpc_lowcomm_endpoint_t *ep,
                     uint64_t src_off,
                     uint64_t dest_off,
                     lcr_memp_t *local_key,
                     lcr_memp_t *remote_key,
                     size_t size,
                     lcr_completion_t *comp) 
{
        _mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
        sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        sctk_ptl_id_t remote = infos->dest;
        sctk_ptl_pte_t *rdma_pte; 

        rdma_pte = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);

        /* set the completion object to be called in md_poll */
        local_key->pin.ptl.md_data->msg = comp; 

        mpc_common_debug_info("PTL: remote key. match=%s, remote=%llu, "
                              "remote off=%llu, local off=%llu, pte idx=%d, local addr=%p", 
                              __sctk_ptl_match_str(sctk_malloc(32), 32, remote_key->pin.ptl.match.raw),
                              remote, src_off, dest_off, rdma_pte->idx, local_key->pin.ptl.start);

        sctk_ptl_emit_get(local_key->pin.ptl.md_data, /* The base MD */
                          size,                      /* request size */
                          remote,                    /* target process */
                          rdma_pte,                  /* Portals entry */
                          remote_key->pin.ptl.match,  /* match bits */
                          src_off, dest_off,         /* offsets */
                          local_key->pin.ptl.md_data  
                         );

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

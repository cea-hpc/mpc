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

#include "ptl_ep.h"

#include "ptl_iface.h"
#include "ptl_types.h"

#include <sctk_alloc.h>
#include <alloca.h>
#include <uthash.h>

#include "mpc_launch_pmi.h"
#include "rail.h"

//FIXME: add safeguard for msg size
ssize_t lcr_ptl_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              unsigned flags)
{
        UNUSED(flags);
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        void* start                = NULL;
        size_t size                = 0;
        ptl_match_bits_t match     = 0;
        lcr_ptl_ep_info_t* infos   = &ep->data.ptl;
        lcr_ptl_addr_t remote      = infos->addr;
        lcr_ptl_op_t *op;

        assert(remote.pts.am != LCR_PTL_PT_NULL);

        //FIXME: use a memory pool.
        start = sctk_malloc(srail->config.eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        assert(size <= srail->config.eager_limit);

        op = mpc_mpool_pop(srail->ops_pool); 
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding operations %d");
                size = -1;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY;
        op->size         = size;
        op->am.bcopy_buf = start;

        mpc_common_debug_info("LCR PTL: send am bcopy to %d, id=%d (iface=%llu, "
                              "remote=%llu, idx=%llu, sz=%llu, user_ptr=%p)", 
                              (int)ep->dest, id, srail->nih, remote.id, remote.pts.am, 
                              size, op);
        lcr_ptl_chk(PtlPut(srail->am_ctx.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote.id,
                            remote.pts.am,
                            match,
                            0, /* remote offset */
                            op, 
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
        lcr_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        size_t size                    = 0;
        ptl_match_bits_t match     = 0;
        lcr_ptl_ep_info_t* infos   = &ep->data.ptl;
        lcr_ptl_addr_t remote          = infos->addr;
	ptl_md_t iov_md;

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type     = LCR_PTL_OP_AM_ZCOPY;
        op->comp = comp;

        op->iov.iov  = sctk_malloc((iovcnt + 1) * sizeof(ptl_iovec_t));
        if (op->iov.iov == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate ptl iov.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        assert(iovcnt <= (size_t)srail->config.max_iovecs);

        /* Fill up IOV structure and compute total size. */
        op->iov.iov[0].iov_len  = header_length;
        op->iov.iov[0].iov_base = (void *)header;
        size += header_length;

        for (i=0; i<(int)iovcnt; i++) {
                op->iov.iov[1 + i].iov_base = iov[i].iov_base;
                size += op->iov.iov[1 + i].iov_len = iov[i].iov_len;
        }

        /* Set size. */
        op->size = size;

        assert(iovcnt + 1 <= (size_t)srail->config.max_iovecs);
        assert(remote.pts.am != LCR_PTL_PT_NULL);

        iov_md = (ptl_md_t){
                .start = op->iov.iov,
                .length = iovcnt + 1,
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->am_ctx.eqh,
                .options = PTL_MD_EVENT_SEND_DISABLE | PTL_IOVEC,
        };

	lcr_ptl_chk(PtlMDBind(
		srail->nih,      /* the NI handler */
		&iov_md,           /* the MD to bind with memory region */
		&op->iov.iovh /* out: the MD handler */
	)); 

        lcr_ptl_chk(PtlPut(op->iov.iovh,
                            0,
                            size,
                            PTL_ACK_REQ,
                            remote.id,
                            remote.pts.am,
                            match,
                            0,
                            op,
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
        lcr_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        void* start                    = NULL;
        size_t size                    = 0;
        lcr_ptl_ep_info_t* infos   = &ep->data.ptl;
        lcr_ptl_addr_t remote          = infos->addr;

        start = sctk_malloc(srail->config.eager_limit);
        size = pack(start, arg);

        assert(size <= srail->config.eager_limit);
        assert(remote.pts.tag != LCR_PTL_PT_NULL);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }

        op->type          = LCR_PTL_OP_TAG_BCOPY;
        op->tag.bcopy_buf = start;
        op->comp          = NULL; // No completion for bcopy

        mpc_common_debug_info("LCR PTL: send tag bcopy to %d (iface=%llu, "
                              "match=[%d,%d,%d], remote=%llu, idx=%llu, sz=%llu, "
                              "user_ptr=%p)", (int)ep->dest, srail->nih, tag.t_tag.tag, 
                              tag.t_tag.src, tag.t_tag.comm, remote.id, remote.pts.tag, 
                              size, op);
        lcr_ptl_chk(PtlPut(srail->tag_ctx.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote.id,
                            remote.pts.tag,
                            tag.t,
                            0, /* remote offset */
                            op,
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
        lcr_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        size_t size                    = iov->iov_len;
        lcr_ptl_ep_info_t* infos   = &ep->data.ptl;
        lcr_ptl_addr_t remote          = infos->addr;

        //FIXME: support for only one iovec
        assert(iovcnt == 1);
        assert(remote.pts.tag != LCR_PTL_PT_NULL);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }

        op->type      = LCR_PTL_OP_TAG_ZCOPY;
        op->size      = iov[0].iov_len;
        op->comp      = comp;

        mpc_common_debug_info("LCR PTL: send tag zcopy to %d (iface=%llu, "
                              "match=[%d,%d,%d], remote=%llu, idx=%llu, sz=%llu, "
                              "user_ptr=%p)", (int)ep->dest, srail->nih, tag.t_tag.tag, 
                              tag.t_tag.src, tag.t_tag.comm, remote.id, remote.pts.tag, 
                              size, op);
        lcr_ptl_chk(PtlPut(srail->tag_ctx.mdh,
                            (ptl_size_t) iov[0].iov_base, /* local offset */
                            iov[0].iov_len,
                            PTL_ACK_REQ,
                            remote.id,
                            remote.pts.tag,
                            tag.t,
                            0, /* remote offset */
                            op,
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
        lcr_ptl_rail_info_t* srail     = &rail->network.ptl;

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
                        .options = SCTK_PTL_ME_PUT_FLAGS | 
                                SCTK_PTL_ME_GET_FLAGS    |
                                persistant,
        };

        mpc_common_debug_info("LCR PTL: post recv zcopy to %p (idx=%d, tag=%llu, "
                              "sz=%llu, buf=%p)", rail, srail->tag_ctx.pti, tag.t,
                              iov[0].iov_len, iov[0].iov_base);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }

        op->type        = search ? LCR_PTL_OP_TAG_SEARCH : 
                LCR_PTL_OP_TAG_ZCOPY;
        op->size        = iov[iovcnt-1].iov_len;
        op->comp        = &ctx->comp;
        op->tag.tag_ctx = ctx;

        if (search) {
                lcr_ptl_chk(PtlMESearch(srail->nih,
                                         srail->tag_ctx.pti,
                                         &me,
                                         PTL_SEARCH_ONLY,
                                         op));
        } else {
                lcr_ptl_chk(PtlMEAppend(srail->nih,
                                        srail->tag_ctx.pti,
                                        &me,
                                        PTL_PRIORITY_LIST,
                                        op,
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
                HASH_ADD(hh, srail->persistant_ht, tag_key, sizeof(uint64_t),
                         persistant_post);
        }

err:
        return rc;
}

int lcr_ptl_unpost_tag_zcopy(sctk_rail_info_t *rail, lcr_tag_t tag)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_persistant_post_t *persistant_post;
        lcr_ptl_rail_info_t* srail     = &rail->network.ptl;

        HASH_FIND(hh, srail->persistant_ht, &tag.t, sizeof(uint64_t),
                  persistant_post);
        if (persistant_post == NULL) {
                mpc_common_debug_error("LCR PTL: could not find posted "
                                       "element with tag: %llu", tag.t);
                rc = MPC_LOWCOMM_ERROR;
                return rc;
        }

        lcr_ptl_chk(PtlMEUnlink(persistant_post->meh));

        return rc;
}

int lcr_ptl_send_put_bcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_pack_callback_t pack,
                           void *arg,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key)
{
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        void* start                = NULL;
        size_t size                = 0;
        ptl_match_bits_t match     = 0;
        lcr_ptl_rma_handle_t *lkey = &local_key->pin.ptl;
        lcr_ptl_ep_info_t* infos   = &ep->data.ptl;
        lcr_ptl_addr_t remote      = infos->addr;

        //TODO: create a pool of pre-registered MD.
        not_implemented();

        start = sctk_malloc(srail->config.eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        remote = infos->addr;

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY; 
        op->size         = size;
        op->comp         = NULL;
        op->am.bcopy_buf = start;

        mpc_common_debug_info("lcr ptl: send put bcopy to %d ("
                              "remote=%llu, sz=%llu, pte=%d)", ep->dest, 
                              remote, size, srail->os_ctx.pti);

        //FIXME: there are questions around the completion of bcopy requests.
        //       With Portals, how should a request be completed?
        lcr_ptl_chk(PtlPut(lkey->mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            remote.id,
                            remote.pts.rma,
                            match,
                            (uint64_t)remote_key->pin.ptl.start + remote_offset,
                            op, 
                            0
                           ));

err:
        return size;
}

int lcr_ptl_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_ep_info_t* infos = &ep->data.ptl;
        lcr_ptl_addr_t remote          = infos->addr;
        lcr_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        ptl_match_bits_t rdv_match = 0;
        lcr_ptl_rma_handle_t *lkey = &local_key->pin.ptl;
        lcr_ptl_rma_handle_t *rkey = &remote_key->pin.ptl;

        mpc_common_debug_info("PTL: remote key. remote start=%p remote=%llu, "
                              "remote off=%llu, pte idx=%d, local addr=%p, "
                              "remote addr=%p", rkey->start,
                              remote.id, remote_offset, srail->os_ctx.pti, 
                              local_addr, remote_key->pin.ptl.start);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY; 
        op->size         = size;
        op->comp         = comp;

        /* Push operation to targeted rma handle list. */
        mpc_common_spinlock_lock(&lkey->ops_lock);
        mpc_queue_push(&lkey->ops, &op->elem);
        mpc_common_spinlock_unlock(&lkey->ops_lock);

        lcr_ptl_chk(PtlPut(lkey->mdh, local_addr, 
                            size, PTL_ACK_REQ, remote.id,
                            remote.pts.rma, rdv_match,
                            (uint64_t)remote_key->pin.ptl.start + remote_offset,
                            op, 0
                           ));
err:
        return rc;
}

int lcr_ptl_send_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_offset,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_ep_info_t* infos = &ep->data.ptl;
        lcr_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
        lcr_ptl_addr_t remote          = infos->addr;
        ptl_match_bits_t rdv_match = 0;
        lcr_ptl_rma_handle_t *lkey = &local_key->pin.ptl;
        lcr_ptl_rma_handle_t *rkey = &remote_key->pin.ptl;

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY; 
        op->size         = size;
        op->comp         = comp;

        mpc_common_spinlock_lock(&lkey->ops_lock);
        mpc_queue_push(&lkey->ops, &op->elem);
        mpc_common_spinlock_unlock(&lkey->ops_lock);

        mpc_common_debug("LCR PTL: op.         size=%llu, addr=%p, offset=%llu.",
                         op->size, local_offset, remote_offset);
        mpc_common_debug("LCR PTL: local key.  size=%llu, addr=%p, cth=%llu, mdh=%llu.",
                         lkey->size, lkey->start, lkey->cth, lkey->mdh);
        mpc_common_debug("LCR PTL: remote key. addr=%p",   rkey->start);

        lcr_ptl_chk(PtlGet(lkey->mdh,
                            local_offset,
                            size,
                            remote.id,
                            remote.pts.rma,
                            rdv_match,
                            (uint64_t)rkey->start + remote_offset,
                            NULL	
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
        lcr_ptl_ep_info_t* infos    = &ep->data.ptl;
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        lcr_ptl_addr_t remote       = infos->addr;

        mpc_common_debug("PTL: get tag remote key. "
                         "iface=%p, remote=%llu, remote off=%llu, pte idx=%d, "
                         "local addr=%p", ep->rail, remote, remote_offset, 
                         srail->tag_ctx.pti, local_offset);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY; 
        op->size         = size;
        op->comp         = comp;

        lcr_ptl_chk(PtlGet(srail->tag_ctx.mdh,
                            local_offset,
                            size,
                            remote.id,
                            remote.pts.tag,
                            tag.t,
                            remote_offset,
                            op	
                           ));

err:
        return rc;

}

void lcr_ptl_mem_register(struct sctk_rail_info_s *rail, 
                          struct sctk_rail_pin_ctx_list *list, 
                          void * addr, size_t size)
{
        ptl_md_t md;

        lcr_ptl_rail_info_t* srail   = &rail->network.ptl;
        lcr_ptl_rma_handle_t *mem = &list->pin.ptl;

        /* Initialize RMA counter. */
        lcr_ptl_chk(PtlCTAlloc(srail->nih, &mem->cth));

        /* Initialize queue of operations on RMA handle. */ 
        mpc_queue_init_head(&mem->ops);
        mpc_common_spinlock_init(&mem->ops_lock, 0);

        /* Prepare PTL memory descriptor. */
        md = (ptl_md_t) {
                .ct_handle = mem->cth,
                .eq_handle = PTL_EQ_NONE,
                .length = size,
                .start  = addr,
                .options = PTL_MD_EVENT_SEND_DISABLE |
                        PTL_MD_EVENT_CT_ACK          | 
                        PTL_MD_EVENT_CT_REPLY,
        };
        lcr_ptl_chk(PtlMDBind(srail->nih, &md, &mem->mdh));

        mem->size  = size;
        mem->start = addr; 
        mem->completed_ops = 0;

        /* Push RMA handle to list of active handles. */
        mpc_common_spinlock_lock(&srail->os_ctx.rmah_lock);
        mpc_list_push_tail(&srail->os_ctx.rmah_head, &mem->elem);
        mpc_common_spinlock_unlock(&srail->os_ctx.rmah_lock);

        mpc_common_debug("LCR PTL: memory registration. cth=%llu, mdh=%llu, "
                         "addr=%p, size=%llu", mem->cth, mem->mdh, 
                         addr, size);
}

void lcr_ptl_mem_unregister(struct sctk_rail_info_s *rail, 
                            struct sctk_rail_pin_ctx_list *list)
{
        lcr_ptl_rail_info_t* srail = &rail->network.ptl;
        lcr_ptl_rma_handle_t *mem  = &list->pin.ptl;

        lcr_ptl_chk(PtlCTFree(mem->cth));

        lcr_ptl_chk(PtlMDRelease(mem->mdh));

        /* Remove RMA handle from active list. */
        mpc_common_spinlock_lock(&srail->os_ctx.rmah_lock);
        mpc_list_del(&mem->elem);
        mpc_common_spinlock_unlock(&srail->os_ctx.rmah_lock);
        
        return;
}

int lcr_ptl_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        /* Serialize data. */
        *(uint64_t *)p = (uint64_t)memp->pin.ptl.start;

        return sizeof(uint64_t);
}

int lcr_ptl_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        /* deserialize data */
        memp->pin.ptl.start = (void *)(*(uint64_t *)p);

        return sizeof(uint64_t);
}


static lcr_ptl_addr_t __map_id_pmi(sctk_rail_info_t* rail, 
                                   mpc_lowcomm_peer_uid_t dest, 
                                   int *found)
{
	char connection_infos[MPC_COMMON_MAX_STRING_SIZE];
        lcr_ptl_addr_t out_id;

	/* retrieve the right neighbour id struct */
        int tmp_ret = 
                mpc_launch_pmi_get_as_rank (connection_infos, /* the recv buffer */
                                            MPC_COMMON_MAX_STRING_SIZE, /* the recv buffer max size */
                                            rail->rail_number, /* rail IB: PMI tag */
                                            mpc_lowcomm_peer_get_rank(dest) /* Target process. */
                                           );

	if(tmp_ret == MPC_LAUNCH_PMI_SUCCESS) {
		*found = 1;

		lcr_ptl_data_deserialize(
				connection_infos, /* the buffer containing raw data */
				&out_id, /* the target struct */
				sizeof(lcr_ptl_addr_t)      /* target struct size */
				);

		return out_id;
	}

	*found = 0;
	return LCR_PTL_ADDR_ANY;
}

static lcr_ptl_addr_t __map_id_monitor(sctk_rail_info_t* rail, mpc_lowcomm_peer_uid_t dest)
{
        mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

        char rail_name[32];

        mpc_lowcomm_monitor_response_t resp = 
                mpc_lowcomm_monitor_ondemand(dest,
                                             __ptl_get_rail_callback_name(rail->rail_number, 
                                                                          rail_name, 32),
                                             NULL,
                                             0,
                                             &ret);

        if(ret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
        {
                mpc_common_debug_fatal("Could not connect to UID %lu", dest);
        }

        mpc_lowcomm_monitor_args_t * content = mpc_lowcomm_monitor_response_get_content(resp);

        mpc_common_debug("LCR PTL: OD got %s", content->on_demand.data);
        lcr_ptl_addr_t out_id;

        lcr_ptl_data_deserialize(content->on_demand.data, /* the buffer containing raw data */
                                  &out_id, /* the target struct */
                                  sizeof (lcr_ptl_addr_t)      /* target struct size */
                                 );

        mpc_lowcomm_monitor_response_free(resp);

        return out_id;
}

static void __ranks_ids_map_set(struct mpc_common_hashtable *ranks_ids_map, 
                                       mpc_lowcomm_peer_uid_t dest, lcr_ptl_addr_t id)
{
	/* We prefer to do this to ensure we have no storage size problem
	   as our hashtable is a pointer storage only */
	lcr_ptl_addr_t * new = sctk_malloc(sizeof(lcr_ptl_addr_t));
	*new = id;
	mpc_common_hashtable_set(ranks_ids_map, dest, new);
}

static lcr_ptl_addr_t __ranks_ids_map_get(struct mpc_common_hashtable *ranks_ids_map,
                                                mpc_lowcomm_peer_uid_t dest)
{
	lcr_ptl_addr_t * ret = mpc_common_hashtable_get(ranks_ids_map, dest);

	if(!ret)
	{
		return LCR_PTL_ADDR_ANY;
	}

	return *ret;
}


lcr_ptl_addr_t lcr_ptl_map_id(sctk_rail_info_t* rail, mpc_lowcomm_peer_uid_t dest)
{
	lcr_ptl_rail_info_t* srail    = &rail->network.ptl;

	if(LCR_PTL_IS_ANY_PROCESS(__ranks_ids_map_get(&srail->ranks_ids_map, dest) ))
	{
		lcr_ptl_addr_t out_id = LCR_PTL_ADDR_ANY;

		int found = 0;
		if( (mpc_lowcomm_peer_get_set(dest) == mpc_lowcomm_monitor_get_gid()) 
                    && mpc_launch_pmi_is_initialized() )
		{
			/* If target belongs to our set try PMI first */
			out_id = __map_id_pmi(rail, dest, &found);
		}

		if(!found)
		{
			/* Here we need to use the cplane
			   to request the info remotely */
			out_id = __map_id_monitor(rail, dest);
		}

		__ranks_ids_map_set(&srail->ranks_ids_map, dest, out_id);
	}

	lcr_ptl_addr_t ret = __ranks_ids_map_get(&srail->ranks_ids_map, dest);
	assert(!LCR_PTL_IS_ANY_PROCESS(ret));
	return ret;
}

void lcr_ptl_add_route(mpc_lowcomm_peer_uid_t dest, lcr_ptl_addr_t id, 
                       sctk_rail_info_t* rail, _mpc_lowcomm_endpoint_type_t origin, 
                       _mpc_lowcomm_endpoint_state_t state)
{
	_mpc_lowcomm_endpoint_t * route;
	lcr_ptl_rail_info_t* srail = &rail->network.ptl;

	route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
	assert(route);

	_mpc_lowcomm_endpoint_init(route, dest, rail, origin);
	_mpc_lowcomm_endpoint_set_state(route, state);

	route->data.ptl.addr = id;
	__ranks_ids_map_set(&srail->ranks_ids_map, dest, id);

	if(origin == _MPC_LOWCOMM_ENDPOINT_STATIC)
	{
		sctk_rail_add_static_route (  rail, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, route );
	}
}

void lcr_ptl_connect_on_demand(struct sctk_rail_info_s *rail, 
                               mpc_lowcomm_peer_uid_t dest)
{
	lcr_ptl_addr_t id = lcr_ptl_map_id(rail, dest);
	lcr_ptl_add_route(dest, id, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC, 
                          _MPC_LOWCOMM_ENDPOINT_CONNECTED);
}

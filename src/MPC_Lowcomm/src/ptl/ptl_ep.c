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

#include "ptl.h"

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
        int rc;
        UNUSED(flags);
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        void* start                = NULL;
        size_t size                = 0;
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;
        lcr_ptl_op_t *op;

        assert(txq->addr.pte.am != LCR_PTL_PT_NULL);

        //FIXME: use a memory pool.
        start = sctk_malloc(srail->config.eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer.");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        assert(size <= srail->config.eager_limit);

        op = mpc_mpool_pop(txq->ops_pool); 
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding operations %d.");
                size = -1;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->am.mdh, 
                                txq->addr.id, 
                                txq->addr.pte.am, 
                                LCR_PTL_OP_AM_BCOPY, 
                                size, NULL, 
                                txq);

        op->am.am_id     = id;
        op->am.bcopy_buf = start;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        int token_num = 1;
        LCR_PTL_AM_HDR_SET(op->hdr, op->am.am_id, 
                           op->txq->idx, 
                           op->txq->num_ops);
        rc = lcr_ptl_post(txq, op, &token_num);

        if (lcr_ptl_txq_needs_tokens(txq, token_num)) {
                lcr_ptl_op_t *tk_op;
                rc = lcr_ptl_create_token_request(srail, txq, &tk_op);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                assert(txq->is_waiting == 1);

                rc = lcr_ptl_do_op(tk_op);
        }
#else 
        op->hdr = (uint64_t)op->am.am_id;
        rc = lcr_ptl_do_op(op);
#endif


        if (rc != MPC_LOWCOMM_SUCCESS) {
                size = -1;
        }

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
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        size_t size                = 0;
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;
	ptl_md_t iov_md;

        lcr_ptl_op_t *op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        /* Memory Descriptor handle and size are set later. */
        _lcr_ptl_init_op_common(op, 0, 0, 
                                txq->addr.id, 
                                txq->addr.pte.am, 
                                LCR_PTL_OP_AM_ZCOPY, 
                                0, comp, 
                                txq);

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
        assert(txq->addr.pte.am != LCR_PTL_PT_NULL);

        iov_md = (ptl_md_t){
                .start = op->iov.iov,
                .length = iovcnt + 1,
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->eqh,
                .options = PTL_MD_EVENT_SEND_DISABLE | PTL_IOVEC,
        };

        /* Set Memory Descriptor handle. */
	lcr_ptl_chk(PtlMDBind(
		srail->nih,      /* the NI handler */
		&iov_md,           /* the MD to bind with memory region */
		&op->mdh /* out: the MD handler */
	)); 

        op->am.am_id = id;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        int token_num = 1;
        LCR_PTL_AM_HDR_SET(op->hdr, op->am.am_id, 
                           op->txq->idx, 
                           op->txq->num_ops);
        rc = lcr_ptl_post(txq, op, &token_num);

        if (lcr_ptl_txq_needs_tokens(txq, token_num)) {
                lcr_ptl_op_t *tk_op;
                rc = lcr_ptl_create_token_request(srail, txq, &tk_op);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                assert(txq->is_waiting == 1);

                rc = lcr_ptl_do_op(tk_op);
        }
#else 
        op->hdr = (uint64_t)op->am.am_id;
        rc = lcr_ptl_do_op(op);
#endif

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
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        not_implemented();
#endif

        start = sctk_malloc(srail->config.eager_limit);
        size = pack(start, arg);

        assert(size <= srail->config.eager_limit);
        assert(txq->addr.pte.tag != LCR_PTL_PT_NULL);

        lcr_ptl_op_t *op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->tag.mdh, 
                                txq->addr.id, 
                                txq->addr.pte.tag, 
                                LCR_PTL_OP_TAG_BCOPY, 
                                size, NULL, 
                                txq);

        op->tag.bcopy_buf = start;

        //FIXME: move to _lcr_ptl_do_op
        lcr_ptl_chk(PtlPut(srail->tag.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            txq->addr.id,
                            txq->addr.pte.tag,
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
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;

        //FIXME: support for only one iovec
        assert(iovcnt == 1);
        assert(txq->addr.pte.tag != LCR_PTL_PT_NULL);

        lcr_ptl_op_t *op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->tag.mdh, 
                                txq->addr.id, 
                                txq->addr.pte.tag, 
                                LCR_PTL_OP_TAG_ZCOPY, 
                                iov[0].iov_len, comp, 
                                txq);

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        not_implemented();
#endif

        //FIXME: move to _lcr_ptl_do_op
        lcr_ptl_chk(PtlPut(srail->tag.mdh,
                            (ptl_size_t) iov[0].iov_base, /* local offset */
                            iov[0].iov_len,
                            PTL_ACK_REQ,
                            txq->addr.id,
                            txq->addr.pte.tag,
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
        unsigned int search = flags & LCR_IFACE_TM_SEARCH;
        lcr_ptl_rail_info_t* srail     = &rail->network.ptl;

        assert(iov && iovcnt == 1);

        /* complete the ME data, this ME will be appended to the PRIORITY_LIST */
        me = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                        .ignore_bits = ign_tag.t,
                        .match_bits  = tag.t,
                        .match_id = { 
                                .phys.nid = PTL_NID_ANY,
                                .phys.pid = PTL_PID_ANY
                        },
                        .min_free = 0,
                        .length = iov[iovcnt-1].iov_len,
                        .start = iov[iovcnt-1].iov_base,
                        .uid = PTL_UID_ANY,
                        .options = SCTK_PTL_ME_PUT_FLAGS | 
                                SCTK_PTL_ME_GET_FLAGS    |
                                PTL_ME_USE_ONCE,
        };

        mpc_common_debug_info("LCR PTL: post recv zcopy to %p (idx=%d, tag=%llu, "
                              "sz=%llu, buf=%p)", rail, srail->tag.pti, tag.t,
                              iov[0].iov_len, iov[0].iov_base);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->iface_ops);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, 0, 
                                LCR_PTL_PROCESS_ANY, 
                                srail->tag.pti, 
                                search ? LCR_PTL_OP_TAG_SEARCH : 
                                LCR_PTL_OP_TAG_ZCOPY, 
                                iov[iovcnt - 1].iov_len, &ctx->comp, 
                                NULL);

        //FIXME: move to _lcr_ptl_do_op
        if (search) {
                lcr_ptl_chk(PtlMESearch(srail->nih,
                                         srail->tag.pti,
                                         &me,
                                         PTL_SEARCH_ONLY,
                                         op));
        } else {
                lcr_ptl_chk(PtlMEAppend(srail->nih,
                                        srail->tag.pti,
                                        &me,
                                        PTL_PRIORITY_LIST,
                                        op,
                                        &op->tag.meh 
                                       ));
        }

err:
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
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;
        lcr_ptl_mem_t *lkey = &local_key->pin.ptl;
        lcr_ptl_mem_t *rkey = &remote_key->pin.ptl;

        //TODO: create a pool of pre-registered MD.
        not_implemented();

        start = sctk_malloc(srail->config.eager_limit);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer");
                size = -1;
                goto err;
        }
        size = pack(start, arg);

        lcr_ptl_op_t *op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                txq->addr.id, 
                                txq->addr.pte.rma, 
                                LCR_PTL_OP_RMA_PUT, 
                                size, NULL, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.local_offset  = 0;
        op->rma.remote_offset = remote_offset;

        mpc_common_debug_info("lcr ptl: send put bcopy to %%llu remote nid=%llu, "
                              "pid=%llu, sz=%llu, pte=%d.", ep->dest, 
                              txq->addr.id.phys.nid, txq->addr.id.phys.pid, size, 
                              srail->rma.pti);

        lcr_ptl_do_op(op);

        /* Increment operation counts. */
        atomic_fetch_add(&op->rma.lkey->op_count, 1);
err:
        return size;
}

int lcr_ptl_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_offset,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;
        lcr_ptl_mem_t *lkey = &local_key->pin.ptl;
        lcr_ptl_mem_t *rkey = &remote_key->pin.ptl;
        lcr_ptl_op_t *op           = NULL;         

        //NOTE: Operation identifier and Put must be done atomically to make
        //      sure the operation is pushed to the NIC with the correct
        //      sequence number.
        op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "ptl operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                txq->addr.id, 
                                txq->addr.pte.rma, 
                                LCR_PTL_OP_RMA_PUT, 
                                size, comp, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.match         = rkey->match;
        op->rma.local_offset  = local_offset;
        op->rma.remote_offset = remote_offset;

        /* Only if completion was requested, push operation to the TX queue. */
        if (comp != NULL) {
                mpc_common_spinlock_lock(&txq->lock);
                mpc_queue_push(&txq->completion_queue, &op->elem);
                mpc_common_spinlock_unlock(&txq->lock);
        }

        lcr_ptl_do_op(op);

        /* Increment operation counts. */
        atomic_fetch_add(&lkey->op_count, 1);
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
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;
        lcr_ptl_mem_t *lkey = &local_key->pin.ptl;
        lcr_ptl_mem_t *rkey = &remote_key->pin.ptl;
        lcr_ptl_op_t *op           = NULL; 

        //NOTE: Operation identifier and Put must be done atomically to make
        //      sure the operation is pushed to the NIC with the correct
        //      sequence number.
        /* Increment global operation sequence number and store to resources. */
        op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "ptl operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                txq->addr.id, 
                                txq->addr.pte.rma, 
                                LCR_PTL_OP_RMA_GET, 
                                size, comp, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.match         = rkey->match;
        op->rma.local_offset  = local_offset;
        op->rma.remote_offset = remote_offset;

        /* Only if completion was requested, push operation to the TX queue. */
        if (comp != NULL) {
                mpc_common_spinlock_lock(&txq->lock);
                mpc_queue_push(&txq->completion_queue, &op->elem);
                mpc_common_spinlock_unlock(&txq->lock);
        }

        lcr_ptl_do_op(op);
        /* Increment operation counts. */
        atomic_fetch_add(&op->rma.lkey->op_count, 1);
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
        lcr_ptl_txq_t* txq         = ep->data.ptl.txq;
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;

        mpc_common_debug("PTL: get tag remote key. "
                         "iface=%p, remote=%llu, remote off=%llu, pte idx=%d, "
                         "local addr=%p", ep->rail, txq->addr, remote_offset, 
                         srail->tag.pti, local_offset);

        lcr_ptl_op_t *op = mpc_mpool_pop(txq->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY; 
        op->size         = size;
        op->comp         = comp;

        lcr_ptl_chk(PtlGet(srail->tag.mdh,
                            local_offset,
                            size,
                            txq->addr.id,
                            txq->addr.pte.tag,
                            tag.t,
                            remote_offset,
                            op	
                           ));

err:
        return rc;

}

int lcr_ptl_ep_flush(_mpc_lowcomm_endpoint_t *ep,
                     unsigned flags)
{
        UNUSED(ep);
        UNUSED(flags);

        return 0;
}

int lcr_ptl_mem_flush(sctk_rail_info_t *rail,
                      struct sctk_rail_pin_ctx_list *list,
                      lcr_completion_t *comp,
                      unsigned flags) 
{
        UNUSED(flags);
        int rc = MPC_LOWCOMM_SUCCESS;
        ptl_size_t op_done, op_count;
        lcr_ptl_op_t *op;
        lcr_ptl_mem_t *lkey = &list->pin.ptl;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;

        op_count = lkey->op_count;
        assert(op_count < UINT64_MAX);
        op_done = lcr_ptl_poll_mem(lkey);

        if (op_done >= op_count) {
                rc = MPC_LOWCOMM_SUCCESS;
        } else {
                op = mpc_mpool_pop(srail->iface_ops);
                if (op == NULL) {
                        mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                                 "ptl operations.");
                        rc = MPC_LOWCOMM_NO_RESOURCE;
                        goto err;
                }
                op->comp = comp;
                op->type = LCR_PTL_OP_RMA_FLUSH;
                op->flush.op_count = op_count;
                op->flush.lkey     = lkey;

                mpc_common_spinlock_lock(&lkey->lock);
                mpc_queue_push(&lkey->pendings, &op->elem);
                mpc_common_spinlock_unlock(&lkey->lock);

                rc = MPC_LOWCOMM_IN_PROGRESS;
        }
err:
        return rc;
}

int lcr_ptl_mem_register(struct sctk_rail_info_s *rail, 
                         struct sctk_rail_pin_ctx_list *list, 
                         void * addr, size_t size, unsigned flags)
{
        ptl_md_t md;
        ptl_me_t me;
        lcr_ptl_rail_info_t* srail   = &rail->network.ptl;
        lcr_ptl_mem_t *mem = &list->pin.ptl;

        mem->size  = size;
        mem->start = addr; 
        mpc_queue_init_head(&mem->pendings);

        if (flags & LCR_IFACE_REGISTER_MEM_DYN) {
                mem->cth   = srail->am.rndv_cth;
                mem->mdh   = srail->am.rndv_mdh;
                mem->meh   = srail->am.rndv_meh;
                mem->match = LCR_PTL_RMA_MB;
        } else {
                /* Initialize RMA counter. */
                lcr_ptl_chk(PtlCTAlloc(srail->nih, &mem->cth));

                /* Prepare PTL memory descriptor. */
                md = (ptl_md_t) {
                        .ct_handle = mem->cth,
                                .eq_handle = srail->eqh,
                                .length = PTL_SIZE_MAX,
                                .start  = 0,
                                .options = PTL_MD_EVENT_SUCCESS_DISABLE |
                                        PTL_MD_EVENT_SEND_DISABLE       |
                                        PTL_MD_EVENT_CT_ACK             | 
                                        PTL_MD_EVENT_CT_REPLY,
                };
                lcr_ptl_chk(PtlMDBind(srail->nih, &md, &mem->mdh));

                mem->muid = atomic_fetch_add(&srail->rma.mem_uid, 1);
                me = (ptl_me_t) {
                        .ct_handle = PTL_CT_NONE,
                                .ignore_bits = 0,
                                .match_bits  = mem->muid | LCR_PTL_RMA_MB,
                                .match_id    = {
                                        .phys.nid = PTL_NID_ANY, 
                                        .phys.pid = PTL_PID_ANY
                                },
                                .uid         = PTL_UID_ANY,
                                .min_free    = 0,
                                .options     = PTL_ME_OP_PUT        | 
                                        PTL_ME_OP_GET               |
                                        PTL_ME_EVENT_LINK_DISABLE   | 
                                        PTL_ME_EVENT_UNLINK_DISABLE |
                                        PTL_ME_EVENT_SUCCESS_DISABLE,
                                .start       = 0,
                                .length      = PTL_SIZE_MAX
                };

                lcr_ptl_chk(PtlMEAppend(srail->nih,
                                        srail->rma.pti,
                                        &me,
                                        PTL_PRIORITY_LIST,
                                        rail,
                                        &mem->meh
                                       ));

                mem->match = me.match_bits;
        }

        mpc_common_spinlock_lock(&srail->rma.lock);
        mpc_list_push_head(&srail->rma.mem_head, &mem->elem);
        mpc_common_spinlock_unlock(&srail->rma.lock);

        mpc_common_debug("LCR PTL: memory registration. cth=%llu, mdh=%llu, "
                         "addr=%p, size=%llu, ct value=%llu", mem->cth, mem->mdh, 
                         addr, size);

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_mem_unregister(struct sctk_rail_info_s *rail, 
                            struct sctk_rail_pin_ctx_list *list)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_rail_info_t* srail   = &rail->network.ptl;
        lcr_ptl_mem_t *mem = &list->pin.ptl;
        
        mpc_common_spinlock_lock(&srail->rma.lock);
        mpc_list_del(&mem->elem);
        mpc_common_spinlock_unlock(&srail->rma.lock);

        lcr_ptl_chk(PtlCTFree(mem->cth));

        lcr_ptl_chk(PtlMDRelease(mem->mdh));

        lcr_ptl_chk(PtlMEUnlink(mem->meh));


        return rc;
}

int lcr_ptl_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        /* Serialize data. */
        *(uint64_t *)p = (uint64_t)memp->pin.ptl.start;
        p += sizeof(uint64_t);
        *(ptl_match_bits_t *)p = memp->pin.ptl.match;

        return sizeof(uint64_t) + sizeof(ptl_match_bits_t);
}

int lcr_ptl_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;	

        /* deserialize data */
        memp->pin.ptl.start = (void *)(*(uint64_t *)p);
        p += sizeof(uint64_t);
        memp->pin.ptl.match = *(ptl_match_bits_t *)p;

        return sizeof(uint64_t) + sizeof(ptl_match_bits_t);
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

static int lcr_ptl_add_route(mpc_lowcomm_peer_uid_t dest, 
                             sctk_rail_info_t* rail, 
                             _mpc_lowcomm_endpoint_type_t origin, 
                             _mpc_lowcomm_endpoint_state_t state, 
                             _mpc_lowcomm_endpoint_t **ep_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
	_mpc_lowcomm_endpoint_t *route;

	route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
        if (route == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate endpoint.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

	_mpc_lowcomm_endpoint_init(route, dest, rail, origin);
	_mpc_lowcomm_endpoint_set_state(route, state);


	if(origin == _MPC_LOWCOMM_ENDPOINT_STATIC)
	{
		sctk_rail_add_static_route (  rail, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, route );
	}

        *ep_p = route;

        return rc;
err:
        sctk_free(route);
        return rc;
}

void lcr_ptl_connect_on_demand(struct sctk_rail_info_s *rail, 
                               mpc_lowcomm_peer_uid_t dest)
{
        int rc, txq_idx;
        int found = 0;
        lcr_ptl_addr_t out_id;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;
        lcr_ptl_txq_t *txq;
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        lcr_ptl_op_t *op;
#endif

        _mpc_lowcomm_endpoint_t *ep = sctk_rail_get_any_route_to_process(rail, dest);
        if (ep != NULL) {
                mpc_common_debug_warning("LCR PTL: connect on demand. endpoint %llu "
                                         "already exists.");
                return;
        }

        /* Else get the address from PMI or monitor. */
        if( (mpc_lowcomm_peer_get_set(dest) == mpc_lowcomm_monitor_get_gid()) 
            && mpc_launch_pmi_is_initialized() ) {
                /* If target belongs to our set try PMI first */
                out_id = __map_id_pmi(rail, dest, &found);
        }

        if(!found)
        {
                /* Here we need to use the cplane
                   to request the info remotely */
                out_id = __map_id_monitor(rail, dest);
        }

        assert(!LCR_PTL_IS_ANY_PROCESS(out_id));

        /* Allocate the endpoint and add it to the rail table. */
	lcr_ptl_add_route(dest, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC, 
                          _MPC_LOWCOMM_ENDPOINT_CONNECTED, &ep);

        /* Now set ptl related data:
         * - ptl address: ptl_process_t and Portal Table Indexes.
         * - TX Queue: initialization of the structure and its associated memory
         *             pool of operations. */

        /* Fetch TQ Queue index atomically. */
        txq_idx = atomic_fetch_add(&srail->num_txqs, 1);
        if (txq_idx == LCR_PTL_MAX_TXQUEUES) {
                mpc_common_debug_error("LCR PTL: reached max endpoint (TX queue) "
                                       "%d.", LCR_PTL_MAX_TXQUEUES);
                rc = MPC_LOWCOMM_ERROR;
                goto err_free_ep;
        }
        txq = &srail->txqt[txq_idx];

	txq->addr = out_id;
        txq->idx  = txq_idx;

        /* Initialize lock, list and queues. */
        mpc_common_spinlock_init(&txq->lock, 0);
        mpc_queue_init_head(&txq->queue);
        mpc_queue_init_head(&txq->completion_queue);
        mpc_list_init_head(&txq->mem_head);

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        txq->tokens         = 0;
        txq->num_ops        = 0;
        txq->is_waiting     = 0;
#endif

        /* Init pool of operations. */
        txq->ops_pool = sctk_malloc(sizeof(mpc_mempool_t));
        if (txq->ops_pool == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate operation"
                                       " memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        mpc_mempool_param_t mp_op_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 256,
                .elem_size = sizeof(lcr_ptl_op_t),
                .max_elems = 2048,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };

        rc = mpc_mpool_init(txq->ops_pool, &mp_op_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        ep->data.ptl.txq = txq; 

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        /* Initialize token. */
        op = mpc_mpool_pop(srail->tk.ops); 
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding operations %d.");
                goto err;
        }
        assert(ep->data.ptl.txq->addr.pte.tk != LCR_PTL_PT_NULL);
        _lcr_ptl_init_op_common(op, 0, srail->tk.mdh, 
                                ep->data.ptl.txq->addr.id, 
                                ep->data.ptl.txq->addr.pte.tk, 
                                LCR_PTL_OP_TK_INIT, 
                                0, NULL, 
                                txq);

        LCR_PTL_CTRL_HDR_SET(op->hdr, op->type, 
                             txq->idx, 
                             0);

        lcr_ptl_do_op(op);
#endif

err:
        return;

err_free_ep:
        sctk_free(ep);
        return;
}

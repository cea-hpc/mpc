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

#include <mpc_config.h>

#ifdef MPC_USE_PORTALS

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
        lcr_ptl_ep_info_t  *ptl_ep = &ep->data.ptl;
        void* start                = NULL;
        size_t size                = 0;
        lcr_ptl_op_t *op;

        assert(ptl_ep->addr.pte.am != LCR_PTL_PT_NULL);

        //FIXME: use a memory pool.
        start = mpc_mpool_pop(srail->buf_mp);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer.");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        assert(size <= srail->config.eager_limit);

        op = mpc_mpool_pop(ptl_ep->ops_pool); 
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding operations %d.");
                size = -1;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->net.am.mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.am, 
                                LCR_PTL_OP_AM_BCOPY, 
                                size, NULL, 
                                &ptl_ep->am_txq);

        op->am.am_id     = id;
        op->am.bcopy_buf = start;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        int token_num = 1;
        op->id = atomic_fetch_add(&srail->net.am.op_sn, 1);
        LCR_PTL_AM_HDR_SET(op->hdr, op->am.am_id, 
                           ptl_ep->idx, 
                           ptl_ep->num_ops);
        rc = lcr_ptl_post(ptl_ep, op, &token_num);

        if (lcr_ptl_ep_needs_tokens(ptl_ep, token_num)) {
                lcr_ptl_op_t *tk_op;
                rc = lcr_ptl_create_token_request(srail, ptl_ep, &tk_op);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                assert(ptl_ep->is_waiting == 1);

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
        lcr_ptl_ep_info_t  *ptl_ep = &ep->data.ptl;
        size_t size                = 0;
	ptl_md_t iov_md;

        lcr_ptl_op_t *op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        /* Memory Descriptor handle and size are set later. */
        _lcr_ptl_init_op_common(op, 0, srail->net.am.mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.am, 
                                LCR_PTL_OP_AM_ZCOPY, 
                                0, comp, 
                                &ptl_ep->am_txq);

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
        assert(ptl_ep->addr.pte.am != LCR_PTL_PT_NULL);

        iov_md = (ptl_md_t){
                .start = op->iov.iov,
                .length = iovcnt + 1,
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->net.eqh,
                .options = PTL_MD_EVENT_SEND_DISABLE | PTL_IOVEC,
        };

        /* Set Memory Descriptor handle. */
	lcr_ptl_chk(PtlMDBind(
		srail->net.nih,      /* the NI handler */
		&iov_md,           /* the MD to bind with memory region */
		&op->mdh /* out: the MD handler */
	)); 

        op->am.am_id = id;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        int token_num = 1;
        op->id = atomic_fetch_add(&srail->net.am.op_sn, 1);
        LCR_PTL_AM_HDR_SET(op->hdr, op->am.am_id, 
                           ptl_ep->idx, 
                           ptl_ep->num_ops);
        rc = lcr_ptl_post(ptl_ep, op, &token_num);

        if (lcr_ptl_ep_needs_tokens(ptl_ep, token_num)) {
                lcr_ptl_op_t *tk_op;
                rc = lcr_ptl_create_token_request(srail, ptl_ep, &tk_op);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                assert(ptl_ep->is_waiting == 1);

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
        void* start                = NULL;
        size_t size                = 0;
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        lcr_ptl_ep_info_t  *ptl_ep = &ep->data.ptl;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        not_implemented();
#endif

        start = mpc_mpool_pop(srail->buf_mp);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer.");
                size = -1;
                goto err;
        }
        size = pack(start, arg);

        assert(size <= srail->config.eager_limit);
        assert(ptl_ep->addr.pte.tag != LCR_PTL_PT_NULL);

        lcr_ptl_op_t *op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->net.tag.mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.tag, 
                                LCR_PTL_OP_TAG_BCOPY, 
                                size, NULL, 
                                &ptl_ep->am_txq);

        op->tag.bcopy_buf = start;

        //FIXME: move to _lcr_ptl_do_op
        lcr_ptl_chk(PtlPut(srail->net.tag.mdh,
                            (ptl_size_t) start, /* local offset */
                            size,
                            PTL_ACK_REQ,
                            ptl_ep->addr.id,
                            ptl_ep->addr.pte.tag,
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
        lcr_ptl_rail_info_t *srail  = &ep->rail->network.ptl;
        lcr_ptl_ep_info_t   *ptl_ep = &ep->data.ptl;

        //FIXME: support for only one iovec
        assert(iovcnt == 1);
        assert(ptl_ep->addr.pte.tag != LCR_PTL_PT_NULL);

        lcr_ptl_op_t *op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->net.tag.mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.tag, 
                                LCR_PTL_OP_TAG_ZCOPY, 
                                iov[0].iov_len, comp, 
                                &ptl_ep->am_txq);

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        not_implemented();
#endif

        //FIXME: move to _lcr_ptl_do_op
        lcr_ptl_chk(PtlPut(srail->net.tag.mdh,
                            (ptl_size_t) iov[0].iov_base, /* local offset */
                            iov[0].iov_len,
                            PTL_ACK_REQ,
                            ptl_ep->addr.id,
                            ptl_ep->addr.pte.tag,
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
                        .options = PTL_ME_OP_PUT | 
                                PTL_ME_OP_GET    |
                                PTL_ME_USE_ONCE,
        };

        mpc_common_debug_info("LCR PTL: post recv zcopy to %p (idx=%d, tag=%llu, "
                              "sz=%llu, buf=%p)", rail, srail->net.tag.pti, tag.t,
                              iov[0].iov_len, iov[0].iov_base);

        lcr_ptl_op_t *op = mpc_mpool_pop(srail->iface_ops);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, srail->net.tag.mdh, 
                                LCR_PTL_PROCESS_ANY, 
                                srail->net.tag.pti, 
                                search ? LCR_PTL_OP_TAG_SEARCH : 
                                LCR_PTL_OP_TAG_ZCOPY, 
                                iov[iovcnt - 1].iov_len, &ctx->comp, 
                                NULL);

        //FIXME: move to _lcr_ptl_do_op
        if (search) {
                lcr_ptl_chk(PtlMESearch(srail->net.nih,
                                         srail->net.tag.pti,
                                         &me,
                                         PTL_SEARCH_ONLY,
                                         op));
        } else {
                lcr_ptl_chk(PtlMEAppend(srail->net.nih,
                                        srail->net.tag.pti,
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
        void* start                = NULL;
        size_t size                = 0;
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        lcr_ptl_ep_info_t  *ptl_ep = &ep->data.ptl;
        lcr_ptl_txq_t      *txq    = NULL;
        lcr_ptl_mem_t *lkey = &local_key->pin.ptl;
        lcr_ptl_mem_t *rkey = &remote_key->pin.ptl;

        //TODO: create a pool of pre-registered MD.
        not_implemented();

        start = mpc_mpool_pop(srail->buf_mp);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer.");
                size = -1;
                goto err;
        }
        size = pack(start, arg);

        /* Link memory to endpoint if not already done. */
        txq = &lkey->txqt[ptl_ep->idx];

        lcr_ptl_op_t *op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_RMA_PUT, 
                                size, NULL, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.local_offset  = 0;
        op->rma.remote_offset = remote_offset;

        /* Increment operation counts. */
        op->id = atomic_fetch_add(lkey->op_count, 1);
        lcr_ptl_do_op(op);

err:
        return size;
}

int lcr_ptl_send_get_bcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_pack_callback_t pack,
                           void *arg,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key)
{
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;
        void* start                = NULL;
        size_t size                = 0;
        lcr_ptl_ep_info_t  *ptl_ep = &ep->data.ptl;
        lcr_ptl_txq_t      *txq    = NULL;
        lcr_ptl_mem_t      *lkey   = &local_key->pin.ptl;
        lcr_ptl_mem_t      *rkey   = &remote_key->pin.ptl;

        //TODO: create a pool of pre-registered MD.
        not_implemented();

        start = mpc_mpool_pop(srail->buf_mp);
        if (start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate bcopy buffer.");
                size = -1;
                goto err;
        }
        size = pack(start, arg);

        txq = &lkey->txqt[ptl_ep->idx];

        lcr_ptl_op_t *op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_RMA_GET, 
                                size, NULL, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.local_offset  = 0;
        op->rma.remote_offset = remote_offset;

        /* Increment operation counts. */
        op->id = atomic_fetch_add(lkey->op_count, 1);
        lcr_ptl_do_op(op);

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
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        lcr_ptl_txq_t     *txq    = NULL;
        lcr_ptl_mem_t     *lkey   = &local_key->pin.ptl;
        lcr_ptl_mem_t     *rkey   = &remote_key->pin.ptl;
        lcr_ptl_op_t      *op     = NULL;         

        /* Link memory to endpoint if not already done. */
        txq = &lkey->txqt[ptl_ep->idx];

        //NOTE: Operation identifier and Put must be done atomically to make
        //      sure the operation is pushed to the NIC with the correct
        //      sequence number.
        op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "ptl operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_RMA_PUT, 
                                size, comp, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.match         = rkey->match;
        op->rma.local_offset  = local_offset;
        if (lkey->flags & LCR_IFACE_REGISTER_MEM_DYN) {
                op->rma.local_offset += (uint64_t)lkey->start;
        } 
        op->rma.remote_offset = remote_offset;
        if (rkey->flags & LCR_IFACE_REGISTER_MEM_DYN) {
                op->rma.remote_offset += (uint64_t)rkey->start;
        }

        mpc_common_spinlock_lock(&txq->lock);
        mpc_queue_push(&txq->ops, &op->elem);
        mpc_common_spinlock_unlock(&txq->lock);

        /* Increment operation counts. */
        op->id = atomic_fetch_add(lkey->op_count, 1);
        assert(op->id < INT64_MAX);

        rc = lcr_ptl_do_op(op);

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
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        lcr_ptl_txq_t     *txq    = NULL;
        lcr_ptl_mem_t     *lkey   = &local_key->pin.ptl;
        lcr_ptl_mem_t     *rkey   = &remote_key->pin.ptl;
        lcr_ptl_op_t      *op     = NULL;         

        txq = &lkey->txqt[ptl_ep->idx];

        op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "ptl operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_RMA_GET, 
                                size, comp, 
                                txq);

        op->rma.lkey          = lkey;
        op->rma.rkey          = rkey;
        op->rma.match         = rkey->match;
        op->rma.local_offset  = local_offset;
        if (lkey->flags & LCR_IFACE_REGISTER_MEM_DYN) {
                op->rma.local_offset += (uint64_t)lkey->start;
        } 
        op->rma.remote_offset = remote_offset;
        if (rkey->flags & LCR_IFACE_REGISTER_MEM_DYN) {
                op->rma.remote_offset += (uint64_t)rkey->start;
        }

        mpc_common_spinlock_lock(&txq->lock);
        mpc_queue_push(&txq->ops, &op->elem);
        mpc_common_spinlock_unlock(&txq->lock);

        /* Increment operation counts. */
        op->id = atomic_fetch_add(lkey->op_count, 1);
        assert(op->id < INT64_MAX);

        rc = lcr_ptl_do_op(op);
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
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        lcr_ptl_rail_info_t* srail = &ep->rail->network.ptl;

        mpc_common_debug("PTL: get tag remote key. "
                         "iface=%p, remote nid=%lu, pid=%lu, remote off=%llu, pte idx=%d, "
                         "local addr=%p", ep->rail, ptl_ep->addr.id.phys.nid, 
                         ptl_ep->addr.id.phys.pid, remote_offset, 
                         srail->net.tag.pti, local_offset);

        lcr_ptl_op_t *op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        op->type         = LCR_PTL_OP_AM_BCOPY; 
        op->size         = size;
        op->comp         = comp;

        lcr_ptl_chk(PtlGet(srail->net.tag.mdh,
                            local_offset,
                            size,
                            ptl_ep->addr.id,
                            ptl_ep->addr.pte.tag,
                            tag.t,
                            remote_offset,
                            op	
                           ));

err:
        return rc;

}

int lcr_ptl_atomic_post(_mpc_lowcomm_endpoint_t *ep,
                        uint64_t local_offset,
                        uint64_t remote_offset,
                        lcr_atomic_op_t op_type,
                        lcr_memp_t *local_key,
                        lcr_memp_t *remote_key,
                        size_t size,
                        lcr_completion_t *comp) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        lcr_ptl_mem_t     *lkey   = &local_key->pin.ptl;
        lcr_ptl_mem_t     *rkey   = &remote_key->pin.ptl;
        lcr_ptl_txq_t     *txq    = NULL;
        lcr_ptl_op_t      *op     = NULL;

        assert(size == sizeof(uint64_t));
        
        /* Link memory to endpoint if not already done. */
        txq = &lkey->txqt[ptl_ep->idx];

        op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_ATOMIC_POST, 
                                size, comp, 
                                txq);

        op->ato.lkey               = lkey;
        op->ato.rkey               = rkey;
        op->ato.match              = rkey->match;
        op->ato.remote_offset      = remote_offset;
        op->ato.op                 = lcr_ptl_atomic_op_table[op_type];
        op->ato.dt                 = PTL_UINT64_T;
        op->ato.post.local_offset  = local_offset;

        op->id = atomic_fetch_add(lkey->op_count, 1);
        rc = lcr_ptl_do_op(op);
        
err:
        return rc;
}

int lcr_ptl_atomic_fetch(_mpc_lowcomm_endpoint_t *ep,
                         uint64_t get_local_offset,
                         uint64_t put_local_offset,
                         uint64_t remote_offset,
                         lcr_atomic_op_t op_type,
                         lcr_memp_t *local_key,
                         lcr_memp_t *remote_key,
                         size_t size,
                         lcr_completion_t *comp) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        lcr_ptl_mem_t     *lkey   = &local_key->pin.ptl;
        lcr_ptl_mem_t     *rkey   = &remote_key->pin.ptl;
        lcr_ptl_txq_t     *txq    = NULL;
        lcr_ptl_op_t      *op     = NULL;

        assert(size == sizeof(uint64_t));

        /* Link memory to endpoint if not already done. */
        txq = &lkey->txqt[ptl_ep->idx];

        op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_ATOMIC_FETCH, 
                                size, comp, 
                                txq);

        op->ato.lkey                   = lkey;
        op->ato.rkey                   = rkey;
        op->ato.match                  = rkey->match;
        op->ato.remote_offset          = remote_offset;
        op->ato.op                     = lcr_ptl_atomic_op_table[op_type];
        op->ato.dt                     = PTL_UINT64_T;
        op->ato.fetch.get_local_offset = get_local_offset;
        op->ato.fetch.put_local_offset = put_local_offset;

        op->id = atomic_fetch_add(lkey->op_count, 1);
        rc = lcr_ptl_do_op(op);
        
err:
        return rc;
}

int lcr_ptl_atomic_cswap(_mpc_lowcomm_endpoint_t *ep,
                         uint64_t get_local_offset,
                         uint64_t put_local_offset,
                         uint64_t remote_offset,
                         lcr_atomic_op_t op_type,
                         lcr_memp_t *local_key,
                         lcr_memp_t *remote_key,
                         uint64_t compare,
                         size_t size,
                         lcr_completion_t *comp) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        lcr_ptl_mem_t     *lkey   = &local_key->pin.ptl;
        lcr_ptl_mem_t     *rkey   = &remote_key->pin.ptl;
        lcr_ptl_txq_t     *txq    = NULL;
        lcr_ptl_op_t      *op     = NULL;

        assert(size == sizeof(uint64_t));

        txq = &lkey->txqt[ptl_ep->idx];

        op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                size = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_ATOMIC_FETCH, 
                                size, comp, 
                                txq);

        op->ato.lkey                   = lkey;
        op->ato.rkey                   = rkey;
        op->ato.match                  = rkey->match;
        op->ato.remote_offset          = remote_offset;
        op->ato.op                     = lcr_ptl_atomic_op_table[op_type];
        op->ato.dt                     = PTL_UINT64_T;
        op->ato.cswap.get_local_offset = get_local_offset;
        op->ato.cswap.put_local_offset = put_local_offset;
        op->ato.cswap.operand          = &compare;

        //FIXME: put this in a function.
        op->id = atomic_fetch_add(lkey->op_count, 1);
        rc = lcr_ptl_do_op(op);
err:
        return rc;
}

int lcr_ptl_flush(sctk_rail_info_t *rail,
                  _mpc_lowcomm_endpoint_t *ep,
                  struct sctk_rail_pin_ctx_list *list,
                  lcr_completion_t *comp,
                  unsigned flags) 
{
        int rc = MPC_LOWCOMM_SUCCESS, i = 0;
        lcr_ptl_op_t *op;
        lcr_ptl_rail_info_t *srail  = &rail->network.ptl;
        lcr_ptl_ep_info_t   *ptl_ep = &ep->data.ptl;
        lcr_ptl_mem_t       *lkey   = &list->pin.ptl;
        lcr_ptl_mem_t       *mem    = NULL;

        op = mpc_mpool_pop(ptl_ep->ops_pool);
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding "
                                         "operations.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }
        _lcr_ptl_init_op_common(op, 0, lkey->mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.rma, 
                                LCR_PTL_OP_RMA_FLUSH, 
                                0, comp, 
                                NULL);

        mpc_list_init_head(&op->flush.mem_head);

        if (flags & LCR_IFACE_FLUSH_EP) {
                assert(ep != NULL);

                mpc_common_spinlock_lock(&srail->net.rma.lock);
                op->flush.outstandings = mpc_list_length(&srail->net.rma.mem_head);
                op->flush.op_count     = sctk_malloc(op->flush.outstandings * sizeof(int64_t));
                if (op->flush.op_count == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate flush "
                                               "op count table.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                };

                i = 0;
                mpc_list_for_each(mem, &srail->net.rma.mem_head, lcr_ptl_mem_t, elem) {
                        op->flush.op_count[i++] = atomic_load(mem->op_count);
                }

                rc = lcr_ptl_flush_ep(srail, ptl_ep, op);

                mpc_common_spinlock_unlock(&srail->net.rma.lock);

        } else if (flags & LCR_IFACE_FLUSH_MEM) {
                assert(mem != NULL);

                mpc_common_spinlock_lock(&mem->lock);
                op->flush.outstandings = 1;
                op->flush.op_count     = sctk_malloc(sizeof(int64_t));
                if (op->flush.op_count == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate flush "
                                               "op count table.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                };

                *op->flush.op_count = atomic_load(mem->op_count);

                rc = lcr_ptl_flush_mem(srail, mem, op);

                mpc_common_spinlock_lock(&mem->lock);

        } else if (flags & LCR_IFACE_FLUSH_EP_MEM) {
                assert(mem != NULL && ep != NULL);

                mpc_common_spinlock_lock(&ptl_ep->lock);
                op->flush.outstandings = 1;
                op->flush.op_count     = sctk_malloc(sizeof(int64_t));
                if (op->flush.op_count == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate flush "
                                               "op count table.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                };

                *op->flush.op_count = atomic_load(mem->op_count);

                rc = lcr_ptl_flush_mem_ep(mem, ptl_ep, op);

                mpc_common_spinlock_unlock(&ptl_ep->lock);

        } else if (flags & LCR_IFACE_FLUSH_IFACE) {

                op->flush.outstandings = 0;

                mpc_common_spinlock_lock(&srail->net.rma.lock);
                op->flush.outstandings = mpc_list_length(&srail->net.rma.mem_head);
                op->flush.op_count     = sctk_malloc(op->flush.outstandings * sizeof(int64_t));
                if (op->flush.op_count == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate flush "
                                               "op count table.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                };

                i = 0;
                mpc_list_for_each(mem, &srail->net.rma.mem_head, lcr_ptl_mem_t, elem) {
                       op->flush.op_count[i] = atomic_load(mem->op_count);
                       i++;
                }
                mpc_common_spinlock_unlock(&srail->net.rma.lock);

                rc = lcr_ptl_flush_iface(srail, op);
        }
err:
        return rc;
}

int lcr_ptl_mem_register(struct sctk_rail_info_s *rail, 
                         struct sctk_rail_pin_ctx_list *list, 
                         const void * addr, size_t size, unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcr_ptl_rail_info_t *srail   = &rail->network.ptl;
        lcr_ptl_mem_t *mem           = &list->pin.ptl;

        mem->size  = size;
        mem->start = addr; 
        mem->flags = flags;
        mpc_queue_init_head(&mem->pending_flush);

        if (flags & LCR_IFACE_REGISTER_MEM_DYN) {
                mem->cth      = srail->net.rma.rndv_cth;
                mem->mdh      = srail->net.rma.rndv_mdh;
                mem->meh      = srail->net.rma.rndv_meh;
                mem->txqt     = srail->net.rma.rndv_txqt;
                mem->match    = (ptl_match_bits_t)LCR_PTL_RNDV_MB;
                mem->op_count = &srail->net.am.rma_count;
        } else {
                mem->txqt  = sctk_malloc(LCR_PTL_MAX_EPS * sizeof(lcr_ptl_txq_t));
                if (mem->txqt == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate "
                                               "memory txq table.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                for (i = 0; i < LCR_PTL_MAX_EPS; i++) {
                        mpc_queue_init_head(&mem->txqt[i].ops);
                        mpc_common_spinlock_init(&mem->txqt[i].lock, 0);
                }

                mem->match = atomic_fetch_add(&srail->net.rma.mem_uid, 1) | 
                        LCR_PTL_RMA_MB;
                mem->op_count = sctk_malloc(sizeof(atomic_uint_least64_t));
                if (mem->op_count == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate "
                                               "memory atomic counter.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                //FIXME: check function signature without casting
                rc = lcr_ptl_post_rma_resources(srail, 
                                                mem->match, 
                                                (void * const)mem->start, 
                                                mem->size, 
                                                &mem->cth, 
                                                &mem->mdh,
                                                &mem->meh);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

        mpc_common_spinlock_lock(&srail->net.rma.lock);
        mpc_list_push_head(&srail->net.rma.mem_head, &mem->elem);
        mpc_common_spinlock_unlock(&srail->net.rma.lock);

        mpc_common_debug("LCR PTL: memory registration. cth=%llu, mdh=%llu, "
                         "addr=%p, size=%llu, match=%llu", *(uint64_t *)&mem->cth, 
                         *(uint64_t *)&mem->mdh, addr, size, mem->match);

err:
        return rc;
}

int lcr_ptl_mem_unregister(struct sctk_rail_info_s *rail, 
                            struct sctk_rail_pin_ctx_list *list)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_rail_info_t* srail   = &rail->network.ptl;
        lcr_ptl_mem_t *mem = &list->pin.ptl;
        
        mpc_common_spinlock_lock(&srail->net.rma.lock);
        mpc_list_del(&mem->elem);
        assert(mpc_queue_is_empty(&mem->pending_flush));
        mpc_common_spinlock_unlock(&srail->net.rma.lock);

        if (!(mem->flags & LCR_IFACE_REGISTER_MEM_DYN)) {
                sctk_free(mem->op_count);

                lcr_ptl_chk(PtlCTFree(mem->cth));

                lcr_ptl_chk(PtlMDRelease(mem->mdh));

                lcr_ptl_chk(PtlMEUnlink(mem->meh));
        }

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
        p += sizeof(ptl_match_bits_t);
        *(unsigned *)p = memp->pin.ptl.flags;

        return sizeof(uint64_t) + sizeof(ptl_match_bits_t) + sizeof(unsigned); 
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
        p += sizeof(ptl_match_bits_t);
        memp->pin.ptl.flags = *(unsigned *)p;

        return sizeof(uint64_t) + sizeof(ptl_match_bits_t) + sizeof(unsigned);
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
        int rc;
        int found = 0;
        lcr_ptl_addr_t out_id;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;
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
        lcr_ptl_ep_info_t *ptl_ep = &ep->data.ptl;
        ptl_ep->idx = atomic_fetch_add(&srail->num_eps, 1);
        if (ptl_ep->idx == LCR_PTL_MAX_EPS) {
                mpc_common_debug_error("LCR PTL: reached max endpoint (TX queue) "
                                       "%d.", LCR_PTL_MAX_EPS);
                rc = MPC_LOWCOMM_ERROR;
                goto err_free_ep;
        }
        ptl_ep->addr = out_id;

        /* Initialize lock, list and queues. */
        mpc_common_spinlock_init(&ptl_ep->lock, 0);
        mpc_queue_init_head(&ptl_ep->am_txq.ops); 

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        ptl_ep->tokens         = 0;
        ptl_ep->num_ops        = 0;
        ptl_ep->is_waiting     = 0;
#endif

        /* Init pool of operations. */
        ptl_ep->ops_pool = sctk_malloc(sizeof(mpc_mempool_t));
        if (ptl_ep->ops_pool == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate operation"
                                       " memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free_ep;
        }
        mpc_mempool_param_t mp_op_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 256,
                .elem_size = sizeof(lcr_ptl_op_t),
                .max_elems = 2048,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };

        rc = mpc_mpool_init(ptl_ep->ops_pool, &mp_op_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err_free_ep;
        }

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        /* Initialize token. */
        op = mpc_mpool_pop(srail->net.tk.ops); 
        if (op == NULL) {
                mpc_common_debug_warning("LCR PTL: maximum outstanding operations %d.");
                goto err_free_ep;
        }
        assert(ptl_ep->addr.pte.tk != LCR_PTL_PT_NULL);
        _lcr_ptl_init_op_common(op, 0, srail->net.tk.mdh, 
                                ptl_ep->addr.id, 
                                ptl_ep->addr.pte.tk, 
                                LCR_PTL_OP_TK_INIT, 
                                0, NULL, 
                                &ptl_ep->am_txq);

        LCR_PTL_CTRL_HDR_SET(op->hdr, op->type, 
                             ptl_ep->idx, 
                             0);

        lcr_ptl_do_op(op);
#endif
        return;

err_free_ep:
        sctk_free(ep);
        return;
}

#endif

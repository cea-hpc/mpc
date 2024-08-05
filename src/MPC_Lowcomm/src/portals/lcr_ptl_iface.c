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


#ifdef MPC_USE_PORTALS

#include "lcr_ptl.h"


#include "sctk_ptl_iface.h"
#include "sctk_ptl_offcoll.h"
#include "lcr_ptl_recv.h"
#include "queue.h"

#include <sctk_alloc.h>

static inline int lcr_ptl_invoke_am(sctk_rail_info_t *rail,
                                    uint8_t am_id,
                                    size_t length,
                                    void *data)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	lcr_am_handler_t handler = rail->am[am_id];
	if (handler.cb == NULL) {
		mpc_common_debug_fatal("LCP: handler id %d not supported.", am_id);
	}

	rc = handler.cb(handler.arg, data, length, 0);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", am_id);
	}

	return rc;
}

static int _lcr_ptl_progress_rma(sctk_ptl_rail_info_t *srail)
{
        size_t op_it, count = 0;
        lcr_ptl_rma_handle_t *rmah = NULL;
        ptl_ct_event_t ev;
        lcr_ptl_op_t *op;

        mpc_common_spinlock_lock(&srail->ptl_info.rmah_lock);
        mpc_list_for_each(rmah, &srail->ptl_info.rmah_head, lcr_ptl_rma_handle_t, elem) {
                sctk_ptl_chk(PtlCTGet(rmah->cth, &ev));

                if (ev.failure > 0) {
                        mpc_common_debug_error("LCR PTL: error on RMA "
                                               "operation %p", rmah);
                        goto unlock;
                }

                /* Complete all ongoing operations. */
                op_it = rmah->completed_ops;
                while ((ptl_size_t)op_it < ev.success) {
                        mpc_common_spinlock_lock(&rmah->ops_lock);
                        op = mpc_queue_pull_elem(&rmah->ops, lcr_ptl_op_t, elem);
                        mpc_common_spinlock_unlock(&rmah->ops_lock);

                        mpc_common_debug("LCR PTL: progress rma op. op=%p", op);
                        op->comp->sent = op->size;
                        op->comp->comp_cb(op->comp);
                        op_it++; count++; rmah->completed_ops++;
                }
        }

unlock:
        mpc_common_spinlock_unlock(&srail->ptl_info.rmah_lock);
        return count;
}

int lcr_ptl_iface_progress(sctk_rail_info_t *rail)
{
	int did_poll = 0;
	int ret;
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
        lcr_ptl_op_t *op;
        lcr_tag_context_t *tag_ctx;
        lcr_ptl_recv_block_t *block;
        uint8_t am_id;
        unsigned flags = 0;

        while (1) {

                mpc_common_spinlock_lock(&srail->ptl_info.poll_lock);

                _lcr_ptl_progress_rma(srail);

                ret = PtlEQGet(srail->ptl_info.eqh, &ev);
                sctk_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, "
                                              "sz=%llu, user=%p, start=%p, "
                                              "remote_offset=%p, iface=%d",
                                              sctk_ptl_event_decode(ev), ev.pt_index,
                                              ev.mlength, ev.user_ptr, ev.start,
                                              ev.remote_offset, srail->iface);
                        did_poll = 1;

                        op = (lcr_ptl_op_t *)ev.user_ptr;
                        assert(op);

                        switch (ev.type) {
                        case PTL_EVENT_SEARCH:
                                if (ev.ni_fail_type == PTL_NI_NO_MATCH) {
                                        goto poll_unlock;
                                }
                                tag_ctx            = op->tag.tag_ctx;
                                tag_ctx->tag       = (lcr_tag_t)ev.match_bits;
                                tag_ctx->imm       = ev.hdr_data;
                                tag_ctx->start     = NULL;
                                tag_ctx->flags    |= flags;

                                /* call completion callback */
                                op->comp->sent = ev.mlength;
                                op->comp->comp_cb(op->comp);
                                break;

                        case PTL_EVENT_SEND:
                                not_reachable();
                                break;
                        case PTL_EVENT_REPLY:
                                assert(op->size == ev.mlength);
                                op->comp->sent = ev.mlength;
                                op->comp->comp_cb(op->comp);
                                break;
                        case PTL_EVENT_ACK:
                                switch (op->type) {
                                case LCR_PTL_OP_TAG_BCOPY:
                                case LCR_PTL_OP_AM_BCOPY:
                                        sctk_free(op->am.bcopy_buf);
                                        break;
                                case LCR_PTL_OP_AM_ZCOPY:
                                        op->comp->sent = ev.mlength;
                                        op->comp->comp_cb(op->comp);
                                        sctk_ptl_chk(PtlMDRelease(op->iov.iovh));
                                        sctk_free(op->iov.iov);
                                        break;
                                case LCR_PTL_OP_TAG_ZCOPY:
                                case LCR_PTL_OP_RMA_PUT:
                                        op->comp->sent = ev.mlength;
                                        op->comp->comp_cb(op->comp);
                                        break;
                                default:
                                        mpc_common_debug_error("LCR PTL: unknown "
                                                               "completion type");
                                        break;
                                }
                                mpc_mpool_push(op);
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_PUT_OVERFLOW:
                                flags = LCR_IFACE_TM_OVERFLOW;
                                /* fall through */
                        case PTL_EVENT_PUT:
                                switch (ev.pt_index) {
                                case LCR_PTL_PTE_IDX_TAG_EAGER:
                                        switch (op->type) {
                                        case LCR_PTL_OP_BLOCK:
                                                break;
                                        case LCR_PTL_OP_TAG_ZCOPY:
                                                tag_ctx            = op->tag.tag_ctx;
                                                tag_ctx->tag       = (lcr_tag_t)ev.match_bits;
                                                tag_ctx->imm       = ev.hdr_data;
                                                tag_ctx->start     = ev.start;
                                                tag_ctx->comp.sent = ev.mlength;
                                                tag_ctx->flags    |= flags;

                                                /* call completion callback */
                                                tag_ctx->comp.comp_cb(&tag_ctx->comp);
                                                break;
                                        default:
                                                mpc_common_debug_error("LCR PTL: wrong comp"
                                                                       " type.");
                                                break;
                                        }
                                        break;
                                case LCR_PTL_PTE_IDX_AM_EAGER:
                                        am_id = (uint8_t)ev.hdr_data;
                                        lcr_ptl_invoke_am(rail, am_id, ev.mlength,
                                                          ev.start);
                                        break;
                                default:
                                        mpc_common_debug_fatal("LCR PTL: wrong pte");
                                        break;
                                }
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_GET_OVERFLOW:
                                mpc_common_debug_info("LCR PTL: got get overflow, not "
                                                      "possible!");
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_GET:
                                op->comp->sent = ev.mlength;
                                op->comp->comp_cb(op->comp);
                                break;
                        case PTL_EVENT_AUTO_UNLINK:
                                block = mpc_container_of(op, lcr_ptl_recv_block_t, op);
                                sctk_ptl_list_t list = ev.pt_index == LCR_PTL_PTE_IDX_TAG_EAGER ?
                                        SCTK_PTL_OVERFLOW_LIST : SCTK_PTL_PRIORITY_LIST;
                                lcr_ptl_recv_block_activate(block, ev.pt_index, list);
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_AUTO_FREE:
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
                        case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
                        case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
                        case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
                        case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
                                not_reachable();              /* have been disabled */
                                break;
                        default:
                                break;
                        }
                } else if (ret == PTL_EQ_EMPTY) {
                        goto poll_unlock;
                        break;
                } else {
                        mpc_common_debug_error("LCR PTL: error returned from PtlEQPoll");
                        break;
                }
                mpc_mpool_push(op);
poll_unlock:
                mpc_common_spinlock_unlock(&srail->ptl_info.poll_lock);
                break;
        }

        return did_poll;
}

int lcr_ptl_software_init(sctk_ptl_rail_info_t *srail, size_t comm_dims)
{
        UNUSED(comm_dims);
        int rc;
        ptl_me_t me_rma;
        ptl_md_t md;
        sctk_ptl_matchbits_t rdv_tag, ign_rdv_tag;
	srail->nb_entries = 0;

        /* create the EQ */
        sctk_ptl_chk(PtlEQAlloc(srail->iface,
                                SCTK_PTL_EQ_PTE_SIZE,
                                &srail->ptl_info.eqh
                               ));

        /* MD spanning all addressable memory */
        md = (ptl_md_t){
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->ptl_info.eqh,
                .length = PTL_SIZE_MAX,
                .start = 0,
                .options = PTL_MD_EVENT_SEND_DISABLE
        };

	sctk_ptl_chk(PtlMDBind(
		srail->iface,
		&md,
		&srail->ptl_info.mdh
	));

	OPA_store_int(&srail->rdma_cpt, 0);
	if(srail->max_mr > srail->max_limits.max_msg_size)
		srail->max_mr = srail->max_limits.max_msg_size;

	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_init(srail);

        /* Init poll lock used to ensure in-order message reception */
        mpc_common_spinlock_init(&srail->ptl_info.poll_lock, 0);

        //FIXME: only allocate if offload is enabled
        /* register the TAG EAGER PTE */
        sctk_ptl_chk(PtlPTAlloc(srail->iface,
                                SCTK_PTL_PTE_FLAGS,
                                srail->ptl_info.eqh,
                                LCR_PTL_PTE_IDX_TAG_EAGER,
                                &srail->ptl_info.tag_pte
                               ));

        if ((int)srail->ptl_info.tag_pte != LCR_PTL_PTE_IDX_TAG_EAGER) {
                mpc_common_debug_error("LCR PTL: could not associate eager pte idx");
                return MPC_LOWCOMM_ERROR;
        }

        /* register the AM EAGER PTE */
        sctk_ptl_chk(PtlPTAlloc(srail->iface,
                                SCTK_PTL_PTE_FLAGS,
                                srail->ptl_info.eqh,
                                LCR_PTL_PTE_IDX_AM_EAGER,
                                &srail->ptl_info.am_pte
                               ));

        if ((int)srail->ptl_info.am_pte != LCR_PTL_PTE_IDX_AM_EAGER) {
                mpc_common_debug_error("LCR PTL: could not associate eager pte idx");
                return MPC_LOWCOMM_ERROR;
        }

        /* activate tag eager block */
        srail->ptl_info.tag_block_list = NULL;
        rc = lcr_ptl_recv_block_enable(srail, srail->ptl_info.tag_pte, SCTK_PTL_OVERFLOW_LIST);
        /* activate am eager block */
        srail->ptl_info.am_block_list = NULL;
        rc = lcr_ptl_recv_block_enable(srail, srail->ptl_info.am_pte, SCTK_PTL_PRIORITY_LIST);

        /* register RNDV PT */
        sctk_ptl_chk(PtlPTAlloc(srail->iface,
                                SCTK_PTL_PTE_FLAGS,
                                srail->ptl_info.eqh,
                                LCR_PTL_PTE_IDX_RMA,
                                &srail->ptl_info.rma_pte
                               ));

        if ((int)srail->ptl_info.rma_pte != LCR_PTL_PTE_IDX_RMA) {
                mpc_common_debug_error("LCR PTL: could not associate rndv pte idx");
                return MPC_LOWCOMM_ERROR;
        }

        rdv_tag     = SCTK_PTL_MATCH_INIT;
        ign_rdv_tag = SCTK_PTL_IGN_ALL;
        me_rma = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                .ignore_bits = ign_rdv_tag.raw,
                .match_bits  = rdv_tag.raw,
                .match_id    = SCTK_PTL_ANY_PROCESS,
                .uid         = PTL_UID_ANY,
                .min_free    = 0,
                .options     = SCTK_PTL_ME_PUT_FLAGS |
                        SCTK_PTL_ME_GET_FLAGS  |
                        PTL_ME_EVENT_LINK_DISABLE |
                        PTL_ME_EVENT_UNLINK_DISABLE |
                        PTL_ME_EVENT_COMM_DISABLE,
                .start       = 0,
                .length      = PTL_SIZE_MAX
        };

        sctk_ptl_chk(PtlMEAppend(srail->iface,
                                 srail->ptl_info.rma_pte,
                                 &me_rma,
                                 SCTK_PTL_PRIORITY_LIST,
                                 NULL,
                                 &srail->ptl_info.rma_meh
                                ));

        /* Initialize list of active RMA handles. */
        mpc_list_init_head(&srail->ptl_info.rmah_head);

        /* Init pool of operations. */
        srail->ptl_info.ops_pool = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->ptl_info.ops_pool == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate operation"
                                       " memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        mpc_mempool_param_t mp_op_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 512,
                .elem_size = sizeof(lcr_ptl_op_t),
                .max_elems = 2048,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };

        rc = mpc_mpool_init(srail->ptl_info.ops_pool, &mp_op_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

err:
        return rc;
}

void lcr_ptl_software_fini(sctk_ptl_rail_info_t* srail)
{
        lcr_ptl_recv_block_disable(srail->ptl_info.tag_block_list);
        lcr_ptl_recv_block_disable(srail->ptl_info.am_block_list);

        sctk_ptl_chk(PtlPTFree(srail->iface, srail->ptl_info.tag_pte));
        sctk_ptl_chk(PtlPTFree(srail->iface, srail->ptl_info.am_pte));

        sctk_ptl_chk(PtlMEUnlink(srail->ptl_info.rma_meh));
        sctk_ptl_chk(PtlPTFree(srail->iface, srail->ptl_info.rma_pte));

	sctk_ptl_chk(PtlMDRelease(
		srail->ptl_info.mdh
	));

	sctk_ptl_chk(PtlEQFree(
		srail->ptl_info.eqh
	));

	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_fini(srail);
}

#endif /* MPC_USE_PORTALS */

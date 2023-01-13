#include "lcr_ptl.h"

#include "lcp_common.h"

#include "sctk_ptl_iface.h"
#include "sctk_ptl_offcoll.h"
#include "lcr_ptl_recv.h"

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

int lcr_ptl_iface_progress(sctk_rail_info_t *rail)
{
	int did_poll = 0;
	int ret;
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
        lcr_ptl_send_comp_t *ptl_comp;
        lcr_tag_context_t *tag_ctx;
        lcr_ptl_recv_block_t *block;
        uint8_t am_id;
        unsigned flags = 0;

        while (1) {

                ret = PtlEQGet(srail->ptl_info.eqh, &ev);
                sctk_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, "
                                              "sz=%llu, user=%p, start=%p, "
                                              "remote_offset=%p", 
                                              sctk_ptl_event_decode(ev), ev.pt_index, 
                                              ev.mlength, ev.user_ptr, ev.start,
                                              ev.remote_offset);
                        did_poll = 1;

                        switch (ev.type) {
                        case PTL_EVENT_SEND:
                                not_reachable();
                                break;
                        case PTL_EVENT_REPLY:
                                ptl_comp = (lcr_ptl_send_comp_t *)ev.user_ptr;
                                assert(ptl_comp);
                                ptl_comp->tag_ctx->comp.sent = ev.mlength;
                                ptl_comp->tag_ctx->
                                        comp.comp_cb(&ptl_comp->tag_ctx->comp);
                                sctk_free(ptl_comp);
                                goto done;
                                break;
                        case PTL_EVENT_ACK:
                                ptl_comp = (lcr_ptl_send_comp_t *)ev.user_ptr;
                                assert(ptl_comp);
                                switch (ptl_comp->type) {
                                case LCR_PTL_COMP_AM_BCOPY:
                                        sctk_free(ptl_comp->bcopy_buf);
                                        break;
                                case LCR_PTL_COMP_AM_ZCOPY:
                                        ptl_comp->comp->sent = ev.mlength;
                                        ptl_comp->comp->comp_cb(ptl_comp->comp);
                                        sctk_ptl_chk(PtlMDRelease(ptl_comp->iov_mdh));
                                        sctk_free(ptl_comp->iov);
                                        break;
                                case LCR_PTL_COMP_TAG_ZCOPY:
                                        ptl_comp->tag_ctx->comp.sent = ev.mlength;
                                        ptl_comp->tag_ctx->comp.comp_cb(&ptl_comp->tag_ctx->comp);
                                        break;
                                case LCR_PTL_COMP_RMA_PUT:
                                        ptl_comp->tag_ctx->comp.sent = ev.mlength;
                                        ptl_comp->tag_ctx->
                                                comp.comp_cb(&ptl_comp->tag_ctx->comp);
                                        break;
                                default:
                                        mpc_common_debug_error("LCR PTL: unknown "
                                                               "completion type");
                                        break;
                                }
                                sctk_free(ptl_comp);
                                goto done;
                                break;
                        case PTL_EVENT_PUT_OVERFLOW:
                        case PTL_EVENT_GET_OVERFLOW:
                                flags = LCR_IFACE_TM_OVERFLOW;
                        case PTL_EVENT_PUT:
                                switch (ev.pt_index) {
                                case LCR_PTL_PTE_IDX_TAG_EAGER:
                                        ptl_comp  = (lcr_ptl_send_comp_t *)ev.user_ptr;
                                        switch (ptl_comp->type) {
                                        case LCR_PTL_COMP_BLOCK:
                                                break;
                                        case LCR_PTL_COMP_TAG_ZCOPY:
                                                tag_ctx            = ptl_comp->tag_ctx;
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
                                        goto done;
                                        break;
                                case LCR_PTL_PTE_IDX_AM_EAGER:
                                        am_id = (uint8_t)ev.hdr_data;
                                        lcr_ptl_invoke_am(rail, am_id, ev.mlength, 
                                                          ev.start);
                                        goto done;
                                        break;
                                default:
                                        mpc_common_debug_fatal("LCR PTL: wrong pte");
                                        break;
                                }
                                break;
                        case PTL_EVENT_GET:
                                goto done;
                                break;
                        case PTL_EVENT_AUTO_UNLINK:
                                ptl_comp = (lcr_ptl_send_comp_t *)ev.user_ptr;
                                block = mpc_container_of(ptl_comp, lcr_ptl_recv_block_t, comp);
                                sctk_ptl_list_t list = ev.pt_index == LCR_PTL_PTE_IDX_TAG_EAGER ? 
                                        SCTK_PTL_OVERFLOW_LIST : SCTK_PTL_PRIORITY_LIST;
                                lcr_ptl_recv_block_activate(block, ev.pt_index, list);
                                goto done;
                                break;
                        case PTL_EVENT_AUTO_FREE:
                                goto done;
                                break;
                        case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
                        case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
                                goto done;
                                break;
                        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
                        case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
                        case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
                        case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
                                /* probably nothing to do here */
                        case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
                                not_reachable();              /* have been disabled */
                                break;
                        default:
                                break;
                        }
                } else if (ret == PTL_EQ_EMPTY) {
                        goto done;
                        break;
                } else {
                        mpc_common_debug_error("LCR PTL: error returned from PtlEQPoll");
                        break;
                }
        }

done:
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
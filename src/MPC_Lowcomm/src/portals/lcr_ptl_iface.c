#include "lcr_ptl.h"

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
        lcr_ptl_recv_block_t *block;
        uint8_t am_id;

        while (1) {

                ret = PtlEQGet(srail->ptl_info.me_eq, &ev);
                sctk_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, "
                                              "sz=%llu, user=%p, start=%p", 
                                              sctk_ptl_event_decode(ev), ev.pt_index, 
                                              //__sctk_ptl_match_str(malloc(32), 32, 
                                              //                     ev.match_bits), 
                                              ev.mlength, ev.user_ptr, ev.start);
                        did_poll = 1;

                        switch (ev.type) {
                        case PTL_EVENT_SEND:
                                not_reachable();
                                break;
                        case PTL_EVENT_REPLY:
                                ptl_comp = (lcr_ptl_send_comp_t *)ev.user_ptr;
                                assert(ptl_comp);
                                ptl_comp->comp->sent = ev.mlength;
                                ptl_comp->tag_ctx->
                                        comp.comp_cb(&ptl_comp->tag_ctx->comp);
                                sctk_free(ptl_comp);
                                goto done;
                                break;
                        case PTL_EVENT_ACK:
                                ptl_comp = (lcr_ptl_send_comp_t *)ev.user_ptr;
                                assert(ptl_comp);
                                switch (ptl_comp->type) {
                                case LCR_PTL_COMP_BCOPY:
                                        sctk_free(ptl_comp->bcopy_buf);
                                        break;
                                case LCR_PTL_COMP_ZCOPY:
                                        ptl_comp->comp->sent = ev.mlength;
                                        ptl_comp->comp->comp_cb(ptl_comp->comp);
                                        sctk_ptl_chk(PtlMDRelease(ptl_comp->iov_mdh));
                                        break;
                                case LCR_PTL_COMP_PUT:
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
                                goto done;
                                break;
                        case PTL_EVENT_PUT:
                                am_id = (uint8_t)ev.hdr_data;
                                lcr_ptl_invoke_am(rail, am_id, ev.mlength, 
                                                  ev.start);
                                goto done;
                                break;
                        case PTL_EVENT_GET:
                                goto done;
                                break;
                        case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
                                block = (lcr_ptl_recv_block_t *)ev.user_ptr;
                                lcr_ptl_recv_block_activate(block);
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
                        case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
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

int lcr_ptl_iface_mprogress(sctk_rail_info_t *rail)
{
	int did_poll = 0;
	int ret;
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
        sctk_ptl_local_data_t *user_ptr;
        lcr_tag_context_t *tag_ctx;
        sctk_ptl_pte_t *pte;

        while (1) {

                ret = PtlEQGet(srail->ptl_info.me_eq, &ev);
                sctk_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, "
                                              "match=%s,  sz=%llu, user=%p, "
                                              "start=%p", sctk_ptl_event_decode(ev), 
                                              ev.pt_index, 
                                              __sctk_ptl_match_str(malloc(32), 32, 
                                                                   ev.match_bits), 
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

                                goto done;
                                break;
                        case PTL_EVENT_PUT_OVERFLOW:
                        case PTL_EVENT_GET_OVERFLOW:
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

                                sctk_free(ev.start);
                                sctk_free(user_ptr);
                                goto done;
                                break;
                        case PTL_EVENT_PUT:
                        case PTL_EVENT_GET:
                                user_ptr = (sctk_ptl_local_data_t *)ev.user_ptr;
                                if (user_ptr->msg == NULL) { 
                                        pte = lcr_ptl_pte_idx_to_pte(srail, ev.pt_index);
                                        sctk_ptl_me_feed(srail, pte, srail->eager_limit, 1, 
                                                         SCTK_PTL_OVERFLOW_LIST, 
                                                         SCTK_PTL_TYPE_STD, 
                                                         SCTK_PTL_PROT_NONE);

                                        sctk_free(user_ptr);
                                        goto done;
                                }
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
                        case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
                        case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
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

int lcr_ptl_software_init_common(sctk_ptl_rail_info_t *srail)
{
        int md_flags;
	sctk_ptl_local_data_t *md_req;

        /* create the EQ */
        sctk_ptl_chk(PtlEQAlloc(srail->iface,         /* the NI handler */
                                SCTK_PTL_EQ_PTE_SIZE, /* the number of slots in the EQ */
                                &srail->ptl_info.me_eq        /* the returned handler */
                               ));

        /* normal MD */
	md_flags   = PTL_MD_EVENT_SEND_DISABLE;
	md_req = sctk_ptl_md_create(srail, 0, PTL_SIZE_MAX, md_flags);
        md_req->slot.md.eq_handle = srail->ptl_info.me_eq;

	sctk_ptl_chk(PtlMDBind(
		srail->iface,           /* the NI handler */
		&md_req->slot.md,   /* the MD to bind with memory region */
		&md_req->slot_h.mdh /* out: the MD handler */
	)); 
        srail->ptl_info.md_req = md_req;

	OPA_store_int(&srail->rdma_cpt, 0);
	if(srail->max_mr > srail->max_limits.max_msg_size)
		srail->max_mr = srail->max_limits.max_msg_size;
	
	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_init(srail);


        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_software_minit(sctk_ptl_rail_info_t *srail, size_t comm_dims)
{
        int rc;
	srail->nb_entries = 0;

	comm_dims = SCTK_PTL_PTE_HIDDEN + comm_dims;
	srail->nb_entries = comm_dims;
        
        rc = lcr_ptl_software_init_common(srail);

        int i;
	sctk_ptl_pte_t * table = NULL;
	table = sctk_malloc(sizeof(sctk_ptl_pte_t) * comm_dims); /* one CM, one recovery, one RDMA */
	memset(table, 0, comm_dims * sizeof(sctk_ptl_pte_t));
	
	mpc_common_hashtable_init(&srail->pt_table, (comm_dims < 64) ? 64 : comm_dims);
	mpc_common_hashtable_init(&srail->reverse_pt_table, (comm_dims < 64) ? 64 : comm_dims);

	for (i = 0; i < SCTK_PTL_PTE_HIDDEN; i++)
	{
		/* register the PT */
		sctk_ptl_chk(PtlPTAlloc(
			srail->iface,          /* the NI handler */
			SCTK_PTL_PTE_FLAGS,    /* PT entry specific flags */
			srail->ptl_info.me_eq, /* the EQ for this entry */
			i,                     /* the desired index value */
			&table[i].idx          /* the effective index value */
		));

	        mpc_common_hashtable_set(&srail->pt_table, table[i].idx, &table[i]);
                lcr_ptl_pte_idx_register(srail, table[i].idx, table[i].idx);
	}

        return rc;
}

int lcr_ptl_software_init(sctk_ptl_rail_info_t *srail, size_t comm_dims)
{
        int rc;
        ptl_me_t me_rndv;
        sctk_ptl_matchbits_t rdv_tag, ign_rdv_tag;
	srail->nb_entries = 0;

	comm_dims = SCTK_PTL_PTE_HIDDEN + comm_dims;
	srail->nb_entries = comm_dims;
        
        rc = lcr_ptl_software_init_common(srail);

        /* register the EAGER PT */
        sctk_ptl_chk(PtlPTAlloc(srail->iface,                 /* the NI handler */
                                SCTK_PTL_PTE_FLAGS,           /* PT entry specific flags */
                                srail->ptl_info.me_eq,        /* the EQ for this entry */
                                LCR_PTL_PTE_IDX_EAGER,        /* the desired index value */
                                &srail->ptl_info.eager_pt_idx /* the effective index value */
                               ));

        if (srail->ptl_info.eager_pt_idx != LCR_PTL_PTE_IDX_EAGER) {
                mpc_common_debug_error("LCR PTL: could not associate eager pte idx");
                return MPC_LOWCOMM_ERROR;
        }

        /* activate eager block */
        srail->ptl_info.block_list = NULL;
        rc = lcr_ptl_recv_block_enable(srail);

        /* register RNDV PT */
        sctk_ptl_chk(PtlPTAlloc(srail->iface,                /* the NI handler */
                                SCTK_PTL_PTE_FLAGS,          /* PT entry specific flags */
                                srail->ptl_info.me_eq,       /* the EQ for this entry */
                                LCR_PTL_PTE_IDX_RNDV,        /* the desired index value */
                                &srail->ptl_info.rndv_pt_idx /* the effective index value */
                               ));

        if (srail->ptl_info.rndv_pt_idx != LCR_PTL_PTE_IDX_RNDV) {
                mpc_common_debug_error("LCR PTL: could not associate rndv pte idx");
                return MPC_LOWCOMM_ERROR;
        }

        rdv_tag     = SCTK_PTL_MATCH_INIT;
        ign_rdv_tag = SCTK_PTL_IGN_ALL;
        me_rndv = (ptl_me_t) {
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
                                 srail->ptl_info.rndv_pt_idx,
                                 &me_rndv,
                                 SCTK_PTL_PRIORITY_LIST,
                                 NULL,
                                 &srail->ptl_info.meh_rndv
                                ));

        return rc;
}

int lcr_ptl_software_fini_common(sctk_ptl_rail_info_t *srail)
{
	sctk_ptl_md_release(srail->ptl_info.md_req);

	sctk_ptl_chk(PtlEQFree(
		srail->ptl_info.me_eq          /* the EQ handler */
	));

	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_fini(srail);

        return MPC_LOWCOMM_SUCCESS;
}

void lcr_ptl_software_fini(sctk_ptl_rail_info_t* srail)
{
        lcr_ptl_recv_block_disable(srail);

        sctk_ptl_chk(PtlPTFree(srail->iface, srail->ptl_info.eager_pt_idx));

        sctk_ptl_chk(PtlMEUnlink(srail->ptl_info.meh_rndv));
        sctk_ptl_chk(PtlPTFree(srail->iface, srail->ptl_info.rndv_pt_idx));

        lcr_ptl_software_fini_common(srail);
}

void lcr_ptl_software_mfini(sctk_ptl_rail_info_t* srail)
{
	int table_dims = srail->nb_entries;
	assert(table_dims > 0);
	int i;
	void* base_ptr = NULL;

	/* don't want to hang the NIC */
	//return;

	for(i = 0; i < table_dims; i++)
	{
		sctk_ptl_pte_t* cur = mpc_common_hashtable_get(&srail->pt_table, i);


		if(i==0)
			base_ptr = cur;

		if (cur) {
                        if(sctk_ptl_offcoll_enabled(srail))
                                sctk_ptl_offcoll_pte_fini(srail, cur);

#ifndef MPC_LOWCOMM_PROTOCOL
                        sctk_ptl_chk(PtlEQFree(
                                               pte->eq       /* the EQ handler */
                                              ));
#endif

                        sctk_ptl_chk(PtlPTFree(
                                               srail->iface,     /* the NI handler */
                                               cur->idx      /* the PTE to destroy */
                                              ));	

                        srail->nb_entries--;
                }
        }

	/* write 'NULL' to be sure */
	sctk_free(base_ptr);
	mpc_common_hashtable_release(&srail->pt_table);
	mpc_common_hashtable_release(&srail->reverse_pt_table);

	srail->nb_entries = 0;

        lcr_ptl_software_fini_common(srail);
}

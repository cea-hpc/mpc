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

#include "ptl_iface.h"

#include <limits.h>

#include "ptl_types.h"
#include "ptl_recv.h"

#include <sctk_alloc.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>
#include "rail.h"

static inline void __ptl_set_limits(ptl_ni_limits_t* l)
{
	*l = (ptl_ni_limits_t){
		.max_entries = INT_MAX,                /* Max number of entries allocated at any time */
		.max_unexpected_headers = INT_MAX,     /* max number of headers at any time */
		.max_mds = INT_MAX,                    /* Max number of MD allocated at any time */
		.max_cts = INT_MAX,                    /* Max number of CT allocated at any time */
		.max_eqs = INT_MAX,                    /* Max number of EQ allocated at any time */
		.max_pt_index = INT_MAX,               /* Max PT index */
		.max_iovecs = INT_MAX,                 /* max number of iovecs for a single MD */
		.max_list_size = INT_MAX,              /* Max number of entrie for one PT entry */
		.max_triggered_ops = INT_MAX,          /* Max number of triggered ops */
		/*.max_cids = INT_MAX,                   [> max number of CID's (?) <]*/
		.max_msg_size = PTL_SIZE_MAX,          /* max message's size */
		.max_atomic_size = PTL_SIZE_MAX,       /* max size writable atomically */
		.max_fetch_atomic_size = PTL_SIZE_MAX, /* Max size for a fetch op */
		.max_waw_ordered_size = PTL_SIZE_MAX,  /* Max length for ordering-guarantee */
		.max_war_ordered_size = PTL_SIZE_MAX,  /* max length for ordering guarantee */
		.max_volatile_size = PTL_SIZE_MAX,     /* Max length for MD w/ VOLATILE flag */
		.features = 0                          /* could be a combination or TARGET_BIND_INACCESSIBLE | TOTAL_DATA_ORDERING | COHERENT_ATOMIC */
	};
}

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

static int _lcr_ptl_iface_progress_am(sctk_rail_info_t *rail)
{
	int did_poll = 0;
	int ret;
	sctk_ptl_event_t ev;
	lcr_ptl_rail_info_t* srail = &rail->network.ptl;
        lcr_ptl_op_t *op;
        lcr_ptl_recv_block_t *block;
        uint8_t am_id;

        while (1) {

                mpc_common_spinlock_lock(&srail->am_ctx.eq_lock);

                ret = PtlEQGet(srail->am_ctx.eqh, &ev);
                lcr_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, "
                                              "sz=%llu, user=%p, start=%p, "
                                              "remote_offset=%p, iface=%d", 
                                              lcr_ptl_event_decode(ev), ev.pt_index, 
                                              ev.mlength, ev.user_ptr, ev.start,
                                              ev.remote_offset, srail->nih);
                        did_poll = 1;

                        op = (lcr_ptl_op_t *)ev.user_ptr;
                        assert(op);

                        switch (ev.type) {
                        case PTL_EVENT_SEARCH:
                                not_reachable();
                                break;
                        case PTL_EVENT_SEND:
                                not_reachable();
                                break;
                        case PTL_EVENT_REPLY:
                                not_reachable();
                                break;
                        case PTL_EVENT_ACK:
                                switch (op->type) {
                                case LCR_PTL_OP_AM_BCOPY:
                                        sctk_free(op->am.bcopy_buf);
                                        break;
                                case LCR_PTL_OP_AM_ZCOPY:
                                        op->comp->sent = ev.mlength;
                                        op->comp->comp_cb(op->comp);
                                        lcr_ptl_chk(PtlMDRelease(op->iov.iovh));
                                        sctk_free(op->iov.iov);
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
                        case PTL_EVENT_PUT:
                                am_id = (uint8_t)ev.hdr_data;
                                lcr_ptl_invoke_am(rail, am_id, ev.mlength, 
                                                  ev.start);
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_GET_OVERFLOW:
                                mpc_common_debug_info("LCR PTL: got get overflow, not "
                                                      "possible!");
                                goto poll_unlock;
                                break;
                        case PTL_EVENT_GET:
                                not_reachable();
                                break;
                        case PTL_EVENT_AUTO_UNLINK:
                                block = mpc_container_of(op, lcr_ptl_recv_block_t, op);
                                lcr_ptl_recv_block_activate(block, ev.pt_index, PTL_PRIORITY_LIST);
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
poll_unlock:
                mpc_common_spinlock_unlock(&srail->am_ctx.eq_lock);
                break;
        }

        return did_poll;
}

static int _lcr_ptl_iface_progress_tag(sctk_rail_info_t *rail)
{
	int did_poll = 0;
	int ret;
	sctk_ptl_event_t ev;
	lcr_ptl_rail_info_t* srail = &rail->network.ptl;
        lcr_ptl_op_t *op;
        lcr_tag_context_t *tag_ctx;
        lcr_ptl_recv_block_t *block;
        unsigned flags = 0;

        while (1) {

                mpc_common_spinlock_lock(&srail->tag_ctx.eq_lock);

                ret = PtlEQGet(srail->tag_ctx.eqh, &ev);
                lcr_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' idx=%d, "
                                              "sz=%llu, user=%p, start=%p, "
                                              "remote_offset=%p, iface=%d", 
                                              lcr_ptl_event_decode(ev), ev.pt_index, 
                                              ev.mlength, ev.user_ptr, ev.start,
                                              ev.remote_offset, srail->nih);
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
                                mpc_mpool_push(op);
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
                                        sctk_free(op->am.bcopy_buf);
                                        break;
                                case LCR_PTL_OP_TAG_ZCOPY:
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
                                if (op->type == LCR_PTL_OP_BLOCK) {
                                        //NOTE: block operation are not from
                                        //      a memory pool, so dont push it
                                        //      back.
                                        break;
                                }
                                tag_ctx            = op->tag.tag_ctx;
                                tag_ctx->tag       = (lcr_tag_t)ev.match_bits;
                                tag_ctx->imm       = ev.hdr_data;
                                tag_ctx->start     = ev.start;
                                tag_ctx->comp.sent = ev.mlength;
                                tag_ctx->flags    |= flags;

                                /* call completion callback */
                                tag_ctx->comp.comp_cb(&tag_ctx->comp);
                                mpc_mpool_push(op);
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
                                mpc_mpool_push(op);
                                break;
                        case PTL_EVENT_AUTO_UNLINK:
                                block = mpc_container_of(op, lcr_ptl_recv_block_t, op);
                                lcr_ptl_recv_block_activate(block, ev.pt_index, PTL_OVERFLOW_LIST);
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
poll_unlock:
                mpc_common_spinlock_unlock(&srail->tag_ctx.eq_lock);
                break;
        }

        return did_poll;
}

static int _lcr_ptl_iface_progress_rma(sctk_rail_info_t *rail)
{
        int count = 0;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;
        lcr_ptl_rma_handle_t *rmah = NULL;
        ptl_ct_event_t ev;
        size_t op_it = 0;
        lcr_ptl_op_t *op;

        mpc_common_spinlock_lock(&srail->os_ctx.rmah_lock);
        mpc_list_for_each(rmah, &(srail->os_ctx.rmah_head), lcr_ptl_rma_handle_t, elem) {
                lcr_ptl_chk(PtlCTGet(rmah->cth, &ev));

                if (ev.failure > 0) {
                        mpc_common_debug_error("LCR PTL: error on RMA "
                                               "operation %p", rmah);
                        goto unlock;
                }

                /* Complete all succeeded operations. */
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
        mpc_common_spinlock_unlock(&srail->os_ctx.rmah_lock);
        return count;
}

int lcr_ptl_iface_progress(sctk_rail_info_t *rail) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        unsigned features = rail->network.ptl.features;

        if (features & LCR_PTL_FEATURE_AM) {
                rc = _lcr_ptl_iface_progress_am(rail);
        }
        if (features & LCR_PTL_FEATURE_TAG) {
                rc = _lcr_ptl_iface_progress_tag(rail);
        }
        if (features & LCR_PTL_FEATURE_AM) {
                rc = _lcr_ptl_iface_progress_rma(rail);
        }

        return rc;
}

/**
 * Create the link between our program and the real driver.
 * \return the Portals rail object
 */
lcr_ptl_rail_info_t lcr_ptl_hardware_init(sctk_ptl_interface_t iface)
{

	/* BXI specific We first need to make sure we do not exhaust command queues on the BXI
	   by settign a few internal variables for CQs sharing */
	int local_proc_count = mpc_common_get_local_process_count();

	if(local_proc_count >= 62)
	{
		int cqmaxpid = (local_proc_count / 62) + 1;
		if(cqmaxpid > 8)
		{
			cqmaxpid = 8;
		}
		char val[16];
		snprintf(val, 16, "%d", cqmaxpid);
		/* Set not overwriting */
		setenv("PORTALS4_CQ_MAXPIDS", val, 0);

		uint64_t job_id = 1111;
		mpc_launch_pmi_get_job_id(&job_id);
		snprintf(val, 16, "%lu", job_id);

		setenv("PORTALS4_CQ_GROUP", val, 0);
	}

	lcr_ptl_rail_info_t res;
	memset(&res, 0, sizeof(lcr_ptl_rail_info_t));
	sctk_ptl_limits_t l;

	/* pre-actions */
	assert(sizeof(sctk_ptl_std_content_t)     <= sizeof(ptl_match_bits_t));
	assert(sizeof(sctk_ptl_offload_content_t) <= sizeof(ptl_match_bits_t));
	assert(sizeof(sctk_ptl_matchbits_t)       <= sizeof(ptl_match_bits_t));
	assert(sizeof(sctk_ptl_std_data_t)        <= sizeof(ptl_hdr_data_t));
	assert(sizeof(sctk_ptl_cm_data_t)         <= sizeof(ptl_hdr_data_t));
	assert(sizeof(sctk_ptl_offload_data_t)    <= sizeof(ptl_hdr_data_t));
	assert(sizeof(sctk_ptl_imm_data_t)        <= sizeof(ptl_hdr_data_t));
	
	/* init the driver */
	lcr_ptl_chk(PtlInit());

	/* set max values */
	__ptl_set_limits(&l);

	/* init one physical interface */
	lcr_ptl_chk(PtlNIInit(
		iface,                             /* Type of interface */
		PTL_NI_MATCHING | PTL_NI_PHYSICAL, /* physical NI, w/ matching enabled */
		PTL_PID_ANY,                       /* Let Portals decide process's PID */
		&l,                                /* desired Portals boundaries */
		&res.config.max_limits,            /* effective Portals limits */
		&res.nih                           /* THE handler */
	));

	/* retrieve the process identifier */
	lcr_ptl_chk(PtlGetPhysId(
		res.nih, /* The NI handler */
		&res.addr.id    /* the structure to store it */
	));

	return res;
}

static inline void _lcr_ptl_iface_init_driver_config(lcr_ptl_rail_info_t *srail, 
                                      struct _mpc_lowcomm_config_struct_net_driver_portals *driver_config)
{
        srail->config.eager_limit = driver_config->eager_limit < sizeof(size_t) ? 
                sizeof(size_t) : driver_config->eager_limit;

	srail->config.max_mr      = driver_config->max_msg_size < 
                srail->config.max_limits.max_msg_size ?
		(unsigned long int)driver_config->max_msg_size : 
                srail->config.max_limits.max_msg_size;
        srail->config.max_get = srail->config.max_mr;
        srail->config.max_put = srail->config.max_mr;

        srail->config.max_iovecs = driver_config->max_iovecs;
        srail->config.num_eager_blocks = driver_config->num_eager_blocks;
        srail->config.eager_block_size = driver_config->eager_block_size;

}

/* Initialize ptl resources needed for Active Message interface. It requires
 * both two-sided and one-sided portals context. Also, the EQ is shared among
 * all context so that it will be instanciated except if the only feature
 * required is RMA. */
static int _lcr_ptl_iface_init_am(lcr_ptl_rail_info_t *srail) 
{
        int rc;
        ptl_md_t md;

        /* Initialize two-sided required resources:
         * - one Event Queue (EQ);
         * - one Memory Descriptor (MD) spanning all address space;
         * - blocks of Memory Entries (ME) to receive unexpected messages;
         * - one Portals Table (PT) used for operations. */

        /* Create the EQ */
        lcr_ptl_chk(PtlEQAlloc(srail->nih,
                                PTL_EQ_PTE_SIZE, 
                                &srail->am_ctx.eqh
                               ));


        /* Init poll lock used to ensure in-order message reception */
        mpc_common_spinlock_init(&srail->am_ctx.eq_lock, 0);

        /* MD spanning all addressable memory */
        md = (ptl_md_t) {
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->am_ctx.eqh,
                .length = PTL_SIZE_MAX,
                .start = 0,
                .options = PTL_MD_EVENT_SEND_DISABLE
        };

	lcr_ptl_chk(PtlMDBind(
		srail->nih,
		&md,
		&srail->am_ctx.mdh 
	)); 

        /* Register portal table used for AM communication. */
        lcr_ptl_chk(PtlPTAlloc(srail->nih,
                                PTL_PT_FLOWCTRL,
                                srail->am_ctx.eqh,
                                PTL_PT_ANY,
                                &srail->am_ctx.pti
                               ));

        /* Activate am eager block */
        srail->am_ctx.blist = NULL;
        rc = lcr_ptl_recv_block_enable(srail, srail->am_ctx.pti, 
                                       PTL_PRIORITY_LIST);

        return rc;
}

static int _lcr_ptl_iface_fini_am(lcr_ptl_rail_info_t *srail) 
{
        lcr_ptl_recv_block_disable(srail->am_ctx.blist);

	lcr_ptl_chk(PtlMDRelease(srail->am_ctx.mdh));

        lcr_ptl_chk(PtlPTFree(srail->nih, srail->am_ctx.pti));

	lcr_ptl_chk(PtlEQFree(srail->am_ctx.eqh));

        //TODO: ongoing request management.
        return MPC_LOWCOMM_SUCCESS;
}

/* Initialize ptl resources needed for TAG interface. */
static int _lcr_ptl_iface_init_tag(lcr_ptl_rail_info_t *srail) 
{
        int rc;
        ptl_md_t md;

        /* Create the EQ */
        lcr_ptl_chk(PtlEQAlloc(srail->nih,
                                PTL_EQ_PTE_SIZE, 
                                &srail->tag_ctx.eqh
                               ));

        /* MD spanning all addressable memory */
        md = (ptl_md_t) {
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->tag_ctx.eqh,
                .length = PTL_SIZE_MAX,
                .start = 0,
                .options = PTL_MD_EVENT_SEND_DISABLE
        };

	lcr_ptl_chk(PtlMDBind(
		srail->nih,
		&md,
		&srail->tag_ctx.mdh 
	)); 

        //FIXME: only allocate if offload is enabled
        /* register the TAG EAGER PTE */
        lcr_ptl_chk(PtlPTAlloc(srail->nih,
                                PTL_PT_FLOWCTRL,
                                srail->tag_ctx.eqh,
                                PTL_PT_ANY,
                                &srail->tag_ctx.pti
                               ));

        /* activate tag eager block */
        srail->tag_ctx.blist= NULL;
        rc = lcr_ptl_recv_block_enable(srail, srail->tag_ctx.pti, 
                                       PTL_OVERFLOW_LIST);

        return rc;
}

static int _lcr_ptl_iface_fini_tag(lcr_ptl_rail_info_t *srail) 
{
        lcr_ptl_recv_block_disable(srail->tag_ctx.blist);

        lcr_ptl_chk(PtlPTFree(srail->nih, srail->tag_ctx.pti));

	lcr_ptl_chk(PtlEQFree(srail->tag_ctx.eqh));

        //TODO: ongoing request management.
        return MPC_LOWCOMM_SUCCESS;
}

/* Initialize ptl resources needed for RMA interface. */
static int _lcr_ptl_iface_init_rma(lcr_ptl_rail_info_t *srail) 
{
        ptl_me_t me_rma;
        ptl_match_bits_t rdv_tag = 0, ign_rdv_tag = 0;

        /* Initialize one-sided required resources:
         * - one Memory Entries (ME) spanning all address space with
         *   communications disable;
         * - one Portals Table (PT) used for operations. */

        /* register RNDV PT */
        lcr_ptl_chk(PtlPTAlloc(srail->nih,
                                0 /* No options. */,
                                PTL_EQ_NONE,
                                PTL_PT_ANY,
                                &srail->os_ctx.pti
                               ));


        rdv_tag     = 0;
        ign_rdv_tag = 0;
        me_rma = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                .ignore_bits = ign_rdv_tag,
                .match_bits  = rdv_tag,
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
                        PTL_ME_EVENT_COMM_DISABLE,
                .start       = 0,
                .length      = PTL_SIZE_MAX
        };

        lcr_ptl_chk(PtlMEAppend(srail->nih,
                                 srail->os_ctx.pti,
                                 &me_rma,
                                 PTL_PRIORITY_LIST,
                                 NULL,
                                 &srail->os_ctx.rma_meh
                                ));

        /* Initialize list of active RMA handles. */
        mpc_list_init_head(&srail->os_ctx.rmah_head);

        return MPC_LOWCOMM_SUCCESS;
}

static int _lcr_ptl_iface_fini_rma(lcr_ptl_rail_info_t *srail) 
{
        lcr_ptl_chk(PtlMEUnlink(srail->os_ctx.rma_meh));

        lcr_ptl_chk(PtlPTFree(srail->nih, srail->os_ctx.pti));

        //TODO: ongoing request management.
        return MPC_LOWCOMM_SUCCESS;
}

static int __ptl_monitor_callback(mpc_lowcomm_peer_uid_t from,
						   char *data,
						   char * return_data,
						   int return_data_len,
						   void *ctx)
{
	UNUSED(from);
	UNUSED(data);
	sctk_rail_info_t* rail = (sctk_rail_info_t*)ctx;
	lcr_ptl_rail_info_t* srail = &rail->network.ptl;

	assume(0 < srail->connection_infos_size);
	assume((int)srail->connection_infos_size < return_data_len);

	mpc_common_debug("PORTALS OD CB SENDS %s", srail->connection_infos);

	snprintf(return_data, return_data_len, "%s", srail->connection_infos);

	return 0;
}

static inline void __register_monitor_callback(sctk_rail_info_t* rail)
{
	char rail_name[32];
	mpc_lowcomm_monitor_register_on_demand_callback(__ptl_get_rail_callback_name(rail->rail_number, rail_name, 32), 
                                                        __ptl_monitor_callback, rail);
}


int lcr_ptl_iface_init(sctk_rail_info_t *rail, unsigned fflags)
{
        int rc;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;
	struct _mpc_lowcomm_config_struct_net_driver_portals *ptl_driver_config =
                &rail->runtime_config_driver_config->driver.value.portals;

	/* Register on-demand callback for rail */
	__register_monitor_callback(rail);

        /* Hardware initialization is needed by all features (AM, TAG, RMA). */
        *srail = lcr_ptl_hardware_init(PTL_IFACE_DEFAULT + (unsigned int)rail->rail_number);

        _lcr_ptl_iface_init_driver_config(srail, ptl_driver_config);

        /* First initialize PT. */
        srail->am_ctx.pti  = LCR_PTL_PT_NULL;
        srail->tag_ctx.pti = LCR_PTL_PT_NULL;
        srail->os_ctx.pti  = LCR_PTL_PT_NULL;
       
        if (fflags & LCR_PTL_FEATURE_AM) {
                _lcr_ptl_iface_init_am(srail);
                srail->features |= LCR_PTL_FEATURE_AM;
        }

        if (fflags & LCR_PTL_FEATURE_RMA) {
                _lcr_ptl_iface_init_rma(srail);
                srail->features |= LCR_PTL_FEATURE_RMA;
        }

        if (fflags & LCR_PTL_FEATURE_TAG) {
                _lcr_ptl_iface_init_tag(srail);
                srail->features |= LCR_PTL_FEATURE_TAG;
        }

        /* Build the Portals address. */
        srail->addr = (lcr_ptl_addr_t) {
                .id      = srail->addr.id,
                .pts.am  = srail->am_ctx.pti,
                .pts.tag = srail->tag_ctx.pti,
                .pts.rma = srail->os_ctx.pti,
        };

        /* Init pool of operations. */
        srail->ops_pool = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->ops_pool == NULL) {
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

        rc = mpc_mpool_init(srail->ops_pool, &mp_op_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Initialize hashtable that store portals addresses. */ 
	mpc_common_hashtable_init(&srail->ranks_ids_map, 
                                  mpc_common_get_process_count());

err:
        return rc;
}

int lcr_ptl_iface_fini(sctk_rail_info_t* rail)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;
        
        if (srail->features & LCR_PTL_FEATURE_AM) 
                rc = _lcr_ptl_iface_fini_am(srail);

        if (srail->features & LCR_PTL_FEATURE_TAG) 
                rc = _lcr_ptl_iface_fini_tag(srail);

        if (srail->features & LCR_PTL_FEATURE_RMA) 
                rc = _lcr_ptl_iface_fini_rma(srail);

        mpc_mpool_fini(srail->ops_pool);

        return rc;
}

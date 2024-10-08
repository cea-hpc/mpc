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

#ifdef MPC_USE_PORTALS

#include <limits.h>

#include "ptl_recv.h"

#include <sctk_alloc.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>
#include <rail.h>

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
		mpc_common_debug_fatal("LCR PTL: handler id %d not supported.", am_id);
	}

	rc = handler.cb(handler.arg, data, length, LCR_IFACE_AM_LAYOUT_BUFFER);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCR PTL: handler id %d failed.", am_id);
	}

	return rc;
}

int lcr_ptl_complete_op(lcr_ptl_op_t *op)
{
        int rc = MPC_LOWCOMM_SUCCESS;

        switch (op->type) {
        case LCR_PTL_OP_AM_BCOPY:
        case LCR_PTL_OP_AM_ZCOPY:
        case LCR_PTL_OP_TAG_BCOPY:
        case LCR_PTL_OP_TAG_ZCOPY:
        case LCR_PTL_OP_TAG_SEARCH:
        case LCR_PTL_OP_RMA_PUT:
        case LCR_PTL_OP_RMA_GET:
        case LCR_PTL_OP_ATOMIC_FETCH:
        case LCR_PTL_OP_ATOMIC_POST:
        case LCR_PTL_OP_ATOMIC_CSWAP:
                if (op->comp != NULL) {
                        op->comp->sent = op->size;
                        op->comp->comp_cb(op->comp);
                }
                break;
        case LCR_PTL_OP_RMA_FLUSH:
                if (--op->flush.outstandings == 0) {
                        if (op->comp != NULL) {
                                op->comp->sent = op->size;
                                op->comp->comp_cb(op->comp);
                        }
                        sctk_free(op->flush.op_count);
                } else goto out;
                break;
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        case LCR_PTL_OP_TK_INIT:
        case LCR_PTL_OP_TK_REQUEST:
        case LCR_PTL_OP_TK_GRANT:
        case LCR_PTL_OP_TK_RELEASE:
                break;
#endif
        default:
                mpc_common_debug_error("LCR PTL: unknown operation type.");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

        mpc_common_debug("LCR PTL: complete op. id=%llu, op=%p, size=%llu, "
                         "nid=%lu, pid=%lu, pti=%d, type=%s, hdr=0x%08x.",
                         op->id, op, op->size, op->addr.phys.nid,
                         op->addr.phys.pid, op->pti, lcr_ptl_op_decode(op),
                         op->hdr);

        mpc_mpool_push(op);
out:
        return rc;
}


static int _lcr_ptl_iface_process_event_am(sctk_rail_info_t *rail,
                                           ptl_event_t *ev)
{
        uint8_t am_id;
        lcr_ptl_recv_block_t *block;
        lcr_ptl_op_t *op = (lcr_ptl_op_t *)ev->user_ptr;
        assert(op);

        switch (ev->type) {
        case PTL_EVENT_ACK:
                switch (op->type) {
                case LCR_PTL_OP_AM_BCOPY:
                        mpc_mpool_push(op->am.bcopy_buf);
                        break;
                case LCR_PTL_OP_AM_ZCOPY:
                        op->comp->sent = ev->mlength;
                        op->comp->comp_cb(op->comp);
                        lcr_ptl_chk(PtlMDRelease(op->mdh));
                        sctk_free(op->iov.iov);
                        break;
                default:
                        mpc_common_debug_error("LCR PTL: unknown "
                                               "completion type.");
                        break;
                }
                lcr_ptl_complete_op(op);
                break;
        case PTL_EVENT_PUT_OVERFLOW:
        case PTL_EVENT_PUT:
                /* First, invoke AM handle. */
                am_id = LCR_PTL_AM_HDR_GET_AM_ID(ev->hdr_data);
                lcr_ptl_invoke_am(rail, am_id, ev->mlength,
                                  ev->start);

#if defined(MPC_USE_PORTALS_CONTROL_FLOW)
                /* Second, handle control flow:
                 * - update token chunk based on current
                 *   contention info piggybacked from sender;
                 * - decrement outstanding token counter. */
                lcr_ptl_tk_release_rsc_token(&rail->network.ptl.net.tk,
                                             ev->initiator,
                                             LCR_PTL_AM_HDR_GET_PEND_OPS(ev->hdr_data));
#endif

                break;
        case PTL_EVENT_GET_OVERFLOW:
        case PTL_EVENT_GET:
                mpc_common_debug_info("LCR PTL: got get overflow, not "
                                      "possible!");
                break;
        case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
                break;
        case PTL_EVENT_AUTO_UNLINK:
                //FIXME: no need to have an op within the block structure. It
                //       could that a cast over ev->user_ptr is performed
                //       depending of event type.
                block = mpc_container_of(op, lcr_ptl_recv_block_t, op);
                lcr_ptl_recv_block_activate(block, ev->pt_index, PTL_PRIORITY_LIST);
                break;
        case PTL_EVENT_AUTO_FREE:
                break;
        case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
        case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
                break;
        case PTL_EVENT_SEARCH:
        case PTL_EVENT_SEND:
        case PTL_EVENT_REPLY:
        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* received FETCH-ATOMIC matched a just appended one */
        case PTL_EVENT_ATOMIC_OVERFLOW:       /* received ATOMIC matched a just appended one */
        case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
                not_reachable();              /* have been disabled */
                break;
        default:
                break;
        }

        return MPC_LOWCOMM_SUCCESS;
}

static int _lcr_ptl_iface_process_event_tag(sctk_rail_info_t *rail,
                                            ptl_event_t *ev)
{
        UNUSED(rail);
        unsigned flags = 0;
        lcr_ptl_recv_block_t *block;
        lcr_tag_context_t *tag_ctx;
        lcr_ptl_op_t *op = (lcr_ptl_op_t *)ev->user_ptr;
        assert(op);


        op = (lcr_ptl_op_t *)ev->user_ptr;
        assert(op);

        switch (ev->type) {
        case PTL_EVENT_SEARCH:
                if (ev->ni_fail_type == PTL_NI_NO_MATCH) {
                        return MPC_LOWCOMM_SUCCESS;
                }
                tag_ctx            = op->tag.tag_ctx;
                tag_ctx->tag       = (lcr_tag_t)ev->match_bits;
                tag_ctx->imm       = ev->hdr_data;
                tag_ctx->start     = NULL;
                tag_ctx->flags    |= flags;

                /* call completion callback */
                op->comp->sent = ev->mlength;
                op->comp->comp_cb(op->comp);
                lcr_ptl_complete_op(op);
                break;

        case PTL_EVENT_SEND:
                not_reachable();
                break;
        case PTL_EVENT_REPLY:
                assert(op->size == ev->mlength);
                op->comp->sent = ev->mlength;
                op->comp->comp_cb(op->comp);
                break;
        case PTL_EVENT_ACK:
                if (ev->ni_fail_type == PTL_NI_PT_DISABLED) {
                        //FIXME: need to handle such error. As soon as the first
                        //       event with such error, subsequent operations
                        //       should not be pulled from TX Queue, all event
                        //       should be drained from the Event Queue,
                        //       operation associated to these event need to be
                        //       handled respecting message ordering.
                        mpc_common_debug_fatal("LCR PTL: PT DISABLED. Increase "
                                               "num_eager_blocks.")
                }
                switch (op->type) {
                case LCR_PTL_OP_TAG_BCOPY:
                        sctk_free(op->am.bcopy_buf);
                        break;
                case LCR_PTL_OP_TAG_ZCOPY:
                        op->comp->sent = ev->mlength;
                        op->comp->comp_cb(op->comp);
                        break;
                default:
                        mpc_common_debug_error("LCR PTL: unknown "
                                               "completion type");
                        break;
                }
                lcr_ptl_complete_op(op);
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
                tag_ctx->tag       = (lcr_tag_t)ev->match_bits;
                tag_ctx->imm       = ev->hdr_data;
                tag_ctx->start     = ev->start;
                tag_ctx->comp.sent = ev->mlength;
                tag_ctx->flags    |= flags;

                /* call completion callback */
                tag_ctx->comp.comp_cb(&tag_ctx->comp);
                lcr_ptl_complete_op(op);
                break;
        case PTL_EVENT_GET_OVERFLOW:
                mpc_common_debug_info("LCR PTL: got get overflow, not "
                                      "possible!");
                break;
        case PTL_EVENT_GET:
                op->comp->sent = ev->mlength;
                op->comp->comp_cb(op->comp);
                lcr_ptl_complete_op(op);
                break;
        case PTL_EVENT_AUTO_UNLINK:
                block = mpc_container_of(op, lcr_ptl_recv_block_t, op);
                lcr_ptl_recv_block_activate(block, ev->pt_index, PTL_OVERFLOW_LIST);
                break;
        case PTL_EVENT_AUTO_FREE:
                break;
        case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
        case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
                break;
        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
        case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
        case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
        case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
                break;
        default:
                break;
        }

        return MPC_LOWCOMM_SUCCESS;
}

static int _lcr_ptl_iface_process_event_rma(sctk_rail_info_t *rail,
                                            ptl_event_t *ev)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_op_t *op;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;

        /* This mean an error has been encountered. */
        op = ev->user_ptr;

        if (ev->ni_fail_type != PTL_NI_OK) {
                mpc_common_debug_info("PORTALS: EQS NI EVENT '%s' idx=%d, "
                                      "sz=%llu, user=%p, start=%p, "
                                      "remote_offset=%p, iface=%d",
                                      lcr_ptl_event_ni_fail_type_decode(*ev), ev->pt_index,
                                      ev->mlength, ev->user_ptr, ev->start,
                                      ev->remote_offset, srail->net.nih);

                /* Target PTE has been disabled, retry operation. */
                if (ev->ni_fail_type == PTL_NI_PT_DISABLED) {
                        mpc_common_debug("LCR PTL: op retry.");

                        rc = lcr_ptl_do_op(op);
                        if (rc == MPC_LOWCOMM_ERROR) {
                                mpc_common_debug_error("LCR PTL: op error.");
                                goto err;
                        }
                } else {
                        //mpc_common_debug_fatal("LCR PTL: RMA error not handled.");
                }

                /* Event failure. */
                lcr_ptl_complete_op(op);
        }

err:
        return rc;
}

//NOTE: must be called with mem lock
ptl_size_t lcr_ptl_poll_mem(lcr_ptl_mem_t *mem)
{
        ptl_ct_event_t ct_ev;

        /* Poll CT and update operation count atomically. */
        lcr_ptl_chk(PtlCTGet(mem->cth, &ct_ev));
        if (ct_ev.failure > 0) {
                /* Failure will be treated with full events. */
                mpc_common_debug_warning("LCR PTL: %llu event failures.",
                                         ct_ev.failure);
        }
        return ct_ev.success;
}

int lcr_ptl_iface_progress_rma(lcr_ptl_rail_info_t *srail)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i, it = 0, num_eps = 0;
        int64_t completed = 0;
        lcr_ptl_mem_t   *mem;
        lcr_ptl_op_t    *flush_op;
        mpc_queue_iter_t iter;

        mpc_common_spinlock_lock(&srail->net.rma.lock);
        /* Loop on all registered memories. */
        mpc_list_for_each(mem, &srail->net.rma.poll_list, lcr_ptl_mem_t, elem) {
                completed = lcr_ptl_poll_mem(mem);

                mpc_common_spinlock_lock(&mem->lock);
                mpc_queue_for_each_safe(flush_op, iter, lcr_ptl_op_t, &mem->pending_flush, elem) {
                        if (flush_op->flush.op_count[it] <= completed) {
                                mpc_queue_del_iter(&mem->pending_flush, iter);
                                lcr_ptl_complete_op(flush_op);
                        }
                }
                mpc_common_spinlock_unlock(&mem->lock);
                it++;

                num_eps = atomic_load(&srail->num_eps);
                for (i = 0; i < num_eps; i++) {
                        lcr_ptl_flush_txq(mem, &mem->txqt[i], completed);
                }
        }
        mpc_common_spinlock_unlock(&srail->net.rma.lock);

        //FIXME: this sync is necessary to make sure host memory is
        //       synchronised, see lockcontention2.c from mpich test suite.
        //FIXME2: disabled for now for performance reasons.
        //PtlAtomicSync();

        return rc;
}

int lcr_ptl_iface_progress(sctk_rail_info_t *rail)
{
        int rc = MPC_LOWCOMM_SUCCESS;

	int ret;
	ptl_event_t ev;
        lcr_ptl_op_t *op;
	lcr_ptl_rail_info_t* srail = &rail->network.ptl;

        mpc_common_spinlock_lock(&srail->lock);
        while (1) {

                ret = PtlEQGet(srail->net.eqh, &ev);
                lcr_ptl_chk(ret);

                op = (lcr_ptl_op_t *)ev.user_ptr;
                if (ret == PTL_OK) {
                        mpc_common_debug("PORTALS: EQS EVENT '%s' eqh=%llu, idx=%d, "
                                         "sz=%llu, user=%p, start=%p, "
                                         "remote_offset=%llu, iface=%llu",
                                         lcr_ptl_event_decode(ev),
                                         srail->net.eqh,
                                         ev.pt_index, ev.mlength,
                                         ev.user_ptr, ev.start,
                                         ev.remote_offset, srail->net.nih);

                        if (ev.type == PTL_EVENT_PT_DISABLED) {
                                mpc_common_debug_fatal("LCR PTL: PT DISABLED. Increase "
                                                       "num_eager_block.");
                        } else if (op->pti == srail->net.am.pti) {
                                _lcr_ptl_iface_process_event_am(rail, &ev);
                        } else if (op->pti == srail->net.tag.pti) {
                                _lcr_ptl_iface_process_event_tag(rail, &ev);
                        } else if (op->pti == srail->net.rma.pti) {
                                _lcr_ptl_iface_process_event_rma(rail, &ev);
                        } else {
                                mpc_common_debug_fatal("LCR PTL: unkown PTE index.");
                        }
                        continue;
                } else if (ret == PTL_EQ_EMPTY) {
                        break;
                } else {
                        mpc_common_debug_error("LCR PTL: error returned from PtlEQPoll.");
                        break;
                }
        }
        mpc_common_spinlock_unlock(&srail->lock);

        /* Poll all attached memory. */
        lcr_ptl_iface_progress_rma(srail);

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        rc = lcr_ptl_iface_progress_tk(srail);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCR PTL: progress tk.");
        }

        rc = lcr_ptl_tk_distribute_tokens(srail);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCR PTL: distribute.");
        }

        int i, count = 0;
        for (i = 0; i < srail->num_eps; i++) {
                rc = lcr_ptl_tk_progress_pending_ops(srail,
                                                     srail->ept[i],
                                                     &count);
                if (rc != MPC_LOWCOMM_SUCCESS) break;
        }
#endif

        return rc;
}

static void lcr_ptl_iface_print_limits(ptl_ni_limits_t *ni_limits)
{
	mpc_common_debug("LCR PTL: limits:");
	assert(ni_limits);
#define s(x)  mpc_common_debug("%-25s: %lu", #x, (long unsigned int) (ni_limits)->x)
	s(max_entries);
	s(max_unexpected_headers);
	s(max_mds);
	s(max_cts);
	s(max_eqs);
	s(max_pt_index);
	s(max_iovecs);
	s(max_list_size);
	s(max_triggered_ops);
	s(max_msg_size);
	s(max_atomic_size);
	s(max_fetch_atomic_size);
	s(max_waw_ordered_size);
	s(max_war_ordered_size);
	s(max_volatile_size);
	s(features);
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
	ptl_ni_limits_t l;

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
		&res.net.nih                           /* THE handler */
	));
	/* retrieve the process identifier */
	lcr_ptl_chk(PtlGetPhysId(
		res.net.nih, /* The NI handler */
		&res.addr.id    /* the structure to store it */
	));

        lcr_ptl_iface_print_limits(&res.config.max_limits);

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

        srail->config.max_iovecs       = driver_config->max_iovecs;
        srail->config.num_eager_blocks = driver_config->num_eager_blocks;
        srail->config.eager_block_size = driver_config->eager_block_size;
        srail->config.max_copyin_buf   = driver_config->num_eager_blocks;
        srail->config.min_cached_mem   = 2;
}

/* Initialize ptl resources needed for Active Message interface. It requires
 * both two-sided and one-sided portals context. Also, the EQ is shared among
 * all context so that it will be instanciated except if the only feature
 * required is RMA. */
static int _lcr_ptl_iface_init_am(lcr_ptl_rail_info_t *srail)
{
        int rc;
        ptl_md_t md;

        /* MD spanning all addressable memory */
        md = (ptl_md_t) {
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->net.eqh,
                .start = 0,
                .length = PTL_SIZE_MAX,
                .options = PTL_MD_EVENT_SEND_DISABLE
        };

	lcr_ptl_chk(PtlMDBind(
		srail->net.nih,
		&md,
		&srail->net.am.mdh
	));

        /* Register portal table used for AM communication. */
        lcr_ptl_chk(PtlPTAlloc(srail->net.nih,
                                PTL_PT_FLOWCTRL,
                                srail->net.eqh,
                                PTL_PT_ANY,
                                &srail->net.am.pti
                               ));

        srail->net.am.block_mp = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->net.am.block_mp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate block "
                                       "memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* First, initialize memory pool of receive buffer. */
        mpc_mempool_param_t mp_block_params = {
                .elem_per_chunk = srail->config.num_eager_blocks,
                .elem_size      = sizeof(lcr_ptl_recv_block_t) +
                        srail->config.eager_block_size,
                .max_elems      = 4*srail->config.num_eager_blocks,
                .alignment      = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .malloc_func    = sctk_malloc,
                .free_func      = sctk_free,
        };

        rc = mpc_mpool_init(srail->net.am.block_mp, &mp_block_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Activate am eager block */
        mpc_list_init_head(&srail->net.am.bhead);
        rc = lcr_ptl_recv_block_enable(srail, srail->net.am.block_mp,
                                       &srail->net.am.bhead,
                                       srail->net.am.pti,
                                       PTL_PRIORITY_LIST);

        atomic_store(&srail->net.am.op_sn, 0);

err:
        return rc;
}

static int _lcr_ptl_iface_fini_am(lcr_ptl_rail_info_t *srail)
{
        lcr_ptl_recv_block_disable(&srail->net.am.bhead);

	lcr_ptl_chk(PtlMDRelease(srail->net.am.mdh));

        lcr_ptl_chk(PtlPTFree(srail->net.nih, srail->net.am.pti));

        //TODO: ongoing request management.
        return MPC_LOWCOMM_SUCCESS;
}

/* Initialize ptl resources needed for TAG interface. */
static int _lcr_ptl_iface_init_tag(lcr_ptl_rail_info_t *srail)
{
        int rc;
        ptl_md_t md;

        /* MD spanning all addressable memory */
        md = (ptl_md_t) {
                .ct_handle = PTL_CT_NONE,
                .eq_handle = srail->net.eqh,
                .start = 0,
                .length = PTL_SIZE_MAX,
                .options = PTL_MD_EVENT_SEND_DISABLE
        };

	lcr_ptl_chk(PtlMDBind(
		srail->net.nih,
		&md,
		&srail->net.tag.mdh
	));

        //FIXME: only allocate if offload is enabled
        /* register the TAG EAGER PTE */
        lcr_ptl_chk(PtlPTAlloc(srail->net.nih,
                                PTL_PT_FLOWCTRL,
                                srail->net.eqh,
                                PTL_PT_ANY,
                                &srail->net.tag.pti
                               ));

        srail->net.tag.block_mp = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->net.tag.block_mp != NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate block "
                                       "memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* First, initialize memory pool of receive buffer. */
        mpc_mempool_param_t mp_block_params = {
                .elem_per_chunk = srail->config.num_eager_blocks,
                .elem_size      = sizeof(lcr_ptl_recv_block_t) +
                        srail->config.eager_block_size,
                .max_elems      = 16,
                .alignment      = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .malloc_func    = sctk_malloc,
                .free_func      = sctk_free,
        };

        rc = mpc_mpool_init(srail->net.tag.block_mp, &mp_block_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Activate am eager block */
        mpc_list_init_head(&srail->net.tag.bhead);
        rc = lcr_ptl_recv_block_enable(srail, srail->net.tag.block_mp,
                                       &srail->net.tag.bhead,
                                       srail->net.tag.pti,
                                       PTL_OVERFLOW_LIST);

err:
        return rc;
}

static int _lcr_ptl_iface_fini_tag(lcr_ptl_rail_info_t *srail)
{
        lcr_ptl_recv_block_disable(&srail->net.tag.bhead);

	lcr_ptl_chk(PtlMDRelease(srail->net.tag.mdh));

        lcr_ptl_chk(PtlPTFree(srail->net.nih, srail->net.tag.pti));

        //TODO: ongoing request management.
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_mem_activate(lcr_ptl_rail_info_t *srail,
                         lcr_ptl_mem_lifetime_t lt,
                         const void *start,
                         ptl_size_t length,
                         lcr_ptl_mem_t *mem)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        ptl_md_t md;
        ptl_me_t me;

        lcr_ptl_chk(PtlCTAlloc(srail->net.nih, &mem->cth));

        //FIXME: For now, the MD attached to the memory is not used by any of
        //       the upper layer calls. Maybe this could be removed?
        md = (ptl_md_t) {
                .ct_handle = mem->cth,
                .eq_handle = srail->net.eqh,
                .length = length,
                .start  = (void *)start,
                .options = PTL_MD_EVENT_SUCCESS_DISABLE |
                        PTL_MD_EVENT_SEND_DISABLE       |
                        PTL_MD_EVENT_CT_ACK             |
                        PTL_MD_EVENT_CT_REPLY,
        };
        lcr_ptl_chk(PtlMDBind(srail->net.nih, &md, &mem->mdh));

        mem->match = atomic_fetch_add(&srail->net.rma.mem_uid, 1);

        me = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                .ignore_bits = 0,
                .match_bits  = mem->match,
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

        lcr_ptl_chk(PtlMEAppend(srail->net.nih,
                                srail->net.rma.pti,
                                &me,
                                PTL_PRIORITY_LIST,
                                srail,
                                &mem->meh
                               ));

        mem->start = (uint64_t)start;
        mem->lifetime = lt;
        mpc_queue_init_head(&mem->pending_flush);
        mpc_common_spinlock_init(&mem->lock, 0);
        mem->op_count = 0;

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
        atomic_store(&mem->outstandings, 0);

err:
        return rc;
}

int lcr_ptl_mem_deactivate(lcr_ptl_mem_t *mem)
{
        sctk_free(mem->txqt);

        lcr_ptl_chk(PtlCTFree(mem->cth));

        lcr_ptl_chk(PtlMDRelease(mem->mdh));

        lcr_ptl_chk(PtlMEUnlink(mem->meh));
        return MPC_LOWCOMM_SUCCESS;
}

/* Initialize ptl resources needed for RMA interface. */
static int _lcr_ptl_iface_init_rma(lcr_ptl_rail_info_t *srail)
{
        int rc = MPC_LOWCOMM_SUCCESS;

        /* Initialize one-sided required resources:
         * - one Memory Entries (ME) spanning all address space with
         *   communications disable;
         * - one Portals Table (PT) used for operations. */

        atomic_init(&srail->net.rma.mem_uid, 0);

        /* Register RMA PT */
        lcr_ptl_chk(PtlPTAlloc(srail->net.nih,
                                0 /* No options. */,
                                srail->net.eqh,
                                PTL_PT_ANY,
                                &srail->net.rma.pti
                               ));

        /* Initialize pool of memory handles. */
        srail->net.rma.mem_mp = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->net.rma.mem_mp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate static "
                                       "memory handles memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        mpc_mempool_param_t mp_mem_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 16,
                .elem_size = sizeof(lcr_ptl_mem_t),
                .max_elems = srail->config.max_limits.max_mds, //FIXME: should be minus number
                                                               //       of MD allocated for
                                                               //       other interface
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };

        rc = mpc_mpool_init(srail->net.rma.mem_mp, &mp_mem_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        srail->net.rma.dynamic_mem = mpc_mpool_pop(srail->net.rma.mem_mp);
        assert(srail->net.rma.dynamic_mem != NULL);

        rc = lcr_ptl_mem_activate(srail,
                                  LCR_PTL_MEM_DYNAMIC,
                                  0,
                                  PTL_SIZE_MAX,
                                  srail->net.rma.dynamic_mem);
        mpc_common_spinlock_init(&srail->net.rma.dynamic_mem->lock, 0);

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mpc_list_init_head(&srail->net.rma.poll_list);

        /* Already append the dynamic memory that will always be polled. */
        mpc_list_push_head(&srail->net.rma.poll_list,
                           &srail->net.rma.dynamic_mem->elem);

err:
        return rc;
}

static int _lcr_ptl_iface_fini_rma(lcr_ptl_rail_info_t *srail)
{
        lcr_ptl_mem_deactivate(srail->net.rma.dynamic_mem);

        lcr_ptl_chk(PtlPTFree(srail->net.nih, srail->net.rma.pti));

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
	mpc_lowcomm_monitor_register_on_demand_callback(__ptl_get_rail_callback_name(rail->pmi_tag, rail_name, 32),
                                                        __ptl_monitor_callback, rail);
}

int lcr_ptl_iface_is_reachable(sctk_rail_info_t *rail, uint64_t uid)
{
        //FIXME: check whether getting connection info should be done here. For
        //       now just return true.
        UNUSED(rail); UNUSED(uid);
        return 1;
}

int lcr_ptl_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{
        lcr_ptl_iface_config_t *config = &rail->network.ptl.config;

        attr->iface.cap.am.max_bcopy  = config->eager_limit;
        attr->iface.cap.am.max_zcopy  = 0;
        attr->iface.cap.am.max_iovecs = config->max_iovecs;

        attr->iface.cap.tag.max_bcopy  = 0;
        attr->iface.cap.tag.max_zcopy  = config->eager_limit;
        attr->iface.cap.tag.max_iovecs = config->max_iovecs;

        attr->iface.cap.rndv.max_send_zcopy = config->max_mr;
        attr->iface.cap.rndv.max_put_zcopy  = config->max_put;
        attr->iface.cap.rndv.max_get_zcopy  = config->max_get;
        attr->iface.cap.rndv.min_frag_size  = config->min_frag_size;

        attr->iface.cap.rma.max_put_bcopy   = 0; //FIXME: put_bcopy not
                                                 //       supported for now
        attr->iface.cap.rma.max_put_zcopy   = config->max_put;
        attr->iface.cap.rma.max_get_bcopy   = 0; //FIXME: get_bcopy not
                                                 //       supported for now
        attr->iface.cap.rma.max_get_zcopy   = config->max_put;
        attr->iface.cap.rma.min_frag_size   = config->min_frag_size;

        attr->iface.cap.ato.max_post_size   = config->max_limits.max_atomic_size;
        attr->iface.cap.ato.max_fetch_size  = config->max_limits.max_fetch_atomic_size;

        attr->iface.cap.flags               = rail->cap;

        attr->mem.cap.max_reg               = PTL_SIZE_MAX;
        attr->mem.size_packed_mkey          = sizeof(uint64_t) +
                sizeof(ptl_match_bits_t) + sizeof(unsigned); //FIXME: to be generalized

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_iface_init(sctk_rail_info_t *rail, unsigned fflags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;
	struct _mpc_lowcomm_config_struct_net_driver_portals *ptl_driver_config =
                &rail->runtime_config_driver_config->driver.value.portals;

        //FIXME: since ptl_process_t is used as to key a hash table, it needs to
        //       be 8 bytes
        assert(sizeof(ptl_process_t) == sizeof(uint64_t));

	/* Register on-demand callback for rail */
	__register_monitor_callback(rail);

        /* Hardware initialization is needed by all features (AM, TAG, RMA). */
        *srail = lcr_ptl_hardware_init(PTL_IFACE_DEFAULT + (unsigned int)rail->rail_number);

        _lcr_ptl_iface_init_driver_config(srail, ptl_driver_config);

        /* First initialize PT. */
        srail->net.am.pti   = LCR_PTL_PT_NULL;
        srail->net.tag.pti  = LCR_PTL_PT_NULL;
        srail->net.rma.pti  = LCR_PTL_PT_NULL;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        srail->net.tk.pti   = LCR_PTL_PT_NULL;
        lcr_ptl_tk_init(srail->net.nih, &srail->net.tk, ptl_driver_config);
#endif

        /* Create the common EQ */
        lcr_ptl_chk(PtlEQAlloc(srail->net.nih,
                                PTL_EQ_PTE_SIZE,
                                &srail->net.eqh
                               ));
        mpc_common_spinlock_init(&srail->lock, 0);

        if (fflags & LCR_PTL_FEATURE_AM) {
                rc = _lcr_ptl_iface_init_am(srail);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                srail->config.features |= LCR_PTL_FEATURE_AM;
        }

        if (fflags & LCR_PTL_FEATURE_RMA) {
                rc = _lcr_ptl_iface_init_rma(srail);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                srail->config.features |= LCR_PTL_FEATURE_RMA;
        }

        if (fflags & LCR_PTL_FEATURE_TAG) {
                rc = _lcr_ptl_iface_init_tag(srail);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                srail->config.features |= LCR_PTL_FEATURE_TAG;
        }

        /* Build the Portals address. */
        srail->addr = (lcr_ptl_addr_t) {
                .id       = srail->addr.id,
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
                .pte.tk   = srail->net.tk.pti,
#else
                .pte.tk   = LCR_PTL_PT_NULL,
#endif
                .pte.am   = srail->net.am.pti,
                .pte.tag  = srail->net.tag.pti,
                .pte.rma  = srail->net.rma.pti,
        };

        mpc_common_debug("LCR PTL: interface address nid=%llu, pid=%llu, "
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
                         "tk pti=%d, "
#endif
                         "am pti=%d, tag pti=%d, rma pti=%d", srail->addr.id.phys.nid,
                         srail->addr.id.phys.pid,
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
                         srail->addr.pte.tk,
#endif
                         srail->addr.pte.am, srail->addr.pte.tag,
                         srail->addr.pte.rma);

        /* Initialize table of endpoints. */
        atomic_store(&srail->num_eps, 0);
        srail->ept = sctk_malloc(LCR_PTL_MAX_EPS * sizeof(lcr_ptl_ep_info_t *));
        if (srail->ept == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocated table of "
                                       "endpoints.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* Initialize pool of recv operations. */
        srail->iface_ops = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->iface_ops == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate receive "
                                       "operation memory pool.");
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

        rc = mpc_mpool_init(srail->iface_ops, &mp_op_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Initialize pool of copy-in buffers. */
        srail->buf_mp = sctk_malloc(sizeof(mpc_mempool_t));
        if (srail->buf_mp == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate copy-in "
                                       "buffer memory pool.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        //FIXME: define suitable size for memory pool.
        mpc_mempool_param_t mp_buf_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 128,
                .elem_size = srail->config.eager_limit,
                .max_elems = 1024,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };

        rc = mpc_mpool_init(srail->buf_mp, &mp_buf_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
err:
        return rc;
}

void lcr_ptl_iface_fini(sctk_rail_info_t* rail)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcr_ptl_rail_info_t *srail = &rail->network.ptl;

	lcr_ptl_chk(PtlEQFree(srail->net.eqh));

        if (srail->config.features & LCR_PTL_FEATURE_AM) {
                rc = _lcr_ptl_iface_fini_am(srail);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCR PTL: error finalizing "
                                               "AM interface.");
                }
        }

        if (srail->config.features & LCR_PTL_FEATURE_TAG) {
                rc = _lcr_ptl_iface_fini_tag(srail);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCR PTL: error finalizing "
                                               "TAG interface.");
                }
        }

        if (srail->config.features & LCR_PTL_FEATURE_RMA) {
                rc = _lcr_ptl_iface_fini_rma(srail);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCR PTL: error finalizing "
                                               "RMA interface.");
                }
        }

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        lcr_ptl_tk_fini(srail->net.nih, &srail->net.tk);
#endif

        /* Finalize interface operations pool. */
        mpc_mpool_fini(srail->iface_ops);

        /* Finalize copy-in buffer pool. */
        mpc_mpool_fini(srail->buf_mp);

        /* Finalize TX Queues. */
        //FIXME: need to check that all memory has been removed from the list.
        for (i = 0; i < srail->num_eps; i++) {
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
                assert(srail->ept[i]->num_ops == 0);
#endif
                assert(mpc_queue_is_empty(&srail->ept[i]->am_txq.ops));
                mpc_mpool_fini(srail->ept[i]->ops_pool);
        }
        sctk_free(srail->ept);

	/* tear down the interface */
	lcr_ptl_chk(PtlNIFini(srail->net.nih));

	/* tear down the driver */
	PtlFini();

}

#endif

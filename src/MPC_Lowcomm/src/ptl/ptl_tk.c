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

#include <mpc_lowcomm_types.h>
#include <mpc_common_rank.h>
#include <mpc_common_helper.h>

int lcr_ptl_tk_progress_pending_ops(lcr_ptl_rail_info_t *srail,
                                    lcr_ptl_txq_t *txq, int *count)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int tmp = 0, token_num = 1;
        mpc_queue_iter_t iter;
        lcr_ptl_op_t *op;

        mpc_common_spinlock_lock(&txq->lock);
        mpc_queue_for_each_safe(op, iter, lcr_ptl_op_t, &txq->queue, elem) {
                if ((token_num = txq->tokens) <= 0) {
                        break;
                }
                txq->tokens--; txq->num_ops--;
                rc = lcr_ptl_do_op(op);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_spinlock_unlock(&txq->lock);
                        return rc;
                }

                mpc_queue_del_iter(&txq->queue, iter);
                tmp++;
        }
        mpc_common_spinlock_unlock(&txq->lock);

        if (lcr_ptl_txq_needs_tokens(txq, token_num)) {
                lcr_ptl_op_t *tk_op;
                rc = lcr_ptl_create_token_request(srail, txq, &tk_op);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
                assert(txq->is_waiting == 1);

                rc = lcr_ptl_do_op(tk_op);
        }

        *count = tmp;
err:
        return rc;
}

static inline int _lcr_ptl_create_token_resource(lcr_ptl_tk_module_t *tk, 
                                                  ptl_process_t remote,
                                                  int remote_tx_id, 
                                                  lcr_ptl_tk_rsc_t **rsc_p)
{
        uint64_t uuid = 0; 

        lcr_ptl_tk_rsc_t *rsc = sctk_malloc(sizeof(lcr_ptl_tk_rsc_t));
        if (rsc == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate token "
                                       "resource.");
                return MPC_LOWCOMM_ERROR;
        }
        LCR_PTL_PROCESS_TO_UINT64(uuid, remote);
        mpc_common_hashtable_set(&tk->rsct, uuid, rsc);

        rsc->remote.id = remote;
        rsc->tx_idx    = remote_tx_id;
        rsc->tk_chunk  = tk->config.min_chunk;

        *rsc_p = rsc;

        return MPC_LOWCOMM_SUCCESS;
}

static inline int _lcr_ptl_get_token_resource(lcr_ptl_tk_module_t *tk,
                                              ptl_process_t remote,
                                              lcr_ptl_tk_rsc_t **rsc_p)
{
        uint64_t uuid = 0; 
        lcr_ptl_tk_rsc_t *rsc;

        LCR_PTL_PROCESS_TO_UINT64(uuid, remote);
        rsc = mpc_common_hashtable_get(&tk->rsct, uuid);
        if (rsc == NULL) {
                mpc_common_debug_error("LCR PTL: Could not find token "
                                       "resource. remote nid=%llu, pid=%llu.",
                                       remote.phys.nid, remote.phys.pid);
                return MPC_LOWCOMM_ERROR;
        }

        *rsc_p = rsc;

        return MPC_LOWCOMM_SUCCESS;
}


static void _lcr_ptl_tk_update_rx_chunk(lcr_ptl_tk_module_t *tk,
                                        lcr_ptl_tk_rsc_t *rsc, 
                                        uint32_t num_pendings)
{
        if (num_pendings > 0 && rsc->tk_chunk < tk->config.max_chunk) {
                rsc->tk_chunk++;
        } else if (rsc->tk_chunk > tk->config.min_chunk) {
                rsc->tk_chunk--;
        }
}

int lcr_ptl_tk_release_rsc_token(lcr_ptl_tk_module_t *tk,
                                 ptl_process_t remote,
                                 int32_t num_pendings)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t uuid = 0;
        lcr_ptl_tk_rsc_t *rsc = NULL;

        LCR_PTL_PROCESS_TO_UINT64(uuid, remote);
        rsc = mpc_common_hashtable_get(&tk->rsct, uuid);
        if (rsc == NULL) {
                mpc_common_debug_error("LCR PTL: token resource not found "
                                       "from remote nid=%llu, pid=%llu.",
                                       remote.phys.nid, remote.phys.pid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mpc_common_spinlock_lock(&tk->lock);
        /* Recompute RX chunk of tokens to be granted based on the remote TX
         * queue contention. */
        _lcr_ptl_tk_update_rx_chunk(tk, rsc, num_pendings);


        /* Put the token back to the pool. */
        tk->pool++;
        mpc_common_spinlock_unlock(&tk->lock);

        mpc_common_debug("LCR PTL: token release. nid=%llu, pid=%llu, "
                         "pool tokens=%d.", remote.phys.nid, remote.phys.pid, 
                         tk->pool);

err:
        return rc;
}


/* Token distribution is has follow:
 * - loop over all exhausted RX Queue in a round robin fashion.
 * - if any RX Queue any the exhausted list means that it does not have any
 *   token.
 * - while there are available tokens in the pool, distribute them to the
 *   exhausted RX Queue one by one. 
 * - an RX Queue that has a large number of remote pending operations should be
 *   given a larger number of tokens since its token chunk should have been
 *   increased appropriately. 
 * - while the RX Queue have not reached its token chunk, it is push back to the
 *   list of exhausted RX Queue so it can receive more. */
int lcr_ptl_tk_distribute_tokens(lcr_ptl_rail_info_t *srail) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int distributed;
        lcr_ptl_op_t     *op;
        lcr_ptl_tk_rsc_t *rsc;
        mpc_queue_iter_t  iter;

        mpc_common_spinlock_lock(&srail->tk.lock);

        /* Fast path. */
        if (srail->tk.pool <= 0 || mpc_queue_is_empty(&srail->tk.exhausted)) {
                goto unlock;
        }

        /* First, distribute between exhausted token resources in a round robin
         * fashion. */
        while (srail->tk.pool > 0) {
                distributed = srail->tk.pool;
                mpc_queue_for_each_safe(rsc, iter, lcr_ptl_tk_rsc_t, 
                                        &srail->tk.exhausted, elem) {

                        if ( (rsc->req.requested == rsc->req.granted) ||
                             (rsc->req.granted >= rsc->tk_chunk) ) {
                                continue;
                        }

                        rsc->req.granted++; srail->tk.pool--;
                }
                if (distributed == srail->tk.pool) {
                        /* No token distributed on this round, exit. */
                        break;
                }
        }

        /* Then, send granted tokens. */
        mpc_queue_for_each_safe(rsc, iter, lcr_ptl_tk_rsc_t, 
                                &srail->tk.exhausted, elem) {
                if (rsc->req.granted > 0) {
                        /* Create token operation. */
                        rc = lcr_ptl_create_token_grant(srail, rsc, &op);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto unlock;
                        }
                        rc = lcr_ptl_do_op(op);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto unlock;
                        }

                        /* Then remote from exhausted queue. */
                        mpc_queue_del_iter(&srail->tk.exhausted, iter);
                }
        }

unlock:
        mpc_common_spinlock_unlock(&srail->tk.lock);

        return rc;
}

static int lcr_ptl_tk_process_event(lcr_ptl_rail_info_t *srail, 
                                    ptl_event_t *ev)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint16_t           token_num;
        lcr_ptl_txq_t     *txq;
        lcr_ptl_op_type_t  op_type;
        lcr_ptl_tk_rsc_t  *rsc;

        switch (ev->type) {
        case PTL_EVENT_PUT:
                op_type = LCR_PTL_CTRL_HDR_GET_OP_ID(ev->hdr_data);
                switch (op_type) {
                case LCR_PTL_OP_TK_INIT:
                        /* Create an RX Queue. */
                        rc = _lcr_ptl_create_token_resource(&srail->tk, ev->initiator, 
                                                            LCR_PTL_CTRL_HDR_GET_TX_ID(ev->hdr_data),
                                                            &rsc);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }

                        break;
                case LCR_PTL_OP_TK_REQUEST:
                        txq = &srail->txqt[LCR_PTL_CTRL_HDR_GET_TX_ID(ev->hdr_data)];

                        rc = _lcr_ptl_get_token_resource(&srail->tk, ev->initiator, &rsc);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }

                        mpc_common_spinlock_lock(&srail->tk.lock);
                        rsc->req.requested = LCR_PTL_CTRL_HDR_GET_TOKEN_NUM(ev->hdr_data);
                        rsc->req.granted   = 0;
                        rsc->req.scheduled = 0;

                        mpc_common_debug("LCR PTL: Received TOKEN REQUEST. nid=%llu, pid=%llu, "
                                         "num token requested=%d", ev->initiator.phys.nid, 
                                         ev->initiator.phys.pid, rsc->req.requested);

                        /* Push to exhausted list to be granted tokens during distribution. */
                        mpc_queue_push(&srail->tk.exhausted, &rsc->elem);
                        mpc_common_spinlock_unlock(&srail->tk.lock);
                        break;
                case LCR_PTL_OP_TK_GRANT:
                        /* Get token info from header. */
                        token_num = LCR_PTL_CTRL_HDR_GET_TOKEN_NUM(ev->hdr_data);
                        txq = &srail->txqt[LCR_PTL_CTRL_HDR_GET_TX_ID(ev->hdr_data)];

                        mpc_common_debug("LCR PTL: Received TOKEN GRANT. nid=%llu, pid=%llu, "
                                         "num token granted=%d, ", ev->initiator.phys.nid, 
                                         ev->initiator.phys.pid, token_num);

                        /* Add tokens to sender's TX Queue. */
                        mpc_common_spinlock_lock(&txq->lock);
                        assert(token_num > 0 && txq->tokens == 0);
                        txq->tokens += token_num;
                        atomic_store(&txq->is_waiting, 0);
                        mpc_common_spinlock_unlock(&txq->lock);
                        break;
                case LCR_PTL_OP_TK_RELEASE:
                        /* Return tokens to the pool. */
                        token_num = LCR_PTL_CTRL_HDR_GET_TOKEN_NUM(ev->hdr_data);
                        srail->tk.pool += token_num;
                        break;
                default:
                        mpc_common_debug_error("LCR PTL: unknown control operation "
                                               "type: %d", op_type);
                        rc = MPC_LOWCOMM_ERROR; 
                        break;
                }
                break;
        case PTL_EVENT_ACK:
                /* Return operation to the pool. */
                lcr_ptl_complete_op((lcr_ptl_op_t *)ev->user_ptr);
                break;
        case PTL_EVENT_PUT_OVERFLOW:
        case PTL_EVENT_GET_OVERFLOW:
        case PTL_EVENT_AUTO_UNLINK:
        case PTL_EVENT_AUTO_FREE:
        case PTL_EVENT_ATOMIC:
        case PTL_EVENT_FETCH_ATOMIC:          
        case PTL_EVENT_SEARCH:
        case PTL_EVENT_SEND:
        case PTL_EVENT_REPLY:
        case PTL_EVENT_GET:
        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: 
        case PTL_EVENT_ATOMIC_OVERFLOW:       
        case PTL_EVENT_PT_DISABLED:           
        case PTL_EVENT_LINK:                  
                not_reachable();
                break;
        default:
                mpc_common_debug_error("LCR PTL: unkown event type.");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

err:
        return rc;
}

int lcr_ptl_iface_progress_tk(lcr_ptl_rail_info_t *srail) 
{
	int ret, rc = MPC_LOWCOMM_SUCCESS;
	ptl_event_t ev;

        while (1) {

                ret = PtlEQGet(srail->tk.eqh, &ev);
                lcr_ptl_chk(ret);

                if (ret == PTL_OK) {
                        mpc_common_debug_info("PORTALS: EQS EVENT '%s' eqh=%llu, idx=%d, "
                                              "sz=%llu, user=%p, start=%p, "
                                              "remote_offset=%p, iface=%d", 
                                              lcr_ptl_event_decode(ev),
                                              *(uint64_t *)&srail->tk.eqh, ev.pt_index,
                                              ev.mlength, ev.user_ptr, ev.start,
                                              ev.remote_offset, *(uint64_t *)&srail->nih);

                        rc = lcr_ptl_tk_process_event(srail, &ev);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto poll_unlock;
                        }
                        
                        continue;
                } else if (ret == PTL_EQ_EMPTY) {
                        goto poll_unlock;
                        break;
                } else {
                        mpc_common_debug_error("LCR PTL: error returned from PtlEQPoll");
                        break;
                }
poll_unlock:
                break;
        }

        return rc;
}

static void _lcr_ptl_tk_init_config(lcr_ptl_tk_config_t *config,
                                    struct _mpc_lowcomm_config_struct_net_driver_portals *ptl_driver_config)
{

        /* Initialize token module configuration. */
        config->max_chunk  = mpc_common_min(ptl_driver_config->control_flow.max_chunk, 
                                            ptl_driver_config->control_flow.max_tokens);
        config->min_chunk  = mpc_common_max(ptl_driver_config->control_flow.min_chunk, 1);

        config->max_tokens = ptl_driver_config->control_flow.max_tokens;

}

int lcr_ptl_tk_init(ptl_handle_ni_t nih, lcr_ptl_tk_module_t *tk, 
                    struct _mpc_lowcomm_config_struct_net_driver_portals *ptl_driver_config)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ptl_md_t md;
        ptl_me_t me;

        /* Initialize configuration. */
        _lcr_ptl_tk_init_config(&tk->config, ptl_driver_config);

        /* Initialize structures. */
        mpc_queue_init_head(&tk->exhausted); 

        mpc_common_spinlock_init(&tk->lock, 0);

        tk->pool = tk->config.max_tokens;

        assert(tk->pool > 0);

        /* Create the common EQ */
        lcr_ptl_chk(PtlEQAlloc(nih,
                                PTL_EQ_PTE_SIZE, 
                                &tk->eqh
                               ));
        /* MD spanning all addressable memory */
        md = (ptl_md_t) {
                .ct_handle = PTL_CT_NONE,
                .eq_handle = tk->eqh,
                .start = 0,
                .length = 0,
                .options = PTL_MD_EVENT_SEND_DISABLE
        };

	lcr_ptl_chk(PtlMDBind(
		nih,
		&md,
		&tk->mdh 
	)); 

        /* Register portal table used for AM communication. */
        lcr_ptl_chk(PtlPTAlloc(nih,
                                PTL_PT_FLOWCTRL,
                                tk->eqh,
                                PTL_PT_ANY,
                                &tk->pti
                               ));

        me = (ptl_me_t) {
                .ct_handle   = PTL_CT_NONE,
                .match_bits  = 0llu,
                .ignore_bits = ~0llu,
                .match_id    = {
                        .phys.nid = PTL_NID_ANY,
                        .phys.pid = PTL_PID_ANY,
                },
                .min_free    = 0,
                .options     = PTL_ME_OP_PUT         | 
                        PTL_ME_NO_TRUNCATE           |
                        PTL_ME_EVENT_LINK_DISABLE    |
                        PTL_ME_EVENT_UNLINK_DISABLE,
                .uid         = PTL_UID_ANY,
                .start       = 0,
                .length      = 0 
        };

        lcr_ptl_chk(PtlMEAppend(nih, 
                                tk->pti, 
                                &me, 
                                PTL_PRIORITY_LIST, 
                                NULL,
                                &tk->meh
                   ));

        /* Init pool of operations. */
        tk->ops = sctk_malloc(sizeof(mpc_mempool_t));
        if (tk->ops == NULL) {
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

        rc = mpc_mpool_init(tk->ops, &mp_op_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

	mpc_common_hashtable_init(&tk->rsct, 
                                  mpc_common_get_process_count());

        mpc_common_spinlock_init(&tk->lock, 0);

err:

        return rc;
}

int lcr_ptl_tk_fini(ptl_handle_ni_t nih, lcr_ptl_tk_module_t *tk)
{
        lcr_ptl_tk_rsc_t *rsc;

        MPC_HT_ITER(&tk->rsct, rsc) 
        {
               sctk_free(rsc); 
        }
        MPC_HT_ITER_END(&tk->rsct);

        mpc_common_hashtable_release(&tk->rsct);

        lcr_ptl_chk(PtlEQFree(tk->eqh));

	lcr_ptl_chk(PtlMDRelease(tk->mdh));

	lcr_ptl_chk(PtlMEUnlink(tk->meh));

        lcr_ptl_chk(PtlPTFree(nih, tk->pti));

        return MPC_LOWCOMM_SUCCESS;
}

#endif

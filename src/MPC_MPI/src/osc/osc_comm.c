/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* # Maintainers:                                                         # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpc_thread_accessor.h"
#include "osc_mpi.h"

#include "mpc_mpi_internal.h"
#include "mpc_common_debug.h"
#include "comm_lib.h"

static int _mpc_osc_request_send_complete(int status, void *request, size_t length)
{
        mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

        assert(req->completion_flag != MPC_LOWCOMM_MESSAGE_DONE);
        if (status != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_fatal("COMM: request failed.");
        }

        //FIXME: MPI standard does not require the size, tag and src fields to
        //       be filled but some MPC test in the CI requires send, so set
        //       them from header info.
        req->status_error     = status;
        req->recv_info.length = length;

        req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

        return MPC_LOWCOMM_SUCCESS;
}

static int start_atomic_lock(mpc_osc_module_t *module, lcp_ep_h ep,
                             lcp_task_h task, int target)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t lock_state = 0;
        uint64_t value = 1;
        size_t remote_lock_addr = module->rstate_win_info[target].addr + 
                OSC_STATE_ACC_LOCK_OFFSET;

        do {

                rc = mpc_osc_perform_atomic_op(module, ep, task, value, sizeof(uint64_t), &lock_state,
                                               remote_lock_addr, module->rstate_win_info[target].rkey,
                                               LCP_ATOMIC_OP_CSWAP);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                if (lock_state == 0) {
                        break;
                }

                rc = lcp_progress(module->mngr);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        } while (1);
        
err:
        return rc;
}

static int end_atomic_lock(mpc_osc_module_t *module, lcp_ep_h ep,
                           lcp_task_h task, int target)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t value = -1;
        size_t remote_lock_addr = module->rstate_win_info[target].addr + 
                OSC_STATE_ACC_LOCK_OFFSET;
        
        rc = mpc_osc_perform_atomic_op(module, ep, task, value, sizeof(uint64_t), NULL, 
                                       remote_lock_addr, module->rstate_win_info[target].rkey, 
                                       LCP_ATOMIC_OP_ADD);

        return rc;
}

static int get_dynamic_win_info(lcp_ep_h ep, lcp_task_h task, uint64_t remote_addr, 
                                mpc_osc_module_t *module,
                                int target)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_status_ptr_t status;
        void *rdyn_win_info; 
        lcp_request_param_t params;
        uint64_t dynamic_win_count;
        mpc_osc_dynamic_win_t *tmp_dyn_win_infos;
        size_t rdyn_win_info_len = sizeof(uint64_t) + OSC_DYNAMIC_WIN_ATTACH_MAX * sizeof(mpc_osc_dynamic_win_t);
        lcp_mem_h rkey_state = module->rstate_win_info[target].rkey;
        uint64_t remote_state_dyn_addr = module->rstate_win_info[target].addr + 
                OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET;
        int found_idx, insert_idx;

        assert(ep != NULL && task != NULL);

        rdyn_win_info  = sctk_malloc(rdyn_win_info_len);
        if (rdyn_win_info == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        params = (lcp_request_param_t) {
                .field_mask = 0,
                .ep = ep,
                .datatype = LCP_DATATYPE_CONTIGUOUS,
        };

        status = lcp_get_nb(ep, task, rdyn_win_info, rdyn_win_info_len, 
                        remote_state_dyn_addr, rkey_state, &params);
        if (LCP_PTR_IS_ERR(status)) {
                goto err;
        }
        
        rc = mpc_osc_perform_flush_op(module, task, ep, NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        memcpy(&dynamic_win_count, rdyn_win_info, sizeof(uint64_t));
        assert(dynamic_win_count < OSC_DYNAMIC_WIN_ATTACH_MAX);

        tmp_dyn_win_infos = (void *)((uintptr_t)rdyn_win_info + sizeof(uint64_t));
        found_idx = mpc_osc_find_attached_region_position(tmp_dyn_win_infos, 0, dynamic_win_count, 
                                                          remote_addr, 1, &insert_idx);

        assert(found_idx >= 0 && (uint64_t)found_idx < dynamic_win_count);

        rc = lcp_mem_unpack(module->mngr, 
                            &(module->rdata_win_info[found_idx].rkey), 
                            tmp_dyn_win_infos[found_idx].rkey_buf, 
                            tmp_dyn_win_infos[found_idx].rkey_len);

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        module->rdata_win_info[found_idx].rkey_init = 1;

        sctk_free(rdyn_win_info);

err:
        return rc;
}

static lcp_status_ptr_t mpc_osc_put_common(const void *origin_addr, int origin_count,
                                           _mpc_lowcomm_general_datatype_t *origin_dt,
                                           int target, ptrdiff_t target_disp, int target_count,
                                           _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win,
                                           MPI_internal_request_t *request) 
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        size_t origin_len, target_len;
        uint64_t remote_addr = module->rdata_win_info[target].addr
                + target_disp * OSC_GET_DISP(module, target); 
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        lcp_task_h task;
        lcp_status_ptr_t status = LCP_STATUS_PTR(rc);

        //TODO: add a macro.
        if (module->eps[target] == NULL) {
                uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                   target);

                rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                   target_uid, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                }
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if (win->flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                rc = get_dynamic_win_info(module->eps[target], task, target_disp, 
                                          module, target);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

        //TODO: add rkey checking
        if (!target_count) {
                return LCP_STATUS_PTR(MPI_SUCCESS);
        }

        is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_dt) ||
                mpc_mpi_cl_type_is_contiguous(origin_dt);
        is_target_dt_contiguous = mpc_lowcomm_datatype_is_common(target_dt) ||
                mpc_mpi_cl_type_is_contiguous(target_dt);

        if (!is_orig_dt_contiguous) {
                not_implemented();
        }

        if (!is_target_dt_contiguous) {
                not_implemented();
        }

        rc = _mpc_cl_type_size(origin_dt, &origin_len);
        if (rc != MPC_SUCCESS) {
                goto err;
        }

        rc = _mpc_cl_type_size(target_dt, &target_len);
        if (rc != MPC_SUCCESS) {
                goto err;
        }

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               origin_len *= origin_count;
               target_len *= target_count;

               assert(target_len == origin_len);

               lcp_request_param_t req_param = {
                       .ep  = module->eps[target],
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .send_cb  = _mpc_osc_request_send_complete,
                       .request  = _mpc_cl_get_lowcomm_request(request),
                       .field_mask = request == MPI_REQUEST_NULL ? 0 :
                               (LCP_REQUEST_USER_REQUEST | 
                                LCP_REQUEST_SEND_CALLBACK),
               };

               status = lcp_put_nb(module->eps[target], task, origin_addr,
                                   origin_len, remote_addr,
                                   module->rdata_win_info[target].rkey,
                                   &req_param);
               if (LCP_PTR_IS_ERR(status)) {
                       mpc_common_debug_error("OSC: put. rc=%d", status);
               }
        }

err:
        return status;

}

static lcp_status_ptr_t mpc_osc_get_common(void *origin_addr, int origin_count, 
                                           _mpc_lowcomm_general_datatype_t *origin_dt,
                                           int target, ptrdiff_t target_disp, int target_count,
                                           _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win,
                                           MPI_internal_request_t *request) 
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        size_t origin_len, target_len;
        uint64_t remote_addr = module->rdata_win_info[target].addr
                + target_disp * OSC_GET_DISP(module, target); 
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        lcp_task_h task;
        lcp_status_ptr_t status = LCP_STATUS_PTR(rc);

        //TODO: add a macro.
        if (module->eps[target] == NULL) {
                uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                   target);

                rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                   target_uid, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                }
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if (win->flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                rc = get_dynamic_win_info(module->eps[target], task, target_disp, 
                                          module, target);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

        //TODO: add rkey checking
        if (!target_count) {
                return LCP_STATUS_PTR(MPI_SUCCESS);
        }

        is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_dt) ||
                mpc_mpi_cl_type_is_contiguous(origin_dt);
        is_target_dt_contiguous = mpc_lowcomm_datatype_is_common(target_dt) ||
                mpc_mpi_cl_type_is_contiguous(target_dt);

        if (!is_orig_dt_contiguous) {
                not_implemented();
        }

        if (!is_target_dt_contiguous) {
                not_implemented();
        }

        rc = _mpc_cl_type_size(origin_dt, &origin_len);
        if (rc != MPC_SUCCESS) {
                goto err;
        }

        rc = _mpc_cl_type_size(target_dt, &target_len);
        if (rc != MPC_SUCCESS) {
                goto err;
        }

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               origin_len *= origin_count;
               target_len *= target_count;

               assert(target_len == origin_len);

               lcp_request_param_t req_param = {
                       .ep  = module->eps[target],
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .send_cb  = _mpc_osc_request_send_complete,
                       .request  = _mpc_cl_get_lowcomm_request(request),
                       .field_mask = request == MPI_REQUEST_NULL ? 0 :
                               (LCP_REQUEST_USER_REQUEST | 
                                LCP_REQUEST_SEND_CALLBACK),
               };

               status = lcp_get_nb(module->eps[target], task, origin_addr,
                                   origin_len, remote_addr, 
                                   module->rdata_win_info[target].rkey,
                                   &req_param);
               if (LCP_PTR_IS_ERR(status)) {
                       mpc_common_debug_error("OSC: get. rc=%d", status);
               }
        }

err:
        return status;
}

int mpc_osc_put(const void *origin_addr, int origin_count,
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win) 
{
        lcp_status_ptr_t status;
        status = mpc_osc_put_common(origin_addr, origin_count, origin_dt, target,
                                    target_disp, target_count, target_dt, win,
                                    MPI_REQUEST_NULL);
        if (LCP_PTR_IS_ERR(status)) {
                return MPC_LOWCOMM_ERROR;
        } else {
                return MPI_SUCCESS;
        }
}

int mpc_osc_rput(const void *origin_addr,
                 int origin_count,
                 _mpc_lowcomm_general_datatype_t *origin_dt,
                 int target,
                 ptrdiff_t target_disp,
                 int target_count,
                 _mpc_lowcomm_general_datatype_t *target_dt,
                 mpc_win_t *win,
                 MPI_Request *request)
{
        int rc = MPI_SUCCESS;
        lcp_status_ptr_t status;
        *request = MPI_REQUEST_NULL;

        MPI_internal_request_t *req = mpc_mpi_alloc_request();

        status = mpc_osc_put_common(origin_addr, origin_count, origin_dt, target,
                                    target_disp, target_count, target_dt, win,
                                    req);
        if (LCP_PTR_IS_ERR(status)) rc = MPC_LOWCOMM_ERROR;

        *request = req;

        return rc;
}

int mpc_osc_get(void *origin_addr, int origin_count, 
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win) 
{
        lcp_status_ptr_t status;
        status = mpc_osc_get_common(origin_addr, origin_count, origin_dt, target,
                                    target_disp, target_count, target_dt, win,
                                    MPI_REQUEST_NULL);
        if (LCP_PTR_IS_ERR(status)) {
                return MPC_LOWCOMM_ERROR;
        } else {
                return MPI_SUCCESS;
        }
}

int mpc_osc_rget(void *origin_addr, int origin_count, 
                 _mpc_lowcomm_general_datatype_t *origin_dt,
                 int target, ptrdiff_t target_disp, int target_count,
                 _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win,
                 MPI_Request *request)
{
        int rc = MPI_SUCCESS;
        lcp_status_ptr_t status;
        *request = MPI_REQUEST_NULL;

        MPI_internal_request_t *req = mpc_mpi_alloc_request();

        status = mpc_osc_get_common(origin_addr, origin_count, origin_dt, target,
                                    target_disp, target_count, target_dt, win,
                                    req);
        if (LCP_PTR_IS_ERR(status)) rc = MPC_LOWCOMM_ERROR;

        *request = req;

        return rc;
}

int mpc_osc_accumulate(const void *origin_addr, int origin_count, 
                       _mpc_lowcomm_general_datatype_t *origin_dt,
                       int target, ptrdiff_t target_disp, int target_count,
                       _mpc_lowcomm_general_datatype_t *target_dt,
                       MPI_Op op, mpc_win_t *win)
{
        int rc = MPI_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task;
        int is_orig_dt_contiguous = 0, is_target_dt_contiguous = 0;

        //TODO: check sync state

        if (op == MPI_NO_OP) {
                return rc;
        }

        is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_dt) ||
                mpc_mpi_cl_type_is_contiguous(origin_dt);
        is_target_dt_contiguous = mpc_lowcomm_datatype_is_common(target_dt) ||
                mpc_mpi_cl_type_is_contiguous(target_dt);

        if (!is_orig_dt_contiguous || !is_target_dt_contiguous) {
                not_implemented();
        }

        assert(origin_count == target_count);
        
        //TODO: add a macro.
        if (module->eps[target] == NULL) {
                uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                   target);

                rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                   target_uid, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                }
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = start_atomic_lock(module, module->eps[target], task, target);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        if (op == MPI_REPLACE) {
                rc = mpc_osc_put(origin_addr, origin_count,
                                 origin_dt, target, target_disp,
                                 target_count, target_dt, win);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        } else {
                //NOTE: this is a contiguous path.
                void *tmp_result_addr;
                mpc_lowcomm_datatype_t tmp_dt;
                int tmp_dt_count;
                size_t tmp_len, tmp_dt_len; 

                if (mpc_lowcomm_datatype_is_common_predefined(target_dt)) {
                        tmp_dt = target_dt;
                        tmp_dt_count = target_count;
                } else {
                        tmp_dt = _mpc_cl_type_get_inner(target_dt, &tmp_dt_count);
                        if (tmp_dt == MPC_DATATYPE_NULL) {
                                rc = MPC_LOWCOMM_ERROR;
                                goto err;
                        }
                        tmp_dt_count *= target_count;
                }
                rc = _mpc_cl_type_size(tmp_dt, &tmp_dt_len);
                if (rc != MPC_SUCCESS) {
                        goto err;
                }
                tmp_len = tmp_dt_count * tmp_dt_len;

                tmp_result_addr = sctk_malloc(tmp_len);
                if (tmp_result_addr == NULL) {
                        mpc_common_debug_error("MPI OSC: could not allocate "
                                               "accumulate temp buffer.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                rc = mpc_osc_get(tmp_result_addr, tmp_dt_count, tmp_dt,
                                 target, target_disp, target_count, target_dt,
                                 win);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                rc = mpc_osc_perform_flush_op(module, task, module->eps[target],
                                              NULL);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                /* Get mpc operation and perform the reduction. */
                sctk_op_t *mpc_op = sctk_convert_to_mpc_op(op);
                sctk_Op_f func = sctk_get_common_function(tmp_dt,
                                                          mpc_op->op);
                func((void *)origin_addr, tmp_result_addr,
                     tmp_dt_count, tmp_dt);

                rc = mpc_osc_put(tmp_result_addr, tmp_dt_count,
                                 tmp_dt, target, target_disp,
                                 target_count, target_dt, win);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

        rc = mpc_osc_perform_flush_op(module, task, module->eps[target], NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = end_atomic_lock(module, module->eps[target], task, target);

err:
        return rc;
}


int mpc_osc_raccumulate(const void *origin_addr, int origin_count, 
                        _mpc_lowcomm_general_datatype_t *origin_dt,
                        int target, ptrdiff_t target_disp, int target_count,
                        _mpc_lowcomm_general_datatype_t *target_dt, MPI_Op op,
                        mpc_win_t *win, MPI_Request *request)
{
        int rc = MPI_SUCCESS;
        MPI_internal_request_t *req;

        req = mpc_mpi_alloc_request();

        rc = mpc_osc_accumulate(origin_addr, origin_count, origin_dt, target,
                                target_disp, target_count, target_dt, op, win);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        *request = req;
err:
        return rc;
}

int mpc_osc_get_accumulate(const void *origin_addr, int origin_count, 
                           _mpc_lowcomm_general_datatype_t *origin_datatype,
                           void *result_addr, int result_count, 
                           _mpc_lowcomm_general_datatype_t *result_datatype,
                           int target, ptrdiff_t target_disp,
                           int target_count, _mpc_lowcomm_general_datatype_t *target_datatype,
                           MPI_Op op, mpc_win_t *win)
{
        int rc = MPI_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task;
        int is_orig_dt_contiguous = 0;
        int is_target_dt_contiguous = 0;
        int is_result_dt_contiguous = 0;

        is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_datatype) ||
                mpc_mpi_cl_type_is_contiguous(origin_datatype);
        is_target_dt_contiguous = mpc_lowcomm_datatype_is_common(target_datatype) ||
                mpc_mpi_cl_type_is_contiguous(target_datatype);
        is_result_dt_contiguous = mpc_lowcomm_datatype_is_common(result_datatype) ||
                mpc_mpi_cl_type_is_contiguous(result_datatype);

        if (!is_orig_dt_contiguous || !is_target_dt_contiguous ||
            !is_result_dt_contiguous) {
                not_implemented();
        }

        assert(origin_count == result_count && result_count == target_count);
        
        //TODO: add a macro.
        if (module->eps[target] == NULL) {
                uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                   target);

                rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                   target_uid, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                }
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = start_atomic_lock(module, module->eps[target], task, target);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = mpc_osc_get(result_addr, result_count, result_datatype, target,
                         target_disp, target_count, target_datatype, win);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Make sure the previous get operation is completed and data is arrived
         * at destination. */
        rc = mpc_osc_perform_flush_op(module, task, module->eps[target], NULL);

        if (op != MPI_NO_OP) {
                if (op == MPI_REPLACE) {
                        rc = mpc_osc_put(origin_addr, origin_count,
                                         origin_datatype, target, target_disp,
                                         target_count, target_datatype, win);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                } else {
                        //NOTE: this is a contiguous path.
                        void *tmp_result_addr;
                        mpc_lowcomm_datatype_t tmp_dt;
                        int tmp_dt_count;
                        size_t tmp_len, tmp_dt_len; 
                        
                        if (mpc_lowcomm_datatype_is_common_predefined(target_datatype)) {
                                tmp_dt = target_datatype;
                                tmp_dt_count = target_count;
                        } else {
                                tmp_dt = _mpc_cl_type_get_inner(target_datatype, &tmp_dt_count);
                                if (tmp_dt == MPC_DATATYPE_NULL) {
                                        rc = MPC_LOWCOMM_ERROR;
                                        goto err;
                                }
                                tmp_dt_count *= target_count;
                        }
                        rc = _mpc_cl_type_size(tmp_dt, &tmp_dt_len);
                        if (rc != MPC_SUCCESS) {
                                goto err;
                        }
                        tmp_len = tmp_dt_count * tmp_dt_len;

                        tmp_result_addr = sctk_malloc(tmp_len);
                        if (tmp_result_addr == NULL) {
                                mpc_common_debug_error("MPI OSC: could not allocate "
                                                       "accumulate temp buffer.");
                                rc = MPC_LOWCOMM_ERROR;
                                goto err;
                        }

                        memcpy(tmp_result_addr, result_addr, tmp_len);

                        /* Get mpc operation and perform the reduction. */
                        sctk_op_t *mpc_op = sctk_convert_to_mpc_op(op);
                        sctk_Op_f func = sctk_get_common_function(tmp_dt, mpc_op->op);
                        func(origin_addr, tmp_result_addr, tmp_dt_count, tmp_dt);

                        rc = mpc_osc_put(tmp_result_addr, tmp_dt_count,
                                         tmp_dt, target, target_disp,
                                         target_count, target_datatype, win);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }

                rc = mpc_osc_perform_flush_op(module, task, module->eps[target], NULL);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

        rc = end_atomic_lock(module, module->eps[target], task, target);

err:
        return rc;
}

int mpc_osc_rget_accumulate(const void *origin_addr, int origin_count, 
                                _mpc_lowcomm_general_datatype_t *origin_datatype,
                                void *result_addr, int result_count, 
                                _mpc_lowcomm_general_datatype_t *result_datatype,
                                int target, ptrdiff_t target_disp, int target_count,
                                _mpc_lowcomm_general_datatype_t *target_datatype,
                                MPI_Op op, mpc_win_t *win,
                                MPI_Request *request)
{
        int rc = MPI_SUCCESS;

        MPI_internal_request_t *mpi_req = *request = mpc_lowcomm_request_alloc();

        rc = mpc_osc_get_accumulate(origin_addr, origin_count, origin_datatype,
                                    result_addr, result_count, result_datatype,
                                    target, target_disp, target_count,
                                    target_datatype, op, win);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        _mpc_osc_request_send_complete(MPC_LOWCOMM_SUCCESS, 
                                       _mpc_cl_get_lowcomm_request(mpi_req), 
                                       origin_count);

err:
        return rc;
}

int mpc_osc_compare_and_swap(const void *origin_addr, const void *compare_addr,
                             void *result_addr, _mpc_lowcomm_general_datatype_t *dt,
                             int target, ptrdiff_t target_disp,
                             mpc_win_t *win)
{
        int rc = MPI_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;        
        uint64_t remote_addr = module->rdata_win_info[target].addr + 
                target_disp * OSC_GET_DISP(module, target); 
        size_t dt_size;
        lcp_task_h task;

        //TODO: add a macro.
        if (module->eps[target] == NULL) {
                uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                   target);

                rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                   target_uid, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                }
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        //TODO: check sync state

        rc = start_atomic_lock(module, module->eps[target], task, target);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        if (win->flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                rc = get_dynamic_win_info(module->eps[target], task, target_disp, 
                                          module, target);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

        rc = _mpc_cl_type_size(dt, &dt_size);
        if (rc != MPC_SUCCESS) {
                goto err;
        }
        assert(dt_size <= sizeof(uint64_t));

        memcpy(result_addr, compare_addr, dt_size);
        mpc_common_debug("OSC MPI: perform cswap. origin addr=%p, dt size=%d, "
                         "compare addr=%p, result_addr=%p, remote addr=%p, "
                         "target disp=%d", origin_addr, dt_size, compare_addr, 
                         remote_addr, remote_addr, target_disp);
        rc = mpc_osc_perform_atomic_op(module, module->eps[target], task, 
                                       *(uint64_t *)origin_addr, dt_size, 
                                       (uint64_t *)result_addr, remote_addr, 
                                       module->rdata_win_info[target].rkey, 
                                       LCP_ATOMIC_OP_CSWAP); 
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = end_atomic_lock(module, module->eps[target], task, target);
err:
        return rc;
}

int mpc_osc_fetch_and_op(const void *origin_addr, void *result_addr,
                         _mpc_lowcomm_general_datatype_t *dt, int target,
                         ptrdiff_t target_disp, MPI_Op op,
                         mpc_win_t *win)
{
        int rc = MPI_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;        

        if (op == MPI_NO_OP || op == MPI_REPLACE) {
                uint64_t remote_addr = module->rdata_win_info[target].addr 
                        + target_disp * OSC_GET_DISP(module, target);
                size_t dt_size;
                uint64_t value;
                lcp_task_h task;
                lcp_atomic_op_t op_code;

                //TODO: add a macro.
                if (module->eps[target] == NULL) {
                        uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                           target);

                        rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                           target_uid, 0);
                        if (rc != 0) {
                                mpc_common_debug_fatal("Could not create endpoint.");
                        }
                }

                task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

                //TODO: check sync state
                rc = start_atomic_lock(module, module->eps[target], task, target);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                if (win->flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                        rc = get_dynamic_win_info(module->eps[target], task, target_disp, 
                                                  module, target);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }

                rc = _mpc_cl_type_size(dt, &dt_size);
                if (rc != MPC_SUCCESS) {
                        goto err;
                }
                assert(dt_size <= sizeof(uint64_t));

                if (op == MPI_REPLACE) {
                        op_code = LCP_ATOMIC_OP_SWAP;
                        value   = *(uint64_t *)origin_addr;
                } else {
                        op_code = LCP_ATOMIC_OP_ADD;
                        value   = 0;
                }

                rc = mpc_osc_perform_atomic_op(module, module->eps[target],
                                               task, value, sizeof(uint64_t),
                                               (uint64_t *)result_addr, remote_addr,
                                               module->rdata_win_info[target].rkey,
                                               op_code);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                return end_atomic_lock(module, module->eps[target], task, target);
        } else {
                return mpc_osc_get_accumulate(origin_addr, 1, dt, result_addr, 1, dt,
                                              target, target_disp, 1, dt, op, win);
        }

err:
        return rc;

}

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
#include "comm_lib.h"

static lcp_status_ptr_t mpc_osc_discontig_common(lcp_ep_h ep, lcp_task_h task,
                                                 const void *origin_addr, int origin_count,
                                                 _mpc_lowcomm_general_datatype_t *origin_dt,
                                                 int is_orig_dt_contig,
                                                 uint64_t remote_addr, int target_count,
                                                 _mpc_lowcomm_general_datatype_t *target_dt, 
                                                 int is_target_dt_contig, int is_get,
                                                 lcp_mem_h rmem,
                                                 MPI_internal_request_t *request);

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

static inline int need_atomic_lock(mpc_osc_module_t *module, 
                                   int target)
{
        mpc_osc_lock_t *lock = mpc_common_hashtable_get(&module->outstanding_locks, 
                                                        target);

        return !(lock != NULL && (lock->type == LOCK_EXCLUSIVE &&
                                  !lock->is_no_check));
}

static inline int atomic_size_supported(uint64_t remote_addr, size_t size)
{
        //FIXME: remote_addr not used for now but some transport may require the
        //       address to be aligned in memory. Either 4 for 32 bits, or 8 for
        //       64 bits operations.
        UNUSED(remote_addr);

        return size <= sizeof(uint64_t);
}

static inline uint64_t load_uint64(const void *ptr, size_t size) 
{
        if (sizeof(uint8_t) == size) {
                return *(uint8_t *) ptr;
        } else if (sizeof(uint16_t) == size) { 
                return *(uint16_t *) ptr;
        } else if (sizeof(uint32_t) == size) {
                return *(uint32_t *) ptr;
        } else {  
                return *(uint64_t *) ptr;
        }
}

static int start_atomic_lock(mpc_osc_module_t *module, lcp_ep_h ep,
                             lcp_task_h task, int target, int *lock_acquired)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t lock_state = 0;
        uint64_t value = 1;
        size_t remote_lock_addr = module->rstate_win_info[target].addr + 
                OSC_STATE_ACC_LOCK_OFFSET;
        *lock_acquired = 0;

        if (need_atomic_lock(module, target)) {
                //FIXME: should this be added to a perform_idle type of function to
                //       avoid active waiting.
                do {

                        rc = mpc_osc_perform_atomic_op(module, ep, task, value,
                                                       sizeof(uint64_t),
                                                       &lock_state,
                                                       remote_lock_addr,
                                                       module->rstate_win_info[target].rkey,
                                                       LCP_ATOMIC_OP_CSWAP);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }

                        if (lock_state == 0) {
                                *lock_acquired = 1;
                                break;
                        }

                        /* Reset compare value. */
                        lock_state = 0;

                        rc = lcp_progress(module->mngr);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                        mpc_thread_yield();

                } while (1);
        }
        
err:
        return rc;
}

static int end_atomic_lock(mpc_osc_module_t *module, lcp_ep_h ep,
                           lcp_task_h task, int target, int lock_acquired)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t value = -1;
        size_t remote_lock_addr = module->rstate_win_info[target].addr + 
                OSC_STATE_ACC_LOCK_OFFSET;
        
        assert(ep);
        if (lock_acquired) {
                rc = mpc_osc_perform_atomic_op(module, ep, task, value,
                                               sizeof(uint64_t), NULL,
                                               remote_lock_addr,
                                               module->rstate_win_info[target].rkey,
                                               LCP_ATOMIC_OP_ADD);
        }

        return rc;
}

static int get_dynamic_win_info(lcp_ep_h ep, lcp_task_h task, 
                                uint64_t remote_addr, 
                                mpc_osc_module_t *module, 
                                int target, lcp_mem_h *rmem_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_status_ptr_t status;
        void *rdyn_win_info; 
        lcp_request_param_t params;
        uint64_t dynamic_win_count;
        mpc_osc_dynamic_win_t *tmp_dyn_win_infos;
        size_t rdyn_win_info_len = sizeof(uint64_t) + OSC_DYNAMIC_WIN_ATTACH_MAX
                * sizeof(mpc_osc_dynamic_win_t);
        lcp_mem_h rkey_state = module->rstate_win_info[target].rkey;
        lcp_mem_h rkey_data;
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
        found_idx = mpc_osc_find_attached_region_position(tmp_dyn_win_infos, 0,
                                                          dynamic_win_count-1,
                                                          remote_addr, 1,
                                                          &insert_idx);

        assert(found_idx >= 0 && (uint64_t)found_idx < dynamic_win_count);

        rc = lcp_mem_unpack(module->mngr, 
                            &rkey_data, 
                            tmp_dyn_win_infos[found_idx].rkey_buf, 
                            tmp_dyn_win_infos[found_idx].rkey_len);

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        sctk_free(rdyn_win_info);

        *rmem_p = rkey_data;

err:
        return rc;
}

static int mpc_osc_get_remote_memory(mpc_osc_module_t *module, lcp_ep_h ep, 
                                     lcp_task_h task, int win_flavor, 
                                     int target, uint64_t remote_addr, 
                                     lcp_mem_h *rmem_p) 
{
        int rc = MPI_SUCCESS;
        lcp_mem_h rmem;
        if (win_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                rc = get_dynamic_win_info(ep, task, remote_addr, module, target,
                                          &rmem);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        } else {
                rmem = module->rdata_win_info[target].rkey;
        }

        *rmem_p = rmem;
err:
        return rc;
}

static inline int mpc_osc_release_remote_memory(mpc_osc_module_t *module,
                                                int win_flavor, lcp_mem_h rmem) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        if (win_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                rc = lcp_mem_deprovision(module->mngr, rmem);
        }
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
        size_t origin_len;
        uint64_t remote_addr = module->rdata_win_info[target].addr
                + target_disp * OSC_GET_DISP(module, target); 
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        long orig_lb, target_lb;
        long orig_extent, target_extent;
        lcp_task_h task;
        lcp_mem_h rmem;
        lcp_ep_h ep;
        lcp_status_ptr_t status = LCP_STATUS_PTR(rc);

        if (!origin_count || !target_count) {
                return LCP_STATUS_PTR(MPI_SUCCESS);
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
        rc = mpc_osc_get_comm_info(module, target, win->comm, &ep);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                status = LCP_STATUS_PTR(rc);
                goto err;
        }

        rc = mpc_osc_get_remote_memory(module, ep, task,
                                       win->flavor, target, remote_addr, &rmem);
        if (rc != MPI_SUCCESS) {
                status = LCP_STATUS_PTR(rc);
                goto err;
        }

        is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_dt) ||
                mpc_mpi_cl_type_is_contiguous(origin_dt);
        is_target_dt_contiguous = mpc_lowcomm_datatype_is_common(target_dt) ||
                mpc_mpi_cl_type_is_contiguous(target_dt);

        _mpc_cl_type_get_true_extent(origin_dt, &orig_lb, &orig_extent);
        _mpc_cl_type_get_true_extent(target_dt, &target_lb, &target_extent);

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
                rc = _mpc_cl_type_size(origin_dt, &origin_len);
                if (rc != MPC_SUCCESS) {
                        status = LCP_STATUS_PTR(rc);
                        goto err;
                }

               origin_len *= origin_count;

               lcp_request_param_t req_param = {
                       .ep  = ep,
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .send_cb  = _mpc_osc_request_send_complete,
                       .request  = _mpc_cl_get_lowcomm_request(request),
                       .field_mask = request == MPI_REQUEST_NULL ? 0 :
                               (LCP_REQUEST_USER_REQUEST | 
                                LCP_REQUEST_SEND_CALLBACK),
               };

               status = lcp_put_nb(ep, task,
                                   (void*)((uint64_t)origin_addr+orig_lb),
                                   origin_len, remote_addr+target_lb, rmem,
                                   &req_param);
        } else {
                status = mpc_osc_discontig_common(ep, task, origin_addr,
                                                  origin_count, origin_dt,
                                                  is_orig_dt_contiguous,
                                                  remote_addr, target_count,
                                                  target_dt,
                                                  is_target_dt_contiguous, 0,
                                                  rmem, request);
        }

        rc = mpc_osc_release_remote_memory(module, win->flavor, rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                status = LCP_STATUS_PTR(rc);
        }
err:
        return status;

}

static lcp_status_ptr_t mpc_osc_get_common(void *origin_addr, int origin_count, 
                                           _mpc_lowcomm_general_datatype_t *origin_dt,
                                           int target, ptrdiff_t target_disp, 
                                           int target_count,
                                           _mpc_lowcomm_general_datatype_t *target_dt, 
                                           mpc_win_t *win,
                                           MPI_internal_request_t *request) 
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        size_t origin_len;
        uint64_t remote_addr = module->rdata_win_info[target].addr
                + target_disp * OSC_GET_DISP(module, target); 
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        long orig_lb, target_lb;
        long orig_extent, target_extent;
        lcp_task_h task;
        lcp_ep_h ep;
        lcp_mem_h rmem;
        lcp_status_ptr_t status = LCP_STATUS_PTR(rc);

        //TODO: add rkey and sync checking 
        if (!origin_count || !target_count) {
                return LCP_STATUS_PTR(MPI_SUCCESS);
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
        rc = mpc_osc_get_comm_info(module, target, win->comm, &ep);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                status = LCP_STATUS_PTR(rc);
                goto err;
        }

        rc = mpc_osc_get_remote_memory(module, ep, task, win->flavor, target,
                                       remote_addr, &rmem);
        if (rc != MPI_SUCCESS) {
                status = LCP_STATUS_PTR(rc);
                goto err;
        }

        is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_dt) ||
                mpc_mpi_cl_type_is_contiguous(origin_dt);
        is_target_dt_contiguous = mpc_lowcomm_datatype_is_common(target_dt) ||
                mpc_mpi_cl_type_is_contiguous(target_dt);

        _mpc_cl_type_get_true_extent(origin_dt, &orig_lb, &orig_extent);
        _mpc_cl_type_get_true_extent(target_dt, &target_lb, &target_extent);

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
                rc = _mpc_cl_type_size(origin_dt, &origin_len);
                if (rc != MPC_SUCCESS) {
                        status = LCP_STATUS_PTR(rc);
                        goto err;
                }

               origin_len *= origin_count;

               lcp_request_param_t req_param = {
                       .ep  = ep,
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .send_cb  = _mpc_osc_request_send_complete,
                       .request  = _mpc_cl_get_lowcomm_request(request),
                       .field_mask = request == MPI_REQUEST_NULL ? 0 :
                               (LCP_REQUEST_USER_REQUEST | 
                                LCP_REQUEST_SEND_CALLBACK),
               };

               status = lcp_get_nb(ep, task,
                                   (void*)((uint64_t)origin_addr+orig_lb),
                                   origin_len, remote_addr+target_lb, rmem,
                                   &req_param);
        } else {
                status = mpc_osc_discontig_common(ep, task, origin_addr,
                                                  origin_count, origin_dt,
                                                  is_orig_dt_contiguous,
                                                  remote_addr, target_count,
                                                  target_dt,
                                                  is_target_dt_contiguous, 1,
                                                  rmem, request);
        }

        rc = mpc_osc_release_remote_memory(module, win->flavor, rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                status = LCP_STATUS_PTR(rc);
        }
err:
        return status;
}


static int mpc_osc_create_dt_blocks(const void *typebuf, int count,
                              mpc_lowcomm_datatype_t dt,
                              int **blocks_p, 
                              MPI_Aint **disps_p,
                              int *intblockct_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        size_t tmp_size;
        int *blocks;
        int intblockct;
        MPI_Aint *disps;
        MPI_Aint blockct;

        if (!MPIT_Type_is_initialized(dt)) {
                MPIT_Type_init(dt);
        }

        rc = _mpc_cl_type_size(dt, &tmp_size);
        if (rc != MPI_SUCCESS) {
                goto err;
        }
        tmp_size *= count;

        rc = MPIT_Type_blockct(count, dt, 0, tmp_size, &blockct);
        if (rc != MPI_SUCCESS) {
                goto err;
        }

        disps = sctk_malloc(blockct * sizeof(MPI_Aint)); 
        if (disps == NULL) {
                mpc_common_debug_error("MPI OSC: could not allocate iovec "
                                       "displacements.");
                rc = MPI_ERR_INTERN;
                goto err;
        }
        
        blocks = sctk_malloc(blockct * sizeof(int));
        if (blocks == NULL) {
                mpc_common_debug_error("MPI OSC: could not allocate iovec "
                                       "blocks.");
                rc = MPI_ERR_INTERN;
                goto err;
        }

        intblockct = blockct;
        rc = MPIT_Type_flatten((void *)typebuf, count, dt, 0, tmp_size, disps,
                               blocks, &intblockct);
        if (rc != MPI_SUCCESS) {
                goto err;
        }
        assert(intblockct == blockct);

        *blocks_p = blocks;
        *disps_p  = disps;
        *intblockct_p = intblockct;
err:
        return rc;
}

static lcp_status_ptr_t mpc_osc_discontig_common(lcp_ep_h ep, lcp_task_h task,
                                                 const void *origin_addr, int origin_count,
                                                 _mpc_lowcomm_general_datatype_t *origin_dt,
                                                 int is_orig_dt_contig,
                                                 uint64_t remote_addr, int target_count,
                                                 _mpc_lowcomm_general_datatype_t *target_dt, 
                                                 int is_target_dt_contig, int is_get,
                                                 lcp_mem_h rmem,
                                                 MPI_internal_request_t *request)
{
        int rc          = MPI_SUCCESS;
        lcp_status_ptr_t status = LCP_STATUS_PTR(rc);
        int *orig_blocks, *target_blocks;
        MPI_Aint *orig_disps, *target_disps;
        int orig_intblockct, target_intblockct;
        int orig_blockidx = 0, target_blockidx = 0;

        if (!is_orig_dt_contig) {
                rc = mpc_osc_create_dt_blocks(origin_addr, origin_count,
                                              origin_dt, &orig_blocks,
                                              &orig_disps, &orig_intblockct);
                if (rc != MPI_SUCCESS) {
                        status = LCP_STATUS_PTR(rc);
                        goto err;
                }
        }

        if (!is_target_dt_contig) {
                rc = mpc_osc_create_dt_blocks((void *)remote_addr, target_count,
                                              target_dt, &target_blocks,
                                              &target_disps, &target_intblockct);
                if (rc != MPI_SUCCESS) {
                        status = LCP_STATUS_PTR(rc);
                        goto err;
                }
        }

        lcp_request_param_t req_param = {
                .ep  = ep,
                .datatype = LCP_DATATYPE_CONTIGUOUS,
                .send_cb  = _mpc_osc_request_send_complete,
                .request  = _mpc_cl_get_lowcomm_request(request),
                .field_mask = request == MPI_REQUEST_NULL ? 0 :
                        (LCP_REQUEST_USER_REQUEST | 
                         LCP_REQUEST_SEND_CALLBACK),
        };
        
        if (!is_orig_dt_contig && !is_target_dt_contig) {
                size_t curr_len;
                while (orig_blockidx < orig_intblockct) {
                        curr_len = mpc_common_min(orig_blocks[orig_blockidx],
                                                  target_blocks[target_blockidx]);

                        if (is_get) {
                               status = lcp_get_nb(ep, task,
                                                   (void*)orig_disps[orig_blockidx],
                                                   curr_len,
                                                   target_disps[target_blockidx],
                                                   rmem, &req_param);
                        } else {
                               status = lcp_put_nb(ep, task,
                                                   (void*)orig_disps[orig_blockidx],
                                                   curr_len,
                                                   target_disps[target_blockidx],
                                                   rmem, &req_param);
                        }
                        if (LCP_PTR_IS_ERR(status)) {
                                goto err;
                        }
                
                        orig_disps[orig_blockidx]  += curr_len;
                        orig_blocks[orig_blockidx] -= curr_len;
                        if (orig_blocks[orig_blockidx] == 0) {
                                orig_blockidx++;
                        }
                        target_disps[target_blockidx] += curr_len;
                        target_blocks[target_blockidx] -= curr_len;
                        if (target_blocks[target_blockidx] == 0) {
                                target_blockidx++;
                        }
                }
        } else if (!is_orig_dt_contig) {
                size_t curr_len = 0;
                while (orig_blockidx < orig_intblockct) {
                        lcp_request_param_t req_param = {
                                .ep  = ep,
                                .datatype = LCP_DATATYPE_CONTIGUOUS,
                                .send_cb  = _mpc_osc_request_send_complete,
                                .request  = _mpc_cl_get_lowcomm_request(request),
                                .field_mask = request == MPI_REQUEST_NULL ? 0 :
                                        (LCP_REQUEST_USER_REQUEST | 
                                         LCP_REQUEST_SEND_CALLBACK),
                        };
                        if (is_get) {
                               status = lcp_get_nb(ep, task,
                                                   (void*)orig_disps[orig_blockidx],
                                                   orig_blocks[orig_blockidx],
                                                   remote_addr + curr_len, rmem,
                                                   &req_param);
                        } else {
                               status = lcp_put_nb(ep, task,
                                                   (void*)orig_disps[orig_blockidx],
                                                   orig_blocks[orig_blockidx],
                                                   remote_addr + curr_len, rmem,
                                                   &req_param);
                        }
                        if (LCP_PTR_IS_ERR(status)) {
                                goto err;
                        }

                        curr_len += orig_blocks[orig_blockidx];
                        orig_blockidx++;
                }
        } else {
                size_t curr_len = 0;
                while (target_blockidx < target_intblockct) {
                        if (is_get) {
                               status = lcp_get_nb(ep, task, (char*)origin_addr
                                                   + curr_len,
                                                   target_blocks[target_blockidx],
                                                   target_disps[target_blockidx],
                                                   rmem, &req_param);
                        } else {
                               status = lcp_put_nb(ep, task, (char*)origin_addr
                                                   + curr_len,
                                                   target_blocks[target_blockidx],
                                                   target_disps[target_blockidx],
                                                   rmem, &req_param);
                        }
                        if (LCP_PTR_IS_ERR(status)) {
                                goto err;
                        }

                        curr_len += target_blocks[target_blockidx];
                        target_blockidx++;
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
        lcp_ep_h ep;
        int lock_acquired = 0;
        void *tmp_result_addr = NULL;

        //TODO: check sync state

        if (op == MPI_NO_OP) {
                return rc;
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
        rc = mpc_osc_get_comm_info(module, target, win->comm, &ep);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = start_atomic_lock(module, ep, task, target, &lock_acquired);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto unlock;
        }
        mpc_common_debug("MPI OSC: acc atomic lock. from=%d, to=%d",
                         mpc_common_get_task_rank(), target);

        if (op == MPI_REPLACE) {
                rc = mpc_osc_put(origin_addr, origin_count,
                                 origin_dt, target, target_disp,
                                 target_count, target_dt, win);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto unlock;
                }
        } else {
                mpc_lowcomm_datatype_t tmp_dt;
                int tmp_dt_count;
                size_t tmp_len, tmp_dt_size;
                long tmp_lb, tmp_extent;
                int is_orig_dt_contiguous = 
                        mpc_lowcomm_datatype_is_common(origin_dt) ||
                        mpc_mpi_cl_type_is_contiguous(origin_dt);

                if (mpc_lowcomm_datatype_is_common_predefined(target_dt)) {
                        tmp_dt = target_dt;
                        tmp_dt_count = target_count;
                } else {
                        rc = _mpc_cl_type_get_primitive_type_info(target_dt,
                                                                  &tmp_dt,
                                                                  &tmp_dt_count);
                        if (rc != MPI_SUCCESS) {
                                mpc_common_debug_error("MPI OSC: no single "
                                                       "primitive type in acc");
                                goto unlock;
                        }
                        tmp_dt_count *= target_count;
                }
                rc = _mpc_cl_type_get_true_extent(tmp_dt, &tmp_lb, &tmp_extent);
                if (rc != MPC_SUCCESS) {
                        goto unlock;
                }
                tmp_len = tmp_dt_count * tmp_extent;

                tmp_result_addr = sctk_malloc(tmp_len);
                if (tmp_result_addr == NULL) {
                        mpc_common_debug_error("MPI OSC: could not allocate "
                                               "accumulate temp buffer.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto unlock;
                }

                rc = mpc_osc_get(tmp_result_addr, tmp_dt_count, tmp_dt, target,
                                 target_disp, target_count, target_dt, win);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto unlock;
                }

                rc = mpc_osc_perform_flush_op(module, task, ep,
                                              NULL);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto unlock;
                }

                /* Get mpc operation and perform the reduction. */
                sctk_op_t *mpc_op = sctk_convert_to_mpc_op(op);
                sctk_Op_f func    = sctk_get_common_function(tmp_dt, mpc_op->op);

                if (is_orig_dt_contiguous) {
                        func((void *)origin_addr, tmp_result_addr, tmp_dt_count,
                             tmp_dt);
                } else {
                        int *blocks;
                        int intblockct;
                        MPI_Aint *disps;
                        int orig_blockidx = 0;

                        rc = mpc_osc_create_dt_blocks(origin_addr, origin_count,
                                                      origin_dt, &blocks,
                                                      &disps, &intblockct);
                        if (rc != MPI_SUCCESS) {
                                goto unlock;
                        }
                        
                        if (mpc_mpi_cl_type_is_contiguous(tmp_dt)) {
                                _mpc_cl_type_size(tmp_dt, &tmp_dt_size);

                                char *curr_tmp_addr = tmp_result_addr;
                                while (orig_blockidx < intblockct) {
                                        func((void *)disps[orig_blockidx],
                                             curr_tmp_addr,
                                             blocks[orig_blockidx]/tmp_dt_size,
                                             tmp_dt);

                                        curr_tmp_addr += blocks[orig_blockidx];
                                        orig_blockidx++;
                                }
                        } else {
                                mpc_common_debug_error("MPI OSC: non contiguous predefined "
                                                       "dataype not implemented.");
                                not_implemented();
                        }
                }

                rc = mpc_osc_put(tmp_result_addr, tmp_dt_count,
                                 tmp_dt, target, target_disp,
                                 target_count, target_dt, win);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto unlock;
                }
        }

        rc = mpc_osc_perform_flush_op(module, task, ep, NULL);

unlock:
        if (tmp_result_addr) sctk_free(tmp_result_addr);
        rc = end_atomic_lock(module, ep, task, target, lock_acquired);

        mpc_common_debug("MPI OSC: acc atomic unlock. from=%d, to=%d",
                         mpc_common_get_task_rank(), target);
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
                           _mpc_lowcomm_general_datatype_t *origin_dt,
                           void *result_addr, int result_count, 
                           _mpc_lowcomm_general_datatype_t *result_dt,
                           int target, ptrdiff_t target_disp,
                           int target_count, _mpc_lowcomm_general_datatype_t *target_dt,
                           MPI_Op op, mpc_win_t *win)
{
        int rc = MPI_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task;
        lcp_ep_h ep;
        int lock_acquired = 0;
        void *tmp_result_addr = NULL;

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
        rc = mpc_osc_get_comm_info(module, target, win->comm, &ep);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = start_atomic_lock(module, ep, task, target, &lock_acquired);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = mpc_osc_get(result_addr, result_count, result_dt, target,
                         target_disp, target_count, target_dt, win);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto unlock;
        }

        /* Make sure the previous get operation is completed and data is arrived
         * at destination. */
        rc = mpc_osc_perform_flush_op(module, task, ep, NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto unlock;
        }

        if (op != MPI_NO_OP) {
                if (op == MPI_REPLACE) {
                        rc = mpc_osc_put(origin_addr, origin_count,
                                         origin_dt, target, target_disp,
                                         target_count, target_dt, win);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto unlock;
                        }
                } else {
                        mpc_lowcomm_datatype_t tmp_dt;
                        int tmp_dt_count;
                        size_t tmp_len, tmp_dt_size;
                        long tmp_lb, tmp_extent;
                        int is_orig_dt_contiguous = 
                                mpc_lowcomm_datatype_is_common(origin_dt) ||
                                mpc_mpi_cl_type_is_contiguous(origin_dt);

                        if (mpc_lowcomm_datatype_is_common_predefined(target_dt)) {
                                tmp_dt = target_dt;
                                tmp_dt_count = target_count;
                        } else {
                                rc = _mpc_cl_type_get_primitive_type_info(target_dt,
                                                                          &tmp_dt,
                                                                          &tmp_dt_count);
                                if (rc != MPI_SUCCESS) {
                                        mpc_common_debug_error("MPI OSC: no single "
                                                               "primitive type in acc");
                                        goto unlock;
                                }
                                tmp_dt_count *= target_count;
                        }
                        rc = _mpc_cl_type_get_true_extent(tmp_dt, &tmp_lb, &tmp_extent);
                        if (rc != MPC_SUCCESS) {
                                goto unlock;
                        }
                        tmp_len = tmp_dt_count * tmp_extent;

                        tmp_result_addr = sctk_malloc(tmp_len);
                        if (tmp_result_addr == NULL) {
                                mpc_common_debug_error("MPI OSC: could not allocate "
                                                       "accumulate temp buffer.");
                                rc = MPC_LOWCOMM_ERROR;
                                goto unlock;
                        }

                        memcpy(tmp_result_addr, result_addr, tmp_len);

                        /* Get mpc operation and perform the reduction. */
                        sctk_op_t *mpc_op = sctk_convert_to_mpc_op(op);
                        sctk_Op_f func = sctk_get_common_function(tmp_dt, mpc_op->op);
                        if (is_orig_dt_contiguous) {
                                func((void *)origin_addr, tmp_result_addr,
                                     tmp_dt_count, tmp_dt);
                        } else {
                                int *blocks;
                                int intblockct;
                                MPI_Aint *disps;
                                int orig_blockidx = 0;

                                rc = mpc_osc_create_dt_blocks(origin_addr, origin_count,
                                                              origin_dt, &blocks,
                                                              &disps, &intblockct);
                                if (rc != MPI_SUCCESS) {
                                        goto unlock;
                                }

                                if (mpc_mpi_cl_type_is_contiguous(tmp_dt)) {
                                        _mpc_cl_type_size(tmp_dt, &tmp_dt_size);

                                        char *curr_tmp_addr = tmp_result_addr;
                                        while (orig_blockidx < intblockct) {
                                                func((void *)disps[orig_blockidx], curr_tmp_addr,
                                                     blocks[orig_blockidx]/tmp_dt_size,
                                                     tmp_dt);

                                                curr_tmp_addr += blocks[orig_blockidx];
                                                orig_blockidx++;
                                        }
                                } else {
                                        mpc_common_debug_error("MPI OSC: non contiguous predefined "
                                                               "dataype not implemented.");
                                        not_implemented();
                                }
                        }

                        rc = mpc_osc_put(tmp_result_addr, tmp_dt_count,
                                         tmp_dt, target, target_disp,
                                         target_count, target_dt, win);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto unlock;
                        }
                }

                rc = mpc_osc_perform_flush_op(module, task, ep, NULL);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto unlock;
                }
        }

unlock:
        if (tmp_result_addr) sctk_free(tmp_result_addr);
        rc = end_atomic_lock(module, ep, task, target, lock_acquired);
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
        lcp_ep_h ep;
        lcp_mem_h rmem;
        int lock_acquired = 0;

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
        rc = mpc_osc_get_comm_info(module, target, win->comm, &ep);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        //TODO: check sync state

        rc = start_atomic_lock(module, ep, task, target, &lock_acquired);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = mpc_osc_get_remote_memory(module, ep, task, win->flavor, target,
                                       remote_addr, &rmem);
        if (rc != MPI_SUCCESS) {
                goto err;
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
        rc = mpc_osc_perform_atomic_op(module, ep, task,
                                       *(uint64_t*)origin_addr, dt_size,
                                       (uint64_t*)result_addr, remote_addr,
                                       rmem, LCP_ATOMIC_OP_CSWAP); 
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = end_atomic_lock(module, ep, task, target, lock_acquired);
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
        uint64_t remote_addr = module->rdata_win_info[target].addr 
                + target_disp * OSC_GET_DISP(module, target);
        size_t dt_size; 
        int lock_acquired = 0;

        _mpc_cl_type_size(dt, &dt_size);
        if (atomic_size_supported(remote_addr, dt_size) && 
            (op == MPI_NO_OP || op == MPI_REPLACE || op == MPI_SUM)) {
                uint64_t value;
                lcp_task_h task;
                lcp_ep_h ep;
                lcp_mem_h rmem;
                lcp_atomic_op_t op_code;

                task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
                rc = mpc_osc_get_comm_info(module, target, win->comm, &ep);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                //TODO: check sync state
                rc = start_atomic_lock(module, ep, task, target,
                                       &lock_acquired);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                rc = mpc_osc_get_remote_memory(module, ep, task, win->flavor,
                                               target, remote_addr, &rmem);
                if (rc != MPI_SUCCESS) {
                        goto err;
                }

                assert(dt_size <= sizeof(uint64_t));

                value = origin_addr ? load_uint64(origin_addr, dt_size) : 0;

                if (op == MPI_REPLACE) {
                        op_code = LCP_ATOMIC_OP_SWAP;
                } else {
                        op_code = LCP_ATOMIC_OP_ADD;
                        if (op == MPI_NO_OP) {
                                value   = 0;
                        }
                }

                rc = mpc_osc_perform_atomic_op(module, ep, task, value,
                                               dt_size, 
                                               (uint64_t*)result_addr,
                                               remote_addr, rmem, op_code);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                return end_atomic_lock(module, ep, task, target, lock_acquired);
        } else {
                return mpc_osc_get_accumulate(origin_addr, 1, dt, result_addr, 1, dt,
                                              target, target_disp, 1, dt, op, win);
        }

err:
        return rc;

}

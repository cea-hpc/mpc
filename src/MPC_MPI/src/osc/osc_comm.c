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

int mpc_osc_put(const void *origin_addr, int origin_count,
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win) 
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        size_t origin_len, target_len;
        uint64_t remote_addr;
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        lcp_task_h task;
        lcp_status_ptr_t status;

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

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               origin_len *= origin_count;
               target_len *= target_count;

               assert(target_len == origin_len);

               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

               lcp_request_param_t req_param = {
                       .ep  = module->eps[target],
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .field_mask = 0,
               };

                remote_addr = (uint64_t)((char *)module->base_rdata[target] + target_disp); 

               status = lcp_put_nb(module->eps[target], task, origin_addr,
                               origin_len, remote_addr,
                               module->rkeys_data[target],
                               &req_param);
               if (LCP_PTR_IS_ERR(status)) {
                       mpc_common_debug_error("OSC: put. rc=%d", status);
               }
        }

err:
        return rc;
}

int mpc_osc_rput(const void *origin_addr,
                 int origin_count,
                 _mpc_lowcomm_general_datatype_t *origin_dt,
                 int target,
                 ptrdiff_t target_disp,
                 int target_count,
                 _mpc_lowcomm_general_datatype_t *target_dt,
                 mpc_win_t *win,
                 MPI_Request *req)
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        uint64_t remote_addr;
        size_t origin_len, target_len;
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        mpc_lowcomm_request_t *request;
        lcp_task_h task;

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

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               origin_len *= origin_count;
               target_len *= target_count;

               assert(target_len == origin_len);

               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

               lcp_request_param_t req_param = {
                       .ep  = module->eps[target],
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .field_mask = 0,
               };
               
                remote_addr = (uint64_t)((char *)module->base_rdata[target] + target_disp); 

               request = (mpc_lowcomm_request_t *)lcp_put_nb(module->eps[target], task, origin_addr,
                                                             origin_len, 
                                                             remote_addr, 
                                                             module->rkeys_data[target],
                                                             &req_param);

               *req = (MPI_internal_request_t *)(request + 1);
               if (LCP_PTR_IS_ERR(request)) {
                       mpc_common_debug_error("OSC: rput. rc=%d", request);
               }
        } else {
                not_implemented();
        }

err:
        return rc;
}

int mpc_osc_get(void *origin_addr, int origin_count, 
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win) 
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        size_t origin_len, target_len;
        uint64_t remote_addr;
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        lcp_task_h task;
        lcp_status_ptr_t status;

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

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               origin_len *= origin_count;
               target_len *= target_count;

               assert(target_len == origin_len);

               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

                remote_addr = (uint64_t)((char *)module->base_rdata[target] + target_disp); 

               lcp_request_param_t req_param = {
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .field_mask = 0,
               };

               status = lcp_get_nb(module->eps[target], task, origin_addr,
                                   origin_len, remote_addr, 
                                   module->rkeys_data[target],
                                   &req_param);
               if (LCP_PTR_IS_ERR(status)) {
                       mpc_common_debug_error("OSC: get. rc=%d", status);
               }
        }

err:
        return rc;
}

int mpc_osc_rget(void *origin_addr, int origin_count, 
                 _mpc_lowcomm_general_datatype_t *origin_dt,
                 int target, ptrdiff_t target_disp, int target_count,
                 _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win,
                 MPI_Request *req)
{
        int rc = MPC_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        size_t origin_len, target_len;
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
        uint64_t remote_addr;
        mpc_lowcomm_request_t *request;
        lcp_task_h task;

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

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               origin_len *= origin_count;
               target_len *= target_count;

               assert(target_len == origin_len);

               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

               lcp_request_param_t req_param = {
                       .datatype = LCP_DATATYPE_CONTIGUOUS,
                       .field_mask = LCP_REQUEST_NO_COMPLETE,
               };

                remote_addr = (uint64_t)((char *)module->base_rdata[target] + target_disp); 

               request = (mpc_lowcomm_request_t *)lcp_get_nb(module->eps[target], task, 
                                                             origin_addr, origin_len, 
                                                             remote_addr, 
                                                             module->rkeys_data[target],
                                                             &req_param);

               *req = (MPI_internal_request_t *)(request + 1);
               if (LCP_PTR_IS_ERR(request)) {
                       mpc_common_debug_error("OSC: rget. rc=%d", request);
               }
        } else {
                not_implemented();
        }

err:
        return rc;

}

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

#include "mpc_common_debug.h"
#include "comm_lib.h"

int mpc_osc_put(const void *origin_addr, int origin_count,
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win) 
{

        int rc = MPC_SUCCESS;
        size_t origin_len, target_len;
        int is_orig_dt_contiguous   = 0;
        int is_target_dt_contiguous = 0;
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

        task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

        if (is_orig_dt_contiguous && is_target_dt_contiguous) {
               _mpc_cl_type_size(origin_dt, &origin_len);
               origin_len += origin_count;
               _mpc_cl_type_size(origin_dt, &target_len);
               target_len += target_count;

               assert(target_len == origin_len);

               lcp_request_param_t req_param = {
                       .mem = win->lkey_data,
                       .ep  = win->eps[target],
                       .flags = LCP_REQUEST_USER_MEMH,
               };

               rc = lcp_put_nb(win->eps[target], task, origin_addr,
                               origin_len, target_disp, win->rkeys_data[target],
                               &req_param);
        }

err:
        return rc;
}

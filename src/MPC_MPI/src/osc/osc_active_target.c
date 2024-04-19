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

#include "osc_mpi.h"

#include "mpc_thread_accessor.h"

int osc_flush_complete(int status, void *data) {
        mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)data;

        if (status != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("OSC MPI: error flushing.");
                return 1;
        }

        req->completion_flag = 1;
        return 0;
}

int mpc_osc_fence(int mpi_assert, mpc_win_t *win)
{
        UNUSED(mpi_assert);
        int rc = MPC_SUCCESS;
        lcp_request_param_t param;
        lcp_task_h task;
        mpc_lowcomm_request_t req;
        int tid = mpc_common_get_task_rank();

        /* Init lowcomm request. */
        req = (mpc_lowcomm_request_t) {.completion_flag = 0};

        /* Get task. */
        task = lcp_context_task_get(win->ctx, tid);

        param = (lcp_request_param_t) {
                .send_cb = osc_flush_complete,
                .user_request = &req,
                .flags = LCP_REQUEST_RMA_CALLBACK |
                        LCP_REQUEST_USER_REQUEST,
        };

        rc = lcp_flush_nb(win->mngr, task, &param);

        if (rc == MPC_LOWCOMM_SUCCESS) {
                req.completion_flag = 1;
                goto barrier;
        }

        while (req.completion_flag != 1) {
                rc = lcp_progress(win->mngr);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

barrier:
        PMPI_Barrier(win->comm);
err:
        return rc;
}

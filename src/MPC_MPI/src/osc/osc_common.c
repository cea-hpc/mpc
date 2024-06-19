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
#include "mpc_mpi_internal.h"
#include "osc_module.h"
#include <lcp.h>

int mpc_osc_perform_atomic_op(mpc_osc_module_t *mod, lcp_ep_h ep,
                              lcp_task_h task, uint64_t value, size_t size,
                              uint64_t *result, uint64_t remote_addr, 
                              lcp_mem_h rkey, lcp_atomic_op_t op)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        void *req;
        lcp_status_ptr_t status;

        req = lcp_request_alloc(task);
        if (req == NULL) {
                mpc_common_debug("OSC COMMON: could not allocate request.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcp_request_param_t params =  {
                .field_mask = (result != NULL ? LCP_REQUEST_REPLY_BUFFER : 0) |
                        LCP_REQUEST_USER_REQUEST,
                .request      = req,
                .reply_buffer = result,
        };

        status = lcp_atomic_op_nb(ep, task, &value, size, remote_addr, rkey,
                                  op, &params);
        if (LCP_PTR_IS_ERR(status)) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        while (lcp_request_check_status(req) != MPC_LOWCOMM_SUCCESS) {
                rc = lcp_progress(mod->mngr);
        }

        lcp_request_free(req);

err:
        return rc;
}

int mpc_osc_perform_flush_op(mpc_osc_module_t *mod, lcp_task_h task, 
                             lcp_ep_h ep, lcp_mem_h mem)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        void *req;
        lcp_status_ptr_t status;

        req = lcp_request_alloc(task);
        if (req == NULL) {
                mpc_common_debug("OSC COMMON: could not allocate request.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        lcp_request_param_t params = {
                .field_mask = ((ep != NULL ? LCP_REQUEST_USER_EPH : 0)    |
                               (mem != NULL ? LCP_REQUEST_USER_MEMH : 0)) |
                        LCP_REQUEST_USER_REQUEST,
                .request = req,
                .ep = ep,
                .mem = mem,
        };
        
        status = lcp_flush_nb(mod->mngr, task, &params);
        if (LCP_PTR_IS_ERR(status)) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        while (lcp_request_check_status(req) != MPC_LOWCOMM_SUCCESS) {
                rc = lcp_progress(mod->mngr);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }
        rc = MPC_LOWCOMM_SUCCESS;

        lcp_request_free(req);

err:
        return rc;
}

void mpc_osc_schedule_progress(lcp_manager_h mngr, volatile int *data,
                              int value)
{
        int trials = 0;
        do
        {
                lcp_progress(mngr);
                trials++;
        } while( (*data != value) && (trials < 16) );

        mpc_lowcomm_perform_idle(data, value, (void (*)(void *) ) lcp_progress,
                                 mngr);
}

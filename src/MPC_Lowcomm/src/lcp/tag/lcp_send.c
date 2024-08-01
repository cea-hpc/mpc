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

#include <lcp.h>
#include <core/lcp_ep.h>
#include <core/lcp_request.h>
#include <tag/lcp_tag.h>
#include <tag/lcp_tag_offload.h>

#include "mpc_common_debug.h"

#define MPC_MODULE "Lowcomm/LCP/Send"

/**
 * @brief Switch between protocols. Available protocols are :
 * - buffered copy
 * - zero copy
 * - rendez-vous
 *
 * @param ep endpoint to send the message
 * @param req request used to send the message
 * @param param request parameter used for offload flag
 * @return int MPI_SUCCESS in case of success
 */
int lcp_tag_send_start(lcp_ep_h ep, lcp_request_t *req,
                       const lcp_request_param_t *param)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        size_t size;

        /* First check offload */
        if (ep->config.offload) {
                size = req->send.length;
                req->state.offloaded = 1;

                if (param->field_mask & LCP_REQUEST_TAG_SYNC) {
                        not_implemented();
                }

                if (size <= ep->config.tag.max_bcopy ||
                    ((req->send.length <= ep->config.tag.max_zcopy) &&
                     (param->datatype & LCP_DATATYPE_DERIVED))) {
                        req->send.func = lcp_send_tag_offload_eager_bcopy;
                } else if ((size <= ep->config.tag.max_zcopy) &&
                           (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                        req->send.func = lcp_send_tag_offload_eager_zcopy;
                } else {
                        rc = lcp_send_rndv_offload_start(req);
                }
        } else { /* Not offload path. */
                //NOTE: multiplexing might not always be efficient (IO NUMA
                //      effects). A specific scheduling policy should be
                //      implemented to decide
                req->state.offloaded = 0;

                /* Get the total payload size */
                size = lcp_send_get_total_tag_payload(req->send.length);

                if (size <= ep->config.am.max_bcopy ||
                    ((req->send.length <= ep->config.tag.max_zcopy) &&
                     (param->datatype & LCP_DATATYPE_DERIVED))) {
                        req->send.func = lcp_send_eager_tag_bcopy;
                } else if ((size <= ep->config.am.max_zcopy) &&
                           (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                        req->send.func = lcp_send_eager_tag_zcopy;
                } else {
                        rc = lcp_send_rndv_tag_start(req);
                }
        }

        return rc;
}

//FIXME: It is not clear whether count is the number of elements or the length in
//       bytes. For now, the actual length in bytes is given taking into account
//       the datatypes and stuff...
//FIXME: Handle loopback, ie, sending to myself.
lcp_status_ptr_t lcp_tag_send_nb(lcp_ep_h ep, lcp_task_h task, void *buffer,
                                 size_t count, lcp_tag_info_t *tag_info,
                                 const lcp_request_param_t *param)
{
        int rc;
        lcp_status_ptr_t ret;
        lcp_request_t *req = NULL;

        // create the request to send
        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                req->flags       |= LCP_REQUEST_USER_CALLBACK;
                req->send.send_cb = param->send_cb;
        }

        // initialize request
        LCP_REQUEST_INIT_TAG_SEND(req, ep->mngr, task, param->request, tag_info,
                                  count, ep, (void *)buffer, 0, param->datatype,
                                  param->field_mask & LCP_REQUEST_TAG_SYNC ? 1 : 0);

        /* prepare request depending on its type */
        rc = lcp_tag_send_start(ep, req, param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not prepare send request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }

#ifdef MPC_ENABLE_TOPOLOGY_SIMULATION
        /* simulate latency/bandwidth if needed */
        mpc_topology_simulate_distance(
                mpc_lowcomm_peer_get_rank(mpc_lowcomm_monitor_get_uid()),
                req->send.tag.dest_tid,
                size);
#endif

        /* send the request */
        ret = lcp_request_send(req);

        return ret;
}

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

#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_tag.h"
#include "lcp_tag_offload.h"
#include "lcp_rndv.h"
#include "lcp_task.h"
#include "lcp_tag_match.h"

#include "mpc_common_debug.h"

int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param)
{
	int rc = LCP_SUCCESS;
	lcp_unexp_ctnr_t *match;
	lcp_request_t *req;
	lcp_context_h ctx = task->ctx;

	// create a request to be matched with the received message
        req = lcp_request_get(task);
        //lcp_request_create(&req);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_ERROR;
        }
	req->flags |= LCP_REQUEST_MPI_COMPLETE;
	LCP_REQUEST_INIT_TAG_RECV(req, ctx, task, request, param->tag_info,
										count, buffer, param->datatype);

	// get interface for the request to go through
	// if we have to try offload
	if (ctx->config.offload || param->flags & LCP_REQUEST_TRY_OFFLOAD) {
	        sctk_rail_info_t *iface = ctx->resources[ctx->priority_rail].iface;
		req->state.offloaded = 1;
		// try to receive using zero copy
		rc = lcp_recv_tag_zcopy(req, iface);

		return rc;
	}

        mpc_common_debug_info("LCP: post recv tag comm=%d, src_tid=%d, src_uid=%lu, "
                              "tag=%d, length=%d, buf=%p, req=%p, lcreq=%p",
                              req->recv.tag.comm, req->recv.tag.src_tid,
                              req->recv.tag.src_uid, req->recv.tag.tag,
                              count, buffer, req, request);

        req->state.offloaded = 0;

	LCP_TASK_LOCK(task);
        match = lcp_match_umqueue(task->umqs,
                                  req->recv.tag.comm,
                                  req->recv.tag.tag,
                                  req->recv.tag.tmask,
                                  req->recv.tag.src_tid,
                                  req->recv.tag.smask);
	if (match == NULL) {
                lcp_append_prqueue(task->prqs, &req->match,
                                   req->recv.tag.comm);

                LCP_TASK_UNLOCK(task);
		return LCP_SUCCESS;
	}

	LCP_TASK_UNLOCK(task);

        /* Get pointer to payload */
	if (match->flags & LCP_RECV_CONTAINER_UNEXP_RNDV_TAG) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);

                lcp_recv_rndv_tag_data(req, match + 1);
                rc = lcp_rndv_process_rts(req, match + 1,
                                          match->length - sizeof(lcp_rndv_hdr_t));

                if (rc != LCP_SUCCESS) {
                        mpc_common_debug_error("LCP: could not process rts.");
                }

                /* Data has been copied out, so container can be released */
                lcp_container_put(match);

	} else if (match->flags & (LCP_RECV_CONTAINER_UNEXP_EAGER_TAG |
                                   LCP_RECV_CONTAINER_UNEXP_TASK_TAG_BCOPY |
                                   LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC )) {
                intptr_t data_offset;
		mpc_common_debug_info("LCP: matched eager bcopy unexp req=%p, flags=%x", 
				      req, match->flags);

                if (match->flags & LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC) {
                        lcp_send_eager_sync_ack(req, match + 1);
                        data_offset = sizeof(lcp_tag_sync_hdr_t);
                } else {
                        data_offset = sizeof(lcp_tag_hdr_t);
                }

                //FIXME: here we use the fact that lcp_tag_hdr_t is first member
                //       of lcp_tag_sync_hdr_t. Hacky way of doing things...
                lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)(match + 1);

                /* Set variables for MPI status */
                req->recv.tag.src_tid  = hdr->src_tid;
                req->seqn              = hdr->seqn;
                req->recv.tag.tag      = hdr->tag;
                req->recv.send_length  = match->length - data_offset;
                        
                rc = lcp_recv_eager_tag_data(req, (char *)hdr + data_offset);

                if (rc != LCP_SUCCESS) {
                        mpc_common_debug_error("LCP: could not unpack unexpected "
                                               "bcopy eager data.");
                }

                /* Data has been copied out, so container can be released */
                lcp_container_put(match);

	} else if (match->flags & LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY) {

		mpc_common_debug_info("LCP: matched eager bcopy sync unexp req=%p, flags=%x", 
				      match, match->flags);

                lcp_task_completion_t *tcomp = (lcp_task_completion_t *)match;

                tcomp->mreq = req;
                tcomp->comp_cb(tcomp);
	} else {
		mpc_common_debug_error("LCP: unknown match flag=%x.", match->flags);
		rc = LCP_ERROR;
	}

	return rc;
}

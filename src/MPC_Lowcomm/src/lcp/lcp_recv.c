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

#include "mpc_common_debug.h"

int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param)
{
	int rc;
	lcp_unexp_ctnr_t *match;
	sctk_rail_info_t *iface;
	lcp_request_t *req;
	lcp_context_h ctx = task->ctx;

	// create a request to be matched with the received message
        req = lcp_request_get(task);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_ERROR;
        }
	req->flags |= LCP_REQUEST_MPI_COMPLETE;
	LCP_REQUEST_INIT_TAG_RECV(req, ctx, task, request, param->recv_info,
										count, buffer, param->datatype);

	// get interface for the request to go through
	iface = ctx->resources[ctx->priority_rail].iface;

	// if we have to try offload
	if (LCR_IFACE_IS_TM(iface) && (ctx->config.offload ||
		(param->flags & LCP_REQUEST_TRY_OFFLOAD))) {

		req->state.offloaded = 1;
		// try to receive using zero copy
		rc = lcp_recv_tag_zcopy(req, iface);

		return rc;
	}

        mpc_common_debug_info("LCP: post recv am comm=%d, src=%d, tag=%d, "
                              "length=%d, buf=%p, req=%p, lcreq=%p",
                              req->recv.tag.comm, req->recv.tag.src_tid, 
                              req->recv.tag.tag, count, buffer, req, request);

        req->state.offloaded = 0;

	LCP_TASK_LOCK(task);
	match = lcp_match_umq(&task->umq_table,
			      req->recv.tag.comm,
			      req->recv.tag.tag,
			      req->recv.tag.src_tid);
	if (match == NULL) {
		lcp_append_prq(&task->prq_table, req,
			       req->recv.tag.comm,
			       req->recv.tag.tag,
			       req->recv.tag.src_tid);

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
	} else if (match->flags & (LCP_RECV_CONTAINER_UNEXP_EAGER_TAG |
                                   LCP_RECV_CONTAINER_UNEXP_TASK_TAG_BCOPY)) {

		mpc_common_debug_info("LCP: matched eager bcopy unexp req=%p, flags=%x", 
				      match, match->flags);

                lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)(match + 1);

                /* Init tag info */
                struct lcp_tag_data tag_data = {
                        .length   = match->length - sizeof(lcp_tag_hdr_t),
                        .src_tid  = hdr->src_tid,
                        .dest_tid = hdr->dest_tid,
                        .comm     = hdr->comm,
                        .tag      = hdr->tag,
                        .seqn     = hdr->seqn
                };
                        
                rc = lcp_recv_eager_tag_data(req, &tag_data, hdr + 1, tag_data.length);

                if (rc != LCP_SUCCESS) {
                        mpc_common_debug_error("LCP: could not unpack unexpected "
                                               "bcopy eager data.");
                }
	} else if (match->flags & (LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC |
                                   LCP_RECV_CONTAINER_UNEXP_TASK_TAG_SYNC  |
                                   LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY)) {
		mpc_common_debug_info("LCP: matched eager zcopy unexp req=%p, flags=%x", 
				      match, match->flags);
                void *data;
                size_t length;
                lcp_request_t *sreq = NULL;
                lcp_tag_sync_hdr_t *hdr = (lcp_tag_sync_hdr_t *)(match + 1);

                /* Ack was requested, send it for inter-process communications. */
                if (match->flags & LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC) {
                        rc = lcp_send_eager_sync_ack(req, hdr);
                        if (rc != LCP_SUCCESS) {
                                mpc_common_debug_error("LCP: could not send ack.");
                        }
                }

                if (match->flags & (LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC |
                                    LCP_RECV_CONTAINER_UNEXP_TASK_TAG_SYNC)) {
                        /* Data pointer from eager protocol is after hdr. */
                        data = (void *)(hdr + 1);
                        length = match->length - sizeof(lcp_tag_sync_hdr_t);
                } else {
                        /* Data pointer from task zcopy protocol is in send buffer. */
                        sreq = (lcp_request_t *)hdr->msg_id;
                        data = sreq->send.buffer;
                        length = sreq->send.length;
                }

                /* Init tag info */
                struct lcp_tag_data tag_data = {
                        .length   = length,
                        .src_tid  = hdr->base.src_tid,
                        .dest_tid = hdr->base.dest_tid,
                        .tag      = hdr->base.tag,
                        .seqn     = hdr->base.seqn,
                        .comm     = hdr->base.comm 
                };
                        
                /* Complete the receive request */
                rc = lcp_recv_eager_tag_data(req, &tag_data, data, length);

                if (rc != LCP_SUCCESS) {
                        mpc_common_debug_error("LCP: could not unpack unexpected "
                                               "sync eager data.");
                }
                
                /* Complete the send request for TASK_TAG_ZCOPY, see
                 * lcp_send_self_tag_zcopy */
                if (match->flags & LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY) {
                        sreq->flags |= LCP_REQUEST_REMOTE_COMPLETED;
                        sreq->state.comp.comp_cb(&(sreq->state.comp));
                }
        } else {
		mpc_common_debug_error("LCP: unkown match flag=%x.", match->flags);
		rc = LCP_ERROR;
	}

        /* Data has been copied out, so container can be released */
        lcp_container_put(match);

	return rc;
}

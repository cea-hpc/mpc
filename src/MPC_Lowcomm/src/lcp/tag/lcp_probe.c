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

#include <core/lcp_context.h>
#include <core/lcp_manager.h>
#include <core/lcp_task.h>
#include <core/lcp_request.h>
#include <core/lcp_header.h>

#include <tag/lcp_tag_match.h>
#include <tag/lcp_tag_offload.h>

int lcp_tag_probe_nb(lcp_manager_h mngr, lcp_task_h task, const int src,
                     const int tag, const uint64_t comm,
                     lcp_tag_recv_info_t *recv_info)
{
        int rc = MPC_LOWCOMM_SUCCESS;
	lcp_unexp_ctnr_t *match = NULL;

        int tmask = tag == MPC_ANY_TAG ? 0 : ~0;
        int smask = src == MPC_ANY_SOURCE ? 0 : ~0;

	if (mngr->ctx->config.offload) {
	        sctk_rail_info_t *iface = mngr->ifaces[mngr->priority_iface];
                rc = lcp_recv_tag_probe(task, iface, src, tag, comm, recv_info);

                return rc;
        }

        LCP_TASK_LOCK(task);
        match = lcp_search_umqueue(task->tcct[mngr->id]->tag.umqs, (uint16_t)comm, tag, tmask, src, smask);
        if (match != NULL) {
                if (match->flags & LCP_RECV_CONTAINER_UNEXP_EAGER_TAG) {
                        lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)(match + 1);

                        recv_info->tag    = hdr->tag;
                        recv_info->length = match->length - sizeof(lcp_tag_hdr_t);
                        recv_info->src    = hdr->src_tid;

                } else if (match->flags & LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC ) {
                        lcp_tag_sync_hdr_t *hdr = (lcp_tag_sync_hdr_t *)(match + 1);

                        recv_info->tag    = hdr->base.tag;
                        recv_info->length = match->length - sizeof(lcp_tag_sync_hdr_t);
                        recv_info->src    = hdr->base.src_tid;

                } else if (match->flags & LCP_RECV_CONTAINER_UNEXP_RNDV_TAG) {
                        lcp_rndv_hdr_t *hdr = (lcp_rndv_hdr_t *)(match + 1);

                        recv_info->src    = hdr->tag.src_tid;
                        recv_info->tag    = hdr->tag.tag;
                        recv_info->length = hdr->size;
                }
                recv_info->found = 1;
                mpc_common_debug("LCP: probed request src=%d, tag=%d, length=%lu",
                                 recv_info->src, recv_info->tag, recv_info->length);

        }

        LCP_TASK_UNLOCK(task);

        return rc;
}

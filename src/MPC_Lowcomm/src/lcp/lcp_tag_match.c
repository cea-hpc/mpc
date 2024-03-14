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

#include <sctk_alloc.h>
#include <uthash.h>

#include "lcp_tag_match.h"
#include "queue.h"
#include "lcp_request.h"

void *lcp_search_umqueue(mpc_queue_head_t *umqs,
                         uint16_t comm_id, int32_t tag, int32_t tmask,
                         int32_t src, int32_t smask)
{
        int result;
	mpc_queue_head_t *queue;
        lcp_unexp_ctnr_t *ctnr;
        mpc_queue_iter_t iter;

	queue = &umqs[comm_id];
        mpc_queue_for_each_safe(ctnr, iter, lcp_unexp_ctnr_t, queue, elem) {
                lcp_tag_hdr_t *hdr = lcp_ctnr_get_tag(ctnr);

                result = ((hdr->tag == tag) || (!tmask && hdr->tag >= 0)) &&
                        ((hdr->src_tid & smask) == (src & smask));
                if (result) {
                        return ctnr;
                }
        }

	return NULL;
}

void lcp_append_umqueue(mpc_queue_head_t *umqs, mpc_queue_elem_t *elem, uint16_t comm_id)
{
        mpc_queue_push(&umqs[comm_id], elem);
}

void *lcp_match_umqueue(mpc_queue_head_t *umqs,
                        uint16_t comm_id, int32_t tag, int32_t tmask,
                        int32_t src, int32_t smask)
{
        int result;
	mpc_queue_head_t *queue;
        lcp_unexp_ctnr_t *ctnr;
        mpc_queue_iter_t iter;

	queue = &umqs[comm_id];
        mpc_queue_for_each_safe(ctnr, iter, lcp_unexp_ctnr_t, queue, elem) {
                lcp_tag_hdr_t *hdr = lcp_ctnr_get_tag(ctnr);

                result = ((hdr->tag == tag) || (!tmask && hdr->tag >= 0)) &&
                        ((hdr->src_tid & smask) == (src & smask));
                if (result) {
                        mpc_queue_del_iter(queue, iter);
                        return ctnr;
                }
        }

	return NULL;
}

void lcp_append_prqueue(mpc_queue_head_t *prqs, mpc_queue_elem_t *elem, uint16_t comm_id)
{
        mpc_queue_push(&prqs[comm_id], elem);
}

void *lcp_match_prqueue(mpc_queue_head_t *prqs, uint16_t comm_id, int32_t tag, int32_t src)
{
        int result;
        mpc_queue_head_t *queue;
        lcp_request_t *req;
        mpc_queue_iter_t iter;

        queue = &prqs[comm_id];
        mpc_queue_for_each_safe(req, iter, lcp_request_t, queue, match) {
		result = ((req->recv.tag.tag == tag) || (!req->recv.tag.tmask && tag >= 0)) &&
			((req->recv.tag.src_tid & req->recv.tag.smask) == (src & req->recv.tag.smask));
                if (result) {
                        mpc_queue_del_iter(queue, iter);
                        return req;
                }
        }

        return NULL;
}

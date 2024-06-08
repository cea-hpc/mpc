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

#include "lcp_request.h"
#include "lcp_prototypes.h"

ssize_t lcp_send_eager_bcopy(lcp_request_t *req,
                         lcr_pack_callback_t pack,
                         uint8_t am_id)
{
        ssize_t payload;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        payload = lcp_send_do_am_bcopy(ep->lct_eps[cc],
                                       am_id, pack, req);

	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
                return MPC_LOWCOMM_ERROR;
	}

        return payload;
}

int lcp_send_eager_zcopy(lcp_request_t *req, uint8_t am_id,
                         void *hdr, size_t hdr_size,
                         struct iovec *iov, size_t iovcnt,
                         lcr_completion_t *comp)
{
	lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        return lcp_send_do_am_zcopy(ep->lct_eps[cc],
                                    am_id, hdr, hdr_size,
                                    iov, iovcnt, comp);
}

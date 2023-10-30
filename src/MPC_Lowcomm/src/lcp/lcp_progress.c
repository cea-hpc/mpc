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

#include "lcp.h"

#include "mpc_common_spinlock.h"
#include "rail.h"

#include "lcp_context.h"
#include "lcp_request.h"

/**
 * @brief progress in the pending request list
 * 
 * @param ctx 
 * @return int 
 */
int lcp_progress(lcp_context_h ctx)
{
	int i, rc = LCP_SUCCESS;

        unsigned char progress_counter = ctx->current_progress_value;

        for (i=0; i<ctx->num_resources; i++) {
                sctk_rail_info_t *iface = ctx->resources[i].iface;
                if (iface->iface_progress == NULL)
                        continue;

                /* This implements progress in function of rail priority */
                unsigned int ressource_cnt = ctx->progress_counter[i];
                if(!ctx->resources[i].used)
                        ressource_cnt = 1;
                if( ressource_cnt < progress_counter  )
                        continue;

                iface->iface_progress(iface);
        }

        ctx->current_progress_value++;

        /* Loop to try sending requests within the pending queue only once. */
        size_t nb_pending = mpc_queue_length(&ctx->pending_queue); 
        while (nb_pending > 0) {
                LCP_CONTEXT_LOCK(ctx);
                /* One request is pulled from pending queue. */
                lcp_request_t *req = mpc_queue_pull_elem(&ctx->pending_queue,
                                                         lcp_request_t, queue);
                LCP_CONTEXT_UNLOCK(ctx);

                /* Send request which will be pushed back in pending queue if 
                 * it could not be sent */
                lcp_request_send(req);

                nb_pending--; 
        }

	return rc;
}

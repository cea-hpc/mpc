#include "lcp.h"

#include "lcp_context.h"
#include "lcp_tag_matching.h"
#include "lcp_prototypes.h"
#include "lcp_request.h"

int lcp_tag_probe_nb(lcp_context_h ctx, const uint64_t src, 
                     const int tag, const uint64_t comm,
                     lcp_tag_recv_info_t *recv_info)
{
        int rc = MPC_LOWCOMM_SUCCESS;
	lcp_unexp_ctnr_t *match = NULL;
	sctk_rail_info_t *iface;

        //FIXME: portals call should be thread safe, so context lock might not
        //       necessary.
        LCP_CONTEXT_LOCK(ctx);

	iface = ctx->resources[ctx->priority_rail].iface;
	if (LCR_IFACE_IS_TM(iface) && ctx->config.offload) {
                rc = lcp_recv_tag_probe(iface, src, tag, comm, recv_info);

                LCP_CONTEXT_UNLOCK(ctx);

                return rc;
        }

        match = lcp_search_umq(ctx->umq_table, (uint16_t)comm, tag, src);
        if (match != NULL) {
                if (match->flags & LCP_RECV_CONTAINER_UNEXP_TAG) {
                        mpc_common_debug("LCP: probe tag unexp flags=%x", 
                                         match->flags);
                        lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)(match + 1);
                        
                        recv_info->tag    = hdr->tag;
                        recv_info->length = match->length - sizeof(lcp_tag_hdr_t);
                        recv_info->src    = hdr->src;

                } else if (match->flags & (LCP_RECV_CONTAINER_UNEXP_RPUT | 
                                           LCP_RECV_CONTAINER_UNEXP_RGET)) {

                        lcp_rndv_hdr_t *hdr = (lcp_rndv_hdr_t *)(match + 1);

                        recv_info->src    = hdr->base.src;
                        recv_info->tag    = hdr->base.tag;
                        recv_info->length = hdr->size;
                }
                recv_info->found = 1;
        }

        LCP_CONTEXT_UNLOCK(ctx);

        return rc;
}

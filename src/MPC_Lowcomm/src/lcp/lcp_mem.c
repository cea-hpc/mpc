#include "lcp_mem.h"

#include "lcp_context.h"
#include <sctk_alloc.h>

int lcp_mem_register(lcp_context_h ctx, lcp_mem_h *mem_p, void *buffer, size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;
        sctk_rail_info_t *iface = NULL;

        mem = sctk_malloc(sizeof(struct lcp_mem));
        if (mem == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory domain");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mem->length     = length;
        mem->base_addr  = (uint64_t)buffer;

        mem->num_ifaces = ctx->num_resources;
        mem->mems = sctk_malloc(mem->num_ifaces * sizeof(lcr_memp_t));
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory pins");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        
        /* Pin the memory and create memory handles */
        for (i=0; i<mem->num_ifaces; i++) {
                iface = ctx->resources[i].iface;
                iface->rail_pin_region(iface, &mem->mems[i], buffer, length);
        }

        *mem_p = mem;
err:
        return rc;
}

int lcp_mem_deregister(lcp_context_h ctx, lcp_mem_h mem)
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        for (i=0; i<mem->num_ifaces; i++) {
                iface = ctx->resources[i].iface;
                iface->rail_unpin_region(iface, &mem->mems[i]);
        }

        sctk_free(mem->mems);
        sctk_free(mem); 

        return rc;
}

#include "lcp_mem.h"

#include "lcp_context.h"
#include <sctk_alloc.h>

size_t lcp_mem_pack(lcp_context_h ctx, void *dest, lcp_mem_h mem)
{
        int i;
        size_t packed_size = 0;
        sctk_rail_info_t *iface = NULL;
        void *p = dest;

        for (i=0; i<mem->num_ifaces; i++) {
                iface = ctx->resources[i].iface;
                packed_size += iface->iface_pack_memp(iface, 
                                                      &mem->mems[i], 
                                                      p + packed_size);
        }

        return packed_size;
}

size_t lcp_mem_unpack(lcp_context_h ctx, lcp_mem_h mem, void *src)
{
        int i;
        size_t unpacked_size = 0;
        sctk_rail_info_t *iface = NULL;
        void *p = src;

        for (i=0; i<mem->num_ifaces; i++) {
                iface = ctx->resources[i].iface;
                unpacked_size += iface->iface_unpack_memp(iface, 
                                                          &mem->mems[i], 
                                                          p + unpacked_size);
        }

        return unpacked_size;
}

int lcp_mem_create(lcp_context_h ctx, lcp_mem_h *mem_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_mem_h mem;

        mem = sctk_malloc(sizeof(struct lcp_mem));
        if (mem == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory domain");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mem->num_ifaces = ctx->num_resources;
        mem->mems = sctk_malloc(mem->num_ifaces * sizeof(lcr_memp_t));
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory pins");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        *mem_p = mem;
err:
        return rc;
}

int lcp_mem_register(lcp_context_h ctx, lcp_mem_h *mem_p, void *buffer, size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;
        sctk_rail_info_t *iface = NULL;

        rc = lcp_mem_create(ctx, &mem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mem->length     = length;
        mem->base_addr  = (uint64_t)buffer;
        
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

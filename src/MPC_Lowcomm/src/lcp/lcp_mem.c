#include "lcp_mem.h"

#include "lcp_context.h"
#include <sctk_alloc.h>

size_t lcp_mem_pack(lcp_context_h ctx, void *dest, lcp_mem_h mem, bmap_t memp_map)
{
        int i;
        size_t packed_size = 0;
        sctk_rail_info_t *iface = NULL;
        void *p = dest;

        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(memp_map, i)) {
                        iface = ctx->resources[i].iface;
                        packed_size += iface->iface_pack_memp(iface, 
                                                              &mem->mems[i], 
                                                              p + packed_size);
                }
        }

        return packed_size;
}

size_t lcp_mem_unpack(lcp_context_h ctx, 
                      lcp_mem_h mem, 
                      void *src, 
                      bmap_t memp_map)
{
        int i;
        size_t unpacked_size = 0;
        sctk_rail_info_t *iface = NULL;
        void *p = src;

        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(memp_map, i)) {
                        iface = ctx->resources[i].iface;
                        unpacked_size += iface->iface_unpack_memp(iface, 
                                                                  &mem->mems[i], 
                                                                  p + unpacked_size);
                        mem->num_ifaces++;
                }
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

        mem->mems       = sctk_malloc(ctx->num_resources * sizeof(lcr_memp_t));
        mem->num_ifaces = 0;
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory pins");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(mem->mems, 0, ctx->num_resources * sizeof(lcr_memp_t));

        *mem_p = mem;
err:
        return rc;
}

void lcp_mem_delete(lcp_mem_h mem)
{
        sctk_free(mem->mems);
        sctk_free(mem);
        mem = NULL;
}

//FIXME: what append if a memory could not get registered ? Miss error handling:
//       if a subset could not be registered, perform the communication on the 
//       successful memory pins ?
int lcp_mem_register(lcp_context_h ctx, 
                     lcp_mem_h *mem_p, 
                     void *buffer, 
                     size_t length,
                     bmap_t memp_map)
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
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(memp_map, i)) {
                    iface = ctx->resources[i].iface;
                    iface->rail_pin_region(iface, &mem->mems[i], buffer, length);
                    mem->num_ifaces++;
                }
        }

        *mem_p = mem;
err:
        return rc;
}

int lcp_mem_deregister(lcp_context_h ctx, lcp_mem_h mem, bmap_t memp_map)
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(memp_map, i)) {
                        iface = ctx->resources[i].iface;
                        iface->rail_unpin_region(iface, &mem->mems[i]);
                }
        }

        sctk_free(mem->mems);
        sctk_free(mem); 

        return rc;
}

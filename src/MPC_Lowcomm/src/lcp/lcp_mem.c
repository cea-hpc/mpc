#include "lcp_mem.h"

#include "lcp_context.h"
#include <sctk_alloc.h>

//FIXME: Needs to clarify the management of remote memory keys and interfaces:
//       - NICs are statically linked (one to one relationship)
//       - add the mem bitmap when sending the rkey to identify to which
//       interface the rkey belongs to.

static inline void build_memory_registration_bitmap(size_t length, 
                                                    size_t min_frag_size, 
                                                    int max_iface,
                                                    bmap_t *bmap_p)
{
        bmap_t bmap = MPC_BITMAP_INIT;
        int num_used_ifaces = 0;

        while ((length > num_used_ifaces * min_frag_size) &&
               (num_used_ifaces < max_iface)) {
                MPC_BITMAP_SET(bmap, num_used_ifaces);
                num_used_ifaces++;
        }

        *bmap_p = bmap;
}

size_t lcp_mem_rkey_pack(lcp_context_h ctx, lcp_mem_h mem, void *dest)
{
        int i;
        void *rkey_buf;
        size_t packed_size = 0;
        sctk_rail_info_t *iface = NULL;

        rkey_buf = dest;
        *(bmap_t *)rkey_buf = mem->bm;
        packed_size += sizeof(bmap_t);
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = ctx->resources[i].iface;
                        //FIXME: no error handling in case returned size is < 0
                        packed_size += iface->iface_pack_memp(iface, &mem->mems[i], 
                                                              rkey_buf + packed_size);
                }
        }

        return packed_size;
}

int lcp_mem_pack(lcp_context_h ctx, lcp_mem_h mem, 
                 void **rkey_buf_p, size_t *rkey_len)
{
        int i;
        size_t packed_size = 0;
        lcr_rail_attr_t attr;
        void *rkey_buf;
        sctk_rail_info_t *iface = NULL;

        packed_size += sizeof(bmap_t);
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = ctx->resources[i].iface;
                        iface->iface_get_attr(iface, &attr);

                        packed_size += attr.mem.size_packed_mkey;
                }
        }

        rkey_buf = sctk_malloc(packed_size);

        lcp_mem_rkey_pack(ctx, mem, rkey_buf);

        *rkey_buf_p = rkey_buf;
        *rkey_len   = packed_size;

        return MPC_LOWCOMM_SUCCESS;
}

void lcp_mem_release_rkey_buf(void *rkey_buffer)
{
        sctk_free(rkey_buffer);
}

int lcp_mem_unpack(lcp_context_h ctx, lcp_mem_h *mem_p, 
                   void *src, size_t size)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        size_t unpacked_size = 0;
        sctk_rail_info_t *iface = NULL;
        void *p = src;
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

        mem->bm = *(bmap_t *)p; unpacked_size += sizeof(bmap_t);
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = ctx->resources[i].iface;
                        unpacked_size += iface->iface_unpack_memp(iface, 
                                                                  &mem->mems[i], 
                                                                  p + unpacked_size);
                        mem->num_ifaces++;
                        if (unpacked_size == size) {
                                break;
                        }
                }
        }

        assert(size == unpacked_size);

        *mem_p = mem;
err: 
        return rc;
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
                     size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;
        lcr_rail_attr_t attr;
        sctk_rail_info_t *iface = ctx->resources[ctx->priority_rail].iface;

        rc = lcp_mem_create(ctx, &mem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mem->length     = length;
        mem->base_addr  = (uint64_t)buffer;

        iface->iface_get_attr(iface, &attr);
        build_memory_registration_bitmap(length,
                                         attr.iface.cap.rndv.min_frag_size,
                                         ctx->num_resources,
                                         &mem->bm);
        
        /* Pin the memory and create memory handles */
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                    iface = ctx->resources[i].iface;
                    iface->rail_pin_region(iface, &mem->mems[i], buffer, length);
                    mem->num_ifaces++;
                }
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

        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = ctx->resources[i].iface;
                        iface->rail_unpin_region(iface, &mem->mems[i]);
                }
        }

        lcp_mem_delete(mem);

        return rc;
}

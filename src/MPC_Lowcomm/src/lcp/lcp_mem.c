#include "lcp_mem.h"

#include "lcp_context.h"
#include <sctk_alloc.h>
#include <sctk_rail.h>

//FIXME: Needs to clarify the management of remote memory keys and interfaces:
//       - NICs are statically linked (one to one relationship)
//       - add the mem bitmap when sending the rkey to identify to which
//       interface the rkey belongs to.
//FIXME: this function is also needed for rget 
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

/**
 * @brief Pack data as rkey buffer.
 * 
 * @param ctx lcp context
 * @param mem data to pack
 * @param dest packed rkey buffer
 * @return size_t length of output buffer
 */
//FIXME: A memory range described by lcp_mem_h is unique (same address, same
//       length. As a consequence, transport memory key will be identical for
//       homogenous tranports, so no need to pack it for all rail in case of
//       multirail.
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

/**
 * @brief Pack data as rkey and return buffer size.
 * 
 * @param ctx lcp context
 * @param mem memory to pack
 * @param rkey_buf_p output rkey buffer
 * @param rkey_len output rkey buffer length
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
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

/**
 * @brief Release an rkey buffer.
 * 
 * @param rkey_buffer buffer to release
 */
void lcp_mem_release_rkey_buf(void *rkey_buffer)
{
        sctk_free(rkey_buffer);
}

/**
 * @brief Unpack an rkey buffer
 * 
 * @param ctx lcp context
 * @param mem_p unpacked memory output buffer
 * @param src data to unpack
 * @param size output size of unpacked data
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
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

/**
 * @brief Allocate memory domain and pins.
 * 
 * @param ctx context
 * @param mem_p memory to alloc
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
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

/**
 * @brief Free memory.
 * 
 * @param mem memory to be freed
 */
void lcp_mem_delete(lcp_mem_h mem)
{
        sctk_free(mem->mems);
        sctk_free(mem);
        mem = NULL;
}

int lcp_mem_reg_from_map(lcp_context_h ctx,
                         lcp_mem_h mem,
                         bmap_t mem_map,
                         void *buffer,
                         size_t length)
{
        int i;
        sctk_rail_info_t *iface = ctx->resources[ctx->priority_rail].iface;

        /* Pin the memory and create memory handles */
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem_map, i)) {
                    iface = ctx->resources[i].iface;
                    iface->rail_pin_region(iface, &mem->mems[i], buffer, length);
                    mem->num_ifaces++;
                }
        }

        //FIXME: no error to be returned because rail_pin_region is void.
        return MPC_LOWCOMM_SUCCESS;
}

//FIXME: what append if a memory could not get registered ? Miss error handling:
//       if a subset could not be registered, perform the communication on the 
//       successful memory pins ?
/**
 * @brief Register memory.
 * 
 * @param ctx context
 * @param mem_p memory object to register (out)
 * @param buffer data to store
 * @param length length of the data
 * @param memp_map 
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_mem_register(lcp_context_h ctx, 
                     lcp_mem_h *mem_p, 
                     void *buffer, 
                     size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
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
        rc = lcp_mem_reg_from_map(ctx, mem, mem->bm, buffer, length); 
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        *mem_p = mem;
err:
        return rc;
}

/**
 * @brief Post a buffer
 * 
 * @param ctx lcp context
 * @param mem_p memory pointer
 * @param buffer buffer to post
 * @param length length of buffer
 * @param tag tag of post
 * @param flags flag of post
 * @param tag_ctx tag context
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_mem_post_from_map(lcp_context_h ctx, 
                          lcp_mem_h mem, 
                          bmap_t bm,
                          void *buffer, 
                          size_t length,
                          lcr_tag_t tag,
                          unsigned flags, 
                          lcr_tag_context_t *tag_ctx)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i = 0, k, nb_posted = 0, nb_ifaces = 0;
        lcr_rail_attr_t attr;
        lcr_tag_t ign = { 0 };
        size_t iovcnt = 0;
        struct iovec iov[1];
        sctk_rail_info_t *iface = ctx->resources[ctx->priority_rail].iface;

        //FIXME: check 0 length memory registration

        /* Count the number of interface to use */
        for (k = 0; k < ctx->num_resources; k++) {
                if (MPC_BITMAP_GET(bm, i)) {
                        nb_ifaces++;
                }
        }

        mem->length     = length;
        mem->base_addr  = (uint64_t)buffer;
        mem->flags      = flags;
        mem->bm         = bm;

        iov[0].iov_base = buffer;
        iov[0].iov_len  = length;
        iovcnt++;

        iface->iface_get_attr(iface, &attr);

        while (length > nb_posted * attr.iface.cap.rndv.max_get_zcopy) {
                if (!MPC_BITMAP_GET(bm, i)) {
                        i = (i + 1) % ctx->num_resources;
                        continue;
                }
                iface = ctx->resources[i].iface;
                rc = iface->post_tag_zcopy(iface, tag, ign, iov, 
                                           iovcnt, flags, tag_ctx);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP MEM: could not post on "
                                               "interface");
                        goto err;
                }

                /* If memory is persistent, only one post per iface. */
                if ((++nb_posted == nb_ifaces) &&
                    (flags & LCR_IFACE_TM_PERSISTANT_MEM)) {
                        break;
                }
        }

err:
        return rc;
}

int lcp_mem_post(lcp_context_h ctx, 
                 lcp_mem_h *mem_p, 
                 void *buffer, 
                 size_t length,
                 lcr_tag_t tag,
                 unsigned flags, 
                 lcr_tag_context_t *tag_ctx)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;
        lcr_rail_attr_t attr;
        lcr_tag_t ign = { 0 };
        size_t iovcnt = 0;
        struct iovec iov[1];
        sctk_rail_info_t *iface = ctx->resources[ctx->priority_rail].iface;

        rc = lcp_mem_create(ctx, &mem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mem->length     = length;
        mem->base_addr  = (uint64_t)buffer;

        iov[0].iov_base = buffer;
        iov[0].iov_len  = length;
        iovcnt++;

        iface->iface_get_attr(iface, &attr);
        build_memory_registration_bitmap(length,
                                         attr.iface.cap.rndv.min_frag_size,
                                         ctx->num_resources,
                                         &mem->bm);
        
        /* Pin the memory and create memory handles */
        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                    iface = ctx->resources[i].iface;
                    iface->post_tag_zcopy(iface, tag, ign, iov, 
                                          iovcnt, flags, tag_ctx);
                    mem->num_ifaces++;
                }
        }

        *mem_p = mem;
err:
        return rc;
}

int lcp_mem_unpost(lcp_context_h ctx, lcp_mem_h mem, lcr_tag_t tag) 
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        /* Memory should be unposted only if it has been previously posted as
         * persistent */
        if (!(mem->flags & LCR_IFACE_TM_PERSISTANT_MEM)) {
                return MPC_LOWCOMM_SUCCESS;
        }

        for (i=0; i<ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = ctx->resources[i].iface;
                        rc = iface->unpost_tag_zcopy(iface, tag);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                               goto err;
                        } 
                }
        }

        lcp_mem_delete(mem);

err:
        return rc;
}

/**
 * @brief deregister memory from the register (unpin and free)
 * 
 * @param ctx lcp context
 * @param mem memory to unpin
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
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

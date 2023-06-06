#ifndef LCP_MEM_H
#define LCP_MEM_H

#include "lcp_types.h"
#include "lcr/lcr_def.h"
#include "lcp_def.h"

/**
 * @brief memory object
 * 
 */
struct lcp_mem {
        uint64_t base_addr;
        size_t length;
        int num_ifaces;
        lcr_memp_t *mems; /* table of memp pointers */
        bmap_t bm;
        unsigned flags;
};

int lcp_mem_create(lcp_context_h ctx, lcp_mem_h *mem_p);
void lcp_mem_delete(lcp_mem_h mem);
size_t lcp_mem_rkey_pack(lcp_context_h ctx, lcp_mem_h mem, void *dest);
int lcp_mem_post_from_map(lcp_context_h ctx, 
                          lcp_mem_h mem, 
                          bmap_t bm,
                          void *buffer, 
                          size_t length,
                          lcr_tag_t tag,
                          unsigned flags, 
                          lcr_tag_context_t *tag_ctx);
int lcp_mem_reg_from_map(lcp_context_h ctx,
                         lcp_mem_h mem,
                         bmap_t mem_map,
                         void *buffer,
                         size_t length);
int lcp_mem_unpost(lcp_context_h ctx, lcp_mem_h mem, lcr_tag_t tag);

#endif

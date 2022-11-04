#ifndef LCP_MEM_H
#define LCP_MEM_H

#include "lcp_types.h"
#include "lcr/lcr_def.h"
#include "lcp_def.h"

struct lcp_mem {
        uint64_t base_addr;
        size_t length;
        int num_ifaces;
        lcr_memp_t *mems; /* table of memp pointers */
};

int lcp_mem_register(lcp_context_h ctx, lcp_mem_h *mem_p, void *buffer, size_t length);
int lcp_mem_deregister(lcp_context_h ctx, lcp_mem_h mem);

#endif

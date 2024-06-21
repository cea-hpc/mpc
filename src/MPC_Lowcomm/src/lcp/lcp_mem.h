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

#ifndef LCP_MEM_H
#define LCP_MEM_H

#include "lcr/lcr_def.h"
#include "lcp_def.h"

#include "rail.h"

#include <bitmap.h>
#include <list.h>
#include <mpc_common_spinlock.h>

struct lcp_pinning_entry_list
{
        struct lcp_pinning_entry *entry;
        uint64_t total_size;
        uint64_t entries_count;

        uint64_t max_entries_count;
        uint64_t max_total_size;

        mpc_common_rwlock_t lock;
};

struct lcp_pinning_mmu
{
        struct lcp_pinning_entry_list list;
        /* In case we want to improve in the future */
};

enum {
        LCP_MEM_FLAG_RELEASE = MPC_BIT(0),
};

typedef enum {
        LCP_MEM_ALLOC_MMAP = 0, /* Use Unix mmap. */
        LCP_MEM_ALLOC_MALLOC,   /* Use malloc.    */
        LCP_MEM_ALLOC_RD        /* Use resource domain, shmem for example. */
} lcp_mem_alloc_method_t;

struct lcp_mem {
        uint64_t base_addr;
        size_t length;
        bmap_t bm;
        unsigned flags;
        lcp_mem_alloc_method_t method;
        void * pointer_to_mmu_ctx; /* When handled by the MMU this 
                                      points to the management slot */
        lcr_memp_t mems[0]; /* table of memp pointers */
};

int lcp_mem_create(lcp_manager_h mngr, lcp_mem_h *mem_p);
void lcp_mem_delete(lcp_mem_h mem);
size_t lcp_mem_rkey_pack(lcp_manager_h mngr, lcp_mem_h mem, void *dest);
int lcp_mem_post_from_map(lcp_manager_h mngr, 
                          lcp_mem_h mem, 
                          bmap_t bm,
                          void *buffer,
                          size_t length,
                          lcr_tag_t tag,
                          unsigned flags,
                          lcr_tag_context_t *tag_ctx);
int lcp_mem_reg_from_map(lcp_manager_h mngr,
                         lcp_mem_h mem,
                         bmap_t mem_map,
                         const void *buffer,
                         size_t length,
                         unsigned flags,
                         bmap_t *reg_bm_p);

int lcp_mem_unpost(lcp_manager_h mngr, lcp_mem_h mem, lcr_tag_t tag);

/************************
 * INTERFACE TO THE MMU *
 ************************/

int lcp_pinning_mmu_init(struct lcp_pinning_mmu **mmu_p, unsigned flags);
int lcp_pinning_mmu_release(struct lcp_pinning_mmu *mmu);

lcp_mem_h lcp_pinning_mmu_pin(lcp_manager_h mngr, const void *addr, 
                              size_t size, bmap_t bitmap, unsigned flags);
int lcp_pinning_mmu_unpin(lcp_manager_h mngr, lcp_mem_h mem);


#endif

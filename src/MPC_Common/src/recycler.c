/* ############################# MPC License ############################## */
/* # Wed Nov 04 16:37:28 CET 2020                                         # */
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
/* # Authors:                                                             # */
/* #   - PEREIRA Romain romain.pereira.ocre@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

# include "mpc_common_recycler.h"

static inline void
__recycler_info_init(
        mpc_common_recycler_info_t * info,
        unsigned int unit_size,
        unsigned int capacity
)
{
    info->unit_size = unit_size;
    info->n         = 0;
    info->capacity  = capacity;
    info->objects   = NULL;
}

void
mpc_common_recycler_init(
    mpc_common_recycler_t * recycler,
    void * (*allocator)(size_t),
    void   (*deallocator)(void *),
    unsigned int unit_size,
    unsigned int capacity
)
{
    recycler->allocator   = allocator;
    recycler->deallocator = deallocator;
    __recycler_info_init(&(recycler->info), unit_size, capacity);
}

static inline void
__recycler_info_deinit(mpc_common_recycler_info_t * info, void (*deallocator)(void *))
{
    if (info->objects) {
        unsigned int i;
        for (i = 0 ; i < info->n ; i++) {
            deallocator(info->objects[i]);
        }
        deallocator(info->objects);
    }
}

void
mpc_common_recycler_deinit(mpc_common_recycler_t * recycler)
{
    __recycler_info_deinit(&(recycler->info), recycler->deallocator);
}

void *
__recycler_info_alloc(mpc_common_recycler_info_t * info, void * (*allocator)(size_t))
{
    if (info->n == 0) {
        return allocator(info->unit_size);
    }
    --info->n;
    return info->objects[info->n];

}

void *
mpc_common_recycler_alloc(mpc_common_recycler_t * recycler)
{
    return __recycler_info_alloc(&(recycler->info), recycler->allocator);
}

static inline void
__recycler_info_recycle(
        mpc_common_recycler_info_t * info,
        void * object,
        void * (*allocator)(size_t),
        void (*deallocator)(void *)
)
{
    if (info->n == info->capacity) {
        deallocator(object);
    } else {
        if (info->objects == NULL) {
            info->objects = (void **) allocator(sizeof(void *) * info->capacity);
            if (!info->objects) {
                deallocator(object);
                return ;
            }
        }
        info->objects[info->n] = object;
        ++info->n;
    }
}

void
mpc_common_recycler_recycle(mpc_common_recycler_t * recycler, void * object)
{
    __recycler_info_recycle(&(recycler->info), object, recycler->allocator, recycler->deallocator);
}

static inline unsigned int
__nrecycler_p2_exponent(unsigned int x)
{
    if (x == 1) {
        return 0;
    }
    return 32 - __builtin_clz(x - 1);
}

void
mpc_common_nrecycler_init(
    mpc_common_nrecycler_t * nrecycler,
    void * (*allocator)(size_t),
    void   (*deallocator)(void *),
    unsigned int capacities[32]
)
{
    nrecycler->allocator   = allocator;
    nrecycler->deallocator = deallocator;

    unsigned int i;
    for (i = 0 ; i < 32 ; i++) {
        __recycler_info_init(
                nrecycler->infos + i,
                1 << i,
                capacities[i]
        );
    }
}

void
mpc_common_nrecycler_deinit(mpc_common_nrecycler_t * nrecycler)
{
    unsigned int i;
    for (i = 0 ; i < 32 ; i++) {
        __recycler_info_deinit(nrecycler->infos + i, nrecycler->deallocator);
    }
}

void *
mpc_common_nrecycler_alloc(mpc_common_nrecycler_t * nrecycler, unsigned int size)
{
    unsigned int n = __nrecycler_p2_exponent(size);
    return __recycler_info_alloc(nrecycler->infos + n, nrecycler->allocator);
}

void
mpc_common_nrecycler_recycle(mpc_common_nrecycler_t * nrecycler, void * object, unsigned int size)
{
    const unsigned int n = __nrecycler_p2_exponent(size);
    __recycler_info_recycle(nrecycler->infos + n, object, nrecycler->allocator, nrecycler->deallocator);
}


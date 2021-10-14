/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:39:43 CEST 2021                                        # */
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
/* # - Romain PEREIRA <pereirar@r1login.c-inti.ccc.ocre.cea.fr>           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __RECYCLER_H__
# define __RECYCLER_H__

# include <stdlib.h>

typedef struct  mpc_common_recycler_info_s
{
    /* recycler capacity */
    unsigned int capacity;

    /* number of objects in the recycler array */
    unsigned int n;

    /* the size of a single element, in bytes */
    unsigned int unit_size;

    /* the pointers */
    void ** objects;
}               mpc_common_recycler_info_t;

/**
 * A recycler of objects having the same size,
 * which cuts the number of allocation and deallocation calls
 */
typedef struct  mpc_common_recycler_s
{
    /* the memory allocator */
    void * (*allocator)(size_t);

    /* the memory de-allocator */
    void   (*deallocator)(void *);

    /* the recycler info */
    mpc_common_recycler_info_t info;
}               mpc_common_recycler_t;

/**
 *	\brief Initialize the given recycler
 *	\param recycler a recycler
 *	\param allocator the memory allocator (malloc)
 *	\param deallocator the memory deallocator (free)
 *	\param unit_size the size of a single element, in bytes
 *	\param capacity number of object that can be recycled at most before being free-ed
 */
void mpc_common_recycler_init(
    mpc_common_recycler_t * recycler,
    void * (*allocator)(size_t),
    void   (*deallocator)(void *),
    unsigned int unit_size,
    unsigned int capacity
);

/**
 *	\brief Deinitialize the recycler (which was initialized by a 'recycler_init()' call)
 *	\param recycler the recycler
 */
void mpc_common_recycler_deinit(mpc_common_recycler_t * recycler);

/**
 * \brief Get a recycled object, or allocate a new one
 * \param recycler the recycler
 * \return the object
 */
void * mpc_common_recycler_alloc(mpc_common_recycler_t * recycler);

/**
 * \brief
 * \param recycler the recycler
 * \param object the object to be recycled
 * \attention If the recycler is full, the object is freed
 */
void mpc_common_recycler_recycle(mpc_common_recycler_t * recycler, void * object);

/**
 * A recycler of objects which may have different sizes
 */
typedef struct  mpc_common_nrecycler_s
{
    /* the memory allocator */
    void * (*allocator)(size_t);

    /* the memory de-allocator */
    void   (*deallocator)(void *);

    /* the recycler info */
    mpc_common_recycler_info_t infos[32];
}               mpc_common_nrecycler_t;

void mpc_common_nrecycler_init(
    mpc_common_nrecycler_t * nrecycler,
    void * (*allocator)(size_t),
    void   (*deallocator)(void *),
    unsigned int capacities[32]
);

/**
 *	\brief Deinitialize the rnecycler (which was initialized by a 'recycler_init()' call)
 *	\param nrecycler the nrecycler
 */
void mpc_common_nrecycler_deinit(mpc_common_nrecycler_t * nrecycler);

/**
 * \brief Get a recycled object, or allocate a new one
 * \param nrecycler the nrecycler
 * \param size the size of the object
 * \return the object
 */
void * mpc_common_nrecycler_alloc(mpc_common_nrecycler_t * nrecycler, unsigned int size);

/**
 * \brief
 * \param nrecycler the nrecycler
 * \param object the object to be nrecycled
 * \param size the size of the object
 * \attention If the recycler is full, the object is freed
 */
void mpc_common_nrecycler_recycle(mpc_common_nrecycler_t * nrecycler, void * object, unsigned int size);

#endif /* __RECYCLER_H__ */

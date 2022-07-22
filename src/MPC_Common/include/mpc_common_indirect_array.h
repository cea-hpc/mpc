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
/* # - Romain PEREIRA <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */

# ifndef __MPC_COMMON_INDIRECT_ARRAY_H__
#  define __MPC_COMMON_INDIRECT_ARRAY_H__

typedef struct  mpc_common_indirect_array_slot_s
{
    /* next slot in the 'slots' array (which is free or taken depending on context) */
    size_t next;
}               mpc_common_indirect_array_slot_t;

typedef struct  mpc_common_indirect_array_s
{
    /* size of the 'slots' array */
    size_t capacity;

    /* size of objects within the array */
    size_t unitsize;

    /* first null element in the array */
    size_t first_null;

    /* first non-null element in the array */
    size_t first_non_null;

    /* slots of size 'mpc_common_indirect_array_slot_t' + 'unitsize' times 'capacity' */
    char * slots;

}               mpc_common_indirect_array_t;

typedef struct  mpc_common_indirect_array_iterator_s
{
    /* the array of this iterator */
    mpc_common_indirect_array_t * array;

    /* next element of this iterator in the 'slots' array */
    size_t next;

    /* last slot returned */
    size_t last;
}               mpc_common_indirect_array_iterator_t;

/* Reset the given iterator */
void mpc_common_indirect_array_iterator_reset(mpc_common_indirect_array_iterator_t * it);

/* Initialize an iterator over the array */
void mpc_common_indirect_array_iterator_init(mpc_common_indirect_array_t * array, mpc_common_indirect_array_iterator_t * it);

/* Add an element where the iterator is pointing to in the array */
void mpc_common_indirect_array_iterator_push(mpc_common_indirect_array_iterator_t * it);

/* Remove and return the element where the iterator is pointing at */
void * mpc_common_indirect_array_iterator_pop(mpc_common_indirect_array_iterator_t * it);

/* Return the element where the iterator is pointing at */
void * mpc_common_indirect_array_iterator_next(mpc_common_indirect_array_iterator_t * it);

/* Initialize an indirect array */
void mpc_common_indirect_array_init(mpc_common_indirect_array_t * array, size_t unitsize, size_t capacity);

/* Release an indirect array */
void mpc_common_indirect_array_deinit(mpc_common_indirect_array_t * array);

/* Add an element to the indirect array */
void * mpc_common_indirect_array_add(mpc_common_indirect_array_t * array);

# endif /* __MPC_COMMON_INDIRECT_ARRAY_H__ */

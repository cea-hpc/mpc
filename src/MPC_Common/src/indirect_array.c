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

/* Return a slot from the array */
static inline mpc_common_indirect_array_slot_t *
__indirect_array_get_slot(
    mpc_common_indirect_array_t * array,
    size_t slot)
{
    return ((char *) array->slots) + (sizeof(mpc_common_indirect_array_slot_t) + unitsize) * slot;
}

/* Return the object attached to a slot */
static inline void *
__indirect_array_get_slot_object(
    mpc_common_indirect_array_slot_t * slot)
{
    return (void *) (slot + 1);
}

/* Reset the given iterator */
void
mpc_common_indirect_array_iterator_reset(
    mpc_common_indirect_array_iterator_t * it)
{
    it->next = it->array->first;
    it->last = (size_t) -1;
}

/* Initialize an iterator over the array */
void
mpc_common_indirect_array_iterator_init(
    mpc_common_indirect_array_t * array,
    mpc_common_indirect_array_iterator_t * it)
{
    it->array = array;
    mpc_common_indirect_array_iterator_reset(it);
}

/* Add an element where the iterator is pointing to in the array */
void
mpc_common_indirect_array_iterator_push(
    mpc_common_indirect_array_iterator_t * it)
{
    not_implemented();
}

/* Remove and return the element where the iterator is pointing at */
void *
mpc_common_indirect_array_iterator_pop(
    mpc_common_indirect_array_iterator_t * it)
{
    /* relink the nodes
     * 'next' is now free and link with the free list within the array
     * 'last->next' should now link with old 'next->next' which points to a non-null object */

    mpc_common_indirect_array_slot_t * last = __indirect_array_get_slot(it->array, it->last);
    mpc_common_indirect_array_slot_t * next = __indirect_array_get_slot(it->array, it->next);

    /* 'last->next' should now link with 'next->next' which points to a non-null object */
    last->next = next->next;

    /* 'next' is now free : insert it in the 'free nodes' list */
    next->next                  = it->array->first_non_null;
    it->array->first_non_null   = it->next;

    /* iterator should now point to the next non-null element  */
    it->next = last->next;

    return __indirect_array_get_slot_object(next);
}

/* Return the element where the iterator is pointing at */
void *
mpc_common_indirect_array_iterator_next(
    mpc_common_indirect_array_iterator_t * it)
{
    mpc_common_indirect_array_slot_t * slot = __indirect_array_get_slot(it->array, it->next);
    it->last = it->next;
    it->next = slot->next;
    return __indirect_array_get_slot_object(slot);
}

static void
__indirect_array_init_slots(
    mpc_common_indirect_array_t * array,
    size_t from)
{
    for (size_t i = from ; i < array->capacity ; ++i)
    {
        mpc_common_indirect_array_slot_t * slot = __indirect_array_get_slot(array, i);
        slot->next = i + 1;
    }
}

/* Initialize an indirect array */
void
mpc_common_indirect_array_init(
    mpc_common_indirect_array_t * array,
    size_t unitsize,
    size_t capacity)
{
    assert(unitsize);
    assert(capacity);

    // TODO : may want to pad the allocation for cache alignements
    array->capacity         = capacity;
    array->unitsize         = unitsize;
    array->next             = 0;
    array->first_null       = 0;
    array->first_non_null   = (size_t) -1;
    array->slots    = (char *) malloc((sizeof(mpc_common_indirect_array_slot_t) + unitsize) * capacity);
    __indirect_array_init_slots(array, 0);
}

/* Release an indirect array */
void
mpc_common_indirect_array_deinit(
    mpc_common_indirect_array_t * array)
{
    free(array->slots);
}

/* Add an element to the indirect array */
void *
mpc_common_indirect_array_add(
    mpc_common_indirect_array_t * array)
{
    size_t idx = array->next;
    if (idx == array->capacity)
    {
        array->capacity = 2 * array->capacity;
        array->slots    = (char *) realloc(array->slots, (sizeof(mpc_common_indirect_array_slot_t) + unitsize) * array->capacity);
        __indirect_array_init_slots(array, array->capacity / 2);
    }

    mpc_common_indirect_array_slot_t * slot = __indirect_array_get_slot(idx);

    /* adding 1st non null element in the array */
    if (array->first_non_null == (size_t) -1)
    {
        array->first_non_null = idx;
    }

    /* adjust next null element */
    array->first_null = slot->next;

    return __indirect_array_get_slot_object(slot);
}

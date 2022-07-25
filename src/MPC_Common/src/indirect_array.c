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

# include "mpc_common_indirect_array.h"
# include "mpc_common_debug.h"

#  define EMPTY ((size_t) -1)

/* Return a slot from the array */
static inline mpc_common_indirect_array_slot_t *
__indirect_array_get_slot(
    mpc_common_indirect_array_t * array,
    size_t slot)
{
    return (mpc_common_indirect_array_slot_t *) (((char *) array->slots) + (sizeof(mpc_common_indirect_array_slot_t) + array->unitsize) * slot);
}

/* Return the object attached to a slot */
static inline void *
__indirect_array_get_slot_object(
    mpc_common_indirect_array_slot_t * slot)
{
    return (void *) (slot + 1);
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

/* resize array if full */
static inline void
__indirect_array_ensure_capacity(
    mpc_common_indirect_array_t * array)
{
    if (array->first_null == array->capacity)
    {
        array->capacity = 2 * array->capacity;
        array->slots    = (char *) realloc(array->slots, (sizeof(mpc_common_indirect_array_slot_t) + array->unitsize) * array->capacity);
        __indirect_array_init_slots(array, array->capacity / 2);
    }
}

/* Return 1 if the iterator iterated over the entire array */
int
mpc_common_indirect_array_iterator_finished(
    mpc_common_indirect_array_iterator_t * it)
{
    return it->next == EMPTY;
}

/* Reset the given iterator */
void
mpc_common_indirect_array_iterator_reset(
    mpc_common_indirect_array_iterator_t * it)
{
    it->next = it->array->first_non_null;
    it->prev = EMPTY;
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

/* Add an element between the previous and next object pointed by the iterator */
void
mpc_common_indirect_array_iterator_push(
    mpc_common_indirect_array_iterator_t * it,
    void * object)
{
    mpc_common_indirect_array_t * array = it->array;
    __indirect_array_ensure_capacity(array);

    size_t prev_idx = it->prev;
    mpc_common_indirect_array_slot_t * prev = (prev_idx == EMPTY) ? NULL : __indirect_array_get_slot(array, prev_idx);

    size_t current_idx = it->next;
    //mpc_common_indirect_array_slot_t * current = (current_idx == EMPTY) ? NULL : __indirect_array_get_slot(array, current_idx);

    size_t insert_idx = array->first_null;
    mpc_common_indirect_array_slot_t * insert = __indirect_array_get_slot(array, insert_idx);
    array->first_null = insert->next;

    if (array->first_non_null == EMPTY)
    {
        assert(prev == NULL);
        array->first_non_null = insert_idx;
    }

    if (prev) prev->next = insert_idx;
    insert->next = current_idx;
    it->prev = insert_idx;

    void * object_storage = __indirect_array_get_slot_object(insert);
    memcpy(object_storage, object, array->unitsize);
}

/* Remove and return the element where the iterator is pointing at */
void *
mpc_common_indirect_array_iterator_pop(
    mpc_common_indirect_array_iterator_t * it)
{
    if (it->next == EMPTY) return NULL;

    mpc_common_indirect_array_t * array = it->array;

    size_t prev_idx = it->prev;
    mpc_common_indirect_array_slot_t * prev = (prev_idx == (size_t) EMPTY) ? NULL : __indirect_array_get_slot(array, prev_idx);

    size_t current_idx = it->next;
    mpc_common_indirect_array_slot_t * current = __indirect_array_get_slot(array, current_idx);

    size_t next_idx = current->next;
    //mpc_common_indirect_array_slot_t * next = __indirect_array_get_slot(array, next_idx);

    /* relink non-null */
    it->next = next_idx;
    if (prev)
    {
        prev->next = next_idx;
    }
    else
    {
        assert(array->first_non_null == current_idx);
        array->first_non_null = next_idx;
    }


    /* relink null */
    current->next = array->first_null;
    array->first_null = current_idx;

    return __indirect_array_get_slot_object(current);
}

/* Return the element where the iterator is pointing at */
void *
mpc_common_indirect_array_iterator_next(
    mpc_common_indirect_array_iterator_t * it)
{
    if (mpc_common_indirect_array_iterator_finished(it)) return NULL;
    mpc_common_indirect_array_slot_t * slot = __indirect_array_get_slot(it->array, it->next);
    it->prev = it->next;
    it->next = slot->next;
    return __indirect_array_get_slot_object(slot);
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
    array->first_null       = 0;
    array->first_non_null   = EMPTY;
    array->slots            = (char *) malloc((sizeof(mpc_common_indirect_array_slot_t) + unitsize) * capacity);
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
    mpc_common_indirect_array_t * array,
    void * object)
{
    (void)object;
    (void)array;
    not_implemented();
    return NULL;
}

# undef EMPTY

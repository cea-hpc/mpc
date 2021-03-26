/* ############################ MPC License ############################## */
/* # Fri Jan 18 14:00:00 CET 2013                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpc_common_datastructure.h>

#include "sctk_alloc.h"
#include "mpc_common_debug.h"
#include <string.h>

void mpc_common_bit_array_init_buff(struct mpc_common_bit_array *ba, uint64_t size,
                              void *buff, uint64_t buff_size) {
  ba->real_size = size;
  ba->size = (size >> 3);

  if (buff_size < ba->size) {
    mpc_common_debug_fatal("%s : Buffer must be at least %ld to store %ld bits (got %ld)",
               __FUNCTION__, ba->size, size, buff_size);
  }

  ba->array = buff;

  if (!ba->array) {
    mpc_common_debug_fatal("%s : Buffer was NULL", __FUNCTION__);
  }

  unsigned int i = 0;
  for (i = 0; i < ba->size; i++)
    ba->array[i] = 0;
}

void mpc_common_bit_array_init(struct mpc_common_bit_array *ba, uint64_t size) {

  size_t array_size = (size >> 3);

  void *array = sctk_malloc(array_size * sizeof(uint8_t));

  if (!array) {
    mpc_common_debug_fatal("Failed to allocate a mpc_common_bit_array \n");
  }

  mpc_common_bit_array_init_buff(ba, size, array, array_size);
}

void mpc_common_bit_array_release(struct mpc_common_bit_array *ba) {
  unsigned int i = 0;
  for (i = 0; i < ba->size; i++)
    ba->array[i] = 0;

  ba->size = 0;
  ba->real_size = 0;

  sctk_free(ba->array);
  ba->array = NULL;
}

void mpc_common_bit_array_replicate(struct mpc_common_bit_array *dest,
                              struct mpc_common_bit_array *src) {
  if (dest->size != src->size) {
    printf("Trying to copy to a different size mpc_common_bit_array\n");
    abort();
  }

  memcpy(dest->array, src->array, dest->size);
}

void mpc_common_bit_array_dump(struct mpc_common_bit_array *ba) {

  unsigned int i = 0;
  unsigned int size = ba->size * 8;

  printf("Array size is %d \n", size);

  for (i = 0; i < size; i++) {
    printf("[%d]%d ", i, mpc_common_bit_array_get(ba, i));
  }

  printf("\n");
}

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
#include "sctk_bit_array.h"
#include "sctk_debug.h"
#include <string.h>

void sctk_bit_array_init_buff(struct sctk_bit_array *ba, sctk_uint64_t size,
                              void *buff, sctk_uint64_t buff_size) {
  ba->real_size = size;
  ba->size = (size >> 3);

  if (buff_size < ba->size) {
    sctk_fatal("%s : Buffer must be at least %ld to store %ld bits (got %ld)",
               __FUNCTION__, ba->size, size, buff_size);
  }

  ba->array = buff;

  if (!ba->array) {
    sctk_fatal("%s : Buffer was NULL", __FUNCTION__);
  }

  int i = 0;
  for (i = 0; i < ba->size; i++)
    ba->array[i] = 0;
}

void sctk_bit_array_init(struct sctk_bit_array *ba, sctk_uint64_t size) {

  size_t array_size = (size >> 3);

  void *array = malloc(array_size * sizeof(sctk_uint8_t));

  if (!array) {
    sctk_fatal("Failed to allocate a sctk_bit_array \n");
  }

  sctk_bit_array_init_buff(ba, size, array, array_size);
}

void sctk_bit_array_release(struct sctk_bit_array *ba) {
  int i = 0;
  for (i = 0; i < ba->size; i++)
    ba->array[i] = 0;

  ba->size = 0;
  ba->real_size = 0;

  free(ba->array);
  ba->array = NULL;
}

void sctk_bit_array_replicate(struct sctk_bit_array *dest,
                              struct sctk_bit_array *src) {
  if (dest->size != src->size) {
    printf("Trying to copy to a different size sctk_bit_array\n");
    abort();
  }

  memcpy(dest->array, src->array, dest->size);
}

void sctk_bit_array_dump(struct sctk_bit_array *ba) {

  int i = 0;
  unsigned int size = ba->size * 8;

  printf("Array size is %d \n", size);

  for (i = 0; i < size; i++) {
    printf("[%d]%d ", i, sctk_bit_array_get(ba, i));
  }

  printf("\n");
}

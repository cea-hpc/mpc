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

/**
 * @addtogroup sctk_bit_array_
 * @{
 */

#include <stdio.h>
#include <stdlib.h>

#include "mpc_common_types.h"

#ifndef SCTK_BIT_ARRAY_H
#define SCTK_BIT_ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Masks used to get and set bits.
 *
 * It is binary anded/ored with the buffer to get/set the bit value :
 *  @code
 *  bit = buffer & mask;
 *  buffer = buffer | mask;
 *  @endcode
 * Example in 4 bits :
 * 0001
 * 0010
 * 0100
 * 1000
 */
static const int get_set_bit_mask[] = {1, 2, 4, 8, 16, 32, 64, 128};

/**
 * @brief Masks used to unset bits
 *
 *  It is binary anded to unset bit on the buffer :
 *  @code
 *  buffer = buffer & mask;
 *  @endcode
 * Example in 4 bits :
 * 1110
 * 1101
 * 1011
 * 0111
 */
static const int unset_bit_mask[] = {254, 253, 251, 247, 239, 223, 191, 127};

/**
 * @brief Defines a bit array
 */
struct sctk_bit_array {
  uint8_t *array; /**< @brief The data */
  uint64_t size;  /**< @brief The number of uint8_t array contains */
  uint64_t real_size; /**< @brief The number of bits the array contains */
};

/**
 * @brief Getter on the array size
 * @param ba the array
 * @return the number of bits ba contains
 */
static inline uint64_t sctk_bit_array_size(struct sctk_bit_array *ba) {
  return ba->real_size;
}

/**
 * @brief sctk_bit_array initializer
 * @param ba the sctk_bit_array to initialize
 * @param size the number of bits wanted fot ba
 */
void sctk_bit_array_init(struct sctk_bit_array *ba, uint64_t size);

/**
 * @brief sctk_bit_array initializer with preallocated buffer
 * @param ba the sctk_bit_array to initialize
 * @param size the number of bits wanted fot ba
 * @param buff a preallocated buffer larger than size bits
 * @param buff_size the size of the preallocated buffer
 */
void sctk_bit_array_init_buff(struct sctk_bit_array *ba, uint64_t size,
                              void *buff, uint64_t buff_size);

/**
 * @brief sctk_bit_array deinitializer
 * @param ba the sctk_bit_array to deinitialize
 */
void sctk_bit_array_release(struct sctk_bit_array *ba);

/**
 * @brief Copies a sctk_bit_array into another
 * @param dest the destination sctk_bit_array
 * @param src the source sctk_bit_array
 *
 * @warning Both src and dest must be initialized at the same size before
 * copying.
 */
void sctk_bit_array_replicate(struct sctk_bit_array *dest,
                              struct sctk_bit_array *src);

/**
 * @brief Prints a sctk_bit_array on standard output
 * @param ba the sctk_bit_array to print
 */
void sctk_bit_array_dump(struct sctk_bit_array *ba);

/**
 * @brief Sets a bit in a sctk_bit_array
 * @param ba the sctk_bit_array where to set the bit
 * @param key the index of the bit to set
 * @param value the value to set
 */
static inline void sctk_bit_array_set(struct sctk_bit_array *ba,
                                      uint64_t key, uint8_t value) {

  if (ba->real_size <= key) {
    // printf(" SET Out of bound (%ld)\n", key);
    return;
  }

  uint64_t extern_offset = key >> 3;
  uint8_t local_offset = key - extern_offset * 8;

  uint8_t *target = &ba->array[extern_offset];

  if (value)
    *target |= get_set_bit_mask[local_offset];
  else
    *target &= unset_bit_mask[local_offset];
}

/**
 * @brief Gets a bit in a sctk_bit_array
 * @param ba the sctk_bit_array where to get the bit
 * @param key the index of the bit to get
 * @return the value of the bit (3 if out of bound)
 */
static inline uint8_t sctk_bit_array_get(struct sctk_bit_array *ba,
                                              uint64_t key) {
  if (ba->real_size <= key) {
    // printf(" GET Out of bound (%ld) \n", key);
    return 3;
  }

  uint64_t extern_offset = key >> 3;
  uint8_t local_offset = key - extern_offset * 8;

  return (ba->array[extern_offset] & get_set_bit_mask[local_offset]) >>
         local_offset;
}

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */

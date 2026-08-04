/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* #   - BEAULIEU Corentin corentin.beaulieu@cea.fr                       # */
/* #                                                                      # */
/* ######################################################################## */

/**@file
 * OpenSHMEM internal symmetric heap allocator API
 */

#ifndef OSHMEM_MEMORY_H_
#define OSHMEM_MEMORY_H_
#include <stddef.h>

/** @defgroup openshmem_memory Memory management
 * @ingroup openshmem_internals
 * @{
 * @brief Symmetric memory management related items
 */

/** @brief Initialized the fixed size buffer stack
 *
 * This simple stack is used to allocate and deallocate memory from the symmetric heap
 *
 * @param[in] size Total size of the stack to create (in Bytes) and real size allocated upon return
 * @retval NULL if the allocation failed
 * @return         Address of the initialized stack
 */
void * mpc_openshmem_memory_symmetric_stack_init(size_t *size);

/** @brief Allocate a memory block on the symmetric stack
 *
 * Allocation is always made to the top of the stack
 *
 * @param[in] stack Address identifying the stack (the one returned by @ref mpc_openshmem_memory_symmetric_stack_init)
 * @param[in] size  Size of the block to allocate (in Bytes)
 *
 * @retval NULL if the stack is full
 * @return          A valid symmetric pointer
 */
void * mpc_openshmem_memory_symmetric_stack_alloc(void *stack, const size_t size);

/** @brief Try to resize an already allocated block on the symmetric stack
 *
 * If the block to resize isn't the last on the stack, we have to allocate a new block.
 *
 * @param[in] stack    Address identifying the stack (the one returned by @ref
 *                     mpc_openshmem_memory_symmetric_stack_init)
 * @param[in] ptr      Address of the previous block to resize
 * @param[in] new_size New size of the block (in Bytes)
 *
 * @retval NULL if the stack is full
 * @return             Address of the resized block
 */
void * mpc_openshmem_memory_symmetric_stack_resize(void *stack, void *ptr, const size_t new_size);

/** @brief Try to free a block from the symmetric stack
 *
 * A memory block is deallocated only if it the last allocation made to date.
 *
 * @param[in] stack Address identifying the stack (the one returned by @ref mpc_openshmem_memory_symmetric_stack_init)
 * @param[in] ptr   Address of the block to free
 */
void mpc_openshmem_memory_symmetric_stack_free(void *stack, void *ptr);

/** @brief Relinquish the symmetric stack
 *
 * @param[in] start_addr Address of the allocated stack
 */
void mpc_openshmem_memory_symmetric_stack_deinit(void *start_addr);

/** @} */ // openshmem_internals

#endif // OSHMEM_MEMORY_H_

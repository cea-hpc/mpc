#pragma once
#include <stddef.h>

#ifdef MPC_MODULE
#undef MPC_MODULE
#endif
#define MPC_MODULE "OpenSHMEM/memory"

/** @defgroup openshmem_memory Memory management
 * @ingroup openshmem_internals
 * @{
 * @brief Symmetric memory management related items
 */

/** @brief Initialized the fixed size buffer stack
 *
 * This simple stack is used to allocate and deallocate memory from the symmetric heap
 *
 * @param[in] size Total size of the stack to create (in Bytes)
 * @retval NULL if the allocation failed
 * @return         Address of the bottom of the stack
 */
void * mpc_openshmem_memory_symmetric_stack_init(const size_t size);

/** @brief Allocate a memory block on the symmetric stack
 *
 * Allocation is always made to the top of the stack
 *
 * @param[in] size Size of the block to allocate (in Bytes)
 * @retval NULL if the stack is full
 * @return         A valid symmetric pointer
 */
void * mpc_openshmem_memory_symmetric_stack_alloc(const size_t size);

/** @brief Try to resize an already allocated block on the symmetric stack
 *
 * If the block to resize isn't the last on the stack, we have to allocate a new block.
 *
 * @param[in] ptr      Address of the previous block to resize
 * @param[in] new_size New size of the block (in Bytes)
 * @retval NULL if the stack is full
 * @return             Address of the resized block
 */
void * mpc_openshmem_memory_symmetric_stack_resize(const void *const ptr, const size_t new_size);

/** @brief Try to free a block from the symmetric stack
 *
 * A memory block is deallocated only if it the last allocation made to date.
 *
 * @param[in] ptr Address of the block to free
 */
void mpc_openshmem_memory_symmetric_stack_free(const void *ptr);

/** @brief Relinquish the symmetric stack
 */
void mpc_openshmem_memory_symmetric_stack_deinit(void);

/** @} */ // openshmem_internals

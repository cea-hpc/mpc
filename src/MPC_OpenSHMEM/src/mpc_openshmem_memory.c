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
 * OpenSHMEM internal symmetric heap allocator implementation
 */

#include <stdbool.h>
#include <string.h>

#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>

#include <sctk_alloc.h>

#include "mpc_openshmem_memory.h"

#ifdef MPC_MODULE
#undef MPC_MODULE
#endif
#define MPC_MODULE "OpenSHMEM/memory"

/** @ingroup openshmem_memory
 * @{
 */

/** @brief Information of the symmetric stack */
typedef struct _mpc_openshmem_stack_info_s
{
	int                   canary;          /**< Canary for integrity checking */
	mpc_common_spinlock_t lock;            /**< Lock for thread safe access */
	void *                bottom;          /**< First address of the stack */
	size_t                size;            /**< Fixed size of the whole stack */
	ptrdiff_t             top;             /**< Offset to next free block in the stack */
	size_t                last_block_size; /**< Size of the last allocation */
	char                  padding[22];     /**< Pad to 64 bytes to align the following header */
} __oshmem_stack_info_t;

/** @brief Header of the memory blocks */
typedef struct __openshmem_allocator_memory_header_s
{
	size_t canary;              /**< Canary for integrity checking */
	size_t previous_block_size; /**< Size of the previous block */
	size_t block_size;          /**< Size of the block */
	char   padding[36];         /**< Pad to 64 bytes to align the following memory block */
} __oshmem_allocator_memory_header_t;

#if __STDC_VERSION__ < 202300L
	static const ptrdiff_t __oshmem_stack_alignment = 64;
#else
	constexpr ptrdiff_t __oshmem_stack_alignment = 64;
#endif

#define __oshmem_memory_stack_info_from_ptr(__ptr) (((__oshmem_stack_info_t *)__ptr) - 1)
#define __shmem_memory_header_from_ptr(__ptr)      (((__oshmem_allocator_memory_header_t *)__ptr) - 1)

/** @brief Check if this address correspond to the last allocated one
 *
 * @param[in] ptr address to check
 * @retval    true ptr is the address of the lastly allocated block
 * @retval    false ptr is not the address of the lastly allocated block
 */
static inline bool __oshmem_memory_is_last_allocation(const __oshmem_stack_info_t *const stack_info,
                                                      const void *const ptr)
{
	return (stack_info->bottom + (stack_info->top - stack_info->last_block_size)) == ptr;
}

#if !defined(NDEBUG)
	#define __oshmem_memory_stack_check_canary(__stack)                                   \
			do                                                                            \
			{                                                                             \
				if ((__stack)->canary != 0xdead)                                          \
				{                                                                         \
					mpc_common_debug_fatal("Corruption in the symmetric heap detected!"); \
				}                                                                         \
			} while (0)
	#define __oshmem_memory_block_check_canary(__mem_block)                                         \
			do                                                                                      \
			{                                                                                       \
				if ((__mem_block)->canary != 0xdeadc0de)                                            \
				{                                                                                   \
					mpc_common_debug_fatal(                                                         \
	"Invalid pointer at %p or Buffer overflow detected in the symmetric heap at %p (canary: %0#x)", \
	__mem_block + 1, __mem_block, __mem_block->canary);                                             \
				}                                                                                   \
			} while (0)
#else
	#define __oshmem_memory_stack_check_canary(__stack)     ;
	#define __oshmem_memory_block_check_canary(__mem_block) ;

#endif

void * mpc_openshmem_memory_symmetric_stack_init(size_t *const size)
{
	*size = *size + 1024 * sizeof(__oshmem_allocator_memory_header_t) + sizeof(__oshmem_stack_info_t);
	__oshmem_stack_info_t *new_stack = sctk_memalign(sizeof(__oshmem_allocator_memory_header_t), *size);
	new_stack->canary = 0xdead;
	mpc_common_spinlock_init(&new_stack->lock, 0);
	new_stack->bottom          = NULL;
	new_stack->size            = *size;
	new_stack->top             = 0;
	new_stack->last_block_size = 0;

	new_stack->bottom = new_stack + 1;
	// Verify that the header is correctly aligned
	assert(((size_t)new_stack->bottom % sizeof(__oshmem_allocator_memory_header_t)) == 0);

	return new_stack->bottom;
}

static void * mpc_openshmem_memory_symmetric_stack_alloc_no_lock(__oshmem_stack_info_t *stack_info,
                                                                 const size_t size)
{
	// Set the block for the next address to be aligned
	const size_t aligned_size =
		((size % __oshmem_stack_alignment) == 0)
		? size + sizeof(__oshmem_allocator_memory_header_t)
		: ((size / __oshmem_stack_alignment) + 1) * __oshmem_stack_alignment
		+ sizeof(__oshmem_allocator_memory_header_t);

	// Not enough space to alloc the required size
	if (stack_info->top + aligned_size >= stack_info->size)
	{
		return NULL;
	}
	// Verify that the header is correctly aligned
	assert((stack_info->top % sizeof(__oshmem_allocator_memory_header_t)) == 0);

	// Compute the address to the newly allocated block
	__oshmem_allocator_memory_header_t *ret = stack_info->bottom + stack_info->top;

	ret->previous_block_size = stack_info->last_block_size;
	ret->block_size          = aligned_size;
	ret->canary = 0xdeadc0de;

	// Reserve the newly allocated block
	stack_info->last_block_size = aligned_size;
	stack_info->top            += aligned_size;

	mpc_common_nodebug("Allocating memory @ %p (size: %llu)", ret + 1, aligned_size);

	return ret + 1;
}

void * mpc_openshmem_memory_symmetric_stack_alloc(void *stack_start, const size_t size)
{
	__oshmem_stack_info_t *stack_info = __oshmem_memory_stack_info_from_ptr(stack_start);

	__oshmem_memory_stack_check_canary(stack_info);
	// Set the block for the next address to be aligned
	const size_t aligned_size =
		((size % __oshmem_stack_alignment) == 0)
		? size + sizeof(__oshmem_allocator_memory_header_t)
		: ((size / __oshmem_stack_alignment) + 1) * __oshmem_stack_alignment
		+ sizeof(__oshmem_allocator_memory_header_t);

	// Not enough space to alloc the required size
	if (stack_info->top + aligned_size >= stack_info->size)
	{
		return NULL;
	}
	// Verify that the header is correctly aligned
	assert((stack_info->top % sizeof(__oshmem_allocator_memory_header_t)) == 0);

	mpc_common_spinlock_lock(&stack_info->lock);
	// Compute the address to the newly allocated block
	__oshmem_allocator_memory_header_t *ret = stack_info->bottom + stack_info->top;

	ret->previous_block_size = stack_info->last_block_size;
	ret->block_size          = aligned_size;
	ret->canary = 0xdeadc0de;

	// Reserve the newly allocated block
	stack_info->last_block_size = aligned_size;
	stack_info->top            += aligned_size;

	mpc_common_spinlock_unlock(&stack_info->lock);

	mpc_common_nodebug("Allocating memory %p @ %p (size: %llu)", ret, ret + 1, aligned_size);

	return ret + 1;
}

static void mpc_openshmem_memory_symmetric_stack_free_no_lock(__oshmem_stack_info_t *stack_info, void *ptr)
{
	// Retrieve the headers
	__oshmem_allocator_memory_header_t *memory_block = __shmem_memory_header_from_ptr(ptr);
	__oshmem_allocator_memory_header_t *prev_block   = ((void *)memory_block - memory_block->previous_block_size);

	mpc_common_nodebug("Freeing memory @ %p", ptr);
	// Check the integrity of the memory block before reading it
	__oshmem_memory_block_check_canary(memory_block);
	// Check if the block has already been freed
	if (memory_block->block_size == 0)
	{
		mpc_common_debug_fatal("Double free detected for %p", ptr);
	}

	// Set the block as freed
	memory_block->block_size = 0;
	if (__oshmem_memory_is_last_allocation(stack_info, memory_block))
	{
		do
		{
			// Move the tail of the allocator
			stack_info->top            -= stack_info->last_block_size;
			stack_info->last_block_size = memory_block->previous_block_size;

			memory_block = prev_block;
			prev_block   = ((void *)memory_block - memory_block->previous_block_size);

			// Try moving the tail as far as possible
		} while (memory_block->block_size == 0 && stack_info->top != 0);
	}
}

void mpc_openshmem_memory_symmetric_stack_free(void *stack_start, void *ptr)
{
	__oshmem_stack_info_t *stack_info = __oshmem_memory_stack_info_from_ptr(stack_start);

	__oshmem_memory_stack_check_canary(stack_info);

	mpc_common_spinlock_lock(&stack_info->lock);
	mpc_openshmem_memory_symmetric_stack_free_no_lock(stack_info, ptr);
	mpc_common_spinlock_unlock(&stack_info->lock);
}

void * mpc_openshmem_memory_symmetric_stack_resize(void *stack_start, void *ptr, const size_t new_size)
{
	__oshmem_stack_info_t *stack_info = __oshmem_memory_stack_info_from_ptr(stack_start);

	__oshmem_memory_stack_check_canary(stack_info);
	void *ret = NULL;

	// Set the block for the next address to be aligned
	const size_t aligned_size =
		((new_size % __oshmem_stack_alignment) == 0)
		? new_size + sizeof(__oshmem_allocator_memory_header_t)
		: ((new_size / __oshmem_stack_alignment) + 1) * __oshmem_stack_alignment
		+ sizeof(__oshmem_allocator_memory_header_t)
	;

	mpc_common_spinlock_lock(&stack_info->lock);
	__oshmem_allocator_memory_header_t *memory_block = __shmem_memory_header_from_ptr(ptr);

	// Check the integrity of the memory block before reading it
	__oshmem_memory_block_check_canary(memory_block);

	// The block is already large enough
	if (memory_block->block_size >= aligned_size)
	{
		mpc_common_spinlock_unlock(&stack_info->lock);
		return ptr;
	}

	// Resize if last allocation
	if (__oshmem_memory_is_last_allocation(stack_info, memory_block))
	{
		const ptrdiff_t old_offset = ptr - stack_info->bottom;
		// Not enough space to alloc the required size
		if (old_offset + aligned_size >= stack_info->size)
		{
			mpc_common_spinlock_unlock(&stack_info->lock);
			return NULL;
		}

		// Compute the address to the newly allocated block
		ret = ptr;
		memory_block->block_size = aligned_size;

		// Reserve the newly allocated block
		stack_info->last_block_size = aligned_size;
		stack_info->top             = old_offset + aligned_size;
	}
	// otherwise allocate a new block
	else
	{
		// Allocate a new block
		ret = mpc_openshmem_memory_symmetric_stack_alloc_no_lock(stack_info, aligned_size);
		if (ret == NULL)
		{
			mpc_common_spinlock_unlock(&stack_info->lock);
			return ret;
		}
		// Copy the data from the old block
		memcpy(ret, memory_block + 1, memory_block->block_size - sizeof(__oshmem_allocator_memory_header_t));
		// Free the old block
		mpc_openshmem_memory_symmetric_stack_free_no_lock(stack_info, ptr);
	}
	mpc_common_spinlock_unlock(&stack_info->lock);

	mpc_common_nodebug("Resize from %p (size: %llu) to %p (size: %llu)", ptr, memory_block->block_size, ret,
		aligned_size);

	return (void *)ret;
}

void mpc_openshmem_memory_symmetric_stack_deinit(void *start_addr)
{
	sctk_free(start_addr);
}

/** @} */ // openshmem_memory

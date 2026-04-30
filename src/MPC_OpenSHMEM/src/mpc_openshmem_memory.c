#include "mpc_openshmem_memory.h"
#include "sctk_alloc.h"
#include "stdbool.h"

/** @ingroup openshmem_memory
 * @{
 */

/** @brief Information of the symmetric stack */
static struct _mpc_openshmem_stack_info_s
{
	void *    bottom;          /**< First address of the stack */
	size_t    size;            /**< Fixed size of the whole stack */
	ptrdiff_t top;             /**< Offset to next free block in the stack */
	size_t    last_block_size; /**< Size of the last allocation */
} _mpc_openshmem_stack_info;

#if __STDC_VERSION__ < 202300L
	static const ptrdiff_t _mpc_openshmem_stack_alignment = 8;
#else
	constexpr ptrdiff_t _mpc_openshmem_stack_alignment = 8;
#endif


/** @brief Check if this address correspond to the last allocated one
 *
 * @param[in] ptr address to check
 * @retval			true		ptr is the address of the lastly allocated block
 * @retval			false		ptr is not the address of the lastly allocated block
 */
static inline bool __openshmem_memory_is_last_allocation(const void *const ptr)
{
	return (_mpc_openshmem_stack_info.bottom
	        + (_mpc_openshmem_stack_info.top
	           - _mpc_openshmem_stack_info.last_block_size)) == ptr;
}

void * mpc_openshmem_memory_symmetric_stack_init(const size_t size)
{
	_mpc_openshmem_stack_info.bottom          = NULL;
	_mpc_openshmem_stack_info.size            = size;
	_mpc_openshmem_stack_info.top             = 0;
	_mpc_openshmem_stack_info.last_block_size = 0;

	_mpc_openshmem_stack_info.bottom = sctk_malloc(size);

	return _mpc_openshmem_stack_info.bottom;
}

void * mpc_openshmem_memory_symmetric_stack_alloc(const size_t size)
{
	// Set the block for the next address to be aligned
	const size_t aligned_size =
		((size % _mpc_openshmem_stack_alignment) == 0)
		? size
		: ((size / _mpc_openshmem_stack_alignment) + 1) * _mpc_openshmem_stack_alignment;

	// Not enough space to alloc the required size
	if (_mpc_openshmem_stack_info.top + aligned_size >= _mpc_openshmem_stack_info.size)
	{
		return NULL;
	}

	// Compute the address to the newly allocated block
	void *ret = _mpc_openshmem_stack_info.bottom + _mpc_openshmem_stack_info.top;

	// Reserve the newly allocated block
	_mpc_openshmem_stack_info.last_block_size = aligned_size;
	_mpc_openshmem_stack_info.top            += aligned_size;

	return ret;
}

void * mpc_openshmem_memory_symmetric_stack_resize(const void *const ptr, const size_t new_size)
{
	const void *ret = NULL;
	// Set the block for the next address to be aligned
	const size_t aligned_size =
		((new_size % _mpc_openshmem_stack_alignment) == 0)
		? new_size
		: ((new_size / _mpc_openshmem_stack_alignment) + 1) * _mpc_openshmem_stack_alignment;

	// Resize if last allocation
	if (__openshmem_memory_is_last_allocation(ptr))
	{
		const ptrdiff_t old_offset = ptr - _mpc_openshmem_stack_info.bottom;
		// Not enough space to alloc the required size
		if (old_offset + aligned_size >= _mpc_openshmem_stack_info.size)
		{
			return NULL;
		}

		// Compute the address to the newly allocated block
		ret = ptr;

		// Reserve the newly allocated block
		_mpc_openshmem_stack_info.last_block_size = aligned_size;
		_mpc_openshmem_stack_info.top             = old_offset + aligned_size;
	}
	// otherwise allocate a new block
	else
	{
		ret = mpc_openshmem_memory_symmetric_stack_alloc(aligned_size);
	}

	return (void *)ret;
}

void mpc_openshmem_memory_symmetric_stack_free(const void *ptr)
{
	// Only deallocating the last allocation has an effect
	if (__openshmem_memory_is_last_allocation(ptr))
	{
		_mpc_openshmem_stack_info.top            -= _mpc_openshmem_stack_info.last_block_size;
		_mpc_openshmem_stack_info.last_block_size = 0;
	}
}

void mpc_openshmem_memory_symmetric_stack_deinit(void *start_addr)
{
	sctk_free(start_addr);
}

/** @} */ // openshmem_memory

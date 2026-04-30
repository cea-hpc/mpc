#include "mpc_lowcomm_one_sided.h"
#include <stddef.h>
#ifndef OSHMEM_INTERNAL_H_

	typedef struct _mpc_oshmem_context_s
	{
		mpc_lowcomm_window_t *   symmetric_heap_window;
		mpc_lowcomm_window_t *   user_bss_window;
		mpc_lowcomm_window_t *   user_data_window;

		void *                   symmetric_heap_start;
		void *                   user_bss_start;
		void *                   user_data_start;

		size_t                   symmetric_heap_size;
		size_t                   user_bss_size;
		size_t                   user_data_size;

		_Atomic volatile ssize_t refcount;
		mpc_common_spinlock_t    context_lock;
	}_mpc_oshmem_context_t;

	#define __openshmem_convert_lcp_request_to_lowcomm(__req) (((mpc_lowcomm_request_t *)__req) - 1)

#endif // !OSHMEM_INTERNAL_H_

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
 * OpenSHMEM standard routines implementation
 */

#include <limits.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <shmem.h>

#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_spinlock.h>
#include <mpc_lowcomm.h>
#include <mpc_lowcomm_communicator.h>
#include <mpc_lowcomm_datatypes.h>
#include <mpc_lowcomm_one_sided.h>
#include <sctk_alloc.h>

#include "mpc_openshmem_elf.h"
#include "mpc_openshmem_memory.h"
#include "mpc_openshmem_internal.h"

#include "mpc_openshmem_pweak.h" // Weak definition of the standard symbols

#ifdef MPC_MODULE
	#undef MPC_MODULE
#endif
#define MPC_MODULE "OpenSHMEM"

/** @brief Default size of the symmetric heap */
static const size_t DEFAULT_SYMMETRIC_SIZE = 1024 * 1024 * 1024;

// HACK: We need to handle it this way for thread-based

/** @brief Task specific contexts */
static _mpc_oshmem_context_t *__oshmem_global_contexts = NULL;
/** @brief Lock of the contexts */
static mpc_common_spinlock_t __oshmem_global_context_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
/** @brief Counter of the number of references on the contexts array */
static _Atomic volatile size_t __oshmem_global_context_refcount = 0;


/**
 * @brief Retrieves a reference of the current task specific context
 *
 * @return The current task openshmem context
 */
static inline _mpc_oshmem_context_t *__oshmem_get_context(void)
{
	const int my_local_rank = mpc_common_get_local_task_rank();

	return &__oshmem_global_contexts[my_local_rank];
}

/** @ingroup openshmem_internals
 * @brief Valid if the given address is from the .bss section
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid .bss address
 * @retval			false		address is not located in the .bss section
 */
static inline bool __shmem_is_addr_bss(const void *const address, const _mpc_oshmem_context_t *const context)
{
	return address >= context->user_bss_start
	       && address < (void *)(((char *)context->user_bss_start) + context->user_bss_size);
}

/** @ingroup openshmem_internals
 * @brief Valid if the given address is from the .data section
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid .data address
 * @retval			false		address is not located in the .data section
 */
static inline bool __shmem_is_addr_data(const void *const address, const _mpc_oshmem_context_t *const context)
{
	return address >= context->user_data_start
	       && address < (void *)(((char *)context->user_data_start) + context->user_data_size);
}

/** @ingroup openshmem_internals
 * @brief Valid if the given address is from the symmetric heap
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid symmetric heap address
 * @retval			false		address is not located in the symmetric heap
 */
static inline bool __shmem_is_addr_symmetric_heap(const void *const address, const _mpc_oshmem_context_t *const context)
{
	return address >= context->symmetric_heap_start
	       && address < (void *)(((char *)context->symmetric_heap_start) + context->symmetric_heap_size);
}

/** @ingroup openshmem_internals
 * @brief Selects the window to use for a communication based on an address
 *
 * @param[in]  address  Address to validate
 * @param[in]  context  Current OpenSHMEM context
 * @param[out] window_p Pointer to store the window to use
 *
 * @retval The start address (for offset computation) if is a valid symmetric address
 * @retval NULL if address is not a valid symmetric address
 */
static inline void * __oshmem_select_memory_region(const void *const address,
                                                   const _mpc_oshmem_context_t *const context,
                                                   mpc_lowcomm_window_t **window_p)
{
	void *ret = NULL;

	if ((__shmem_is_addr_symmetric_heap(address, context)))
	{
		*window_p = context->symmetric_heap_window;
		ret       = context->symmetric_heap_start;
	}
	else if ((__shmem_is_addr_data(address, context)))
	{
		*window_p = context->user_data_window;
		ret       = context->user_data_start;
	}
	else if ((__shmem_is_addr_bss(address, context)))
	{
		*window_p = context->user_bss_window;
		ret       = context->user_bss_start;
	}
	else
	{
#ifndef MPC_IN_PROCESS_MODE
			mpc_common_debug_fatal("Symmetric global and static variables are not supported in thread-based")
#endif
	}

	return ret;
}

#ifndef NDEBUG

/**
 * @brief Checks if the address + size requested for a communication will overflow
 *
 * @warning If the check fails, this function will make the program panic
 *
 * @param[in] address Symmetric address on which the communication will happen
 * @param[in] size    Size (in Bytes) of the data to be written
 * @param[in] context Openshmem context on which the check should be performed
 */
static inline void __oshmem_check_overflow(const void *const address, const size_t size,
                                           const _mpc_oshmem_context_t * const context)
{
	const void *const end_addr = (void *)((char *)address + size - 1);

	if ((__shmem_is_addr_symmetric_heap(end_addr, context)) || (__shmem_is_addr_data(end_addr, context))
	    || (__shmem_is_addr_bss(end_addr, context)))
	{
		return;
	}

	mpc_common_debug_fatal("Overflow detected address=%p size=%llu", address, size);
}

#else
static inline void __oshmem_check_overflow(const void *const address, const size_t size,
                                           const _mpc_oshmem_context_t * const context)
{
	UNUSED(address);
	UNUSED(size);
	UNUSED(context);
}

#endif

/**
 * @brief Waits a request than frees it
 *
 * @warning If the waiting of the request is not success, this function will panic
 *
 * @param[in] req Request to wait and free
 * @param[in] msg Error message in case the wait is not successful
 */
static inline void __oshmem_wait_request_and_free(mpc_lowcomm_request_t *req, char *msg)
{
	mpc_lowcomm_status_t status = {};

	mpc_lowcomm_wait(req, &status);

	if (status.MPC_ERROR != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal(msg);
	}

	mpc_lowcomm_request_free(req);
}

/******************
 * Communications *
 ******************/

// PUT

/**
 * @brief Helper macro to generate a typed version of the put routine
 *
 * @param[in] functype Name of the type in the function name
 * @param[in] ctype    C type of the operands
 */
#define __oshmem_put_impl(functype, ctype)                                                                             \
		void pshmem_ ## functype ## _put(ctype * dest, const ctype * src, size_t nelems, int pe)                       \
		{                                                                                                              \
			const size_t                 size    = nelems * sizeof(ctype);                                             \
			mpc_lowcomm_window_t *       win     = NULL;                                                               \
			const _mpc_oshmem_context_t *context = __oshmem_get_context();                                             \
                                                                                                                       \
			const void *local_start_addr = __oshmem_select_memory_region(dest, context, &win);                         \
                                                                                                                       \
			if (local_start_addr == NULL)                                                                              \
			{                                                                                                          \
				mpc_common_debug_fatal("Invalid destination address (not a symmetric address)");                       \
			}                                                                                                          \
                                                                                                                       \
			__oshmem_check_overflow(dest, size, context);                                                              \
                                                                                                                       \
			const intptr_t offset = (intptr_t)dest - (intptr_t)local_start_addr;                                       \
                                                                                                                       \
			mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());      \
                                                                                                                       \
			mpc_common_nodebug("put with src=%p, size=%2d, local_addr=%p, offset=%lu", src, size, local_addr, offset); \
                                                                                                                       \
			mpc_lowcomm_iput(src, size, pe, offset, win, req);                                                         \
                                                                                                                       \
			__oshmem_wait_request_and_free(req, "ERROR happened during Put");                                          \
		}

__oshmem_put_impl(float,      float)
__oshmem_put_impl(double,     double)
__oshmem_put_impl(longdouble, long double)
__oshmem_put_impl(char,       char)
__oshmem_put_impl(schar,      signed char)
__oshmem_put_impl(short,      short)
__oshmem_put_impl(int,        int)
__oshmem_put_impl(long,       long)
__oshmem_put_impl(longlong,   long long)
__oshmem_put_impl(uchar,      unsigned char)
__oshmem_put_impl(ushort,     unsigned short)
__oshmem_put_impl(uint,       unsigned int)
__oshmem_put_impl(ulong,      unsigned long)
__oshmem_put_impl(ulonglong,  unsigned long long)
__oshmem_put_impl(int8_t,     int8_t)
__oshmem_put_impl(uint8_t,    uint8_t)
__oshmem_put_impl(int16_t,    int16_t)
__oshmem_put_impl(uint16_t,   uint16_t)
__oshmem_put_impl(int32_t,    int32_t)
__oshmem_put_impl(uint32_t,   uint32_t)
__oshmem_put_impl(int64_t,    int64_t)
__oshmem_put_impl(uint64_t,   uint64_t)
__oshmem_put_impl(size_t,     size_t)
__oshmem_put_impl(ptrdiff_t,  ptrdiff_t)

#undef __oshmem_put_impl

// P

/**
 * @brief Helper macro to generate a typed version of the p routine
 *
 * @param[in] functype Name of the type in the function name
 * @param[in] ctype    C type of the operands
 */
#define __oshmem_p_impl(functype, ctype)                                                                          \
		void pshmem_ ## functype ## _p(ctype * dest, const ctype value, int pe)                                   \
		{                                                                                                         \
			const size_t                 size    = sizeof(ctype);                                                 \
			mpc_lowcomm_window_t *       win     = NULL;                                                          \
			const _mpc_oshmem_context_t *context = __oshmem_get_context();                                        \
                                                                                                                  \
			const void *local_start_addr = __oshmem_select_memory_region(dest, context, &win);                    \
                                                                                                                  \
			if (local_start_addr == NULL)                                                                         \
			{                                                                                                     \
				mpc_common_debug_fatal("Invalid destination address (not a symmetric address)");                  \
			}                                                                                                     \
                                                                                                                  \
			__oshmem_check_overflow(dest, size, context);                                                         \
                                                                                                                  \
			const intptr_t offset = (intptr_t)dest - (intptr_t)local_start_addr;                                  \
                                                                                                                  \
			mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc()); \
                                                                                                                  \
			mpc_common_nodebug("put with value=%p, size=%2d, local_addr=%p, offset=%lu\n",                        \
	&value, size, local_addr, offset);                                                                            \
                                                                                                                  \
			mpc_lowcomm_iput((void *)&value, size, pe, offset, win, req);                                         \
                                                                                                                  \
			__oshmem_wait_request_and_free(req, "ERROR happened during P");                                       \
		}

__oshmem_p_impl(float,      float)
__oshmem_p_impl(double,     double)
__oshmem_p_impl(longdouble, long double)
__oshmem_p_impl(char,       char)
__oshmem_p_impl(schar,      signed char)
__oshmem_p_impl(short,      short)
__oshmem_p_impl(int,        int)
__oshmem_p_impl(long,       long)
__oshmem_p_impl(longlong,   long long)
__oshmem_p_impl(uchar,      unsigned char)
__oshmem_p_impl(ushort,     unsigned short)
__oshmem_p_impl(uint,       unsigned int)
__oshmem_p_impl(ulong,      unsigned long)
__oshmem_p_impl(ulonglong,  unsigned long long)
__oshmem_p_impl(int8_t,     int8_t)
__oshmem_p_impl(uint8_t,    uint8_t)
__oshmem_p_impl(int16_t,    int16_t)
__oshmem_p_impl(uint16_t,   uint16_t)
__oshmem_p_impl(int32_t,    int32_t)
__oshmem_p_impl(uint32_t,   uint32_t)
__oshmem_p_impl(int64_t,    int64_t)
__oshmem_p_impl(uint64_t,   uint64_t)
__oshmem_p_impl(size_t,     size_t)
__oshmem_p_impl(ptrdiff_t,  ptrdiff_t)

#undef __oshmem_p_impl

// PUTMEM

void pshmem_putmem(void *dest, const void *src, size_t size, int pe)
{
	mpc_lowcomm_window_t *       win     = NULL;
	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	const void *local_start_addr = __oshmem_select_memory_region(dest, context, &win);

	if (local_start_addr == NULL)
	{
		mpc_common_debug_fatal("Invalid destination address (not a symmetric address)");
	}

	__oshmem_check_overflow(dest, size, context);

	const intptr_t offset = (intptr_t)dest - (intptr_t)local_start_addr;

	mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());

	mpc_common_nodebug("put with src=%p, size=%2d, local_addr=%p, offset=%lu", src, size, local_addr, offset);

	mpc_lowcomm_iput(src, size, pe, offset, win, req);
	__oshmem_wait_request_and_free(req, "ERROR happened during Put");
}

// GET

/**
 * @brief Helper macro to generate a typed version of the get routine
 *
 * @param[in] functype Name of the type in the function name
 * @param[in] ctype    C type of the operands
 */
#define __oshmem_get_impl(functype, ctype)                                                                        \
		void pshmem_ ## functype ## _get(ctype * dest, const ctype * src, size_t nelems, int pe)                  \
		{                                                                                                         \
			const size_t                 size    = nelems * sizeof(ctype);                                        \
			mpc_lowcomm_window_t *       win     = NULL;                                                          \
			const _mpc_oshmem_context_t *context = __oshmem_get_context();                                        \
                                                                                                                  \
			const void *local_start_addr = __oshmem_select_memory_region(src, context, &win);                     \
                                                                                                                  \
			if (local_start_addr == NULL)                                                                         \
			{                                                                                                     \
				mpc_common_debug_fatal("Invalid source address (not a symmetric address)");                       \
			}                                                                                                     \
                                                                                                                  \
			__oshmem_check_overflow(src, size, context);                                                          \
                                                                                                                  \
			const intptr_t offset = (intptr_t)src - (intptr_t)local_start_addr;                                   \
                                                                                                                  \
			mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc()); \
                                                                                                                  \
			mpc_common_nodebug("get with src=%p, size=%2d, local_addr=%p, offset=%lu",                            \
	src, size, dest, offset);                                                                                     \
                                                                                                                  \
			mpc_lowcomm_iget(dest, size, pe, offset, win, req);                                                   \
                                                                                                                  \
			__oshmem_wait_request_and_free(req, "ERROR happened during Get");                                     \
		}

__oshmem_get_impl(float,      float)
__oshmem_get_impl(double,     double)
__oshmem_get_impl(longdouble, long double)
__oshmem_get_impl(char,       char)
__oshmem_get_impl(schar,      signed char)
__oshmem_get_impl(short,      short)
__oshmem_get_impl(int,        int)
__oshmem_get_impl(long,       long)
__oshmem_get_impl(longlong,   long long)
__oshmem_get_impl(uchar,      unsigned char)
__oshmem_get_impl(ushort,     unsigned short)
__oshmem_get_impl(uint,       unsigned int)
__oshmem_get_impl(ulong,      unsigned long)
__oshmem_get_impl(ulonglong,  unsigned long long)
__oshmem_get_impl(int8_t,     int8_t)
__oshmem_get_impl(uint8_t,    uint8_t)
__oshmem_get_impl(int16_t,    int16_t)
__oshmem_get_impl(uint16_t,   uint16_t)
__oshmem_get_impl(int32_t,    int32_t)
__oshmem_get_impl(uint32_t,   uint32_t)
__oshmem_get_impl(int64_t,    int64_t)
__oshmem_get_impl(uint64_t,   uint64_t)
__oshmem_get_impl(size_t,     size_t)
__oshmem_get_impl(ptrdiff_t,  ptrdiff_t)

#undef __oshmem_get_impl

// G

/**
 * @brief Helper macro to generate a typed version of the G routine
 *
 * @param[in] functype Name of the type in the function name
 * @param[in] ctype    C type of the operands
 */
#define __oshmem_g_impl(functype, ctype)                                                                          \
		ctype pshmem_ ## functype ## _g(const ctype * src, int pe)                                                \
		{                                                                                                         \
			const size_t                 size = sizeof(ctype);                                                    \
			ctype                        ret;                                                                     \
			mpc_lowcomm_window_t *       win     = NULL;                                                          \
			const _mpc_oshmem_context_t *context = __oshmem_get_context();                                        \
                                                                                                                  \
			const void *local_start_addr = __oshmem_select_memory_region(src, context, &win);                     \
                                                                                                                  \
			if (local_start_addr == NULL)                                                                         \
			{                                                                                                     \
				mpc_common_debug_fatal("Invalid source address (not a symmetric address)");                       \
			}                                                                                                     \
                                                                                                                  \
			__oshmem_check_overflow(src, size, context);                                                          \
                                                                                                                  \
			const intptr_t offset = (intptr_t)src - (intptr_t)local_start_addr;                                   \
                                                                                                                  \
			mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc()); \
                                                                                                                  \
			mpc_common_nodebug("g with size=%2d, local_addr=%p, offset=%lu\n",                                    \
	size, local_addr, offset);                                                                                    \
                                                                                                                  \
			mpc_lowcomm_iget(&ret, size, pe, offset, win, req);                                                   \
                                                                                                                  \
			__oshmem_wait_request_and_free(req, "ERROR happened during G");                                       \
                                                                                                                  \
			return ret;                                                                                           \
		}

__oshmem_g_impl(float,      float)
__oshmem_g_impl(double,     double)
__oshmem_g_impl(longdouble, long double)
__oshmem_g_impl(char,       char)
__oshmem_g_impl(schar,      signed char)
__oshmem_g_impl(short,      short)
__oshmem_g_impl(int,        int)
__oshmem_g_impl(long,       long)
__oshmem_g_impl(longlong,   long long)
__oshmem_g_impl(uchar,      unsigned char)
__oshmem_g_impl(ushort,     unsigned short)
__oshmem_g_impl(uint,       unsigned int)
__oshmem_g_impl(ulong,      unsigned long)
__oshmem_g_impl(ulonglong,  unsigned long long)
__oshmem_g_impl(int8_t,     int8_t)
__oshmem_g_impl(uint8_t,    uint8_t)
__oshmem_g_impl(int16_t,    int16_t)
__oshmem_g_impl(uint16_t,   uint16_t)
__oshmem_g_impl(int32_t,    int32_t)
__oshmem_g_impl(uint32_t,   uint32_t)
__oshmem_g_impl(int64_t,    int64_t)
__oshmem_g_impl(uint64_t,   uint64_t)
__oshmem_g_impl(size_t,     size_t)
__oshmem_g_impl(ptrdiff_t,  ptrdiff_t)

#undef __oshmem_g_impl

// GETMEM

void pshmem_getmem(void *dest, const void *src, size_t size, int pe)
{
	mpc_lowcomm_window_t *       win     = NULL;
	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	const void *local_start_addr = __oshmem_select_memory_region(src, context, &win);

	if (local_start_addr == NULL)
	{
		mpc_common_debug_fatal("Invalid source address (not a symmetric address)");
	}

	__oshmem_check_overflow(src, size, context);

	const intptr_t offset = (intptr_t)src - (intptr_t)local_start_addr;

	mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());

	mpc_common_nodebug("get with src=%p, size=%2d, dest=%p, offset=%lu", src, size, dest, offset);

	mpc_lowcomm_iget(dest, size, pe, offset, win, req);

	__oshmem_wait_request_and_free(req, "ERROR happened during Get");
}

/***********
 * Atomics *
 ***********/


/**
 * @ingroup openshmem_internals
 * @brief Common logic to perform a fetch and add atomic operation
 * @note This function is non-blocking
 *
 * @param[in]  src      Remote address of the object to fetch
 * @param[out] ret      Address to store the fetched item
 * @param[in]  operand  Address containing the element to add
 * @param[in]  datatype Datatype of the object to fetch and add
 * @param[in]  pe       Unique identifier (rank) of the target
 *
 * @return              Request to wait for blocking behavior
 */
static void *_mpc_openshmem_internal_fetch_add_nb(const void *src,
                                                  void *ret, const void *const operand,
                                                  mpc_lowcomm_datatype_t datatype, int pe)
{
	mpc_lowcomm_window_t *       win     = NULL;
	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	const void *local_start_addr = __oshmem_select_memory_region(src, context, &win);

	if (local_start_addr == NULL)
	{
		mpc_common_debug_fatal("Invalid source address (not a symmetric address)");
	}

	const intptr_t offset = (intptr_t)src - (intptr_t)local_start_addr;

	mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());

	mpc_common_nodebug("fetch with src=%p, operand=%p offset=%lu", src, operand, offset);

	mpc_lowcomm_iatomic(operand, ret, NULL, datatype, pe, offset, LOWCOMM_ATOMIC_OP_FETCH_ADD, win, req);

	return req;
}

/**
 * @ingroup openshmem_internals
 * @brief Common logic to perform a compare and swap atomic operation
 * @note This function is non-blocking
 *
 * @param[in] dest     Remote address of the object to compare and swap against
 * @param[inout] cond  Buffer containing the condition at start and the fetch value once the communication is completed
 * @param[in] value    Buffer containing the object to put if the condition is fulfilled
 * @param[in] datatype Datatype of the object to compare and swap
 * @param[in] pe       Unique identifier (rank) of the target
 *
 * @return             Request to wait for blocking behavior
 */
static void * _mpc_openshmem_internal_cswap_nb(void *dest, void *cond, void *value,
                                               mpc_lowcomm_datatype_t datatype, int pe)
{
	mpc_lowcomm_window_t *       win     = NULL;
	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	const void *local_start_addr = __oshmem_select_memory_region(dest, context, &win);

	if (local_start_addr == NULL)
	{
		mpc_common_debug_fatal("Invalid source address (not a symmetric address)");
	}

	const intptr_t offset = (intptr_t)dest - (intptr_t)local_start_addr;

	mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());

	mpc_lowcomm_iatomic(value, cond, cond, datatype, pe, offset, LOWCOMM_ATOMIC_OP_CSWAP, win, req);

	return req;
}

/**
 * @ingroup openshmem_internals
 * @brief Common logic to perform a swap atomic operation
 * @note This function is non-blocking
 *
 * @param[in] dest     Remote address of the object to compare and swap against
 * @param[in] value    Buffer containing the object to put at target and containing fetched value
 * @param[in] datatype Datatype of the object to compare and swap
 * @param[in] pe       Unique identifier (rank) of the target
 *
 * @return             Request to wait for blocking behavior
 */
static void * _mpc_openshmem_internal_swap_nb(void *dest, void *value, mpc_lowcomm_datatype_t datatype, int pe)
{
	mpc_lowcomm_window_t *       win     = NULL;
	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	const void *local_start_addr = __oshmem_select_memory_region(dest, context, &win);

	if (local_start_addr == NULL)
	{
		mpc_common_debug_fatal("Invalid source address (not a symmetric address)");
	}

	const intptr_t offset = (intptr_t)dest - (intptr_t)local_start_addr;

	mpc_lowcomm_request_t *req = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());

	mpc_common_nodebug("compare and swap with value=%p, dest=%p, offset=%lu", value, dest, offset);

	mpc_lowcomm_iatomic(value, value, NULL, datatype, pe, offset, LOWCOMM_ATOMIC_OP_SWAP, win, req);

	return req;
}

// FETCH

/**
 * @brief Helper macro to generate a typed version of the atomic_fetch routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_fetch_impl(functype, ctype, lowcommtype)                               \
		ctype pshmem_ ## functype ## _atomic_fetch(const ctype * src, int pe)                  \
		{                                                                                      \
			ctype       ret;                                                                   \
			ctype       value = 0;                                                             \
			void *const req   =                                                                \
				_mpc_openshmem_internal_fetch_add_nb(src, &ret, &value, lowcommtype, pe);      \
                                                                                               \
			__oshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete"); \
                                                                                               \
			return ret;                                                                        \
		}

__oshmem_atomic_fetch_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_fetch_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_fetch_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_fetch_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_fetch_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_fetch_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_fetch_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_fetch_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_fetch_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_fetch_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_fetch_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_fetch_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_fetch_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_fetch_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_fetch_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_fetch_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_fetch_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_fetch_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_fetch_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_fetch_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_fetch_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_fetch_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_fetch_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef  __oshmem_atomic_fetch_impl

float shmem_float_fetch(const float *src, int pe)
{
	return pshmem_float_atomic_fetch(src, pe);
}

double shmem_double_fetch(const double *src, int pe)
{
	return pshmem_double_atomic_fetch(src, pe);
}

int shmem_int_fetch(const int *src, int pe)
{
	return pshmem_int_atomic_fetch(src, pe);
}

long shmem_long_fetch(const long *src, int pe)
{
	return pshmem_long_atomic_fetch(src, pe);
}

long long shmem_longlong_fetch(const long long *src, int pe)
{
	return pshmem_longlong_atomic_fetch(src, pe);
}

// SET

/**
 * @brief Helper macro to generate a typed version of the atomic_set routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_set_impl(functype, ctype, lowcommtype)                                \
		void pshmem_ ## functype ## _atomic_set(ctype * dest, ctype value, int pe)            \
		{                                                                                     \
			void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, lowcommtype, pe); \
                                                                                              \
			__oshmem_wait_request_and_free(req, "Failed while waiting for set to complete");  \
		}

__oshmem_atomic_set_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_set_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_set_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_set_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_set_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_set_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_set_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_set_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_set_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_set_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_set_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_set_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_set_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_set_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_set_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_set_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_set_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_set_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_set_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_set_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_set_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_set_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_set_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef  __oshmem_atomic_set_impl

void shmem_float_set(float *dest, float value, int pe)
{
	pshmem_float_atomic_set(dest, value, pe);
}

void shmem_double_set(double *dest, double value, int pe)
{
	pshmem_double_atomic_set(dest, value, pe);
}

void shmem_int_set(int *dest, int value, int pe)
{
	pshmem_int_atomic_set(dest, value, pe);
}

void shmem_long_set(long *dest, long value, int pe)
{
	pshmem_long_atomic_set(dest, value, pe);
}

void shmem_longlong_set(long long *dest, long long value, int pe)
{
	pshmem_longlong_atomic_set(dest, value, pe);
}

// COMPARE & SWAP

/**
 * @brief Helper macro to generate a typed version of the atomic_compare_swap routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_compare_swap_impl(functype, ctype, lowcommtype)                                  \
		ctype pshmem_ ## functype ## _atomic_compare_swap(ctype * dest, ctype cond, ctype value, int pe) \
		{                                                                                                \
			void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, lowcommtype, pe);    \
                                                                                                         \
			__oshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");    \
                                                                                                         \
			return cond;                                                                                 \
		}

__oshmem_atomic_compare_swap_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_compare_swap_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_compare_swap_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_compare_swap_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_compare_swap_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_compare_swap_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_compare_swap_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_compare_swap_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_compare_swap_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_compare_swap_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_compare_swap_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_compare_swap_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_compare_swap_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_compare_swap_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_compare_swap_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_compare_swap_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_compare_swap_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_compare_swap_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_compare_swap_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_compare_swap_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_compare_swap_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_compare_swap_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_compare_swap_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef __oshmem_atomic_compare_swap_impl

int shmem_int_cswap(int *dest, int cond, int value, int pe)
{
	return pshmem_int_atomic_compare_swap(dest, cond, value, pe);
}

long shmem_long_cswap(long *dest, long cond, long value, int pe)
{
	return pshmem_long_atomic_compare_swap(dest, cond, value, pe);
}

long long shmem_longlong_cswap(long long *dest, long long cond, long long value, int pe)
{
	return pshmem_longlong_atomic_compare_swap(dest, cond, value, pe);
}

// SWAP

/**
 * @brief Helper macro to generate a typed version of the atomic_swap routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_swap_impl(functype, ctype, lowcommtype)                               \
		ctype pshmem_ ## functype ## _atomic_swap(ctype * dest, ctype value, int pe)          \
		{                                                                                     \
			void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, lowcommtype, pe); \
                                                                                              \
			__oshmem_wait_request_and_free(req, "Failed while waiting for swap to complete"); \
                                                                                              \
			return value;                                                                     \
		}

__oshmem_atomic_swap_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_swap_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_swap_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_swap_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_swap_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_swap_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_swap_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_swap_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_swap_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_swap_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_swap_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_swap_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_swap_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_swap_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_swap_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_swap_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_swap_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_swap_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_swap_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_swap_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_swap_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_swap_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_swap_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef  __oshmem_atomic_swap_impl

float shmem_float_swap(float *dest, float value, int pe)
{
	return pshmem_float_atomic_swap(dest, value, pe);
}

double shmem_double_swap(double *dest, double value, int pe)
{
	return pshmem_double_atomic_swap(dest, value, pe);
}

int shmem_int_swap(int *dest, int value, int pe)
{
	return pshmem_int_atomic_swap(dest, value, pe);
}

long shmem_long_swap(long *dest, long value, int pe)
{
	return pshmem_long_atomic_swap(dest, value, pe);
}

long long shmem_longlong_swap(long long *dest, long long value, int pe)
{
	return pshmem_longlong_atomic_swap(dest, value, pe);
}

// FETCH ADD

/**
 * @brief Helper macro to generate a typed version of the atomic_fetch_add routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_fetch_add_impl(functype, ctype, lowcommtype)                               \
		ctype pshmem_ ## functype ## _atomic_fetch_add(ctype * dest, ctype value, int pe)          \
		{                                                                                          \
			ctype       ret;                                                                       \
			void *const req =                                                                      \
				_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, lowcommtype, pe);         \
                                                                                                   \
			__oshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete"); \
                                                                                                   \
			return ret;                                                                            \
		}

__oshmem_atomic_fetch_add_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_fetch_add_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_fetch_add_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_fetch_add_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_fetch_add_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_fetch_add_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_fetch_add_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_fetch_add_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_fetch_add_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_fetch_add_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_fetch_add_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_fetch_add_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_fetch_add_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_fetch_add_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_fetch_add_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_fetch_add_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_fetch_add_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_fetch_add_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_fetch_add_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_fetch_add_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_fetch_add_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_fetch_add_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_fetch_add_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef __oshmem_atomic_fetch_add_impl

int shmem_int_fadd(int *dest, int value, int pe)
{
	return pshmem_int_atomic_fetch_add(dest, value, pe);
}

long shmem_long_fadd(long *dest, long value, int pe)
{
	return pshmem_long_atomic_fetch_add(dest, value, pe);
}

long long shmem_longlong_fadd(long long *dest, long long value, int pe)
{
	return pshmem_longlong_atomic_fetch_add(dest, value, pe);
}

// ADD

/**
 * @brief Helper macro to generate a typed version of the atomic_add routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_add_impl(functype, ctype, lowcommtype)                               \
		void pshmem_ ## functype ## _atomic_add(ctype * dest, ctype value, int pe)           \
		{                                                                                    \
			ctype       ret;                                                                 \
			void *const req =                                                                \
				_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, lowcommtype, pe);   \
                                                                                             \
			__oshmem_wait_request_and_free(req, "Failed while waiting for add to complete"); \
		}
__oshmem_atomic_add_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_add_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_add_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_add_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_add_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_add_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_add_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_add_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_add_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_add_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_add_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_add_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_add_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_add_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_add_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_add_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_add_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_add_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_add_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_add_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_add_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_add_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_add_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef __oshmem_atomic_add_impl

void shmem_int_add(int *dest, int value, int pe)
{
	pshmem_int_atomic_add(dest, value, pe);
}

void shmem_long_add(long *dest, long value, int pe)
{
	pshmem_long_atomic_add(dest, value, pe);
}

void shmem_longlong_add(long long *dest, long long value, int pe)
{
	pshmem_longlong_atomic_add(dest, value, pe);
}

// FETCH INC

/**
 * @brief Helper macro to generate a typed version of the atomic_fetch_inc routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_fetch_inc_impl(functype, ctype, lowcommtype)                               \
		ctype pshmem_ ## functype ## _atomic_fetch_inc(ctype * dest, int pe)                       \
		{                                                                                          \
			ctype       ret;                                                                       \
			ctype       value = 1;                                                                 \
			void *const req   =                                                                    \
				_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, lowcommtype, pe);         \
                                                                                                   \
			__oshmem_wait_request_and_free(req, "Failed while waiting for fetch inc to complete"); \
                                                                                                   \
			return ret;                                                                            \
		}

__oshmem_atomic_fetch_inc_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_fetch_inc_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_fetch_inc_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_fetch_inc_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_fetch_inc_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_fetch_inc_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_fetch_inc_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_fetch_inc_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_fetch_inc_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_fetch_inc_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_fetch_inc_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_fetch_inc_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_fetch_inc_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_fetch_inc_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_fetch_inc_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_fetch_inc_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_fetch_inc_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_fetch_inc_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_fetch_inc_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_fetch_inc_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_fetch_inc_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_fetch_inc_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_fetch_inc_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef __oshmem_atomic_fetch_inc_impl

int shmem_int_finc(int *dest, int pe)
{
	return pshmem_int_atomic_fetch_inc(dest, pe);
}

long shmem_long_finc(long *dest, int pe)
{
	return pshmem_long_atomic_fetch_inc(dest, pe);
}

long long shmem_longlong_finc(long long *dest, int pe)
{
	return pshmem_longlong_atomic_fetch_inc(dest, pe);
}

// INC

/**
 * @brief Helper macro to generate a typed version of the atomic_inc routine
 *
 * @param[in] functype    Name of the type in the function name
 * @param[in] ctype       C type of the operands
 * @param[in] lowcommtype MPC Lowcomm datatype of the operands
 */
#define __oshmem_atomic_inc_impl(functype, ctype, lowcommtype)                                     \
		void pshmem_ ## functype ## _atomic_inc(ctype * dest, int pe)                              \
		{                                                                                          \
			ctype       ret;                                                                       \
			ctype       value = 1;                                                                 \
			void *const req   =                                                                    \
				_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, lowcommtype, pe);         \
                                                                                                   \
			__oshmem_wait_request_and_free(req, "Failed while waiting for fetch inc to complete"); \
		}

__oshmem_atomic_inc_impl(float,     float,              MPC_LOWCOMM_FLOAT);
__oshmem_atomic_inc_impl(double,    double,             MPC_LOWCOMM_DOUBLE);
__oshmem_atomic_inc_impl(char,      char,               MPC_LOWCOMM_CHAR);
__oshmem_atomic_inc_impl(schar,     signed char,        MPC_LOWCOMM_SIGNED_CHAR);
__oshmem_atomic_inc_impl(short,     short,              MPC_LOWCOMM_SHORT);
__oshmem_atomic_inc_impl(int,       int,                MPC_LOWCOMM_INT);
__oshmem_atomic_inc_impl(long,      long,               MPC_LOWCOMM_LONG);
__oshmem_atomic_inc_impl(longlong,  long long,          MPC_LOWCOMM_LONG_LONG);
__oshmem_atomic_inc_impl(uchar,     unsigned char,      MPC_LOWCOMM_UNSIGNED_CHAR);
__oshmem_atomic_inc_impl(ushort,    unsigned short,     MPC_LOWCOMM_UNSIGNED_SHORT);
__oshmem_atomic_inc_impl(uint,      unsigned int,       MPC_LOWCOMM_UNSIGNED);
__oshmem_atomic_inc_impl(ulong,     unsigned long,      MPC_LOWCOMM_UNSIGNED_LONG);
__oshmem_atomic_inc_impl(ulonglong, unsigned long long, MPC_LOWCOMM_UNSIGNED_LONG_LONG);
__oshmem_atomic_inc_impl(int8_t,    int8_t,             MPC_LOWCOMM_INT8_T);
__oshmem_atomic_inc_impl(uint8_t,   uint8_t,            MPC_LOWCOMM_UINT8_T);
__oshmem_atomic_inc_impl(int16_t,   int16_t,            MPC_LOWCOMM_INT16_T);
__oshmem_atomic_inc_impl(uint16_t,  uint16_t,           MPC_LOWCOMM_UINT16_T);
__oshmem_atomic_inc_impl(int32_t,   int32_t,            MPC_LOWCOMM_INT32_T);
__oshmem_atomic_inc_impl(uint32_t,  uint32_t,           MPC_LOWCOMM_UINT32_T);
__oshmem_atomic_inc_impl(int64_t,   int64_t,            MPC_LOWCOMM_INT64_T);
__oshmem_atomic_inc_impl(uint64_t,  uint64_t,           MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_inc_impl(size_t,    size_t,             MPC_LOWCOMM_UINT64_T);
__oshmem_atomic_inc_impl(ptrdiff_t, ptrdiff_t,          MPC_LOWCOMM_AINT);

#undef __oshmem_atomic_inc_impl

void shmem_int_inc(int *dest, int pe)
{
	pshmem_int_atomic_inc(dest, pe);
}

void shmem_long_inc(long *dest, int pe)
{
	pshmem_long_atomic_inc(dest, pe);
}

void shmem_longlong_inc(long long *dest, int pe)
{
	pshmem_longlong_atomic_inc(dest, pe);
}

/*************************
 * Setup, Exit and Query *
 *************************/


/**
 * @brief Parses the a string representing a size
 *
 * @warning If the string is not a valid size with a correct suffix, this function will panic
 *
 * @param[in] variable String to parse
 * @return             Size in bytes parsed
 */
static size_t __oshmem_parse_environment_symmetric_size(const char *const variable)
{
	if (variable == NULL)
	{
		return DEFAULT_SYMMETRIC_SIZE;
	}
	char *remaining_string = NULL;

	double floating_value = strtod(variable, &remaining_string);
	if (floating_value < 0.0F)
	{
		mpc_common_debug_fatal("Invalid value %s for pshmem_SYMMETRIC_SIZE (size cannot be negative)", variable);
	}

	double shift = 1.0F;
	switch (*remaining_string)
	{
	case 'k':
	case 'K':
		shift = 1024.0F;
		break;

	case 'm':
	case 'M':
		shift = 1024.0F * 1024.0F;
		break;

	case 'g':
	case 'G':
		shift = 1024.0F * 1024.0F * 1024.0F;
		break;

	case 't':
	case 'T':
		shift = 1024.0F * 1024.0F * 1024.0F * 1024.0F;
		break;

	case '\0':
		break;

	default:
		goto error;
	}

	floating_value *= shift;

	return (size_t)(floating_value) + 1ull;

error:
	mpc_common_debug_fatal("Invalid value %s for pshmem_SYMMETRIC_SIZE (invalid char %c (%0#2x))",
		variable, *remaining_string, *remaining_string);
}

void pshmem_query_initialized(int *initialized)
{
	_mpc_oshmem_context_t *my_task_context = __oshmem_get_context();

	*initialized = (int)atomic_load(&my_task_context->refcount);
}

void pshmem_info_get_version(int *major, int *minor)
{
	*major = SHMEM_MAJOR_VERSION;
	*minor = SHMEM_MINOR_VERSION;
}

void pshmem_info_get_name(char *name)
{
	strncpy(name, SHMEM_VENDOR_STRING, SHMEM_MAX_NAME_LEN);
}

int pshmem_init_thread(int requested, int *provided)
{
	UNUSED(requested); // We always provide the maximum
	int ierr;

	if (atomic_load(&__oshmem_global_context_refcount) == 0)
	{
		mpc_common_spinlock_lock(&__oshmem_global_context_lock);
		if (__oshmem_global_contexts == NULL)
		{
			const int n_local_tasks = mpc_common_get_local_task_count();

			__oshmem_global_contexts = sctk_malloc(n_local_tasks * sizeof(_mpc_oshmem_context_t));

			if (__oshmem_global_contexts == NULL)
			{
				mpc_common_debug_fatal("OpenSHMEM contexts allocation failed");
			}

			const size_t symmetric_size = __oshmem_parse_environment_symmetric_size(getenv("SHMEM_SYMMETRIC_SIZE"));

			for (int i = 0; i < n_local_tasks; i++)
			{
				__oshmem_global_contexts[i].symmetric_heap_size   = symmetric_size;
				__oshmem_global_contexts[i].symmetric_heap_window = NULL;
				mpc_common_spinlock_init(&__oshmem_global_contexts[i].context_lock, 0);
				atomic_store(&__oshmem_global_contexts[i].refcount, 0);
			}
		}
		mpc_common_spinlock_unlock(&__oshmem_global_context_lock);
	}

	atomic_fetch_add(&__oshmem_global_context_refcount, 1);

	_mpc_oshmem_context_t *my_task_context = __oshmem_get_context();

	if (atomic_load(&my_task_context->refcount) == 0)
	{
		mpc_common_spinlock_lock(&my_task_context->context_lock);
		if (my_task_context->symmetric_heap_window == NULL)
		{
			_mpc_oshmem_section_info_t section_info = {};
			_mpc_oshmem_get_elf_info(&section_info);

			my_task_context->user_bss_start = section_info.bss.address;
			my_task_context->user_bss_size  = section_info.bss.size;
			mpc_lowcomm_window_create(&my_task_context->user_bss_start, my_task_context->user_bss_size,
				MPC_COMM_WORLD, MPC_LOWCOMM_WINDOW_FLAG_CREATE, &my_task_context->user_bss_window);

			my_task_context->user_data_start = section_info.data.address;
			my_task_context->user_data_size  = section_info.data.size;
			mpc_lowcomm_window_create(&my_task_context->user_data_start, my_task_context->user_data_size,
				MPC_COMM_WORLD, MPC_LOWCOMM_WINDOW_FLAG_CREATE, &my_task_context->user_data_window);

			// Allocate and exchange symmetric heap memory information
			my_task_context->symmetric_heap_start =
				mpc_openshmem_memory_symmetric_stack_init(&my_task_context->symmetric_heap_size);

			mpc_lowcomm_window_create(&my_task_context->symmetric_heap_start, my_task_context->symmetric_heap_size,
				MPC_COMM_WORLD, MPC_LOWCOMM_WINDOW_FLAG_CREATE, &my_task_context->symmetric_heap_window);
		}
		mpc_common_spinlock_unlock(&my_task_context->context_lock);
	}

	atomic_fetch_add(&my_task_context->refcount, 1);

	if (provided != NULL)
	{
		*provided = SHMEM_THREAD_MULTIPLE;
	}
	ierr = mpc_lowcomm_barrier(mpc_lowcomm_osc_win_get_comm(my_task_context->symmetric_heap_window));
	if (ierr != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("Initialisation barrier failed");
	}

	return MPC_LOWCOMM_SUCCESS;
}

void pshmem_init(void)
{
	pshmem_init_thread(SHMEM_THREAD_SINGLE, NULL);
}

void start_pes()
{
	pshmem_init();
}

/**
 * @brief Flushes the underlying manager
 */
static void __oshmem_flush_all_windows(void)
{
	// FIXME: Works only with LCP ! We should flush all the windows...
	mpc_lowcomm_request_t *flush_request = _mpc_oshmem_convert_mpi_request_to_lowcomm(mpc_lowcomm_request_alloc());

	_mpc_oshmem_context_t *my_task_context = __oshmem_get_context();

	// HACK: The manager is shared across the windows so only one flush is required
	mpc_lowcomm_iflush(MPC_ANY_SOURCE, my_task_context->symmetric_heap_window, flush_request);

	mpc_lowcomm_wait(flush_request, NULL);

	mpc_lowcomm_request_free(flush_request);
}

/**
 * @brief Synchronizes all the processes in the communicator contained in the symmetric heap window
 */
static void __oshmem_sync_all_windows(void)
{
	__oshmem_flush_all_windows();

	_mpc_oshmem_context_t *my_task_context = __oshmem_get_context();

	mpc_lowcomm_barrier(mpc_lowcomm_osc_win_get_comm(my_task_context->symmetric_heap_window));
}

void pshmem_finalize(void)
{
	_mpc_oshmem_context_t *my_task_context = __oshmem_get_context();
	if (atomic_load(&__oshmem_global_context_refcount) > 0)
	{
		if (atomic_load(&my_task_context->refcount) > 0)
		{
			__oshmem_sync_all_windows();
			if (atomic_fetch_sub(&my_task_context->refcount, 1) == 0)
			{
				mpc_common_spinlock_lock(&my_task_context->context_lock);

				mpc_lowcomm_window_free(&my_task_context->symmetric_heap_window);
				mpc_lowcomm_window_free(&my_task_context->user_bss_window);
				mpc_lowcomm_window_free(&my_task_context->user_data_window);

				// Allocate and exchange symmetric heap memory information
				mpc_openshmem_memory_symmetric_stack_deinit(my_task_context->symmetric_heap_start);

				my_task_context->symmetric_heap_start = NULL;
				my_task_context->user_bss_start       = NULL;
				my_task_context->user_data_start      = NULL;

				my_task_context->symmetric_heap_window = NULL;
				my_task_context->user_bss_window       = NULL;
				my_task_context->user_data_window      = NULL;

				my_task_context->symmetric_heap_size = 0;
				my_task_context->user_bss_size       = 0;
				my_task_context->user_data_size      = 0;

				mpc_common_spinlock_unlock(&my_task_context->context_lock);
			}
			if (atomic_fetch_sub(&__oshmem_global_context_refcount, 1) == 0)
			{
				sctk_free(__oshmem_global_contexts);
			}
		}
		else
		{
			mpc_common_debug_warning("Finalization in an uninitialized context detected");
		}
	}
	else
	{
		mpc_common_debug_warning("Finalization in an uninitialized context detected");
	}
}

int pshmem_my_pe(void)
{
	// Should we differentiate MPI process from OpenSHMEM one ?
	return mpc_common_get_task_rank();
}

int _my_pe(void)
{
	return pshmem_my_pe();
}

int pshmem_n_pes(void)
{
	// Should we differentiate MPI process from OpenSHMEM one ?
	return mpc_common_get_task_count();
}

int _num_pes(void)
{
	return pshmem_n_pes();
}

/**********
 * Memory *
 **********/


// FIXME: Should query an allocator to manage the memory
void *pshmem_malloc(size_t size)
{
	if (size == 0)
	{
		return NULL;
	}

	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	void *ret = mpc_openshmem_memory_symmetric_stack_alloc(context->symmetric_heap_start, size);
	__oshmem_sync_all_windows();

	return ret;
}

void *shmalloc(size_t size)
{
	return pshmem_malloc(size);
}

void *pshmem_realloc(void *ptr, size_t size)
{
	const _mpc_oshmem_context_t *context = __oshmem_get_context();
	if (size == 0)
	{
		__oshmem_sync_all_windows();

		if (ptr != NULL)
		{
			mpc_openshmem_memory_symmetric_stack_free(context->symmetric_heap_start, ptr);
		}
		return NULL;
	}
	void *ret = NULL;
	if (ptr == NULL)
	{
		ret = mpc_openshmem_memory_symmetric_stack_alloc(context->symmetric_heap_start, size);
	}
	else
	{
		ret = mpc_openshmem_memory_symmetric_stack_resize(context->symmetric_heap_start, ptr, size);
	}

	__oshmem_sync_all_windows();

	return ret;
}

void *shrealloc(void *ptr, size_t size)
{
	return pshmem_realloc(ptr, size);
}

void *pshmem_calloc(size_t count, size_t size)
{
	if (count == 0 || size == 0)
	{
		return NULL;
	}
	const _mpc_oshmem_context_t *context = __oshmem_get_context();
	void *const ret = mpc_openshmem_memory_symmetric_stack_alloc(context->symmetric_heap_start, size);
	if (ret == NULL)
	{
		return NULL;
	}
	memset(ret, 0, count * size);
	__oshmem_sync_all_windows();

	return ret;
}

void pshmem_free(void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}

	__oshmem_sync_all_windows();

	const _mpc_oshmem_context_t *context = __oshmem_get_context();

	mpc_openshmem_memory_symmetric_stack_free(context->symmetric_heap_start, ptr);
}

void shfree(void *ptr)
{
	pshmem_free(ptr);
}

/*******************************
 * Collective Synchronizations *
 *******************************/
_Noreturn void pshmem_global_exit(int status)
{
	(void)fflush(stderr);
	(void)fflush(stdout);
	mpc_common_debug_abort(status);
}

void pshmem_quiet(void)
{
	__oshmem_flush_all_windows();
}

void pshmem_fence(void)
{
	// FIXME: We should find a way to use the fact that flushes are non-blocking in LowComm
	__oshmem_flush_all_windows();
}

void pshmem_barrier_all(void)
{
	__oshmem_sync_all_windows();
}

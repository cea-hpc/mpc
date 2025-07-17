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

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include <shmem.h>

#include "mpc_common_debug.h"
#include "mpc_common_rank.h"
#include <mpc_common_progress.h>
#include "mpc_lowcomm.h"
#include "lcp.h"
#include "sctk_alloc.h"

#include "mpc_openshmem_setup.h"
#include "mpc_openshmem_memory.h"

#ifdef MPC_MODULE
	#undef MPC_MODULE
#endif
#define MPC_MODULE "OpenSHMEM"

extern lcp_context_h lcp_ctx_loc;

static lcp_manager_h shmem_mngr_loc;
static lcp_task_h    shmem_task_loc;
static lcp_mem_h     shmem_symmetric_heap_loc;
static void *        shmem_symmetric_heap_start;
static size_t        shmem_symmetric_heap_size;

static lcp_ep_h * shmem_eps;
static lcp_mem_h *shmem_remote_mem;
static void **    shmem_remote_addr;

static _Atomic volatile size_t shmem_initialized_refcount = 0;
static const size_t            MAX_SYMMETRIC_SIZE         = 1024 * 1024 * 1024;

static void *     shmem_user_bss_start;
static void *     shmem_user_data_start;
static size_t     shmem_user_bss_size;
static size_t     shmem_user_data_size;
static lcp_mem_h  shmem_user_bss_loc;
static lcp_mem_h  shmem_user_data_loc;
static lcp_mem_h *shmem_remote_user_bss_mem;
static lcp_mem_h *shmem_remote_user_data_mem;
static void **    shmem_remote_user_bss_addr;
static void **    shmem_remote_user_data_addr;

/** @ingroup openshmem_internals
 * @brief Valid if the given address is from the .bss section
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid .bss address
 * @retval			false		address is not located in the .bss section
 */
static inline bool __shmem_is_addr_bss(const void *const address)
{
	return address >= shmem_user_bss_start
	       && address < (void *)(((char *)shmem_user_bss_start) + shmem_user_bss_size);
}

/** @ingroup openshmem_internals
 * @brief Valid if the given address is from the .data section
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid .data address
 * @retval			false		address is not located in the .data section
 */
static inline bool __shmem_is_addr_data(const void *const address)
{
	return address >= shmem_user_data_start
	       && address < (void *)(((char *)shmem_user_data_start) + shmem_user_data_size);
}

/** @ingroup openshmem_internals
 * @brief Valid if the given address is from the symmetric heap
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid symmetric heap address
 * @retval			false		address is not located in the symmetric heap
 */
static inline bool __shmem_is_addr_symmetric_heap(const void *const address)
{
	return address >= shmem_symmetric_heap_start
	       && address < (void *)(((char *)shmem_symmetric_heap_start) + shmem_symmetric_heap_size);
}

/** @ingroup openshmem_internals
 * @brief Valid if the given address is a valid target symmetric address
 *
 * @param[in] address Address to validate
 * @retval			true		address is a valid symmetric address
 * @retval			false		address is not a valid symmetric address
 */
static inline bool __shmem_is_addr_symmetric_valid(const void *const address)
{
	return __shmem_is_addr_bss(address)
	       || __shmem_is_addr_data(address)
	       || __shmem_is_addr_symmetric_heap(address);
}

/** @ingroup openshmem_internals
 * @brief Progress OpenSHMEM communications in a common way
 *
 * @return lcp_progress result
 */
int mpc_openshmem_progress(void)
{
	return lcp_progress(shmem_mngr_loc);
}

/** @ingroup openshmem_internals
 * @brief Progress communications until the given request is not marked in progress anymore
 *
 * @param[in] req       Request to wait for (consumed)
 * @param[in] error_msg String to print in case of failing progress
 * @return              lcp_progress result
 */
static inline int _mpc_openshmem_wait_request_and_free(void *const req, char *const error_msg)
{
	while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)
	{
		if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal(error_msg);
		}
	}

	const int ret = lcp_request_check_status(req);

	lcp_request_free(req);

	return ret;
}

/******************
 * Communications *
 ******************/

// Put
#define __shmem_put_impl(functype, ctype)                                                          \
		void shmem_ ## functype ## _put(ctype * dest, const ctype * src, size_t nelems, int pe)    \
		{                                                                                          \
			const size_t size        = nelems * sizeof(ctype);                                     \
			void *       req         = lcp_request_alloc(shmem_task_loc);                          \
			void *       remote_addr = NULL;                                                       \
			void *       local_addr  = NULL;                                                       \
			lcp_mem_h    remote_mem;                                                               \
                                                                                                   \
			assert(__shmem_is_addr_symmetric_valid(dest));                                         \
                                                                                                   \
			if ((__shmem_is_addr_bss(dest)))                                                       \
			{                                                                                      \
				remote_addr = shmem_remote_user_bss_addr[pe];                                      \
				remote_mem  = shmem_remote_user_bss_mem[pe];                                       \
				local_addr  = shmem_user_bss_start;                                                \
			}                                                                                      \
			else if ((__shmem_is_addr_data(dest)))                                                 \
			{                                                                                      \
				remote_addr = shmem_remote_user_data_addr[pe];                                     \
				remote_mem  = shmem_remote_user_data_mem[pe];                                      \
				local_addr  = shmem_user_data_start;                                               \
			}                                                                                      \
			else if ((__shmem_is_addr_symmetric_heap(dest)))                                       \
			{                                                                                      \
				remote_addr = shmem_remote_addr[pe];                                               \
				remote_mem  = shmem_remote_mem[pe];                                                \
				local_addr  = shmem_symmetric_heap_start;                                          \
			}                                                                                      \
			else                                                                                   \
			{                                                                                      \
				not_reachable();                                                                   \
			}                                                                                      \
                                                                                                   \
			lcp_request_param_t req_param =                                                        \
			{                                                                                      \
				.field_mask = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST,                     \
				.datatype   = LCP_DATATYPE_CONTIGUOUS,                                             \
				.send_cb    = MPC_LOWCOMM_SUCCESS,                                                 \
				.request    = req,                                                                 \
				.ep         = shmem_eps[pe],                                                       \
			};                                                                                     \
                                                                                                   \
			const uint64_t offset = (ptrdiff_t)dest - (ptrdiff_t)local_addr;                       \
                                                                                                   \
			mpc_common_nodebug("put with src=%p, size=%2d, local_addr=%p, offset=%lu\n",           \
	src, size, local_addr, offset);                                                                \
                                                                                                   \
			lcp_status_ptr_t status = lcp_put_nb(shmem_eps[pe], shmem_task_loc, (void *)src, size, \
	(uint64_t)((char *)remote_addr + offset), remote_mem, &req_param);                             \
			if (LCP_PTR_IS_ERR(status))                                                            \
			{                                                                                      \
				mpc_common_debug_fatal("Failed put");                                              \
			}                                                                                      \
                                                                                                   \
			while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)                       \
			{                                                                                      \
				if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)                           \
				{                                                                                  \
					mpc_common_debug_fatal("Fail in progressing put");                             \
				}                                                                                  \
			}                                                                                      \
                                                                                                   \
			lcp_request_free(req);                                                                 \
		}

__shmem_put_impl(float,      float)
__shmem_put_impl(double,     double)
__shmem_put_impl(longdouble, long double)
__shmem_put_impl(char,       char)
__shmem_put_impl(schar,      signed char)
__shmem_put_impl(short,      short)
__shmem_put_impl(int,        int)
__shmem_put_impl(long,       long)
__shmem_put_impl(longlong,   long long)
__shmem_put_impl(uchar,      unsigned char)
__shmem_put_impl(ushort,     unsigned short)
__shmem_put_impl(uint,       unsigned int)
__shmem_put_impl(ulong,      unsigned long)
__shmem_put_impl(ulonglong,  unsigned long long)
__shmem_put_impl(int8_t,     int8_t)
__shmem_put_impl(uint8_t,    uint8_t)
__shmem_put_impl(int16_t,    int16_t)
__shmem_put_impl(uint16_t,   uint16_t)
__shmem_put_impl(int32_t,    int32_t)
__shmem_put_impl(uint32_t,   uint32_t)
__shmem_put_impl(int64_t,    int64_t)
__shmem_put_impl(uint64_t,   uint64_t)
__shmem_put_impl(size_t,     size_t)
__shmem_put_impl(ptrdiff_t,  ptrdiff_t)

#undef __shmem_put_impl

// P
#define __shmem_p_impl(functype, ctype)                                                         \
		void shmem_ ## functype ## _p(ctype * dest, const ctype value, int pe)                  \
		{                                                                                       \
			const size_t size        = sizeof(ctype);                                           \
			void *       req         = lcp_request_alloc(shmem_task_loc);                       \
			void *       remote_addr = NULL;                                                    \
			void *       local_addr  = NULL;                                                    \
			lcp_mem_h    remote_mem;                                                            \
                                                                                                \
			assert(__shmem_is_addr_symmetric_valid(dest));                                      \
                                                                                                \
			if ((__shmem_is_addr_bss(dest)))                                                    \
			{                                                                                   \
				remote_addr = shmem_remote_user_bss_addr[pe];                                   \
				remote_mem  = shmem_remote_user_bss_mem[pe];                                    \
				local_addr  = shmem_user_bss_start;                                             \
			}                                                                                   \
			else if ((__shmem_is_addr_data(dest)))                                              \
			{                                                                                   \
				remote_addr = shmem_remote_user_data_addr[pe];                                  \
				remote_mem  = shmem_remote_user_data_mem[pe];                                   \
				local_addr  = shmem_user_data_start;                                            \
			}                                                                                   \
			else if ((__shmem_is_addr_symmetric_heap(dest)))                                    \
			{                                                                                   \
				remote_addr = shmem_remote_addr[pe];                                            \
				remote_mem  = shmem_remote_mem[pe];                                             \
				local_addr  = shmem_symmetric_heap_start;                                       \
			}                                                                                   \
			else                                                                                \
			{                                                                                   \
				not_reachable();                                                                \
			}                                                                                   \
                                                                                                \
			lcp_request_param_t req_param =                                                     \
			{                                                                                   \
				.field_mask = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST,                  \
				.datatype   = LCP_DATATYPE_CONTIGUOUS,                                          \
				.send_cb    = MPC_LOWCOMM_SUCCESS,                                              \
				.request    = req,                                                              \
				.ep         = shmem_eps[pe],                                                    \
			};                                                                                  \
                                                                                                \
			const uint64_t offset = (ptrdiff_t)dest - (ptrdiff_t)local_addr;                    \
                                                                                                \
			mpc_common_nodebug("put with value=%p, size=%2d, local_addr=%p, offset=%lu\n",      \
	&value, size, local_addr, offset);                                                          \
                                                                                                \
			lcp_status_ptr_t status = lcp_put_nb(shmem_eps[pe], shmem_task_loc, (void *)&value, \
	size, (uint64_t)((char *)remote_addr + offset),                                             \
	remote_mem, &req_param);                                                                    \
			if (LCP_PTR_IS_ERR(status))                                                         \
			{                                                                                   \
				mpc_common_debug_fatal("Failed p");                                             \
			}                                                                                   \
                                                                                                \
			while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)                    \
			{                                                                                   \
				if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)                        \
				{                                                                               \
					mpc_common_debug_fatal("Fail in progressing p");                            \
				}                                                                               \
			}                                                                                   \
                                                                                                \
			lcp_request_free(req);                                                              \
		}

__shmem_p_impl(float,      float)
__shmem_p_impl(double,     double)
__shmem_p_impl(longdouble, long double)
__shmem_p_impl(char,       char)
__shmem_p_impl(schar,      signed char)
__shmem_p_impl(short,      short)
__shmem_p_impl(int,        int)
__shmem_p_impl(long,       long)
__shmem_p_impl(longlong,   long long)
__shmem_p_impl(uchar,      unsigned char)
__shmem_p_impl(ushort,     unsigned short)
__shmem_p_impl(uint,       unsigned int)
__shmem_p_impl(ulong,      unsigned long)
__shmem_p_impl(ulonglong,  unsigned long long)
__shmem_p_impl(int8_t,     int8_t)
__shmem_p_impl(uint8_t,    uint8_t)
__shmem_p_impl(int16_t,    int16_t)
__shmem_p_impl(uint16_t,   uint16_t)
__shmem_p_impl(int32_t,    int32_t)
__shmem_p_impl(uint32_t,   uint32_t)
__shmem_p_impl(int64_t,    int64_t)
__shmem_p_impl(uint64_t,   uint64_t)
__shmem_p_impl(size_t,     size_t)
__shmem_p_impl(ptrdiff_t,  ptrdiff_t)

#undef __shmem_p_impl

void shmem_putmem(void *dest, const void *src, size_t size, int pe)
{
	void *    req         = lcp_request_alloc(shmem_task_loc);
	void *    remote_addr = NULL;
	void *    local_addr  = NULL;
	lcp_mem_h remote_mem;

	assert(__shmem_is_addr_symmetric_valid(dest));

	if ((__shmem_is_addr_bss(dest)))
	{
		remote_addr = shmem_remote_user_bss_addr[pe];
		remote_mem  = shmem_remote_user_bss_mem[pe];
		local_addr  = shmem_user_bss_start;
	}
	else if ((__shmem_is_addr_data(dest)))
	{
		remote_addr = shmem_remote_user_data_addr[pe];
		remote_mem  = shmem_remote_user_data_mem[pe];
		local_addr  = shmem_user_data_start;
	}
	else if ((__shmem_is_addr_symmetric_heap(dest)))
	{
		remote_addr = shmem_remote_addr[pe];
		remote_mem  = shmem_remote_mem[pe];
		local_addr  = shmem_symmetric_heap_start;
	}
	else
	{
		not_reachable();
	}

	lcp_request_param_t req_param =
	{
		.field_mask = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST,
		.datatype   = LCP_DATATYPE_CONTIGUOUS,
		.send_cb    = MPC_LOWCOMM_SUCCESS,
		.request    = req,
		.ep         = shmem_eps[pe],
	};

	const uint64_t offset = (ptrdiff_t)dest - (ptrdiff_t)local_addr;

	mpc_common_debug("put with src=%p, size=%2d, local_addr=%p, offset=%lun", src, size, local_addr, offset);

	lcp_status_ptr_t status = lcp_put_nb(shmem_eps[pe], shmem_task_loc, (void *)src, size,
		(uint64_t)((char *)remote_addr + offset), remote_mem, &req_param);
	if (LCP_PTR_IS_ERR(status))
	{
		mpc_common_debug_fatal("Failed put");
	}

	while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)
	{
		if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Fail in progressing put");
		}
	}

	lcp_request_free(req);
}

// GET
#define __shmem_get_impl(functype, ctype)                                                           \
		void shmem_ ## functype ## _get(ctype * dest, const ctype * src, size_t nelems, int pe)     \
		{                                                                                           \
			const size_t size        = nelems * sizeof(ctype);                                      \
			void *       req         = lcp_request_alloc(shmem_task_loc);                           \
			void *       remote_addr = NULL;                                                        \
			void *       local_addr  = NULL;                                                        \
			lcp_mem_h    remote_mem;                                                                \
                                                                                                    \
			assert(__shmem_is_addr_symmetric_valid(src));                                           \
                                                                                                    \
			if ((__shmem_is_addr_bss(src)))                                                         \
			{                                                                                       \
				remote_addr = shmem_remote_user_bss_addr[pe];                                       \
				remote_mem  = shmem_remote_user_bss_mem[pe];                                        \
				local_addr  = shmem_user_bss_start;                                                 \
			}                                                                                       \
			else if ((__shmem_is_addr_data(src)))                                                   \
			{                                                                                       \
				remote_addr = shmem_remote_user_data_addr[pe];                                      \
				remote_mem  = shmem_remote_user_data_mem[pe];                                       \
				local_addr  = shmem_user_data_start;                                                \
			}                                                                                       \
			else if ((__shmem_is_addr_symmetric_heap(src)))                                         \
			{                                                                                       \
				remote_addr = shmem_remote_addr[pe];                                                \
				remote_mem  = shmem_remote_mem[pe];                                                 \
				local_addr  = shmem_symmetric_heap_start;                                           \
			}                                                                                       \
			else                                                                                    \
			{                                                                                       \
				not_reachable();                                                                    \
			}                                                                                       \
                                                                                                    \
			lcp_request_param_t req_param =                                                         \
			{                                                                                       \
				.field_mask = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST,                      \
				.datatype   = LCP_DATATYPE_CONTIGUOUS,                                              \
				.send_cb    = MPC_LOWCOMM_SUCCESS,                                                  \
				.request    = req,                                                                  \
				.ep         = shmem_eps[pe],                                                        \
			};                                                                                      \
                                                                                                    \
			const uint64_t offset = (ptrdiff_t)src - (ptrdiff_t)local_addr;                         \
                                                                                                    \
			mpc_common_debug_info("get with src=%p, size=%2d, local_addr=%p, offset=%lu\n",         \
	src, size, local_addr, offset);                                                                 \
                                                                                                    \
			lcp_status_ptr_t status = lcp_get_nb(shmem_eps[pe], shmem_task_loc, (void *)dest, size, \
	(uint64_t)((char *)remote_addr + offset),                                                       \
	remote_mem, &req_param);                                                                        \
			if (LCP_PTR_IS_ERR(status))                                                             \
			{                                                                                       \
				mpc_common_debug_fatal("Failed get");                                               \
			}                                                                                       \
                                                                                                    \
			while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)                        \
			{                                                                                       \
				if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)                            \
				{                                                                                   \
					mpc_common_debug_fatal("Fail in progressing get");                              \
				}                                                                                   \
			}                                                                                       \
                                                                                                    \
			lcp_request_free(req);                                                                  \
		}

__shmem_get_impl(float,      float)
__shmem_get_impl(double,     double)
__shmem_get_impl(longdouble, long double)
__shmem_get_impl(char,       char)
__shmem_get_impl(schar,      signed char)
__shmem_get_impl(short,      short)
__shmem_get_impl(int,        int)
__shmem_get_impl(long,       long)
__shmem_get_impl(longlong,   long long)
__shmem_get_impl(uchar,      unsigned char)
__shmem_get_impl(ushort,     unsigned short)
__shmem_get_impl(uint,       unsigned int)
__shmem_get_impl(ulong,      unsigned long)
__shmem_get_impl(ulonglong,  unsigned long long)
__shmem_get_impl(int8_t,     int8_t)
__shmem_get_impl(uint8_t,    uint8_t)
__shmem_get_impl(int16_t,    int16_t)
__shmem_get_impl(uint16_t,   uint16_t)
__shmem_get_impl(int32_t,    int32_t)
__shmem_get_impl(uint32_t,   uint32_t)
__shmem_get_impl(int64_t,    int64_t)
__shmem_get_impl(uint64_t,   uint64_t)
__shmem_get_impl(size_t,     size_t)
__shmem_get_impl(ptrdiff_t,  ptrdiff_t)

#undef __shmem_get_impl

// G
#define __shmem_g_impl(functype, ctype)                                                       \
		ctype shmem_ ## functype ## _g(const ctype * src, int pe)                             \
		{                                                                                     \
			const size_t size        = sizeof(ctype);                                         \
			void *       req         = lcp_request_alloc(shmem_task_loc);                     \
			void *       remote_addr = NULL;                                                  \
			void *       local_addr  = NULL;                                                  \
			ctype        ret;                                                                 \
			lcp_mem_h    remote_mem;                                                          \
                                                                                              \
			assert(__shmem_is_addr_symmetric_valid(src));                                     \
                                                                                              \
			if ((__shmem_is_addr_bss(src)))                                                   \
			{                                                                                 \
				remote_addr = shmem_remote_user_bss_addr[pe];                                 \
				remote_mem  = shmem_remote_user_bss_mem[pe];                                  \
				local_addr  = shmem_user_bss_start;                                           \
			}                                                                                 \
			else if ((__shmem_is_addr_data(src)))                                             \
			{                                                                                 \
				remote_addr = shmem_remote_user_data_addr[pe];                                \
				remote_mem  = shmem_remote_user_data_mem[pe];                                 \
				local_addr  = shmem_user_data_start;                                          \
			}                                                                                 \
			else if ((__shmem_is_addr_symmetric_heap(src)))                                   \
			{                                                                                 \
				remote_addr = shmem_remote_addr[pe];                                          \
				remote_mem  = shmem_remote_mem[pe];                                           \
				local_addr  = shmem_symmetric_heap_start;                                     \
			}                                                                                 \
			else                                                                              \
			{                                                                                 \
				not_reachable();                                                              \
			}                                                                                 \
                                                                                              \
			lcp_request_param_t req_param =                                                   \
			{                                                                                 \
				.field_mask = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST,                \
				.datatype   = LCP_DATATYPE_CONTIGUOUS,                                        \
				.send_cb    = MPC_LOWCOMM_SUCCESS,                                            \
				.request    = req,                                                            \
				.ep         = shmem_eps[pe],                                                  \
			};                                                                                \
                                                                                              \
			const uint64_t offset = (ptrdiff_t)src - (ptrdiff_t)local_addr;                   \
                                                                                              \
			mpc_common_debug_info("g with size=%2d, local_addr=%p, offset=%lu\n",             \
	size, local_addr, offset);                                                                \
                                                                                              \
			lcp_status_ptr_t status = lcp_get_nb(shmem_eps[pe], shmem_task_loc, (void *)&ret, \
	size, (uint64_t)((char *)remote_addr + offset),                                           \
	remote_mem, &req_param);                                                                  \
			if (LCP_PTR_IS_ERR(status))                                                       \
			{                                                                                 \
				mpc_common_debug_fatal("Failed g");                                           \
			}                                                                                 \
                                                                                              \
			while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)                  \
			{                                                                                 \
				if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)                      \
				{                                                                             \
					mpc_common_debug_fatal("Fail in progressing g");                          \
				}                                                                             \
			}                                                                                 \
                                                                                              \
			lcp_request_free(req);                                                            \
			return ret;                                                                       \
		}

__shmem_g_impl(float,      float)
__shmem_g_impl(double,     double)
__shmem_g_impl(longdouble, long double)
__shmem_g_impl(char,       char)
__shmem_g_impl(schar,      signed char)
__shmem_g_impl(short,      short)
__shmem_g_impl(int,        int)
__shmem_g_impl(long,       long)
__shmem_g_impl(longlong,   long long)
__shmem_g_impl(uchar,      unsigned char)
__shmem_g_impl(ushort,     unsigned short)
__shmem_g_impl(uint,       unsigned int)
__shmem_g_impl(ulong,      unsigned long)
__shmem_g_impl(ulonglong,  unsigned long long)
__shmem_g_impl(int8_t,     int8_t)
__shmem_g_impl(uint8_t,    uint8_t)
__shmem_g_impl(int16_t,    int16_t)
__shmem_g_impl(uint16_t,   uint16_t)
__shmem_g_impl(int32_t,    int32_t)
__shmem_g_impl(uint32_t,   uint32_t)
__shmem_g_impl(int64_t,    int64_t)
__shmem_g_impl(uint64_t,   uint64_t)
__shmem_g_impl(size_t,     size_t)
__shmem_g_impl(ptrdiff_t,  ptrdiff_t)

#undef __shmem_g_impl

void shmem_getmem(void *dest, const void *src, size_t size, int pe)
{
	void *    req         = lcp_request_alloc(shmem_task_loc);
	void *    remote_addr = NULL;
	void *    local_addr  = NULL;
	lcp_mem_h remote_mem;

	assert(__shmem_is_addr_symmetric_valid(src));

	if ((__shmem_is_addr_bss(src)))
	{
		remote_addr = shmem_remote_user_bss_addr[pe];
		remote_mem  = shmem_remote_user_bss_mem[pe];
		local_addr  = shmem_user_bss_start;
	}
	else if ((__shmem_is_addr_data(src)))
	{
		remote_addr = shmem_remote_user_data_addr[pe];
		remote_mem  = shmem_remote_user_data_mem[pe];
		local_addr  = shmem_user_data_start;
	}
	else if ((__shmem_is_addr_symmetric_heap(src)))
	{
		remote_addr = shmem_remote_addr[pe];
		remote_mem  = shmem_remote_mem[pe];
		local_addr  = shmem_symmetric_heap_start;
	}
	else
	{
		not_reachable();
	}

	lcp_request_param_t req_param =
	{
		.field_mask = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST,
		.datatype   = LCP_DATATYPE_CONTIGUOUS,
		.send_cb    = MPC_LOWCOMM_SUCCESS,
		.request    = req,
		.ep         = shmem_eps[pe],
	};

	const uint64_t offset = (ptrdiff_t)src - (ptrdiff_t)local_addr;

	mpc_common_debug_info("get with src=%p, size=%2d, dest=%p, local_addr=%p, offset=%lun",
		src,
		size,
		dest,
		local_addr,
		offset);

	lcp_status_ptr_t status = lcp_get_nb(shmem_eps[pe], shmem_task_loc, dest, size,
		(uint64_t)((char *)remote_addr + offset), remote_mem, &req_param);
	if (LCP_PTR_IS_ERR(status))
	{
		mpc_common_debug_fatal("Failed get");
	}

	while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)
	{
		if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Fail in progressing get");
		}
	}

	lcp_request_free(req);
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
 * @param[in]  datatype Datatype of the object to compare and swap
 * @param[in]  pe       Unique identifier (rank) of the target
 *
 * @return              Request to wait for blocking behavior
 */
static void *_mpc_openshmem_internal_fetch_add_nb(const void *src,
                                                  void *ret, const void *const operand,
                                                  lcp_atomic_dt_t datatype, int pe)
{
	void *    req         = lcp_request_alloc(shmem_task_loc);
	void *    remote_addr = NULL;
	void *    local_addr  = NULL;
	lcp_mem_h remote_mem;

	assert(__shmem_is_addr_symmetric_valid(src));

	if ((__shmem_is_addr_bss(src)))
	{
		remote_addr = shmem_remote_user_bss_addr[pe];
		remote_mem  = shmem_remote_user_bss_mem[pe];
		local_addr  = shmem_user_bss_start;
	}
	else if ((__shmem_is_addr_data(src)))
	{
		remote_addr = shmem_remote_user_data_addr[pe];
		remote_mem  = shmem_remote_user_data_mem[pe];
		local_addr  = shmem_user_data_start;
	}
	else if ((__shmem_is_addr_symmetric_heap(src)))
	{
		remote_addr = shmem_remote_addr[pe];
		remote_mem  = shmem_remote_mem[pe];
		local_addr  = shmem_symmetric_heap_start;
		mpc_common_debug("src = %p local_addr = %p offset = %lu",
			src,
			local_addr,
			(ptrdiff_t)src - (ptrdiff_t)local_addr);
		assert(src - local_addr < shmem_symmetric_heap_size);
	}
	else
	{
		not_reachable();
	}

	lcp_request_param_t req_param =
	{
		.field_mask   = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST | LCP_REQUEST_REPLY_BUFFER,
		.datatype     = LCP_DATATYPE_CONTIGUOUS,
		.send_cb      = MPC_LOWCOMM_SUCCESS,
		.request      = req,
		.ep           = shmem_eps[pe],
		.reply_buffer = ret,
	};

	const uint64_t offset = (ptrdiff_t)src - (ptrdiff_t)local_addr;

	mpc_common_debug_info("fetch with src=%p, remote_addr=%p, offset=%lu", src, remote_addr, offset);

	lcp_status_ptr_t status = lcp_atomic_op_nb(shmem_eps[pe], shmem_task_loc, operand, datatype,
		(uint64_t)((char *)remote_addr + offset), remote_mem, LCP_ATOMIC_OP_ADD, &req_param);
	if (LCP_PTR_IS_ERR(status))
	{
		mpc_common_debug_fatal("Failed fetch");
	}

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
static void * _mpc_openshmem_internal_cswap_nb(void *dest, void *cond, void *value, lcp_atomic_dt_t datatype, int pe)
{
	void *    req         = lcp_request_alloc(shmem_task_loc);
	void *    remote_addr = NULL;
	void *    local_addr  = NULL;
	lcp_mem_h remote_mem;

	assert(__shmem_is_addr_symmetric_valid(dest));

	if ((__shmem_is_addr_bss(dest)))
	{
		remote_addr = shmem_remote_user_bss_addr[pe];
		remote_mem  = shmem_remote_user_bss_mem[pe];
		local_addr  = shmem_user_bss_start;
	}
	else if ((__shmem_is_addr_data(dest)))
	{
		remote_addr = shmem_remote_user_data_addr[pe];
		remote_mem  = shmem_remote_user_data_mem[pe];
		local_addr  = shmem_user_data_start;
	}
	else if ((__shmem_is_addr_symmetric_heap(dest)))
	{
		remote_addr = shmem_remote_addr[pe];
		remote_mem  = shmem_remote_mem[pe];
		local_addr  = shmem_symmetric_heap_start;
	}
	else
	{
		not_reachable();
	}

	lcp_request_param_t req_param =
	{
		.field_mask   = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST | LCP_REQUEST_REPLY_BUFFER,
		.datatype     = LCP_DATATYPE_CONTIGUOUS,
		.send_cb      = MPC_LOWCOMM_SUCCESS,
		.request      = req,
		.ep           = shmem_eps[pe],
		.reply_buffer = cond,
	};

	const uint64_t offset = (ptrdiff_t)dest - (ptrdiff_t)local_addr;

	mpc_common_debug_info("compare and swap with value=%d, local_addr=%p, offset=%lu", value, local_addr, offset);

	lcp_status_ptr_t status = lcp_atomic_op_nb(shmem_eps[pe], shmem_task_loc, value, datatype,
		(uint64_t)((char *)remote_addr + offset), remote_mem, LCP_ATOMIC_OP_CSWAP, &req_param);
	if (LCP_PTR_IS_ERR(status))
	{
		mpc_common_debug_fatal("Failed compare and swap");
	}

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
static void * _mpc_openshmem_internal_swap_nb(void *dest, void *value, lcp_atomic_dt_t datatype, int pe)
{
	void *    req         = lcp_request_alloc(shmem_task_loc);
	void *    remote_addr = NULL;
	void *    local_addr  = NULL;
	lcp_mem_h remote_mem;

	assert(__shmem_is_addr_symmetric_valid(dest));

	if ((__shmem_is_addr_bss(dest)))
	{
		remote_addr = shmem_remote_user_bss_addr[pe];
		remote_mem  = shmem_remote_user_bss_mem[pe];
		local_addr  = shmem_user_bss_start;
	}
	else if ((__shmem_is_addr_data(dest)))
	{
		remote_addr = shmem_remote_user_data_addr[pe];
		remote_mem  = shmem_remote_user_data_mem[pe];
		local_addr  = shmem_user_data_start;
	}
	else if ((__shmem_is_addr_symmetric_heap(dest)))
	{
		remote_addr = shmem_remote_addr[pe];
		remote_mem  = shmem_remote_mem[pe];
		local_addr  = shmem_symmetric_heap_start;
	}
	else
	{
		not_reachable();
	}

	lcp_request_param_t req_param =
	{
		.field_mask   = LCP_REQUEST_USER_EPH | LCP_REQUEST_USER_REQUEST | LCP_REQUEST_REPLY_BUFFER,
		.datatype     = LCP_DATATYPE_CONTIGUOUS,
		.send_cb      = MPC_LOWCOMM_SUCCESS,
		.request      = req,
		.ep           = shmem_eps[pe],
		.reply_buffer = value,
	};

	const uint64_t offset = (ptrdiff_t)dest - (ptrdiff_t)local_addr;

	mpc_common_debug_info("swap with value=%d, local_addr=%p, offset=%lu", value, local_addr, offset);

	lcp_status_ptr_t status = lcp_atomic_op_nb(shmem_eps[pe], shmem_task_loc, value, datatype,
		(uint64_t)((char *)remote_addr + offset), remote_mem, LCP_ATOMIC_OP_SWAP, &req_param);
	if (LCP_PTR_IS_ERR(status))
	{
		mpc_common_debug_fatal("Failed compare and swap");
	}

	return req;
}

// fetch
float shmem_float_atomic_fetch(const float *src, int pe)
{
	float       ret;
	const float null_element = 0.0f;
	void *const req          = _mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_FLOAT, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

double shmem_double_atomic_fetch(const double *src, int pe)
{
	double       ret;
	const double null_element = 0.0F;
	void *const  req          = _mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_DOUBLE,
		pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

int shmem_int_atomic_fetch(const int *src, int pe)
{
	int         ret;
	const int   null_element = 0x0;
	void *const req          = _mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

long shmem_long_atomic_fetch(const long *src,
                             int pe)
{
	int            ret;
	const long int null_element = 0x0;
	void *const    req          = _mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_INT64,
		pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

long long shmem_longlong_atomic_fetch(const long long *src, int pe)
{
	int             ret;
	const long long null_element = 0x0;
	void *const     req          = _mpc_openshmem_internal_fetch_add_nb(src,
		&ret,
		&null_element,
		LCP_ATOMIC_DT_INT64,
		pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

unsigned int shmem_uint_atomic_fetch(const unsigned int *src, int pe)
{
	unsigned int       ret;
	const unsigned int null_element = 0x0;
	void *const        req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

unsigned long shmem_ulong_atomic_fetch(const unsigned long *src, int pe)
{
	unsigned int            ret;
	const unsigned long int null_element = 0x0;
	void *const             req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

unsigned long long shmem_ulonglong_atomic_fetch(const unsigned long long *src, int pe)
{
	unsigned int ret;
	const unsigned long long int null_element = 0x0;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

int32_t shmem_int32_t_atomic_fetch(const int32_t *src, int pe)
{
	int32_t       ret;
	const int32_t null_element = 0x0;
	void *const   req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

uint32_t shmem_uint32_t_atomic_fetch(const uint32_t *src, int pe)
{
	uint32_t       ret;
	const uint32_t null_element = 0x0;
	void *const    req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

int64_t shmem_int64_t_atomic_fetch(const int64_t *src, int pe)
{
	int64_t       ret;
	const int64_t null_element = 0x0;
	void *const   req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

uint64_t shmem_uint64_t_atomic_fetch(const uint64_t *src, int pe)
{
	uint64_t       ret;
	const uint64_t null_element = 0x0;
	void *const    req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

size_t shmem_size_t_atomic_fetch(const size_t *src, int pe)
{
	size_t       ret;
	const size_t null_element = 0x0;
	void *const  req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

ptrdiff_t shmem_ptrdiff_t_atomic_fetch(const ptrdiff_t *src, int pe)
{
	ptrdiff_t       ret;
	const ptrdiff_t null_element = 0x0;
	void *const     req          =
		_mpc_openshmem_internal_fetch_add_nb(src, &ret, &null_element, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch to complete");

	return ret;
}

float shmem_float_fetch(const float *src, int pe)
{
	return shmem_float_atomic_fetch(src, pe);
}

double shmem_double_fetch(const double *src, int pe)
{
	return shmem_double_atomic_fetch(src, pe);
}

int shmem_int_fetch(const int *src, int pe)
{
	return shmem_int_atomic_fetch(src, pe);
}

long shmem_long_fetch(const long *src, int pe)
{
	return shmem_long_atomic_fetch(src, pe);
}

long long shmem_longlong_fetch(const long long *src, int pe)
{
	return shmem_longlong_atomic_fetch(src, pe);
}

// Set
void shmem_float_atomic_set(float *dest, float value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_FLOAT, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_double_atomic_set(double *dest, double value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_DOUBLE, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_int_atomic_set(int *dest, int value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_long_atomic_set(long *dest, long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_longlong_atomic_set(long long *dest, long long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_uint_atomic_set(unsigned int *dest, unsigned int value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_ulong_atomic_set(unsigned long *dest, unsigned long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_ulonglong_atomic_set(unsigned long long *dest, unsigned long long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_int32_t_atomic_set(int32_t *dest, int32_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_uint32_t_atomic_set(uint32_t *dest, uint32_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_int64_t_atomic_set(int64_t *dest, int64_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_uint64_t_atomic_set(uint64_t *dest, uint64_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_size_t_atomic_set(size_t *dest, size_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_ptrdiff_t_atomic_set(ptrdiff_t *dest, ptrdiff_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for set to complete");
}

void shmem_float_set(float *dest, float value, int pe)
{
	shmem_float_atomic_set(dest, value, pe);
}

void shmem_double_set(double *dest, double value, int pe)
{
	shmem_double_atomic_set(dest, value, pe);
}

void shmem_int_set(int *dest, int value, int pe)
{
	shmem_int_atomic_set(dest, value, pe);
}

void shmem_long_set(long *dest, long value, int pe)
{
	shmem_long_atomic_set(dest, value, pe);
}

void shmem_longlong_set(long long *dest, long long value, int pe)
{
	shmem_longlong_atomic_set(dest, value, pe);
}

// Compare & Swap
int shmem_int_atomic_compare_swap(int *dest, int cond, int value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

long shmem_long_atomic_compare_swap(long *dest, long cond, long value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

long long shmem_longlong_atomic_compare_swap(long long *dest, long long cond, long long value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

unsigned int shmem_uint_atomic_compare_swap(unsigned int *dest, unsigned int cond, unsigned int value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

unsigned long shmem_ulong_atomic_compare_swap(unsigned long *dest, unsigned long cond, unsigned long value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

unsigned long long shmem_ulonglong_atomic_compare_swap(unsigned long long *dest, unsigned long long cond,
                                                       unsigned long long value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

int32_t shmem_int32_t_atomic_compare_swap(int32_t *dest, int32_t cond, int32_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;

	not_implemented();
}

uint32_t shmem_uint32_t_atomic_compare_swap(uint32_t *dest, uint32_t cond, uint32_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

int64_t shmem_int64_t_atomic_compare_swap(int64_t *dest, int64_t cond, int64_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

uint64_t shmem_uint64_t_atomic_compare_swap(uint64_t *dest, uint64_t cond, uint64_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

size_t shmem_size_t_atomic_compare_swap(size_t *dest, size_t cond, size_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

ptrdiff_t shmem_ptrdiff_t_atomic_compare_swap(ptrdiff_t *dest, ptrdiff_t cond, ptrdiff_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_cswap_nb(dest, &cond, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for Compare Swap to complete");

	return cond;
}

int shmem_int_cswap(int *dest, int cond, int value, int pe)
{
	return shmem_int_atomic_compare_swap(dest, cond, value, pe);
}

long shmem_long_cswap(long *dest, long cond, long value, int pe)
{
	return shmem_long_atomic_compare_swap(dest, cond, value, pe);
}

long long shmem_longlong_cswap(long long *dest, long long cond, long long value, int pe)
{
	return shmem_longlong_atomic_compare_swap(dest, cond, value, pe);
}

// Swap
float shmem_float_atomic_swap(float *dest, float value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_FLOAT, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

double shmem_double_atomic_swap(double *dest, double value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_DOUBLE, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

int shmem_int_atomic_swap(int *dest, int value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

long shmem_long_atomic_swap(long *dest, long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

long long shmem_longlong_atomic_swap(long long *dest, long long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

unsigned int shmem_uint_atomic_swap(unsigned int *dest, unsigned int value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

unsigned long shmem_ulong_atomic_swap(unsigned long *dest, unsigned long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

unsigned long long shmem_ulonglong_atomic_swap(unsigned long long *dest, unsigned long long value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

int32_t shmem_int32_t_atomic_swap(int32_t *dest, int32_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

uint32_t shmem_uint32_t_atomic_swap(uint32_t *dest, uint32_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

int64_t shmem_int64_t_atomic_swap(int64_t *dest, int64_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

uint64_t shmem_uint64_t_atomic_swap(uint64_t *dest, uint64_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

size_t shmem_size_t_atomic_swap(size_t *dest, size_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

ptrdiff_t shmem_ptrdiff_t_atomic_swap(ptrdiff_t *dest, ptrdiff_t value, int pe)
{
	void *const req = _mpc_openshmem_internal_swap_nb(dest, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for swap to complete");

	return value;
}

float shmem_float_swap(float *dest, float value, int pe)
{
	return shmem_float_atomic_swap(dest, value, pe);
}

double shmem_double_swap(double *dest, double value, int pe)
{
	return shmem_double_atomic_swap(dest, value, pe);
}

int shmem_int_swap(int *dest, int value, int pe)
{
	return shmem_int_atomic_swap(dest, value, pe);
}

long shmem_long_swap(long *dest, long value, int pe)
{
	return shmem_long_atomic_swap(dest, value, pe);
}

long long shmem_longlong_swap(long long *dest, long long value, int pe)
{
	return shmem_longlong_atomic_swap(dest, value, pe);
}

// Fetch add
int shmem_int_atomic_fetch_add(int *dest, int value, int pe)
{
	int         ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

long shmem_long_atomic_fetch_add(long *dest, long value, int pe)
{
	long int    ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

long long shmem_longlong_atomic_fetch_add(long long *dest, long long value, int pe)
{
	long long int ret;
	void *const   req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

unsigned int shmem_uint_atomic_fetch_add(unsigned int *dest, unsigned int value, int pe)
{
	unsigned int ret;
	void *const  req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

unsigned long shmem_ulong_atomic_fetch_add(unsigned long *dest, unsigned long value, int pe)
{
	unsigned long int ret;
	void *const       req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

unsigned long long shmem_ulonglong_atomic_fetch_add(unsigned long long *dest, unsigned long long value, int pe)
{
	unsigned long long int ret;
	void *const            req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

int32_t shmem_int32_t_atomic_fetch_add(int32_t *dest, int32_t value, int pe)
{
	int32_t     ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

uint32_t shmem_uint32_t_atomic_fetch_add(uint32_t *dest, uint32_t value, int pe)
{
	uint32_t    ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

int64_t shmem_int64_t_atomic_fetch_add(int64_t *dest, int64_t value, int pe)
{
	int64_t     ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

uint64_t shmem_uint64_t_atomic_fetch_add(uint64_t *dest, uint64_t value, int pe)
{
	uint64_t    ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

size_t shmem_size_t_atomic_fetch_add(size_t *dest, size_t value, int pe)
{
	size_t      ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

ptrdiff_t shmem_ptrdiff_t_atomic_fetch_add(ptrdiff_t *dest, ptrdiff_t value, int pe)
{
	ptrdiff_t   ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for fetch add to complete");

	return ret;
}

int shmem_int_fadd(int *dest, int value, int pe)
{
	return shmem_int_atomic_fetch_add(dest, value, pe);
}

long shmem_long_fadd(long *dest, long value, int pe)
{
	return shmem_long_atomic_fetch_add(dest, value, pe);
}

long long shmem_longlong_fadd(long long *dest, long long value, int pe)
{
	return shmem_longlong_atomic_fetch_add(dest, value, pe);
}

// Add
void shmem_int_atomic_add(int *dest, int value, int pe)
{
	int         ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_long_atomic_add(long *dest, long value, int pe)
{
	long int    ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_longlong_atomic_add(long long *dest, long long value, int pe)
{
	long long int ret;
	void *const   req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_uint_atomic_add(unsigned int *dest, unsigned int value, int pe)
{
	unsigned int ret;
	void *const  req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_ulong_atomic_add(unsigned long *dest, unsigned long value, int pe)
{
	unsigned long int ret;
	void *const       req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_ulonglong_atomic_add(unsigned long long *dest, unsigned long long value, int pe)
{
	unsigned long long int ret;
	void *const            req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_int32_t_atomic_add(int32_t *dest, int32_t value, int pe)
{
	int32_t     ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_uint32_t_atomic_add(uint32_t *dest, uint32_t value, int pe)
{
	uint32_t    ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT32, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_int64_t_atomic_add(int64_t *dest, int64_t value, int pe)
{
	int64_t     ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_uint64_t_atomic_add(uint64_t *dest, uint64_t value, int pe)
{
	uint64_t    ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_size_t_atomic_add(size_t *dest, size_t value, int pe)
{
	size_t      ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_UINT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_ptrdiff_t_atomic_add(ptrdiff_t *dest, ptrdiff_t value, int pe)
{
	ptrdiff_t   ret;
	void *const req =
		_mpc_openshmem_internal_fetch_add_nb(dest, &ret, &value, LCP_ATOMIC_DT_INT64, pe);

	_mpc_openshmem_wait_request_and_free(req, "Failed while waiting for add to complete");
}

void shmem_int_add(int *dest, int value, int pe)
{
	shmem_int_atomic_add(dest, value, pe);
}

void shmem_long_add(long *dest, long value, int pe)
{
	shmem_long_atomic_add(dest, value, pe);
}

void shmem_longlong_add(long long *dest, long long value, int pe)
{
	shmem_longlong_atomic_add(dest, value, pe);
}

/**********
 * Memory *
 **********/


// FIXME: Should query an allocator to manage the memory
void *shmem_malloc(size_t size)
{
	if (size == 0)
	{
		return NULL;
	}
	void *    ret  = mpc_openshmem_memory_symmetric_stack_alloc(size);
	const int ierr = mpc_lowcomm_barrier(MPC_COMM_WORLD);
	mpc_common_debug("%lu bytes allocated at %p", size, ret);
	if (ierr != MPC_LOWCOMM_SUCCESS)
	{
		return NULL;
	}

	return ret;
}

void *shmalloc(size_t size)
{
	return shmem_malloc(size);
}

void *shmem_realloc(void *ptr, size_t size)
{
	if (size == 0)
	{
		const int ierr = mpc_lowcomm_barrier(MPC_COMM_WORLD);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("shmem_realloc barrier on entry failed");
		}

		if (ptr != NULL)
		{
			mpc_openshmem_memory_symmetric_stack_free(ptr);
		}
		return NULL;
	}
	void *ret = NULL;
	if (ptr == NULL)
	{
		ret = mpc_openshmem_memory_symmetric_stack_alloc(size);
	}
	else
	{
		ret = mpc_openshmem_memory_symmetric_stack_resize(ptr, size);
	}

	const int ierr = mpc_lowcomm_barrier(MPC_COMM_WORLD);
	if (ierr != MPC_LOWCOMM_SUCCESS)
	{
		return NULL;
	}

	return ret;
}

void *shrealloc(void *ptr, size_t size)
{
	return shmem_realloc(ptr, size);
}

void *shmem_calloc(size_t count, size_t size)
{
	if (count == 0 || size == 0)
	{
		return NULL;
	}
	void *const ret = mpc_openshmem_memory_symmetric_stack_alloc(size);
	if (ret == NULL)
	{
		return NULL;
	}
	memset(ret, 0, count * size);
	const int ierr = mpc_lowcomm_barrier(MPC_COMM_WORLD);
	if (ierr != MPC_LOWCOMM_SUCCESS)
	{
		return NULL;
	}

	return ret;
}

void shmem_free(void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}

	const int ierr = mpc_lowcomm_barrier(MPC_COMM_WORLD);
	if (ierr != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("shmem_free barrier on entry failed");
	}

	mpc_openshmem_memory_symmetric_stack_free(ptr);
}

void shfree(void *ptr)
{
	shmem_free(ptr);
}

/*************************
 * Setup, Exit and Query *
 *************************/


void shmem_init(void)
{
	shmem_init_thread(SHMEM_THREAD_SINGLE, NULL);
}

void start_pes()
{
	shmem_init();
}

int shmem_init_thread(int requested, int *provided)
{
	ssize_t expect_non_initialized = 0;
	int     ierr;

	if ((atomic_compare_exchange_weak(&shmem_initialized_refcount, (size_t *)&expect_non_initialized, 1)))
	{
		const int tid     = mpc_common_get_task_rank();
		const int n_tasks = mpc_common_get_task_count();

		lcp_manager_param_t mngr_param =
		{
			.field_mask    = LCP_MANAGER_ESTIMATED_EPS | LCP_MANAGER_COMM_MODEL,
			.estimated_eps = n_tasks,
			.flags         = LCP_MANAGER_OSC_MODEL,
		};

		ierr = lcp_manager_create(lcp_ctx_loc, &shmem_mngr_loc, &mngr_param);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("lcp_manager_create failed to create the main osc manager");
		}

		shmem_task_loc = lcp_context_task_get(lcp_ctx_loc, tid);

		ierr = lcp_task_associate(shmem_task_loc, shmem_mngr_loc);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("lcp_task_associate failed to associate the main task");
		}

		// Allocate and exchange symmetric heap memory information
		shmem_symmetric_heap_size  = MAX_SYMMETRIC_SIZE;
		shmem_symmetric_heap_start = mpc_openshmem_memory_symmetric_stack_init(shmem_symmetric_heap_size);
		if (shmem_symmetric_heap_start == NULL)
		{
			mpc_common_debug_fatal("sctk_malloc could not allocate memory");
		}
		lcp_mem_param_t mem_param =
		{
			.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
			.flags      = LCP_MEM_REGISTER_STATIC | LCP_MEM_REGISTER_CREATE,
			.address    = shmem_symmetric_heap_start,
			.size       = shmem_symmetric_heap_size,
		};

		ierr = lcp_mem_provision(shmem_mngr_loc, &shmem_symmetric_heap_loc, &mem_param);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal(
				"lcp_mem_provision failed to provision the main addressable memory");
		}

		shmem_remote_mem = sctk_malloc(n_tasks * sizeof(lcp_mem_h));
		if (shmem_remote_mem == NULL)
		{
			mpc_common_debug_fatal(
				"lcp_mem_provision failed to provision the main addressable memory");
		}
		shmem_remote_addr = sctk_malloc(n_tasks * sizeof(void *));
		if (shmem_remote_mem == NULL)
		{
			mpc_common_debug_fatal("sctk_malloc could not allocate memory");
		}
		void *tmp_rkey_buf = NULL;
		int   tmp_rkey_len = 0;

		ierr = lcp_mem_pack(shmem_mngr_loc, shmem_symmetric_heap_loc, &tmp_rkey_buf, &tmp_rkey_len);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Failed to pack lcp_mem");
		}

		const int total_size = tmp_rkey_len + sizeof(void *);

		void *tmp_send_buf = sctk_malloc(total_size);
		memcpy(tmp_send_buf,                  &mem_param.address, sizeof(void *));
		memcpy(tmp_send_buf + sizeof(void *), tmp_rkey_buf,       tmp_rkey_len);
		void *tmp_rkey_recv_buf = sctk_calloc(n_tasks, total_size);

		ierr = mpc_lowcomm_allgather(tmp_send_buf, tmp_rkey_recv_buf, total_size, MPC_COMM_WORLD);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Memory key exchange failed");
		}

		for (int pe = 0; pe < n_tasks; pe++)
		{
			memcpy(shmem_remote_addr + pe, (char *)tmp_rkey_recv_buf + (pe * total_size), sizeof(void *));
			ierr =
				lcp_mem_unpack(shmem_mngr_loc,
					&shmem_remote_mem[pe],
					(char *)tmp_rkey_recv_buf + (pe * total_size) + sizeof(void *),
					tmp_rkey_len);

			if (ierr != MPC_LOWCOMM_SUCCESS || shmem_remote_addr[pe] == 0)
			{
				mpc_common_debug_fatal("Memory key exchange failed");
			}
		}

		sctk_free(tmp_send_buf);
		sctk_free(tmp_rkey_recv_buf);
		lcp_mem_release_rkey_buf(tmp_rkey_buf);

		_mpc_openshmem_section_info_t section_info = {};
		_mpc_openshmem_get_elf_info(&section_info);

		shmem_user_bss_start  = section_info.bss.address;
		shmem_user_bss_size   = section_info.bss.size;
		shmem_user_data_start = section_info.data.address;
		shmem_user_data_size  = section_info.data.size;

		// Allocate and exchange .bss information
		shmem_remote_user_bss_mem = sctk_malloc(n_tasks * sizeof(lcp_mem_h));
		if (shmem_remote_user_bss_mem == NULL)
		{
			mpc_common_debug_fatal(
				"lcp_mem_provision failed to provision the .bss addressable memory");
		}
		lcp_mem_param_t mem_param_bss =
		{
			.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
			.flags      = LCP_MEM_REGISTER_STATIC | LCP_MEM_REGISTER_CREATE,
			.address    = shmem_user_bss_start,
			.size       = shmem_user_bss_size,
		};

		ierr = lcp_mem_provision(shmem_mngr_loc, &shmem_user_bss_loc, &mem_param_bss);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal(
				"lcp_mem_provision failed to provision the bss addressable memory");
		}
		shmem_remote_user_bss_addr = sctk_malloc(n_tasks * sizeof(void *));
		if (shmem_remote_user_bss_addr == NULL)
		{
			mpc_common_debug_fatal("sctk_malloc could not allocate memory");
		}
		tmp_rkey_buf = NULL;
		tmp_rkey_len = 0;

		ierr = lcp_mem_pack(shmem_mngr_loc, shmem_user_bss_loc, &tmp_rkey_buf,
			&tmp_rkey_len);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Failed to pack lcp_mem");
		}

		tmp_send_buf = sctk_malloc(total_size);
		memcpy(tmp_send_buf,                  &shmem_user_bss_start, sizeof(void *));
		memcpy(tmp_send_buf + sizeof(void *), tmp_rkey_buf,          tmp_rkey_len);
		tmp_rkey_recv_buf = sctk_calloc(n_tasks, total_size);

		ierr = mpc_lowcomm_allgather(tmp_send_buf, tmp_rkey_recv_buf, total_size, MPC_COMM_WORLD);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Memory key exchange failed");
		}

		for (int pe = 0; pe < n_tasks; pe++)
		{
			memcpy(shmem_remote_user_bss_addr + pe, (char *)tmp_rkey_recv_buf + (pe * total_size),
				sizeof(void *));
			ierr = lcp_mem_unpack(shmem_mngr_loc, &shmem_remote_user_bss_mem[pe],
				(char *)tmp_rkey_recv_buf + (pe * total_size) + sizeof(void *),
				tmp_rkey_len);

			if (ierr != MPC_LOWCOMM_SUCCESS || shmem_remote_user_bss_addr[pe] == 0)
			{
				mpc_common_debug_fatal("Memory key exchange failed");
			}
		}

		sctk_free(tmp_send_buf);
		sctk_free(tmp_rkey_recv_buf);
		lcp_mem_release_rkey_buf(tmp_rkey_buf);

		// Allocate and exchange .data information
		shmem_remote_user_data_mem = sctk_malloc(n_tasks * sizeof(lcp_mem_h));
		if (shmem_remote_user_data_mem == NULL)
		{
			mpc_common_debug_fatal(
				"lcp_mem_provision failed to provision the .data addressable memory");
		}
		lcp_mem_param_t mem_param_data =
		{
			.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
			.flags      = LCP_MEM_REGISTER_STATIC | LCP_MEM_REGISTER_CREATE,
			.address    = shmem_user_data_start,
			.size       = shmem_user_data_size,
		};

		ierr = lcp_mem_provision(shmem_mngr_loc, &shmem_user_data_loc, &mem_param_data);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal(
				"lcp_mem_provision failed to provision the data addressable memory");
		}
		shmem_remote_user_data_addr = sctk_malloc(n_tasks * sizeof(void *));
		if (shmem_remote_user_data_addr == NULL)
		{
			mpc_common_debug_fatal("sctk_malloc could not allocate memory");
		}
		tmp_rkey_buf = NULL;
		tmp_rkey_len = 0;

		ierr = lcp_mem_pack(shmem_mngr_loc, shmem_user_data_loc, &tmp_rkey_buf,
			&tmp_rkey_len);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Failed to pack lcp_mem");
		}

		tmp_send_buf = sctk_malloc(total_size);
		memcpy(tmp_send_buf,                  &shmem_user_data_start, sizeof(void *));
		memcpy(tmp_send_buf + sizeof(void *), tmp_rkey_buf,           tmp_rkey_len);
		tmp_rkey_recv_buf = sctk_calloc(n_tasks, total_size);

		ierr = mpc_lowcomm_allgather(tmp_send_buf, tmp_rkey_recv_buf, total_size, MPC_COMM_WORLD);
		if (ierr != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Memory key exchange failed");
		}

		for (int pe = 0; pe < n_tasks; pe++)
		{
			memcpy(shmem_remote_user_data_addr + pe, (char *)tmp_rkey_recv_buf + (pe * total_size),
				sizeof(void *));
			ierr = lcp_mem_unpack(shmem_mngr_loc, &shmem_remote_user_data_mem[pe],
				(char *)tmp_rkey_recv_buf + (pe * total_size) + sizeof(void *),
				tmp_rkey_len);

			if (ierr != MPC_LOWCOMM_SUCCESS || shmem_remote_user_data_addr[pe] == 0)
			{
				mpc_common_debug_fatal("Memory key exchange failed");
			}
		}

		sctk_free(tmp_send_buf);
		sctk_free(tmp_rkey_recv_buf);
		lcp_mem_release_rkey_buf(tmp_rkey_buf);

		shmem_eps = sctk_malloc(n_tasks * sizeof(lcp_ep_h));
		if (shmem_eps == NULL)
		{
			mpc_common_debug_fatal("sctk_malloc could not allocate memory");
		}

		for (int i = 0; i < n_tasks; i++)
		{
			const mpc_lowcomm_peer_uid_t target_uid =
				mpc_lowcomm_communicator_uid(MPC_COMM_WORLD, i);
			ierr = lcp_ep_create(shmem_mngr_loc, &shmem_eps[i], target_uid, 0);
			if (ierr != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_fatal("Failed to create endpoint");
			}
		}
	}
	else
	{
		if (atomic_fetch_add(&shmem_initialized_refcount, 1) == 0)
		{
			not_reachable();
		}
	}

	mpc_common_progress_register(mpc_openshmem_progress);

	if (provided != NULL)
	{
		*provided = requested;
	}
	ierr = mpc_lowcomm_barrier(MPC_COMM_WORLD);
	if (ierr != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("Initialisation barrier failed");
	}

	return MPC_LOWCOMM_SUCCESS;
}

void shmem_finalize(void)
{
	mpc_common_debug_info("shmem_finalize called");
	void *req = lcp_request_alloc(shmem_task_loc);
	lcp_request_param_t req_param =
	{
		.field_mask = LCP_REQUEST_USER_MEMH | LCP_REQUEST_USER_REQUEST,
		.request    = req,
		.mem        = shmem_symmetric_heap_loc,
	};

	lcp_flush_nb(shmem_mngr_loc, shmem_task_loc, &req_param);
	while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)
	{
		if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Failed to flush before barrier");
		}
	}
	mpc_lowcomm_barrier(MPC_COMM_WORLD);

	lcp_request_free(req);
	if (atomic_fetch_sub(&shmem_initialized_refcount, 1) == 0)
	{
		lcp_mem_deprovision(shmem_mngr_loc, shmem_symmetric_heap_loc);
		mpc_openshmem_memory_symmetric_stack_deinit();

		lcp_task_dissociate(shmem_task_loc, shmem_mngr_loc);
		shmem_task_loc = NULL;

		sctk_free(shmem_eps);
		sctk_free(shmem_remote_mem);
		sctk_free(shmem_remote_addr);
		sctk_free(shmem_remote_user_bss_mem);
		sctk_free(shmem_remote_user_data_mem);
		sctk_free(shmem_remote_user_bss_addr);
		sctk_free(shmem_remote_user_data_addr);

		lcp_manager_fini(shmem_mngr_loc);
	}
	else
	{
		if (atomic_fetch_sub(&shmem_initialized_refcount, 1) == -1)
		{
			not_reachable();
		}
	}
}

int shmem_my_pe(void)
{
	// Should we differentiate MPI process from OpenSHMEM one ?
	return mpc_common_get_task_rank();
}

int _my_pe(void)
{
	return shmem_my_pe();
}

int shmem_n_pes(void)
{
	// Should we differentiate MPI process from OpenSHMEM one ?
	return mpc_common_get_task_count();
}

int _num_pes(void)
{
	return shmem_n_pes();
}

/*******************************
 * Collective Synchronizations *
 *******************************/
_Noreturn void shmem_global_exit(int status)
{
	(void)fflush(stderr);
	(void)fflush(stdout);
	mpc_common_debug_abort(status);
}

void shmem_quiet(void)
{
	void *req = lcp_request_alloc(shmem_task_loc);
	lcp_request_param_t req_param =
	{
		.field_mask = LCP_REQUEST_USER_MEMH | LCP_REQUEST_USER_REQUEST,
		.request    = req,
		.mem        = shmem_symmetric_heap_loc,
	};

	lcp_flush_nb(shmem_mngr_loc, shmem_task_loc, &req_param);
	while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)
	{
		if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Failed to flush");
		}
	}
}

void shmem_barrier_all(void)
{
	mpc_common_debug_info("shmem_barrier_all called");
	void *req = lcp_request_alloc(shmem_task_loc);
	lcp_request_param_t req_param =
	{
		.field_mask = LCP_REQUEST_USER_MEMH | LCP_REQUEST_USER_REQUEST,
		.request    = req,
		.mem        = shmem_symmetric_heap_loc,
	};

	lcp_flush_nb(shmem_mngr_loc, shmem_task_loc, &req_param);
	while (lcp_request_check_status(req) == MPC_LOWCOMM_IN_PROGRESS)
	{
		if (lcp_progress(shmem_mngr_loc) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Failed to flush before barrier");
		}
	}
	mpc_lowcomm_barrier(MPC_COMM_WORLD);

	lcp_request_free(req);
}

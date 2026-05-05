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

#ifndef MPC_OPENSHMEM_PROFILING_WEAK_SYMBOLS_H_
#define MPC_OPENSHMEM_PROFILING_WEAK_SYMBOLS_H_

#include <pshmem.h>

#pragma weak shmem_init = pshmem_init
#pragma weak shmem_my_pe = pshmem_my_pe
#pragma weak shmem_n_pes = pshmem_n_pes
#pragma weak shmem_finalize = pshmem_finalize
#pragma weak shmem_global_exit = pshmem_global_exit
#pragma weak shmem_init_thread = pshmem_init_thread

#pragma weak shmem_malloc = pshmem_malloc
#pragma weak shmem_free = pshmem_free
#pragma weak shmem_realloc = pshmem_realloc
#pragma weak shmem_calloc = pshmem_calloc

#pragma weak shmem_float_put = pshmem_float_put
#pragma weak shmem_double_put = pshmem_double_put
#pragma weak shmem_longdouble_put = pshmem_longdouble_put
#pragma weak shmem_char_put = pshmem_char_put
#pragma weak shmem_schar_put = pshmem_schar_put
#pragma weak shmem_short_put = pshmem_short_put
#pragma weak shmem_int_put = pshmem_int_put
#pragma weak shmem_long_put = pshmem_long_put
#pragma weak shmem_longlong_put = pshmem_longlong_put
#pragma weak shmem_uchar_put = pshmem_uchar_put
#pragma weak shmem_ushort_put = pshmem_ushort_put
#pragma weak shmem_uint_put = pshmem_uint_put
#pragma weak shmem_ulong_put = pshmem_ulong_put
#pragma weak shmem_ulonglong_put = pshmem_ulonglong_put
#pragma weak shmem_int8_t_put = pshmem_int8_t_put
#pragma weak shmem_uint8_t_put = pshmem_uint8_t_put
#pragma weak shmem_int16_t_put = pshmem_int16_t_put
#pragma weak shmem_uint16_t_put = pshmem_uint16_t_put
#pragma weak shmem_int32_t_put = pshmem_int32_t_put
#pragma weak shmem_uint32_t_put = pshmem_uint32_t_put
#pragma weak shmem_int64_t_put = pshmem_int64_t_put
#pragma weak shmem_uint64_t_put = pshmem_uint64_t_put
#pragma weak shmem_size_t_put = pshmem_size_t_put
#pragma weak shmem_ptrdiff_t_put = pshmem_ptrdiff_t_put

#pragma weak shmem_putmem = pshmem_putmem

#pragma weak shmem_float_p = pshmem_float_p
#pragma weak shmem_double_p = pshmem_double_p
#pragma weak shmem_longdouble_p = pshmem_longdouble_p
#pragma weak shmem_char_p = pshmem_char_p
#pragma weak shmem_schar_p = pshmem_schar_p
#pragma weak shmem_short_p = pshmem_short_p
#pragma weak shmem_int_p = pshmem_int_p
#pragma weak shmem_long_p = pshmem_long_p
#pragma weak shmem_longlong_p = pshmem_longlong_p
#pragma weak shmem_uchar_p = pshmem_uchar_p
#pragma weak shmem_ushort_p = pshmem_ushort_p
#pragma weak shmem_uint_p = pshmem_uint_p
#pragma weak shmem_ulong_p = pshmem_ulong_p
#pragma weak shmem_ulonglong_p = pshmem_ulonglong_p
#pragma weak shmem_int8_t_p = pshmem_int8_t_p
#pragma weak shmem_uint8_t_p = pshmem_uint8_t_p
#pragma weak shmem_int16_t_p = pshmem_int16_t_p
#pragma weak shmem_uint16_t_p = pshmem_uint16_t_p
#pragma weak shmem_int32_t_p = pshmem_int32_t_p
#pragma weak shmem_uint32_t_p = pshmem_uint32_t_p
#pragma weak shmem_int64_t_p = pshmem_int64_t_p
#pragma weak shmem_uint64_t_p = pshmem_uint64_t_p
#pragma weak shmem_size_t_p = pshmem_size_t_p
#pragma weak shmem_ptrdiff_t_p = pshmem_ptrdiff_t_p

#pragma weak shmem_float_get = pshmem_float_get
#pragma weak shmem_double_get = pshmem_double_get
#pragma weak shmem_longdouble_get = pshmem_longdouble_get
#pragma weak shmem_char_get = pshmem_char_get
#pragma weak shmem_schar_get = pshmem_schar_get
#pragma weak shmem_short_get = pshmem_short_get
#pragma weak shmem_int_get = pshmem_int_get
#pragma weak shmem_long_get = pshmem_long_get
#pragma weak shmem_longlong_get = pshmem_longlong_get
#pragma weak shmem_uchar_get = pshmem_uchar_get
#pragma weak shmem_ushort_get = pshmem_ushort_get
#pragma weak shmem_uint_get = pshmem_uint_get
#pragma weak shmem_ulong_get = pshmem_ulong_get
#pragma weak shmem_ulonglong_get = pshmem_ulonglong_get
#pragma weak shmem_int8_t_get = pshmem_int8_t_get
#pragma weak shmem_uint8_t_get = pshmem_uint8_t_get
#pragma weak shmem_int16_t_get = pshmem_int16_t_get
#pragma weak shmem_uint16_t_get = pshmem_uint16_t_get
#pragma weak shmem_int32_t_get = pshmem_int32_t_get
#pragma weak shmem_uint32_t_get = pshmem_uint32_t_get
#pragma weak shmem_int64_t_get = pshmem_int64_t_get
#pragma weak shmem_uint64_t_get = pshmem_uint64_t_get
#pragma weak shmem_size_t_get = pshmem_size_t_get
#pragma weak shmem_ptrdiff_t_get = pshmem_ptrdiff_t_get

#pragma weak shmem_getmem = pshmem_getmem

#pragma weak shmem_float_g = pshmem_float_g
#pragma weak shmem_double_g = pshmem_double_g
#pragma weak shmem_longdouble_g = pshmem_longdouble_g
#pragma weak shmem_char_g = pshmem_char_g
#pragma weak shmem_schar_g = pshmem_schar_g
#pragma weak shmem_short_g = pshmem_short_g
#pragma weak shmem_int_g = pshmem_int_g
#pragma weak shmem_long_g = pshmem_long_g
#pragma weak shmem_longlong_g = pshmem_longlong_g
#pragma weak shmem_uchar_g = pshmem_uchar_g
#pragma weak shmem_ushort_g = pshmem_ushort_g
#pragma weak shmem_uint_g = pshmem_uint_g
#pragma weak shmem_ulong_g = pshmem_ulong_g
#pragma weak shmem_ulonglong_g = pshmem_ulonglong_g
#pragma weak shmem_int8_t_g = pshmem_int8_t_g
#pragma weak shmem_uint8_t_g = pshmem_uint8_t_g
#pragma weak shmem_int16_t_g = pshmem_int16_t_g
#pragma weak shmem_uint16_t_g = pshmem_uint16_t_g
#pragma weak shmem_int32_t_g = pshmem_int32_t_g
#pragma weak shmem_uint32_t_g = pshmem_uint32_t_g
#pragma weak shmem_int64_t_g = pshmem_int64_t_g
#pragma weak shmem_uint64_t_g = pshmem_uint64_t_g
#pragma weak shmem_size_t_g = pshmem_size_t_g
#pragma weak shmem_ptrdiff_t_g = pshmem_ptrdiff_t_g

#pragma weak shmem_float_atomic_fetch = pshmem_float_atomic_fetch
#pragma weak shmem_double_atomic_fetch = pshmem_double_atomic_fetch
#pragma weak shmem_char_atomic_fetch = pshmem_char_atomic_fetch
#pragma weak shmem_schar_atomic_fetch = pshmem_schar_atomic_fetch
#pragma weak shmem_short_atomic_fetch = pshmem_short_atomic_fetch
#pragma weak shmem_int_atomic_fetch = pshmem_int_atomic_fetch
#pragma weak shmem_long_atomic_fetch = pshmem_long_atomic_fetch
#pragma weak shmem_longlong_atomic_fetch = pshmem_longlong_atomic_fetch
#pragma weak shmem_uchar_atomic_fetch = pshmem_uchar_atomic_fetch
#pragma weak shmem_ushort_atomic_fetch = pshmem_ushort_atomic_fetch
#pragma weak shmem_uint_atomic_fetch = pshmem_uint_atomic_fetch
#pragma weak shmem_ulong_atomic_fetch = pshmem_ulong_atomic_fetch
#pragma weak shmem_ulonglong_atomic_fetch = pshmem_ulonglong_atomic_fetch
#pragma weak shmem_int8_t_atomic_fetch = pshmem_int8_t_atomic_fetch
#pragma weak shmem_uint8_t_atomic_fetch = pshmem_uint8_t_atomic_fetch
#pragma weak shmem_int16_t_atomic_fetch = pshmem_int16_t_atomic_fetch
#pragma weak shmem_uint16_t_atomic_fetch = pshmem_uint16_t_atomic_fetch
#pragma weak shmem_int32_t_atomic_fetch = pshmem_int32_t_atomic_fetch
#pragma weak shmem_uint32_t_atomic_fetch = pshmem_uint32_t_atomic_fetch
#pragma weak shmem_int64_t_atomic_fetch = pshmem_int64_t_atomic_fetch
#pragma weak shmem_uint64_t_atomic_fetch = pshmem_uint64_t_atomic_fetch
#pragma weak shmem_size_t_atomic_fetch = pshmem_size_t_atomic_fetch
#pragma weak shmem_ptrdiff_t_atomic_fetch = pshmem_ptrdiff_t_atomic_fetch

#pragma weak shmem_float_atomic_set = pshmem_float_atomic_set
#pragma weak shmem_double_atomic_set = pshmem_double_atomic_set
#pragma weak shmem_char_atomic_set = pshmem_char_atomic_set
#pragma weak shmem_schar_atomic_set = pshmem_schar_atomic_set
#pragma weak shmem_short_atomic_set = pshmem_short_atomic_set
#pragma weak shmem_int_atomic_set = pshmem_int_atomic_set
#pragma weak shmem_long_atomic_set = pshmem_long_atomic_set
#pragma weak shmem_longlong_atomic_set = pshmem_longlong_atomic_set
#pragma weak shmem_uchar_atomic_set = pshmem_uchar_atomic_set
#pragma weak shmem_ushort_atomic_set = pshmem_ushort_atomic_set
#pragma weak shmem_uint_atomic_set = pshmem_uint_atomic_set
#pragma weak shmem_ulong_atomic_set = pshmem_ulong_atomic_set
#pragma weak shmem_ulonglong_atomic_set = pshmem_ulonglong_atomic_set
#pragma weak shmem_int8_t_atomic_set = pshmem_int8_t_atomic_set
#pragma weak shmem_uint8_t_atomic_set = pshmem_uint8_t_atomic_set
#pragma weak shmem_int16_t_atomic_set = pshmem_int16_t_atomic_set
#pragma weak shmem_uint16_t_atomic_set = pshmem_uint16_t_atomic_set
#pragma weak shmem_int32_t_atomic_set = pshmem_int32_t_atomic_set
#pragma weak shmem_uint32_t_atomic_set = pshmem_uint32_t_atomic_set
#pragma weak shmem_int64_t_atomic_set = pshmem_int64_t_atomic_set
#pragma weak shmem_uint64_t_atomic_set = pshmem_uint64_t_atomic_set
#pragma weak shmem_size_t_atomic_set = pshmem_size_t_atomic_set
#pragma weak shmem_ptrdiff_t_atomic_set = pshmem_ptrdiff_t_atomic_set

#pragma weak shmem_float_atomic_compare_swap = pshmem_float_atomic_compare_swap
#pragma weak shmem_double_atomic_compare_swap = pshmem_double_atomic_compare_swap
#pragma weak shmem_char_atomic_compare_swap = pshmem_char_atomic_compare_swap
#pragma weak shmem_schar_atomic_compare_swap = pshmem_schar_atomic_compare_swap
#pragma weak shmem_short_atomic_compare_swap = pshmem_short_atomic_compare_swap
#pragma weak shmem_int_atomic_compare_swap = pshmem_int_atomic_compare_swap
#pragma weak shmem_long_atomic_compare_swap = pshmem_long_atomic_compare_swap
#pragma weak shmem_longlong_atomic_compare_swap = pshmem_longlong_atomic_compare_swap
#pragma weak shmem_uchar_atomic_compare_swap = pshmem_uchar_atomic_compare_swap
#pragma weak shmem_ushort_atomic_compare_swap = pshmem_ushort_atomic_compare_swap
#pragma weak shmem_uint_atomic_compare_swap = pshmem_uint_atomic_compare_swap
#pragma weak shmem_ulong_atomic_compare_swap = pshmem_ulong_atomic_compare_swap
#pragma weak shmem_ulonglong_atomic_compare_swap = pshmem_ulonglong_atomic_compare_swap
#pragma weak shmem_int8_t_atomic_compare_swap = pshmem_int8_t_atomic_compare_swap
#pragma weak shmem_uint8_t_atomic_compare_swap = pshmem_uint8_t_atomic_compare_swap
#pragma weak shmem_int16_t_atomic_compare_swap = pshmem_int16_t_atomic_compare_swap
#pragma weak shmem_uint16_t_atomic_compare_swap = pshmem_uint16_t_atomic_compare_swap
#pragma weak shmem_int32_t_atomic_compare_swap = pshmem_int32_t_atomic_compare_swap
#pragma weak shmem_uint32_t_atomic_compare_swap = pshmem_uint32_t_atomic_compare_swap
#pragma weak shmem_int64_t_atomic_compare_swap = pshmem_int64_t_atomic_compare_swap
#pragma weak shmem_uint64_t_atomic_compare_swap = pshmem_uint64_t_atomic_compare_swap
#pragma weak shmem_size_t_atomic_compare_swap = pshmem_size_t_atomic_compare_swap
#pragma weak shmem_ptrdiff_t_atomic_compare_swap = pshmem_ptrdiff_t_atomic_compare_swap

#pragma weak shmem_float_atomic_swap = pshmem_float_atomic_swap
#pragma weak shmem_double_atomic_swap = pshmem_double_atomic_swap
#pragma weak shmem_char_atomic_swap = pshmem_char_atomic_swap
#pragma weak shmem_schar_atomic_swap = pshmem_schar_atomic_swap
#pragma weak shmem_short_atomic_swap = pshmem_short_atomic_swap
#pragma weak shmem_int_atomic_swap = pshmem_int_atomic_swap
#pragma weak shmem_long_atomic_swap = pshmem_long_atomic_swap
#pragma weak shmem_longlong_atomic_swap = pshmem_longlong_atomic_swap
#pragma weak shmem_uchar_atomic_swap = pshmem_uchar_atomic_swap
#pragma weak shmem_ushort_atomic_swap = pshmem_ushort_atomic_swap
#pragma weak shmem_uint_atomic_swap = pshmem_uint_atomic_swap
#pragma weak shmem_ulong_atomic_swap = pshmem_ulong_atomic_swap
#pragma weak shmem_ulonglong_atomic_swap = pshmem_ulonglong_atomic_swap
#pragma weak shmem_int8_t_atomic_swap = pshmem_int8_t_atomic_swap
#pragma weak shmem_uint8_t_atomic_swap = pshmem_uint8_t_atomic_swap
#pragma weak shmem_int16_t_atomic_swap = pshmem_int16_t_atomic_swap
#pragma weak shmem_uint16_t_atomic_swap = pshmem_uint16_t_atomic_swap
#pragma weak shmem_int32_t_atomic_swap = pshmem_int32_t_atomic_swap
#pragma weak shmem_uint32_t_atomic_swap = pshmem_uint32_t_atomic_swap
#pragma weak shmem_int64_t_atomic_swap = pshmem_int64_t_atomic_swap
#pragma weak shmem_uint64_t_atomic_swap = pshmem_uint64_t_atomic_swap
#pragma weak shmem_size_t_atomic_swap = pshmem_size_t_atomic_swap
#pragma weak shmem_ptrdiff_t_atomic_swap = pshmem_ptrdiff_t_atomic_swap

#pragma weak shmem_float_atomic_fetch_add = pshmem_float_atomic_fetch_add
#pragma weak shmem_double_atomic_fetch_add = pshmem_double_atomic_fetch_add
#pragma weak shmem_char_atomic_fetch_add = pshmem_char_atomic_fetch_add
#pragma weak shmem_schar_atomic_fetch_add = pshmem_schar_atomic_fetch_add
#pragma weak shmem_short_atomic_fetch_add = pshmem_short_atomic_fetch_add
#pragma weak shmem_int_atomic_fetch_add = pshmem_int_atomic_fetch_add
#pragma weak shmem_long_atomic_fetch_add = pshmem_long_atomic_fetch_add
#pragma weak shmem_longlong_atomic_fetch_add = pshmem_longlong_atomic_fetch_add
#pragma weak shmem_uchar_atomic_fetch_add = pshmem_uchar_atomic_fetch_add
#pragma weak shmem_ushort_atomic_fetch_add = pshmem_ushort_atomic_fetch_add
#pragma weak shmem_uint_atomic_fetch_add = pshmem_uint_atomic_fetch_add
#pragma weak shmem_ulong_atomic_fetch_add = pshmem_ulong_atomic_fetch_add
#pragma weak shmem_ulonglong_atomic_fetch_add = pshmem_ulonglong_atomic_fetch_add
#pragma weak shmem_int8_t_atomic_fetch_add = pshmem_int8_t_atomic_fetch_add
#pragma weak shmem_uint8_t_atomic_fetch_add = pshmem_uint8_t_atomic_fetch_add
#pragma weak shmem_int16_t_atomic_fetch_add = pshmem_int16_t_atomic_fetch_add
#pragma weak shmem_uint16_t_atomic_fetch_add = pshmem_uint16_t_atomic_fetch_add
#pragma weak shmem_int32_t_atomic_fetch_add = pshmem_int32_t_atomic_fetch_add
#pragma weak shmem_uint32_t_atomic_fetch_add = pshmem_uint32_t_atomic_fetch_add
#pragma weak shmem_int64_t_atomic_fetch_add = pshmem_int64_t_atomic_fetch_add
#pragma weak shmem_uint64_t_atomic_fetch_add = pshmem_uint64_t_atomic_fetch_add
#pragma weak shmem_size_t_atomic_fetch_add = pshmem_size_t_atomic_fetch_add
#pragma weak shmem_ptrdiff_t_atomic_fetch_add = pshmem_ptrdiff_t_atomic_fetch_add

#pragma weak shmem_float_atomic_add = pshmem_float_atomic_add
#pragma weak shmem_double_atomic_add = pshmem_double_atomic_add
#pragma weak shmem_char_atomic_add = pshmem_char_atomic_add
#pragma weak shmem_schar_atomic_add = pshmem_schar_atomic_add
#pragma weak shmem_short_atomic_add = pshmem_short_atomic_add
#pragma weak shmem_int_atomic_add = pshmem_int_atomic_add
#pragma weak shmem_long_atomic_add = pshmem_long_atomic_add
#pragma weak shmem_longlong_atomic_add = pshmem_longlong_atomic_add
#pragma weak shmem_uchar_atomic_add = pshmem_uchar_atomic_add
#pragma weak shmem_ushort_atomic_add = pshmem_ushort_atomic_add
#pragma weak shmem_uint_atomic_add = pshmem_uint_atomic_add
#pragma weak shmem_ulong_atomic_add = pshmem_ulong_atomic_add
#pragma weak shmem_ulonglong_atomic_add = pshmem_ulonglong_atomic_add
#pragma weak shmem_int8_t_atomic_add = pshmem_int8_t_atomic_add
#pragma weak shmem_uint8_t_atomic_add = pshmem_uint8_t_atomic_add
#pragma weak shmem_int16_t_atomic_add = pshmem_int16_t_atomic_add
#pragma weak shmem_uint16_t_atomic_add = pshmem_uint16_t_atomic_add
#pragma weak shmem_int32_t_atomic_add = pshmem_int32_t_atomic_add
#pragma weak shmem_uint32_t_atomic_add = pshmem_uint32_t_atomic_add
#pragma weak shmem_int64_t_atomic_add = pshmem_int64_t_atomic_add
#pragma weak shmem_uint64_t_atomic_add = pshmem_uint64_t_atomic_add
#pragma weak shmem_size_t_atomic_add = pshmem_size_t_atomic_add
#pragma weak shmem_ptrdiff_t_atomic_add = pshmem_ptrdiff_t_atomic_add

#pragma weak shmem_float_atomic_fetch_inc = pshmem_float_atomic_fetch_inc
#pragma weak shmem_double_atomic_fetch_inc = pshmem_double_atomic_fetch_inc
#pragma weak shmem_char_atomic_fetch_inc = pshmem_char_atomic_fetch_inc
#pragma weak shmem_schar_atomic_fetch_inc = pshmem_schar_atomic_fetch_inc
#pragma weak shmem_short_atomic_fetch_inc = pshmem_short_atomic_fetch_inc
#pragma weak shmem_int_atomic_fetch_inc = pshmem_int_atomic_fetch_inc
#pragma weak shmem_long_atomic_fetch_inc = pshmem_long_atomic_fetch_inc
#pragma weak shmem_longlong_atomic_fetch_inc = pshmem_longlong_atomic_fetch_inc
#pragma weak shmem_uchar_atomic_fetch_inc = pshmem_uchar_atomic_fetch_inc
#pragma weak shmem_ushort_atomic_fetch_inc = pshmem_ushort_atomic_fetch_inc
#pragma weak shmem_uint_atomic_fetch_inc = pshmem_uint_atomic_fetch_inc
#pragma weak shmem_ulong_atomic_fetch_inc = pshmem_ulong_atomic_fetch_inc
#pragma weak shmem_ulonglong_atomic_fetch_inc = pshmem_ulonglong_atomic_fetch_inc
#pragma weak shmem_int8_t_atomic_fetch_inc = pshmem_int8_t_atomic_fetch_inc
#pragma weak shmem_uint8_t_atomic_fetch_inc = pshmem_uint8_t_atomic_fetch_inc
#pragma weak shmem_int16_t_atomic_fetch_inc = pshmem_int16_t_atomic_fetch_inc
#pragma weak shmem_uint16_t_atomic_fetch_inc = pshmem_uint16_t_atomic_fetch_inc
#pragma weak shmem_int32_t_atomic_fetch_inc = pshmem_int32_t_atomic_fetch_inc
#pragma weak shmem_uint32_t_atomic_fetch_inc = pshmem_uint32_t_atomic_fetch_inc
#pragma weak shmem_int64_t_atomic_fetch_inc = pshmem_int64_t_atomic_fetch_inc
#pragma weak shmem_uint64_t_atomic_fetch_inc = pshmem_uint64_t_atomic_fetch_inc
#pragma weak shmem_size_t_atomic_fetch_inc = pshmem_size_t_atomic_fetch_inc
#pragma weak shmem_ptrdiff_t_atomic_fetch_inc = pshmem_ptrdiff_t_atomic_fetch_inc

#pragma weak shmem_float_atomic_inc = pshmem_float_atomic_inc
#pragma weak shmem_double_atomic_inc = pshmem_double_atomic_inc
#pragma weak shmem_char_atomic_inc = pshmem_char_atomic_inc
#pragma weak shmem_schar_atomic_inc = pshmem_schar_atomic_inc
#pragma weak shmem_short_atomic_inc = pshmem_short_atomic_inc
#pragma weak shmem_int_atomic_inc = pshmem_int_atomic_inc
#pragma weak shmem_long_atomic_inc = pshmem_long_atomic_inc
#pragma weak shmem_longlong_atomic_inc = pshmem_longlong_atomic_inc
#pragma weak shmem_uchar_atomic_inc = pshmem_uchar_atomic_inc
#pragma weak shmem_ushort_atomic_inc = pshmem_ushort_atomic_inc
#pragma weak shmem_uint_atomic_inc = pshmem_uint_atomic_inc
#pragma weak shmem_ulong_atomic_inc = pshmem_ulong_atomic_inc
#pragma weak shmem_ulonglong_atomic_inc = pshmem_ulonglong_atomic_inc
#pragma weak shmem_int8_t_atomic_inc = pshmem_int8_t_atomic_inc
#pragma weak shmem_uint8_t_atomic_inc = pshmem_uint8_t_atomic_inc
#pragma weak shmem_int16_t_atomic_inc = pshmem_int16_t_atomic_inc
#pragma weak shmem_uint16_t_atomic_inc = pshmem_uint16_t_atomic_inc
#pragma weak shmem_int32_t_atomic_inc = pshmem_int32_t_atomic_inc
#pragma weak shmem_uint32_t_atomic_inc = pshmem_uint32_t_atomic_inc
#pragma weak shmem_int64_t_atomic_inc = pshmem_int64_t_atomic_inc
#pragma weak shmem_uint64_t_atomic_inc = pshmem_uint64_t_atomic_inc
#pragma weak shmem_size_t_atomic_inc = pshmem_size_t_atomic_inc
#pragma weak shmem_ptrdiff_t_atomic_inc = pshmem_ptrdiff_t_atomic_inc

#pragma weak shmem_quiet = pshmem_quiet
#pragma weak shmem_fence = pshmem_fence
#pragma weak shmem_barrier_all = pshmem_barrier_all

#endif /* ifdef MPC_OPENSHMEM_PROFILING_WEAK_SYMBOLS_H_ */

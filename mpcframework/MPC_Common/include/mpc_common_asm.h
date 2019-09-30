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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_ASM_H_
#define __SCTK_ASM_H_

#include "sctk_config.h"
#include "mpc_common_asm.h"



#include <sched.h>
#include <libpause.h>
#ifdef __MIC__
#include <immintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 * OPENPA INTEGRATION *
 **********************/

#include "opa_config.h"
#include "sctk_config.h"

#if defined(OPA_USE_UNSAFE_PRIMITIVES)\
 || defined(OPA_HAVE_GCC_AND_POWERPC_ASM)\
 || defined(OPA_HAVE_GCC_X86_32_64) \
 || defined(OPA_HAVE_GCC_X86_32_64_P3)\
 || defined(OPA_HAVE_GCC_AND_IA64_ASM)\
 || defined(OPA_HAVE_GCC_AND_SICORTEX_ASM) \
 || defined(OPA_HAVE_GCC_INTRINSIC_ATOMICS)\
 || defined(OPA_HAVE_SUN_ATOMIC_OPS)\
 || defined(OPA_HAVE_NT_INTRINSICS) \
 || defined(OPA_USE_LOCK_BASED_PRIMITIVES)

        #define SCTK_OPENPA_AVAILABLE

        #include "opa_primitives.h"

        // Rename the OPA types into sctk_atomics ones
        typedef OPA_int_t sctk_atomics_int;
        typedef OPA_ptr_t sctk_atomics_ptr;

        // Rename the OPA functions into sctk_atomics ones
        #define sctk_atomics_store_int           OPA_store_int
        #define sctk_atomics_swap_int            OPA_swap_int
        #define sctk_atomics_load_int            OPA_load_int
        #define sctk_atomics_load_ptr            OPA_load_ptr
        #define sctk_atomics_store_ptr           OPA_store_ptr
        #define sctk_atomics_add_int             OPA_add_int
        #define sctk_atomics_incr_int            OPA_incr_int
        #define sctk_atomics_decr_int            OPA_decr_int
        #define sctk_atomics_decr_and_test_int   OPA_decr_and_test_int
        #define sctk_atomics_fetch_and_add_int   OPA_fetch_and_add_int
        #define sctk_atomics_fetch_and_incr_int  OPA_fetch_and_incr_int
        #define sctk_atomics_fetch_and_decr_int  OPA_fetch_and_decr_int
        #define sctk_atomics_cas_ptr             OPA_cas_ptr
        #define sctk_atomics_cas_int             OPA_cas_int
        #define sctk_atomics_swap_ptr            OPA_swap_ptr
        #define sctk_atomics_write_barrier       OPA_write_barrier
        #define sctk_atomics_read_barrier        OPA_read_barrier
        #define sctk_atomics_read_write_barrier  OPA_read_write_barrier
        #define sctk_atomics_pause               OPA_busy_wait

        #define SCTK_ATOMICS_INT_T_INIT	OPA_INT_T_INITIALIZER
        #define SCTK_ATOMICS_PTR_T_INIT	OPA_PTR_T_INITIALIZER

#else /* OpenPA Test Variable */
        #error Unsupported architecture. Cannot compile MPC
#endif

/**********
 * TIMERS *
 **********/

#include <libtimer.h>

#define sctk_get_time_stamp sctk_atomics_get_timestamp
#define sctk_get_time_stamp_gettimeofday sctk_atomics_get_timestamp_gettimeofday

/***********************************
 * MPC TEST AND SET IMPLEMENTATION *
 ***********************************/

typedef volatile int sctk_atomic_test_t;


static inline int sctk_test_and_set( sctk_atomic_test_t *atomic )
{
        return sctk_atomics_swap_int( (OPA_int_t *) atomic, 1 );
}


#ifdef __cplusplus
}
#endif
#endif
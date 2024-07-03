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

#include <sched.h>
#include <mpc_arch.h>

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
#include "opa_primitives.h"
#else /* OpenPA Test Variable */
        #error Unsupported architecture. Cannot compile MPC
#endif


/***********************************
 * MPC TEST AND SET IMPLEMENTATION *
 ***********************************/

typedef volatile int sctk_atomic_test_t;

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline int sctk_test_and_set( OPA_int_t *atomic )
{
        return OPA_swap_int(atomic, 1 );
}


#ifdef __cplusplus
}
#endif
#endif

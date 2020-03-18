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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "omp_intel_types.h"
#include "omp_intel_atomics.h"

void __kmpc_atomic_4(__UNUSED__ ident_t *id_ref, __UNUSED__ int gtid, void *lhs, void *rhs,
                     void (*f)(void *, void *, void *)) {
  static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  if (
#if defined __i386 || defined __x86_64
      TRUE /* no alignment problems */
#else
      !((kmp_uintptr_t)lhs & 0x3) /* make sure address is 4-byte aligned */
#endif
      ) {
    kmp_int32 old_value, new_value;
    old_value = *(kmp_int32 *)lhs;
    (*f)(&new_value, &old_value, rhs);

    /* TODO: Should this be acquire or release? */
    while (!__kmp_compare_and_store32(
        (kmp_int32 *)lhs, *(kmp_int32 *)&old_value, *(kmp_int32 *)&new_value)) {
      DO_PAUSE;
      old_value = *(kmp_int32 *)lhs;
      (*f)(&new_value, &old_value, rhs);
    }
    return;
  } else {
    mpc_common_spinlock_lock(&lock);
    (*f)(lhs, lhs, rhs);
    mpc_common_spinlock_unlock(&lock);
  }
}

void __kmpc_atomic_fixed4_wr(__UNUSED__ ident_t *id_ref, __UNUSED__ int gtid, kmp_int32 *lhs,
                             kmp_int32 rhs) {
  __UNUSED__ mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  sctk_nodebug("[%d] %s: Write %d", t->rank, __func__, rhs);
  __kmp_xchg_fixed32(lhs, rhs);

#if 0
  __mpcomp_atomic_begin() ;

    *lhs = rhs ;

      __mpcomp_atomic_end() ;

        /* TODO: use assembly functions by Intel if available */
#endif
}

void __kmpc_atomic_float8_add(__UNUSED__ ident_t *id_ref,__UNUSED__  int gtid, kmp_real64 *lhs,
                              kmp_real64 rhs) {
  __UNUSED__ mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  sctk_nodebug("[%d] (ASM) %s: Add %g", t->rank, rhs);

#if 0
  __kmp_test_then_add_real64( lhs, rhs ) ;
#endif

/* TODO check how we can add this function to asssembly-dedicated module */

/* TODO use MPCOMP_MIC */

#if __MIC__ || __MIC2__
  __mpcomp_atomic_begin();

  *lhs += rhs;

  __mpcomp_atomic_end();
#else
  __kmp_test_then_add_real64(lhs, rhs);
#endif

  /* TODO: use assembly functions by Intel if available */
}
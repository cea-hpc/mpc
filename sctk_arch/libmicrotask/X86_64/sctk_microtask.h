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
/* #   - CARRIBAULT patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_MICROTASK_H
#define SCTK_MICROTASK_H

#if 0
typedef void (*kmpc_micro) (kmp_int32 * global_tid, kmp_int32 * bound_tid, ...) ;


int __kmp_invoke_microtask(
    kmpc_micro pkfn, int gtid, int npr, int argc, void *argv[] );

kmp_real64 __kmp_test_then_add_real64( volatile kmp_real64 *addr, kmp_real64 data );


X86_64

void __kmp_x86_cpuid( int mode, int mode2, void *cpuid_buffer );

#endif

int __kmp_invoke_microtask(
    void (*pkfn) (int * global_tid, int * bound_tid, ...), 
    int gtid, int npr, int argc, void *argv[] );


int __kmp_xchg_fixed32( volatile int * p, int d ) ;
int __kmp_test_then_add32( volatile int * addr, int data ) ;
long __kmp_test_then_add64( volatile long * addr, long data ) ;
double __kmp_test_then_add_real64( volatile double *addr, double data );

#endif

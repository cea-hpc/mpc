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
#include <assert.h>

int __kmp_invoke_microtask(
    void (*pkfn) (int * global_tid, int * bound_tid, ...), 
    int gtid, int npr, int argc, void *argv[] ){
  assert(0);
}


int __kmp_xchg_fixed32( volatile int * p, int d ) {
  assert(0);
}
int __kmp_test_then_add32( volatile int * addr, int data ) {
  assert(0);
}
long __kmp_test_then_add64( volatile long * addr, long data ) {
  assert(0);
}
double __kmp_test_then_add_real64( volatile double *addr, double data ){
  assert(0);
}

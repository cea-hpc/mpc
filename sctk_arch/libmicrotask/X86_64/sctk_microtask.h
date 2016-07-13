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



typedef char               kmp_int8;
typedef unsigned char      kmp_uint8;
typedef short              kmp_int16;
typedef unsigned short     kmp_uint16;
typedef int                kmp_int32;
typedef unsigned int       kmp_uint32;
typedef long long          kmp_int64;
typedef unsigned long long kmp_uint64;

typedef float              kmp_real32;
typedef double             kmp_real64;

kmp_int32  __kmp_test_then_add32( volatile kmp_int32 *, kmp_int32  ) ;
kmp_int64  __kmp_test_then_add64( volatile kmp_int64 *, kmp_int64 ) ;

kmp_int8   __kmp_compare_and_store8( volatile kmp_int8 *, kmp_int8, kmp_int8);
kmp_int16  __kmp_compare_and_store16( volatile kmp_int16 *, kmp_int16, kmp_int16);
kmp_int32  __kmp_compare_and_store32( volatile kmp_int32 *, kmp_int32, kmp_int32);
kmp_int32  __kmp_compare_and_store64( volatile kmp_int64 *, kmp_int64, kmp_int64);
kmp_int8   __kmp_compare_and_store_ret8( volatile kmp_int8  *p, kmp_int8  cv, kmp_int8  sv );
kmp_int16  __kmp_compare_and_store_ret16( volatile kmp_int16 *p, kmp_int16 cv, kmp_int16 sv );
kmp_int32  __kmp_compare_and_store_ret32( volatile kmp_int32 *p, kmp_int32 cv, kmp_int32 sv );
kmp_int64  __kmp_compare_and_store_ret64( volatile kmp_int64 *p, kmp_int64 cv, kmp_int64 sv );

kmp_int8   __kmp_xchg_fixed8( volatile kmp_int8  *p, kmp_int8  v );
kmp_int16  __kmp_xchg_fixed16( volatile kmp_int16 *p, kmp_int16 v );
kmp_int32  __kmp_xchg_fixed32( volatile kmp_int32 *, kmp_int32  ) ;
kmp_int64  __kmp_xchg_fixed64( volatile kmp_int64 *, kmp_int64  ) ;
kmp_real32 __kmp_xchg_real32( volatile kmp_real32 *p, kmp_real32 v );
kmp_real64 __kmp_xchg_real64( volatile kmp_real64 *p, kmp_real64 v );

double     __kmp_test_then_add_real32( kmp_real32 *, kmp_real32 );
double     __kmp_test_then_add_real64( kmp_real64 *, kmp_real64 );

int __kmp_x86_pause();

#endif

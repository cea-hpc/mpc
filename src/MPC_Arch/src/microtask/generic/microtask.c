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
#include "src/MPC_Common/include/mpc_keywords.h"
#include <assert.h>
//#include "sctk_microtask.h"
//#include "mpc_arch.h"

#include <stddef.h>
#include <stdio.h>

typedef size_t  kmp_size_t;
typedef float   kmp_real32;
typedef double  kmp_real64;
typedef char              kmp_int8;
typedef unsigned char     kmp_uint8;
typedef short             kmp_int16;
typedef unsigned short    kmp_uint16;
typedef int               kmp_int32;
typedef unsigned int      kmp_uint32;
typedef long long          kmp_int64;
typedef unsigned long long kmp_uint64;

typedef void (*microtask_t)(int *gtid, int *npr, ...);

#if !(KMP_ARCH_X86 || KMP_ARCH_X86_64 || KMP_MIC ||                            \
      ((KMP_OS_LINUX || KMP_OS_DARWIN) && KMP_ARCH_AARCH64) ||                 \
      KMP_ARCH_PPC64 || KMP_ARCH_RISCV64 || KMP_ARCH_LOONGARCH64 ||            \
      KMP_ARCH_ARM)

int
__kmp_invoke_microtask(microtask_t pkfn, int gtid, int tid, int argc, void *p_argv[]
#if OMPT_SUPPORT
                           ,
                           void **exit_frame_ptr
#endif
) {
#if OMPT_SUPPORT
  *exit_frame_ptr = OMPT_GET_FRAME_ADDRESS(0);
#endif

  switch (argc) {
  default:
    printf("Too many args (%d)\n", argc);
    assert(0);
  case 0:
    (*pkfn)(&gtid, &tid);
    break;
  case 1:
    (*pkfn)(&gtid, &tid, p_argv[0]);
    break;
  case 2:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1]);
    break;
  case 3:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1], p_argv[2]);
    break;
  case 4:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1], p_argv[2], p_argv[3]);
    break;
  case 5:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1], p_argv[2], p_argv[3], p_argv[4]);
    break;
  case 6:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1], p_argv[2], p_argv[3], p_argv[4], p_argv[5]);
    break;
  case 7:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1], p_argv[2], p_argv[3], p_argv[4], p_argv[5], p_argv[6]);
    break;
  case 8:
    (*pkfn)(&gtid, &tid, p_argv[0], p_argv[1], p_argv[2], p_argv[3], p_argv[4], p_argv[5], p_argv[6], p_argv[7]);
    break;





  }
  return 1;
}

#endif

// int __kmp_invoke_microtask(
//     void (*pkfn) (int * global_tid, int * bound_tid, ...),
//     int gtid, int npr, int argc, void *argv[] ){
//   assert(0);
// }

kmp_int32  __kmp_test_then_add32( volatile __UNUSED__ kmp_int32 *a, __UNUSED__ kmp_int32  b) {assert(0);}
kmp_int64  __kmp_test_then_add64( volatile __UNUSED__ kmp_int64 *a, __UNUSED__ kmp_int64 b) {assert(0);}

kmp_int8   __kmp_compare_and_store8( volatile __UNUSED__ kmp_int8 *a, __UNUSED__ kmp_int8 b, __UNUSED__ kmp_int8 c){assert(0);}
kmp_int16  __kmp_compare_and_store16( volatile __UNUSED__ kmp_int16 *a, __UNUSED__ kmp_int16 b, __UNUSED__ kmp_int16 c){assert(0);}
kmp_int32  __kmp_compare_and_store32( volatile __UNUSED__ kmp_int32 *a, __UNUSED__ kmp_int32 b, __UNUSED__ kmp_int32 c){assert(0);}
kmp_int32  __kmp_compare_and_store64( volatile __UNUSED__ kmp_int64 *a, __UNUSED__ kmp_int64 b, __UNUSED__ kmp_int64 c){assert(0);}
kmp_int8   __kmp_compare_and_store_ret8( volatile __UNUSED__ kmp_int8  *p, __UNUSED__ kmp_int8  cv, __UNUSED__ kmp_int8  sv ){assert(0);}
kmp_int16  __kmp_compare_and_store_ret16( volatile __UNUSED__ kmp_int16 *p, __UNUSED__ kmp_int16 cv, __UNUSED__ kmp_int16 sv ){assert(0);}
kmp_int32  __kmp_compare_and_store_ret32( volatile __UNUSED__ kmp_int32 *p, __UNUSED__ kmp_int32 cv, __UNUSED__ kmp_int32 sv ){assert(0);}
kmp_int64  __kmp_compare_and_store_ret64( volatile __UNUSED__ kmp_int64 *p, __UNUSED__ kmp_int64 cv, __UNUSED__ kmp_int64 sv ){assert(0);}

kmp_int8   __kmp_xchg_fixed8( volatile __UNUSED__ kmp_int8  *p, __UNUSED__ kmp_int8  v ){assert(0);}
kmp_int16  __kmp_xchg_fixed16( volatile __UNUSED__ kmp_int16 *p, __UNUSED__ kmp_int16 v ){assert(0);}
kmp_int32  __kmp_xchg_fixed32( volatile __UNUSED__ kmp_int32 *p, __UNUSED__ kmp_int32  v) {assert(0);}
kmp_int64  __kmp_xchg_fixed64( volatile __UNUSED__ kmp_int64 *p, __UNUSED__ kmp_int64  v) {assert(0);}
kmp_real32 __kmp_xchg_real32( volatile __UNUSED__ kmp_real32 *p, __UNUSED__ kmp_real32 v ){assert(0);}
kmp_real64 __kmp_xchg_real64( volatile __UNUSED__ kmp_real64 *p, __UNUSED__ kmp_real64 v ){assert(0);}

double     __kmp_test_then_add_real32( __UNUSED__ kmp_real32 *a, __UNUSED__ kmp_real32 b){assert(0);}
double     __kmp_test_then_add_real64( __UNUSED__ kmp_real64 *a, __UNUSED__ kmp_real64 b){assert(0);}

extern  void __sctk_cpu_relax ();
int __kmp_x86_pause(){__sctk_cpu_relax(); return 0;}

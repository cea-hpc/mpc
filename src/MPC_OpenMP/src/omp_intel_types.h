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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_TYPES_KMP_H__
#define __MPC_OMP_TYPES_KMP_H__

#include <stdlib.h>
#include <mpc_config.h>

#define MPC_OMP_FORCE_PARALLEL_REGION_ALLOC 1

#define KMP_HASH_TABLE_LOG2 9 /* log2 of the hash table size */
#define KMP_HASH_TABLE_SIZE                                                    \
  (1 << KMP_HASH_TABLE_LOG2) /* size of the hash table */
#define KMP_HASH_SHIFT 3 /* throw away this many low bits from the address */
#define KMP_HASH(x)                                                            \
  ((((uint64_t)x) >> KMP_HASH_SHIFT) & (KMP_HASH_TABLE_SIZE - 1))

/* KMP_OS.H */

typedef char kmp_int8;
typedef unsigned char kmp_uint8;
typedef short kmp_int16;
typedef unsigned short kmp_uint16;
typedef int kmp_int32;
typedef unsigned int kmp_uint32;
typedef long long kmp_int64;
typedef unsigned long long kmp_uint64;

typedef float kmp_real32;
typedef double kmp_real64;

#if defined(MPC_X86_64_ARCH) && defined(HAVE___FLOAT128)
typedef __float128 _Quad;
/* Check for quad-precision extension. Here, forced to 1 for MPC for x86*/
#define KMP_HAVE_QUAD 1
#endif

typedef unsigned char uchar;

/* KMP.H */

#ifndef KMP_INTPTR
#define KMP_INTPTR 1
typedef long kmp_intptr_t;
typedef unsigned long kmp_uintptr_t;
#define KMP_INTPTR_SPEC "ld"
#define KMP_UINTPTR_SPEC "lu"
#endif

typedef struct ident {
  kmp_int32 reserved_1;
  kmp_int32 flags;
  kmp_int32 reserved_2;
  kmp_int32 reserved_3;
  char const *psource;
} ident_t;

typedef void (*kmpc_micro)(kmp_int32 *global_tid, kmp_int32 *bound_tid, ...);

enum sched_type {
  kmp_sch_lower = 32, /**< lower bound for unordered values */
  kmp_sch_static_chunked = 33,
  kmp_sch_static = 34, /**< static unspecialized */
  kmp_sch_dynamic_chunked = 35,
  kmp_sch_guided_chunked = 36, /**< guided unspecialized */
  kmp_sch_runtime = 37,
  kmp_sch_auto = 38, /**< auto */
  kmp_sch_trapezoidal = 39,
  /* accessible only through KMP_SCHEDULE environment variable */
  kmp_sch_static_greedy = 40,
  kmp_sch_static_balanced = 41,
  /* accessible only through KMP_SCHEDULE environment variable */
  kmp_sch_guided_iterative_chunked = 42,
  kmp_sch_guided_analytical_chunked = 43,

  kmp_sch_static_steal =
      44, /**< accessible only through KMP_SCHEDULE environment variable */

  /* accessible only through KMP_SCHEDULE environment variable */
  kmp_sch_upper = 45, /**< upper bound for unordered values */

  kmp_ord_lower = 64, /**< lower bound for ordered values, must be power of 2 */
  kmp_ord_static_chunked = 65,
  kmp_ord_static = 66, /**< ordered static unspecialized */
  kmp_ord_dynamic_chunked = 67,
  kmp_ord_guided_chunked = 68,
  kmp_ord_runtime = 69,
  kmp_ord_auto = 70, /**< ordered auto */
  kmp_ord_trapezoidal = 71,
  kmp_ord_upper = 72, /**< upper bound for ordered values */

#if OMP_40_ENABLED
  /* Schedules for Distribute construct */
  kmp_distribute_static_chunked = 91, /**< distribute static chunked */
  kmp_distribute_static = 92,         /**< distribute static unspecialized */
#endif

  /*
   * For the "nomerge" versions, kmp_dispatch_next*() will always return
   * a single iteration/chunk, even if the loop is serialized.  For the
   * schedule types listed above, the entire iteration vector is returned
   * if the loop is serialized.  This doesn't work for gcc/gcomp sections.
   */
  kmp_nm_lower = 160, /**< lower bound for nomerge values */

  kmp_nm_static_chunked =
      (kmp_sch_static_chunked - kmp_sch_lower + kmp_nm_lower),
  kmp_nm_static = 162, /**< static unspecialized */
  kmp_nm_dynamic_chunked = 163,
  kmp_nm_guided_chunked = 164, /**< guided unspecialized */
  kmp_nm_runtime = 165,
  kmp_nm_auto = 166, /**< auto */
  kmp_nm_trapezoidal = 167,

  /* accessible only through KMP_SCHEDULE environment variable */
  kmp_nm_static_greedy = 168,
  kmp_nm_static_balanced = 169,
  /* accessible only through KMP_SCHEDULE environment variable */
  kmp_nm_guided_iterative_chunked = 170,
  kmp_nm_guided_analytical_chunked = 171,
  kmp_nm_static_steal =
      172, /* accessible only through OMP_SCHEDULE environment variable */

  kmp_nm_ord_static_chunked = 193,
  kmp_nm_ord_static = 194, /**< ordered static unspecialized */
  kmp_nm_ord_dynamic_chunked = 195,
  kmp_nm_ord_guided_chunked = 196,
  kmp_nm_ord_runtime = 197,
  kmp_nm_ord_auto = 198, /**< auto */
  kmp_nm_ord_trapezoidal = 199,
  kmp_nm_upper = 200,              /**< upper bound for nomerge values */
  kmp_sch_default = kmp_sch_static /**< default scheduling algorithm */
};

typedef kmp_int32 kmp_critical_name[8];

/*!
 * Values for bit flags used in the ident_t to describe the fields.
 * */
/*! Use trampoline for internal microtasks */
#define KMP_IDENT_IMB 0x01
/*! Use c-style ident structure */
#define KMP_IDENT_KMPC 0x02
/* 0x04 is no longer used */
/*! Entry point generated by auto-parallelization */
#define KMP_IDENT_AUTOPAR 0x08
/*! Compiler generates atomic reduction option for kmpc_reduce* */
#define KMP_IDENT_ATOMIC_REDUCE 0x10
/*! To mark a 'barrier' directive in user code */
#define KMP_IDENT_BARRIER_EXPL 0x20
/*! To Mark implicit barriers. */
#define KMP_IDENT_BARRIER_IMPL 0x0040
#define KMP_IDENT_BARRIER_IMPL_MASK 0x01C0
#define KMP_IDENT_BARRIER_IMPL_FOR 0x0040
#define KMP_IDENT_BARRIER_IMPL_SECTIONS 0x00C0

#define KMP_IDENT_BARRIER_IMPL_SINGLE 0x0140
#define KMP_IDENT_BARRIER_IMPL_WORKSHARE 0x01C0

/*******************************
  * REDUCTION
  *****************************/

/* different reduction cases */
enum _reduction_method {
  reduction_method_not_defined = 0,
  critical_reduce_block = (1 << 8),
  atomic_reduce_block = (2 << 8),
  tree_reduce_block = (3 << 8),
  empty_reduce_block = (4 << 8)
};

#define FAST_REDUCTION_ATOMIC_METHOD_GENERATED                                 \
  ((loc->flags & (KMP_IDENT_ATOMIC_REDUCE)) == (KMP_IDENT_ATOMIC_REDUCE))
#define FAST_REDUCTION_TREE_METHOD_GENERATED ((reduce_data) && (reduce_func))

/*******************************
  * THREADPRIVATE
  ******************************/
/* keeps tracked of threadprivate cache allocations for cleanup later */
typedef void (*microtask_t)(int *gtid, int *npr, ...);
typedef void *(*kmpc_ctor)(void *);
typedef void (*kmpc_dtor)(void *);

typedef void *(*kmpc_cctor)(void *, void *);
typedef void *(*kmpc_ctor_vec)(void *, size_t);
typedef void (*kmpc_dtor_vec)(void *, size_t);
typedef void *(*kmpc_cctor_vec)(void *, void *, size_t);

typedef struct kmp_cached_addr {
  void **addr;                  /* address of allocated cache */
  struct kmp_cached_addr *next; /* pointer to next cached address */
} kmp_cached_addr_t;

struct private_data {
  struct private_data *next; /* The next descriptor in the list      */
  void *data;                /* The data buffer for this descriptor  */
  int more;                  /* The repeat count for this descriptor */
  size_t size;               /* The data size for this descriptor    */
};

struct private_common {
  struct private_common *next;
  struct private_common *link;
  void *gbl_addr;
  void *par_addr; /* par_addr == gbl_addr for MASTER thread */
  size_t cmn_size;
};

struct shared_common {
  struct shared_common *next;
  struct private_data *pod_init;
  void *obj_init;
  void *gbl_addr;
  union {
    kmpc_ctor ctor;
    kmpc_ctor_vec ctorv;
  } ct;
  union {
    kmpc_cctor cctor;
    kmpc_cctor_vec cctorv;
  } cct;
  union {
    kmpc_dtor dtor;
    kmpc_dtor_vec dtorv;
  } dt;
  size_t vec_len;
  int is_vec;
  size_t cmn_size;
};

struct shared_table {
  struct shared_common *data[KMP_HASH_TABLE_SIZE];
};

#endif /* __MPC_OMP_TYPES_KMP_H__ */

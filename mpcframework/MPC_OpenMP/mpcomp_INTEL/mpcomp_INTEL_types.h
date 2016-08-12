#ifndef __MPCOMP_INTEL_TYPES_H__
#define __MPCOMP_INTEL_TYPES_H__


/* KMP_OS.H */

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

#ifdef SCTK_x86_64_ARCH_SCTK 
typedef __float128         _Quad;
/* Check for quad-precision extension. Here, forced to 1 for MPC for x86*/
#define KMP_HAVE_QUAD 1
#endif

typedef unsigned char      uchar;

/* KMP.H */

#ifndef KMP_INTPTR
# define KMP_INTPTR 1
  typedef long             kmp_intptr_t;
  typedef unsigned long    kmp_uintptr_t;
# define KMP_INTPTR_SPEC   "ld"
# define KMP_UINTPTR_SPEC  "lu"
#endif

typedef struct ident {
  kmp_int32 reserved_1 ;
  kmp_int32 flags ;
  kmp_int32 reserved_2 ;
  kmp_int32 reserved_3 ;
  char const *psource ;
} ident_t ;

#endif /* __MPCOMP_INTEL_TYPES_H__ */

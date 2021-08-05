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

TODO("this file triggers many warnings: 'dereferencing type-punned pointer will break strict-aliasing rules'");

#ifndef __MPC_OMP_TYPES_KMP_ATOMICS_H__
#define __MPC_OMP_TYPES_KMP_ATOMICS_H__

/********************************
 * ATOMIC_OPERATIONS
 *******************************/
/* Decleration of assembly coded routines used in atomic operations */
extern kmp_int32 __kmp_test_then_add32(volatile kmp_int32 *, kmp_int32);
extern kmp_int64 __kmp_test_then_add64(volatile kmp_int64 *, kmp_int64);

extern kmp_int8 __kmp_compare_and_store8(volatile kmp_int8 *, kmp_int8,
                                         kmp_int8);
extern kmp_int16 __kmp_compare_and_store16(volatile kmp_int16 *, kmp_int16,
                                           kmp_int16);
extern kmp_int32 __kmp_compare_and_store32(volatile kmp_int32 *, kmp_int32,
                                           kmp_int32);
extern kmp_int32 __kmp_compare_and_store64(volatile kmp_int64 *, kmp_int64,
                                           kmp_int64);
extern kmp_int8 __kmp_compare_and_store_ret8(volatile kmp_int8 *p, kmp_int8 cv,
                                             kmp_int8 sv);
extern kmp_int16 __kmp_compare_and_store_ret16(volatile kmp_int16 *p,
                                               kmp_int16 cv, kmp_int16 sv);
extern kmp_int32 __kmp_compare_and_store_ret32(volatile kmp_int32 *p,
                                               kmp_int32 cv, kmp_int32 sv);
extern kmp_int64 __kmp_compare_and_store_ret64(volatile kmp_int64 *p,
                                               kmp_int64 cv, kmp_int64 sv);

extern kmp_int8 __kmp_xchg_fixed8(volatile kmp_int8 *p, kmp_int8 v);
extern kmp_int16 __kmp_xchg_fixed16(volatile kmp_int16 *p, kmp_int16 v);

extern kmp_int32 __kmp_xchg_fixed32(volatile kmp_int32 *, kmp_int32);
extern kmp_int64 __kmp_xchg_fixed64(volatile kmp_int64 *, kmp_int64);
extern kmp_real32 __kmp_xchg_real32(volatile kmp_real32 *p, kmp_real32 v);
extern kmp_real64 __kmp_xchg_real64(volatile kmp_real64 *p, kmp_real64 v);

extern double __kmp_test_then_add_real32(kmp_real32 *, kmp_real32);
extern double __kmp_test_then_add_real64(kmp_real64 *, kmp_real64);

#if __MIC__ || __MIC2__
#define DO_PAUSE    _mm_delay_32(1)
#else
int __kmp_x86_pause();

#define DO_PAUSE    __kmp_x86_pause()
#endif

void __kmpc_atomic_4(ident_t *id_ref, int gtid, void *lhs, void *rhs,
                     void (*f)(void *, void *, void *) );
void __kmpc_atomic_fixed4_wr(ident_t *id_ref, int gtid, kmp_int32 *lhs,
                             kmp_int32 rhs);
void __kmpc_atomic_float8_add(ident_t *id_ref, int gtid, kmp_real64 *lhs,
                              kmp_real64 rhs);

/* begins for atomic functions */
#define ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, RET_TYPE)                                                       \
	RET_TYPE __kmpc_atomic_ ## TYPE_ID ## _ ## OP_ID(__UNUSED__ ident_t * id_ref, __UNUSED__ int gtid, \
	                                                 TYPE * lhs, TYPE rhs){
#define ATOMIC_BEGIN_REV(TYPE_ID, OP_ID, TYPE, RET_TYPE)                                                           \
	RET_TYPE __kmpc_atomic_ ## TYPE_ID ## _ ## OP_ID ## _rev(__UNUSED__ ident_t * id_ref, __UNUSED__ int gtid, \
	                                                         TYPE * lhs, TYPE rhs){
#define ATOMIC_BEGIN_MIX(TYPE_ID, TYPE, OP_ID, RTYPE_ID, RTYPE)        \
	void __kmpc_atomic_ ## TYPE_ID ## _ ## OP_ID ## _ ## RTYPE_ID( \
	        __UNUSED__ ident_t * id_ref, __UNUSED__ int gtid, TYPE * lhs, RTYPE rhs){
#define ATOMIC_BEGIN_READ(TYPE_ID, OP_ID, TYPE, RET_TYPE)                                                  \
	RET_TYPE __kmpc_atomic_ ## TYPE_ID ## _ ## OP_ID(__UNUSED__ ident_t * id_ref, __UNUSED__ int gtid, \
	                                                 TYPE * loc){
#define ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, RET_TYPE)                                                    \
	RET_TYPE __kmpc_atomic_ ## TYPE_ID ## _ ## OP_ID(__UNUSED__ ident_t * id_ref, __UNUSED__  int gtid, \
	                                                 TYPE * lhs, TYPE rhs, int flag){
/* atomic functions */
#define ATOMIC_FIXED_ADD(TYPE_ID, OP_ID, TYPE, BITS, OP, LCK_ID, MASK, \
	                 GOMP_FLAG)                                    \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                       \
	__kmp_test_then_add ## BITS( (lhs), (+rhs) );                  \
	}

#define ATOMIC_CMPXCHG(TYPE_ID, OP_ID, TYPE, BITS, OP, LCK_ID, MASK,                    \
	               GOMP_FLAG)                                                       \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                        \
	TYPE old_value, new_value;                                                      \
	old_value = *(TYPE volatile *)lhs;                                              \
	new_value = old_value OP rhs;                                                   \
	while(!__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),              \
	                                        (*(kmp_int ## BITS *) & old_value),     \
	                                        (*(kmp_int ## BITS *) & new_value) ) ){ \
		DO_PAUSE;                                                               \
		old_value = *(TYPE volatile *)lhs;                                      \
		new_value = old_value OP rhs;                                           \
	}                                                                               \
	}

#define ATOMIC_CMPX_L(TYPE_ID, OP_ID, TYPE, BITS, OP, LCK_ID, MASK, GOMP_FLAG)          \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                        \
	TYPE old_value, new_value;                                                      \
	old_value = *(TYPE volatile *)lhs;                                              \
	new_value = old_value OP rhs;                                                   \
	while(!__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),              \
	                                        (*(kmp_int ## BITS *) & old_value),     \
	                                        (*(kmp_int ## BITS *) & new_value) ) ){ \
		DO_PAUSE;                                                               \
		old_value = *(TYPE volatile *)lhs;                                      \
		new_value = old_value OP rhs;                                           \
	}                                                                               \
	}

#define MIN_MAX_COMPXCHG(TYPE_ID, OP_ID, TYPE, BITS, OP, LCK_ID, MASK,                      \
	                 GOMP_FLAG)                                                         \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                            \
	if(*lhs OP rhs){                                                                    \
		TYPE volatile temp_val;                                                     \
		TYPE          old_value;                                                    \
		temp_val  = *lhs;                                                           \
		old_value = temp_val;                                                       \
		while(old_value OP rhs &&                                                   \
		      !__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),          \
		                                        (*(kmp_int ## BITS *) & old_value), \
		                                        (*(kmp_int ## BITS *) & rhs) ) ){   \
			DO_PAUSE;                                                           \
			temp_val  = *lhs;                                                   \
			old_value = temp_val;                                               \
		}                                                                           \
	}                                                                                   \
	}

#define ATOMIC_CMPX_EQV(TYPE_ID, OP_ID, TYPE, BITS, OP, LCK_ID, MASK,                   \
	                GOMP_FLAG)                                                      \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                        \
	TYPE old_value, new_value;                                                      \
	old_value = *(TYPE volatile *)lhs;                                              \
	new_value = old_value OP rhs;                                                   \
	while(!__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),              \
	                                        (*(kmp_int ## BITS *) & old_value),     \
	                                        (*(kmp_int ## BITS *) & new_value) ) ){ \
		DO_PAUSE;                                                               \
		old_value = *(TYPE volatile *)lhs;                                      \
		new_value = old_value OP rhs;                                           \
	}                                                                               \
	}

#define ATOMIC_CRITICAL(TYPE_ID, OP_ID, TYPE, OP, LCK_ID, GOMP_FLAG)            \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                \
	static mpc_common_spinlock_t critical_lock = SCTK_SPINLOCK_INITIALIZER; \
	mpc_common_spinlock_lock(&critical_lock);                               \
	(*lhs)OP ## = (rhs);                                                    \
	mpc_common_spinlock_unlock(&critical_lock);                             \
	}

#define ATOMIC_CRITICAL_REV(TYPE_ID, OP_ID, TYPE, OP, LCK_ID, GOMP_FLAG)        \
	ATOMIC_BEGIN_REV(TYPE_ID, OP_ID, TYPE, void)                            \
	static mpc_common_spinlock_t critical_lock = SCTK_SPINLOCK_INITIALIZER; \
	mpc_common_spinlock_lock(&critical_lock);                               \
	(*lhs) = (rhs)OP(*lhs);                                                 \
	mpc_common_spinlock_unlock(&critical_lock);                             \
	}

#define MIN_MAX_COMPXCHG_CPT(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG)                     \
	ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, TYPE)                                        \
	TYPE old_value;                                                                     \
	if(*lhs OP rhs){                                                                    \
		TYPE volatile temp_val;                                                     \
		temp_val  = *lhs;                                                           \
		old_value = temp_val;                                                       \
		while(old_value OP rhs &&                                                   \
		      !__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),          \
		                                        (*(kmp_int ## BITS *) & old_value), \
		                                        (*(kmp_int ## BITS *) & rhs) ) ){   \
			DO_PAUSE;                                                           \
			temp_val  = *lhs;                                                   \
			old_value = temp_val;                                               \
		}                                                                           \
		if(flag){                                                                   \
			return rhs; }                                                       \
		else{                                                                       \
			return old_value; }                                                 \
	}                                                                                   \
	return *lhs;                                                                        \
	}

#define ATOMIC_CMPXCHG_CPT(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG)                   \
	ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, TYPE)                                    \
	TYPE volatile temp_val;                                                         \
	TYPE old_value, new_value;                                                      \
	temp_val  = *lhs;                                                               \
	old_value = temp_val;                                                           \
	new_value = old_value OP rhs;                                                   \
	while(!__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),              \
	                                        (*(kmp_int ## BITS *) & old_value),     \
	                                        (*(kmp_int ## BITS *) & new_value) ) ){ \
		DO_PAUSE;                                                               \
		temp_val  = *lhs;                                                       \
		old_value = temp_val;                                                   \
		new_value = old_value OP rhs;                                           \
	}                                                                               \
	if(flag){                                                                       \
		return new_value;                                                       \
	} else{                                                                         \
		return old_value; }                                                     \
	}

#define ATOMIC_CMPX_EQV_CPT(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG)                  \
	ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, TYPE)                                    \
	TYPE volatile temp_val;                                                         \
	TYPE old_value, new_value;                                                      \
	temp_val  = *lhs;                                                               \
	old_value = temp_val;                                                           \
	new_value = old_value OP rhs;                                                   \
	while(!__kmp_compare_and_store ## BITS( ( (kmp_int ## BITS *)lhs),              \
	                                        (*(kmp_int ## BITS *) & old_value),     \
	                                        (*(kmp_int ## BITS *) & new_value) ) ){ \
		DO_PAUSE;                                                               \
		temp_val  = *lhs;                                                       \
		old_value = temp_val;                                                   \
		new_value = old_value OP rhs;                                           \
	}                                                                               \
	if(flag){                                                                       \
		return new_value;                                                       \
	} else{                                                                         \
		return old_value; }                                                     \
	}

#define ATOMIC_CRITICAL_CPT(TYPE_ID, OP_ID, TYPE, OP, LCK_ID, GOMP_FLAG)        \
	ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, TYPE)                            \
	static mpc_common_spinlock_t critical_lock = SCTK_SPINLOCK_INITIALIZER; \
	TYPE new_value;                                                         \
	mpc_common_spinlock_lock(&critical_lock);                               \
	if(flag){                                                               \
		(*lhs)OP ## = rhs;                                              \
		new_value   = (*lhs);                                           \
	} else {                                                                \
		new_value   = (*lhs);                                           \
		(*lhs)OP ## = rhs;                                              \
	}                                                                       \
	mpc_common_spinlock_unlock(&critical_lock);                             \
	return new_value;                                                       \
	}

#define ATOMIC_CMPXCHG_REV(TYPE_ID, OP_ID, TYPE, BITS, OP, LCK_ID, GOMP_FLAG)        \
	ATOMIC_BEGIN_REV(TYPE_ID, OP_ID, TYPE, void)                                 \
	TYPE volatile temp_val;                                                      \
	TYPE old_value, new_value;                                                   \
	temp_val  = *lhs;                                                            \
	old_value = temp_val;                                                        \
	new_value = rhs OP old_value;                                                \
	while(!__kmp_compare_and_store ## BITS( (kmp_int ## BITS *)lhs,              \
	                                        *(kmp_int ## BITS *) & old_value,    \
	                                        *(kmp_int ## BITS *) & new_value) ){ \
		DO_PAUSE;                                                            \
		temp_val  = *lhs;                                                    \
		old_value = temp_val;                                                \
		new_value = rhs OP old_value;                                        \
	}                                                                            \
	}

#define ATOMIC_CMPXCHG_MIX(TYPE_ID, TYPE, OP_ID, BITS, OP, RTYPE_ID, RTYPE,          \
	                   LCK_ID, MASK, GOMP_FLAG)                                  \
	ATOMIC_BEGIN_MIX(TYPE_ID, TYPE, OP_ID, RTYPE_ID, RTYPE)                      \
	TYPE old_value, new_value;                                                   \
	old_value = *(TYPE volatile *)lhs;                                           \
	new_value = old_value OP(TYPE) rhs;                                          \
	while(!__kmp_compare_and_store ## BITS( (kmp_int ## BITS *)lhs,              \
	                                        *(kmp_int ## BITS *) & old_value,    \
	                                        *(kmp_int ## BITS *) & new_value) ){ \
		DO_PAUSE;                                                            \
		old_value = *(TYPE volatile *)lhs;                                   \
		new_value = old_value OP(TYPE) rhs;                                  \
	}                                                                            \
	}

#define ATOMIC_FIXED_READ(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG) \
	ATOMIC_BEGIN_READ(TYPE_ID, OP_ID, TYPE, TYPE)                \
	TYPE new_value;                                              \
	new_value = __kmp_test_then_add ## BITS(loc, OP 0);          \
	return new_value;                                            \
	}

#define ATOMIC_CMPXCHG_READ(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG)               \
	ATOMIC_BEGIN_READ(TYPE_ID, OP_ID, TYPE, TYPE)                                \
	TYPE new_value;                                                              \
	TYPE volatile temp_val;                                                      \
	union f_i_union {                                                            \
		TYPE f_val;                                                          \
		kmp_int ## BITS i_val;                                               \
	};                                                                           \
	union f_i_union old_value;                                                   \
	temp_val        = *loc;                                                      \
	old_value.f_val = temp_val;                                                  \
	old_value.i_val = __kmp_compare_and_store_ret ## BITS(                       \
	        ( (kmp_int ## BITS *)loc), (*(kmp_int ## BITS *) & old_value.i_val), \
	        (*(kmp_int ## BITS *) & old_value.i_val) );                          \
	new_value = old_value.f_val;                                                 \
	return new_value;                                                            \
	}

#define ATOMIC_CRITICAL_READ(TYPE_ID, OP_ID, TYPE, OP, LCK_ID, GOMP_FLAG)       \
	ATOMIC_BEGIN_READ(TYPE_ID, OP_ID, TYPE, TYPE)                           \
	TYPE new_value;                                                         \
	static mpc_common_spinlock_t critical_lock = SCTK_SPINLOCK_INITIALIZER; \
	mpc_common_spinlock_lock(&critical_lock);                               \
	new_value = (*loc);                                                     \
	mpc_common_spinlock_unlock(&critical_lock);                             \
	return new_value;                                                       \
	}

#define ATOMIC_XCHG_WR(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG) \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                  \
	__kmp_xchg_fixed ## BITS( (lhs), (rhs) );                 \
	}

#define ATOMIC_XCHG_FLOAT_WR(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG) \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                        \
	__kmp_xchg_real ## BITS( (lhs), (rhs) );                        \
	}

#define ATOMIC_CMPXCHG_WR(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG)                 \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                     \
	TYPE volatile temp_val;                                                      \
	TYPE old_value, new_value;                                                   \
	temp_val  = *lhs;                                                            \
	old_value = temp_val;                                                        \
	new_value = rhs;                                                             \
	while(!__kmp_compare_and_store ## BITS( (kmp_int ## BITS *)lhs,              \
	                                        *(kmp_int ## BITS *) & old_value,    \
	                                        *(kmp_int ## BITS *) & new_value) ){ \
		DO_PAUSE;                                                            \
		temp_val  = *lhs;                                                    \
		old_value = temp_val;                                                \
		new_value = rhs;                                                     \
	}                                                                            \
	}

#define ATOMIC_CRITICAL_WR(TYPE_ID, OP_ID, TYPE, OP, LCK_ID, GOMP_FLAG)         \
	ATOMIC_BEGIN(TYPE_ID, OP_ID, TYPE, void)                                \
	static mpc_common_spinlock_t critical_lock = SCTK_SPINLOCK_INITIALIZER; \
	mpc_common_spinlock_lock(&critical_lock);                               \
	(*lhs)OP(rhs);                                                          \
	mpc_common_spinlock_unlock(&critical_lock);                             \
	}

#define ATOMIC_FIXED_ADD_CPT(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG) \
	ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, TYPE)                    \
	TYPE old_value;                                                 \
	old_value = __kmp_test_then_add ## BITS( (lhs), (OP rhs) );     \
	if(flag){                                                       \
		return old_value OP rhs;                                \
	} else{                                                         \
		return old_value; }                                     \
	}

#define ATOMIC_CMPX_L_CPT(TYPE_ID, OP_ID, TYPE, BITS, OP, GOMP_FLAG)                 \
	ATOMIC_BEGIN_CPT(TYPE_ID, OP_ID, TYPE, TYPE)                                 \
	TYPE volatile temp_val;                                                      \
	TYPE old_value, new_value;                                                   \
	temp_val  = *lhs;                                                            \
	old_value = temp_val;                                                        \
	new_value = old_value OP rhs;                                                \
	while(!__kmp_compare_and_store ## BITS( (kmp_int ## BITS *)lhs,              \
	                                        *(kmp_int ## BITS *) & old_value,    \
	                                        *(kmp_int ## BITS *) & new_value) ){ \
		DO_PAUSE;                                                            \
		temp_val  = *lhs;                                                    \
		old_value = temp_val;                                                \
		new_value = old_value OP rhs;                                        \
	}                                                                            \
                                                                                     \
	if(flag){                                                                    \
		return new_value;                                                    \
	} else{                                                                      \
		return old_value; }                                                  \
	}

// Routines for ATOMIC 4-byte operands addition and subtraction
ATOMIC_FIXED_ADD(fixed4, add, kmp_int32, 32, +, 4i, 3,
                 0) // __kmpc_atomic_fixed4_add
ATOMIC_FIXED_ADD(fixed4, sub, kmp_int32, 32, -, 4i, 3,
                 0) // __kmpc_atomic_fixed4_sub

ATOMIC_CMPXCHG(float4, add, kmp_real32, 32, +, 4r, 3,
               KMP_ARCH_X86) // __kmpc_atomic_float4_add
ATOMIC_CMPXCHG(float4, sub, kmp_real32, 32, -, 4r, 3,
               KMP_ARCH_X86) // __kmpc_atomic_float4_sub

// Routines for ATOMIC 8-byte operands addition and subtraction
ATOMIC_FIXED_ADD(fixed8, add, kmp_int64, 64, +, 8i, 7,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed8_add
ATOMIC_FIXED_ADD(fixed8, sub, kmp_int64, 64, -, 8i, 7,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed8_sub

// ATOMIC_CMPXCHG( float8,  add, kmp_real64, 64, +,  8r, 7, KMP_ARCH_X86 )  //
// __kmpc_atomic_float8_add
ATOMIC_CMPXCHG(float8, sub, kmp_real64, 64, -, 8r, 7,
               KMP_ARCH_X86) // __kmpc_atomic_float8_sub

// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//              TYPE_ID,OP_ID, TYPE,          OP, LCK_ID, GOMP_FLAG
ATOMIC_CMPXCHG(fixed1, add, kmp_int8, 8, +, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_add
ATOMIC_CMPXCHG(fixed1, andb, kmp_int8, 8, &, 1i, 0,
               0)            // __kmpc_atomic_fixed1_andb
ATOMIC_CMPXCHG(fixed1, div, kmp_int8, 8, /, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_div
ATOMIC_CMPXCHG(fixed1u, div, kmp_uint8, 8, /, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1u_div
ATOMIC_CMPXCHG(fixed1, mul, kmp_int8, 8, *, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_mul
ATOMIC_CMPXCHG(fixed1, orb, kmp_int8, 8, |, 1i, 0,
               0)            // __kmpc_atomic_fixed1_orb
ATOMIC_CMPXCHG(fixed1, shl, kmp_int8, 8, <<, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_shl
ATOMIC_CMPXCHG(fixed1, shr, kmp_int8, 8, >>, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_shr
ATOMIC_CMPXCHG(fixed1u, shr, kmp_uint8, 8, >>, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1u_shr
ATOMIC_CMPXCHG(fixed1, sub, kmp_int8, 8, -, 1i, 0,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_sub
ATOMIC_CMPXCHG(fixed1, xor, kmp_int8, 8, ^, 1i, 0,
               0)            // __kmpc_atomic_fixed1_xor
ATOMIC_CMPXCHG(fixed2, add, kmp_int16, 16, +, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_add
ATOMIC_CMPXCHG(fixed2, andb, kmp_int16, 16, &, 2i, 1,
               0)            // __kmpc_atomic_fixed2_andb
ATOMIC_CMPXCHG(fixed2, div, kmp_int16, 16, /, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_div
ATOMIC_CMPXCHG(fixed2u, div, kmp_uint16, 16, /, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2u_div
ATOMIC_CMPXCHG(fixed2, mul, kmp_int16, 16, *, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_mul
ATOMIC_CMPXCHG(fixed2, orb, kmp_int16, 16, |, 2i, 1,
               0)            // __kmpc_atomic_fixed2_orb
ATOMIC_CMPXCHG(fixed2, shl, kmp_int16, 16, <<, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_shl
ATOMIC_CMPXCHG(fixed2, shr, kmp_int16, 16, >>, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_shr
ATOMIC_CMPXCHG(fixed2u, shr, kmp_uint16, 16, >>, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2u_shr
ATOMIC_CMPXCHG(fixed2, sub, kmp_int16, 16, -, 2i, 1,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_sub
ATOMIC_CMPXCHG(fixed2, xor, kmp_int16, 16, ^, 2i, 1,
               0)            // __kmpc_atomic_fixed2_xor
ATOMIC_CMPXCHG(fixed4, andb, kmp_int32, 32, &, 4i, 3,
               0)            // __kmpc_atomic_fixed4_andb
ATOMIC_CMPXCHG(fixed4, div, kmp_int32, 32, /, 4i, 3,
               KMP_ARCH_X86) // __kmpc_atomic_fixed4_div
ATOMIC_CMPXCHG(fixed4u, div, kmp_uint32, 32, /, 4i, 3,
               KMP_ARCH_X86) // __kmpc_atomic_fixed4u_div
ATOMIC_CMPXCHG(fixed4, mul, kmp_int32, 32, *, 4i, 3,
               KMP_ARCH_X86) // __kmpc_atomic_fixed4_mul
ATOMIC_CMPXCHG(fixed4, orb, kmp_int32, 32, |, 4i, 3,
               0)            // __kmpc_atomic_fixed4_orb
ATOMIC_CMPXCHG(fixed4, shl, kmp_int32, 32, <<, 4i, 3,
               KMP_ARCH_X86) // __kmpc_atomic_fixed4_shl
ATOMIC_CMPXCHG(fixed4, shr, kmp_int32, 32, >>, 4i, 3,
               KMP_ARCH_X86) // __kmpc_atomic_fixed4_shr
ATOMIC_CMPXCHG(fixed4u, shr, kmp_uint32, 32, >>, 4i, 3,
               KMP_ARCH_X86) // __kmpc_atomic_fixed4u_shr
ATOMIC_CMPXCHG(fixed4, xor, kmp_int32, 32, ^, 4i, 3,
               0)            // __kmpc_atomic_fixed4_xor
ATOMIC_CMPXCHG(fixed8, andb, kmp_int64, 64, &, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_andb
ATOMIC_CMPXCHG(fixed8, div, kmp_int64, 64, /, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_div
ATOMIC_CMPXCHG(fixed8u, div, kmp_uint64, 64, /, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8u_div
ATOMIC_CMPXCHG(fixed8, mul, kmp_int64, 64, *, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_mul
ATOMIC_CMPXCHG(fixed8, orb, kmp_int64, 64, |, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_orb
ATOMIC_CMPXCHG(fixed8, shl, kmp_int64, 64, <<, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_shl
ATOMIC_CMPXCHG(fixed8, shr, kmp_int64, 64, >>, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_shr
ATOMIC_CMPXCHG(fixed8u, shr, kmp_uint64, 64, >>, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8u_shr
ATOMIC_CMPXCHG(fixed8, xor, kmp_int64, 64, ^, 8i, 7,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_xor
ATOMIC_CMPXCHG(float4, div, kmp_real32, 32, /, 4r, 3,
               KMP_ARCH_X86) // __kmpc_atomic_float4_div
ATOMIC_CMPXCHG(float4, mul, kmp_real32, 32, *, 4r, 3,
               KMP_ARCH_X86) // __kmpc_atomic_float4_mul
ATOMIC_CMPXCHG(float8, div, kmp_real64, 64, /, 8r, 7,
               KMP_ARCH_X86) // __kmpc_atomic_float8_div
ATOMIC_CMPXCHG(float8, mul, kmp_real64, 64, *, 8r, 7,
               KMP_ARCH_X86) // __kmpc_atomic_float8_mul
//              TYPE_ID,OP_ID, TYPE,          OP, LCK_ID, GOMP_FLAG

/* ------------------------------------------------------------------------ */
/* Routines for C/C++ Reduction operators && and ||                         */
/* ------------------------------------------------------------------------ */

ATOMIC_CMPX_L(fixed1, andl, char, 8, &&, 1i, 0,
              KMP_ARCH_X86) // __kmpc_atomic_fixed1_andl
ATOMIC_CMPX_L(fixed1, orl, char, 8, ||, 1i, 0,
              KMP_ARCH_X86) // __kmpc_atomic_fixed1_orl
ATOMIC_CMPX_L(fixed2, andl, short, 16, &&, 2i, 1,
              KMP_ARCH_X86) // __kmpc_atomic_fixed2_andl
ATOMIC_CMPX_L(fixed2, orl, short, 16, ||, 2i, 1,
              KMP_ARCH_X86) // __kmpc_atomic_fixed2_orl
ATOMIC_CMPX_L(fixed4, andl, kmp_int32, 32, &&, 4i, 3,
              0)            // __kmpc_atomic_fixed4_andl
ATOMIC_CMPX_L(fixed4, orl, kmp_int32, 32, ||, 4i, 3,
              0)            // __kmpc_atomic_fixed4_orl
ATOMIC_CMPX_L(fixed8, andl, kmp_int64, 64, &&, 8i, 7,
              KMP_ARCH_X86) // __kmpc_atomic_fixed8_andl
ATOMIC_CMPX_L(fixed8, orl, kmp_int64, 64, ||, 8i, 7,
              KMP_ARCH_X86) // __kmpc_atomic_fixed8_orl

/* ------------------------------------------------------------------------- */
/* Routines for Fortran operators that matched no one in C:                  */
/* MAX, MIN, .EQV., .NEQV.                                                   */
/* Operators .AND., .OR. are covered by __kmpc_atomic_*_{andl,orl}           */
/* Intrinsics IAND, IOR, IEOR are covered by __kmpc_atomic_*_{andb,orb,xor}  */
/* ------------------------------------------------------------------------- */

MIN_MAX_COMPXCHG(fixed1, max, char, 8, <, 1i, 0,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed1_max
MIN_MAX_COMPXCHG(fixed1, min, char, 8, >, 1i, 0,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed1_min
MIN_MAX_COMPXCHG(fixed2, max, short, 16, <, 2i, 1,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed2_max
MIN_MAX_COMPXCHG(fixed2, min, short, 16, >, 2i, 1,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed2_min
MIN_MAX_COMPXCHG(fixed4, max, kmp_int32, 32, <, 4i, 3,
                 0)            // __kmpc_atomic_fixed4_max
MIN_MAX_COMPXCHG(fixed4, min, kmp_int32, 32, >, 4i, 3,
                 0)            // __kmpc_atomic_fixed4_min
MIN_MAX_COMPXCHG(fixed8, max, kmp_int64, 64, <, 8i, 7,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed8_max
MIN_MAX_COMPXCHG(fixed8, min, kmp_int64, 64, >, 8i, 7,
                 KMP_ARCH_X86) // __kmpc_atomic_fixed8_min
MIN_MAX_COMPXCHG(float4, max, kmp_real32, 32, <, 4r, 3,
                 KMP_ARCH_X86) // __kmpc_atomic_float4_max
MIN_MAX_COMPXCHG(float4, min, kmp_real32, 32, >, 4r, 3,
                 KMP_ARCH_X86) // __kmpc_atomic_float4_min
MIN_MAX_COMPXCHG(float8, max, kmp_real64, 64, <, 8r, 7,
                 KMP_ARCH_X86) // __kmpc_atomic_float8_max
MIN_MAX_COMPXCHG(float8, min, kmp_real64, 64, >, 8r, 7,
                 KMP_ARCH_X86) // __kmpc_atomic_float8_min
#if 0
#if KMP_HAVE_QUAD
MIN_MAX_CRITICAL(float16, max, QUAD_LEGACY, <, 16r, 1)                   // __kmpc_atomic_float16_max
MIN_MAX_CRITICAL(float16, min, QUAD_LEGACY, >, 16r, 1)                   // __kmpc_atomic_float16_min
#if (KMP_ARCH_X86)
MIN_MAX_CRITICAL(float16, max_a16, Quad_a16_t, <, 16r, 1)                // __kmpc_atomic_float16_max_a16
MIN_MAX_CRITICAL(float16, min_a16, Quad_a16_t, >, 16r, 1)                // __kmpc_atomic_float16_min_a16
#endif
#endif
#endif

ATOMIC_CMPXCHG(fixed1, neqv, kmp_int8, 8, ^, 1i, 0,
               KMP_ARCH_X86)  // __kmpc_atomic_fixed1_neqv
ATOMIC_CMPXCHG(fixed2, neqv, kmp_int16, 16, ^, 2i, 1,
               KMP_ARCH_X86)  // __kmpc_atomic_fixed2_neqv
ATOMIC_CMPXCHG(fixed4, neqv, kmp_int32, 32, ^, 4i, 3,
               KMP_ARCH_X86)  // __kmpc_atomic_fixed4_neqv
ATOMIC_CMPXCHG(fixed8, neqv, kmp_int64, 64, ^, 8i, 7,
               KMP_ARCH_X86)  // __kmpc_atomic_fixed8_neqv
ATOMIC_CMPX_EQV(fixed1, eqv, kmp_int8, 8, ^ ~, 1i, 0,
                KMP_ARCH_X86) // __kmpc_atomic_fixed1_eqv
ATOMIC_CMPX_EQV(fixed2, eqv, kmp_int16, 16, ^ ~, 2i, 1,
                KMP_ARCH_X86) // __kmpc_atomic_fixed2_eqv
ATOMIC_CMPX_EQV(fixed4, eqv, kmp_int32, 32, ^ ~, 4i, 3,
                KMP_ARCH_X86) // __kmpc_atomic_fixed4_eqv
ATOMIC_CMPX_EQV(fixed8, eqv, kmp_int64, 64, ^ ~, 8i, 7,
                KMP_ARCH_X86) // __kmpc_atomic_fixed8_eqv

// ------------------------------------------------------------------------
// Routines for Extended types: long double, _Quad, complex flavours (use
// critical section)
//     TYPE_ID, OP_ID, TYPE - detailed above
//     OP      - operator
//     LCK_ID  - lock identifier, used to possibly distinguish lock variable

/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL(float10, add, long double, +, 10r,
                1) // __kmpc_atomic_float10_add
ATOMIC_CRITICAL(float10, sub, long double, -, 10r,
                1) // __kmpc_atomic_float10_sub
ATOMIC_CRITICAL(float10, mul, long double, *, 10r,
                1) // __kmpc_atomic_float10_mul
ATOMIC_CRITICAL(float10, div, long double, /, 10r,
                1) // __kmpc_atomic_float10_div
#if 0
#if KMP_HAVE_QUAD
// routines for _Quad type
ATOMIC_CRITICAL(float16, add, QUAD_LEGACY, +, 16r, 1)                    // __kmpc_atomic_float16_add
ATOMIC_CRITICAL(float16, sub, QUAD_LEGACY, -, 16r, 1)                    // __kmpc_atomic_float16_sub
ATOMIC_CRITICAL(float16, mul, QUAD_LEGACY, *, 16r, 1)                    // __kmpc_atomic_float16_mul
ATOMIC_CRITICAL(float16, div, QUAD_LEGACY, /, 16r, 1)                    // __kmpc_atomic_float16_div
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL(float16, add_a16, Quad_a16_t, +, 16r, 1)                 // __kmpc_atomic_float16_add_a16
ATOMIC_CRITICAL(float16, sub_a16, Quad_a16_t, -, 16r, 1)                 // __kmpc_atomic_float16_sub_a16
ATOMIC_CRITICAL(float16, mul_a16, Quad_a16_t, *, 16r, 1)                 // __kmpc_atomic_float16_mul_a16
ATOMIC_CRITICAL(float16, div_a16, Quad_a16_t, /, 16r, 1)                 // __kmpc_atomic_float16_div_a16
#endif
#endif

/* routines for complex */
#if USE_CMPXCHG_FIX
// workaround for C78287 (complex(kind=4) data type)
ATOMIC_CMPXCHG_WORKAROUND(cmplx4, add, kmp_cmplx32, 64, +, 8c, 7, 1)     // __kmpc_atomic_cmplx4_add
ATOMIC_CMPXCHG_WORKAROUND(cmplx4, sub, kmp_cmplx32, 64, -, 8c, 7, 1)     // __kmpc_atomic_cmplx4_sub
ATOMIC_CMPXCHG_WORKAROUND(cmplx4, mul, kmp_cmplx32, 64, *, 8c, 7, 1)     // __kmpc_atomic_cmplx4_mul
ATOMIC_CMPXCHG_WORKAROUND(cmplx4, div, kmp_cmplx32, 64, /, 8c, 7, 1)     // __kmpc_atomic_cmplx4_div
// end of the workaround for C78287
#else
ATOMIC_CRITICAL(cmplx4, add, kmp_cmplx32, +, 8c, 1)                      // __kmpc_atomic_cmplx4_add
ATOMIC_CRITICAL(cmplx4, sub, kmp_cmplx32, -, 8c, 1)                      // __kmpc_atomic_cmplx4_sub
ATOMIC_CRITICAL(cmplx4, mul, kmp_cmplx32, *, 8c, 1)                      // __kmpc_atomic_cmplx4_mul
ATOMIC_CRITICAL(cmplx4, div, kmp_cmplx32, /, 8c, 1)                      // __kmpc_atomic_cmplx4_div
#endif // USE_CMPXCHG_FIX

ATOMIC_CRITICAL(cmplx8, add, kmp_cmplx64, +, 16c, 1)                     // __kmpc_atomic_cmplx8_add
ATOMIC_CRITICAL(cmplx8, sub, kmp_cmplx64, -, 16c, 1)                     // __kmpc_atomic_cmplx8_sub
ATOMIC_CRITICAL(cmplx8, mul, kmp_cmplx64, *, 16c, 1)                     // __kmpc_atomic_cmplx8_mul
ATOMIC_CRITICAL(cmplx8, div, kmp_cmplx64, /, 16c, 1)                     // __kmpc_atomic_cmplx8_div
ATOMIC_CRITICAL(cmplx10, add, kmp_cmplx80, +, 20c, 1)                    // __kmpc_atomic_cmplx10_add
ATOMIC_CRITICAL(cmplx10, sub, kmp_cmplx80, -, 20c, 1)                    // __kmpc_atomic_cmplx10_sub
ATOMIC_CRITICAL(cmplx10, mul, kmp_cmplx80, *, 20c, 1)                    // __kmpc_atomic_cmplx10_mul
ATOMIC_CRITICAL(cmplx10, div, kmp_cmplx80, /, 20c, 1)                    // __kmpc_atomic_cmplx10_div
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL(cmplx16, add, CPLX128_LEG, +, 32c, 1)                    // __kmpc_atomic_cmplx16_add
ATOMIC_CRITICAL(cmplx16, sub, CPLX128_LEG, -, 32c, 1)                    // __kmpc_atomic_cmplx16_sub
ATOMIC_CRITICAL(cmplx16, mul, CPLX128_LEG, *, 32c, 1)                    // __kmpc_atomic_cmplx16_mul
ATOMIC_CRITICAL(cmplx16, div, CPLX128_LEG, /, 32c, 1)                    // __kmpc_atomic_cmplx16_div
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL(cmplx16, add_a16, kmp_cmplx128_a16_t, +, 32c, 1)         // __kmpc_atomic_cmplx16_add_a16
ATOMIC_CRITICAL(cmplx16, sub_a16, kmp_cmplx128_a16_t, -, 32c, 1)         // __kmpc_atomic_cmplx16_sub_a16
ATOMIC_CRITICAL(cmplx16, mul_a16, kmp_cmplx128_a16_t, *, 32c, 1)         // __kmpc_atomic_cmplx16_mul_a16
ATOMIC_CRITICAL(cmplx16, div_a16, kmp_cmplx128_a16_t, /, 32c, 1)         // __kmpc_atomic_cmplx16_div_a16
#endif
#endif
#endif

// ------------------------------------------------------------------------
// Entries definition for integer operands
//     TYPE_ID - operands type and size (fixed4, float4)
//     OP_ID   - operation identifier (add, sub, mul, ...)
//     TYPE    - operand type
//     BITS    - size in bits, used to distinguish low level calls
//     OP      - operator (used in critical section)
//     LCK_ID  - lock identifier, used to possibly distinguish lock variable

//               TYPE_ID,OP_ID,  TYPE,   BITS,OP,LCK_ID,GOMP_FLAG
// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//                  TYPE_ID,OP_ID, TYPE,    BITS, OP, LCK_ID, GOMP_FLAG
ATOMIC_CMPXCHG_REV(fixed1, div, kmp_int8, 8, /, 1i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_div_rev
ATOMIC_CMPXCHG_REV(fixed1u, div, kmp_uint8, 8, /, 1i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1u_div_rev
ATOMIC_CMPXCHG_REV(fixed1, shl, kmp_int8, 8, <<, 1i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_shl_rev
ATOMIC_CMPXCHG_REV(fixed1, shr, kmp_int8, 8, >>, 1i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_shr_rev
ATOMIC_CMPXCHG_REV(fixed1u, shr, kmp_uint8, 8, >>, 1i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1u_shr_rev
ATOMIC_CMPXCHG_REV(fixed1, sub, kmp_int8, 8, -, 1i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_sub_rev

ATOMIC_CMPXCHG_REV(fixed2, div, kmp_int16, 16, /, 2i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_div_rev
ATOMIC_CMPXCHG_REV(fixed2u, div, kmp_uint16, 16, /, 2i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2u_div_rev
ATOMIC_CMPXCHG_REV(fixed2, shl, kmp_int16, 16, <<, 2i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_shl_rev
ATOMIC_CMPXCHG_REV(fixed2, shr, kmp_int16, 16, >>, 2i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_shr_rev
ATOMIC_CMPXCHG_REV(fixed2u, shr, kmp_uint16, 16, >>, 2i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2u_shr_rev
ATOMIC_CMPXCHG_REV(fixed2, sub, kmp_int16, 16, -, 2i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_sub_rev

ATOMIC_CMPXCHG_REV(fixed4, div, kmp_int32, 32, /, 4i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_div_rev
ATOMIC_CMPXCHG_REV(fixed4u, div, kmp_uint32, 32, /, 4i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4u_div_rev
ATOMIC_CMPXCHG_REV(fixed4, shl, kmp_int32, 32, <<, 4i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_shl_rev
ATOMIC_CMPXCHG_REV(fixed4, shr, kmp_int32, 32, >>, 4i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_shr_rev
ATOMIC_CMPXCHG_REV(fixed4u, shr, kmp_uint32, 32, >>, 4i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4u_shr_rev
ATOMIC_CMPXCHG_REV(fixed4, sub, kmp_int32, 32, -, 4i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_sub_rev

ATOMIC_CMPXCHG_REV(fixed8, div, kmp_int64, 64, /, 8i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_div_rev
ATOMIC_CMPXCHG_REV(fixed8u, div, kmp_uint64, 64, /, 8i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8u_div_rev
ATOMIC_CMPXCHG_REV(fixed8, shl, kmp_int64, 64, <<, 8i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_shl_rev
ATOMIC_CMPXCHG_REV(fixed8, shr, kmp_int64, 64, >>, 8i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_shr_rev
ATOMIC_CMPXCHG_REV(fixed8u, shr, kmp_uint64, 64, >>, 8i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8u_shr_rev
ATOMIC_CMPXCHG_REV(fixed8, sub, kmp_int64, 64, -, 8i,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_sub_rev

ATOMIC_CMPXCHG_REV(float4, div, kmp_real32, 32, /, 4r,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_div_rev
ATOMIC_CMPXCHG_REV(float4, sub, kmp_real32, 32, -, 4r,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_sub_rev

ATOMIC_CMPXCHG_REV(float8, div, kmp_real64, 64, /, 8r,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_div_rev
ATOMIC_CMPXCHG_REV(float8, sub, kmp_real64, 64, -, 8r,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_sub_rev
//                  TYPE_ID,OP_ID, TYPE,     BITS,OP,LCK_ID, GOMP_FLAG

/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL_REV(float10, sub, long double, -, 10r,
                    1) // __kmpc_atomic_float10_sub_rev
ATOMIC_CRITICAL_REV(float10, div, long double, /, 10r,
                    1) // __kmpc_atomic_float10_div_rev
#if 0
#if KMP_HAVE_QUAD
// routines for _Quad type
ATOMIC_CRITICAL_REV(float16, sub, QUAD_LEGACY, -, 16r, 1)                    // __kmpc_atomic_float16_sub_rev
ATOMIC_CRITICAL_REV(float16, div, QUAD_LEGACY, /, 16r, 1)                    // __kmpc_atomic_float16_div_rev
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL_REV(float16, sub_a16, Quad_a16_t, -, 16r, 1)                 // __kmpc_atomic_float16_sub_a16_rev
ATOMIC_CRITICAL_REV(float16, div_a16, Quad_a16_t, /, 16r, 1)                 // __kmpc_atomic_float16_div_a16_rev
#endif
#endif

// routines for complex types
ATOMIC_CRITICAL_REV(cmplx4, sub, kmp_cmplx32, -, 8c, 1)                      // __kmpc_atomic_cmplx4_sub_rev
ATOMIC_CRITICAL_REV(cmplx4, div, kmp_cmplx32, /, 8c, 1)                      // __kmpc_atomic_cmplx4_div_rev
ATOMIC_CRITICAL_REV(cmplx8, sub, kmp_cmplx64, -, 16c, 1)                     // __kmpc_atomic_cmplx8_sub_rev
ATOMIC_CRITICAL_REV(cmplx8, div, kmp_cmplx64, /, 16c, 1)                     // __kmpc_atomic_cmplx8_div_rev
ATOMIC_CRITICAL_REV(cmplx10, sub, kmp_cmplx80, -, 20c, 1)                    // __kmpc_atomic_cmplx10_sub_rev
ATOMIC_CRITICAL_REV(cmplx10, div, kmp_cmplx80, /, 20c, 1)                    // __kmpc_atomic_cmplx10_div_rev
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_REV(cmplx16, sub, CPLX128_LEG, -, 32c, 1)                    // __kmpc_atomic_cmplx16_sub_rev
ATOMIC_CRITICAL_REV(cmplx16, div, CPLX128_LEG, /, 32c, 1)                    // __kmpc_atomic_cmplx16_div_rev
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL_REV(cmplx16, sub_a16, kmp_cmplx128_a16_t, -, 32c, 1)         // __kmpc_atomic_cmplx16_sub_a16_rev
ATOMIC_CRITICAL_REV(cmplx16, div_a16, kmp_cmplx128_a16_t, /, 32c, 1)         // __kmpc_atomic_cmplx16_div_a16_rev
#endif
#endif
#endif

/* ------------------------------------------------------------------------ */
/* Routines for mixed types of LHS and RHS, when RHS is "larger"            */
/* Note: in order to reduce the total number of types combinations          */
/*       it is supposed that compiler converts RHS to longest floating type,*/
/*       that is _Quad, before call to any of these routines                */
/* Conversion to _Quad will be done by the compiler during calculation,     */
/*    conversion back to TYPE - before the assignment, like:                */
/*    *lhs = (TYPE)( (_Quad)(*lhs) OP rhs )                                 */
/* Performance penalty expected because of SW emulation use                 */
/* ------------------------------------------------------------------------ */
// RHS=float8
ATOMIC_CMPXCHG_MIX(fixed1, char, mul, 8, *, float8, kmp_real64, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_mul_float8
ATOMIC_CMPXCHG_MIX(fixed1, char, div, 8, /, float8, kmp_real64, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_div_float8
ATOMIC_CMPXCHG_MIX(fixed2, short, mul, 16, *, float8, kmp_real64, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_mul_float8
ATOMIC_CMPXCHG_MIX(fixed2, short, div, 16, /, float8, kmp_real64, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_div_float8
ATOMIC_CMPXCHG_MIX(fixed4, kmp_int32, mul, 32, *, float8, kmp_real64, 4i, 3,
                   0)            // __kmpc_atomic_fixed4_mul_float8
ATOMIC_CMPXCHG_MIX(fixed4, kmp_int32, div, 32, /, float8, kmp_real64, 4i, 3,
                   0)            // __kmpc_atomic_fixed4_div_float8
ATOMIC_CMPXCHG_MIX(fixed8, kmp_int64, mul, 64, *, float8, kmp_real64, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_mul_float8
ATOMIC_CMPXCHG_MIX(fixed8, kmp_int64, div, 64, /, float8, kmp_real64, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_div_float8
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, add, 32, +, float8, kmp_real64, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_add_float8
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, sub, 32, -, float8, kmp_real64, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_sub_float8
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, mul, 32, *, float8, kmp_real64, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_mul_float8
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, div, 32, /, float8, kmp_real64, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_div_float8

// RHS=float16 (deprecated, to be removed when we are sure the compiler does not
// use them)
#if KMP_HAVE_QUAD
ATOMIC_CMPXCHG_MIX(fixed1, char, add, 8, +, fp, _Quad, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_add_fp
ATOMIC_CMPXCHG_MIX(fixed1, char, sub, 8, -, fp, _Quad, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_sub_fp
ATOMIC_CMPXCHG_MIX(fixed1, char, mul, 8, *, fp, _Quad, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_mul_fp
ATOMIC_CMPXCHG_MIX(fixed1, char, div, 8, /, fp, _Quad, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_div_fp
ATOMIC_CMPXCHG_MIX(fixed1u, uchar, div, 8, /, fp, _Quad, 1i, 0,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1u_div_fp

ATOMIC_CMPXCHG_MIX(fixed2, short, add, 16, +, fp, _Quad, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_add_fp
ATOMIC_CMPXCHG_MIX(fixed2, short, sub, 16, -, fp, _Quad, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_sub_fp
ATOMIC_CMPXCHG_MIX(fixed2, short, mul, 16, *, fp, _Quad, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_mul_fp
ATOMIC_CMPXCHG_MIX(fixed2, short, div, 16, /, fp, _Quad, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_div_fp
ATOMIC_CMPXCHG_MIX(fixed2u, ushort, div, 16, /, fp, _Quad, 2i, 1,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2u_div_fp

ATOMIC_CMPXCHG_MIX(fixed4, kmp_int32, add, 32, +, fp, _Quad, 4i, 3,
                   0) // __kmpc_atomic_fixed4_add_fp
ATOMIC_CMPXCHG_MIX(fixed4, kmp_int32, sub, 32, -, fp, _Quad, 4i, 3,
                   0) // __kmpc_atomic_fixed4_sub_fp
ATOMIC_CMPXCHG_MIX(fixed4, kmp_int32, mul, 32, *, fp, _Quad, 4i, 3,
                   0) // __kmpc_atomic_fixed4_mul_fp
ATOMIC_CMPXCHG_MIX(fixed4, kmp_int32, div, 32, /, fp, _Quad, 4i, 3,
                   0) // __kmpc_atomic_fixed4_div_fp
ATOMIC_CMPXCHG_MIX(fixed4u, kmp_uint32, div, 32, /, fp, _Quad, 4i, 3,
                   0) // __kmpc_atomic_fixed4u_div_fp

ATOMIC_CMPXCHG_MIX(fixed8, kmp_int64, add, 64, +, fp, _Quad, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_add_fp
ATOMIC_CMPXCHG_MIX(fixed8, kmp_int64, sub, 64, -, fp, _Quad, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_sub_fp
ATOMIC_CMPXCHG_MIX(fixed8, kmp_int64, mul, 64, *, fp, _Quad, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_mul_fp
ATOMIC_CMPXCHG_MIX(fixed8, kmp_int64, div, 64, /, fp, _Quad, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_div_fp
ATOMIC_CMPXCHG_MIX(fixed8u, kmp_uint64, div, 64, /, fp, _Quad, 8i, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8u_div_fp

ATOMIC_CMPXCHG_MIX(float4, kmp_real32, add, 32, +, fp, _Quad, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_add_fp
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, sub, 32, -, fp, _Quad, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_sub_fp
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, mul, 32, *, fp, _Quad, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_mul_fp
ATOMIC_CMPXCHG_MIX(float4, kmp_real32, div, 32, /, fp, _Quad, 4r, 3,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_div_fp

ATOMIC_CMPXCHG_MIX(float8, kmp_real64, add, 64, +, fp, _Quad, 8r, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_add_fp
ATOMIC_CMPXCHG_MIX(float8, kmp_real64, sub, 64, -, fp, _Quad, 8r, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_sub_fp
ATOMIC_CMPXCHG_MIX(float8, kmp_real64, mul, 64, *, fp, _Quad, 8r, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_mul_fp
ATOMIC_CMPXCHG_MIX(float8, kmp_real64, div, 64, /, fp, _Quad, 8r, 7,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_div_fp
#endif

#if 0
ATOMIC_CRITICAL_FP(float10, long double, add, +, fp, _Quad, 10r, 1)                             // __kmpc_atomic_float10_add_fp
ATOMIC_CRITICAL_FP(float10, long double, sub, -, fp, _Quad, 10r, 1)                             // __kmpc_atomic_float10_sub_fp
ATOMIC_CRITICAL_FP(float10, long double, mul, *, fp, _Quad, 10r, 1)                             // __kmpc_atomic_float10_mul_fp
ATOMIC_CRITICAL_FP(float10, long double, div, /, fp, _Quad, 10r, 1)                             // __kmpc_atomic_float10_div_fp

ATOMIC_CMPXCHG_CMPLX(cmplx4, kmp_cmplx32, add, 64, +, cmplx8, kmp_cmplx64, 8c, 7, KMP_ARCH_X86) // __kmpc_atomic_cmplx4_add_cmplx8
ATOMIC_CMPXCHG_CMPLX(cmplx4, kmp_cmplx32, sub, 64, -, cmplx8, kmp_cmplx64, 8c, 7, KMP_ARCH_X86) // __kmpc_atomic_cmplx4_sub_cmplx8
ATOMIC_CMPXCHG_CMPLX(cmplx4, kmp_cmplx32, mul, 64, *, cmplx8, kmp_cmplx64, 8c, 7, KMP_ARCH_X86) // __kmpc_atomic_cmplx4_mul_cmplx8
ATOMIC_CMPXCHG_CMPLX(cmplx4, kmp_cmplx32, div, 64, /, cmplx8, kmp_cmplx64, 8c, 7, KMP_ARCH_X86) // __kmpc_atomic_cmplx4_div_cmplx8
#endif

// READ, WRITE, CAPTURE are supported only on IA-32 architecture and Intel(R) 64
//////////////////////////////////////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------
// Atomic READ routines
// ------------------------------------------------------------------------
//                  TYPE_ID,OP_ID, TYPE,      OP, GOMP_FLAG
ATOMIC_FIXED_READ(fixed4, rd, kmp_int32, 32, +, 0) // __kmpc_atomic_fixed4_rd
ATOMIC_FIXED_READ(fixed8, rd, kmp_int64, 64, +,
                  KMP_ARCH_X86)                    // __kmpc_atomic_fixed8_rd
ATOMIC_CMPXCHG_READ(float4, rd, kmp_real32, 32, +,
                    KMP_ARCH_X86)                  // __kmpc_atomic_float4_rd
ATOMIC_CMPXCHG_READ(float8, rd, kmp_real64, 64, +,
                    KMP_ARCH_X86)                  // __kmpc_atomic_float8_rd

// !!! TODO: Remove lock operations for "char" since it can't be non-atomic
ATOMIC_CMPXCHG_READ(fixed1, rd, kmp_int8, 8, +,
                    KMP_ARCH_X86) // __kmpc_atomic_fixed1_rd
ATOMIC_CMPXCHG_READ(fixed2, rd, kmp_int16, 16, +,
                    KMP_ARCH_X86) // __kmpc_atomic_fixed2_rd

ATOMIC_CRITICAL_READ(float10, rd, long double, +, 10r,
                     1) // __kmpc_atomic_float10_rd
#if 0
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_READ(float16, rd, QUAD_LEGACY, +, 16r, 1)             // __kmpc_atomic_float16_rd
#endif // KMP_HAVE_QUAD

// Fix for CQ220361 on Windows* OS
#if (KMP_OS_WINDOWS)
ATOMIC_CRITICAL_READ_WRK(cmplx4, rd, kmp_cmplx32, +, 8c, 1)           // __kmpc_atomic_cmplx4_rd
#else
ATOMIC_CRITICAL_READ(cmplx4, rd, kmp_cmplx32, +, 8c, 1)               // __kmpc_atomic_cmplx4_rd
#endif
ATOMIC_CRITICAL_READ(cmplx8, rd, kmp_cmplx64, +, 16c, 1)              // __kmpc_atomic_cmplx8_rd
ATOMIC_CRITICAL_READ(cmplx10, rd, kmp_cmplx80, +, 20c, 1)             // __kmpc_atomic_cmplx10_rd
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_READ(cmplx16, rd, CPLX128_LEG, +, 32c, 1)             // __kmpc_atomic_cmplx16_rd
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL_READ(float16, a16_rd, Quad_a16_t, +, 16r, 1)          // __kmpc_atomic_float16_a16_rd
ATOMIC_CRITICAL_READ(cmplx16, a16_rd, kmp_cmplx128_a16_t, +, 32c, 1)  // __kmpc_atomic_cmplx16_a16_rd
#endif
#endif
#endif

// ------------------------------------------------------------------------
// Atomic WRITE routines
// ------------------------------------------------------------------------
ATOMIC_XCHG_WR(fixed1, wr, kmp_int8, 8, =,
               KMP_ARCH_X86) // __kmpc_atomic_fixed1_wr
ATOMIC_XCHG_WR(fixed2, wr, kmp_int16, 16, =,
               KMP_ARCH_X86) // __kmpc_atomic_fixed2_wr
// ATOMIC_XCHG_WR( fixed4,  wr, kmp_int32,  32, =,  KMP_ARCH_X86 )  //
// __kmpc_atomic_fixed4_wr
#if (KMP_ARCH_X86)
ATOMIC_CMPXCHG_WR(fixed8, wr, kmp_int64, 64, =,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed8_wr
#else
ATOMIC_XCHG_WR(fixed8, wr, kmp_int64, 64, =,
               KMP_ARCH_X86) // __kmpc_atomic_fixed8_wr
#endif

ATOMIC_XCHG_FLOAT_WR(float4, wr, kmp_real32, 32, =,
                     KMP_ARCH_X86) // __kmpc_atomic_float4_wr
#if (KMP_ARCH_X86)
ATOMIC_CMPXCHG_WR(float8, wr, kmp_real64, 64, =,
                  KMP_ARCH_X86) // __kmpc_atomic_float8_wr
#else
ATOMIC_XCHG_FLOAT_WR(float8, wr, kmp_real64, 64, =,
                     KMP_ARCH_X86) // __kmpc_atomic_float8_wr
#endif

ATOMIC_CRITICAL_WR(float10, wr, long double, =, 10r,
                   1) // __kmpc_atomic_float10_wr
#if 0
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_WR(float16, wr, QUAD_LEGACY, =, 16r, 1)             // __kmpc_atomic_float16_wr
#endif
ATOMIC_CRITICAL_WR(cmplx4, wr, kmp_cmplx32, =, 8c, 1)               // __kmpc_atomic_cmplx4_wr
ATOMIC_CRITICAL_WR(cmplx8, wr, kmp_cmplx64, =, 16c, 1)              // __kmpc_atomic_cmplx8_wr
ATOMIC_CRITICAL_WR(cmplx10, wr, kmp_cmplx80, =, 20c, 1)             // __kmpc_atomic_cmplx10_wr
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_WR(cmplx16, wr, CPLX128_LEG, =, 32c, 1)             // __kmpc_atomic_cmplx16_wr
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL_WR(float16, a16_wr, Quad_a16_t, =, 16r, 1)          // __kmpc_atomic_float16_a16_wr
ATOMIC_CRITICAL_WR(cmplx16, a16_wr, kmp_cmplx128_a16_t, =, 32c, 1)  // __kmpc_atomic_cmplx16_a16_wr
#endif
#endif
#endif

// ------------------------------------------------------------------------
// Atomic CAPTURE routines
// ------------------------------------------------------------------------
ATOMIC_FIXED_ADD_CPT(fixed4, add_cpt, kmp_int32, 32, +,
                     0)            // __kmpc_atomic_fixed4_add_cpt
ATOMIC_FIXED_ADD_CPT(fixed4, sub_cpt, kmp_int32, 32, -,
                     0)            // __kmpc_atomic_fixed4_sub_cpt
ATOMIC_FIXED_ADD_CPT(fixed8, add_cpt, kmp_int64, 64, +,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed8_add_cpt
ATOMIC_FIXED_ADD_CPT(fixed8, sub_cpt, kmp_int64, 64, -,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed8_sub_cpt

ATOMIC_CMPXCHG_CPT(float4, add_cpt, kmp_real32, 32, +,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_add_cpt
ATOMIC_CMPXCHG_CPT(float4, sub_cpt, kmp_real32, 32, -,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_sub_cpt
ATOMIC_CMPXCHG_CPT(float8, add_cpt, kmp_real64, 64, +,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_add_cpt
ATOMIC_CMPXCHG_CPT(float8, sub_cpt, kmp_real64, 64, -,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_sub_cpt

// ------------------------------------------------------------------------
// Entries definition for integer operands
//     TYPE_ID - operands type and size (fixed4, float4)
//     OP_ID   - operation identifier (add, sub, mul, ...)
//     TYPE    - operand type
//     BITS    - size in bits, used to distinguish low level calls
//     OP      - operator (used in critical section)
//               TYPE_ID,OP_ID,  TYPE,   BITS,OP,GOMP_FLAG
// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//                  TYPE_ID,  OP_ID,   TYPE,         OP,  GOMP_FLAG
ATOMIC_CMPXCHG_CPT(fixed1, add_cpt, kmp_int8, 8, +,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_add_cpt
ATOMIC_CMPXCHG_CPT(fixed1, andb_cpt, kmp_int8, 8, &,
                   0)            // __kmpc_atomic_fixed1_andb_cpt
ATOMIC_CMPXCHG_CPT(fixed1, div_cpt, kmp_int8, 8, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_div_cpt
ATOMIC_CMPXCHG_CPT(fixed1u, div_cpt, kmp_uint8, 8, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1u_div_cpt
ATOMIC_CMPXCHG_CPT(fixed1, mul_cpt, kmp_int8, 8, *,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_mul_cpt
ATOMIC_CMPXCHG_CPT(fixed1, orb_cpt, kmp_int8, 8, |,
                   0)            // __kmpc_atomic_fixed1_orb_cpt
ATOMIC_CMPXCHG_CPT(fixed1, shl_cpt, kmp_int8, 8, <<,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_shl_cpt
ATOMIC_CMPXCHG_CPT(fixed1, shr_cpt, kmp_int8, 8, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed1u, shr_cpt, kmp_uint8, 8, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1u_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed1, sub_cpt, kmp_int8, 8, -,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed1_sub_cpt
ATOMIC_CMPXCHG_CPT(fixed1, xor_cpt, kmp_int8, 8, ^,
                   0)            // __kmpc_atomic_fixed1_xor_cpt
ATOMIC_CMPXCHG_CPT(fixed2, add_cpt, kmp_int16, 16, +,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_add_cpt
ATOMIC_CMPXCHG_CPT(fixed2, andb_cpt, kmp_int16, 16, &,
                   0)            // __kmpc_atomic_fixed2_andb_cpt
ATOMIC_CMPXCHG_CPT(fixed2, div_cpt, kmp_int16, 16, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_div_cpt
ATOMIC_CMPXCHG_CPT(fixed2u, div_cpt, kmp_uint16, 16, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2u_div_cpt
ATOMIC_CMPXCHG_CPT(fixed2, mul_cpt, kmp_int16, 16, *,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_mul_cpt
ATOMIC_CMPXCHG_CPT(fixed2, orb_cpt, kmp_int16, 16, |,
                   0)            // __kmpc_atomic_fixed2_orb_cpt
ATOMIC_CMPXCHG_CPT(fixed2, shl_cpt, kmp_int16, 16, <<,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_shl_cpt
ATOMIC_CMPXCHG_CPT(fixed2, shr_cpt, kmp_int16, 16, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed2u, shr_cpt, kmp_uint16, 16, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2u_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed2, sub_cpt, kmp_int16, 16, -,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed2_sub_cpt
ATOMIC_CMPXCHG_CPT(fixed2, xor_cpt, kmp_int16, 16, ^,
                   0)            // __kmpc_atomic_fixed2_xor_cpt
ATOMIC_CMPXCHG_CPT(fixed4, andb_cpt, kmp_int32, 32, &,
                   0)            // __kmpc_atomic_fixed4_andb_cpt
ATOMIC_CMPXCHG_CPT(fixed4, div_cpt, kmp_int32, 32, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_div_cpt
ATOMIC_CMPXCHG_CPT(fixed4u, div_cpt, kmp_uint32, 32, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4u_div_cpt
ATOMIC_CMPXCHG_CPT(fixed4, mul_cpt, kmp_int32, 32, *,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_mul_cpt
ATOMIC_CMPXCHG_CPT(fixed4, orb_cpt, kmp_int32, 32, |,
                   0)            // __kmpc_atomic_fixed4_orb_cpt
ATOMIC_CMPXCHG_CPT(fixed4, shl_cpt, kmp_int32, 32, <<,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_shl_cpt
ATOMIC_CMPXCHG_CPT(fixed4, shr_cpt, kmp_int32, 32, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed4u, shr_cpt, kmp_uint32, 32, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed4u_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed4, xor_cpt, kmp_int32, 32, ^,
                   0)            // __kmpc_atomic_fixed4_xor_cpt
ATOMIC_CMPXCHG_CPT(fixed8, andb_cpt, kmp_int64, 64, &,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_andb_cpt
ATOMIC_CMPXCHG_CPT(fixed8, div_cpt, kmp_int64, 64, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_div_cpt
ATOMIC_CMPXCHG_CPT(fixed8u, div_cpt, kmp_uint64, 64, /,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8u_div_cpt
ATOMIC_CMPXCHG_CPT(fixed8, mul_cpt, kmp_int64, 64, *,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_mul_cpt
ATOMIC_CMPXCHG_CPT(fixed8, orb_cpt, kmp_int64, 64, |,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_orb_cpt
ATOMIC_CMPXCHG_CPT(fixed8, shl_cpt, kmp_int64, 64, <<,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_shl_cpt
ATOMIC_CMPXCHG_CPT(fixed8, shr_cpt, kmp_int64, 64, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed8u, shr_cpt, kmp_uint64, 64, >>,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8u_shr_cpt
ATOMIC_CMPXCHG_CPT(fixed8, xor_cpt, kmp_int64, 64, ^,
                   KMP_ARCH_X86) // __kmpc_atomic_fixed8_xor_cpt
ATOMIC_CMPXCHG_CPT(float4, div_cpt, kmp_real32, 32, /,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_div_cpt
ATOMIC_CMPXCHG_CPT(float4, mul_cpt, kmp_real32, 32, *,
                   KMP_ARCH_X86) // __kmpc_atomic_float4_mul_cpt
ATOMIC_CMPXCHG_CPT(float8, div_cpt, kmp_real64, 64, /,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_div_cpt
ATOMIC_CMPXCHG_CPT(float8, mul_cpt, kmp_real64, 64, *,
                   KMP_ARCH_X86) // __kmpc_atomic_float8_mul_cpt
// ------------------------------------------------------------------------
// Routines for C/C++ Reduction operators && and ||
// ------------------------------------------------------------------------
ATOMIC_CMPX_L_CPT(fixed1, andl_cpt, char, 8, &&,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed1_andl_cpt
ATOMIC_CMPX_L_CPT(fixed1, orl_cpt, char, 8, ||,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed1_orl_cpt
ATOMIC_CMPX_L_CPT(fixed2, andl_cpt, short, 16, &&,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed2_andl_cpt
ATOMIC_CMPX_L_CPT(fixed2, orl_cpt, short, 16, ||,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed2_orl_cpt
ATOMIC_CMPX_L_CPT(fixed4, andl_cpt, kmp_int32, 32, &&,
                  0)            // __kmpc_atomic_fixed4_andl_cpt
ATOMIC_CMPX_L_CPT(fixed4, orl_cpt, kmp_int32, 32, ||,
                  0)            // __kmpc_atomic_fixed4_orl_cpt
ATOMIC_CMPX_L_CPT(fixed8, andl_cpt, kmp_int64, 64, &&,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed8_andl_cpt
ATOMIC_CMPX_L_CPT(fixed8, orl_cpt, kmp_int64, 64, ||,
                  KMP_ARCH_X86) // __kmpc_atomic_fixed8_orl_cpt

// -------------------------------------------------------------------------
// Routines for Fortran operators that matched no one in C:
// MAX, MIN, .EQV., .NEQV.
// Operators .AND., .OR. are covered by __kmpc_atomic_*_{andl,orl}_cpt
// Intrinsics IAND, IOR, IEOR are covered by __kmpc_atomic_*_{andb,orb,xor}_cpt
// -------------------------------------------------------------------------
MIN_MAX_COMPXCHG_CPT(fixed1, max_cpt, char, 8, <,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed1_max_cpt
MIN_MAX_COMPXCHG_CPT(fixed1, min_cpt, char, 8, >,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed1_min_cpt
MIN_MAX_COMPXCHG_CPT(fixed2, max_cpt, short, 16, <,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed2_max_cpt
MIN_MAX_COMPXCHG_CPT(fixed2, min_cpt, short, 16, >,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed2_min_cpt
MIN_MAX_COMPXCHG_CPT(fixed4, max_cpt, kmp_int32, 32, <,
                     0)            // __kmpc_atomic_fixed4_max_cpt
MIN_MAX_COMPXCHG_CPT(fixed4, min_cpt, kmp_int32, 32, >,
                     0)            // __kmpc_atomic_fixed4_min_cpt
MIN_MAX_COMPXCHG_CPT(fixed8, max_cpt, kmp_int64, 64, <,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed8_max_cpt
MIN_MAX_COMPXCHG_CPT(fixed8, min_cpt, kmp_int64, 64, >,
                     KMP_ARCH_X86) // __kmpc_atomic_fixed8_min_cpt
MIN_MAX_COMPXCHG_CPT(float4, max_cpt, kmp_real32, 32, <,
                     KMP_ARCH_X86) // __kmpc_atomic_float4_max_cpt
MIN_MAX_COMPXCHG_CPT(float4, min_cpt, kmp_real32, 32, >,
                     KMP_ARCH_X86) // __kmpc_atomic_float4_min_cpt
MIN_MAX_COMPXCHG_CPT(float8, max_cpt, kmp_real64, 64, <,
                     KMP_ARCH_X86) // __kmpc_atomic_float8_max_cpt
MIN_MAX_COMPXCHG_CPT(float8, min_cpt, kmp_real64, 64, >,
                     KMP_ARCH_X86) // __kmpc_atomic_float8_min_cpt
#if 0
#if KMP_HAVE_QUAD
MIN_MAX_CRITICAL_CPT(float16, max_cpt, QUAD_LEGACY, <, 16r, 1)            // __kmpc_atomic_float16_max_cpt
MIN_MAX_CRITICAL_CPT(float16, min_cpt, QUAD_LEGACY, >, 16r, 1)            // __kmpc_atomic_float16_min_cpt
#if (KMP_ARCH_X86)
MIN_MAX_CRITICAL_CPT(float16, max_a16_cpt, Quad_a16_t, <, 16r, 1)         // __kmpc_atomic_float16_max_a16_cpt
MIN_MAX_CRITICAL_CPT(float16, min_a16_cpt, Quad_a16_t, >, 16r, 1)         // __kmpc_atomic_float16_mix_a16_cpt
#endif
#endif
#endif

ATOMIC_CMPXCHG_CPT(fixed1, neqv_cpt, kmp_int8, 8, ^,
                   KMP_ARCH_X86)  // __kmpc_atomic_fixed1_neqv_cpt
ATOMIC_CMPXCHG_CPT(fixed2, neqv_cpt, kmp_int16, 16, ^,
                   KMP_ARCH_X86)  // __kmpc_atomic_fixed2_neqv_cpt
ATOMIC_CMPXCHG_CPT(fixed4, neqv_cpt, kmp_int32, 32, ^,
                   KMP_ARCH_X86)  // __kmpc_atomic_fixed4_neqv_cpt
ATOMIC_CMPXCHG_CPT(fixed8, neqv_cpt, kmp_int64, 64, ^,
                   KMP_ARCH_X86)  // __kmpc_atomic_fixed8_neqv_cpt
ATOMIC_CMPX_EQV_CPT(fixed1, eqv_cpt, kmp_int8, 8, ^ ~,
                    KMP_ARCH_X86) // __kmpc_atomic_fixed1_eqv_cpt
ATOMIC_CMPX_EQV_CPT(fixed2, eqv_cpt, kmp_int16, 16, ^ ~,
                    KMP_ARCH_X86) // __kmpc_atomic_fixed2_eqv_cpt
ATOMIC_CMPX_EQV_CPT(fixed4, eqv_cpt, kmp_int32, 32, ^ ~,
                    KMP_ARCH_X86) // __kmpc_atomic_fixed4_eqv_cpt
ATOMIC_CMPX_EQV_CPT(fixed8, eqv_cpt, kmp_int64, 64, ^ ~,
                    KMP_ARCH_X86) // __kmpc_atomic_fixed8_eqv_cpt

// ------------------------------------------------------------------------
// Routines for Extended types: long double, _Quad, complex flavours (use
// critical section)
/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL_CPT(float10, add_cpt, long double, +, 10r,
                    1) // __kmpc_atomic_float10_add_cpt
ATOMIC_CRITICAL_CPT(float10, sub_cpt, long double, -, 10r,
                    1) // __kmpc_atomic_float10_sub_cpt
ATOMIC_CRITICAL_CPT(float10, mul_cpt, long double, *, 10r,
                    1) // __kmpc_atomic_float10_mul_cpt
ATOMIC_CRITICAL_CPT(float10, div_cpt, long double, /, 10r,
                    1) // __kmpc_atomic_float10_div_cpt
#if 0
#if KMP_HAVE_QUAD
// routines for _Quad type
ATOMIC_CRITICAL_CPT(float16, add_cpt, QUAD_LEGACY, +, 16r, 1)                    // __kmpc_atomic_float16_add_cpt
ATOMIC_CRITICAL_CPT(float16, sub_cpt, QUAD_LEGACY, -, 16r, 1)                    // __kmpc_atomic_float16_sub_cpt
ATOMIC_CRITICAL_CPT(float16, mul_cpt, QUAD_LEGACY, *, 16r, 1)                    // __kmpc_atomic_float16_mul_cpt
ATOMIC_CRITICAL_CPT(float16, div_cpt, QUAD_LEGACY, /, 16r, 1)                    // __kmpc_atomic_float16_div_cpt
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL_CPT(float16, add_a16_cpt, Quad_a16_t, +, 16r, 1)                 // __kmpc_atomic_float16_add_a16_cpt
ATOMIC_CRITICAL_CPT(float16, sub_a16_cpt, Quad_a16_t, -, 16r, 1)                 // __kmpc_atomic_float16_sub_a16_cpt
ATOMIC_CRITICAL_CPT(float16, mul_a16_cpt, Quad_a16_t, *, 16r, 1)                 // __kmpc_atomic_float16_mul_a16_cpt
ATOMIC_CRITICAL_CPT(float16, div_a16_cpt, Quad_a16_t, /, 16r, 1)                 // __kmpc_atomic_float16_div_a16_cpt
#endif
#endif

// routines for complex types

// cmplx4 routines to return void
ATOMIC_CRITICAL_CPT_WRK(cmplx4, add_cpt, kmp_cmplx32, +, 8c, 1)              // __kmpc_atomic_cmplx4_add_cpt
ATOMIC_CRITICAL_CPT_WRK(cmplx4, sub_cpt, kmp_cmplx32, -, 8c, 1)              // __kmpc_atomic_cmplx4_sub_cpt
ATOMIC_CRITICAL_CPT_WRK(cmplx4, mul_cpt, kmp_cmplx32, *, 8c, 1)              // __kmpc_atomic_cmplx4_mul_cpt
ATOMIC_CRITICAL_CPT_WRK(cmplx4, div_cpt, kmp_cmplx32, /, 8c, 1)              // __kmpc_atomic_cmplx4_div_cpt

ATOMIC_CRITICAL_CPT(cmplx8, add_cpt, kmp_cmplx64, +, 16c, 1)                 // __kmpc_atomic_cmplx8_add_cpt
ATOMIC_CRITICAL_CPT(cmplx8, sub_cpt, kmp_cmplx64, -, 16c, 1)                 // __kmpc_atomic_cmplx8_sub_cpt
ATOMIC_CRITICAL_CPT(cmplx8, mul_cpt, kmp_cmplx64, *, 16c, 1)                 // __kmpc_atomic_cmplx8_mul_cpt
ATOMIC_CRITICAL_CPT(cmplx8, div_cpt, kmp_cmplx64, /, 16c, 1)                 // __kmpc_atomic_cmplx8_div_cpt
ATOMIC_CRITICAL_CPT(cmplx10, add_cpt, kmp_cmplx80, +, 20c, 1)                // __kmpc_atomic_cmplx10_add_cpt
ATOMIC_CRITICAL_CPT(cmplx10, sub_cpt, kmp_cmplx80, -, 20c, 1)                // __kmpc_atomic_cmplx10_sub_cpt
ATOMIC_CRITICAL_CPT(cmplx10, mul_cpt, kmp_cmplx80, *, 20c, 1)                // __kmpc_atomic_cmplx10_mul_cpt
ATOMIC_CRITICAL_CPT(cmplx10, div_cpt, kmp_cmplx80, /, 20c, 1)                // __kmpc_atomic_cmplx10_div_cpt
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_CPT(cmplx16, add_cpt, CPLX128_LEG, +, 32c, 1)                // __kmpc_atomic_cmplx16_add_cpt
ATOMIC_CRITICAL_CPT(cmplx16, sub_cpt, CPLX128_LEG, -, 32c, 1)                // __kmpc_atomic_cmplx16_sub_cpt
ATOMIC_CRITICAL_CPT(cmplx16, mul_cpt, CPLX128_LEG, *, 32c, 1)                // __kmpc_atomic_cmplx16_mul_cpt
ATOMIC_CRITICAL_CPT(cmplx16, div_cpt, CPLX128_LEG, /, 32c, 1)                // __kmpc_atomic_cmplx16_div_cpt
#if (KMP_ARCH_X86)
ATOMIC_CRITICAL_CPT(cmplx16, add_a16_cpt, kmp_cmplx128_a16_t, +, 32c, 1)     // __kmpc_atomic_cmplx16_add_a16_cpt
ATOMIC_CRITICAL_CPT(cmplx16, sub_a16_cpt, kmp_cmplx128_a16_t, -, 32c, 1)     // __kmpc_atomic_cmplx16_sub_a16_cpt
ATOMIC_CRITICAL_CPT(cmplx16, mul_a16_cpt, kmp_cmplx128_a16_t, *, 32c, 1)     // __kmpc_atomic_cmplx16_mul_a16_cpt
ATOMIC_CRITICAL_CPT(cmplx16, div_a16_cpt, kmp_cmplx128_a16_t, /, 32c, 1)     // __kmpc_atomic_cmplx16_div_a16_cpt
#endif
#endif
#endif

#endif /* __MPC_OMP_TYPES_KMP_ATOMICS_H__ */

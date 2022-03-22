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
#ifndef __SCTK_MPC_REDUCTION_H_
#define __SCTK_MPC_REDUCTION_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "mpc.h"
#include <mpc_common_types.h>
#include "mpc_mpi_comm_lib.h"

typedef struct {float a ; int b;} mpc_float_int;
typedef struct {double a ; int b;} mpc_double_int;
typedef struct {long a ; int b;} mpc_long_int;
typedef struct {short a ; int b;} mpc_short_int;
typedef struct {int a ; int b;} mpc_int_int;
typedef struct {float a ; float b;} mpc_float_float;
typedef struct {double a ; double b;} mpc_double_double;
typedef struct {long double a;int b;} mpc_long_double_int;
typedef struct {long double a;long double b;} mpc_longdouble_longdouble;
typedef struct {int a; int b;} mpc_integer_integer;
typedef struct {float a ; float b;} mpc_real_real;

typedef unsigned long long int mpc_unsigned_long_long_int;
typedef mpc_unsigned_long_long_int mpc_unsigned_long_long;
typedef long long mpc_long_long;
typedef mpc_long_long  mpc_long_long_int;

void mpc_no_exec (const void *in, void *inout, size_t size, mpc_lowcomm_datatype_t datatype, int line, char *file);

#define MPC_DEFINED_FUNCS(t, tt, name)                                         \
  void MPC_##name##_func##_##t(const tt * restrict in,                      \
                               tt * restrict inout, size_t size,            \
                               mpc_lowcomm_datatype_t datatype)

#define MPC_PROTOTYPES(name)						\
void MPC_##name##_func(const void* in ,void*inout ,size_t size ,mpc_lowcomm_datatype_t datatype); \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_CHAR, char,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_SIGNED_CHAR, char,name);             \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_CHARACTER, char,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_BYTE, unsigned char,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_SHORT, short,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INT, int,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INTEGER, int,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LONG, long,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LONG_LONG, mpc_long_long,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_FLOAT, float,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_DOUBLE, double,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_DOUBLE_PRECISION, double,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UNSIGNED, unsigned int,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UNSIGNED_LONG, unsigned long,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LONG_DOUBLE, long double,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_FLOAT_INT,mpc_float_int ,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LONG_INT,mpc_long_int ,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_DOUBLE_INT,mpc_double_int ,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LONG_DOUBLE_INT,mpc_long_double_int ,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_SHORT_INT,mpc_short_int ,name);		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_2INT,mpc_int_int ,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_2INTEGER,mpc_integer_integer ,name);          \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_2FLOAT,mpc_float_float ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_COMPLEX,mpc_float_float ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_COMPLEX8,mpc_float_float ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_COMPLEX16,mpc_double_double ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_DOUBLE_COMPLEX,mpc_double_double ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_COMPLEX32,mpc_longdouble_longdouble ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT,mpc_unsigned_long_long_int ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UNSIGNED_LONG_LONG,mpc_unsigned_long_long_int ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int ,name)	;		\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_LOGICAL, int,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_2DOUBLE_PRECISION,mpc_double_double ,name);	\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_REAL,float,name);                                \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_2REAL,mpc_real_real,name);                                \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_REAL4,float,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_REAL8,double,name);				\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_REAL16,long double,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INTEGER1,int8_t,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INTEGER2,int16_t,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INTEGER4,int32_t,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INTEGER8,int64_t,name);			\
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UINT8_T, uint8_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UINT16_T, uint16_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UINT32_T, uint32_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_UINT64_T, uint64_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INT8_T, int8_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INT16_T, int16_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INT32_T, int32_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_INT64_T, int64_t,name);   \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_COUNT, size_t, name);       \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_AINT, size_t, name);         \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_OFFSET, size_t, name);       \
MPC_DEFINED_FUNCS(MPC_LOWCOMM_C_BOOL, char, name)

MPC_PROTOTYPES (MIN);
MPC_PROTOTYPES (MAX);
MPC_PROTOTYPES (SUM);
MPC_PROTOTYPES (PROD);
MPC_PROTOTYPES (MINLOC);
MPC_PROTOTYPES (MAXLOC);
MPC_PROTOTYPES (LOR);
MPC_PROTOTYPES (BOR);
MPC_PROTOTYPES (LXOR);
MPC_PROTOTYPES (BXOR);
MPC_PROTOTYPES (LAND);
MPC_PROTOTYPES (BAND);

void MPC_SUM_func_MPC_COMPLEX (const mpc_float_float * in,mpc_float_float * inout, size_t size,mpc_lowcomm_datatype_t datatype);
void MPC_MIN_func_MPC_COMPLEX (const mpc_float_float * in,mpc_float_float * inout, size_t size,mpc_lowcomm_datatype_t datatype);
void MPC_MAX_func_MPC_COMPLEX (const mpc_float_float * in,mpc_float_float * inout, size_t size,mpc_lowcomm_datatype_t datatype);
void MPC_PROD_func_MPC_COMPLEX (const mpc_float_float * in,mpc_float_float * inout, size_t size,mpc_lowcomm_datatype_t datatype);

void MPC_SUM_func_MPC_DOUBLE_COMPLEX (const mpc_double_double * in,mpc_double_double * inout, size_t size,mpc_lowcomm_datatype_t datatype);
void MPC_MIN_func_MPC_DOUBLE_COMPLEX (const mpc_double_double * in,mpc_double_double * inout, size_t size,mpc_lowcomm_datatype_t datatype);
void MPC_MAX_func_MPC_DOUBLE_COMPLEX (const mpc_double_double * in,mpc_double_double * inout, size_t size,mpc_lowcomm_datatype_t datatype);
void MPC_PROD_func_MPC_DOUBLE_COMPLEX (const mpc_double_double * in,mpc_double_double * inout, size_t size,mpc_lowcomm_datatype_t datatype);

#ifdef __cplusplus
}
#endif
#endif

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

#define MPC_CREATE_TYPE(t)\
     typedef struct {\
       t val;\
       int pos;\
     } mpc_##t##_int\

  MPC_CREATE_TYPE (float);
    MPC_CREATE_TYPE (long);
    MPC_CREATE_TYPE (double);
    MPC_CREATE_TYPE (short);
    MPC_CREATE_TYPE (int);

  typedef struct
  {
    float val;
    float pos;
  } mpc_float_float;
  typedef struct
  {
    double val;
    double pos;
  } mpc_double_double;

  void mpc_no_exec (const void *in, void *inout, size_t size,
		    MPC_Datatype datatype, int line, char *file);

#define MPC_DEFINED_FUNCS(t,tt,name)					\
  void MPC_##name##_func##_##t(const tt* in ,tt*inout ,size_t size ,MPC_Datatype datatype)

#define MPC_PROTOTYPES(name)						\
  void MPC_##name##_func(const void* in ,void*inout ,size_t size ,MPC_Datatype datatype); \
    MPC_DEFINED_FUNCS(MPC_CHAR, char,name);				\
    MPC_DEFINED_FUNCS(MPC_BYTE, unsigned char,name);			\
    MPC_DEFINED_FUNCS(MPC_SHORT, short,name);				\
    MPC_DEFINED_FUNCS(MPC_INT, int,name);				\
    MPC_DEFINED_FUNCS(MPC_LONG, long,name);				\
    MPC_DEFINED_FUNCS(MPC_FLOAT, float,name);				\
    MPC_DEFINED_FUNCS(MPC_DOUBLE, double,name);				\
    MPC_DEFINED_FUNCS(MPC_UNSIGNED_CHAR, unsigned char,name);		\
    MPC_DEFINED_FUNCS(MPC_UNSIGNED_SHORT, unsigned short,name);		\
    MPC_DEFINED_FUNCS(MPC_UNSIGNED, unsigned int,name);			\
    MPC_DEFINED_FUNCS(MPC_UNSIGNED_LONG, unsigned long,name);		\
    MPC_DEFINED_FUNCS(MPC_LONG_DOUBLE, long double,name);		\
    MPC_DEFINED_FUNCS(MPC_LONG_LONG_INT, long long,name);		\
    MPC_DEFINED_FUNCS(MPC_FLOAT_INT,mpc_float_int ,name);		\
    MPC_DEFINED_FUNCS(MPC_LONG_INT,mpc_long_int ,name);			\
    MPC_DEFINED_FUNCS(MPC_DOUBLE_INT,mpc_double_int ,name);		\
    MPC_DEFINED_FUNCS(MPC_SHORT_INT,mpc_short_int ,name);		\
    MPC_DEFINED_FUNCS(MPC_2INT,mpc_int_int ,name);			\
    MPC_DEFINED_FUNCS(MPC_2FLOAT,mpc_float_float ,name)	;		\
    MPC_DEFINED_FUNCS(MPC_LOGICAL, int,name);				\
    MPC_DEFINED_FUNCS(MPC_2DOUBLE_PRECISION,mpc_double_double ,name)

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

  void MPC_SUM_func_MPC_COMPLEX (const mpc_float_float * in,
				 mpc_float_float * inout, size_t size,
				 MPC_Datatype datatype);
  void MPC_MIN_func_MPC_COMPLEX (const mpc_float_float * in,
				 mpc_float_float * inout, size_t size,
				 MPC_Datatype datatype);
  void MPC_MAX_func_MPC_COMPLEX (const mpc_float_float * in,
				 mpc_float_float * inout, size_t size,
				 MPC_Datatype datatype);
  void MPC_PROD_func_MPC_COMPLEX (const mpc_float_float * in,
				  mpc_float_float * inout, size_t size,
				  MPC_Datatype datatype);

#ifdef __cplusplus
}
#endif
#endif

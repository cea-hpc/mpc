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
#include "mpc_config.h"
#include "mpc_common_debug.h"
#include "mpc_reduction.h"
#include "mpc_common_types.h"
#include <mpc_lowcomm_datatypes.h>


#if 0
#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 4) && (__GNUC_PATCHLEVEL__ == 0)
/* Optim bug with BUG GCC4.4.0: remove restrict keyword*/
#undef  restrict
#define  restrict 
#endif
#endif

void
mpc_no_exec (const void * restrict in, void * restrict inout, size_t size,
	     mpc_lowcomm_datatype_t datatype, int line, char * restrict file)
{
  fprintf (stderr,
	   "Internal error: This can not be run at line %d in %s in %p out %p size %lu, datatype %d\n",
	   line, file, in, inout, (unsigned long) size, (int) datatype);
  not_reachable ();
}

static inline void
mpc_redution_check_type (__UNUSED__ mpc_lowcomm_datatype_t a, __UNUSED__ mpc_lowcomm_datatype_t b)
{
  assert (a == b);
  mpc_common_nodebug("%d == %d",a,b);
}

#define MPC_PROTOTYPES_IMPL(name)						\
  void MPC_##name##_func(const void*  restrict  in ,void*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }

#define MPC_DEFINED_FUNCS_IMPL(t, tt)                                          \
  void MPC_SUM_func_##t(const tt * restrict in, tt * restrict inout,     \
                        size_t size, mpc_lowcomm_datatype_t datatype) {                  \
    unsigned int i;                                                            \
    mpc_redution_check_type(datatype, t);                                      \
    for (i = 0; i < size; i++) {                                               \
      inout[i] = in[i] + inout[i];                                             \
    }                                                                          \
  }                                                                            \
  void MPC_MIN_func_##t(const tt * restrict in, tt * restrict inout,     \
                        size_t size, mpc_lowcomm_datatype_t datatype) {                  \
    unsigned int i;                                                            \
    mpc_redution_check_type(datatype, t);                                      \
    for (i = 0; i < size; i++) {                                               \
      if (inout[i] > in[i])                                                    \
        inout[i] = in[i];                                                      \
    }                                                                          \
  }                                                                            \
  void MPC_MAX_func_##t(const tt * restrict in, tt * restrict inout,     \
                        size_t size, mpc_lowcomm_datatype_t datatype) {                  \
    unsigned int i;                                                            \
    mpc_redution_check_type(datatype, t);                                      \
    for (i = 0; i < size; i++) {                                               \
      if (inout[i] < in[i])                                                    \
        inout[i] = in[i];                                                      \
    }                                                                          \
  }                                                                            \
  void MPC_PROD_func_##t(const tt * restrict in, tt * restrict inout,    \
                         size_t size, mpc_lowcomm_datatype_t datatype) {                 \
    unsigned int i;                                                            \
    mpc_redution_check_type(datatype, t);                                      \
    for (i = 0; i < size; i++) {                                               \
      inout[i] = in[i] * inout[i];                                             \
    }                                                                          \
  }

 void
MPC_MAX_func_MPC_LOWCOMM_COMPLEX8 (const mpc_float_float *  restrict in,
        mpc_float_float *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX8);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_MIN_func_MPC_LOWCOMM_COMPLEX8 (const mpc_float_float *  restrict in,
        mpc_float_float *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX8);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_SUM_func_MPC_LOWCOMM_COMPLEX8 (const mpc_float_float *  restrict in,
        mpc_float_float *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX8);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}
void
MPC_PROD_func_MPC_LOWCOMM_COMPLEX8 (const mpc_float_float *  restrict in,
        mpc_float_float *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX8);
  for (i = 0; i < size; i++)
    {
      inout[i].a = (in[i].a * inout[i].a) - (in[i].b * inout[i].b);
      inout[i].b = (in[i].b * inout[i].a) + (in[i].a * inout[i].b);
    }
}

 void
MPC_MAX_func_MPC_LOWCOMM_COMPLEX16 (const mpc_double_double *  restrict in,
        mpc_double_double *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX16);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_MIN_func_MPC_LOWCOMM_COMPLEX16 (const mpc_double_double *  restrict in,
        mpc_double_double *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX16);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_SUM_func_MPC_LOWCOMM_COMPLEX16 (const mpc_double_double *  restrict in,
        mpc_double_double *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX16);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }

}
void
MPC_PROD_func_MPC_LOWCOMM_COMPLEX16 (const mpc_double_double *  restrict in,
        mpc_double_double *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX16);
  for (i = 0; i < size; i++)
    {
      inout[i].a = (in[i].a * inout[i].a) - (in[i].b * inout[i].b);
      inout[i].b = (in[i].b * inout[i].a) + (in[i].a * inout[i].b);
    }

}

 void
MPC_MAX_func_MPC_LOWCOMM_COMPLEX32 (const mpc_longdouble_longdouble*  restrict in,
        mpc_longdouble_longdouble *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX32);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_MIN_func_MPC_LOWCOMM_COMPLEX32 (const mpc_longdouble_longdouble*  restrict in,
        mpc_longdouble_longdouble *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX32);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_SUM_func_MPC_LOWCOMM_COMPLEX32 (const mpc_longdouble_longdouble *  restrict in,
        mpc_longdouble_longdouble*  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX32);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}
void
MPC_PROD_func_MPC_LOWCOMM_COMPLEX32 (const mpc_longdouble_longdouble *  restrict in,
        mpc_longdouble_longdouble *  restrict inout, size_t size,
        mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX32);
  for (i = 0; i < size; i++)
    {
      inout[i].a = (in[i].a * inout[i].a) - (in[i].b * inout[i].b);
      inout[i].b = (in[i].b * inout[i].a) + (in[i].a * inout[i].b);
    }
}

void
MPC_SUM_func_MPC_LOWCOMM_DOUBLE_COMPLEX (const mpc_double_double *  restrict in,
			  mpc_double_double *  restrict inout, size_t size,
			  mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_DOUBLE_COMPLEX);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}

void
MPC_MIN_func_MPC_LOWCOMM_DOUBLE_COMPLEX (const mpc_double_double *  restrict in,
			  mpc_double_double *  restrict inout, size_t size,
			  mpc_lowcomm_datatype_t datatype)
{
  mpc_redution_check_type (datatype, MPC_LOWCOMM_DOUBLE_COMPLEX);
  not_available ();
  assert (in == NULL);
  assert (inout == NULL);
  assert (size == 0);
  assert (datatype == 0);
}

void
MPC_MAX_func_MPC_LOWCOMM_DOUBLE_COMPLEX (const mpc_double_double *  restrict in,
			  mpc_double_double *  restrict inout, size_t size,
			  mpc_lowcomm_datatype_t datatype)
{
  mpc_redution_check_type (datatype, MPC_LOWCOMM_DOUBLE_COMPLEX);
  not_available ();
  assert (in == NULL);
  assert (inout == NULL);
  assert (size == 0);
  assert (datatype == 0);
}

void
MPC_PROD_func_MPC_LOWCOMM_DOUBLE_COMPLEX (const mpc_double_double *  restrict in,
			   mpc_double_double *  restrict inout, size_t size,
			   mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_DOUBLE_COMPLEX);
  for (i = 0; i < size; i++)
    {
      inout[i].a = (in[i].a * inout[i].a) - (in[i].b * inout[i].b);
      inout[i].b = (in[i].b * inout[i].a) + (in[i].a * inout[i].b);
    }
}

void
MPC_SUM_func_MPC_LOWCOMM_COMPLEX (const mpc_float_float *  restrict in,
			  mpc_float_float *  restrict inout, size_t size,
			  mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX);
  for (i = 0; i < size; i++)
    {
      inout[i].a = in[i].a + inout[i].a;
      inout[i].b = in[i].b + inout[i].b;
    }
}
void
MPC_MIN_func_MPC_LOWCOMM_COMPLEX (const mpc_float_float *  restrict in,
			  mpc_float_float *  restrict inout, size_t size,
			  mpc_lowcomm_datatype_t datatype)
{
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX);
  not_available ();
  assert (in == NULL);
  assert (inout == NULL);
  assert (size == 0);
  assert (datatype == 0);
}

void
MPC_MAX_func_MPC_LOWCOMM_COMPLEX (const mpc_float_float *  restrict in,
			  mpc_float_float *  restrict inout, size_t size,
			  mpc_lowcomm_datatype_t datatype)
{
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX);
  not_available ();
  assert (in == NULL);
  assert (inout == NULL);
  assert (size == 0);
  assert (datatype == 0);
}

void
MPC_PROD_func_MPC_LOWCOMM_COMPLEX (const mpc_float_float *  restrict in,
			   mpc_float_float *  restrict inout, size_t size,
			   mpc_lowcomm_datatype_t datatype)
{
  size_t i;
  mpc_redution_check_type (datatype, MPC_LOWCOMM_COMPLEX);
  for (i = 0; i < size; i++)
    {
      inout[i].a = (in[i].a * inout[i].a) - (in[i].b * inout[i].b);
      inout[i].b = (in[i].b * inout[i].a) + (in[i].a * inout[i].b);
    }
}

#define MPC_DEFINED_FUNCS_NOIMPL(t,tt)						\
  void MPC_SUM_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_MIN_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_MAX_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_PROD_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }

#define MPC_DEFINED_FUNCS_IMPL2(t,tt)						\
  void MPC_BAND_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] & inout[i]);				\
    }									\
  }									\
  void MPC_LAND_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] && inout[i]);				\
    }									\
  }									\
  void MPC_BXOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] ^ inout[i]);				\
    }									\
  }									\
  void MPC_LXOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)((in[i] && !(inout[i])) || (!(in[i])		\
					       && inout[i]));		\
    }									\
  }									\
  void MPC_BOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] | inout[i]);				\
    }									\
  }									\
  void MPC_LOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] || inout[i]);				\
    }									\
  }

#define MPC_DEFINED_FUNCS_IMPL2_LOG(t,tt)						\
  void MPC_BAND_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] & inout[i]);				\
    }									\
  }									\
  void MPC_LAND_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] && inout[i]);				\
    }									\
  }									\
  void MPC_BXOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] ^ inout[i]);				\
    }									\
  }									\
  void MPC_LXOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)((in[i] && !(inout[i])) || (!(in[i])		\
					       && inout[i]));		\
    }									\
  }									\
  void MPC_BOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      inout[i] = (tt)(in[i] | inout[i]);				\
    }									\
  }									\
  void MPC_LOR_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      fprintf(stderr,"%ld : %d || %d ",(long)i,in[i],inout[i]);		\
      inout[i] = (tt)(in[i] || inout[i]);				\
      fprintf(stderr,"= %d\n",inout[i]);				\
    }									\
  }

#define MPC_DEFINED_FUNCS_NOIMPL2(t,tt)						\
  void MPC_BAND_func_##t(const tt* in ,tt*inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_LAND_func_##t(const tt* in ,tt*inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_BXOR_func_##t(const tt* in ,tt*inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_LXOR_func_##t(const tt* in ,tt*inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_BOR_func_##t(const tt* in ,tt*inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_LOR_func_##t(const tt* in ,tt*inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }

#define MPC_DEFINED_FUNCS_IMPL3(t,tt)					\
  void MPC_MAXLOC_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      if (inout[i].a < in[i].a)						\
	inout[i] = in[i];						\
      if ((inout[i].a == in[i].a) && (inout[i].b > in[i].b))		\
	inout[i] = in[i];						\
    }									\
  }									\
  void MPC_MINLOC_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    size_t i;								\
    mpc_redution_check_type(datatype,t);				\
    for(i = 0; i < size; i++){						\
      if (inout[i].a > in[i].a)						\
	inout[i] = in[i];						\
      if ((inout[i].a == in[i].a) && (inout[i].b > in[i].b))		\
	inout[i] = in[i];						\
    }									\
  }

#define MPC_DEFINED_FUNCS_NOIMPL3(t,tt)					\
  void MPC_MAXLOC_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }									\
  void MPC_MINLOC_func_##t(const tt*   restrict in ,tt*  restrict inout ,size_t size ,mpc_lowcomm_datatype_t datatype){ \
    mpc_no_exec(in,inout,size,datatype,__LINE__,__FILE__);		\
  }

MPC_PROTOTYPES_IMPL (MIN)
MPC_PROTOTYPES_IMPL (MAX)
MPC_PROTOTYPES_IMPL (SUM)
MPC_PROTOTYPES_IMPL (PROD)
MPC_PROTOTYPES_IMPL (MINLOC)
MPC_PROTOTYPES_IMPL (MAXLOC)
MPC_PROTOTYPES_IMPL (LOR)
MPC_PROTOTYPES_IMPL (BOR)
MPC_PROTOTYPES_IMPL (LXOR)
MPC_PROTOTYPES_IMPL (BXOR)
MPC_PROTOTYPES_IMPL (LAND)
MPC_PROTOTYPES_IMPL (BAND) 

MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_CHAR, char)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_CHARACTER, char)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INTEGER, int)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_LONG_LONG, mpc_long_long)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_IMPL(MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_DOUBLE_PRECISION, double)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UNSIGNED_LONG_LONG, mpc_unsigned_long_long_int)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_LONG_DOUBLE, long double)

MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_COUNT, size_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_AINT, size_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_OFFSET, size_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_C_BOOL, char)

MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_REAL, float)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_REAL16, long double)

MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UINT8_T, uint8_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UINT16_T, uint16_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UINT32_T, uint32_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_UINT64_T, uint64_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INT8_T, int8_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INT16_T, int16_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INT32_T, int32_t)
MPC_DEFINED_FUNCS_IMPL (MPC_LOWCOMM_INT64_T, int64_t)

MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UINT8_T, uint8_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UINT16_T, uint16_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UINT32_T, uint32_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UINT64_T, uint64_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INT8_T, int8_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INT16_T, int16_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INT32_T, int32_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INT64_T, int64_t)
  

MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_COUNT, size_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_AINT, size_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_OFFSET, size_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_C_BOOL, char)

MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_FLOAT_INT, mpc_float_int)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_LONG_INT, mpc_long_int)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_DOUBLE_INT, mpc_double_int)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_LONG_DOUBLE_INT, mpc_long_double_int)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_SHORT_INT, mpc_short_int)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_2INT, mpc_int_int)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_2FLOAT, mpc_float_float)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_2INTEGER, mpc_integer_integer)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_2REAL, mpc_real_real)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_2DOUBLE_PRECISION, mpc_double_double)
MPC_DEFINED_FUNCS_NOIMPL (MPC_LOWCOMM_LOGICAL, int)

MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_CHAR, char)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_LOGICAL, int)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INTEGER, int)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_LONG_LONG, mpc_long_long)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_IMPL2 (MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_NOIMPL2 (MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_NOIMPL2 (MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_NOIMPL2 (MPC_LOWCOMM_REAL16, long double)


MPC_DEFINED_FUNCS_NOIMPL2 (MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_NOIMPL2 (MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_NOIMPL2 (MPC_LOWCOMM_LONG_DOUBLE, long double)

MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_CHAR, char)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_LOGICAL, int)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_LONG_DOUBLE, long double)

MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_REAL16, long double)



MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_FLOAT_INT, mpc_float_int)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_LONG_INT, mpc_long_int)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_DOUBLE_INT, mpc_double_int)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_LONG_DOUBLE_INT, mpc_long_double_int)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_SHORT_INT, mpc_short_int)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_2INT, mpc_int_int)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_2FLOAT, mpc_float_float)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_2INTEGER, mpc_integer_integer)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_2REAL, mpc_real_real)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_2DOUBLE_PRECISION, mpc_double_double)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_COMPLEX8, mpc_float_float)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_COMPLEX16, mpc_double_double)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_IMPL3 (MPC_LOWCOMM_COMPLEX32, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_NOIMPL3 (MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)




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
#include "mpc_mpi_comm_lib.h"
#include <mpc_lowcomm_datatypes.h>


#if 0
#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 4) && (__GNUC_PATCHLEVEL__ == 0)
/* Optim bug with BUG GCC4.4.0: remove restrict keyword*/
#undef  restrict
#define  restrict
#endif
#endif

void
mpc_no_exec(const void * restrict in, void * restrict inout, size_t size,
            mpc_lowcomm_datatype_t datatype, int line, char * restrict file)
{
	fprintf(stderr,
	        "Internal error: This can not be run at line %d in %s in %p out %p size %lu, datatype %d\n",
	        line, file, in, inout, (unsigned long)size, (int)datatype->id);
	not_reachable();
}

static inline void
mpc_reduction_check_type(__UNUSED__ mpc_lowcomm_datatype_t a, __UNUSED__ mpc_lowcomm_datatype_t b)
{
	assert(a == b);
	mpc_common_nodebug("%d == %d", a, b);
}

#define MPC_PROTOTYPES_IMPL(name)                                                                                                    \
	void MPC_ ## name ## _func(const void *  restrict in, void *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                          \
	}


/* Min, max */

#define MPC_DEFINED_FUNCS_MIN_MAX_IMPL(t, tt)                                           \
	void MPC_MIN_func_ ## t(const tt * restrict in, tt * restrict inout,    \
	                        size_t size, mpc_lowcomm_datatype_t datatype){  \
		unsigned int i;                                                 \
		mpc_reduction_check_type(datatype, t);                           \
		for(i = 0; i < size; i++){                                      \
			if(inout[i] > in[i])                                    \
			inout[i] = in[i];                                       \
		}                                                               \
	}\
	void MPC_MAX_func_ ## t(const tt * restrict in, tt * restrict inout,    \
	                        size_t size, mpc_lowcomm_datatype_t datatype){  \
		unsigned int i;                                                 \
		mpc_reduction_check_type(datatype, t);                           \
		for(i = 0; i < size; i++){                                      \
			if(inout[i] < in[i])                                    \
			inout[i] = in[i];                                       \
		}                                                               \
	}

#define MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(t, tt)                                                                                         \
	void MPC_MIN_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                     \
	}                                                                                                                       \
	void MPC_MAX_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                     \
	}


/* Sum, product */

#define MPC_DEFINED_FUNCS_SUM_PROD_IMPL(t, tt)                                           \
	void MPC_SUM_func_ ## t(const tt * restrict in, tt * restrict inout,    \
	                        size_t size, mpc_lowcomm_datatype_t datatype){  \
		unsigned int i;                                                 \
		mpc_reduction_check_type(datatype, t);                           \
		for(i = 0; i < size; i++){                                      \
			inout[i] = in[i] + inout[i];                            \
		}                                                               \
	}                                                                       \
	void MPC_PROD_func_ ## t(const tt * restrict in, tt * restrict inout,   \
	                         size_t size, mpc_lowcomm_datatype_t datatype){ \
		unsigned int i;                                                 \
		mpc_reduction_check_type(datatype, t);                           \
		for(i = 0; i < size; i++){                                      \
			inout[i] = in[i] * inout[i];                            \
		}                                                               \
	}

#define MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(mpi_type, c_type)\
	void MPC_SUM_func_ ## mpi_type (const c_type *  restrict in,\
                          c_type *  restrict inout, size_t size,\
                          mpc_lowcomm_datatype_t datatype)\
	{\
		size_t i;\
		\
		mpc_reduction_check_type(datatype, mpi_type);\
		for(i = 0; i < size; i++)\
		{\
			inout[i].a = in[i].a + inout[i].a;\
			inout[i].b = in[i].b + inout[i].b;\
		}\
	}\
	\
	void MPC_PROD_func_ ## mpi_type (const c_type *  restrict in,\
                          c_type *  restrict inout, size_t size,\
                          mpc_lowcomm_datatype_t datatype)\
	{\
		size_t i;\
		\
		mpc_reduction_check_type(datatype, mpi_type);\
		for(i = 0; i < size; i++)\
		{\
			inout[i].a = (in[i].a * inout[i].a) - (in[i].b * inout[i].b);\
			inout[i].b = (in[i].b * inout[i].a) + (in[i].a * inout[i].b);\
		}\
	}


#define MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(t, tt)                                                                                         \
	void MPC_SUM_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                     \
	}                                                                                                                       \
	void MPC_PROD_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                     \
	}


/* Binary operations */

#define MPC_DEFINED_FUNCS_BINARY_OP_IMPL(t, tt)                                                                                          \
	void MPC_BAND_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		size_t i;                                                                                                       \
		mpc_reduction_check_type(datatype, t);                                                                           \
		for(i = 0; i < size; i++){                                                                                      \
			inout[i] = (tt)(in[i] & inout[i]);                                                                      \
		}                                                                                                               \
	}                                                                                                                       \
	void MPC_BXOR_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		size_t i;                                                                                                       \
		mpc_reduction_check_type(datatype, t);                                                                           \
		for(i = 0; i < size; i++){                                                                                      \
			inout[i] = (tt)(in[i] ^ inout[i]);                                                                      \
		}                                                                                                               \
	}                                                                                                                       \
	void MPC_BOR_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		size_t i;                                                                                                       \
		mpc_reduction_check_type(datatype, t);                                                                           \
		for(i = 0; i < size; i++){                                                                                      \
			inout[i] = (tt)(in[i] | inout[i]);                                                                      \
		}                                                                                                               \
	}

#define MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(t, tt)                                                                   \
	void MPC_BAND_func_ ## t(const tt * in, tt * inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                \
	}                                                                                                  \
	void MPC_BXOR_func_ ## t(const tt * in, tt * inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                \
	}                                                                                                  \
	void MPC_BOR_func_ ## t(const tt * in, tt * inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                \
	}


/* Logical operations */

#define MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(t, tt)                                                                                          \
	void MPC_LAND_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		size_t i;                                                                                                       \
		mpc_reduction_check_type(datatype, t);                                                                           \
		for(i = 0; i < size; i++){                                                                                      \
			inout[i] = (tt)(in[i] && inout[i]);                                                                     \
		}                                                                                                               \
	}                                                                                                                       \
	void MPC_LXOR_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		size_t i;                                                                                                       \
		mpc_reduction_check_type(datatype, t);                                                                           \
		for(i = 0; i < size; i++){                                                                                      \
			inout[i] = (tt)( (in[i] && !(inout[i]) ) || (!(in[i])                                                   \
			                                             && inout[i]) );                                            \
		}                                                                                                               \
	}                                                                                                                       \
	void MPC_LOR_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		size_t i;                                                                                                       \
		mpc_reduction_check_type(datatype, t);                                                                           \
		for(i = 0; i < size; i++){                                                                                      \
			inout[i] = (tt)(in[i] || inout[i]);                                                                     \
		}                                                                                                               \
	}

#define MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(t, tt)                                                                   \
	void MPC_LAND_func_ ## t(const tt * in, tt * inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                \
	}                                                                                                  \
	void MPC_LXOR_func_ ## t(const tt * in, tt * inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                \
	}                                                                                                  \
	void MPC_LOR_func_ ## t(const tt * in, tt * inout, size_t size, mpc_lowcomm_datatype_t datatype){  \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                \
	}


/* Minloc, maxloc */

#define MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(t, tt)                                                                                            \
	void MPC_MAXLOC_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		size_t i;                                                                                                         \
		mpc_reduction_check_type(datatype, t);                                                                             \
		for(i = 0; i < size; i++){                                                                                        \
			if(inout[i].a < in[i].a)                                                                                  \
			inout[i] = in[i];                                                                                         \
			if( (inout[i].a == in[i].a) && (inout[i].b > in[i].b) )                                                   \
			inout[i] = in[i];                                                                                         \
		}                                                                                                                 \
	}                                                                                                                         \
	void MPC_MINLOC_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		size_t i;                                                                                                         \
		mpc_reduction_check_type(datatype, t);                                                                             \
		for(i = 0; i < size; i++){                                                                                        \
			if(inout[i].a > in[i].a)                                                                                  \
			inout[i] = in[i];                                                                                         \
			if( (inout[i].a == in[i].a) && (inout[i].b > in[i].b) )                                                   \
			inout[i] = in[i];                                                                                         \
		}                                                                                                                 \
	}

#define MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(t, tt)                                                                                          \
	void MPC_MAXLOC_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                       \
	}                                                                                                                         \
	void MPC_MINLOC_func_ ## t(const tt *   restrict in, tt *  restrict inout, size_t size, mpc_lowcomm_datatype_t datatype){ \
		mpc_no_exec(in, inout, size, datatype, __LINE__, __FILE__);                                                       \
	}

MPC_PROTOTYPES_IMPL(MIN)
MPC_PROTOTYPES_IMPL(MAX)
MPC_PROTOTYPES_IMPL(SUM)
MPC_PROTOTYPES_IMPL(PROD)
MPC_PROTOTYPES_IMPL(MINLOC)
MPC_PROTOTYPES_IMPL(MAXLOC)
MPC_PROTOTYPES_IMPL(LOR)
MPC_PROTOTYPES_IMPL(BOR)
MPC_PROTOTYPES_IMPL(LXOR)
MPC_PROTOTYPES_IMPL(BXOR)
MPC_PROTOTYPES_IMPL(LAND)
MPC_PROTOTYPES_IMPL(BAND)


/*
 * We cannot factorize code with macros for each datatype group,
 * e.g. MPC_DEFINED_FUNCS_C_INTEGER(mpi_type, c_type):
 * mpi_type would be expanded to its value prior to the calls
 * to MPC_DEFINED_FUNCS_*, for instance as (mpc_lowcomm_datatype_t) 7 ).
 *
 * Since mpi_type must be used as is to generate function names,
 * we do not want macros to be expanded here.
 *
 * We could stringify mpi_type to prevent expansion,
 * but as far as I know, we cannot unstringify it later on
 * to use it as a function token in MPC_DEFINED_FUNCS_*.
 */



/**************
 * C Integers *
 **************/

/* MPC_LOWCOMM_INT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INT, int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INT, int)

/* MPC_LOWCOMM_LONG */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_LONG, long)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_LONG, long)

/* MPC_LOWCOMM_SHORT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_SHORT, short)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_SHORT, short)

/* MPC_LOWCOMM_UNSIGNED_SHORT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short)

/* MPC_LOWCOMM_UNSIGNED */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UNSIGNED, unsigned int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UNSIGNED, unsigned int)

/* MPC_LOWCOMM_UNSIGNED_LONG */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UNSIGNED_LONG, unsigned long)

/* MPC_LOWCOMM_LONG_LONG_INT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_LONG_LONG_INT, mpc_long_long_int)

/* MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, mpc_unsigned_long_long_int)


/* MPC_LOWCOMM_LONG_LONG */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_LONG_LONG, mpc_long_long)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_LONG_LONG, mpc_long_long)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_LONG_LONG, mpc_long_long)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_LONG_LONG, mpc_long_long)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_LONG_LONG, mpc_long_long)

/* MPC_LOWCOMM_SIGNED_CHAR */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_SIGNED_CHAR, char)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_SIGNED_CHAR, char)

/* MPC_LOWCOMM_UNSIGNED_CHAR */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char)

/* MPC_LOWCOMM_INT8_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INT8_T, int8_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INT8_T, int8_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_INT8_T, int8_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INT8_T, int8_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INT8_T, int8_t)

/* MPC_LOWCOMM_INT16_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INT16_T, int16_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INT16_T, int16_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_INT16_T, int16_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INT16_T, int16_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INT16_T, int16_t)

/* MPC_LOWCOMM_INT32_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INT32_T, int32_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INT32_T, int32_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_INT32_T, int32_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INT32_T, int32_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INT32_T, int32_t)

/* MPC_LOWCOMM_INT64_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INT64_T, int64_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INT64_T, int64_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_INT64_T, int64_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INT64_T, int64_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INT64_T, int64_t)

/* MPC_LOWCOMM_UINT8_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UINT8_T, uint8_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UINT8_T, uint8_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UINT8_T, uint8_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UINT8_T, uint8_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UINT8_T, uint8_t)

/* MPC_LOWCOMM_UINT16_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UINT16_T, uint16_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UINT16_T, uint16_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UINT16_T, uint16_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UINT16_T, uint16_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UINT16_T, uint16_t)

/* MPC_LOWCOMM_UINT32_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UINT32_T, uint32_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UINT32_T, uint32_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UINT32_T, uint32_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UINT32_T, uint32_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UINT32_T, uint32_t)

/* MPC_LOWCOMM_UINT64_T */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_UINT64_T, uint64_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_UINT64_T, uint64_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_UINT64_T, uint64_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_UINT64_T, uint64_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_UINT64_T, uint64_t)


/********************
 * Fortran Integers *
 ********************/

/* Same as C integers, except logical operations are disallowed */

/* MPC_LOWCOMM_INTEGER */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INTEGER, int)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INTEGER, int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_INTEGER, int)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INTEGER, int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INTEGER, int)

/* MPC_LOWCOMM_INTEGER1 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INTEGER1, int8_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INTEGER1, int8_t)

/* MPC_LOWCOMM_INTEGER2 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INTEGER2, int16_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INTEGER2, int16_t)

/* MPC_LOWCOMM_INTEGER4 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INTEGER4, int32_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INTEGER4, int32_t)

/* MPC_LOWCOMM_INTEGER8 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_INTEGER8, int64_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_INTEGER8, int64_t)


/******************
 * Floating Point *
 ******************/

/* MPC_LOWCOMM_FLOAT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_FLOAT, float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_FLOAT, float)

/* MPC_LOWCOMM_DOUBLE */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_DOUBLE, double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_DOUBLE, double)

/* MPC_LOWCOMM_REAL */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_REAL, float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_REAL, float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_REAL, float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_REAL, float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_REAL, float)

/* MPC_LOWCOMM_DOUBLE_PRECISION */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_DOUBLE_PRECISION, double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_DOUBLE_PRECISION, double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_DOUBLE_PRECISION, double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_DOUBLE_PRECISION, double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_DOUBLE_PRECISION, double)

/* MPC_LOWCOMM_LONG_DOUBLE */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_LONG_DOUBLE, long double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_LONG_DOUBLE, long double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_LONG_DOUBLE, long double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_LONG_DOUBLE, long double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_LONG_DOUBLE, long double)

/* MPC_LOWCOMM_REAL4 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_REAL4, float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_REAL4, float)

/* MPC_LOWCOMM_REAL8 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_REAL8, double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_REAL8, double)

/* MPC_LOWCOMM_REAL16 */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_REAL16, long double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_REAL16, long double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_REAL16, long double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_REAL16, long double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_REAL16, long double)


/***********
 * Logical *
 ***********/

/* MPC_LOWCOMM_LOGICAL */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_LOGICAL, int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_LOWCOMM_LOGICAL, int)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_LOGICAL, int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_LOGICAL, int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_LOGICAL, int)

/* MPC_LOWCOMM_C_BOOL */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_C_BOOL, char)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_LOWCOMM_C_BOOL, char)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_C_BOOL, char)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_C_BOOL, char)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_C_BOOL, char)

/* MPC_LOWCOMM_CXX_BOOL */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_CXX_BOOL, char)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_LOWCOMM_CXX_BOOL, char)
MPC_DEFINED_FUNCS_LOGICAL_OP_IMPL(MPC_LOWCOMM_CXX_BOOL, char)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_CXX_BOOL, char)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_CXX_BOOL, char)


/***********
 * Complex *
 ***********/

/* MPC_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_COMPLEX, mpc_float_float)

/* MPC_LOWCOMM_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_COMPLEX, mpc_float_float)

/* MPC_LOWCOMM_C_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_C_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_C_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_C_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_C_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_C_COMPLEX, mpc_float_float)

/* MPC_LOWCOMM_C_FLOAT_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_C_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_C_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_C_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_C_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_C_FLOAT_COMPLEX, mpc_float_float)

/* MPC_LOWCOMM_C_DOUBLE_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_C_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_C_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_C_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_C_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_C_DOUBLE_COMPLEX, mpc_double_double)

/* MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)

/* MPC_LOWCOMM_CXX_FLOAT_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_CXX_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_CXX_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_CXX_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_CXX_FLOAT_COMPLEX, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_CXX_FLOAT_COMPLEX, mpc_float_float)

/* MPC_LOWCOMM_CXX_DOUBLE_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_CXX_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_CXX_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_CXX_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_CXX_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_CXX_DOUBLE_COMPLEX, mpc_double_double)

/* MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX, mpc_longdouble_longdouble)

/* MPC_DOUBLE_COMPLEX */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_DOUBLE_COMPLEX, mpc_double_double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_DOUBLE_COMPLEX, mpc_double_double)

/* MPC_COMPLEX8 */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_COMPLEX8, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_COMPLEX8, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_COMPLEX8, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_COMPLEX8, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_COMPLEX8, mpc_float_float)

/* MPC_COMPLEX16 */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_COMPLEX16, mpc_double_double)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_COMPLEX16, mpc_double_double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_COMPLEX16, mpc_double_double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_COMPLEX16, mpc_double_double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_COMPLEX16, mpc_double_double)

/* MPC_COMPLEX32 */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_COMPLEX32, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL_COMPLEX(MPC_COMPLEX32, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_COMPLEX32, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_COMPLEX32, mpc_longdouble_longdouble)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_COMPLEX32, mpc_longdouble_longdouble)


/********
 * Byte *
 ********/

/* MPC_LOWCOMM_BYTE */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_BYTE, unsigned char)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_BYTE, unsigned char)


/************************
 * Multi-language types *
 ************************/

/* MPC_LOWCOMM_AINT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_AINT, size_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_AINT, size_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_AINT, size_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_AINT, size_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_AINT, size_t)

/* MPC_LOWCOMM_OFFSET */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_OFFSET, size_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_OFFSET, size_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_OFFSET, size_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_OFFSET, size_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_OFFSET, size_t)

/* MPC_LOWCOMM_COUNT */
MPC_DEFINED_FUNCS_MIN_MAX_IMPL(MPC_LOWCOMM_COUNT, size_t)
MPC_DEFINED_FUNCS_SUM_PROD_IMPL(MPC_LOWCOMM_COUNT, size_t)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LOWCOMM_COUNT, size_t)
MPC_DEFINED_FUNCS_BINARY_OP_IMPL(MPC_LOWCOMM_COUNT, size_t)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_NOIMPL(MPC_LOWCOMM_COUNT, size_t)


/******************
 * Minloc, maxloc *
 ******************/

/* MPC_2REAL */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_2REAL, mpc_real_real)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_2REAL, mpc_real_real)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_2REAL, mpc_real_real)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_2REAL, mpc_real_real)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_2REAL, mpc_real_real)

/* MPC_2DOUBLE_PRECISION */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_2DOUBLE_PRECISION, mpc_double_double)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_2DOUBLE_PRECISION, mpc_double_double)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_2DOUBLE_PRECISION, mpc_double_double)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_2DOUBLE_PRECISION, mpc_double_double)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_2DOUBLE_PRECISION, mpc_double_double)

/* MPC_2INTEGER */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_2INTEGER, mpc_integer_integer)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_2INTEGER, mpc_integer_integer)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_2INTEGER, mpc_integer_integer)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_2INTEGER, mpc_integer_integer)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_2INTEGER, mpc_integer_integer)

/* MPC_FLOAT_INT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_FLOAT_INT, mpc_float_int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_FLOAT_INT, mpc_float_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_FLOAT_INT, mpc_float_int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_FLOAT_INT, mpc_float_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_FLOAT_INT, mpc_float_int)

/* MPC_DOUBLE_INT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_DOUBLE_INT, mpc_double_int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_DOUBLE_INT, mpc_double_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_DOUBLE_INT, mpc_double_int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_DOUBLE_INT, mpc_double_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_DOUBLE_INT, mpc_double_int)

/* MPC_LONG_INT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LONG_INT, mpc_long_int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_LONG_INT, mpc_long_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LONG_INT, mpc_long_int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LONG_INT, mpc_long_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_LONG_INT, mpc_long_int)

/* MPC_2INT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_2INT, mpc_int_int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_2INT, mpc_int_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_2INT, mpc_int_int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_2INT, mpc_int_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_2INT, mpc_int_int)

/* MPC_2FLOAT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_2FLOAT, mpc_float_float)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_2FLOAT, mpc_float_float)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_2FLOAT, mpc_float_float)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_2FLOAT, mpc_float_float)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_2FLOAT, mpc_float_float)

/* MPC_SHORT_INT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_SHORT_INT, mpc_short_int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_SHORT_INT, mpc_short_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_SHORT_INT, mpc_short_int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_SHORT_INT, mpc_short_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_SHORT_INT, mpc_short_int)

/* MPC_LONG_DOUBLE_INT */
MPC_DEFINED_FUNCS_MIN_MAX_NOIMPL(MPC_LONG_DOUBLE_INT, mpc_long_double_int)
MPC_DEFINED_FUNCS_SUM_PROD_NOIMPL(MPC_LONG_DOUBLE_INT, mpc_long_double_int)
MPC_DEFINED_FUNCS_LOGICAL_OP_NOIMPL(MPC_LONG_DOUBLE_INT, mpc_long_double_int)
MPC_DEFINED_FUNCS_BINARY_OP_NOIMPL(MPC_LONG_DOUBLE_INT, mpc_long_double_int)
MPC_DEFINED_FUNCS_MIN_MAX_LOC_IMPL(MPC_LONG_DOUBLE_INT, mpc_long_double_int)

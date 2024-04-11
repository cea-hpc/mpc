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
/* #   - DEHENNE Remi remi.dehenne@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_REDUCTION_H_
#define __MPC_REDUCTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nbc.h"

/**
 * Start a chain of reduction macros with \ref MPC_NBC_OP_LOOP.
 */
#define MPC_NBC_OP_START\
	if (0) {}

/**
 * End a chain of reduction macros with \ref MPC_NBC_OP_LOOP.
 */
#define MPC_NBC_OP_END\
	else {\
		return NBC_OP_NOT_SUPPORTED;\
	}

/**
 * Innermost reduction macro, see \ref MPC_NBC_OP_COMPARE_REDUCE
 * and \ref MPC_NBC_OP_REDUCE.
 */
#define MPC_NBC_OP_LOOP(_mpi_op, _body)\
	else if (op == _mpi_op)\
	{\
		for ( i = 0; i < count; i++ )\
		{\
			_body\
		}\
	}\

/**
 * Reduction shorthand for comparison-assignment macros `c = (a OP b) ? b : a`,
 * i.e. min and max.
 */
#define MPC_NBC_OP_COMPARE_REDUCE(_mpi_op, _c_op, _c_type)\
	MPC_NBC_OP_LOOP(_mpi_op,\
	{\
		if ( *( ( (_c_type *) buf1 ) + i ) _c_op *( ( (_c_type *) buf2 ) + i ) )\
			*( ( (_c_type *) buf3 ) + i ) = *( ( (_c_type *) buf2 ) + i );\
		else\
			*( ( (_c_type *) buf3 ) + i ) = *( ( (_c_type *) buf1 ) + i );\
	})

/**
 * Reduction shorthand for comparison-assignment macros `c = (a OP b) ? b : a`
 * which also store the index of the matching element in a distinct field,
 * i.e. minloc or maxloc.
 */
#define MPC_NBC_OP_COMPARE_REDUCE_LOC(_mpi_op, _c_op, _c_type, _c_type_name)\
	MPC_NBC_OP_LOOP(_mpi_op,\
	{\
		typedef struct\
		{\
			_c_type val;\
			int rank;\
		} _c_type_name##_int;\
		_c_type_name##_int *ptr1;\
		_c_type_name##_int *ptr2;\
		_c_type_name##_int *ptr3;\
\
		ptr1 = ( (_c_type_name##_int *) buf1 ) + i;\
		ptr2 = ( (_c_type_name##_int *) buf2 ) + i;\
		ptr3 = ( (_c_type_name##_int *) buf3 ) + i;\
\
		if ( ptr1->val _c_op ptr2->val )\
		{\
			ptr3->val = ptr2->val;\
			ptr3->rank = ptr2->rank;\
		}\
		else\
		{\
			ptr3->val = ptr1->val;\
			ptr3->rank = ptr1->rank;\
		}\
	})

/**
 * Reduction shorthand for macros `c = a OP b`, e.g. sum or product.
 */
#define MPC_NBC_OP_REDUCE(_mpi_op, _c_op, _c_type)\
	MPC_NBC_OP_LOOP(_mpi_op,\
	{\
		*( ( (_c_type *) buf3 ) + i ) = *( ( (_c_type *) buf1 ) + i ) _c_op *( ( (_c_type *) buf2 ) + i );\
	})

/**
 * Reduction shorthand for macros `c = bool(a) OP bool(b)`,
 * where a and b need to be normalized to 0 or 1
 * prior to calling OP.
 * Only used for logical XOR (LXOR).
 */
#define MPC_NBC_OP_REDUCE_NORM_LOGICAL(_mpi_op, _c_op, _c_type)\
	MPC_NBC_OP_LOOP(_mpi_op,\
	{\
		*( ( (_c_type *) buf3 ) + i ) = ( *( ( (_c_type *) buf1 ) + i ) ? 1 : 0)\
			_c_op ( *( ( (_c_type *) buf2 ) + i ) ? 1 : 0);\
	})

/*
 * Comparison-assignment operators `c = (a OP b) ? b : a`
 */
#define MPC_NBC_OP_MIN(_c_type) MPC_NBC_OP_COMPARE_REDUCE(MPI_MIN, >, _c_type)
#define MPC_NBC_OP_MAX(_c_type) MPC_NBC_OP_COMPARE_REDUCE(MPI_MAX, <, _c_type)

/*
 * Comparison-assignment operators `c = (a OP b) ? b : a` that attach the index of the min/max value
 */
#define MPC_NBC_OP_MINLOC(_c_type, _c_type_name) MPC_NBC_OP_COMPARE_REDUCE_LOC(MPI_MINLOC, >, _c_type, _c_type_name)
#define MPC_NBC_OP_MAXLOC(_c_type, _c_type_name) MPC_NBC_OP_COMPARE_REDUCE_LOC(MPI_MAXLOC, <, _c_type, _c_type_name)

/*
 * `c = a OP b` operators
 */
#define MPC_NBC_OP_SUM(_c_type)  MPC_NBC_OP_REDUCE(MPI_SUM, +, _c_type)
#define MPC_NBC_OP_PROD(_c_type) MPC_NBC_OP_REDUCE(MPI_PROD, *, _c_type)
#define MPC_NBC_OP_LAND(_c_type) MPC_NBC_OP_REDUCE(MPI_LAND, &&, _c_type)
#define MPC_NBC_OP_BAND(_c_type) MPC_NBC_OP_REDUCE(MPI_BAND, &, _c_type)
#define MPC_NBC_OP_LOR(_c_type)  MPC_NBC_OP_REDUCE(MPI_LOR, ||, _c_type)
#define MPC_NBC_OP_BOR(_c_type)  MPC_NBC_OP_REDUCE(MPI_BOR, |, _c_type)
#define MPC_NBC_OP_LXOR(_c_type) MPC_NBC_OP_REDUCE_NORM_LOGICAL(MPI_LXOR, ^, _c_type)
#define MPC_NBC_OP_BXOR(_c_type) MPC_NBC_OP_REDUCE(MPI_BXOR, ^, _c_type)


/**
 * Define the list of reduction operators allowed for the "C Integer" group of
 * MPI basic datatypes, as defined in the MPI Standard 4.1, Section 6.9.2.
 */
#define MPC_NBC_OP_DEFINE_C_INTEGER(_c_type)\
	{\
		MPC_NBC_OP_START\
\
		/* Min, max */\
		MPC_NBC_OP_MIN(_c_type)\
		MPC_NBC_OP_MAX(_c_type)\
\
		/* Sum, product */\
		MPC_NBC_OP_SUM(_c_type)\
		MPC_NBC_OP_PROD(_c_type)\
\
		/* Logical And, Or, Xor */\
		MPC_NBC_OP_LAND(_c_type)\
		MPC_NBC_OP_LOR(_c_type)\
		MPC_NBC_OP_LXOR(_c_type)\
\
		/* Binary And, Or, Xor */\
		MPC_NBC_OP_BAND(_c_type)\
		MPC_NBC_OP_BOR(_c_type)\
		MPC_NBC_OP_BXOR(_c_type)\
\
		MPC_NBC_OP_END\
	}

/**
 * Define the list of reduction operators allowed for the "Floating Point" group of
 * MPI basic datatypes, as defined in the MPI Standard 4.1, Section 6.9.2.
 */
#define MPC_NBC_OP_DEFINE_FLOATING_POINT(_c_type)\
	{\
		MPC_NBC_OP_START\
\
		/* Min, max */\
		MPC_NBC_OP_MIN(_c_type)\
		MPC_NBC_OP_MAX(_c_type)\
\
		/* Sum, product */\
		MPC_NBC_OP_SUM(_c_type)\
		MPC_NBC_OP_PROD(_c_type)\
\
		MPC_NBC_OP_END\
	}

/**
 * Define the list of reduction operators allowed for the "Logical" group of
 * MPI basic datatypes, as defined in the MPI Standard 4.1, Section 6.9.2.
 */
#define MPC_NBC_OP_DEFINE_LOGICAL(_c_type)\
	{\
		MPC_NBC_OP_START\
\
		/* Logical And, Or, Xor */\
		MPC_NBC_OP_LAND(_c_type)\
		MPC_NBC_OP_LOR(_c_type)\
		MPC_NBC_OP_LXOR(_c_type)\
\
		MPC_NBC_OP_END\
	}

/**
 * Define the list of reduction operators allowed for the "Complex" group of
 * MPI basic datatypes, as defined in the MPI Standard 4.1, Section 6.9.2.
 */
#define MPC_NBC_OP_DEFINE_COMPLEX(_c_type)\
	{\
		MPC_NBC_OP_START\
\
		/* Sum, product */\
		MPC_NBC_OP_SUM(_c_type)\
		MPC_NBC_OP_PROD(_c_type)\
\
		MPC_NBC_OP_END\
	}

/**
 * Define the list of reduction operators allowed for the "Byte" group of
 * MPI basic datatypes, as defined in the MPI Standard 4.1, Section 6.9.2.
 */
#define MPC_NBC_OP_DEFINE_BYTE(_c_type)\
	{\
		MPC_NBC_OP_START\
\
		/* Binary And, Or, Xor */\
		MPC_NBC_OP_BAND(_c_type)\
		MPC_NBC_OP_BOR(_c_type)\
		MPC_NBC_OP_BXOR(_c_type)\
\
		MPC_NBC_OP_END\
	}

/**
 * Define the list of reduction operators allowed for the "Multi-language types" group of
 * MPI basic datatypes, as defined in the MPI Standard 4.1, Section 6.9.2.
 */
#define MPC_NBC_OP_DEFINE_MULTI_LANG(_c_type)\
	{\
		MPC_NBC_OP_START\
\
		/* Min, max */\
		MPC_NBC_OP_MIN(_c_type)\
		MPC_NBC_OP_MAX(_c_type)\
\
		/* Sum, product */\
		MPC_NBC_OP_SUM(_c_type)\
		MPC_NBC_OP_PROD(_c_type)\
\
		/* Binary And, Or, Xor */\
		MPC_NBC_OP_BAND(_c_type)\
		MPC_NBC_OP_BOR(_c_type)\
		MPC_NBC_OP_BXOR(_c_type)\
\
		MPC_NBC_OP_END\
	}

/**
 * Define the list of reduction operators allowed for the minloc/maxloc tuple types,
 * as defined in the MPI Standard 4.1, Section 6.9.4.
 *
 * @param _c_type The actual C type name, e.g `double` or `long double`
 * @param _c_type_name The C type name with no whitespace, e.g. `double` or `long_double`
 * @see MPC_NBC_OP_DEFINE_LOC if _c_type does not contain any whitespace, i.e. _c_type == _c_type_name
 */
#define MPC_NBC_OP_DEFINE_LOC_NAME(_c_type, _c_type_name)\
	{\
		MPC_NBC_OP_START\
\
		/* Minloc, maxloc */\
		MPC_NBC_OP_MINLOC(_c_type, _c_type_name)\
		MPC_NBC_OP_MAXLOC(_c_type, _c_type_name)\
\
		MPC_NBC_OP_END\
	}

#define MPC_NBC_OP_DEFINE_LOC(_c_type) MPC_NBC_OP_DEFINE_LOC_NAME(_c_type, _c_type)

#ifdef __cplusplus
}
#endif

#endif

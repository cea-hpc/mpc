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

#include <complex>

#include "mpc_mpi.h"
#include "mpc_nbc_reduction_macros.h"

int __mpc_mpi_reduce_cpp_type(void *buf3, void *buf1, void *buf2, MPI_Op op, MPI_Datatype type, int count)
{
	int i;

	if ( type == MPI_CXX_BOOL )
	{
		MPC_NBC_OP_DEFINE_LOGICAL(bool)
	}
	else if ( type == MPI_CXX_FLOAT_COMPLEX )
	{
		MPC_NBC_OP_DEFINE_COMPLEX(std::complex<float>)
	}
	else if ( type == MPI_CXX_DOUBLE_COMPLEX )
	{
		MPC_NBC_OP_DEFINE_COMPLEX(std::complex<double>)
	}
	else if ( type == MPI_CXX_LONG_DOUBLE_COMPLEX )
	{
		MPC_NBC_OP_DEFINE_COMPLEX(std::complex<long double>)
	}
	else
	{
		return NBC_DATATYPE_NOT_SUPPORTED;
	}
	
	return NBC_OK;
}

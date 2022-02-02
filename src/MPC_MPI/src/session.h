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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_MPI_SESSION_H
#define MPC_MPI_SESSION_H

#include <mpc_mpi.h>
#include <mpc_lowcomm_handle_ctx.h>

typedef struct mpc_mpi_session_s
{
	int            id;
	MPI_Errhandler errh;
	MPI_Info       infos;
	mpc_lowcomm_handle_ctx_t handle_ctx;
}mpc_mpi_session_t;

MPI_Session mpc_mpi_session_f2c(int session_id);
int mpc_mpi_session_c2f(MPI_Session session);


#endif /* MPC_MPI_SESSION_H */

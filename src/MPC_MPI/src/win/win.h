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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPI_WIN_H 
#define MPI_WIN_H 

#include <mpc_mpi_internal.h> //FIXME: for MPI_Info
#include <mpc_lowcomm_communicator.h> //NOTE: for lowcomm_communicator_t

#define MAX_WIN_NAME 64

typedef struct MPI_ABI_Win {
        mpc_lowcomm_communicator_t comm;
        MPI_Info info;

        char win_name[MAX_WIN_NAME];

        int flavor; 
} mpc_win_t;

#endif

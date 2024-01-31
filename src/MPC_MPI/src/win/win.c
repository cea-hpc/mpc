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

#include "win.h"

#include "lcp.h"

int mpc_win_create(void *base, size_t size, 
                   int disp_unit, MPI_Info info,
                   mpc_lowcomm_communicator_t comm, 
                   mpc_win_t *win)
{
        mpc_win_t *win;

        /* Allocate the window. */
        win = sctk_malloc(sizeof(mpc_win_t));

        /* Initialize fields. */
        win->flavor = MPI_WIN_FLAVOR_CREATE;
        win->info = info;

	PMPI_Comm_dup(comm, &win->comm);
}

static int mpc_alloc_window(

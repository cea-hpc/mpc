/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:29:07 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPC_THREAD_MPI_OMP_INTEROP_H__
# define __MPC_THREAD_MPI_OMP_INTEROP_H__

# include <mpc_config.h>

# if MPC_OpenMP && MPC_MPI

/** If true, enable MPI/OMP interoperability */
#  define MPC_ENABLE_INTEROP_MPI_OMP 0
#  if MPC_ENABLE_INTEROP_MPI_OMP
#   include "mpi.h"
#   include "mpc_omp_task_trace.h"

/**
 * To be called by the MPI runtime when it is about
 * to wait the `n` requests of the `req` array
 */
int mpc_thread_mpi_omp_wait(int n, MPI_Request * req, int * index, MPI_Status * statuses);

#  endif /* MPC_ENABLE_INTEROP_MPI_OMP */
# else /* MPC_OpenMP && MPC_MPI */
#  define MPC_ENABLE_INTEROP_MPI_OMP 0
# endif /* MPC_OpenMP && MPC_MPI */

#endif /* __MPC_THREAD_MPI_OMP_INTEROP_H__ */

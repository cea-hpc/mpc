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
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #   - PEPIN Thibaut thibaut.pepin@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

/* Collectives  */
#pragma weak MPI_Ibcast = PMPI_Ibcast
#pragma weak MPI_Bcast_init = PMPI_Bcast_init

#pragma weak MPI_Ireduce = PMPI_Ireduce
#pragma weak MPI_Reduce_init = PMPI_Reduce_init

#pragma weak MPI_Iallreduce = PMPI_Iallreduce
#pragma weak MPI_Allreduce_init = PMPI_Allreduce_init

#pragma weak MPI_Iscatter = PMPI_Iscatter
#pragma weak MPI_Scatter_init = PMPI_Scatter_init

#pragma weak MPI_Iscatterv = PMPI_Iscatterv
#pragma weak MPI_Scatterv_init = PMPI_Scatterv_init

#pragma weak MPI_Igather = PMPI_Igather
#pragma weak MPI_Gather_init = PMPI_Gather_init

#pragma weak MPI_Igatherv = PMPI_Igatherv
#pragma weak MPI_Gatherv_init = PMPI_Gatherv_init

#pragma weak MPI_Ireduce_scatter_block = PMPI_Ireduce_scatter_block
#pragma weak MPI_Reduce_scatter_block_init = PMPI_Reduce_scatter_block_init

#pragma weak MPI_Iallgather = PMPI_Iallgather
#pragma weak MPI_Allgather_init = PMPI_Allgather_init

#pragma weak MPI_Ialltoall = PMPI_Ialltoall
#pragma weak MPI_Alltoall_init = PMPI_Alltoall_init

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
/* #                                                                      # */
/* ######################################################################## */

/* Non Blocking Collectives  */
#pragma weak MPI_Iallreduce = PMPI_Iallreduce
#pragma weak MPI_Ibarrier = PMPI_Ibarrier
#pragma weak MPI_Ibcast = PMPI_Ibcast
#pragma weak MPI_Igather = PMPI_Igather
#pragma weak MPI_Igatherv = PMPI_Igatherv
#pragma weak MPI_Iscatter = PMPI_Iscatter
#pragma weak MPI_Iscatterv = PMPI_Iscatterv
#pragma weak MPI_Iallgather = PMPI_Iallgather
#pragma weak MPI_Iallgatherv = PMPI_Iallgatherv
#pragma weak MPI_Ialltoall = PMPI_Ialltoall
#pragma weak MPI_Ialltoallv = PMPI_Ialltoallv
#pragma weak MPI_Ialltoallw = PMPI_Ialltoallw
#pragma weak MPI_Ireduce = PMPI_Ireduce
#pragma weak MPI_Ireduce_scatter = PMPI_Ireduce_scatter
#pragma weak MPI_Ireduce_scatter_block = PMPI_Ireduce_scatter_block
#pragma weak MPI_Iscan = PMPI_Iscan
#pragma weak MPI_Iexscan = PMPI_Iexscan

#pragma weak mpi_iallreduce_ = pmpi_iallreduce_
#pragma weak mpi_ibarrier_ = pmpi_ibarrier_
#pragma weak mpi_ibcast_ = pmpi_ibcast_
#pragma weak mpi_igather_ = pmpi_igather_
#pragma weak mpi_igatherv_ = pmpi_igatherv_
#pragma weak mpi_iscatter_ = pmpi_iscatter_
#pragma weak mpi_iscatterv_ = pmpi_iscatterv_
#pragma weak mpi_iallgather_ = pmpi_iallgather_
#pragma weak mpi_iallgatherv_ = pmpi_iallgatherv_
#pragma weak mpi_ialltoall_ = pmpi_ialltoall_
#pragma weak mpi_ialltoallv_ = pmpi_ialltoallv_
#pragma weak mpi_ialltoallw_ = pmpi_ialltoallw_
#pragma weak mpi_ireduce_ = pmpi_ireduce_
#pragma weak mpi_ireduce_scatter_ = pmpi_ireduce_scatter_
#pragma weak mpi_ireduce_scatter_block_ = pmpi_ireduce_scatter_block_
#pragma weak mpi_iscan_ = pmpi_iscan_
#pragma weak mpi_iexscan_ = pmpi_iexscan_

#pragma weak mpi_iallreduce__ = pmpi_iallreduce__
#pragma weak mpi_ibarrier__ = pmpi_ibarrier__
#pragma weak mpi_ibcast__ = pmpi_ibcast__
#pragma weak mpi_igather__ = pmpi_igather__
#pragma weak mpi_igatherv__ = pmpi_igatherv__
#pragma weak mpi_iscatter__ = pmpi_iscatter__
#pragma weak mpi_iscatterv__ = pmpi_iscatterv__
#pragma weak mpi_iallgather__ = pmpi_iallgather__
#pragma weak mpi_iallgatherv__ = pmpi_iallgatherv__
#pragma weak mpi_ialltoall__ = pmpi_ialltoall__
#pragma weak mpi_ialltoallv__ = pmpi_ialltoallv__
#pragma weak mpi_ialltoallw__ = pmpi_ialltoallw__
#pragma weak mpi_ireduce__ = pmpi_ireduce__
#pragma weak mpi_ireduce_scatter__ = pmpi_ireduce_scatter__
#pragma weak mpi_ireduce_scatter_block__ = pmpi_ireduce_scatter_block__
#pragma weak mpi_iscan__ = pmpi_iscan__
#pragma weak mpi_iexscan__ = pmpi_iexscan__




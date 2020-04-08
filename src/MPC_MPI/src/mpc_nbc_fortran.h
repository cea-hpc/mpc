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

char * sctk_char_fortran_to_c (char *buf, int size, char ** free_ptr );
void sctk_char_c_to_fortran (char *buf, int size);

/* Non Blocking Collectives Operations */
void ffunc(mpi_iallreduce) (void *sendbuf, void *recvbuf, int count,MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,MPI_Request *request);

void ffunc(mpi_ibarrier) (MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ibcast) (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_igather) (void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_igatherv) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_iscatter) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_iscatterv) (void *sendbuf, int sendcounts[], int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_iallgather) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_iallgatherv) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ialltoall) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ialltoallv) (void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ialltoallw) (void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ireduce) (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ireduce_scatter) (void *sendbuf, void *recvbuf, int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_ireduce_scatter_block) (void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_iscan) (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);

void ffunc(mpi_iexscan) (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);



/* Non Blocking Collectives PMPI fortran binding */

void ffunc(pmpi_iallreduce) (void *sendbuf, void *recvbuf, int *count, MPI_Datatype * datatype, MPI_Op *op, MPI_Comm *comm,MPI_Request *request, int *res)
{
	*res= PMPI_Iallreduce(sendbuf, recvbuf, *count, *datatype, *op, *comm, request);
}

void ffunc(pmpi_ibarrier) (MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ibarrier(*comm, request);
}


void ffunc(pmpi_ibcast) (void *buffer, int *count, MPI_Datatype *datatype, int *root, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ibcast (buffer, *count, *datatype, *root, *comm, request);
}

void ffunc(pmpi_igather) (void *sendbuf, int *sendcount, MPI_Datatype *sendtype, void *recvbuf, int *recvcount, MPI_Datatype *recvtype, int *root, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Igather (sendbuf, *sendcount, *sendtype, recvbuf, *recvcount, *recvtype, *root, *comm, request);
}

void ffunc(pmpi_igatherv) (void *sendbuf, int *sendcount, MPI_Datatype *sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype *recvtype, int *root, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Igatherv (sendbuf, *sendcount, *sendtype, recvbuf, recvcounts, displs, *recvtype, *root, *comm, request);
}

void ffunc(pmpi_iscatter) (void *sendbuf, int *sendcount, MPI_Datatype *sendtype, void *recvbuf, int *recvcount, MPI_Datatype *recvtype, int *root, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Iscatter (sendbuf, *sendcount, *sendtype, recvbuf, *recvcount, *recvtype, *root, *comm, request);
}

void ffunc(pmpi_iscatterv) (void *sendbuf, int *sendcounts, int *displs, MPI_Datatype *sendtype, void *recvbuf, int *recvcount, MPI_Datatype *recvtype, int *root, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Iscatterv (sendbuf, sendcounts, displs, *sendtype, recvbuf, *recvcount, *recvtype, *root, *comm, request); 
}

void ffunc(pmpi_iallgather) (void *sendbuf, int *sendcount, MPI_Datatype *sendtype, void *recvbuf, int *recvcount, MPI_Datatype *recvtype, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Iallgather (sendbuf, *sendcount, *sendtype, recvbuf, *recvcount, *recvtype, *comm, request);
}

void ffunc(pmpi_iallgatherv) (void *sendbuf, int *sendcount, MPI_Datatype *sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype *recvtype, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Iallgatherv (sendbuf, *sendcount, *sendtype, recvbuf, recvcounts, displs, *recvtype, *comm, request);
}

void ffunc(pmpi_ialltoall) (void *sendbuf, int *sendcount, MPI_Datatype *sendtype, void *recvbuf, int *recvcount, MPI_Datatype *recvtype, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ialltoall (sendbuf, *sendcount, *sendtype, recvbuf, *recvcount, *recvtype, *comm, request);
}

void ffunc(pmpi_ialltoallv) (void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype *sendtype, void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype *recvtype, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ialltoallv (sendbuf, sendcounts, sdispls, *sendtype, recvbuf, recvcounts, rdispls, *recvtype, *comm, request);
}

void ffunc(pmpi_ialltoallw) (void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype *sendtypes, void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ialltoallw (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, *comm, request);
}

void ffunc(pmpi_ireduce) (void *sendbuf, void *recvbuf, int *count, MPI_Datatype *datatype, MPI_Op *op, int *root, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ireduce (sendbuf, recvbuf, *count, *datatype, *op, *root, *comm, request);
}

void ffunc(pmpi_ireduce_scatter) (void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype *datatype, MPI_Op *op, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ireduce_scatter (sendbuf, recvbuf, recvcounts, *datatype, *op, *comm, request);
}

void ffunc(pmpi_ireduce_scatter_block) (void *sendbuf, void *recvbuf, int *recvcount, MPI_Datatype *datatype, MPI_Op *op, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Ireduce_scatter_block (sendbuf, recvbuf, *recvcount, *datatype, *op, *comm, request);
}

void ffunc(pmpi_iscan) (void *sendbuf, void *recvbuf, int *count, MPI_Datatype *datatype, MPI_Op *op, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Iscan (sendbuf, recvbuf, *count, *datatype, *op, *comm, request);
}

void ffunc(pmpi_iexscan) (void *sendbuf, void *recvbuf, int *count, MPI_Datatype *datatype, MPI_Op *op, MPI_Comm *comm, MPI_Request *request, int *res)
{
	*res = PMPI_Iexscan (sendbuf, recvbuf, *count, *datatype, *op, *comm, request);
}




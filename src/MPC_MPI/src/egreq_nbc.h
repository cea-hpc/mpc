#ifndef EGREQ_NBC_H
#define EGREQ_NBC_H

#include "nbc.h"


int NBC_Operation(void *buf3, void *buf1, void *buf2, MPI_Op op, MPI_Datatype type, int count);

int MPI_Ixbcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ixbarrier( MPI_Comm comm_in , MPI_Request * req );
int MPI_Ixgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm_in, MPI_Request * request);
int MPI_Ixscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request);
int MPI_Ixreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);

#endif /* EGREQ_NBC_H */

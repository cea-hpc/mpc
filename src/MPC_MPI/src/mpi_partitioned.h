#ifndef MPI_PARTITIONED_H
#define MPI_PARTITIONED_H

#include "mpc_mpi.h"
#include "mpc_common_rank.h"

typedef struct MPI_internal_request_s MPI_internal_request_t;

int mpi_psend_init(const void *buf, int partitions, int count,
                    MPI_Datatype datatype, int dest, int tag,
                    MPI_Comm comm, MPI_internal_request_t *req);

int mpi_precv_init(void *buf, int partitions, int count,
                    MPI_Datatype datatype, int source, int tag,
                    MPI_Comm comm, MPI_internal_request_t *req);

int mpi_pstart(MPI_internal_request_t *req);
int mpi_pready(int partition, MPI_internal_request_t *req);
int mpi_parrived(int partition, MPI_internal_request_t *req, int *flags_p);

#endif

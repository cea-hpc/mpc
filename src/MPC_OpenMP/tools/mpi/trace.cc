# include <assert.h>
# include <pthread.h>
# include <mpi.h>
# include <mpc_omp_task_trace.h>

# include "uthash.h"

typedef struct
{
    MPI_Comm comm;
    int rank;
    OPA_int_t traced;
    UT_hash_handle hh;
} mpi_rank_t;

static mpi_rank_t * mpi_ranks = NULL;
static pthread_mutex_t mpi_ranks_mutex = PTHREAD_MUTEX_INITIALIZER;
static OPA_int_t callback_saved = { 0 };

static int
comm_to_int(MPI_Comm comm)
{
    // TODO
    return 0;
}

static int
__trace_communicators_rank(void * unused)
{
    (void) unused;

    pthread_mutex_lock(&mpi_ranks_mutex);
    {
        mpi_rank_t * mpi_rank, * tmp;
        HASH_ITER(hh, mpi_ranks, mpi_rank, tmp)
        {
            MPC_OMP_TASK_TRACE_RANK(comm_to_int(mpi_rank->comm), mpi_rank->rank);
            HASH_DEL(mpi_ranks, mpi_rank);
        }
    }
    pthread_mutex_unlock(&mpi_ranks_mutex);
    return 1;
}

/* save a callback for the ending of tracing, to trace the communicators rank of the current process */
static void
__ensure_trace_end_callback(void)
{
    if (mpc_omp_task_trace_begun() && OPA_cas_int(&callback_saved, 0, 1) == 0)
    {
        mpc_omp_callback(
            __trace_communicators_rank,
            NULL,
            MPC_OMP_CALLBACK_SCOPE_INSTANCE,
            MPC_OMP_CALLBACK_TASK_TRACE_END,
            MPC_OMP_CALLBACK_REPEAT_RETURN
        );
    }
}

int
MPI_Comm_rank(MPI_Comm comm, int * rank)
{
    /* ensure the end callback is registered to trace communicators rank */
    __ensure_trace_end_callback();

    int r = PMPI_Comm_rank(comm, rank);
    pthread_mutex_lock(&mpi_ranks_mutex);
    {
        mpi_rank_t * mpi_rank = NULL;
        HASH_FIND(hh, mpi_ranks, &comm, sizeof(MPI_Comm), mpi_rank);

        if (mpi_rank == NULL)
        {
            mpi_rank = (mpi_rank_t *) malloc(sizeof(mpi_rank_t));
            assert(mpi_rank);

            mpi_rank->comm      = comm;
            mpi_rank->rank      = *rank;
            mpi_rank->traced    = { 1 };

            HASH_ADD(hh, mpi_ranks, comm, sizeof(MPI_Comm), mpi_rank);
        }
    }
    pthread_mutex_unlock(&mpi_ranks_mutex);
    return r;
}

int
MPI_Isend(const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request)
{
    /* ensure the end callback is registered to trace communicators rank */
    __ensure_trace_end_callback();

    // prototype : MPC_OMP_TASK_TRACE_SEND(count, dtype, dst, tag, comm)
    MPC_OMP_TASK_TRACE_SEND(count, 0, dest, tag, 0, 0);

    int r = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);

    // prototype : MPC_OMP_TASK_TRACE_SEND(count, dtype, dst, tag, comm)
    MPC_OMP_TASK_TRACE_SEND(count, 0, dest, tag, 0, 1);

    return r;
}

int
MPI_Irecv(void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request)
{
    /* ensure the end callback is registered to trace communicators rank */
    __ensure_trace_end_callback();

    // prototype : MPC_OMP_TASK_TRACE_RECV(count, dtype, src, tag, comm, start|end)
    MPC_OMP_TASK_TRACE_RECV(count, 0, source, tag, 0, 0);

    int r = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);

    // prototype : MPC_OMP_TASK_TRACE_RECV(count, dtype, src, tag, comm, start|end)
    MPC_OMP_TASK_TRACE_RECV(count, 0, source, tag, 0, 1);

    return r;
}

int
MPI_Iallreduce(const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request)
{
    /* ensure the end callback is registered to trace communicators rank */
    __ensure_trace_end_callback();

    // prototype : MPC_OMP_TASK_TRACE_ALLREDUCE(count, dtype, op, comm)
    MPC_OMP_TASK_TRACE_ALLREDUCE(count, 0, 0, 0, 0);

    int r = PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request);

    // prototype : MPC_OMP_TASK_TRACE_ALLREDUCE(count, dtype, op, comm)
    MPC_OMP_TASK_TRACE_ALLREDUCE(count, 0, 0, 0, 1);

    return r;
}

#ifndef MPC_OMP_INTEROP_MPI_H
# define MPC_OMP_INTEROP_MPI_H

# include <assert.h>
# include <mpi.h>
# include <mpc_omp.h>

# define MPIX_PARTITION_NONE (-1)

typedef struct  mpix_progress_info_s
{
    /* the omp event */
    mpc_omp_event_handle_block_t * handle;

    /* the request array */
    MPI_Request * req;

    /* status */
    MPI_Status * status;

    /* index */
    int * index;

    /* number of requests */
    unsigned int n;

    /* the partition id in case of partitionned communication */
    int partition;
}               mpix_progress_info_t;

/* test of request completion */
typedef int (*mpc_lowcomm_request_test_t)(mpix_progress_info_t *);

static int
__request_test(mpix_progress_info_t * infos)
{
    assert(infos->n == 1);
    assert(infos->index == NULL);
    assert(infos->partition == MPIX_PARTITION_NONE);

    int completed;
    MPI_Test(infos->req, &completed, infos->status);
    return completed;
}

static int
__request_testall(mpix_progress_info_t * infos)
{
    assert(infos->n > 1);
    assert(infos->index == NULL);
    assert(infos->partition == MPIX_PARTITION_NONE);

    int completed;
    MPI_Testall(infos->n, infos->req, &completed, infos->status);
    return completed;
}

static int
__request_testany(mpix_progress_info_t * infos)
{
    assert(infos->n > 1);
    assert(infos->index != NULL);
    assert(infos->partition == MPIX_PARTITION_NONE);

    int completed;
    MPI_Testany(infos->n, infos->req, infos->index, &completed, infos->status);
    return completed;
}

static int
__request_testpartition(mpix_progress_info_t * infos)
{
    assert(infos->n == 1);
    assert(infos->index == NULL);
    assert(infos->partition != MPIX_PARTITION_NONE);

    int completed;
    MPI_Parrived(*infos->req, infos->partition, &completed);
    return completed;
}

static mpc_lowcomm_request_test_t
__request_get_test(mpix_progress_info_t * infos)
{
    if (infos->n == 1)                              return __request_test;
    if (infos->index == NULL)                       return __request_testall;
    if (infos->index != NULL)                       return __request_testany;
    if (infos->partition != MPIX_PARTITION_NONE)    return __request_testpartition;
    assert(0); return NULL;
}

/** Progress the request */

static int
__request_progress(mpix_progress_info_t * infos)
{
    mpc_lowcomm_request_test_t test = __request_get_test(infos);

    /* check if the request completed */
    if (test(infos))
    {
        mpc_omp_fulfill_event((mpc_omp_event_handle_t *) infos->handle);
        return 0;
    }

    /* check if the blocking event was cancelled */
    if (infos->handle->cancel && OPA_load_int(infos->handle->cancel))
    {
        if (OPA_cas_int(&(infos->handle->cancelled), 0, 1) == 0)
        {
            /** cancel MPI requests, but do not fulfill the event
             *  the event will be fulfilled later on
             *  by a succesful MPI_Test on the cancelled request */
            unsigned int i;
            for (i = 0 ; i < infos->n ; ++i)
            {
                MPI_Cancel(infos->req + i);
            }
        }
    }
    return 1;
}

// TODO : this could be optimized by creating an individual entry point
// per blocking type (MPI_Wait, MPI_Waitall, MPI_PWait)
static inline void
___task_block(int n, MPI_Request * reqs, int * index, MPI_Status * status, int partition)
{
    /* generate test function */
    mpix_progress_info_t infos;
    infos.req       = reqs;
    infos.index     = index;
    infos.status    = status;
    infos.n         = n;
    infos.partition = partition;

    /* test before blocking */
    mpc_lowcomm_request_test_t test = __request_get_test(&infos);
    if (test(&infos)) return ;

    /* communication is blocking, block the task */

    /* create a task-blocking handler */
    mpc_omp_event_handle_init(
        (mpc_omp_event_handle_t **) &(infos.handle),
        MPC_OMP_EVENT_TASK_BLOCK);

    /* register the progression callback */
    mpc_omp_callback(
        (int (*)(void *)) __request_progress,
        &infos,
        MPC_OMP_CALLBACK_SCOPE_INSTANCE,
        MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE,
        MPC_OMP_CALLBACK_REPEAT_RETURN
    );

    /* block current task until the associated event is fulfilled */
    mpc_omp_task_block(infos.handle);

    /* deinit the handle */
    mpc_omp_event_handle_deinit((mpc_omp_event_handle_t *) infos.handle);
}

/** Parameters correspond to the ones of MPI_Test, MPI_Testall and MPI_Testany */
static inline int
mpc_thread_mpi_omp_wait(int n, MPI_Request * reqs, int * index, MPI_Status * status, int partition)
{
    if (mpc_omp_in_explicit_task())
    {
        /* suspend the task */
        ___task_block(n, reqs, index, status, partition);
        return 1;
    }
    return 0;
}

/* these routines suspend current task until the associated request is completed */
static inline int
MPIX_Wait(MPI_Request * request, MPI_Status * status)
{
    if (mpc_thread_mpi_omp_wait(1, request, NULL, status, MPIX_PARTITION_NONE)) return 0;
    return MPI_Wait(request, status);
}

static inline int
MPIX_Waitall(int n, MPI_Request * requests, MPI_Status * status)
{
    if (mpc_thread_mpi_omp_wait(n, requests, NULL, status, MPIX_PARTITION_NONE)) return 0;
    return MPI_Waitall(n, requests, status);
}

/* Wait for a partition of a Partionned Request to arrive */
static inline int
MPIX_PWait(MPI_Request * request, MPI_Status * status, int partition)
{
    if (mpc_thread_mpi_omp_wait(1, request, NULL, status, partition)) return 0;

    // TODO : there is no 'MPI_PWait' in MPI standards, to wait
    // for a specific partition to arrive. Implement it naively for now.
    int flag;
    do {
        MPI_Parrived(*request, partition, &flag);
    } while (!flag);
    return 0;
}

#endif /* MPC_OMP_INTEROP_MPI_H */

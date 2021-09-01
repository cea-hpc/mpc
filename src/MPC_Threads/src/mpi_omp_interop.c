# include "mpc_thread_mpi_omp_interop.h"

# if MPC_ENABLE_INTEROP_MPI_OMP
# include "mpc_omp.h"
# include "mpc_mpi_internal.h"
# include "mpc_lowcomm_msg.h"
# include "mpc_lowcomm_types.h"

typedef struct  mpc_lowcomm_request_omp_progress_s
{
    /* the omp event */
    mpc_omp_event_handle_t * event;

    /* the request array */
    MPI_Request * req;

    /* statuses */
    MPI_Status * statuses;

    /* index */
    int * index;

    /* number of requests */
    int n;
}               mpc_lowcomm_request_omp_progress_t;

/* test of request completion */
typedef int (*mpc_lowcomm_request_test_t)(mpc_lowcomm_request_omp_progress_t *);

static int
__request_test(mpc_lowcomm_request_omp_progress_t * infos)
{
    assert(infos->n == 1);

    int completed;
    MPI_Test(infos->req, &completed, infos->statuses);
    return completed;
}

static int
__request_testall(mpc_lowcomm_request_omp_progress_t * infos)
{
    assert(infos->n > 1);
    assert(infos->index == NULL);

    int completed;
    MPI_Testall(infos->n, infos->req, &completed, infos->statuses);
    return completed;
}

static int
__request_testany(mpc_lowcomm_request_omp_progress_t * infos)
{
    assert(infos->n > 1);
    assert(infos->index);

    int completed;
    MPI_Testany(infos->n, infos->req, infos->index, &completed, infos->statuses);
    return completed;
}

static mpc_lowcomm_request_test_t
__request_get_test(mpc_lowcomm_request_omp_progress_t * infos)
{
    if (infos->n == 1)
    {
        return __request_test;
    }
    if (infos->index == NULL)
    {
        return __request_testall;
    }
    return __request_testany;
}

/** Progress the request */

static int
__request_progress(mpc_lowcomm_request_omp_progress_t * infos)
{
    mpc_lowcomm_request_test_t test = __request_get_test(infos);
    if (test(infos))
    {
        mpc_omp_fulfill_event(infos->event);
        return 0;
    }
    return 1;
}

/* task blocking */

static inline void
___task_block(int n, MPI_Request * reqs, int * index, MPI_Status * statuses)
{
    /* generate test function */
    mpc_lowcomm_request_omp_progress_t infos;
    infos.req       = reqs;
    infos.index     = index;
    infos.statuses  = statuses;
    infos.n         = n;

    /* test before blocking */
    mpc_lowcomm_request_test_t test = __request_get_test(&infos);
    if (test(&infos))
    {
        return ;
    }

    /* communication is blocking, block the task */

    /* progression callback */
    mpc_omp_event_handle_t event;
    mpc_omp_event_handle_init(&event, MPC_OMP_EVENT_TASK_BLOCK);

    infos.event = &event;

    mpc_omp_callback_t callback;
    callback.func      = (int (*)(void *)) __request_progress;
    callback.data      = &infos;
    callback.when      = MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE;
    callback.repeat    = MPC_OMP_CALLBACK_REPEAT_RETURN;
    callback.n         = 0;

    /* register the progression callback */
    mpc_omp_callback(&callback);

    /* block current task until the associated event is fulfilled */
    mpc_omp_task_block(&event);
}

/** Parameters correspond to the ones of MPI_Test, MPI_Testall and MPI_Testany */
int
mpc_thread_mpi_omp_wait(int n, MPI_Request * reqs, int * index, MPI_Status * statuses)
{
    if (mpc_omp_in_explicit_task())
    {
        /* suspend the task*/
        ___task_block(n, reqs, index, statuses);

        return 1;
    }
    return 0;
}

# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

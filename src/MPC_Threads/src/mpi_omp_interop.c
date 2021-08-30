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

    /* number of requests */
    int n;
}               mpc_lowcomm_request_omp_progress_t;

static int
__request_progress(mpc_lowcomm_request_omp_progress_t * infos)
{
    int completed;
    MPI_Testall(infos->n, infos->req, &completed, MPI_STATUSES_IGNORE);
    if (completed)
    {
        mpc_omp_fulfill_event(infos->event);
        return 0;
    }
    return 1;
}

static inline void
___task_block(int n, MPI_Request * reqs)
{
    /* test once before suspending */
    int completed;
    MPI_Testall(n, reqs, &completed, MPI_STATUSES_IGNORE);
    if (completed)
    {
        return ;
    }

    /* prepare the communication progression callback */
    mpc_omp_event_handle_t event;
    mpc_omp_event_handle_init(&event, MPC_OMP_EVENT_TASK_BLOCK);

    mpc_lowcomm_request_omp_progress_t infos;
    infos.event = &event;
    infos.req   = reqs;
    infos.n     = n;

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

int
mpc_thread_mpi_omp_wait(int n, MPI_Request * reqs)
{
    if (mpc_omp_in_explicit_task())
    {
        /* suspend the task*/
        ___task_block(n, reqs);

        return 1;
    }
    return 0;
}

# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

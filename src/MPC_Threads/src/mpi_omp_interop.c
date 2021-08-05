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
    /* TODO : maybe add a MPI_Test before suspending */

    /* suspend the task until associated requests terminated */
    mpc_omp_event_handle_t event;
    memset(&event, 0, sizeof(mpc_omp_event_handle_t));

    mpc_lowcomm_request_omp_progress_t infos;
    infos.event = &event;
    infos.req   = reqs;
    infos.n     = n;

    /* create the progression callback */
    mpc_omp_callback_t callback;
    callback.func      = (int (*)(void *)) __request_progress;
    callback.data      = &infos;
    callback.when      = MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE;
    callback.repeat    = MPC_OMP_CALLBACK_REPEAT_RETURN;
    callback.n         = 0;

    // TODO : split this in 2 calls (CARE FOR RACE CONDITION!)
    // mpc_omp_callback(&callback);
    // mpc_omp_task_block(&event);

    mpc_omp_task_block(&event, &callback);
}

static inline void
___task_notify_send(int n, MPI_Request * reqs)
{
    MPI_request_struct_t * requests = __sctk_internal_get_MPC_requests();
    int i;
    for (i = 0; i < n; i++)
    {
        MPI_internal_request_t * internal_req = __sctk_convert_mpc_request_internal(reqs + i, requests);
        mpc_lowcomm_request_t * lowcomm_req = &(internal_req->req);
        if (lowcomm_req->request_type == REQUEST_SEND)
        {
            mpc_omp_task_is_send();
            return ;
        }
    }
}

int
mpc_thread_mpi_omp_wait(int n, MPI_Request * reqs)
{
    if (mpc_omp_in_explicit_task())
    {
        /* if the requests array only contains sends, mark this task as critical */
        ___task_notify_send(n, reqs);

        /* suspend the task*/
        ___task_block(n, reqs);

        return 1;
    }
    return 0;
}

# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:29:47 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */

# include "mpc_thread_mpi_omp_interop.h"

# if MPC_ENABLE_INTEROP_MPI_OMP
# include "mpc_omp.h"
# include "mpc_mpi_internal.h"
# include "mpc_lowcomm_msg.h"
# include "mpc_lowcomm_types.h"

typedef struct  mpc_lowcomm_request_omp_progress_s
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
}               mpc_lowcomm_request_omp_progress_t;

/* test of request completion */
typedef int (*mpc_lowcomm_request_test_t)(mpc_lowcomm_request_omp_progress_t *);

static int
__request_test(mpc_lowcomm_request_omp_progress_t * infos)
{
    assert(infos->n == 1);

    int completed;
    MPI_Test(infos->req, &completed, infos->status);
    return completed;
}

static int
__request_testall(mpc_lowcomm_request_omp_progress_t * infos)
{
    assert(infos->n > 1);
    assert(infos->index == NULL);

    int completed;
    MPI_Testall(infos->n, infos->req, &completed, infos->status);
    return completed;
}

static int
__request_testany(mpc_lowcomm_request_omp_progress_t * infos)
{
    assert(infos->n > 1);
    assert(infos->index);

    int completed;
    MPI_Testany(infos->n, infos->req, infos->index, &completed, infos->status);
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
            unsigned int i;
            for (i = 0 ; i < infos->n ; ++i)
            {
                /** cancel MPI requests, but do not fulfill the event
                 *  the event will be fulfilled later on
                 *  by a succesful MPI_Test on the cancelled request */
                MPI_Cancel(infos->req + i);
            }
        }
    }
    return 1;
}

/* task blocking */

static inline void
___task_block(int n, MPI_Request * reqs, int * index, MPI_Status * status)
{
    /* generate test function */
    mpc_lowcomm_request_omp_progress_t infos;
    infos.req       = reqs;
    infos.index     = index;
    infos.status    = status;
    infos.n         = n;

    /* test before blocking */
    mpc_lowcomm_request_test_t test = __request_get_test(&infos);
    if (test(&infos))
    {
        return ;
    }

    /* communication is blocking, block the task */

    /* create a task-blocking handler */
    mpc_omp_event_handle_init((mpc_omp_event_handle_t **) &(infos.handle), MPC_OMP_EVENT_TASK_BLOCK);

    /* register the progression callback */
    mpc_omp_callback(
        (int (*)(void *)) __request_progress,
        &infos,
        MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE,
        MPC_OMP_CALLBACK_REPEAT_RETURN
    );

    /* block current task until the associated event is fulfilled */
    mpc_omp_task_block(infos.handle);
}

static inline void
__task_check_send(int n, MPI_Request * reqs)
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
            break ;
        }
    }
}

/** Parameters correspond to the ones of MPI_Test, MPI_Testall and MPI_Testany */
int
mpc_thread_mpi_omp_wait(int n, MPI_Request * reqs, int * index, MPI_Status * status)
{
    if (mpc_omp_in_explicit_task())
    {
        /* check task */
        __task_check_send(n, reqs);

        /* suspend the task*/
        ___task_block(n, reqs, index, status);
        return 1;
    }
    return 0;
}

# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

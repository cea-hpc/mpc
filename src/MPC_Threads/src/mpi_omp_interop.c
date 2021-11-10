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
    mpc_omp_event_handle_t * handle;

    /* the request array */
    MPI_Request * req;

    /* statuses */
    MPI_Status * statuses;

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

    /* check if the request completed */
    if (test(infos))
    {
        mpc_omp_fulfill_event(infos->handle);
        return 0;
    }

    /* check if the blocking event was cancelled */
    if (infos->handle->cancelled && OPA_load_int(infos->handle->cancelled))
    {
        unsigned int i;
        for (i = 0 ; i < infos->n ; ++i)
        {
            MPI_Cancel(infos->req + i);
        }
        mpc_omp_fulfill_event(infos->handle);
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
    mpc_omp_event_handle_t handle;
    mpc_omp_event_handle_init(&handle, MPC_OMP_EVENT_TASK_BLOCK);
    infos.handle = &handle;

    /* register the progression callback */
    mpc_omp_callback(
        (int (*)(void *)) __request_progress,
        &infos,
        MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE,
        MPC_OMP_CALLBACK_REPEAT_RETURN
    );

    /* block current task until the associated event is fulfilled */
    mpc_omp_task_block(&handle);
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

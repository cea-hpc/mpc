/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:25:56 CEST 2021                                        # */
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
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Augustin Serraz <augustin.serraz@exascale-computing.eu>            # */
/* # - Aurele Maheo <aurele.maheo@exascale-computing.eu>                  # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Ricardo Bispo vieira <ricardo.bispo-vieira@exascale-computing.eu>  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Souad Koliai <skoliai@exascale-computing.eu>                       # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPC_OMP__H
#define __MPC_OMP__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <errno.h>
#include <opa_primitives.h>
#include <pthread.h>
#include <stdint.h>

#include "omp.h"
#include "omp_gomp_constants.h"

    /* OpenMP 2.5 API - For backward compatibility with old patched GCC */
    int mpc_omp_get_num_threads( void );
    int mpc_omp_get_thread_num( void );

    /* MPC-OpenMP event types */
    typedef enum    mpc_omp_event_e
    {
        MPC_OMP_EVENT_NULL,
        MPC_OMP_EVENT_TASK_BLOCK,
        MPC_OMP_EVENT_MAX
    }               mpc_omp_event_t;

    /* event status */
    typedef enum    mpc_omp_event_handle_status_e
    {
        MPC_OMP_EVENT_HANDLE_STATUS_INIT,
        MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED,
        MPC_OMP_EVENT_HANDLE_STATUS_UNBLOCKED
    }               mpc_omp_event_handle_status_t;

    /**
     * OpenMP specificates that :
     * > The type omp_event_handle_t, which must be an implementation-defined enum type
     *
     * This has limitations, and so we define an MPC-specific event type for internal events
     */
    typedef struct  mpc_omp_event_handle_s
    {
        void            * attr;         /* the event attributes */
        OPA_int_t       lock;           /* a spinlock */
        mpc_omp_event_t type;           /* the event type */
        OPA_int_t       status;         /* the handle status */
        OPA_int_t       * cancel;       /* point to 1 if the handle should be cancelled */
        OPA_int_t       cancelled;      /* point to 1 if the handle was cancelled already */
    }               mpc_omp_event_handle_t;

    /* initialize an event handler */
    void mpc_omp_event_handle_init(mpc_omp_event_handle_t * event, mpc_omp_event_t type);

    /* fulfill an mpc-omp event handler */
    void mpc_omp_fulfill_event(mpc_omp_event_handle_t * handle);

    /** Dump the openmp thread tree */
    void mpc_omp_tree_print(FILE * file);

    /**
     * Async. callback
     */
    typedef enum    mpc_omp_callback_when_t
    {
        /* scheduler points */
        MPC_OMP_CALLBACK_TASK_SCHEDULER_POINT_NEW,
        MPC_OMP_CALLBACK_TASK_SCHEDULER_POINT_COMPLETE,
        MPC_OMP_CALLBACK_TASK_SCHEDULER_POINT_YIELD,
        /* [...] */
        MPC_OMP_CALLBACK_SCHEDULER_POINT_ANY,

        /* before a new task should be scheduled */
        MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE,

        /* when there is no more ready tasks */
        MPC_OMP_CALLBACK_TASK_SCHEDULER_FAMINE,

        /* max number */
        MPC_OMP_CALLBACK_MAX
    }               mpc_omp_callback_when_t;

    typedef enum    mpc_omp_callback_repeat_t
    {
        MPC_OMP_CALLBACK_REPEAT_RETURN  /* repeat until the callback returned 0 */
    }               mpc_omp_callback_repeat_t;

    typedef struct  mpc_omp_callback_s
    {
        /* next callback for this event */
        struct mpc_omp_callback_s * next;

        /* user */
        int (* func)(void * data);      /* Return > 0 if the function should be re-run later on */
        void * data;
        mpc_omp_callback_when_t when;
        mpc_omp_callback_repeat_t repeat;
    }               mpc_omp_callback_t;

    /**
     * `pragma omp callback [when(indicator)] [repeat(when)]`
     *
     *  A **callback** region defines a routine that is executed by the runtime.
     *
     *  The **when** clause defines when the region should be executed (see `mpc_omp_callback_when`)
     *  Default value is `MPC_OMP_CALLBACK_SCHEDULER_FAMINE`.
     *
     * The **repeat** clause defines how many times should the region run.
     *     - `repeat(until: event-handle)` will make the region run as long
     *      as the event represented by `event-handle` is not fullfiled.
     *     - `repeat(n > 0)` will make the region run `n` times.
     *
     *  Default value is `repeat(1)`
     */
    void mpc_omp_callback(int (* func)(void * data), void * data, mpc_omp_callback_when_t when, mpc_omp_callback_repeat_t repeat);

    /** Maximum length of a task label */
    # define MPC_OMP_TASK_LABEL_MAX_LENGTH 64

    /** # pragma omp task label("potrf") */
    void mpc_omp_task_label(char * label);

    /** # pragma omp task fiber */
    void mpc_omp_task_fiber(void);

    /* tasks dependencies */
    typedef enum    mpc_omp_task_dep_type_e
    {
        MPC_OMP_TASK_DEP_NONE            = 0,
        MPC_OMP_TASK_DEP_IN              = 1,
        MPC_OMP_TASK_DEP_OUT             = 2,
        MPC_OMP_TASK_DEP_INOUT           = 3,
        MPC_OMP_TASK_DEP_MUTEXINOUTSET   = 4,
        MPC_OMP_TASK_DEP_INOUTSET        = 5,
        MPC_OMP_TASK_DEP_COUNT           = 6
    }               mpc_omp_task_dep_type_t;

    /* a task data dependency */
    typedef struct  mpc_omp_task_dependency_t
    {
        void ** addrs;
        unsigned int addrs_size;
        mpc_omp_task_dep_type_t type;
    }               mpc_omp_task_dependency_t;

    /**
     * Add a data dependencies for the next task to be created.
     * \param dependencies a dependency array
     * \param n size of the dependency array
     * ATTENTION : the 'dependencies' array should be sorted by types, so that
     *  OUT >= INOUT > MUTEXINOUTSET > INOUTSET > IN
     */
    void mpc_omp_task_dependencies(mpc_omp_task_dependency_t * dependencies, unsigned int n);

    /**
     * # pragma omp taskyield block(event)
     * suspend current task until the associated event is fulfilled
     */
    void mpc_omp_task_block(mpc_omp_event_handle_t * event);

    /* task trace calls */
    void mpc_omp_task_trace_begin(void);
    void mpc_omp_task_trace_end(void);

    /* return true if the thread is currently within an omp task */
    int mpc_omp_in_explicit_task(void);

#ifdef __cplusplus
}
#endif

#endif

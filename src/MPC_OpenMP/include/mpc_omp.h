/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
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

    /* OpenMP 2.5 API - For backward compatibility with old patched GCC */
    int mpc_omp_get_num_threads( void );
    int mpc_omp_get_thread_num( void );

    /* MPC-OpenMP event types */
    typedef enum {
        MPC_OMP_EVENT_TASK_BLOCK
    }   mpc_omp_event_t;

    /**
     * OpenMP specificates that :
     * > The type omp_event_handle_t, which must be an implementation-defined enum type
     *
     * This has limitations, and so we define an MPC-specific event type for internal events
     */
    typedef struct  mpc_omp_event_handle_s
    {
        mpc_omp_event_t type;   /* the event type */
        void            * attr; /* the event attributes */
    }               mpc_omp_event_handle_t;

    void mpc_omp_fulfill_event(mpc_omp_event_handle_t * handle);

    /** Dump the openmp thread tree */
    void mpc_omp_tree_print(FILE * file);

    /**
     *  * Async callback
     *   */
    typedef enum    mpc_omp_async_when_t
    {
        /* scheduler points */
        MPC_OMP_ASYNC_TASK_SCHEDULER_POINT_NEW,
        MPC_OMP_ASYNC_TASK_SCHEDULER_POINT_COMPLETE,
        MPC_OMP_ASYNC_TASK_SCHEDULER_POINT_YIELD,
        /* [...] */
        MPC_OMP_ASYNC_SCHEDULER_POINT_ANY,

        /* before a new task should be scheduled */
        MPC_OMP_ASYNC_TASK_SCHEDULE_BEFORE,

        /* when there is no more ready tasks */
        MPC_OMP_ASYNC_TASK_SCHEDULER_FAMINE,

        /* max number */
        MPC_OMP_ASYNC_MAX
    }               mpc_omp_async_when_t;

    typedef enum    mpc_omp_async_repeat_t
    {
        MPC_MPC_OMP_ASYNC_REPEAT_UNTIL,
        MPC_MPC_OMP_ASYNC_REPEAT_N
    }               mpc_omp_async_repeat_t;

    typedef struct  mpc_omp_async_s
    {
        /* internal */
        struct mpc_omp_async_s * _next;

        /* user */
        int (* func)(void * data);      /* Return > 0 if the function should be re-run later on */
        void * data;
        mpc_omp_async_when_t when;
        mpc_omp_async_repeat_t repeat;
        omp_event_handle_t * event;     /* the event in case of `MPC_MPC_OMP_ASYNC_REPEAT_UNTIL`    */
        size_t n;                       /* the `n` in case of `MPC_MPC_OMP_ASYNC_REPEAT_N`          */
    }               mpc_omp_async_t;

    /**
     * `pragma omp async [when(indicator)] [repeat(when)]`
     * 
     *   A **async** region defines a routine that is executed by the runtime.
     * 
     * The **when** clause defines when the region should be executed (see `mpc_omp_async_when`)
     *  Default value is `MPC_OMP_ASYNC_SCHEDULER_FAMINE`.
     * 
     *The **repeat** clause defines how many times should the region run.
     *     - `repeat(until: event-handle)` will make the region run as long
     *      as the event represented by `event-handle` is not fullfiled.
     *      - `repeat(n > 0)` will make the region run `n` times.
     *
     *  Default value is `repeat(1)`
     */
    void mpc_omp_async(mpc_omp_async_t * async);

    /** Maximum length of a task label */
# define MPC_OMP_TASK_LABEL_MAX_LENGTH 64

    /** Give extra information of the incoming task - must be called right before a `#pragma omp task` */
    void mpc_omp_task(char * label);

    /** Taskyield which blocks on event */
    void mpc_omp_task_block(omp_event_handle_t * event, mpc_omp_async_t * async);
    void mpc_omp_task_unblock(omp_event_handle_t * event);

    /* task trace calls */
    void mpc_omp_task_trace_begin(void);
    void mpc_omp_task_trace_end(void);

    /* return true if the thread is currently within an omp task */
    int mpc_omp_in_task(void);

#ifdef __cplusplus
}
#endif

#endif

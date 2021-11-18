/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:55 CEST 2021                                        # */
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
# include <mpc_omp.h>

# include "mpcomp_core.h"

/**
 * Register a callback to the OpenMP runtime
 * @param callback
 */
void
mpc_omp_callback(
    int (* func)(void * data),
    void * data,
    mpc_omp_callback_when_t when,
    mpc_omp_callback_repeat_t repeat)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t * instance = thread->instance;
    assert(instance);

    mpc_omp_callback_t * callback = (mpc_omp_callback_t *) malloc(sizeof(mpc_omp_callback_t));
    assert(callback);

    callback->func      = func;
    callback->data      = data;
    callback->when      = when;
    callback->repeat    = repeat;

    mpc_omp_instance_callback_infos_t * infos = &(instance->callback_infos);
    mpc_common_spinlock_t * lock = &(infos->locks[callback->when]);

    mpc_common_spinlock_lock(lock);
    {
        callback->next = infos->callbacks[callback->when];
        infos->callbacks[callback->when] = callback;
    }
    mpc_common_spinlock_unlock(lock);
}

/**
 * Run the callback for the associated event
 * @param when - the event
 */
void
_mpc_omp_callback_run(mpc_omp_callback_when_t when)
{
    mpc_omp_thread_t     * thread    = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t   * instance  = thread->instance;
    assert(instance);

    mpc_omp_instance_callback_infos_t * infos = &(instance->callback_infos);

    mpc_common_spinlock_t * lock = &(infos->locks[when]);

    if (mpc_common_spinlock_trylock(lock) == 0)
    {
        mpc_omp_callback_t * prev  = NULL;
        mpc_omp_callback_t * callback = infos->callbacks[when];
        while (callback)
        {
            /* if the callback should be popped out of the list */
            int pop = 0;

            /* check if the callback should be popped */
            switch (callback->repeat)
            {
                case (MPC_OMP_CALLBACK_REPEAT_RETURN):
                {
                    pop = !callback->func(callback->data);
                    break ;
                }
                default:
                {
                    fprintf(stderr, "callback->repeat=%d\n", callback->repeat);
                    not_implemented();
                    break ;
                }
            }

            /* if so, pop it */
            if (pop)
            {
                if (prev)
                {
                    prev->next = callback->next;
                }
                else
                {
                    infos->callbacks[when] = callback->next;
                }
                mpc_omp_callback_t * next = callback->next;
                mpc_omp_free(callback);
                callback = next;
            }
            else
            {
                prev = callback;
                callback = callback->next;
            }

            /* process next callback for this event */
        }
        mpc_common_spinlock_unlock(lock);
    }
}

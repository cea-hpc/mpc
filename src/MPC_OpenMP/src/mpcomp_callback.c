# include <mpc_omp.h>

# include "mpcomp_core.h"

/**
 * Register a callback to the OpenMP runtime
 * @param callback
 */
void
mpc_omp_callback(mpc_omp_callback_t * callback)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t * instance = thread->instance;
    assert(instance);

    mpc_omp_instance_callback_infos_t * infos = &(instance->callback_infos);
    mpc_common_spinlock_t * lock = &(infos->locks[callback->when]);

    mpc_common_spinlock_lock(lock);
    {
        callback->_next = infos->callbacks[callback->when];
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
                    prev->_next = callback->_next;
                }
                else
                {
                    infos->callbacks[when] = callback->_next;
                }
            }
            else
            {
                prev = callback;
            }

            /* process next callback for this event */
            callback = callback->_next;
        }
        mpc_common_spinlock_unlock(lock);
    }
}

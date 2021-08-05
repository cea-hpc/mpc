# include <mpc_omp.h>

# include "mpcomp_core.h"

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
 * Run the callback functions of the given list
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
            if (callback->func(callback->data) == 0)
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
            callback = callback->_next;
        }
        mpc_common_spinlock_unlock(lock);
    }
}

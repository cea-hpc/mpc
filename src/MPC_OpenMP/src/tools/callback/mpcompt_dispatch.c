#include "mpcompt_dispatch.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
__mpcompt_callback_dispatch ( ompt_dispatch_t kind,
                              ompt_data_t instance ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_dispatch_t callback;

        callback = (ompt_callback_dispatch_t)
            ___mpcompt_get_callback( thread, ompt_callback_dispatch );

        if( callback ) {
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
            ompt_data_t* parallel_data = &thread->instance->team->info.ompt_parallel_data;

            if( kind == ompt_dispatch_iteration )
                mpc_common_debug( "%s: %p ( %p, %p, %d, %ld )",
                                  __func__, callback,
                                  parallel_data, task_data, (int) kind, (long) instance.value );
            else
                mpc_common_debug( "%s: %p ( %p, %p, %d, %p )",
                                  __func__, callback,
                                  parallel_data, task_data, (int) kind, instance.ptr );

            callback( parallel_data, task_data, kind, instance );
        }
    }
}

#endif /* OMPT_SUPPORT */

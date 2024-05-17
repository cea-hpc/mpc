#include "mpcompt_sync.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_sync_region ( ompt_sync_region_t kind,
                                 ompt_scope_endpoint_t endpoint ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_sync_region_t callback;

        callback = (ompt_callback_sync_region_t)
           __mpc_omp_ompt_get_callback( thread, ompt_callback_sync_region );

        if( callback ) {
            ompt_data_t* parallel_data = &thread->instance->team->info.ompt_parallel_data;
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %d, %p, %p, %p )",
                              __func__, callback, (int) kind, (int) endpoint,
                              parallel_data, task_data, ret_addr );

            callback( kind, endpoint, endpoint == ompt_scope_end
                                      && kind == ompt_sync_region_barrier_implicit_parallel ?
                                      NULL : parallel_data, task_data, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_sync_region_wait ( ompt_sync_region_t kind,
                                      ompt_scope_endpoint_t endpoint ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_sync_region_t callback;

        callback = (ompt_callback_sync_region_t)
           __mpc_omp_ompt_get_callback( thread, ompt_callback_sync_region_wait );

        if( callback ) {
            ompt_data_t* parallel_data = &thread->instance->team->info.ompt_parallel_data;
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %d, %p, %p, %p )",
                              __func__, callback, (int) kind, (int) endpoint,
                              parallel_data, task_data, ret_addr );

            callback( kind, endpoint, endpoint == ompt_scope_end
                                      && kind == ompt_sync_region_barrier_implicit_parallel ?
                                      NULL : parallel_data, task_data, ret_addr );
        }
    }
}

#endif /* OMPT_SUPPORT */

#include "mpcompt_sync.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
__mpcompt_callback_sync_region ( ompt_sync_region_t kind,
                                 ompt_scope_endpoint_t endpoint ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_sync_region_t callback;

        callback = (ompt_callback_sync_region_t)
           ___mpcompt_get_callback( thread, ompt_callback_sync_region );

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
                                      && kind == ompt_sync_region_barrier_implicit ?
                                      NULL : parallel_data, task_data, ret_addr );
        }
    }
}

void
__mpcompt_callback_sync_region_wait ( ompt_sync_region_t kind,
                                      ompt_scope_endpoint_t endpoint ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_sync_region_t callback;

        callback = (ompt_callback_sync_region_t)
           ___mpcompt_get_callback( thread, ompt_callback_sync_region_wait );

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

            callback( kind, endpoint, parallel_data, task_data, ret_addr );
        }
    }
}

#endif /* OMPT_SUPPORT */

#include "mpcompt_workShare.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_master ( ompt_scope_endpoint_t endpoint ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_master_t callback;

        callback = (ompt_callback_master_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_master );

        if( callback ) {
            ompt_data_t* parallel_data = &thread->instance->team->info.ompt_parallel_data;
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL; 
#endif

            mpc_common_debug( "%s: %p ( %d, %p, %p, %p )",
                              __func__, callback, (int) endpoint,
                              parallel_data, task_data, ret_addr );

            callback( endpoint, parallel_data, task_data, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_work ( ompt_work_t wstype,
                          ompt_scope_endpoint_t endpoint,
                          uint64_t count ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_work_t callback;

        callback = (ompt_callback_work_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_work );

        if( callback ) {
            /* Handle single construct */
            if( wstype == ompt_work_single_executor
                || wstype == ompt_work_single_other ) {
                if( endpoint == ompt_scope_begin ) {
                    thread->info.in_single = 1;

                    if( wstype == ompt_work_single_executor )
                        thread->instance->team->info.doing_single = thread->rank;
                    }
                else {
                    thread->info.in_single = 0;

                    if( wstype == ompt_work_single_executor )
                        thread->instance->team->info.doing_single = -1;
                }
            }

            ompt_data_t* parallel_data = &thread->instance->team->info.ompt_parallel_data;
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL; 
#endif

            mpc_common_debug( "%s: %p ( %d, %d, %p, %p, %llu, %p )",
                              __func__, callback, (int) wstype, (int) endpoint,
                              parallel_data, task_data,
                              endpoint == ompt_scope_begin ? count : 0, ret_addr );

            callback( wstype, endpoint, parallel_data, task_data,
                      endpoint == ompt_scope_begin ? count : 0, ret_addr );
        }
    }
}

#endif /* OMPT_SUPPORT */

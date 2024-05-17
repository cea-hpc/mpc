#include "mpcompt_parallel.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_parallel_begin ( unsigned int requested_parallelism,
                                    int flags ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_parallel_begin_t callback;

        callback = (ompt_callback_parallel_begin_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_parallel_begin );

        if( callback ) {
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
            ompt_frame_t* frame_data = &thread->task_infos.current_task->ompt_task_frame;
            ompt_data_t* parallel_data = &thread->children_instance->team->info.ompt_parallel_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %p, %p, %p, %u, %d, %p )",
                              __func__, callback,
                              task_data, frame_data, parallel_data,
                              requested_parallelism, flags, ret_addr );

            callback( task_data, frame_data, parallel_data, requested_parallelism, flags, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_parallel_end ( int flags ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_parallel_end_t callback;

        callback = (ompt_callback_parallel_end_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_parallel_end );

        if( callback ) {
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;
            ompt_data_t* parallel_data = &thread->children_instance->team->info.ompt_parallel_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %p, %p, %d, %p )",
                              __func__, callback,
                              parallel_data, task_data, flags, ret_addr );

            callback( parallel_data, task_data, flags, ret_addr );
        }
    }
}

#endif /* OMPT_SUPPORT */

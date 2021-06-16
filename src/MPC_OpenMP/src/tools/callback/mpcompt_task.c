#include "mpcompt_task.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
__mpcompt_callback_task_create ( mpcomp_task_t *task,
                                 int flags,
                                 int has_dependences ) {
    mpcomp_thread_t* thread;
    assert( task );

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    if( !thread )
        thread = task->thread;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_task_create_t callback;

        callback = (ompt_callback_task_create_t)
            ___mpcompt_get_callback( thread, ompt_callback_task_create );

        if( callback ) {
            void* ret_addr = NULL;
            ompt_data_t* curr_task_data = NULL;
            ompt_frame_t* curr_task_frame = NULL;
            ompt_data_t* task_data = &task->ompt_task_data;

            if( flags & ompt_task_initial ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
                ret_addr = thread->frame_infos.ompt_return_addr;
#endif

                mpc_common_debug( "%s: %p ( %p, %p, %p, %d, %d, %p )",
                                  __func__, callback, NULL, NULL,
                                  task_data, flags, has_dependences, ret_addr );

                callback( NULL, NULL, task_data, flags, has_dependences, ret_addr );
            }
            else if( flags & ompt_task_implicit ) {
                if( thread->next ) {
                    curr_task_data = &thread->next->task_infos.current_task->ompt_task_data;
                    curr_task_frame = &thread->next->task_infos.current_task->ompt_task_frame;
#if MPCOMPT_HAS_FRAME_SUPPORT
                    ret_addr = thread->frame_infos.ompt_return_addr;
#endif
                }

                mpc_common_debug( "%s: %p ( %p, %p, %p, %d, %d, %p )",
                                  __func__, callback,
                                  curr_task_data, curr_task_frame,
                                  task_data, flags, has_dependences, ret_addr );

                callback( curr_task_data, curr_task_frame, task_data,
                          flags, has_dependences, ret_addr );
            }
            else if( flags & ompt_task_explicit ) {
                curr_task_data = &thread->task_infos.current_task->ompt_task_data;
                curr_task_frame = &thread->task_infos.current_task->ompt_task_frame;
#if MPCOMPT_HAS_FRAME_SUPPORT
                ret_addr = thread->frame_infos.ompt_return_addr;
#endif

                mpc_common_debug( "%s: %p ( %p, %p, %p, %d, %d, %p )",
                                  __func__, callback,
                                  curr_task_data, curr_task_frame,
                                  task_data, flags, has_dependences, ret_addr );

                callback( curr_task_data, curr_task_frame, task_data,
                          flags, has_dependences, ret_addr );
            }
            else {
                mpc_common_debug( "%s: unknown task type", __func__ );
                not_reachable();
            }
        }
    }
}

void
__mpcompt_callback_dependences ( mpcomp_task_t *task,
                                 const ompt_dependence_t *deps,
                                 int ndeps ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_dependences_t callback;

        callback = (ompt_callback_dependences_t)
            ___mpcompt_get_callback( thread, ompt_callback_dependences );

        if( callback ) {
            ompt_data_t* task_data = &task->ompt_task_data;

            mpc_common_debug( "%s: %p ( %p, %p, %d )",
                              __func__, callback, task_data, deps, ndeps );

            callback( task_data, deps, ndeps );
        }
    }
}

void
__mpcompt_callback_task_dependence ( ompt_data_t *src_task_data,
                                     ompt_data_t *sink_task_data ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_task_dependence_t callback;

        callback = (ompt_callback_task_dependence_t)
            ___mpcompt_get_callback( thread, ompt_callback_task_dependence );

        if( callback ) {
            mpc_common_debug( "%s: %p ( %p %p )",
                              __func__, src_task_data, sink_task_data );

            callback( src_task_data, sink_task_data );
        }
    }
}

void
__mpcompt_callback_task_schedule ( ompt_data_t *prior_task_data,
                                   ompt_task_status_t prior_task_status,
                                   ompt_data_t *next_task_data ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_task_schedule_t callback;

        callback = (ompt_callback_task_schedule_t)
            ___mpcompt_get_callback( thread, ompt_callback_task_schedule );

        if( callback ) {
            mpc_common_debug( "%s: %p ( %p, %d, %p )",
                              __func__, callback, prior_task_data,
                              (int) prior_task_status, next_task_data );

            callback( prior_task_data, prior_task_status, next_task_data );
        }
    }
}

void
__mpcompt_callback_implicit_task ( ompt_scope_endpoint_t endpoint,
                                   unsigned int actual_parallelism,
                                   unsigned int index,
                                   int flags ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_implicit_task_t callback;

        callback = (ompt_callback_implicit_task_t)
            ___mpcompt_get_callback( thread, ompt_callback_implicit_task );

        if( callback ) {
            ompt_data_t* parallel_data = &thread->instance->team->info.ompt_parallel_data;
            ompt_data_t* task_data = &thread->task_infos.current_task->ompt_task_data;

            mpc_common_debug( "%s: %p ( %d, %p, %p, %u, %u, %d )",
                              __func__, callback, (int) endpoint,
                              endpoint == ompt_scope_begin ? parallel_data : NULL,
                              task_data, actual_parallelism, index, flags );

            callback( endpoint, endpoint == ompt_scope_begin ? parallel_data : NULL,
                      task_data, actual_parallelism, index, flags );
        }
    }
}

#endif /* OMPT_SUPPORT */

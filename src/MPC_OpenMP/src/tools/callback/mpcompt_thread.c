#include "mpcompt_thread.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_thread_begin ( mpc_omp_thread_t* thread,
                                  ompt_thread_t thread_type ) {
    assert( thread );
    mpc_omp_thread_t* cur_thread = (mpc_omp_thread_t*) mpc_omp_tls;

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_thread_begin_t callback;

        callback = (ompt_callback_thread_begin_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_thread_begin );

        if( callback ) {
            ompt_data_t* ompt_data;

            if( thread_type == ompt_thread_other )
                ompt_data = &thread->mvp->ompt_mvp_data;
            else
                ompt_data = &thread->ompt_thread_data;

            mpc_common_debug( "%s: %p ( %d, %p )",
                              __func__, callback,
                              (int) thread_type, ompt_data );

            if( cur_thread != thread ) {
                mpc_omp_tls = (void*) thread;
                callback( thread_type, ompt_data );
                mpc_omp_tls = (void*) cur_thread;
            }
            else
                callback( thread_type, ompt_data );
        }
    }
}

void
_mpc_omp_ompt_callback_thread_end ( mpc_omp_thread_t* thread,
                                ompt_thread_t thread_type ) {
    assert( thread );
    mpc_omp_thread_t* cur_thread = (mpc_omp_thread_t*) mpc_omp_tls;

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_thread_end_t callback;

        callback = (ompt_callback_thread_end_t)
            __mpc_omp_ompt_get_callback( thread,
                                     ompt_callback_thread_end );

        if( callback ) {
            ompt_data_t* ompt_data;

            if( thread_type == ompt_thread_other )
                ompt_data = &thread->mvp->ompt_mvp_data;
            else
                ompt_data = &thread->ompt_thread_data;

            mpc_common_debug( "%s: %p( %p )",
                              __func__, callback, ompt_data );

            if( cur_thread != thread ) {
                mpc_omp_tls = (void*) thread;
                callback( ompt_data );
                mpc_omp_tls = (void*) cur_thread;
            }
            else
                callback( ompt_data );
        }
    }
}

#endif /* OMPT_SUPPORT */

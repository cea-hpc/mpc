#include "mpcompt_thread.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
__mpcompt_callback_thread_begin ( mpcomp_thread_t* thread,
                                  ompt_thread_t thread_type ) {
    assert( thread );
    mpcomp_thread_t* cur_thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_thread_begin_t callback;

        callback = (ompt_callback_thread_begin_t)
            ___mpcompt_get_callback( thread, ompt_callback_thread_begin );

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
                sctk_openmp_thread_tls = (void*) thread;
                callback( thread_type, ompt_data );
                sctk_openmp_thread_tls = (void*) cur_thread;
            }
            else
                callback( thread_type, ompt_data );
        }
    }
}

void
__mpcompt_callback_thread_end ( mpcomp_thread_t* thread,
                                ompt_thread_t thread_type ) {
    assert( thread );
    mpcomp_thread_t* cur_thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_thread_end_t callback;

        callback = (ompt_callback_thread_end_t)
            ___mpcompt_get_callback( thread,
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
                sctk_openmp_thread_tls = (void*) thread;
                callback( ompt_data );
                sctk_openmp_thread_tls = (void*) cur_thread;
            }
            else
                callback( ompt_data );
        }
    }
}

#endif /* OMPT_SUPPORT */

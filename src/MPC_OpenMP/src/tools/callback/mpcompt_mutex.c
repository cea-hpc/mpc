#include "mpcompt_mutex.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_mutex_acquire ( ompt_mutex_t kind,
                                   unsigned int hint,
                                   unsigned int impl,
                                   ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_mutex_acquire_t callback;

        callback = (ompt_callback_mutex_acquire_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_mutex_acquire );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %u, %u, %llu, %p )",
                              __func__, callback, (int) kind,
                              hint, impl, wait_id, ret_addr );

            callback( kind, hint, impl, wait_id, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_lock_init ( ompt_mutex_t kind,
                               unsigned int hint,
                               unsigned int impl,
                               ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_mutex_acquire_t callback;

        callback = (ompt_callback_mutex_acquire_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_lock_init );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %u, %u, %llu, %p )",
                              __func__, callback, (int) kind,
                              hint, impl, wait_id, ret_addr );

            callback( kind, hint, impl, wait_id, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_lock_destroy ( ompt_mutex_t kind,
                                  ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_mutex_t callback;

        callback = (ompt_callback_mutex_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_lock_destroy );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %llu, %p )",
                              __func__, callback, (int) kind,
                              wait_id, ret_addr );

            callback( kind, wait_id, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_mutex_acquired ( ompt_mutex_t kind,
                                    ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_mutex_t callback;

        callback = (ompt_callback_mutex_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_mutex_acquired );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %llu, %p )",
                              __func__, callback, (int) kind,
                              wait_id, ret_addr );

            callback( kind, wait_id, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_mutex_released ( ompt_mutex_t kind,
                                    ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_mutex_t callback;

        callback = (ompt_callback_mutex_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_mutex_released );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %llu, %p )",
                              __func__, callback, (int) kind,
                              wait_id, ret_addr );

            callback( kind, wait_id, ret_addr );
        }
    }
}

void
_mpc_omp_ompt_callback_nest_lock ( ompt_scope_endpoint_t endpoint,
                               ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( __mpc_omp_ompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_nest_lock_t callback;

        callback = (ompt_callback_nest_lock_t)
            __mpc_omp_ompt_get_callback( thread, ompt_callback_nest_lock );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %d, %llu, %p )",
                              __func__, callback,
                              (int) endpoint, wait_id, ret_addr );

            callback( endpoint, wait_id, ret_addr );
        }
    }
}

#endif /* OMPT_SUPPORT */

#include "mpcompt_control_tool.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

omp_control_tool_result_t
__mpcompt_callback_control_tool ( uint64_t command,
                                  uint64_t modifier,
                                  void *arg ) {
    mpcomp_thread_t* thread;
    omp_control_tool_result_t ret = omp_control_tool_notool;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;

    if( !thread )
        return ret;

    if( ___mpcompt_isActive( thread )) {
        assert( thread->tool_instance );
        ompt_callback_control_tool_t callback;

        callback = (ompt_callback_control_tool_t)
            ___mpcompt_get_callback( thread, ompt_callback_control_tool );

        if( callback ) {
#if MPCOMPT_HAS_FRAME_SUPPORT
            const void* ret_addr = thread->frame_infos.ompt_return_addr;
#else
            const void* ret_addr = NULL;
#endif

            mpc_common_debug( "%s: %p ( %llu, %llu, %p, %p )",
                              __func__, callback,
                              command, modifier, arg, ret_addr );

            ret = callback( command, modifier, arg, ret_addr );
        }
        else
            ret = omp_control_tool_nocallback;
    }

    return ret;
}

#endif /* OMPT_SUPPORT */

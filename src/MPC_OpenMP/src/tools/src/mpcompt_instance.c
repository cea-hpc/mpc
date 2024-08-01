#include "mpcompt_instance.h"

#if OMPT_SUPPORT
#include "mpcompt_entryPoints.h"
#include "mpcomp_types.h"

int
mpc_omp_ompt_register_tool ( ompt_start_tool_result_t* tool_result,
                        char *path ) {
    assert( tool_result );

    int ret = 0;
    mpc_omp_thread_t* thread;
    mpc_omp_ompt_tool_instance_t* tool_instance;

    /* Get current thread */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    /* Allocate tool instance */
    if( !thread->tool_instance ) {
        tool_instance = (mpc_omp_ompt_tool_instance_t*) sctk_malloc( sizeof( mpc_omp_ompt_tool_instance_t ));
        assert( tool_instance );
        memset( tool_instance, 0, sizeof( mpc_omp_ompt_tool_instance_t ));

        mpc_common_spinlock_init( &tool_instance->wait_id_lock, 0 );

        thread->tool_instance = tool_instance;
    }

    /* Call tool intialize method */
    ret = tool_result->initialize( mpc_omp_ompt_get_lookup_fn(),
                                   0,
                                   &tool_result->tool_data );

    /* Failed to initialize tool? free tool instance infos */
    if( !ret ) {
        sctk_free( thread->tool_instance );
        thread->tool_instance = NULL;
    }
    else {
        thread->tool_instance->start_result = tool_result;

        if( path )
            thread->tool_instance->path = path;
    }

    mpc_common_debug( "%s: tool initialize = %d", __func__, ret );

    return ret;
}

void
mpc_omp_ompt_unregister_tool () {
    mpc_omp_thread_t* thread;
    mpc_omp_ompt_tool_instance_t* tool_instance;
    ompt_start_tool_result_t* tool_result;

    /* Get current thread */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    /* Get tool instance */
    tool_instance = thread->tool_instance;
    assert( tool_instance );

    /* Get tool result infos */
    tool_result = tool_instance->start_result;
    assert( tool_result );

    /* Call tool finalize method */
    tool_result->finalize( &tool_result->tool_data );

    /* Free tool instance infos */
    if( tool_instance->path)
        sctk_free( tool_instance->path );
    if( tool_instance->callbacks )
        sctk_free( tool_instance->callbacks );
    sctk_free( tool_instance );
    thread->tool_instance = NULL;

    mpc_common_debug( "%s: tool finalize", __func__ );
}

#endif /* OMPT_SUPPORT */

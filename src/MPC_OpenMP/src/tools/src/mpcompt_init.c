#define _GNU_SOURCE
#include <dlfcn.h>

#include "mpcompt_init.h"

#if OMPT_SUPPORT
#include <string.h>
#include "mpcompt_entryPoints.h"
#include "mpcompt_instance.h"
#include "mpcomp_types.h"
#include "mpcomp_abi.h"

/* Global vars */
extern mpcomp_global_icv_t mpcomp_global_icvs;

#pragma weak ompt_start_tool
ompt_start_tool_result_t*
ompt_start_tool ( unsigned int omp_version,
                  const char *runtime_version ) {
    ompt_start_tool_result_t* ret = NULL;
    ompt_start_tool_result_t* (*start_tool_ptr) ( unsigned int omp_version,
                                                  const char *runtime_version ) = NULL;

    mpc_common_debug( "(WEAK) %s: look for symbol in dynamically-linked tool",
                      __func__ );

    start_tool_ptr = dlsym( RTLD_NEXT, "ompt_start_tool");

    if( start_tool_ptr )
        ret = start_tool_ptr( omp_version, runtime_version );

    mpc_common_debug( "(WEAK) %s: %s (ret=%p)",
                      __func__, ret ? "found" : "not found", ret );

    return ret;
}

static inline ompt_start_tool_result_t*
___mpcompt_get_start_tool_result ( unsigned int omp_version,
                                   const char *runtime_version,
                                   char** tool_path ) {
    char* save_ptr;
    char* tool_libraries = NULL;
    char* candidate_tool_path = NULL;
    ompt_start_tool_result_t* ret = NULL;
    ompt_start_tool_result_t* (*start_tool_ptr) ( unsigned int omp_version,
                                                  const char *runtime_version ) = NULL;

    mpc_common_debug( "%s: omp version support \"%u\", omp runtime version \"%s\", %s (%p)",
                      __func__, omp_version, runtime_version, tool_path ?
                      "record tool path" : "no tool path record", tool_path );

    /* Is omp_start_tool routine defined in address space? */
    ret = ompt_start_tool( omp_version, runtime_version );

    /* Otherwise, try to find ompt_start_tool routine in one of candidate tools 
     * provided by user OMP_TOOL_LIBRARIES environment variable */
    if( !ret && mpcomp_global_icvs.tool_libraries ) {
        tool_libraries = strdup( mpcomp_global_icvs.tool_libraries );
        candidate_tool_path = strtok_r( tool_libraries, ":", &save_ptr );

        while( !ret && candidate_tool_path ) {
            void* handle = dlopen( candidate_tool_path, RTLD_LAZY );

            if( handle ) {
                start_tool_ptr = dlsym( handle, "ompt_start_tool");

                mpc_common_debug( "%s: %s %s", __func__,
                                  candidate_tool_path, start_tool_ptr ?
                                  "process handle" : dlerror() );

                if( start_tool_ptr )
                    ret = start_tool_ptr( omp_version, runtime_version );
            }
            else
                mpc_common_debug( "%s", dlerror() );

            if( !ret )
                candidate_tool_path = strtok_r( NULL, ":", &save_ptr );
        }

        mpc_common_debug( "%s: (ret=%p) %s %s", __func__,
                          ret, ret ? "found in" : "not found",
                          candidate_tool_path ? candidate_tool_path : "" );

        if( ret && tool_path )
            *tool_path = strdup( candidate_tool_path );

        if( tool_libraries )
            sctk_free( tool_libraries );
    }

    return ret;  
}

void
__mpcompt_init () {
    char* tool_path = NULL;
    mpcomp_thread_t* thread;
    ompt_start_tool_result_t* tool_res = NULL;

    /* Get current thread */
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
    assert( thread );

    mpc_common_debug( "%s: %s tool interface (OMP_TOOL)", __func__,
                      mpcomp_global_icvs.tool == mpcomp_tool_enabled ?
                      "initialize" : "ignore" );

    /* Tool interface disabled by OMP_TOOL env variable */
    if( mpcomp_global_icvs.tool != mpcomp_tool_enabled ) {
        thread->tool_status = inactive;

        return;
    }

    /* Already initialized */
    if( thread->tool_status != uninitialized )
        return;

    thread->tool_status = initialized;

    /* Try to find a tool */
    tool_res = ___mpcompt_get_start_tool_result( _OPENMP,
                                                 "mpc-omp",
                                                 &tool_path );

    thread->tool_status = active;

    /* Found a tool, call initialize method and register it */
    if( tool_res
        && mpcompt_register_tool( tool_res, tool_path ? tool_path : NULL )) {
        mpc_common_debug( "%s: Tool found! Tool interface active.\n"
                          "(tool result = %p, tool instance = %p)",
                          __func__, thread->tool_instance->start_result,
                          thread->tool_instance );
    }
    else {
        thread->tool_status = inactive;

        mpc_common_debug( "%s: No tool found, tool interface inactive", __func__ );
    }
}

void
__mpcompt_finalize () {
    mpcomp_thread_t* thread;
    OPA_int_t* nb_threads_exit;

    /* Get current thread */
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
    assert( thread );

    mpc_common_debug( "%s: tool state = %s", __func__,
                      thread->tool_status == active ? "active" : "inactive" );

    if( thread->tool_status == active ) {
        nb_threads_exit = &thread->tool_instance->nb_native_threads_exited;

        OPA_incr_int( nb_threads_exit );

        /* Wait for all threads and their last events */
        while( mpcomp_global_icvs.nmicrovps_var
               != OPA_load_int( nb_threads_exit ))
            mpc_thread_yield();

        mpc_common_debug( "%s: unregister tool (tool result = %p, tool instance = %p)",
                          __func__, thread->tool_instance->start_result,
                          thread->tool_instance );

        /* Call tool finalize method, unregister callabcks and free tool infos */
        mpcompt_unregister_tool();

        /* No more active tool */
        thread->tool_status = inactive;
    }
}

#endif /* OMPT_SUPPORT */

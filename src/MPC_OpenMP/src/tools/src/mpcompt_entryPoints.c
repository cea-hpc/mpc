#include "mpcompt_entryPoints.h"

#if OMPT_SUPPORT
#include <string.h>
#include "mpcompt_state.h"
#include "mpcompt_mutexImpl.h"
#include "mpcompt_callback.h"
#include "mpcomp_types.h"

extern mpcomp_global_icv_t mpcomp_global_icvs;

#define FOREACH_OMPT_INQUIRY_FN( macro )                                      \
    /* ENUMERATE */                                                           \
    macro( ompt_enumerate_states )                                            \
    macro( ompt_enumerate_mutex_impls )                                       \
    /* CALLBACK */                                                            \
    macro( ompt_set_callback )                                                \
    macro( ompt_get_callback )                                                \
    /* GETTER */                                                              \
    macro( ompt_get_thread_data )                                             \
    macro( ompt_get_num_procs )                                               \
    macro( ompt_get_num_places )                                              \
    macro( ompt_get_place_proc_ids )                                          \
    macro( ompt_get_place_num )                                               \
    macro( ompt_get_partition_place_nums )                                    \
    macro( ompt_get_proc_id )                                                 \
    macro( ompt_get_state )                                                   \
    macro( ompt_get_parallel_info )                                           \
    macro( ompt_get_task_info )                                               \
    macro( ompt_get_task_memory )                                             \
    macro( ompt_get_target_info )                                             \
    macro( ompt_get_num_devices )                                             \
    macro( ompt_get_unique_id )                                               \
    /* FINALIZE */                                                            \
    macro( ompt_finalize_tool )

static int
ompt_enumerate_states ( int current_state,
                        int *next_state,
                        const char **next_state_name ) {
    return mpcompt_get_next_state( current_state, next_state, next_state_name );
}

static int
ompt_enumerate_mutex_impls ( int current_impl,
                             int *next_impl,
                             const char **next_impl_name ) {
    return mpcompt_get_next_mutexImpl( current_impl, next_impl, next_impl_name );
}

static ompt_set_result_t
ompt_set_callback ( ompt_callbacks_t event,
                    ompt_callback_t callback ) {
    ompt_set_result_t status;
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );
    assert( thread->tool_instance );

    /* callback index begins with value 1 */
    const uint64_t cb_array_len =
        sizeof( mpcompt_callback_infos ) / sizeof( mpcompt_callback_infos_t ) + 1;
    assert( event > 0 && event < cb_array_len );

    /* Get status of event callback */
    status = mpcompt_get_callback_status( event );
    if( (int) status >= (int) ompt_set_sometimes
        && (int) status <= (int) ompt_set_always ) {
        mpcompt_tool_instance_t* tool_instance = thread->tool_instance;

        /* Allocate callback array if needed */
        if( !tool_instance->callbacks )
        {
            mpc_common_rwlock_t ini_lk = SCTK_SPIN_RWLOCK_INITIALIZER;
            tool_instance->lock = ini_lk;

            tool_instance->callbacks =
                (ompt_callback_t*) sctk_malloc( cb_array_len * sizeof( ompt_callback_t ));
            assert( tool_instance->callbacks );
            memset( tool_instance->callbacks, 0, cb_array_len * sizeof( ompt_callback_t ));
        }

        /* Set callback */
        mpc_common_rwlock_t *lck = &tool_instance->lock;
        mpc_common_spinlock_write_lock( lck );

        tool_instance->callbacks[event] = callback;

        mpc_common_spinlock_write_unlock( lck );
    }

    return status;
}

static int
ompt_get_callback ( ompt_callbacks_t event,
                    ompt_callback_t* callback ) {
    int ret = 0;
    mpcomp_thread_t* thread;
    ompt_set_result_t status = mpcompt_get_callback_status( event );

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );
    assert( thread->tool_instance );

    mpcompt_tool_instance_t* tool_instance = thread->tool_instance;

    if( (int) status >= (int) ompt_set_sometimes
        && (int) status <= (int) ompt_set_always
        && tool_instance->callbacks ) {
        /* Get callback */
        mpc_common_rwlock_t *lck = &tool_instance->lock;
        mpc_common_spinlock_read_lock( lck );

        *callback = tool_instance->callbacks[event];

        mpc_common_spinlock_read_unlock( lck );

        if( *callback )
            ret = 1;
    }

    return ret;
}

static ompt_data_t*
ompt_get_thread_data ( void ) {
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    return &thread->ompt_thread_data;
}

static int
ompt_get_num_procs ( void ) {
    return mpcomp_global_icvs.nmicrovps_var;
}

static int
ompt_get_num_places ( void ) {
    not_implemented();

    return 0;
}

static int
ompt_get_place_proc_ids ( __UNUSED__ int place_num,
                          __UNUSED__ int ids_size,
                          __UNUSED__ int *ids ) {
    not_implemented();

    return 0;
}

static int
ompt_get_place_num ( void ) {
    not_implemented();

    return 0;
}

static int
ompt_get_partition_place_nums ( __UNUSED__ int place_nums_size,
                                __UNUSED__ int *place_nums ) {
    not_implemented();

    return 0;
}

static int
ompt_get_proc_id ( void ) {
    int ret = -1;
    mpcomp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    if( thread->mvp )
        ret = thread->mvp->pu_id;

    return ret;
}

static int
ompt_get_state ( ompt_wait_id_t *wait_id ) {
    return __mpcompt_thread_get_state( wait_id );
}

static int
ompt_get_parallel_info ( int ancestor_level,
                         ompt_data_t **parallel_data,
                         int *team_size ) {
    int ret = 0;
    mpcomp_thread_t* thread;

    if( parallel_data )
        *parallel_data = NULL;
    if( team_size )
        *team_size = 0;

    if( 0 <= ancestor_level ) {
        /* Get current thread infos */
        thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
        assert( thread );

        while( 0 < ancestor_level 
               && thread
               && thread->instance ) {
            thread = thread->instance->thread_ancestor;
            ancestor_level--;
        }

        if( thread && thread->instance && thread->instance->team ) {
            if( parallel_data )
                *parallel_data = &thread->instance->team->info.ompt_parallel_data;
            if( team_size )
                *team_size = thread->info.num_threads;

            ret = 2;
        }
    }

    return ret;
}

static int
ompt_get_task_info ( int ancestor_level,
                     int *flags,
                     ompt_data_t **task_data,
                     ompt_frame_t **task_frame,
                     ompt_data_t **parallel_data,
                     int *thread_num ) {
    int ret = 0;

#if MPCOMP_TASK
    int i, team_size;
    mpcomp_thread_t* thread;
    mpcomp_task_t* current_task = NULL;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );

    current_task = thread->task_infos.current_task;

    if( !current_task )
        return ret;

    for( i = 0; i < ancestor_level; i++) {
        current_task = current_task->parent;

        if( current_task == NULL )
            break;
    }

    if( current_task ) {
        if( flags )
            *flags = current_task->ompt_task_type;
        if( task_data )
            *task_data = &current_task->ompt_task_data;
        if( task_frame )
            *task_frame = &current_task->ompt_task_frame;
        if( parallel_data && !ompt_get_parallel_info( 0, parallel_data, &team_size ))
            *parallel_data = NULL;
        if( thread_num )
            *thread_num = thread->rank;

        ret = 2;
    }
#endif /* MPCOMP_TASK */

    return ret;
}

static int
ompt_get_task_memory ( void **addr,
                       size_t *size,
                       int block ) {
#if MPCOMP_TASK
    mpcomp_thread_t* thread;
    mpcomp_task_t* current_task = NULL;

    if( size ) {
        *size = 0;

        /* Get current thread infos */
        thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
        assert( thread );

        current_task = thread->task_infos.current_task;

        if( block == 0
            && current_task
            && current_task->data_size ) {
            *size = current_task->data_size;

            if( addr ) {
                addr[block] = (void*) malloc( sizeof( current_task->data_size ));
                memcpy( addr[block], current_task->data, current_task->data_size );
            }
        }
    }
#endif /* MPCOMP_TASK */

    return 0;
}

static int
ompt_get_target_info ( __UNUSED__ uint64_t *device_num,
                       __UNUSED__ ompt_id_t *target_id,
                       __UNUSED__ ompt_id_t *host_op_id ) {
    not_implemented();

    return 0;
}

static int
ompt_get_num_devices ( void ) {
    not_implemented();

    return 0;
}

static uint64_t
ompt_get_unique_id ( void ) {
    uint64_t wait_id;
    mpcomp_thread_t* thread;
    mpcompt_tool_instance_t* tool_instance;

    /* Get current thread infos */
    thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( thread );
    assert( thread->tool_instance );

    tool_instance = thread->tool_instance;

    mpc_common_spinlock_lock( &tool_instance->wait_id_lock );
    tool_instance->wait_id += 1;
    wait_id = (uint64_t) tool_instance->wait_id;
    mpc_common_spinlock_unlock( &tool_instance->wait_id_lock );

    return wait_id;
}

static void
ompt_finalize_tool ( void ) {
    not_implemented();
}

static ompt_interface_fn_t
ompt_function_lookup ( const char *interface_function_name ) {
#define ompt_interface_fn(fn)                                                \
  if(strcmp(interface_function_name,#fn) == 0) return (ompt_interface_fn_t) fn;
  FOREACH_OMPT_INQUIRY_FN(ompt_interface_fn)
#undef ompt_interface_fn

  return (ompt_interface_fn_t) 0;
}

ompt_function_lookup_t
mpcompt_get_lookup_fn () {
  return ompt_function_lookup;
}

#endif /* OMPT_SUPPORT */

#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_wrapper.h"
#include "mpcomp_intel_global.h"

kmp_int32 __kmpc_ok_to_fork(ident_t * loc)
{
  sctk_nodebug( "%s: entering...", __func__ );
  return (kmp_int32) 1;
}

#if MPCOMP_INTEL_USE_BUFFERED_ARGS_LIST
int intel_temp_argc = 10 ;
void ** intel_temp_args_copy = NULL ;
#endif /* MPCOMP_INTEL_USE_BUFFERED_ARGS_LIST */

void
__kmpc_fork_call(ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ...)
{
  va_list args ;
  int i ;
  void ** args_copy ;
  mpcomp_intel_wrapper_t *w;
  mpcomp_thread_t *t;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

  sctk_nodebug( "__kmpc_fork_call: entering w/ %d arg(s)...", argc ) ;

#if 1
     w = malloc( sizeof( mpcomp_intel_wrapper_t ));
#else
    mpcomp_intel_wrapper_t w_noalloc;
    w = &w_noalloc;
#endif

  /* Handle orphaned directive (initialize OpenMP environment) */
 __mpcomp_init() ;
  
  /* Threadprvate initialisation */
  sctk_thread_mutex_lock(&lock);
  if( !__kmp_init_common )
  {
    for (i = 0; i < KMP_HASH_TABLE_SIZE; ++i)
      __kmp_threadprivate_d_table.data[ i ] = 0;
    __kmp_init_common = 1;
  }
  sctk_thread_mutex_unlock(&lock);

  /* Grab info on the current thread */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
#if MPCOMP_INTEL_USE_BUFFERED_ARGS_LIST
    if ( intel_temp_args_copy == NULL ) 
    {
        intel_temp_args_copy = (void **)malloc( intel_temp_argc * sizeof( void * ) ) ;
    }
    if ( argc <= intel_temp_argc ) 
    {
        args_copy = intel_temp_args_copy ;
    } 
    else 
    {
#endif /* MPCOMP_INTEL_USE_BUFFERED_ARGS_LIST */
    args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
    sctk_assert( args_copy ) ;
#if MPCOMP_INTEL_USE_BUFFERED_ARGS_LIST
    }
#endif /* MPCOMP_INTEL_USE_BUFFERED_ARGS_LIST */

  va_start( args, microtask ) ;

  for ( i = 0 ; i < argc ; i++ ) 
  {
     args_copy[i] = va_arg( args, void * ) ;
  }

  va_end(args) ;

  w->f = microtask ;
  w->argc = argc ;
  w->args = args_copy ;

  /* translate for gcc value */
  t->push_num_threads = ( t->push_num_threads <= 0 ) ? 0 : t->push_num_threads;
  sctk_nodebug(" f: %p, argc: %d, args: %p, num_thread: %d", w->f, w->argc, w->args,  t->push_num_threads );
  __mpcomp_start_parallel_region( mpcomp_intel_wrapper_func, w, t->push_num_threads );

  /* restore the number of threads w/ num_threads clause */
  t->push_num_threads = 0 ;
}

void
__kmpc_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
  mpcomp_thread_t* t ;
  mpcomp_new_parallel_region_info_t* info ;

#ifdef MPCOMP_FORCE_PARALLEL_REGION_ALLOC
    info = sctk_malloc( sizeof( mpcomp_new_parallel_region_info_t ) );
    assert( info );
#else   /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
    mpcomp_new_parallel_region_info_t noallocate_info;
    info = &noallocate_info;
#endif  /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */


    sctk_nodebug( "%s: entering (%d) ...", __func__, global_tid ) ;

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

	info->func = NULL ; /* No function to call */
	info->shared = NULL ;
	info->num_threads = 1 ;
	info->new_root = NULL ;
	info->icvs = t->info.icvs ; 
	info->combined_pragma = MPCOMP_COMBINED_NONE;

	__mpcomp_internal_begin_parallel_region( info, 1 ) ;

	/* Save the current tls */
	t->children_instance->mvps[0]->threads[0].father = sctk_openmp_thread_tls ;

	/* Switch TLS to nested thread for region-body execution */
    sctk_openmp_thread_tls = &(t->children_instance->mvps[0]->threads[0]) ;

    sctk_nodebug( "%s: leaving (%d) ...", __func__, global_tid ) ;
}

void
__kmpc_end_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
    mpcomp_thread_t * t ;

    sctk_nodebug( "%s: entering (%d)...", __func__, global_tid ) ;

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

	/* Restore the previous thread info */
	sctk_openmp_thread_tls = t->father ;
    __mpcomp_internal_end_parallel_region( t->instance ) ;

    sctk_nodebug( "%s: leaving (%d)...", __func__, global_tid ) ;
}

void __kmpc_push_num_threads( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_threads )
{
  mpcomp_thread_t * t ;
  sctk_warning( "%s: pushing %d thread(s)", __func__, num_threads ) ;

  /* Handle orphaned directive (initialize OpenMP environment) */
 __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_assert( t->push_num_threads == 0 ) ;

  t->push_num_threads = num_threads ;
}

void __kmpc_push_num_teams(ident_t *loc, kmp_int32 global_tid, kmp_int32 num_teams, kmp_int32 num_threads )
{
  not_implemented() ;
}

void __kmpc_fork_teams( ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ... )
{
  not_implemented() ;
}

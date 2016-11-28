
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_task_utils.h"

#include "mpcomp_types_kmp.h"
#include "mpcomp_types_kmp_atomics.h"

/* Declaration from MPC
TODO: put these declarations in header file
*/
//void mpcomp_internal_begin_parallel_region(int arg_num_threads,
//		mpcomp_new_parallel_region_info_t info ) ;

//void mpcomp_internal_end_parallel_region( mpcomp_instance_t * instance ) ;

/*
 *
 * Target high-priority directives:
 *
 * ---------------
 * SIGNATURE ONLY:
 * ---------------
 *
 * __kmpc_for_static_init_4u
 * __kmpc_for_static_init_8u
 * __kmpc_flush
 *
 * __kmpc_set_lock
 * __kmpc_destroy_lock
 * __kmpc_init_lock
 * __kmpc_unset_lock
 *
 * __kmpc_begin
 * __kmpc_end
 * __kmpc_serialized_parallel
 * __kmpc_end_serialized_parallel
 *
 * __kmpc_atomic_fixed4_wr
 * __kmpc_atomic_fixed8_add
 * __kmpc_atomic_float8_add
 *
 * ---------------
 * IMPLEMENTATION:
 * ---------------
 *
 * __kmpc_for_static_init_8
 * __kmpc_for_static_init_4
 * __kmpc_for_static_fini
 *
 * __kmpc_dispatch_init_4
 * __kmpc_dispatch_next_4
 *
 * __kmpc_critical
 * __kmpc_end_critical
 *
 * __kmpc_single
 * __kmpc_end_single
 *
 * __kmpc_barrier
 *
 * __kmpc_push_num_threads
 * __kmpc_ok_to_fork
 * __kmpc_fork_call
 *
 * __kmpc_master
 * __kmpc_end_master
 *
 * __kmpc_atomic_fixed4_add
 *
 * __kmpc_global_thread_num
 * 
 * __kmpc_reduce
 * __kmpc_end_reduce
 * __kmpc_reduce_nowait
 * __kmpc_end_reduce_nowait
 *
 *
 */

/* MY STRUCTS */

typedef struct wrapper {
  microtask_t f ;
  int argc ;
  void ** args ;
} wrapper_t ;


/********************************
  * FUNCTIONS
  *******************************/
extern int __kmp_x86_pause();
extern int __kmp_invoke_microtask(kmpc_micro pkfn, int gtid, int npr, int argc, void *argv[] );

void wrapper_function( void * arg ) 
{
    int rank;
    wrapper_t * w;

    mpcomp_thread_t* t = ( mpcomp_thread_t*) sctk_openmp_thread_tls;
    rank = omp_get_thread_num();

    w = (wrapper_t *) arg;
    sctk_nodebug("%s: f: %p, argc: %d, args: %p", __func__, w->f, w->argc, w->args );
    __kmp_invoke_microtask( w->f, t->rank, 0, w->argc, w->args );
}

/********************************
  * STARTUP AND SHUTDOWN
  *******************************/

void   
__kmpc_begin ( ident_t * loc, kmp_int32 flags ) 
{
    sctk_error("YO --- hihi");
    mpcomp_init();
// not_implemented() ;
}

void   
__kmpc_end ( ident_t * loc )
{
// not_implemented() ;
}


/********************************
  * PARALLEL FORK/JOIN 
  *******************************/

kmp_int32
__kmpc_ok_to_fork(ident_t * loc)
{
  sctk_nodebug( "__kmpc_ok_to_fork: entering..." ) ;
  return 1 ;
}

/* TEMP */
#if 0 
int intel_temp_argc = 10 ;
void ** intel_temp_args_copy = NULL ;
#endif

void
__kmpc_fork_call(ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ...)
{
  va_list args ;
  int i ;
  void ** args_copy ;
  wrapper_t *w;
  mpcomp_thread_t *t;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

  sctk_error( "__kmpc_fork_call: entering w/ %d arg(s)...", argc ) ;

#if 0
     w = malloc( sizeof( wrapper_t ));
#else
    wrapper_t w_noalloc;
    w = &w_noalloc;
#endif

  /* Handle orphaned directive (initialize OpenMP environment) */
  mpcomp_init() ;
  
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


#if 0 
if ( intel_temp_args_copy == NULL ) 
{
intel_temp_args_copy = (void **)malloc( intel_temp_argc * sizeof( void * ) ) ;
}
if ( argc <= intel_temp_argc ) 
{
args_copy = intel_temp_args_copy ;
} else 
{
  args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
  sctk_assert( args_copy ) ;
}
#else
   args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
   sctk_assert( args_copy ) ;
#endif

  va_start( args, microtask ) ;

  for ( i = 0 ; i < argc ; i++ ) 
  {
     args_copy[i] = va_arg( args, void * ) ;
  }

  va_end(args) ;

  w->f = microtask ;
  w->argc = argc ;
  w->args = args_copy ;
  sctk_error(" f: %p, argc: %d, args: %p", w->f, w->argc, w->args );

  mpcomp_start_parallel_region( wrapper_function, w, t->push_num_threads );

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

	mpcomp_internal_begin_parallel_region( info, 1 ) ;

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
    mpcomp_internal_end_parallel_region( t->instance ) ;

    sctk_nodebug( "%s: leaving (%d)...", __func__, global_tid ) ;
}

void
__kmpc_push_num_threads( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_threads )
{
  mpcomp_thread_t * t ;

  sctk_nodebug( "%s: pushing %d thread(s)", __func__, num_threads ) ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_assert( t->push_num_threads == 0 ) ;

  t->push_num_threads = num_threads ;
}

void 
__kmpc_push_num_teams(ident_t *loc, kmp_int32 global_tid, kmp_int32 num_teams,
    kmp_int32 num_threads )
{
  not_implemented() ;
}

void
__kmpc_fork_teams( ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ... )
{
  not_implemented() ;
}

/********************************
  * SYNCHRONIZATION 
  *******************************/

kmp_int32
__kmpc_global_thread_num(ident_t * loc)
{
  sctk_nodebug( "__kmpc_global_thread_num: " ) ;
  return omp_get_thread_num() ;
}

kmp_int32
__kmpc_global_num_threads(ident_t * loc)
{
  sctk_nodebug( "__kmpc_global_num_threads: " ) ;
  return omp_get_num_threads() ;
}

kmp_int32
__kmpc_bound_thread_num(ident_t * loc)
{
  sctk_nodebug( "__kmpc_bound_thread_num: " ) ;
  return omp_get_thread_num() ;
}

kmp_int32
__kmpc_bound_num_threads(ident_t * loc)
{
  sctk_nodebug( "__kmpc_bound_num_threads: " ) ;
  return omp_get_num_threads() ;
}

kmp_int32
__kmpc_in_parallel(ident_t * loc)
{
  sctk_nodebug( "__kmpc_in_parallel: " ) ;
  return omp_in_parallel() ;
}


/********************************
  * SYNCHRONIZATION 
  *******************************/

void
__kmpc_flush(ident_t *loc, ...) 
{
  /* TODO depending on the compiler, call the right function
   * Maybe need to do the same for mpcomp_flush...
   */
  __sync_synchronize() ;
}

void
__kmpc_barrier(ident_t *loc, kmp_int32 global_tid) 
{
  sctk_nodebug( "[%d] __kmpc_barrier: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;

  mpcomp_barrier() ;
}

kmp_int32
__kmpc_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;

  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }

  mpcomp_barrier() ;

  return 0 ;
}

void
__kmpc_end_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;

  mpcomp_barrier() ;
}

kmp_int32
__kmpc_barrier_master_nowait(ident_t *loc, kmp_int32 global_tid)
{
  not_implemented() ;

  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }

  return 0 ;
}

int __kmp_determine_reduction_method( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, 
                                      size_t reduce_size, void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
                                      kmp_critical_name *lck, mpcomp_thread_t * t )
{
    int retval = critical_reduce_block;
    int team_size;
    int teamsize_cutoff = 4;
    team_size = t->instance->team->info.num_threads;
    
    if (team_size == 1)
    {
        return empty_reduce_block;
    }
    else
    {
        int atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
        int tree_available   = FAST_REDUCTION_TREE_METHOD_GENERATED;
        
        if( tree_available ) 
        {
            if( team_size <= teamsize_cutoff ) 
            {
                if ( atomic_available ) 
                {
                    retval = atomic_reduce_block;
                }
            } 
            else 
            {
                retval = tree_reduce_block;
            }
        } 
        else if ( atomic_available ) 
        {
            retval = atomic_reduce_block;
        }
    }
    
    /* force reduction method */
    if( __kmp_force_reduction_method != reduction_method_not_defined ) 
    {
        int forced_retval;
        int atomic_available, tree_available;
        switch( ( forced_retval = __kmp_force_reduction_method ) )
        {
            case critical_reduce_block:
                sctk_assert(lck);
                if(team_size <= 1)
                    forced_retval = empty_reduce_block;
            break;
            case atomic_reduce_block:
                atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
                sctk_assert(atomic_available);
            break;
            case tree_reduce_block:
                tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;
                sctk_assert(tree_available);
                forced_retval = tree_reduce_block;
            break;
            default:
                forced_retval = critical_reduce_block;
                sctk_debug("Unknown reduction method");
        }
        retval = forced_retval;
    }
    return retval;
}

kmp_int32 
__kmpc_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
    int retval = 0;
    int packed_reduction_method;
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

    /* get reduction method */
    packed_reduction_method = __kmp_determine_reduction_method( loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t);
    t->reduction_method = packed_reduction_method; 
    if( packed_reduction_method == critical_reduce_block ) 
    {
        mpcomp_anonymous_critical_begin() ;
        retval = 1;
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
        retval = 1;
    }
    else if( packed_reduction_method == atomic_reduce_block )
    {
        retval = 2;
    }
    else if ( packed_reduction_method == tree_reduce_block )
    {
        mpcomp_anonymous_critical_begin() ;
        retval = 1;
    }
    else
    {
        sctk_error("__kmpc_reduce_nowait: Unexpected reduction method %d", packed_reduction_method);
        sctk_abort();
    }
    return retval;
}

void 
__kmpc_end_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck ) 
{
    int packed_reduction_method;

    /* get reduction method */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    
    packed_reduction_method = t->reduction_method;
    
    if( packed_reduction_method == critical_reduce_block )
    {
        mpcomp_anonymous_critical_end() ;
        mpcomp_barrier();
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == atomic_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == tree_reduce_block ) 
    {
        mpcomp_anonymous_critical_end() ;
    } 
    else 
    {
        sctk_error("__kmpc_end_reduce_nowait: Unexpected reduction method %d", packed_reduction_method);
        sctk_abort();
    }
}

kmp_int32 
__kmpc_reduce( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /*
     TODO
     lck!=NULL ==> CRITICAL version
     reduce_data!=NULL && reduce_func!=NULL ==> TREE version
     (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE ==> ATOMIC version

     Need to add one variable to remember the choice taken.
     This variable will be written here and read in the end_reduce function

     To be compliant, we execute only the CRITICAL version which is supposed to be available
     in every situation.
   */


  sctk_debug( "[%d] __kmpc_reduce %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;

#if 0
  printf( "[%d] __kmpc_reduce %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d\n",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;
#endif

#if 0
  /* Version with atomic operation */
  return 2 ;
#endif

  /* Version with critical section */
  sctk_assert( lck != NULL ) ;
  mpcomp_anonymous_critical_begin() ;
  return 1 ;
}

void 
__kmpc_end_reduce( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck )
{

  sctk_debug( "[%d] __kmpc_end_reduce",
      ( (mpcomp_thread_t *) sctk_openmp_thread_tls )->rank ) ;
#if 0
  /* Version with atomic operation */
  return ;
#endif


#if 1
  /* Version with critical section */
  sctk_assert( lck != NULL ) ;
  mpcomp_anonymous_critical_end() ;
  mpcomp_barrier();
  return ;
#endif

  // not_implemented() ;
}



/********************************
  * WORK_SHARING
  *******************************/

kmp_int32
__kmpc_master(ident_t *loc, kmp_int32 global_tid)
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_nodebug( "[%d] __kmp_master: entering",
      t->rank ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }
  return 0 ;

}

void
__kmpc_end_master(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_end_master: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void 
__kmpc_ordered(ident_t *loc, kmp_int32 global_tid)
{
    mpcomp_ordered_begin();
}

void
__kmpc_end_ordered(ident_t *loc, kmp_int32 global_tid)
{
    mpcomp_ordered_end();
}

void
__kmpc_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  sctk_nodebug( "[%d] __kmpc_critical: enter %p (%d,%d,%d,%d,%d,%d,%d,%d)",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      crit,
      (*crit)[0], (*crit)[1], (*crit)[2], (*crit)[3],
      (*crit)[4], (*crit)[5], (*crit)[6], (*crit)[7]
      ) ;
  /* TODO Handle named critical */
mpcomp_anonymous_critical_begin () ;
}

void
__kmpc_end_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  /* TODO Handle named critical */
  mpcomp_anonymous_critical_end () ;
}

kmp_int32
__kmpc_single(ident_t *loc, kmp_int32 global_tid)
{
    mpcomp_thread_t* t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert( t );
    sctk_nodebug( "[%d] %s: entering...", t->rank, __func__ ) ;
    return mpcomp_do_single() ;
}

void
__kmpc_end_single(ident_t *loc, kmp_int32 global_tid)
{
    mpcomp_thread_t* t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert( t );
    sctk_nodebug( "[%d] %s: entering...", t->rank, __func__ ) ;
}

void
__kmpc_for_static_init_4( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int32 * plower, kmp_int32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
    long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_int32 trip_count;
    //save old pupper
    kmp_int32 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_4: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_int32)from ;
                *pupper=(kmp_int32)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_init_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_uint32 * plower, kmp_uint32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
    unsigned long long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_uint32 trip_count;
    //save old pupper
    kmp_uint32 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_4u: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = mpcomp_static_schedule_get_single_chunk_ull( (unsigned long long)*plower, 
                (unsigned long long)*pupper+(unsigned long long)incr, (unsigned long long)incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_4u (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_uint32)from ;
                *pupper=(kmp_uint32)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_init_8( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int64 * plower, kmp_int64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
    long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_int64 trip_count;
    //save old pupper
    kmp_int64 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_8: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_8 (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_int64)from ;
                *pupper=(kmp_int64)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_init_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_uint64 * plower, kmp_uint64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
  /* TODO: the same as unsigned long long in GCC... */
  sctk_nodebug( "__kmpc_for_static_init_8u: siweof long = %d, sizeof long long %d",
      sizeof( long ), sizeof( long long ) ) ;
    unsigned long long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_uint64 trip_count;
    //save old pupper
    kmp_uint64 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
       
        if(incr > 0)
        {
            *pstride = (*pupper - *plower + 1);
            if(*pstride < incr) *pstride = incr;
        }
        else
        {
            *pstride = ((-1)*(*plower - *pupper + 1));
            if(*pstride > incr) *pstride = incr; 
        }
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_8u: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = mpcomp_static_schedule_get_single_chunk_ull( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_8u (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=from ;
                *pupper=to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_fini( ident_t * loc, kmp_int32 global_tid ) 
{
  sctk_nodebug( "[%d] __kmpc_for_static_fini: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank ) ;

  /* Nothing to do here... */
}

void 
__kmpc_dispatch_init_4(ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
    kmp_int32 lb, kmp_int32 ub, kmp_int32 st, kmp_int32 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_4: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* add to sync with MPC runtime bounds */
    long long add = ((ub - lb) % st == 0) ? st : st - ((ub - lb) % st);
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->schedule_is_forced = 1;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)chunk) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)chunk) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_4u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_uint32 lb, kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_4u: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* add to sync with MPC runtime bounds */
    long long add = ((ub - lb) % st == 0) ? st : st - ((ub - lb) % st);
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->schedule_is_forced = 1;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      mpcomp_dynamic_loop_init_ull( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (unsigned long long)lb, (unsigned long long)(ub+add), (unsigned long long)st, (unsigned long long)chunk) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      mpcomp_static_loop_init_ull(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (unsigned long long)lb, (unsigned long long)(ub+add), (unsigned long long)st, (unsigned long long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      mpcomp_static_loop_init_ull(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (unsigned long long)lb, (unsigned long long)(ub+add), (unsigned long long)st, (unsigned long long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      mpcomp_dynamic_loop_init_ull(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (unsigned long long)lb, (unsigned long long)(ub+add), (unsigned long long)st, (unsigned long long)chunk) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_8( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_int64 lb, kmp_int64 ub,
                        kmp_int64 st, kmp_int64 chunk )
{
    /* add to sync with MPC runtime bounds */
    long long add = ((ub - lb) % st == 0) ? st : st - ((ub - lb) % st);
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->schedule_is_forced = 1;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)chunk) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)(ub+add), (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	 (long)lb, (long)(ub+add), (long)st, (long)chunk) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_8u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                         kmp_uint64 lb, kmp_uint64 ub,
                         kmp_int64 st, kmp_int64 chunk )
{
    /* add to sync with MPC runtime bounds */
    unsigned long long add = ((ub - lb) % st == 0) ? st : st - ((ub - lb) % st);

    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->schedule_is_forced = 1;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      mpcomp_dynamic_loop_init_ull( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+add, st, chunk) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      mpcomp_static_loop_init_ull(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+add, st, 0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      mpcomp_static_loop_init_ull(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+add, st, chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      mpcomp_dynamic_loop_init_ull(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+add, st, chunk) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}



int
__kmpc_dispatch_next_4( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int32 *p_lb, kmp_int32 *p_ub, kmp_int32 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
  long long add = ((*p_ub - *p_lb) % *p_st == 0) ? *p_st : *p_st - ((*p_ub - *p_lb) % *p_st);
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(add)), (long)(*p_st), &from, &to);
        else
            ret = mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
    case kmp_ord_runtime:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(add)), (long)(*p_st), &from, &to);
        else
            ret = mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_int32) from ;
  *p_ub = (kmp_int32)to - (kmp_int32)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_sch_static_chunked:
            mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
        case kmp_ord_runtime:
            mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint32 *p_lb, kmp_uint32 *p_ub, kmp_int32 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  unsigned long long from, to ;
  unsigned long long add = ((*p_ub - *p_lb) % *p_st == 0) ? *p_st : *p_st - ((*p_ub - *p_lb) % *p_st);
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = mpcomp_loop_ull_dynamic_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk_ull ((unsigned long long)(*p_lb), (unsigned long long)((*p_ub)+(add)), (unsigned long long)(*p_st), &from, &to);
        else
            ret = mpcomp_loop_ull_static_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = mpcomp_loop_ull_static_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = mpcomp_loop_ull_guided_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = mpcomp_loop_ull_ordered_dynamic_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
    case kmp_ord_runtime:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk_ull ((unsigned long long)(*p_lb), (unsigned long long)((*p_ub)+(add)), (unsigned long long)(*p_st), &from, &to);
        else
            ret = mpcomp_loop_ull_ordered_static_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = mpcomp_loop_ull_ordered_guided_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_uint32) from ;
  *p_ub = (kmp_uint32)to - (kmp_uint32)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_sch_static_chunked:
            mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
        case kmp_ord_runtime:
            mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_8( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int64 *p_lb, kmp_int64 *p_ub, kmp_int64 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
  long long add = ((*p_ub - *p_lb) % *p_st == 0) ? *p_st : *p_st - ((*p_ub - *p_lb) % *p_st);
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(add)), (long)(*p_st), &from, &to);
        else
            ret = mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
    case kmp_ord_runtime:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(add)), (long)(*p_st), &from, &to);
        else
            ret = mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_int64) from ;
  *p_ub = (kmp_int64)to - (kmp_int64)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_sch_static_chunked:
            mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
        case kmp_ord_runtime:
            mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint64 *p_lb, kmp_uint64 *p_ub, kmp_int64 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  unsigned long long from, to ;
  unsigned long long add = ((*p_ub - *p_lb) % *p_st == 0) ? *p_st : *p_st - ((*p_ub - *p_lb) % *p_st);
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = mpcomp_loop_ull_dynamic_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk_ull (*p_lb, (*p_ub+add), *p_st, &from, &to);
        else
            ret = mpcomp_loop_ull_static_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = mpcomp_loop_ull_static_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = mpcomp_loop_ull_guided_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = mpcomp_loop_ull_ordered_dynamic_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
    case kmp_ord_runtime:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = mpcomp_static_schedule_get_single_chunk_ull (*p_lb, (*p_ub+add), *p_st, &from, &to);
        else
            ret = mpcomp_loop_ull_ordered_static_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = mpcomp_loop_ull_ordered_guided_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_8u: %llu -> %llu excl, %llu incl [%llu] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = from ;
  *p_ub = to - (kmp_uint64)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_sch_static_chunked:
            mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
        case kmp_ord_runtime:
            mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

void
__kmpc_dispatch_fini_4( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_4 */
}

void
__kmpc_dispatch_fini_8( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_4u */
}

void
__kmpc_dispatch_fini_4u( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_8 */
}

void
__kmpc_dispatch_fini_8u( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_8u */
}



/********************************
  * THREAD PRIVATE DATA SUPPORT
  *******************************/

int __kmp_default_tp_capacity() 
{
    char * env;
    int nth = 128;
    int __kmp_xproc = 0, req_nproc = 0, r = 0, __kmp_max_nth = 32 * 1024;
    
    r = sysconf( _SC_NPROCESSORS_ONLN );
    __kmp_xproc = (r > 0) ? r : 2;

    env = getenv ("OMP_NUM_THREADS");
    if (env != NULL)
    {
        req_nproc = strtol(env, NULL, 10);              
    }

    if (nth < (4 * req_nproc))
        nth = (4 * req_nproc);
    if (nth < (4 * __kmp_xproc))
        nth = (4 * __kmp_xproc);

    if (nth > __kmp_max_nth)
        nth = __kmp_max_nth;

    return nth;
}

struct private_common *
__kmp_threadprivate_find_task_common( struct common_table *tbl, int gtid, void *pc_addr )
{
    struct private_common *tn;
    for (tn = tbl->data[ KMP_HASH(pc_addr) ]; tn; tn = tn->next) 
    {
        if (tn->gbl_addr == pc_addr) 
        {
            return tn;
        }   
    }
    return 0;
}

struct shared_common *
__kmp_find_shared_task_common( struct shared_table *tbl, int gtid, void *pc_addr )
{
    struct shared_common *tn;
    sctk_error( "%s -- YO", __func__ );
    for (tn = tbl->data[ KMP_HASH(pc_addr) ]; tn; tn = tn->next) 
    {
        if (tn->gbl_addr == pc_addr) 
        {
            return tn;
        }   
    }
    return 0;
}

struct private_common *
kmp_threadprivate_insert( int gtid, void *pc_addr, void *data_addr, size_t pc_size )
{
    struct private_common *tn, **tt;
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    mpcomp_thread_t *t ; 
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

    sctk_error( "%s -- YO", __func__ );
    /* critical section */
    sctk_thread_mutex_lock (&lock);
        tn = (struct private_common *) sctk_malloc( sizeof (struct private_common) );
        tn->gbl_addr = pc_addr;
        tn->cmn_size = pc_size;
        
        if(gtid == 0)
            tn->par_addr = (void *) pc_addr;
        else
            tn->par_addr = (void *) sctk_malloc( tn->cmn_size );
        
        memcpy(tn->par_addr, pc_addr, pc_size);
    sctk_thread_mutex_unlock (&lock);
    /* end critical section */
    
    tt = &(t->th_pri_common->data[ KMP_HASH(pc_addr) ]);
    tn->next = *tt;
    *tt = tn;
    tn->link = t->th_pri_head;
    t->th_pri_head = tn;
    
    return tn;
}

void * 
__kmpc_threadprivate ( ident_t *loc, kmp_int32 global_tid, void * data, size_t size )
{
    void *ret = NULL;
    struct private_common *tn;
    mpcomp_thread_t *t ; 
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

    tn = __kmp_threadprivate_find_task_common( t->th_pri_common, global_tid, data );
    if (!tn)
    {
        tn = kmp_threadprivate_insert( global_tid, data, data, size );
    }
    else
    {
        if((size_t)size > tn->cmn_size)
        {
            sctk_error("TPCommonBlockInconsist: -> Wong size threadprivate variable found");
            sctk_abort();
        }
    }
    
    ret = tn->par_addr;
    return ret ;
}

void
__kmpc_copyprivate(ident_t *loc, kmp_int32 global_tid, size_t cpy_size, void *cpy_data,
    void (*cpy_func)(void *, void *), kmp_int32 didit )
{
    mpcomp_thread_t *t ;    /* Info on the current thread */

    void **data_ptr;
    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

    /* In this flow path, the number of threads should not be 1 */
    sctk_assert( t->info.num_threads > 0 ) ;

    /* Grab the team info */
    sctk_assert( t->instance != NULL ) ;
    sctk_assert( t->instance->team != NULL ) ;
    
    data_ptr = &(t->instance->team->single_copyprivate_data);
    if (didit) *data_ptr = cpy_data;
    
    mpcomp_barrier();

    if (! didit) (*cpy_func)( cpy_data, *data_ptr );

    mpcomp_barrier();
}

void *
__kmpc_threadprivate_cached( ident_t *loc, kmp_int32 global_tid, void *data, size_t size, void *** cache)
{  
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER; 
  int __kmp_tp_capacity = __kmp_default_tp_capacity();
  if (*cache == 0)
  {
    sctk_thread_mutex_lock (&lock);
    if (*cache == 0)
    {
        //handle cache to be dealt with later
        void ** my_cache;
        my_cache = (void**) malloc(sizeof( void * ) * __kmp_tp_capacity + sizeof ( kmp_cached_addr_t ));
        kmp_cached_addr_t *tp_cache_addr;
        tp_cache_addr = (kmp_cached_addr_t *) & my_cache[__kmp_tp_capacity];
        tp_cache_addr -> addr = my_cache;
        tp_cache_addr -> next = __kmp_threadpriv_cache_list;
        __kmp_threadpriv_cache_list = tp_cache_addr;
        *cache = my_cache;
    }
    sctk_thread_mutex_unlock (&lock);
  }
  
  void *ret = NULL;
  if ((ret = (*cache)[ global_tid ]) == 0)
  {
      ret = __kmpc_threadprivate( loc, global_tid, data, (size_t) size);
      (*cache)[ global_tid ] = ret;
  }
  return ret;
}

void
__kmpc_threadprivate_register( ident_t *loc, void *data, kmpc_ctor ctor, kmpc_cctor cctor,
    kmpc_dtor dtor)
{
    struct shared_common *d_tn, **lnk_tn;

    /* Only the global data table exists. */
    d_tn = __kmp_find_shared_task_common( &__kmp_threadprivate_d_table, -1, data );

    if (d_tn == 0) 
    {
        d_tn = (struct shared_common *) sctk_malloc( sizeof( struct shared_common ) );
        d_tn->gbl_addr = data;

        d_tn->ct.ctor = ctor;
        d_tn->cct.cctor = cctor;
        d_tn->dt.dtor = dtor;
        
        lnk_tn = &(__kmp_threadprivate_d_table.data[ KMP_HASH(data) ]);

        d_tn->next = *lnk_tn;
        *lnk_tn = d_tn;
    }
}

void
__kmpc_threadprivate_register_vec(ident_t *loc, void *data, kmpc_ctor_vec ctor, 
    kmpc_cctor_vec cctor, kmpc_dtor_vec dtor, size_t vector_length )
{
    struct shared_common *d_tn, **lnk_tn;

    d_tn = __kmp_find_shared_task_common( &__kmp_threadprivate_d_table,
                                          -1, data );        /* Only the global data table exists. */
    if (d_tn == 0) 
    {
        d_tn = (struct shared_common *) sctk_malloc( sizeof( struct shared_common ) );
        d_tn->gbl_addr = data;

        d_tn->ct.ctorv = ctor;
        d_tn->cct.cctorv = cctor;
        d_tn->dt.dtorv = dtor;
        d_tn->is_vec = TRUE;
        d_tn->vec_len = (size_t) vector_length;
        
        lnk_tn = &(__kmp_threadprivate_d_table.data[ KMP_HASH(data) ]);

        d_tn->next = *lnk_tn;
        *lnk_tn = d_tn;
    }
}

/********************************
  * TASKQ INTERFACE
  *******************************/

typedef struct kmpc_thunk_t {
} kmpc_thunk_t ;

typedef void (*kmpc_task_t) (kmp_int32 global_tid, struct kmpc_thunk_t *thunk);

typedef struct kmpc_shared_vars_t {
} kmpc_shared_vars_t;

kmpc_thunk_t * 
__kmpc_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_task_t taskq_task, size_t sizeof_thunk,
    size_t sizeof_shareds, kmp_int32 flags, kmpc_shared_vars_t **shareds)
{
not_implemented() ;
return NULL ;
}

void 
__kmpc_end_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmp_int32 
__kmpc_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk, kmp_int32 status)
{
not_implemented() ;
}

void __kmpc_end_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmpc_thunk_t * 
__kmpc_task_buffer (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *taskq_thunk, kmpc_task_t task)
{
not_implemented() ;
return NULL ;
}

/********************************
  * TASKING SUPPORT
  *******************************/

typedef kmp_int32 (* kmp_routine_entry_t)( kmp_int32, void * );

typedef struct kmp_task {                   /* GEH: Shouldn't this be aligned somehow? */
    void *              shareds;            /**< pointer to block of pointers to shared vars   */
    kmp_routine_entry_t routine;            /**< pointer to routine to call for executing task */
    kmp_int32           part_id;            /**< part id for the task                          */
#if OMP_40_ENABLED
    kmp_routine_entry_t destructors;        /* pointer to function to invoke deconstructors of firstprivate C++ objects */
#endif // OMP_40_ENABLED
    /*  private vars  */
} kmp_task_t;

/* From kmp_os.h for this type */
typedef long             kmp_intptr_t;

typedef struct kmp_depend_info {
     kmp_intptr_t               base_addr;
     size_t 	                len;
     struct {
         bool                   in:1;
         bool                   out:1;
     } flags;
} kmp_depend_info_t;

struct kmp_taskdata {                                 /* aligned during dynamic allocation       */
#if 0
    kmp_int32               td_task_id;               /* id, assigned by debugger                */
    kmp_tasking_flags_t     td_flags;                 /* task flags                              */
    kmp_team_t *            td_team;                  /* team for this task                      */
    kmp_info_p *            td_alloc_thread;          /* thread that allocated data structures   */
                                                      /* Currently not used except for perhaps IDB */
    kmp_taskdata_t *        td_parent;                /* parent task                             */
    kmp_int32               td_level;                 /* task nesting level                      */
    ident_t *               td_ident;                 /* task identifier                         */
                            // Taskwait data.
    ident_t *               td_taskwait_ident;
    kmp_uint32              td_taskwait_counter;
    kmp_int32               td_taskwait_thread;       /* gtid + 1 of thread encountered taskwait */
    kmp_internal_control_t  td_icvs;                  /* Internal control variables for the task */
    volatile kmp_uint32     td_allocated_child_tasks;  /* Child tasks (+ current task) not yet deallocated */
    volatile kmp_uint32     td_incomplete_child_tasks; /* Child tasks not yet complete */
#if OMP_40_ENABLED
    kmp_taskgroup_t *       td_taskgroup;         // Each task keeps pointer to its current taskgroup
    kmp_dephash_t *         td_dephash;           // Dependencies for children tasks are tracked from here
    kmp_depnode_t *         td_depnode;           // Pointer to graph node if this task has dependencies
#endif
#if KMP_HAVE_QUAD
    _Quad                   td_dummy;             // Align structure 16-byte size since allocated just before kmp_task_t
#else
    kmp_uint32              td_dummy[2];
#endif
#endif
} ;
typedef struct kmp_taskdata  kmp_taskdata_t;

/* FOR OPENMP 3 */

#define KMP_TASKDATA_TO_TASK(task_data) (( mpcomp_task_t* ) taskdata - 1 )
#define KMP_TASKDATA_TO_TASK(task) ((kmp_task_t*) taskdata +1 )

kmp_int32
__kmpc_omp_task( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
    struct mpcomp_task_s* mpcomp_task = (char*) new_task - sizeof( struct mpcomp_task_s );
    __mpcomp_task_process( mpcomp_task, true ); 
    return 0 ;
}

void
__kmp_omp_task_wrapper( void* task_ptr )
{
    //sctk_error("run task");
    kmp_task_t* kmp_task_ptr = (kmp_task_t*) task_ptr;
    kmp_task_ptr->routine( 0, kmp_task_ptr );
}

kmp_task_t*
__kmpc_omp_task_alloc( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 flags,
                       size_t sizeof_kmp_task_t, size_t sizeof_shareds,
                       kmp_routine_entry_t task_entry )
{
   sctk_assert( sctk_openmp_thread_tls );	 
   mpcomp_thread_t* t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

   mpcomp_init();

   sctk_assert( t->mvp );
   sctk_assert( t->mvp->father );
   sctk_assert( t->mvp->father->id_numa );

   /* default pading */
   const long align_size =  sizeof( void* ) ;
    
   // mpcomp_task + arg_size
   const long mpcomp_kmp_taskdata = sizeof( mpcomp_task_t) + sizeof_kmp_task_t;
   const long mpcomp_task_info_size = mpcomp_task_align_single_malloc( mpcomp_kmp_taskdata, align_size);
   const long mpcomp_task_data_size = mpcomp_task_align_single_malloc( sizeof_shareds, align_size ); 

   /* Compute task total size */
   long mpcomp_task_tot_size = mpcomp_task_info_size;

   sctk_assert( MPCOMP_OVERFLOW_SANITY_CHECK( mpcomp_task_tot_size, mpcomp_task_data_size ));
   mpcomp_task_tot_size += mpcomp_task_data_size;

   struct mpcomp_task_s *new_task = mpcomp_malloc( 1, mpcomp_task_tot_size, 0/* t->mvp->father->id_numa */);
   sctk_assert( new_task != NULL );
   
   void* task_data = ( sizeof_shareds > 0 ) ? ( void*) ( (uintptr_t) new_task + mpcomp_task_info_size ) : NULL;
   
   /* Create new task */
   kmp_task_t* compiler_infos = ( kmp_task_t* ) ( ( char* ) new_task + sizeof( struct mpcomp_task_s ) );
   __mpcomp_task_infos_init( new_task, __kmp_omp_task_wrapper, compiler_infos, t );

   compiler_infos->shareds = task_data;
   compiler_infos->routine = task_entry;
   compiler_infos->part_id = 0;
   mpcomp_task_add_to_parent( new_task ); 

   return compiler_infos;
}

void
__kmpc_omp_task_begin_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void
__kmpc_omp_task_complete_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}

kmp_int32
__kmpc_omp_task_parts( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskwait( ident_t *loc_ref, kmp_int32 gtid )
{
    mpcomp_taskwait();
    return 0 ;
}

kmp_int32
__kmpc_omp_taskyield( ident_t *loc_ref, kmp_int32 gtid, int end_part )
{
not_implemented() ;
return 0 ;
}

/* Following 2 functions should be enclosed by "if TASK_UNUSED" */

void 
__kmpc_omp_task_begin( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void 
__kmpc_omp_task_complete( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}


/* FOR OPENMP 4 */

void 
__kmpc_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

void 
__kmpc_end_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

kmp_int32 
__kmpc_omp_task_with_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task,
    kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_omp_wait_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
}

void 
__kmp_release_deps ( kmp_int32 gtid, kmp_taskdata_t *task )
{
not_implemented() ;
}







/********************************
  * OTHERS FROM KMP.H
  *******************************/

void *
kmpc_malloc( size_t size )
{
not_implemented() ;
return NULL ;
}

void *
kmpc_calloc( size_t nelem, size_t elsize )
{
not_implemented() ;
return NULL ;
}

void *
kmpc_realloc( void *ptr, size_t size )
{
not_implemented() ;
return NULL ;
}

void  kmpc_free( void *ptr )
{
not_implemented() ;
}

int 
__kmpc_invoke_task_func( int gtid )
{
not_implemented() ;
return 0 ;
}

void 
kmpc_set_blocktime (int arg)
{
not_implemented() ;
}


/*
 * Lock structure
 */
typedef struct omp_lock_t {
  void* lk;
} iomp_lock_t;

/*
 *  * Lock interface routines (fast versions with gtid passed in)
 *   */
void
__kmpc_init_lock( ident_t *loc, kmp_int32 gtid,  void **user_lock )
{
  sctk_debug( "[%d] __kmpc_init_lock: "
      "Addr %p",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, user_lock ) ;

  if( user_lock == NULL ) {
    sctk_abort();
  }

  //((iomp_lock_t*) user_lock)->lk = (void*) sctk_malloc( sizeof( omp_lock_t ));
  omp_init_lock((omp_lock_t *)&((iomp_lock_t *)user_lock)->lk);
}

void
__kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_abort();
  }

  //((iomp_lock_t*) user_lock)->lk = (void*) sctk_malloc( sizeof(
  // omp_nest_lock_t ));
  omp_init_nest_lock((omp_nest_lock_t *)&((iomp_lock_t *)user_lock)->lk);
}

void
__kmpc_destroy_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  omp_lock_t* llk;

  if( user_lock == NULL ) {
    sctk_abort();
  }

  llk = ((iomp_lock_t*) user_lock)->lk;
  omp_destroy_lock(&llk);
  // free( llk );
  // llk = NULL;
}

void
__kmpc_destroy_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  omp_nest_lock_t* llk;

  if( user_lock == NULL ) {
    sctk_abort();
  }

  llk = ((iomp_lock_t*) user_lock)->lk;
  omp_destroy_nest_lock(&llk);
  // free( llk );
  // llk = NULL;
}

void
__kmpc_set_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_abort();
  }

  omp_set_lock( (omp_lock_t*) &((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_set_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_abort();
  }

  omp_set_nest_lock( (omp_nest_lock_t*) &((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_unset_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_abort();
  }

  omp_unset_lock( (omp_lock_t*) &((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_unset_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_abort();
  }

  omp_unset_nest_lock( (omp_nest_lock_t*) &((iomp_lock_t*) user_lock)->lk );
}

int
__kmpc_test_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_abort();
  }

  return omp_test_lock( (omp_lock_t*) &((iomp_lock_t*) user_lock)->lk );
}

int
__kmpc_test_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_test_nest_lock: Null lock reference");
    sctk_abort();
  }
  return omp_test_nest_lock( (omp_nest_lock_t*) &((iomp_lock_t*) user_lock)->lk );
}

/* ------------------------------------------------------------------------ */
/* Generic atomic routines                                                  */
/* ------------------------------------------------------------------------ */
void
__kmpc_atomic_4( ident_t *id_ref, int gtid, void* lhs, void* rhs, void (*f)( void *, void *, void * ) )
{
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    if (
#if defined __i386 || defined __x86_64
    TRUE                    /* no alignment problems */
#else
    ! ( (kmp_uintptr_t) lhs & 0x3)      /* make sure address is 4-byte aligned */
#endif
    )
    {
        kmp_int32 old_value, new_value;
        old_value = *(kmp_int32 *) lhs;
        (*f)( &new_value, &old_value, rhs );

        /* TODO: Should this be acquire or release? */
        while ( ! __kmp_compare_and_store32 ( (kmp_int32 *) lhs,
                *(kmp_int32 *) &old_value, *(kmp_int32 *) &new_value ) )
        {
            DO_PAUSE;
            old_value = *(kmp_int32 *) lhs;
            (*f)( &new_value, &old_value, rhs );
        }
        return;
    }
    else 
    {
        sctk_thread_mutex_lock( &lock);
        (*f)( lhs, lhs, rhs );
        sctk_thread_mutex_unlock( &lock);
    }
}

void 
__kmpc_atomic_fixed4_wr(  ident_t *id_ref, int gtid, kmp_int32   * lhs, kmp_int32   rhs ) 
{
      sctk_nodebug( "[%d] __kmpc_atomic_fixed4_wr: "
            "Write %d",
                  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

        __kmp_xchg_fixed32( lhs, rhs ) ;

#if 0
  mpcomp_atomic_begin() ;

    *lhs = rhs ;

      mpcomp_atomic_end() ;

        /* TODO: use assembly functions by Intel if available */
#endif
}

void 
__kmpc_atomic_float8_add(  ident_t *id_ref, int gtid, kmp_real64 * lhs, kmp_real64 rhs )
{
      sctk_nodebug( "[%d] (ASM) __kmpc_atomic_float8_add: "
            "Add %g",
                  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

#if 0
  __kmp_test_then_add_real64( lhs, rhs ) ;
#endif

    /* TODO check how we can add this function to asssembly-dedicated module */

    /* TODO use MPCOMP_MIC */

#if __MIC__ || __MIC2__
#warning "MIC => atomic locks"
  mpcomp_atomic_begin() ;

  *lhs += rhs ;

  mpcomp_atomic_end() ;
#else 
#warning "NON MIC => atomic optim"
  __kmp_test_then_add_real64( lhs, rhs ) ;
#endif


          /* TODO: use assembly functions by Intel if available */
}



#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_global.h"
#include "mpcomp_intel_reduction.h"

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
        __mpcomp_anonymous_critical_begin() ;
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
        __mpcomp_anonymous_critical_begin() ;
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
        __mpcomp_anonymous_critical_end() ;
        __mpcomp_barrier();
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == atomic_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == tree_reduce_block ) 
    {
        __mpcomp_anonymous_critical_end() ;
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
  __mpcomp_anonymous_critical_begin() ;
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
  __mpcomp_anonymous_critical_end() ;
  __mpcomp_barrier();
  return ;
#endif

  // not_implemented() ;
}

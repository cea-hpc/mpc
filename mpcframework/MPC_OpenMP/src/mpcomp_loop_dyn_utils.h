/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_MPCOMP_LOOP_DYN_UTILS_H__
#define __MPCOMP_MPCOMP_LOOP_DYN_UTILS_H__

#include "sctk_debug.h"
#include "mpcomp_types.h"

int __mpcomp_get_static_nb_chunks_per_rank( int rank, int num_threads,  mpcomp_loop_long_iter_t* loop );

/* dest = src1+src2 in base 'base' of size 'depth' up to dimension 'max_depth'
 */
static inline int
__mpcomp_loop_dyn_dynamic_add( int * dest, int * src1, int *src2, 
		int * base, int depth, int max_depth, 
		int include_carry_over ) 
{
		int i ;
		int ret = 1 ;
		int carry_over = 1 ; 

		/* Step to the next target */
		for ( i = depth -1 ; i >= max_depth ; i-- ) {
			int value ;

			value = src1[i] + src2[i];

			carry_over = 0 ;
			dest[i] = value ;
			if ( value >= base[i] ) {
				if ( i == max_depth ) {
					ret = 0 ;
				}
				if ( include_carry_over ) {
					carry_over = value / base[i] ;
				}
				dest[i] = (value %  base[i] ) ;
			}
		}

		return ret ;
}

/* Return 1 if overflow, otherwise 0 */
static inline int
__mpcomp_loop_dyn_dynamic_increase( int * target, int * base,
		int depth, int max_depth, int include_carry_over ) 
{
		int i ;
		int carry_over = 1;
		int ret = 0 ;

		/* Step to the next target */
		for ( i = depth -1 ; i >= max_depth ; i-- ) {
			int value ;

			value = target[i] + carry_over ;

			carry_over = 0 ;
			target[i] = value ;
			if ( value >= base[i] ) {
				if ( i == max_depth ) {
					ret = 1 ;
				}
				if ( include_carry_over ) {
					carry_over = value / base[i] ;
				}
				target[i] = (value %  base[i] ) ;
			}
		}

		return ret ;
}

static inline int 
__mpcomp_loop_dyn_get_for_dyn_current( mpcomp_thread_t* thread )
{
    return sctk_atomics_load_int( &( thread->for_dyn_ull_current ) );
}

static inline int 
__mpcomp_loop_dyn_get_for_dyn_prev_index( mpcomp_thread_t* thread ) 
{
    const int for_dyn_current = __mpcomp_loop_dyn_get_for_dyn_current( thread );
    return ( for_dyn_current + MPCOMP_MAX_ALIVE_FOR_DYN -1  ) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
}   

static inline int 
__mpcomp_loop_dyn_get_for_dyn_index( mpcomp_thread_t* thread )
{
    const int for_dyn_current = __mpcomp_loop_dyn_get_for_dyn_current( thread );
    return ( for_dyn_current % ( MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) );
}

static inline int
__mpcomp_loop_dyn_get_chunk_from_target( mpcomp_thread_t* thread, mpcomp_thread_t* target )
{   
    int prev, cur;
        
    /* Compute the index of the dynamic for construct */
    const int index = __mpcomp_loop_dyn_get_for_dyn_index( thread );
    
        cur = sctk_atomics_load_int( &(target->for_dyn_remain[index].i) );

    if( cur <= 0 ) 
    {
        return 0;
    }

    do
    {
        prev = cur;
        cur = sctk_atomics_cas_int( &(target->for_dyn_remain[index].i), prev, prev-1 );
    } while( cur > 0 && cur != prev );

    return ( cur > 0 ) ? cur : 0;
}

static inline void 
__mpcomp_loop_dyn_target_init( mpcomp_thread_t* thread ) 
{
    int i ;

    sctk_assert( thread );

    if( thread->for_dyn_target )
        return;

    sctk_assert( thread->instance );
    const int tree_depth = thread->instance->tree_depth;

    thread->for_dyn_target = (int *) malloc( tree_depth * sizeof( int ) ) ;
    sctk_assert( thread->for_dyn_target );

	thread->for_dyn_shift = (int *) malloc( tree_depth * sizeof( int ) ) ;
    sctk_assert( thread->for_dyn_shift );
    
    sctk_assert( thread->mvp );
    sctk_assert( thread->mvp->tree_rank );

    for ( i = 0 ; i < tree_depth ; i++ ) 
    {
		thread->for_dyn_shift[i] = 0 ;
	    thread->for_dyn_target[i] = thread->mvp->tree_rank[i];
	}

	sctk_nodebug( "[%d] %s: initialization of target and shift", thread->rank, __func__ );
}



static inline void 
__mpcomp_loop_dyn_init_target_chunk_ull( mpcomp_thread_t* thread, mpcomp_thread_t* target, unsigned int num_threads )
{
	/* Compute the index of the dynamic for construct */
	const int index = __mpcomp_loop_dyn_get_for_dyn_index( thread );
     
    //if( !sctk_spinlock_trylock( &( target->info.update_lock ) ) )
    //{
        sctk_spinlock_lock(  &( target->info.update_lock ) );
        /* Get the current id of remaining chunk for the target */
        int cur = sctk_atomics_load_int( &(target->for_dyn_remain[index].i) );
        if( cur < 0 && ( ( target == thread ) || ( ( __mpcomp_loop_dyn_get_for_dyn_current( thread )  > __mpcomp_loop_dyn_get_for_dyn_current( target ))) ) )
        {
            /* t->for_dyn_total  = for_dyn_total  ?? */
            mpcomp_loop_ull_iter_t* loop = &(thread->info.loop_infos.loop.mpcomp_ull);
            const int for_dyn_total = __mpcomp_compute_static_nb_chunks_per_rank_ull( target->rank, num_threads, loop );
		    int ret =  sctk_atomics_cas_int( &(target->for_dyn_remain[index].i), -1, for_dyn_total );
        }
        sctk_spinlock_unlock(  &( target->info.update_lock ) );
    //}
}

static inline void 
__mpcomp_loop_dyn_init_target_chunk( mpcomp_thread_t* thread, mpcomp_thread_t* target, unsigned int num_threads )
{
	/* Compute the index of the dynamic for construct */
	const int index = __mpcomp_loop_dyn_get_for_dyn_index( thread );

    //if( !sctk_spinlock_trylock( &( target->info.update_lock ) ) )
    //{
        sctk_spinlock_lock(  &( target->info.update_lock ) );
        /* Get the current id of remaining chunk for the target */
        int cur = sctk_atomics_load_int( &(target->for_dyn_remain[index].i) );

        if( cur < 0 && ( target == thread ) || ( ( __mpcomp_loop_dyn_get_for_dyn_current( thread )  > __mpcomp_loop_dyn_get_for_dyn_current( target ))) )
        {
            const int for_dyn_total = __mpcomp_get_static_nb_chunks_per_rank( target->rank, num_threads, &(thread->info.loop_infos.loop.mpcomp_long) );
            int ret = sctk_atomics_cas_int( &(target->for_dyn_remain[index].i), -1, for_dyn_total );
        }
        sctk_spinlock_unlock(  &( target->info.update_lock ) );
    //}
}

static inline int 
__mpcomp_loop_dyn_get_victim_rank( mpcomp_thread_t* thread )
{
    int i, target;

    sctk_assert( thread );
    sctk_assert( thread->instance );

    const int tree_depth = thread->instance->tree_depth;

    sctk_assert( thread->for_dyn_target );
    sctk_assert( thread->instance->tree_cumulative );

	for ( i = 0, target = 0 ; i < tree_depth ; i++ ) 
    {
		target += thread->for_dyn_target[i] * thread->instance->tree_cumulative[i] ;
	}

	sctk_assert( target >= 0 ) ;
	sctk_assert( target < thread->instance->nb_mvps ) ;
	
    return target;
}

static inline void 
__mpcomp_loop_dyn_target_reset( mpcomp_thread_t* thread ) 
{
    int i;
    sctk_assert( thread );

    sctk_assert( thread->instance );
    const int tree_depth = thread->instance->tree_depth;

    sctk_assert( thread->mvp );
    sctk_assert( thread->mvp->tree_rank );
    
    sctk_assert( thread->for_dyn_target );
    sctk_assert( thread->for_dyn_shift );

    /* Re-initialize the target to steal */
	for( i = 0 ; i < tree_depth ; i++ ) 
    {
	    thread->for_dyn_shift[i] = 0 ;
	    thread->for_dyn_target[i] = thread->mvp->tree_rank[i] ;
    }
}


#endif /* __MPCOMP_MPCOMP_LOOP_DYN_UTILS_H__ */

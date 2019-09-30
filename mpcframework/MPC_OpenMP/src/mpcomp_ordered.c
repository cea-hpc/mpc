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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpc_common_spinlock.h"
#include "mpcomp_ordered.h"

static inline void __mpcomp_internal_ordered_begin( mpcomp_thread_t *t, mpcomp_loop_gen_info_t* loop_infos )
{
    sctk_assert( loop_infos && loop_infos->type == MPCOMP_LOOP_TYPE_LONG );
    mpcomp_loop_long_iter_t* loop = &( loop_infos->loop.mpcomp_long );
    const long cur_ordered_iter = loop->cur_ordered_iter;

	/* First iteration of the loop -> initialize 'next_ordered_offset' */
	if( cur_ordered_iter == loop->lb ) 
    {
        while( OPA_cas_int( &( t->instance->team->next_ordered_offset_finalized ), 0, 1 ) )
            sctk_thread_yield();

        return;
    }

    /* Do we have to wait for the right iteration? */
    while( cur_ordered_iter != ( loop->lb + loop->incr * t->instance->team->next_ordered_offset ) )
	    sctk_thread_yield();
} 

static inline void __mpcomp_internal_ordered_begin_ull( mpcomp_thread_t *t, mpcomp_loop_gen_info_t* loop_infos )
{
    sctk_assert( loop_infos && loop_infos->type == MPCOMP_LOOP_TYPE_ULL );
    mpcomp_loop_ull_iter_t* loop = &( loop_infos->loop.mpcomp_ull );
    const unsigned long long cur_ordered_iter = loop->cur_ordered_iter;

    /* First iteration of the loop -> initialize 'next_ordered_offset' */
    if( cur_ordered_iter == loop->lb )                   
    {
        while( OPA_cas_int( &( t->instance->team->next_ordered_offset_finalized), 0, 1 ) )
            sctk_thread_yield();

        return;
    }

    /* Do we have to wait for the right iteration? */
    while( cur_ordered_iter != ( loop->lb + loop->incr * t->instance->team->next_ordered_offset_ull ) )
        sctk_thread_yield();
}

void __mpcomp_ordered_begin( void )
{
	mpcomp_thread_t *t;
    mpcomp_loop_gen_info_t* loop_infos; 

	__mpcomp_init();

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 )
         return ;

    loop_infos = &( t->info.loop_infos );

    if( loop_infos->type == MPCOMP_LOOP_TYPE_LONG )
    { 
//        fprintf(stderr, ":: %s :: Start ordered  %d\n", __func__, t->rank);
        __mpcomp_internal_ordered_begin( t, loop_infos );
    }
    else
    {
        __mpcomp_internal_ordered_begin_ull( t, loop_infos );
    }
}

static inline void __mpcomp_internal_ordered_end( mpcomp_thread_t* t, mpcomp_loop_gen_info_t* loop_infos  )
{
    int isLastIteration;

    sctk_assert( loop_infos && loop_infos->type == MPCOMP_LOOP_TYPE_LONG );
    mpcomp_loop_long_iter_t* loop = &( loop_infos->loop.mpcomp_long );

    isLastIteration = 0;
    isLastIteration += (loop->up  && loop->cur_ordered_iter >= loop->b) ? ( long) 1 : (long) 0;
    isLastIteration += (!loop->up && loop->cur_ordered_iter <= loop->b) ? ( long) 1 : (long) 0;

	loop->cur_ordered_iter += loop->incr;

    isLastIteration += ((loop->incr > 0)  && loop->cur_ordered_iter >= loop->b) ? ( long) 1 : (long) 0;
    isLastIteration += ((loop->incr < 0)  && loop->cur_ordered_iter <= loop->b) ? ( long) 1 : (long) 0;

    if( isLastIteration )
    {
	    t->instance->team->next_ordered_offset = (long) 0;
        OPA_cas_int(&(t->instance->team->next_ordered_offset_finalized), 1, 0 );
    }
    else
    {
        t->instance->team->next_ordered_offset += (long) 1;
    }
}

static inline void __mpcomp_internal_ordered_end_ull( mpcomp_thread_t* t, mpcomp_loop_gen_info_t* loop_infos  )
{
    int isLastIteration;
    sctk_assert( loop_infos && loop_infos->type == MPCOMP_LOOP_TYPE_ULL );
    mpcomp_loop_ull_iter_t* loop = &( loop_infos->loop.mpcomp_ull );

    unsigned long long cast_cur_ordered_iter = (unsigned long long) loop->cur_ordered_iter;

    isLastIteration = 0;
    isLastIteration += (loop->up && cast_cur_ordered_iter >= loop->b) ?  (unsigned long long) 1 : (unsigned long long) 0;
    isLastIteration += (!loop->up && cast_cur_ordered_iter <= loop->b) ? (unsigned long long) 1 : (unsigned long long) 0;

    if( loop->up )
    {
	    cast_cur_ordered_iter += loop->incr;
    }
    else
    {
	    cast_cur_ordered_iter -=  loop->incr;
    }

    loop->cur_ordered_iter = ( long ) cast_cur_ordered_iter; 

    isLastIteration += (loop->up && cast_cur_ordered_iter >= loop->b) ?  (unsigned long long) 1 : (unsigned long long) 0;
    isLastIteration += (!loop->up && cast_cur_ordered_iter <= loop->b) ? (unsigned long long) 1 : (unsigned long long) 0;

    if( isLastIteration )
    {
	    t->instance->team->next_ordered_offset_ull = (long) 0;
        OPA_cas_int(&(t->instance->team->next_ordered_offset_finalized), 1, 0 );
    }
    else
    {
        t->instance->team->next_ordered_offset_ull += (long) 1;
    }
}

void __mpcomp_ordered_end( void )
{
	mpcomp_thread_t *t;
    mpcomp_loop_gen_info_t* loop_infos; 

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 ) return ;

    loop_infos = &( t->info.loop_infos );

    if( loop_infos->type == MPCOMP_LOOP_TYPE_LONG )
    { 
        __mpcomp_internal_ordered_end( t, loop_infos );
    }
    else
    {
        __mpcomp_internal_ordered_end_ull( t, loop_infos );
    }
}

#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
    __asm__(".symver __mpcomp_ordered_begin, GOMP_ordered_start@@GOMP_1.0"); 
    __asm__(".symver __mpcomp_ordered_end, GOMP_ordered_end@@GOMP_1.0"); 
#endif /* OPTIMIZED_GOMP_API_SUPPORT */

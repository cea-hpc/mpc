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
#include "mpcomp_types.h"
#include "sctk_spinlock.h"
#include "mpcomp_ordered.h"

void __mpcomp_ordered_begin( void )
{
	mpcomp_thread_t *t;

	__mpcomp_init();

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 ) return ;

	sctk_nodebug( "[%d] %s: enter w/ iteration %d and team %d",
			t->rank, __func__, t->current_ordered_iteration, t->instance->team->next_ordered_offset ) ;

	/* First iteration of the loop -> initialize 'next_ordered_offset' */
	if (t->current_ordered_iteration == t->info.loop_lb) {
		t->instance->team->next_ordered_offset = 0;
	} else {
		/* Do we have to wait for the right iteration? */
		if (t->current_ordered_iteration != 
				(t->info.loop_lb + t->info.loop_incr * 
				 t->instance->team->next_ordered_offset)) {
			mpcomp_mvp_t *mvp;

			sctk_nodebug("[%d] %s: Waiting to schedule iteration %d",
					t->rank, __func__, t->current_ordered_iteration);

			/* Grab the corresponding microVP */
			mvp = t->mvp;
			while ( t->current_ordered_iteration != 
					(t->info.loop_lb + t->info.loop_incr * 
					 t->instance->team->next_ordered_offset) )
			{
				sctk_thread_yield();
			}
#if 0	       
			TODO("use correct primitives")
				mpcomp_fork_when_blocked (my_vp, info->step);

			/* Spin while the condition is not satisfied */
			mpcomp_macro_scheduler (my_vp, info->step);
			while ( info->current_ordered_iteration != 
					(info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
				mpcomp_macro_scheduler (my_vp, info->step);
				if ( info->current_ordered_iteration != 
						(info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
					sctk_thread_yield ();
				}
			}
#endif

		}
	}

	sctk_nodebug( "[%d] %s: Allowed to schedule iteration %d", t->rank, __func__, t->current_ordered_iteration ) ;
}


void __mpcomp_ordered_end( void )
{
	mpcomp_thread_t *t;

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 ) return ;

	t->current_ordered_iteration += t->info.loop_incr ;
	if ( (t->info.loop_incr > 0 && t->current_ordered_iteration >= t->info.loop_b) ||
			(t->info.loop_incr < 0 && t->current_ordered_iteration <= t->info.loop_b) ) {
		t->instance->team->next_ordered_offset = -1 ;
	} else {
		t->instance->team->next_ordered_offset++ ;
	}
}

#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
    __asm__(".symver __mpcomp_ordered_begin, GOMP_ordered_start@@GOMP_1.0"); 
    __asm__(".symver __mpcomp_ordered_end, GOMP_ordered_end@@GOMP_1.0"); 
#endif /* OPTIMIZED_GOMP_API_SUPPORT */

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
#include <mpcomp.h>
#include <mpcomp_abi.h>
#include "mpcomp_internal.h"
#include <sctk_debug.h>

TODO(runtime schedule: ICVs are not well transfered!)

/****
 *
 * LONG VERSION
 *
 *
 *****/
int
__mpcomp_runtime_loop_begin(
  long lb, long b, long incr, 
    long * from, long * to )
{
     mpcomp_thread_t *t ;	/* Info on the current thread */
     long chunk_size;

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     sctk_debug( "__mpcomp_runtime_loop_begin: value of schedule %d",
         t->info.icvs.run_sched_var ) ;

     chunk_size = t->info.icvs.modifier_sched_var;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_static_loop_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_dynamic_loop_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_guided_loop_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

     return 0 ;
}

int
__mpcomp_runtime_loop_next( long * from, long * to )
{
     mpcomp_thread_t *t ;	/* Info on the current thread */

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_static_loop_next( from, to ) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_dynamic_loop_next( from, to ) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_guided_loop_next( from, to ) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

     return 0 ;
}

void
__mpcomp_runtime_loop_end ()
{
     mpcomp_thread_t *t ;	/* Info on the current thread */

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_static_loop_end() ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_dynamic_loop_end() ;
         break ;
       case omp_sched_guided:
         return __mpcomp_guided_loop_end() ;
         break ;
       default:
	 not_reachable();
         break ;
     }
}

void
__mpcomp_runtime_loop_end_nowait ()
{
     mpcomp_thread_t *t ;	/* Info on the current thread */

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_static_loop_end_nowait() ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_dynamic_loop_end_nowait() ;
         break ;
       case omp_sched_guided:
         return __mpcomp_guided_loop_end_nowait() ;
         break ;
       default:
	 not_reachable();
         break ;
     }
}

/****
 *
 * ORDERED LONG VERSION 
 *
 *
 *****/

int
__mpcomp_ordered_runtime_loop_begin (long lb, long b, long incr,
				     long *from, long *to)
{
     mpcomp_thread_t *t ;	/* Info on the current thread */
     long chunk_size;

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     chunk_size = t->info.icvs.modifier_sched_var;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_ordered_static_loop_begin(lb,b,incr,chunk_size,from,to) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_ordered_dynamic_loop_begin(lb,b,incr,chunk_size,from,to) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_ordered_guided_loop_begin(lb,b,incr,chunk_size,from,to) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

	 return 0 ;
}

int
__mpcomp_ordered_runtime_loop_next(long *from, long *to)
{
     mpcomp_thread_t *t ;	/* Info on the current thread */

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_ordered_static_loop_next(from,to) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_ordered_dynamic_loop_next(from,to) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_ordered_guided_loop_next(from,to) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

	 return 0 ;
}

void
__mpcomp_ordered_runtime_loop_end()
{
     __mpcomp_runtime_loop_end();
}

void
__mpcomp_ordered_runtime_loop_end_nowait()
{
     __mpcomp_runtime_loop_end();
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
int
__mpcomp_loop_ull_runtime_begin(unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long * from, unsigned long long * to )
{
     mpcomp_thread_t *t ;	/* Info on the current thread */
     unsigned long long chunk_size;

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     sctk_debug( "__mpcomp_runtime_loop_begin: value of schedule %d",
         t->info.icvs.run_sched_var ) ;

     chunk_size = (unsigned long long)t->info.icvs.modifier_sched_var;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_loop_ull_static_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_loop_ull_dynamic_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_loop_ull_guided_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

     return 0 ;
}

int
__mpcomp_loop_ull_runtime_next( unsigned long long * from, unsigned long long * to )
{
     mpcomp_thread_t *t ;	/* Info on the current thread */

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_loop_ull_static_next( from, to ) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_loop_ull_dynamic_next( from, to ) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_loop_ull_guided_next( from, to ) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

     return 0 ;
}

/****
 *
 * ORDERED ULL VERSION
 *
 *
 *****/
int
__mpcomp_loop_ull_ordered_runtime_begin(unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long * from, unsigned long long * to )
{
     mpcomp_thread_t *t ;	/* Info on the current thread */
     unsigned long long chunk_size;

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     sctk_debug( "__mpcomp_runtime_loop_begin: value of schedule %d",
         t->info.icvs.run_sched_var ) ;

     chunk_size = (unsigned long long)t->info.icvs.modifier_sched_var;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_loop_ull_ordered_static_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_loop_ull_ordered_dynamic_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_loop_ull_ordered_guided_begin( lb, b, incr, chunk_size, from, to ) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

     return 0 ;
}

int
__mpcomp_loop_ull_ordered_runtime_next( unsigned long long * from, unsigned long long * to )
{
     mpcomp_thread_t *t ;	/* Info on the current thread */

     /* Handle orphaned directive (initialize OpenMP environment) */
     __mpcomp_init();

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
     sctk_assert( t != NULL ) ;

     switch( t->info.icvs.run_sched_var ) {
       case omp_sched_static:
         return __mpcomp_loop_ull_ordered_static_next( from, to ) ;
         break ;
       case omp_sched_dynamic:
         return __mpcomp_loop_ull_ordered_dynamic_next( from, to ) ;
         break ;
       case omp_sched_guided:
         return __mpcomp_loop_ull_ordered_guided_next( from, to ) ;
         break ;
       default:
	 not_reachable();
         break ;
     }

     return 0 ;
}




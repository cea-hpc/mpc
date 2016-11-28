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
#include "mpcomp.h"
#include "mpcomp_abi.h"
#include "sctk_debug.h"
#include "mpcomp_types.h"

/* 
   This file includes the function related to the 'guided' schedule of a shared
   for loop.
*/

/****
 *
 * LONG VERSION
 *
 *
 *****/
int mpcomp_guided_loop_begin (long lb, long b, long incr, long chunk_size, long *from, long *to)
{
    return mpcomp_dynamic_loop_begin( lb, b, incr, chunk_size, from, to);
}

int
mpcomp_guided_loop_next (long *from, long *to)
{
     return mpcomp_dynamic_loop_next(from, to);
}

void
mpcomp_guided_loop_end ()
{
     mpcomp_dynamic_loop_end();
}

void
mpcomp_guided_loop_end_nowait ()
{
     mpcomp_dynamic_loop_end_nowait();
}



/* Start a loop shared by the team w/ a guided schedule.
   !WARNING! This function assumes that there is no loops w/ guided schedule
   and nowait clause previously executed in the same parallel region 
*/
int
mpcomp_guided_loop_begin_ignore_nowait (long lb, long b, long incr, long
					  chunk_size, long *from, long *to)
{
     not_implemented();
     return 0;
}


int
mpcomp_guided_loop_next_ignore_nowait (long *from, long *to)
{
     return mpcomp_dynamic_loop_next_ignore_nowait(from, to);
}

/****
 *
 * ORDERED LONG VERSION 
 *
 *
 *****/

int
mpcomp_ordered_guided_loop_begin (long lb, long b, long incr, long chunk_size,
				    long *from, long *to)
{
     mpcomp_thread_t *t;
     int res;

     res = mpcomp_guided_loop_begin(lb, b, incr, chunk_size, from, to );

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert (t != NULL);

     t->current_ordered_iteration = *from;

     return res;
}

int
mpcomp_ordered_guided_loop_next (long *from, long *to) {
     mpcomp_thread_t *t;
     int res;

     res = mpcomp_guided_loop_next(from, to);

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert (t != NULL);

     t->current_ordered_iteration = *from;

     return res;
}

void
mpcomp_ordered_guided_loop_end ()
{
     mpcomp_guided_loop_end();
}

void
mpcomp_ordered_guided_loop_end_nowait ()
{
     mpcomp_guided_loop_end();
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
int mpcomp_loop_ull_guided_begin (bool up, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size,
                unsigned long long *from, unsigned long long *to)
{
     return mpcomp_loop_ull_dynamic_begin(up, lb, b, incr, chunk_size, from, to);
}

int mpcomp_loop_ull_guided_next (unsigned long long *from, unsigned long long *to)
{
     return mpcomp_loop_ull_dynamic_next(from, to);
}

void mpcomp_guided_loop_ull_end ()
{
	not_implemented() ;
	mpcomp_guided_loop_end() ;
}

void
mpcomp_guided_loop_ull_end_nowait ()
{
	not_implemented() ;
	mpcomp_guided_loop_end_nowait() ;
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int
mpcomp_loop_ull_ordered_guided_begin (unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size,
                    unsigned long long *from, unsigned long long *to)
{
     int res;
     mpcomp_thread_t *t;

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert (t != NULL);
    
     res = mpcomp_loop_ull_guided_begin(1, lb, b, incr, chunk_size, from, to );
     t->current_ordered_iteration = *from;

     return res;
}

int
mpcomp_loop_ull_ordered_guided_next (unsigned long long *from, unsigned long long *to) {
     mpcomp_thread_t *t;
     int res;

     res = mpcomp_loop_ull_guided_next(from, to);

     /* Grab the thread info */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert (t != NULL);

     t->current_ordered_iteration = *from;

     return res;
}

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

#ifndef __MPCOMP_SYNC_H__
#define __MPCOMP_SYNC_H__

#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "sctk_debug.h"

/***********
 * ATOMICS *
 ***********/

void __mpcomp_atomic_begin( void );
void __mpcomp_atomic_end( void );

/************
 * CRITICAL *
 ************/

void __mpcomp_anonymous_critical_begin( void );
void __mpcomp_anonymous_critical_end( void );
void __mpcomp_named_critical_begin( void ** );
void __mpcomp_named_critical_end( void ** );

/**********
 * SINGLE *
 **********/

int __mpcomp_do_single( void );
void *__mpcomp_do_single_copyprivate_begin( void );
void __mpcomp_do_single_copyprivate_end( void *data );
void __mpcomp_single_coherency_entering_parallel_region( void );
void __mpcomp_single_coherency_end_barrier( void );

/***********
 * BARRIER *
 ***********/

void __mpcomp_barrier( void );
void __mpcomp_internal_half_barrier( mpcomp_mvp_t *mvp );
void __mpcomp_internal_full_barrier( mpcomp_mvp_t *mvp );

/************
 * SECTIONS *
 ************/

void __mpcomp_sections_init(struct mpcomp_thread_s *, int);
int __mpcomp_sections_begin(int);
int __mpcomp_sections_next(void);
void __mpcomp_sections_end(void);
void __mpcomp_sections_end_nowait(void);
int __mpcomp_sections_coherency_exiting_paralel_region(void);
int __mpcomp_sections_coherency_barrier(void);

/*********
 * LOCKS *
 *********/

/* REGULAR LOCKS */
void omp_init_lock( omp_lock_t *lock );
void omp_destroy_lock( omp_lock_t *lock );
void omp_set_lock( omp_lock_t *lock );
void omp_unset_lock( omp_lock_t *lock );
int omp_test_lock( omp_lock_t *lock );

/* NESTED LOCKS */
void omp_init_nest_lock( omp_nest_lock_t *lock );
void omp_set_nest_lock( omp_nest_lock_t *lock );
void omp_unset_nest_lock( omp_nest_lock_t *lock );
int omp_test_nest_lock( omp_nest_lock_t *lock );

/* If the current task (thread if implicit task or explicit task)
*     is not the owner of the lock */
static inline int
mpcomp_nest_lock_test_task( mpcomp_thread_t *thread,
                            mpcomp_nest_lock_t *mpcomp_user_nest_lock )
{
#if MPCOMP_TASK
	struct mpcomp_task_s *current_task =
	    MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
	sctk_assert( current_task );
	const bool is_task_owner =
	    ( mpcomp_user_nest_lock->owner_task == current_task );
	const bool is_thread_owned =
	    ( mpcomp_user_nest_lock->owner_thread == ( void * )thread );
	const bool have_task_owner = ( mpcomp_user_nest_lock->owner_task != NULL );
	return !( is_task_owner && ( is_thread_owned || have_task_owner ) );
#endif /* MPCOMP_TASK */
}

#endif /*  __MPCOMP_SYNC_H__ */

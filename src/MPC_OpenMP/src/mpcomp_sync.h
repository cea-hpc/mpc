/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:06 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Augustin Serraz <augustin.serraz@exascale-computing.eu>            # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Romain Pereira <pereirar@ocre.cea.fr>                              # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_SYNC_H__
#define __MPC_OMP_SYNC_H__

#include "mpcomp_core.h"
#include "mpcomp_task.h"
#include "mpcomp_types.h"
#include "mpc_common_debug.h"

/***********
 * ATOMICS *
 ***********/

void mpc_omp_atomic_begin( void );
void mpc_omp_atomic_end( void );

/************
 * CRITICAL *
 ************/

void mpc_omp_anonymous_critical_begin( void );
void mpc_omp_anonymous_critical_end( void );
void mpc_omp_named_critical_begin( void ** );
void mpc_omp_named_critical_end( void ** );

/**********
 * SINGLE *
 **********/

int mpc_omp_do_single( void );
void *mpc_omp_do_single_copyprivate_begin( void );
void mpc_omp_do_single_copyprivate_end( void *data );
void _mpc_omp_single_coherency_entering_parallel_region( void );
void _mpc_omp_single_coherency_end_barrier( void );

/***********
 * BARRIER *
 ***********/

void mpc_omp_barrier( void );

/************
 * SECTIONS *
 ************/

void _mpc_omp_sections_init(struct mpc_omp_thread_s *, int);
int mpc_omp_sections_begin(int);
int mpc_omp_sections_next(void);
void mpc_omp_sections_end(void);
void mpc_omp_sections_end_nowait(void);
int _mpc_omp_sections_coherency_exiting_paralel_region(void);
int _mpc_omp_sections_coherency_barrier(void);

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
omp_nest_lock_test_task( mpc_omp_thread_t *thread,
                            omp_nest_lock_t *mpcomp_user_nest_lock )
{
	struct mpc_omp_task_s *current_task =
	    MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread );
	assert( current_task );
	const bool is_task_owner =
	    ( mpcomp_user_nest_lock->owner_task == current_task );
	const bool is_thread_owned =
	    ( mpcomp_user_nest_lock->owner_thread == ( void * )thread );
	const bool have_task_owner = ( mpcomp_user_nest_lock->owner_task != NULL );
	return !( is_task_owner && ( is_thread_owned || have_task_owner ) );
}

#endif /*  __MPC_OMP_SYNC_H__ */

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
#include "sctk_alloc.h"
#include "mpcomp_taskgroup.h"
#include "mpcomp_types.h"

#ifdef MPCOMP_TASKGROUP
void mpcomp_taskgroup_start( void )
{
    mpcomp_task_t* current_task = NULL;         		/* Current task execute 	*/
    mpcomp_thread_t* omp_thread_tls = NULL;     		/* thread private data  	*/
	mpcomp_task_taskgroup_t* new_taskgroup = NULL;	    /* new_taskgroup allocated  */

    __mpcomp_init();
  	mpcomp_task_scheduling_infos_init();

	omp_thread_tls = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
	sctk_assert( omp_thread_tls );
	current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
	sctk_assert( current_task );	
    
	new_taskgroup = sctk_malloc( sizeof( mpcomp_task_taskgroup_t ) );
	sctk_assert( new_taskgroup );

	/* Init new task group and store it in current task */
	memset( new_taskgroup, 0, sizeof( mpcomp_task_taskgroup_t ) ); 
	new_taskgroup->prev = current_task->taskgroup;
	current_task->taskgroup = new_taskgroup;
}

void mpcomp_taskgroup_end( void )
{
	mpcomp_task_t* current_task = NULL;             /* Current task execute     */
    mpcomp_thread_t* omp_thread_tls = NULL;         /* thread private data      */
    mpcomp_task_taskgroup_t* taskgroup = NULL;      /* new_taskgroup allocated  */

    omp_thread_tls = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
    sctk_assert( omp_thread_tls );

	current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
	sctk_assert( current_task );

    taskgroup = current_task->taskgroup;
    
    if( !taskgroup ) return;    

	while( sctk_atomics_load_int( &( taskgroup->children_num ) ))
	{
	    mpcomp_task_schedule( 1 );	
	} 

    current_task->taskgroup = taskgroup->prev;
    sctk_free( taskgroup );
}
#else /* MPCOMP_TASKGROUP */
void mpcomp_taskgroup_start( void )
{
}
void mpcomp_taskgroup_end( void )
{
}
#endif /* MPCOMP_TASKGROUP */

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
    __asm__(".symver mpcomp_taskgroup_start, GOMP_taskgroup_start@@GOMP_4.0"); 
    __asm__(".symver mpcomp_taskgroup_end, GOMP_taskgroup_end@@GOMP_4.0"); 
#endif /* OPTIMIZED_GOMP_API_SUPPORT */

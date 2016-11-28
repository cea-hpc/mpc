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

#ifndef __MPCOMP_TASGROUP_H__
#define __MPCOMP_TASGROUP_H__

#include "sctk_atomics.h"
#include "mpcomp_types.h"

/* Functions prototypes */
void mpcomp_taskgroup_start(void);
void mpcomp_taskgroup_end(void);

#ifdef MPCOMP_TASKGROUP
/* Inline functions */
static inline void 
mpcomp_taskgroup_add_task( mpcomp_task_t* new_task )
{
    mpcomp_task_t* current_task;
    mpcomp_thread_t* omp_thread_tls;
    mpcomp_task_taskgroup_t* taskgroup;

  	sctk_assert( sctk_openmp_thread_tls );	 

    omp_thread_tls = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
    taskgroup = current_task->taskgroup;
    if( taskgroup )
    {
        new_task->taskgroup = taskgroup;
        sctk_atomics_incr_int( &( taskgroup->children_num ) ); 
    }
}

static inline void 
mpcomp_taskgroup_del_task( mpcomp_task_t* task )
{
    if( task->taskgroup )
        sctk_atomics_decr_int( &( task->taskgroup->children_num ) );    
}

#else /* MPCOMP_TASKGROUP */

static inline void 
mpcomp_taskgroup_add_task( mpcomp_task_t* new_task )
{
}

static inline void 
mpcomp_taskgroup_del_task( mpcomp_task_t* task )
{
}

#endif /* MPCOMP_TASKGROUP */
#endif /* __MPCOMP_TASGROUP_H__ */

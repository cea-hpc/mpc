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

#ifndef __MPC_OMP_CORE_H__
#define __MPC_OMP_CORE_H__

#include "mpcomp_types.h" /* need mpc_omp_mvp_t && mpc_omp_instance_t */
#include <mpc_conf.h>
/**************
 * ALLOC HOOK *
 **************/

static inline void* mpc_omp_alloc( size_t size )
{
  return sctk_malloc(size);
}

static inline void mpc_omp_free( void *p )
{
    sctk_free(p);
}

/*************************
 * MPC_OMP CONFIGURATION *
 *************************/

typedef struct  mpc_omp_conf_s
{
	/* General OMP params */

	/* Maximum number of nested active parallel regions */
	int OMP_MAX_ACTIVE_LEVELS;
	/* Should we emit a warning when nesting is used? */
	int OMP_WARN_NESTED ;

	/* OMP places configuration */
	char places[MPC_CONF_STRING_SIZE];

	/* OpenMP allocator" */
	char allocator[MPC_CONF_STRING_SIZE];

	/* Schedule modifier */

	int OMP_MODIFIER_SCHEDULE ;

	/* Threads */


	/* Defaults number of threads */
	int OMP_NUM_THREADS ;
	/* Is nested parallelism handled or flaterned? */
	int OMP_NESTED ;
	/* Number of VPs for each OpenMP team */
	int OMP_MICROVP_NUMBER ;
	/* Is thread binding enabled or not */
	int OMP_PROC_BIND ;
	/* Size of the thread stack size */
	int OMP_STACKSIZE ;
	/* Behavior of waiting threads */
	int OMP_WAIT_POLICY ;
	/* Maximum number of threads participing in the whole program */
	int OMP_THREAD_LIMIT ;
	/* Is dynamic adaptation on? */
    int OMP_DYNAMIC ;

    /* Tasks */
    int maximum_tasks;
    int maximum_ready_tasks;
    int pqueue_new_depth;
    int pqueue_untied_depth;
    int task_recycler_capacity;
    int task_fiber_stack_size;
    int task_fiber_recycler_capacity;
    int queue_empty_if_full;
    int task_depth_threshold;
    int task_use_fiber;
    int task_trace;
    mpc_omp_task_yield_mode_t task_yield_mode;
    mpc_omp_task_priority_policy_t task_priority_policy;

    /* task steal */
    int task_steal_last_stolen;
    int task_steal_last_thief;
    char task_larceny_mode_str[MPC_CONF_STRING_SIZE];
    mpc_omp_task_larceny_mode_t task_larceny_mode;

	/* Tools */
	char omp_tool[MPC_CONF_STRING_SIZE];
	char omp_tool_libraries[MPC_CONF_STRING_SIZE];

}               mpc_omp_conf_t;


struct mpc_omp_conf_s * mpc_omp_conf_get(void);


/*******************************
 * INITIALIZATION AND FINALIZE *
 *******************************/

void mpc_omp_init(void);
void mpc_omp_exit(void);
void _mpc_omp_instance_init(mpc_omp_instance_t *, int, mpc_omp_team_t *);
void _mpc_omp_in_order_scheduler(mpc_omp_thread_t *);
void _mpc_omp_flush(void);

#endif /* __MPC_OMP_CORE_H__ */

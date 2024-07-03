/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:03 CEST 2021                                        # */
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
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_CORE_H__
#define __MPC_OMP_CORE_H__

#include "mpcomp_types.h" /* need mpc_omp_mvp_t && mpc_omp_instance_t */
#include <mpc_conf.h>
/**************
 * ALLOC HOOK *
 **************/

/* NOLINTBEGIN(clang-diagnostic-unused-function) */
static inline void* mpc_omp_alloc( size_t size )
{
  return sctk_malloc(size);
}

static inline void mpc_omp_free( void *p )
{
    sctk_free(p);
}
/* NOLINTEND(clang-diagnostic-unused-function) */

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

    /* Print (debug) the cores binding */
    int showbindings;

    /* Tasks */

    /* maximum number of tasks that can exist in the runtime */
    int maximum_tasks;

    /* maximum number of ready tasks that can exist in the runtime */
    int maximum_ready_tasks;

    /* task recycler capacity */
    int task_recycler_capacity;

    /* priority queues topological depth */
    int pqueue_new_depth;
    int pqueue_untied_depth;

#if MPC_OMP_TASK_COMPILE_LIST_TYPE
    /* task list policy (LIFO/FIFO) */
    int task_list_policy;
#endif

    /* task ucontext */
    int task_use_ucontext;
    int task_ucontext_stack_size;
    int task_ucontext_recycler_capacity;

    /* nested task depth threshold until they are serialized */
    int task_depth_threshold;

    /* if the tasks should be run sequentially by their producer thread */
    int task_sequential;

    /* if the tasks body function shall be skipped (for dry run) */
    int task_dry_run;

    /* task tracing */
    int task_trace;
    int task_trace_auto;
    int task_trace_mask;
    int task_trace_recycler_capacity;
    char task_trace_dir[MPC_CONF_STRING_SIZE];

    /* openmp thread condition wait, when there is no ready tasks */
    int task_cond_wait_enabled;
    int task_cond_wait_nhyperactive;

    /* taskyield */
    mpc_omp_task_yield_mode_t task_yield_mode;

    /* when using the 'fair' scheduler, minimum
     * time in s. before switching to another task */
    double task_yield_fair_min_time;

    /* task default hashing function */
    int task_dependency_default_hash;

    /* priorities */
    int task_direct_successor_enabled;
    mpc_omp_task_priority_policy_t task_priority_policy;
    mpc_omp_task_priority_propagation_policy_t task_priority_propagation_policy;
    mpc_omp_task_priority_propagation_synchronousity_t task_priority_propagation_synchronousity;

    /* task steal */
    int task_steal_last_stolen;
    int task_steal_last_thief;
    char task_larceny_mode_str[MPC_CONF_STRING_SIZE];
    mpc_omp_task_larceny_mode_t task_larceny_mode;

#if MPC_OMP_TASK_TRACE_USE_PAPI
    int task_trace_use_papi;
    char task_trace_papi_events[MPC_CONF_STRING_SIZE];
#endif /* MPC_OMP_TASK_TRACE_USE_PAPI */

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
void _mpc_omp_conf_init(void);
void _mpc_omp_instance_init(mpc_omp_instance_t *, int, mpc_omp_team_t *);
void _mpc_omp_in_order_scheduler(mpc_omp_thread_t *);
void _mpc_omp_flush(void);

#endif /* __MPC_OMP_CORE_H__ */

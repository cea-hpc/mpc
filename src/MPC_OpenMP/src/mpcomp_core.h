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

static inline void* mpc_omp_alloc( int size )
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

struct mpc_omp_conf
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

	int omp_new_task_depth;
    int omp_untied_task_depth;
	char omp_task_larceny_mode_str[MPC_CONF_STRING_SIZE];
	mpc_omp_task_larceny_mode_t omp_task_larceny_mode; 

	int omp_task_nesting_max;
	int mpcomp_task_max_delayed;
	int omp_task_use_lockfree_queue;
	int omp_task_steal_last_stolen_list;
	int omp_task_resteal_to_last_thief;

	/* Tools */
	char omp_tool[MPC_CONF_STRING_SIZE];
	char omp_tool_libraries[MPC_CONF_STRING_SIZE];
};


struct mpc_omp_conf * mpc_omp_conf_get(void);


/*******************************
 * INITIALIZATION AND FINALIZE *
 *******************************/

void mpc_omp_init(void);
void mpc_omp_exit(void);
void _mpc_omp_instance_init(mpc_omp_instance_t *, int, mpc_omp_team_t *);
void _mpc_omp_in_order_scheduler(mpc_omp_thread_t *);
void _mpc_omp_flush(void);

#endif /* __MPC_OMP_CORE_H__ */

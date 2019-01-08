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

#ifndef __MPCOMP_TYPES_ICV_H__
#define __MPCOMP_TYPES_ICV_H__

#include "sctk_debug.h"
#include "mpcomp.h"

/* Global Internal Control Variables
 * One structure per OpenMP instance */
typedef struct mpcomp_global_icv_s {
  omp_sched_t def_sched_var;  /**< Default schedule when no 'schedule' clause is
                                 present 			*/
  int bind_var;               /**< Is the OpenMP threads bound to cpu cores
                                 */
  int stacksize_var;          /**< Size of the stack per thread (in Bytes)
                                 */
  int active_wait_policy_var; /**< Is the threads wait policy active or passive
                                 */
  int thread_limit_var; /**< Number of Open threads to use for the whole program
                           */
  int max_active_levels_var; /**< Maximum number of nested active parallel
                                regions 				*/
  int nmicrovps_var;         /**< Number of VPs
                                */
  int warn_nested;           /**< Emit warning for nested parallelism?
                                */
  int affinity;              /**< OpenMP threads affinity
                                */
} mpcomp_global_icv_t;

/** Local Internal Control Variables
 * One structure per OpenMP thread 				*/
typedef struct mpcomp_local_icv_s {
  unsigned int nthreads_var;          /**< Number of threads for the next team creation
                                */
  int dyn_var;               /**< Is dynamic thread adjustement on?
                                */
  int nest_var;              /**< Is nested OpenMP handled/allowed?
                                */
  omp_sched_t run_sched_var; /**< Schedule to use when a 'schedule' clause is
                                set to 'runtime' */
  int modifier_sched_var;    /**< Size of chunks for loop schedule
                                */
  int active_levels_var;     /**< Number of nested, active enclosing parallel
                                regions 			*/
  int levels_var;            /**< Number of nested enclosing parallel regions
                                */
} mpcomp_local_icv_t;

static inline void 
__mpcomp_apply_icvs_global_to_local( __UNUSED__ mpcomp_global_icv_t* global, __UNUSED__ mpcomp_local_icv_t* local )
{
}

#endif /* __MPCOMP_TYPES_ICV_H__ */

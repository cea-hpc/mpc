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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include <sys/time.h>
#include "mpc_common_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_task.h"
#include "mpcomp_openmp_tls.h"
#include <stdbool.h>
#include "mpcompt_control_tool.h"
#include "mpcompt_frame.h"
void omp_set_num_threads(int num_threads) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d-%p] %s new num threads: %d", t->rank, t, __func__,
               num_threads);
  t->info.icvs.nthreads_var = num_threads;
}

/* ICC renames set_num_threads to ompc_ so provide it too
 * https://software.intel.com/en-us/forums/intel-c-compiler/topic/798675
 */
void ompc_set_num_threads(int num_threads) {
    omp_set_num_threads(num_threads);
}

/*
 * Return the thread number (ID) of the current thread whitin its team.
 * The master thread of each team has a thread ID equals to 0.
 * This function returns 0 if called from a serial part of the code (or from a
 * flatterned nested parallel region).
 * See OpenMP API 2.5 Section 3.2.4
 */
int omp_get_thread_num(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s thread id: ", t->rank, __func__);
  return t->rank;
}

/*
 * Return the maximum number of threads that can be used inside a parallel region.
 * This function may be called either from serial or parallel parts of the program.
 * See OpenMP API 2.5 Section 3.2.3
 */
int omp_get_max_threads(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s max num threads : %d", t->rank, __func__,
               t->info.icvs.nthreads_var);
  return t->info.icvs.nthreads_var;
}

/*
 * Return the maximum number of processors.
 * See OpenMP API 1.0 Section 3.2.5
 */
int omp_get_num_procs(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  return mpcomp_global_icvs.nmicrovps_var;
}


/**
  * Set or unset the dynamic adaptation of the thread number.
  * See OpenMP API 2.5 Section 3.1.7
  */
void omp_set_dynamic(int dynamic_threads) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  t->info.icvs.dyn_var = dynamic_threads;
}


/**
  * Retrieve the current dynamic adaptation of the program.
  * See OpenMP API 2.5 Section 3.2.8
  */
int omp_get_dynamic(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  return t->info.icvs.dyn_var;
}

/**
  *
  * See OpenMP API 2.5 Section 3.2.9
  */
void omp_set_nested(int nested) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  /* no mpcomp nested support */
  t->info.icvs.nest_var = nested;
}

/**
  *
  * See OpenMP API 2.5 Section 3.2.10
  */
int omp_get_nested(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  return t->info.icvs.nest_var;
}


/**
  *
  * See OpenMP API 3.0 Section 3.2.11
  */
void omp_set_schedule(omp_sched_t kind, int modifier) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  t->info.icvs.run_sched_var = kind;
  t->info.icvs.modifier_sched_var = modifier;
}

/**
  *
  * See OpenMP API 3.0 Section 3.2.12
  */
void omp_get_schedule(omp_sched_t *kind, int *modifier) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  assert(kind && modifier);
  *kind = t->info.icvs.run_sched_var;
  *modifier = t->info.icvs.modifier_sched_var;
}


/*
 * Check whether the current flow is located inside a parallel region or not.
 * See OpenMP API 2.5 Section 3.2.6
 */
int omp_in_parallel(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s enter ...", t->rank, __func__);
  return (t->instance->team->depth != 0);
}

/*
 * OpenMP 3.0. Returns the nesting level for the parallel block, 
 * which enclose the calling call.
 */
int omp_get_level(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s level: %d", t->rank, __func__, t->info.icvs.levels_var);
  return t->info.icvs.levels_var;
}

/*
 * OpenMP 3.0. Returns the nesting level for the active parallel block, 
 * which enclose the calling call.
 */
int omp_get_active_level(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s level: %d", t->rank, __func__,
               t->info.icvs.active_levels_var);
  return t->info.icvs.active_levels_var;
}

/*
 * OpenMP 3.0. This function returns the thread identification number 
 * for the given nesting level of the current thread.
 * For values of level outside zero to omp_get_level -1 is returned. 
 * if level is omp_get_level the result is identical to omp_get_thread_num
 */
int omp_get_ancestor_thread_num(int level) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  if( !level ) return 0; 
  mpc_omp_thread_t *t = mpc_omp_tree_array_ancestor_get_thread_tls(level);
  return (t == NULL) ? -1 : t->rank;
}

/*
 * OpenMP 3.0. This function returns the number of threads in a 
 * thread team to which either the current thread or its ancestor belongs.
 * For values of level outside zero to omp_get_level, -1 is returned.
 * if level is zero, 1 is returned, and for omp_get_level, the result is 
 * identical to omp_get_num_threads.
 */
int omp_get_team_size(int level) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  if( !level ) return 1;
  mpc_omp_thread_t *t = mpc_omp_tree_array_ancestor_get_thread_tls(level);
  return (t == NULL) ? -1 : t->info.num_threads;
}

/*
 * Return the number of threads used in the current team (direct enclosing
 * parallel region).
 * If called from a sequential part of the program, this function returns 1.
 * (See OpenMP API 2.5 Section 3.2.2)
 */
int omp_get_num_threads(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_common_nodebug("[%d] %s entering ...", t->rank, __func__);
  return t->info.num_threads;
}

/* timing routines */
double omp_get_wtime(void) {
  double res;
  struct timeval tp;

  gettimeofday (&tp, NULL);
  res = tp.tv_sec + tp.tv_usec * 0.000001;

  mpc_common_nodebug("%s Wtime = %f", __func__, res);
  return res;
}

double omp_get_wtick(void) { return (double)10e-6; }

/**
 * The omp_get_thread_limit routine returns the maximum number of OpenMP
 * threads available to the program.
 */
int omp_get_thread_limit() {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  return mpcomp_global_icvs.thread_limit_var;
}

/*
 * The omp_set_max_active_levels routine limits the number of nested 
 * active parallel regions, by setting the max-active-levels-var ICV.
 */
void omp_set_max_active_levels(__UNUSED__ int max_levels) {
  /*
* According current implementation of nested parallelism
* This is equivalent of having only one active parallel region
* No operation is performed then
* */
}

/*
 * The omp_get_max_active_levels routine returns the value of the 
 * max-active-levels-var ICV, which determines the maximum number of 
 * nested active parallel regions.
 */
int omp_get_max_active_levels() {
  /*
   *  According current implementation of nested parallelism
   *  This is equivalent of having only one active parallel region
   *  */

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_omp_init();
  return mpcomp_global_icvs.max_active_levels_var;
}

/*
 * The omp_in_final routine returns true if the routine is executed 
 * in a final task region; otherwise, it returns false.
 */
int omp_in_final(void) {
  mpc_omp_thread_t *thread = mpc_omp_get_thread_tls();
  mpc_omp_task_t *task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
  return (task &&
          mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_FINAL));
}

/*
 * The omp_control_tool routine allows to interact with an active tool.
 */
int omp_control_tool(int command, int modifier, void * arg) {
  omp_control_tool_result_t result = omp_control_tool_notool;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */
#if OMPT_SUPPORT

  result = _mpc_omp_ompt_callback_control_tool( command, modifier, arg );

#endif /* OMPT_SUPPORT */
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

  return result;
}

/* For backward compatibility with old patched GCC */
int mpc_omp_get_num_threads(void) { return omp_get_num_threads(); }

int mpc_omp_get_thread_num(void) { return omp_get_thread_num(); }

/**
 *  * The omp fulfill event routine
 *   * Warning: this does not respect the standard
 *    *  `omp_fulfill_event(omp_event_handle_t handle)`
 *     */
void
mpc_omp_fulfill_event(mpc_omp_event_handle_t * handle)
{
    if (handle->type & MPC_OMP_EVENT_TASK_BLOCK) mpc_omp_task_unblock(handle);
}

int
mpc_omp_in_explicit_task(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    if (!thread)
    {
        return 0;
    }
    mpc_omp_task_t * task = thread->task_infos.current_task;
    if (!task)
    {
        return 0;
    }
    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_IMPLICIT)
            || mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_INITIAL))
    {
        return 0;
    }
    return 1;
}


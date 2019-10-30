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
#include "mpcomp_alloc.h"
#include "mpcomp_types.h"
#include "mpcomp_task.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_tasking.h"
#include "mpcomp_taskgroup.h"

#include "mpcomp_core.h"

#include <time.h>

#define START_TIMER\
	struct timespec rss;\
clock_gettime(CLOCK_MONOTONIC, &rss);

#define END_TIMER\
	struct timespec rse;\
clock_gettime(CLOCK_MONOTONIC, &rse);\
double wtime = rse.tv_sec - rss.tv_sec + (rse.tv_nsec - rss.tv_nsec) / 1000000000.;
int cpt_iter = 0;
double mean_time=0;

#define TIMER wtime

#if MPCOMP_TASK
kmp_int32 __kmpc_omp_task(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                          kmp_task_t *new_task) {
  struct mpcomp_task_s *mpcomp_task =
      (struct mpcomp_task_s * ) ( (char *)new_task - sizeof(struct mpcomp_task_s));
  // TODO: Handle final clause
  __mpcomp_task_process(mpcomp_task, true);
  return (kmp_int32)0;
}

void __kmp_omp_task_wrapper(void *task_ptr) {
  kmp_task_t *kmp_task_ptr = (kmp_task_t *)task_ptr;
  kmp_task_ptr->routine(0, kmp_task_ptr);
}

kmp_task_t *__kmpc_omp_task_alloc(__UNUSED__ ident_t *loc_ref,__UNUSED__  kmp_int32 gtid,
                                  kmp_int32 flags, size_t sizeof_kmp_task_t,
                                  size_t sizeof_shareds,
                                  kmp_routine_entry_t task_entry) {
  sctk_assert(sctk_openmp_thread_tls);
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  __mpcomp_init();

  /* default pading */
  const long align_size = sizeof(void *);

  // mpcomp_task + arg_size
  const long mpcomp_kmp_taskdata = sizeof(mpcomp_task_t) + sizeof_kmp_task_t;
  const long mpcomp_task_info_size =
      mpcomp_task_align_single_malloc(mpcomp_kmp_taskdata, align_size);
  const long mpcomp_task_data_size =
      mpcomp_task_align_single_malloc(sizeof_shareds, align_size);

  /* Compute task total size */
  long mpcomp_task_tot_size = mpcomp_task_info_size;

  sctk_assert(MPCOMP_OVERFLOW_SANITY_CHECK((unsigned long)mpcomp_task_tot_size,
                                           (unsigned long)mpcomp_task_data_size));
  mpcomp_task_tot_size += mpcomp_task_data_size;

  struct mpcomp_task_s *new_task ;
  /* Reuse another task if we can to avoid alloc overhead */
	if(t->task_infos.one_list_per_thread)
	{
		if(mpcomp_task_tot_size > t->task_infos.max_task_tot_size)
    /* Tasks stored too small, free them */
		{
			t->task_infos.max_task_tot_size = mpcomp_task_tot_size;
			int i;
			for(i=0;i<t->task_infos.nb_reusable_tasks;i++) 
      {
				sctk_free(t->task_infos.reusable_tasks[i]);
				t->task_infos.reusable_tasks[i] = NULL;
      }
			t->task_infos.nb_reusable_tasks = 0;
		}

		if(t->task_infos.nb_reusable_tasks == 0) 
		{
			new_task = (struct mpcomp_task_s*) mpcomp_alloc( t->task_infos.max_task_tot_size);
		}
    /* Reuse last task stored */
		else
		{
	  	sctk_assert(t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks-1]);
			new_task = (mpcomp_task_t*) t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks -1];
			t->task_infos.nb_reusable_tasks --;
		}
	}
	else
	{
		new_task = (struct mpcomp_task_s*) mpcomp_alloc( mpcomp_task_tot_size );
	}

  sctk_assert(new_task != NULL);

  void *task_data = (sizeof_shareds > 0)
                        ? (void *)((uintptr_t)new_task + mpcomp_task_info_size)
                        : NULL;
  /* Create new task */
  kmp_task_t *compiler_infos =
      (kmp_task_t *)((char *)new_task + sizeof(struct mpcomp_task_s));
  __mpcomp_task_infos_init(new_task, __kmp_omp_task_wrapper, compiler_infos, t);

 /* MPCOMP_USE_TASKDEP */

  compiler_infos->shareds = task_data;
  compiler_infos->routine = task_entry;
  compiler_infos->part_id = 0;

  /* taskgroup */
  mpcomp_task_t *current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t);
  mpcomp_taskgroup_add_task(new_task);
  mpcomp_task_ref_parent_task(new_task);
  new_task->is_stealed = false;
  new_task->task_size = t->task_infos.max_task_tot_size;
  if(new_task->depth % MPCOMP_TASKS_DEPTH_JUMP == 2) new_task->far_ancestor = new_task->parent;
  else new_task->far_ancestor = new_task->parent->far_ancestor;
  t->task_infos.sizeof_kmp_task_t = sizeof_kmp_task_t;


  /* If its parent task is final, the new task must be final too */
  if (mpcomp_task_is_final(flags, new_task->parent)) {
      mpcomp_task_set_property(&(new_task->property), MPCOMP_TASK_FINAL);
  }

  /* to handle if0 with deps */
  current_task->last_task_alloc = new_task;

  return compiler_infos;
}

void __kmpc_omp_task_begin_if0(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                               kmp_task_t *task) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  struct mpcomp_task_s *mpcomp_task = (struct mpcomp_task_s *) (
      (char *)task - sizeof(struct mpcomp_task_s));
  sctk_assert(t);
  mpcomp_task->icvs = t->info.icvs;
  mpcomp_task->prev_icvs = t->info.icvs;
  /* Because task code is between the current call and the next call 
   * __kmpc_omp_task_complete_if0 */
  MPCOMP_TASK_THREAD_SET_CURRENT_TASK(t, mpcomp_task);
}

void __kmpc_omp_task_complete_if0(__UNUSED__ ident_t *loc_ref,__UNUSED__  kmp_int32 gtid,
                                  kmp_task_t *task) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  struct mpcomp_task_s *mpcomp_task = (struct mpcomp_task_s *)
      ((char *)task - sizeof(struct mpcomp_task_s));
  sctk_assert(t);
  __mpcomp_task_finalize_deps(mpcomp_task); /* if task if0 with deps */
  mpcomp_taskgroup_del_task(mpcomp_task);
  mpcomp_task_unref_parent_task(mpcomp_task);
  MPCOMP_TASK_THREAD_SET_CURRENT_TASK(t, mpcomp_task->parent);
  t->info.icvs = mpcomp_task->prev_icvs;
}

kmp_int32 __kmpc_omp_task_parts(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                kmp_task_t *new_task) {
  // TODO: Check if this is the correct way to implement __kmpc_omp_task_parts
  struct mpcomp_task_s *mpcomp_task = (struct mpcomp_task_s *)
      ((char *)new_task - sizeof(struct mpcomp_task_s));
  __mpcomp_task_process(mpcomp_task, true);
  return (kmp_int32)0;
}

kmp_int32 __kmpc_omp_taskwait(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid) {
  mpcomp_taskwait();
  return (kmp_int32)0;
}

kmp_int32 __kmpc_omp_taskyield(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, __UNUSED__ int end_part) {
  //not_implemented();
  return (kmp_int32)0;
}

/* Following 2 functions should be enclosed by "if TASK_UNUSED" */

void __kmpc_omp_task_begin(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, __UNUSED__ kmp_task_t *task) {
  //not_implemented();
}

void __kmpc_omp_task_complete(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                              __UNUSED__ kmp_task_t *task) {
  //not_implemented();
}

/* FOR OPENMP 4 */

void __kmpc_taskgroup(__UNUSED__ ident_t *loc, __UNUSED__ int gtid) { mpcomp_taskgroup_start(); }

void __kmpc_end_taskgroup(__UNUSED__ ident_t *loc, __UNUSED__ int gtid) { mpcomp_taskgroup_end(); }

static void mpcomp_intel_translate_taskdep_to_gomp(  kmp_int32 ndeps, kmp_depend_info_t *dep_list, kmp_int32 ndeps_noalias, __UNUSED__ kmp_depend_info_t *noalias_dep_list, void** gomp_list_deps)
{
    int i;

    long long int num_out_dep = 0;
    int num_in_dep = 0;
    long long int nd= (ndeps + ndeps_noalias);
    gomp_list_deps[0] = (void *)(uintptr_t)nd;
    uintptr_t dep_not_out[nd];

    for ( i = 0; i < nd ; i++ )
    {
        if ( dep_list[i].base_addr != 0)
        {
            if(dep_list[i].flags.out){
                long long int ibaseaddr = dep_list[i].base_addr;
                gomp_list_deps[num_out_dep+2] = (void *)ibaseaddr;
                num_out_dep++;
            }
            else{
                dep_not_out[num_in_dep] = (uintptr_t)dep_list[i].base_addr;
                num_in_dep++;
            }
        }
    }
    gomp_list_deps[1] = (void *)num_out_dep;
    for ( i = 0; i < num_in_dep; i++ ){
        gomp_list_deps[2 + num_out_dep + i] = (void *)dep_not_out[i];
    }
    return;
}

kmp_int32 __kmpc_omp_task_with_deps(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                    kmp_task_t *new_task, kmp_int32 ndeps,
                                    kmp_depend_info_t *dep_list,
                                    kmp_int32 ndeps_noalias,
                                    kmp_depend_info_t *noalias_dep_list) {
  struct mpcomp_task_s *mpcomp_task =
      (struct mpcomp_task_s * ) ( (char *)new_task - sizeof(struct mpcomp_task_s));
  void *data = (void *)mpcomp_task->data;
  long arg_size = 0;
  long arg_align = 0;
  bool if_clause = true; /* if0 task doesn't go here */
  unsigned flags = 8; /* dep flags */
  void ** depend;
  depend = (void**)sctk_malloc(sizeof(uintptr_t) * ((int)(ndeps + ndeps_noalias)+2));
  mpcomp_intel_translate_taskdep_to_gomp(ndeps, dep_list, ndeps_noalias, noalias_dep_list, depend);
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
  mpcomp_task_with_deps(mpcomp_task->func, data, NULL, arg_size, arg_align,
          if_clause, flags, depend, true, mpcomp_task);
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
  return (kmp_int32)0;
}

void __kmpc_omp_wait_deps(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, kmp_int32 ndeps,
                          kmp_depend_info_t *dep_list, kmp_int32 ndeps_noalias,
                          kmp_depend_info_t *noalias_dep_list) {
    mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    mpcomp_task_t * current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t);
    mpcomp_task_t * task = current_task->last_task_alloc;
    long arg_size = 0; 
    long arg_align = 0;
    bool if_clause = false; /* if0 task here */
    unsigned flags = 8; /* dep flags */
    void ** depend = (void**)sctk_malloc(sizeof(uintptr_t) * ((int)(ndeps + ndeps_noalias)+2));
    mpcomp_intel_translate_taskdep_to_gomp(ndeps, dep_list, ndeps_noalias, noalias_dep_list, depend);
    sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
    mpcomp_task_with_deps(NULL, NULL, NULL, arg_size, arg_align,
    if_clause, flags, depend, true, task); /* create the dep node and set the task to the node then return */
    /* next call should be __kmpc_omp_task_begin_if0 to execute undeferred if0 task */
    sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
}

void __kmp_release_deps(__UNUSED__ kmp_int32 gtid, __UNUSED__ kmp_taskdata_t *task) {
  //not_implemented();
}

typedef void(*p_task_dup_t)(kmp_task_t *, kmp_task_t *, kmp_int32);
void __kmpc_taskloop(ident_t *loc, int gtid, kmp_task_t *task, __UNUSED__ int if_val,
                kmp_uint64 *lb, kmp_uint64 *ub, kmp_int64 st,
                int nogroup, int sched, kmp_uint64 grainsize, void *task_dup )
{
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  unsigned long i, trip_count, num_tasks = 0, extras = 0, lower = *lb, upper = *ub;
  kmp_task_t * next_task;
  int lastpriv;
  kmp_taskdata_t *taskdata_src, *taskdata;
  size_t lower_offset = (char*)lb - (char*)task;
  size_t upper_offset = (char*)ub - (char*)task;
  size_t shareds_offset,sizeof_kmp_task_t,sizeof_shareds;
  p_task_dup_t ptask_dup = (p_task_dup_t)task_dup;

if(nogroup == 0) {
    mpcomp_taskgroup_start();
  }
  if ( st == 1 ) {   // most common case
       trip_count = upper - lower + 1;
   } else if ( st < 0 ) {
     trip_count = (lower - upper) / (-st) + 1;
   } else {       // st > 0
     trip_count = (upper - lower) / st + 1;
   }

  if (!(trip_count)) {
    return;
  }

  switch(sched) {
    case 0:
      grainsize = t->info.num_threads * 10;
    case 2:
      if(grainsize > trip_count) {
        num_tasks = trip_count; 
        grainsize = 1;
        extras = 0;
      }
      else {
        num_tasks = grainsize;
        grainsize = trip_count/num_tasks;
        extras = trip_count % num_tasks;
      }
      break;
    case 1:
      if(grainsize > trip_count) {
        num_tasks = 1; 
        grainsize = trip_count;
        extras = 0;
      }
      else {
        num_tasks = trip_count/grainsize;
        grainsize = trip_count/num_tasks;
        extras = trip_count % num_tasks;
      }
      break;
    default:
      sctk_nodebug("Unknown scheduling of taskloop");
  }

    unsigned long chunk;
    if(extras == 0) {
      chunk = grainsize -1;
    }
    else {
      chunk = grainsize;
      --extras;
    }
    sizeof_shareds = sizeof(kmp_task_t);
    sizeof_kmp_task_t = t->task_infos.sizeof_kmp_task_t;
    taskdata_src = ((kmp_taskdata_t *) task) -1;
    shareds_offset = (char*)task->shareds - (char*)taskdata_src;
    taskdata = sctk_malloc(sizeof_shareds + shareds_offset);
    memcpy(taskdata, taskdata_src, sizeof_shareds + shareds_offset);
  for(i=0;i<num_tasks;++i)
  {
    upper = lower + st * chunk;
    if(i == num_tasks -1) {
      lastpriv = 1;
    }
    next_task = __kmpc_omp_task_alloc(loc, gtid,1,
        sizeof_kmp_task_t,
        sizeof_shareds,task->routine);

    if( task->shareds != NULL ) { // need setup shareds pointer
      next_task->shareds = &((char*)taskdata)[shareds_offset];
    }    

    if(ptask_dup !=NULL) ptask_dup(next_task,task,lastpriv);

    *(kmp_uint64*)((char*)next_task + lower_offset) = lower; // adjust task-specific bounds
    *(kmp_uint64*)((char*)next_task + upper_offset) = upper;
    struct mpcomp_task_s *mpcomp_task =
      (struct mpcomp_task_s * ) ( (char *)next_task - sizeof(struct mpcomp_task_s));
    __mpcomp_task_process(mpcomp_task, true);
    lower = upper + st;
  }
  // Free pattern task without executing it


  struct mpcomp_task_s *mpcomp_taskloop_task =
    (struct mpcomp_task_s * ) ( (char *)task - sizeof(struct mpcomp_task_s));

  mpcomp_taskgroup_del_task(mpcomp_taskloop_task);

  mpcomp_task_unref_parent_task(mpcomp_taskloop_task);

  if(nogroup == 0) {
    mpcomp_taskgroup_end();
  }
}
#endif

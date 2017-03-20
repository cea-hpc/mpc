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

#ifndef __MPCOMP_INTEL_TASKING_H__
#define __MPCOMP_INTEL_TASKING_H__

#include "mpcomp_intel_types.h"

#define KMP_TASKDATA_TO_TASK(task_data) ((mpcomp_task_t *)taskdata - 1)
#define KMP_TASK_TO_TASKDATA(task) ((kmp_task_t *)taskdata + 1)

typedef kmp_int32 (*kmp_routine_entry_t)(kmp_int32, void *);

typedef struct kmp_task { /* GEH: Shouldn't this be aligned somehow? */
  void *shareds;          /**< pointer to block of pointers to shared vars   */
  kmp_routine_entry_t
      routine;       /**< pointer to routine to call for executing task */
  kmp_int32 part_id; /**< part id for the task                          */
#if OMP_40_ENABLED
  kmp_routine_entry_t
      destructors; /* pointer to function to invoke deconstructors of
                      firstprivate C++ objects */
#endif             // OMP_40_ENABLED
                   /*  private vars  */
} kmp_task_t;

/* From kmp_os.h for this type */
typedef long kmp_intptr_t;

typedef struct kmp_depend_info {
  kmp_intptr_t base_addr;
  size_t len;
  struct {
    bool in : 1;
    bool out : 1;
  } flags;
} kmp_depend_info_t;

struct kmp_taskdata { /* aligned during dynamic allocation       */
#if 0
    kmp_int32               td_task_id;               /* id, assigned by debugger                */
    kmp_tasking_flags_t     td_flags;                 /* task flags                              */
    kmp_team_t *            td_team;                  /* team for this task                      */
    kmp_info_p *            td_alloc_thread;          /* thread that allocated data structures   */
                                                      /* Currently not used except for perhaps IDB */
    kmp_taskdata_t *        td_parent;                /* parent task                             */
    kmp_int32               td_level;                 /* task nesting level                      */
    ident_t *               td_ident;                 /* task identifier                         */
                            // Taskwait data.
    ident_t *               td_taskwait_ident;
    kmp_uint32              td_taskwait_counter;
    kmp_int32               td_taskwait_thread;       /* gtid + 1 of thread encountered taskwait */
    kmp_internal_control_t  td_icvs;                  /* Internal control variables for the task */
    volatile kmp_uint32     td_allocated_child_tasks;  /* Child tasks (+ current task) not yet deallocated */
    volatile kmp_uint32     td_incomplete_child_tasks; /* Child tasks not yet complete */
#if OMP_40_ENABLED
    kmp_taskgroup_t *       td_taskgroup;         // Each task keeps pointer to its current taskgroup
    kmp_dephash_t *         td_dephash;           // Dependencies for children tasks are tracked from here
    kmp_depnode_t *         td_depnode;           // Pointer to graph node if this task has dependencies
#endif
#if KMP_HAVE_QUAD
    _Quad                   td_dummy;             // Align structure 16-byte size since allocated just before kmp_task_t
#else
    kmp_uint32              td_dummy[2];
#endif
#endif
};
typedef struct kmp_taskdata kmp_taskdata_t;

kmp_int32 __kmpc_omp_task(ident_t *, kmp_int32, kmp_task_t *);
void __kmp_omp_task_wrapper(void *);
kmp_task_t *__kmpc_omp_task_alloc(ident_t *, kmp_int32, kmp_int32, size_t,
                                  size_t, kmp_routine_entry_t);
void __kmpc_omp_task_begin_if0(ident_t *, kmp_int32, kmp_task_t *);
void __kmpc_omp_task_complete_if0(ident_t *, kmp_int32, kmp_task_t *);
kmp_int32 __kmpc_omp_task_parts(ident_t *, kmp_int32, kmp_task_t *);
kmp_int32 __kmpc_omp_taskwait(ident_t *loc_ref, kmp_int32 gtid);
kmp_int32 __kmpc_omp_taskwait(ident_t *loc_ref, kmp_int32 gtid);
kmp_int32 __kmpc_omp_taskyield(ident_t *loc_ref, kmp_int32 gtid, int end_part);
void __kmpc_omp_task_begin(ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task);
void __kmpc_omp_task_complete(ident_t *loc_ref, kmp_int32 gtid,
                              kmp_task_t *task);
void __kmpc_taskgroup(ident_t *loc, int gtid);
void __kmpc_end_taskgroup(ident_t *loc, int gtid);
kmp_int32 __kmpc_omp_task_with_deps(ident_t *, kmp_int32, kmp_task_t *,
                                    kmp_int32, kmp_depend_info_t *, kmp_int32,
                                    kmp_depend_info_t *);
void __kmpc_omp_wait_deps(ident_t *, kmp_int32, kmp_int32, kmp_depend_info_t *,
                          kmp_int32, kmp_depend_info_t *);
void __kmp_release_deps(kmp_int32, kmp_taskdata_t *);

#endif /*__MPCOMP_INTEL_TASKING_H__ */

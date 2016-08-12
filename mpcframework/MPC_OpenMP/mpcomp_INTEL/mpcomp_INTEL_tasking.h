#ifndef __MPCOMP_INTEL_TASKING_H__
#define __MPCOMP_INTEL_TASKING_H__

#include "mpcomp_internal.h"
#include "mpcomp_INTEL_types.h"

/********************************
  * TASKING SUPPORT
  *******************************/

typedef kmp_int32 (* kmp_routine_entry_t)( kmp_int32, void * );

typedef struct kmp_task {                   /* GEH: Shouldn't this be aligned somehow? */
    void *              shareds;            /**< pointer to block of pointers to shared vars   */
    kmp_routine_entry_t routine;            /**< pointer to routine to call for executing task */
    kmp_int32           part_id;            /**< part id for the task                          */
#if OMP_40_ENABLED
    kmp_routine_entry_t destructors;        /* pointer to function to invoke deconstructors of firstprivate C++ objects */
#endif // OMP_40_ENABLED
    /*  private vars  */
} kmp_task_t;

/* From kmp_os.h for this type */
typedef long             kmp_intptr_t;

typedef struct kmp_depend_info {
     kmp_intptr_t               base_addr;
     size_t 	                len;
     struct {
         bool                   in:1;
         bool                   out:1;
     } flags;
} kmp_depend_info_t;

struct kmp_taskdata {                                 /* aligned during dynamic allocation       */
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
} ;
typedef struct kmp_taskdata  kmp_taskdata_t;


/* FOR OPENMP 3 */

kmp_int32 __kmpc_omp_task(ident_t*, kmp_int32, kmp_task_t*);
kmp_task_t* __kmpc_omp_task_alloc(ident_t*, kmp_int32, kmp_int32, size_t, size_t, kmp_routine_entry_t);
void __kmpc_omp_task_begin_if0(ident_t*, kmp_int32, kmp_task_t*);
void __kmpc_omp_task_complete_if0(ident_t*, kmp_int32, kmp_task_t*);

/* FOR OPENMP 4 */ 

void __kmpc_taskgroup(ident_t*, int);
void __kmpc_end_taskgroup(ident_t*, int);
kmp_int32 __kmpc_omp_task_with_deps(ident_t*, kmp_int32, kmp_task_t*, kmp_int32, kmp_depend_info_t*, kmp_int32, kmp_depend_info_t*);
void __kmpc_omp_wait_deps(ident_t*, kmp_int32, kmp_int32, kmp_depend_info_t*, kmp_int32, kmp_depend_info_t*);
void __kmp_release_deps(kmp_int32, kmp_taskdata_t*);

#endif /* __MPCOMP_INTEL_TASKING_H__ */

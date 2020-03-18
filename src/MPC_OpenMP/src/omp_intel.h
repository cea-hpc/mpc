#ifndef OMP_INTEL_H_
#define OMP_INTEL_H_

#include "mpcomp_api.h"
#include "mpcomp_types.h"
#include "omp_intel_types.h"

/***********************
 * RUNTIME BEGIN & END *
 ***********************/

void __kmpc_begin( ident_t *loc, kmp_int32 flags );
void __kmpc_end( ident_t *loc );

/*************
 * INTEL API *
 *************/

kmp_int32 __kmpc_global_thread_num( ident_t * );
kmp_int32 __kmpc_global_num_threads( ident_t * );
kmp_int32 __kmpc_bound_thread_num( ident_t * );
kmp_int32 __kmpc_bound_num_threads( ident_t * );
kmp_int32 __kmpc_in_parallel( ident_t * );
void __kmpc_flush( ident_t *loc, ... );

/***********
 * BARRIER *
 ***********/

void __kmpc_barrier( ident_t *, kmp_int32 );
kmp_int32 __kmpc_barrier_master( ident_t *, kmp_int32 );
void __kmpc_end_barrier_master( ident_t *, kmp_int32 );
kmp_int32 __kmpc_barrier_master_nowait( ident_t *, kmp_int32 );

/*******************
 * PARALLEL REGION *
 *******************/

kmp_int32 __kmpc_ok_to_fork( ident_t * );
void __kmpc_fork_call( ident_t *, kmp_int32, kmpc_micro, ... );
void __kmpc_serialized_parallel( ident_t *, kmp_int32 );
void __kmpc_end_serialized_parallel( ident_t *, kmp_int32 );
void __kmpc_push_num_threads( ident_t *, kmp_int32, kmp_int32 );
void __kmpc_push_num_teams( ident_t *, kmp_int32, kmp_int32, kmp_int32 );
void __kmpc_fork_teams( ident_t *, kmp_int32, kmpc_micro, ... );

/*******************
 * SYNCHRONIZATION *
 *******************/

kmp_int32 __kmpc_master( ident_t *, kmp_int32 );
void __kmpc_end_master( ident_t *, kmp_int32 );
void __kmpc_ordered( ident_t *, kmp_int32 );
void __kmpc_end_ordered( ident_t *, kmp_int32 );
void __kmpc_critical( ident_t *, kmp_int32, kmp_critical_name * );
void __kmpc_end_critical( ident_t *, kmp_int32, kmp_critical_name * );
kmp_int32 __kmpc_single( ident_t *, kmp_int32 );
void __kmpc_end_single( ident_t *, kmp_int32 );

/*********
 * LOCKS *
 *********/

typedef struct iomp_lock_s
{
	void *lk;
} iomp_lock_t;

void __kmpc_init_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_destroy_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_destroy_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_set_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_set_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_unset_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_unset_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
int __kmpc_test_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
int __kmpc_test_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );

/*************
 * REDUCTION *
 *************/

int __kmp_determine_reduction_method(
    ident_t *loc,  kmp_int32 global_tid,  kmp_int32 num_vars,  size_t reduce_size,
    void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ),
    kmp_critical_name *lck, mpcomp_thread_t *t );

kmp_int32 __kmpc_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                                size_t reduce_size, void *reduce_data,
                                void ( *reduce_func )( void *lhs_data, void *rhs_data ),
                                kmp_critical_name *lck );

void __kmpc_end_reduce_nowait(  ident_t *loc,  kmp_int32 global_tid,
                                kmp_critical_name *lck ) ;

kmp_int32 __kmpc_reduce( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                         size_t reduce_size, void *reduce_data,
                         void ( *reduce_func )( void *lhs_data, void *rhs_data ),
                         kmp_critical_name *lck );

void __kmpc_end_reduce(  ident_t *loc,  kmp_int32 global_tid,
                         kmp_critical_name *lck );

/**********
 * STATIC *
 **********/

#if 0
	void __kmpc_for_static_init_4( ident_t *, kmp_int32, kmp_int32, kmp_int32 *, kmp_uint32 *, kmp_uint32 *, kmp_int32 *, kmp_int32, kmp_int32 );
	void __kmpc_for_static_init_4u( ident_t *, kmp_int32, kmp_int32, kmp_int32 *, kmp_uint32 *, kmp_uint32 *, kmp_int32 *, kmp_int32, kmp_int32 );
	void __kmpc_for_static_init_8( ident_t *, kmp_int32, kmp_int32, kmp_int32 *, kmp_uint32 *, kmp_uint64 *, kmp_int64 *, kmp_int64, kmp_int64 );
	void __kmpc_for_static_init_8u( ident_t *, kmp_int32, kmp_int32, kmp_int32 *, kmp_uint32 *, kmp_uint64 *, kmp_int64 *, kmp_int64, kmp_int64 );
#endif

void __kmpc_for_static_fini( ident_t *, kmp_int32 );

/************
 * DISPATCH *
 ************/

void __kmpc_dispatch_init_4( ident_t *loc,  kmp_int32 gtid,
                             enum sched_type schedule, kmp_int32 lb,
                             kmp_int32 ub, kmp_int32 st, kmp_int32 chunk );

void __kmpc_dispatch_fini_4( ident_t *loc,  kmp_int32 gtid );


void __kmpc_dispatch_init_4u( ident_t *loc,  kmp_int32 gtid,
                              enum sched_type schedule, kmp_uint32 lb,
                              kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk );

void __kmpc_dispatch_fini_4u( ident_t *loc,  kmp_int32 gtid );


void __kmpc_dispatch_init_8( ident_t *loc,  kmp_int32 gtid,
                             enum sched_type schedule, kmp_int64 lb,
                             kmp_int64 ub, kmp_int64 st, kmp_int64 chunk );

void __kmpc_dispatch_fini_8( ident_t *loc,  kmp_int32 gtid );

void __kmpc_dispatch_init_8u( ident_t *loc,  kmp_int32 gtid,
                              enum sched_type schedule, kmp_uint64 lb,
                              kmp_uint64 ub, kmp_int64 st, kmp_int64 chunk );

void __kmpc_dispatch_fini_8u( ident_t *loc,  kmp_int32 gtid );

int __kmpc_dispatch_next_4( ident_t *loc, kmp_int32 gtid,  kmp_int32 *p_last,
                            kmp_int32 *p_lb, kmp_int32 *p_ub,   kmp_int32 *p_st );

int __kmpc_dispatch_next_4u( ident_t *loc, kmp_int32 gtid,  kmp_int32 *p_last,
                             kmp_uint32 *p_lb, kmp_uint32 *p_ub,
                             kmp_int32 *p_st );

int __kmpc_dispatch_next_8( ident_t *loc, kmp_int32 gtid,  kmp_int32 *p_last,
                            kmp_int64 *p_lb, kmp_int64 *p_ub,  kmp_int64 *p_st );

int __kmpc_dispatch_next_8u( ident_t *loc, kmp_int32 gtid,  kmp_int32 *p_last,
                             kmp_uint64 *p_lb, kmp_uint64 *p_ub,
                             kmp_int64 *p_st );

/*********
 * ALLOC *
 *********/

void *kmpc_malloc( size_t size );
void *kmpc_calloc( size_t nelem, size_t elsize );
void *kmpc_realloc( void *ptr, size_t size );
void kmpc_free( void *ptr );


/******************
 * THREAD PRIVATE *
 ******************/

typedef struct common_table
{
	struct private_common *data[KMP_HASH_TABLE_SIZE];
} mpcomp_kmp_common_table_t;

struct private_common *
__kmp_threadprivate_find_task_common( struct common_table *tbl, kmp_int32,
                                      void * );
struct shared_common *__kmp_find_shared_task_common( struct shared_table *,
        kmp_int32, void * );
struct private_common *kmp_threadprivate_insert( kmp_int32, void *, void *,
        size_t );
void *__kmpc_threadprivate( ident_t *, kmp_int32, void *, size_t );
kmp_int32 __kmp_default_tp_capacity( void );
void __kmpc_copyprivate( ident_t *, kmp_int32, size_t, void *,
                         void ( *cpy_func )( void *, void * ), kmp_int32 );
void *__kmpc_threadprivate_cached( ident_t *, kmp_int32, void *, size_t,
                                   void ** * );
void __kmpc_threadprivate_register( ident_t *, void *, kmpc_ctor, kmpc_cctor,
                                    kmpc_dtor );
void __kmpc_threadprivate_register_vec( ident_t *, void *, kmpc_ctor_vec,
                                        kmpc_cctor_vec, kmpc_dtor_vec, size_t );


/*********
 * TASKQ *
 *********/

typedef struct kmpc_thunk_t
{
} kmpc_thunk_t;

typedef void ( *kmpc_task_t )( kmp_int32 global_tid, struct kmpc_thunk_t *thunk );

typedef struct kmpc_shared_vars_t
{
} kmpc_shared_vars_t;


/***********
 * TASKING *
 ***********/

#define KMP_TASKDATA_TO_TASK(task_data) ((mpcomp_task_t *)taskdata - 1)
#define KMP_TASK_TO_TASKDATA(task) ((kmp_task_t *)taskdata + 1)

typedef kmp_int32 ( *kmp_routine_entry_t )( kmp_int32, void * );

typedef struct kmp_task   /* GEH: Shouldn't this be aligned somehow? */
{
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

typedef struct kmp_depend_info
{
	kmp_intptr_t base_addr;
	size_t len;
	struct
	{
		bool in : 1;
		bool out : 1;
	} flags;
} kmp_depend_info_t;

struct kmp_taskdata   /* aligned during dynamic allocation       */
{
#if 0
	kmp_int32               td_task_id;               /* id, assigned by debugger                */
	kmp_tasking_flags_t     td_flags;                 /* task flags                              */
	kmp_team_t             *td_team;                  /* team for this task                      */
	kmp_info_p             *td_alloc_thread;          /* thread that allocated data structures   */
	/* Currently not used except for perhaps IDB */
	kmp_taskdata_t         *td_parent;                /* parent task                             */
	kmp_int32               td_level;                 /* task nesting level                      */
	ident_t                *td_ident;                 /* task identifier                         */
	// Taskwait data.
	ident_t                *td_taskwait_ident;
	kmp_uint32              td_taskwait_counter;
	kmp_int32               td_taskwait_thread;       /* gtid + 1 of thread encountered taskwait */
	kmp_internal_control_t  td_icvs;                  /* Internal control variables for the task */
	volatile kmp_uint32     td_allocated_child_tasks;  /* Child tasks (+ current task) not yet deallocated */
	volatile kmp_uint32     td_incomplete_child_tasks; /* Child tasks not yet complete */
#if OMP_40_ENABLED
	kmp_taskgroup_t        *td_taskgroup;         // Each task keeps pointer to its current taskgroup
	kmp_dephash_t          *td_dephash;           // Dependencies for children tasks are tracked from here
	kmp_depnode_t          *td_depnode;           // Pointer to graph node if this task has dependencies
#endif
#if KMP_HAVE_QUAD
	_Quad                   td_dummy;             // Align structure 16-byte size since allocated just before kmp_task_t
#else
	kmp_uint32              td_dummy[2];
#endif
#endif
};

typedef struct kmp_taskdata kmp_taskdata_t;

kmp_int32 __kmpc_omp_task( ident_t *, kmp_int32, kmp_task_t * );
void __kmp_omp_task_wrapper( void * );
kmp_task_t *__kmpc_omp_task_alloc( ident_t *, kmp_int32, kmp_int32, size_t,
                                   size_t, kmp_routine_entry_t );
void __kmpc_omp_task_begin_if0( ident_t *, kmp_int32, kmp_task_t * );
void __kmpc_omp_task_complete_if0( ident_t *, kmp_int32, kmp_task_t * );
kmp_int32 __kmpc_omp_task_parts( ident_t *, kmp_int32, kmp_task_t * );
kmp_int32 __kmpc_omp_taskwait( ident_t *loc_ref, kmp_int32 gtid );
kmp_int32 __kmpc_omp_taskwait( ident_t *loc_ref, kmp_int32 gtid );
kmp_int32 __kmpc_omp_taskyield( ident_t *loc_ref, kmp_int32 gtid, int end_part );
void __kmpc_omp_task_begin( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task );
void __kmpc_omp_task_complete( ident_t *loc_ref, kmp_int32 gtid,
                               kmp_task_t *task );
void __kmpc_taskgroup( ident_t *loc, int gtid );
void __kmpc_end_taskgroup( ident_t *loc, int gtid );
kmp_int32 __kmpc_omp_task_with_deps( ident_t *, kmp_int32, kmp_task_t *,
                                     kmp_int32, kmp_depend_info_t *, kmp_int32,
                                     kmp_depend_info_t * );
void __kmpc_omp_wait_deps( ident_t *, kmp_int32, kmp_int32, kmp_depend_info_t *,
                           kmp_int32, kmp_depend_info_t * );
void __kmp_release_deps( kmp_int32, kmp_taskdata_t * );

/*******************
 * WRAPPER FUNTION *
 *******************/

extern int __kmp_invoke_microtask( kmpc_micro pkfn, int gtid, int npr, int argc,
                                   void *argv[] );

typedef struct mpcomp_intel_wrapper_s
{
	microtask_t f;
	int argc;
	void **args;
} mpcomp_intel_wrapper_t;

void mpcomp_intel_wrapper_func( void * );

#endif /*OMP_INTEL_H_*/
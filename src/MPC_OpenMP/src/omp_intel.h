/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:10 CEST 2021                                        # */
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
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
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
void __kmpc_critical_with_hint(ident_t *, kmp_int32,kmp_critical_name *, uint32_t);
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
// TODO
//void __kmpc_init_lock_with_hint(ident_t *loc, kmp_int32 gtid, void **user_lock, uintptr_t hint);
void __kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
// TODO
//void __kmpc_init_nest_lock_with_hint(ident_t *loc, kmp_int32 gtid, void **user_lock, uintptr_t hint);
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
    kmp_critical_name *lck, mpc_omp_thread_t *t );

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

#define mpcomp_kmp_task_serial 0x20000
#define mpcomp_kmp_tasking_ser 0x40000

#if OMPT_SUPPORT
/*! To mark a static loop in OMPT callbacks */
#define KMP_IDENT_WORK_LOOP 0x200
/*! To mark a sections directive in OMPT callbacks */
#define KMP_IDENT_WORK_SECTIONS 0x400
/*! To mark a distirbute construct in OMPT callbacks */
#define KMP_IDENT_WORK_DISTRIBUTE 0x800
#endif /* OMPT_SUPPORT */

#define KMP_TASKDATA_TO_TASK(task_data) ((mpc_omp_task_t *)taskdata - 1)
#define KMP_TASK_TO_TASKDATA(task) ((kmp_task_t *)taskdata + 1)

/* kmp.h */
typedef kmp_int32 ( *kmp_routine_entry_t )( kmp_int32, void * );

typedef union kmp_cmplrdata {
    kmp_int32 priority;                 /**< priority specified by user for the task */
    kmp_routine_entry_t destructors;    /* pointer to function to invoke deconstructors of firstprivate C++ objects */
} kmp_cmplrdata_t;

typedef struct kmp_task   /* GEH: Shouldn't this be aligned somehow? */
{
	void * shareds;                     /**< pointer to block of pointers to shared vars   */
	kmp_routine_entry_t routine;        /**< pointer to routine to call for executing task */
	kmp_int32 part_id;                  /**< part id for the task                          */
    kmp_cmplrdata_t data1;              /* Two known optional additions: destructors and priority */
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

/* Following commented structs here for info on intel tasking internals. */
typedef struct kmp_tasking_flags { /* Total struct must be exactly 32 bits */
  /* Compiler flags */ /* Total compiler flags must be 16 bits */
  unsigned tiedness : 1; /* task is either tied (1) or untied (0) */
  unsigned final : 1; /* task is final(1) so execute immediately */
  unsigned merged_if0 : 1; /* no __kmpc_task_{begin/complete}_if0 calls in if0
                              code path */
  unsigned destructors_thunk : 1; /* set if the compiler creates a thunk to
                                     invoke destructors from the runtime */
  unsigned proxy : 1; /* task is a proxy task (it will be executed outside the
                         context of the RTL) */
  unsigned priority_specified : 1; /* set if the compiler provides priority
                                      setting for the task */
  unsigned detachable : 1; /* 1 == can detach */
  unsigned reserved : 9; /* reserved for compiler use */

  /* Library flags */ /* Total library flags must be 16 bits */
  unsigned tasktype : 1; /* task is either explicit(1) or implicit (0) */
  unsigned task_serial : 1; // task is executed immediately (1) or deferred (0)
  unsigned tasking_ser : 1; // all tasks in team are either executed immediately (1) or may be deferred (0)
  unsigned team_serial : 1; // entire team is serial (1) [1 thread] or parallel

  // (0) [>= 2 threads]
  /* If either team_serial or tasking_ser is set, task team may be NULL */
  /* Task State Flags: */
  unsigned started : 1; /* 1==started, 0==not started     */
  unsigned executing : 1; /* 1==executing, 0==not executing */
  unsigned complete : 1; /* 1==complete, 0==not complete   */
  unsigned freed : 1; /* 1==freed, 0==allocateed        */
  unsigned native : 1; /* 1==gcc-compiled task, 0==intel */
  unsigned reserved31 : 7; /* reserved for library use */
} kmp_tasking_flags_t;

struct kmp_taskdata   /* aligned during dynamic allocation       */
{
};

typedef struct kmp_taskdata kmp_taskdata_t;

kmp_int32 __kmpc_omp_task( ident_t *, kmp_int32, kmp_task_t * );
kmp_task_t *__kmpc_omp_task_alloc( ident_t *, kmp_int32, kmp_int32, size_t,
                                   size_t, kmp_routine_entry_t );
kmp_task_t *__kmpc_omp_target_task_alloc(ident_t *, kmp_int32, kmp_int32, size_t,
                                        size_t, kmp_routine_entry_t, kmp_int64);
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

typedef struct mpc_omp_intel_wrapper_s
{
	microtask_t f;
	int argc;
	void **args;
} mpc_omp_intel_wrapper_t;

void mpc_omp_intel_wrapper_func( void * );

/******************
 * TARGET INTEROP *
 ******************/

void **__kmpc_omp_get_target_async_handle_ptr(kmp_int32 gtid);
bool __kmpc_omp_has_task_team(kmp_int32 gtid);

#endif /*OMP_INTEL_H_*/

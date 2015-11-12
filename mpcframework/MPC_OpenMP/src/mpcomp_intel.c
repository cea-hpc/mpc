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

#include <mpcomp_abi.h>
#include <sctk_debug.h>
#include "mpcomp_internal.h"

/* Declaration from MPC
TODO: put these declarations in header file
*/
void __mpcomp_internal_begin_parallel_region(int arg_num_threads,
		mpcomp_new_parallel_region_info_t info ) ;

void __mpcomp_internal_end_parallel_region( mpcomp_instance_t * instance ) ;

/*
 *
 * Target high-priority directives:
 *
 * ---------------
 * SIGNATURE ONLY:
 * ---------------
 *
 * __kmpc_for_static_init_4u
 * __kmpc_for_static_init_8u
 * __kmpc_flush
 *
 * __kmpc_set_lock
 * __kmpc_destroy_lock
 * __kmpc_init_lock
 * __kmpc_unset_lock
 *
 * __kmpc_begin
 * __kmpc_end
 * __kmpc_serialized_parallel
 * __kmpc_end_serialized_parallel
 *
 * __kmpc_atomic_fixed4_wr
 * __kmpc_atomic_fixed8_add
 * __kmpc_atomic_float8_add
 *
 * ---------------
 * IMPLEMENTATION:
 * ---------------
 *
 * __kmpc_for_static_init_8
 * __kmpc_for_static_init_4
 * __kmpc_for_static_fini
 *
 * __kmpc_dispatch_init_4
 * __kmpc_dispatch_next_4
 *
 * __kmpc_critical
 * __kmpc_end_critical
 *
 * __kmpc_single
 * __kmpc_end_single
 *
 * __kmpc_barrier
 *
 * __kmpc_push_num_threads
 * __kmpc_ok_to_fork
 * __kmpc_fork_call
 *
 * __kmpc_master
 * __kmpc_end_master
 *
 * __kmpc_atomic_fixed4_add
 *
 * __kmpc_global_thread_num
 * 
 * __kmpc_reduce
 * __kmpc_end_reduce
 * __kmpc_reduce_nowait
 * __kmpc_end_reduce_nowait
 *
 *
 */

/* KMP_OS.H */

typedef char               kmp_int8;
typedef unsigned char      kmp_uint8;
typedef short              kmp_int16;
typedef unsigned short     kmp_uint16;
typedef int                kmp_int32;
typedef unsigned int       kmp_uint32;
typedef long long          kmp_int64;
typedef unsigned long long kmp_uint64;


typedef float   kmp_real32;
typedef double  kmp_real64;

/* KMP.H */

typedef struct ident {
  kmp_int32 reserved_1 ;
  kmp_int32 flags ;
  kmp_int32 reserved_2 ;
  kmp_int32 reserved_3 ;
  char const *psource ;
} ident_t ;


typedef void (*kmpc_micro) (kmp_int32 * global_tid, kmp_int32 * bound_tid, ...) ;

enum sched_type {
    kmp_sch_lower                     = 32,   /**< lower bound for unordered values */
    kmp_sch_static_chunked            = 33,
    kmp_sch_static                    = 34,   /**< static unspecialized */
    kmp_sch_dynamic_chunked           = 35,
    kmp_sch_guided_chunked            = 36,   /**< guided unspecialized */
    kmp_sch_runtime                   = 37,
    kmp_sch_auto                      = 38,   /**< auto */
    kmp_sch_trapezoidal               = 39,

    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_sch_static_greedy             = 40,
    kmp_sch_static_balanced           = 41,
    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_sch_guided_iterative_chunked  = 42,
    kmp_sch_guided_analytical_chunked = 43,

    kmp_sch_static_steal              = 44,   /**< accessible only through KMP_SCHEDULE environment variable */

    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_sch_upper                     = 45,   /**< upper bound for unordered values */

    kmp_ord_lower                     = 64,   /**< lower bound for ordered values, must be power of 2 */
    kmp_ord_static_chunked            = 65,
    kmp_ord_static                    = 66,   /**< ordered static unspecialized */
    kmp_ord_dynamic_chunked           = 67,
    kmp_ord_guided_chunked            = 68,
    kmp_ord_runtime                   = 69,
    kmp_ord_auto                      = 70,   /**< ordered auto */
    kmp_ord_trapezoidal               = 71,
    kmp_ord_upper                     = 72,   /**< upper bound for ordered values */

#if OMP_40_ENABLED
    /* Schedules for Distribute construct */
    kmp_distribute_static_chunked     = 91,   /**< distribute static chunked */
    kmp_distribute_static             = 92,   /**< distribute static unspecialized */
#endif

    /*
     * For the "nomerge" versions, kmp_dispatch_next*() will always return
     * a single iteration/chunk, even if the loop is serialized.  For the
     * schedule types listed above, the entire iteration vector is returned
     * if the loop is serialized.  This doesn't work for gcc/gcomp sections.
     */
    kmp_nm_lower                      = 160,  /**< lower bound for nomerge values */

    kmp_nm_static_chunked             = (kmp_sch_static_chunked - kmp_sch_lower + kmp_nm_lower),
    kmp_nm_static                     = 162,  /**< static unspecialized */
    kmp_nm_dynamic_chunked            = 163,
    kmp_nm_guided_chunked             = 164,  /**< guided unspecialized */
    kmp_nm_runtime                    = 165,
    kmp_nm_auto                       = 166,  /**< auto */
    kmp_nm_trapezoidal                = 167,

    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_nm_static_greedy              = 168,
    kmp_nm_static_balanced            = 169,
    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_nm_guided_iterative_chunked   = 170,
    kmp_nm_guided_analytical_chunked  = 171,
    kmp_nm_static_steal               = 172,  /* accessible only through OMP_SCHEDULE environment variable */

    kmp_nm_ord_static_chunked         = 193,
    kmp_nm_ord_static                 = 194,  /**< ordered static unspecialized */
    kmp_nm_ord_dynamic_chunked        = 195,
    kmp_nm_ord_guided_chunked         = 196,
    kmp_nm_ord_runtime                = 197,
    kmp_nm_ord_auto                   = 198,  /**< auto */
    kmp_nm_ord_trapezoidal            = 199,
    kmp_nm_upper                      = 200,  /**< upper bound for nomerge values */

    kmp_sch_default = kmp_sch_static  /**< default scheduling algorithm */
};

typedef kmp_int32 kmp_critical_name[8] ;



/*!
 * Values for bit flags used in the ident_t to describe the fields.
 * */
/*! Use trampoline for internal microtasks */
#define KMP_IDENT_IMB             0x01
/*! Use c-style ident structure */
#define KMP_IDENT_KMPC            0x02
/* 0x04 is no longer used */
/*! Entry point generated by auto-parallelization */
#define KMP_IDENT_AUTOPAR         0x08
/*! Compiler generates atomic reduction option for kmpc_reduce* */
#define KMP_IDENT_ATOMIC_REDUCE   0x10
/*! To mark a 'barrier' directive in user code */
#define KMP_IDENT_BARRIER_EXPL    0x20
/*! To Mark implicit barriers. */
#define KMP_IDENT_BARRIER_IMPL           0x0040
#define KMP_IDENT_BARRIER_IMPL_MASK      0x01C0
#define KMP_IDENT_BARRIER_IMPL_FOR       0x0040
#define KMP_IDENT_BARRIER_IMPL_SECTIONS  0x00C0

#define KMP_IDENT_BARRIER_IMPL_SINGLE    0x0140
#define KMP_IDENT_BARRIER_IMPL_WORKSHARE 0x01C0

/*******************************
  * REDUCTION
  *****************************/

/* different reduction cases */
enum _reduction_method 
{
    reduction_method_not_defined = 0,
    critical_reduce_block        = ( 1 << 8 ),
    atomic_reduce_block          = ( 2 << 8 ),
    tree_reduce_block            = ( 3 << 8 ),
    empty_reduce_block           = ( 4 << 8 )
};

int __kmp_force_reduction_method = reduction_method_not_defined;

#define FAST_REDUCTION_ATOMIC_METHOD_GENERATED ( ( loc->flags & ( KMP_IDENT_ATOMIC_REDUCE ) ) == ( KMP_IDENT_ATOMIC_REDUCE ) )
#define FAST_REDUCTION_TREE_METHOD_GENERATED   ( ( reduce_data ) && ( reduce_func ) )

/*******************************
  * THREADPRIVATE
  ******************************/
int __kmp_init_common = 0;
/* keeps tracked of threadprivate cache allocations for cleanup later */
typedef void    (*microtask_t)( int *gtid, int *npr, ... );
typedef void *(* kmpc_ctor)(void *) ;
typedef void  (* kmpc_dtor)(void *) ;
typedef void *(* kmpc_cctor)(void *, void *) ;
typedef void *(* kmpc_ctor_vec)(void *, size_t) ;
typedef void  (* kmpc_dtor_vec)(void *, size_t) ;
typedef void *(* kmpc_cctor_vec)(void *, void *, size_t) ;

typedef struct kmp_cached_addr {
    void                      **addr;           /* address of allocated cache */
    struct kmp_cached_addr     *next;           /* pointer to next cached address */
} kmp_cached_addr_t;

kmp_cached_addr_t  *__kmp_threadpriv_cache_list = NULL; /* list for cleaning */

struct private_data {
    struct private_data *next;          /* The next descriptor in the list      */
    void                *data;          /* The data buffer for this descriptor  */
    int                  more;          /* The repeat count for this descriptor */
    size_t               size;          /* The data size for this descriptor    */
};

struct private_common {
    struct private_common     *next;
    struct private_common     *link;
    void                      *gbl_addr;
    void                      *par_addr;        /* par_addr == gbl_addr for MASTER thread */
    size_t                     cmn_size;
};

struct shared_common
{
    struct shared_common      *next;
    struct private_data       *pod_init;
    void                      *obj_init;
    void                      *gbl_addr;
    union {
        kmpc_ctor              ctor;
        kmpc_ctor_vec          ctorv;
    } ct;
    union {
        kmpc_cctor             cctor;
        kmpc_cctor_vec         cctorv;
    } cct;
    union {
        kmpc_dtor              dtor;
        kmpc_dtor_vec          dtorv;
    } dt;
    size_t                     vec_len;
    int                        is_vec;
    size_t                     cmn_size;
};

struct shared_table {
    struct shared_common *data[ KMP_HASH_TABLE_SIZE ];
};

struct shared_table __kmp_threadprivate_d_table;

/* MY STRUCTS */

typedef struct wrapper {
  microtask_t f ;
  int argc ;
  void ** args ;
} wrapper_t ;


/********************************
  * FUNCTIONS
  *******************************/
extern int __kmp_x86_pause();
extern int __kmp_compare_and_store8(kmp_int8 *, kmp_int8, kmp_int8);
extern int __kmp_compare_and_store16(kmp_int16 *, kmp_int16, kmp_int16);
extern int __kmp_compare_and_store32(kmp_int32 *, kmp_int32, kmp_int32);
extern int __kmp_compare_and_store64(kmp_int64 *, kmp_int64, kmp_int64);
extern int __kmp_xchg_fixed32( kmp_int32 *, kmp_int32  ) ;
extern int __kmp_xchg_fixed64( kmp_int64 *, kmp_int64  ) ;
extern int __kmp_test_then_add32( kmp_int32 *, kmp_int32  ) ;
extern long __kmp_test_then_add64( kmp_int64 *, kmp_int64 ) ;
extern double __kmp_test_then_add_real32( kmp_real32 *, kmp_real32 );
extern double __kmp_test_then_add_real64( kmp_real64 *, kmp_real64 );
extern int __kmp_invoke_microtask(kmpc_micro pkfn, int gtid, int npr, int argc, void *argv[] );

void *
wrapper_function( void * arg ) 
{
    int rank;
    wrapper_t * w;

    rank = mpcomp_get_thread_num();
    w = (wrapper_t *) arg;

    __kmp_invoke_microtask( w->f, rank, rank, w->argc, w->args );

    return NULL ;
}

/********************************
  * STARTUP AND SHUTDOWN
  *******************************/

void   
__kmpc_begin ( ident_t * loc, kmp_int32 flags ) 
{
// not_implemented() ;
}

void   
__kmpc_end ( ident_t * loc )
{
// not_implemented() ;
}


/********************************
  * PARALLEL FORK/JOIN 
  *******************************/

kmp_int32
__kmpc_ok_to_fork(ident_t * loc)
{
  sctk_nodebug( "__kmpc_ok_to_fork: entering..." ) ;
  return 1 ;
}

/* TEMP */
#if 0
int intel_temp_argc = 10 ;
void ** intel_temp_args_copy = NULL ;
#endif

void
__kmpc_fork_call(ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ...)
{
  va_list args ;
  int i ;
  void ** args_copy ;
  wrapper_t w ;
  mpcomp_thread_t *t;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

  sctk_nodebug( "__kmpc_fork_call: entering w/ %d arg(s)...", argc ) ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;
  
  /* Threadprvate initialisation */
  sctk_thread_mutex_lock(&lock);
  if( !__kmp_init_common )
  {
    for (i = 0; i < KMP_HASH_TABLE_SIZE; ++i)
      __kmp_threadprivate_d_table.data[ i ] = 0;
    __kmp_init_common = 1;
  }
  sctk_thread_mutex_unlock(&lock);

  /* Grab info on the current thread */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

// fprintf( stderr, "__kmpc_fork_call: parallel region w/ %d args\n", argc ) ;

#if 0
if ( intel_temp_args_copy == NULL ) 
{
intel_temp_args_copy = (void **)malloc( intel_temp_argc * sizeof( void * ) ) ;
}
if ( argc <= intel_temp_argc ) 
{
args_copy = intel_temp_args_copy ;
} else 
{
  args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
  sctk_assert( args_copy ) ;
}
#else
  args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
  sctk_assert( args_copy ) ;
#endif

  va_start( args, microtask ) ;

  for ( i = 0 ; i < argc ; i++ ) {
    args_copy[i] = va_arg( args, void * ) ;
  }

  va_end(args) ;

  w.f = microtask ;
  w.argc = argc ;
  w.args = args_copy ;

  __mpcomp_start_parallel_region( t->push_num_threads, wrapper_function, &w ) ;

  /* restore the number of threads w/ num_threads clause */
  t->push_num_threads = -1 ;
}

void
__kmpc_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_nodebug( "__kmpc_serialized_parallel: entering (%d) ...", global_tid ) ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	info.func = NULL ; /* No function to call */
	info.shared = NULL ;
	info.num_threads = 1 ;
	info.new_root = NULL ;
	info.icvs = t->info.icvs ; 
	info.combined_pragma = MPCOMP_COMBINED_NONE;

	__mpcomp_internal_begin_parallel_region( 1, info ) ;

	/* Save the current tls */
	t->children_instance->mvps[0]->threads[0].father = sctk_openmp_thread_tls ;

	/* Switch TLS to nested thread for region-body execution */
	sctk_openmp_thread_tls = &(t->children_instance->mvps[0]->threads[0]) ;
}

void
__kmpc_end_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
  mpcomp_thread_t * t ;

  sctk_nodebug( "__kmpc_end_serialized_parallel: entering (%d)...", global_tid ) ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	/* Restore the previous thread info */
	sctk_openmp_thread_tls = t->father ;

	__mpcomp_internal_end_parallel_region( t->instance ) ;

}

void
__kmpc_push_num_threads( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_threads )
{
  mpcomp_thread_t * t ;

  sctk_nodebug( "__kmpc_push_num_threads: pushing %d thread(s)", num_threads ) ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_assert( t->push_num_threads == -1 ) ;

  t->push_num_threads = num_threads ;
}

void 
__kmpc_push_num_teams(ident_t *loc, kmp_int32 global_tid, kmp_int32 num_teams,
    kmp_int32 num_threads )
{
  not_implemented() ;
}

void
__kmpc_fork_teams( ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ... )
{
  not_implemented() ;
}

/********************************
  * SYNCHRONIZATION 
  *******************************/

kmp_int32
__kmpc_global_thread_num(ident_t * loc)
{
  sctk_nodebug( "__kmpc_global_thread_num: " ) ;
  return mpcomp_get_thread_num() ;
}

kmp_int32
__kmpc_global_num_threads(ident_t * loc)
{
  sctk_nodebug( "__kmpc_global_num_threads: " ) ;
  return mpcomp_get_num_threads() ;
}

kmp_int32
__kmpc_bound_thread_num(ident_t * loc)
{
  sctk_nodebug( "__kmpc_bound_thread_num: " ) ;
  return mpcomp_get_thread_num() ;
}

kmp_int32
__kmpc_bound_num_threads(ident_t * loc)
{
  sctk_nodebug( "__kmpc_bound_num_threads: " ) ;
  return mpcomp_get_num_threads() ;
}

kmp_int32
__kmpc_in_parallel(ident_t * loc)
{
  sctk_nodebug( "__kmpc_in_parallel: " ) ;
  return mpcomp_in_parallel() ;
}


/********************************
  * SYNCHRONIZATION 
  *******************************/

void
__kmpc_flush(ident_t *loc, ...) 
{
  /* TODO depending on the compiler, call the right function
   * Maybe need to do the same for mpcomp_flush...
   */
  __sync_synchronize() ;
}

void
__kmpc_barrier(ident_t *loc, kmp_int32 global_tid) 
{
  sctk_nodebug( "[%d] __kmpc_barrier: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;

  __mpcomp_barrier() ;
}

kmp_int32
__kmpc_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;

  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }

  __mpcomp_barrier() ;

  return 0 ;
}

void
__kmpc_end_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;

  __mpcomp_barrier() ;
}

kmp_int32
__kmpc_barrier_master_nowait(ident_t *loc, kmp_int32 global_tid)
{
  not_implemented() ;

  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }

  return 0 ;
}

int __kmp_determine_reduction_method( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, 
                                      size_t reduce_size, void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
                                      kmp_critical_name *lck, mpcomp_thread_t * t )
{
    int retval = critical_reduce_block;
    int team_size;
    int teamsize_cutoff = 4;
    team_size = t->instance->team->info.num_threads;
    
    if (team_size == 1)
    {
        return empty_reduce_block;
    }
    else
    {
        int atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
        int tree_available   = FAST_REDUCTION_TREE_METHOD_GENERATED;
        
        if( tree_available ) 
        {
            if( team_size <= teamsize_cutoff ) 
            {
                if ( atomic_available ) 
                {
                    retval = atomic_reduce_block;
                }
            } 
            else 
            {
                retval = tree_reduce_block;
            }
        } 
        else if ( atomic_available ) 
        {
            retval = atomic_reduce_block;
        }
    }
    
    /* force reduction method */
    if( __kmp_force_reduction_method != reduction_method_not_defined ) 
    {
        int forced_retval;
        int atomic_available, tree_available;
        switch( ( forced_retval = __kmp_force_reduction_method ) )
        {
            case critical_reduce_block:
                sctk_assert(lck);
                if(team_size <= 1)
                    forced_retval = empty_reduce_block;
            break;
            case atomic_reduce_block:
                atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
                sctk_assert(atomic_available);
            break;
            case tree_reduce_block:
                tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;
                sctk_assert(tree_available);
                forced_retval = tree_reduce_block;
            break;
            default:
                forced_retval = critical_reduce_block;
                sctk_debug("Unknown reduction method");
        }
        retval = forced_retval;
    }
    return retval;
}

kmp_int32 
__kmpc_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
    int retval = 0;
    int packed_reduction_method;
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

    /* get reduction method */
    packed_reduction_method = __kmp_determine_reduction_method( loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t);
    t->reduction_method = packed_reduction_method; 
    if( packed_reduction_method == critical_reduce_block ) 
    {
        __mpcomp_anonymous_critical_begin() ;
        retval = 1;
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
        retval = 1;
    }
    else if( packed_reduction_method == atomic_reduce_block )
    {
        retval = 2;
    }
    else if ( packed_reduction_method == tree_reduce_block )
    {
        __mpcomp_anonymous_critical_begin() ;
        retval = 1;
    }
    else
    {
        sctk_error("__kmpc_reduce_nowait: Unexpected reduction method %d", packed_reduction_method);
        sctk_abort();
    }
    return retval;
}

void 
__kmpc_end_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck ) 
{
    int packed_reduction_method;

    /* get reduction method */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    
    packed_reduction_method = t->reduction_method;
    
    if( packed_reduction_method == critical_reduce_block )
    {
        __mpcomp_anonymous_critical_end() ;
        __mpcomp_barrier();
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == atomic_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == tree_reduce_block ) 
    {
        __mpcomp_anonymous_critical_end() ;
    } 
    else 
    {
        sctk_error("__kmpc_end_reduce_nowait: Unexpected reduction method %d", packed_reduction_method);
        sctk_abort();
    }
}

kmp_int32 
__kmpc_reduce( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /*
     TODO
     lck!=NULL ==> CRITICAL version
     reduce_data!=NULL && reduce_func!=NULL ==> TREE version
     (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE ==> ATOMIC version

     Need to add one variable to remember the choice taken.
     This variable will be written here and read in the end_reduce function

     To be compliant, we execute only the CRITICAL version which is supposed to be available
     in every situation.
   */


  sctk_debug( "[%d] __kmpc_reduce %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;

#if 0
  printf( "[%d] __kmpc_reduce %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d\n",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;
#endif

#if 0
  /* Version with atomic operation */
  return 2 ;
#endif

  /* Version with critical section */
  sctk_assert( lck != NULL ) ;
  __mpcomp_anonymous_critical_begin() ;
  return 1 ;
}

void 
__kmpc_end_reduce( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck )
{

  sctk_debug( "[%d] __kmpc_end_reduce",
      ( (mpcomp_thread_t *) sctk_openmp_thread_tls )->rank ) ;
#if 0
  /* Version with atomic operation */
  return ;
#endif


#if 1
  /* Version with critical section */
  sctk_assert( lck != NULL ) ;
  __mpcomp_anonymous_critical_end() ;
  __mpcomp_barrier();
  return ;
#endif

  // not_implemented() ;
}



/********************************
  * WORK_SHARING
  *******************************/

kmp_int32
__kmpc_master(ident_t *loc, kmp_int32 global_tid)
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_nodebug( "[%d] __kmp_master: entering",
      t->rank ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }
  return 0 ;

}

void
__kmpc_end_master(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_end_master: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void 
__kmpc_ordered(ident_t *loc, kmp_int32 global_tid)
{
    __mpcomp_ordered_begin();
}

void
__kmpc_end_ordered(ident_t *loc, kmp_int32 global_tid)
{
    __mpcomp_ordered_end();
}

void
__kmpc_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  sctk_nodebug( "[%d] __kmpc_critical: enter %p (%d,%d,%d,%d,%d,%d,%d,%d)",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      crit,
      (*crit)[0], (*crit)[1], (*crit)[2], (*crit)[3],
      (*crit)[4], (*crit)[5], (*crit)[6], (*crit)[7]
      ) ;
  /* TODO Handle named critical */
__mpcomp_anonymous_critical_begin () ;
}

void
__kmpc_end_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  /* TODO Handle named critical */
  __mpcomp_anonymous_critical_end () ;
}

kmp_int32
__kmpc_single(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_single: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;

  return __mpcomp_do_single() ;
}

void
__kmpc_end_single(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_end_single: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void
__kmpc_for_static_init_4( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int32 * plower, kmp_int32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
    long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_int32 trip_count;
    //save old pupper
    kmp_int32 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_4: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_int32)from ;
                *pupper=(kmp_int32)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_init_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_uint32 * plower, kmp_uint32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
    long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_uint32 trip_count;
    //save old pupper
    kmp_uint32 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_4u: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_4u (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_uint32)from ;
                *pupper=(kmp_uint32)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_init_8( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int64 * plower, kmp_int64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
    long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_int64 trip_count;
    //save old pupper
    kmp_int64 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_8: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_8 (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_int64)from ;
                *pupper=(kmp_int64)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_init_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_uint64 * plower, kmp_uint64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
  /* TODO: the same as unsigned long long in GCC... */
  sctk_nodebug( "__kmpc_for_static_init_8u: siweof long = %d, sizeof long long %d",
      sizeof( long ), sizeof( long long ) ) ;
    long from, to ;
    mpcomp_thread_t *t;
    int res ;
    kmp_uint64 trip_count;
    //save old pupper
    kmp_uint64 pupper_old = *pupper;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);   
    
    if ( t->info.num_threads == 1 ) 
    {
        if( plastiter != NULL )
            *plastiter = TRUE;
        *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
        
        return;
    }

    //trip_count computation            
    if ( incr == 1 ) {
        trip_count = *pupper - *plower + 1;
    } else if (incr == -1) {
        trip_count = *plower - *pupper + 1;
    } else {
        if ( incr > 1 ) {
            trip_count = (*pupper - *plower) / incr + 1;
        } else {
            trip_count = (*plower - *pupper) / ( -incr ) + 1;
        }
    }            

    sctk_nodebug( "[%d] __kmpc_for_static_init_8u: <%s> "
    "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
    ,
    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
    loc->psource,
    schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
    ) ;

    switch( schedtype ) 
    {
        case kmp_sch_static:
            /* Get the single chunk for the current thread */
            res = __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr, &from, &to ) ;

            /* Chunk to execute? */
            if ( res ) 
            {
                sctk_nodebug( "[%d] Results for __kmpc_for_static_init_8u (kmp_sch_static): "
	            "%ld -> %ld excl %ld incl [%d]"
	            ,
	            ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	            from, to, to-incr, incr
	            ) ;

                /* Remarks:
	            - MPC chunk has not-inclusive upper bound while Intel runtime includes
	            upper bound for calculation 
	            - Need to cast from long to int because MPC handles everything has a long
	            (like GCC) while Intel creates different functions
	            */
                *plower=(kmp_uint64)from ;
                *pupper=(kmp_uint64)to-incr;

            } 
            else 
            {
	            /* No chunk */
	            *pupper=*pupper+incr;
	            *plower=*pupper;
            }

            //plastiter computation
            if(trip_count < t->info.num_threads)
            {    
                if( plastiter != NULL )
                    *plastiter = ( t->rank == trip_count - 1 ); 
            } else {
                if ( incr > 0 ) {
                    if( plastiter != NULL )
                        *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
                } else {
                    if( plastiter != NULL )
                        *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
                }    
            }
        
        break ;
        case kmp_sch_static_chunked:
            if(chunk < 1)
                chunk = 1;
            
            *pstride = (chunk * incr) * t->info.num_threads ;
            *plower = *plower + ((chunk * incr)* t->rank );
            *pupper = *plower + (chunk * incr) - incr;
           
            //plastiter computation 
            if( plastiter != NULL )
                *plastiter = (t->rank == ((trip_count - 1)/chunk) % t->info.num_threads);

            /* Remarks:
	        - MPC chunk has not-inclusive upper bound while Intel runtime includes
	        upper bound for calculation 
	        - Need to cast from long to int because MPC handles everything has a long
	        (like GCC) while Intel creates different functions
	        */
            
            sctk_nodebug("[%d] Results: "
	        "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d"
	        ,
	        t->rank, 
	        *plower, *pupper, *pupper-incr, incr, trip_count, *plastiter
	        ) ;

        break ;
        default:
            not_implemented() ;
        break ;
    } 
}

void
__kmpc_for_static_fini( ident_t * loc, kmp_int32 global_tid ) 
{
  sctk_nodebug( "[%d] __kmpc_for_static_fini: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank ) ;

  /* Nothing to do here... */
}

void 
__kmpc_dispatch_init_4(ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
    kmp_int32 lb, kmp_int32 ub, kmp_int32 st, kmp_int32 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_4: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_4u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_uint32 lb, kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_4u: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_8( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_int64 lb, kmp_int64 ub,
                        kmp_int64 st, kmp_int64 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_8: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, 0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	 lb, ub+st, st, chunk ) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_8u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                         kmp_uint64 lb, kmp_uint64 ub,
                         kmp_int64 st, kmp_int64 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_8u: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
    break ;
    default:
      sctk_error("Schedule not handled: %d\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}



int
__kmpc_dispatch_next_4( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int32 *p_lb, kmp_int32 *p_ub, kmp_int32 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_int32) from ;
  *p_ub = (kmp_int32)to - (kmp_int32)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_ord_runtime:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint32 *p_lb, kmp_uint32 *p_ub, kmp_int32 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_uint32) from ;
  *p_ub = (kmp_uint32)to - (kmp_uint32)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_ord_runtime:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_8( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int64 *p_lb, kmp_int64 *p_ub, kmp_int64 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_int64) from ;
  *p_ub = (kmp_int64)to - (kmp_int64)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_ord_runtime:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint64 *p_lb, kmp_uint64 *p_ub, kmp_int64 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
    case kmp_sch_runtime:
    case kmp_ord_runtime:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_uint64) from ;
  *p_ub = (kmp_uint64)to - (kmp_uint64)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_runtime:
        case kmp_ord_runtime:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

void
__kmpc_dispatch_fini_4( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_4 */
}

void
__kmpc_dispatch_fini_8( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_4u */
}

void
__kmpc_dispatch_fini_4u( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_8 */
}

void
__kmpc_dispatch_fini_8u( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_8u */
}



/********************************
  * THREAD PRIVATE DATA SUPPORT
  *******************************/

int __kmp_default_tp_capacity() 
{
    char * env;
    int nth = 128;
    int __kmp_xproc = 0, req_nproc = 0, r = 0, __kmp_max_nth = 32 * 1024;
    
    r = sysconf( _SC_NPROCESSORS_ONLN );
    __kmp_xproc = (r > 0) ? r : 2;

    env = getenv ("OMP_NUM_THREADS");
    if (env != NULL)
    {
        req_nproc = strtol(env, NULL, 10);              
    }

    if (nth < (4 * req_nproc))
        nth = (4 * req_nproc);
    if (nth < (4 * __kmp_xproc))
        nth = (4 * __kmp_xproc);

    if (nth > __kmp_max_nth)
        nth = __kmp_max_nth;

    return nth;
}

struct private_common *
__kmp_threadprivate_find_task_common( struct common_table *tbl, int gtid, void *pc_addr )
{
    struct private_common *tn;
    for (tn = tbl->data[ KMP_HASH(pc_addr) ]; tn; tn = tn->next) 
    {
        if (tn->gbl_addr == pc_addr) 
        {
            return tn;
        }   
    }
    return 0;
}

struct shared_common *
__kmp_find_shared_task_common( struct shared_table *tbl, int gtid, void *pc_addr )
{
    struct shared_common *tn;
    for (tn = tbl->data[ KMP_HASH(pc_addr) ]; tn; tn = tn->next) 
    {
        if (tn->gbl_addr == pc_addr) 
        {
            return tn;
        }   
    }
    return 0;
}

struct private_common *
kmp_threadprivate_insert( int gtid, void *pc_addr, void *data_addr, size_t pc_size )
{
    struct private_common *tn, **tt;
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    mpcomp_thread_t *t ; 
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

    /* critical section */
    sctk_thread_mutex_lock (&lock);
        tn = (struct private_common *) sctk_malloc( sizeof (struct private_common) );
        tn->gbl_addr = pc_addr;
        tn->cmn_size = pc_size;
        
        if(gtid == 0)
            tn->par_addr = (void *) pc_addr;
        else
            tn->par_addr = (void *) sctk_malloc( tn->cmn_size );
        
        memcpy(tn->par_addr, pc_addr, pc_size);
    sctk_thread_mutex_unlock (&lock);
    /* end critical section */
    
    tt = &(t->th_pri_common->data[ KMP_HASH(pc_addr) ]);
    tn->next = *tt;
    *tt = tn;
    tn->link = t->th_pri_head;
    t->th_pri_head = tn;
    
    return tn;
}

void * 
__kmpc_threadprivate ( ident_t *loc, kmp_int32 global_tid, void * data, size_t size )
{
    void *ret = NULL;
    struct private_common *tn;
    mpcomp_thread_t *t ; 
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

    tn = __kmp_threadprivate_find_task_common( t->th_pri_common, global_tid, data );
    if (!tn)
    {
        tn = kmp_threadprivate_insert( global_tid, data, data, size );
    }
    else
    {
        if((size_t)size > tn->cmn_size)
        {
            sctk_error("TPCommonBlockInconsist: -> Wong size threadprivate variable found");
            sctk_abort();
        }
    }
    
    ret = tn->par_addr;
    return ret ;
}

void
__kmpc_copyprivate(ident_t *loc, kmp_int32 global_tid, size_t cpy_size, void *cpy_data,
    void (*cpy_func)(void *, void *), kmp_int32 didit )
{
    mpcomp_thread_t *t ;    /* Info on the current thread */

    void **data_ptr;
    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

    /* In this flow path, the number of threads should not be 1 */
    sctk_assert( t->info.num_threads > 0 ) ;

    /* Grab the team info */
    sctk_assert( t->instance != NULL ) ;
    sctk_assert( t->instance->team != NULL ) ;
    
    data_ptr = &(t->instance->team->single_copyprivate_data);
    if (didit) *data_ptr = cpy_data;
    
    __mpcomp_barrier();

    if (! didit) (*cpy_func)( cpy_data, *data_ptr );

    __mpcomp_barrier();
}

void *
__kmpc_threadprivate_cached( ident_t *loc, kmp_int32 global_tid, void *data, size_t size, void *** cache)
{  
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER; 
  int __kmp_tp_capacity = __kmp_default_tp_capacity();
  if (*cache == 0)
  {
    sctk_thread_mutex_lock (&lock);
    if (*cache == 0)
    {
        //handle cache to be dealt with later
        void ** my_cache;
        my_cache = (void**) malloc(sizeof( void * ) * __kmp_tp_capacity + sizeof ( kmp_cached_addr_t ));
        kmp_cached_addr_t *tp_cache_addr;
        tp_cache_addr = (kmp_cached_addr_t *) & my_cache[__kmp_tp_capacity];
        tp_cache_addr -> addr = my_cache;
        tp_cache_addr -> next = __kmp_threadpriv_cache_list;
        __kmp_threadpriv_cache_list = tp_cache_addr;
        *cache = my_cache;
    }
    sctk_thread_mutex_unlock (&lock);
  }
  
  void *ret = NULL;
  if ((ret = (*cache)[ global_tid ]) == 0)
  {
      ret = __kmpc_threadprivate( loc, global_tid, data, (size_t) size);
      (*cache)[ global_tid ] = ret;
  }
  return ret;
}

void
__kmpc_threadprivate_register( ident_t *loc, void *data, kmpc_ctor ctor, kmpc_cctor cctor,
    kmpc_dtor dtor)
{
    struct shared_common *d_tn, **lnk_tn;

    /* Only the global data table exists. */
    d_tn = __kmp_find_shared_task_common( &__kmp_threadprivate_d_table, -1, data );

    if (d_tn == 0) 
    {
        d_tn = (struct shared_common *) sctk_malloc( sizeof( struct shared_common ) );
        d_tn->gbl_addr = data;

        d_tn->ct.ctor = ctor;
        d_tn->cct.cctor = cctor;
        d_tn->dt.dtor = dtor;
        
        lnk_tn = &(__kmp_threadprivate_d_table.data[ KMP_HASH(data) ]);

        d_tn->next = *lnk_tn;
        *lnk_tn = d_tn;
    }
}

void
__kmpc_threadprivate_register_vec(ident_t *loc, void *data, kmpc_ctor_vec ctor, 
    kmpc_cctor_vec cctor, kmpc_dtor_vec dtor, size_t vector_length )
{
    struct shared_common *d_tn, **lnk_tn;

    d_tn = __kmp_find_shared_task_common( &__kmp_threadprivate_d_table,
                                          -1, data );        /* Only the global data table exists. */
    if (d_tn == 0) 
    {
        d_tn = (struct shared_common *) sctk_malloc( sizeof( struct shared_common ) );
        d_tn->gbl_addr = data;

        d_tn->ct.ctorv = ctor;
        d_tn->cct.cctorv = cctor;
        d_tn->dt.dtorv = dtor;
        d_tn->is_vec = TRUE;
        d_tn->vec_len = (size_t) vector_length;
        
        lnk_tn = &(__kmp_threadprivate_d_table.data[ KMP_HASH(data) ]);

        d_tn->next = *lnk_tn;
        *lnk_tn = d_tn;
    }
}

/********************************
  * TASKQ INTERFACE
  *******************************/

typedef struct kmpc_thunk_t {
} kmpc_thunk_t ;

typedef void (*kmpc_task_t) (kmp_int32 global_tid, struct kmpc_thunk_t *thunk);

typedef struct kmpc_shared_vars_t {
} kmpc_shared_vars_t;

kmpc_thunk_t * 
__kmpc_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_task_t taskq_task, size_t sizeof_thunk,
    size_t sizeof_shareds, kmp_int32 flags, kmpc_shared_vars_t **shareds)
{
not_implemented() ;
return NULL ;
}

void 
__kmpc_end_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmp_int32 
__kmpc_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk, kmp_int32 status)
{
not_implemented() ;
}

void __kmpc_end_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmpc_thunk_t * 
__kmpc_task_buffer (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *taskq_thunk, kmpc_task_t task)
{
not_implemented() ;
return NULL ;
}

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

kmp_int32
__kmpc_omp_task( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
not_implemented() ;
return 0 ;
}

kmp_task_t*
__kmpc_omp_task_alloc( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 flags,
                       size_t sizeof_kmp_task_t, size_t sizeof_shareds,
                       kmp_routine_entry_t task_entry )
{
not_implemented() ;
return NULL ;
}

void
__kmpc_omp_task_begin_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void
__kmpc_omp_task_complete_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}

kmp_int32
__kmpc_omp_task_parts( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskwait( ident_t *loc_ref, kmp_int32 gtid )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskyield( ident_t *loc_ref, kmp_int32 gtid, int end_part )
{
not_implemented() ;
return 0 ;
}

/* Following 2 functions should be enclosed by "if TASK_UNUSED" */

void 
__kmpc_omp_task_begin( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void 
__kmpc_omp_task_complete( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}


/* FOR OPENMP 4 */

void 
__kmpc_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

void 
__kmpc_end_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

kmp_int32 
__kmpc_omp_task_with_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task,
    kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_omp_wait_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
}

void 
__kmp_release_deps ( kmp_int32 gtid, kmp_taskdata_t *task )
{
not_implemented() ;
}







/********************************
  * OTHERS FROM KMP.H
  *******************************/

void *
kmpc_malloc( size_t size )
{
not_implemented() ;
return NULL ;
}

void *
kmpc_calloc( size_t nelem, size_t elsize )
{
not_implemented() ;
return NULL ;
}

void *
kmpc_realloc( void *ptr, size_t size )
{
not_implemented() ;
return NULL ;
}

void  kmpc_free( void *ptr )
{
not_implemented() ;
}

int 
__kmpc_invoke_task_func( int gtid )
{
not_implemented() ;
return 0 ;
}

void 
kmpc_set_blocktime (int arg)
{
not_implemented() ;
}

#if 0
void
omp_init_lock( mpcomp_lock_t * lock)
{
not_implemented() ;
}
#endif

/*
 * Lock interface routines (fast versions with gtid passed in)
 */
void 
__kmpc_init_lock( ident_t *loc, kmp_int32 gtid,  void **user_lock )
{
  sctk_debug( "[%d] __kmpc_init_lock: "
      "Addr %p",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, user_lock ) ;

  not_implemented() ;
}

void 
__kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_destroy_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_destroy_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_set_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_set_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_unset_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_unset_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

int 
__kmpc_test_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
  return 0 ;
}

int 
__kmpc_test_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
  return 0 ;
}










/********************************
  * ATOMIC_OPERATIONS
  *******************************/
#if __MIC__ || __MIC2__
    #define DO_PAUSE _mm_delay_32( 1 )
#else
    #define DO_PAUSE __kmp_x86_pause()
#endif

/* ------------------------------------------------------------------------ */
/* Generic atomic routines                                                  */
/* ------------------------------------------------------------------------ */
void
__kmpc_atomic_4( ident_t *id_ref, int gtid, void* lhs, void* rhs, void (*f)( void *, void *, void * ) )
{
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    if (
#if defined __i386 || defined __x86_64
    TRUE                    /* no alignment problems */
#else
    ! ( (kmp_uintptr_t) lhs & 0x3)      /* make sure address is 4-byte aligned */
#endif
    )
    {
        kmp_int32 old_value, new_value;
        old_value = *(kmp_int32 *) lhs;
        (*f)( &new_value, &old_value, rhs );

        /* TODO: Should this be acquire or release? */
        while ( ! __kmp_compare_and_store32 ( (kmp_int32 *) lhs,
                *(kmp_int32 *) &old_value, *(kmp_int32 *) &new_value ) )
        {
            DO_PAUSE;
            old_value = *(kmp_int32 *) lhs;
            (*f)( &new_value, &old_value, rhs );
        }
        return;
    }
    else 
    {
        sctk_thread_mutex_lock( &lock);
        (*f)( lhs, lhs, rhs );
        sctk_thread_mutex_unlock( &lock);
    }
}

void 
__kmpc_atomic_fixed4_wr(  ident_t *id_ref, int gtid, kmp_int32   * lhs, kmp_int32   rhs ) 
{
      sctk_nodebug( "[%d] __kmpc_atomic_fixed4_wr: "
            "Write %d",
                  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

        __kmp_xchg_fixed32( lhs, rhs ) ;

#if 0
  __mpcomp_atomic_begin() ;

    *lhs = rhs ;

      __mpcomp_atomic_end() ;

        /* TODO: use assembly functions by Intel if available */
#endif
}

void 
__kmpc_atomic_float8_add(  ident_t *id_ref, int gtid, kmp_real64 * lhs, kmp_real64 rhs )
{
      sctk_nodebug( "[%d] (ASM) __kmpc_atomic_float8_add: "
            "Add %g",
                  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

#if 0
  __kmp_test_then_add_real64( lhs, rhs ) ;
#endif

    /* TODO check how we can add this function to asssembly-dedicated module */

    /* TODO use MPCOMP_MIC */

#if __MIC__ || __MIC2__
#warning "MIC => atomic locks"
  __mpcomp_atomic_begin() ;

    *lhs += rhs ;

      __mpcomp_atomic_end() ;
#else 
#warning "NON MIC => atomic optim"
        __kmp_test_then_add_real64( lhs, rhs ) ;
#endif


          /* TODO: use assembly functions by Intel if available */
}

/* begins for atomic functions */
#define ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE, RET_TYPE)                                                          \
RET_TYPE __kmpc_atomic_##TYPE_ID##_##OP_ID( ident_t *id_ref, int gtid, TYPE * lhs, TYPE rhs )               \
{

#define ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE, RET_TYPE)                                                      \
RET_TYPE __kmpc_atomic_##TYPE_ID##_##OP_ID( ident_t *id_ref, int gtid, TYPE * lhs, TYPE rhs, int flag )     \
{

#define ATOMIC_BEGIN_REV(TYPE_ID,OP_ID,TYPE, RET_TYPE)                                                      \
RET_TYPE __kmpc_atomic_##TYPE_ID##_##OP_ID##_rev( ident_t *id_ref, int gtid, TYPE * lhs, TYPE rhs )         \
{                                   

#define ATOMIC_BEGIN_MIX(TYPE_ID,TYPE,OP_ID,RTYPE_ID,RTYPE)                                                 \
void __kmpc_atomic_##TYPE_ID##_##OP_ID##_##RTYPE_ID( ident_t *id_ref, int gtid, TYPE * lhs, RTYPE rhs )     \
{

#define ATOMIC_BEGIN_READ(TYPE_ID,OP_ID,TYPE, RET_TYPE)                                                     \
RET_TYPE __kmpc_atomic_##TYPE_ID##_##OP_ID( ident_t *id_ref, int gtid, TYPE * loc )                         \
{



/* atomic functions */
#define ATOMIC_FIXED_ADD(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                  \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    __kmp_test_then_add##BITS( (lhs), (+ rhs) );                                                            \
}

#define ATOMIC_CMPXCHG(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                    \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    TYPE old_value, new_value;                                                                              \
    old_value = *(TYPE volatile *) lhs;                                                                     \
    new_value = old_value OP rhs;                                                                           \
    while ( ! __kmp_compare_and_store##BITS( (kmp_int##BITS *) lhs,                                         \
                                          *(volatile kmp_int##BITS *) &old_value,                           \
                                          *(volatile kmp_int##BITS *) &new_value ))                         \
    {                                                                                                       \
        DO_PAUSE;                                                                                           \
        old_value = *(TYPE volatile *)lhs;                                                                  \
        new_value = old_value OP rhs;                                                                       \
    }                                                                                                       \
}

#define ATOMIC_CMPX_L(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                     \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    TYPE old_value, new_value;                                                                              \
    old_value = *(TYPE volatile *)lhs;                                                                      \
    new_value = old_value OP rhs;                                                                           \
    while ( ! __kmp_compare_and_store##BITS( ((kmp_int##BITS *) lhs),                                       \
    (*(kmp_int##BITS *) &old_value), (*(kmp_int##BITS *) &new_value) ) )                                    \
    {                                                                                                       \
        DO_PAUSE;                                                                                           \
        old_value = *(TYPE volatile *)lhs;                                                                  \
        new_value = old_value OP rhs;                                                                       \
    }                                                                                                       \
}


#define MIN_MAX_COMPXCHG(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                  \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    if ( *lhs OP rhs )                                                                                      \
    {                                                                                                       \
        TYPE volatile temp_val;                                                                             \
        TYPE old_value;                                                                                     \
        temp_val = *lhs;                                                                                    \
        old_value = temp_val;                                                                               \
        while ( old_value OP rhs &&                                                                         \
                ! __kmp_compare_and_store##BITS( ((kmp_int##BITS *) lhs),                                   \
                                             (*(kmp_int##BITS *) &old_value),                               \
                                             (*(kmp_int##BITS *) &rhs)                                      \
                                           )                                                                \
              )                                                                                             \
        {                                                                                                   \
            DO_PAUSE;                                                                                       \
            temp_val = *lhs;                                                                                \
            old_value = temp_val;                                                                           \
        }                                                                                                   \
    }                                                                                                       \
}

#define ATOMIC_CMPX_EQV(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                   \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    TYPE old_value, new_value;                                                                              \
    old_value = *(TYPE volatile *)lhs;                                                                      \
    new_value = old_value OP rhs;                                                                           \
    while ( ! __kmp_compare_and_store##BITS( ((kmp_int##BITS *) lhs),                                       \
    (*(kmp_int##BITS *) &old_value), (*(kmp_int##BITS *) &new_value) ) )                                    \
    {                                                                                                       \
        DO_PAUSE;                                                                                           \
        old_value = *(TYPE volatile *)lhs;                                                                  \
        new_value = old_value OP rhs;                                                                       \
    }                                                                                                       \
}                                                                                                           \

#define ATOMIC_CRITICAL(TYPE_ID,OP_ID,TYPE,OP,LCK_ID,GOMP_FLAG)                                             \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    static sctk_thread_mutex_t critical_lock = SCTK_THREAD_MUTEX_INITIALIZER;                               \
    sctk_thread_mutex_lock(&critical_lock);                                                                 \
    (*lhs) OP##= (rhs);                                                                                     \
    sctk_thread_mutex_unlock(&critical_lock);                                                               \
}


#define MIN_MAX_COMPXCHG_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                          \
ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE,TYPE)                                                                   \
    TYPE new_value, old_value;                                                                              \
    if ( *lhs OP rhs )                                                                                      \
    {                                                                                                       \
        TYPE volatile temp_val;                                                                             \
        temp_val = *lhs;                                                                                    \
        old_value = temp_val;                                                                               \
        while ( old_value OP rhs &&                                                                         \
        ! __kmp_compare_and_store##BITS( ((kmp_int##BITS *) lhs),                                           \
        (*(kmp_int##BITS *) &old_value), (*(kmp_int##BITS *) &rhs) ) )                                      \
        {                                                                                                   \
            DO_PAUSE;                                                                                       \
            temp_val = *lhs;                                                                                \
            old_value = temp_val;                                                                           \
        }                                                                                                   \
        if( flag )                                                                                          \
            return rhs;                                                                                     \
        else                                                                                                \
            return old_value;                                                                               \
    }                                                                                                       \
    return *lhs;                                                                                            \
}

#define ATOMIC_CMPXCHG_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                            \
ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE,TYPE)                                                                   \
    TYPE volatile temp_val;                                                                                 \
    TYPE old_value, new_value;                                                                              \
    temp_val = *lhs;                                                                                        \
    old_value = temp_val;                                                                                   \
    new_value = old_value OP rhs;                                                                           \
    while ( ! __kmp_compare_and_store##BITS( ((kmp_int##BITS *) lhs),                                       \
    (*(kmp_int##BITS *) &old_value), (*(kmp_int##BITS *) &new_value) ) )                                    \
    {                                                                                                       \
        DO_PAUSE;                                                                                           \
        temp_val = *lhs;                                                                                    \
        old_value = temp_val;                                                                               \
        new_value = old_value OP rhs;                                                                       \
    }                                                                                                       \
    if( flag ) { return new_value; } else return old_value;                                                 \
}

#define ATOMIC_CMPX_EQV_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                           \
ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE,TYPE)                                                                   \
    TYPE volatile temp_val;                                                                                 \
    TYPE old_value, new_value;                                                                              \
    temp_val = *lhs;                                                                                        \
    old_value = temp_val;                                                                                   \
    new_value = old_value OP rhs;                                                                           \
    while ( ! __kmp_compare_and_store##BITS( ((kmp_int##BITS *) lhs),                                       \
    (*(kmp_int##BITS *) &old_value), (*(kmp_int##BITS *) &new_value) ) )                                    \
    {                                                                                                       \
        DO_PAUSE;                                                                                           \
        temp_val = *lhs;                                                                                    \
        old_value = temp_val;                                                                               \
        new_value = old_value OP rhs;                                                                       \
    }                                                                                                       \
    if( flag ) { return new_value; } else return old_value;                                                 \
}


#define ATOMIC_CRITICAL_CPT(TYPE_ID,OP_ID,TYPE,OP,LCK_ID,GOMP_FLAG)                                         \
ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE,TYPE)                                                                   \
    static sctk_thread_mutex_t critical_lock = SCTK_THREAD_MUTEX_INITIALIZER;                               \
    TYPE new_value;                                                                                         \
    sctk_thread_mutex_lock(&critical_lock);                                                                 \
    if( flag ) {                                                                                            \
        (*lhs) OP##= rhs; new_value = (*lhs);                                                               \
    } else {                                                                                                \
        new_value = (*lhs); (*lhs) OP##= rhs;                                                               \
    }                                                                                                       \
    sctk_thread_mutex_unlock(&critical_lock);                                                               \
    return new_value;                                                                                       \
}

#define ATOMIC_CMPXCHG_REV(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,GOMP_FLAG)                                     \
ATOMIC_BEGIN_REV(TYPE_ID,OP_ID,TYPE,void)                                                                   \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPXCHG_MIX(TYPE_ID,TYPE,OP_ID,BITS,OP,RTYPE_ID,RTYPE,LCK_ID,MASK,GOMP_FLAG)                 \
ATOMIC_BEGIN_MIX(TYPE_ID,TYPE,OP_ID,RTYPE_ID,RTYPE)                                                         \
    not_implemented();                                                                                      \
}

#define ATOMIC_FIXED_READ(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                             \
ATOMIC_BEGIN_READ(TYPE_ID,OP_ID,TYPE,TYPE)                                                                  \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPXCHG_READ(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                           \
ATOMIC_BEGIN_READ(TYPE_ID,OP_ID,TYPE,TYPE)                                                                  \
    not_implemented();                                                                                      \
}

#define ATOMIC_CRITICAL_READ(TYPE_ID,OP_ID,TYPE,OP,LCK_ID,GOMP_FLAG)                                        \
ATOMIC_BEGIN_READ(TYPE_ID,OP_ID,TYPE,TYPE)                                                                  \
    not_implemented();                                                                                      \
}

#define ATOMIC_XCHG_WR(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                                \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_XCHG_FLOAT_WR(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                          \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPXCHG_WR(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                             \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CRITICAL_WR(TYPE_ID,OP_ID,TYPE,OP,LCK_ID,GOMP_FLAG)                                          \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_FIXED_ADD_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                          \
ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE,TYPE)                                                                   \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPX_L_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                             \
ATOMIC_BEGIN_CPT(TYPE_ID,OP_ID,TYPE,TYPE)                                                                   \
    not_implemented();                                                                                      \
}



// Routines for ATOMIC 4-byte operands addition and subtraction
ATOMIC_FIXED_ADD( fixed4, add, kmp_int32,  32, +, 4i, 3, 0            )  // __kmpc_atomic_fixed4_add
ATOMIC_FIXED_ADD( fixed4, sub, kmp_int32,  32, -, 4i, 3, 0            )  // __kmpc_atomic_fixed4_sub

ATOMIC_CMPXCHG( float4,  add, kmp_real32, 32, +,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_add
ATOMIC_CMPXCHG( float4,  sub, kmp_real32, 32, -,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_sub

// Routines for ATOMIC 8-byte operands addition and subtraction
ATOMIC_FIXED_ADD( fixed8, add, kmp_int64,  64, +, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_add
ATOMIC_FIXED_ADD( fixed8, sub, kmp_int64,  64, -, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_sub

//ATOMIC_CMPXCHG( float8,  add, kmp_real64, 64, +,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_add
ATOMIC_CMPXCHG( float8,  sub, kmp_real64, 64, -,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_sub

// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//              TYPE_ID,OP_ID, TYPE,          OP, LCK_ID, GOMP_FLAG
ATOMIC_CMPXCHG( fixed1,  add, kmp_int8,    8, +,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_add
ATOMIC_CMPXCHG( fixed1, andb, kmp_int8,    8, &,  1i, 0, 0            )  // __kmpc_atomic_fixed1_andb
ATOMIC_CMPXCHG( fixed1,  div, kmp_int8,    8, /,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_div
ATOMIC_CMPXCHG( fixed1u, div, kmp_uint8,   8, /,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_div
ATOMIC_CMPXCHG( fixed1,  mul, kmp_int8,    8, *,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_mul
ATOMIC_CMPXCHG( fixed1,  orb, kmp_int8,    8, |,  1i, 0, 0            )  // __kmpc_atomic_fixed1_orb
ATOMIC_CMPXCHG( fixed1,  shl, kmp_int8,    8, <<, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shl
ATOMIC_CMPXCHG( fixed1,  shr, kmp_int8,    8, >>, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shr
ATOMIC_CMPXCHG( fixed1u, shr, kmp_uint8,   8, >>, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_shr
ATOMIC_CMPXCHG( fixed1,  sub, kmp_int8,    8, -,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_sub
ATOMIC_CMPXCHG( fixed1,  xor, kmp_int8,    8, ^,  1i, 0, 0            )  // __kmpc_atomic_fixed1_xor
ATOMIC_CMPXCHG( fixed2,  add, kmp_int16,  16, +,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_add
ATOMIC_CMPXCHG( fixed2, andb, kmp_int16,  16, &,  2i, 1, 0            )  // __kmpc_atomic_fixed2_andb
ATOMIC_CMPXCHG( fixed2,  div, kmp_int16,  16, /,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_div
ATOMIC_CMPXCHG( fixed2u, div, kmp_uint16, 16, /,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_div
ATOMIC_CMPXCHG( fixed2,  mul, kmp_int16,  16, *,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_mul
ATOMIC_CMPXCHG( fixed2,  orb, kmp_int16,  16, |,  2i, 1, 0            )  // __kmpc_atomic_fixed2_orb
ATOMIC_CMPXCHG( fixed2,  shl, kmp_int16,  16, <<, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shl
ATOMIC_CMPXCHG( fixed2,  shr, kmp_int16,  16, >>, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shr
ATOMIC_CMPXCHG( fixed2u, shr, kmp_uint16, 16, >>, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_shr
ATOMIC_CMPXCHG( fixed2,  sub, kmp_int16,  16, -,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_sub
ATOMIC_CMPXCHG( fixed2,  xor, kmp_int16,  16, ^,  2i, 1, 0            )  // __kmpc_atomic_fixed2_xor
ATOMIC_CMPXCHG( fixed4, andb, kmp_int32,  32, &,  4i, 3, 0            )  // __kmpc_atomic_fixed4_andb
ATOMIC_CMPXCHG( fixed4,  div, kmp_int32,  32, /,  4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_div
ATOMIC_CMPXCHG( fixed4u, div, kmp_uint32, 32, /,  4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_div
ATOMIC_CMPXCHG( fixed4,  mul, kmp_int32,  32, *,  4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_mul
ATOMIC_CMPXCHG( fixed4,  orb, kmp_int32,  32, |,  4i, 3, 0            )  // __kmpc_atomic_fixed4_orb
ATOMIC_CMPXCHG( fixed4,  shl, kmp_int32,  32, <<, 4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shl
ATOMIC_CMPXCHG( fixed4,  shr, kmp_int32,  32, >>, 4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shr
ATOMIC_CMPXCHG( fixed4u, shr, kmp_uint32, 32, >>, 4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_shr
ATOMIC_CMPXCHG( fixed4,  xor, kmp_int32,  32, ^,  4i, 3, 0            )  // __kmpc_atomic_fixed4_xor
ATOMIC_CMPXCHG( fixed8, andb, kmp_int64,  64, &,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_andb
ATOMIC_CMPXCHG( fixed8,  div, kmp_int64,  64, /,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_div
ATOMIC_CMPXCHG( fixed8u, div, kmp_uint64, 64, /,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_div
ATOMIC_CMPXCHG( fixed8,  mul, kmp_int64,  64, *,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_mul
ATOMIC_CMPXCHG( fixed8,  orb, kmp_int64,  64, |,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_orb
ATOMIC_CMPXCHG( fixed8,  shl, kmp_int64,  64, <<, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shl
ATOMIC_CMPXCHG( fixed8,  shr, kmp_int64,  64, >>, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shr
ATOMIC_CMPXCHG( fixed8u, shr, kmp_uint64, 64, >>, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_shr
ATOMIC_CMPXCHG( fixed8,  xor, kmp_int64,  64, ^,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_xor
ATOMIC_CMPXCHG( float4,  div, kmp_real32, 32, /,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_div
ATOMIC_CMPXCHG( float4,  mul, kmp_real32, 32, *,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_mul
ATOMIC_CMPXCHG( float8,  div, kmp_real64, 64, /,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_div
ATOMIC_CMPXCHG( float8,  mul, kmp_real64, 64, *,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_mul
//              TYPE_ID,OP_ID, TYPE,          OP, LCK_ID, GOMP_FLAG

/* ------------------------------------------------------------------------ */
/* Routines for C/C++ Reduction operators && and ||                         */
/* ------------------------------------------------------------------------ */

ATOMIC_CMPX_L( fixed1, andl, char,       8, &&, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_andl
ATOMIC_CMPX_L( fixed1,  orl, char,       8, ||, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_orl
ATOMIC_CMPX_L( fixed2, andl, short,     16, &&, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_andl
ATOMIC_CMPX_L( fixed2,  orl, short,     16, ||, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_orl
ATOMIC_CMPX_L( fixed4, andl, kmp_int32, 32, &&, 4i, 3, 0 )             // __kmpc_atomic_fixed4_andl
ATOMIC_CMPX_L( fixed4,  orl, kmp_int32, 32, ||, 4i, 3, 0 )             // __kmpc_atomic_fixed4_orl
ATOMIC_CMPX_L( fixed8, andl, kmp_int64, 64, &&, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_andl
ATOMIC_CMPX_L( fixed8,  orl, kmp_int64, 64, ||, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_orl

/* ------------------------------------------------------------------------- */
/* Routines for Fortran operators that matched no one in C:                  */
/* MAX, MIN, .EQV., .NEQV.                                                   */
/* Operators .AND., .OR. are covered by __kmpc_atomic_*_{andl,orl}           */
/* Intrinsics IAND, IOR, IEOR are covered by __kmpc_atomic_*_{andb,orb,xor}  */
/* ------------------------------------------------------------------------- */

MIN_MAX_COMPXCHG( fixed1,  max, char,        8, <, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_max
MIN_MAX_COMPXCHG( fixed1,  min, char,        8, >, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_min
MIN_MAX_COMPXCHG( fixed2,  max, short,      16, <, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_max
MIN_MAX_COMPXCHG( fixed2,  min, short,      16, >, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_min
MIN_MAX_COMPXCHG( fixed4,  max, kmp_int32,  32, <, 4i, 3, 0 )            // __kmpc_atomic_fixed4_max
MIN_MAX_COMPXCHG( fixed4,  min, kmp_int32,  32, >, 4i, 3, 0 )            // __kmpc_atomic_fixed4_min
MIN_MAX_COMPXCHG( fixed8,  max, kmp_int64,  64, <, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_max
MIN_MAX_COMPXCHG( fixed8,  min, kmp_int64,  64, >, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_min
MIN_MAX_COMPXCHG( float4,  max, kmp_real32, 32, <, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_max
MIN_MAX_COMPXCHG( float4,  min, kmp_real32, 32, >, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_min
MIN_MAX_COMPXCHG( float8,  max, kmp_real64, 64, <, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_max
MIN_MAX_COMPXCHG( float8,  min, kmp_real64, 64, >, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_min


ATOMIC_CMPXCHG(  fixed1, neqv, kmp_int8,   8,   ^, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_neqv
ATOMIC_CMPXCHG(  fixed2, neqv, kmp_int16, 16,   ^, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_neqv
ATOMIC_CMPXCHG(  fixed4, neqv, kmp_int32, 32,   ^, 4i, 3, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_neqv
ATOMIC_CMPXCHG(  fixed8, neqv, kmp_int64, 64,   ^, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_neqv
ATOMIC_CMPX_EQV( fixed1, eqv,  kmp_int8,   8,  ^~, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_eqv
ATOMIC_CMPX_EQV( fixed2, eqv,  kmp_int16, 16,  ^~, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_eqv
ATOMIC_CMPX_EQV( fixed4, eqv,  kmp_int32, 32,  ^~, 4i, 3, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_eqv
ATOMIC_CMPX_EQV( fixed8, eqv,  kmp_int64, 64,  ^~, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_eqv

// ------------------------------------------------------------------------
// Routines for Extended types: long double, _Quad, complex flavours (use critical section)
//     TYPE_ID, OP_ID, TYPE - detailed above
//     OP      - operator
//     LCK_ID  - lock identifier, used to possibly distinguish lock variable

/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL( float10, add, long double,     +, 10r,   1 )            // __kmpc_atomic_float10_add
ATOMIC_CRITICAL( float10, sub, long double,     -, 10r,   1 )            // __kmpc_atomic_float10_sub
ATOMIC_CRITICAL( float10, mul, long double,     *, 10r,   1 )            // __kmpc_atomic_float10_mul
ATOMIC_CRITICAL( float10, div, long double,     /, 10r,   1 )            // __kmpc_atomic_float10_div
#if 0
#if KMP_HAVE_QUAD
// routines for _Quad type
ATOMIC_CRITICAL( float16, add, QUAD_LEGACY,     +, 16r,   1 )            // __kmpc_atomic_float16_add
ATOMIC_CRITICAL( float16, sub, QUAD_LEGACY,     -, 16r,   1 )            // __kmpc_atomic_float16_sub
ATOMIC_CRITICAL( float16, mul, QUAD_LEGACY,     *, 16r,   1 )            // __kmpc_atomic_float16_mul
ATOMIC_CRITICAL( float16, div, QUAD_LEGACY,     /, 16r,   1 )            // __kmpc_atomic_float16_div
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL( float16, add_a16, Quad_a16_t, +, 16r, 1 )           // __kmpc_atomic_float16_add_a16
    ATOMIC_CRITICAL( float16, sub_a16, Quad_a16_t, -, 16r, 1 )           // __kmpc_atomic_float16_sub_a16
    ATOMIC_CRITICAL( float16, mul_a16, Quad_a16_t, *, 16r, 1 )           // __kmpc_atomic_float16_mul_a16
    ATOMIC_CRITICAL( float16, div_a16, Quad_a16_t, /, 16r, 1 )           // __kmpc_atomic_float16_div_a16
#endif
#endif

/* routines for complex */
#if USE_CMPXCHG_FIX
// workaround for C78287 (complex(kind=4) data type)
ATOMIC_CMPXCHG_WORKAROUND( cmplx4, add, kmp_cmplx32, 64, +, 8c, 7, 1 )   // __kmpc_atomic_cmplx4_add
ATOMIC_CMPXCHG_WORKAROUND( cmplx4, sub, kmp_cmplx32, 64, -, 8c, 7, 1 )   // __kmpc_atomic_cmplx4_sub
ATOMIC_CMPXCHG_WORKAROUND( cmplx4, mul, kmp_cmplx32, 64, *, 8c, 7, 1 )   // __kmpc_atomic_cmplx4_mul
ATOMIC_CMPXCHG_WORKAROUND( cmplx4, div, kmp_cmplx32, 64, /, 8c, 7, 1 )   // __kmpc_atomic_cmplx4_div
// end of the workaround for C78287
#else
ATOMIC_CRITICAL( cmplx4,  add, kmp_cmplx32,     +,  8c,   1 )            // __kmpc_atomic_cmplx4_add
ATOMIC_CRITICAL( cmplx4,  sub, kmp_cmplx32,     -,  8c,   1 )            // __kmpc_atomic_cmplx4_sub
ATOMIC_CRITICAL( cmplx4,  mul, kmp_cmplx32,     *,  8c,   1 )            // __kmpc_atomic_cmplx4_mul
ATOMIC_CRITICAL( cmplx4,  div, kmp_cmplx32,     /,  8c,   1 )            // __kmpc_atomic_cmplx4_div
#endif // USE_CMPXCHG_FIX

ATOMIC_CRITICAL( cmplx8,  add, kmp_cmplx64,     +, 16c,   1 )            // __kmpc_atomic_cmplx8_add
ATOMIC_CRITICAL( cmplx8,  sub, kmp_cmplx64,     -, 16c,   1 )            // __kmpc_atomic_cmplx8_sub
ATOMIC_CRITICAL( cmplx8,  mul, kmp_cmplx64,     *, 16c,   1 )            // __kmpc_atomic_cmplx8_mul
ATOMIC_CRITICAL( cmplx8,  div, kmp_cmplx64,     /, 16c,   1 )            // __kmpc_atomic_cmplx8_div
ATOMIC_CRITICAL( cmplx10, add, kmp_cmplx80,     +, 20c,   1 )            // __kmpc_atomic_cmplx10_add
ATOMIC_CRITICAL( cmplx10, sub, kmp_cmplx80,     -, 20c,   1 )            // __kmpc_atomic_cmplx10_sub
ATOMIC_CRITICAL( cmplx10, mul, kmp_cmplx80,     *, 20c,   1 )            // __kmpc_atomic_cmplx10_mul
ATOMIC_CRITICAL( cmplx10, div, kmp_cmplx80,     /, 20c,   1 )            // __kmpc_atomic_cmplx10_div
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL( cmplx16, add, CPLX128_LEG,     +, 32c,   1 )            // __kmpc_atomic_cmplx16_add
ATOMIC_CRITICAL( cmplx16, sub, CPLX128_LEG,     -, 32c,   1 )            // __kmpc_atomic_cmplx16_sub
ATOMIC_CRITICAL( cmplx16, mul, CPLX128_LEG,     *, 32c,   1 )            // __kmpc_atomic_cmplx16_mul
ATOMIC_CRITICAL( cmplx16, div, CPLX128_LEG,     /, 32c,   1 )            // __kmpc_atomic_cmplx16_div
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL( cmplx16, add_a16, kmp_cmplx128_a16_t, +, 32c, 1 )   // __kmpc_atomic_cmplx16_add_a16
    ATOMIC_CRITICAL( cmplx16, sub_a16, kmp_cmplx128_a16_t, -, 32c, 1 )   // __kmpc_atomic_cmplx16_sub_a16
    ATOMIC_CRITICAL( cmplx16, mul_a16, kmp_cmplx128_a16_t, *, 32c, 1 )   // __kmpc_atomic_cmplx16_mul_a16
    ATOMIC_CRITICAL( cmplx16, div_a16, kmp_cmplx128_a16_t, /, 32c, 1 )   // __kmpc_atomic_cmplx16_div_a16
#endif
#endif

#endif

// ------------------------------------------------------------------------
// Entries definition for integer operands
//     TYPE_ID - operands type and size (fixed4, float4)
//     OP_ID   - operation identifier (add, sub, mul, ...)
//     TYPE    - operand type
//     BITS    - size in bits, used to distinguish low level calls
//     OP      - operator (used in critical section)
//     LCK_ID  - lock identifier, used to possibly distinguish lock variable

//               TYPE_ID,OP_ID,  TYPE,   BITS,OP,LCK_ID,GOMP_FLAG
// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//                  TYPE_ID,OP_ID, TYPE,    BITS, OP, LCK_ID, GOMP_FLAG
ATOMIC_CMPXCHG_REV( fixed1,  div, kmp_int8,    8, /,  1i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_div_rev
ATOMIC_CMPXCHG_REV( fixed1u, div, kmp_uint8,   8, /,  1i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_div_rev
ATOMIC_CMPXCHG_REV( fixed1,  shl, kmp_int8,    8, <<, 1i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shl_rev
ATOMIC_CMPXCHG_REV( fixed1,  shr, kmp_int8,    8, >>, 1i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shr_rev
ATOMIC_CMPXCHG_REV( fixed1u, shr, kmp_uint8,   8, >>, 1i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_shr_rev
ATOMIC_CMPXCHG_REV( fixed1,  sub, kmp_int8,    8, -,  1i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_sub_rev

ATOMIC_CMPXCHG_REV( fixed2,  div, kmp_int16,  16, /,  2i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_div_rev
ATOMIC_CMPXCHG_REV( fixed2u, div, kmp_uint16, 16, /,  2i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_div_rev
ATOMIC_CMPXCHG_REV( fixed2,  shl, kmp_int16,  16, <<, 2i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shl_rev
ATOMIC_CMPXCHG_REV( fixed2,  shr, kmp_int16,  16, >>, 2i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shr_rev
ATOMIC_CMPXCHG_REV( fixed2u, shr, kmp_uint16, 16, >>, 2i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_shr_rev
ATOMIC_CMPXCHG_REV( fixed2,  sub, kmp_int16,  16, -,  2i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_sub_rev

ATOMIC_CMPXCHG_REV( fixed4,  div, kmp_int32,  32, /,  4i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_div_rev
ATOMIC_CMPXCHG_REV( fixed4u, div, kmp_uint32, 32, /,  4i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_div_rev
ATOMIC_CMPXCHG_REV( fixed4,  shl, kmp_int32,  32, <<, 4i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shl_rev
ATOMIC_CMPXCHG_REV( fixed4,  shr, kmp_int32,  32, >>, 4i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shr_rev
ATOMIC_CMPXCHG_REV( fixed4u, shr, kmp_uint32, 32, >>, 4i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_shr_rev
ATOMIC_CMPXCHG_REV( fixed4,  sub, kmp_int32,  32, -,  4i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_sub_rev

ATOMIC_CMPXCHG_REV( fixed8,  div, kmp_int64,  64, /,  8i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_div_rev
ATOMIC_CMPXCHG_REV( fixed8u, div, kmp_uint64, 64, /,  8i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_div_rev
ATOMIC_CMPXCHG_REV( fixed8,  shl, kmp_int64,  64, <<, 8i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shl_rev
ATOMIC_CMPXCHG_REV( fixed8,  shr, kmp_int64,  64, >>, 8i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shr_rev
ATOMIC_CMPXCHG_REV( fixed8u, shr, kmp_uint64, 64, >>, 8i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_shr_rev
ATOMIC_CMPXCHG_REV( fixed8,  sub, kmp_int64,  64, -,  8i, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_sub_rev

ATOMIC_CMPXCHG_REV( float4,  div, kmp_real32, 32, /,  4r, KMP_ARCH_X86 )  // __kmpc_atomic_float4_div_rev
ATOMIC_CMPXCHG_REV( float4,  sub, kmp_real32, 32, -,  4r, KMP_ARCH_X86 )  // __kmpc_atomic_float4_sub_rev

ATOMIC_CMPXCHG_REV( float8,  div, kmp_real64, 64, /,  8r, KMP_ARCH_X86 )  // __kmpc_atomic_float8_div_rev
ATOMIC_CMPXCHG_REV( float8,  sub, kmp_real64, 64, -,  8r, KMP_ARCH_X86 )  // __kmpc_atomic_float8_sub_rev
//                  TYPE_ID,OP_ID, TYPE,     BITS,OP,LCK_ID, GOMP_FLAG

#if 0
/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL_REV( float10, sub, long double,     -, 10r,   1 )            // __kmpc_atomic_float10_sub_rev
ATOMIC_CRITICAL_REV( float10, div, long double,     /, 10r,   1 )            // __kmpc_atomic_float10_div_rev
#if KMP_HAVE_QUAD
// routines for _Quad type
ATOMIC_CRITICAL_REV( float16, sub, QUAD_LEGACY,     -, 16r,   1 )            // __kmpc_atomic_float16_sub_rev
ATOMIC_CRITICAL_REV( float16, div, QUAD_LEGACY,     /, 16r,   1 )            // __kmpc_atomic_float16_div_rev
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL_REV( float16, sub_a16, Quad_a16_t, -, 16r, 1 )           // __kmpc_atomic_float16_sub_a16_rev
    ATOMIC_CRITICAL_REV( float16, div_a16, Quad_a16_t, /, 16r, 1 )           // __kmpc_atomic_float16_div_a16_rev
#endif
#endif

// routines for complex types
ATOMIC_CRITICAL_REV( cmplx4,  sub, kmp_cmplx32,     -, 8c,    1 )            // __kmpc_atomic_cmplx4_sub_rev
ATOMIC_CRITICAL_REV( cmplx4,  div, kmp_cmplx32,     /, 8c,    1 )            // __kmpc_atomic_cmplx4_div_rev
ATOMIC_CRITICAL_REV( cmplx8,  sub, kmp_cmplx64,     -, 16c,   1 )            // __kmpc_atomic_cmplx8_sub_rev
ATOMIC_CRITICAL_REV( cmplx8,  div, kmp_cmplx64,     /, 16c,   1 )            // __kmpc_atomic_cmplx8_div_rev
ATOMIC_CRITICAL_REV( cmplx10, sub, kmp_cmplx80,     -, 20c,   1 )            // __kmpc_atomic_cmplx10_sub_rev
ATOMIC_CRITICAL_REV( cmplx10, div, kmp_cmplx80,     /, 20c,   1 )            // __kmpc_atomic_cmplx10_div_rev
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_REV( cmplx16, sub, CPLX128_LEG,     -, 32c,   1 )            // __kmpc_atomic_cmplx16_sub_rev
ATOMIC_CRITICAL_REV( cmplx16, div, CPLX128_LEG,     /, 32c,   1 )            // __kmpc_atomic_cmplx16_div_rev
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL_REV( cmplx16, sub_a16, kmp_cmplx128_a16_t, -, 32c, 1 )   // __kmpc_atomic_cmplx16_sub_a16_rev
    ATOMIC_CRITICAL_REV( cmplx16, div_a16, kmp_cmplx128_a16_t, /, 32c, 1 )   // __kmpc_atomic_cmplx16_div_a16_rev
#endif
#endif
#endif

/* ------------------------------------------------------------------------ */
/* Routines for mixed types of LHS and RHS, when RHS is "larger"            */
/* Note: in order to reduce the total number of types combinations          */
/*       it is supposed that compiler converts RHS to longest floating type,*/
/*       that is _Quad, before call to any of these routines                */
/* Conversion to _Quad will be done by the compiler during calculation,     */
/*    conversion back to TYPE - before the assignment, like:                */
/*    *lhs = (TYPE)( (_Quad)(*lhs) OP rhs )                                 */
/* Performance penalty expected because of SW emulation use                 */
/* ------------------------------------------------------------------------ */
// RHS=float8
ATOMIC_CMPXCHG_MIX( fixed1, char,       mul,  8, *, float8, kmp_real64, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_mul_float8
ATOMIC_CMPXCHG_MIX( fixed1, char,       div,  8, /, float8, kmp_real64, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_div_float8
ATOMIC_CMPXCHG_MIX( fixed2, short,      mul, 16, *, float8, kmp_real64, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_mul_float8
ATOMIC_CMPXCHG_MIX( fixed2, short,      div, 16, /, float8, kmp_real64, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_div_float8
ATOMIC_CMPXCHG_MIX( fixed4, kmp_int32,  mul, 32, *, float8, kmp_real64, 4i, 3, 0 )            // __kmpc_atomic_fixed4_mul_float8
ATOMIC_CMPXCHG_MIX( fixed4, kmp_int32,  div, 32, /, float8, kmp_real64, 4i, 3, 0 )            // __kmpc_atomic_fixed4_div_float8
ATOMIC_CMPXCHG_MIX( fixed8, kmp_int64,  mul, 64, *, float8, kmp_real64, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_mul_float8
ATOMIC_CMPXCHG_MIX( fixed8, kmp_int64,  div, 64, /, float8, kmp_real64, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_div_float8
ATOMIC_CMPXCHG_MIX( float4, kmp_real32, add, 32, +, float8, kmp_real64, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_add_float8
ATOMIC_CMPXCHG_MIX( float4, kmp_real32, sub, 32, -, float8, kmp_real64, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_sub_float8
ATOMIC_CMPXCHG_MIX( float4, kmp_real32, mul, 32, *, float8, kmp_real64, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_mul_float8
ATOMIC_CMPXCHG_MIX( float4, kmp_real32, div, 32, /, float8, kmp_real64, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_div_float8

#if 0
// RHS=float16 (deprecated, to be removed when we are sure the compiler does not use them)
#if KMP_HAVE_QUAD
ATOMIC_CMPXCHG_MIX( fixed1,  char,       add,  8, +, fp, _Quad, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_add_fp
ATOMIC_CMPXCHG_MIX( fixed1,  char,       sub,  8, -, fp, _Quad, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_sub_fp
ATOMIC_CMPXCHG_MIX( fixed1,  char,       mul,  8, *, fp, _Quad, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_mul_fp
ATOMIC_CMPXCHG_MIX( fixed1,  char,       div,  8, /, fp, _Quad, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_div_fp
ATOMIC_CMPXCHG_MIX( fixed1u, uchar,      div,  8, /, fp, _Quad, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1u_div_fp

ATOMIC_CMPXCHG_MIX( fixed2,  short,      add, 16, +, fp, _Quad, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_add_fp
ATOMIC_CMPXCHG_MIX( fixed2,  short,      sub, 16, -, fp, _Quad, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_sub_fp
ATOMIC_CMPXCHG_MIX( fixed2,  short,      mul, 16, *, fp, _Quad, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_mul_fp
ATOMIC_CMPXCHG_MIX( fixed2,  short,      div, 16, /, fp, _Quad, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_div_fp
ATOMIC_CMPXCHG_MIX( fixed2u, ushort,     div, 16, /, fp, _Quad, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2u_div_fp

ATOMIC_CMPXCHG_MIX( fixed4,  kmp_int32,  add, 32, +, fp, _Quad, 4i, 3, 0 )            // __kmpc_atomic_fixed4_add_fp
ATOMIC_CMPXCHG_MIX( fixed4,  kmp_int32,  sub, 32, -, fp, _Quad, 4i, 3, 0 )            // __kmpc_atomic_fixed4_sub_fp
ATOMIC_CMPXCHG_MIX( fixed4,  kmp_int32,  mul, 32, *, fp, _Quad, 4i, 3, 0 )            // __kmpc_atomic_fixed4_mul_fp
ATOMIC_CMPXCHG_MIX( fixed4,  kmp_int32,  div, 32, /, fp, _Quad, 4i, 3, 0 )            // __kmpc_atomic_fixed4_div_fp
ATOMIC_CMPXCHG_MIX( fixed4u, kmp_uint32, div, 32, /, fp, _Quad, 4i, 3, 0 )            // __kmpc_atomic_fixed4u_div_fp

ATOMIC_CMPXCHG_MIX( fixed8,  kmp_int64,  add, 64, +, fp, _Quad, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_add_fp
ATOMIC_CMPXCHG_MIX( fixed8,  kmp_int64,  sub, 64, -, fp, _Quad, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_sub_fp
ATOMIC_CMPXCHG_MIX( fixed8,  kmp_int64,  mul, 64, *, fp, _Quad, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_mul_fp
ATOMIC_CMPXCHG_MIX( fixed8,  kmp_int64,  div, 64, /, fp, _Quad, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_div_fp
ATOMIC_CMPXCHG_MIX( fixed8u, kmp_uint64, div, 64, /, fp, _Quad, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8u_div_fp

ATOMIC_CMPXCHG_MIX( float4,  kmp_real32, add, 32, +, fp, _Quad, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_add_fp
ATOMIC_CMPXCHG_MIX( float4,  kmp_real32, sub, 32, -, fp, _Quad, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_sub_fp
ATOMIC_CMPXCHG_MIX( float4,  kmp_real32, mul, 32, *, fp, _Quad, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_mul_fp
ATOMIC_CMPXCHG_MIX( float4,  kmp_real32, div, 32, /, fp, _Quad, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_div_fp

ATOMIC_CMPXCHG_MIX( float8,  kmp_real64, add, 64, +, fp, _Quad, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_add_fp
ATOMIC_CMPXCHG_MIX( float8,  kmp_real64, sub, 64, -, fp, _Quad, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_sub_fp
ATOMIC_CMPXCHG_MIX( float8,  kmp_real64, mul, 64, *, fp, _Quad, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_mul_fp
ATOMIC_CMPXCHG_MIX( float8,  kmp_real64, div, 64, /, fp, _Quad, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_div_fp

ATOMIC_CRITICAL_FP( float10, long double,    add, +, fp, _Quad, 10r,   1 )            // __kmpc_atomic_float10_add_fp
ATOMIC_CRITICAL_FP( float10, long double,    sub, -, fp, _Quad, 10r,   1 )            // __kmpc_atomic_float10_sub_fp
ATOMIC_CRITICAL_FP( float10, long double,    mul, *, fp, _Quad, 10r,   1 )            // __kmpc_atomic_float10_mul_fp
ATOMIC_CRITICAL_FP( float10, long double,    div, /, fp, _Quad, 10r,   1 )            // __kmpc_atomic_float10_div_fp
#endif

ATOMIC_CMPXCHG_CMPLX( cmplx4, kmp_cmplx32, add, 64, +, cmplx8,  kmp_cmplx64,  8c, 7, KMP_ARCH_X86 ) // __kmpc_atomic_cmplx4_add_cmplx8
ATOMIC_CMPXCHG_CMPLX( cmplx4, kmp_cmplx32, sub, 64, -, cmplx8,  kmp_cmplx64,  8c, 7, KMP_ARCH_X86 ) // __kmpc_atomic_cmplx4_sub_cmplx8
ATOMIC_CMPXCHG_CMPLX( cmplx4, kmp_cmplx32, mul, 64, *, cmplx8,  kmp_cmplx64,  8c, 7, KMP_ARCH_X86 ) // __kmpc_atomic_cmplx4_mul_cmplx8
ATOMIC_CMPXCHG_CMPLX( cmplx4, kmp_cmplx32, div, 64, /, cmplx8,  kmp_cmplx64,  8c, 7, KMP_ARCH_X86 ) // __kmpc_atomic_cmplx4_div_cmplx8
#endif

// READ, WRITE, CAPTURE are supported only on IA-32 architecture and Intel(R) 64
//////////////////////////////////////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------
// Atomic READ routines
// ------------------------------------------------------------------------
//                  TYPE_ID,OP_ID, TYPE,      OP, GOMP_FLAG
ATOMIC_FIXED_READ( fixed4, rd, kmp_int32,  32, +, 0            )      // __kmpc_atomic_fixed4_rd
ATOMIC_FIXED_READ( fixed8, rd, kmp_int64,  64, +, KMP_ARCH_X86 )      // __kmpc_atomic_fixed8_rd
ATOMIC_CMPXCHG_READ( float4, rd, kmp_real32, 32, +, KMP_ARCH_X86 )    // __kmpc_atomic_float4_rd
ATOMIC_CMPXCHG_READ( float8, rd, kmp_real64, 64, +, KMP_ARCH_X86 )    // __kmpc_atomic_float8_rd

// !!! TODO: Remove lock operations for "char" since it can't be non-atomic
ATOMIC_CMPXCHG_READ( fixed1,  rd, kmp_int8,    8, +,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_rd
ATOMIC_CMPXCHG_READ( fixed2,  rd, kmp_int16,  16, +,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_rd

ATOMIC_CRITICAL_READ( float10, rd, long double, +, 10r,   1 )         // __kmpc_atomic_float10_rd
#if 0
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_READ( float16, rd, QUAD_LEGACY, +, 16r,   1 )         // __kmpc_atomic_float16_rd
#endif // KMP_HAVE_QUAD

// Fix for CQ220361 on Windows* OS
#if ( KMP_OS_WINDOWS )
    ATOMIC_CRITICAL_READ_WRK( cmplx4,  rd, kmp_cmplx32, +,  8c, 1 )   // __kmpc_atomic_cmplx4_rd
#else
    ATOMIC_CRITICAL_READ( cmplx4,  rd, kmp_cmplx32, +,  8c, 1 )       // __kmpc_atomic_cmplx4_rd
#endif
ATOMIC_CRITICAL_READ( cmplx8,  rd, kmp_cmplx64, +, 16c, 1 )           // __kmpc_atomic_cmplx8_rd
ATOMIC_CRITICAL_READ( cmplx10, rd, kmp_cmplx80, +, 20c, 1 )           // __kmpc_atomic_cmplx10_rd
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_READ( cmplx16, rd, CPLX128_LEG, +, 32c, 1 )           // __kmpc_atomic_cmplx16_rd
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL_READ( float16, a16_rd, Quad_a16_t, +, 16r, 1 )         // __kmpc_atomic_float16_a16_rd
    ATOMIC_CRITICAL_READ( cmplx16, a16_rd, kmp_cmplx128_a16_t, +, 32c, 1 ) // __kmpc_atomic_cmplx16_a16_rd
#endif
#endif
#endif

// ------------------------------------------------------------------------
// Atomic WRITE routines
// ------------------------------------------------------------------------
ATOMIC_XCHG_WR( fixed1,  wr, kmp_int8,    8, =,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_wr
ATOMIC_XCHG_WR( fixed2,  wr, kmp_int16,  16, =,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_wr
//ATOMIC_XCHG_WR( fixed4,  wr, kmp_int32,  32, =,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_wr
#if ( KMP_ARCH_X86 )
    ATOMIC_CMPXCHG_WR( fixed8,  wr, kmp_int64,  64, =,  KMP_ARCH_X86 )      // __kmpc_atomic_fixed8_wr
#else
    ATOMIC_XCHG_WR( fixed8,  wr, kmp_int64,  64, =,  KMP_ARCH_X86 )         // __kmpc_atomic_fixed8_wr
#endif

ATOMIC_XCHG_FLOAT_WR( float4, wr, kmp_real32, 32, =, KMP_ARCH_X86 )         // __kmpc_atomic_float4_wr
#if ( KMP_ARCH_X86 )
    ATOMIC_CMPXCHG_WR( float8,  wr, kmp_real64,  64, =,  KMP_ARCH_X86 )     // __kmpc_atomic_float8_wr
#else
    ATOMIC_XCHG_FLOAT_WR( float8,  wr, kmp_real64,  64, =,  KMP_ARCH_X86 )  // __kmpc_atomic_float8_wr
#endif

ATOMIC_CRITICAL_WR( float10, wr, long double, =, 10r,   1 )         // __kmpc_atomic_float10_wr
#if 0
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_WR( float16, wr, QUAD_LEGACY, =, 16r,   1 )         // __kmpc_atomic_float16_wr
#endif
ATOMIC_CRITICAL_WR( cmplx4,  wr, kmp_cmplx32, =,  8c,   1 )         // __kmpc_atomic_cmplx4_wr
ATOMIC_CRITICAL_WR( cmplx8,  wr, kmp_cmplx64, =, 16c,   1 )         // __kmpc_atomic_cmplx8_wr
ATOMIC_CRITICAL_WR( cmplx10, wr, kmp_cmplx80, =, 20c,   1 )         // __kmpc_atomic_cmplx10_wr
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_WR( cmplx16, wr, CPLX128_LEG, =, 32c,   1 )         // __kmpc_atomic_cmplx16_wr
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL_WR( float16, a16_wr, Quad_a16_t,         =, 16r, 1 ) // __kmpc_atomic_float16_a16_wr
    ATOMIC_CRITICAL_WR( cmplx16, a16_wr, kmp_cmplx128_a16_t, =, 32c, 1 ) // __kmpc_atomic_cmplx16_a16_wr
#endif
#endif
#endif

// ------------------------------------------------------------------------
// Atomic CAPTURE routines
// ------------------------------------------------------------------------
ATOMIC_FIXED_ADD_CPT( fixed4, add_cpt, kmp_int32,  32, +, 0            )  // __kmpc_atomic_fixed4_add_cpt
ATOMIC_FIXED_ADD_CPT( fixed4, sub_cpt, kmp_int32,  32, -, 0            )  // __kmpc_atomic_fixed4_sub_cpt
ATOMIC_FIXED_ADD_CPT( fixed8, add_cpt, kmp_int64,  64, +, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_add_cpt
ATOMIC_FIXED_ADD_CPT( fixed8, sub_cpt, kmp_int64,  64, -, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_sub_cpt

ATOMIC_CMPXCHG_CPT( float4, add_cpt, kmp_real32, 32, +, KMP_ARCH_X86 )  // __kmpc_atomic_float4_add_cpt
ATOMIC_CMPXCHG_CPT( float4, sub_cpt, kmp_real32, 32, -, KMP_ARCH_X86 )  // __kmpc_atomic_float4_sub_cpt
ATOMIC_CMPXCHG_CPT( float8, add_cpt, kmp_real64, 64, +, KMP_ARCH_X86 )  // __kmpc_atomic_float8_add_cpt
ATOMIC_CMPXCHG_CPT( float8, sub_cpt, kmp_real64, 64, -, KMP_ARCH_X86 )  // __kmpc_atomic_float8_sub_cpt

// ------------------------------------------------------------------------
// Entries definition for integer operands
//     TYPE_ID - operands type and size (fixed4, float4)
//     OP_ID   - operation identifier (add, sub, mul, ...)
//     TYPE    - operand type
//     BITS    - size in bits, used to distinguish low level calls
//     OP      - operator (used in critical section)
//               TYPE_ID,OP_ID,  TYPE,   BITS,OP,GOMP_FLAG
// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//                  TYPE_ID,  OP_ID,   TYPE,         OP,  GOMP_FLAG
ATOMIC_CMPXCHG_CPT( fixed1,  add_cpt, kmp_int8,    8, +,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_add_cpt
ATOMIC_CMPXCHG_CPT( fixed1, andb_cpt, kmp_int8,    8, &,  0            )  // __kmpc_atomic_fixed1_andb_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  div_cpt, kmp_int8,    8, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_div_cpt
ATOMIC_CMPXCHG_CPT( fixed1u, div_cpt, kmp_uint8,   8, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_div_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  mul_cpt, kmp_int8,    8, *,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_mul_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  orb_cpt, kmp_int8,    8, |,  0            )  // __kmpc_atomic_fixed1_orb_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  shl_cpt, kmp_int8,    8, <<, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shl_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  shr_cpt, kmp_int8,    8, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed1u, shr_cpt, kmp_uint8,   8, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  sub_cpt, kmp_int8,    8, -,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_sub_cpt
ATOMIC_CMPXCHG_CPT( fixed1,  xor_cpt, kmp_int8,    8, ^,  0            )  // __kmpc_atomic_fixed1_xor_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  add_cpt, kmp_int16,  16, +,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_add_cpt
ATOMIC_CMPXCHG_CPT( fixed2, andb_cpt, kmp_int16,  16, &,  0            )  // __kmpc_atomic_fixed2_andb_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  div_cpt, kmp_int16,  16, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_div_cpt
ATOMIC_CMPXCHG_CPT( fixed2u, div_cpt, kmp_uint16, 16, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_div_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  mul_cpt, kmp_int16,  16, *,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_mul_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  orb_cpt, kmp_int16,  16, |,  0            )  // __kmpc_atomic_fixed2_orb_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  shl_cpt, kmp_int16,  16, <<, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shl_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  shr_cpt, kmp_int16,  16, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed2u, shr_cpt, kmp_uint16, 16, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  sub_cpt, kmp_int16,  16, -,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_sub_cpt
ATOMIC_CMPXCHG_CPT( fixed2,  xor_cpt, kmp_int16,  16, ^,  0            )  // __kmpc_atomic_fixed2_xor_cpt
ATOMIC_CMPXCHG_CPT( fixed4, andb_cpt, kmp_int32,  32, &,  0            )  // __kmpc_atomic_fixed4_andb_cpt
ATOMIC_CMPXCHG_CPT( fixed4,  div_cpt, kmp_int32,  32, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_div_cpt
ATOMIC_CMPXCHG_CPT( fixed4u, div_cpt, kmp_uint32, 32, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_div_cpt
ATOMIC_CMPXCHG_CPT( fixed4,  mul_cpt, kmp_int32,  32, *,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_mul_cpt
ATOMIC_CMPXCHG_CPT( fixed4,  orb_cpt, kmp_int32,  32, |,  0            )  // __kmpc_atomic_fixed4_orb_cpt
ATOMIC_CMPXCHG_CPT( fixed4,  shl_cpt, kmp_int32,  32, <<, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shl_cpt
ATOMIC_CMPXCHG_CPT( fixed4,  shr_cpt, kmp_int32,  32, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed4u, shr_cpt, kmp_uint32, 32, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed4,  xor_cpt, kmp_int32,  32, ^,  0            )  // __kmpc_atomic_fixed4_xor_cpt
ATOMIC_CMPXCHG_CPT( fixed8, andb_cpt, kmp_int64,  64, &,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_andb_cpt
ATOMIC_CMPXCHG_CPT( fixed8,  div_cpt, kmp_int64,  64, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_div_cpt
ATOMIC_CMPXCHG_CPT( fixed8u, div_cpt, kmp_uint64, 64, /,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_div_cpt
ATOMIC_CMPXCHG_CPT( fixed8,  mul_cpt, kmp_int64,  64, *,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_mul_cpt
ATOMIC_CMPXCHG_CPT( fixed8,  orb_cpt, kmp_int64,  64, |,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_orb_cpt
ATOMIC_CMPXCHG_CPT( fixed8,  shl_cpt, kmp_int64,  64, <<, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shl_cpt
ATOMIC_CMPXCHG_CPT( fixed8,  shr_cpt, kmp_int64,  64, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed8u, shr_cpt, kmp_uint64, 64, >>, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_shr_cpt
ATOMIC_CMPXCHG_CPT( fixed8,  xor_cpt, kmp_int64,  64, ^,  KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_xor_cpt
ATOMIC_CMPXCHG_CPT( float4,  div_cpt, kmp_real32, 32, /,  KMP_ARCH_X86 )  // __kmpc_atomic_float4_div_cpt
ATOMIC_CMPXCHG_CPT( float4,  mul_cpt, kmp_real32, 32, *,  KMP_ARCH_X86 )  // __kmpc_atomic_float4_mul_cpt
ATOMIC_CMPXCHG_CPT( float8,  div_cpt, kmp_real64, 64, /,  KMP_ARCH_X86 )  // __kmpc_atomic_float8_div_cpt
ATOMIC_CMPXCHG_CPT( float8,  mul_cpt, kmp_real64, 64, *,  KMP_ARCH_X86 )  // __kmpc_atomic_float8_mul_cpt
// ------------------------------------------------------------------------
// Routines for C/C++ Reduction operators && and ||
// ------------------------------------------------------------------------
ATOMIC_CMPX_L_CPT( fixed1, andl_cpt, char,       8, &&, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_andl_cpt
ATOMIC_CMPX_L_CPT( fixed1,  orl_cpt, char,       8, ||, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_orl_cpt
ATOMIC_CMPX_L_CPT( fixed2, andl_cpt, short,     16, &&, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_andl_cpt
ATOMIC_CMPX_L_CPT( fixed2,  orl_cpt, short,     16, ||, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_orl_cpt
ATOMIC_CMPX_L_CPT( fixed4, andl_cpt, kmp_int32, 32, &&, 0 )             // __kmpc_atomic_fixed4_andl_cpt
ATOMIC_CMPX_L_CPT( fixed4,  orl_cpt, kmp_int32, 32, ||, 0 )             // __kmpc_atomic_fixed4_orl_cpt
ATOMIC_CMPX_L_CPT( fixed8, andl_cpt, kmp_int64, 64, &&, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_andl_cpt
ATOMIC_CMPX_L_CPT( fixed8,  orl_cpt, kmp_int64, 64, ||, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_orl_cpt

// -------------------------------------------------------------------------
// Routines for Fortran operators that matched no one in C:
// MAX, MIN, .EQV., .NEQV.
// Operators .AND., .OR. are covered by __kmpc_atomic_*_{andl,orl}_cpt
// Intrinsics IAND, IOR, IEOR are covered by __kmpc_atomic_*_{andb,orb,xor}_cpt
// -------------------------------------------------------------------------
MIN_MAX_COMPXCHG_CPT( fixed1,  max_cpt, char,        8, <, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed1,  min_cpt, char,        8, >, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_min_cpt
MIN_MAX_COMPXCHG_CPT( fixed2,  max_cpt, short,      16, <, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed2,  min_cpt, short,      16, >, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_min_cpt
MIN_MAX_COMPXCHG_CPT( fixed4,  max_cpt, kmp_int32,  32, <, 0 )            // __kmpc_atomic_fixed4_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed4,  min_cpt, kmp_int32,  32, >, 0 )            // __kmpc_atomic_fixed4_min_cpt
MIN_MAX_COMPXCHG_CPT( fixed8,  max_cpt, kmp_int64,  64, <, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed8,  min_cpt, kmp_int64,  64, >, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_min_cpt
MIN_MAX_COMPXCHG_CPT( float4,  max_cpt, kmp_real32, 32, <, KMP_ARCH_X86 ) // __kmpc_atomic_float4_max_cpt
MIN_MAX_COMPXCHG_CPT( float4,  min_cpt, kmp_real32, 32, >, KMP_ARCH_X86 ) // __kmpc_atomic_float4_min_cpt
MIN_MAX_COMPXCHG_CPT( float8,  max_cpt, kmp_real64, 64, <, KMP_ARCH_X86 ) // __kmpc_atomic_float8_max_cpt
MIN_MAX_COMPXCHG_CPT( float8,  min_cpt, kmp_real64, 64, >, KMP_ARCH_X86 ) // __kmpc_atomic_float8_min_cpt
#if 0
#if KMP_HAVE_QUAD
MIN_MAX_CRITICAL_CPT( float16, max_cpt, QUAD_LEGACY,    <, 16r,   1 )     // __kmpc_atomic_float16_max_cpt
MIN_MAX_CRITICAL_CPT( float16, min_cpt, QUAD_LEGACY,    >, 16r,   1 )     // __kmpc_atomic_float16_min_cpt
#if ( KMP_ARCH_X86 )
    MIN_MAX_CRITICAL_CPT( float16, max_a16_cpt, Quad_a16_t, <, 16r,  1 )  // __kmpc_atomic_float16_max_a16_cpt
    MIN_MAX_CRITICAL_CPT( float16, min_a16_cpt, Quad_a16_t, >, 16r,  1 )  // __kmpc_atomic_float16_mix_a16_cpt
#endif
#endif
#endif

ATOMIC_CMPXCHG_CPT(  fixed1, neqv_cpt, kmp_int8,   8,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_neqv_cpt
ATOMIC_CMPXCHG_CPT(  fixed2, neqv_cpt, kmp_int16, 16,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_neqv_cpt
ATOMIC_CMPXCHG_CPT(  fixed4, neqv_cpt, kmp_int32, 32,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_neqv_cpt
ATOMIC_CMPXCHG_CPT(  fixed8, neqv_cpt, kmp_int64, 64,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_neqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed1, eqv_cpt,  kmp_int8,   8,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_eqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed2, eqv_cpt,  kmp_int16, 16,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_eqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed4, eqv_cpt,  kmp_int32, 32,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_eqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed8, eqv_cpt,  kmp_int64, 64,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_eqv_cpt

// ------------------------------------------------------------------------
// Routines for Extended types: long double, _Quad, complex flavours (use critical section)
/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL_CPT( float10, add_cpt, long double,     +, 10r,   1 )            // __kmpc_atomic_float10_add_cpt
ATOMIC_CRITICAL_CPT( float10, sub_cpt, long double,     -, 10r,   1 )            // __kmpc_atomic_float10_sub_cpt
ATOMIC_CRITICAL_CPT( float10, mul_cpt, long double,     *, 10r,   1 )            // __kmpc_atomic_float10_mul_cpt
ATOMIC_CRITICAL_CPT( float10, div_cpt, long double,     /, 10r,   1 )            // __kmpc_atomic_float10_div_cpt
#if 0
#if KMP_HAVE_QUAD
// routines for _Quad type
ATOMIC_CRITICAL_CPT( float16, add_cpt, QUAD_LEGACY,     +, 16r,   1 )            // __kmpc_atomic_float16_add_cpt
ATOMIC_CRITICAL_CPT( float16, sub_cpt, QUAD_LEGACY,     -, 16r,   1 )            // __kmpc_atomic_float16_sub_cpt
ATOMIC_CRITICAL_CPT( float16, mul_cpt, QUAD_LEGACY,     *, 16r,   1 )            // __kmpc_atomic_float16_mul_cpt
ATOMIC_CRITICAL_CPT( float16, div_cpt, QUAD_LEGACY,     /, 16r,   1 )            // __kmpc_atomic_float16_div_cpt
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL_CPT( float16, add_a16_cpt, Quad_a16_t, +, 16r,  1 )          // __kmpc_atomic_float16_add_a16_cpt
    ATOMIC_CRITICAL_CPT( float16, sub_a16_cpt, Quad_a16_t, -, 16r,  1 )          // __kmpc_atomic_float16_sub_a16_cpt
    ATOMIC_CRITICAL_CPT( float16, mul_a16_cpt, Quad_a16_t, *, 16r,  1 )          // __kmpc_atomic_float16_mul_a16_cpt
    ATOMIC_CRITICAL_CPT( float16, div_a16_cpt, Quad_a16_t, /, 16r,  1 )          // __kmpc_atomic_float16_div_a16_cpt
#endif
#endif

// routines for complex types

// cmplx4 routines to return void
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  add_cpt, kmp_cmplx32, +, 8c,    1 )            // __kmpc_atomic_cmplx4_add_cpt
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  sub_cpt, kmp_cmplx32, -, 8c,    1 )            // __kmpc_atomic_cmplx4_sub_cpt
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  mul_cpt, kmp_cmplx32, *, 8c,    1 )            // __kmpc_atomic_cmplx4_mul_cpt
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  div_cpt, kmp_cmplx32, /, 8c,    1 )            // __kmpc_atomic_cmplx4_div_cpt

ATOMIC_CRITICAL_CPT( cmplx8,  add_cpt, kmp_cmplx64, +, 16c,   1 )            // __kmpc_atomic_cmplx8_add_cpt
ATOMIC_CRITICAL_CPT( cmplx8,  sub_cpt, kmp_cmplx64, -, 16c,   1 )            // __kmpc_atomic_cmplx8_sub_cpt
ATOMIC_CRITICAL_CPT( cmplx8,  mul_cpt, kmp_cmplx64, *, 16c,   1 )            // __kmpc_atomic_cmplx8_mul_cpt
ATOMIC_CRITICAL_CPT( cmplx8,  div_cpt, kmp_cmplx64, /, 16c,   1 )            // __kmpc_atomic_cmplx8_div_cpt
ATOMIC_CRITICAL_CPT( cmplx10, add_cpt, kmp_cmplx80, +, 20c,   1 )            // __kmpc_atomic_cmplx10_add_cpt
ATOMIC_CRITICAL_CPT( cmplx10, sub_cpt, kmp_cmplx80, -, 20c,   1 )            // __kmpc_atomic_cmplx10_sub_cpt
ATOMIC_CRITICAL_CPT( cmplx10, mul_cpt, kmp_cmplx80, *, 20c,   1 )            // __kmpc_atomic_cmplx10_mul_cpt
ATOMIC_CRITICAL_CPT( cmplx10, div_cpt, kmp_cmplx80, /, 20c,   1 )            // __kmpc_atomic_cmplx10_div_cpt
#if KMP_HAVE_QUAD
ATOMIC_CRITICAL_CPT( cmplx16, add_cpt, CPLX128_LEG, +, 32c,   1 )            // __kmpc_atomic_cmplx16_add_cpt
ATOMIC_CRITICAL_CPT( cmplx16, sub_cpt, CPLX128_LEG, -, 32c,   1 )            // __kmpc_atomic_cmplx16_sub_cpt
ATOMIC_CRITICAL_CPT( cmplx16, mul_cpt, CPLX128_LEG, *, 32c,   1 )            // __kmpc_atomic_cmplx16_mul_cpt
ATOMIC_CRITICAL_CPT( cmplx16, div_cpt, CPLX128_LEG, /, 32c,   1 )            // __kmpc_atomic_cmplx16_div_cpt
#if ( KMP_ARCH_X86 )
    ATOMIC_CRITICAL_CPT( cmplx16, add_a16_cpt, kmp_cmplx128_a16_t, +, 32c,   1 )   // __kmpc_atomic_cmplx16_add_a16_cpt
    ATOMIC_CRITICAL_CPT( cmplx16, sub_a16_cpt, kmp_cmplx128_a16_t, -, 32c,   1 )   // __kmpc_atomic_cmplx16_sub_a16_cpt
    ATOMIC_CRITICAL_CPT( cmplx16, mul_a16_cpt, kmp_cmplx128_a16_t, *, 32c,   1 )   // __kmpc_atomic_cmplx16_mul_a16_cpt
    ATOMIC_CRITICAL_CPT( cmplx16, div_a16_cpt, kmp_cmplx128_a16_t, /, 32c,   1 )   // __kmpc_atomic_cmplx16_div_a16_cpt
#endif
#endif
#endif


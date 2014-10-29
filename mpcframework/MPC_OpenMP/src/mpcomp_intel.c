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


/* MY STRUCTS */

typedef struct wrapper {
  kmpc_micro f ;
  int argc ;
  void ** args ;
} wrapper_t ;


/********************************
  * FUNCTIONS
  *******************************/

extern double __kmp_test_then_add_real64( volatile double *addr, double data ); 

extern int __kmp_invoke_microtask( 
    kmpc_micro pkfn, int gtid, int npr, int argc, void *argv[] );

void *
wrapper_function( void * arg ) 
{
  int rank ;
  wrapper_t * w ;

  rank = mpcomp_get_thread_num() ;
  w = (wrapper_t *) arg ;

  sctk_debug( "[%d] wrapper_function: entering w/ %d arg(s)...",
     rank, w->argc ) ;

#if 0
  switch( w->argc ) {
    case 0:
      w->f( &rank, &rank ) ;
      break ;
    case 1:
      w->f( &rank, &rank, w->args[0] ) ;
      break ;
    case 2:
      w->f( &rank, &rank, w->args[0], w->args[1] ) ;
      break ;
    case 3:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2] ) ;
      break ;
    case 4:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3] ) ;
      break ;
    case 5:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4] ) ;
      break ;
    case 6:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5] ) ;
      break ;
    case 7:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6] ) ;
      break ;
    case 9:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8] ) ;
      break ;
    case 10:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8],
	 w->args[9] ) ;
      break ;
    case 11:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8],
	 w->args[9], w->args[10] ) ;
      break ;
    case 12:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8],
	 w->args[9], w->args[10], w->args[11] ) ;
      break ;
    case 13:
      w->f( &rank, &rank, w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8],
	 w->args[9], w->args[10], w->args[11], w->args[12] ) ;
      break ;
    case 18:
      w->f( &rank, &rank, 
	  w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8], w->args[9], 
	  w->args[10], w->args[11], w->args[12], w->args[13], w->args[14], w->args[15], w->args[16], w->args[17]
	  ) ;
      break ;
    case 59:
      w->f( &rank, &rank, 
	  w->args[0], w->args[1], w->args[2], w->args[3], w->args[4], w->args[5], w->args[6], w->args[7], w->args[8], w->args[9], 
	  w->args[10], w->args[11], w->args[12], w->args[13], w->args[14], w->args[15], w->args[16], w->args[17], w->args[18], w->args[19], 
	  w->args[20], w->args[21], w->args[22], w->args[23], w->args[24], w->args[25], w->args[26], w->args[27], w->args[28], w->args[29], 
	  w->args[30], w->args[31], w->args[32], w->args[33], w->args[34], w->args[35], w->args[36], w->args[37], w->args[38], w->args[39], 
	  w->args[40], w->args[41], w->args[42], w->args[43], w->args[44], w->args[45], w->args[46], w->args[47], w->args[48], w->args[49], 
	  w->args[50], w->args[51], w->args[52], w->args[53], w->args[54], w->args[55], w->args[56], w->args[57], w->args[58]
	  ) ;
      break ;
    default:
      not_reachable() ;
      break ;
  }
#endif

#if 1

  sctk_debug( "[%d] wrapper_function: invoking microtask...",
     rank ) ;

  __kmp_invoke_microtask( w->f, rank, rank, w->argc, w->args ) ;
#endif

  return NULL ;
}

/********************************
  * STARTUP AND SHUTDOWN
  *******************************/

void   
__kmpc_begin ( ident_t * loc, kmp_int32 flags ) 
{
not_implemented() ;
}

void   
__kmpc_end ( ident_t * loc )
{
not_implemented() ;
}


/********************************
  * PARALLEL FORK/JOIN 
  *******************************/

kmp_int32
__kmpc_ok_to_fork(ident_t * loc)
{
  sctk_debug( "__kmpc_ok_to_fork: entering..." ) ;
  return 1 ;
}

void
__kmpc_fork_call(ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ...)
{
  va_list args ;
  int i ;
  void ** args_copy ;
  wrapper_t w ;

  sctk_debug( "__kmpc_fork_call: entering w/ %d arg(s)...", argc ) ;

  args_copy = (void **)malloc( argc * sizeof( void * ) ) ;

  va_start( args, microtask ) ;

  for ( i = 0 ; i < argc ; i++ ) {
    args_copy[i] = va_arg( args, void * ) ;
  }

  va_end(args) ;

  w.f = microtask ;
  w.argc = argc ;
  w.args = args_copy ;

  __mpcomp_start_parallel_region( -1, wrapper_function, &w ) ;
}

void
__kmpc_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_debug( "__kmpc_serialized_parallel: entering (%d) ...", global_tid ) ;

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

  sctk_debug( "__kmpc_end_serialized_parallel: entering (%d)...", global_tid ) ;

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

  sctk_debug( "__kmpc_push_num_threads: pushing %d thread(s)", num_threads ) ;

  mpcomp_set_num_threads(num_threads) ;

  /* TODO this is not completly correct regarding OpenMP standard... */
  /* TODO check if the bug with too large numn_threads() appears in 
     final version of the runtime... */
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
  sctk_debug( "__kmpc_global_thread_num: " ) ;
  return mpcomp_get_thread_num() ;
}

kmp_int32
__kmpc_global_num_threads(ident_t * loc)
{
  sctk_debug( "__kmpc_global_num_threads: " ) ;
  return mpcomp_get_num_threads() ;
}

kmp_int32
__kmpc_bound_thread_num(ident_t * loc)
{
  sctk_debug( "__kmpc_bound_thread_num: " ) ;
  return mpcomp_get_thread_num() ;
}

kmp_int32
__kmpc_bound_num_threads(ident_t * loc)
{
  sctk_debug( "__kmpc_bound_num_threads: " ) ;
  return mpcomp_get_num_threads() ;
}

kmp_int32
__kmpc_in_parallel(ident_t * loc)
{
  sctk_debug( "__kmpc_in_parallel: " ) ;
  return mpcomp_in_parallel() ;
}


/********************************
  * SYNCHRONIZATION 
  *******************************/

void
__kmpc_flush(ident_t *loc, ...) 
{
  not_implemented() ;
}

void
__kmpc_barrier(ident_t *loc, kmp_int32 global_tid) 
{
  sctk_debug( "[%d] __kmpc_barrier: entering...",
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


kmp_int32 
__kmpc_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* TODO merge this function body w/ __kmpc_reduce */

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

  sctk_debug( "[%d] __kmpc_reduce_nowait %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;

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
__kmpc_end_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck ) 
{

  /* TODO merge this function body w/ __kmpc_end_reduce */

  sctk_debug( "[%d] __kmpc_end_reduce_nowait",
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

  sctk_debug( "[%d] __kmp_master: entering",
      t->rank ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }
  return 0 ;

}

void
__kmpc_end_master(ident_t *loc, kmp_int32 global_tid)
{
  sctk_debug( "[%d] __kmpc_end_master: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void 
__kmpc_ordered(ident_t *loc, kmp_int32 global_tid)
{
  not_implemented() ;
}

void
__kmpc_end_ordered(ident_t *loc, kmp_int32 global_tid)
{
  not_implemented() ;
}

void
__kmpc_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  sctk_debug( "[%d] __kmpc_critical: enter %p (%d,%d,%d,%d,%d,%d,%d,%d)",
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
  sctk_debug( "[%d] __kmpc_single: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;

  return __mpcomp_do_single() ;
}

void
__kmpc_end_single(ident_t *loc, kmp_int32 global_tid)
{
  sctk_debug( "[%d] __kmpc_end_single: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void
__kmpc_for_static_init_4( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int32 * plower, kmp_int32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
  long from, to ;
  mpcomp_thread_t *t;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);   


  sctk_debug( "[%d] __kmpc_for_static_init_4: <%s> "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
      loc->psource,
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
      ) ;

  switch( schedtype ) {
    case kmp_sch_static:

      /* Get the single chunk for the current thread */
      __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr,
	  &from, &to ) ;

      sctk_debug( "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static): "
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

      //* TODO what about pstride and plastiter? */
      // *pstride = incr ;
      // *plastiter = 1 ;
      break ;
    case kmp_sch_static_chunked:

      sctk_assert( chunk >= 1 ) ;


      // span = chunk * incr;
      *pstride = (chunk * incr) * t->info.num_threads ;
      *plower = *plower + ((chunk * incr)* t->rank );
      *pupper = *plower + (chunk * incr) - incr;


      /* __mpcomp_static_schedule_get_specific_chunk( *plower, *pupper+incr, incr,
	  chunk, 0, &from, &to ) ;
	  */

      sctk_debug( "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static_chunked): "
	  "%ld -> %ld excl %ld incl [%d]"
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  *plower, *pupper, *pupper-incr, incr
	  ) ;

      /* Remarks:
	 - MPC chunk has not-inclusive upper bound while Intel runtime includes
	 upper bound for calculation 
	 - Need to cast from long to int because MPC handles everything has a long
	 (like GCC) while Intel creates different functions
	 */
      // *plower=(kmp_int32)from ;
      // *pupper=(kmp_int32)to-incr;

      /* TODO what should we do w/ plastiter? */
      /* TODO what if the number of chunk is > 1? */

      // not_implemented() ;
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
  not_implemented() ;
}

void
__kmpc_for_static_init_8( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int64 * plower, kmp_int64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
  long from, to ;

  switch( schedtype ) {
    case kmp_sch_static:
      sctk_debug( "[%d] __kmpc_for_static_init_8: "
	  "schedtype=%d, %d? %ld -> %ld incl. [%ld], incr=%ld chunk=%ld "
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk
	  ) ;

      /* Get the single chunk for the current thread */
      __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr,
	  &from, &to ) ;

      sctk_debug( "[%d] Results for __kmpc_for_static_init_8: "
	  "%ld -> %ld [%ld]"
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  from, to, incr
	  ) ;

      /* Remarks:
	 - MPC chunk has not-inclusive upper bound while Intel runtime includes
	 upper bound for calculation 
	 - Need to case from long to int because MPC handles everything has a long
	 (like GCC) while Intel creates different functions
	 */
      *plower=(kmp_int64)from ;
      *pupper=(kmp_int64)to-incr;
      // *pstride = incr ;
      // *plastiter = 1 ;
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
  not_implemented() ;
}

void
__kmpc_for_static_fini( ident_t * loc, kmp_int32 global_tid ) 
{
  sctk_debug( "[%d] __kmpc_for_static_fini: entering...",
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

  switch( schedule ) {
    case kmp_sch_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk
	  ) ;
      break ;
    default:
      not_implemented() ;
      break ;
  }
}

void
__kmpc_dispatch_init_4u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_uint32 lb, kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk )
{
  not_implemented() ;
}

void
__kmpc_dispatch_init_8( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_int64 lb, kmp_int64 ub,
                        kmp_int64 st, kmp_int64 chunk )
{
  sctk_debug(
      "[%d] __kmpc_dispatch_init_8: enter %ld -> %ld incl, %ld excl [%ld] ck:%ld sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;

  switch( schedule ) {
    case kmp_sch_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, chunk ) ;
      break ;
    default:
      not_implemented() ;
      break ;
  }

}

void
__kmpc_dispatch_init_8u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                         kmp_uint64 lb, kmp_uint64 ub,
                         kmp_int64 st, kmp_int64 chunk )
{
  not_implemented() ;
}



int
__kmpc_dispatch_next_4( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int32 *p_lb, kmp_int32 *p_ub, kmp_int32 *p_st )
{
  /* TODO need to check the current schedule */

  int ret ;
  mpcomp_thread_t *t ;
  long from, to ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

  ret = __mpcomp_dynamic_loop_next( &from, &to ) ;

  sctk_debug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;


  *p_lb = (kmp_int32) from ;
  *p_ub = (kmp_int32)to - (kmp_int32)t->info.loop_incr ;

  if ( ret == 0 ) {
    __mpcomp_dynamic_loop_end_nowait() ;
  }

  return ret ;
}

int
__kmpc_dispatch_next_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint32 *p_lb, kmp_uint32 *p_ub, kmp_int32 *p_st )
{
  not_implemented() ;
  return 0 ; /* no more work */
}

int
__kmpc_dispatch_next_8( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int64 *p_lb, kmp_int64 *p_ub, kmp_int64 *p_st )
{
  
  /* TODO need to check the current schedule */

  int ret ;
  mpcomp_thread_t *t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

  ret = __mpcomp_dynamic_loop_next( (long *)p_lb, (long *)p_ub ) ;

  sctk_debug( 
      "[%d] __kmpc_dispatch_next_8: %ld -> %ld excl, %ld incl (ret=%d)",
      t->rank,
      *p_lb, *p_ub, *p_ub - t->info.loop_incr, ret ) ;

  *p_ub = *p_ub - t->info.loop_incr ;

  if ( ret == 0 ) {
    __mpcomp_dynamic_loop_end_nowait() ;
  }

  return ret ;
}

int
__kmpc_dispatch_next_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint64 *p_lb, kmp_uint64 *p_ub, kmp_int64 *p_st )
{
  not_implemented() ;
  return 0 ; /* no more work */
}

void
__kmpc_dispatch_fini_4( ident_t *loc, kmp_int32 gtid )
{
  not_implemented() ;
}

void
__kmpc_dispatch_fini_8( ident_t *loc, kmp_int32 gtid )
{
  not_implemented() ;
}

void
__kmpc_dispatch_fini_4u( ident_t *loc, kmp_int32 gtid )
{
  not_implemented() ;
}

void
__kmpc_dispatch_fini_8u( ident_t *loc, kmp_int32 gtid )
{
  not_implemented() ;
}



/********************************
  * THREAD PRIVATE DATA SUPPORT
  *******************************/
void
__kmpc_copyprivate(ident_t *loc, kmp_int32 global_tid, size_t cpy_size, void *cpy_data,
    void (*cpy_func)(void *, void *), kmp_int32 didit )
{
  not_implemented() ;
}

typedef void *(* kmpc_ctor)(void *) ;
typedef void  (* kmpc_dtor)(void *) ;
typedef void *(* kmpc_cctor)(void *, void *) ;
typedef void *(* kmpc_ctor_vec)(void *, size_t) ;
typedef void  (* kmpc_dtor_vec)(void *, size_t) ;
typedef void *(* kmpc_cctor_vec)(void *, void *, size_t) ;

void
__kmpc_threadprivate_register( ident_t *loc, void *data, kmpc_ctor ctor, kmpc_cctor cctor,
    kmpc_dtor dtor)
{
  sctk_error("Detection of threadprivate variables w/ Intel Compiler: please re-compile with automatic privatization for MPC") ;
  sctk_abort() ;
}

void *
__kmpc_threadprivate_cached( ident_t *loc, kmp_int32 global_tid, void *data, size_t size,
    void *** cache)
{
  sctk_error("Detection of threadprivate variables w/ Intel Compiler: please re-compile with automatic privatization for MPC") ;
  sctk_abort() ;
  return NULL ;
}

void
__kmpc_threadprivate_register_vec(ident_t *loc, void *data, kmpc_ctor_vec ctor, 
    kmpc_cctor_vec cctor, kmpc_dtor_vec dtor, size_t vector_length )
{
  sctk_error("Detection of threadprivate variables w/ Intel Compiler: please re-compile with automatic privatization for MPC") ;
  sctk_abort() ;
}

void * 
__kmpc_threadprivate ( ident_t *loc, kmp_int32 global_tid, void * data, size_t size )
{
not_implemented() ;
return NULL ;
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

void
__kmpc_atomic_fixed4_add( ident_t * loc, int global_tid, kmp_int32 * lhs, kmp_int32 rhs )
{

  sctk_nodebug( "[%d] __kmpc_atomic_fixed4_add: "
      "Add %d",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __mpcomp_atomic_begin() ;

  *lhs += rhs ;

  __mpcomp_atomic_end() ;

  /* TODO: use assembly functions by Intel if available */
}


void 
__kmpc_atomic_fixed4_wr(  ident_t *id_ref, int gtid, kmp_int32   * lhs, kmp_int32   rhs ) 
{
  sctk_nodebug( "[%d] __kmpc_atomic_fixed4_wr: "
      "Write %d",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __mpcomp_atomic_begin() ;

  *lhs = rhs ;

  __mpcomp_atomic_end() ;

  /* TODO: use assembly functions by Intel if available */
}

void 
__kmpc_atomic_fixed8_add(  ident_t *id_ref, int gtid, kmp_int64 * lhs, kmp_int64 rhs )
{
  sctk_nodebug( "[%d] __kmpc_atomic_fixed8_add: "
      "Add %ld",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __mpcomp_atomic_begin() ;

  *lhs += rhs ;

  __mpcomp_atomic_end() ;
  /* TODO: use assembly functions by Intel if available */
}

void 
__kmpc_atomic_float8_add(  ident_t *id_ref, int gtid, kmp_real64 * lhs, kmp_real64 rhs )
{
  sctk_nodebug( "[%d] (ASM) __kmpc_atomic_float8_add: "
      "Add %g",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __kmp_test_then_add_real64( lhs, rhs ) ;

  /* TODO check how we can add this function to asssembly-dedicated module */

#if 0
  __mpcomp_atomic_begin() ;

  *lhs += rhs ;

  __mpcomp_atomic_end() ;
#endif


  /* TODO: use assembly functions by Intel if available */
}


#define NON_IMPLEMENTED_ATOMIC(TYPE_ID,OP_ID,TYPE, RET_TYPE) \
  RET_TYPE __kmpc_atomic_##TYPE_ID##_##OP_ID( ident_t *id_ref, int gtid, TYPE * lhs, TYPE rhs ) \
{                                                                                         \
  not_implemented(); \
}

// Routines for ATOMIC 4-byte operands addition and subtraction
// NON_IMPLEMENTED_ATOMIC( fixed4, add, kmp_int32,   void )  // __kmpc_atomic_fixed4_add
NON_IMPLEMENTED_ATOMIC( fixed4, sub, kmp_int32,   void )  // __kmpc_atomic_fixed4_sub
NON_IMPLEMENTED_ATOMIC( float4, add, kmp_real32,  void )  // __kmpc_atomic_float4_add
NON_IMPLEMENTED_ATOMIC( float4, sub, kmp_real32,  void )  // __kmpc_atomic_float4_sub

// Routines for ATOMIC 8-byte operands addition and subtraction
// NON_IMPLEMENTED_ATOMIC( fixed8, add, kmp_int64,   void )  // __kmpc_atomic_fixed8_add
NON_IMPLEMENTED_ATOMIC( fixed8, sub, kmp_int64,   void )  // __kmpc_atomic_fixed8_sub
// NON_IMPLEMENTED_ATOMIC( float8, add, kmp_real64,  void )  // __kmpc_atomic_float8_add
NON_IMPLEMENTED_ATOMIC( float8, sub, kmp_real64,  void )  // __kmpc_atomic_float8_sub

// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
NON_IMPLEMENTED_ATOMIC( fixed1,  add, kmp_int8,   void )  // __kmpc_atomic_fixed1_add
NON_IMPLEMENTED_ATOMIC( fixed1, andb, kmp_int8,   void )  // __kmpc_atomic_fixed1_andb
NON_IMPLEMENTED_ATOMIC( fixed1,  div, kmp_int8,   void )  // __kmpc_atomic_fixed1_div
NON_IMPLEMENTED_ATOMIC( fixed1u, div, kmp_uint8,  void )  // __kmpc_atomic_fixed1u_div
NON_IMPLEMENTED_ATOMIC( fixed1,  mul, kmp_int8,   void )  // __kmpc_atomic_fixed1_mul
NON_IMPLEMENTED_ATOMIC( fixed1,  orb, kmp_int8,   void )  // __kmpc_atomic_fixed1_orb
NON_IMPLEMENTED_ATOMIC( fixed1,  shl, kmp_int8,   void )  // __kmpc_atomic_fixed1_shl
NON_IMPLEMENTED_ATOMIC( fixed1,  shr, kmp_int8,   void )  // __kmpc_atomic_fixed1_shr
NON_IMPLEMENTED_ATOMIC( fixed1u, shr, kmp_uint8,  void )  // __kmpc_atomic_fixed1u_shr
NON_IMPLEMENTED_ATOMIC( fixed1,  sub, kmp_int8,   void )  // __kmpc_atomic_fixed1_sub
NON_IMPLEMENTED_ATOMIC( fixed1,  xor, kmp_int8,   void )  // __kmpc_atomic_fixed1_xor
NON_IMPLEMENTED_ATOMIC( fixed2,  add, kmp_int16,  void )  // __kmpc_atomic_fixed2_add
NON_IMPLEMENTED_ATOMIC( fixed2, andb, kmp_int16,  void )  // __kmpc_atomic_fixed2_andb
NON_IMPLEMENTED_ATOMIC( fixed2,  div, kmp_int16,  void )  // __kmpc_atomic_fixed2_div
NON_IMPLEMENTED_ATOMIC( fixed2u, div, kmp_uint16, void )  // __kmpc_atomic_fixed2u_div
NON_IMPLEMENTED_ATOMIC( fixed2,  mul, kmp_int16,  void )  // __kmpc_atomic_fixed2_mul
NON_IMPLEMENTED_ATOMIC( fixed2,  orb, kmp_int16,  void )  // __kmpc_atomic_fixed2_orb
NON_IMPLEMENTED_ATOMIC( fixed2,  shl, kmp_int16,  void )  // __kmpc_atomic_fixed2_shl
NON_IMPLEMENTED_ATOMIC( fixed2,  shr, kmp_int16,  void )  // __kmpc_atomic_fixed2_shr
NON_IMPLEMENTED_ATOMIC( fixed2u, shr, kmp_uint16, void )  // __kmpc_atomic_fixed2u_shr
NON_IMPLEMENTED_ATOMIC( fixed2,  sub, kmp_int16,  void )  // __kmpc_atomic_fixed2_sub
NON_IMPLEMENTED_ATOMIC( fixed2,  xor, kmp_int16,  void )  // __kmpc_atomic_fixed2_xor
NON_IMPLEMENTED_ATOMIC( fixed4, andb, kmp_int32,  void )  // __kmpc_atomic_fixed4_andb
NON_IMPLEMENTED_ATOMIC( fixed4,  div, kmp_int32,  void )  // __kmpc_atomic_fixed4_div
NON_IMPLEMENTED_ATOMIC( fixed4u, div, kmp_uint32, void )  // __kmpc_atomic_fixed4u_div
NON_IMPLEMENTED_ATOMIC( fixed4,  mul, kmp_int32,  void )  // __kmpc_atomic_fixed4_mul
NON_IMPLEMENTED_ATOMIC( fixed4,  orb, kmp_int32,  void )  // __kmpc_atomic_fixed4_orb
NON_IMPLEMENTED_ATOMIC( fixed4,  shl, kmp_int32,  void )  // __kmpc_atomic_fixed4_shl
NON_IMPLEMENTED_ATOMIC( fixed4,  shr, kmp_int32,  void )  // __kmpc_atomic_fixed4_shr
NON_IMPLEMENTED_ATOMIC( fixed4u, shr, kmp_uint32, void )  // __kmpc_atomic_fixed4u_shr
NON_IMPLEMENTED_ATOMIC( fixed4,  xor, kmp_int32,  void )  // __kmpc_atomic_fixed4_xor
NON_IMPLEMENTED_ATOMIC( fixed8, andb, kmp_int64,  void )  // __kmpc_atomic_fixed8_andb
NON_IMPLEMENTED_ATOMIC( fixed8,  div, kmp_int64,  void )  // __kmpc_atomic_fixed8_div
NON_IMPLEMENTED_ATOMIC( fixed8u, div, kmp_uint64, void )  // __kmpc_atomic_fixed8u_div
NON_IMPLEMENTED_ATOMIC( fixed8,  mul, kmp_int64,  void )  // __kmpc_atomic_fixed8_mul
NON_IMPLEMENTED_ATOMIC( fixed8,  orb, kmp_int64,  void )  // __kmpc_atomic_fixed8_orb
NON_IMPLEMENTED_ATOMIC( fixed8,  shl, kmp_int64,  void )  // __kmpc_atomic_fixed8_shl
NON_IMPLEMENTED_ATOMIC( fixed8,  shr, kmp_int64,  void )  // __kmpc_atomic_fixed8_shr
NON_IMPLEMENTED_ATOMIC( fixed8u, shr, kmp_uint64, void )  // __kmpc_atomic_fixed8u_shr
NON_IMPLEMENTED_ATOMIC( fixed8,  xor, kmp_int64,  void )  // __kmpc_atomic_fixed8_xor
NON_IMPLEMENTED_ATOMIC( float4,  div, kmp_real32, void )  // __kmpc_atomic_float4_div
NON_IMPLEMENTED_ATOMIC( float4,  mul, kmp_real32, void )  // __kmpc_atomic_float4_mul
NON_IMPLEMENTED_ATOMIC( float8,  div, kmp_real64, void )  // __kmpc_atomic_float8_div
NON_IMPLEMENTED_ATOMIC( float8,  mul, kmp_real64, void )  // __kmpc_atomic_float8_mul

/* ------------------------------------------------------------------------ */
/* Routines for C/C++ Reduction operators && and ||                         */
/* ------------------------------------------------------------------------ */
NON_IMPLEMENTED_ATOMIC( fixed1, andl, char,       void )   // __kmpc_atomic_fixed1_andl
NON_IMPLEMENTED_ATOMIC( fixed1,  orl, char,       void )   // __kmpc_atomic_fixed1_orl
NON_IMPLEMENTED_ATOMIC( fixed2, andl, short,      void )   // __kmpc_atomic_fixed2_andl
NON_IMPLEMENTED_ATOMIC( fixed2,  orl, short,      void )   // __kmpc_atomic_fixed2_orl
NON_IMPLEMENTED_ATOMIC( fixed4, andl, kmp_int32,  void )   // __kmpc_atomic_fixed4_andl
NON_IMPLEMENTED_ATOMIC( fixed4,  orl, kmp_int32,  void )   // __kmpc_atomic_fixed4_orl
NON_IMPLEMENTED_ATOMIC( fixed8, andl, kmp_int64,  void )   // __kmpc_atomic_fixed8_andl
NON_IMPLEMENTED_ATOMIC( fixed8,  orl, kmp_int64,  void )   // __kmpc_atomic_fixed8_orl

/* ------------------------------------------------------------------------- */
/* Routines for Fortran operators that matched no one in C:                  */
/* MAX, MIN, .EQV., .NEQV.                                                   */
/* Operators .AND., .OR. are covered by __kmpc_atomic_*_{andl,orl}           */
/* Intrinsics IAND, IOR, IEOR are covered by __kmpc_atomic_*_{andb,orb,xor}  */
/* ------------------------------------------------------------------------- */

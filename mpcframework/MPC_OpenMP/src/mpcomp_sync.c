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
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"


#include "mpcomp_sync.h"

#include "mpcomp_task.h"
#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_openmp_tls.h"
#include "ompt.h"
#include "mpcomp.h"
#include "mpc_common_spinlock.h"
#include "omp_ompt.h"
#include "sched.h"
#include "mpc_common_asm.h"
#include "sctk_thread.h"

/***********
 * ATOMICS *
 ***********/

/*
   This file contains all synchronization functions related to OpenMP
 */

/* TODO move these locks (atomic and critical) in a per-task structure:
   - mpcomp_thread_info, (every instance sharing the same lock)
   - Key
   - TLS if available
 */
TODO( "OpenMP: Anonymous critical and global atomic are not per-task locks" )

/* Atomic emulated */
__UNUSED__ static ompt_wait_id_t __mpcomp_ompt_atomic_lock_wait_id = 0;
static OPA_int_t __mpcomp_atomic_lock_init_once 		= OPA_INT_T_INITIALIZER( 0 );
/* Critical anonymous */
__UNUSED__  static ompt_wait_id_t __mpcomp_ompt_critical_lock_wait_id = 0;
static OPA_int_t __mpcomp_critical_lock_init_once 	= OPA_INT_T_INITIALIZER( 0 );

static mpc_common_spinlock_t *__mpcomp_omp_global_atomic_lock = NULL;
static mpcomp_lock_t 	*__mpcomp_omp_global_critical_lock = NULL;
static mpc_common_spinlock_t 	__mpcomp_global_init_critical_named_lock = SCTK_SPINLOCK_INITIALIZER;

void __mpcomp_atomic_begin( void )
{
	if ( OPA_load_int( &__mpcomp_atomic_lock_init_once ) < 2 )
	{
		//prevent multi call to init
		if ( !OPA_cas_int( &__mpcomp_atomic_lock_init_once, 0, 1 ) )
		{
			__mpcomp_omp_global_atomic_lock = (mpc_common_spinlock_t*) sctk_malloc(sizeof(mpc_common_spinlock_t));
			sctk_assert( __mpcomp_omp_global_atomic_lock );
			mpc_common_spinlock_init( __mpcomp_omp_global_atomic_lock, 0 );
#if OMPT_SUPPORT

			if ( _mpc_omp_ompt_is_enabled() )
			{
				if ( OMPT_Callbacks )
				{
					ompt_callback_lock_init_t callback_init;
					/* Prevent non thread safe wait_id init */
					__mpcomp_ompt_atomic_lock_wait_id = mpcomp_OMPT_gen_wait_id();
					callback_init = ( ompt_callback_lock_init_t ) OMPT_Callbacks[ompt_callback_lock_init];

					if ( callback_init )
					{
						const void *code_ra = __builtin_return_address( 0 );
						callback_init( ompt_mutex_atomic, omp_lock_hint_none, mpcomp_spinlock, __mpcomp_ompt_atomic_lock_wait_id, code_ra );
					}
				}
			}

#endif //OMPT_SUPPORT
			OPA_store_int( &__mpcomp_atomic_lock_init_once, 2 );
		}
		else
		{
			/* Wait lock init */
			while ( OPA_load_int( &__mpcomp_atomic_lock_init_once ) != 2 )
			{
				sctk_cpu_relax();
			}
		}
	}

#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback_acquire;
			callback_acquire = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback_acquire )
			{
				const void *code_ra = __builtin_return_address( 0 );
				callback_acquire( ompt_mutex_atomic, omp_lock_hint_none, mpcomp_spinlock, __mpcomp_ompt_atomic_lock_wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
	mpc_common_spinlock_lock( __mpcomp_omp_global_atomic_lock );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback_acquired;
			callback_acquired = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback_acquired )
			{
				const void *code_ra = __builtin_return_address( 0 );
				callback_acquired( ompt_mutex_atomic, __mpcomp_ompt_atomic_lock_wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
}

void __mpcomp_atomic_end( void )
{
	mpc_common_spinlock_unlock( __mpcomp_omp_global_atomic_lock );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback_released;
			callback_released = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_released];

			if ( callback_released )
			{
				const void *code_ra = __builtin_return_address( 0 );
				callback_released( ompt_mutex_atomic, __mpcomp_ompt_atomic_lock_wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
}


/************
 * CRITICAL *
 ************/

INFO( "Wrong atomic/critical behavior in case of OpenMP oversubscribing" )

TODO( "BUG w/ nested anonymous critical (and maybe named critical) -> need nested locks" )

void __mpcomp_anonymous_critical_begin( void )
{
	if ( OPA_load_int( &__mpcomp_critical_lock_init_once )  != 2 )
	{
		//prevent multi call to init
		if ( !OPA_cas_int( &__mpcomp_critical_lock_init_once, 0, 1 ) )
		{
			__mpcomp_omp_global_critical_lock = (mpcomp_lock_t *) sctk_malloc( sizeof( mpcomp_lock_t ));
			sctk_assert( __mpcomp_omp_global_critical_lock );
			memset( __mpcomp_omp_global_critical_lock, 0, sizeof( mpcomp_lock_t ) );
			mpc_common_spinlock_init( &( __mpcomp_omp_global_critical_lock->lock ), 0 );
#if OMPT_SUPPORT

			if ( _mpc_omp_ompt_is_enabled() )
			{
				/* Prevent non thread safe wait_id init */
				__mpcomp_omp_global_critical_lock->wait_id = mpcomp_OMPT_gen_wait_id();
				__mpcomp_omp_global_critical_lock->hint = omp_lock_hint_none;

				if ( OMPT_Callbacks )
				{
					ompt_callback_lock_init_t callback_init;
					callback_init = ( ompt_callback_lock_init_t ) OMPT_Callbacks[ompt_callback_lock_init];

					if ( callback_init )
					{
						const void *code_ra = __builtin_return_address( 0 );
						const omp_lock_hint_t hint = __mpcomp_omp_global_critical_lock->hint;
						const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
						callback_init( ompt_mutex_critical, hint, mpcomp_mutex, wait_id, code_ra );
					}
				}
			}

#endif //OMPT_SUPPORT
			OPA_store_int( &__mpcomp_critical_lock_init_once, 2 );
		}
		else
		{
			/* Wait lock init */
			while ( OPA_load_int( &__mpcomp_critical_lock_init_once ) != 2 )
			{
				sctk_cpu_relax();
			}
		}
	}

#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback_acquire;
			callback_acquire = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback_acquire )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const omp_lock_hint_t hint = __mpcomp_omp_global_critical_lock->hint;
				const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
				callback_acquire( ompt_mutex_critical, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
	mpc_common_spinlock_lock( &( __mpcomp_omp_global_critical_lock->lock ) );
#if  OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback_acquired;
			callback_acquired = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback_acquired )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
				callback_acquired( ompt_mutex_critical, wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
}

void __mpcomp_anonymous_critical_end( void )
{
	mpc_common_spinlock_unlock( &( __mpcomp_omp_global_critical_lock->lock ) );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback_released;
			callback_released = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_released];

			if ( callback_released )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
				callback_released( ompt_mutex_critical, wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
}

void __mpcomp_named_critical_begin( void **l )
{
	sctk_assert( l );
	mpcomp_lock_t *named_critical_lock;

	if ( *l == NULL )
	{
		mpc_common_spinlock_lock( &( __mpcomp_global_init_critical_named_lock ) );

		if ( *l == NULL )
		{
			named_critical_lock = (mpcomp_lock_t *) sctk_malloc( sizeof( mpcomp_lock_t ));
			sctk_assert( named_critical_lock );
			memset( named_critical_lock, 0, sizeof( mpcomp_lock_t ) );
			mpc_common_spinlock_init( &( named_critical_lock->lock ), 0 );
#if OMPT_SUPPORT

			if ( _mpc_omp_ompt_is_enabled() )
			{
				named_critical_lock->wait_id = mpcomp_OMPT_gen_wait_id();
				named_critical_lock->hint = omp_lock_hint_none;
			}

#endif //OMPT_SUPPORT	
			*l = named_critical_lock;
		}

		mpc_common_spinlock_unlock( &( __mpcomp_global_init_critical_named_lock ) );
	}

	named_critical_lock = ( mpcomp_lock_t * )( *l );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback_acquire;
			callback_acquire = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback_acquire )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const omp_lock_hint_t hint = named_critical_lock->hint;
				const ompt_wait_id_t wait_id = named_critical_lock->wait_id;
				callback_acquire( ompt_mutex_critical, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
	mpc_common_spinlock_lock( &( named_critical_lock->lock ) );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback_acquired;
			callback_acquired = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback_acquired )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const omp_lock_hint_t hint = named_critical_lock->hint;
				const ompt_wait_id_t wait_id = named_critical_lock->wait_id;
				callback_acquired( ompt_mutex_critical, wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
}

void __mpcomp_named_critical_end( void **l )
{
	sctk_assert( l );
	sctk_assert( *l );
	mpcomp_lock_t *named_critical_lock = ( mpcomp_lock_t * )( *l );
	mpc_common_spinlock_unlock( &( named_critical_lock->lock ) );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback_released;
			callback_released = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_released];

			if ( callback_released )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const omp_lock_hint_t hint = named_critical_lock->hint;
				const ompt_wait_id_t wait_id = named_critical_lock->wait_id;
				callback_released( ompt_mutex_critical, wait_id, code_ra );
			}
		}
	}

#endif //OMPT_SUPPORT
}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
	__asm__( ".symver __mpcomp_atomic_begin, GOMP_atomic_start@@GOMP_1.0" );
	__asm__( ".symver __mpcomp_atomic_end, GOMP_atomic_end@@GOMP_1.0" );
	__asm__( ".symver __mpcomp_anonymous_critical_begin, GOMP_critical_start@@GOMP_1.0" );
	__asm__( ".symver __mpcomp_anonymous_critical_end, GOMP_critical_end@@GOMP_1.0" );
	__asm__( ".symver __mpcomp_named_critical_begin, GOMP_critical_name_start@@GOMP_1.0" );
	__asm__( ".symver __mpcomp_named_critical_end, GOMP_critical_name_end@@GOMP_1.0" );
#endif /* OPTIMIZED_GOMP_API_SUPPORT */

/**********
 * SINGLE *
 **********/

/* 
   Perform a single construct.
   This function handles the 'nowait' clause.
   Return '1' if the next single construct has to be executed, '0' otherwise 
 */

int __mpcomp_do_single(void) 
{
	int retval = 0;
  	mpcomp_team_t *team ;	/* Info on the team */

  	/* Handle orphaned directive (initialize OpenMP environment) */
  	__mpcomp_init() ;

  	/* Grab the thread info */
 	mpcomp_thread_t *t = mpcomp_get_thread_tls();

  	/* Number of threads in the current team */
  	const long num_threads = t->info.num_threads;
  	sctk_assert( num_threads > 0 ) ;

  	/* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  	if( num_threads == 1	) 
	{
		retval = 1;
	}
	else
	{
  		/* Grab the team info */
  		sctk_assert( t->instance != NULL ) ;
  		team = t->instance->team ;
  		sctk_assert( team != NULL ) ;

  		int current ;

  		/* Catch the current construct
   	    * and prepare the next single/section construct */
  		current = t->single_sections_current ;
  		t->single_sections_current++ ;

  		sctk_nodebug("[%d]%s : Entering with current %d...", __func__, t->rank,
               current);
  		sctk_nodebug("[%d]%s : team current is %d", __func__, t->rank,
               OPA_load_int(&(team->single_sections_last_current)));

  		if (current == OPA_load_int(&(team->single_sections_last_current))) 
		{
    		const int res = OPA_cas_int(&(team->single_sections_last_current),
                                         current, current + 1);
    		sctk_nodebug("[%d]%s: incr team %d -> %d ==> %d", __func__, t->rank,
                 current, current + 1, res);
    		/* Success => execute the single block */
    		if (res == current) 
			{
				retval = 1;
			}
  		}
	}

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
                t->info.in_single = 1;
                if( retval ) team->info.doing_single = t->rank;
                ompt_data_t* parallel_data = &(team->info.ompt_region_data);
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				ompt_work_t wstype = (retval) ? ompt_work_single_executor : ompt_work_single_other;
				const void* code_ra = __builtin_return_address(0);
				callback( wstype, ompt_scope_begin, parallel_data, task_data, 1, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
	/* 0 means not execute */
 	return retval ;
}

void *__mpcomp_do_single_copyprivate_begin( void )
{
	mpcomp_team_t *team; /* Info on the team */

	if ( __mpcomp_do_single() )
	{
		return NULL;
	}

	/* Grab the thread info */
	mpcomp_thread_t *t = mpcomp_get_thread_tls();
	/* In this flow path, the number of threads should not be 1 */
	sctk_assert( t->info.num_threads > 0 );
	/* Grab the team info */
	sctk_assert( t->instance != NULL );
	team = t->instance->team;
	sctk_assert( team != NULL );
	__mpcomp_barrier();
	return team->single_copyprivate_data;
}

void __mpcomp_do_single_copyprivate_end( void *data )
{
	mpcomp_team_t *team; /* Info on the team */
	mpcomp_thread_t *t = mpcomp_get_thread_tls();

	if ( t->info.num_threads == 1 )
	{
		return;
	}

	/* Grab the team info */
	sctk_assert( t->instance != NULL );
	team = t->instance->team;
	sctk_assert( team != NULL );
	team->single_copyprivate_data = data;
	__mpcomp_barrier();
}

void __mpcomp_single_coherency_entering_parallel_region( void ) {}

void __mpcomp_single_coherency_end_barrier( void )
{
	int i;
	/* Grab the thread info */
	mpcomp_thread_t *t = mpcomp_get_thread_tls();
	/* Number of threads in the current team */
	const long num_threads = t->info.num_threads;
	sctk_assert( num_threads > 0 );

	for ( i = 0; i < num_threads; i++ )
	{
		/* mpcomp_thread_t *target_t = &(t->instance->mvps[i].ptr.mvp->threads[0]);
		mpc_common_debug("%s: thread %d single_sections_current:%d "
		             "single_sections_last_current:%d",
		             __func__, target_t->rank, target_t->single_sections_current,
		             OPA_load_int(
		                 &(t->instance->team->single_sections_last_current))); */
	}
}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
__asm__( ".symver __mpcomp_do_single_copyprivate_end, "
         "GOMP_single_copy_end@@GOMP_1.0" );
__asm__( ".symver __mpcomp_do_single_copyprivate_begin, "
         "GOMP_single_copy_start@@GOMP_1.0" );
__asm__( ".symver __mpcomp_do_single, GOMP_single_start@@GOMP_1.0" );
#endif /* OPTIMIZED_GOMP_API_SUPPORT */



/***********
 * BARRIER *
 ***********/


// missing forward declaration
void _mpc_task_wait( void );

/** Barrier for all threads of the same team */
void __mpcomp_internal_full_barrier(mpcomp_mvp_t *mvp) {
  long b, b_done;
  mpcomp_node_t *c, *new_root;

  sctk_assert(mvp);

  if( mvp->threads->info.num_threads == 1)
  {
    return;
  }

  sctk_assert(mvp->father);
  sctk_assert(mvp->father->instance);
  sctk_assert(mvp->father->instance->team);

  mpcomp_thread_t* thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
  c = mvp->father;
  new_root = thread->instance->root;

#if MPCOMP_TASK
	_mpc_task_wait();
#endif /* MPCOMP_TASK */

  /* Step 1: Climb in the tree */
  b_done = c->barrier_done; /* Move out of sync region? */
  b = OPA_fetch_and_incr_int(&(c->barrier)) + 1;

  while (b == c->barrier_num_threads && c != new_root) {
    OPA_store_int(&(c->barrier), 0);
    c = c->father;
    b_done = c->barrier_done;
    b = OPA_fetch_and_incr_int(&(c->barrier)) + 1;
  }

  /* Step 2 - Wait for the barrier to be done */
  if (c != new_root || (c == new_root && b != c->barrier_num_threads)) {
#if OMPT_SUPPORT
    if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
      ompt_callback_sync_region_t callback;
      callback = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region_wait];

      if( callback ) {
         mpcomp_thread_t *t = mvp->threads;
         ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
         ompt_data_t* task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
         const void* code_ra = __builtin_return_address( 1 );

         callback( ompt_sync_region_barrier, ompt_scope_begin, parallel_data, task_data, code_ra );
      }
    }
#endif /* OMPT_SUPPORT */

    /* Wait for c->barrier == c->barrier_num_threads */
    while (b_done == c->barrier_done) {
      sctk_thread_yield();
#if MPCOMP_TASK
			_mpc_task_schedule( 1 );
#endif /* MPCOMP_TASK */
    }

#if OMPT_SUPPORT
    if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
      ompt_callback_sync_region_t callback;
      callback = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region_wait];

      if( callback ) {
         mpcomp_thread_t *t = mvp->threads;
         ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
         ompt_data_t* task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
         const void* code_ra = __builtin_return_address( 1 );

         callback( ompt_sync_region_barrier, ompt_scope_end, parallel_data, task_data, code_ra );
      }
    }
#endif /* OMPT_SUPPORT */
  } else {
    OPA_store_int(&(c->barrier), 0);

#if MPCOMP_COHERENCY_CHECKING
    mpcomp_for_dyn_coherency_end_barrier();
    mpcomp_single_coherency_end_barrier();
#endif

    c->barrier_done++ ; /* No need to lock I think... */
  }

  /* Step 3 - Go down */
  while ( c->child_type != MPCOMP_CHILDREN_LEAF ) {
    c = c->children.node[mvp->tree_rank[c->depth]];
    c->barrier_done++; /* No need to lock I think... */
  }
#if MPCOMP_TASK
#if MPCOMP_COHERENCY_CHECKING
// mpcomp_task_coherency_barrier();
#endif
#endif /* MPCOMP_TASK */
}

/* Half barrier for the end of a parallel region */
void __mpcomp_internal_half_barrier_start( mpcomp_mvp_t *mvp )
{
	mpcomp_node_t *c, *new_root;
	sctk_assert( mvp );
	sctk_assert( mvp->father );
	sctk_assert( mvp->father->instance );
	sctk_assert( mvp->father->instance->team );
	new_root = mvp->instance->root;
	c = mvp->father;
	sctk_assert( c );
	sctk_assert( new_root != NULL );
#if 0 //MPCOMP_TASK
	( void )mpcomp_thread_tls_store( mvp->threads[0] );
	__mpcomp_internal_full_barrier( mvp );
	( void )mpcomp_thread_tls_store_father(); // To check...
#endif /* MPCOMP_TASK */
	/* Step 1: Climb in the tree */
	long b = OPA_fetch_and_incr_int( &( c->barrier ) ) + 1;

	while ( b == c->barrier_num_threads && c != new_root )
	{
		OPA_store_int( &( c->barrier ), 0 );
		c = c->father;
		b = OPA_fetch_and_incr_int( &( c->barrier ) ) + 1;
	}
}

void __mpcomp_internal_half_barrier_end( mpcomp_mvp_t *mvp )
{
	mpcomp_thread_t *cur_thread;
	mpcomp_node_t *root;
	assert( mvp->threads );
	cur_thread = mvp->threads;

	/* End barrier for master thread */
	if ( cur_thread->rank )
	{
		return;
	}

	root = cur_thread->instance->root;
	const int expected_num_threads = root->barrier_num_threads;

	while ( OPA_load_int( &( root->barrier ) ) != expected_num_threads )
	{
		sctk_thread_yield();
	}

	OPA_store_int( &( root->barrier ), 0 );
}

/*
   OpenMP barrier.
   All threads of the same team must meet.
   This barrier uses some optimizations for threads inside the same microVP.
 */
void __mpcomp_barrier(void) {

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  sctk_nodebug("[%d] %s: Entering w/ %d thread(s)\n", t->rank, __func__,
               t->info.num_threads);

#if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
    ompt_data_t* parallel_data;
    ompt_data_t* task_data;

    if( t->info.in_single ) {
      t->info.in_single = 0;
      ompt_work_t wstype = (t->rank == t->instance->team->info.doing_single) ? ompt_work_single_executor : ompt_work_single_other;
      if( wstype == ompt_work_single_executor ) t->instance->team->info.doing_single = -1;

      ompt_callback_work_t callback_work;
      callback_work = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];

      if( callback_work ) {
        parallel_data = &(t->instance->team->info.ompt_region_data );
        task_data = &( t->task_infos.current_task->ompt_task_data );
        const void* code_ra = __builtin_return_address( 0 );
        callback_work( wstype, ompt_scope_end, parallel_data, task_data, 1, code_ra );
      }
    }

    ompt_callback_sync_region_t callback_sync;
    callback_sync = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region];

    if( callback_sync ) {
      parallel_data = &( t->instance->team->info.ompt_region_data );
      task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
      const void* code_ra = __builtin_return_address( 1 );

      callback_sync( ompt_sync_region_barrier, ompt_scope_begin, parallel_data, task_data, code_ra );
    }
  //__mpcomp_ompt_barrier_begin( false );
  }
#endif /* OMPT_SUPPORT */

  if (t->info.num_threads != 1)
  {
    /* Get the corresponding microVP */
    mpcomp_mvp_t *mvp = t->mvp;
    sctk_assert(mvp != NULL);
    sctk_nodebug("[%d] %s: t->mvp = %p", t->rank, __func__, t->mvp);
 
    /* Call the real barrier */
    sctk_assert( t->info.num_threads == t->mvp->threads->info.num_threads );
    __mpcomp_internal_full_barrier(mvp);
  }

#if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
    ompt_callback_sync_region_t callback;
    callback = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region];

    if( callback ) {
       ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
       ompt_data_t* task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
       const void* code_ra = __builtin_return_address( 1 );

       callback( ompt_sync_region_barrier, ompt_scope_end, parallel_data, task_data, code_ra );
    }
  }
  //__mpcomp_ompt_barrier_end( true );
#endif /* OMPT_SUPPORT */
}


/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
	__asm__( ".symver __mpcomp_barrier, GOMP_barrier@@GOMP_1.0" );
#endif /* OPTIMIZED_GOMP_API_SUPPORT */

/************
 * SECTIONS *
 ************/

/*
   This file contains all functions related to SECTIONS constructs in OpenMP
 */

static int __sync_section_next(mpcomp_thread_t *t,
                                           mpcomp_team_t *team) {
  int r ;
  int success ;

  t->single_sections_current =
      OPA_load_int(&(team->single_sections_last_current));
  sctk_nodebug("[%d] %s: Current = %d (target = %d)", t->rank, __func__,
               t->single_sections_current, t->single_sections_target_current);
  success = 0 ;

  while (t->single_sections_current < t->single_sections_target_current &&
         !success) {
    r = OPA_cas_int(&(team->single_sections_last_current),
                             t->single_sections_current,
                             t->single_sections_current + 1);

    sctk_nodebug("[%d] %s: CAS %d -> %d (res %d)", t->rank, __func__,
                 t->single_sections_current, t->single_sections_current + 1, r);

    if (r != t->single_sections_current) {
      t->single_sections_current =
          OPA_load_int(&(team->single_sections_last_current));
      if (t->single_sections_current > t->single_sections_target_current) {
        t->single_sections_current = t->single_sections_target_current;
      }
    } else {
      success = 1;
    }
  }

  if ( t->single_sections_current < t->single_sections_target_current ) {
    sctk_nodebug("[%d] %s: Success w/ current = %d", t->rank, __func__,
                 t->single_sections_current);
    return t->single_sections_current - t->single_sections_start_current + 1;
  }

  sctk_nodebug("[%d] %s: Fail w/ final current = %d", t->rank, __func__,
               t->single_sections_current);

  return 0 ;
}


void __mpcomp_sections_init( mpcomp_thread_t * t, int nb_sections ) {
  long num_threads ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  sctk_nodebug("[%d] %s: Entering w/ %d section(s)", t->rank, __func__,
               nb_sections);

  /* Update current sections construct and update
   * the target according to the number of sections */
  t->single_sections_start_current = t->single_sections_current ;
  t->single_sections_target_current = t->single_sections_current + nb_sections ;
  t->nb_sections = nb_sections;

  sctk_nodebug("[%d] %s: Current %d, start %d, target %d", t->rank, __func__,
               t->single_sections_current, t->single_sections_start_current,
               t->single_sections_target_current);

  return ;
}

/* Return >0 to execute corresponding section.
   Return 0 otherwise to leave sections construct
   */
int __mpcomp_sections_begin(int nb_sections) {
  mpcomp_thread_t *t ;	/* Info on the current thread */
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Leave if the number of sections is wrong */
  if ( nb_sections <= 0 ) {
	  return 0 ;
  }

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_nodebug("[%d] %s: entering w/ %d section(s)", t->rank, __func__,
               nb_sections);

  __mpcomp_sections_init( t, nb_sections ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
                ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				const void* code_ra = __builtin_return_address(0);	
				callback( ompt_work_sections, ompt_scope_begin, parallel_data, task_data, nb_sections, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute all sections 
   */
  if (num_threads == 1) {
	  /* Start with the first section */
	  t->single_sections_current++ ;
	  return 1 ;
  }

  /* Grab the team info */
  sctk_assert( t->instance != NULL ) ;
  team = t->instance->team ;
  sctk_assert( team != NULL ) ;

  return __sync_section_next( t, team ) ;
}

int __mpcomp_sections_next(void) {
  mpcomp_thread_t *t ;	/* Info on the current thread */
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  sctk_nodebug("[%d] %s: Current %d, start %d, target %d", t->rank, __func__,
               t->single_sections_current, t->single_sections_start_current,
               t->single_sections_target_current);

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute all sections 
   */
  if (num_threads == 1) {
	  /* Proceed to the next section if available */
	  if ( t->single_sections_current >= t->single_sections_target_current ) {
		  /* Update current section construct */
		  t->single_sections_current++ ;
		  return 0 ;
	  }
	  /* Update current section construct */
	  t->single_sections_current++ ;
	  return t->single_sections_current - t->single_sections_start_current ;
  }

  /* Grab the team info */
  sctk_assert( t->instance != NULL ) ;
  team = t->instance->team ;
  sctk_assert( team != NULL ) ;

  return __sync_section_next( t, team ) ;
}


void __mpcomp_sections_end_nowait(void) { 

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
  				mpcomp_thread_t *t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
                ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				const void* code_ra = __builtin_return_address(0);	
				callback( ompt_work_sections, ompt_scope_end, parallel_data, task_data, t->nb_sections, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
}

void __mpcomp_sections_end(void) 
{ 
	__mpcomp_sections_end_nowait();
	__mpcomp_barrier(); 
}

int __mpcomp_sections_coherency_exiting_paralel_region(void) { return 0; }

int __mpcomp_sections_coherency_barrier(void) { return 0; }


/*********
 * LOCKS *
 *********/

/**
 *  \fn void omp_init_lock_with_hint( omp_lock_t *lock, omp_lock_hint_t hint)
 *  \brief These routines initialize an OpenMP lock with a hint. The effect of
 * the hint is implementation-defined. The OpenMP implementation can ignore the
 * hint without changing program semantics.
 */
static void __sync_lock_init_with_hint( omp_lock_t *lock, omp_lock_hint_t hint )
{
	mpcomp_lock_t *mpcomp_user_lock = NULL;
	__mpcomp_init();
	mpcomp_user_lock = ( mpcomp_lock_t * )mpcomp_alloc( sizeof( mpcomp_lock_t ) );
	sctk_assert( mpcomp_user_lock );
	memset( mpcomp_user_lock, 0, sizeof( mpcomp_lock_t ) );
	mpc_common_spinlock_init( &( mpcomp_user_lock->lock ), 0 );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_lock_init_t callback;
			callback = ( ompt_callback_lock_init_t ) OMPT_Callbacks[ompt_callback_lock_init];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_OMPT_gen_wait_id();
				mpcomp_user_lock->wait_id = wait_id;
				callback( ompt_mutex_lock, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	sctk_assert( lock );
	*lock = mpcomp_user_lock;
	mpcomp_user_lock->hint = hint;
}

void omp_init_lock_with_hint( omp_lock_t *lock, omp_lock_hint_t hint )
{
	__sync_lock_init_with_hint( lock, hint );
}

/**
 *  \fn void omp_init_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 *  \param omp_lock_t adress of a lock address (\see mpcomp.h)
 */
void omp_init_lock( omp_lock_t *lock )
{
	__sync_lock_init_with_hint( lock, omp_lock_hint_none );
}

void omp_destroy_lock( omp_lock_t *lock )
{
	mpcomp_lock_t *mpcomp_user_lock = NULL;
	sctk_assert( lock );
	mpcomp_user_lock = ( mpcomp_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_lock_destroy_t callback;
			callback = ( ompt_callback_lock_destroy_t ) OMPT_Callbacks[ompt_callback_lock_destroy];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;
				callback( ompt_mutex_lock, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	sctk_free( mpcomp_user_lock );
	*lock = NULL;
}

void omp_set_lock( omp_lock_t *lock )
{
	mpcomp_lock_t *mpcomp_user_lock = NULL;
	sctk_assert( lock );
	mpcomp_user_lock = ( mpcomp_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback;
			callback = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;
				const unsigned hint = mpcomp_user_lock->hint;
				callback( ompt_mutex_lock, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	mpc_common_spinlock_lock( &( mpcomp_user_lock->lock ) );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback;
			callback = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;
				callback( ompt_mutex_lock, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
}

void omp_unset_lock( omp_lock_t *lock )
{
	mpcomp_lock_t *mpcomp_user_lock = NULL;
	sctk_assert( lock );
	mpcomp_user_lock = ( mpcomp_lock_t * )*lock;
	mpc_common_spinlock_unlock( &( mpcomp_user_lock->lock ) );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback;
			callback = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_released];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;
				callback( ompt_mutex_lock, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
}

int omp_test_lock( omp_lock_t *lock )
{
	int retval;
	mpcomp_lock_t *mpcomp_user_lock = NULL;
	sctk_assert( lock );
	mpcomp_user_lock = ( mpcomp_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback;
			callback = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;
				const unsigned hint = mpcomp_user_lock->hint;
				callback( ompt_mutex_lock, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	retval = !mpc_common_spinlock_trylock( &( mpcomp_user_lock->lock ) );
#if OMPT_SUPPORT

	if ( retval && _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback;
			callback = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;
				callback( ompt_mutex_lock, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	return retval;
}

static void __sync_nest_lock_init_with_hint( omp_nest_lock_t *lock, omp_lock_hint_t hint )
{
	mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
	__mpcomp_init();
	mpcomp_user_nest_lock = ( mpcomp_nest_lock_t * )mpcomp_alloc( sizeof( mpcomp_nest_lock_t ) );
	sctk_assert( mpcomp_user_nest_lock );
	memset( mpcomp_user_nest_lock, 0, sizeof( mpcomp_nest_lock_t ) );
	mpc_common_spinlock_init( &( mpcomp_user_nest_lock->lock ), 0 );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_lock_init_t callback;
			callback = ( ompt_callback_lock_init_t ) OMPT_Callbacks[ompt_callback_lock_init];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_OMPT_gen_wait_id();
				mpcomp_user_nest_lock->wait_id = wait_id;
				callback( ompt_mutex_nest_lock, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	sctk_assert( lock );
	*lock = mpcomp_user_nest_lock;
	mpcomp_user_nest_lock->hint = hint;
}

/**
 *  \fn void omp_init_nest_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 * \param omp_lock_t adress of a lock address (\see mpcomp.h)
 */
void omp_init_nest_lock( omp_nest_lock_t *lock )
{
	__sync_nest_lock_init_with_hint( lock, omp_lock_hint_none );
}

void omp_init_nest_lock_with_hint( omp_nest_lock_t *lock, omp_lock_hint_t hint )
{
	__sync_nest_lock_init_with_hint( lock, hint );
}

void omp_destroy_nest_lock( omp_nest_lock_t *lock )
{
	mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
	sctk_assert( lock );
	mpcomp_user_nest_lock = ( mpcomp_nest_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_lock_destroy_t callback;
			callback = ( ompt_callback_lock_destroy_t ) OMPT_Callbacks[ompt_callback_lock_destroy];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
				callback( ompt_mutex_nest_lock, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	sctk_free( mpcomp_user_nest_lock );
	*lock = NULL;
}

void omp_set_nest_lock( omp_nest_lock_t *lock )
{
	mpcomp_nest_lock_t *mpcomp_user_nest_lock;
	__mpcomp_init();
	mpcomp_thread_t *thread = mpcomp_get_thread_tls();
	sctk_assert( lock );
	mpcomp_user_nest_lock = ( mpcomp_nest_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback;
			callback = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
				const unsigned hint = mpcomp_user_nest_lock->hint;
				callback( ompt_mutex_nest_lock, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
#if MPCOMP_TASK

	if ( mpcomp_nest_lock_test_task( thread, mpcomp_user_nest_lock ) )
#else
	if ( mpcomp_user_nest_lock->owner_thread != ( void * )thread )
#endif
	{
		mpc_common_spinlock_lock( &( mpcomp_user_nest_lock->lock ) );
		mpcomp_user_nest_lock->owner_thread = thread;
#if MPCOMP_TASK
		mpcomp_user_nest_lock->owner_task =
		    MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
#endif
	}

#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback;
			callback = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
				callback( ompt_mutex_lock, wait_id, code_ra );
			}

			if ( mpcomp_user_nest_lock->nb_nested == 0 )
			{
				ompt_callback_nest_lock_t callback_nest;
				callback_nest = ( ompt_callback_nest_lock_t ) OMPT_Callbacks[ompt_callback_nest_lock];

				if ( callback_nest )
				{
					const void *code_ra = __builtin_return_address( 0 );
					const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
					callback_nest( ompt_scope_begin, wait_id, code_ra );
				}
			}
		}
	}

#endif /* OMPT_SUPPORT */
	mpcomp_user_nest_lock->nb_nested += 1;
}

void omp_unset_nest_lock( omp_nest_lock_t *lock )
{
	mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
	sctk_assert( lock );
	mpcomp_user_nest_lock = ( mpcomp_nest_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback;
			callback = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_released];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
				callback( ompt_mutex_nest_lock, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
	mpcomp_user_nest_lock->nb_nested -= 1;

	if ( mpcomp_user_nest_lock->nb_nested == 0 )
	{
		mpcomp_user_nest_lock->owner_thread = NULL;
#if MPCOMP_TASK
		mpcomp_user_nest_lock->owner_task = NULL;
#endif
#if OMPT_SUPPORT

		if ( _mpc_omp_ompt_is_enabled() )
		{
			if ( OMPT_Callbacks )
			{
				ompt_callback_nest_lock_t callback_nest;
				callback_nest = ( ompt_callback_nest_lock_t ) OMPT_Callbacks[ompt_callback_nest_lock];

				if ( callback_nest )
				{
					const void *code_ra = __builtin_return_address( 0 );
					const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
					callback_nest( ompt_scope_end, wait_id, code_ra );
				}
			}
		}

#endif /* OMPT_SUPPORT */
		mpc_common_spinlock_unlock( &( mpcomp_user_nest_lock->lock ) );
	}
}

int omp_test_nest_lock( omp_nest_lock_t *lock )
{
	mpcomp_nest_lock_t *mpcomp_user_nest_lock;
	__mpcomp_init();
	mpcomp_thread_t *thread = mpcomp_get_thread_tls();
	sctk_assert( lock );
	mpcomp_user_nest_lock = ( mpcomp_nest_lock_t * )*lock;
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_acquire_t callback;
			callback = ( ompt_callback_mutex_acquire_t ) OMPT_Callbacks[ompt_callback_mutex_acquire];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
				const unsigned hint = mpcomp_user_nest_lock->hint;
				callback( ompt_mutex_nest_lock, hint, mpcomp_mutex, wait_id, code_ra );
			}
		}
	}

#endif /* OMPT_SUPPORT */
#if MPCOMP_TASK

	if ( mpcomp_nest_lock_test_task( thread, mpcomp_user_nest_lock ) )
#else
	if ( mpcomp_user_nest_lock->owner_thread != ( void * )thread )
#endif
	{
		if ( mpc_common_spinlock_trylock( &( mpcomp_user_nest_lock->lock ) ) )
		{
			return 0;
		}

		mpcomp_user_nest_lock->owner_thread = ( void * )thread;
#if MPCOMP_TASK
		mpcomp_user_nest_lock->owner_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
#endif
	}

#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_mutex_t callback;
			callback = ( ompt_callback_mutex_t ) OMPT_Callbacks[ompt_callback_mutex_acquired];

			if ( callback )
			{
				const void *code_ra = __builtin_return_address( 0 );
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
				callback( ompt_mutex_nest_lock, wait_id, code_ra );
			}

			if ( mpcomp_user_nest_lock->nb_nested == 0 )
			{
				ompt_callback_nest_lock_t callback_nest;
				callback_nest = ( ompt_callback_nest_lock_t ) OMPT_Callbacks[ompt_callback_nest_lock];

				if ( callback_nest )
				{
					const void *code_ra = __builtin_return_address( 0 );
					const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;
					callback_nest( ompt_scope_begin, wait_id, code_ra );
				}
			}
		}
	}

#endif /* OMPT_SUPPORT */
	mpcomp_user_nest_lock->nb_nested += 1;
	return mpcomp_user_nest_lock->nb_nested;
}

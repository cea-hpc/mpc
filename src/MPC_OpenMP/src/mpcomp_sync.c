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

#include "mpcomp_sync.h"
#include "mpc_common_debug.h"
#include "mpcomp_task.h"
#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_openmp_tls.h"
#include "mpc_omp.h"
#include "mpc_common_spinlock.h"
#include "sched.h"
#include "mpc_common_asm.h"
#include "mpc_thread.h"
#include "mpcompt_sync.h"
#include "mpcompt_mutex.h"
#include "mpcompt_workShare.h"
#include "mpcompt_frame.h"

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
#if OMPT_SUPPORT
static ompt_wait_id_t _mpcomp_ompt_atomic_lock_wait_id = 0;
#endif /* OMPT_SUPPORT */
/* Critical anonymous */
//static OPA_int_t _mpcomp_critical_lock_init_once 	= OPA_INT_T_INITIALIZER( 0 );

//static mpc_common_spinlock_t *_mpcomp_omp_global_atomic_lock = NULL;
//static omp_lock_t 	*_mpcomp_omp_global_critical_lock = NULL;
static mpc_common_spinlock_t 	_mpcomp_global_init_critical_named_lock = SCTK_SPINLOCK_INITIALIZER;

void mpc_omp_atomic_begin( void )
{
  mpc_omp_team_t *team ;
  mpc_omp_init() ;
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

#if OMPT_SUPPORT
            _mpcomp_ompt_atomic_lock_wait_id = _mpc_omp_ompt_mutex_gen_wait_id();
            _mpc_omp_ompt_callback_lock_init( ompt_mutex_atomic,
                                          omp_lock_hint_none,
                                          ompt_mutex_impl_spinlock,
                                          _mpcomp_ompt_atomic_lock_wait_id );
#endif /* OMPT_SUPPORT */

  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  assert( t->instance != NULL ) ;
  team = t->instance->team ;
  assert( team != NULL ) ;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquire( ompt_mutex_atomic,
                                      omp_lock_hint_none,
                                      ompt_mutex_impl_spinlock,
                                      _mpcomp_ompt_atomic_lock_wait_id );
#endif /* OMPT_SUPPORT */

	mpc_common_spinlock_lock( &(team->atomic_lock ));

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_atomic, _mpcomp_ompt_atomic_lock_wait_id );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_atomic_end( void )
{
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_omp_team_t *team ;
  assert( t->instance != NULL ) ;
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */
  team = t->instance->team ;
  assert( team != NULL ) ;

	mpc_common_spinlock_unlock( &(team->atomic_lock ));

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_released( ompt_mutex_atomic, _mpcomp_ompt_atomic_lock_wait_id );
#endif /* OMPT_SUPPORT */
}


/************
 * CRITICAL *
 ************/

INFO( "Wrong atomic/critical behavior in case of OpenMP oversubscribing" )

TODO( "BUG w/ nested anonymous critical (and maybe named critical) -> need nested locks" )

void mpc_omp_anonymous_critical_begin( void )
{
  mpc_omp_team_t *team ;
  mpc_omp_init() ;
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  assert( t->instance != NULL ) ;
  team = t->instance->team ;
  assert( team != NULL ) ;
	omp_lock_t* critical_lock = team->critical_lock;
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

#if OMPT_SUPPORT
            critical_lock->ompt_wait_id = (uint64_t) _mpc_omp_ompt_mutex_gen_wait_id();
            critical_lock->hint = omp_lock_hint_none;
            _mpc_omp_ompt_callback_lock_init( ompt_mutex_critical,
                                          omp_lock_hint_none,
                                          ompt_mutex_impl_mutex,
                                          (ompt_wait_id_t) critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquire( ompt_mutex_critical,
                                      critical_lock->hint,
                                      ompt_mutex_impl_mutex,
                                      (ompt_wait_id_t) critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

	mpc_common_spinlock_lock( &(critical_lock->lock ));

#if  OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_critical,
                                       (ompt_wait_id_t) critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_anonymous_critical_end( void )
{
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
  mpc_omp_team_t *team ;
  assert( t->instance != NULL ) ;
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */
  team = t->instance->team ;
  assert( team != NULL ) ;
	omp_lock_t* critical_lock = team->critical_lock;

	mpc_common_spinlock_unlock( &(critical_lock->lock ) );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_released( ompt_mutex_critical,
                                       (ompt_wait_id_t) critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_named_critical_begin( void **l )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	assert( l );

	omp_lock_t *named_critical_lock;

	if ( *l == NULL )
	{
		mpc_common_spinlock_lock( &( _mpcomp_global_init_critical_named_lock ) );

		if ( *l == NULL )
		{
			named_critical_lock = (omp_lock_t *) sctk_malloc( sizeof( omp_lock_t ));
			assert( named_critical_lock );
			memset( named_critical_lock, 0, sizeof( omp_lock_t ) );
			mpc_common_spinlock_init( &( named_critical_lock->lock ), 0 );

#if OMPT_SUPPORT
            named_critical_lock->ompt_wait_id = (uint64_t) _mpc_omp_ompt_mutex_gen_wait_id();
            named_critical_lock->hint = omp_lock_hint_none;
#endif /* OMPT_SUPPORT */

			*l = named_critical_lock;
		}

		mpc_common_spinlock_unlock( &( _mpcomp_global_init_critical_named_lock ) );
	}

	named_critical_lock = ( omp_lock_t * )( *l );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquire( ompt_mutex_critical,
                                      named_critical_lock->hint,
                                      ompt_mutex_impl_mutex,
                                      (ompt_wait_id_t) named_critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

	mpc_common_spinlock_lock( &( named_critical_lock->lock ) );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_critical,
                                       (ompt_wait_id_t) named_critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_named_critical_end( void **l )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	assert( l );
	assert( *l );

	omp_lock_t *named_critical_lock = ( omp_lock_t * )( *l );
	mpc_common_spinlock_unlock( &( named_critical_lock->lock ) );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_released( ompt_mutex_critical,
                                       (ompt_wait_id_t) named_critical_lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

/**********
 * SINGLE *
 **********/

/* 
   Perform a single construct.
   This function handles the 'nowait' clause.
   Return '1' if the next single construct has to be executed, '0' otherwise 
 */

int mpc_omp_do_single(void) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	int retval = 0;
  	mpc_omp_team_t *team ;	/* Info on the team */

  	/* Handle orphaned directive (initialize OpenMP environment) */
  	mpc_omp_init() ;

  	/* Grab the thread info */
 	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();

  	/* Number of threads in the current team */
  	const long num_threads = t->info.num_threads;
  	assert( num_threads > 0 ) ;

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
  		assert( t->instance != NULL ) ;
  		team = t->instance->team ;
  		assert( team != NULL ) ;

  		int current ;

  		/* Catch the current construct
   	    * and prepare the next single/section construct */
  		current = t->single_sections_current ;
  		t->single_sections_current++ ;

  		mpc_common_nodebug("[%d]%s : Entering with current %d...", __func__, t->rank,
               current);
  		mpc_common_nodebug("[%d]%s : team current is %d", __func__, t->rank,
               OPA_load_int(&(team->single_sections_last_current)));

  		if (current == OPA_load_int(&(team->single_sections_last_current))) 
		{
    		const int res = OPA_cas_int(&(team->single_sections_last_current),
                                         current, current + 1);
    		mpc_common_nodebug("[%d]%s: incr team %d -> %d ==> %d", __func__, t->rank,
                 current, current + 1, res);
    		/* Success => execute the single block */
    		if (res == current) 
			{
				retval = 1;
			}
  		}
	}

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( (retval) ? ompt_work_single_executor : ompt_work_single_other,
                             ompt_scope_begin,
                             1 );
#endif /* OMPT_SUPPORT */

	/* 0 means not execute */
 	return retval ;
}

void *mpc_omp_do_single_copyprivate_begin( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    /* Info on the team */
	mpc_omp_team_t *team;

	if ( mpc_omp_do_single() )
	{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
        _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

		return NULL;
	}

	/* Grab the thread info */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();

	/* In this flow path, the number of threads should not be 1 */
	assert( t->info.num_threads > 0 );

	/* Grab the team info */
	assert( t->instance != NULL );
	team = t->instance->team;
	assert( team != NULL );

	mpc_omp_barrier();

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

	return team->single_copyprivate_data;
}

void mpc_omp_do_single_copyprivate_end( void *data )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_omp_team_t *team; /* Info on the team */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();

	if ( t->info.num_threads == 1 )
	{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
        _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

		return;
	}

	/* Grab the team info */
	assert( t->instance != NULL );
	team = t->instance->team;
	assert( team != NULL );

	team->single_copyprivate_data = data;

	mpc_omp_barrier();

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void _mpc_omp_single_coherency_entering_parallel_region( void ) {}

void _mpc_omp_single_coherency_end_barrier( void )
{
	int i;
	/* Grab the thread info */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
	/* Number of threads in the current team */
	const long num_threads = t->info.num_threads;
	assert( num_threads > 0 );

	for ( i = 0; i < num_threads; i++ )
	{
		/* mpc_omp_thread_t *target_t = &(t->instance->mvps[i].ptr.mvp->threads[0]);
		mpc_common_debug("%s: thread %d single_sections_current:%d "
		             "single_sections_last_current:%d",
		             __func__, target_t->rank, target_t->single_sections_current,
		             OPA_load_int(
		                 &(t->instance->team->single_sections_last_current))); */
	}
}

/***********
 * BARRIER *
 ***********/

/** Barrier for all threads of the same team */
void
_mpc_omp_internal_full_barrier(mpc_omp_mvp_t * mvp)
{
    /**
     * TODO : compare this naive barrier approach with the current implementation
     *
     *   assert(mvp);
     *   
     *   _mpc_omp_task_wait();
     *  
     *   mpc_omp_team_t * team = mvp->instance->team;
     *   int num_threads = team->info.num_threads;
     *   if (num_threads == 1) return ;
     *  
     *   int old_version = team->barrier_version;
     *  
     *   if (OPA_fetch_and_incr_int(&(team->threads_in_barrier)) == num_threads - 1)
     *   {
     *       OPA_store_int(&(team->threads_in_barrier), 0);
     *       ++team->barrier_version;
     *   }   
     *   while (team->barrier_version == old_version)
     *   {
     *       _mpc_omp_task_schedule();
     *   } 
    */

    assert(mvp);

    /* wait for children tasks to complete */
    _mpc_omp_task_wait();

#if OMPT_SUPPORT
    ompt_sync_region_t kind = thread->reduction_method ? ompt_sync_region_reduction: ompt_sync_region_barrier;
#endif /* OMPT_SUPPORT */

    if( mvp->threads->info.num_threads == 1)
    {
#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_sync_region_wait( kind, ompt_scope_begin );
        _mpc_omp_ompt_callback_sync_region_wait( kind, ompt_scope_end );
#endif /* OMPT_SUPPORT */
        return ;
    }

    assert(mvp->father);
    assert(mvp->father->instance);
    assert(mvp->father->instance->team);

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    mpc_omp_node_t * c = mvp->father;
    mpc_omp_node_t * new_root = thread->instance->root;

    /* Step 1: Climb in the tree */
    long b_done = c->barrier_done; /* Move out of sync region? */
    long b = OPA_fetch_and_incr_int(&(c->barrier)) + 1;

    while (b == c->barrier_num_threads && c != new_root)
    {
        OPA_store_int(&(c->barrier), 0);
        c = c->father;
        b_done = c->barrier_done;
        b = OPA_fetch_and_incr_int(&(c->barrier)) + 1;
    }

    /* Step 2 - Wait for the barrier to be done */
    if (c != new_root || (c == new_root && b != c->barrier_num_threads))
    {
#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_sync_region_wait( kind, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

        /* Wait for c->barrier == c->barrier_num_threads */
        while (b_done == c->barrier_done)
        {
            mpc_thread_yield();
            _mpc_omp_task_schedule();
            _mpc_omp_task_wait();
        }

#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_sync_region_wait( kind, ompt_scope_end );
#endif /* OMPT_SUPPORT */
    } else {
#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_sync_region_wait( kind, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

        OPA_store_int(&(c->barrier), 0);

#if MPC_OMP_COHERENCY_CHECKING
        mpc_omp_for_dyn_coherency_end_barrier();
        mpc_omp_single_coherency_end_barrier();
#endif

        c->barrier_done++ ; /* No need to lock I think... */

#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_sync_region_wait( kind, ompt_scope_end );
#endif /* OMPT_SUPPORT */
    }

    /* Step 3 - Go down */
    while (c->child_type != MPC_OMP_CHILDREN_LEAF)
    {
        c = c->children.node[mvp->tree_rank[c->depth]];
        c->barrier_done++; /* No need to lock I think... */
    }
}

/* Half barrier for the end of a parallel region */
void _mpc_omp_internal_half_barrier_start( mpc_omp_mvp_t *mvp )
{
    mpc_omp_node_t *c, *new_root;
    assert( mvp );
    assert( mvp->father );
    assert( mvp->father->instance );
    assert( mvp->father->instance->team );
    new_root = mvp->instance->root;
    c = mvp->father;
    assert( c );
    assert( new_root != NULL );
#if 0
    ( void )mpc_omp_thread_tls_store( mvp->threads[0] );
    _mpc_omp_internal_full_barrier( mvp );
    ( void )mpc_omp_thread_tls_store_father(); // To check...
#endif
    /* Step 1: Climb in the tree */
    long b = OPA_fetch_and_incr_int( &( c->barrier ) ) + 1;

    while ( b == c->barrier_num_threads && c != new_root )
    {
        OPA_store_int( &( c->barrier ), 0 );
        c = c->father;
        b = OPA_fetch_and_incr_int( &( c->barrier ) ) + 1;
    }
}

void _mpc_omp_internal_half_barrier_end( mpc_omp_mvp_t *mvp )
{
    mpc_omp_thread_t *cur_thread;
    mpc_omp_node_t *root;
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
        mpc_thread_yield();
    }

    OPA_store_int( &( root->barrier ), 0 );
}

/*
   OpenMP barrier.
   All threads of the same team must meet.
   This barrier uses some optimizations for threads inside the same microVP.
 */
void mpc_omp_barrier(void) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

  /* Handle orphaned directive (initialize OpenMP environment) */
  mpc_omp_init();

  /* Grab info on the current thread */
  mpc_omp_thread_t *t = mpc_omp_get_thread_tls();

  mpc_common_nodebug("[%d] %s: Entering w/ %d thread(s)\n", t->rank, __func__,
               t->info.num_threads);

#if OMPT_SUPPORT
  _mpc_omp_ompt_callback_sync_region( ompt_sync_region_barrier, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

  if (t->info.num_threads != 1)
  {
    /* Get the corresponding microVP */
    mpc_omp_mvp_t *mvp = t->mvp;
    assert(mvp != NULL);
    mpc_common_nodebug("[%d] %s: t->mvp = %p", t->rank, __func__, t->mvp);
 
    /* Call the real barrier */
    assert( t->info.num_threads == t->mvp->threads->info.num_threads );
    _mpc_omp_internal_full_barrier(mvp);
  }
#if OMPT_SUPPORT
  else {
    _mpc_omp_ompt_callback_sync_region_wait( ompt_sync_region_barrier, ompt_scope_begin );
    _mpc_omp_ompt_callback_sync_region_wait( ompt_sync_region_barrier, ompt_scope_end );
  }
#endif /* OMPT_SUPPORT */

#if OMPT_SUPPORT
  _mpc_omp_ompt_callback_sync_region( ompt_sync_region_barrier, ompt_scope_end );

  if( t->info.in_single ) {
    _mpc_omp_ompt_callback_work( (t->rank == t->instance->team->info.doing_single) ?
                             ompt_work_single_executor : ompt_work_single_other,
                             ompt_scope_end, 1 );
  }
#endif /* OMPT_SUPPORT */
}


/************
 * SECTIONS *
 ************/

/*
   This file contains all functions related to SECTIONS constructs in OpenMP
 */

static int __sync_section_next(mpc_omp_thread_t *t,
                                           mpc_omp_team_t *team) {
  int r ;
  int success ;

  t->single_sections_current =
      OPA_load_int(&(team->single_sections_last_current));
  mpc_common_nodebug("[%d] %s: Current = %d (target = %d)", t->rank, __func__,
               t->single_sections_current, t->single_sections_target_current);
  success = 0 ;

  while (t->single_sections_current < t->single_sections_target_current &&
         !success) {
    r = OPA_cas_int(&(team->single_sections_last_current),
                             t->single_sections_current,
                             t->single_sections_current + 1);

    mpc_common_nodebug("[%d] %s: CAS %d -> %d (res %d)", t->rank, __func__,
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
    mpc_common_nodebug("[%d] %s: Success w/ current = %d", t->rank, __func__,
                 t->single_sections_current);
    return t->single_sections_current - t->single_sections_start_current + 1;
  }

  mpc_common_nodebug("[%d] %s: Fail w/ final current = %d", t->rank, __func__,
               t->single_sections_current);

  return 0 ;
}


void _mpc_omp_sections_init( mpc_omp_thread_t * t, int nb_sections ) {
  long  __UNUSED__ num_threads ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  assert( num_threads > 0 ) ;

  mpc_common_nodebug("[%d] %s: Entering w/ %d section(s)", t->rank, __func__,
               nb_sections);

  /* Update current sections construct and update
   * the target according to the number of sections */
  t->single_sections_start_current = t->single_sections_current ;
  t->single_sections_target_current = t->single_sections_current + nb_sections ;
  t->nb_sections = nb_sections;

#if OMPT_SUPPORT
  _mpc_omp_ompt_callback_work( ompt_work_sections, ompt_scope_begin, nb_sections );
#endif /* OMPT_SUPPORT */

  mpc_common_nodebug("[%d] %s: Current %d, start %d, target %d", t->rank, __func__,
               t->single_sections_current, t->single_sections_start_current,
               t->single_sections_target_current);

  return ;
}

/* Return >0 to execute corresponding section.
   Return 0 otherwise to leave sections construct
   */
int mpc_omp_sections_begin(int nb_sections) {
  mpc_omp_thread_t *t ;	/* Info on the current thread */
  mpc_omp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Leave if the number of sections is wrong */
  if ( nb_sections <= 0 ) {
	  return 0 ;
  }

  /* Handle orphaned directive (initialize OpenMP environment) */
  mpc_omp_init() ;

  /* Grab the thread info */
  t = (mpc_omp_thread_t *) mpc_omp_tls ;
  assert( t != NULL ) ;

  mpc_common_nodebug("[%d] %s: entering w/ %d section(s)", t->rank, __func__,
               nb_sections);

  _mpc_omp_sections_init( t, nb_sections ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  assert( num_threads > 0 ) ;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute all sections 
   */
  if (num_threads == 1) {
	  /* Start with the first section */
	  t->single_sections_current++ ;
	  return 1 ;
  }

  /* Grab the team info */
  assert( t->instance != NULL ) ;
  team = t->instance->team ;
  assert( team != NULL ) ;

  return __sync_section_next( t, team ) ;
}

int mpc_omp_sections_next(void) {
  mpc_omp_thread_t *t ;	/* Info on the current thread */
  mpc_omp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Grab the thread info */
  t = (mpc_omp_thread_t *) mpc_omp_tls ;
  assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  assert( num_threads > 0 ) ;

  mpc_common_nodebug("[%d] %s: Current %d, start %d, target %d", t->rank, __func__,
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
  assert( t->instance != NULL ) ;
  team = t->instance->team ;
  assert( team != NULL ) ;

  return __sync_section_next( t, team ) ;
}

void mpc_omp_sections_end_nowait(void) { 
#if OMPT_SUPPORT
  _mpc_omp_ompt_callback_work( ompt_work_sections, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_sections_end(void) 
{ 
	mpc_omp_sections_end_nowait();
	mpc_omp_barrier(); 
}

int _mpc_omp_sections_coherency_exiting_paralel_region(void) { return 0; }

int _mpc_omp_sections_coherency_barrier(void) { return 0; }


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
	assert(lock);

	mpc_omp_init();

	memset(lock, 0, sizeof(omp_lock_t));
	mpc_common_spinlock_init(&(lock->lock), 0 );
	lock->hint = hint;

#if OMPT_SUPPORT
    lock->ompt_wait_id = (uint64_t) _mpc_omp_ompt_mutex_gen_wait_id();
    _mpc_omp_ompt_callback_lock_init( ompt_mutex_lock,
                                  hint,
                                  ompt_mutex_impl_mutex,
                                  (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

void omp_init_lock_with_hint( omp_lock_t *lock, omp_lock_hint_t hint )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */

	__sync_lock_init_with_hint( lock, hint );
}

/**
 *  \fn void omp_init_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 *  \param omp_lock_t adress of a lock address (\see mpc_omp.h)
 */
void omp_init_lock( omp_lock_t *lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */

	__sync_lock_init_with_hint( lock, omp_lock_hint_none );
}

void omp_destroy_lock( omp_lock_t *lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_callback_lock_destroy( ompt_mutex_lock, (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

	sctk_free(lock);
}

void omp_set_lock( omp_lock_t *lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_callback_mutex_acquire(ompt_mutex_lock, lock->hint, ompt_mutex_impl_mutex, (ompt_wait_id_t) lock->ompt_wait_id);
#endif /* OMPT_SUPPORT */

	mpc_common_spinlock_lock(&(lock->lock));

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_lock, (ompt_wait_id_t) lock->ompt_wait_id);
#endif /* OMPT_SUPPORT */
}

void omp_unset_lock( omp_lock_t *lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */

	mpc_common_spinlock_unlock(&(lock->lock));

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_released( ompt_mutex_lock, (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

int omp_test_lock( omp_lock_t *lock )
{
#if OMPT_SUPPORT
# if MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
# endif /* MPCOMPT_HAS_FRAME_SUPPORT */
    _mpc_omp_ompt_callback_mutex_acquire(ompt_mutex_lock, lock->hint, ompt_mutex_impl_mutex, (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

	int retval = !mpc_common_spinlock_trylock(&(lock->lock));

#if OMPT_SUPPORT
    if (retval)
    {
        _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_lock, (ompt_wait_id_t) lock->ompt_wait_id );
    }

#if MPCOMPT_HAS_FRAME_SUPPORT
    /* If outter call, unset it here */
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* MPCOMPT_HAS_FRAME_SUPPORT */
#endif /* OMPT_SUPPORT */

	return retval;
}

static void __sync_nest_lock_init_with_hint( omp_nest_lock_t *lock, omp_lock_hint_t hint )
{
	assert(lock);

	mpc_omp_init();

	memset(lock, 0, sizeof(omp_nest_lock_t));
	mpc_common_spinlock_init(&(lock->lock), 0);
	lock->hint = hint;

#if OMPT_SUPPORT
    lock->ompt_wait_id = (uint64_t) _mpc_omp_ompt_mutex_gen_wait_id();

    _mpc_omp_ompt_callback_lock_init( ompt_mutex_nest_lock,
                                  hint,
                                  ompt_mutex_impl_mutex,
                                  (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

/**
 *  \fn void omp_init_nest_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 * \param omp_lock_t adress of a lock address (\see mpc_omp.h)
 */
void omp_init_nest_lock( omp_nest_lock_t *lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */

	__sync_nest_lock_init_with_hint( lock, omp_lock_hint_none );
}

void omp_init_nest_lock_with_hint( omp_nest_lock_t *lock, omp_lock_hint_t hint )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */

	__sync_nest_lock_init_with_hint( lock, hint );
}

void omp_destroy_nest_lock( omp_nest_lock_t *lock )
{
    assert(lock);
	mpc_omp_init();
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_callback_lock_destroy( ompt_mutex_nest_lock, (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
	sctk_free(lock);
}

void omp_set_nest_lock( omp_nest_lock_t *lock )
{
    assert(lock);
	
    mpc_omp_init();

	mpc_omp_thread_t *thread = mpc_omp_get_thread_tls();

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_callback_mutex_acquire( ompt_mutex_nest_lock,
                                      lock->hint,
                                      ompt_mutex_impl_mutex,
                                      (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

	if (omp_nest_lock_test_task(thread, lock))
	{
		mpc_common_spinlock_lock(&(lock->lock));
		lock->owner_thread = thread;
		lock->owner_task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread );
#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_nest_lock,
                                       (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
	}

    lock->nb_nested += 1;

#if OMPT_SUPPORT
    if( lock->nb_nested > 1 )
        _mpc_omp_ompt_callback_nest_lock( ompt_scope_begin,
                                      (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
}

void omp_unset_nest_lock( omp_nest_lock_t *lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
#endif /* OMPT_SUPPORT */

	lock->nb_nested -= 1;

	if ( lock->nb_nested == 0 )
	{
		lock->owner_thread = NULL;
		lock->owner_task = NULL;
		mpc_common_spinlock_unlock( &( lock->lock ) );

#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_mutex_released( ompt_mutex_nest_lock, (ompt_wait_id_t) lock->ompt_wait_id );
	}
    else
    {
        _mpc_omp_ompt_callback_nest_lock( ompt_scope_end, (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
    }
}

int omp_test_nest_lock( omp_nest_lock_t *lock )
{
	assert( lock );
	mpc_omp_init();
	mpc_omp_thread_t *thread = mpc_omp_get_thread_tls();

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_callback_mutex_acquire( ompt_mutex_nest_lock, lock->hint, ompt_mutex_impl_mutex, (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */

	if ( omp_nest_lock_test_task( thread, lock ) )
	{
		if ( mpc_common_spinlock_trylock( &( lock->lock ) ) )
		{
			return 0;
		}

		lock->owner_thread = ( void * )thread;
		lock->owner_task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread );
#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_nest_lock,
                                           (ompt_wait_id_t) lock->ompt_wait_id );
#endif /* OMPT_SUPPORT */
	}

	lock->nb_nested += 1;

#if OMPT_SUPPORT
    if (lock->nb_nested > 1)
    {
        _mpc_omp_ompt_callback_nest_lock( ompt_scope_begin, (ompt_wait_id_t) lock->ompt_wait_id );
    }

#if MPCOMPT_HAS_FRAME_SUPPORT
    /* If outter call, unset it here */
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif
#endif /* OMPT_SUPPORT */

	return lock->nb_nested;
}

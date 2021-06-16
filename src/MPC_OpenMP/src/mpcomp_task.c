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
#define _GNU_SOURCE
#include <stdlib.h>
#include "mpcomp_task.h"
#include <mpc_common_debug.h>
#include <mpc_common_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_rank.h>
#include "mpcomp_core.h"
#include "mpcomp_tree.h"
#include "mpcompt_sync.h"
#include "mpcompt_task.h"
#include "mpcompt_frame.h"

#if MPCOMP_TASK

/******************
 * LISTS LOCKFREE *
 ******************/

static inline void __task_lockfree_list_consumer_lock( mpcomp_task_list_t *list, void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	sctk_mcslock_push_ticket( &( list->mpcomp_task_lock ), ( sctk_mcslock_ticket_t * ) opaque, SCTK_MCSLOCK_WAIT );
#else  /* MPCOMP_USE_MCS_LOCK */
	mpc_common_spinlock_lock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void __task_lockfree_list_consumer_unlock( mpcomp_task_list_t *list, void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	sctk_mcslock_trash_ticket( &( list->mpcomp_task_lock ), ( sctk_mcslock_ticket_t * ) opaque );
#else  /* MPCOMP_USE_MCS_LOCK */
	mpc_common_spinlock_unlock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int __task_lockfree_list_consumer_trylock( __UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	not_implemented();
	return 0;
#else  /* MPCOMP_USE_MCS_LOCK */
	return mpc_common_spinlock_trylock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void __task_lockfree_list_producer_lock( __UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void *opaque )
{
}

static inline void __task_lockfree_list_producer_unlock( __UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void *opaque )
{
}

static inline int __task_lockfree_list_producer_trylock( __UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void *opaque )
{
	return 0;
}

static inline int __task_lockfree_list_is_empty( mpcomp_task_list_t *list )
{
	int ret = 0;

	if ( !OPA_load_ptr( &( list->lockfree_shadow_head ) ) )
	{
		if ( !OPA_load_ptr( &( list->lockfree_head ) ) )
		{
			ret = 1;
		}
		else
		{
			OPA_store_ptr( &( list->lockfree_shadow_head ), OPA_load_ptr( &( list->lockfree_head ) ) );
			OPA_store_ptr( &( list->lockfree_head ), NULL );
		}
	}

	return ret;
}

static inline mpcomp_task_t *__task_lockfree_list_peek_head( mpcomp_task_list_t *list )
{
	assert( list );

	if ( __task_lockfree_list_is_empty( list ) )
	{
		return NULL;
	}

	return OPA_load_ptr( &( list->lockfree_shadow_head ) );
}

static inline void __task_lockfree_list_push_to_head( mpcomp_task_list_t *list, mpcomp_task_t *task )
{
	assert( task );
	task->list = list;
	OPA_store_ptr( &( task->lockfree_prev ), NULL );
	OPA_write_barrier();
	void *head = OPA_load_ptr( &( list->lockfree_shadow_head ) );

	if ( head )
	{
		mpcomp_task_t *next_task = ( mpcomp_task_t * ) head;
		OPA_store_ptr( &( list->lockfree_shadow_head ), task );
		OPA_store_ptr( &( task->lockfree_next ), next_task );
		OPA_store_ptr( &( next_task->lockfree_prev ), ( void * ) task );
	}
	else
	{
		head = OPA_load_ptr( &( list->lockfree_head ) );

		if ( head )
		{
			mpcomp_task_t *next_task = ( mpcomp_task_t * ) head;
			OPA_store_ptr( &( list->lockfree_head ), task );
			OPA_store_ptr( &( task->lockfree_next ), next_task );
			OPA_store_ptr( &( next_task->lockfree_prev ), ( void * ) task );
		}
		else
		{
			OPA_store_ptr( &( list->lockfree_head ), ( void * ) task );
			OPA_store_ptr( &( list->lockfree_tail ), ( void * ) task );
		}
	}
}

static inline void __task_lockfree_list_push_to_tail( mpcomp_task_list_t *list, mpcomp_task_t *task )
{
	assert( task );
	task->list = list;
	OPA_store_ptr( ( OPA_ptr_t * ) & ( task->next ), NULL );
	/* From OpenPA lockfree implementation */
	OPA_write_barrier();
	void *prev = OPA_swap_ptr( &( list->lockfree_tail ), task );

	if ( !prev )
	{
		OPA_store_ptr( ( OPA_ptr_t * ) & ( list->lockfree_head ), ( void * ) task );
		OPA_store_ptr( ( OPA_ptr_t * ) & ( task->lockfree_prev ), NULL );
	}
	else
	{
		mpcomp_task_t *prev_task = ( mpcomp_task_t * ) prev;
		OPA_store_ptr( ( OPA_ptr_t * ) & ( prev_task->lockfree_next ), ( void * ) task );
		OPA_store_ptr( ( OPA_ptr_t * ) & ( task->lockfree_prev ), ( void * ) prev_task );
	}
}

static inline mpcomp_task_t *__task_lockfree_list_pop_from_head( mpcomp_task_list_t *list )
{
	mpcomp_task_t *task;

	/* No task to dequeue */

	if ( __task_lockfree_list_is_empty( list ) )
	{
		return NULL;
	}

	/* Get first task in list */
	void *shadow = OPA_load_ptr( &( list->lockfree_shadow_head ) );
	task = ( mpcomp_task_t * ) shadow;

	if ( OPA_load_ptr( ( OPA_ptr_t * ) & ( task->lockfree_next ) ) )
	{
		OPA_store_ptr( ( OPA_ptr_t * ) & ( list->lockfree_shadow_head ), OPA_load_ptr( &( task->lockfree_next ) ) );
		mpcomp_task_t *next_task = ( mpcomp_task_t * ) OPA_load_ptr( &( task->lockfree_next ) );
		OPA_store_ptr( ( OPA_ptr_t * ) & ( next_task->lockfree_prev ), NULL );
	}
	else
	{
		OPA_store_ptr( ( OPA_ptr_t * ) & ( list->lockfree_shadow_head ), NULL );

		if ( OPA_cas_ptr( ( OPA_ptr_t * ) & ( list->lockfree_tail ), shadow, NULL ) != shadow )
		{
			while ( !OPA_load_ptr( ( OPA_ptr_t * ) & ( task->lockfree_next ) ) )
			{
				sctk_cpu_relax();
			}

			OPA_store_ptr( ( OPA_ptr_t * ) & ( list->lockfree_shadow_head ), OPA_load_ptr( &( task->lockfree_next ) ) );
		}
	}

	return task;
}

static inline mpcomp_task_t *__task_lockfree_list_pop_from_tail( mpcomp_task_list_t *list )
{
	mpcomp_task_t *task, *shadow;

	if ( __task_lockfree_list_is_empty( list ) )
	{
		return NULL;
	}

	void *tail = OPA_load_ptr( &( list->lockfree_tail ) );
	shadow = ( mpcomp_task_t * ) OPA_load_ptr( &( list->lockfree_shadow_head ) );
	task = ( mpcomp_task_t * ) tail;

	if ( OPA_load_ptr( &( shadow->lockfree_next ) ) )
	{
		OPA_store_ptr( &( list->lockfree_tail ), OPA_load_ptr( &( task->lockfree_prev ) ) );
		mpcomp_task_t *prev_task = ( mpcomp_task_t * ) OPA_load_ptr( &( task->lockfree_prev ) );
		OPA_store_ptr( &( prev_task->lockfree_next ), NULL );
	}
	else
	{
		OPA_store_ptr( ( OPA_ptr_t * ) & ( list->lockfree_shadow_head ), NULL );

		if ( OPA_cas_ptr( ( OPA_ptr_t * ) & ( list->lockfree_tail ), shadow, NULL ) != shadow )
		{
			while ( !OPA_load_ptr( ( OPA_ptr_t * ) & ( task->lockfree_next ) ) )
			{
				sctk_cpu_relax();
			}

			OPA_store_ptr( ( OPA_ptr_t * ) & ( list->lockfree_shadow_head ), OPA_load_ptr( &( task->lockfree_next ) ) );
		}
	}

	return task;
}

/***************
 * LIST LOCKED *
 ***************/


static inline int __task_locked_list_is_empty( mpcomp_task_list_t *list )
{
	return ( list->head == NULL );
}

static inline void __task_locked_list_push_to_head( mpcomp_task_list_t *list,
        mpcomp_task_t *task )
{
	if ( __task_locked_list_is_empty( list ) )
	{
		task->prev = NULL;
		task->next = NULL;
		list->head = task;
		list->tail = task;
	}
	else
	{
		task->prev = NULL;
		task->next = list->head;
		list->head->prev = task;
		list->head = task;
	}

	list->total += 1;
	task->list = list;
}

static inline void __task_locked_list_push_to_tail( mpcomp_task_list_t *list,
        mpcomp_task_t *task )
{
	if ( __task_locked_list_is_empty( list ) )
	{
		task->prev = NULL;
		task->next = NULL;
		list->head = task;
		list->tail = task;
	}
	else
	{
		task->prev = list->tail;
		task->next = NULL;
		list->tail->next = task;
		list->tail = task;
	}

	list->total += 1;
	task->list = list;
}

static inline int __task_is_child( mpcomp_task_t *task, mpcomp_task_t *current_task );

static inline mpcomp_task_t * __task_locked_list_pop_from_head( mpcomp_task_list_t *list )
{
	mpcomp_task_t *task = NULL;

	/* No task in list */
	if ( __task_locked_list_is_empty( list ) )
	{
		return NULL;
	}

	task = list->head;
	mpcomp_thread_t *thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	mpcomp_task_t *current_task = thread->task_infos.current_task;

	/* Only child tasks can be scheduled to respect OpenMP norm */
	if ( current_task->func && !__task_is_child( task, current_task ) && thread->task_infos.one_list_per_thread ) /* If not implicit task and one list per thread*/
	{
		return NULL;
	}

	/* Get First task in list */
	list->head = task->next;

	if ( list->head )
	{
		list->head->prev = NULL;
	}

	if ( !( task->next ) )
	{
		list->tail = NULL;
	}

	OPA_decr_int( &list->nb_elements );
	task->list = NULL;
	return task;
}

static inline mpcomp_task_t * __task_locked_list_pop_from_tail( mpcomp_task_list_t *list )
{
	if ( !__task_locked_list_is_empty( list ) )
	{
		mpcomp_task_t *task = list->tail;
		mpcomp_thread_t *thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
		mpcomp_task_t *current_task = thread->task_infos.current_task;

		/* Only child tasks can be scheduled to respect OpenMP norm */
		if ( current_task->func && !__task_is_child( task, current_task ) && thread->task_infos.one_list_per_thread ) /* If not implicit task and one list per thread*/
		{
			return NULL;
		}

		list->tail = task->prev;

		if ( list->tail )
		{
			list->tail->next = NULL;
		}

		if ( !( task->prev ) )
		{
			list->head = NULL;
		}

		OPA_decr_int( &( list->nb_elements ) );
		return task;
	}

	return NULL;
}

static inline int __task_locked_list_remove( mpcomp_task_list_t *list,
        mpcomp_task_t *task )
{
	if ( task->list == NULL || __task_locked_list_is_empty( list ) )
	{
		return -1;
	}

	if ( task == list->head )
	{
		list->head = task->next;
	}

	if ( task == list->tail )
	{
		list->tail = task->prev;
	}

	if ( task->next )
	{
		task->next->prev = task->prev;
	}

	if ( task->prev )
	{
		task->prev->next = task->next;
	}

	OPA_decr_int( &( list->nb_elements ) );
	task->list = NULL;
	return 1;
}

static inline mpcomp_task_t * __task_locked_list_search( mpcomp_task_list_t *list, int depth )
{
	mpcomp_task_t *cur_task = list->head;

	while ( cur_task )
	{
		if ( cur_task->depth > depth )
		{
			break;
		}

		cur_task = cur_task->next;
	}

	if ( cur_task != NULL )
	{
		__task_locked_list_remove( list, cur_task );
	}

	return cur_task;
}

static inline void __task_locked_list_consumer_lock( mpcomp_task_list_t *list, void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	sctk_mcslock_push_ticket( &( list->mpcomp_task_lock ), ( sctk_mcslock_ticket_t * ) opaque, SCTK_MCSLOCK_WAIT );
#else  /* MPCOMP_USE_MCS_LOCK */
	mpc_common_spinlock_lock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void __task_locked_list_consumer_unlock( mpcomp_task_list_t *list, void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	sctk_mcslock_trash_ticket( &( list->mpcomp_task_lock ), ( sctk_mcslock_ticket_t * ) opaque );
#else  /* MPCOMP_USE_MCS_LOCK */
	mpc_common_spinlock_unlock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int __task_locked_list_consumer_trylock( __UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	not_implemented();
	return 0;
#else  /* MPCOMP_USE_MCS_LOCK */
	return mpc_common_spinlock_trylock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void __task_locked_list_producer_lock( mpcomp_task_list_t *list, void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	sctk_mcslock_push_ticket( &( list->mpcomp_task_lock ), ( sctk_mcslock_ticket_t * ) opaque, SCTK_MCSLOCK_WAIT );
#else  /* MPCOMP_USE_MCS_LOCK */
	mpc_common_spinlock_lock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void __task_locked_list_producer_unlock( mpcomp_task_list_t *list, void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	sctk_mcslock_trash_ticket( &( list->mpcomp_task_lock ), ( sctk_mcslock_ticket_t * ) opaque );
#else  /* MPCOMP_USE_MCS_LOCK */
	mpc_common_spinlock_unlock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int __task_locked_list_producer_trylock( __UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void *opaque )
{
#ifdef MPCOMP_USE_MCS_LOCK
	not_implemented();
	return 0;
#else  /* MPCOMP_USE_MCS_LOCK */
	return mpc_common_spinlock_trylock( &( list->mpcomp_task_lock ) );
#endif /* MPCOMP_USE_MCS_LOCK */
}

/*******************************
 * TASK LIST POINTER INTERFACE *
 *******************************/

static int ( *__task_list_is_empty )( mpcomp_task_list_t * );
static void ( *__task_list_push_to_head )( mpcomp_task_list_t *, mpcomp_task_t * );
static void ( *__task_list_push_to_tail )( mpcomp_task_list_t *, mpcomp_task_t * );
static mpcomp_task_t *( *__task_list_pop_from_head )( mpcomp_task_list_t * );
static mpcomp_task_t *( *__task_list_pop_from_tail )( mpcomp_task_list_t * );
static void ( *__task_list_consumer_lock )( mpcomp_task_list_t *, void * );
static void ( *__task_list_consumer_unlock )( mpcomp_task_list_t *, void * );
static int ( *__task_list_consumer_trylock )( mpcomp_task_list_t *, void * );
static void ( *__task_list_producer_lock )( mpcomp_task_list_t *, void * );
static void ( *__task_list_producer_unlock )( mpcomp_task_list_t *, void * );
static int ( *__task_list_producer_trylock )( mpcomp_task_list_t *, void * );

void _mpc_task_list_interface_init()
{
	if ( mpc_omp_conf_get()->omp_task_use_lockfree_queue )
	{
		__task_list_is_empty = __task_lockfree_list_is_empty;
		__task_list_push_to_head = __task_lockfree_list_push_to_head;
		__task_list_push_to_tail = __task_lockfree_list_push_to_tail;
		__task_list_pop_from_head = __task_lockfree_list_pop_from_head;
		__task_list_pop_from_tail = __task_lockfree_list_pop_from_tail;
		__task_list_consumer_lock = __task_lockfree_list_consumer_lock;
		__task_list_consumer_unlock = __task_lockfree_list_consumer_unlock;
		__task_list_consumer_trylock = __task_lockfree_list_consumer_trylock;
		__task_list_producer_lock = __task_lockfree_list_producer_lock;
		__task_list_producer_unlock = __task_lockfree_list_producer_unlock;
		__task_list_producer_trylock = __task_lockfree_list_producer_trylock;
	}
	else
	{
		__task_list_is_empty = __task_locked_list_is_empty;
		__task_list_push_to_head = __task_locked_list_push_to_head;
		__task_list_push_to_tail = __task_locked_list_push_to_tail;
		__task_list_pop_from_head = __task_locked_list_pop_from_head;
		__task_list_pop_from_tail = __task_locked_list_pop_from_tail;
		__task_list_consumer_lock = __task_locked_list_consumer_lock;
		__task_list_consumer_unlock = __task_locked_list_consumer_unlock;
		__task_list_consumer_trylock = __task_locked_list_consumer_trylock;
		__task_list_producer_lock = __task_locked_list_producer_lock;
		__task_list_producer_unlock = __task_locked_list_producer_unlock;
		__task_list_producer_trylock = __task_locked_list_producer_trylock;
	}
}

static inline void __task_list_push( mpcomp_task_list_t *mvp_task_list, mpcomp_task_t *new_task )
{
        TODO("WHY DO WE DIFFERENCIATE AGAIN ? (see previous func)");
	if ( mpc_omp_conf_get()->omp_task_use_lockfree_queue )
	{
		__task_list_push_to_tail( mvp_task_list, new_task );
	}
	else
	{
		__task_list_push_to_head( mvp_task_list, new_task );
	}
}


static inline mpcomp_task_t *__task_list_pop( mpcomp_task_list_t *mvp_task_list, bool is_stealing, bool one_list_per_thread )
{
         TODO("WHY DO WE DIFFERENCIATE AGAIN ? (see previous func)");
	if ( mpc_omp_conf_get()->omp_task_use_lockfree_queue )
	{
		if ( is_stealing )
		{
			return __task_list_pop_from_head( mvp_task_list );
		}
		else
		{
			/* For lockfree queue, pop from tail (same side that when pushing) when thread is not stealing and there is one list per thread to avoid big call stacks */
			if ( one_list_per_thread )
			{
				return __task_list_pop_from_tail( mvp_task_list );
			}
			else
			{
				return __task_list_pop_from_head( mvp_task_list );
			}
		}
	}
	else
	{
		return __task_list_pop_from_head( mvp_task_list );
	}
}

/*********
 * UTILS *
 *********/

int mpcomp_task_parse_larceny_mode(char * mode)
{
	if(!strcmp(mode, "hierarchical"))
	{
		return MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL;
	}else if(!strcmp(mode, "random"))
	{
		return MPCOMP_TASK_LARCENY_MODE_RANDOM;
	}else if(!strcmp(mode, "random_order"))
	{
		return MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER;
	}else if(!strcmp(mode, "round_robin"))
	{
		return MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN;
	}else if(!strcmp(mode, "producer"))
	{
		return MPCOMP_TASK_LARCENY_MODE_PRODUCER;
	}else if(!strcmp(mode, "producer_order"))
	{
		return MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER;
	}else if(!strcmp(mode, "hierarchical_random"))
	{
		return MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM;
	}else
	{
		bad_parameter("Could not parse mpc.omp.task.larceny = '%s' it must be one of: hierarchical, random, random_order, round_robin, producer, producer_order, hierarchical_random", mode);
	}
	
	return -1;
}


static inline int __task_is_child( mpcomp_task_t *task, mpcomp_task_t *current_task )
{
	mpcomp_task_t *parent_task = task->parent;

	while ( parent_task->depth > current_task->depth + MPCOMP_TASKS_DEPTH_JUMP )
	{
		parent_task = parent_task->far_ancestor;
	}

	while ( parent_task->depth > current_task->depth )
	{
		parent_task = parent_task->parent;
	}

	if ( parent_task != current_task )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

static inline int __task_get_victim( int globalRank, int index,
                                          mpcomp_tasklist_type_t type );

/* FOR DEBUG */
static inline int __task_extract_victim_list( int **victims_list, int *nbList, const int type )
{
	int i, nbVictims;
	int *__victims_list;
	assert( sctk_openmp_thread_tls );
	mpcomp_thread_t *thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	mpcomp_mvp_t *mvp = thread->mvp;
	assert( mvp );
	assert( thread->instance );
	mpcomp_team_t *team = thread->instance->team;
	assert( team );
	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team );
	const int isMonoVictim = ( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
	                           larcenyMode == MPCOMP_TASK_LARCENY_MODE_PRODUCER );
	const int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( team, type );
	const int nbTasklists = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	*nbList = nbTasklists;

	if ( nbTasklists > 1 )
	{
		int rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, type );
		nbVictims = ( isMonoVictim ) ? 1 : nbTasklists - 1;
		__victims_list = ( int * ) malloc( sizeof( int ) * nbVictims );

		for ( i = 1; i < nbVictims + 1; i++ )
		{
			__victims_list[i - 1] = __task_get_victim( rank, i, ( mpcomp_tasklist_type_t ) type );
		}
	}
	else
	{
		nbVictims = 0;
		__victims_list = NULL;
	}

	*victims_list = __victims_list;
	return nbVictims;
}

/* Is Serialized Task Context if_clause not handle */
static inline int __task_no_deps_is_serialized( mpcomp_thread_t *thread )
{
	mpcomp_task_t *current_task = NULL;
	assert( thread );
	current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
	assert( current_task );

	if ( ( current_task &&
	       mpcomp_task_property_isset( current_task->property, MPCOMP_TASK_FINAL ) ) ||
	     ( thread->info.num_threads == 1 ) ||
	     ( current_task &&
	       current_task->depth > mpc_omp_conf_get()->omp_task_nesting_max ) )
	{
		return 1;
	}

	return 0;
}

static inline int __task_node_list_init( struct mpcomp_node_s *parent, struct mpcomp_node_s *child, const mpcomp_tasklist_type_t type )
{
	int tasklistNodeRank, allocation;
	mpcomp_task_list_t *list;
	mpcomp_task_node_infos_t *infos;
	const int task_vdepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( parent->instance->team, type );
	const int child_vdepth = child->depth - parent->instance->root->depth;

	if ( child_vdepth < task_vdepth )
	{
		return 0;
	}

	infos = &( child->task_infos );

	if ( parent->depth >= task_vdepth )
	{
		allocation = 0;
		list = parent->task_infos.tasklist[type];
		assert( list );
		tasklistNodeRank = parent->task_infos.tasklistNodeRank[type];
	}
	else
	{
		assert( child->depth == task_vdepth );
		allocation = 1;
		list = ( struct mpcomp_task_list_s * ) mpcomp_alloc( sizeof( struct mpcomp_task_list_s ) );
		assert( list );
		__task_list_reset( list );
		tasklistNodeRank = child->tree_array_rank;
	}

	infos->tasklist[type] = list;
	infos->lastStolen_tasklist[type] = NULL;
	infos->tasklistNodeRank[type] = tasklistNodeRank;
	return allocation;
}


void _mpc_task_tree_array_thread_init( struct mpcomp_thread_s *thread )
{
    assert( thread );
    mpcomp_task_t *implicit_task;
    mpcomp_task_list_t *tied_tasks_list;

    if ( !MPCOMP_TASK_THREAD_IS_INITIALIZED( thread )) {
        _mpc_task_list_interface_init();

        implicit_task = ( mpcomp_task_t * ) mpcomp_alloc( sizeof( mpcomp_task_t ) );
        assert( implicit_task );
        memset( implicit_task, 0, sizeof( mpcomp_task_t ) );

        _mpc_task_info_init( implicit_task, NULL, NULL, NULL, 0, thread );
        OPA_store_int( &( implicit_task->refcount ), 1 );
        thread->task_infos.current_task = implicit_task;
        implicit_task->thread = thread;

        if ( thread->next )
            implicit_task->parent = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread->next);

#if OMPT_SUPPORT
        if ( thread->instance
             && thread->instance->team
             && thread->instance->team->depth == 0 ) {
            __mpcompt_callback_task_create( implicit_task, ompt_task_initial, 0 );
        }
        else {
            __mpcompt_callback_task_create( implicit_task, ompt_task_implicit, 0 );
        }
#endif /* OMPT_SUPPORT */

#ifdef MPCOMP_USE_TASKDEP
        implicit_task->task_dep_infos =
          ( mpcomp_task_dep_task_infos_t * ) mpcomp_alloc( sizeof( mpcomp_task_dep_task_infos_t ) );
        assert( implicit_task->task_dep_infos );
        memset( implicit_task->task_dep_infos, 0, sizeof( mpcomp_task_dep_task_infos_t ) );
#endif /* MPCOMP_USE_TASKDEP */

        tied_tasks_list = ( mpcomp_task_list_t * ) mpcomp_alloc( sizeof( mpcomp_task_list_t ) );
        assert( tied_tasks_list );
        memset( tied_tasks_list, 0, sizeof( mpcomp_task_list_t ) );

        thread->task_infos.tied_tasks = tied_tasks_list;

#ifdef MPCOMP_USE_MCS_LOCK
        sctk_mcslock_ticket_t *new_ticket = sctk_mcslock_alloc_ticket();
        thread->task_infos.opaque = ( void * ) new_ticket;
#else
        thread->task_infos.opaque = ( void * ) NULL;
#endif /* MPCOMP_USE_MCS_LOCK  */

        MPCOMP_TASK_THREAD_CMPL_INIT( thread );
    }
}

static inline int __task_mpv_list_init( struct mpcomp_node_s *parent, struct mpcomp_mvp_s *child, const mpcomp_tasklist_type_t type )
{
	int tasklistNodeRank, allocation;
	mpcomp_task_list_t *list;
	mpcomp_task_mvp_infos_t *infos;
	infos = &( child->task_infos );

	if ( parent->task_infos.tasklist[type] )
	{
		allocation = 0;
		list = parent->task_infos.tasklist[type];
		tasklistNodeRank = parent->task_infos.tasklistNodeRank[type];
	}
	else
	{
		allocation = 1;
		list = ( struct mpcomp_task_list_s * ) mpcomp_alloc( sizeof( struct mpcomp_task_list_s ) );
		assert( list );
		__task_list_reset( list );
		assert( child->threads );
		tasklistNodeRank = child->threads->tree_array_rank;
	}

	infos->tasklist[type] = list;
	infos->lastStolen_tasklist[type] = NULL;
	infos->tasklistNodeRank[type] = tasklistNodeRank;
	infos->last_thief = -1;
	return allocation;
}

static inline int __task_root_list_init( struct mpcomp_node_s *root, const mpcomp_tasklist_type_t type )
{
	mpcomp_task_list_t *list;
	mpcomp_task_node_infos_t *infos;
	const int task_vdepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( root->instance->team, type );

	if ( task_vdepth != 0 )
	{
		return 0;
	}

	infos = &( root->task_infos );
	list = ( struct mpcomp_task_list_s * ) mpcomp_alloc( sizeof( struct mpcomp_task_list_s ) );
	assert( list );
	__task_list_reset( list );
	infos->tasklist[type] = list;
	infos->lastStolen_tasklist[type] = NULL;
	infos->tasklistNodeRank[type] = 0;
	return 1;
}


void __task_random_victim_init( mpcomp_generic_node_t *gen_node )
{
	int already_init;
	struct drand48_data *randBuffer;
	assert( gen_node != NULL );
	already_init = ( gen_node->type == MPCOMP_CHILDREN_LEAF && MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER( gen_node->ptr.mvp ) );
	already_init |= ( gen_node->type == MPCOMP_CHILDREN_NODE && MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER( gen_node->ptr.node ) );

	if ( already_init && gen_node->type == MPCOMP_CHILDREN_NULL )
	{
		return;
	}

	randBuffer = ( struct drand48_data * ) mpcomp_alloc( sizeof( struct drand48_data ) );
	assert( randBuffer );

	if ( gen_node->type == MPCOMP_CHILDREN_LEAF )
	{
		assert( gen_node->ptr.mvp->threads );
		const int __globalRank = gen_node->ptr.mvp->threads->tree_array_rank;
		srand48_r( mpc_arch_get_timestamp_gettimeofday() * __globalRank, randBuffer );
		MPCOMP_TASK_MVP_SET_TASK_LIST_RANDBUFFER( gen_node->ptr.mvp, randBuffer );
	}
	else
	{
		assert( gen_node->type == MPCOMP_CHILDREN_NODE );
		const int __globalRank = gen_node->ptr.node->tree_array_rank;
		srand48_r( mpc_arch_get_timestamp_gettimeofday() * __globalRank, randBuffer );
		MPCOMP_TASK_NODE_SET_TASK_LIST_RANDBUFFER( gen_node->ptr.node, randBuffer );
	}
}

void _mpc_task_node_info_init( struct mpcomp_node_s *parent, struct mpcomp_node_s *child )
{
	int ret, type;
	mpcomp_instance_t *instance;
	assert( child );
	assert( parent );
	assert( parent->instance );
	instance = parent->instance;
	assert( instance->team );
	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team );

	for ( type = 0, ret = 0; type < MPCOMP_TASK_TYPE_COUNT; type++ )
	{
		ret += __task_node_list_init( parent, child, ( mpcomp_tasklist_type_t ) type );
	}

	if ( ret )
	{
		if ( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
		     larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER )
		{
			const int global_rank = child->tree_array_rank;
			__task_random_victim_init( &( instance->tree_array[global_rank] ) );
		}
	}
}

void _mpc_task_mvp_info_init( struct mpcomp_node_s *parent, struct mpcomp_mvp_s *child )
{
	int ret, type;
	mpcomp_instance_t *instance;
	assert( child );
	assert( parent );
	assert( parent->instance );
	instance = parent->instance;
	assert( instance->team );
	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team );

	for ( type = 0, ret = 0; type < MPCOMP_TASK_TYPE_COUNT; type++ )
	{
		ret += __task_mpv_list_init( parent, child, ( mpcomp_tasklist_type_t ) type );
	}

	if ( ret )
	{
		if ( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
		     larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER )
		{
			assert( child->threads );
			const int global_rank = child->threads->tree_array_rank;
			__task_random_victim_init( &( instance->tree_array[global_rank] ) );
		}
	}

	const int new_tasks_depth_value =
	    mpc_omp_conf_get()->omp_new_task_depth;
	int depth = child->threads->instance->tree_depth;

	/* Ensure tasks lists depths are correct */
	if ( new_tasks_depth_value >= depth - 1 )
	{
		child->threads->task_infos.one_list_per_thread = true;
	}
	else
	{
		child->threads->task_infos.one_list_per_thread = false;
	}

	child->threads->task_infos.nb_reusable_tasks = 0;
	child->threads->task_infos.max_task_tot_size = 0;

	if ( !child->threads->task_infos.reusable_tasks )
	{
		child->threads->task_infos.reusable_tasks = ( mpcomp_task_t ** ) mpcomp_alloc( MPCOMP_NB_REUSABLE_TASKS * sizeof( mpcomp_task_t * ) );
	}
}

void _mpc_task_root_info_init( struct mpcomp_node_s *root )
{
	int ret, type;
	mpcomp_instance_t *instance;
	assert( root );
	assert( root->instance );
	instance = root->instance;
	assert( instance->team );
	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team );

	for ( type = 0, ret = 0; type < MPCOMP_TASK_TYPE_COUNT; type++ )
	{
		ret += __task_root_list_init( root, ( mpcomp_tasklist_type_t ) type );
	}

	if ( ret )
	{
		if ( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
		     larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER )
		{
			__task_random_victim_init( &( instance->tree_array[0] ) );
		}
	}

	mpcomp_thread_t *thread = root->mvp->threads;
	const int new_tasks_depth_value =
	    mpc_omp_conf_get()->omp_new_task_depth;
	int depth = root->instance->tree_depth;

	/* Ensure tasks lists depths are correct */
	if ( new_tasks_depth_value >= depth - 1 )
	{
		thread->task_infos.one_list_per_thread = true;
	}
	else
	{
		thread->task_infos.one_list_per_thread = false;
	}

	thread->task_infos.nb_reusable_tasks = 0;
	thread->task_infos.max_task_tot_size = 0;

	if ( !thread->task_infos.reusable_tasks )
	{
		thread->task_infos.reusable_tasks = ( mpcomp_task_t ** ) mpcomp_alloc( MPCOMP_NB_REUSABLE_TASKS * sizeof( mpcomp_task_t * ) );
	}

	instance->task_infos.is_initialized = false;
}

void _mpc_task_team_info_init( struct mpcomp_team_s *team,
                                  int depth )
{
	memset( &( team->task_infos ), 0, sizeof( mpcomp_task_team_infos_t ) );
	const int new_tasks_depth_value = mpc_omp_conf_get()->omp_new_task_depth;
	const int untied_tasks_depth_value = mpc_omp_conf_get()->omp_untied_task_depth;
	const int omp_task_nesting_max = mpc_omp_conf_get()->omp_task_nesting_max;
	const int omp_task_larceny_mode = mpc_omp_conf_get()->omp_task_larceny_mode;
	/* Ensure tasks lists depths are correct */
	const int new_tasks_depth =
	    ( new_tasks_depth_value < depth ) ? new_tasks_depth_value : depth - 1;
	const int untied_tasks_depth =
	    ( untied_tasks_depth_value < depth ) ? untied_tasks_depth_value : depth - 1;
	/* Set variable in team task infos */
	MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH( team, MPCOMP_TASK_TYPE_NEW,
	                                     new_tasks_depth );
	MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH( team, MPCOMP_TASK_TYPE_UNTIED,
	                                     untied_tasks_depth );
	MPCOMP_TASK_TEAM_SET_TASK_NESTING_MAX( team, omp_task_nesting_max );
	MPCOMP_TASK_TEAM_SET_TASK_LARCENY_MODE( team, omp_task_larceny_mode );
}

/******************
 * TASK INTERFACE *
 ******************/

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
	static OPA_int_t __private_perf_call_steal = OPA_INT_T_INITIALIZER( 0 );
	static OPA_int_t __private_perf_success_steal = OPA_INT_T_INITIALIZER( 0 );
	static OPA_int_t __private_perf_create_task = OPA_INT_T_INITIALIZER( 0 );
	static OPA_int_t __private_perf_executed_task = OPA_INT_T_INITIALIZER( 0 );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */


/* Return list of 'type' tasks contained in element of rank 'globalRank' */
static inline struct mpcomp_task_list_s * __task_get_list( int globalRank, mpcomp_tasklist_type_t type )
{
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	mpcomp_generic_node_t *gen_node;
	assert( type >= 0 && type < MPCOMP_TASK_TYPE_COUNT );
	/* Retrieve the current thread information */
	assert( sctk_openmp_thread_tls != NULL );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->mvp );
	/* Retrieve the current thread instance */
	assert( thread->instance );
	instance = thread->instance;
	/* Retrieve target node or mvp generic node */
	assert( globalRank >= 0 && globalRank < instance->tree_array_size );
	gen_node = &( instance->tree_array[globalRank] );

	/* Retrieve target node or mvp tasklist */
	if ( gen_node->type == MPCOMP_CHILDREN_LEAF )
	{
		return MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD( gen_node->ptr.mvp, type );
	}

	if ( gen_node->type == MPCOMP_CHILDREN_NODE )
	{
		return MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD( gen_node->ptr.node, type );
	}

	/* Extra node to preserve regular tree_shape */
	//assert( gen_node->type == MPCOMP_CHILDREN_NULL );
	return NULL;
}

mpcomp_task_t * _mpc_task_alloc( void ( *fn )( void * ), void *data,
                     void ( *cpyfn )( void *, void * ),
                     long arg_size, long arg_align,
                     __UNUSED__ bool if_clause, unsigned flags,
                     int has_deps )
{
	assert( sctk_openmp_thread_tls );
	mpcomp_thread_t *t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	/* default pading */
	const long align_size = ( arg_align == 0 ) ? 8 : arg_align;
	// mpcomp_task + arg_size
	const long mpcomp_task_info_size =
	    _mpc_task_align_single_malloc( sizeof( mpcomp_task_t ),
	                                     align_size );
	const long mpcomp_task_data_size =
	    _mpc_task_align_single_malloc( arg_size,
	                                     align_size );
	/* Compute task total size */
	long mpcomp_task_tot_size = mpcomp_task_info_size;
	assert( MPCOMP_OVERFLOW_SANITY_CHECK( ( unsigned long ) mpcomp_task_tot_size,
	             ( unsigned long ) mpcomp_task_data_size ) );
	mpcomp_task_tot_size += mpcomp_task_data_size;
	mpcomp_task_t *new_task;

	/* Reuse another task if we can to avoid alloc overhead */
	if ( t->task_infos.one_list_per_thread )
	{
		if ( mpcomp_task_tot_size > t->task_infos.max_task_tot_size )
			/* Tasks stored too small, free them */
		{
			t->task_infos.max_task_tot_size = mpcomp_task_tot_size;
			int i;

			for ( i = 0; i < t->task_infos.nb_reusable_tasks; i++ )
			{
				sctk_free( t->task_infos.reusable_tasks[i] );
				t->task_infos.reusable_tasks[i] = NULL;
			}

			t->task_infos.nb_reusable_tasks = 0;
		}

		if ( t->task_infos.nb_reusable_tasks == 0 )
		{
			new_task = mpcomp_alloc( t->task_infos.max_task_tot_size );
		}
		/* Reuse last task stored */
		else
		{
			assert( t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks - 1] );
			new_task = ( mpcomp_task_t * ) t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks - 1];
			t->task_infos.nb_reusable_tasks--;
		}
	}
	else
	{
		new_task = mpcomp_alloc( mpcomp_task_tot_size );
	}

	assert( new_task != NULL );
	void *task_data = ( arg_size > 0 )
	                  ? ( void * ) ( ( uintptr_t ) new_task + mpcomp_task_info_size )
	                  : NULL;
	/* Create new task */
	_mpc_task_info_init( new_task, fn, task_data, task_data, arg_size, t );
#ifdef MPCOMP_USE_TASKDEP

	if ( has_deps )
	{
		new_task->task_dep_infos = ( mpcomp_task_dep_task_infos_t * ) sctk_malloc( sizeof( mpcomp_task_dep_task_infos_t ) );
		assert( new_task->task_dep_infos );
		memset( new_task->task_dep_infos, 0, sizeof( mpcomp_task_dep_task_infos_t ) );
	}

#endif /* MPCOMP_USE_TASKDEP */

	if ( arg_size > 0 )
	{
		if ( cpyfn )
		{
			cpyfn( task_data, data );
		}
		else
		{
			memcpy( task_data, data, arg_size );
		}
	}

	/* If its parent task is final, the new task must be final too */
	if ( _mpc_task_is_final( flags, new_task->parent ) )
	{
		mpcomp_task_set_property( &( new_task->property ), MPCOMP_TASK_FINAL );
	}

    /* if clause */
    if( !if_clause )
        mpcomp_task_set_property(&(new_task->property), MPCOMP_TASK_UNDEFERRED );

	/* taskgroup */
	mpcomp_taskgroup_add_task( new_task );
	_mpc_task_ref_parent_task( new_task );
	new_task->is_stealed = false;
	new_task->task_size = t->task_infos.max_task_tot_size;

	if ( new_task->depth % MPCOMP_TASKS_DEPTH_JUMP == 2 )
	{
		new_task->far_ancestor = new_task->parent;
	}
	else
	{
		new_task->far_ancestor = new_task->parent->far_ancestor;
	}

#if OMPT_SUPPORT
    __mpcompt_callback_task_create( new_task,
                                    ___mpcompt_get_task_flags( t, new_task ),
                                    has_deps );
#endif /* OMPT_SUPPORT */

	return new_task;
}

void _mpc_task_free( mpcomp_thread_t *thread )
{
	int i;

	if ( thread->task_infos.reusable_tasks )
	{
		for ( i = 0; i < thread->task_infos.nb_reusable_tasks; i++ )
		{
			sctk_free( thread->task_infos.reusable_tasks[i] );
		}

		thread->task_infos.nb_reusable_tasks = 0;
		sctk_free( thread->task_infos.reusable_tasks );
		thread->task_infos.reusable_tasks = NULL;
	}
}


/**
 * Execute a specific task by the current thread.
 *
 * \param task Target task to execute by the current thread
 */

static inline void __task_execute( mpcomp_task_t *task )
{
	mpcomp_thread_t *thread;
	mpcomp_local_icv_t saved_icvs;
	mpcomp_task_t *saved_current_task;
	assert( task );

	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	/* A task can't be executed by multiple threads */
	assert( task->thread == NULL );

	/* Update task owner */
	task->thread = thread;

	/* Saved thread current task */
	saved_current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
	OPA_incr_int( &__private_perf_executed_task );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */

	/* Update thread current task */
	MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, task );

	/* Saved thread icv environnement */
	saved_icvs = thread->info.icvs;
	/* Get task icv environnement */
	thread->info.icvs = task->icvs;

#if OMPT_SUPPORT
    __mpcompt_callback_task_schedule( &(saved_current_task->ompt_task_data),
                                      ompt_task_switch,
                                      &(task->ompt_task_data));

#if MPCOMPT_HAS_FRAME_SUPPORT
    /* Record and reset current frame infos */
    mpcompt_frame_info_t prev_frame_infos = __mpcompt_frame_reset_infos();

    __mpcompt_frame_set_exit( MPCOMPT_GET_FRAME_ADDRESS );
#endif
#endif /* OMPT_SUPPORT */

	/* Execute task */
	task->func( task->func_data );

#if OMPT_SUPPORT
#if MPCOMPT_HAS_FRAME_SUPPORT
    /* Restore previous frame infos */
    __mpcompt_frame_set_infos( &prev_frame_infos );
#endif

    __mpcompt_callback_task_schedule( &(task->ompt_task_data),
                                      ompt_task_complete,
                                      &(saved_current_task->ompt_task_data));
#endif /* OMPT_SUPPORT */

	/* Restore thread icv envionnement */
	thread->info.icvs = saved_icvs;

	/* Restore current task */
	MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, saved_current_task );

	/* Reset task owner for implicite task */
	task->thread = NULL;
}

void _mpc_task_ref_parent_task( mpcomp_task_t *task )
{
	assert( task );

	if ( !( task->parent ) )
	{
		return;
	}

	OPA_incr_int( &( task->refcount ) );
	OPA_incr_int( &( task->parent->refcount ) );
}

void _mpc_task_unref_parent_task( mpcomp_task_t *task )
{
	bool no_more_ref;
	mpcomp_task_t *mother;
	assert( task );
	mpcomp_thread_t *thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	mother = task->parent;
	no_more_ref = ( OPA_fetch_and_decr_int( &( task->refcount ) ) == 1 );

	while ( mother && no_more_ref ) // FREE MY TASK AND CLIMB TREE
	{
		/* Try to store task to reuse it after and avoid alloc overhead */
		if ( thread->task_infos.nb_reusable_tasks < MPCOMP_NB_REUSABLE_TASKS && !task->is_stealed && thread->task_infos.one_list_per_thread && task->task_size == thread->task_infos.max_task_tot_size )
		{
			thread->task_infos.reusable_tasks[thread->task_infos.nb_reusable_tasks] = task;
			thread->task_infos.nb_reusable_tasks++;
		}
		else
		{
			sctk_free( task );
		}

		task = mother;
		mother = task->parent;
		no_more_ref = ( OPA_fetch_and_decr_int( &( task->refcount ) ) == 1 );
	}

	if ( !mother && no_more_ref ) // ROOT TASK
	{
		OPA_decr_int( &( task->refcount ) );
	}
}


/**
 * Try to find a list to put the task (if needed).
 *
 * \param thread OpenMP thread that will perform the actions
 * \param if_clause True if an if clause evaluated to 'true'
 * has been provided by the user
 * \return The list where this task can be inserted or NULL ortherwise
 */
static inline mpcomp_task_list_t * __task_try_delay( mpcomp_thread_t *thread, bool if_clause )
{
	mpcomp_task_list_t *mvp_task_list = NULL;

	/* Is Serialized Task */
	if ( !if_clause || __task_no_deps_is_serialized( thread ) )
	{
		return NULL;
	}

	/* Reserved place in queue */
	assert( thread->mvp != NULL );
	const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK(
	                          thread->mvp, MPCOMP_TASK_TYPE_NEW );
	assert( node_rank >= 0 );

	// No list for a mvp should be an error ??
	if ( !( mvp_task_list =
	            __task_get_list( node_rank, MPCOMP_TASK_TYPE_NEW ) ) )
	{
		abort();
		return NULL;
	}

	const int __max_delayed = mpc_omp_conf_get()->mpcomp_task_max_delayed;

	//Too much delayed tasks
	if ( OPA_fetch_and_incr_int( &( mvp_task_list->nb_elements ) ) >=
	     __max_delayed )
	{
		OPA_decr_int( &( mvp_task_list->nb_elements ) );
		return NULL;
	}

	return mvp_task_list;
}


void _mpc_task_process( mpcomp_task_t *new_task, bool if_clause )
{
	mpcomp_thread_t *thread;
	mpcomp_task_list_t *mvp_task_list;
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
	OPA_incr_int( &__private_perf_create_task );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
	mvp_task_list = __task_try_delay( thread, if_clause );

	/* If possible, push the task in the list of new tasks */
	if ( mvp_task_list )
	{
		if ( thread->instance->task_infos.is_initialized == false )
		{
			thread->instance->task_infos.is_initialized = true;
		}

		__task_list_producer_lock( mvp_task_list, thread->task_infos.opaque );
		__task_list_push( mvp_task_list, new_task );
		__task_list_producer_unlock( mvp_task_list, thread->task_infos.opaque );
		return;
	}

	/* Otherwise, execute directly this task */
	mpcomp_task_set_property( &( new_task->property ), MPCOMP_TASK_UNDEFERRED );
	__task_execute( new_task );
	mpcomp_taskgroup_del_task( new_task );
#ifdef MPCOMP_USE_TASKDEP
	_mpc_task_dep_new_finalize( new_task );
#endif /* MPCOMP_USE_TASKDEP */
	_mpc_task_unref_parent_task( new_task );
}


void _mpc_task_new( void ( *fn )( void * ), void *data,
                    void ( *cpyfn )( void *, void * ), long arg_size, long arg_align,
                    bool if_clause, unsigned flags )
{
	mpcomp_task_t *new_task;
	/* Intialize the OpenMP environnement (if needed) */
	__mpcomp_init();
	/* Allocate the new task w/ provided information */
	new_task = _mpc_task_alloc( fn, data, cpyfn, arg_size, arg_align,
	                                if_clause, flags, 0 /* no deps */ );
	/* Process this new task (put inside a list or direct execution) */
	_mpc_task_process( new_task, if_clause );
}

/**
 * Try to steal a task inside a target list.
 *
 * \param list Target list where to look for a task
 * \return the stolen task or NULL if failed
 */
static inline mpcomp_task_t * __task_steal( mpcomp_task_list_t *list, __UNUSED__ int rank, __UNUSED__ int victim )
{
	mpcomp_thread_t *thread;
	struct mpcomp_task_s *task = NULL;
	/* Check input argument */
	assert( list != NULL );
	/* Get current OpenMP thread */
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread );
#ifdef MPCOMP_USE_MCS_LOCK
	__task_list_consumer_lock( list, thread->task_infos.opaque );
#else /* MPCOMP_USE_MCS_LOCK */

	if ( __task_list_consumer_trylock( list, thread->task_infos.opaque ) )
	{
		return NULL;
	}

#endif /* MPCOMP_USE_MCS_LOCK */
	task = __task_list_pop( list, true, thread->task_infos.one_list_per_thread );
	//if(task)
	//{
	//gen_node = &(thread->instance->tree_array[victim] );
	//if( gen_node->type == MPCOMP_CHILDREN_LEAF )
	//{
	//MPCOMP_TASK_MVP_SET_LAST_STOLEN_TASK_LIST(thread->mvp, 0, list);
	//thread->mvp->task_infos.lastStolen_tasklist_rank[0] = victim;
	//gen_node->ptr.mvp->task_infos.last_thief = rank;
	//}
	//}
	__task_list_consumer_unlock( list, thread->task_infos.opaque );
	return task;
}


/** Hierarchical stealing policy **/

static inline int __task_get_victim_hierarchical( int globalRank, __UNUSED__ int index,
        __UNUSED__ mpcomp_tasklist_type_t type )
{
	return mpc_omp_tree_array_get_neighbor( globalRank, index );
}

/** Random stealing policy **/


static inline int __task_get_victim_random( const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type )
{
	int victim, i;
	long int value;
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	struct drand48_data *randBuffer;
	mpcomp_generic_node_t *gen_node;
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls != NULL );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	instance = thread->instance;
	assert( instance->tree_array );
	assert( instance->tree_nb_nodes_per_depth );
	assert( instance->tree_first_node_per_depth );
	int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( instance->team, type );
	int nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];

	while ( nbVictims == 0 )
	{
		tasklistDepth--;
		nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	}

	int first_rank = 0;

	for ( i = 0; i < tasklistDepth; i++ )
	{
		first_rank += thread->instance->tree_nb_nodes_per_depth[i];
	}

	assert( globalRank >= 0 && globalRank < instance->tree_array_size );
	gen_node = &( instance->tree_array[globalRank] );
	assert( gen_node->type != MPCOMP_CHILDREN_NULL );

	if ( gen_node->type == MPCOMP_CHILDREN_LEAF )
	{
		assert( gen_node->ptr.mvp->threads );
		randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER( thread->mvp );
	}
	else
	{
		assert( gen_node->type == MPCOMP_CHILDREN_NODE );
		randBuffer = MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER( gen_node->ptr.node );
	}

	do
	{
		/* Get a random value */
		lrand48_r( randBuffer, &value );
		/* Select randomly a globalRank among neighbourhood */
		victim = ( value % nbVictims + first_rank );
	}
	while ( victim == globalRank );

	return victim;
}


static inline void ___task_allocate_larceny_order( mpcomp_thread_t *thread )
{
	assert( thread );
	assert( thread->instance );
	assert( thread->instance->team );
	mpcomp_team_t *team = thread->instance->team;
	const int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( team, MPCOMP_TASK_TYPE_UNTIED );
	const int max_num_victims = thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	assert( max_num_victims >= 0 );
	int *larceny_order = ( int * ) mpcomp_alloc( max_num_victims * sizeof( int ) );
	MPCOMP_TASK_THREAD_SET_LARCENY_ORDER( thread, larceny_order );
}

static inline int __task_get_victim_hierarchical_random( const int globalRank, const int index, mpcomp_tasklist_type_t type )
{
	int i, j;
	long int value;
	int *order;
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	mpcomp_node_t *node;
	struct mpcomp_mvp_s *mvp;
	struct drand48_data *randBuffer = NULL;
	mpcomp_generic_node_t *gen_node;
	/* Retrieve the information (microthread structure and current region) */
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	mvp = thread->mvp;
	node = mvp->father;
	instance = thread->instance;

	if ( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
		___task_allocate_larceny_order( thread );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	assert( order );

	if ( index == 1 )
	{
		int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( instance->team, type );
		int nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
		int first_rank = 0;

		while ( nbVictims == 0 )
		{
			tasklistDepth--;
			nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
		}

		for ( i = 0; i < tasklistDepth; i++ )
		{
			first_rank += thread->instance->tree_nb_nodes_per_depth[i];
		}

		assert( globalRank >= 0 && globalRank < instance->tree_array_size );
		gen_node = &( instance->tree_array[globalRank] );
		assert( gen_node->type != MPCOMP_CHILDREN_NULL );

		if ( gen_node->type == MPCOMP_CHILDREN_LEAF )
		{
			assert( gen_node->ptr.mvp->threads );
			randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER( thread->mvp );
		}
		else
		{
			assert( gen_node->type == MPCOMP_CHILDREN_NODE );
		}

		int tmporder[nbVictims - 1];

		for ( i = 0; i < nbVictims - 1; i++ )
		{
			order[i] = first_rank + i;
			order[i] = ( order[i] >= globalRank ) ? order[i] + 1 : order[i];
			tmporder[i] = order[i];
		}

		int parentrank = node->instance_global_rank - first_rank;
		int startrank = parentrank;
		int x = 1, k, l, cpt = 0;

		for ( i = 0; i < tasklistDepth; i++ )
		{
			for ( j = 0; j < ( node->barrier_num_threads - 1 ) * x; j++ )
			{
				lrand48_r( randBuffer, &value );
				k = ( value % ( ( node->barrier_num_threads - 1 ) * x - j ) + parentrank + j ) % ( nbVictims - 1 );
				l = ( parentrank + j ) % ( nbVictims - 1 );
				int tmp = tmporder[l];
				tmporder[l] = tmporder[k];
				tmporder[k] = tmp;
				order[cpt] = tmporder[l];
				cpt++;
			}

			x *= node->barrier_num_threads;
			startrank = startrank - node->rank * x;

			if ( node->father )
			{
				node = node->father;
			}

			parentrank = ( ( ( ( parentrank / x ) * x ) + x ) % ( x * node->barrier_num_threads ) + startrank ) % ( nbVictims - 1 );
			parentrank = ( parentrank >= globalRank - first_rank ) ? parentrank - 1 : parentrank;
		}
	}

	return order[index - 1];
}

static inline  void ___task_get_victim_random_order_prepare( const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type )
{
	int i;
	int *order;
	long int value;
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	struct drand48_data *randBuffer;
	mpcomp_generic_node_t *gen_node;
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls != NULL );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	if ( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
		___task_allocate_larceny_order( thread );
	}

	assert( thread->instance );
	instance = thread->instance;
	assert( instance->tree_array );
	assert( instance->tree_nb_nodes_per_depth );
	assert( instance->tree_first_node_per_depth );
	int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( instance->team, type );
	int nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	int first_rank = 0;

	while ( nbVictims == 0 )
	{
		tasklistDepth--;
		nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	}

	for ( i = 0; i < tasklistDepth; i++ )
	{
		first_rank += thread->instance->tree_nb_nodes_per_depth[i];
	}

	assert( globalRank >= 0 && globalRank < instance->tree_array_size );
	gen_node = &( instance->tree_array[globalRank] );
	assert( gen_node->type != MPCOMP_CHILDREN_NULL );

	if ( gen_node->type == MPCOMP_CHILDREN_LEAF )
	{
		assert( gen_node->ptr.mvp->threads );
		randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER( thread->mvp );
		//TODO transfert data to current thread ...
		//randBuffer = gen_node->ptr.mvp->threads->task_infos.tasklist_randBuffer;
	}
	else
	{
		assert( gen_node->type == MPCOMP_CHILDREN_NODE );
		randBuffer = gen_node->ptr.node->task_infos.tasklist_randBuffer;
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	assert( order );

	for ( i = 0; i < nbVictims; i++ )
	{
		order[i] = first_rank + i;
	}

	for ( i = 0; i < nbVictims; i++ )
	{
		int tmp = order[i];
		lrand48_r( randBuffer, &value );
		int j = value % ( nbVictims - i );
		order[i] = order[i + j];
		order[i + j] = tmp;
	}
}

static inline int __task_get_victim_random_order( int globalRank, int index,
        mpcomp_tasklist_type_t type )
{
	int *order;
	mpcomp_thread_t *thread;
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	if ( index == 1 )
	{
		___task_get_victim_random_order_prepare( globalRank, index, type );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	assert( order );
	return order[index - 1];
}

static inline  void ___task_get_victim_roundrobin_prepare( const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type )
{
	int i;
	int *order;
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls != NULL );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	instance = thread->instance;
	assert( instance->tree_array );
	assert( instance->tree_nb_nodes_per_depth );
	assert( instance->tree_first_node_per_depth );
	int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( instance->team, type );
	int nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	int first_rank = 0;

	while ( nbVictims == 0 )
	{
		tasklistDepth--;
		nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	}

	for ( i = 0; i < tasklistDepth; i++ )
	{
		first_rank += thread->instance->tree_nb_nodes_per_depth[i];
	}

	int decal = ( globalRank - first_rank + 1 ) % ( nbVictims );

	if ( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
		___task_allocate_larceny_order( thread );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	assert( order );

	for ( i = 0; i < nbVictims; i++ )
	{
		order[i] = first_rank + ( decal + i ) % ( nbVictims );
	}
}

static inline int __task_get_victim_roundrobin( const int globalRank, const int index, mpcomp_tasklist_type_t type )
{
	int *order;
	mpcomp_thread_t *thread;
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	if ( index == 1 )
	{
		___task_get_victim_roundrobin_prepare( globalRank, index, type );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	assert( order );
	return order[index - 1];
}

static inline int __task_get_victim_producer( int globalRank, __UNUSED__ int index, mpcomp_tasklist_type_t type )
{
	int i, max_elt, victim, rank, nb_elt;
	mpcomp_thread_t *thread;
	mpcomp_task_list_t *list;
	mpcomp_instance_t *instance;
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	instance = thread->instance;
	int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( instance->team, type );
	int nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	int first_rank = 0;

	while ( nbVictims == 0 )
	{
		tasklistDepth--;
		nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	}

	for ( i = 0; i < tasklistDepth; i++ )
	{
		first_rank += thread->instance->tree_nb_nodes_per_depth[i];
	}

	victim = ( globalRank == first_rank ) ? first_rank + 1 : first_rank;

	for ( max_elt = 0, i = 0; i < nbVictims - 1; i++ )
	{
		rank = i + first_rank;
		rank += ( rank >= globalRank ) ? 1 : 0;
		list = ( mpcomp_task_list_t * ) __task_get_list( rank, type );
		nb_elt = ( list ) ? OPA_load_int( &( list->nb_elements ) ) : -1;

		if ( max_elt < nb_elt )
		{
			max_elt = nb_elt;
			victim = rank;
		}
	}

	return victim;
}


/* Compare nb element in first entry of tab
 */
static inline int ___task_sort_decr( const void *opq_a, const void *opq_b,
        __UNUSED__ void *args )
{
	const int *a = ( int * ) opq_a;
	const int *b = ( int * ) opq_b;
	return ( a[0] == b[0] ) ? b[1] - a[1] : b[0] - a[0];
}

static inline int ___task_get_victim_producer_order_prepare( __UNUSED__ const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type )
{
	int i;
	int *order;
	void *ext_arg = NULL;
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	struct mpcomp_task_list_s *list;
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	instance = thread->instance;
	int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( instance->team, type );
	int nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	int first_rank = 0;

	while ( nbVictims == 0 )
	{
		tasklistDepth--;
		nbVictims = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
	}

	for ( i = 0; i < tasklistDepth; i++ )
	{
		first_rank += thread->instance->tree_nb_nodes_per_depth[i];
	}

	if ( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
		___task_allocate_larceny_order( thread );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	int nbElts[nbVictims][2];

	for ( i = 0; i < nbVictims; i++ )
	{
		nbElts[i][1] = i + first_rank;
		list = __task_get_list( nbElts[i][1], type );
		nbElts[i][0] = ( list ) ? OPA_load_int( &list->nb_elements ) : -1;
	}

	qsort_r( nbElts, nbVictims, 2 * sizeof( int ), ___task_sort_decr, ext_arg );

	for ( i = 0; i < nbVictims; i++ )
	{
		order[i] = nbElts[i][1];
	}

	return 0;
}

static inline int __task_get_victim_producer_order( const int globalRank, const int index, mpcomp_tasklist_type_t type )
{
	int *order;
	mpcomp_thread_t *thread;
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	if ( index == 1 )
	{
		___task_get_victim_producer_order_prepare( globalRank, index, type );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	assert( order );
	return order[index - 1];
}

static inline int __task_get_victim_default( int globalRank, __UNUSED__ int index,
                                    __UNUSED__ mpcomp_tasklist_type_t type )
{
	return globalRank;
}


/* Return the ith victim for task stealing initiated from element at
 * 'globalRank' */
static inline int __task_get_victim( int globalRank, int index,
                            mpcomp_tasklist_type_t type )
{
	int victim;
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls );
	mpcomp_thread_t *thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	assert( thread->instance->team );
	mpcomp_team_t *team = thread->instance->team;
	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team );

	switch ( larcenyMode )
	{
		case MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL:
			victim = __task_get_victim_hierarchical( globalRank, index, type );
			break;

		case MPCOMP_TASK_LARCENY_MODE_RANDOM:
			victim = __task_get_victim_random( globalRank, index, type );
			break;

		case MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER:
			victim = __task_get_victim_random_order( globalRank, index, type );
			break;

		case MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN:
			victim = __task_get_victim_roundrobin( globalRank, index, type );
			break;

		case MPCOMP_TASK_LARCENY_MODE_PRODUCER:
			victim = __task_get_victim_producer( globalRank, index, type );
			break;

		case MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER:
			victim = __task_get_victim_producer_order( globalRank, index, type );
			break;

		case MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM:
			victim = __task_get_victim_hierarchical_random( globalRank, index, type );
			break;

		default:
			victim = __task_get_victim_default( globalRank, index, type );
			break;
	}

	return victim;
}

/* Look in new and untied tasks lists of others */
static struct mpcomp_task_s *_mpc_task_new_larceny( void )
{
	int i, type;
	mpcomp_thread_t *thread;
	struct mpcomp_task_s *task = NULL;
	struct mpcomp_mvp_s *mvp;
	struct mpcomp_team_s *team;
	struct mpcomp_task_list_s *list;
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
	OPA_incr_int( &__private_perf_call_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
	/* Retrieve the information (microthread structure and current region) */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	mvp = thread->mvp;
	assert( mvp );
	assert( thread->instance );
	team = thread->instance->team;
	assert( team );
	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team );
	const int isMonoVictim = ( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
	                           larcenyMode == MPCOMP_TASK_LARCENY_MODE_PRODUCER );

	/* Check first for NEW tasklists, then for UNTIED tasklists */
	for ( type = 0; type <= MPCOMP_TASK_TYPE_NEW; type++ )
	{
		/* Retrieve informations about task lists */
		int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( team, type );
		int nbTasklists = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];

		while ( nbTasklists == 0 )
		{
			tasklistDepth--;
			nbTasklists = ( !tasklistDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
		}

		/* If there are task lists in which we could steal tasks */
		int rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, type );
		int victim;

		if ( nbTasklists > 1 )
		{
			if ( mpc_omp_conf_get()->omp_task_steal_last_stolen_list )
				/* Try to steal inside the last stolen list*/
			{
				if ( ( list = MPCOMP_TASK_MVP_GET_LAST_STOLEN_TASK_LIST( mvp, type ) ) )
				{
					if ( ( task = __task_steal( list, rank, victim ) ) )
					{
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
						OPA_incr_int( &__private_perf_success_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
						return task;
					}
				}
			}

			if ( mpc_omp_conf_get()->omp_task_resteal_to_last_thief )
				/* Try to steal to last thread that stole us a task */
			{
				if ( mvp->task_infos.last_thief != -1 && ( list = __task_get_list( mvp->task_infos.last_thief, type ) ) )
				{
					victim = mvp->task_infos.last_thief;

					if ( ( task = __task_steal( list, rank, victim ) ) )
					{
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
						OPA_incr_int( &__private_perf_success_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
						return task;
					}

					mvp->task_infos.last_thief = -1;
				}
			}

			/* Get the rank of the ancestor containing the task list */
			int nbVictims = ( isMonoVictim ) ? 1 : nbTasklists;

			/* Look for a task in all victims lists */
			for ( i = 1; i < nbVictims + 1; i++ )
			{
				victim = __task_get_victim( rank, i, type );

				if ( victim != rank )
				{
					list = __task_get_list( victim, type );

					if ( list )
					{
						task = __task_steal( list, rank, victim );
					}

					if ( task )
					{
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
						OPA_incr_int( &__private_perf_success_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
						return task;
					}
				}
			}
		}
	}

	return NULL;
}

/*
 * Schedule remaining tasks in the different task lists (tied, untied, new).
 *
 * Called at several schedule points :
 *     - in taskyield regions
 *     - in taskwait regions
 *     - in implicit and explicit barrier regions
 *
 * \param thread
 * \param mvp
 * \param team
 */

static void ___task_schedule( mpcomp_thread_t *thread, mpcomp_mvp_t *mvp,  __UNUSED__ mpcomp_team_t *team )
{
	int type;
	mpcomp_task_t *task;
	mpcomp_task_list_t *list;
	/* All arguments must be non null */
	assert( thread && mvp && team );

	/*  If only one thread is running or no task has been find yet, tasks are not delayed.
	    No need to schedule                                    */
	if ( thread->info.num_threads == 1 || thread->instance->task_infos.is_initialized == false )
	{
		mpc_common_nodebug( "sequential or no task" );
		return;
	}

	for ( type = 0, task = NULL; !task && type <= MPCOMP_TASK_TYPE_NEW; type++ )
	{
		const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, type );
		list = __task_get_list( node_rank, ( mpcomp_tasklist_type_t ) type );
		assert( list );
#ifdef MPCOMP_USE_MCS_LOCK
		__task_list_consumer_lock( list, thread->task_infos.opaque );
#else /* MPCOMP_USE_MCS_LOCK */

		if ( __task_list_consumer_trylock( list, thread->task_infos.opaque ) )
		{
			continue;
		}

#endif /* MPCOMP_USE_MCS_LOCK */
		task = __task_list_pop( list, false, thread->task_infos.one_list_per_thread );
		__task_list_consumer_unlock( list, thread->task_infos.opaque );
	}

	/* If no task found previously, try to thieve a task somewhere */
	if ( task == NULL )
	{
		task = _mpc_task_new_larceny();

		if ( task )
		{
			task->is_stealed = true;
		}
	}

	/* All tasks lists are empty, so exit task scheduling function */
	if ( task == NULL )
	{
		return;
	}

	__task_execute( task );
	/* Clean function */
	mpcomp_taskgroup_del_task( task );
#ifdef MPCOMP_USE_TASKDEP
	_mpc_task_dep_new_finalize( task );
#endif /* MPCOMP_USE_TASKDEP */
	_mpc_task_unref_parent_task( task );
}

/**
 * Perform a taskwait construct.
 *
 * Can be the entry point for a compiler.
 */
void _mpc_task_wait( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpcomp_task_t *current_task = NULL;
	mpcomp_thread_t *omp_thread_tls = NULL;

    __mpcomp_init();

	omp_thread_tls = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( omp_thread_tls );
	assert( omp_thread_tls->info.num_threads > 0 );

#if OMPT_SUPPORT
    __mpcompt_callback_sync_region( ompt_sync_region_taskwait, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

	/* Perform this construct only with multiple threads
	 * (with one threads, tasks are schedulded directly,
	 * therefore taskwait has no effect)
	 */
	if ( omp_thread_tls->info.num_threads > 1 )
	{
		current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
        assert(current_task); // Fail if tasks disable...(from full barrier call

#if OMPT_SUPPORT
        __mpcompt_callback_sync_region_wait( ompt_sync_region_taskwait, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

		/* Look for a children tasks list */
		while ( OPA_load_int( &( current_task->refcount ) ) != 1 )
		{
			/* Schedule any other task
			 * prevent recursive calls to _mpc_task_wait with argument 0 */
			_mpc_task_schedule( 0 );
		}

#if OMPT_SUPPORT
        __mpcompt_callback_sync_region_wait( ompt_sync_region_taskwait, ompt_scope_end );
#endif /* OMPT_SUPPORT */
	}
#if OMPT_SUPPORT
    else {
        __mpcompt_callback_sync_region_wait( ompt_sync_region_taskwait, ompt_scope_begin );
        __mpcompt_callback_sync_region_wait( ompt_sync_region_taskwait, ompt_scope_end );
    }
#endif /* OMPT_SUPPORT */

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
	const int a = OPA_load_int( &__private_perf_call_steal );
	const int b = OPA_load_int( &__private_perf_success_steal );
	const int c = OPA_load_int( &__private_perf_create_task );
	const int d = OPA_load_int( &__private_perf_executed_task );

	if ( 1 && !omp_thread_tls->rank )
	{
		fprintf( stderr, "try steal : %d - success steal : %d -- total tasks : %d -- performed tasks : %d\n", a, b, c, d );
	}
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */

#if OMPT_SUPPORT
    __mpcompt_callback_sync_region( ompt_sync_region_taskwait, ompt_scope_end );
#endif /* OMPT_SUPPORT */
}

/**
 * Task scheduling.
 *
 * Try to find a task to be scheduled and execute it.
 * This is the main function (it calls an internal function).
 *
 * \param[in] need_taskwait   True if it is necessary to perform a taskwait
 *                        after scheduling some tasks.
 */
void _mpc_task_schedule( int need_taskwait )
{
	mpcomp_mvp_t *mvp;
	mpcomp_team_t *team;
	mpcomp_thread_t *thread;
	/* Get thread from current tls */
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread );
	/* Get mvp father from thread */
	mvp = thread->mvp;

	/* Sequential execution => no delayed task */
	if ( !mvp )
	{
		return;
	}

	/* Get team from thread */
	assert( thread->instance );
	team = thread->instance->team;
	assert( team );
	___task_schedule( thread, mvp, team );

	/* schedule task can produce task ... */
	if ( thread->info.num_threads > 1
         && need_taskwait )
	{
        while( OPA_load_int( &( MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread)->refcount ) ) != 1 )
            _mpc_task_schedule( 0 );
	}

	return;
}

/**
 * Taskyield construct.
 *
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void _mpc_taskyield( void )
{
	/* Actually, do nothing */
}

void _mpc_task_new_coherency_entering_parallel_region()
{
#if 0
	struct mpcomp_team_s *team;
	struct mpcomp_mvp_s **mvp;
	mpcomp_thread_t *t;
	mpcomp_thread_t *lt;
	int i, nb_mvps;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	assert( t != NULL );

	if ( t->children_instance->nb_mvps > 1 )
	{
		team = t->children_instance->team;
		assert( team != NULL );
		/* Check team tasking cohenrency */
		mpc_common_debug( "_mpc_task_new_coherency_entering_parallel_region: "
		            "Team init %d and tasklist init %d with %d nb_tasks\n",
		            OPA_load_int( &team->tasking_init_done ),
		            OPA_load_int( &team->tasklist_init_done ),
		            OPA_load_int( &team->nb_tasks ) );
		/* Check per thread task system coherency */
		mvp = t->children_instance->mvps;
		assert( mvp != NULL );
		nb_mvps = t->children_instance->nb_mvps;

		for ( i = 0; i < nb_mvps; i++ )
		{
			lt = &mvp[i]->threads[0];
			assert( lt != NULL );
			mpc_common_debug( "_mpc_task_new_coherency_entering_parallel_region: "
			            "Thread in mvp %d init %d in implicit task %p\n",
			            mvp[i]->rank, lt->tasking_init_done, lt->current_task );
			assert( mpcomp_task_property_isset( lt->current_task->property,
			             MPCOMP_TASK_IMPLICIT ) != 0 );
		}
	}

#endif
}

void _mpc_task_new_coherency_ending_parallel_region()
{
#if 0
	struct mpcomp_team_s *team;
	struct mpcomp_mvp_s **mvp;
	mpcomp_thread_t *t;
	mpcomp_thread_t *lt;
	int i_task, i, nb_mvps;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	assert( t != NULL );

	if ( t->children_instance->nb_mvps > 1 )
	{
		team = t->children_instance->team;
		assert( team != NULL );
		/* Check team tasking cohenrency */
		mpc_common_debug( "_mpc_task_new_coherency_ending_parallel_region: "
		            "Team init %d and tasklist init %d with %d nb_tasks\n",
		            OPA_load_int( &team->tasking_init_done ),
		            OPA_load_int( &team->tasklist_init_done ),
		            OPA_load_int( &team->nb_tasks ) );
		/* Check per thread and mvp task system coherency */
		mvp = t->children_instance->mvps;
		assert( mvp != NULL );
		nb_mvps = t->children_instance->nb_mvps;

		for ( i = 0; i < nb_mvps; i++ )
		{
			lt = &mvp[i]->threads[0];
			mpc_common_debug( "_mpc_task_new_coherency_ending_parallel_region: "
			            "Thread %d init %d in implicit task %p\n",
			            lt->rank, lt->tasking_init_done, lt->current_task );
			assert( lt->current_task != NULL );
			assert( lt->current_task->children == NULL );
			assert( lt->current_task->list == NULL );
			assert( lt->current_task->depth == 0 );

			if ( lt->tasking_init_done )
			{
				if ( mvp[i]->threads[0].tied_tasks )
					assert( __task_list_is_empty( mvp[i]->threads[0].tied_tasks ) ==
					             1 );

				for ( i_task = 0; i_task < MPCOMP_TASK_TYPE_COUNT; i_task++ )
				{
					if ( mvp[i]->tasklist[i_task] )
					{
						assert( __task_list_is_empty( mvp[i]->tasklist[i_task] ) ==
						             1 );
					}
				}
			}
		}
	}

#endif
}

#if 0
void _mpc_task_new_coherency_barrier()
{

	mpcomp_thread_t *t;
	struct mpcomp_task_list_s *list = NULL;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	assert( t != NULL );
	mpc_common_debug( "_mpc_task_new_coherency_barrier: "
	            "Thread %d exiting barrier in implicit task %p\n",
	            t->rank, t->current_task );
	assert( t->current_task != NULL );
	assert( t->current_task->children == NULL );
	assert( t->current_task->list == NULL );
	assert( t->current_task->depth == 0 );

	if ( t->tasking_init_done )
	{
		/* Check tied tasks list */
		assert( __task_list_is_empty( t->tied_tasks ) == 1 );
		/* Check untied tasks list */
		list =
		    __task_get_list( t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED],
		                          MPCOMP_TASK_TYPE_UNTIED );
		assert( list != NULL );
		assert( __task_list_is_empty( list ) == 1 );
		/* Check New type tasks list */
		list = __task_get_list( t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW],
		                             MPCOMP_TASK_TYPE_NEW );
		assert( list != NULL );
		assert( __task_list_is_empty( list ) == 1 );
	}


}
#endif

#else /* MPCOMP_TASK */

/*
 * Creation of an OpenMP task
 * Called when encountering an 'omp task' construct
 */
void _mpc_task_new( void ( *fn )( void * ), void *data,
                    void ( *cpyfn )( void *, void * ), long arg_size, long arg_align,
                    bool if_clause, unsigned flags )
{
	if ( cpyfn )
	{
		mpc_common_debug_abort();
	}

	fn( data );
}

/*
 * Wait for the completion of the current task's children
 * Called when encountering a taskwait construct
 * Do nothing here as task are executed directly
 */
void _mpc_task_wait() {}

/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void _mpc_taskyield()
{
	/* Actually, do nothing */
}

#endif /* MPCOMP_TASK */

/*********************
 * TASK DEPENDENCIES *
 *********************/

#ifdef MPCOMP_USE_TASKDEP

static inline int ___task_dep_flag_with_dep( const unsigned flags )
{
	return flags & MPCOMP_TASK_DEP_GOMP_DEPS_FLAG;
}

/** HASHING INTEL */
static inline uint32_t ___task_dep_intel_hash_func( uintptr_t addr, uint32_t size, uint32_t seed )
{
	return ( ( addr >> seed ) ^ addr ) % size;
}

/** HASHING MPC */
static inline uint32_t ___task_dep_mpc_hash_fun( uintptr_t addr, uint32_t size, uint32_t seed )
{
	return ( addr >> seed ) % size;
}

static inline mpcomp_task_dep_node_t * ___task_dep_new_node( void )
{
	mpcomp_task_dep_node_t *new_node;
	new_node = sctk_malloc( sizeof( mpcomp_task_dep_node_t ) );
	assert( new_node );
	memset( new_node, 0, sizeof( mpcomp_task_dep_node_t ) );
	OPA_store_int( &( new_node->ref_counter ), 1 );
	return new_node;
}

static inline mpcomp_task_dep_node_t * ___task_dep_node_ref( mpcomp_task_dep_node_t *node )
{
	assert( node );
        OPA_fetch_and_incr_int(&(node->ref_counter));
	return node;
}

static inline int ___task_dep_node_unref( mpcomp_task_dep_node_t *node )
{
	if ( !node )
	{
		return 0;
	}

	assert( OPA_load_int( &( node->ref_counter ) ) );
	/* Fetch and decr to prevent double free */
	const int prev = OPA_fetch_and_decr_int( &( node->ref_counter ) ) - 1;

	if ( !prev )
	{
		assert( !OPA_load_int( &( node->ref_counter ) ) );
		sctk_free( node );
		node = NULL;
		return 1;
	}

	return 0;
}

static inline void ___task_dep_free_node_list_elem( mpcomp_task_dep_node_list_t *list )
{
	mpcomp_task_dep_node_list_t *node_list;

	if ( !list )
	{
		return;
	}

	while ( ( node_list = list ) )
	{
		___task_dep_node_unref( node_list->node );
		list = node_list->next;
		sctk_free( node_list );
	}
}

static inline mpcomp_task_dep_node_list_t * ___task_dep_alloc_node_list_elem( mpcomp_task_dep_node_list_t *list,
                                                                                 mpcomp_task_dep_node_t *node )
{
	mpcomp_task_dep_node_list_t *new_node;
	assert( node );
	//assert(list);
	new_node = sctk_malloc( sizeof( mpcomp_task_dep_node_list_t ) );
	assert( new_node );
	new_node->node = ___task_dep_node_ref( node );
	new_node->next = list;
	return new_node;
}

static inline int ___task_dep_free_hash_table( mpcomp_task_dep_ht_table_t *htable )
{
	unsigned int i;
	int removed_entries = 0;

	for ( i = 0; htable && i < htable->hsize; i++ )
	{
		if ( htable[i].buckets->entry )
		{
			mpcomp_task_dep_ht_entry_t *entry;

			while ( ( entry = htable[i].buckets->entry ) )
			{
				___task_dep_free_node_list_elem( entry->last_in );
				___task_dep_node_unref( entry->last_out );
				htable[i].buckets->entry = entry;
				sctk_free( entry );
			}
		}
	}

	return removed_entries;
}

/**
 *
 */
static inline mpcomp_task_dep_ht_table_t * ___task_dep_alloc_hash_table( mpcomp_task_dep_hash_func_t hfunc )
{
	mpcomp_task_dep_ht_table_t *new_htable;
	assert( hfunc );
	const long infos_size = _mpc_task_align_single_malloc(
	                            sizeof( mpcomp_task_dep_ht_table_t ), MPCOMP_TASK_DEFAULT_ALIGN );
	const long array_size =
	    sizeof( mpcomp_task_dep_ht_bucket_t ) * MPCOMP_TASK_DEP_MPC_HTABLE_SIZE;
	assert( MPCOMP_OVERFLOW_SANITY_CHECK( ( unsigned long ) infos_size, ( unsigned long ) array_size ) );
	new_htable =
	    ( mpcomp_task_dep_ht_table_t * ) sctk_malloc( infos_size + array_size );
	assert( new_htable );
	/* Better than a loop */
	memset( new_htable, 0, infos_size + array_size );
	new_htable->hsize = MPCOMP_TASK_DEP_MPC_HTABLE_SIZE;
	new_htable->hseed = MPCOMP_TASK_DEP_MPC_HTABLE_SEED;
	new_htable->hfunc = hfunc;
	new_htable->buckets =
	    ( mpcomp_task_dep_ht_bucket_t * ) ( ( uintptr_t ) new_htable + infos_size );
	return new_htable;
}

static inline mpcomp_task_dep_ht_entry_t * ___task_dep_ht_add( mpcomp_task_dep_ht_table_t *htable, uintptr_t addr )
{
	mpcomp_task_dep_ht_entry_t *entry;
	const uint32_t hash = htable->hfunc( addr, htable->hsize, htable->hseed );

	for ( entry = htable->buckets[hash].entry; entry; entry = entry->next )
	{
		if ( entry->base_addr == addr )
		{
			break;
		}
	}

	if ( entry )
	{
		return entry;
	}

	/* Allocation */
	entry = ( mpcomp_task_dep_ht_entry_t * ) sctk_malloc(
	            sizeof( mpcomp_task_dep_ht_entry_t ) );
	assert( entry );
	memset( entry, 0, sizeof( mpcomp_task_dep_ht_entry_t ) );
	entry->base_addr = addr;

	/* No previous addr in bucket */
	if ( htable->buckets[hash].num_entries > 0 )
	{
		entry->next = htable->buckets[hash].entry;
	}

	htable->buckets[hash].entry = entry;
	htable->buckets[hash].num_entries += 1;
	return entry;
}

static inline mpcomp_task_dep_ht_entry_t * ___task_dep_ht_find( mpcomp_task_dep_ht_table_t *htable,
                               uintptr_t addr )
{
	mpcomp_task_dep_ht_entry_t *entry = NULL;
	const uint32_t hash = htable->hfunc( addr, htable->hsize, htable->hseed );

	/* Search correct entry in bucket */
	if ( htable->buckets[hash].num_entries > 0 )
	{
		for ( entry = htable->buckets[hash].entry; entry; entry = entry->next )
		{
			if ( entry->base_addr == addr )
			{
				break;
			}
		}
	}

	return entry;
}

static inline int ___task_dep_process( mpcomp_task_dep_node_t *task_node,
                                       mpcomp_task_dep_ht_table_t *htable,
                                       void **depend )
{
        assert(task_node);
        assert(htable);
        assert(depend);

	size_t i, j;
	int predecessors_num;
	mpcomp_task_dep_ht_entry_t *entry;
	mpcomp_task_dep_node_t *last_out;
	const size_t tot_deps_num = ( uintptr_t ) depend[0];
	const size_t out_deps_num = ( uintptr_t ) depend[1];

	if ( !tot_deps_num )
	{
		return 0;
	}

	// Filter redundant value
#if OMPT_SUPPORT
  uintptr_t task_already_process_num = 0;

  ompt_dependence_t* task_deps =
    (ompt_dependence_t*) sctk_malloc( sizeof( ompt_dependence_t) * tot_deps_num );
  assert( task_deps );
  memset( task_deps, 0, sizeof( ompt_dependence_t) * tot_deps_num );
#else
	size_t task_already_process_num = 0;
	uintptr_t *task_already_process_list =
	    ( uintptr_t * ) sctk_malloc( sizeof( uintptr_t ) * tot_deps_num );
	assert( task_already_process_list );
#endif /* OMPT_SUPPORT */
	predecessors_num = 0;

	for ( i = 0; i < tot_deps_num; i++ )
	{
		int redundant = 0;
		/* FIND HASH IN HTABLE */
		const uintptr_t addr = ( uintptr_t ) depend[2 + i];
		const int type =
		    ( i < out_deps_num ) ? MPCOMP_TASK_DEP_OUT : MPCOMP_TASK_DEP_IN;
		assert( task_already_process_num < tot_deps_num );

#if OMPT_SUPPORT
        ompt_dependence_type_t ompt_type = (i < out_deps_num) ?
        ompt_dependence_type_out : ompt_dependence_type_in;
#endif /* OMPT_SUPPORT */


		for ( j = 0; j < task_already_process_num; j++ )
		{
#if OMPT_SUPPORT
      if( (uintptr_t)task_deps[j].variable.ptr == addr ) {
        if( task_deps[j].dependence_type != ompt_dependence_type_inout
              && ompt_type != task_deps[j].dependence_type )
          task_deps[j].dependence_type = ompt_dependence_type_inout;
        redundant = 1;
        break;
      }
#else
			if ( task_already_process_list[j] == addr )
			{
				redundant = 1;
				break;
			}
#endif /* OMPT_SUPPORT */
		}

		mpc_common_nodebug( "task: %p deps: %p redundant : %d \n", task_node, addr,
		              redundant );

		/** OUT are in first position en OUT > IN deps */
		if ( redundant )
		{
			continue;
		}

#if OMPT_SUPPORT
        task_deps[task_already_process_num].variable.ptr = (void*) addr;
        task_deps[task_already_process_num].dependence_type = ompt_type;
        task_already_process_num++;
#else
		task_already_process_list[task_already_process_num++] = addr;
#endif /* OMPT_SUPPORT */
		entry = ___task_dep_ht_add( htable, addr );
		assert( entry );
		last_out = entry->last_out;

		// NEW [out] dep must be after all [in] deps
		if ( type == MPCOMP_TASK_DEP_OUT && entry->last_in != NULL )
		{
			mpcomp_task_dep_node_list_t *node_list;

			for ( node_list = entry->last_in; node_list; node_list = node_list->next )
			{
				mpcomp_task_dep_node_t *node = node_list->node;

				if ( OPA_load_int( &( node->status ) ) <
				     MPCOMP_TASK_DEP_TASK_FINALIZED )
					// if( node->task )
				{
					MPCOMP_TASK_DEP_LOCK_NODE( node );

					if ( OPA_load_int( &( node->status ) ) <
					     MPCOMP_TASK_DEP_TASK_FINALIZED )
						// if( node->task )
					{
						node->successors = ___task_dep_alloc_node_list_elem(
						                       node->successors, task_node );
						predecessors_num++;
						mpc_common_nodebug( "IN predecessors" );

#if OMPT_SUPPORT
                        __mpcompt_callback_task_dependence( &node->task->ompt_task_data,
                                                            &task_node->task->ompt_task_data );
#endif /* OMPT_SUPPORT */
					}

					MPCOMP_TASK_DEP_UNLOCK_NODE( node );
				}
			}

			___task_dep_free_node_list_elem( entry->last_in );
			entry->last_in = NULL;
		}
		else
		{
			/** Non executed OUT dependency**/
			if ( last_out && ( OPA_load_int( &( last_out->status ) ) <
			                   MPCOMP_TASK_DEP_TASK_FINALIZED ) )
				// if( last_out && last_out->task )
			{
				MPCOMP_TASK_DEP_LOCK_NODE( last_out );

				if ( OPA_load_int( &( last_out->status ) ) <
				     MPCOMP_TASK_DEP_TASK_FINALIZED )
					// if( last_out->task )
				{
					last_out->successors = ___task_dep_alloc_node_list_elem(
					                           last_out->successors, task_node );
					predecessors_num++;
					mpc_common_nodebug( "OUT predecessors" );

#if OMPT_SUPPORT
                    __mpcompt_callback_task_dependence( &last_out->task->ompt_task_data,
                                                        &task_node->task->ompt_task_data );
#endif /* OMPT_SUPPORT */
				}

				MPCOMP_TASK_DEP_UNLOCK_NODE( last_out );
			}
		}

		if ( type == MPCOMP_TASK_DEP_OUT )
		{
			___task_dep_node_unref( last_out );
			entry->last_out = ___task_dep_node_ref( task_node );
			mpc_common_nodebug( "last_out : %p -- %p", last_out, addr );
		}
		else
		{
			assert( type == MPCOMP_TASK_DEP_IN );
			entry->last_in =
			    ___task_dep_alloc_node_list_elem( entry->last_in, task_node );
		}
	}

#if OMPT_SUPPORT
    task_node->ompt_task_deps = task_deps;
    depend[0] = (void*) task_already_process_num;
#else
	sctk_free( task_already_process_list );
#endif /* OMPT_SUPPORT */

	return predecessors_num;
}

void _mpc_task_dep_new_finalize( mpcomp_task_t *task )
{
	mpcomp_task_dep_node_t *task_node, *succ_node;
	mpcomp_task_dep_node_list_t *list_elt;
	assert( task );

	//if ( !( task->task_dep_infos->htable ) )
	//{
		/* remove all elements from task dep hash table */
		//(void) ___task_dep_free_hash_table( task->task_dep_infos->htable );
		// task->task_dep_infos->htable = NULL;
	//}

	if ( !( task->task_dep_infos ) )
	{
		return;
	}

	task_node = task->task_dep_infos->node;

	/* No dependers */
	if ( !( task_node ) )
	{
		return;
	}

	/* Release Task Deps */
	MPCOMP_TASK_DEP_LOCK_NODE( task_node );
	OPA_store_int( &( task_node->status ), MPCOMP_TASK_DEP_TASK_FINALIZED );
	MPCOMP_TASK_DEP_UNLOCK_NODE( task_node );

	/* Unref my successors */
	while ( ( list_elt = task_node->successors ) )
	{
		succ_node = list_elt->node;
		const int prev =
		    OPA_fetch_and_decr_int( &( succ_node->predecessors ) ) - 1;

		if ( !prev && succ_node->if_clause )
		{
			if ( OPA_load_int( &( succ_node->status ) ) != MPCOMP_TASK_DEP_TASK_FINALIZED )
				if ( OPA_cas_int( &( succ_node->status ), MPCOMP_TASK_DEP_TASK_NOT_EXECUTE, MPCOMP_TASK_DEP_TASK_RELEASED ) == MPCOMP_TASK_DEP_TASK_NOT_EXECUTE )
				{
					_mpc_task_process( succ_node->task, 1 );
				}
		}

		task_node->successors = list_elt->next;
		___task_dep_node_unref( succ_node );
		sctk_free( list_elt );
	}


#if OMPT_SUPPORT
    if( task_node->ompt_task_deps ) {
        sctk_free( task_node->ompt_task_deps );
        task_node->ompt_task_deps = NULL;
    }
#endif /* OMPT_SUPPORT */


	___task_dep_node_unref( task_node );
	return;
}

void _mpc_task_new_with_deps( void ( *fn )( void * ), void *data,
                            void ( *cpyfn )( void *, void * ), long arg_size,
                            long arg_align, bool if_clause, unsigned flags,
                            void **depend, bool intel_alloc, mpcomp_task_t *intel_task )
{
	int predecessors_num;
	mpcomp_thread_t *thread;
	mpcomp_task_dep_node_t *task_node;
	mpcomp_task_t *current_task, *new_task;

	__mpcomp_init();

	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

    if ( !intel_alloc ) {
        new_task = _mpc_task_alloc( fn, data, cpyfn, arg_size, arg_align, if_clause, flags,
                                    ___task_dep_flag_with_dep( flags ));
    }
    else{
        new_task = intel_task;
    }

    assert(new_task);

    if ( thread->info.num_threads == 1
         || !( ___task_dep_flag_with_dep( flags ))) {
#if OMPT_SUPPORT
        if( ___task_dep_flag_with_dep( flags )) {
            ompt_dependence_t *ompt_task_deps = ___mpcompt_task_process_deps( depend );

            __mpcompt_callback_dependences( new_task,
                                            ompt_task_deps,
                                            (int) depend[0] );

            sctk_free( ompt_task_deps );
            ompt_task_deps = NULL;
        }
#endif /* OMPT_SUPPORT */

        _mpc_task_process( new_task, if_clause );
        return;
    }

	current_task = ( mpcomp_task_t * ) MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
	/* Is it possible ?! See GOMP source code	*/
	assert( ( uintptr_t ) depend[0] > ( uintptr_t ) 0 );

	if ( !( current_task->task_dep_infos->htable ) )
	{
		current_task->task_dep_infos->htable =
		    ___task_dep_alloc_hash_table( ___task_dep_mpc_hash_fun );
		assert( current_task->task_dep_infos->htable );
	}

	task_node = ___task_dep_new_node();
	assert( task_node );

    if( intel_alloc ) {
        new_task->task_dep_infos = sctk_malloc(sizeof(mpcomp_task_dep_task_infos_t));
        assert(new_task->task_dep_infos);
        memset(new_task->task_dep_infos, 0, sizeof(mpcomp_task_dep_task_infos_t));
    }

	assert( new_task );
	new_task->task_dep_infos = sctk_malloc( sizeof( mpcomp_task_dep_task_infos_t ) );
	assert( new_task->task_dep_infos );
	memset( new_task->task_dep_infos, 0, sizeof( mpcomp_task_dep_task_infos_t ) );
	/* TODO remove redundant assignement (see ___task_dep_new_node) */
	task_node->task = NULL;

	/* Can't be execute by release func */
	OPA_store_int( &( task_node->predecessors ), 0 );
	OPA_store_int( &( task_node->status ),
	               MPCOMP_TASK_DEP_TASK_PROCESS_DEP );
	predecessors_num = ___task_dep_process(
	                       task_node, current_task->task_dep_infos->htable, depend );
	task_node->if_clause = if_clause;
	task_node->task = new_task;
	new_task->task_dep_infos->node = task_node;
	/* Should be remove TOTEST */
	OPA_read_write_barrier();

#if OMPT_SUPPORT
    __mpcompt_callback_dependences( new_task,
                                    new_task->task_dep_infos->node->ompt_task_deps,
                                    (int) depend[0] );
#endif

	/* task_node->predecessors can be update by release task */
	OPA_add_int( &( task_node->predecessors ), predecessors_num );
	OPA_store_int( &( task_node->status ),
	               MPCOMP_TASK_DEP_TASK_NOT_EXECUTE );

	if ( !if_clause )
	{
		while ( OPA_load_int( &( task_node->predecessors ) ) )
		{
			_mpc_task_schedule( 0 ); /* schedule thread doing if0 with dependances until deps resolution
                                        _mpc_task_schedule(0) because refcount will remain >= 2 with if0
                                        _mpc_task_schedule(1) would not stop looping */
		}

		if ( intel_alloc )
		{
			/* Because with intel compiler task code is between begin_if0 and
			 * complet_if0 call, so we don't call it in _mpc_task_process */
			return;
		}
	}

	if ( OPA_load_int( &( task_node->predecessors ) ) == 0 )
	{
		if ( OPA_load_int( &( task_node->status ) ) != MPCOMP_TASK_DEP_TASK_FINALIZED )
			if ( OPA_cas_int( &( task_node->status ), MPCOMP_TASK_DEP_TASK_NOT_EXECUTE, MPCOMP_TASK_DEP_TASK_RELEASED ) == MPCOMP_TASK_DEP_TASK_NOT_EXECUTE )
			{
				_mpc_task_process( new_task, if_clause );
			}
	}
}
#endif /* MPCOMP_USE_TASKDEP */



/*************
 * TASKGROUP *
 *************/

#ifdef MPCOMP_TASKGROUP

void _mpc_task_taskgroup_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpcomp_task_t *current_task = NULL;
	mpcomp_thread_t *omp_thread_tls = NULL;
	mpcomp_task_taskgroup_t *new_taskgroup = NULL;

	__mpcomp_init();

	omp_thread_tls = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( omp_thread_tls );

	current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
	assert( current_task );

	new_taskgroup = ( mpcomp_task_taskgroup_t * ) sctk_malloc( sizeof( mpcomp_task_taskgroup_t ) );
	assert( new_taskgroup );

	/* Init new task group and store it in current task */
	memset( new_taskgroup, 0, sizeof( mpcomp_task_taskgroup_t ) );
	new_taskgroup->prev = current_task->taskgroup;
	current_task->taskgroup = new_taskgroup;

#if OMPT_SUPPORT
    __mpcompt_callback_sync_region( ompt_sync_region_taskgroup, ompt_scope_begin );
#endif /* OMPT_SUPPORT */
}

void _mpc_task_taskgroup_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpcomp_task_t *current_task = NULL;
	mpcomp_thread_t *omp_thread_tls = NULL;
	mpcomp_task_taskgroup_t *taskgroup = NULL;

	omp_thread_tls = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( omp_thread_tls );

	current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
	assert( current_task );
	taskgroup = current_task->taskgroup;

	if ( !taskgroup )
	{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
        __mpcompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

		return;
	}

#if OMPT_SUPPORT
    __mpcompt_callback_sync_region_wait( ompt_sync_region_taskgroup, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

	while ( OPA_load_int( &( taskgroup->children_num ) ) )
	{
		_mpc_task_schedule( 1 );
	}

#if OMPT_SUPPORT
    __mpcompt_callback_sync_region_wait( ompt_sync_region_taskgroup, ompt_scope_end );
#endif /* OMPT_SUPPORT */

	current_task->taskgroup = taskgroup->prev;
	sctk_free( taskgroup );

#if OMPT_SUPPORT
    __mpcompt_callback_sync_region( ompt_sync_region_taskgroup, ompt_scope_end );
#endif /* OMPT_SUPPORT */
}
#else  /* MPCOMP_TASKGROUP */
void _mpc_task_taskgroup_start( void )
{
}
void _mpc_task_taskgroup_end( void )
{
}
#endif /* MPCOMP_TASKGROUP */



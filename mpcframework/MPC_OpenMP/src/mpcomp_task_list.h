#if !defined( __MPCOMP_TASK_LIST_H__ ) && defined( MPCOMP_TASK )
#define __MPCOMP_TASK_LIST_H__

#include "sctk_debug.h" 
#include "mpcomp_task.h"
#include "sctk_atomics.h"
#include "sctk_spinlock.h"

/*** Task list primitives ***/

static inline void 
mpcomp_task_list_new( mpcomp_task_list_t *list )
{
	list->total = 0;
	list->head = NULL;
	list->tail = NULL;
	list->lock = SCTK_SPINLOCK_INITIALIZER;
	sctk_atomics_store_int( &( list->nb_elements ), 0 );
	sctk_atomics_store_int( &( list->nb_larcenies ), 0 );
}

static inline void 
mpcomp_task_list_free( mpcomp_task_list_t *list )
{
	mpcomp_free( 1, list, sizeof( mpcomp_task_list_t ) );
}

static inline int 
mpcomp_task_list_isempty( mpcomp_task_list_t *list )
{
	return ( sctk_atomics_load_int( &( list->nb_elements ) == 0 ) );
}

static inline void 
mpcomp_task_list_pushtohead( mpcomp_task_list_t *list, mpcomp_task_t *task)
{
	if( mpcomp_task_list_isempty( list ) ) 
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

	sctk_atomics_incr_int( &( list->nb_elements ) );
	list->total += 1;
	task->list = list;
}

static inline void 
mpcomp_task_list_pushtotail(mpcomp_task_list_t *list, mpcomp_task_t *task)
{
	if( mpcomp_task_list_isempty( list ) ) 
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
	  
	sctk_atomics_incr_int( &( list->nb_elements ) );
	list->total += 1;
	task->list = list;
}

static inline mpcomp_task_t *
mpcomp_task_list_popfromhead( mpcomp_task_list_t *list )
{
	if( !mpcomp_task_list_isempty( list ) ) 
	{
		mpcomp_task_t *task = list->head;
    	if( task->list ) 
		{
      	if( task->next )
			{
          	task->next->prev = NULL;
			}

        	list->head = task->next;
         sctk_atomics_decr_int(&list->nb_elements);
         task->list = NULL;

         return task;
     	}
	}
	return NULL;
}

static inline mpcomp_task_t*
mpcomp_task_list_popfromtail( mpcomp_task_list_t *list )
{
	if( !mpcomp_task_list_isempty( list ) ) 
	{
		mpcomp_task_t *task = list->tail;
	   if( task->list ) 
		{
			if( task->prev )
			{
				task->prev->next = NULL;
			}
		   
			list->tail = task->prev;
		   sctk_atomics_decr_int( &( list->nb_elements ) );
		   task->list = NULL;
		   return task;
	  	}
	}
	return NULL;
} 

static inline int 
mpcomp_task_list_remove( mpcomp_task_list_t *list, mpcomp_task_t *task )
{

	if( task->list == NULL || mpcomp_task_list_isempty( list ) )
		return -1;

	if( task == list->head )
		list->head = task->next;
	  
	if( task == list->tail )
		list->tail = task->prev;

	if( task->next )
		task->next->prev = task->prev;
	  
	if (task->prev)
		task->prev->next = task->next;

	sctk_atomics_decr_int(&list->nb_elements);
 	task->list = NULL;

	return 1;
}

static inline void 
mpcomp_task_list_lock( mpcomp_task_list_t *list )
{
	sctk_spinlock_lock( &( list->lock ) );
}

static inline void 
mpcomp_task_list_unlock( mpcomp_task_list_t *list )
{
	sctk_spinlock_unlock( &( list->lock ) );
}

static inline void 
mpcomp_task_list_trylock( mpcomp_task_list_t *list )
{
	sctk_spinlock_trylock( &( list->lock ) );
}

#endif /* !__MPCOMP_TASK_LIST_H__ && MPCOMP_TASK */

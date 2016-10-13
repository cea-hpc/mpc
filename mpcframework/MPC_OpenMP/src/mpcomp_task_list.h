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

#include "mpcomp_macros.h"

#if ( !defined( __MPCOMP_TASK_LIST_H__ ) && defined( MPCOMP_TASK ) )
#define __MPCOMP_TASK_LIST_H__

#include "mpcomp.h"
#include "sctk.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"
#include "sctk_context.h"
#include "sctk_tls.h"

#include "mpcomp_task.h"
#include "mpcomp_alloc.h"

/** OpenMP task list data structure */
typedef struct mpcomp_task_list_s
{
   sctk_atomics_int nb_elements;       /**< Number of tasks in the list                      */
   sctk_spinlock_t lock;               /**< Lock of the list                                 */
   struct mpcomp_task_s *head;         /**< First task of the list                           */
   struct mpcomp_task_s *tail;         /**< Last task of the list                            */
   int total;                          /**< Total number of tasks pushed in the list         */
   sctk_atomics_int nb_larcenies;      /**< Number of tasks in the list                      */
} mpcomp_task_list_t;

static inline void mpcomp_task_list_new(struct mpcomp_task_list_s *list)
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
	mpcomp_free( list );
}

static inline int 
mpcomp_task_list_isempty( mpcomp_task_list_t *list )
{
	//return ( sctk_atomics_load_int( &( list->nb_elements ) ) == 0 );
	return ( list->head == NULL );
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
		mpcomp_task_t* tail = list->tail;

	    if( tail->list ) 
		{
			if( tail->prev )
			{
				tail->prev->next = NULL;
			}
		   
			list->tail = tail->prev;
		    sctk_atomics_decr_int( &( list->nb_elements ) );
		    tail->list = NULL;
		    return tail;
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

	sctk_atomics_decr_int( &( list->nb_elements ) );
 	task->list = NULL;

	return 1;
}

static inline void 
mpcomp_task_list_lock( mpcomp_task_list_t* list )
{
	sctk_spinlock_lock( &( list->lock ) );
}

static inline void 
mpcomp_task_list_unlock( mpcomp_task_list_t* list )
{
	sctk_spinlock_unlock( &( list->lock ) );
}

static inline void 
mpcomp_task_list_trylock( mpcomp_task_list_t* list )
{
	sctk_spinlock_trylock( &( list->lock ) );
}

#endif /* !__MPCOMP_TASK_LIST_H__ && MPCOMP_TASK */

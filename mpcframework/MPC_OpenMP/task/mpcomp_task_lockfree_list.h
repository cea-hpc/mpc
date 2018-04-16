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

#include "mpcomp_types_def.h"

#if (!defined(__MPCOMP_TASK_LOCKFREE_LIST_H__) && defined(MPCOMP_TASK))
#define __MPCOMP_TASK_LOCKFREE_LIST_H__

#include "mpcomp.h"
#include "sctk.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"
#include "sctk_context.h"
#include "sctk_tls.h"

#include "mpcomp_task.h"
#include "mpcomp_alloc.h"
#include "sctk_debug.h"

#ifdef MPCOMP_USE_MCS_LOCK
#include "sctk_mcslock.h"
#else /* MPCOMP_USE_MCS_LOCK */
#include "sctk_spinlock.h"
#endif /* MPCOMP_USE_MCS_LOCK */

#define MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING 128

#ifdef MPCOMP_USE_MCS_LOCK
typedef sctk_mcslock_t sctk_mpcomp_task_lock_t;
#else /* MPCOMP_USE_MCS_LOCK */
typedef sctk_spinlock_t sctk_mpcomp_task_lock_t; 
#endif /* MPCOMP_USE_MCS_LOCK */


/** OpenMP task list data structure */
typedef struct mpcomp_task_list_s {
  OPA_ptr_t head; 
  OPA_ptr_t tail; 
  char pad1[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING-2*sizeof(OPA_ptr_t)];	
  OPA_ptr_t shadow_head;
  char pad2[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING-1*sizeof(OPA_ptr_t)];	
  sctk_atomics_int nb_elements;
  sctk_mpcomp_task_lock_t mpcomp_task_lock;	
  sctk_atomics_int nb_larcenies; /**< Number of tasks in the list */
} mpcomp_task_list_t;


static inline void mpcomp_task_list_consummer_lock( mpcomp_task_list_t *list, void* opaque ) 
{
#ifdef MPCOMP_USE_MCS_LOCK
  sctk_mcslock_push_ticket(&( list->mpcomp_task_lock ), (sctk_mcslock_ticket_t *) opaque, SCTK_MCSLOCK_WAIT );
#else /* MPCOMP_USE_MCS_LOCK */
  sctk_spinlock_lock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_list_consummer_unlock( mpcomp_task_list_t *list, void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
  sctk_mcslock_trash_ticket( &(list->mpcomp_task_lock), (sctk_mcslock_ticket_t *) opaque );
#else /* MPCOMP_USE_MCS_LOCK */
  sctk_spinlock_unlock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int mpcomp_task_list_consummer_trylock(mpcomp_task_list_t *list, void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
  not_implemented();
#else /* MPCOMP_USE_MCS_LOCK */
  return sctk_spinlock_trylock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_list_producer_lock(mpcomp_task_list_t *list, void* opaque) {
}

static inline void mpcomp_task_list_producer_unlock(mpcomp_task_list_t *list, void* opaque) {
}

static inline int mpcomp_task_list_producer_trylock(mpcomp_task_list_t *list, void* opaque) {
  return 0;
}
static inline void mpcomp_task_list_reset( mpcomp_task_list_t *list ) {
  memset( list, 0, sizeof( mpcomp_task_list_t ));
  sctk_atomics_store_int(&(list->shadow_head),NULL);
  sctk_atomics_store_int(&(list->tail),NULL);
  sctk_atomics_store_int(&(list->head),NULL);
}

static inline void mpcomp_task_list_free(mpcomp_task_list_t *list) {
  mpcomp_free(list);
}

static inline int mpcomp_task_list_isempty(mpcomp_task_list_t *list) {
  int ret = 0;
  if( ! sctk_atomics_load_ptr( &( list->shadow_head ) ) )
  {
    if( ! sctk_atomics_load_ptr( &( list->head ) ) )
    {
      ret = 1;
    }
    else
    {
      sctk_atomics_store_ptr( &( list->shadow_head ), sctk_atomics_load_ptr( &( list->head ) ));
      sctk_atomics_store_ptr( &( list->head ), NULL ); 
    }
  } 
  return ret;
}

static inline mpcomp_task_t * mpcomp_task_peek_head( mpcomp_task_list_t *list)
{
  sctk_assert( list );

  if( mpcomp_task_list_isempty( list ) )
  {
    return NULL;
  }

  return sctk_atomics_load_ptr( &( list->shadow_head ) );
}



static inline void mpcomp_task_list_pushtohead( mpcomp_task_list_t *list, mpcomp_task_t *task)
{
  sctk_assert( task );
  task->list = list;

  sctk_atomics_store_ptr( &( task->prev ), NULL );
  sctk_atomics_write_barrier();
  void* head = sctk_atomics_load_ptr( &( list->shadow_head ));	

  if(head)
  {
    mpcomp_task_t* next_task = ( mpcomp_task_t*) head;
    sctk_atomics_store_ptr( &( list->shadow_head), task);
    sctk_atomics_store_ptr( &( task->next ), next_task );
    sctk_atomics_store_ptr( &( next_task->prev ), (void*)  task );
  }

  else
  {
    head = sctk_atomics_load_ptr( &( list->head ));
    if(head)
    {
      mpcomp_task_t* next_task = ( mpcomp_task_t*) head;
      sctk_atomics_store_ptr( &( list->head), task);
      sctk_atomics_store_ptr( &( task->next ), next_task );
      sctk_atomics_store_ptr( &( next_task->prev ), (void*)  task );
    }
    else
    {
      sctk_atomics_store_ptr( &( list->head ), (void*)  task );
      sctk_atomics_store_ptr( &( list->tail ), (void*)  task );
    }
  }

}

static inline void mpcomp_task_list_pushtotail( mpcomp_task_list_t *list, mpcomp_task_t *task)
{
  sctk_assert( task );
  task->list = list;
  sctk_atomics_store_ptr( &( task->next ), NULL );

  /* From OpenPA lockfree implementation */
  sctk_atomics_write_barrier();

  void* prev = sctk_atomics_swap_ptr( &( list->tail ), task );

  if( !prev ) 
  {
    sctk_atomics_store_ptr( &( list->head ), (void*)  task );
    sctk_atomics_store_ptr( &( task->prev ), NULL );

  }
  else
  {
    mpcomp_task_t* prev_task = ( mpcomp_task_t*) prev;
    sctk_atomics_store_ptr( &( prev_task->next ), (void*)  task );
    sctk_atomics_store_ptr( &( task->prev), (void*) prev_task);
  }
}
static inline mpcomp_task_t* mpcomp_task_list_popfromhead(mpcomp_task_list_t *list)
{
  mpcomp_task_t *task;

  /* No task to dequeue */

  if( mpcomp_task_list_isempty( list ) )
  {
    return NULL;
  }
  /* Get first task in list */
  void* shadow = sctk_atomics_load_ptr( &( list->shadow_head ) );	
  task = ( mpcomp_task_t* ) shadow;

  if( sctk_atomics_load_ptr( &( task->next ) ))
  {
    sctk_atomics_store_ptr( &( list->shadow_head ), sctk_atomics_load_ptr( &( task->next ) ) );
    mpcomp_task_t* next_task = ( mpcomp_task_t*) sctk_atomics_load_ptr(&(task->next));
    sctk_atomics_store_ptr( &( next_task->prev ), NULL);
  }	
  else
  { 
    sctk_atomics_store_ptr( &( list->shadow_head ) , NULL );
    if( sctk_atomics_cas_ptr( &( list->tail ), shadow, NULL ) != shadow )
    {
      while( !sctk_atomics_load_ptr( &( task->next ) ))
      {
        sctk_cpu_relax();
      }
      sctk_atomics_store_ptr( &( list->shadow_head ), sctk_atomics_load_ptr( &( task->next ))); 
    }
  }
  const int value = sctk_atomics_fetch_and_decr_int(&(list->nb_elements));

  return task;
}

static inline mpcomp_task_t *mpcomp_task_list_popfromtail(mpcomp_task_list_t *list) {
  mpcomp_task_t *task,*shadow;

  if( mpcomp_task_list_isempty( list ) )
  {
    return NULL;
  }
  void* tail = sctk_atomics_load_ptr( &( list->tail ) );	
  shadow = ( mpcomp_task_t* )sctk_atomics_load_ptr( &( list->shadow_head ) );	
  task = (mpcomp_task_t*) tail;
  if( sctk_atomics_load_ptr( &( shadow->next ) ))
  {
    sctk_atomics_store_ptr( &( list->tail ), sctk_atomics_load_ptr( &( task->prev) ) );
    mpcomp_task_t* prev_task = ( mpcomp_task_t*) sctk_atomics_load_ptr(&(task->prev));
    sctk_atomics_store_ptr( &( prev_task->next ), NULL);
  }
  else
  {
    sctk_atomics_store_ptr( &( list->shadow_head), NULL );
    if( sctk_atomics_cas_ptr( &( list->tail ), shadow, NULL ) != shadow )
    {
      while( !sctk_atomics_load_ptr( &( task->next ) ))
      {
        sctk_cpu_relax();
      }
      sctk_atomics_store_ptr( &( list->shadow_head ), sctk_atomics_load_ptr( &( task->next ))); 
    }
  }	

  const int value = sctk_atomics_fetch_and_decr_int(&(list->nb_elements));

  return task;

}


#endif /* !__MPCOMP_TASK_LOCKFREE_LIST_H__ && MPCOMP_TASK */

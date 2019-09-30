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
#include "mpc_common_asm.h"
#include "mpc_common_asm.h"
#include "sctk_context.h"
#include "sctk_tls.h"

#include "mpcomp_task.h"
#include "mpcomp_alloc.h"
#include "sctk_debug.h"
#include "mpcomp_task_locked_list.h"

#ifdef MPCOMP_USE_MCS_LOCK
#include "mpc_common_spinlock.h"
#else /* MPCOMP_USE_MCS_LOCK */
#include "mpc_common_spinlock.h"
#endif /* MPCOMP_USE_MCS_LOCK */

#define MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING 128

#ifdef MPCOMP_USE_MCS_LOCK
typedef sctk_mcslock_t sctk_mpcomp_task_lock_t;
#else /* MPCOMP_USE_MCS_LOCK */
typedef mpc_common_spinlock_t sctk_mpcomp_task_lock_t; 
#endif /* MPCOMP_USE_MCS_LOCK */


/** OpenMP task list data structure */
typedef struct mpcomp_task_lockfree_list_s {
  OPA_ptr_t lockfree_head; 
  OPA_ptr_t lockfree_tail; 
  char pad1[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING-2*sizeof(OPA_ptr_t)];	
  OPA_ptr_t lockfree_shadow_head;
  char pad2[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING-1*sizeof(OPA_ptr_t)];	
  OPA_int_t nb_elements;
  sctk_mpcomp_task_lock_t mpcomp_task_lock;	
  OPA_int_t nb_larcenies; /**< Number of tasks in the list */
} mpcomp_task_lockfree_list_t;


static inline void mpcomp_task_lockfree_list_consummer_lock( mpcomp_task_list_t *list, void* opaque ) 
{
#ifdef MPCOMP_USE_MCS_LOCK
  sctk_mcslock_push_ticket(&( list->mpcomp_task_lock ), (sctk_mcslock_ticket_t *) opaque, SCTK_MCSLOCK_WAIT );
#else /* MPCOMP_USE_MCS_LOCK */
  mpc_common_spinlock_lock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_lockfree_list_consummer_unlock( mpcomp_task_list_t *list, void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
  sctk_mcslock_trash_ticket( &(list->mpcomp_task_lock), (sctk_mcslock_ticket_t *) opaque );
#else /* MPCOMP_USE_MCS_LOCK */
  mpc_common_spinlock_unlock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int mpcomp_task_lockfree_list_consummer_trylock(__UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
  not_implemented();
  return 0;
#else /* MPCOMP_USE_MCS_LOCK */
  return mpc_common_spinlock_trylock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_lockfree_list_producer_lock(__UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void* opaque) {
}

static inline void mpcomp_task_lockfree_list_producer_unlock(__UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void* opaque) {
}

static inline int mpcomp_task_lockfree_list_producer_trylock(__UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void* opaque) {
  return 0;
}

static inline int mpcomp_task_lockfree_list_isempty(mpcomp_task_list_t *list) {
  int ret = 0;
  if( ! OPA_load_ptr( &( list->lockfree_shadow_head ) ) )
  {
    if( ! OPA_load_ptr( &( list->lockfree_head ) ) )
    {
      ret = 1;
    }
    else
    {
      OPA_store_ptr( &( list->lockfree_shadow_head ), OPA_load_ptr( &( list->lockfree_head ) ));
      OPA_store_ptr( &( list->lockfree_head ), NULL ); 
    }
  } 
  return ret;
}

static inline mpcomp_task_t * mpcomp_task_peek_head( mpcomp_task_list_t *list)
{
  sctk_assert( list );

  if( mpcomp_task_lockfree_list_isempty( list ) )
  {
    return NULL;
  }

  return OPA_load_ptr( &( list->lockfree_shadow_head ) );
}



static inline void mpcomp_task_lockfree_list_pushtohead( mpcomp_task_list_t *list, mpcomp_task_t *task)
{
  sctk_assert( task );
  task->list = list;

  OPA_store_ptr( &( task->lockfree_prev ), NULL );
  OPA_write_barrier();
  void* head = OPA_load_ptr( &( list->lockfree_shadow_head ));	

  if(head)
  {
    mpcomp_task_t* next_task = ( mpcomp_task_t*) head;
    OPA_store_ptr( &( list->lockfree_shadow_head), task);
    OPA_store_ptr( &( task->lockfree_next ), next_task );
    OPA_store_ptr( &( next_task->lockfree_prev ), (void*)  task );
  }

  else
  {
    head = OPA_load_ptr( &( list->lockfree_head ));
    if(head)
    {
      mpcomp_task_t* next_task = ( mpcomp_task_t*) head;
      OPA_store_ptr( &( list->lockfree_head), task);
      OPA_store_ptr( &( task->lockfree_next ), next_task );
      OPA_store_ptr( &( next_task->lockfree_prev ), (void*)  task );
    }
    else
    {
      OPA_store_ptr( &( list->lockfree_head ), (void*)  task );
      OPA_store_ptr( &( list->lockfree_tail ), (void*)  task );
    }
  }

}

static inline void mpcomp_task_lockfree_list_pushtotail( mpcomp_task_list_t *list, mpcomp_task_t *task)
{
  sctk_assert( task );
  task->list = list;
  OPA_store_ptr( (OPA_ptr_t*)&( task->next ), NULL );

  /* From OpenPA lockfree implementation */
  OPA_write_barrier();

  void* prev = OPA_swap_ptr( &( list->lockfree_tail ), task );

  if( !prev )
  {
    OPA_store_ptr( (OPA_ptr_t*)&( list->lockfree_head ), (void*)  task );
    OPA_store_ptr( (OPA_ptr_t*)&( task->lockfree_prev ), NULL );

  }
  else
  {
    mpcomp_task_t* prev_task = ( mpcomp_task_t*) prev;
    OPA_store_ptr( (OPA_ptr_t*)&( prev_task->lockfree_next ), (void*)  task );
    OPA_store_ptr( (OPA_ptr_t*)&( task->lockfree_prev), (void*) prev_task);
  }
}

static inline mpcomp_task_t* mpcomp_task_lockfree_list_popfromhead(mpcomp_task_list_t *list)
{
  mpcomp_task_t *task;

  /* No task to dequeue */

  if( mpcomp_task_lockfree_list_isempty( list ) )
  {
    return NULL;
  }
  /* Get first task in list */
  void* shadow = OPA_load_ptr( &( list->lockfree_shadow_head ) );
  task = ( mpcomp_task_t* ) shadow;

  if( OPA_load_ptr( (OPA_ptr_t*)&( task->lockfree_next ) ))
  {
    OPA_store_ptr( (OPA_ptr_t*)&( list->lockfree_shadow_head ), OPA_load_ptr( &( task->lockfree_next ) ) );
    mpcomp_task_t* next_task = ( mpcomp_task_t*) OPA_load_ptr(&(task->lockfree_next));
    OPA_store_ptr( (OPA_ptr_t*)&( next_task->lockfree_prev ), NULL);
  }
  else
  {
    OPA_store_ptr( (OPA_ptr_t*)&( list->lockfree_shadow_head ) , NULL );
    if( OPA_cas_ptr( (OPA_ptr_t*)&( list->lockfree_tail ), shadow, NULL ) != shadow )
    {
      while( !OPA_load_ptr( (OPA_ptr_t*)&( task->lockfree_next ) ))
      {
        sctk_cpu_relax();
      }
      OPA_store_ptr( (OPA_ptr_t*)&( list->lockfree_shadow_head ), OPA_load_ptr( &( task->lockfree_next )));
    }
  }

  return task;
}

static inline mpcomp_task_t *mpcomp_task_lockfree_list_popfromtail(mpcomp_task_list_t *list) {
  mpcomp_task_t *task,*shadow;

  if( mpcomp_task_lockfree_list_isempty( list ) )
  {
    return NULL;
  }
  void* tail = OPA_load_ptr( &( list->lockfree_tail ) );	
  shadow = ( mpcomp_task_t* )OPA_load_ptr( &( list->lockfree_shadow_head ) );	
  task = (mpcomp_task_t*) tail;
  if( OPA_load_ptr( &( shadow->lockfree_next ) ))
  {
    OPA_store_ptr( &( list->lockfree_tail ), OPA_load_ptr( &( task->lockfree_prev) ) );
    mpcomp_task_t* prev_task = ( mpcomp_task_t*) OPA_load_ptr(&(task->lockfree_prev));
    OPA_store_ptr( &( prev_task->lockfree_next ), NULL);
  }
  else
  {
    OPA_store_ptr( (OPA_ptr_t*)&( list->lockfree_shadow_head), NULL );
    if( OPA_cas_ptr( (OPA_ptr_t*)&( list->lockfree_tail ), shadow, NULL ) != shadow )
    {
      while( !OPA_load_ptr( (OPA_ptr_t*)&( task->lockfree_next ) ))
      {
        sctk_cpu_relax();
      }
      OPA_store_ptr( (OPA_ptr_t*)&( list->lockfree_shadow_head ), OPA_load_ptr( &( task->lockfree_next ))); 
    }
  }	


  return task;

}


#endif /* !__MPCOMP_TASK_LOCKFREE_LIST_H__ && MPCOMP_TASK */

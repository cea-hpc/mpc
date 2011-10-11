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
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <sctk_thread.h>
#include <uthash.h>

void
sctk_terminaison_barrier (const int id)
{

  if(sctk_process_number == 1){
    int local;
    int total; 
    static volatile int done = 0;
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    static sctk_thread_cond_t cond = SCTK_THREAD_COND_INITIALIZER;
  
  
    local = sctk_get_nb_task_local(SCTK_COMM_WORLD);
    total = sctk_get_nb_task_total(SCTK_COMM_WORLD);
  
    sctk_thread_mutex_lock(&lock);
    done ++; 
    if(done == local){
      done = 0; 
      sctk_thread_cond_broadcast(&cond);
    } else {
      sctk_thread_cond_wait(&cond,&lock);
    }
    sctk_thread_mutex_unlock(&lock);
  
  } else {
    not_implemented();
  }
}

typedef struct{
  sctk_communicator_t id;

  volatile int done /* = 0 */;
  sctk_thread_mutex_t lock/*  = SCTK_THREAD_MUTEX_INITIALIZER */;
  sctk_thread_cond_t cond/* = SCTK_THREAD_COND_INITIALIZER */;

  UT_hash_handle hh;
} sctk_internal_collectives_t;

static sctk_internal_collectives_t* sctk_collectives_table = NULL;
static sctk_spin_rwlock_t sctk_collectives_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;


/*Init data structures used for task i*/
void sctk_collectives_init (sctk_communicator_t id){
  sctk_internal_collectives_t * tmp;
  tmp = sctk_malloc(sizeof(sctk_internal_collectives_t));
  memset(tmp,0,sizeof(sctk_internal_collectives_t));
  tmp->id = SCTK_COMM_WORLD;

  tmp->done = 0;
  sctk_thread_mutex_init(&(tmp->lock),NULL);
  sctk_thread_cond_init(&(tmp->cond),NULL);

  sctk_spinlock_write_lock(&sctk_collectives_table_lock);
  HASH_ADD(hh,sctk_collectives_table,id,sizeof(sctk_communicator_t),tmp);
  sctk_spinlock_write_unlock(&sctk_collectives_table_lock);
}

void sctk_barrier(const sctk_communicator_t communicator){
  sctk_internal_collectives_t * tmp;
  
  sctk_spinlock_read_lock(&sctk_collectives_table_lock);
  HASH_FIND(hh,sctk_collectives_table,&communicator,sizeof(sctk_communicator_t),tmp);
  sctk_spinlock_read_unlock(&sctk_collectives_table_lock);
  assume(tmp != NULL);

  if(sctk_process_number == 1){
    int local;
    local = sctk_get_nb_task_local(communicator);
    sctk_thread_mutex_lock(&tmp->lock);
    tmp->done ++; 
    if(tmp->done == local){
      tmp->done = 0; 
      sctk_thread_cond_broadcast(&tmp->cond);
    } else {
      sctk_thread_cond_wait(&tmp->cond,&tmp->lock);
    }
    sctk_thread_mutex_unlock(&tmp->lock);
  } else {
    not_implemented();
  } 
}

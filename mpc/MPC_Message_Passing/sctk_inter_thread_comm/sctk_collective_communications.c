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
#include <sctk_pmi.h>

/************************************************************************/
/*Terminaison Barrier                                                   */
/************************************************************************/
void
sctk_terminaison_barrier (const int id)
{
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
    if(sctk_process_number != 1){
      sctk_pmi_barrier();
    }
    sctk_thread_cond_broadcast(&cond);
  } else {
    sctk_thread_cond_wait(&cond,&lock);
  }
  sctk_thread_mutex_unlock(&lock);
}

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/

void sctk_barrier(const sctk_communicator_t communicator){
  sctk_internal_collectives_struct_t * tmp;
  
  tmp = sctk_get_internal_collectives(communicator);
  
  tmp->barrier_func(communicator,tmp);
}

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/
void sctk_broadcast (void *buffer, const size_t size,
		     const int root, const sctk_communicator_t communicator)
{
  sctk_internal_collectives_struct_t * tmp;
  
  tmp = sctk_get_internal_collectives(communicator);
  
  tmp->broadcast_func(buffer,size,root,communicator,tmp);
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
void sctk_all_reduce (const void *buffer_in, void *buffer_out,
		      const size_t elem_size,
		      const size_t elem_number,
		      void (*func) (const void *, void *, size_t,
				    sctk_datatype_t),
		      const sctk_communicator_t communicator,
		      const sctk_datatype_t data_type)
{
  sctk_internal_collectives_struct_t * tmp;
  
  tmp = sctk_get_internal_collectives(communicator);
  
  tmp->allreduce_func(buffer_in,buffer_out,elem_size,
		      elem_number,func,communicator,data_type,tmp);
}

/************************************************************************/
/*INIT                                                                  */
/************************************************************************/

void (*sctk_collectives_init_hook)(sctk_communicator_t id) = sctk_collectives_init_messages;

/*Init data structures used for task i*/
void sctk_collectives_init (sctk_communicator_t id,
			    void (*barrier)(sctk_internal_collectives_struct_t *),
			    void (*broadcast)(sctk_internal_collectives_struct_t *),
			    void (*allreduce)(sctk_internal_collectives_struct_t *)){
  sctk_internal_collectives_struct_t * tmp;
  tmp = sctk_malloc(sizeof(sctk_internal_collectives_struct_t));
  memset(tmp,0,sizeof(sctk_internal_collectives_struct_t));

  barrier(tmp);
  broadcast(tmp);
  allreduce(tmp);
  
  sctk_set_internal_collectives(id,tmp);
}

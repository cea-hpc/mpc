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

#include <sctk_collective_communications.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <sctk_thread.h>

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/
static 
void sctk_barrier_simple(const sctk_communicator_t communicator,
			 sctk_internal_collectives_struct_t * tmp){
  int local;
  local = sctk_get_nb_task_local(communicator);
  sctk_thread_mutex_lock(&tmp->barrier.barrier_simple.lock);
  tmp->barrier.barrier_simple.done ++; 
  if(tmp->barrier.barrier_simple.done == local){
    tmp->barrier.barrier_simple.done = 0; 
    sctk_thread_cond_broadcast(&tmp->barrier.barrier_simple.cond);
  } else {
    sctk_thread_cond_wait(&tmp->barrier.barrier_simple.cond,
			  &tmp->barrier.barrier_simple.lock);
  }
  sctk_thread_mutex_unlock(&tmp->barrier.barrier_simple.lock);
}

void sctk_barrier_simple_init(sctk_internal_collectives_struct_t * tmp){
  if(sctk_process_number == 1){
    tmp->barrier.barrier_simple.done = 0;
    sctk_thread_mutex_init(&(tmp->barrier.barrier_simple.lock),NULL);
    sctk_thread_cond_init(&(tmp->barrier.barrier_simple.cond),NULL);
    tmp->barrier_func = sctk_barrier_simple;    
  } else {
    not_reachable();
  } 
}

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/

void sctk_broadcast_simple (void *buffer, const size_t size,
			    const int root, const sctk_communicator_t com_id,
			    struct sctk_internal_collectives_struct_s *tmp){
  sctk_thread_data_t *thread_data;
  int local;
  int id;

  thread_data = sctk_thread_data_get ();
  local = sctk_get_nb_task_local(com_id);
  id = sctk_get_rank (com_id, thread_data->task_id);

  sctk_thread_mutex_lock(&tmp->broadcast.broadcast_simple.lock);
  {
    if(size > tmp->broadcast.broadcast_simple.size){
      tmp->broadcast.broadcast_simple.buffer = 
	sctk_realloc(tmp->broadcast.broadcast_simple.buffer,size);
      tmp->broadcast.broadcast_simple.size = size;
    }
    tmp->broadcast.broadcast_simple.done ++; 

    if(root == id){
      memcpy(tmp->broadcast.broadcast_simple.buffer,buffer,size);
    }

    if(tmp->broadcast.broadcast_simple.done == local){
      tmp->broadcast.broadcast_simple.done = 0; 
      sctk_thread_cond_broadcast(&tmp->broadcast.broadcast_simple.cond);
    } else {
      sctk_thread_cond_wait(&tmp->broadcast.broadcast_simple.cond,
			    &tmp->broadcast.broadcast_simple.lock);
    }
    if(root != id){
      memcpy(buffer,tmp->broadcast.broadcast_simple.buffer,size);
    }
  }
  sctk_thread_mutex_unlock(&tmp->broadcast.broadcast_simple.lock);
  sctk_barrier_simple(com_id,tmp);
}

void sctk_broadcast_simple_init(struct sctk_internal_collectives_struct_s * tmp){
  if(sctk_process_number == 1){
    tmp->broadcast.broadcast_simple.buffer = NULL;
    tmp->broadcast.broadcast_simple.size = 0;
    tmp->broadcast.broadcast_simple.done = 0;
    sctk_thread_mutex_init(&(tmp->broadcast.broadcast_simple.lock),NULL);
    sctk_thread_cond_init(&(tmp->broadcast.broadcast_simple.cond),NULL);
    tmp->broadcast_func = sctk_broadcast_simple;    
  } else {
    not_reachable();
  } 
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
static void sctk_allreduce_simple (const void *buffer_in, void *buffer_out,
				   const size_t elem_size,
				   const size_t elem_number,
				   void (*func) (const void *, void *, size_t,
						 sctk_datatype_t),
				   const sctk_communicator_t com_id,
				   const sctk_datatype_t data_type,
				   struct sctk_internal_collectives_struct_s *tmp){
  int local;
  size_t size;

  size = elem_size * elem_number;
  local = sctk_get_nb_task_local(com_id);

  sctk_thread_mutex_lock(&tmp->allreduce.allreduce_simple.lock);
  {
    if(size > tmp->allreduce.allreduce_simple.size){
      tmp->allreduce.allreduce_simple.buffer = 
	sctk_realloc(tmp->allreduce.allreduce_simple.buffer,size);
      tmp->allreduce.allreduce_simple.size = size;
    }
    
    if(tmp->allreduce.allreduce_simple.done == 0){
      memcpy(tmp->allreduce.allreduce_simple.buffer,buffer_in,size);
    } else {
      func(buffer_in,tmp->allreduce.allreduce_simple.buffer,elem_number,data_type);
    }

    tmp->allreduce.allreduce_simple.done ++; 
    if(tmp->allreduce.allreduce_simple.done == local){
      tmp->allreduce.allreduce_simple.done = 0;       
      sctk_thread_cond_broadcast(&tmp->allreduce.allreduce_simple.cond);
    } else {
      sctk_thread_cond_wait(&tmp->allreduce.allreduce_simple.cond,
			    &tmp->allreduce.allreduce_simple.lock);
    }
    memcpy(buffer_out,tmp->allreduce.allreduce_simple.buffer,size);
  }
  sctk_thread_mutex_unlock(&tmp->allreduce.allreduce_simple.lock);
  sctk_barrier_simple(com_id,tmp);
}

void sctk_allreduce_simple_init(struct sctk_internal_collectives_struct_s * tmp){
  if(sctk_process_number == 1){
    tmp->allreduce.allreduce_simple.buffer = NULL;
    tmp->allreduce.allreduce_simple.size = 0;
    tmp->allreduce.allreduce_simple.done = 0;
    sctk_thread_mutex_init(&(tmp->allreduce.allreduce_simple.lock),NULL);
    sctk_thread_cond_init(&(tmp->allreduce.allreduce_simple.cond),NULL);
    tmp->allreduce_func = sctk_allreduce_simple;    
  } else {
    not_reachable();
  } 
}

/************************************************************************/
/*Init                                                                  */
/************************************************************************/
void sctk_collectives_init_simple (sctk_communicator_t id){
  if(sctk_process_number == 1){
    sctk_collectives_init(id,
			  sctk_barrier_simple_init,
			  sctk_broadcast_simple_init,
			  sctk_allreduce_simple_init);
  } else {
    not_implemented();
  }
}

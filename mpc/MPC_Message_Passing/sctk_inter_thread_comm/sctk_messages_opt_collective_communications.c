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
#include <string.h>
#include <math.h>

/************************************************************************/
/*PARAMETERS                                                            */
/************************************************************************/
static int BARRIER_ARRITY  = 8;
static int BROADCAST_ARITY_MAX = 8;
static int BROADCAST_MAX_SIZE = 1024;
static int ALLREDUCE_ARITY_MAX = 8;
static int ALLREDUCE_MAX_SIZE = 1024;
static int ALLREDUCE_MAX_NB_ELEM_SIZE = 1024;
#define SCTK_MAX_ASYNC 32

/************************************************************************/
/*TOOLS                                                                 */
/************************************************************************/
static void sctk_free_opt_messages(void* ptr){

}


typedef struct {
  sctk_request_t request;
  sctk_thread_ptp_message_t msg;
}sctk_opt_messages_t;

typedef struct {
  sctk_opt_messages_t msg_req[SCTK_MAX_ASYNC];
  int nb_used;
}sctk_opt_messages_table_t;

static void sctk_opt_messages_send(const sctk_communicator_t communicator,int myself,int dest,int tag, void* buffer,size_t size,
				   specific_message_tag_t specific_message_tag, sctk_opt_messages_t* msg_req){
  
  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_opt_messages,
		   sctk_message_copy); 
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size); 
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator, myself, dest,
			      &(msg_req->request), size,specific_message_tag);
  sctk_send_message (&(msg_req->msg));
}

static void sctk_opt_messages_recv(const sctk_communicator_t communicator,int src, int myself,int tag, void* buffer,size_t size,
				   specific_message_tag_t specific_message_tag, sctk_opt_messages_t* msg_req){
  
  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_opt_messages,
		   sctk_message_copy); 
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size); 
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator,  src,myself,
			      &(msg_req->request), size,specific_message_tag);
  sctk_recv_message (&(msg_req->msg));
}

static void sctk_opt_messages_wait(sctk_opt_messages_table_t* tab){
  int i; 
  for(i = 0; i < tab->nb_used; i++){
    sctk_wait_message (&(tab->msg_req[i].request));
  }
  tab->nb_used = 0;
}



static sctk_opt_messages_t* sctk_opt_messages_get_item(sctk_opt_messages_table_t* tab){
  sctk_opt_messages_t* tmp;
  if(tab->nb_used == SCTK_MAX_ASYNC){
    sctk_opt_messages_wait(tab);
  } 

  tmp = &(tab->msg_req[tab->nb_used]);
  tab->nb_used++;
  return tmp;
}

static void sctk_opt_messages_init_items(sctk_opt_messages_table_t* tab){
    tab->nb_used = 0;
}

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/
static 
void sctk_barrier_opt_messages(const sctk_communicator_t communicator,
			   sctk_internal_collectives_struct_t * tmp){
  sctk_thread_data_t *thread_data;
  int myself;
  int total;
  int total_max;
  int i; 
  sctk_opt_messages_table_t table;
  char c = 'c';

  sctk_opt_messages_init_items(&table);
  
  thread_data = sctk_thread_data_get ();
  total = sctk_get_nb_task_total(communicator);
  myself = sctk_get_rank (communicator, thread_data->task_id);

  total_max = log(total) / log(BARRIER_ARRITY);
  total_max = pow(BARRIER_ARRITY,total_max);
  if(total_max < total){
    total_max = total_max * BARRIER_ARRITY;
  }
  assume(total_max >= total);
  
  for(i = BARRIER_ARRITY; i <= total_max; i = i*BARRIER_ARRITY){
    if(myself % i == 0){
      int src; 
      int j;
      
      src = myself;
      for(j = 1; j < BARRIER_ARRITY; j++){
	if((src + (j*(i/BARRIER_ARRITY))) < total){
	  sctk_opt_messages_recv(communicator,src + (j*(i/BARRIER_ARRITY)),myself,0,&c,1,barrier_specific_message_tag,sctk_opt_messages_get_item(&table));
	}
      }
      sctk_opt_messages_wait(&table);
    } else {
      int dest; 

      dest = (myself / i) * i;
      if(dest >= 0){
	sctk_opt_messages_send(communicator,myself,dest,0,&c,1,barrier_specific_message_tag,sctk_opt_messages_get_item(&table));
	sctk_opt_messages_recv(communicator,dest,myself,1,&c,1,barrier_specific_message_tag,sctk_opt_messages_get_item(&table));
	sctk_opt_messages_wait(&table);
	break;
      }
    }
  }
  sctk_opt_messages_wait(&table);

  for(; i >=BARRIER_ARRITY ; i = i / BARRIER_ARRITY){
    if(myself % i == 0){
      int dest; 
      int j;
      
      dest = myself;
      for(j = 1; j < BARRIER_ARRITY; j++){
	if((dest + (j*(i/BARRIER_ARRITY))) < total){
	  sctk_opt_messages_send(communicator,myself,dest+(j*(i/BARRIER_ARRITY)),1,&c,1,barrier_specific_message_tag,sctk_opt_messages_get_item(&table));
	}    
      }
    }
  }
  sctk_opt_messages_wait(&table);
}

void sctk_barrier_opt_messages_init(sctk_internal_collectives_struct_t * tmp){
  tmp->barrier_func = sctk_barrier_opt_messages;   
}

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/
void sctk_broadcast_opt_messages (void *buffer, const size_t size,
			    const int root, const sctk_communicator_t communicator,
			    struct sctk_internal_collectives_struct_s *tmp){
  sctk_thread_data_t *thread_data;
  int myself;
  int related_myself;
  int total;
  int total_max;
  int i; 
  sctk_opt_messages_table_t table;
  int BROADCAST_ARRITY = 2;

  assume(size > 0);

  sctk_opt_messages_init_items(&table);
  
  BROADCAST_ARRITY = BROADCAST_MAX_SIZE / size;
  if(BROADCAST_ARRITY < 2){
    BROADCAST_ARRITY = 2;
  }
  if(BROADCAST_ARRITY > BROADCAST_ARITY_MAX){
    BROADCAST_ARRITY = BROADCAST_ARITY_MAX;
  }

  thread_data = sctk_thread_data_get ();
  total = sctk_get_nb_task_total(communicator);
  myself = sctk_get_rank (communicator, thread_data->task_id);
  related_myself = (myself + total - root) % total; 

  total_max = log(total) / log(BROADCAST_ARRITY);
  total_max = pow(BROADCAST_ARRITY,total_max);
  if(total_max < total){
    total_max = total_max * BROADCAST_ARRITY;
  }
  assume(total_max >= total);

  for(i = BROADCAST_ARRITY; i <= total_max; i = i*BROADCAST_ARRITY){
    if(related_myself % i != 0){
      int dest; 

      dest = (related_myself/i) * i;
      if(dest >= 0){
	sctk_opt_messages_recv(communicator,(dest+root)%total,myself,root,buffer,size,broadcast_specific_message_tag,sctk_opt_messages_get_item(&table));
	sctk_opt_messages_wait(&table);
	break;
      }
    }
  }

  for(; i >=BROADCAST_ARRITY ; i = i / BROADCAST_ARRITY){
    if(related_myself % i == 0){
      int dest; 
      int j; 
      
      dest = related_myself;
      for(j = 1; j < BROADCAST_ARRITY; j++){
	if((dest + (j*(i/BROADCAST_ARRITY)))< total){
	  sctk_opt_messages_send(communicator,myself,(dest+root+(j*(i/BROADCAST_ARRITY))) % total,root,buffer,size,broadcast_specific_message_tag,sctk_opt_messages_get_item(&table));
	}
      }    
    }
  } 
  sctk_opt_messages_wait(&table); 
}

void sctk_broadcast_opt_messages_init(struct sctk_internal_collectives_struct_s * tmp){
    tmp->broadcast_func = sctk_broadcast_opt_messages;    
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
static void sctk_allreduce_opt_messages_intern (const void *buffer_in, void *buffer_out,
				   const size_t elem_size,
				   const size_t elem_number,
				   void (*func) (const void *, void *, size_t,
						 sctk_datatype_t),
				   const sctk_communicator_t communicator,
				   const sctk_datatype_t data_type,
				   struct sctk_internal_collectives_struct_s *tmp){
  sctk_thread_data_t *thread_data;
  int myself;
  int total;
  size_t size;
  int dest;
  int src;
  int i;
  void* buffer_tmp;
  void** buffer_table;
  sctk_opt_messages_table_t table;
  int ALLREDUCE_ARRITY = 2;
  int total_max;
  
  /*
    MPI require that the result of the allreduce is the same on all MPI tasks.
    This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating 
    point datatypes.
  */
  sctk_opt_messages_init_items(&table);
  
  size = elem_size * elem_number;

  ALLREDUCE_ARRITY = ALLREDUCE_MAX_SIZE / size;
  if(ALLREDUCE_ARRITY < 2){
    ALLREDUCE_ARRITY = 2;
  }
  if(ALLREDUCE_ARRITY > ALLREDUCE_ARITY_MAX){
    ALLREDUCE_ARRITY = ALLREDUCE_ARITY_MAX;
  }

  buffer_tmp = sctk_malloc(size*(ALLREDUCE_ARRITY -1));
  buffer_table = sctk_malloc((ALLREDUCE_ARRITY -1) * sizeof(void*));
  {
    int j;
    for(j = 1; j < ALLREDUCE_ARRITY; j++){
      buffer_table[j-1] = ((char*)buffer_tmp) + (size * (j-1));
    }
  }
  
  memcpy(buffer_out,buffer_in,size);

  assume(size > 0);

  thread_data = sctk_thread_data_get ();
  total = sctk_get_nb_task_total(communicator);
  myself = sctk_get_rank (communicator, thread_data->task_id);
  
  total_max = log(total) / log(ALLREDUCE_ARRITY);
  total_max = pow(ALLREDUCE_ARRITY,total_max);
  if(total_max < total){
    total_max = total_max * ALLREDUCE_ARRITY;
  }
  assume(total_max >= total);

  for(i = ALLREDUCE_ARRITY; i <= total_max; i = i*ALLREDUCE_ARRITY){
    if(myself % i == 0){
      int src; 
      int j;
      
      src = myself;
      for(j = 1; j < ALLREDUCE_ARRITY; j++){
	if((src + (j*(i/ALLREDUCE_ARRITY))) < total){
	  sctk_opt_messages_recv(communicator,src + (j*(i/ALLREDUCE_ARRITY)),myself,0,buffer_table[j-1],size,allreduce_specific_message_tag,
				 sctk_opt_messages_get_item(&table));
	}
      }
      sctk_opt_messages_wait(&table);
      for(j = 1; j < ALLREDUCE_ARRITY; j++){
	if((src + (j*(i/ALLREDUCE_ARRITY))) < total){
	  func(buffer_table[j-1],buffer_out,elem_number,data_type);	  
	}
      }
    } else {
      int dest; 

      dest = (myself / i) * i;
      if(dest >= 0){
	sctk_opt_messages_send(communicator,myself,dest,0,buffer_out,size,allreduce_specific_message_tag,sctk_opt_messages_get_item(&table));
	sctk_opt_messages_wait(&table);
	sctk_opt_messages_recv(communicator,dest,myself,1,buffer_out,size,allreduce_specific_message_tag,sctk_opt_messages_get_item(&table));
	sctk_opt_messages_wait(&table);
	break;
      }
    }
  }
  sctk_opt_messages_wait(&table);

  for(; i >=ALLREDUCE_ARRITY ; i = i / ALLREDUCE_ARRITY){
    if(myself % i == 0){
      int dest; 
      int j;
      
      dest = myself;
      for(j = 1; j < ALLREDUCE_ARRITY; j++){
	if((dest + (j*(i/ALLREDUCE_ARRITY))) < total){
	  sctk_opt_messages_send(communicator,myself,dest+(j*(i/ALLREDUCE_ARRITY)),1,buffer_out,size,allreduce_specific_message_tag,sctk_opt_messages_get_item(&table));
	}    
      }
    }
  }
  sctk_opt_messages_wait(&table);
  sctk_free(buffer_tmp);
  sctk_free(buffer_table);
}

static void sctk_allreduce_opt_messages (const void *buffer_in, void *buffer_out,
				   const size_t elem_size,
				   const size_t elem_number,
				   void (*func) (const void *, void *, size_t,
						 sctk_datatype_t),
				   const sctk_communicator_t communicator,
				   const sctk_datatype_t data_type,
				   struct sctk_internal_collectives_struct_s *tmp){
#warning "Add buffer splitting"
  sctk_allreduce_opt_messages_intern(buffer_in,buffer_out,elem_size,elem_number,func,communicator,data_type,tmp);
}
void sctk_allreduce_opt_messages_init(struct sctk_internal_collectives_struct_s * tmp){
    tmp->allreduce_func = sctk_allreduce_opt_messages;    
}

/************************************************************************/
/*Init                                                                  */
/************************************************************************/
void sctk_collectives_init_opt_messages (sctk_communicator_t id){
  sctk_collectives_init(id,
			sctk_barrier_opt_messages_init,
			sctk_broadcast_opt_messages_init,
			sctk_allreduce_opt_messages_init);
  if(sctk_process_rank == 0){
    sctk_warning("Use low performances collectives");
  }
}

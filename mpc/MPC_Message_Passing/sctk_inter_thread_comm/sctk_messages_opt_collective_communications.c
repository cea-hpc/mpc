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

/************************************************************************/
/*TOOLS                                                                 */
/************************************************************************/
static void sctk_free_opt_messages(void* ptr){

}

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/
static void sctk_barrier_opt_messages_send(const sctk_communicator_t communicator,int myself,int dest,int tag){
  char buffer = 'l';
  int size = 1;
  sctk_thread_ptp_message_t send_msg;
  sctk_request_t send_request;
  
  sctk_init_header(&send_msg,myself,sctk_message_contiguous,sctk_free_opt_messages,
		   sctk_message_copy); 
  sctk_add_adress_in_message(&send_msg,&buffer,size); 
  sctk_set_header_in_message (&send_msg, tag, communicator, myself, dest,
			      &send_request, size,barrier_specific_message_tag);
  sctk_send_message (&send_msg);
  sctk_wait_message (&send_request); 
}

static void sctk_barrier_opt_messages_recv(const sctk_communicator_t communicator,int src, int myself,int tag){
  char buffer = 'l';
  int size = 1;
  sctk_thread_ptp_message_t recv_msg;
  sctk_request_t recv_request;
  
  sctk_init_header(&recv_msg,myself,sctk_message_contiguous,sctk_free_opt_messages,
		   sctk_message_copy); 
  sctk_add_adress_in_message(&recv_msg,&buffer,size); 
  sctk_set_header_in_message (&recv_msg, tag, communicator,  src,myself,
			      &recv_request, size,barrier_specific_message_tag);
  sctk_recv_message (&recv_msg);
  sctk_wait_message (&recv_request); 
}

static 
void sctk_barrier_opt_messages(const sctk_communicator_t communicator,
			   sctk_internal_collectives_struct_t * tmp){
  sctk_thread_data_t *thread_data;
  int myself;
  int total;
  int total_max;
  int i; 
  
  thread_data = sctk_thread_data_get ();
  total = sctk_get_nb_task_total(communicator);
  myself = sctk_get_rank (communicator, thread_data->task_id);

  total_max = (total / 2) * 2;
  if(total_max < total){
    total_max = total_max * 2;
  }
  
  for(i = 2; i <= total_max; i = i*2){
    if(myself % i == 0){
      int src; 
      
      src = myself + (i/2);
      if(src < total){
	sctk_barrier_opt_messages_recv(communicator,src,myself,0);
      }
    } else {
      int dest; 

      dest = myself - (i/2);
      if(dest >= 0){
	sctk_barrier_opt_messages_send(communicator,myself,dest,0);
	sctk_barrier_opt_messages_recv(communicator,dest,myself,1);
	break;
      }
    }
  }

  for(; i >=2 ; i = i / 2){
    if(myself % i == 0){
      int dest; 
      
      dest = myself + (i/2);
      if(dest < total){
	sctk_barrier_opt_messages_send(communicator,myself,dest,1);
      }    
    }
  }
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
  int total;
  
  thread_data = sctk_thread_data_get ();
  total = sctk_get_nb_task_total(communicator);
  myself = sctk_get_rank (communicator, thread_data->task_id);

  if(myself == root){
    int i; 
    for(i = 0; i < total; i++){
      if(i != root){
	sctk_thread_ptp_message_t send_msg;
	sctk_request_t send_request;
	
	sctk_init_header(&send_msg,myself,sctk_message_contiguous,sctk_free_opt_messages,
			 sctk_message_copy); 
	sctk_add_adress_in_message(&send_msg,buffer,size); 
	sctk_set_header_in_message (&send_msg, root, communicator, root, i,
				    &send_request, size,broadcast_specific_message_tag);
	sctk_send_message (&send_msg);
	sctk_wait_message (&send_request);  
      }
    }
  } else {  
    sctk_thread_ptp_message_t recv_msg;
    sctk_request_t recv_request;

    sctk_init_header(&recv_msg,myself,sctk_message_contiguous,sctk_free_opt_messages,
		     sctk_message_copy);
    sctk_add_adress_in_message(&recv_msg,buffer,size);
    sctk_set_header_in_message (&recv_msg, root, communicator, root, myself,
				&recv_request, size,broadcast_specific_message_tag);
    sctk_recv_message (&recv_msg);
    sctk_wait_message (&recv_request);
  }
}

void sctk_broadcast_opt_messages_init(struct sctk_internal_collectives_struct_s * tmp){
    tmp->broadcast_func = sctk_broadcast_opt_messages;    
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
static void sctk_allreduce_opt_messages (const void *buffer_in, void *buffer_out,
				   const size_t elem_size,
				   const size_t elem_number,
				   void (*func) (const void *, void *, size_t,
						 sctk_datatype_t),
				   const sctk_communicator_t communicator,
				   const sctk_datatype_t data_type,
				   struct sctk_internal_collectives_struct_s *tmp){
  sctk_thread_ptp_message_t send_msg;
  sctk_request_t send_request;
  sctk_thread_ptp_message_t recv_msg;
  sctk_request_t recv_request;
  sctk_thread_data_t *thread_data;
  int myself;
  int total;
  size_t size;
  int dest;
  int src;
  int i;
  void* buffer_tmp;
  int root = 0;
  
  /*
    MPI require that the result of the allreduce is the same on all MPI tasks.
    This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating 
    point datatypes.
  */
  size = elem_size * elem_number;
  buffer_tmp = sctk_malloc(size);
  
  thread_data = sctk_thread_data_get ();
  total = sctk_get_nb_task_total(communicator);
  myself = sctk_get_rank (communicator, thread_data->task_id);

  memcpy(buffer_out,buffer_in,size);
  if(myself == root){
    sctk_thread_ptp_message_t recv_msg;
    sctk_request_t recv_request;

    for(i = 1; i < total; i++){
      sctk_init_header(&recv_msg,myself,sctk_message_contiguous,sctk_free_opt_messages,
		       sctk_message_copy);
      sctk_add_adress_in_message(&recv_msg,buffer_tmp,size);
      sctk_set_header_in_message (&recv_msg, root, communicator, i, root,
				  &recv_request, size,allreduce_specific_message_tag);
      sctk_recv_message (&recv_msg);
      sctk_wait_message (&recv_request);
      
      func(buffer_tmp,buffer_out,elem_number,data_type);
    }
  } else {
    sctk_thread_ptp_message_t send_msg;
    sctk_request_t send_request;
    
    sctk_init_header(&send_msg,myself,sctk_message_contiguous,sctk_free_opt_messages,
		     sctk_message_copy); 
    sctk_add_adress_in_message(&send_msg,buffer_out,size); 
    sctk_set_header_in_message (&send_msg, root, communicator,myself,root,
				&send_request, size,allreduce_specific_message_tag);
    sctk_send_message (&send_msg);
    sctk_wait_message (&send_request);      
  }

  sctk_free(buffer_tmp);
  sctk_broadcast_opt_messages(buffer_out,size,root,communicator,tmp);
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

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

#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_reorder.h>
#include <sctk_communicator.h>
#include <sctk_spinlock.h>

static sctk_route_table_t* sctk_dynamic_route_table = NULL;
static sctk_route_table_t* sctk_static_route_table = NULL;
static sctk_spin_rwlock_t sctk_route_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static sctk_rail_info_t* rails = NULL;
static sctk_spin_rwlock_t sctk_route_table_init_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static int sctk_route_table_init_lock_needed = 0;

#define TABLE_LOCK() if(sctk_route_table_init_lock_needed) sctk_spinlock_write_lock(&sctk_route_table_init_lock);
#define TABLE_UNLOCK() if(sctk_route_table_init_lock_needed) sctk_spinlock_write_unlock(&sctk_route_table_init_lock);


void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;

  sctk_add_dynamic_reorder_buffer(dest);

  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_ADD(hh,sctk_dynamic_route_table,key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
  
}

void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;

  sctk_add_static_reorder_buffer(dest);
  TABLE_LOCK();
  HASH_ADD(hh,sctk_static_route_table,key,sizeof(sctk_route_key_t),tmp); 
  TABLE_UNLOCK(); 
}

static inline 
sctk_route_table_t* sctk_get_route_to_process_no_route(int dest, sctk_rail_info_t* rail){
  sctk_route_key_t key;
  sctk_route_table_t* tmp;

  key.destination = dest; 
  key.rail = rail->rail_number;

  TABLE_LOCK();  
  HASH_FIND(hh,sctk_static_route_table,&key,sizeof(sctk_route_key_t),tmp);
  TABLE_UNLOCK(); 
  if(tmp == NULL){
    sctk_spinlock_read_lock(&sctk_route_table_lock);
    HASH_FIND(hh,sctk_dynamic_route_table,&key,sizeof(sctk_route_key_t),tmp);
    sctk_spinlock_read_lock(&sctk_route_table_lock);
  }

  return tmp;
}

sctk_route_table_t* sctk_get_route_to_process(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  tmp = sctk_get_route_to_process_no_route(dest,rail);

  if(tmp == NULL){
    dest = rail->route(dest,rail);
    return sctk_get_route_to_process(dest,rail);
  }

  return tmp;
}

sctk_route_table_t* sctk_get_route(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  int process;

  process = sctk_get_process_rank_from_task_rank(dest);

  tmp = sctk_get_route_to_process(process,rail);

  return tmp;
}

void sctk_route_set_rail_nb(int i){
  rails = sctk_malloc(i*sizeof(sctk_rail_info_t));
}

sctk_rail_info_t* sctk_route_get_rail(int i){
  return &(rails[i]);
}

/**** routes *****/

static void sctk_free_route_messages(void* ptr){

}

typedef struct {
  sctk_request_t request;
  sctk_thread_ptp_message_t msg;
}sctk_route_messages_t;

void sctk_route_messages_send(int myself,int dest,int tag, void* buffer,size_t size){
  sctk_communicator_t communicator = SCTK_COMM_WORLD;
  specific_message_tag_t specific_message_tag = process_specific_message_tag;
  sctk_route_messages_t msg;
  sctk_route_messages_t* msg_req;

  msg_req = &msg;

  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_route_messages,
		   sctk_message_copy); 
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size); 
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator, myself, dest,
			      &(msg_req->request), size,specific_message_tag);
  sctk_send_message (&(msg_req->msg));
  sctk_wait_message (&(msg_req->request));
}

void sctk_route_messages_recv(int src, int myself,int tag, void* buffer,size_t size){
  sctk_communicator_t communicator = SCTK_COMM_WORLD;
  specific_message_tag_t specific_message_tag = process_specific_message_tag;
  sctk_route_messages_t msg;
  sctk_route_messages_t* msg_req;
  
  msg_req = &msg;

  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_route_messages,
		   sctk_message_copy); 
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size); 
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator,  src,myself,
			      &(msg_req->request), size,specific_message_tag);
  sctk_recv_message (&(msg_req->msg));
  sctk_wait_message (&(msg_req->request));
}

void sctk_route_ring_init(sctk_rail_info_t* rail){

}

int sctk_route_ring(int dest, sctk_rail_info_t* rail){
  int old_dest;
  
  old_dest = dest;
  dest = (dest + sctk_process_number -1) % sctk_process_number;
  sctk_nodebug("Route via dest - 1 %d to %d",dest,old_dest);
  
  return dest;
}

int sctk_route_tree(int dest, sctk_rail_info_t* rail){
  not_implemented();
}

void sctk_route_tree_init(sctk_rail_info_t* rail){
  if(sctk_process_number > 3){
    not_implemented();
  }
}

int sctk_route_fully(int dest, sctk_rail_info_t* rail){
  not_reachable();
}

void sctk_route_fully_init(sctk_rail_info_t* rail){
  int (*sav_sctk_route)(int , sctk_rail_info_t* );

  sctk_pmi_barrier();   
  sctk_route_table_init_lock_needed = 1;
  sctk_pmi_barrier();   
  sav_sctk_route = rail->route;
  rail->route = sctk_route_ring;
  if(sctk_process_number > 3){
    int from; 
    int to;
    for(from = 0; from < sctk_process_number; from++){
      for(to = 0; to < sctk_process_number; to ++){
	if(to != from){
	  sctk_route_table_t* tmp;	
	  if(from == sctk_process_rank){
	    tmp = sctk_get_route_to_process_no_route(to,rail);
	    if(tmp == NULL){
	      rail->connect_from(from,to,rail);
	    } 
	  } 
	  if(to == sctk_process_rank){
	    tmp = sctk_get_route_to_process_no_route(from,rail);
	    if(tmp == NULL){
	      rail->connect_to(from,to,rail);
	    }
	  }
	} 
      }
      sctk_pmi_barrier();
    }
  }
  rail->route = sav_sctk_route;
  sctk_pmi_barrier();
  sctk_route_table_init_lock_needed = 0;
  sctk_pmi_barrier();   
}

void sctk_route_init_in_rail(sctk_rail_info_t* rail, char* topology){
  if(strcmp("ring",topology) == 0){
    rail->route = sctk_route_ring;
    rail->route_init = sctk_route_ring_init;
    rail->topology_name = "ring";
  } else if(strcmp("tree",topology) == 0){
    rail->route = sctk_route_tree;
    rail->route_init = sctk_route_tree_init;
    rail->topology_name = "tree";
  } else if(strcmp("fully",topology) == 0){
    rail->route = sctk_route_fully;
    rail->route_init = sctk_route_fully_init;
    rail->topology_name = "fully connected";
  } else {
    not_reachable();
  }
}


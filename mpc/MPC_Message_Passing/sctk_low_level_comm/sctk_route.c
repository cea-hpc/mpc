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
#include <sctk_ib_cm.h>
#include <sctk_pmi.h>

static sctk_route_table_t* sctk_dynamic_route_table = NULL;
static sctk_route_table_t* sctk_static_route_table = NULL;
static sctk_spin_rwlock_t sctk_route_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static sctk_rail_info_t* rails = NULL;
static int rail_number = 0;
static sctk_spin_rwlock_t sctk_route_table_init_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static int sctk_route_table_init_lock_needed = 0;
static int is_route_finalized = 0;

#define TABLE_LOCK() if(sctk_route_table_init_lock_needed) sctk_spinlock_write_lock(&sctk_route_table_init_lock);
#define TABLE_UNLOCK() if(sctk_route_table_init_lock_needed) sctk_spinlock_write_unlock(&sctk_route_table_init_lock);

/* For ondemand connexions */
void sctk_route_set_connected(sctk_route_table_t* tmp, int connected){
  OPA_store_int(&tmp->connected, connected);
}
int sctk_route_is_connected(sctk_route_table_t* tmp){
  return (int) OPA_load_int(&tmp->connected);
}

/* For low memory mode */
int sctk_route_is_low_memory_mode_local(sctk_route_table_t* tmp) {
  return (int) OPA_load_int(&tmp->low_memory_mode_local);
}
void sctk_route_set_low_memory_mode_local(sctk_route_table_t* tmp, int low) {
  if (low) sctk_warning("Local process %d set to low level memory", tmp->key.destination);
  OPA_store_int(&tmp->low_memory_mode_local, low);
}
int sctk_route_is_low_memory_mode_remote(sctk_route_table_t* tmp) {
  return (int) OPA_load_int(&tmp->low_memory_mode_remote);
}
void sctk_route_set_low_memory_mode_remote(sctk_route_table_t* tmp, int low) {
  if (low) sctk_warning("Remote process %d set to low level memory", tmp->key.destination);
  OPA_store_int(&tmp->low_memory_mode_remote, low);
}


sctk_route_table_t *sctk_route_dynamic_safe_add(int dest, sctk_rail_info_t* rail, sctk_route_table_t* (*func)(int dest, sctk_rail_info_t* rail), int *added) {
  sctk_route_key_t key;
  sctk_route_table_t* tmp;
  *added = 0;

  key.destination = dest;
  key.rail = rail->rail_number;

  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_FIND(hh,sctk_dynamic_route_table,&key,sizeof(sctk_route_key_t),tmp);
  if (tmp == NULL) {
    tmp = func(dest, rail);
    tmp->key.destination = dest;
    tmp->key.rail = rail->rail_number;
    tmp->rail = rail;
    sctk_route_set_connected(tmp, 0);
    sctk_route_set_low_memory_mode_local(tmp, 0);
    sctk_route_set_low_memory_mode_remote(tmp, 0);
    HASH_ADD(hh,sctk_dynamic_route_table,key,sizeof(sctk_route_key_t),tmp);
    *added = 1;
  }
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
  return tmp;
}

sctk_route_table_t *sctk_route_dynamic_search(int dest, sctk_rail_info_t* rail){
  sctk_route_key_t key;
  sctk_route_table_t* tmp;

  key.destination = dest;
  key.rail = rail->rail_number;
  sctk_spinlock_read_lock(&sctk_route_table_lock);
  HASH_FIND(hh,sctk_dynamic_route_table,&key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_read_unlock(&sctk_route_table_lock);
  return tmp;
}

void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;

  /* XXX: For debug only. Assume that entry not already added */
  assume (sctk_route_dynamic_search(dest, rail) == NULL);

  sctk_add_dynamic_reorder_buffer(dest);

  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_ADD(hh,sctk_dynamic_route_table,key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
}

void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;
  sctk_route_set_connected(tmp, 1);
  sctk_route_set_low_memory_mode_local(tmp, 0);
  sctk_route_set_low_memory_mode_remote(tmp, 0);

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
    sctk_spinlock_read_unlock(&sctk_route_table_lock);
    /* If the route is deconnected, we do not use it*/
    if (tmp && !sctk_route_is_connected(tmp)) {
      tmp = NULL;
    }
  }
  return tmp;
}

struct wait_connexion_args_s {
  sctk_route_table_t* route_table;
  sctk_rail_info_t* rail;
  int done;
};

void* __wait_connexion(void* a) {
  struct wait_connexion_args_s *args = (struct wait_connexion_args_s*) a;

  if (sctk_route_is_connected(args->route_table)) {
    args->done = 1;
  } else {
    sctk_network_poll_all(args->rail);
  }
  return NULL;
}


inline sctk_route_table_t* sctk_get_route_to_process_no_ondemand(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  tmp = sctk_get_route_to_process_no_route(dest,rail);

  if(tmp == NULL){
    dest = rail->route(dest,rail);
    return sctk_get_route_to_process_no_ondemand(dest,rail);
  }

  return tmp;
}

sctk_route_table_t* sctk_get_route_to_process(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  tmp = sctk_get_route_to_process_no_route(dest,rail);

  if(tmp == NULL){
#if MPC_USE_INFINIBAND
    if (rail->on_demand) {
      sctk_nodebug("%d Trying to connect to process %d (remote:%p)", sctk_process_rank, dest, tmp);
      tmp = sctk_ib_cm_on_demand_request(dest,rail);
      assume(tmp);
      /* If route not connected, so we wait for until it is connected */
      if (sctk_route_is_connected(tmp) == 0) {
        struct wait_connexion_args_s args;
        args.route_table = tmp;
        args.done = 0;
        args.rail = rail;
        sctk_thread_wait_for_value_and_poll((int*) &args.done, 1,
            (void (*)(void*)) __wait_connexion, &args);
        assume(sctk_route_is_connected(tmp));
      }

      sctk_nodebug("Connected to process %d", dest);
      return tmp;
    } else {
#endif
      dest = rail->route(dest,rail);
      return sctk_get_route_to_process_no_ondemand(dest,rail);
#if MPC_USE_INFINIBAND
    }
#endif
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
  rail_number = i;
}

sctk_rail_info_t* sctk_route_get_rail(int i){
  return &(rails[i]);
}

int sctk_route_is_finalized() {
  return is_route_finalized;
}

void sctk_route_finalize(){
  char* net_name;
  int i;
  char* name_ptr;

  net_name = sctk_malloc(rail_number*4096);
  name_ptr = net_name;
  for(i = 0; i < rail_number; i++){
    rails[i].route_init(&(rails[i]));
    sprintf(name_ptr,"[%d:%s (%s)]",i,rails[i].network_name,rails[i].topology_name);
    name_ptr = net_name + strlen(net_name);
    sctk_pmi_barrier();
  }
  sctk_network_mode = net_name;
  is_route_finalized = 1;
}

/**** routes *****/

static void sctk_free_route_messages(void* ptr){

}

typedef struct {
  sctk_request_t request;
  sctk_thread_ptp_message_t msg;
}sctk_route_messages_t;

void sctk_route_messages_send(int myself,int dest, specific_message_tag_t specific_message_tag, int tag, void* buffer,size_t size){
  sctk_communicator_t communicator = SCTK_COMM_WORLD;
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

void sctk_route_messages_recv(int src, int myself,specific_message_tag_t specific_message_tag, int tag, void* buffer,size_t size){
  sctk_communicator_t communicator = SCTK_COMM_WORLD;
  sctk_route_messages_t msg;
  sctk_route_messages_t* msg_req;

  msg_req = &msg;

  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_route_messages,
		   sctk_message_copy);
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size);
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator,  src,myself,
			      &(msg_req->request), size,specific_message_tag);
  sctk_recv_message (&(msg_req->msg),NULL);
  sctk_wait_message (&(msg_req->request));
}

/* RING CONNECTED */
void sctk_route_ring_init(sctk_rail_info_t* rail){

}

int sctk_route_ring(int dest, sctk_rail_info_t* rail){
  int old_dest;

  old_dest = dest;
  dest = (dest + sctk_process_number -1) % sctk_process_number;
  sctk_nodebug("Route via dest - 1 %d to %d",dest,old_dest);

  return dest;
}

/* FULLY CONNECTED */
int sctk_route_fully(int dest, sctk_rail_info_t* rail){
  not_reachable();
  return -1;
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
    }
    sctk_pmi_barrier();
  }
  rail->route = sav_sctk_route;
  sctk_pmi_barrier();
  sctk_route_table_init_lock_needed = 0;
  sctk_pmi_barrier();
}

/* ONDEMAND */
int sctk_route_ondemand(int dest, sctk_rail_info_t* rail){
  not_reachable();
  return -1;
}

void sctk_route_ondemand_init(sctk_rail_info_t* rail){
  rail->route = sctk_route_ring;
  rail->on_demand=1;
}

void sctk_route_init_in_rail(sctk_rail_info_t* rail, char* topology){
  rail->on_demand = 0;
  if(strcmp("ring",topology) == 0){
    rail->route = sctk_route_ring;
    rail->route_init = sctk_route_ring_init;
    rail->topology_name = "ring";
  } else if(strcmp("fully",topology) == 0){
    rail->route = sctk_route_fully;
    rail->route_init = sctk_route_fully_init;
    rail->topology_name = "fully connected";
  } else if(strcmp("ondemand",topology) == 0){
    rail->route = sctk_route_ondemand;
    rail->route_init = sctk_route_ondemand_init;
    rail->topology_name = "OD connexion";
  } else {
    not_reachable();
  }
}


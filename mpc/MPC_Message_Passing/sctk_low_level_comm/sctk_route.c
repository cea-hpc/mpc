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
#include <sctk_low_level_comm.h>
#include <sctk_pmi.h>
#include "sctk_ib_qp.h"
#include <utarray.h>

static int size=MIN_SIZE_DIM;
static sctk_Torus_t Torus;
//static sctk_Torus_route route;
static sctk_Node_t node;

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

int sctk_route_cas_low_memory_mode_local(sctk_route_table_t* tmp, int oldv, int newv) {
  return (int) OPA_cas_int(&tmp->low_memory_mode_local, oldv, newv);
}

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

/*
 * Return the route entry of the process 'dest'.
 * If the entry is not found, it is created using the 'create_func' function and
 * initialized using the 'init_funct' function.
 *
 * Return:
 *  - added: if the entry has been created
 *  - is_initiator: if the current process is the initiator of the entry creation.
 */
sctk_route_table_t *sctk_route_dynamic_safe_add(int dest, sctk_rail_info_t* rail, sctk_route_table_t* (*create_func)(), void (*init_func)(int dest, sctk_rail_info_t* rail, sctk_route_table_t *route_table, int ondemand), int *added, char is_initiator) {
  sctk_route_key_t key;
  sctk_route_table_t* tmp;
  *added = 0;

  key.destination = dest;
  key.rail = rail->rail_number;

  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_FIND(hh,sctk_dynamic_route_table,&key,sizeof(sctk_route_key_t),tmp);
  /* Entry not found, we create it */
  if (tmp == NULL) {
    /* QP added on demand */
    tmp = create_func();
    init_func(dest, rail, tmp, 1);
    /* We init the entry and add it to the table */
    sctk_init_dynamic_route(dest, tmp, rail);
    tmp->is_initiator = is_initiator;
    HASH_ADD(hh,sctk_dynamic_route_table,key,sizeof(sctk_route_key_t),tmp);
    sctk_nodebug("Entry created for %d", dest);
    *added = 1;
  } else if (sctk_route_get_state(tmp) == state_reset) { /* QP in a reset state */
    /* If the remote is in a reset state, we can reinit all the fields
     * and we set added to 1 */
    /* TODO: Reinit the structures */
    ROUTE_LOCK(tmp);
    sctk_route_set_state(tmp, state_reconnecting);
    init_func(dest, rail, tmp, 1);
    sctk_route_set_low_memory_mode_local(tmp, 0);
    sctk_route_set_low_memory_mode_remote(tmp, 0);
    ROUTE_UNLOCK(tmp);
    *added = 1;
  }
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
  return tmp;
}

/* Return if the process is the initiator of the connexion or not */
char sctk_route_get_is_initiator(sctk_route_table_t * route_table) {
  assume(route_table);
  int is_initiator;

  ROUTE_LOCK(route_table);
  is_initiator = route_table->is_initiator;
  ROUTE_UNLOCK(route_table);

  return is_initiator;
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

/*-----------------------------------------------------------
 *  Initialize and add dynamic routes
 *----------------------------------------------------------*/
void sctk_init_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;

  /* sctk_assert (sctk_route_dynamic_search(dest, rail) == NULL); */
  sctk_route_set_low_memory_mode_local(tmp, 0);
  sctk_route_set_low_memory_mode_remote(tmp, 0);
  sctk_route_set_state(tmp, state_deconnected);

  tmp->is_initiator = CHAR_MAX;
  tmp->lock = SCTK_SPINLOCK_INITIALIZER;

  tmp->origin = route_origin_dynamic;
  sctk_add_dynamic_reorder_buffer(dest);
}

void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_ADD(hh,sctk_dynamic_route_table,key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
}

void sctk_init_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;
  /* FIXME: the following commented line may potentially break other modules (like TCP). */
  sctk_route_set_low_memory_mode_local(tmp, 0);
  sctk_route_set_low_memory_mode_remote(tmp, 0);
  sctk_route_set_state(tmp, state_deconnected);

  tmp->is_initiator = CHAR_MAX;
  tmp->lock = SCTK_SPINLOCK_INITIALIZER;

  tmp->origin = route_origin_static;
  sctk_add_static_reorder_buffer(dest);
}

void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  /* FIXME: Ideally the initialization should not be in the 'add' function */
  sctk_init_static_route(dest, tmp, rail);
  TABLE_LOCK();
  HASH_ADD(hh,sctk_static_route_table,key,sizeof(sctk_route_key_t),tmp);
  TABLE_UNLOCK();
}


/*-----------------------------------------------------------
 *  Routes walking
 *----------------------------------------------------------*/

UT_icd ptr_icd = {sizeof(sctk_route_table_t**), NULL, NULL, NULL};

/*
 * Walk through all registered routes and call the function 'func'.
 * Static and dynamic routes are involved
 */
void sctk_walk_all_routes(const sctk_rail_info_t* rail, void (*func) (const sctk_rail_info_t* rail,sctk_route_table_t* table) ) {
  sctk_route_table_t *current_route, *tmp, **tmp2;
  UT_array *routes;
  utarray_new(routes, &ptr_icd);

  /* We do not need to take a lock */
  HASH_ITER(hh, sctk_static_route_table,current_route, tmp) {
    if ( sctk_route_get_state(current_route) == state_connected) {
      if (sctk_route_cas_low_memory_mode_local(current_route, 0, 1) == 0) {
        utarray_push_back(routes, &current_route);
      }
    }
  }

  sctk_spinlock_read_lock(&sctk_route_table_lock);
  HASH_ITER(hh, sctk_dynamic_route_table,current_route, tmp) {
    if ( sctk_route_get_state(current_route) == state_connected) {
      if (sctk_route_cas_low_memory_mode_local(current_route, 0, 1) == 0) {
        utarray_push_back(routes, &current_route);
      }
    }
  }
  sctk_spinlock_read_unlock(&sctk_route_table_lock);

  tmp2 = NULL;
  while( (tmp2=(sctk_route_table_t**) utarray_next(routes,tmp2))) {
      func(rail, *tmp2);
  }
}

/* Walk through all registered routes and call the function 'func'.
 * Only dynamic routes are involved */
void sctk_walk_all_dynamic_routes(const sctk_rail_info_t* rail, void (*func) (const sctk_rail_info_t* rail,sctk_route_table_t* table) ) {
  sctk_route_table_t *current_route, *tmp, **tmp2;
  UT_array *routes;
  utarray_new(routes, &ptr_icd);

  /* Do not walk on static routes */

  sctk_spinlock_read_lock(&sctk_route_table_lock);
  HASH_ITER(hh, sctk_dynamic_route_table,current_route, tmp) {
    if ( sctk_route_get_state(current_route) == state_connected) {
      if (sctk_route_cas_low_memory_mode_local(current_route, 0, 1) == 0) {
        utarray_push_back(routes, &current_route);
      }
    }
  }
  sctk_spinlock_read_unlock(&sctk_route_table_lock);

  tmp2 = NULL;
  while( (tmp2=(sctk_route_table_t**) utarray_next(routes,tmp2))) {
      func(rail, *tmp2);
  }
}


/*-----------------------------------------------------------
 *  Route calculation
 *----------------------------------------------------------*/

static inline
sctk_route_table_t* sctk_get_route_to_process_no_route_static(int dest, sctk_rail_info_t* rail){
  sctk_route_key_t key;
  sctk_route_table_t* tmp;

  key.destination = dest;
  key.rail = rail->rail_number;

  /* FIXME: We do not need to take a lock for the static table. No route can be created
   * or destructed during execution time */
  /* TABLE_LOCK(); */
  HASH_FIND(hh,sctk_static_route_table,&key,sizeof(sctk_route_key_t),tmp);
  /* TABLE_UNLOCK(); */
  return tmp;
}

sctk_route_table_t* sctk_get_route_to_process_no_route(int dest, sctk_rail_info_t* rail){
  sctk_route_key_t key;
  sctk_route_table_t* tmp;

  key.destination = dest;
  key.rail = rail->rail_number;

  HASH_FIND(hh,sctk_static_route_table,&key,sizeof(sctk_route_key_t),tmp);
  if(tmp == NULL){
    sctk_spinlock_read_lock(&sctk_route_table_lock);
    HASH_FIND(hh,sctk_dynamic_route_table,&key,sizeof(sctk_route_key_t),tmp);
    sctk_spinlock_read_unlock(&sctk_route_table_lock);

    /* Wait if route beeing deconnected */
    /* FIXME: not compatible with other module than IB */
    if (tmp) {
      sctk_ib_qp_t *remote = tmp->data.ib.remote;
      sctk_thread_wait_for_value (&remote->deco_lock, 0);
    }
    /* If the route is deconnected, we do not use it*/
    if (tmp && sctk_route_get_state(tmp) != state_connected) {
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

  if (sctk_route_get_state(args->route_table) == state_connected) {
    args->done = 1;
  } else {
    /* The notify idle message *MUST* be filled for supporting on-demand
     * connexion */
    sctk_network_notify_idle_message();
  }
  return NULL;
}

/* Get a route to a process only on static routes */
inline sctk_route_table_t* sctk_get_route_to_process_static(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  tmp = sctk_get_route_to_process_no_route_static(dest,rail);

  if(tmp == NULL){
    dest = rail->route(dest,rail);
    return sctk_get_route_to_process_static(dest,rail);
  }

  return tmp;
}

/* Get a route to a process with no ondemand connexions */
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
  if (tmp) {
  	sctk_nodebug("Directly connected to %d", dest);
  } else {
  	sctk_nodebug("NOT Directly connected to %d", dest);
  }

  if(tmp == NULL){
#if MPC_USE_INFINIBAND
    if (rail->on_demand) {
      sctk_nodebug("%d Trying to connect to process %d (remote:%p)", sctk_process_rank, dest, tmp);
      /* We send the request using the signalization rail */
      tmp = sctk_ib_cm_on_demand_request(dest,rail);
      assume(tmp);
      /* If route not connected, so we wait for until it is connected */
      if (sctk_route_get_state(tmp) != state_connected) {
        struct wait_connexion_args_s args;
        args.route_table = tmp;
        args.done = 0;
        args.rail = rail;
        sctk_thread_wait_for_value_and_poll((int*) &args.done, 1,
            (void (*)(void*)) __wait_connexion, &args);
        assume(sctk_route_get_state(tmp) == state_connected);
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

int sctk_route_get_rail_nb(){
  return rail_number;
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
    sprintf(name_ptr,"\nRail %d/%d [%s (%s)]",i+1, rail_number, rails[i].network_name,rails[i].topology_name);
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

/* TORUS */
inline void sctk_Node_zero (sctk_Node_t *Node )
    {
        unsigned i = 0,j = 0;
        for ( i = 0; i < Node->d; i++ )
        {
		    for ( j = 0; j < 4; j++ )
		    {
		        Node->neigh[i][j] = -1;
		        if(j<2)
		        	Node->breakdown[i*2+j] = 0;
		    }
            Node->c[i] = 0;
        }
    }

inline void sctk_Node_print (sctk_Node_t *Node )
    {
        int i = 0;
        sctk_debug( "Node %ld",Node->id);
        for ( i = 0; i < Node->d; i++ )
        {
            sctk_debug( "[%d]=%d", i, Node->c[i] );
        }
    }

void sctk_Node_init (sctk_Node_t *Node, int id)
    {
    	unsigned d = Torus.dimension;
    	int regular = Torus.node_regular;
        if ( MAX_SCTK_FAST_NODE_DIM < d)
        {
            sctk_error("MAX_SCTK_FAST_NODE_DIM cannot be enabled on more than %d dimensions",
                      MAX_SCTK_FAST_NODE_DIM );
            not_reachable();
        }
		unsigned i;
		Node->id = id;
        Node->d = d;
        sctk_Node_zero ( Node );
        //printf("dim %d\nregular %ld\nsize %d\n",d,regular,size);
        if(d > 1){
		    if(id < regular){//normal case
				Node->c[0] = id / (regular/size);
				regular = regular/size;
				for(i=1;i<d;i++){
					id = id - regular * Node->c[i-1];
					if(i!=d-1){
						regular = regular/size;
						Node->c[i] = id / regular;
					}
					else{
						regular = regular/Torus.size_last_dimension;
						Node->c[i] = id / regular;
					}
				}
			}
			else{
				id = id - regular;
				regular = regular / size / Torus.size_last_dimension;
				Node->c[0] = id / regular;

				for(i=1;i<d;i++){

					if(i!=d-1){
						id = id - regular * Node->c[i-1];
						regular = regular/size;
						//printf("%d %d\n",id,regular);
						Node->c[i] = id / regular;
					}
					else{
						Node->c[d-1] = Torus.size_last_dimension;
					}

				}
				//sctk_Node_print ( Node );
			}
		}
		else
			Node->c[0] = id;

		//sctk_Node_print ( Node );
    }


inline void sctk_Node_release ( sctk_Node_t *Node )
    {
        Node->d = 0;
    }

inline void sctk_Node_set_from ( sctk_Node_t *Node, sctk_Node_t *NodeToCopy )
    {
		unsigned i,j;

		Node->d = NodeToCopy->d;
		sctk_Node_zero ( Node );
		Node->id = NodeToCopy->id;
		for(i=0;i<NodeToCopy->d;i++){
			Node->c[i] = NodeToCopy->c[i];
			for ( j = 0; j < 4; j++ )
		    {
		        Node->neigh[i][j] = NodeToCopy->neigh[i][j];
		    }
		}
    }

inline int sctk_Node_distance (int a, int b ,unsigned sdim)
    {

        int tmp = 0;
        int tmp2 = 0;

        long int dist = 0;



        if(a > b){
        	tmp = a - b;
        	tmp2 = sdim - (a - b);
        }
        else{
        	tmp = b - a;
        	tmp2 = sdim - (b - a);
        }
        dist = (tmp < tmp2) ? tmp : tmp2;

        return dist;
    }





unsigned sctk_Torus_dim_set(int node_count){
	unsigned dim = 1;
	while(pow(MIN_SIZE_DIM,dim+1) <= node_count){
		dim++;
		if(dim > MAX_SCTK_FAST_NODE_DIM){
			sctk_nodebug("\nWARNING : the dimension was set at the maximum (%d) because of the number of nodes which is too big.\nThe size of each dimension may be high\n",MAX_SCTK_FAST_NODE_DIM);
			return MAX_SCTK_FAST_NODE_DIM;
		}
	}

	return dim;
}

void sctk_Torus_init ( int node_count, uint8_t dimension)
{
    if ( dimension == 0 )
    {
        sctk_error("0 dimension is not supported %s:%d", __FILE__, __LINE__ );
        not_reachable();
    }
	int tmp;
	unsigned i;
	int diff=0;
	unsigned min;
	unsigned last;
    /*Setup the node count */
    Torus.node_count = node_count;
    Torus.dimension = dimension;
    Torus.node_regular = 1;
    Torus.node_left = 0;


    diff = node_count / pow ( size, dimension-1 ) - size;
    if(diff<0)
    	diff = -diff;
    min = diff;
    size++;
	while(diff <= min){
		last = node_count / pow ( size, dimension-1 );
    	diff = last - size;
    	if(diff<0)
    		diff = -diff;
    	if(last < MIN_SIZE_DIM)
    		break;
    	if(diff <= min){
    		min = diff;
    		size++;
    	}
	}
	size--;

    tmp = node_count;
    for ( i = 0; i < dimension - 1; i++ )
    {
    	tmp = tmp / size;
    	Torus.node_regular *= size;
    }
    /* Put the rest in the last dimension */
    Torus.node_regular *= tmp;
	Torus.size_last_dimension = tmp;
    Torus.node_left = Torus.node_count - Torus.node_regular;

    /*imperfect Torus*/
    sctk_Node_init ( &Torus.last_node,node_count-1);
    //sctk_Node_print ( &Torus.last_node);
    ///**********************************************
    sctk_nodebug("dimension %d",dimension);
   	sctk_nodebug("size %d\nsize_last_dimension %d\nnode_regular %ld\nnode_left %ld",size,Torus.size_last_dimension ,Torus.node_regular,Torus.node_left);

}



void sctk_Torus_release ()
{
    Torus.node_count = 0;
    Torus.dimension = 0;
    sctk_Node_release ( &Torus.last_node );
}

int sctk_Node_id ( sctk_Node_t *coord ){

	int id = 0;
	int size_in_this_dim = Torus.node_regular / size;
	unsigned i;
	if(coord->c[Torus.dimension-1] < Torus.size_last_dimension){//normal case
		for(i=0;i<Torus.dimension-1;i++){
			id += coord->c[i] * size_in_this_dim;
			size_in_this_dim /= size;
		}
		id += coord->c[Torus.dimension-1];
	}
	else
	{

		size_in_this_dim /= Torus.size_last_dimension;
		id =  Torus.node_regular;
		for(i=0;i<Torus.dimension-1;i++){
			id += coord->c[i] * size_in_this_dim;
			//printf("+%ld\n",coord->c[i] * size_in_this_dim);
			size_in_this_dim /= size;
		}


	}
	//coord->id = id;

	return id;
}
int sctk_Torus_neighbour_dimension( unsigned i,unsigned j){
	unsigned k;
	unsigned IsSpecialCase=1;
	unsigned l;

	switch(j){
		case 0://"left" (decr)

			if(node.c[i] - 1 >= 0)
				return node.c[i]-1;
			else
				if(node.id < Torus.node_regular){//normal node
					if(i!=node.d-1)
						return size-1;
					else{
						k=0;
						while(k<node.d){

							if((Torus.node_regular != Torus.node_count) && (node.c[k] < Torus.last_node.c[k]))
								break;

							if((Torus.node_regular == Torus.node_count)||(node.c[k] > Torus.last_node.c[k])){
								IsSpecialCase = 0;
								break;
							}
							k++;
						}

						if(IsSpecialCase) {
              if (Torus.size_last_dimension > node.c[i] + 1) {
							  return Torus.size_last_dimension;
              } else return -1;
            } else {
              if ( (Torus.size_last_dimension - 1) > node.c[i] + 1) {
							  return Torus.size_last_dimension -1 ;
              } else return -1;

            }
					}
				}
				else{
					k=0;
					l=size-1;

					while(k<node.d-1){
						if(i==k){
							if(Torus.last_node.c[k] < l){
								l = Torus.last_node.c[k];
							}
							if(Torus.last_node.c[k] > l){
								return l;
							}
						}
						else{
							if(Torus.last_node.c[k] > node.c[k]){
									return l;
							}
							if(Torus.last_node.c[k] < node.c[k]){
								l--;
                break;
              }
						}
						k++;
					}

          if(l<=node.c[i]+1)
            return -1;
          else
            return l;
				}
			break;
		case 1://"right" (incr)

			if(node.id < Torus.node_regular){//normal node
				if(i!=node.d-1)
					return ((node.c[i]+1) % size);
				else if(i==node.d-1 && node.c[i]+1 < Torus.size_last_dimension)
					return (node.c[i]+1);

				k=0;

				while(k<node.d){

					if((Torus.node_regular != Torus.node_count) && (node.c[k] < Torus.last_node.c[k]))
						break;

					if((Torus.node_regular == Torus.node_count)||(node.c[k] > Torus.last_node.c[k])){
						IsSpecialCase = 0;
						break;
					}
					k++;
				}
				if(IsSpecialCase) {
					return Torus.size_last_dimension;
        } else {
          if ( 0 < node.c[i] - 1) {
  					return 0;
          } else return -1;
        }
			}
			else{
				if(i==node.d-1)
					return 0;
				k=0;
				l=node.c[i] + 1;


				while(k<node.d-1){

					if(i==k){
						if(Torus.last_node.c[k] < l){
							l = 0;
							break;
						}
						if(Torus.last_node.c[k] > l){
							break;
						}
					}
					else{
						if(Torus.last_node.c[k] > node.c[k]){
							l = l % size;
							break;
						}
						if(Torus.last_node.c[k] < node.c[k]){
							l = 0;
							break;
						}
					}
					k++;
				}
				if(l==node.c[i] || l==node.c[i]-1)
					return -1;
				else
					return l;
			}

			break;
		default:
			sctk_error("Wrong argument j aborting in %s it must be 0 or 1, here is %d",__FUNCTION__,j);
      not_implemented();
      break;

	}

	return -1;
}

int sctk_Torus_route_next(sctk_Node_t *dest){
	int i,iorigin;
	unsigned j,k,IsSpecialCase = 1;
	//unsigned imin,jmin;
	//unsigned dep;
	unsigned PerDefault=0;
	long long int dist;
	long long int min_way=0;
	int nearest_id,id,current_coord;
	int poss[Torus.dimension],otherPoss[Torus.dimension],otherPoss2[Torus.dimension];
	int DerPoss = 0;
	int DerOtherPoss = 0;
	int DerOtherPoss2 = 0;
	int indPoss;

	unsigned g,d;
	unsigned sizeDim;
	//for(i=0;i<Torus.dimension-1;i++){
	min_way += size;
	//}
	min_way += Torus.size_last_dimension;
	min_way *= min_way;

	i=0;
	if((node.id >= Torus.node_regular) && ((dest->id < Torus.node_regular)||(node.id < dest->id))){
		i = Torus.dimension-1;
		j=0;
	   	while(i >= 0){
	   		if(node.c[i]!=dest->c[i]){
	   			poss[DerPoss] = i;
	   			DerPoss++;
	   		}
	   		else{
	   			otherPoss2[DerOtherPoss2] = i;
	   			DerOtherPoss2++;
	   		}
	   		i--;
	   	}
	   	//dep = 0;
	}
	else{
		j=0;
	   	while(i < Torus.dimension){
	   		if(node.c[i]!=dest->c[i]){
	   			poss[DerPoss] = i;
	   			DerPoss++;
	   		}
	   		else{
	   			otherPoss2[DerOtherPoss2] = i;
	   			DerOtherPoss2++;
	   		}
	   		i++;
	   	}
   		//dep = 1;
   	}
   	/*
   	if(i==Torus.dimension){
   		sctk_Node_print ( &node );
   		sctk_Node_print ( dest );
   		abort();
   	}
   	*/
   	if(node.id >= Torus.node_regular){
   		indPoss = 0;
   	}
   	else{
   		if(dest->id >= Torus.node_regular){
   			k=0;
   			IsSpecialCase = 1;
			while(k<node.d){

				if(node.c[k] < Torus.last_node.c[k])
					break;

				if(node.c[k] > Torus.last_node.c[k]){
					IsSpecialCase = 0;
					break;
				}
				k++;
			}
			if(!IsSpecialCase)
				DerPoss--;
   		}
   		//indPoss = rand() % DerPoss;

   	}

   	nearest_id = -1;
   	indPoss = 0;
   	i = poss[indPoss];
    /*
    printf("route--->\n");
   	sctk_Node_print ( &node );
   	sctk_Node_print ( dest );
   	fflush(stdout);
	*/
	current_coord = node.c[i];
	iorigin = i;
	while(nearest_id == -1){
		//printf("%d \n",i);
		for(j=0;j<2;j++){

				IsSpecialCase = 1;
				if(node.neigh[i][j*2] != node.id){

          if(node.neigh[i][j*2] < -1 || node.neigh[i][j*2] > Torus.node_count){
						sctk_error("Error route_next");
						not_reachable();
					}

					if(node.neigh[i][j*2+1]!=-1){
						//printf("i %d j %d id %ld c %d\n",i,j,node.id,node.c[i]);
						//fflush(stdout);
						//sleep(1);
						if(i==Torus.dimension-1){
								k=0;
								while(k<node.d){

									if((Torus.node_regular != Torus.node_count) && (node.c[k] < Torus.last_node.c[k]))
										break;

									if((Torus.node_regular == Torus.node_count)||(node.c[k] > Torus.last_node.c[k])){
										IsSpecialCase = 0;
										break;
									}
									k++;
								}
								if(IsSpecialCase)
									sizeDim = Torus.size_last_dimension+1;
								else
									sizeDim = Torus.size_last_dimension;
						}
						else
							sizeDim = size;

						dist = sctk_Node_distance (node.neigh[i][j*2+1], dest->c[i], sizeDim);
						/*if((node.neigh[i][j*2] == 65 || node.neigh[i][j*2] == 5)&& sctk_process_rank==4){
							printf("%d -> %d (%d %d %d)\n",j,dist,node.neigh[i][j*2+1], dest->c[i],sizeDim);
						}*/
						if(dist <= min_way || PerDefault){
							//printf("min trouve !!!\n");
							if(PerDefault){
								if(node.breakdown[i*2 + j] == 0)
									nearest_id = node.neigh[i][j*2];

							}
							else{

								if(dist == min_way-1){
									otherPoss[DerOtherPoss] = i;
									DerOtherPoss++;
								}
								else if(dist == min_way-2){
									if(node.neigh[i][1] > dest->c[i]){
										g = node.neigh[i][1] - dest->c[i];
										d = sizeDim - (node.neigh[i][1] - dest->c[i]);
									}
									else{
										d = dest->c[i] - node.neigh[i][1];
										g = sizeDim - (dest->c[i] - node.neigh[i][1]);
									}
									if(g==d){
										otherPoss2[DerOtherPoss2] = i;
										DerOtherPoss2++;
									}
								}

								/*if this neighbour is available*/
								if(node.breakdown[i*2 + j] == 0){
									if(dist < min_way || nearest_id == -1/* || rand()%2*/){
										nearest_id = node.neigh[i][j*2];
									}
								}
								else{
									if(dist < min_way){
										nearest_id = -1;
									}
								}
								min_way = dist;

							}


						}
						else if(dist == min_way+1){
							otherPoss[DerOtherPoss] = i;
							DerOtherPoss++;
						}
						else if(dist == min_way+2){
							if(node.neigh[i][3] > dest->c[i]){
								g = node.neigh[i][3] - dest->c[i];
								d = sizeDim - (node.neigh[i][3] - dest->c[i]);
							}
							else{
								d = dest->c[i] - node.neigh[i][3];
								g = sizeDim - (dest->c[i] - node.neigh[i][3]);
							}

							if(g==d){
								otherPoss2[DerOtherPoss2] = i;
								DerOtherPoss2++;
							}
						}
					}
				}
		}

		if(nearest_id ==-1){
			min_way=0;
			min_way += size;
			min_way += Torus.size_last_dimension;
			min_way *= min_way;
			if(DerPoss == 1){
				indPoss = -1;
				DerPoss--;
			}

			if(DerPoss > 1){
				poss[indPoss] = poss[DerPoss-1];
				DerPoss--;
				indPoss = rand() % DerPoss;
				i = poss[indPoss];
				if(DerPoss == 1){
					indPoss = -1;
				}
			}
			else if(DerOtherPoss >= 1){
				if(indPoss == -1){
					PerDefault = 1;
				}
				else{
					otherPoss[indPoss] = otherPoss[DerOtherPoss-1];
					DerOtherPoss--;
				}

				if(DerOtherPoss == 0){
					indPoss = -1;
				}
				else{
					indPoss = rand() % DerOtherPoss;
					i = otherPoss[indPoss];
				}

			}
			else if(DerOtherPoss2 >= 1){
				if(indPoss == -1){
					PerDefault = 1;
				}
				else{
					otherPoss2[indPoss] = otherPoss2[DerOtherPoss2-1];
					DerOtherPoss2--;
				}

				if(DerOtherPoss2 == 0){
					indPoss = -1;
				}
				else{
					indPoss = rand() % DerOtherPoss2;
					i = otherPoss2[indPoss];
				}
			}
			else{
				i = rand() % Torus.dimension;//au cas où l'on peut vraiment rien faire on tente
				PerDefault = 1;
			}
		}
		//sctk_debug("routing to %d",dest->id);
		//sctk_debug("routing passed by %d (%d %d %d) %d",nearest_id,DerPoss,DerOtherPoss,DerOtherPoss2,indPoss);
	}
	sctk_nodebug("routing passed by %d (%d %d %d)",nearest_id,DerPoss,DerOtherPoss,DerOtherPoss2);
	return nearest_id;
}

void sctk_torus_breakroute(int val){//just to simulate

	sctk_nodebug("neigh number %d disabled on the %dth dimension",val%2,val/2);
	node.breakdown[val] = 1;

}

void sctk_torus_restoreroute(int val){//just to simulate

	sctk_nodebug("neigh number %d enabled on the %dth dimension",val%2,val/2);
	node.breakdown[val] = 0;

}



void sctk_route_torus_init(sctk_rail_info_t* rail){
	sctk_nodebug("process %d started",sctk_process_rank);
	int (*sav_sctk_route)(int , sctk_rail_info_t* );
	unsigned dim;
	unsigned current_coord;

  	dim = sctk_Torus_dim_set(sctk_process_number);
    sctk_Torus_init (sctk_process_number, dim);
    sctk_Node_init (&node ,sctk_process_rank);

	sctk_pmi_barrier();
	sctk_route_table_init_lock_needed = 1;
	sctk_pmi_barrier();
	sav_sctk_route = rail->route;
	rail->route = sctk_route_ring;


	if(sctk_process_number > 3){
		int me=sctk_process_rank;
		//sleep(me*2);
		int neigh;
		int i,j;

		for(i=0;i<dim;i++){

			current_coord = node.c[i];
			if(node.c[i] % 2){
				for(j=1;j>=0;j--){
					sctk_nodebug("process %d search neigbour %d",me,j);
					node.c[i] = sctk_Torus_neighbour_dimension(i,j);
					sctk_nodebug("process %d have neighbour %d in dim %d -> %d",me,j,i, node.c[i]);
					if(node.c[i]==-1){
							node.neigh[i][j*2] = node.id;
							sctk_nodebug("process %d don't have neighbour %d in dim %d",me,j,i);
					}
					else{
						node.neigh[i][j*2+1] = node.c[i];
						neigh = sctk_Node_id ( &node );
						node.neigh[i][j*2] = neigh;
						sctk_route_table_t* tmp;

						tmp = sctk_get_route_to_process_no_route(neigh,rail);
						if(tmp == NULL){
              assume(neigh != me);
							if(me < neigh)
								rail->connect_from(me,neigh,rail);
							else
								rail->connect_to(neigh,me,rail);
						}


					}
					node.c[i] = current_coord;

				}
			}
			else{
				for(j=0;j<2;j++){
					node.c[i] = sctk_Torus_neighbour_dimension(i,j);
					sctk_nodebug("process %d have neighbour %d in dim %d -> %d",me,j,i, node.c[i]);
					if(node.c[i]==-1){
							node.neigh[i][j*2] = node.id;
							sctk_nodebug("process %d don't have neighbour %d in dim %d",me,j,i);
					}
					else{
						node.neigh[i][j*2+1] = node.c[i];
						neigh = sctk_Node_id ( &node );
						node.neigh[i][j*2] = neigh;
						sctk_route_table_t* tmp;

						tmp = sctk_get_route_to_process_no_route(neigh,rail);
						if(tmp == NULL){
              assume(neigh != me);
							if(me < neigh)
								rail->connect_from(me,neigh,rail);
							else
								rail->connect_to(neigh,me,rail);
						}
					}
					node.c[i] = current_coord;

				}
			}
			sctk_nodebug("process %d passed %d step",me,i);
		}
		sctk_nodebug("process %d passed",me);
	  }

	  sctk_pmi_barrier();
	  rail->route = sav_sctk_route;
	  sctk_route_table_init_lock_needed = 0;
	  sctk_pmi_barrier();
}

int sctk_route_torus(int dest, sctk_rail_info_t* rail){
  int old_dest;

  old_dest = dest;
  sctk_Node_t dest_node;
  sctk_Node_init (&dest_node, dest);

  dest = sctk_Torus_route_next(&dest_node);
  sctk_nodebug("Route via dest - 1 %d to %d",dest,old_dest);
  return dest;
}

/*****************************************************/


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
    rail->topology_name = "On-Demand connections";
  } else if(strcmp("torus",topology) == 0){
    rail->route = sctk_route_torus;
    rail->route_init = sctk_route_torus_init;
    rail->topology_name = "torus";
  } else {
    not_reachable();
  }
}

/*
 * Signalization rails: getters and setters
 */
static sctk_rail_info_t* rail_signalization = NULL;

void sctk_route_set_signalization_rail(sctk_rail_info_t* rail) {
  rail_signalization = rail;
}
sctk_rail_info_t* sctk_route_get_signalization_rail() {
  assume(rail_signalization);
  return rail_signalization;
}

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
            fprintf ( stderr, "MAX_SCTK_FAST_NODE_DIM cannot be enabled on more than %d dimensions\n",
                      MAX_SCTK_FAST_NODE_DIM );
            abort ();
        }
		unsigned i;
		Node->id = id;
        Node->d = d;
        sctk_Node_zero ( Node );
        //printf("dim %d\nregular %ld\nsize %d\n",d,regular,size);
        if(d > 1){
		    if(id < regular){
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

inline int sctk_Node_quadratic_distance (int a, int b ,unsigned sdim)
    {

        int tmp = 0;
        int tmp2 = 0;

        long int dist = 0;


        tmp = a - b;
        if(a > b)
        	tmp2 = sdim - (a - b);
        else
        	tmp2 = sdim - (b - a);

        dist = (tmp*tmp < tmp2*tmp2) ? tmp*tmp : tmp2*tmp2;

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
        fprintf ( stderr,  "0 dimension is not supported %s:%d\n", __FILE__, __LINE__ );
        abort ();
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
	coord->id = id;

	return id;
}
int sctk_Torus_neighbour_dimension( unsigned i,unsigned j){
	unsigned k;
	unsigned IsSpecialCase=1;
	unsigned l;

	switch(j){
		case 0://gauche

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
						if(IsSpecialCase)
							return Torus.size_last_dimension;
						else
							return Torus.size_last_dimension - 1;
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
								if(l<=Torus.last_node.c[i]+1)
									return -1;
								else
									return l;
							}
						}
						k++;
					}

					return l;
				}
			break;
		case 1:

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
				if(IsSpecialCase)
					return Torus.size_last_dimension;
				else
					return 0;

			}
			else{
				if(i==node.d-1)
					return 0;
				k=0;
				l=(node.c[i]+1) % size;
				if(l==node.c[i] || l==node.c[i]-1)
					return -1;

				while(k<node.d-1){

					if(i==k){
						if(Torus.last_node.c[k] < l){
							return -1;
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
							return -1;
						}
					}
					k++;
				}

				return l;
			}

			break;
		default:
			fprintf(stderr,"Wrong argument j aborting in %s it must be 0 or 1, here is %d\n",__FUNCTION__,j);
			abort();
			break;

	}

	return -1;
}

int sctk_Torus_route_next(sctk_Node_t *dest){
	unsigned i,j,k,IsSpecialCase = 1;
	unsigned imin,jmin;
	long long int dist;
	long long int min_way=0;
	int nearest_id;
	//for(i=0;i<Torus.dimension-1;i++){
	min_way += size;
	//}
	min_way += Torus.size_last_dimension;
	min_way *= min_way;
	sctk_Node_t node_tmp;
	sctk_Node_set_from(&node_tmp,&node);

	i=0;
	if((node_tmp.id >= Torus.node_regular) && ((dest->id < Torus.node_regular)||(node_tmp.id < dest->id))){
		i = Torus.dimension-1;
		if(dest->id >= Torus.node_regular){
			while(node_tmp.c[i]==dest->c[i]){
	   			i--;
	   		}
	   	}
	}
	else{
   		while(node_tmp.c[i]==dest->c[i])
   			i++;
   	}
   	if(i==Torus.dimension){
   		sctk_Node_print ( &node_tmp );
   		sctk_Node_print ( dest );
   		abort();
   	}
   /*	printf("route--->\n");
   	sctk_Node_print ( &node_tmp );
   	sctk_Node_print ( dest );
   	fflush(stdout);
	*/

	for(j=0;j<2;j++){
			IsSpecialCase = 1;
			if(node.neigh[i][j*2] != node.id){

				if(node.neigh[i][j*2] == -1){

					node_tmp.c[i] = sctk_Torus_neighbour_dimension(i,j);
					if(node_tmp.c[i]==-1){
						node.neigh[i][j*2] = node.id;
					}
					else{
						node.neigh[i][j*2+1] = node_tmp.c[i];
						node.neigh[i][j*2] = sctk_Node_id ( &node_tmp );
					}
				}
				else
					node_tmp.c[i] = node.neigh[i][j*2+1];
				if(node.neigh[i][j*2] < -1 || node.neigh[i][j*2] > Torus.node_count){
					fprintf(stderr,"Error route_next\n");
					abort();
				}

				if(node_tmp.c[i]!=-1){
					//printf("i %d j %d id %ld c %d\n",i,j,node_tmp.id,node_tmp.c[i]);
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
								dist = sctk_Node_quadratic_distance (node_tmp.c[i], dest->c[i], Torus.size_last_dimension+1);
							else
								dist = sctk_Node_quadratic_distance (node_tmp.c[i], dest->c[i], Torus.size_last_dimension);
					}
					else
						dist = sctk_Node_quadratic_distance (node_tmp.c[i], dest->c[i], size);

					if(dist < min_way){
						//printf("min trouve !!!\n");
						min_way = dist;
						nearest_id = node.neigh[i][j*2];
						imin = i;
						jmin = j;

					}
				}
			}
		}

   	/*
	//this part would be changed in MPC context
	node.c[imin] = node.neigh[imin][jmin*2+1];
	node.id = nearest_id;

	for(i=0;i<Torus.dimension;i++)
		for(j=0;j<4;j++)
			node.neigh[i][j] = -1;
	*/
	sctk_debug("routing passed by %d",nearest_id);
	return nearest_id;
}





void sctk_route_torus_init(sctk_rail_info_t* rail){
	sctk_nodebug("process %d started",sctk_process_rank);
	int (*sav_sctk_route)(int , sctk_rail_info_t* );
	unsigned dim;
	sctk_pmi_barrier();
	sctk_route_table_init_lock_needed = 1;
	sctk_pmi_barrier();
	dim = sctk_Torus_dim_set(sctk_process_number);
    sctk_Torus_init (sctk_process_number, dim);

	sctk_Node_init (&node ,sctk_process_rank);
	sav_sctk_route = rail->route;
	rail->route = sctk_route_ring;


	if(sctk_process_number > 3){
		int me=sctk_process_rank;
		//sleep(me*2);
		int neigh;
		int i,j;
		sctk_Node_t node_tmp;
		sctk_Node_set_from(&node_tmp,&node);
		//for(from = 0; from < sctk_process_number; from++){
		//for(to = 0; to < sctk_process_number; to ++){
		for(i=0;i<dim;i++){

			if(node_tmp.c[i] % 2){
				for(j=1;j>=0;j--){
					sctk_nodebug("process %d search neigbour %d",me,j);
					node_tmp.c[i] = sctk_Torus_neighbour_dimension(i,j);
					if(node_tmp.c[i]==-1){
							node.neigh[i][j] = node.id;
					}
					else{
						node.neigh[i][j*2+1] = node_tmp.c[i];
						neigh = sctk_Node_id ( &node_tmp );
						node.neigh[i][j*2] = neigh;
						sctk_route_table_t* tmp;

						tmp = sctk_get_route_to_process_no_route(neigh,rail);
						if(tmp == NULL){
							sctk_nodebug("process %d want connection to %d",me,neigh);
							if(me < neigh)
								rail->connect_from(me,neigh,rail);
							else
								rail->connect_to(neigh,me,rail);
						}


					}

				}
			}
			else{
				for(j=0;j<2;j++){
					node_tmp.c[i] = sctk_Torus_neighbour_dimension(i,j);
					if(node_tmp.c[i]==-1){
							node.neigh[i][j] = node.id;
					}
					else{
						node.neigh[i][j*2+1] = node_tmp.c[i];
						neigh = sctk_Node_id ( &node_tmp );
						node.neigh[i][j*2] = neigh;
						sctk_route_table_t* tmp;

						tmp = sctk_get_route_to_process_no_route(neigh,rail);
						if(tmp == NULL){
							sctk_nodebug("process %d want connection to %d",me,neigh);
							if(me < neigh)
								rail->connect_from(me,neigh,rail);
							else
								rail->connect_to(neigh,me,rail);
						}


					}
				}
			}
			sctk_nodebug("process %d passed %d step",me,i);
			node_tmp.c[i] = node.c[i];
		}
		sctk_nodebug("process %d passed",me);
		//}
		//}
		//sctk_pmi_barrier();
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
  sctk_debug("Route via dest - 1 %d to %d",dest,old_dest);

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

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
#ifndef __SCTK_ROUTE_H_
#define __SCTK_ROUTE_H_

#include <sctk_inter_thread_comm.h>
#include <sctk_runtime_config.h>
#include <uthash.h>
#include <math.h>
#include <sctk_portals.h>


typedef struct sctk_rail_info_s sctk_rail_info_t;
struct sctk_ib_data_s;

#include <sctk_tcp.h>
#include <sctk_ib.h>

/** This statically defines the maximum size of a Node */
#define MAX_SCTK_FAST_NODE_DIM 10

/** This statically defines the minimum number of ranks for each dimension */
#define SCTK_TORUS_MIN_PROC_IN_DIM 4


/************************************************************************/
/* sctk_Node_t                                                          */
/************************************************************************/

/**
* \brief Implements a static Node
*
* The sctk_Node_t is a static Node which maximum dimension
* is determined by the define (MAX_SCTK_FAST_NODE_DIM)
*/
typedef struct sctk_Node_t
{
	int c[MAX_SCTK_FAST_NODE_DIM];				/**< Node storage */
	int neigh[MAX_SCTK_FAST_NODE_DIM][4];			/**< Node neigbours storage */
	sctk_uint8_t breakdown[MAX_SCTK_FAST_NODE_DIM*2];
	sctk_uint8_t d;                        			/**< Node dimension */
	int id;							/**< Node ident */
}sctk_Node_t;

/**
* \brief Zero a Node
* \param Node pointer to sctk_Node_t
*
* This call will set all the dimensions
* of the given tupple to 0
*/
inline void sctk_Node_zero ( sctk_Node_t *Node );

/**
* \brief Display a Node to stdout
* \param Node pointer to sctk_Node_t
*/
inline void sctk_Node_print (sctk_Node_t *Node );

/**
* \brief Initialize a Node
* \param Node pointer to an sctk_Node_t
* \param int the ident of the node
*/
void sctk_Node_init (sctk_Node_t *Node, int id);

/**
* \brief Compute the id (~rank) of a given coordinate in the Torus
* \param coord coordinates
* \return id associated to these coordinates
*
* Once the sctk_Torus_t is initialized this call is thread safe
* This call will abort if check are enabled and entries invalid
*/
int sctk_Node_id ( sctk_Node_t *coord );

/**
* \brief Release a Node
* \param Node pointer to sctk_Node_t
*
* This call just set the dimension to 0
* making the Node unusable
*/
inline void sctk_Node_release ( sctk_Node_t *Node );

/**
* \brief Set a Node by copy
* \param Node pointer to sctk_Node_t, it will be updated
* \param Node pointer to sctk_Node_t
*/
inline void sctk_Node_set_from ( sctk_Node_t *Node, sctk_Node_t *NodeToCopy );

/************************************************************************/
/* sctk_Torus_t                                                         */
/************************************************************************/

/**
* \brief Implements a Torus topology
*
* The sctk_Torus_t implements a d-Torus with dimension
* varying from 1 to MAX_SCTK_FAST_NODE_DIM
*
* The sctk_Torus_t is able to compute the neighbor
* of a given rank and also to compute a route
* using distance heuristic
*
*/
typedef struct sctk_Torus_t
{
	int node_count;            		/**< Number of nodes in the Torus */
	sctk_uint8_t dimension;               	/**< Torus dimension */
	sctk_uint8_t size_last_dimension;	/**< minimum number of ranks for the last dimension  */
	int node_regular; 			/**< node_count when it is a perfect torus */
	int node_left; 			 	/**< zero when it is a perfect torus */
	sctk_Node_t last_node;			/**< useful when it is an imperfect torus */
}sctk_Torus_t;

/**
* \brief Initialize a Torus or torus topology
* \param node_count number of nodes in the Torus (task count in MPI)
* \param dimension dimension of the Torus/torus ( from 1 to MAX_SCTK_FAST_NODE_DIM )
* This call can handle any non null node_count.
*
*/
void sctk_Torus_init ( int node_count, sctk_uint8_t dimension);


/**
* \brief Compute the distance between two Nodes
* \param a integer, the value of the coordinate of the first node
* \param b integer, the value of the coordinate of the second node
* \param sdim integer, the size of the current dimension
* \return distance beetween a and b
*
*/
inline int sctk_Torus_Node_distance (int a, int b ,unsigned sdim);

/**
* \brief Release a sctk_Torus_t
* After this call any call reffering the Torus
* is likely to fail ...
*
*/
void sctk_Torus_release ();

/**
* \brief search the next node for the route
* \param dest node destination of message
* \return the id of the nearest node
*/
int sctk_Torus_route_next(sctk_Node_t *dest);

/*
* Utilities
*/

/**
* \brief give a neighbour of a node
* \param unsigned int the dimension which is different between the cuurent node and its neighbour
* \param unsigned int 0 it's the "left" neigbour, 1 is the "right"
* \return the coordinate of the neighbour in the dimension i
*/
int sctk_Torus_neighbour_dimension(unsigned i,unsigned j);

/************************************************************************/
/* Rail Info                                                            */
/************************************************************************/

/** \brief Network dependent RAIL informations */
typedef union
{
	sctk_tcp_rail_info_t tcp;	/**< TCP Rail Info */
	sctk_ib_rail_info_t ib;		/**< IB Rail Info */
#ifdef MPC_USE_PORTAL
	sctk_portals_rail_info_t portals; /**< Portals Info */
#endif
}sctk_rail_info_spec_t;

/** This structure gathers all informations linked to a network rail
 * 
 *  All rails informations are stored in the sctk_route file 
 *  using the \ref sctk_route_set_rail_infos function
 */
struct sctk_rail_info_s
{
	/* Global Info */
	int rail_number; /**< ID of this rail */
	char* network_name; /**< Name of this rail */
	/* Network Infos */
	sctk_rail_info_spec_t network;	/**< Network dependent rail info */
	char* topology_name; /**< Name of the target topology */
	char on_demand;	/**< If the rail allows on demand-connexions */
	/* Configuration Info */
	struct sctk_runtime_config_struct_net_rail * runtime_config_rail; /**< Rail config */
	struct sctk_runtime_config_struct_net_driver_config * runtime_config_driver_config; /**< Driver config */
	
	/* HOOKS */
	
	/* Task Init and release */
	void (*finalize_task) (struct sctk_rail_info_s*);
	void (*initialize_task) (struct sctk_rail_info_s*);
	void (*initialize_leader_task) (struct sctk_rail_info_s*);
	/* Network interactions */
	void (*send_message) (sctk_thread_ptp_message_t *,struct sctk_rail_info_s*);
	void (*notify_recv_message) (sctk_thread_ptp_message_t * ,struct sctk_rail_info_s*);
	void (*notify_matching_message) (sctk_thread_ptp_message_t * ,struct sctk_rail_info_s*);
	void (*notify_perform_message) (int ,int, int, int, struct sctk_rail_info_s*);
	void (*notify_idle_message) (struct sctk_rail_info_s*);
	void (*notify_any_source_message) (int, int, struct sctk_rail_info_s*);
	int (*send_message_from_network) (sctk_thread_ptp_message_t * );
	/* Connection management */
	void(*connect_to)(int,int,sctk_rail_info_t*);
	void(*connect_from)(int,int,sctk_rail_info_t*);
	/* Routing */
	int (*route)(int , sctk_rail_info_t* );
	void (*route_init)(sctk_rail_info_t*);
};

void sctk_route_set_rail_nb(int nb);
void sctk_route_set_rail_infos(int rail, struct sctk_runtime_config_struct_net_rail * runtime_config_rail,
					 struct sctk_runtime_config_struct_net_driver_config * runtime_config_driver_config);
int sctk_route_get_rail_nb();
sctk_rail_info_t* sctk_route_get_rail(int i);

/************************************************************************/
/* Route Info                                                           */
/************************************************************************/

/** \brief Route type */
typedef enum
{
	ROUTE_ORIGIN_DYNAMIC  = 111, /**< Route has been created dynamically */
	ROUTE_ORIGIN_STATIC   = 222  /**< Route has been created statically */
} sctk_route_origin_t;

/** \brief Route lookup key */
typedef struct
{
	int destination;
	int rail;
}sctk_route_key_t;

/** \brief Network dependent ROUTE informations */
typedef union
{
	sctk_tcp_route_info_t tcp; /**< TCP route info */
	sctk_ib_data_t ib; /**< IB route info */
#ifdef MPC_USE_PORTALS
	sctk_portals_data_t portals; /**< Portals route info */
#endif
}sctk_route_info_spec_t;

/** \brief Route table entry
 *  Describes a network endpoint
 */
typedef struct sctk_route_table_s
{
	UT_hash_handle hh;
	sctk_route_key_t key;
	sctk_route_info_spec_t data;
	sctk_rail_info_t* rail;
	sctk_route_origin_t origin;       /**< Origin of the route entry: static or dynamic route */
	OPA_int_t state; 		  /**< State of the route */
	OPA_int_t low_memory_mode_local;  /**< If a message "out of memory" has already been sent to the
					   *   process to notice him that we are out of memory */
	OPA_int_t low_memory_mode_remote; /**< If the remote process is running out of memory */
	char is_initiator;		  /**< Return if the process is the initiator of the remote creation.
					   * if 'is_initiator == CHAR_MAX, value not set */
	sctk_spinlock_t lock;
} sctk_route_table_t;

#define ROUTE_LOCK(r) sctk_spinlock_lock(&(r)->lock)
#define ROUTE_UNLOCK(r) sctk_spinlock_unlock(&(r)->lock)
#define ROUTE_TRYLOCK(r) sctk_spinlock_trylock(&(r)->lock)

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_init_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);
void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);

/*THREAD SAFE use to add a route at compute time*/
void sctk_init_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);
void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);
struct sctk_route_table_s *sctk_route_dynamic_search(int dest, sctk_rail_info_t* rail);

sctk_route_table_t *sctk_route_dynamic_safe_add(int dest, sctk_rail_info_t* rail, sctk_route_table_t* (*create_func)(), void (*init_func)(int dest, sctk_rail_info_t* rail, sctk_route_table_t *route_table, int ondemand), int *added, char is_initiator);

char sctk_route_get_is_initiator(sctk_route_table_t * route_table);

/* For low_memory_mode */
int sctk_route_cas_low_memory_mode_local(sctk_route_table_t* tmp, int oldv, int newv);
int sctk_route_is_low_memory_mode_remote(sctk_route_table_t* tmp);
void sctk_route_set_low_memory_mode_remote(sctk_route_table_t* tmp, int low);
int sctk_route_is_low_memory_mode_local(sctk_route_table_t* tmp);
void sctk_route_set_low_memory_mode_local(sctk_route_table_t* tmp, int low);

/* Function for getting a route */
sctk_route_table_t* sctk_get_route(int dest, sctk_rail_info_t* rail);
sctk_route_table_t* sctk_get_route_to_process(int dest, sctk_rail_info_t* rail);
inline sctk_route_table_t* sctk_get_route_to_process_no_ondemand(int dest, sctk_rail_info_t* rail);
inline sctk_route_table_t* sctk_get_route_to_process_static(int dest, sctk_rail_info_t* rail);
sctk_route_table_t* sctk_get_route_to_process_no_route(int dest, sctk_rail_info_t* rail);

/* Routes */
void sctk_route_messages_send(int myself,int dest, specific_message_tag_t specific_message_tag, int tag, void* buffer,size_t size);
void sctk_route_messages_recv(int src, int myself,specific_message_tag_t specific_message_tag, int tag, void* buffer,size_t size);
void sctk_walk_all_routes( const sctk_rail_info_t* rail, void (*func) (const sctk_rail_info_t* rail,sctk_route_table_t* table) );

void sctk_route_init_in_rail(sctk_rail_info_t* rail, char* topology);
void sctk_route_finalize();
int sctk_route_is_finalized();

/************************************************************************/
/* On-Demand                                                            */
/************************************************************************/

/** \brief State of the QP */
typedef enum sctk_route_state_e 
{
  STATE_CONNECTED     = 111,
  STATE_FLUSHING      = 222,
  STATE_FLUSHING_CHECK= 233,
  STATE_FLUSHED       = 234,
  STATE_DECONNECTED   = 333,
  STATE_CONNECTING    = 666,
  STATE_RECONNECTING  = 444,
  STATE_RESET         = 555,
  STATE_RESIZING      = 777,
  STATE_REQUESTING      = 888,
} sctk_route_state_t;

__UNUSED__ static void sctk_route_set_state(sctk_route_table_t* tmp, sctk_route_state_t state){
  OPA_store_int(&tmp->state, state);
}

__UNUSED__ static int sctk_route_cas_state(sctk_route_table_t* tmp, sctk_route_state_t oldv,
  sctk_route_state_t newv ){
  return (int) OPA_cas_int(&tmp->state, oldv, newv);
}

__UNUSED__ static int sctk_route_get_state(sctk_route_table_t* tmp){
  return (int) OPA_load_int(&tmp->state);
}

/* Return the origin of a route entry: from dynamic or static allocation */
__UNUSED__ static sctk_route_origin_t sctk_route_get_origin(sctk_route_table_t *tmp) {
  return tmp->origin;
}

/* Signalization rails: getters and setters */
void sctk_route_set_signalization_rail(sctk_rail_info_t* rail);
sctk_rail_info_t* sctk_route_get_signalization_rail();



#endif

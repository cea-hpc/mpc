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


#include <uthash.h>
#include <math.h>
#include <sctk_ht.h>
#include <sctk_rail.h>

/************************************************************************/
/* ENDPOINTS                                                            */
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
} sctk_route_key_t;

/** \brief Network dependent ROUTE informations */
typedef union
{
	sctk_tcp_route_info_t tcp; /**< TCP route info */
#ifdef MPC_USE_INFINIBAND
	sctk_ib_route_info_t ib; /**< IB route info */
#endif
	sctk_shm_route_info_t shm;
#ifdef MPC_USE_PORTALS
	sctk_ptl_route_info_t ptl; /*< Portals route info */
#endif
} sctk_route_info_spec_t;



/** \brief State of the QP */
typedef enum sctk_route_state_e
{
    STATE_DECONNECTED   = 0,
    STATE_CONNECTED     = 111,
    STATE_FLUSHING      = 222,
    STATE_FLUSHING_CHECK = 233,
    STATE_FLUSHED       = 234,
    STATE_CONNECTING    = 666,
    STATE_RECONNECTING  = 444,
    STATE_RESET         = 555,
    STATE_RESIZING      = 777,
    STATE_REQUESTING      = 888,
} sctk_endpoint_state_t;

/** \brief Route table entry
 *  Describes a network endpoint
 */
struct sctk_endpoint_s
{
	int dest;
	sctk_route_info_spec_t data;
	sctk_rail_info_t *parent_rail;/**< Pointer to the parent rail (called by default if present) */
	sctk_rail_info_t *rail;	/**< Pointer to the rail owning this endpoint */
	int subrail_id; 		/**< The id of the subrail (if applicable otherwise -1) */
	sctk_route_origin_t origin;       /**< Origin of the route entry: static or dynamic route */
	OPA_int_t state; 		  /**< State of the route \ref sctk_endpoint_state_t */
	
	OPA_int_t low_memory_mode_local;  /**< If a message "out of memory" has already been sent to the
					   *   process to notice him that we are out of memory */
	OPA_int_t low_memory_mode_remote; /**< If the remote process is running out of memory */
	char is_initiator;		  /**< Return if the process is the initiator of the remote creation.
					   * if 'is_initiator == CHAR_MAX, value not set */
	
	sctk_spinlock_t lock;
};

#define ROUTE_LOCK(r) sctk_spinlock_lock(&(r)->lock)
#define ROUTE_UNLOCK(r) sctk_spinlock_unlock(&(r)->lock)
#define ROUTE_TRYLOCK(r) sctk_spinlock_trylock(&(r)->lock)

void sctk_endpoint_init( sctk_endpoint_t *tmp,  int dest, sctk_rail_info_t *rail, sctk_route_origin_t origin );
void sctk_endpoint_init_static ( sctk_endpoint_t *tmp, int dest, sctk_rail_info_t *rail );
void sctk_endpoint_init_dynamic ( sctk_endpoint_t *tmp, int dest, sctk_rail_info_t *rail );


char sctk_endpoint_get_is_initiator ( sctk_endpoint_t *route_table );

/* For low_memory_mode */
int sctk_endpoint_cas_low_memory_mode_local ( sctk_endpoint_t *tmp, int oldv, int newv );
int sctk_endpoint_is_low_memory_mode_remote ( sctk_endpoint_t *tmp );
void sctk_endpoint_set_low_memory_mode_remote ( sctk_endpoint_t *tmp, int low );
int sctk_endpoint_is_low_memory_mode_local ( sctk_endpoint_t *tmp );
void sctk_endpoint_set_low_memory_mode_local ( sctk_endpoint_t *tmp, int low );



__UNUSED__ static void sctk_endpoint_set_state ( sctk_endpoint_t *tmp, sctk_endpoint_state_t state )
{
	OPA_store_int ( &tmp->state, state );
}

__UNUSED__ static int sctk_endpoint_cas_state ( sctk_endpoint_t *tmp, sctk_endpoint_state_t oldv,
                                             sctk_endpoint_state_t newv )
{
	return ( int ) OPA_cas_int ( &tmp->state, oldv, newv );
}

__UNUSED__ static int sctk_endpoint_get_state ( sctk_endpoint_t *tmp )
{
	if( !tmp )
		return STATE_DECONNECTED;
	
	return ( int ) OPA_load_int ( &tmp->state );
}

/* Return the origin of a route entry: from dynamic or static allocation */
__UNUSED__ static sctk_route_origin_t sctk_endpoint_get_origin ( sctk_endpoint_t *tmp )
{
	return tmp->origin;
}


/************************************************************************/
/* Route Table                                                          */
/************************************************************************/

struct sctk_route_table_s
{
	/* Dynamic Routes */
	struct MPCHT dynamic_route_table; /** Here are stored the dynamic routes (hash table) */
	sctk_spin_rwlock_t dynamic_route_table_lock; /** This is the dynamic route lock */
	/* Static Routes */
	struct MPCHT static_route_table; /** Here are stored static routes (hash table) -- no lock as they are read only */
};


sctk_route_table_t * sctk_route_table_new();
void sctk_route_table_destroy(sctk_route_table_t*);
int sctk_route_table_empty(sctk_route_table_t *);
void sctk_route_table_clear(sctk_route_table_t** table);
/* Functions for adding a route */
void sctk_route_table_add_static_route (  sctk_route_table_t * table, sctk_endpoint_t *tmp, int push_in_multirail );
void sctk_route_table_add_dynamic_route (  sctk_route_table_t * table, sctk_endpoint_t *tmp, int push_in_multirail );
void sctk_route_table_add_dynamic_route_no_lock (  sctk_route_table_t * table, sctk_endpoint_t *tmp, int push_in_multirail );
/* Functions for getting a route */
sctk_endpoint_t * sctk_route_table_get_static_route(   sctk_route_table_t * table , int dest );
sctk_endpoint_t * sctk_route_table_get_dynamic_route(   sctk_route_table_t * table , int dest );
sctk_endpoint_t * sctk_route_table_get_dynamic_route_no_lock( sctk_route_table_t * table , int dest );

void sctk_walk_all_dynamic_routes ( sctk_route_table_t * table, void ( *func ) (  sctk_endpoint_t *endpoint, void * arg  ), void * arg );
void sctk_walk_all_static_routes( sctk_route_table_t * table, void ( *func ) (  sctk_endpoint_t *endpoint, void * arg  ), void * arg );

static inline void sctk_walk_all_routes ( sctk_route_table_t * table, void ( *func ) ( sctk_endpoint_t *endpoint, void * arg ), void * arg )
{
	sctk_walk_all_static_routes ( table, func, arg );
	sctk_walk_all_dynamic_routes ( table, func, arg );
}

/************************************************************************/
/* Signalization rails: getters and setters                             */
/************************************************************************/
/* Routes */
void sctk_route_messages_send ( int myself, int dest, sctk_message_class_t message_class, int tag, void *buffer, size_t size );
void sctk_route_messages_recv ( int src, int myself, sctk_message_class_t message_class, int tag, void *buffer, size_t size );


#endif

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

#include <sctk_rail.h>
#include <sctk_net_topology.h>

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
} sctk_route_key_t;

/** \brief Network dependent ROUTE informations */
typedef union
{
	sctk_tcp_route_info_t tcp; /**< TCP route info */
	sctk_ib_route_info_t ib; /**< IB route info */
#ifdef MPC_USE_PORTALS
	sctk_portals_data_t portals; /**< Portals route info */
#endif
} sctk_route_info_spec_t;

/** \brief Route table entry
 *  Describes a network endpoint
 */
typedef struct sctk_endpoint_s
{
	UT_hash_handle hh;
	sctk_route_key_t key;
	sctk_route_info_spec_t data;
	sctk_rail_info_t *rail;
	sctk_route_origin_t origin;       /**< Origin of the route entry: static or dynamic route */
	OPA_int_t state; 		  /**< State of the route */
	OPA_int_t low_memory_mode_local;  /**< If a message "out of memory" has already been sent to the
					   *   process to notice him that we are out of memory */
	OPA_int_t low_memory_mode_remote; /**< If the remote process is running out of memory */
	char is_initiator;		  /**< Return if the process is the initiator of the remote creation.
					   * if 'is_initiator == CHAR_MAX, value not set */
	sctk_spinlock_t lock;
} sctk_endpoint_t;

#define ROUTE_LOCK(r) sctk_spinlock_lock(&(r)->lock)
#define ROUTE_UNLOCK(r) sctk_spinlock_unlock(&(r)->lock)
#define ROUTE_TRYLOCK(r) sctk_spinlock_trylock(&(r)->lock)

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_init_static_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail );
void sctk_add_static_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail );

/*THREAD SAFE use to add a route at compute time*/
void sctk_init_dynamic_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail );
void sctk_add_dynamic_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail );
struct sctk_endpoint_s *sctk_route_dynamic_search ( int dest, sctk_rail_info_t *rail );

sctk_endpoint_t *sctk_route_dynamic_safe_add ( int dest, sctk_rail_info_t *rail, sctk_endpoint_t * ( *create_func ) (), void ( *init_func ) ( int dest, sctk_rail_info_t *rail, sctk_endpoint_t *route_table, int ondemand ), int *added, char is_initiator );

char sctk_route_get_is_initiator ( sctk_endpoint_t *route_table );

/* For low_memory_mode */
int sctk_route_cas_low_memory_mode_local ( sctk_endpoint_t *tmp, int oldv, int newv );
int sctk_route_is_low_memory_mode_remote ( sctk_endpoint_t *tmp );
void sctk_route_set_low_memory_mode_remote ( sctk_endpoint_t *tmp, int low );
int sctk_route_is_low_memory_mode_local ( sctk_endpoint_t *tmp );
void sctk_route_set_low_memory_mode_local ( sctk_endpoint_t *tmp, int low );

/* Function for getting a route */
sctk_endpoint_t *sctk_get_route ( int dest, sctk_rail_info_t *rail );
sctk_endpoint_t *sctk_get_route_to_process ( int dest, sctk_rail_info_t *rail );
inline sctk_endpoint_t *sctk_get_route_to_process_no_ondemand ( int dest, sctk_rail_info_t *rail );
inline sctk_endpoint_t *sctk_get_route_to_process_static ( int dest, sctk_rail_info_t *rail );
sctk_endpoint_t *sctk_get_route_to_process_no_routing ( int dest, sctk_rail_info_t *rail );

/* Routes */
void sctk_route_messages_send ( int myself, int dest, specific_message_tag_t specific_message_tag, int tag, void *buffer, size_t size );
void sctk_route_messages_recv ( int src, int myself, specific_message_tag_t specific_message_tag, int tag, void *buffer, size_t size );
void sctk_walk_all_routes ( const sctk_rail_info_t *rail, void ( *func ) ( const sctk_rail_info_t *rail, sctk_endpoint_t *table ) );


/************************************************************************/
/* On-Demand                                                            */
/************************************************************************/

/** \brief State of the QP */
typedef enum sctk_route_state_e
{
    STATE_CONNECTED     = 111,
    STATE_FLUSHING      = 222,
    STATE_FLUSHING_CHECK = 233,
    STATE_FLUSHED       = 234,
    STATE_DECONNECTED   = 333,
    STATE_CONNECTING    = 666,
    STATE_RECONNECTING  = 444,
    STATE_RESET         = 555,
    STATE_RESIZING      = 777,
    STATE_REQUESTING      = 888,
} sctk_route_state_t;

__UNUSED__ static void sctk_route_set_state ( sctk_endpoint_t *tmp, sctk_route_state_t state )
{
	OPA_store_int ( &tmp->state, state );
}

__UNUSED__ static int sctk_route_cas_state ( sctk_endpoint_t *tmp, sctk_route_state_t oldv,
                                             sctk_route_state_t newv )
{
	return ( int ) OPA_cas_int ( &tmp->state, oldv, newv );
}

__UNUSED__ static int sctk_route_get_state ( sctk_endpoint_t *tmp )
{
	return ( int ) OPA_load_int ( &tmp->state );
}

/* Return the origin of a route entry: from dynamic or static allocation */
__UNUSED__ static sctk_route_origin_t sctk_route_get_origin ( sctk_endpoint_t *tmp )
{
	return tmp->origin;
}

/* Signalization rails: getters and setters */
void sctk_route_set_signalization_rail ( sctk_rail_info_t *rail );
sctk_rail_info_t *sctk_route_get_signalization_rail();



#endif

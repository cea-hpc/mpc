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
#ifndef SCTK_RAIL_H
#define SCTK_RAIL_H

#include <sctk_inter_thread_comm.h>
#include <sctk_runtime_config.h>

/* Forward struct declarations */
typedef struct sctk_rail_info_s sctk_rail_info_t;
typedef struct sctk_route_table_s sctk_route_table_t; 
typedef struct sctk_endpoint_s sctk_endpoint_t;

/* Driver definitions */
#include <sctk_drivers.h>

/************************************************************************/
/* Rail Info                                                            */
/************************************************************************/

/** \brief Network dependent RAIL informations */
typedef union
{
	sctk_tcp_rail_info_t tcp;	/**< TCP Rail Info */
	sctk_ib_rail_info_t ib;	/**< IB Rail Info */
#ifdef MPC_USE_PORTAL
	sctk_portals_rail_info_t portals; /**< Portals Info */
#endif
} sctk_rail_info_spec_t;


/************************************************************************/
/* Rail                                                                 */
/************************************************************************/

/** This structure gathers all informations linked to a network rail
 *
 *  All rails informations are stored in the sctk_route file
 *  using the \ref sctk_route_set_rail_infos function
 */
struct sctk_rail_info_s
{
	/* Global Info */
	int rail_number; /**< ID of this rail */
	char *network_name; /**< Name of this rail */
	
	/* Network Infos */
	sctk_rail_info_spec_t network;	/**< Network dependent rail info */
	char *topology_name; /**< Name of the target topology */
	char on_demand;	/**< If the rail allows on demand-connexions */
	
	/* Configuration Info */
	struct sctk_runtime_config_struct_net_rail *runtime_config_rail;  /**< Rail config */
	struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config;  /**< Driver config */

	/* Route table */
	sctk_route_table_t * route_table;

	/* HOOKS */

	/* Task Init and release */
	void ( *finalize_task ) ( struct sctk_rail_info_s * );
	void ( *initialize_task ) ( struct sctk_rail_info_s * );
	void ( *initialize_leader_task ) ( struct sctk_rail_info_s * );
	/* Network interactions */
	void ( *send_message ) ( sctk_thread_ptp_message_t *, struct sctk_rail_info_s * );
	
	void ( *send_message_endpoint ) ( sctk_thread_ptp_message_t *, sctk_endpoint_t * );
	
	void ( *notify_recv_message ) ( sctk_thread_ptp_message_t * , struct sctk_rail_info_s * );
	void ( *notify_matching_message ) ( sctk_thread_ptp_message_t * , struct sctk_rail_info_s * );
	void ( *notify_perform_message ) ( int , int, int, int, struct sctk_rail_info_s * );
	void ( *notify_idle_message ) ( struct sctk_rail_info_s * );
	void ( *notify_any_source_message ) ( int, int, struct sctk_rail_info_s * );
	
	int ( *send_message_from_network ) ( sctk_thread_ptp_message_t * );
	sctk_endpoint_t * ( *on_demand_connection ) ( struct sctk_rail_info_s * rail , int dest );
	
	
	void (*control_message_handler)( struct sctk_rail_info_s * rail, int source_process, int source_rank, char subtype, char param, void * data );
	
	/* Connection management */
	
	void ( *connect_to ) ( int, int, sctk_rail_info_t * );
	void ( *connect_from ) ( int, int, sctk_rail_info_t * );
	
	/* Routing */
	
	int ( *route ) ( int , sctk_rail_info_t * );
	void ( *route_init ) ( sctk_rail_info_t * );
};

/* Rail  Array                                                          */


/** This structure gathers all rails in an array */
struct sctk_rail_array
{
	/** Dynamic array storing rails */
	sctk_rail_info_t *rails ;
	/** Number of rails */
	int rail_number;
	int rail_current_id;
	/** Set to 1 when routes have been committed by a call to \ref sctk_rail_commit */
	int rails_committed;
};




void sctk_rail_allocate ( int count );

sctk_rail_info_t *sctk_rail_new ( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                  struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config );
int sctk_rail_count();
sctk_rail_info_t * sctk_rail_get_by_id ( int i );
void sctk_rail_commit();
int sctk_rail_committed();
void sctk_rail_init_route ( sctk_rail_info_t *rail, char *topology );
void sctk_rail_dump_routes();

/************************************************************************/
/* Add Routes to Rail                                                   */
/************************************************************************/

void sctk_rail_add_static_route ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp  );
void sctk_rail_add_dynamic_route ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp );
sctk_endpoint_t * sctk_rail_add_or_reuse_route_dynamic ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t * ( *create_func ) (), void ( *init_func ) ( int dest, sctk_rail_info_t *rail, sctk_endpoint_t *route_table, int ondemand ), int *added, char is_initiator );

/************************************************************************/
/* Get Routes From RAIL                                                 */
/************************************************************************/

sctk_endpoint_t * sctk_rail_get_static_route_to_process ( sctk_rail_info_t *rail, int dest );
sctk_endpoint_t * sctk_rail_get_any_route_to_process (  sctk_rail_info_t *rail, int dest );
sctk_endpoint_t * sctk_rail_get_dynamic_route_to_process ( sctk_rail_info_t *rail, int dest );
sctk_endpoint_t * sctk_rail_get_any_route_to_task_or_on_demand ( sctk_rail_info_t *rail, int dest );
sctk_endpoint_t * sctk_rail_get_any_route_to_process_or_forward ( sctk_rail_info_t *rail, int dest );
sctk_endpoint_t * sctk_rail_get_static_route_to_process_or_forward (  sctk_rail_info_t *rail,  int dest );
sctk_endpoint_t * sctk_rail_get_any_route_to_process_or_on_demand ( sctk_rail_info_t *rail, int dest );


#endif /* SCTK_RAIL_H */

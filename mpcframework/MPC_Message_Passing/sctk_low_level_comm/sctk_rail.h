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
#include "sctk_device_topology.h"
#include "sctk_topological_polling.h"
#include "sctk_types.h"

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
#ifdef MPC_USE_PORTALS
	sctk_portals_rail_info_t portals; /**< Portals Info */
#endif
	sctk_topological_rail_info_t topological; /**< Topological rail info */
} sctk_rail_info_spec_t;

/************************************************************************/
/* Rail Pin CTX                                                         */
/************************************************************************/

#ifdef MPC_USE_INFINIBAND

#include "sctk_ib_mmu.h"
#include <infiniband/verbs.h>

struct sctk_rail_ib_pin_ctx
{
	struct ibv_mr mr;
	sctk_ib_mmu_entry_t * p_entry;
};
#endif

#ifdef MPC_USE_PORTAL
struct sctk_rail_portals_pin_ctx
{
	
};
#endif /* MPC_USE_PORTAL */

typedef union
{
#ifdef MPC_USE_INFINIBAND
	struct sctk_rail_ib_pin_ctx ib;
#endif
#ifdef MPC_USE_PORTAL
	struct sctk_rail_portals_pin_ctx portals;
#endif /* MPC_USE_PORTAL */
}sctk_rail_pin_ctx_internal_t;

struct sctk_rail_pin_ctx_list
{
	sctk_rail_pin_ctx_internal_t pin;
	int rail_id;
};

#define SCTK_PIN_LIST_SIZE 25

typedef struct
{
	struct sctk_rail_pin_ctx_list list[SCTK_PIN_LIST_SIZE];
	int size;
}sctk_rail_pin_ctx_t;

void sctk_rail_pin_ctx_init( sctk_rail_pin_ctx_t * ctx, void * addr, size_t size );
void sctk_rail_pin_ctx_release( sctk_rail_pin_ctx_t * ctx );

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
	int subrail_id; /**< ID of this rail if it is a subrail (-1 otherwise) */
	int priority; /** Priority of this rail */
	char *network_name; /**< Name of this rail */
	sctk_device_t * rail_device; /**< Device associated with the rail */
	
	struct sctk_rail_info_s * parent_rail; /**< This is used for rail hierarchies 
	                                            (note that parent initializes it for the child) */
	struct sctk_rail_info_s ** subrails;   /**< Pointer to subrails (if present) */
	int subrail_count;   /**< number of subrails */
	
	/* Network Infos */
	sctk_rail_info_spec_t network;	/**< Network dependent rail info */
	char *topology_name; /**< Name of the target topology */
	int requires_bootstrap_ring; /**< This flag is set to 0 when the rail does not need the initial rin
	                                  case of the "none" topology */
	char on_demand;	/**< If the rail allows on demand-connexions */
	char is_rdma;        /**< If the rail supports RDMA operations */
	
	/* Configuration Info */
	struct sctk_runtime_config_struct_net_rail *runtime_config_rail;  /**< Rail config */
	struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config;  /**< Driver config */

	/* Route table */
	sctk_route_table_t * route_table;

	/* Polling mechanism */
	struct sctk_topological_polling_tree idle_polling_tree;
	struct sctk_topological_polling_tree any_source_polling_tree;

	/* HOOKS */

	/* Task Init and release */
	void ( *finalize_task ) ( struct sctk_rail_info_s * );
	void ( *initialize_task ) ( struct sctk_rail_info_s * );
	void ( *initialize_leader_task ) ( struct sctk_rail_info_s * );
	
	/* Network interactions */
	void ( *send_message_endpoint ) ( sctk_thread_ptp_message_t *, sctk_endpoint_t * );
	
	void ( *notify_recv_message ) ( sctk_thread_ptp_message_t * , struct sctk_rail_info_s * );
	void ( *notify_matching_message ) ( sctk_thread_ptp_message_t * , struct sctk_rail_info_s * );
	void ( *notify_perform_message ) ( int , int, int, int, struct sctk_rail_info_s * );
	void ( *notify_idle_message ) ( struct sctk_rail_info_s * );
	void ( *notify_any_source_message ) ( int, int, struct sctk_rail_info_s * );
	
	int ( *send_message_from_network ) ( sctk_thread_ptp_message_t * );
	void ( *connect_on_demand ) ( struct sctk_rail_info_s * rail , int dest );
	
	void (*control_message_handler)( struct sctk_rail_info_s * rail, int source_process, int source_rank, char subtype, char param, void * data, size_t size );
	
	/* RDMA Ops */
	
	int (*rdma_fetch_and_op_gate)( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type );
	
	
	void (*rdma_fetch_and_op)(  sctk_rail_info_t *rail,
							    sctk_thread_ptp_message_t *msg,
							    void * fetch_addr,
							    struct  sctk_rail_pin_ctx_list * local_key,
							    void * remote_addr,
							    struct  sctk_rail_pin_ctx_list * remote_key,
							    void * add,
							    RDMA_op op,
							    RDMA_type type );


	
	
	int (*rdma_cas_gate)( sctk_rail_info_t *rail, size_t size, RDMA_type type );

	void (*rdma_cas)(   sctk_rail_info_t *rail,
						  sctk_thread_ptp_message_t *msg,
						  void *  res_addr,
						  struct  sctk_rail_pin_ctx_list * local_key,
						  void * remote_addr,
						  struct  sctk_rail_pin_ctx_list * remote_key,
						  void * comp,
						  void * new,
						  RDMA_type type );
	
	void (*rdma_write)(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
                         void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
                         size_t size );
	
	void (*rdma_read)(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
                         void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
                         size_t size );
	
	/* Pinning */
	void (*rail_pin_region)( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size );
	void (*rail_unpin_region)( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list );

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
	/** Id of the RDMA rail */
	int rdma_rail;
};




void sctk_rail_allocate ( int count );

sctk_rail_info_t *sctk_rail_register( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                  struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config );
int sctk_rail_count();
sctk_rail_info_t * sctk_rail_get_by_id ( int i );
int sctk_rail_get_rdma_id();
sctk_rail_info_t * sctk_rail_get_rdma ();
void sctk_rail_commit();
int sctk_rail_committed();
void sctk_rail_init_route ( sctk_rail_info_t *rail, char *topology, void (*on_demand)( struct sctk_rail_info_s * rail , int dest ) );
void sctk_rail_dump_routes();

/** Retrieve the HWLOC device associated with a rail */
static inline hwloc_obj_t sctk_rail_get_device_hwloc_obj( sctk_rail_info_t *rail )
{
	if( !rail->rail_device )
		return NULL;
	
	return rail->rail_device->obj;
}

/** Return the name of the device (as put in the config) */
static inline char * sctk_rail_get_device_name( sctk_rail_info_t *rail )
{
	if( !rail )
		return NULL;
	
	return rail->runtime_config_rail->device;
}

/** Returns 1 if the rail is based on a regexp */
static inline int sctk_rail_device_is_regexp( sctk_rail_info_t *rail )
{
	char * dev = sctk_rail_get_device_name( rail );
	
	if( !dev )
		return 0;
	
	if( dev[0] == '!' )
		return 1;
	
	return 0;
}

/** Returns the id of the subrail using this device -1 otherwise */
static inline int sctk_rail_get_subrail_id_with_device( sctk_rail_info_t *rail, sctk_device_t * dev )
{
	if( ! rail )
		return -1;
	
	if( rail->subrail_count == 0 )
		return -1;
	
	int i;
	
	for( i = 0 ; i < rail->subrail_count ; i++ )
	{
		if( dev == rail->subrails[i]->rail_device )
		{
			return i;
		}
	}
	
	return -1;
}

/************************************************************************/
/* Add Routes to Rail                                                   */
/************************************************************************/

void sctk_rail_add_static_route ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp  );
void sctk_rail_add_dynamic_route ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp );
void sctk_rail_add_dynamic_route_no_lock (  sctk_rail_info_t * rail, sctk_endpoint_t *tmp );

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

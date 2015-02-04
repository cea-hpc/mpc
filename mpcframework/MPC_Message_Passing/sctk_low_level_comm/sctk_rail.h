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

/* Used inside netork dependent rail info */
typedef struct sctk_rail_info_s sctk_rail_info_t;

/* Networks */
#include <sctk_portals.h>
#include <sctk_tcp.h>
#include <sctk_ib.h>


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
} sctk_rail_info_spec_t;

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

	/* HOOKS */

	/* Task Init and release */
	void ( *finalize_task ) ( struct sctk_rail_info_s * );
	void ( *initialize_task ) ( struct sctk_rail_info_s * );
	void ( *initialize_leader_task ) ( struct sctk_rail_info_s * );
	/* Network interactions */
	void ( *send_message ) ( sctk_thread_ptp_message_t *, struct sctk_rail_info_s * );
	void ( *notify_recv_message ) ( sctk_thread_ptp_message_t * , struct sctk_rail_info_s * );
	void ( *notify_matching_message ) ( sctk_thread_ptp_message_t * , struct sctk_rail_info_s * );
	void ( *notify_perform_message ) ( int , int, int, int, struct sctk_rail_info_s * );
	void ( *notify_idle_message ) ( struct sctk_rail_info_s * );
	void ( *notify_any_source_message ) ( int, int, struct sctk_rail_info_s * );
	int ( *send_message_from_network ) ( sctk_thread_ptp_message_t * );
	/* Connection management */
	void ( *connect_to ) ( int, int, sctk_rail_info_t * );
	void ( *connect_from ) ( int, int, sctk_rail_info_t * );
	/* Routing */
	int ( *route ) ( int , sctk_rail_info_t * );
	void ( *route_init ) ( sctk_rail_info_t * );
};

void sctk_rail_allocate ( int count );
sctk_rail_info_t *sctk_rail_new ( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                         struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config );
int sctk_rail_count();
sctk_rail_info_t * sctk_rail_get_by_id ( int i );
void sctk_rail_commit();
int sctk_rail_committed();


void sctk_rail_init_route ( sctk_rail_info_t *rail, char *topology );


#endif /* SCTK_RAIL_H */

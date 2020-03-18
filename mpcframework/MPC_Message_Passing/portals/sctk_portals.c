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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#include <sctk_debug.h>
#include "sctk_rail.h"
#include "sctk_ptl_toolkit.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_rdma.h"
#include "sctk_route.h"

static volatile short rail_is_ready = 0;

/**
 * @brief Main entry point for sending message (called by higher layers)
 *
 * @param[in,out] msg message to send
 * @param[in]  endpoint connection handler to take for routing
 */
static void sctk_network_send_message_endpoint_ptl ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
	sctk_ptl_send_message(msg, endpoint);
}

/**
 * Callback used to notify the driver a new 'recv' message has been locally posted.
 * \param[in] msg the posted msdg
 * \param[in] rail the Portals rail
 */
static void sctk_network_notify_recv_message_ptl ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
	/* by construction, a network-received CM will generate a local recv
	 * So, in this case, we have nothing to do here
	 */
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		return;
	}

	/* in any other case... */
	sctk_ptl_notify_recv(msg, rail);
}

/**
 * Not relevant in this implementation */
static void sctk_network_notify_matching_message_ptl ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
	/* nothing to do */
	UNUSED(msg);
	UNUSED(rail);
}

/** Not relevant in this implementation */
static void sctk_network_notify_perform_message_ptl ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
	/* nothing to do */
	UNUSED(remote);
	UNUSED(remote_task_id);
	
	UNUSED(polling_task_id);
	UNUSED(blocking);
	UNUSED(rail);
}

/**
 * @brief NOTIFIER : routine called by idle task (hierarchical election to avoid contention)
 *
 * @param[in,out] rail driver data from current network
 */
static void sctk_network_notify_idle_message_ptl (sctk_rail_info_t* rail)
{
	/* '5' has been chosen arbitrarly for now */
	sctk_ptl_eqs_poll( rail, 5 );
	sctk_ptl_mds_poll( rail, 5 );
}

/**
 * @brief NOTIFIER : routine called when an any_source message is arrived
 *
 * @param[in] polling_task_id not used by Portals implementation
 * @param[in] blocking not used by Portals implementation
 * @param[in,out] rail associated rail
 */
static void sctk_network_notify_any_source_message_ptl ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
	UNUSED(polling_task_id);
	UNUSED(blocking);
	UNUSED(rail);
}

static void sctk_network_notify_probe_message_ptl (sctk_rail_info_t* rail, sctk_thread_message_header_t* hdr, int *status)
{
	*status = sctk_ptl_pending_me_probe(rail, hdr, SCTK_PTL_ME_PROBE_ONLY);
}

/**
 * Notify the driver a new MPI communicator has been created.
 * Becasue Portals driver has been split according to MPI_Comm semantics, each 
 * new communicator creation has to trigger a new PT entry creation.
 * \param[in] rail the Portals rail
 * \param[in] comm_idx the communicator ID
 * \param[in] comm_size number of processes in this comm
 */
static void sctk_network_notify_new_communicator_ptl(sctk_rail_info_t* rail, int comm_idx, size_t comm_size)
{
	sctk_ptl_comm_register(&rail->network.ptl, comm_idx, comm_size);
}

/**
 * @brief Routine called just before a message is forwarded to higher layer (sctk_inter_thread_comm)
 *
 * @param[in,out] msg message to forward
 *
 * @returns 1, in any case
 */
static int sctk_send_message_from_network_ptl ( sctk_thread_ptp_message_t *msg )
{
	if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
	{
		/* No reordering */
		sctk_send_message_try_check ( msg, 1 );
	}

	return 1;
}

/** 
 * Function called globally when C/R is enabled, to close the rail before checkpointing.
 * \param[in] rail the Portals rail to shut down.
 */
void sctk_network_finalize_ptl(sctk_rail_info_t* rail)
{
	sctk_ptl_fini_interface(rail);
}

/**
 * Function called at task scope, before closing the rail.
 * \param[in] rail the Portals rail
 * \param[in] taskid the Task ID to stop
 * \param[in] vp the VP the task is bound
 */
void sctk_network_finalize_task_ptl(sctk_rail_info_t* rail, int taskid, int vp)
{
	UNUSED(rail);
	UNUSED(taskid);
	UNUSED(vp);
}

/**
 * Function called at task scope, to re-init the Portals context for each task.
 * \param[in] rail the Portals rail
 * \param[in] taskid the Task ID to restart
 * \param[in] vp the VP the task is bound
 */
void sctk_network_initialize_task_ptl(sctk_rail_info_t* rail, int taskid, int vp)
{
	UNUSED(rail);
	UNUSED(taskid);
	UNUSED(vp);
}

/**
 * Proceed to establish a connection to a given destination.
 * \param[in] rail the route owner
 * \param[in] dest the remote process id
 */
static void sctk_network_connect_on_demand_ptl ( struct sctk_rail_info_s * rail , int dest )
{
	sctk_ptl_id_t id = sctk_ptl_map_id(rail, dest);
	sctk_ptl_add_route(dest, id, rail, ROUTE_ORIGIN_DYNAMIC, STATE_CONNECTED);
}

/**
 * Handle incoming messages, flagged as control_messages.
 * Portals does not need (for now) to handle rail-specific control-messages.
 * \param[in] rail the rail owner
 * \param[in] source_process the process that requested the CM
 * \param[in] source_rank the task requesting the CM (can be -1 if not a MPI task)
 * \param[in] subtype type of CM
 * \param[in] param
 * \param[in] data the payload embedded w/ the CM
 * \param[in] size datatype's size
 */
static void sctk_network_cm_handler_ptl( struct sctk_rail_info_s * rail, int source_process, int source_rank, char subtype, char param, void * data, size_t size )
{
	UNUSED(rail);
	UNUSED(source_process);
	UNUSED(source_rank);
	UNUSED(subtype);
	UNUSED(param);
	UNUSED(data);
	UNUSED(size);
	not_implemented();
}

/**
 * Nothing to do here for Portals.
 * Because we suppose that a connect_to maps with a connect_from during the
 * first topology setup, we only have to support one end (here, connect_from().
 * The other and will respond through control messages
 */
static void sctk_network_connect_to_ptl( int from, int to, sctk_rail_info_t * rail )
{
	/* nothing to do */
	UNUSED(from);
	UNUSED(to);
	UNUSED(rail);
}

/**
 * Simply create a route to a given destination by using On-demand support.
 * \see sctk_network_connect_on_demand_ptl.
 * \param[in] from the process id initiating the request
 * \param[in] to the targeted process
 * \param[in] rail the forthcoming route owner.
 */
static void sctk_network_connect_from_ptl( int from, int to, sctk_rail_info_t * rail)
{
	UNUSED(from);
	sctk_network_connect_on_demand_ptl(rail, to);
}

/************ INIT ****************/
/**
 * Entry point to initialize a Portals rail.
 *
 * \param[in,out] rail
 */
void sctk_network_init_ptl (sctk_rail_info_t *rail)
{
	if(sctk_rail_count() > 1 && sctk_get_process_rank() == 0)
	{
		sctk_warning("This Portals 4 process-based driver is not suited for multi-rail usage.");
		sctk_warning("Please do not consider using more than one rail to avoid memory leaks.");
	}
	/* just select the type of init for this rail (ring,full..), nothing more */
	sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, NULL );
	rail->network_name                 = "Portals Process-Based optimization";

	/* Register msg hooks in rail */
	rail->send_message_endpoint     = sctk_network_send_message_endpoint_ptl;
	rail->notify_recv_message       = sctk_network_notify_recv_message_ptl;
	rail->notify_matching_message   = sctk_network_notify_matching_message_ptl;
	rail->notify_perform_message    = sctk_network_notify_perform_message_ptl;
	rail->notify_idle_message       = sctk_network_notify_idle_message_ptl;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_ptl;
	rail->notify_new_comm           = sctk_network_notify_new_communicator_ptl;
	rail->send_message_from_network = sctk_send_message_from_network_ptl;
	rail->notify_probe_message      = sctk_network_notify_probe_message_ptl;

	/* RDMA */
	rail->rail_pin_region        = sctk_ptl_pin_region;
	rail->rail_unpin_region      = sctk_ptl_unpin_region;
	rail->rdma_write             = sctk_ptl_rdma_write;
	rail->rdma_read              = sctk_ptl_rdma_read;
	rail->rdma_fetch_and_op_gate = sctk_ptl_rdma_fetch_and_op_gate;
	rail->rdma_fetch_and_op      = sctk_ptl_rdma_fetch_and_op;
	rail->rdma_cas_gate          = sctk_ptl_rdma_cas_gate;
	rail->rdma_cas               = sctk_ptl_rdma_cas;

	/* rail closing/re-opening calls */
	rail->driver_finalize         = sctk_network_finalize_ptl;
	rail->finalize_task           = sctk_network_finalize_task_ptl;
	rail->initialize_task         = sctk_network_initialize_task_ptl;
	rail->connect_to              = sctk_network_connect_to_ptl;
	rail->connect_from            = sctk_network_connect_from_ptl;
	rail->connect_on_demand       = sctk_network_connect_on_demand_ptl;
	rail->control_message_handler = sctk_network_cm_handler_ptl;

	sctk_ptl_init_interface( rail );


	if(rail->requires_bootstrap_ring)
		sctk_ptl_create_ring( rail );

	rail_is_ready = 1;
	sctk_info("rank %d mapped to Portals ID (nid/pid): %llu/%llu", sctk_get_process_rank(), rail->network.ptl.id.phys.nid, rail->network.ptl.id.phys.pid);
}

#endif

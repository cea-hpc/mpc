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
/* #   - GONCALVES Thomas thomas.goncalves@cea.fr                         # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_portals.h>
#include <sctk_portals_rdma.h>

static volatile short rail_is_ready = 0;

/**
 * @brief Main entry point for sending message (called by higher layers)
 *
 * @param msg message to send
 * @param endpoint connection handler to take for routing
 */
static void sctk_network_send_message_endpoint_portals ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
    sctk_portals_send_put(endpoint, msg);
}

static void sctk_network_notify_recv_message_portals ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
    sctk_portals_polling_queue_for(rail, SCTK_MSG_DEST_PROCESS(msg) % rail->network.portals.ptable.nb_entries);
}

static void sctk_network_notify_matching_message_portals ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}

static void sctk_network_notify_perform_message_portals ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
}

/**
 * @brief NOTIFIER : routine called by idle task (hierarchical election to avoid contention)
 *
 * @param rail driver data from current network
 */
static void sctk_network_notify_idle_message_portals (sctk_rail_info_t* rail)
{
	if(!rail_is_ready) return;

	size_t mytask = sctk_get_task_rank() % rail->network.portals.ptable.nb_entries;

	// check if current task and neighbors have pending message. If not, poll the entire portals table
	if(sctk_portals_polling_queue_for(rail, mytask)){
		sctk_portals_polling_queue_for(rail, SCTK_PORTALS_POLL_ALL);
	}

	//progress RDMA messages queue
	sctk_portals_poll_one_queue(rail, rail->network.portals.ptable.nb_entries);

	//progress special messages queue
	sctk_portals_poll_one_queue(rail, rail->network.portals.ptable.nb_entries+1);

	//check pending requests
	if(sctk_spinlock_trylock(&rail->network.portals.ptable.pending_list.msg_lock) == 0){
		sctk_portals_poll_pending_msg_list(rail);
		sctk_spinlock_unlock(&rail->network.portals.ptable.pending_list.msg_lock);
	}
}

/**
 * @brief NOTIFIER : routine called when an any_source message is arrived
 *
 * @param polling_task_id
 * @param blocking
 * @param rail
 */
static void sctk_network_notify_any_source_message_portals ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
	//need to poll all messages entries to find any_source's target
	sctk_portals_polling_queue_for(rail, SCTK_PORTALS_POLL_ALL);
}

/**
 * @brief Routine called just before a message is forwarded to higher layer (sctk_inter_thread_comm)
 *
 * @param msg message to forward
 *
 * @return
 */
static int sctk_send_message_from_network_portals ( sctk_thread_ptp_message_t *msg )
{
    if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
    {
        /* No reordering */
        sctk_send_message_try_check ( msg, 1 );
    }

	return 1;
}


/************ INIT ****************/
/**
 * @brief unique initialization for this rail (already created and managed by multirail handler)
 *
 * @param rail
 */
void sctk_network_init_portals (sctk_rail_info_t *rail)
{
    /* Register hooks in rail */
    rail->send_message_endpoint = sctk_network_send_message_endpoint_portals;
    rail->notify_recv_message = sctk_network_notify_recv_message_portals;
    rail->notify_matching_message = sctk_network_notify_matching_message_portals;
    rail->notify_perform_message = sctk_network_notify_perform_message_portals;
    rail->notify_idle_message = sctk_network_notify_idle_message_portals;
    rail->notify_any_source_message = sctk_network_notify_any_source_message_portals;
    rail->send_message_from_network = sctk_send_message_from_network_portals;

    /* PIN */
    rail->rail_pin_region = sctk_portals_pin_region;
    rail->rail_unpin_region = sctk_portals_unpin_region;

    /* RDMA */
    rail->rdma_write = sctk_portals_rdma_write;
    rail->rdma_read = sctk_portals_rdma_read;

    rail->rdma_fetch_and_op_gate = sctk_portals_rdma_fetch_and_op_gate;
    rail->rdma_fetch_and_op = sctk_portals_rdma_fetch_and_op;
    rail->rdma_cas_gate = sctk_portals_rdma_cas_gate;
    rail->rdma_cas = sctk_portals_rdma_cas;

    rail->network_name = "PORTALS";

    sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, sctk_portals_on_demand_connection_handler );
    sctk_network_init_portals_all ( rail );
	rail_is_ready = 1;
}

#endif

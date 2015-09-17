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
/* #   - ADAM Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_portals.h>

/************ INTER_THEAD_COMM HOOOKS ****************/
//TODO: Refactor and extract portals routine into sctk_portals_toolkit.c
static void sctk_network_send_message_endpoint_portals ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
	sctk_portals_send_put(endpoint, msg);
}

static void sctk_network_notify_recv_message_portals ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}

static void sctk_network_notify_matching_message_portals ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}

static void sctk_network_notify_perform_message_portals ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
}

static void sctk_network_notify_idle_message_portals (sctk_rail_info_t* rail) //plus de calcul,blocage
{
	size_t mytask = sctk_get_task_rank() % rail->network.portals.ptable.nb_entries;

	if( ! sctk_portals_polling_queue_for(rail, mytask)){
		sctk_portals_polling_queue_for(rail, SCTK_PORTALS_POLL_ALL);
	}
}

static void sctk_network_notify_any_source_message_portals ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
	sctk_portals_polling_queue_for(rail, SCTK_PORTALS_POLL_ALL);
}

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
    rail->network_name = "PORTALS";

    sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, sctk_portals_on_demand_connection_handler );
    sctk_network_init_portals_all ( rail );
}

#endif

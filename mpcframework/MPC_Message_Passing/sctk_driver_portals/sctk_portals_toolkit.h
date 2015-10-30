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
/* # had knowledge of the CeCILL-C license and tt you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - GONCALVES Thomas thomas.goncalves@cea.fr                         # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PORTALS_TOOLKIT_H_
#define __SCTK_PORTALS_TOOLKIT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_portals_helper.h>

struct sctk_endpoint_s;
struct sctk_rail_info_s;
struct sctk_thread_ptp_message_s;

typedef enum
{
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC
} sctk_portals_control_message_type_t;

/**
 * A connection context structure represents data which are transfered at connection
 * initialization through control_message. it contains all informations for remote process
 * to create its own route and information about remote ME for bidirectional init
 */
typedef struct sctk_portals_connection_context_s
{
	sctk_portals_process_id_t from; ///< local ptl_process for remote route cration
	ptl_pt_index_t entry;           ///< local entry where ack data are waited
	ptl_match_bits_t match;         ///< local specific match_bits associated

} sctk_portals_connection_context_t;

/**
 * @brief freeing portals-specific data from ptp_message, especially preallocated slot for ME
 *
 * @param msg
 */
void sctk_portals_free ( void *msg );

/**
 * @brief Send a given ptp_message to a given process through the endpoint
 *
 * For now, this call make a blocking request when message have to be routed. As data are not still here,
 * current process should make a Get() request on data before reemit them.
 *
 * @param endpoint the route
 * @param msg the message
 */
void sctk_portals_send_put ( struct sctk_endpoint_s *endpoint, struct sctk_thread_ptp_message_s *msg);

/**
 * @brief RECV handling routing, stores incoming header for accessing it later (send_from_network call)
 *
 * @param rail current rail
 * @param task indicce, used to  unlock before inter_thread_comm calls
 * @param event event generated by Portals for reception (containing important info)
 */
void sctk_portals_recv_put (struct sctk_rail_info_s* rail, unsigned int indice, ptl_event_t* event);

/**
 * @brief When a message have been sent, source has to notify and unlock tasks waiting for completion for this message
 *
 * The message to deliver is attached as a use pointer in the event.
 *
 * @param rail
 * @param event event triggered by PtlGet from target
 */
void sctk_portals_ack_get (struct sctk_rail_info_s* rail, ptl_event_t* event);

/**
 * @brief polling function for request where local process is the initiator. When local send a request
 * the associated MD is registered and polled later. This allows to have non-blocking request on
 * Get and Put actions
 *
 * @param rail current rail
 */
void sctk_portals_poll_pending_msg_list(struct sctk_rail_info_s *rail);

/**
 * @brief Poll incoming events in a event queue for the entry <b>id</b>
 *
 * @param rail current rail
 * @param id id where event queue have to be polled
 *
 * @return 0 if at least one message have been polled, 1 otherwise
 */
int sctk_portals_poll_one_queue(struct sctk_rail_info_s *rail, size_t id);

/**
 * @brief follows polling algorithms for current task
 *
 * Polling strategy is :
 *  - Polling task event queue
 *  - If empty, poll neighbours event queue
 *  - If empty, poll the whole table
 *
 * @param rail the current rail
 * @param task_id the associated task
 *
 * @return 0 if at least one event have been triggered, 0 otherwise
 */
int sctk_portals_polling_queue_for(struct sctk_rail_info_s* rail, size_t task_id);

/**
 * sctk_portals_network_connection_from(...) wrapper for automatic static route creation
 */
void sctk_portals_connection_from(int from, int to , struct sctk_rail_info_s *rail);

/**
 * sctk_portals_network_connection_to(...) wrapper for automatic static route creation
 */
void sctk_portals_connection_to(int from, int to , struct sctk_rail_info_s *rail);

/**
 * @brief on demand route creation, wrapper around sctk_portals_network_connection_from(...) for dynamic route creation
 * @param rail current rail
 * @param dest_process process we have to create a route
 */
void sctk_portals_on_demand_connection_handler( struct sctk_rail_info_s *rail, int dest_process );

/**
 * @brief control message handler, when a control message is incoming, this routine is called if
 * the control message have been sent from portals rail at initiator
 * @param rail curretn rail
 * @param src_process process who requests the control message
 * @param source_rank and its source rank (can be ANY_SOURCE)
 * @param subtype
 * @param param
 * @param data data from control message
 * @param size data size
 */
void sctk_portals_control_message_handler( struct sctk_rail_info_s *rail, int src_process, int source_rank, char subtype, char param, void * data, size_t size);

/**
 * @brief Portals rail initialization routine
 * @param current rail to init
 */
void sctk_network_init_portals_all ( struct sctk_rail_info_s *rail );
#endif
#ifdef __cplusplus
}
#endif // MPC_USE_PORTALS
#endif // end .h file

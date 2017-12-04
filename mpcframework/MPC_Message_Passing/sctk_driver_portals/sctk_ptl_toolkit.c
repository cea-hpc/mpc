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

#include "sctk_route.h"
#include "sctk_net_tools.h"
#include "sctk_io_helper.h" /* for MAX_STRING_SIZE */
#include "sctk_ptl_types.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_eager.h"
#include "sctk_ptl_rdma.h"
#include "sctk_ptl_cm.h"
#include "sctk_ptl_toolkit.h"

/* global shortcut */
sctk_ptl_id_t* ranks_ids_map = NULL;

/**
 * Add a route to the multirail, for Portals use.
 * \param[in] dest the remote process rank
 * \param[in] id the associated Portals-related ID
 * \param[in] origin route type
 * \param[in] state route initial state
 */
void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state)
{
	sctk_endpoint_t * route;

	route = sctk_malloc(sizeof(sctk_endpoint_t));
	assert(route);

	sctk_endpoint_init(route, dest, rail, origin);
	sctk_endpoint_set_state(route, state);

	route->data.ptl.dest = id;
	ranks_ids_map[dest] = id;

	if(origin == ROUTE_ORIGIN_STATIC)
	{
		sctk_rail_add_static_route (  rail, dest, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, dest, route );
	}
}

/**
 * Routine called when the current process posted a RECV call.
 * As we are in a single-rail configuration, we can optimize msg incomings.
 * \param[in] msg the local request
 * \param[in] rail the Portals rail
 */
void sctk_ptl_notify_recv(sctk_thread_ptp_message_t* msg, sctk_rail_info_t* rail)
{
	sctk_ptl_rail_info_t* srail     = &rail->network.ptl;
	void* start                     = NULL;
	size_t size                     = 0;
	sctk_ptl_matchbits_t match      = SCTK_PTL_MATCH_INIT;
	sctk_ptl_matchbits_t ign        = SCTK_PTL_MATCH_INIT;
	int flags                       = 0;
	sctk_ptl_pte_t* pte             = NULL;
	sctk_ptl_local_data_t* user_ptr = NULL;
	sctk_ptl_id_t remote            = SCTK_PTL_ANY_PROCESS;

	/* pre-actions */
	sctk_assert(msg);
	sctk_assert(rail);
	sctk_assert(srail);
	sctk_assert(SCTK_PTL_PTE_EXIST(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg)));

	if(SCTK_MSG_SIZE(msg) <= srail->eager_limit)
	{
		sctk_ptl_eager_notify_recv(msg, srail);
	}
	else
	{
		sctk_ptl_rdv_notify_recv(msg, srail);
	}
}

/**
 * Send a message through Portals API.
 * This is the main entry point for sending message, when requested by the low_level_comm
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
void sctk_ptl_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	int process_rank            = sctk_get_process_rank();
	sctk_ptl_rail_info_t* srail = &endpoint->rail->network.ptl;

	sctk_assert(SCTK_PTL_PTE_EXIST(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg)));

	/* specific cases: control messages */
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		sctk_ptl_cm_send_message(msg, endpoint);
		return;
	}

	/* if the message is lower than the fixed boundary, send it as an eager */
	if(SCTK_MSG_SIZE(msg) <= srail->eager_limit)
	{
		sctk_ptl_eager_send_message(msg, endpoint);
	}
	else
	{
		sctk_ptl_rdv_send_message(msg, endpoint);
	}
}

/**
 * Routine called periodically to notify upper-layer from incoming messages.
 * \param[in] rail the Portals rail
 * \param[in] threshold max number of messages to poll, if any
 */
void sctk_ptl_eqs_poll(sctk_rail_info_t* rail, int threshold)
{
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_local_data_t* user_ptr;
	sctk_ptl_pte_t* cur_pte;

	size_t i = 0, size = srail->nb_entries;
	int ret, max = 0;

	/* at least, try each entry once */
	threshold = (size > threshold) ? size : threshold;
	sctk_assert(threshold > 0);
	while(max++ < threshold)
	{
		do
		{
			cur_pte = MPCHT_get(&srail->pt_table, ((i++)%size));
		}
		while(!cur_pte && i < size);
		if(!cur_pte)
		{
			/* no valid PTE found (after the last 'i'): stop the polling */
			break;
		}

		ret      = sctk_ptl_eq_poll_me(srail, cur_pte , &ev);
		user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;

		if(ret == PTL_OK)
		{
			sctk_info("PORTALS: EQS EVENT '%s' idx=%d, type=%x, prot=%x, match=%s from %s, sz=%llu", sctk_ptl_event_decode(ev), ev.pt_index, user_ptr->type, user_ptr->prot, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), SCTK_PTL_STR_LIST(((sctk_ptl_local_data_t*)ev.user_ptr)->list), ev.mlength);

	
			/* we only consider Portals-succeded events */
			if(ev.ni_fail_type != PTL_NI_OK) 
				sctk_fatal("ME: Failed event %s: %d", sctk_ptl_event_decode(ev), ev.ni_fail_type);
		
			/* if the request consumed an unexpected slot, append a new one */
			if(user_ptr->list == SCTK_PTL_OVERFLOW_LIST)
			{
				sctk_ptl_pte_t fake = (sctk_ptl_pte_t){.idx = ev.pt_index};
				sctk_ptl_me_feed(srail,  &fake,  srail->eager_limit, 1, SCTK_PTL_OVERFLOW_LIST, SCTK_PTL_TYPE_STD, SCTK_PTL_PROT_NONE);
				sctk_free(user_ptr);
				continue;
			}
			
			switch((int)user_ptr->type) /* normal message */
			{
				case SCTK_PTL_TYPE_STD:
					switch((int)user_ptr->prot)
					{
						case SCTK_PTL_PROT_EAGER:
							sctk_ptl_eager_event_me(rail, ev); break;
						case SCTK_PTL_PROT_RDV:
							sctk_ptl_rdv_event_me(rail, ev); break;
						default:
							not_reachable();
					}
					break;
				case SCTK_PTL_TYPE_RDMA:
					sctk_ptl_rdma_event_me(rail, ev);
					break;
				case SCTK_PTL_TYPE_CM:
					sctk_ptl_cm_event_me(rail, ev);
					break;
				case SCTK_PTL_TYPE_RECOVERY:
					not_implemented();
					break;
				default:
					not_reachable();
			}
		}
	}
}

/**
 * Make locally-initiated request progress.
 * Here, only MD-specific events are processed.
 * \param[in] arg the Portals rail, to cast before use.
 */
void sctk_ptl_mds_poll(sctk_rail_info_t* rail, int threshold)
{
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_event_t ev;
	sctk_ptl_local_data_t* user_ptr;
	int ret, max = 0;
	while(max++ < threshold)
	{
		ret = sctk_ptl_eq_poll_md(srail, &ev);

		if(ret == PTL_OK)
		{
			user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
			sctk_assert(user_ptr != NULL);
			sctk_info("PORTALS: MDS EVENT '%s' from %s, type=%d, prot=%d",sctk_ptl_event_decode(ev), SCTK_PTL_STR_LIST(ev.ptl_list), user_ptr->type, user_ptr->prot);
			/* we only care about Portals-sucess events */
			if(ev.ni_fail_type != PTL_NI_OK)
				sctk_fatal("MD: Failed event %s: %d", sctk_ptl_event_decode(ev), ev.ni_fail_type);


			switch((int)user_ptr->type)
			{
				case SCTK_PTL_TYPE_STD:
					/*
					 * Dirty way to distiguish RDV vs EAGER.
					 * Remember that full event at initiator-side are really limited.
					 * (Only type, list, length, user_ptr & fail_type)
					 *
					 * As we know that 
					 */
					switch((int)user_ptr->prot)
					{
						case SCTK_PTL_PROT_EAGER:
							sctk_ptl_eager_event_md(rail, ev); break;
						case SCTK_PTL_PROT_RDV:
							sctk_ptl_rdv_event_md(rail, ev); break;
						default:
							not_reachable();
					}
					break;
				case SCTK_PTL_TYPE_RDMA:
					sctk_ptl_rdma_event_md(rail, ev);
					break;
				case SCTK_PTL_TYPE_CM:
					sctk_ptl_cm_event_md(rail, ev);
					break;
				case SCTK_PTL_TYPE_RECOVERY:
					not_implemented();
					break;
				default:
					not_reachable();
			}
		}
	}
}

/**
 * Create the initial Portals topology: the ring.
 * \param[in] rail the just-initialized Portals rail
 */
void sctk_ptl_create_ring ( sctk_rail_info_t *rail )
{
	int right_rank, left_rank, tmp_ret;
	sctk_ptl_id_t right_id, left_id;
	char right_rank_connection_infos[MAX_STRING_SIZE];
	char left_rank_connection_infos[MAX_STRING_SIZE];
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;

	/** Portals initialization : Ring topology.
	 * 1. bind to right process through PMI
	 * 2. if process number > 2 create a route to the process to the left
	 * => Bidirectional ring
	 */
	
	/* serialize the id_t to a string, compliant with PMI handling */
	srail->connection_infos_size = sctk_ptl_data_serialize(
			&srail->id,              /* the process it to serialize */
			sizeof (sctk_ptl_id_t),  /* size of the Portals ID struct */
			srail->connection_infos, /* the string to store the serialization */
			MAX_STRING_SIZE          /* max allowed string's size */
	);
	assert(srail->connection_infos_size > 0);

	/* register the serialized id into the PMI */
	tmp_ret = sctk_pmi_put_connection_info (
			srail->connection_infos,      /* the string to publish */
			srail->connection_infos_size, /* string size */
			rail->rail_number             /* rail ID: PMI tag */
	);
	assert(tmp_ret == 0);
	ranks_ids_map[sctk_get_process_rank()] = srail->id;

	/* what are my neighbour ranks ? */
	right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
	left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;

	/* wait for each process to register its own serialized ID */
	sctk_pmi_barrier();

	/* retrieve the right neighbour id struct */
	tmp_ret = sctk_pmi_get_connection_info (
			right_rank_connection_infos, /* the recv buffer */
			MAX_STRING_SIZE,             /* the recv buffer max size */
			rail->rail_number,           /* rail IB: PMI tag */
			right_rank                   /* which process we are targeting */
	);
	assert(tmp_ret == 0);
	
	/* de-serialize the string to retrive the real ptl_id_t */
	sctk_ptl_data_deserialize(
			right_rank_connection_infos, /* the buffer containing raw data */
			&right_id,                   /* the target struct */
			sizeof ( right_id )          /* target struct size */
	);

	//if we need an initialization to the left process (bidirectional ring)
	if(sctk_process_number > 2)
	{

		/* retrieve the left neighbour id struct */
		tmp_ret = sctk_pmi_get_connection_info (
				left_rank_connection_infos, /* the recv buffer */
				MAX_STRING_SIZE,             /* the recv buffer max size */
				rail->rail_number,           /* rail IB: PMI tag */
				left_rank                   /* which process we are targeting */
				);
		assert(tmp_ret == 0);

		/* de-serialize the string to retrive the real ptl_id_t */
		sctk_ptl_data_deserialize(
				left_rank_connection_infos, /* the buffer containing raw data */
				&left_id,                   /* the target struct */
				sizeof ( left_id )          /* target struct size */
				);
	}

	/* add routes */
	sctk_ptl_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
	if ( sctk_process_number > 2 )
	{
		sctk_ptl_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
	}

	//Wait for all processes to complete the ring topology init */
	sctk_pmi_barrier();
}

/** 
 * Retrieve the ptl_process_id object, associated with a given MPC process rank.
 * \param[in] rail the Portals rail
 * \param[in] dest the MPC process rank
 * \return the Portals process id
 */
sctk_ptl_id_t sctk_ptl_map_id(sctk_rail_info_t* rail, int dest)
{
	int tmp_ret;
	char connection_infos[MAX_STRING_SIZE];
	sctk_ptl_id_t id = SCTK_PTL_ANY_PROCESS;

	/* retrieve the right neighbour id struct */
	tmp_ret = sctk_pmi_get_connection_info (
			connection_infos,  /* the recv buffer */
			MAX_STRING_SIZE,   /* the recv buffer max size */
			rail->rail_number, /* rail IB: PMI tag */
			dest               /* which process we are targeting */
	);

	assert(tmp_ret == 0);
	
	sctk_ptl_data_deserialize(
			connection_infos, /* the buffer containing raw data */
			&id,               /* the target struct */
			sizeof (id )      /* target struct size */
	);

	return id;
}

void sctk_ptl_comm_register(sctk_ptl_rail_info_t* srail, int comm_idx, size_t comm_size)
{
	if(!SCTK_PTL_PTE_EXIST(srail->pt_table, comm_idx))
	{
		sctk_ptl_pte_t* new_entry = sctk_malloc(sizeof(sctk_ptl_pte_t));
		sctk_ptl_pte_create(srail, new_entry, comm_idx + SCTK_PTL_PTE_HIDDEN);
	}
}

/**
 * Initialize the Portals API and main structs.
 * \param[in,out] rail the Portals rail
 */
void sctk_ptl_init_interface(sctk_rail_info_t* rail)
{
	size_t cut, eager_limit, min_comms;

	/* get back the Portals config */
	cut         = rail->runtime_config_driver_config->driver.value.portals.block_cut;
	eager_limit = rail->runtime_config_driver_config->driver.value.portals.eager_limit;
	min_comms   = rail->runtime_config_driver_config->driver.value.portals.min_comms;

	if(cut < eager_limit)
	{
		sctk_warning("PORTALS: eager are larger than allowed memory region size ! Resize eager limits.");
		eager_limit = cut;
	}

	/* init low-level driver */
	rail->network.ptl             = sctk_ptl_hardware_init();
	rail->network.ptl.eager_limit = eager_limit;
	
	sctk_ptl_software_init( &rail->network.ptl, min_comms);
	
	sctk_assert(eager_limit == rail->network.ptl.eager_limit);
	rail->network.ptl.max_mr      = rail->network.ptl.max_limits.max_msg_size;
	rail->network.ptl.cutoff      = cut;
	/*rail->network.ptl.max_mr = (1024 * 1024 * 8);*/

	if(!ranks_ids_map)
	{
		ranks_ids_map = sctk_calloc(sctk_get_process_number(), sizeof(sctk_ptl_id_t));
	}
}

/**
 * Close the Portals API.
 * \param[in] rail the rail to close
 */
void sctk_ptl_fini_interface(sctk_rail_info_t* rail)
{
}

void sctk_ptl_free_memory(void* msg)
{
}

#endif

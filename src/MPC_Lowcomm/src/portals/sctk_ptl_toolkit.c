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

#include "endpoint.h"
#include "sctk_net_tools.h"
#include "mpc_common_helper.h" /* for MPC_COMMON_MAX_STRING_SIZE */
#include "sctk_ptl_types.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_eager.h"
#include "sctk_ptl_rdma.h"
#include "sctk_ptl_cm.h"
#include "sctk_ptl_toolkit.h"
#include "sctk_ptl_offcoll.h"

#include <mpc_common_datastructure.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>

/** global shortcut, where each cell maps to the portals process object */
sctk_ptl_id_t* ranks_ids_map = NULL;

/**
 * Add a route to the multirail, for Portals use.
 * \param[in] dest the remote process rank
 * \param[in] id the associated Portals-related ID
 * \param[in] origin route type
 * \param[in] state route initial state
 */
void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, _mpc_lowcomm_endpoint_type_t origin, _mpc_lowcomm_endpoint_state_t state)
{
	_mpc_lowcomm_endpoint_t * route;

	route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
	assert(route);

	_mpc_lowcomm_endpoint_init(route, dest, rail, origin);
	_mpc_lowcomm_endpoint_set_state(route, state);

	route->data.ptl.dest = id;
	ranks_ids_map[dest] = id;

	if(origin == _MPC_LOWCOMM_ENDPOINT_STATIC)
	{
		sctk_rail_add_static_route (  rail, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, route );
	}
}

/**
 * Routine called when the current process posted a RECV call.
 * As we are in a single-rail configuration, we can optimize msg incomings.
 * \param[in] msg the local request
 * \param[in] rail the Portals rail
 */
void sctk_ptl_notify_recv(mpc_lowcomm_ptp_message_t* msg, sctk_rail_info_t* rail)
{
	sctk_ptl_rail_info_t* srail     = &rail->network.ptl;
	
	/* pre-actions */
	assert(msg);
	assert(rail);
	assert(srail);
	assert(SCTK_PTL_PTE_EXIST(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg)));
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
void sctk_ptl_send_message(mpc_lowcomm_ptp_message_t* msg, _mpc_lowcomm_endpoint_t* endpoint)
{
	sctk_ptl_rail_info_t* srail = &endpoint->rail->network.ptl;

	assert(SCTK_PTL_PTE_EXIST(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg)));

	/* specific cases: control messages */
	if(_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg)))
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
void sctk_ptl_eqs_poll(sctk_rail_info_t* rail, size_t threshold)
{
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_local_data_t* user_ptr;
	sctk_ptl_pte_t* cur_pte;

	size_t i = 0, size = srail->nb_entries;
	int ret;
	size_t max = 0;

	/* at least, try each entry once */
	threshold = (size > threshold) ? size : threshold;
	assert(threshold > 0);
	while(max++ < threshold)
	{
		do
		{
			cur_pte = mpc_common_hashtable_get(&srail->pt_table, ((i++)%size));
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
			mpc_common_nodebug("PORTALS: EQS EVENT '%s' idx=%d, from %s, type=%s, prot=%s, match=%s,  sz=%llu, user=%p, start=%p", sctk_ptl_event_decode(ev), ev.pt_index, SCTK_PTL_STR_LIST(((sctk_ptl_local_data_t*)ev.user_ptr)->list), SCTK_PTL_STR_TYPE(user_ptr->type), SCTK_PTL_STR_PROT(user_ptr->prot), __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), ev.mlength, ev.user_ptr, ev.start);
			
			/* if the event is related to a probe request */
			if(ev.type == PTL_EVENT_SEARCH)
			{
				/* sanity check */
				assert(user_ptr->type == SCTK_PTL_TYPE_PROBE);
				int found = (ev.ni_fail_type == PTL_NI_OK);
				if(found)
				{
					/* when the RDV protocol is used, the initial PUT contains the total message size. The overflow event contains the imm_data of this PUT */
					user_ptr->probe.size = (((sctk_ptl_imm_data_t)ev.hdr_data).std.putsz) ? *((size_t*)ev.start) : ev.mlength;
					user_ptr->probe.rank = ((sctk_ptl_matchbits_t)ev.match_bits).data.rank;
					user_ptr->probe.tag  = ((sctk_ptl_matchbits_t)ev.match_bits).data.tag;
				}
				/* unlock the polling (if not done by the same thread) */
				OPA_store_int(&user_ptr->probe.found, found );
				continue;
			}

			/* we only consider Portals-succeded events */
			if(ev.ni_fail_type != PTL_NI_OK)
			{
				mpc_common_debug_fatal("PORTALS: FAILED EQS EVENT '%s' idx=%d, from %s, type=%s, prot=%s, match=%s,  sz=%llu, user=%p err='%s'", sctk_ptl_event_decode(ev), ev.pt_index, SCTK_PTL_STR_LIST(((sctk_ptl_local_data_t*)ev.user_ptr)->list), SCTK_PTL_STR_TYPE(user_ptr->type), SCTK_PTL_STR_PROT(user_ptr->prot), __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), ev.mlength, ev.user_ptr, sctk_ptl_ni_fail_decode(ev));
			}
		
			/* if the request consumed an unexpected slot, append a new one */
			if(user_ptr->list == SCTK_PTL_OVERFLOW_LIST)
			{
				sctk_ptl_me_feed(srail,  cur_pte,  srail->eager_limit, 1, SCTK_PTL_OVERFLOW_LIST, SCTK_PTL_TYPE_STD, SCTK_PTL_PROT_NONE);
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
							/*not_reachable();*/
							break;
					}
					break;
				case SCTK_PTL_TYPE_PROBE:
					break;
				case SCTK_PTL_TYPE_OFFCOLL:
					sctk_ptl_offcoll_event_me(rail, ev);
					break;
				case SCTK_PTL_TYPE_RDMA:
					sctk_ptl_rdma_event_me(rail, ev);
					break;
				case SCTK_PTL_TYPE_CM:
					sctk_ptl_cm_event_me(rail, ev);
					break;
				case SCTK_PTL_TYPE_RECOVERY:
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
void sctk_ptl_mds_poll(sctk_rail_info_t* rail, size_t threshold)
{
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_event_t ev;
	sctk_ptl_local_data_t* user_ptr;
	int ret;
	size_t max = 0;
	while(max++ < threshold)
	{
		ret = sctk_ptl_eq_poll_md(srail, &ev);

		if(ret == PTL_OK)
		{
			user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
			assert(user_ptr != NULL);
			mpc_common_nodebug("PORTALS: MDS EVENT '%s' from %s, type=%s, prot=%s",sctk_ptl_event_decode(ev), SCTK_PTL_STR_LIST(ev.ptl_list), SCTK_PTL_STR_TYPE(user_ptr->type), SCTK_PTL_STR_PROT(user_ptr->prot));
			/* we only care about Portals-sucess events */
			if(ev.ni_fail_type != PTL_NI_OK)
			{
				mpc_common_debug_fatal("PORTALS: FAILED MDS EVENT '%s' from %s, type=%s, prot=%s, match=%s err='%s'",sctk_ptl_event_decode(ev), SCTK_PTL_STR_LIST(ev.ptl_list), SCTK_PTL_STR_TYPE(user_ptr->type), SCTK_PTL_STR_PROT(user_ptr->prot), __sctk_ptl_match_str(sctk_malloc(32), 32, user_ptr->match.raw), sctk_ptl_ni_fail_decode(ev));
			}

			assert(user_ptr->type != SCTK_PTL_TYPE_NONE);


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
					assert(user_ptr->prot != SCTK_PTL_PROT_NONE);
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
				case SCTK_PTL_TYPE_OFFCOLL:
					assert(user_ptr->prot != SCTK_PTL_PROT_NONE);
					sctk_ptl_offcoll_event_md(rail, ev);
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
	char right_rank_connection_infos[MPC_COMMON_MAX_STRING_SIZE];
	char left_rank_connection_infos[MPC_COMMON_MAX_STRING_SIZE];
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
			MPC_COMMON_MAX_STRING_SIZE          /* max allowed string's size */
	);
	assert(srail->connection_infos_size > 0);

	/* register the serialized id into the PMI */
	tmp_ret = mpc_launch_pmi_put_as_rank (
			srail->connection_infos,      /* the string to publish */
			rail->rail_number,             /* rail ID: PMI tag */
			0 /* Not local */
	);
	assert(tmp_ret == 0);
	ranks_ids_map[mpc_common_get_process_rank()] = srail->id;

	/* what are my neighbour ranks ? */
	right_rank = ( mpc_common_get_process_rank() + 1 ) % mpc_common_get_process_count();
	left_rank = ( mpc_common_get_process_rank() + mpc_common_get_process_count() - 1 ) % mpc_common_get_process_count();

	/* wait for each process to register its own serialized ID */
 mpc_launch_pmi_barrier();

	/* retrieve the right neighbour id struct */
	tmp_ret = mpc_launch_pmi_get_as_rank (
			right_rank_connection_infos, /* the recv buffer */
			MPC_COMMON_MAX_STRING_SIZE,             /* the recv buffer max size */
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
	if(mpc_common_get_process_count() > 2)
	{

		/* retrieve the left neighbour id struct */
		tmp_ret = mpc_launch_pmi_get_as_rank (
				left_rank_connection_infos, /* the recv buffer */
				MPC_COMMON_MAX_STRING_SIZE,             /* the recv buffer max size */
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
	sctk_ptl_add_route (right_rank, right_id, rail, _MPC_LOWCOMM_ENDPOINT_STATIC, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
	if ( mpc_common_get_process_count() > 2 )
	{
		sctk_ptl_add_route (left_rank, left_id, rail, _MPC_LOWCOMM_ENDPOINT_STATIC, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
	}

	//Wait for all processes to complete the ring topology init */
 mpc_launch_pmi_barrier();
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
	char connection_infos[MPC_COMMON_MAX_STRING_SIZE];

	if(SCTK_PTL_IS_ANY_PROCESS(ranks_ids_map[dest]) )
	{
		/* retrieve the right neighbour id struct */
		tmp_ret = mpc_launch_pmi_get_as_rank (
				connection_infos,  /* the recv buffer */
				MPC_COMMON_MAX_STRING_SIZE,   /* the recv buffer max size */
				rail->rail_number, /* rail IB: PMI tag */
				dest               /* which process we are targeting */
				);

		assert(tmp_ret == 0);

		sctk_ptl_data_deserialize(
				connection_infos, /* the buffer containing raw data */
				ranks_ids_map + dest, /* the target struct */
				sizeof (sctk_ptl_id_t)      /* target struct size */
				);
	}
	assert(!SCTK_PTL_IS_ANY_PROCESS(ranks_ids_map[dest]));
	return ranks_ids_map[dest];
}

/**
 * Notify the driver that a new communicator has been created.
 * Will trigger the creation of new Portals entry.
 * \param[in] srail the Portals rail
 * \param[in] com_idx the communicator ID
 * \param[in] comm_size the number of processes in that communicator
 */
void sctk_ptl_comm_register(sctk_ptl_rail_info_t* srail, int comm_idx, size_t comm_size)
{
	UNUSED(comm_size);
	if(!SCTK_PTL_PTE_EXIST(srail->pt_table, comm_idx))
	{
		sctk_ptl_pte_t* new_entry = sctk_malloc(sizeof(sctk_ptl_pte_t));
		sctk_ptl_pte_create(srail, new_entry, comm_idx + SCTK_PTL_PTE_HIDDEN);
	}
}

/**
 * @brief probe a message described by its header.
 * 
 * This function will block until a request is notified from the Portals interface.
 * 
 * @param rail the Portals rail
 * @param hdr the message header built from upstream
 * @param probe_level searching level, from Portals semantics (SEARCH_ONLY | SEARCH_DELETE)
 * @return 1 if a matching message is found, zero otherwise
 */
int sctk_ptl_pending_me_probe(sctk_rail_info_t* rail, mpc_lowcomm_ptp_message_header_t* hdr, int probe_level)
{
	uint32_t comm = hdr->communicator_id;
	int rank = hdr->source_task;
	int tag = hdr->message_tag;
	int ret = -1;

	mpc_common_nodebug("PROBE: c%d r%d t%d", comm, rank, tag);
	
	sctk_ptl_rail_info_t* prail = &rail->network.ptl;
	sctk_ptl_pte_t* pte = mpc_common_hashtable_get(&prail->pt_table, (int)((comm + SCTK_PTL_PTE_HIDDEN_NB) % prail->nb_entries));
	sctk_ptl_matchbits_t match, ign;

	/* build a temporary ME to match caller criteria */
	match.data = (sctk_ptl_std_content_t)
	{
		.rank = rank,
		.tag = tag
	};

	/* almost sure the rank could be translated to the corresponding PID/NID and be used
	 * as an additional criteria. This is not mandatory as the rank is already part of the 
	 * match_bits. The @see ranks_ids_map table could be used. However, the struct could not 
	 * be defined if no route have been created to the remote process yet.
	 */
	ign.data = (sctk_ptl_std_content_t)
	{
		.rank = (rank == MPC_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK,
		.tag = (tag == MPC_ANY_TAG) ? SCTK_PTL_IGN_TAG : SCTK_PTL_MATCH_TAG,
		.type = hdr->message_type.type,
		.uid = SCTK_PTL_IGN_UID
	};
	
	/* create the ME to match. The 'start' field does not need to be valid,
	 * in case of a match, the event will return the address of the overflow buffer.
	 * The size could be used if, one day, the NO_TRUNCATE attribute is set,
	 */
	sctk_ptl_local_data_t* data = sctk_ptl_me_create(NULL, sizeof(size_t), SCTK_PTL_ANY_PROCESS, match, ign, SCTK_PTL_ME_PUT_FLAGS);

	/* -1 means "request submitted and not completed yet" */
	OPA_store_int(&data->probe.found, -1);
	sctk_ptl_me_emit_probe(prail, pte, data, probe_level);

	/* Active polling */
	while((ret = OPA_load_int(&data->probe.found)) == -1)
	{
		sctk_ptl_eqs_poll(rail, 1);
	}
	if(ret)
	{
		hdr->source_task = data->probe.rank;
		hdr->msg_size = data->probe.size;
		hdr->message_tag = data->probe.tag;
	}
	sctk_free(data);
	return ret;
}

/**
 * Initialize the Portals API and main structs.
 * \param[in,out] rail the Portals rail
 */
void sctk_ptl_init_interface(sctk_rail_info_t* rail)
{
	size_t cut, eager_limit, min_comms, offloading;

	/* get back the Portals config */
	cut         = rail->runtime_config_driver_config->driver.value.portals.block_cut;
	eager_limit = rail->runtime_config_driver_config->driver.value.portals.eager_limit;
	min_comms   = rail->runtime_config_driver_config->driver.value.portals.min_comms;
	offloading  = SCTK_PTL_OFFLOAD_NONE_FLAG;
	
	/* avoid truncating size_t payload in RDV protocols */
	if(eager_limit < sizeof(size_t))
		eager_limit = sizeof(size_t);

	if(rail->runtime_config_driver_config->driver.value.portals.offloading.ondemand)
		offloading |= SCTK_PTL_OFFLOAD_OD_FLAG;
	if(rail->runtime_config_driver_config->driver.value.portals.offloading.collectives)
		offloading |= SCTK_PTL_OFFLOAD_COLL_FLAG;

	if(cut < eager_limit)
	{
		mpc_common_debug_warning("PORTALS: eager are larger than allowed memory region size ! Resize eager limits.");
		eager_limit = cut;
	}

	/* init low-level driver */
	rail->network.ptl                 = sctk_ptl_hardware_init();
	rail->network.ptl.eager_limit     = eager_limit;
	rail->network.ptl.cutoff          = cut;
	rail->network.ptl.max_mr          = rail->network.ptl.max_limits.max_msg_size;
	rail->network.ptl.offload_support = offloading;
	
	sctk_ptl_software_init( &rail->network.ptl, min_comms);
	assert(eager_limit == rail->network.ptl.eager_limit);

	if(!ranks_ids_map)
	{
		ranks_ids_map = sctk_malloc(mpc_common_get_process_count() * sizeof(sctk_ptl_id_t));
		int i;
		for(i = 0; i < mpc_common_get_process_count(); i++)
		{
			ranks_ids_map[i] = SCTK_PTL_ANY_PROCESS;
		}
	}
}

/**
 * Close the Portals API.
 * \param[in] rail the rail to close
 */
void sctk_ptl_fini_interface(sctk_rail_info_t* rail)
{
	UNUSED(rail);
}
